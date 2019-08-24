#include "tests.h"
#include "common/strop.h"
#include "common/dateop.h"
#include "common/decimal.h"
#include "common/error.h"
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
    if(strop_fmt_date(buf, &ptr, 99, _ach("yyyy-mm-dd hh:mi:ss"), 63718704000UL) != 0) return __LINE__;
    if(memcmp(buf, "2020-03-02 00:00:00", 19)) return __LINE__;

    ptr = 0;
    if(strop_fmt_date(buf, &ptr, 99, _ach("yyyy-mm-dd hh:mi:ss"), 0UL) != 0) return __LINE__;
    if(memcmp(buf, "0001-01-01 00:00:00", 19)) return __LINE__;

    ptr = 0;
    if(strop_fmt_date(buf, &ptr, 99, _ach("yyyx.mx.dx hx/mx/sx"), 0UL) != 0) return __LINE__;
    if(memcmp(buf, "yyyx.mx.dx hx/mx/sx", 19)) return __LINE__;

    ptr = 0;
    if(strop_fmt_date(buf, &ptr, 99, _ach("yyyyyyyymmmmdddd hhhh:mimi:ssss"), 3665UL) != 0) return __LINE__;
    if(memcmp(buf, "0001000101010101 0101:0101:0505", 31)) return __LINE__;

    ptr = 0;
    if(strop_fmt_date(buf, &ptr, 99, _ach("ymd hhhmiisss"), 63718704000UL + 86399) != 0) return __LINE__;
    if(memcmp(buf, "ymd 23h59i59s", 12)) return __LINE__;

    ptr = 0;
    if(strop_fmt_date(buf, &ptr, 99, _ach("YYYY MM DD HH MI SS"), 63718704000UL + 86399) != 0) return __LINE__;
    if(memcmp(buf, "YYYY MM DD HH MI SS", 19)) return __LINE__;


    puts("Testing strop_fmt_timestamp");

    ptr = 0;
    if(strop_fmt_timestamp(buf, &ptr, 99, _ach("yyyy-mm-dd hh:mi:ss.ffffff"), (63718704000UL + 86399) * 1000000 + 123456) != 0) return __LINE__;
    if(memcmp(buf, "2020-03-02 23:59:59.123456", 26)) return __LINE__;

    ptr = 0;
    if(strop_fmt_timestamp(buf, &ptr, 99, _ach("yyyy-mm-dd hh:mi:ss.f"), (63718704000UL + 86399) * 1000000 + 723456) != 0) return __LINE__;
    if(memcmp(buf, "2020-03-02 23:59:59.7", 21)) return __LINE__;

    ptr = 0;
    if(strop_fmt_timestamp(buf, &ptr, 99, _ach("yyyy-mm-dd hh:mi:ss.fffffff"), (63718704000UL + 86399) * 1000000 + 123456) != 0) return __LINE__;
    if(memcmp(buf, "2020-03-02 23:59:59.1234561", 27)) return __LINE__;

    ptr = 0;
    if(strop_fmt_timestamp_with_tz(buf, &ptr, 99, _ach("yyyy-mm-dd hh:mi:ss.ffftz"), (63718704000UL + 86399) * 1000000 + 765432, 691) != 0) return __LINE__;
    if(memcmp(buf, "2020-03-02 23:59:59.765+11:31", 29)) return __LINE__;

    ptr = 0;
    if(strop_fmt_timestamp_with_tz(buf, &ptr, 99, _ach("yyyy-mm-dd hh:mi:ss.ffftz"), (63718704000UL + 86399) * 1000000 + 123456, -691) != 0) return __LINE__;
    if(memcmp(buf, "2020-03-02 23:59:59.123-11:31", 29)) return __LINE__;


    puts("Testing strop_fmt_decimal");

    decimal q;
    const achar* fmt = _ach("");

    memset(&q, 0, sizeof(q));
    q.sign = DECIMAL_SIGN_NEG;
    q.m[0] = 1234;
    q.m[1] = 5678;
    q.m[2] = 0;
    q.m[3] = 9;
    q.n = 13;
    q.e = -8;
    ptr = 0;
    memset(buf, 0, 100);
    if(strop_fmt_decimal(buf, &ptr, 99, fmt, &q) != 0) return __LINE__;
    if(ptr != 1) return __LINE__;
    if(memcmp(buf, "?", 1)) return __LINE__;

    fmt = _ach("dddd");
    ptr = 0;
    memset(buf, 0, 100);
    if(strop_fmt_decimal(buf, &ptr, 99, fmt, &q) != 0) return __LINE__;
    if(ptr != 1) return __LINE__;
    if(memcmp(buf, "?", 1)) return __LINE__;

    // basic format (without exponent)
    fmt = _ach("ddddd");
    ptr = 0;
    memset(buf, 0, 100);
    if(strop_fmt_decimal(buf, &ptr, 99, fmt, &q) != 0) return __LINE__;
    if(ptr != 6) return __LINE__;
    if(memcmp(buf, "-90000", 6)) return __LINE__;

    ptr = 0;
    q.sign = DECIMAL_SIGN_POS;
    memset(buf, 0, 100);
    if(strop_fmt_decimal(buf, &ptr, 99, fmt, &q) != 0) return __LINE__;
    if(ptr != 6) return __LINE__;
    if(memcmp(buf, " 90000", 6)) return __LINE__;

    ptr = 0;
    fmt = _ach("dddddd");
    memset(buf, 0, 100);
    if(strop_fmt_decimal(buf, &ptr, 99, fmt, &q) != 0) return __LINE__;
    if(ptr != 6) return __LINE__;
    if(memcmp(buf, " 90000", 6)) return __LINE__;

    ptr = 0;
    fmt = _ach("ddddddds");
    memset(buf, 0, 100);
    if(strop_fmt_decimal(buf, &ptr, 99, fmt, &q) != 0) return __LINE__;
    if(ptr != 6) return __LINE__;
    if(memcmp(buf, " 90000", 6)) return __LINE__;

    ptr = 0;
    fmt = _ach("dddddddsdd");
    memset(buf, 0, 100);
    if(strop_fmt_decimal(buf, &ptr, 99, fmt, &q) != 0) return __LINE__;
    if(ptr != 9) return __LINE__;
    if(memcmp(buf, " 90000.56", 9)) return __LINE__;

    ptr = 0;
    fmt = _ach("dddddddsdddddddddd");
    memset(buf, 0, 100);
    if(strop_fmt_decimal(buf, &ptr, 99, fmt, &q) != 0) return __LINE__;
    if(ptr != 17) return __LINE__;
    if(memcmp(buf, " 90000.5678123400", 17)) return __LINE__;

    ptr = 0;
    q.e = -15;
    fmt = _ach("sdddddddddd");
    memset(buf, 0, 100);
    if(strop_fmt_decimal(buf, &ptr, 99, fmt, &q) != 0) return __LINE__;
    if(ptr != 12) return __LINE__;
    if(memcmp(buf, " .0090000567", 12)) return __LINE__;

    ptr = 0;
    q.sign = DECIMAL_SIGN_NEG;
    fmt = _ach("sdddddddddd");
    memset(buf, 0, 100);
    if(strop_fmt_decimal(buf, &ptr, 99, fmt, &q) != 0) return __LINE__;
    if(ptr != 12) return __LINE__;
    if(memcmp(buf, "-.0090000567", 12)) return __LINE__;

    ptr = 0;
    fmt = _ach("dddsdddddddddd");
    memset(buf, 0, 100);
    if(strop_fmt_decimal(buf, &ptr, 99, fmt, &q) != 0) return __LINE__;
    if(ptr != 13) return __LINE__;
    if(memcmp(buf, "-0.0090000567", 13)) return __LINE__;

    ptr = 0;
    q.sign = DECIMAL_SIGN_POS;
    fmt = _ach("dddsdddddddddd");
    memset(buf, 0, 100);
    if(strop_fmt_decimal(buf, &ptr, 99, fmt, &q) != 0) return __LINE__;
    if(ptr != 13) return __LINE__;
    if(memcmp(buf, " 0.0090000567", 13)) return __LINE__;

    // format with exponent
    ptr = 0;
    q.e = -8;
    fmt = _ach("ddddde");
    memset(buf, 0, 100);
    if(strop_fmt_decimal(buf, &ptr, 99, fmt, &q) != 0) return __LINE__;
    if(ptr != 9) return __LINE__;
    if(memcmp(buf, " 90000e+0", 9)) return __LINE__;

    q.sign = DECIMAL_SIGN_NEG;
    ptr = 0;
    fmt = _ach("ddde");
    memset(buf, 0, 100);
    if(strop_fmt_decimal(buf, &ptr, 99, fmt, &q) != 0) return __LINE__;
