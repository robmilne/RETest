#include "test.h"

#ifdef RET_GROUP_1_TESTS

static ret_val_t Group1Test0(ret_param_t* param);
static ret_val_t Group1Test1(ret_param_t* param);

/* Example of a test list that contains both leaf and branch functions */
static ret_test_t tests [] = {
  {Group1Test0, "Group1Test0"},
  {Group1Test1, "Group1Test1"},
#ifdef RET_GROUP_2_TESTS
  {group_2_tests, "group_2_tests"}
#endif
};
static ret_list_t test_list = {
  sizeof tests / sizeof *tests,
  tests
};

ret_val_t group_1_tests(ret_param_t* param) {
  return(retExecuteList(param, &test_list));
}

static ret_val_t Group1Test0(ret_param_t* param) {
  RET_MODE_SEARCH();

  RET_ASSERT(1);

  return RET_PASS;
}
static ret_val_t Group1Test1(ret_param_t* param) {
  RET_MODE_SEARCH();

  RET_ASSERT(0);

  return RET_PASS;
}

#endif // #ifdef RET_GROUP_1_TESTS
