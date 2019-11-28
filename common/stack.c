#include "common/stack.h"
#include "common/error.h"
#include <string.h>
#include <assert.h>


typedef struct _stack_state
{
    size_t  el_sz;
    uint32  el_num;
    uint32  sp;
    uint8   data_buf[];
} stack_state;


size_t stack_get_alloc_sz(uint32 n, size_t el_sz)
{
    return sizeof(stack_state) + n * el_sz;
}

// create a new instance
// return handle on success, NULL on error
handle stack_create(uint32 n, size_t el_sz, void *buf)
{
    stack_state *ss = (stack_state *)buf;

    ss->sp = 0;
    ss->el_num = n;
    ss->el_sz = el_sz;

    return (handle)ss;
}

// set stack empty
void stack_reset_sp(handle sh)
{
    stack_state *ss = (stack_state *)sh;

    ss->sp = 0u;
}

// return 0 on success, non-0 otherwise
sint8 stack_push(handle sh, const void *el)
{
    stack_state *ss = (stack_state *)sh;

    if(ss->sp == ss->el_num)
    {
        error_set(ERROR_OUT_OF_MEMORY);
        return -1;
    }

    memcpy(&ss->data_buf[ss->sp * ss->el_sz], el, ss->el_sz);
    ss->sp++;

    return 0;
}


uint32 stack_elnum(handle sh)
{
    stack_state *ss = (stack_state *)sh;

    return ss->sp;
}


sint8 stack_pop(handle sh, void *el)
{
    stack_state *ss = (stack_state *)sh;

    assert(ss->sp > 0);
    if(ss->sp == 0)
    {
        return -1;
    }

    ss->sp--;
    memcpy(el, &ss->data_buf[ss->sp * ss->el_sz], ss->el_sz);

    return 0;
}

