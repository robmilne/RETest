RET (acronym for "Recursive Embedded Test")

RET is a simple embedded C test framework that uses a test list walker and a
setjmp/longjmp exception handler to execute test functions according to string
comparisons.  When a test function is encountered it's tag identifier is
concatenated to a string for comparison to the user's input string to determine
if the function is to be executed.  If the user's input string is present
in the generated 'path' then the function is executed and a report is appended
to the output buffer.  Before the next test function is processed the tag of
the previous test is removed from the test path.  If the user's input string is
the root tag then every test of the test tree will be executed.

Test functions can be recursive calls into the test engine with additional
lists of tests to create a branch of the test tree. Test functions are either
branches or leaves of the test tree and they must be constructed and declared
according to a minimal set of RET rules.

ret.h contains contains definitions and macros that must be configured to allow
RET to function inside an embedded system.  The RET_..._SIZE preprocessor
definitions must be set so as to stay within the available RAM resources of the
target device.  The RET_SYS_TICK_FUNC() and RET_SEND_BUF() macros must refer to
system calls that provide time data and communication transmission.

This test framework has been used with Segger RTT.  Segger's J-link probe can be
used to both program and run the unit tests on the target device.  The RTT
viewer and embedded RTT driver code is downloadable from:
https://www.segger.com/jlink-software.html

The following simple example code can be used on the target device to trigger
a pre-compiled test via the JLinkRTTViewer software or custom software developed
with the Segger SDK:

#ifdef UNIT_TEST
  SEGGER_RTT_Init();
  while(1) {
    if(SEGGER_RTT_HasKey()) {
      char c = SEGGER_RTT_GetKey();
      if((c == 't') || (c == 'T')) {
        Test();
      } else {
        SEGGER_RTT_Write(0, (char*)&c, 1);
      }
    }
  }
#endif

A fuller RTT command processor implementation could permit execution of
specific tests.