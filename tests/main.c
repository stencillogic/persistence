#include <stdio.h>
#include <stdlib.h>

#include "tests.h"

int g_test_continue_on_fail = 0;

int process_test_fail(int test_result, const char* test, int line)
{
    if(test_result != 0)
    {
        printf("Failed test: %s, at line %d\n", test, line);
        if(0 == g_test_continue_on_fail) exit(1);
    }
    else
    {
        printf("Successfully passed the test: %s\n", test);
    }
    return 0;
}

int main(int argc, char **argv)
{
    process_test_fail(test_auth_sha3_512(), "test_auth_sha3_512", __LINE__);

    printf("Test execution completed.\n");
    return 0;
}
