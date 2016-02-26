/**************************************************************************//**
 * @file test.c
 * @brief Run unit tests with RET framework
 */
#include "test.h"
#include "ret.h"

#ifdef UNIT_TEST

/******************************************************************************
* S T A T I C    D A T A
******************************************************************************/
/* Preprocessor defines in test.h determine which test branches to execute */
static ret_test_t trunk[] = {
  #ifdef RET_GROUP_0_TESTS
    {group_0_tests, "group_0_tests"},
  #endif
  #ifdef RET_GROUP_1_TESTS
    {group_1_tests, "group_1_tests"},
  #endif
};
static ret_list_t trunk_list = {
  sizeof trunk / sizeof *trunk,
  trunk
};


/**************************************************************************//**
 * @brief Entry point for unit testing using RET
 * @param none
 * @return none
 */
void Test(void) {
  /* Global user configuration that is passed to all test functions */
  ret_param_t param;

#if 1
  /* Run tests */
  param.mode = RET_MODE_EXE;
  param.test_tag = RET_ROOT_TAG; // Executes entire compiled test tree
  //param.test_tag = "Group1Test1"; // Execute Group1Test1 only
#else
  /* Search test tree  */
  param.mode = RET_MODE_SEARCH;
  param.test_tag = RET_ROOT_TAG; // Display tags for all compiled tests
  //param.test_tag = "group_1_tests"; // Display test tags for group_1_tests
#endif

  /* Start test engine */
  retStart(&param);
}

/**************************************************************************//**
 * @brief RET test function that runs each test branch
 * @param ret_param_t* - pointer to user control structure
 * @return ret_val_t
 */
ret_val_t RunTrunk(ret_param_t* param) {
  /* Branch functions are not tests - they execute a list of tests */
  return(retExecuteList(param, &trunk_list));
}

#endif // #ifdef UNIT_TEST
