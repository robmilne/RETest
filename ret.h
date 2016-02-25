/**************************************************************************//**
 * @file ret.h
 * @brief Recursive Embedded Test header
 *
 * This file contains definitions and macros that must be configured to allow
 * RET to function inside an embedded system.
 *
 * The RET...SIZE preprocessor definitions must be set so as to stay within the
 * available RAM resources of the target device.
 *
 * The RET_SYS_TICK_FUNC() and RET_SEND_BUF() macros must refer to system calls
 * that provide time data and communication transmission.
 *
 * This test framework has been used with Segger RTT.  Segger's J-link probe
 * can be used to both program and run the unit tests on the target device.
 * The RTT viewer and embedded RTT driver code is downloadable from here...
 * @see https://www.segger.com/jlink-software.html
 * The following simple example code can be used on the target device to
 * trigger a pre-compiled test via the JLinkRTTViewer software:
 * @code
 * #ifdef UNIT_TEST
 *   SEGGER_RTT_Init();
 *   while(1) {
 *     if(SEGGER_RTT_HasKey()) {
 *       char c = SEGGER_RTT_GetKey();
 *       if((c == 't') || (c == 'T')) {
 *         Test();
 *       } else {
 *         SEGGER_RTT_Write(0, (char*)&c, 1);
 *       }
 *     }
 *   }
 * #endif
 * @endcode
 * A fuller RTT command processor implementation would permit execution of
 * specific tests.
 */
#ifdef __cplusplus
extern "C" {
#endif

#ifndef __RET_H_
#define __RET_H_

#include <stdint.h>
#include <setjmp.h>
#include <string.h>
#include <stdbool.h>
/* Includes for RET_SYS_TICK_FUNC() and RET_SEND_BUF() macros */
#include "stm32f1xx_hal.h"
#include "SEGGER_RTT.h"


/******************************************************************************
* P U B L I C    D E F I N I T I O N S
******************************************************************************/
/**
 * @brief RET memory controls
 *
 * The following block of definitions set the size limits of buffers and test
 * recursion.  Adjust according to target device memory constraints.
 */
#define RET_REPORT_BUF_SIZE       0x1000
#define RET_MAX_TAG_STRING_SIZE   256
#define RET_MAX_NEST_SIZE         6

/**
 * @brief Root tag that prefixes all test tag strings
 */
#define RET_ROOT_TAG              "ROOT"

/* Definitions for pause arg of retInfoLine() */
#define RET_PAUSE                 1
#define RET_NO_PAUSE              0

/* Optional preprocessor define to reduce code size */
#define RET_NO_PRINTF


/******************************************************************************
* P U B L I C    M A C R O S
******************************************************************************/
/**
 * @brief Timer macro
 *
 * Macro to give RET access to a system timer function for calculating the
 * elapsed time to execute a test function.  Time statistics are presented in
 * the test report at the conclusion of the test.
 */
#define RET_SYS_TICK_FUNC() HAL_GetTick()

/**
 * @brief Communication Tx macro
 *
 * Macro to give RET access to a communication transmission function
 */
#define RET_SEND_BUF(x) SEGGER_RTT_WriteString(0, (x));

/**
 * @brief Test function diagnostic macro
 *
 * This macro returns immediately if the argument equates to non-zero (ie: true)
 * A zero argument constitutes a function failure which causes an information
 * line to be generated for the final report and terminates the function.
 */
#define RET_ASSERT(x) retAssert((x),param,(__LINE__),(__FILE__))

/**
 * @brief RET search/skip macro
 *
 * Place this macro at the start of each leaf test function.
 * Used for executing specific tests or listing test string tags.
 */
#define RET_MODE_SEARCH()                  \
{                                     \
  if((param->mode == RET_MODE_SEARCH) ||   \
     (param->mode == RET_MODE_SKIP))  \
    return RET_PASS;                  \
}


/******************************************************************************
* P U B L I C    D A T A T Y P E S
******************************************************************************/
/**
 * @brief Test function return values
 */
typedef enum {
  RET_PASS,
  RET_FAIL,
  RET_ERR_TIMEOUT,
  RET_ERR_TAG  /**< Test tree is too deep for RET...SIZE definitions */
} ret_val_t;

/**
 * @brief Test modes
 */
typedef enum {
  RET_MODE_SEARCH,
  RET_MODE_EXE,
  RET_MODE_SKIP,  /**< RET engine use only */
} ret_mode_t;

/**
 * @brief RET control passed to all tests - elements 1 & 2 are set by the user
 */
typedef struct {
  ret_mode_t  mode; /**< User test type (RET_MODE_EXE or RET_MODE_SEARCH) */
  char*       test_tag; /**< User test string */
  int32_t     tag_found;  /**< Search flag */
  int32_t     retval; /**< Local test function return value */
} ret_param_t;

/**
 * @brief RET test function prototype
 */
typedef ret_val_t ret_func_t(ret_param_t* param);

/**
 * @brief RET test structure
 */
typedef struct {
  ret_func_t* func;
  const char* tag;
} ret_test_t;

/**
 * @brief RET test list structure
 */
typedef struct {
  uint32_t    size;
  ret_test_t* first;
} ret_list_t;


/******************************************************************************
* P U B L I C    F U N C T I O N    P R O T O T Y P E S
******************************************************************************/
void      retStart        (ret_param_t* param);
ret_val_t retExecuteList  (ret_param_t* param, ret_list_t* list);
void      retAssert       (int assert_condition, ret_param_t* param,
                           int line_number, char *file_name);
void      retInfoLineFmt  (char const* str);
void      retInfoLine     (char const* str, bool pause);

#endif  /* __RET_H_ */

#ifdef __cplusplus
}
#endif
