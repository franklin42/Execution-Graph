// (C) Copyright Jonathan Franklin 2011.
// Use, modification and distribution are subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt).

#define BOOST_TEST_MODULE SHARED_PROC_NODE_TEST
#include <boost/test/unit_test.hpp>

#include "active_proc_node.hpp"

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
template<typename Input_T, typename Output_T>
class my_active_node
: public active_proc_node<my_active_node<Input_T, Output_T>, Input_T, Output_T>
{
public:
  Output_T visit_impl()
  { return this->inputQueue().front(); }

private:
  friend class active_proc_node<my_active_node<Input_T, Output_T>, Input_T, Output_T>;
  void thread_run() { }
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

BOOST_AUTO_TEST_CASE(active_proc_node_basic_tests)
{
  typedef boost::fusion::vector<int, char, std::string> vector_t;
  vector_t input1(1, 'x', "howdy");

  my_active_node<vector_t, vector_t> node;
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
}

BOOST_AUTO_TEST_CASE(active_proc_node_thread_tests)
{
  typedef boost::fusion::vector<int, char, std::string> vector_t;
  vector_t input1(1, 'x', "howdy");

  my_active_node<vector_t, vector_t> node;
  BOOST_CHECK_EQUAL(false, node.running());
  BOOST_CHECK_EQUAL(true, node.startup());
  BOOST_CHECK_EQUAL(false, node.startup());
  //BOOST_CHECK_EQUAL(true, node.running());
  node.shutdown(true /* join */);
  BOOST_CHECK_EQUAL(false, node.running());
}

template<typename Input_T, typename Output_T>
class splitter_node
: public active_proc_node<my_active_node<Input_T, Output_T>, Input_T, Output_T>
{
public:
  Output_T visit_impl()
  { return this->inputQueue().front(); }

private:
  friend class active_proc_node<my_active_node<Input_T, Output_T>, Input_T, Output_T>;
  void thread_run() { }
};

BOOST_AUTO_TEST_CASE(active_proc_node_tie_exec_tests)
{
  typedef boost::fusion::vector<int, int, std::string> inputs_t;
  typedef boost::fusion::vector<int, std::string>      outputs_t;
  typedef my_active_node<inputs_t, outputs_t> source_node_t;
  typedef my_active_node<outputs_t, void>      sync_node_t;

  inputs_t input1(10, 20, "1234567890");
  source_node_t node1;
  sync_node_t   node2;
  node1.consumer<0, int>(boost::bind(
    & sync_node_t::enqueue<0, int>, & node2, _1, true));
  node1.consumer<1, std::string>(boost::bind(
    & sync_node_t::enqueue<1, std::string>, & node2, _1, true));
  //splitter_node<vector_t> node2;
}
