#include "tests.h"
#include "common/string_literal.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int test_string_literal_functions()
{
    puts("Starting test test_string_literal_functions");

    uint8 buf[1024];
    uint64 sz, len;
    const achar *str = _ach("some test data string");
    sint8 ret;


    // create
    handle strlit = string_literal_create(malloc(string_literal_alloc_sz()));
    if(NULL == strlit) return __LINE__;
    handle strlit2 = string_literal_create(malloc(string_literal_alloc_sz()));
    if(NULL == strlit2) return __LINE__;


    // write and read, determine size
    if(0 != string_literal_truncate(strlit)) return __LINE__;

    len = strlen(str);
    strcpy((char *)buf, str);
    if(0 != string_literal_append_char(strlit, buf, len)) return __LINE__;
    if(0 != string_literal_byte_length(strlit, &sz)) return __LINE__;
    if(sz != len) return __LINE__;

    sz = 10;
    memset(buf, 0, 1024);
    if(0 != string_literal_read(strlit, buf, &sz)) return __LINE__;
    if(sz != 10) return __LINE__;
    if(memcmp(buf, str, sz)) return __LINE__;

    sz = len+1;
    memset(buf, 0, 1024);
    if(0 != string_literal_read(strlit, buf, &sz)) return __LINE__;
    if(sz != len) return __LINE__;
    if(memcmp(buf, str, sz)) return __LINE__;


    // truncate
    if(0 != string_literal_truncate(strlit)) return __LINE__;
    if(0 != string_literal_byte_length(strlit, &sz)) return __LINE__;
    if(sz != 0) return __LINE__;

    sz = 10;
    if(0 != string_literal_read(strlit, buf, &sz)) return __LINE__;
    if(sz != 0) return __LINE__;


    // move
    len = strlen(str);
    strcpy((char *)buf, str);
    if(0 != string_literal_append_char(strlit, buf, len)) return __LINE__;
    if(0 != string_literal_byte_length(strlit, &sz)) return __LINE__;
    if(sz != len) return __LINE__;

    sz = len;
    memset(buf, 0, 1024);
    if(0 != string_literal_read(strlit, buf, &sz)) return __LINE__;
    if(sz != len) return __LINE__;
    if(memcmp(buf, str, sz)) return __LINE__;

    if(0 != string_literal_move(strlit, strlit2)) return __LINE__;

    if(0 != string_literal_byte_length(strlit, &sz)) return __LINE__;
    if(sz != 0) return __LINE__;

    sz = 10;
    if(0 != string_literal_read(strlit, buf, &sz)) return __LINE__;
    if(sz != 0) return __LINE__;

    if(0 != string_literal_byte_length(strlit2, &sz)) return __LINE__;
    if(sz != len) return __LINE__;

    sz = len;
    memset(buf, 0, 1024);
    if(0 != string_literal_read(strlit2, buf, &sz)) return __LINE__;
    if(sz != len) return __LINE__;
    if(memcmp(buf, str, sz)) return __LINE__;


    // compare
    len = strlen(str);
    strcpy((char *)buf, str);
    if(0 != string_literal_append_char(strlit, buf, len)) return __LINE__;

    ret = 1;
    if(0 != string_literal_byte_compare(strlit, strlit2, &ret)) return __LINE__;
    if(ret != 0) return __LINE__;

    buf[0] = _ach('a');
    if(0 != string_literal_append_char(strlit, buf, 1)) return __LINE__;

    ret = 0;
    if(0 != string_literal_byte_compare(strlit, strlit2, &ret)) return __LINE__;
    if(ret != 1) return __LINE__;

    buf[0] = _ach('a');
    buf[1] = _ach('x');
    if(0 != string_literal_append_char(strlit2, buf, 2)) return __LINE__;

    ret = 0;
    if(0 != string_literal_byte_compare(strlit, strlit2, &ret)) return __LINE__;
    if(ret != -1) return __LINE__;

    buf[0] = _ach('a');
    if(0 != string_literal_append_char(strlit, buf, 1)) return __LINE__;

    ret = 0;
    if(0 != string_literal_byte_compare(strlit, strlit2, &ret)) return __LINE__;
    if(ret != -1) return __LINE__;

    buf[0] = _ach('z');
    if(0 != string_literal_append_char(strlit, buf, 1)) return __LINE__;

    ret = 0;
    if(0 != string_literal_byte_compare(strlit, strlit2, &ret)) return __LINE__;
    if(ret != -1) return __LINE__;

    ret = 0;
    if(0 != string_literal_byte_compare(strlit2, strlit, &ret)) return __LINE__;
    if(ret != 1) return __LINE__;

    return 0;
}
