#include "test.h"

#if (1 == RET_COMMON_TESTS)

static ret_retval_t Group0Test0(ret_param_t* param);
static ret_retval_t Group0Test1(ret_param_t* param);

static ret_test_t tests [] = {
  {Group0Test0, "Group0Test0"},
  {Group0Test1, "Group0Test1"}
};
static ret_list_t test_list = {
  sizeof tests / sizeof *tests,
  tests
};

/**************************************************************************//**
 * @brief Example test group
 * @param ret_param_t* - pointer to user control structure
 * @return ret_retval_t
 */
ret_retval_t group_0_tests(ret_param_t* param) {
  /* Branch functions are not tests - they execute a list of tests */
  return(retExecuteList(param, &test_list));
}

/**************************************************************************//**
 * @brief Example test function
 * @param ret_param_t* - pointer to user control structure
 * @return ret_retval_t
 */
static ret_retval_t Group0Test0(ret_param_t* param) {
  /* Every 'leaf' function must place the RET_MODE_SEARCH macro at the start
     of the function body */
  RET_MODE_SEARCH();

  RET_ASSERT(1);

  return RET_PASS;
}

/**************************************************************************//**
 * @brief Example test function
 * @param ret_param_t* - pointer to user control structure
 * @return ret_retval_t
 */
static ret_retval_t Group0Test1(ret_param_t* param) {
  RET_MODE_SEARCH();

  RET_ASSERT(1);

  return RET_PASS;
}


#endif // #ifdef RET_GROUP_0_TESTS
