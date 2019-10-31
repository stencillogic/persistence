#ifndef _STRING_LITERAL_H
#define _STRING_LITERAL_H


// string representation and functions


#include "defs/defs.h"
#include "common/encoding.h"


// size for string_literal
size_t string_literal_alloc_sz();

// create string literal instance
// return NULL on error
handle string_literal_create(void *buf);

// truncate
// return 0 on success, non-0 on error
sint8 string_literal_truncate(handle sh);

// append buffer to string literal
// return 0 on success, 1 on error
sint8 string_literal_append_char(handle sh, uint8 *buf, uint32 sz);

// move ownership from one string literal to another
// from is trucated
// return 0 on success, non-0 on error
sint8 string_literal_move(handle from, handle to);


#endif