//    printf("ptr: %d\n", ptr);
//    buf[ptr] = '\0'; printf("buf: %s\n", buf);
    if(ptr != 7) return __LINE__;
    if(memcmp(buf, "-900e+2", 7)) return __LINE__;

    ptr = 0;
    fmt = _ach("dddddddse");
    memset(buf, 0, 100);
    if(strop_fmt_decimal(buf, &ptr, 99, fmt, &q) != 0) return __LINE__;
    if(ptr != 11) return __LINE__;
    if(memcmp(buf, "-9000056e-2", 11)) return __LINE__;

    ptr = 0;
    fmt = _ach("dddddddsdddddddddde");
    memset(buf, 0, 100);
    if(strop_fmt_decimal(buf, &ptr, 99, fmt, &q) != 0) return __LINE__;
    if(ptr != 22) return __LINE__;
    if(memcmp(buf, "-9000056.7812340000e-2", 22)) return __LINE__;

    ptr = 0;
    fmt = _ach("sdddddddddde");
    memset(buf, 0, 100);
    if(strop_fmt_decimal(buf, &ptr, 99, fmt, &q) != 0) return __LINE__;
    if(ptr != 15) return __LINE__;
    if(memcmp(buf, "-.9000056781e+5", 15)) return __LINE__;

    ptr = 0;
    q.sign = DECIMAL_SIGN_POS;
    fmt = _ach("sdddddddddde");
    memset(buf, 0, 100);
    if(strop_fmt_decimal(buf, &ptr, 99, fmt, &q) != 0) return __LINE__;
    if(ptr != 15) return __LINE__;
    if(memcmp(buf, " .9000056781e+5", 15)) return __LINE__;

    ptr = 0;
    fmt = _ach("dsdddddddddde");
    memset(buf, 0, 100);
    if(strop_fmt_decimal(buf, &ptr, 99, fmt, &q) != 0) return __LINE__;
    if(ptr != 16) return __LINE__;
    if(memcmp(buf, " 9.0000567812e+4", 16)) return __LINE__;

    // format zero
    memset(&q, 0, sizeof(q));
    q.sign = DECIMAL_SIGN_POS;

    ptr = 0;
    fmt = _ach("");
    memset(buf, 0, 100);
    if(strop_fmt_decimal(buf, &ptr, 99, fmt, &q) != 0) return __LINE__;
    if(ptr != 0) return __LINE__;

    ptr = 0;
    fmt = _ach("s");
    memset(buf, 0, 100);
    if(strop_fmt_decimal(buf, &ptr, 99, fmt, &q) != 0) return __LINE__;
    if(ptr != 0) return __LINE__;

    ptr = 0;
    fmt = _ach("d");
    memset(buf, 0, 100);
    if(strop_fmt_decimal(buf, &ptr, 99, fmt, &q) != 0) return __LINE__;
    if(ptr != 1) return __LINE__;
    if(memcmp(buf, "0", 1)) return __LINE__;

    ptr = 0;
    fmt = _ach("ds");
    memset(buf, 0, 100);
    if(strop_fmt_decimal(buf, &ptr, 99, fmt, &q) != 0) return __LINE__;
    if(ptr != 1) return __LINE__;
    if(memcmp(buf, "0", 1)) return __LINE__;

    ptr = 0;
    fmt = _ach("dsd");
    memset(buf, 0, 100);
    if(strop_fmt_decimal(buf, &ptr, 99, fmt, &q) != 0) return __LINE__;
    if(ptr != 3) return __LINE__;
    if(memcmp(buf, "0.0", 3)) return __LINE__;

    ptr = 0;
    fmt = _ach("ddddsdddd");
    memset(buf, 0, 100);
    if(strop_fmt_decimal(buf, &ptr, 99, fmt, &q) != 0) return __LINE__;
    if(ptr != 6) return __LINE__;
    if(memcmp(buf, "0.0000", 6)) return __LINE__;

    ptr = 0;
    fmt = _ach("sddd");
    memset(buf, 0, 100);
    if(strop_fmt_decimal(buf, &ptr, 99, fmt, &q) != 0) return __LINE__;
    if(ptr != 4) return __LINE__;
    if(memcmp(buf, ".000", 4)) return __LINE__;

    ptr = 0;
    fmt = _ach("e");
    memset(buf, 0, 100);
    if(strop_fmt_decimal(buf, &ptr, 99, fmt, &q) != 0) return __LINE__;
    if(ptr != 3) return __LINE__;
    if(memcmp(buf, "e+0", 3)) return __LINE__;

    ptr = 0;
    fmt = _ach("se");
    memset(buf, 0, 100);
    if(strop_fmt_decimal(buf, &ptr, 99, fmt, &q) != 0) return __LINE__;
    if(ptr != 3) return __LINE__;
    if(memcmp(buf, "e+0", 3)) return __LINE__;

    ptr = 0;
    fmt = _ach("de");
    memset(buf, 0, 100);
    if(strop_fmt_decimal(buf, &ptr, 99, fmt, &q) != 0) return __LINE__;
    if(ptr != 4) return __LINE__;
    if(memcmp(buf, "0e+0", 4)) return __LINE__;

    ptr = 0;
    fmt = _ach("dse");
    memset(buf, 0, 100);
    if(strop_fmt_decimal(buf, &ptr, 99, fmt, &q) != 0) return __LINE__;
    if(ptr != 4) return __LINE__;
    if(memcmp(buf, "0e+0", 4)) return __LINE__;

    ptr = 0;
    fmt = _ach("dsde");
    memset(buf, 0, 100);
    if(strop_fmt_decimal(buf, &ptr, 99, fmt, &q) != 0) return __LINE__;
    if(ptr != 6) return __LINE__;
    if(memcmp(buf, "0.0e+0", 6)) return __LINE__;

    ptr = 0;
    fmt = _ach("ddddsdddde");
    memset(buf, 0, 100);
    if(strop_fmt_decimal(buf, &ptr, 99, fmt, &q) != 0) return __LINE__;
    if(ptr != 9) return __LINE__;
    if(memcmp(buf, "0.0000e+0", 9)) return __LINE__;

    ptr = 0;
    fmt = _ach("sddde");
    memset(buf, 0, 100);
    if(strop_fmt_decimal(buf, &ptr, 99, fmt, &q) != 0) return __LINE__;
    if(ptr != 7) return __LINE__;
    if(memcmp(buf, ".000e+0", 7)) return __LINE__;

    // wrong parameters
    ptr = 0;
    fmt = _ach("dsed");
    memset(buf, 0, 100);
    if(strop_fmt_decimal(buf, &ptr, 99, fmt, &q) == 0) return __LINE__;
    if(error_get() != ERROR_INVALID_DECIMAL_FORMAT) return __LINE__;

    ptr = 0;
    fmt = _ach("dsse");
    memset(buf, 0, 100);
    if(strop_fmt_decimal(buf, &ptr, 99, fmt, &q) == 0) return __LINE__;
    if(error_get() != ERROR_INVALID_DECIMAL_FORMAT) return __LINE__;

    ptr = 0;
    fmt = _ach("dsee");
    memset(buf, 0, 100);
    if(strop_fmt_decimal(buf, &ptr, 99, fmt, &q) == 0) return __LINE__;
    if(error_get() != ERROR_INVALID_DECIMAL_FORMAT) return __LINE__;

    ptr = 0;
    fmt = _ach("dsd-");
    memset(buf, 0, 100);
    if(strop_fmt_decimal(buf, &ptr, 99, fmt, &q) == 0) return __LINE__;
    if(error_get() != ERROR_INVALID_DECIMAL_FORMAT) return __LINE__;

    return 0;
}
