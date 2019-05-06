#include "tests.h"
#include "common/decimal.h"
#include "common/error.h"
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <unistd.h>

int test_decimal_functions()
{
    decimal d1, d2, d3, ref;

    if(DECIMAL_PARTS < 10) return __LINE__;
    memset(d1.m, 0, sizeof(d1.m)); d1.sign = DECIMAL_SIGN_POS; d1.e = 0; d1.n = 0;
    memset(d2.m, 0, sizeof(d2.m)); d2.sign = DECIMAL_SIGN_POS; d2.e = 0; d2.n = 0;
    memset(d3.m, 0, sizeof(d3.m)); d3.sign = DECIMAL_SIGN_POS; d3.e = 0; d3.n = 0;
    memset(ref.m, 0, sizeof(ref.m)); ref.sign = DECIMAL_SIGN_POS; ref.e = 0; ref.n = 0;

    puts("Testing decimal comparisons");

    if(decimal_cmp(&d1, &d2) != 0) return __LINE__;

    d1.m[0] = 1;    // 1, 0
    d1.n = 1;
    if(decimal_cmp(&d1, &d2) <= 0) return __LINE__;

    d2.m[0] = 2;    // 1, 2
    d2.n = 1;
    if(decimal_cmp(&d1, &d2) >= 0) return __LINE__;

    d1.sign = DECIMAL_SIGN_NEG; // -1, 2
    if(decimal_cmp(&d1, &d2) >= 0) return __LINE__;

    d2.sign = DECIMAL_SIGN_NEG; // -1, -2
    if(decimal_cmp(&d1, &d2) <= 0) return __LINE__;

    d1.sign = DECIMAL_SIGN_POS; // 1, -2
    if(decimal_cmp(&d1, &d2) <= 0) return __LINE__;

    d1.m[0] = 3;    // 3, -2
    if(decimal_cmp(&d1, &d2) <= 0) return __LINE__;

    d2.sign = DECIMAL_SIGN_POS;    // 3, 2
    if(decimal_cmp(&d1, &d2) <= 0) return __LINE__;

    d2.m[1] = 9;    // 3, 90002
    d2.n = 5;
    if(decimal_cmp(&d1, &d2) >= 0) return __LINE__;

    d1.m[1] = 9;    // 90003, 90002
    d1.n = 5;
    if(decimal_cmp(&d1, &d2) <= 0) return __LINE__;

    d2.m[0] = 3;    // 90003, 90003
    if(decimal_cmp(&d1, &d2) != 0) return __LINE__;

    d1.sign = d2.sign = DECIMAL_SIGN_POS;    // -90003, -90003
    if(decimal_cmp(&d1, &d2) != 0) return __LINE__;


    puts("Testing decimal additions");

    d2.sign = d1.sign = DECIMAL_SIGN_POS;
    for(int i=0; i<DECIMAL_PARTS; i++)
    {
        d1.m[i] = 9999;
        d2.m[i] = 0;
    }
    d1.m[0] = 9990;
    d2.m[0] = 10;
    d1.n = DECIMAL_POSITIONS;
    d2.n = 2;
    d2.e = d1.e = DECIMAL_MAX_EXPONENT;

    error_set(ERROR_NO_ERROR);
    if(decimal_add(&d1, &d2, &d3) == 0) return __LINE__;
    if(ERROR_DECIMAL_OVERFLOW != error_get()) return __LINE__;

    d2.sign = d1.sign = DECIMAL_SIGN_NEG;
    error_set(ERROR_NO_ERROR);
    if(decimal_add(&d1, &d2, &d3) == 0) return __LINE__;
    if(ERROR_DECIMAL_OVERFLOW != error_get()) return __LINE__;

    d2.e = d1.e = 0;
    memset(d3.m, 0, sizeof(d3.m)); d3.sign = DECIMAL_SIGN_POS; d3.e = 123; d3.n = 123;
    memset(ref.m, 0, sizeof(ref.m));
    ref.m[DECIMAL_PARTS - 1] = DECIMAL_BASE / 10;
    ref.sign = DECIMAL_SIGN_NEG;
    ref.n = DECIMAL_POSITIONS;
    ref.e = 1;
    if(decimal_add(&d1, &d2, &d3) != 0) return __LINE__;
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;

    d2.sign = DECIMAL_SIGN_POS;
    memset(d3.m, 0, sizeof(d3.m)); d3.sign = DECIMAL_SIGN_POS; d3.e = 123; d3.n = 123;
    ref.sign = DECIMAL_SIGN_NEG;
    for(int i=0; i<DECIMAL_PARTS; i++)
        ref.m[i] = 9999;
    ref.m[0] = 9980;
    ref.n = DECIMAL_POSITIONS;
    ref.e = 0;
    if(decimal_add(&d1, &d2, &d3) != 0) return __LINE__;
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;

    memset(d3.m, 0, sizeof(d3.m)); d3.sign = DECIMAL_SIGN_NEG; d3.e = 123; d3.n = 123;
    ref.sign = d1.sign = DECIMAL_SIGN_POS;
    d2.sign = DECIMAL_SIGN_NEG;
    if(decimal_add(&d1, &d2, &d3) != 0) return __LINE__;
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;

    memset(d1.m, 0, sizeof(d1.m)); d1.sign = DECIMAL_SIGN_POS; d1.e = 0; d1.n = 0;
    memset(d2.m, 0, sizeof(d2.m)); d2.sign = DECIMAL_SIGN_POS; d2.e = 0; d2.n = 0;
    memset(d3.m, 0, sizeof(d3.m)); d3.sign = DECIMAL_SIGN_POS; d3.e = 0; d3.n = 0;
    memset(ref.m, 0, sizeof(ref.m)); ref.sign = DECIMAL_SIGN_POS; ref.e = 0; ref.n = 0;
    d1.m[1] = 9999;
    d2.m[1] = 9999;
    ref.m[1] = 9998;
    ref.m[2] = 1;
    d1.n = d2.n = 8;
    ref.n = 9;
    if(decimal_add(&d1, &d2, &d3) != 0) return __LINE__;
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;

    // 0 + d2
    d1.m[1] = 0;
    d1.n = 0;
    d2.e = 3;
    memcpy(&ref, &d2, sizeof(ref));
    memset(d3.m, 0, sizeof(d3.m)); d3.sign = DECIMAL_SIGN_NEG; d3.e = 123; d3.n = 123;
    if(decimal_add(&d1, &d2, &d3) != 0) return __LINE__;
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;

    // d1 + 0
    d2.m[1] = 0;
    d2.n = 0;
    d2.e = 0;
    d1.m[1] = 11;
    d1.n = 6;
    d1.e = 123;
    memcpy(&ref, &d1, sizeof(ref));
    memset(d3.m, 0, sizeof(d3.m)); d3.sign = DECIMAL_SIGN_NEG; d3.e = 123; d3.n = 123;
    if(decimal_add(&d1, &d2, &d3) != 0) return __LINE__;
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;

    // different exponents and precisions
    memset(d1.m, 0, sizeof(d1.m)); d1.sign = DECIMAL_SIGN_POS; d1.e = 0; d1.n = 0;
    memset(d2.m, 0, sizeof(d2.m)); d2.sign = DECIMAL_SIGN_POS; d2.e = 0; d2.n = 0;
    memset(d3.m, 0, sizeof(d3.m)); d3.sign = DECIMAL_SIGN_POS; d3.e = 0; d3.n = 0;
    memset(ref.m, 0, sizeof(ref.m)); ref.sign = DECIMAL_SIGN_POS; ref.e = 0; ref.n = 0;

    for(int i=0; i<DECIMAL_PARTS; i++)
    {
        d1.m[i] = 9999;
    }
    d1.n = DECIMAL_POSITIONS;
    d1.e = 5;
    d2.m[0] = 11;
    d2.n = 2;
    d2.e = 3;
    memcpy(&ref, &d1, sizeof(ref));
    memset(d3.m, 0, sizeof(d3.m)); d3.sign = DECIMAL_SIGN_NEG; d3.e = 0; d3.n = 0;
    if(decimal_add(&d1, &d2, &d3) != 0) return __LINE__;
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;

    d1.e = 4;
    d2.m[0] = 11;
    d2.n = 2;
    d2.e = 3;
    memset(ref.m, 0, sizeof(ref.m));
    ref.e = 5;
    ref.n = DECIMAL_POSITIONS;
    ref.m[DECIMAL_PARTS - 1] = DECIMAL_BASE / 10;
    memset(d3.m, 0, sizeof(d3.m)); d3.sign = DECIMAL_SIGN_NEG; d3.e = 0; d3.n = 0;
    if(decimal_add(&d1, &d2, &d3) != 0) return __LINE__;
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;

    d1.e = 5;   // 0000 0022 2..2 2211 0000e+5 + 1122 3346e+3
    for(int i=0; i<DECIMAL_PARTS; i++)
    {
        ref.m[i] = d1.m[i] = 2222;
    }
    d1.m[0] = 0;
    d1.m[1] = 2211;
    d1.m[DECIMAL_PARTS - 1] = 0;
    d1.m[DECIMAL_PARTS - 2] = 22;
    d1.n = DECIMAL_POSITIONS - 6;
    d2.m[0] = 3346;
    d2.m[1] = 1122;
    d2.n = 8;
    d2.e = 3;
    ref.e = 3;
    ref.m[0] = 3346;
    ref.m[DECIMAL_PARTS - 1] = 0;
    ref.n = DECIMAL_POSITIONS - 4;
    memset(d3.m, 0, sizeof(d3.m)); d3.sign = DECIMAL_SIGN_NEG; d3.e = 0; d3.n = 0;
    if(decimal_add(&d1, &d2, &d3) != 0) return __LINE__;
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;

    d1.m[DECIMAL_PARTS - 1] = 222;  // 0222 2..2 2211 0000e+5 + 1122 3346e+3
    d1.m[DECIMAL_PARTS - 2] = 2222;
    d1.n = DECIMAL_POSITIONS - 1;
    ref.e = 4;
    ref.m[0] = 2334;
    ref.m[DECIMAL_PARTS - 1] = 2222;
    ref.n = DECIMAL_POSITIONS;
    memset(d3.m, 0, sizeof(d3.m)); d3.sign = DECIMAL_SIGN_NEG; d3.e = 0; d3.n = 0;
    if(decimal_add(&d1, &d2, &d3) != 0) return __LINE__;
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;


    puts("Testing decimal subtraction");

    d2.sign = DECIMAL_SIGN_POS;
    d1.sign = DECIMAL_SIGN_NEG;
    for(int i=0; i<DECIMAL_PARTS; i++)
    {
        d1.m[i] = 9999;
        d2.m[i] = 0;
    }
    d1.m[0] = 9990;
    d2.m[0] = 10;
    d1.n = DECIMAL_POSITIONS;
    d2.n = 2;
    d1.e = d2.e = DECIMAL_MAX_EXPONENT;

    error_set(ERROR_NO_ERROR);
    if(decimal_sub(&d1, &d2, &d3) == 0) return __LINE__;
    if(ERROR_DECIMAL_OVERFLOW != error_get()) return __LINE__;

    d2.sign = DECIMAL_SIGN_NEG;
    d1.sign = DECIMAL_SIGN_POS;
    error_set(ERROR_NO_ERROR);
    if(decimal_sub(&d1, &d2, &d3) == 0) return __LINE__;
    if(ERROR_DECIMAL_OVERFLOW != error_get()) return __LINE__;

    d1.e--;
    memset(d3.m, 0, sizeof(d3.m)); d3.sign = DECIMAL_SIGN_NEG; d3.e = 123; d3.n = 123;
    memset(ref.m, 0, sizeof(ref.m));
    ref.m[DECIMAL_PARTS - 1] = DECIMAL_BASE / 10;
    ref.m[0] = 9;
    ref.sign = DECIMAL_SIGN_POS;
    ref.n = DECIMAL_POSITIONS;
    ref.e = DECIMAL_MAX_EXPONENT;
    if(decimal_sub(&d1, &d2, &d3) != 0) return __LINE__;
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;

    d1.sign = d2.sign = DECIMAL_SIGN_POS;
    d1.e++;
    memset(d3.m, 1, sizeof(d3.m)); d3.sign = DECIMAL_SIGN_NEG; d3.e = 123; d3.n = 123;
    ref.sign = DECIMAL_SIGN_POS;
    for(int i=0; i<DECIMAL_PARTS; i++)
        ref.m[i] = 9999;
    ref.m[0] = 9980;
    ref.n = DECIMAL_POSITIONS;
    if(decimal_sub(&d1, &d2, &d3) != 0) return __LINE__;
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;

    ref.sign = d1.sign = d2.sign = DECIMAL_SIGN_NEG;
    memset(d3.m, 1, sizeof(d3.m)); d3.sign = DECIMAL_SIGN_POS; d3.e = 123; d3.n = 123;
    if(decimal_sub(&d1, &d2, &d3) != 0) return __LINE__;
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;

    memset(d1.m, 0, sizeof(d1.m)); d1.sign = DECIMAL_SIGN_POS; d1.e = 0; d1.n = 0;
    memset(d2.m, 0, sizeof(d2.m)); d2.sign = DECIMAL_SIGN_POS; d2.e = 0; d2.n = 0;
    memset(d3.m, 0, sizeof(d3.m)); d3.sign = DECIMAL_SIGN_POS; d3.e = 0; d3.n = 0;
    memset(ref.m, 0, sizeof(ref.m)); ref.sign = DECIMAL_SIGN_POS; ref.e = 0; ref.n = 0;
    d1.m[1] = 9998;
    d1.m[2] = 1;
    d1.n = 9;
    d2.m[1] = 9999;
    d2.n = 8;
    ref.m[1] = 9999;
    ref.n = 8;
    if(decimal_sub(&d1, &d2, &d3) != 0) return __LINE__;
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;

    // d1 - 0
    d2.m[1] = 0;
    d2.n = 0;
    d2.e = -5;
    d1.e = 5;
    memcpy(&ref, &d1, sizeof(ref));
    memset(d3.m, 1, sizeof(d3.m)); d3.sign = DECIMAL_SIGN_POS; d3.e = 123; d3.n = 123;
    if(decimal_sub(&d1, &d2, &d3) != 0) return __LINE__;
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;

    // 0 - d2
    d1.m[1] = d1.m[2] = 0;
    d1.n = 0;
    d2.m[3] = 345;
    d2.n = 15;
    memcpy(&ref, &d2, sizeof(ref));
    ref.sign = -ref.sign;
    memset(d3.m, 1, sizeof(d3.m)); d3.sign = DECIMAL_SIGN_POS; d3.e = 123; d3.n = 123;
    if(decimal_sub(&d1, &d2, &d3) != 0) return __LINE__;
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;


    puts("Testing decimal multiplication");

    // 0 * 0
    memset(d1.m, 0, sizeof(d1.m)); d1.sign = DECIMAL_SIGN_POS; d1.e = 0; d1.n = 0;
    memset(d2.m, 0, sizeof(d2.m)); d2.sign = DECIMAL_SIGN_POS; d2.e = 0; d2.n = 0;
    memset(d3.m, 123, sizeof(d3.m)); d3.sign = DECIMAL_SIGN_NEG; d3.e = 123; d3.n = 123;
    memset(ref.m, 0, sizeof(ref.m)); ref.sign = DECIMAL_SIGN_POS; ref.e = 0; ref.n = 0;
    if(decimal_mul(&d1, &d2, &d3) != 0) return __LINE__;
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;

    // 99 0000 * 0
    d1.m[1] = 99;
    d1.n = 6;
    memset(d3.m, 123, sizeof(d3.m)); d3.sign = DECIMAL_SIGN_NEG; d3.e = 123; d3.n = 123;
    if(decimal_mul(&d1, &d2, &d3) != 0) return __LINE__;
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;

    // 0 * 1234 9999
    d2.m[0] = 9999;
    d2.m[1] = 1234;
    d2.n = 8;
    d1.m[0] = d1.m[1] = 0;
    d1.n = 0;
    memset(d3.m, 123, sizeof(d3.m)); d3.sign = DECIMAL_SIGN_NEG; d3.e = 123; d3.n = 123;
    if(decimal_mul(&d1, &d2, &d3) != 0) return __LINE__;
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;

    // 1 * 1
    d1.m[1] = d2.m[1] = 0;
    d2.m[0] = 1;
    d1.m[0] = 1;
    ref.m[0] = 1;
    d1.n = d2.n = ref.n = 1;
    memset(d3.m, 123, sizeof(d3.m)); d3.sign = DECIMAL_SIGN_NEG; d3.e = 123; d3.n = 123;
    if(decimal_mul(&d1, &d2, &d3) != 0) return __LINE__;
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;

    // 1 * -1
    d2.sign = DECIMAL_SIGN_NEG;
    ref.sign = DECIMAL_SIGN_NEG;
    memset(d3.m, 123, sizeof(d3.m)); d3.sign = DECIMAL_SIGN_POS; d3.e = 123; d3.n = 123;
    if(decimal_mul(&d1, &d2, &d3) != 0) return __LINE__;
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;

    // -1 * 1
    d2.sign = DECIMAL_SIGN_POS;
    d1.sign = DECIMAL_SIGN_NEG;
    memset(d3.m, 123, sizeof(d3.m)); d3.sign = DECIMAL_SIGN_POS; d3.e = 123; d3.n = 123;
    if(decimal_mul(&d1, &d2, &d3) != 0) return __LINE__;
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;

    // -1 * -1
    d1.sign = d2.sign = DECIMAL_SIGN_NEG;
    ref.sign = DECIMAL_SIGN_POS;
    memset(d3.m, 123, sizeof(d3.m)); d3.sign = DECIMAL_SIGN_NEG; d3.e = 123; d3.n = 123;
    if(decimal_mul(&d1, &d2, &d3) != 0) return __LINE__;
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;

    // 9999 * 9999
    d2.m[0] = 9999;
    d1.m[0] = 9999;
    d1.n = d2.n = 4;
    ref.m[0] = 1;
    ref.m[1] = 9998;
    ref.n = 8;
    memset(d3.m, 123, sizeof(d3.m)); d3.sign = DECIMAL_SIGN_NEG; d3.e = 123; d3.n = 123;
    if(decimal_mul(&d1, &d2, &d3) != 0) return __LINE__;
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;

    // 2 0000 9999 * 9999
    d1.m[2] = 2;
    d1.n = 9;
    ref.m[0] = 1;
    ref.m[1] = 9998;
    ref.m[2] = 9998;
    ref.m[3] = 1;
    ref.n = 13;
    if(decimal_mul(&d1, &d2, &d3) != 0) return __LINE__;
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;

    // 2 0000 9999 * 5000 0000 9999
    d2.m[2] = 5000;
    d2.n = 12;
    ref.m[0] = 1;
    ref.m[1] = 9998;
    ref.m[2] = 4998;
    ref.m[3] = 5001;
    ref.m[4] = 0;
    ref.m[5] = 1;
    ref.n = 21;
    if(decimal_mul(&d1, &d2, &d3) != 0) return __LINE__;
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;

    // exponent modificaton and overflows
    memset(d1.m, 0, sizeof(d1.m)); d1.sign = DECIMAL_SIGN_POS; d1.e = 0; d1.n = 0;
    memset(d2.m, 0, sizeof(d2.m)); d2.sign = DECIMAL_SIGN_POS; d2.e = 0; d2.n = 0;
    memset(d3.m, 123, sizeof(d3.m)); d3.sign = DECIMAL_SIGN_NEG; d3.e = 123; d3.n = 123;
    memset(ref.m, 0, sizeof(ref.m)); ref.sign = DECIMAL_SIGN_POS; ref.e = 0; ref.n = 0;

    // 5000..0 * 2
    d1.m[DECIMAL_PARTS - 1] = DECIMAL_BASE / 2;
    d1.n = DECIMAL_POSITIONS;
    d2.m[0] = 2;
    d2.n = 1;
    ref.m[DECIMAL_PARTS - 1] = DECIMAL_BASE / 10;
    ref.n = DECIMAL_POSITIONS;
    ref.e = 1;
    if(decimal_mul(&d1, &d2, &d3) != 0) return __LINE__;
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;

    // overflow
    d2.e = 123;
    d1.e = DECIMAL_MAX_EXPONENT - d2.e;
    if(decimal_mul(&d1, &d2, &d3) == 0) return __LINE__;
    if(error_get() != ERROR_DECIMAL_OVERFLOW) return __LINE__;

    // no overflow
    d1.m[DECIMAL_PARTS-1] = DECIMAL_BASE/2-1;
    if(decimal_mul(&d1, &d2, &d3) != 0) return __LINE__;

    // no overflow with negative e
    d1.m[DECIMAL_PARTS-1] = DECIMAL_BASE/2;
    d2.e = -123;
    d1.e = DECIMAL_MIN_EXPONENT + 122;  // d1.e + d2.e = min_exp - 1
    if(decimal_mul(&d1, &d2, &d3) != 0) return __LINE__;

    // overflow with negative e
    d1.m[DECIMAL_PARTS-1] = DECIMAL_BASE/2-1;
    if(decimal_mul(&d1, &d2, &d3) == 0) return __LINE__;
    if(error_get() != ERROR_DECIMAL_OVERFLOW) return __LINE__;

    // overflow
    d2.e = 123;
    d1.e = DECIMAL_MAX_EXPONENT - d2.e;
    d2.m[0] = d1.m[DECIMAL_PARTS-1] = 0;
    d1.m[5] = d2.m[5] = 1;
    d1.n = d2.n = 21;
    if(decimal_mul(&d1, &d2, &d3) == 0) return __LINE__;
    if(error_get() != ERROR_DECIMAL_OVERFLOW) return __LINE__;

    // no overflow
    d1.m[5] = 0;
    d1.m[0] = d1.m[1] = d1.m[2] = d1.m[3] = d1.m[4] = 9999;
    d1.n = 20;
    if(decimal_mul(&d1, &d2, &d3) != 0) return __LINE__;

    // overflow
    for(int i=0; i<DECIMAL_PARTS; i++)
    {
        d1.m[i] = d2.m[i] = 9999;
    }
    d1.n = d2.n = DECIMAL_POSITIONS;
    if(decimal_mul(&d1, &d2, &d3) == 0) return __LINE__;
    if(error_get() != ERROR_DECIMAL_OVERFLOW) return __LINE__;

/*
    puts("Testing decimal multiplication performance");

    decimal decimal_set[10000], *deci, *decj;
    int num_to_read = 2000;
    int rd;
    int fd = open("/home/ivan/projects/cpp/test/gen_decimal_bank/decimal_set", O_RDONLY);
    if(fd == -1) { perror("opening"); return __LINE__; }
    for(int i=0; i<num_to_read; i++)
    {
        decimal_set[i].sign = DECIMAL_SIGN_POS;
        decimal_set[i].e = 0;
        decimal_set[i].n =
        for(int j=9; j>=0; j--)
        {
            rd = read(fd, decimal_set[i].m + j, 2);
            if(rd != 2) { perror("reading"); return __LINE__; }
        }
    }
    close(fd);

    struct timeval t1, t2;
    long elapsed;

    gettimeofday(&t1, NULL);
    for(int i=0; i<num_to_read; i++)
    {
        deci = decimal_set + i;
        for(int j=0; j<num_to_read/10; j++)
        {
            decj = decimal_set + j;
            decimal_mul(decj++, deci, &d3);
            decimal_mul(decj++, deci, &d3);
            decimal_mul(decj++, deci, &d3);
            decimal_mul(decj++, deci, &d3);
            decimal_mul(decj++, deci, &d3);
            decimal_mul(decj++, deci, &d3);
            decimal_mul(decj++, deci, &d3);
            decimal_mul(decj++, deci, &d3);
            decimal_mul(decj++, deci, &d3);
        }
    }
    gettimeofday(&t2, NULL);
    elapsed = (t2.tv_sec - t1.tv_sec) * 1000;
    elapsed += (t2.tv_usec - t1.tv_usec) / 1000;
    printf("Elapsed: %d ms.\n", elapsed);

    d1.sign = DECIMAL_SIGN_POS;
    for(int i=0; i<DECIMAL_PARTS; i++) d1.m[i] = 9999;

    gettimeofday(&t1, NULL);
    for(int i=0; i<10000; i++)
    {
        for(int j=0; j<num_to_read/10; j++)
        {
            decj = decimal_set + j;
            decimal_sub(&d1, decj, &d3);
            decimal_sub(&d1, decj, &d3);
            decimal_sub(&d1, decj, &d3);
            decimal_sub(&d1, decj, &d3);
            decimal_sub(&d1, decj, &d3);
            decimal_sub(&d1, decj, &d3);
            decimal_sub(&d1, decj, &d3);
            decimal_sub(&d1, decj, &d3);
            decimal_sub(&d1, decj, &d3);
        }
    }

    gettimeofday(&t2, NULL);
    elapsed = (t2.tv_sec - t1.tv_sec) * 1000;
    elapsed += (t2.tv_usec - t1.tv_usec) / 1000;
    printf("Elapsed: %d ms.\n", elapsed);
    if(decimal_sub(&d1, &d2, &d3) != 0) return __LINE__;
*/

    puts("Testing decimal division");

    memset(d1.m, 0, sizeof(d1.m)); d1.sign = DECIMAL_SIGN_POS; d1.e = 0; d1.n = 0;
    memset(d2.m, 0, sizeof(d2.m)); d2.sign = DECIMAL_SIGN_POS; d2.e = 0; d2.n = 0;
    memset(d3.m, 123, sizeof(d3.m)); d3.sign = DECIMAL_SIGN_NEG; d3.e = 123; d3.n = 123;
    memset(ref.m, 0, sizeof(ref.m)); ref.sign = DECIMAL_SIGN_POS; ref.e = 0; ref.n = 0;

    // division by zero
    if(decimal_div(&d1, &d2, &d3) == 0) return __LINE__;
    if(error_get() != ERROR_DIVISION_BY_ZERO) return __LINE__;

    // 0 / d2
    d2.m[0] = 1;
    d2.n = 1;
    memset(d3.m, 123, sizeof(d3.m)); d3.sign = DECIMAL_SIGN_NEG; d3.e = 123; d3.n = 123;
    if(decimal_div(&d1, &d2, &d3) != 0) return __LINE__;
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;

    d2.sign = DECIMAL_SIGN_NEG;
    memset(d3.m, 123, sizeof(d3.m)); d3.sign = DECIMAL_SIGN_NEG; d3.e = 123; d3.n = 123;
    if(decimal_div(&d1, &d2, &d3) != 0) return __LINE__;
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;

    // division by single digit
    // 9998 / 3
    ref.sign = d1.sign = d2.sign = DECIMAL_SIGN_POS;
    d2.m[0] = 3;
    d2.n = 1;
    d1.m[0] = 9998;
    d1.n = 4;
    for(int i=0; i<DECIMAL_PARTS-1; i++)
        ref.m[i] = 6666;
    ref.m[DECIMAL_PARTS - 1] = 3332;
    ref.n = DECIMAL_POSITIONS;
    ref.e = -36;
    memset(d3.m, 123, sizeof(d3.m)); d3.sign = DECIMAL_SIGN_NEG; d3.e = 123; d3.n = 123;
    if(decimal_div(&d1, &d2, &d3) != 0) return __LINE__;
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;

    // 1998 / 3
    ref.sign = d1.sign = d2.sign = DECIMAL_SIGN_POS;
    d2.m[0] = 3;
    d2.n = 1;
    d1.m[0] = 1998;
    d1.n = 1;
    for(int i=0; i<DECIMAL_PARTS-1; i++)
        ref.m[i] = 0;
    ref.m[DECIMAL_PARTS - 1] = 666;
    ref.n = DECIMAL_POSITIONS-1;
    ref.e = -36;
    memset(d3.m, 123, sizeof(d3.m)); d3.sign = DECIMAL_SIGN_NEG; d3.e = 123; d3.n = 123;
    if(decimal_div(&d1, &d2, &d3) != 0) return __LINE__;
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;

    // 91998 / 33
    d1.m[1] = 9;
    d1.n = 5;
    d2.m[0] = 33;
    d2.n = 1;
    for(int i=0; i<DECIMAL_PARTS-1; i++)
        ref.m[i] = 8181;
    ref.m[DECIMAL_PARTS - 1] = 2787;
    ref.n = DECIMAL_POSITIONS;
    ref.e = -36;
    memset(d3.m, 123, sizeof(d3.m)); d3.sign = DECIMAL_SIGN_NEG; d3.e = 123; d3.n = 123;
    if(decimal_div(&d1, &d2, &d3) != 0) return __LINE__;
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;

    // 9999 9..9 9999 / 3
    for(int i=0; i<DECIMAL_PARTS; i++)
    {
        d1.m[i] = 9999;
        ref.m[i] = 3333;
    }
    ref.e = 0;
    d1.n = ref.n = DECIMAL_POSITIONS;
    d2.m[0] = 3;
    d2.n = 1;
    memset(d3.m, 123, sizeof(d3.m)); d3.sign = DECIMAL_SIGN_NEG; d3.e = 123; d3.n = 123;
    if(decimal_div(&d1, &d2, &d3) != 0) return __LINE__;
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;

    // 3333 3..3 3333 / 9999
    for(int i=0; i<DECIMAL_PARTS; i++)
    {
        d1.m[i] = 3333;
        ref.m[i] = (i%3 == 1 ? 0 : (i%3 == 2 ? 6667 : 3333));
    }
    d2.m[0] = 9999;
    d2.n = 4;
    ref.e = -4;
    memset(d3.m, 123, sizeof(d3.m)); d3.sign = DECIMAL_SIGN_NEG; d3.e = 123; d3.n = 123;
    if(decimal_div(&d1, &d2, &d3) != 0) return __LINE__;
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;

    // signs again but with non zero quotinent
    ref.sign = d2.sign = DECIMAL_SIGN_NEG;
    memset(d3.m, 123, sizeof(d3.m)); d3.sign = DECIMAL_SIGN_POS; d3.e = 123; d3.n = 123;
    if(decimal_div(&d1, &d2, &d3) != 0) return __LINE__;
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;

    ref.sign = d1.sign = DECIMAL_SIGN_NEG;
    d2.sign = DECIMAL_SIGN_POS;
    memset(d3.m, 123, sizeof(d3.m)); d3.sign = DECIMAL_SIGN_POS; d3.e = 123; d3.n = 123;
    if(decimal_div(&d1, &d2, &d3) != 0) return __LINE__;
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;

    d2.sign = d1.sign = DECIMAL_SIGN_NEG;
    ref.sign = DECIMAL_SIGN_POS;
    memset(d3.m, 123, sizeof(d3.m)); d3.sign = DECIMAL_SIGN_NEG; d3.e = 123; d3.n = 123;
    if(decimal_div(&d1, &d2, &d3) != 0) return __LINE__;
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;


    // division by divisor with two or more digits
    memset(d1.m, 0, sizeof(d1.m)); d1.sign = DECIMAL_SIGN_POS; d1.e = 0; d1.n = 0;
    memset(d2.m, 0, sizeof(d2.m)); d2.sign = DECIMAL_SIGN_POS; d2.e = 0; d2.n = 0;
    memset(d3.m, 123, sizeof(d3.m)); d3.sign = DECIMAL_SIGN_NEG; d3.e = 123; d3.n = 123;
    memset(ref.m, 0, sizeof(ref.m)); ref.sign = DECIMAL_SIGN_POS; ref.e = 0; ref.n = 0;

    // 1111 9999 9999 9999 / 3333 3333
    d1.m[3] = 1111;
    d1.m[2] = d1.m[1] = d1.m[0] = 9999;
    d1.n = 16;
    d2.m[1] = d2.m[0] = 3333;
    d2.n = 8;
    for(int i = 0; i < DECIMAL_PARTS - 2; i++)
    {
        ref.m[i] = (i % 2 > 0) ? 3335 : 9997;
    }
    ref.m[DECIMAL_PARTS - 2] = 0;
    ref.m[DECIMAL_PARTS - 1] = 3336;
    ref.n = DECIMAL_POSITIONS;
    ref.e = -32;
    memset(d3.m, 123, sizeof(d3.m)); d3.sign = DECIMAL_SIGN_NEG; d3.e = 123; d3.n = 123;
    if(decimal_div(&d1, &d2, &d3) != 0) return __LINE__;
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;

    // 9999 9999. 9999 9999 / 3333 3333
    ref.sign = d1.sign = d2.sign = DECIMAL_SIGN_POS;
    d1.e = -8;
    d2.e = 0;
    d1.m[3] = d1.m[2] = d1.m[1] = d1.m[0] = 9999;
    d1.n = 16;
    d2.m[1] = d2.m[0] = 3333;
    d2.n = 8;
    memset(ref.m, 0, sizeof(ref.m));
    ref.m[DECIMAL_PARTS - 1] = ref.m[DECIMAL_PARTS - 3] = 3;
    ref.m[DECIMAL_PARTS - 2] = 0;
    ref.n = 37;
    ref.e = -36;
    memset(d3.m, 123, sizeof(d3.m)); d3.sign = DECIMAL_SIGN_NEG; d3.e = 123; d3.n = 123;
    if(decimal_div(&d1, &d2, &d3) != 0) return __LINE__;
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;

    // 9999 9999. 9999 9999 / 6666 6666
    ref.sign = d1.sign = DECIMAL_SIGN_NEG;
    d2.sign = DECIMAL_SIGN_POS;
    d2.m[1] = d2.m[0] = 6666;
    ref.m[DECIMAL_PARTS-1] = ref.m[DECIMAL_PARTS-3] = 1;
    ref.m[DECIMAL_PARTS-2] = ref.m[DECIMAL_PARTS-4] = 5000;
    memset(d3.m, 123, sizeof(d3.m)); d3.sign = DECIMAL_SIGN_NEG; d3.e = 123; d3.n = 123;
    if(decimal_div(&d1, &d2, &d3) != 0) return __LINE__;
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;

    // 6666 6665 / 6666 6666
    ref.sign = d2.sign = DECIMAL_SIGN_NEG;
    d1.sign = DECIMAL_SIGN_POS;
    d1.m[3] = d1.m[2] = 0;
    d1.m[0] = 6665;
    d1.m[1] = d2.m[1] = d2.m[0] = 6666;
    d1.e = DECIMAL_MAX_EXPONENT;
    d2.e = DECIMAL_MAX_EXPONENT;
    d1.n = d2.n = 8;
    for(int i = 0; i < DECIMAL_PARTS - 2; i++)
    {
        ref.m[i] = (i % 2 > 0) ? 9998 : 4999;
    }
    ref.m[DECIMAL_PARTS - 2] = 9999;
    ref.m[DECIMAL_PARTS - 1] = 0;
    ref.n = 36;
    ref.e = -36;
    memset(d3.m, 123, sizeof(d3.m)); d3.sign = DECIMAL_SIGN_NEG; d3.e = 123; d3.n = 123;
    if(decimal_div(&d1, &d2, &d3) != 0) return __LINE__;
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;

    // special cases for 100% coverage of Knuth's div
    d1.sign = d2.sign = DECIMAL_SIGN_NEG;
    ref.sign = DECIMAL_SIGN_POS;
    d1.e = d2.e = DECIMAL_MIN_EXPONENT;
    d1.m[4] = 6666;
    d1.m[3] = 6666;
    d1.m[2] = 0;
    d1.m[1] = 0;
    d1.m[0] = 0;
    d1.n = 20;
    d2.m[2] = 6666;
    d2.m[1] = 6666;
    d2.m[0] = 9999;
    d2.n = 12;
    ref.m[9] = 0;
    ref.m[8] = 9999;
    ref.m[7] = 9998;
    ref.m[6] = 5001;
    ref.m[5] = 5000;
    ref.m[4] = 7497;
    ref.m[3] = 1;
    ref.m[2] = 8752;
    ref.m[1] = 6244;
    ref.m[0] = 5626;
    ref.n = 36;
    ref.e = -28;
    memset(d3.m, 123, sizeof(d3.m)); d3.sign = DECIMAL_SIGN_NEG; d3.e = 123; d3.n = 123;
    if(decimal_div(&d1, &d2, &d3) != 0) return __LINE__;
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;

    d1.m[4] = 0;
    d1.m[3] = 0;
    d1.m[2] = 4999;
    d1.m[1] = 9997;
    d1.m[0] = 0;
    d1.n = 12;
    d1.e = -4;
    d2.m[2] = 0;
    d2.m[1] = 5003;
    d2.m[0] = 9999;
    d2.n = 8;
    d2.e = 3;
    ref.m[9] = 9992;
    ref.m[8] = 59;
    ref.m[7] = 9504;
    ref.m[6] = 4084;
    ref.m[5] = 6331;
    ref.m[4] = 7515;
    ref.m[3] = 2541;
    ref.m[2] = 4698;
    ref.m[1] = 7492;
    ref.m[0] = 9453;
    ref.n = 40;
    ref.e = -43;
    memset(d3.m, 123, sizeof(d3.m)); d3.sign = DECIMAL_SIGN_NEG; d3.e = 123; d3.n = 123;
    if(decimal_div(&d1, &d2, &d3) != 0) return __LINE__;
//    printf("ref.m = %c %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d e=%d, n=%d\n", (ref.sign == DECIMAL_SIGN_NEG ? '-' : '+'), ref.m[9], ref.m[8], ref.m[7], ref.m[6], ref.m[5], ref.m[4], ref.m[3], ref.m[2], ref.m[1], ref.m[0], ref.e, ref.n);
//    printf("d3.m = %c %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d e=%d, n=%d\n", (d3.sign == DECIMAL_SIGN_NEG ? '-' : '+'), d3.m[9], d3.m[8], d3.m[7], d3.m[6], d3.m[5], d3.m[4], d3.m[3], d3.m[2], d3.m[1], d3.m[0], d3.e, d3.n);
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;

    // overflow
    d2.e = -35;
    d1.e = DECIMAL_MIN_EXPONENT;
    memset(d3.m, 123, sizeof(d3.m)); d3.sign = DECIMAL_SIGN_NEG; d3.e = -111; d3.n = 123;
    if(decimal_div(&d1, &d2, &d3) == 0) return __LINE__;
    if(error_get() != ERROR_DECIMAL_OVERFLOW) return __LINE__;

    d2.e = -37;
    d1.e = DECIMAL_MAX_EXPONENT;
    memset(d3.m, 123, sizeof(d3.m)); d3.sign = DECIMAL_SIGN_NEG; d3.e = -111; d3.n = 123;
    if(decimal_div(&d1, &d2, &d3) == 0) return __LINE__;
    if(error_get() != ERROR_DECIMAL_OVERFLOW) return __LINE__;

/*
    puts("Testing decimal left shift");

    int shift_by, t;

    for(shift_by = 1; shift_by <= DECIMAL_POSITIONS; shift_by++)
    {
        memset(d3.m, 0, sizeof(d3.m));
        d3.sign = DECIMAL_SIGN_POS;
        ref.sign = DECIMAL_SIGN_POS;
        for(int i = 0, t = 1; i < DECIMAL_POSITIONS; i++)
        {
            d3.m[i / DECIMAL_BASE_LOG10] += (i % 9 + 1) * t;
            t *= 10;
            if(t == DECIMAL_BASE) t = 1;
        }

        memset(ref.m, 0, sizeof(ref.m));
        t = 1;
        for(int i = 0; i < shift_by % DECIMAL_BASE_LOG10; i++) t *= 10;
        for(int i = shift_by; i < DECIMAL_POSITIONS; i++)
        {
            ref.m[i / DECIMAL_BASE_LOG10] += ((i - shift_by) % 9 + 1) * t;
            t *= 10;
            if(t == DECIMAL_BASE) t = 1;
        }
        decimal_shift_left(d3.m, shift_by);
        if(decimal_cmp(&d3, &ref) != 0) return __LINE__;
    }


    puts("Testing decimal right shift");

    for(shift_by = 1; shift_by <= DECIMAL_POSITIONS; shift_by++)
    {
        memset(d3.m, 0, sizeof(d3.m));
        d3.sign = DECIMAL_SIGN_POS;
        ref.sign = DECIMAL_SIGN_POS;
        for(int i = 0, t = 1; i < DECIMAL_POSITIONS; i++)
        {
            d3.m[i / DECIMAL_BASE_LOG10] += (i % 9 + 1) * t;
            t *= 10;
            if(t == DECIMAL_BASE) t = 1;
        }

        memset(ref.m, 0, sizeof(ref.m));
        t = 1;
        for(int i = 0; i < DECIMAL_POSITIONS - shift_by; i++)
        {
            ref.m[i / DECIMAL_BASE_LOG10] += ((i + shift_by) % 9 + 1) * t;
            t *= 10;
            if(t == DECIMAL_BASE) t = 1;
        }
//        printf("ref.m = %c %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d\n", (ref.sign == DECIMAL_SIGN_NEG ? '-' : '+'), ref.m[9], ref.m[8], ref.m[7], ref.m[6], ref.m[5], ref.m[4], ref.m[3], ref.m[2], ref.m[1], ref.m[0]);

        decimal_shift_right(d3.m, shift_by);
//        printf("d3.m  = %c %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d\n", (d3.sign == DECIMAL_SIGN_NEG ? '-' : '+'), d3.m[9], d3.m[8], d3.m[7], d3.m[6], d3.m[5], d3.m[4], d3.m[3], d3.m[2], d3.m[1], d3.m[0]);
        if(decimal_cmp(&d3, &ref) != 0) return __LINE__;
    }
*/
    return 0;
}
