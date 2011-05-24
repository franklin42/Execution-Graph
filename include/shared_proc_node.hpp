// (C) Copyright Jonathan Franklin 2011.
// Use, modification and distribution are subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt).

#if ! defined(SHARED_PROC_NODE_HPP)
#define SHARED_PROC_NODE_HPP

#include "proc_node.hpp"

#include <boost/thread/condition_variable.hpp>
#include <boost/thread/recursive_mutex.hpp>

//==============================================================================
/** This ... object provides a standard, generic interface for
 * passing jobs to a "processor" node.  All work is processed in FIFO order. 
 * All access to the inputs queue (m_inputs) is synchronized.
 *
 * We use the Curiously Recurring Template Pattern (CRTP) to provide a
 * higher-performance, pseudo-polymorphic interface for dispatching the work to
 * derived classes.  This requires derived classes to implement
 * the following dispatch interface:
 *   typedef <JOB TYPE> Job_t;
 *   void dispatch(const Job_t & work);
 */
template<typename Derived_T, typename Input_T, typename Output_T>
class shared_proc_node : public proc_node<Derived_T, Input_T, Output_T>
{
public:
  /** @param[in] maxQueueSize The maximum size of the work queue.  0 indicates
   *    unbounded size.  */
  explicit shared_proc_node(size_t maxQueueSize = 0);

  ~shared_proc_node() {}

  /** Enqueue the N-th input component.
   * TODO: Add enable_if code to turn this off when Input_T is not a fusion
   * sequence type.
   * @param[in] component The input component to be processed.
   * @return true if the input component was successfully enqueued for
   *   processing, false otherwise.  false may indicate that the queue is
   *   currently full.
   */
  template<int N, typename InputComponent_T>
  bool enqueue(const InputComponent_T & component, bool block = true)
  {
    // TRICKY: The input queue is a shared resource, so synchronize access.
    mutex_t::scoped_lock l(m_mutex);

    if (block)
    {
      // Block if the queue is full.
      while (this->maxQueueSize() && (this->queueSize() >=this->maxQueueSize()))
        m_cond.wait(l);
    }

    // This will return false if the queue is full.
    bool const enqueued = proc_node::enqueue<N>(component);

    // Notify any waiting threads that there is something on the queue.
    if (enqueued)
      m_cond.notify_one();

    return enqueued;
  }

  /** @return The maximum allowed queue size. */
  size_t maxQueueSize() const;

  /** @param[in] size The maximum allowed queue size to set. */
  void maxQueueSize(size_t size);

  /** @return The current number of inputs in the queue. */
  size_t queueSize() const;

  bool is_ready() const;

  Output_T visit();

protected:
  typedef boost::recursive_mutex mutex_t;
  mutex_t & mutex()       { return m_mutex; }
  mutex_t & mutex() const { return m_mutex; }
  typedef boost::condition_variable_any cond_t;
  cond_t & cond() { return m_cond; }

private:
  /** Mutex for synchronizing access to the remove and inputs queue. */
  mutable mutex_t m_mutex;
  /** Queue empty/full condition. */
  cond_t m_cond;

};


//==============================================================================
template<typename Derived_T, typename Input_T, typename Output_T>
shared_proc_node<Derived_T, Input_T, Output_T>::
shared_proc_node(size_t maxQueueSize)
:
  proc_node(maxQueueSize),
  m_mutex(),
  m_cond()
{
}

//------------------------------------------------------------------------------
template<typename Derived_T, typename Input_T, typename Output_T>
size_t shared_proc_node<Derived_T, Input_T, Output_T>::
maxQueueSize() const
{
  // TRICKY: The input queue is a shared resource, so synchronize access.
  mutex_t::scoped_lock l(m_mutex);
  return proc_node::maxQueueSize();
}

//------------------------------------------------------------------------------
template<typename Derived_T, typename Input_T, typename Output_T>
void shared_proc_node<Derived_T, Input_T, Output_T>::
maxQueueSize(size_t size)
{
  // TRICKY: The input queue is a shared resource, so synchronize access.
  mutex_t::scoped_lock l(m_mutex);
  proc_node::maxQueueSize(size);
}

//------------------------------------------------------------------------------
template<typename Derived_T, typename Input_T, typename Output_T>
size_t shared_proc_node<Derived_T, Input_T, Output_T>::queueSize() const
{
  // TRICKY: The input queue is a shared resource, so synchronize access.
  mutex_t::scoped_lock l(m_mutex);
  return proc_node::queueSize();
}

//------------------------------------------------------------------------------
template<typename Derived_T, typename Input_T, typename Output_T>
bool shared_proc_node<Derived_T, Input_T, Output_T>::is_ready() const
{
  // TRICKY: The input queue is a shared resource, so synchronize access.
  mutex_t::scoped_lock l(m_mutex);
  return proc_node::is_ready();
}

//------------------------------------------------------------------------------
template<typename Derived_T, typename Input_T, typename Output_T>
Output_T shared_proc_node<Derived_T, Input_T, Output_T>::visit()
{
  // TRICKY: The inputs queue is a shared resource, so synchronize access.
  mutex_t::scoped_lock l(m_mutex);
  return proc_node::visit();
}

#endif
