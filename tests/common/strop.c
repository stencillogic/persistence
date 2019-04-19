#include "tests.h"
#include "common/strop.h"
#include "common/dateop.h"
#include <string.h>
#include <stdio.h>

int test_strop_functions()
{
    puts("Starting test test_strop_functions");
    encoding_init();
    dateop_set_calendar(DATEOP_CALENDAR_GRIGORIAN);
    strop_set_encoding(ENCODING_UTF8);

    uint8 buf[100];
    uint32 ptr = 0;
    uint64 num;


    puts("Testing strop_fmt_uint64");

    if(strop_fmt_uint64(buf, &ptr, 16, 1234567890123456UL) != 0) return __LINE__;
    if(ptr != 16) return __LINE__;
    if(memcmp(buf, "1234567890123456", 16)) return __LINE__;

    ptr = 0;
    if(strop_fmt_uint64(buf, &ptr, 16, 0UL) != 0) return __LINE__;
    if(ptr != 1) return __LINE__;
    if(memcmp(buf, "0", 1)) return __LINE__;

    ptr = 0;
    if(strop_fmt_uint64(buf, &ptr, 16, 9UL) != 0) return __LINE__;
    if(ptr != 1) return __LINE__;
    if(memcmp(buf, "9", 1)) return __LINE__;

    ptr = 1;
    if(strop_fmt_uint64(buf, &ptr, 16, 1234567890123456UL) == 0) return __LINE__;
    ptr = 0;
    if(strop_fmt_uint64(buf, &ptr, 0, 1234567890123456UL) == 0) return __LINE__;

    ptr = 0;
    memset(&num, 0xff, sizeof(uint64));
    if(strop_fmt_uint64(buf, &ptr, 100, num) != 0) return __LINE__;
    if(ptr != 20) return __LINE__;


    puts("Testing strop_fmt_uint64_padded");

    ptr = 0;
    if(strop_fmt_uint64_padded(buf, &ptr, 16, 1234567890123456UL, 16) != 0) return __LINE__;
    if(ptr != 16) return __LINE__;
    if(memcmp(buf, "1234567890123456", 16)) return __LINE__;

    ptr = 0;
    if(strop_fmt_uint64_padded(buf, &ptr, 16, 0UL, 1) != 0) return __LINE__;
    if(ptr != 1) return __LINE__;
    if(memcmp(buf, "0", 1)) return __LINE__;

    ptr = 0;
    if(strop_fmt_uint64_padded(buf, &ptr, 16, 9UL, 1) != 0) return __LINE__;
    if(ptr != 1) return __LINE__;
    if(memcmp(buf, "9", 1)) return __LINE__;

    ptr = 1;
    if(strop_fmt_uint64_padded(buf, &ptr, 16, 1234567890123456UL, 16) == 0) return __LINE__;
    ptr = 0;
    if(strop_fmt_uint64_padded(buf, &ptr, 0, 1234567890123456UL, 1) == 0) return __LINE__;
    ptr = 0;
    if(strop_fmt_uint64_padded(buf, &ptr, 0, 1234567890123456UL, 0) != 0) return __LINE__;
    ptr = 10;
    if(strop_fmt_uint64_padded(buf, &ptr, 5, 1234567890123456UL, 1) != 0) return __LINE__;

    ptr = 0;
    memset(&num, 0xff, sizeof(uint64));
    if(strop_fmt_uint64_padded(buf, &ptr, 100, num, 20) != 0) return __LINE__;
    if(ptr != 20) return __LINE__;

    ptr = 0;
    if(strop_fmt_uint64_padded(buf, &ptr, 16, 0UL, 5) != 0) return __LINE__;
    if(ptr != 5) return __LINE__;
    if(memcmp(buf, "00000", 5)) return __LINE__;

    ptr = 0;
    if(strop_fmt_uint64_padded(buf, &ptr, 16, 123321UL, 3) != 0) return __LINE__;
    if(ptr != 3) return __LINE__;
    if(memcmp(buf, "321", 3)) return __LINE__;

    ptr = 0;
    if(strop_fmt_uint64_padded(buf, &ptr, 16, 92UL, 4) != 0) return __LINE__;
    if(ptr != 4) return __LINE__;
    if(memcmp(buf, "0092", 4)) return __LINE__;

    ptr = 10;
    if(strop_fmt_uint64_padded(buf, &ptr, 16, 92UL, 4) != 0) return __LINE__;
    if(ptr != 14) return __LINE__;
    if(memcmp(buf+10, "0092", 4)) return __LINE__;


    puts("Testing strop_fmt_date");

    ptr = 0;
    if(strop_fmt_date(buf, &ptr, 99, _ach("yyyy-mm-dd hh:mi:ss"), 63718704000UL) != 0) return 1;
    if(memcmp(buf, "2020-03-02 00:00:00", 19)) return __LINE__;

    ptr = 0;
    if(strop_fmt_date(buf, &ptr, 99, _ach("yyyy-mm-dd hh:mi:ss"), 0UL) != 0) return 1;
    if(memcmp(buf, "0001-01-01 00:00:00", 19)) return __LINE__;

    ptr = 0;
    if(strop_fmt_date(buf, &ptr, 99, _ach("yyyx.mx.dx hx/mx/sx"), 0UL) != 0) return 1;
    if(memcmp(buf, "yyyx.mx.dx hx/mx/sx", 19)) return __LINE__;

    ptr = 0;
    if(strop_fmt_date(buf, &ptr, 99, _ach("yyyyyyyymmmmdddd hhhh:mimi:ssss"), 3665UL) != 0) return 1;
    if(memcmp(buf, "0001000101010101 0101:0101:0505", 31)) return __LINE__;

    ptr = 0;
    if(strop_fmt_date(buf, &ptr, 99, _ach("ymd hhhmiisss"), 63718704000UL + 86399) != 0) return 1;
    if(memcmp(buf, "ymd 23h59i59s", 12)) return __LINE__;

    ptr = 0;
    if(strop_fmt_date(buf, &ptr, 99, _ach("YYYY MM DD HH MI SS"), 63718704000UL + 86399) != 0) return 1;
    if(memcmp(buf, "YYYY MM DD HH MI SS", 19)) return __LINE__;

    return 0;
}
