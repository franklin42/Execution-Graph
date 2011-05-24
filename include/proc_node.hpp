// (C) Copyright Jonathan Franklin 2011.
// Use, modification and distribution are subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt).

#if ! defined(PROC_NODE_HPP)
#define PROC_NODE_HPP

#include <boost/array.hpp>
#include <boost/fusion/include/at_c.hpp>
#include <boost/fusion/include/mpl.hpp>
#include <boost/fusion/include/value_of.hpp>

#include <deque>

//==============================================================================
/** This ... object provides a standard, generic interface for
 * passing inputs to a "processor" node.  All work is processed in FIFO order. 
 * All access to the input queue (m_inputs) is synchronized.
 *
 * We use the Curiously Recurring Template Pattern (CRTP) to provide a
 * higher-performance, pseudo-polymorphic interface for dispatching the work to
 * derived classes.  This requires derived classes to implement
 * the following dispatch interface:
 *   typedef <JOB TYPE> Job_t;
 *   void dispatch(const Job_t & work);
 */
template<typename Derived_T, typename Input_T, typename Output_T>
class proc_node
{
public:
  /** @param[in] maxQueueSize The maximum size of the work queue.  0 indicates
   *    unbounded size.  */
  explicit proc_node(size_t maxQueueSize = 0);

  ~proc_node() {}

  /** Enqueue a complete input set to proces (in FIFO order).
   * @param[in] input The set of inputs to process.
   * @return true if the input was successfully enqueued for dispatch, false
   *   otherwise.  false may indicate that the queue is currently full.
   */
  //bool enqueue(const Input_T & input);

  /** Enqueue the N-th input component.
   * TODO: Add enable_if code to turn this off when Input_T is not a fusion
   * sequence type.
   * @param[in] component The input component to be processed.
   * @return true if the input component was successfully enqueued for
   *   processing, false otherwise.  false may indicate that the queue is
   *   currently full.
   */
  template<size_t N, typename InputComponent_T>
  bool enqueue(const InputComponent_T & component)
  {
    // Return false if the queue is full.
    if (m_maxQueueSize && (m_inputs.size() >= m_maxQueueSize))
      return false;

    typedef typename boost::fusion::result_of::begin<Input_T>::type first;
    typedef boost::mpl::int_<N> N_;
    typedef typename boost::fusion::result_of::advance<first, N_ >::type comp;
    typedef typename boost::fusion::result_of::value_of<comp>::type
      InputComponent_t;
    BOOST_MPL_ASSERT(( boost::is_same<InputComponent_t, InputComponent_T> ));

    const size_t NUM_INPUTS = m_numInputs[N];
    if (m_inputs.size() == NUM_INPUTS)
    { // Create a new input tuple.
      m_inputs.push_back(Input_T());
    }

    // Update the tuple.
    boost::fusion::at_c<N>(m_inputs[NUM_INPUTS]) = component;
    ++m_numInputs[N];

    return true;
  }

  /** @return The maximum allowed queue size. */
  size_t maxQueueSize() const { return m_maxQueueSize; }

  /** @param[in] size The maximum allowed queue size to set. */
  void maxQueueSize(size_t size) { m_maxQueueSize = size; }

  /** @return The current number of inputs in the queue. */
  size_t queueSize() const { return m_inputs.size(); }

  /** @return true if a complete set of input values is ready for processing. */
  bool is_ready() const;

  /** @return the processed output value(s). */
  Output_T visit();

protected:
  /** Work queue type requires copy construction. */
  typedef std::deque<Input_T> InputQueue_t;
  const InputQueue_t & inputQueue() const { return m_inputs; }
  InputQueue_t & inputQueue() { return m_inputs; }

private:
  /** The maximum allowed queue size. */
  size_t m_maxQueueSize;
  /** Work queue. */
  InputQueue_t m_inputs;

  /** The cardinality of the input tuple. */
  static size_t const CARDINALITY =
    boost::fusion::result_of::size<Input_T>::type::value;
  boost::array<size_t, CARDINALITY> m_numInputs;
};


//==============================================================================
template<typename Derived_T, typename Input_T, typename Output_T>
proc_node<Derived_T, Input_T, Output_T>::proc_node(size_t maxQueueSize)
:
  m_maxQueueSize(maxQueueSize),
  m_inputs(),
  m_numInputs()
{
}

#if 0
//------------------------------------------------------------------------------
template<typename Derived_T, typename Input_T, typename Output_T>
bool proc_node<Derived_T, Input_T, Output_T>::enqueue(const Input_T & input)
{
  // Return false if the queue is full.
  if (m_maxQueueSize && (m_inputs.size() >= m_maxQueueSize))
    return false;

  m_inputs.push_back(input);
  return true;
}
#endif

//------------------------------------------------------------------------------
template<typename Derived_T, typename Input_T, typename Output_T>
bool proc_node<Derived_T, Input_T, Output_T>::is_ready() const
{
  for (size_t n = 0; n < m_numInputs.size(); ++n)
  {
    if (! m_numInputs[n])
      return false;
  }

  return true;
}

//------------------------------------------------------------------------------
template<typename Derived_T, typename Input_T, typename Output_T>
Output_T proc_node<Derived_T, Input_T, Output_T>::visit()
{
  if (! this->is_ready())
    throw std::runtime_error("proc_node not ready");

  Output_T output = static_cast<Derived_T *>(this)->visit_impl();

  m_inputs.pop_front();
  for (size_t n = 0; n < CARDINALITY; ++n)
    --m_numInputs[n];

  return output;
}

#endif
