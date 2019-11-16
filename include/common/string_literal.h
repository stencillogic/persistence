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
sint8 string_literal_append_char(handle sh, const uint8 *buf, uint32 sz);

// move ownership from one string literal to another
// from is trucated
// return 0 on success, non-0 on error
sint8 string_literal_move(handle from, handle to);

// read data of size sz to buf from string literal
// upon completion sz is set to the number of bytes read
// return 0 on success, non-0 on error
sint8 string_literal_read(handle sh, uint8 *buf, uint64 *sz);

// binary compare of two string literals
// ret is 0 if equal, 1 if sh1 greater then sh2, -1 if sh2 greater than sh1
// return 0 on success, non-0 on error
sint8 string_literal_byte_compare(handle sh1, handle sh2, sint8 *ret);

// return byte length of the string literal
// return 0 on success, non-0 on error
sint8 string_literal_byte_length(handle sh1, uint64 *len);


#endif
