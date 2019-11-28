#ifndef _STACK_H
#define _STACK_H


// stack datastructure


#include "defs/defs.h"


// size of the buffer required to create stack data structrue of n elements of size el_sz
size_t stack_get_alloc_sz(uint32 n, size_t el_sz);

// create a new instance
// return handle on success, NULL on error
handle stack_create(uint32 n, size_t el_sz, void *buf);

// set stack empty
void stack_reset_sp(handle sh);

// return 0 on success, non-0 otherwise
sint8 stack_push(handle sh, const void *el);

// return number of elements on the stack sh
uint32 stack_elnum(handle sh);

// pop element from the stack
// return 0 on success, non-0 otherwise
sint8 stack_pop(handle sh, void *el);


#endif
