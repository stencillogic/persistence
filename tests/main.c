#include <stdio.h>
#include <stdlib.h>

#include "tests.h"

int g_test_continue_on_fail = 0;

int process_test_fail(int test_result, const char* test)
{
    if(test_result != 0)
    {
        printf("Failed test: %s, at line %d\n", test, test_result);
        if(0 == g_test_continue_on_fail) exit(1);
    }
    else
    {
        printf("Successfully passed the test: %s\n", test);
    }
    fflush(NULL);
    return 0;
}

int main(int argc, char **argv)
{
    process_test_fail(test_auth_sha3_512(), "test_auth_sha3_512");
    process_test_fail(test_grigorian_calendar(), "test_grigorian_calendar");
    process_test_fail(test_strop_functions(), "test_strop_functions");
    process_test_fail(test_dateop_functions(), "test_dateop_functions");
    process_test_fail(test_encoding_functions(), "test_encoding_functions");
    process_test_fail(test_decimal_functions(), "test_decimal_functions");

    printf("Test execution completed.\n");
    return 0;
}
