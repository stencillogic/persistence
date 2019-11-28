#include "tests.h"
#include "common/stack.h"
#include "common/error.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>


int test_stack_functions()
{
    puts("Starting test test_stack_functions");

    uint32 elnum = 10u;
    uint32 val;


    puts("Testing stack functions");

    // create
    handle sh = stack_create(elnum, sizeof(uint64), malloc(stack_get_alloc_sz(elnum, sizeof(uint64))));
    if(NULL == sh) return __LINE__;

    // truncation
    if(0 != stack_elnum(sh)) return __LINE__;
    if(0 != stack_push(sh, &val)) return __LINE__;
    if(1 != stack_elnum(sh)) return __LINE__;

    stack_reset_sp(sh);
    if(0 != stack_elnum(sh)) return __LINE__;

    // push & pop
    val = 567;
    if(0 != stack_push(sh, &val)) return __LINE__;
    if(1 != stack_elnum(sh)) return __LINE__;

    val = 345;
    if(0 != stack_push(sh, &val)) return __LINE__;
    if(2 != stack_elnum(sh)) return __LINE__;

    val = 0;
    if(0 != stack_pop(sh, &val)) return __LINE__;
    if(1 != stack_elnum(sh)) return __LINE__;
    if(val != 345) return __LINE__;

    val = 0;
    if(0 != stack_pop(sh, &val)) return __LINE__;
    if(0 != stack_elnum(sh)) return __LINE__;
    if(val != 567) return __LINE__;

    // edge cases
    for(int i=0; i<elnum; i++)
    {
        if(0 != stack_push(sh, &val)) return __LINE__;
    }
    if(0 == stack_push(sh, &val)) return __LINE__;

    return 0;
}
