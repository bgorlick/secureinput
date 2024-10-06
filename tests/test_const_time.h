#ifndef TEST_CONST_TIME_H
#define TEST_CONST_TIME_H

#include "secureinput.h"

// Test version of get_password_input
int test_get_password_input(pw_state_t *pw_state, const char *prompt);

// Test version of getpw_asm
void test_getpw_asm();

#endif // TEST_CONST_TIME_H