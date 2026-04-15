// Unity runner for the scheme-layer vim motion suite.
//
// The heavy lifting — chibi bootstrap, library load, per-test buffer
// seeding, and motion assertions — lives in test/scheme_test_init.c
// and test/scheme/vim/motion-test.scm (SRFI 64). This file is just a
// thin gate: if SRFI 64 reports any failures, Unity fails the test.

#include "unity/unity.h"

#include "scheme_test_init.h"
#include "../src/text/buffer.h"

void setUp(void) {
    buffer_list_init();
    scheme_test_init();
}

void tearDown(void) {
    scheme_test_quit();
    buffer_list_quit();
}

void test_vim_motion_suite(void) {
    int fails = scheme_test_run("./test/scheme/main.scm");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, fails, "SRFI 64 suite reported failures");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_vim_motion_suite);
    return UNITY_END();
}
