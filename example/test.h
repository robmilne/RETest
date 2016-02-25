#ifndef __TEST_H_
#define __TEST_H_

#include "ret.h"

/******************************************************************************
* P U B L I C    D E F I N I T I O N S
******************************************************************************/
#ifdef UNIT_TEST
  #define RET_GROUP_0_TESTS
  #define RET_GROUP_1_TESTS
  #define RET_GROUP_2_TESTS
#endif

/******************************************************************************
* P U B L I C    F U N C T I O N    P R O T O T Y P E S
******************************************************************************/
void      Test          (void);
ret_val_t RunTrunk      (ret_param_t* param);
ret_val_t group_0_tests (ret_param_t* param);
ret_val_t group_1_tests (ret_param_t* param);
ret_val_t group_2_tests (ret_param_t* param);

#endif // #ifdef __TEST_H_
