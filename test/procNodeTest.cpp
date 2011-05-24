// (C) Copyright Jonathan Franklin 2011.
// Use, modification and distribution are subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt).

#define BOOST_TEST_MODULE PROC_NODE_TEST
#include <boost/test/unit_test.hpp>

#include "proc_node.hpp"

#include <boost/array.hpp>
#include <boost/fusion/include/at.hpp>
#include <boost/fusion/include/at_c.hpp>
//#include <boost/fusion/include/is_sequence.hpp>
//#include <boost/fusion/include/has_key.hpp>
#include <boost/fusion/include/mpl.hpp>
#include <boost/fusion/include/value_of.hpp>
#include <boost/fusion/include/vector.hpp>
//#include <boost/mpl/vector.hpp>

#include <deque>
#include <string>

//==============================================================================
#if 0
template<typename Derived_T, typename Inputs_T, typename Outputs_T>
class proc_node
{
public:
  proc_node() : m_inputs(), m_numInputs() {}

  template<int N, typename InputComponent_T>
  void enqueue(const InputComponent_T & val)
  {
    typedef boost::fusion::result_of::begin<Inputs_T>::type first;
    typedef boost::mpl::int_<N> N_;
    typedef boost::fusion::result_of::advance<first, N_ >::type comp;
    typedef boost::fusion::result_of::value_of<comp>::type InputComponent_t;
    BOOST_MPL_ASSERT(( boost::is_same<InputComponent_t, InputComponent_T> ));

    const size_t NUM_INPUTS = m_numInputs[N];
    if (m_inputs.size() == NUM_INPUTS)
    { // Create a new input tuple.
      m_inputs.push_back(Inputs_T());
    }

    // Update the tuple.
    boost::fusion::at_c<N>(m_inputs[NUM_INPUTS]) = val;
    ++m_numInputs[N];
  }

  bool is_ready() const
  {
    for (size_t n = 0; n < m_numInputs.size(); ++n)
    {
      if (! m_numInputs[n])
        return false;
    }

    return true;
  }

  bool visit(Outputs_T & out)
  {
    if (! this->is_ready())
      return false;

    static_cast<Derived_T *>(this)->visit_impl(out);


    m_inputs.pop_front();
    for (size_t n = 0; n < CARDINALITY; ++n)
      --m_numInputs[n];

    return true;
  }

protected:
  std::deque<Inputs_T> m_inputs;
  static const size_t CARDINALITY =
    typename boost::fusion::result_of::size<Inputs_T>::type::value;
  boost::array<int, CARDINALITY> m_numInputs;
};
#endif

//------------------------------------------------------------------------------
template<typename Input_T, typename Output_T>
class my_proc_node
: public proc_node<my_proc_node<Input_T, Output_T>, Input_T, Output_T>
{
public:
  Output_T visit_impl()
  { return this->inputQueue().front(); }
};

//==============================================================================
template<typename T1, typename T2>
void check_output(const T1 & lhs, const T2 & rhs)
{
  BOOST_CHECK_EQUAL(boost::fusion::at_c<0>(lhs),
                    boost::fusion::at_c<0>(rhs));
  BOOST_CHECK_EQUAL(boost::fusion::at_c<1>(lhs),
                    boost::fusion::at_c<1>(rhs));
  BOOST_CHECK_EQUAL(boost::fusion::at_c<2>(lhs),
                    boost::fusion::at_c<2>(rhs));
}

BOOST_AUTO_TEST_CASE(proc_node_tests)
{
  typedef boost::fusion::vector<int, char, std::string> vector_t;
  vector_t input1(1, 'x', "howdy");

  my_proc_node<vector_t, vector_t> node;
  BOOST_CHECK_EQUAL(false, node.is_ready());

  node.enqueue<0>(boost::fusion::at<boost::mpl::int_<0> >(input1));
  BOOST_CHECK_EQUAL(false, node.is_ready());

  node.enqueue<1>(boost::fusion::at<boost::mpl::int_<1> >(input1));
  BOOST_CHECK_EQUAL(false, node.is_ready());

  node.enqueue<1>('y'); // Should match input2 value.
  BOOST_CHECK_EQUAL(false, node.is_ready());

  node.enqueue<2>(boost::fusion::at<boost::mpl::int_<2> >(input1));
  BOOST_CHECK_EQUAL(true, node.is_ready());
  vector_t output1 = node.visit();
  check_output(input1, output1);

  // The previous call to visit should have consumed the first set of input1.
  vector_t input2(2, 'y', "pardner");
  BOOST_CHECK_EQUAL(false, node.is_ready());

  node.enqueue<0>(boost::fusion::at<boost::mpl::int_<0> >(input2));
  BOOST_CHECK_EQUAL(false, node.is_ready());

  node.enqueue<2>(boost::fusion::at<boost::mpl::int_<2> >(input2));
  BOOST_CHECK_EQUAL(true, node.is_ready());
  vector_t output2 = node.visit();
  check_output(input2, output2);

  //node.enqueue<0>(1.0);
  //node.enqueue<3>(1);
  
  //boost::fusion::result_of::at<vector_t, boost::mpl::int_<2> >::type t = NULL;
  //typedef boost::fusion::result_of::begin<vector_t>::type first;
  //boost::fusion::result_of::value_of<first>::type t = std::string();

}
