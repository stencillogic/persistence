#ifndef _STROP_H
#define _STROP_H

// text string operations

#include "common/encoding.h"

// set current encoding
void strop_set_encoding(encoding enc);

// length of the string in characters excluding terminating character
uint64 strop_len(const void *str);

// compare two strings
// return an -1, 0, 1 if, respectively, str1 < str2, str1 == str2, str1 > str2
sint8 strop_cmp(const void *str1, const void *str2);

#endif
