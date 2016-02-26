/**************************************************************************//**
 * @file ret.c
 * @brief Recursive Embedded Test Engine
 *
 * RET uses a list walker and an exception handler using setjmp/longjmp to
 * execute test functions according to string comparisons.  The current
 * position within the tree of tests is determined by a tag string that is
 * constructed/destructed by the retEnter and retExit functions.
 *
 * The retExecuteList function is nestable such that test functions can execute
 * a sub-list of tests.  If the target test path is present in the generated
 * path then the function is executed and a report is appended to the output
 * buffer.  An execution of the root tag will cause the execution of every
 * compiled test function.
 *
 * Memory for test functions and the results buffer is statically allocated.
 * If the target device lacks sufficient memory for a large collection of tests
 * then a partial list can be built using test branch defines (see example).
 * The ...SIZE preprocessor definitions in ret.h must be set appropriately for
 * the RAM memory resources of the target device.
 */
#ifdef UNIT_TEST

#ifdef __cplusplus
extern "C" {
#endif

#include "ret.h"


/******************************************************************************
* S T A T I C    D A T A T Y P E S
******************************************************************************/
/**
 * @brief Setjmp/longjmp environment for nested calls into retExecuteList
 */
typedef struct {
  jmp_buf   env; /**< setjmp environment as per compiler */
  char*     tag_ptr; /**< pointer to the end of the test tag at this nest level */
  uint32_t  timer; /**< start time for elapsed time calculation of nest level */
} ret_env_t;

/**
 * @brief Test function that executes the test branches (feel free to rename)
 */
extern ret_val_t RunTrunk(ret_param_t *param);


/******************************************************************************
* S T A T I C   D A T A
******************************************************************************/
/**
 * @brief Static tag and recursion data for test execution & output formatting
 */
static struct {
  uint32_t  next_line_number; /**< Output buffer line number */
  char      tag_str[RET_MAX_TAG_STRING_SIZE]; /**< current test string */
  char*     tag_ptr; /**< test string end position for retRemoveTag() */
  uint32_t  nest; /**< Recursion level into retExecuteList() */
} ret;

/**
 * @brief Static communication buffer & controls
 */
static struct {
  char  buf[RET_REPORT_BUF_SIZE]; /**< Character output buffer */
  char* next_in; /**< Pointer to next available output buffer location */
  bool  is_pause; /**< flag to control when the output buffer is sent  */
} ret_buf;

/**
 * @brief Root test
 *
 * RunTrunk is the top level test function that runs each test branch via
 * retExecuteList.  The root tag prefixes every node of the test tree.
 */
static ret_test_t root[] = {
  {RunTrunk, RET_ROOT_TAG}
};

/**
 * @brief Root test list (DO NOT EDIT)
 */
static ret_list_t root_list = { 1, root };

/**
 * @brief Array of setjmp environments indexed via ret.nest variable
 */
static ret_env_t   ret_env[RET_MAX_NEST_SIZE];

/* Const data */
static const char* RET_TAG_ERR_MSG = "Error: RET_MAX_TAG_STRING_SIZE exceeded";
static const char* RET_LAYER_ERR_MSG = "Error: RET_MAX_NEST_SIZE exceeded";
static const char* RET_PATH_ERR_MSG = "test path not found";
static const char* RET_TEST_DONE_MSG = "DONE";
static const char* RET_RETVAL_STR[4] = {"PASS", "FAIL", "TIMEOUT", "TAG_ID"};
static const char  RET_DIGITS[16] = {'0', '1', '2', '3', '4', '5', '6', '7',
                                     '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
/* DO NOT USE THIS CHARACTER IN A TEST FUNCTION TAG! */
#define RET_TOKEN_DELIMITER '@'


/******************************************************************************
* S T A T I C    F U N C T I O N    P R O T O T Y P E S
******************************************************************************/
static ret_val_t  retEnter            (ret_param_t* param, ret_test_t* test);
static void       retExit             (ret_param_t* param, ret_val_t retval);
static bool       retFindTagToken     (ret_param_t *param);
static ret_val_t  retAddTag           (char const* const tag);
static void       retRemoveTag        (uint32_t nest_val);
static void       retTestLineFormat   (ret_val_t retval, uint32_t elapsed_time);

static void       retDecimalDigits    (uint32_t value, uint32_t width);
static void       retConvIntToDecAscii(char* buf, int32_t val);

static void       retPutChar          (char const printable_ascii);
static void       retPutString        (char const* out_string);
static void       retPutLineFeed      (void);
static void       retPutCommaSeparator(void);
static void       retSendBuffer       (void);
static void       retSearchLine       (char const* tag);
static void       retFormatLine    (char msg_type, char const* str, bool pause);


/**************************************************************************//**
 * @brief Initialize & start test engine
 * @param ret_param_t* - pointer to user control structure
 * @return none
 */
void retStart(ret_param_t* param) {
  ret.next_line_number = 0;
  ret.tag_str[0] = '\0'; /* empty tag string at start of test */
  ret.tag_ptr = ret.tag_str;
  ret.nest = 0;
  ret_buf.is_pause = RET_PAUSE;
  ret_buf.next_in = ret_buf.buf;
  param->tag_found = 0;
  param->retval = 0;

  /* Start test */
  retExecuteList(param, &root_list);
}


/**************************************************************************//**
 * @brief Execute a test list
 * @param ret_param_t* - pointer to user control structure
 * @param ret_list_t* - pointer to test list structure (size + ret_test_t ptr)
 * @return ret_val_t - see ret.h
 */
ret_val_t retExecuteList(ret_param_t* param, ret_list_t* list) {
  int         longjmp_val;
  ret_test_t* test;
  ret_test_t* last = list->first + list->size;
  ret_val_t   retval, err_flag;
  bool        save_pause;

  /* Prevent nesting beyond end of environment buffer (recursion limit) */
  if(ret.nest >= RET_MAX_NEST_SIZE) {
    retInfoLineFmt(RET_LAYER_ERR_MSG);
    return RET_FAIL;
  }

  /* Save IO verbose/quiet setting */
  save_pause = ret_buf.is_pause;

  /* Save the current terminator position of ret.tag_ptr with the environment.
   * This allows longjump to return back to any previous nested environment
   * and it's corresponding tag_str string.
   */
  ret_env[ret.nest].tag_ptr = ret.tag_ptr;

  for(test = list->first, err_flag = RET_PASS; test < last; test++) {
    if((longjmp_val = setjmp(ret_env[ret.nest].env)) == 0) {
      retval = retEnter(param, test);
    } else {
      /* longjmp value (cannot be zero) */
      switch(longjmp_val) {
        case -1:
          /* value returned by retAssert() */
          retval = RET_FAIL;
          break;

        default:
          retval = RET_PASS;
          break;
      }
    }
    if(retval != RET_PASS)
      err_flag = RET_FAIL;

    retExit(param, retval);
  }

  ret_buf.is_pause = save_pause;
  return(err_flag);
}


/**************************************************************************//**
 * @brief Execute a test
 *
 * Append the test tag to the global tag path (length permitting).  If the test
 * is not a search then determine the execution mode of the test by comparing
 * ret.tag_str with the test tag inside the  ret_param_t parameter.  Set the
 * start time and run the test.
 * @bNB: Each leaf function MUST have the RET_MODE_SEARCH macro at the start of
 * the body of the function.
 * @bNB: Branch functions (recursive calls to retExecuteList) do not require
 * the RET_MODE_SEARCH macro
 *
 * @param ret_param_t* - pointer to user control structure
 * @param ret_test_t* - pointer to test structure (func + tag)
 * @return ret_val_t - see ret.h
 */
static ret_val_t retEnter(ret_param_t* param, ret_test_t* test) {
  /* Append tag of current function to end of the global tag path
   * Increment ret nesting value
   */
  if(retAddTag(test->tag) == RET_ERR_TAG) {
    retInfoLineFmt(RET_TAG_ERR_MSG);
    return RET_ERR_TAG;
  }

  if(param->mode != RET_MODE_SEARCH) {
    if(!retFindTagToken(param)) {
      /* If test tag not present in global tag_str, skip leaf function
       * NB: Only leaf functions use the RET_MODE_SEARCH macro (permits skip)
       */
      if(param->mode == RET_MODE_EXE)
        param->mode = RET_MODE_SKIP;
    } else {
      /* Test tag present in global tag_str
       * Save start time for elapsed time calculation in retExit
       */
      if(param->mode == RET_MODE_SKIP)
        param->mode = RET_MODE_EXE;

      /* Get millisecond timer count from system (see ret.h) */
      ret_env[ret.nest - 1].timer = RET_SYS_TICK_FUNC();
    }
  }

  /* Execute test function */
  return(test->func(param));
}


/**************************************************************************//**
 * @brief Cleanup after return from test function
 *
 * Remove the last test tag from the executed test.  Jump to the root level
 * (test completed) if the param.tag is the last segment of ret.tag_str.  When
 * the root level calls this function the test report is sent to the host.
 *
 * @param ret_param_t* - pointer to user control structure
 * @param ret_val_t - setjmp return value or tag string error from retEnter
 * @return none
 */
static void retExit(ret_param_t* param, ret_val_t retval) {
  uint32_t elapsed_time;

  if(retval == RET_ERR_TAG) {
    /* Tag length error
     * Do not decrement nesting since no corresponding increment
     */
    retTestLineFormat(retval, 0);
    return;
  }

  if(retFindTagToken(param)) {
    if(param->mode != RET_MODE_SEARCH) {
      /* Execution clean-up
       * All functions with test name in tag_str have been executed and
       * require timer cleanup and result reporting
       */
      elapsed_time = RET_SYS_TICK_FUNC() - ret_env[ret.nest - 1].timer;
      retTestLineFormat(retval, elapsed_time);
    } else {
      /* Search - return branches from supplied path */
      retSearchLine(ret.tag_str);
    }
  }

  /* If param->test_tag is the last segment of ret.tag_str then the requested
   * test has completed (subtests are always present at the end of the tag until
   * completion of the higher level test)
   * longjmp to root environment to exit RET
   */
  if(strcmp(ret.tag_str, param->test_tag) == 0) {
    retRemoveTag(0);
    /* NB: If the value passed to longjmp is 0, setjmp will behave as if it had
     *     returned 1
     */
    longjmp(ret_env[0].env, retval);
  }

  /* Return tag terminator to original position prior to current function call
   * Decrement ret.nest value by one
   */
  if(ret.nest)
    retRemoveTag(ret.nest - 1);

  if(ret.nest == 0) {
    /* End of test
     * param->tag_found is 0 if function tag not found
     */
    if((param->tag_found == 0) && (strcmp(param->test_tag, RET_ROOT_TAG) != 0))
    {
      retInfoLine(RET_PATH_ERR_MSG, RET_PAUSE);
    } else {
      /* Output test report */
      retPutLineFeed();
      retPutString(RET_TEST_DONE_MSG);
      retSendBuffer();
    }
  }
}


/**************************************************************************//**
 * @brief Determine if the test tag is contained exactly in the constructed tag
 *
 * Remove the last test tag from the executed test.  Jump to the root level
 * (test completed) if the param.tag is the last segment of ret.tag_str.  When
 * the root level calls this function the result buffer is sent to the host.
 *
 * @param ret_param_t* - pointer to user control structure
 * @return ret_val_t - see ret.h
 */
static bool retFindTagToken(ret_param_t *param) {
  char* tag_pos;

  /* Check if the test string is a ret.tag_str token
   * Check character beyond end of test string since it may be a subset
   * of a larger tag (ie: ...@testXXX@... and ...@testXXXConfig@...)
   */
  tag_pos = strstr(ret.tag_str, param->test_tag);
  if((tag_pos == NULL) ||
      ((*(tag_pos + strlen(param->test_tag)) != RET_TOKEN_DELIMITER) &&
      (*(tag_pos + strlen(param->test_tag)) != '\0'))) {
    return false;
  } else {
    /* Increment flag to indicate path tag_found */
    param->tag_found++;
    return true;
  }
}


/**************************************************************************//**
 * @brief Append a test tag to the tag path
 * @param char* - tag
 * @return none
 */
static ret_val_t retAddTag(char const* const tag) {
  char* save_previous;
  static char tag_separator[] = {RET_TOKEN_DELIMITER, '\0'};

  save_previous = ret.tag_ptr;
  ret.tag_ptr += strlen(tag) + sizeof tag_separator - 1;

  /* Check for tag buffer overrun */
  if(!(ret.tag_ptr < ret.tag_str + RET_MAX_TAG_STRING_SIZE)) {
    ret.tag_ptr = save_previous;
    return RET_ERR_TAG;
  }

  /* Append test tag to the global tag path */
  strcat(save_previous, tag_separator);
  strcat(save_previous, tag);

  /* Increment nesting level */
  ret.nest++;
  return RET_PASS;
}


/**************************************************************************//**
 * @brief Remove tag(s) from the end of the tag path & set the recusion level
 * @param uint32_t - desired recursion level
 * @return none
 */
static void retRemoveTag(uint32_t nest_val) {
  if(nest_val >= RET_MAX_NEST_SIZE)
    return;
  /* Terminator location is always legal since checked by retAddTag */
  ret.tag_ptr = ret_env[nest_val].tag_ptr;
  *ret.tag_ptr = '\0' ;

  /* Decrement nesting level */
  ret.nest = nest_val;
}


/**************************************************************************//**
 * @brief Diagnostic routine called by RET_ASSERT macro
 *
 * This function returns immediately if assert_condition is non-zero (ie: true)
 * A zero assert_condition constitutes a function failure which causes an
 * information line to be generated for the final report as well as terminating
 * the execution of the test function via a longjmp to retEnter.
 *
 * @param int - statement for TRUE/FALSE evaluation
 * @param ret_param_t* - pointer to user control structure
 * @param int - __LINE__ macro from calling file
 * @param int - __FILE__ macro from calling file
 * @return none
 */
void retAssert(int assert_condition, ret_param_t *param, int line_number,
               char *file_name) {

  if(assert_condition) {
    return;
  } else {
    char assert_buf[128];

#ifndef RET_NO_PRINTF
    sprintf(assert_buf, "Assert at line %d of %s == %d", line_number, file_name,
            param->retval);
#else
    char ascii_buf[12];

    assert_buf[0] = '\0';
    strcat(assert_buf, "Assert at line ");
    retConvIntToDecAscii(ascii_buf, line_number);
    strcat(assert_buf, ascii_buf);
    strcat(assert_buf, " of ");
    strcat(assert_buf, file_name);
    strcat(assert_buf, " == ");
    retConvIntToDecAscii(ascii_buf, param->retval);
    strcat(assert_buf, ascii_buf);
#endif
    retInfoLineFmt(assert_buf);
    longjmp (ret_env[ret.nest - 1].env, -1);
  }
}


/**************************************************************************//**
 * @brief Append information line to output buffer
 *
 * Construct information line and pass to output buffer for display at the
 * conclusion of the test.
 *
 * @param char* - message to append to output report buffer
 * @return none
 */
void retInfoLineFmt(char const* str) {
  retInfoLine(str, RET_NO_PAUSE);
}


/**************************************************************************//**
 * @brief Append information line to output buffer with pause option
 * @param char* - message to append to output report buffer
 * @param bool - 1|0 : send msg immediately|send msg at completion of test
 * @return none
 */
void retInfoLine(char const* str, bool pause) {
  retFormatLine('I', str, pause);
}


/**************************************************************************//**
 * @brief Send search line to communication port immediately
 * @param char* - message to append to output report buffer
 * @return none
 */
static void retSearchLine(char const* str) {
  retFormatLine('S', str, RET_NO_PAUSE);
}


/**************************************************************************//**
 * @brief Send msg to output buffer or to comm port immediately
 * @param char - message type character
 * @param char* - message to append to output report buffer
 * @param bool - 1|0 : send msg immediately|send msg at completion of test
 * @return none
 */
static void retFormatLine(char msg_type, char const* str, bool pause) {
  bool save_pause;

  save_pause = ret_buf.is_pause;
  ret_buf.is_pause = pause;
  retPutChar(msg_type);
  retPutCommaSeparator();
  retDecimalDigits(ret.next_line_number++ , 4);
  retPutCommaSeparator();
  retPutString("    ");
  retPutCommaSeparator();
  retPutString("      ");
  retPutCommaSeparator();
  if(strlen(str) > RET_MAX_TAG_STRING_SIZE) {
    str = "<string exceeds length limit>";
  }
  retPutString(str);
  retPutLineFeed();
  ret_buf.is_pause = save_pause;
}


/**************************************************************************//**
 * @brief Send test result to output buffer
 * @param ret_val_t - return value of test
 * @param uint32_t - elapsed time for test execution
 * @return none
 */
static void retTestLineFormat(ret_val_t retval, uint32_t elapsed_time) {
  retPutChar('T');
  retPutCommaSeparator();
  retDecimalDigits(ret.next_line_number++ , 4) ;
  retPutCommaSeparator();
  retPutString(RET_RETVAL_STR[retval]);
  retPutCommaSeparator();
  retDecimalDigits(elapsed_time , 6);
  retPutCommaSeparator();
  retPutString(ret.tag_str);
  retPutLineFeed();
}

/* Helper routine to reverse the order of string characters */
static void str_rev(char *start, char *end) {
  char temp;
  while(end > start) {
    temp = *end;
    *end-- = *start;
    *start++ = temp;
  }
}


/**************************************************************************//**
 * @brief Convert signed long binary to signed decimal ascii
 * @param char* - pointer to destination buffer (minimum size = 12)
 * @param int32_t - binary signed input value
 * @return none
 */
static void retConvIntToDecAscii(char* dst_buf, int32_t val) {
  char  *tmp_str = dst_buf;
  bool    neg_flag = false;

  /* Sign logic */
  if(val < 0) {
    /* Make input binary value positive */
    val = -val;
    neg_flag = true;
  }

  /* Characters are reversed by conversion */
  do {
    *tmp_str++ = (char)RET_DIGITS[val % 10];
  } while(val /= 10);

  /* Add negative sign character if required */
  if(neg_flag) {
    *tmp_str++ = '-';
  }

  /* Add zero terminator to temp string */
  *tmp_str = '\0';

  /* Reverse characters in temp string (exclude terminator) */
  str_rev(dst_buf, tmp_str - 1);
}


/**************************************************************************//**
 * @brief Convert unsigned long binary to unsigned decimal ascii & right justify
 * into array of 'width' space characters for a vertically aligned output
 * @param uint32_t - binary unsigned input value
 * @param uint32_t - Length of text area in which to right justify char output
 * @return none
 */
static void retDecimalDigits(uint32_t value, uint32_t width) {
  static char const blanks[] = "          ";
  static char work[sizeof blanks];
  uint32_t next;
  char* ch_ptr;

  if((width > 0) && (width < sizeof work - 1)) {
    strcpy(work, blanks);
    ch_ptr = work + width;
    *ch_ptr = '\0';
    do {
      next = value / 10 ;
      *-- ch_ptr = (char)('0' + value - next * 10);
      value = next ;
    } while(value && ch_ptr > work);

    retPutString(work);
  }
}


/**************************************************************************//**
 * @brief Output char to the output buffer
 *
 * retPutChar should buffer characters until it sees a linefeed.
 * On receipt of linefeed retPutChar may may do a synchronous output
 * through a comm channel depending on ret_buf.is_pause
 *
 * @param char - character
 * @return none
 */
static void retPutChar(char const c) {
  if(ret_buf.next_in < ret_buf.buf + RET_REPORT_BUF_SIZE) {
    *ret_buf.next_in++ = c;
    if('\n' == c && ret_buf.is_pause)
      retSendBuffer();
  }
}


/**************************************************************************//**
 * @brief Output string to the output buffer
 * @param char* - string
 * @return none
 */
static void retPutString(char const* str) {
  for(char c = *str++; c; c = *str++)
    retPutChar(c);
}


/**************************************************************************//**
 * @brief Output line feed to the output buffer
 * @param none
 * @return none
 */
static void retPutLineFeed(void) {
#if 0
  retPutChar ('\r');
#endif
  retPutChar('\n');
}


/**************************************************************************//**
 * @brief Output field separator feed to the output buffer
 * @param none
 * @return none
 */
static void retPutCommaSeparator(void) {
  retPutChar(',');
}


/**************************************************************************//**
 * @brief Send output buffer to the communication channel
 * @param none
 * @return none
 */
static void retSendBuffer(void) {
  if(ret_buf.next_in != ret_buf.buf) {
    *ret_buf.next_in++ = '\0';
    RET_SEND_BUF(ret_buf.buf)
    ret_buf.next_in = ret_buf.buf;
  }
}


#ifdef __cplusplus
}
#endif

#endif /* #ifdef UNIT_TEST */