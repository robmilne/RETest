#include "test.h"

#ifdef RET_GROUP_2_TESTS

static ret_retval_t Group2Test0(ret_param_t* param);
static ret_retval_t Group2Test1(ret_param_t* param);

static ret_test_t tests [] = {
  {Group2Test0, "Group2Test0"},
  {Group2Test1, "Group2Test1"}
};
static ret_list_t test_list = {
  sizeof tests / sizeof *tests,
  tests
};

ret_retval_t group_2_tests(ret_param_t* param) {
  return(retExecuteList(param, &test_list));
}

static ret_retval_t Group2Test0(ret_param_t* param) {
  RET_MODE_SEARCH();

  RET_ASSERT(1);

  return RET_PASS;
}
static ret_retval_t Group2Test1(ret_param_t* param) {
  RET_MODE_SEARCH();

  RET_ASSERT(1);

  return RET_PASS;
}


#endif // #ifdef RET_GROUP_2_TESTS
