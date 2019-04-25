#ifndef _STROP_H
#define _STROP_H

// text string operations

#include "defs/defs.h"
#include "common/encoding.h"
#include "common/decimal.h"

// set current encoding
void strop_set_encoding(encoding enc);

// length of the string in characters excluding terminating character
uint64 strop_len(const void *str);

// compare two strings
// return an -1, 0, 1 if, respectively, str1 < str2, str1 == str2, str1 > str2
sint8 strop_cmp(const void *str1, const void *str2);

// format uint64 number to string, put in buf starting from ptr, adjust ptr to point at the end of formatted value
// return 0 on success, non-0 on error
sint8 strop_fmt_uint64(uint8* buf, uint32 *ptr, uint32 sz, uint64 number);

// format uint64 number to string, put in buf starting from ptr, adjust ptr to point at the end of formatted value
// if number length less than pad, add 0 on left size to pad to desired padding length
// return 0 on success, non-0 on buffer overflow
sint8 strop_fmt_uint64_padded(uint8* buf, uint32 *ptr, uint32 sz, uint64 number, uint32 pad);

// format date value according to fmt and put result to buf + start
// return 0 on success, non-0 on error
// start value updated to point after formatted date
// format:
//   yyyy:  4-number year
//   mm:    2-number month
//   dd:    day of month
//   hh:    hour from 0 to 23
//   mi:    minutes from 0 to 59
//   ss:    seconds from 0 to 59
sint8 strop_fmt_date(uint8 *buf, uint32 *start, uint32 bufsz, const achar* fmt, uint64 date);

// format decimal value and put result to buf + start
// return 0 on success, non-0 on error
// start value updated to point after formatted date
sint8 strop_fmt_decimal(uint8 *buf, uint32 *start, uint32 bufsz, decimal *d);

#endif
