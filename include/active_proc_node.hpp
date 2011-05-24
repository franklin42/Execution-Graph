// (C) Copyright Jonathan Franklin 2011.
// Use, modification and distribution are subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt).

#if ! defined(ACTIVE_PROC_NODE_HPP)
#define ACTIVE_PROC_NODE_HPP

#include "shared_proc_node.hpp"

#include <boost/thread.hpp>

//==============================================================================
/** This ... object provides a built-in thread of execution for ...
 * TRICKY: This object is *not* polymorphic!
 */
template<typename Derived_T, typename Input_T, typename Output_T>
class active_proc_node : public shared_proc_node<Derived_T, Input_T, Output_T>
{
public:
  /** @param[in] maxQueueSize The maximum size of the work queue.  0 indicates
   *    unbounded size.  */
  explicit active_proc_node(size_t maxQueueSize = 0);

  ~active_proc_node() {}

  /** Start the background thread.
   * @return true if a new thread was started, false if there was already a
   *   thread running, or the thread failed to start. */
  bool startup();

  /** Shut down and optionally join the worker thread.
   * NOTE: This function will attempt to interrupt (terminate) the running
   * thread gracefully.
   * TRICKY: If this object goes out of scope before the thread actually
   * starts running, the thread interrupt may fail. This needs to be handled
   * by the thread_run() method in derived objects.
   * @param[in] block If true, then block the calling thread until the derived
   *   class's thread function (thread_run()) exits.
   * @param[in] timeout A value in milliseconds that dictates how long to wait
   *   for the thread to end. Default is 0 which means wait forever.
   */
  void shutdown(bool block = true, unsigned timeout = 0);

  /** Join the worker thread.
   * NOTE: This function will block until the derived class's thread function
   * (thread_run()) exits.
   */
  void join();

  /** @return true indicates that the background thread is running. */
  bool running() const;

  template<size_t M, typename OutputComponent_T>
  void consumer(boost::function<bool (const OutputComponent_T &)> cb)
  {
    typedef typename boost::fusion::result_of::begin<Output_T>::type first;
    typedef boost::mpl::int_<M> M_;
    typedef typename boost::fusion::result_of::advance<first, M_ >::type comp;
    typedef typename boost::fusion::result_of::value_of<comp>::type
      OutputComponent_t;
    BOOST_MPL_ASSERT(( boost::is_same<OutputComponent_t, OutputComponent_T> ));

    // TODO: Generate storage mechanism for each consumer function type.
  }

protected:

private:
  /** Thread worker function.
   */
  void run();

  /** true indicates that startup has been called. */
  bool m_startup;
  /** true indicates the worker thread is currently running. */
  bool m_running;
  /** true indicates a shutdown has been signaled. */
  bool m_shutdown;
  /** Worker thread. */
  boost::thread m_thread;

};


//==============================================================================
template<typename Derived_T, typename Input_T, typename Output_T>
active_proc_node<Derived_T, Input_T, Output_T>::
active_proc_node(size_t maxQueueSize)
:
  shared_proc_node(maxQueueSize),
  m_startup(false),
  m_running(false),
  m_shutdown(false),
  m_thread()
{
}

//------------------------------------------------------------------------------
template<typename Derived_T, typename Input_T, typename Output_T>
bool active_proc_node<Derived_T, Input_T, Output_T>::startup()
{
  mutex_t::scoped_lock l(this->mutex());
  
  if (! m_startup)
  {
    m_startup = true;
    m_running = false;
    m_shutdown = false;
    m_thread = boost::thread(boost::bind(& active_proc_node::run, this));
    return true;
  }

  // TODO: Should we wait and try again in case caller didn't block on shutdown,
  // and is trying to startup again before the thread function has exited?
  return false;
}

//------------------------------------------------------------------------------
template<typename Derived_T, typename Input_T, typename Output_T>
void active_proc_node<Derived_T, Input_T, Output_T>::
shutdown(bool block, unsigned timeout)
{
  using boost::posix_time::microsec_clock;
  using boost::posix_time::milliseconds;
  
  m_shutdown = true;
  m_thread.interrupt();

  if (block)
  {
    if (! timeout)
      this->join();
    else
    {
      m_thread.timed_join(microsec_clock::universal_time() +
                          milliseconds(timeout));
    }
  }
}

//------------------------------------------------------------------------------
template<typename Derived_T, typename Input_T, typename Output_T>
void active_proc_node<Derived_T, Input_T, Output_T>::join()
{
  m_thread.join();
}

//------------------------------------------------------------------------------
template<typename Derived_T, typename Input_T, typename Output_T>
bool active_proc_node<Derived_T, Input_T, Output_T>::running() const
{
  mutex_t::scoped_lock l(this->mutex());
  return m_running;
}

//------------------------------------------------------------------------------
template<typename Derived_T, typename Input_T, typename Output_T>
void active_proc_node<Derived_T, Input_T, Output_T>::run()
{
  {
    mutex_t::scoped_lock l(this->mutex());
    m_running = true;
  }

  try
  {
    while (! m_shutdown)
    {
      mutex_t::scoped_lock l(this->mutex());

      // TRICKY: We can't use the swap idiom here since the input queue is
      //   filled one component at a time.

      // Block while we're not ready.
      while (! this->is_ready())
        this->cond().wait(l);

      this->visit();
      // TODO: enqueue output components w/ consumers.
    }
  }
  catch (const boost::thread_interrupted &)
  {
    // Someone interrupted the thread: it's quittin' time!
    // Don't propagate, just let run() exit, which will shutdown the thread.
  }
  catch (...)
  {
    // TODO: Report error?
    // Don't propagate, just let run() exit, which will shutdown the thread.
  }

  mutex_t::scoped_lock l(this->mutex());
  m_startup = false;
  m_running = false;
}


#endif
