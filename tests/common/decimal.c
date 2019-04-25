#include "tests.h"
#include "common/decimal.h"
#include "common/error.h"
#include <string.h>
#include <stdio.h>

int test_decimal_functions()
{
    decimal d1, d2, d3, ref;

    if(DECIMAL_PARTS < 10) return __LINE__;
    memset(d1.m, 0, sizeof(d1.m)); d1.sign = DECIMAL_SIGN_POS;
    memset(d2.m, 0, sizeof(d2.m)); d2.sign = DECIMAL_SIGN_POS;
    memset(d3.m, 0, sizeof(d3.m)); d3.sign = DECIMAL_SIGN_POS;
    memset(ref.m, 0, sizeof(ref.m)); ref.sign = DECIMAL_SIGN_POS;

    puts("Testing decimal comparisons");

    if(decimal_cmp(&d1, &d2) != 0) return __LINE__;

    d1.m[0] = 1;    // 1, 0
    if(decimal_cmp(&d1, &d2) <= 0) return __LINE__;

    d2.m[0] = 2;    // 1, 2
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
    if(decimal_cmp(&d1, &d2) >= 0) return __LINE__;

    d1.m[1] = 9;    // 90003, 90002
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

    error_set(ERROR_NO_ERROR);
    if(decimal_add(&d1, &d2, &d3) == 0) return __LINE__;
    if(ERROR_DECIMAL_OVERFLOW != error_get()) return __LINE__;

    d2.sign = d1.sign = DECIMAL_SIGN_NEG;
    error_set(ERROR_NO_ERROR);
    if(decimal_add(&d1, &d2, &d3) == 0) return __LINE__;
    if(ERROR_DECIMAL_OVERFLOW != error_get()) return __LINE__;

    d2.sign = DECIMAL_SIGN_POS;
    memset(d3.m, 0, sizeof(d3.m)); d3.sign = DECIMAL_SIGN_POS;
    ref.sign = DECIMAL_SIGN_NEG;
    for(int i=0; i<DECIMAL_PARTS; i++)
        ref.m[i] = 9999;
    ref.m[0] = 9980;
    if(decimal_add(&d1, &d2, &d3) != 0) return __LINE__;
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;

    ref.sign = d1.sign = DECIMAL_SIGN_POS;
    d2.sign = DECIMAL_SIGN_NEG;
    if(decimal_add(&d1, &d2, &d3) != 0) return __LINE__;
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;

    memset(d1.m, 0, sizeof(d1.m)); d1.sign = DECIMAL_SIGN_POS;
    memset(d2.m, 0, sizeof(d2.m)); d2.sign = DECIMAL_SIGN_POS;
    memset(d3.m, 0, sizeof(d3.m)); d3.sign = DECIMAL_SIGN_POS;
    memset(ref.m, 0, sizeof(ref.m)); ref.sign = DECIMAL_SIGN_POS;
    d1.m[1] = 9999;
    d2.m[1] = 9999;
    ref.m[1] = 9998;
    ref.m[2] = 1;
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

    error_set(ERROR_NO_ERROR);
    if(decimal_sub(&d1, &d2, &d3) == 0) return __LINE__;
    if(ERROR_DECIMAL_OVERFLOW != error_get()) return __LINE__;

    d2.sign = DECIMAL_SIGN_NEG;
    d1.sign = DECIMAL_SIGN_POS;
    error_set(ERROR_NO_ERROR);
    if(decimal_sub(&d1, &d2, &d3) == 0) return __LINE__;
    if(ERROR_DECIMAL_OVERFLOW != error_get()) return __LINE__;

    d1.sign = d2.sign = DECIMAL_SIGN_POS;
    memset(d3.m, 1, sizeof(d3.m)); d3.sign = DECIMAL_SIGN_NEG;
    ref.sign = DECIMAL_SIGN_POS;
    for(int i=0; i<DECIMAL_PARTS; i++)
        ref.m[i] = 9999;
    ref.m[0] = 9980;
    if(decimal_sub(&d1, &d2, &d3) != 0) return __LINE__;
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;

    ref.sign = d1.sign = d2.sign = DECIMAL_SIGN_NEG;
    memset(d3.m, 1, sizeof(d3.m)); d3.sign = DECIMAL_SIGN_POS;
    if(decimal_sub(&d1, &d2, &d3) != 0) return __LINE__;
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;

    memset(d1.m, 0, sizeof(d1.m)); d1.sign = DECIMAL_SIGN_POS;
    memset(d2.m, 0, sizeof(d2.m)); d2.sign = DECIMAL_SIGN_POS;
    memset(d3.m, 0, sizeof(d3.m)); d3.sign = DECIMAL_SIGN_POS;
    memset(ref.m, 0, sizeof(ref.m)); ref.sign = DECIMAL_SIGN_POS;
    d1.m[1] = 9998;
    d1.m[2] = 1;
    d2.m[1] = 9999;
    ref.m[1] = 9999;
    if(decimal_sub(&d1, &d2, &d3) != 0) return __LINE__;
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;


    puts("Testing decimal multiplication");

    memset(d1.m, 0, sizeof(d1.m)); d1.sign = DECIMAL_SIGN_POS;
    memset(d2.m, 0, sizeof(d2.m)); d2.sign = DECIMAL_SIGN_POS;
    memset(d3.m, 0, sizeof(d3.m)); d3.sign = DECIMAL_SIGN_POS;
    memset(ref.m, 0, sizeof(ref.m)); ref.sign = DECIMAL_SIGN_POS;
    if(decimal_mul(&d1, &d2, &d3) != 0) return __LINE__;
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;

    d1.m[0] = 9999;
    if(decimal_mul(&d1, &d2, &d3) != 0) return __LINE__;
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;

    d2.m[0] = 9999;
    d1.m[0] = 0;
    if(decimal_mul(&d1, &d2, &d3) != 0) return __LINE__;
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;

    d2.m[0] = 1;
    d1.m[0] = 1;
    ref.m[0] = 1;
    if(decimal_mul(&d1, &d2, &d3) != 0) return __LINE__;
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;

    d2.sign = DECIMAL_SIGN_NEG;
    ref.sign = DECIMAL_SIGN_NEG;
    if(decimal_mul(&d1, &d2, &d3) != 0) return __LINE__;
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;

    d2.sign = DECIMAL_SIGN_POS;
    d1.sign = DECIMAL_SIGN_NEG;
    if(decimal_mul(&d1, &d2, &d3) != 0) return __LINE__;
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;

    d1.sign = d2.sign = DECIMAL_SIGN_NEG;
    ref.sign = DECIMAL_SIGN_POS;
    if(decimal_mul(&d1, &d2, &d3) != 0) return __LINE__;
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;

    d2.m[0] = 9999;
    d1.m[0] = 9999;
    ref.m[0] = 1;
    ref.m[1] = 9998;
    if(decimal_mul(&d1, &d2, &d3) != 0) return __LINE__;
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;

    d1.m[2] = 2;        // 2 0000 9999
    ref.m[0] = 1;
    ref.m[1] = 9998;
    ref.m[2] = 9998;
    ref.m[3] = 1;
    if(decimal_mul(&d1, &d2, &d3) != 0) return __LINE__;
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;

    d2.m[2] = 5000;     // 5000 0000 9999
    ref.m[0] = 1;
    ref.m[1] = 9998;
    ref.m[2] = 4998;
    ref.m[3] = 5001;
    ref.m[4] = 0;
    ref.m[5] = 1;
    if(decimal_mul(&d1, &d2, &d3) != 0) return __LINE__;
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;

    // overflows
    memset(d1.m, 0, sizeof(d1.m)); d1.sign = DECIMAL_SIGN_POS;
    memset(d2.m, 0, sizeof(d2.m)); d2.sign = DECIMAL_SIGN_POS;
    memset(d3.m, 0, sizeof(d3.m)); d3.sign = DECIMAL_SIGN_POS;
    memset(ref.m, 0, sizeof(ref.m)); ref.sign = DECIMAL_SIGN_POS;

    d1.m[DECIMAL_PARTS-1] = DECIMAL_BASE/2;
    d2.m[0] = 2;
    if(decimal_mul(&d1, &d2, &d3) == 0) return __LINE__;
    if(error_get() != ERROR_DECIMAL_OVERFLOW) return __LINE__;

    d1.m[DECIMAL_PARTS-1] = DECIMAL_BASE/2-1;
    if(decimal_mul(&d1, &d2, &d3) != 0) return __LINE__;

    d2.m[0] = d1.m[DECIMAL_PARTS-1] = 0;
    d1.m[5] = d2.m[5] = 1;
    if(decimal_mul(&d1, &d2, &d3) == 0) return __LINE__;
    if(error_get() != ERROR_DECIMAL_OVERFLOW) return __LINE__;

    d1.m[5] = 0;
    d1.m[0] = d1.m[1] = d1.m[2] = d1.m[3] = d1.m[4] = 9999;
    if(decimal_mul(&d1, &d2, &d3) != 0) return __LINE__;

    for(int i=0; i<DECIMAL_PARTS; i++)
    {
        d1.m[i] = d2.m[i] = 9999;
    }
    if(decimal_mul(&d1, &d2, &d3) == 0) return __LINE__;
    if(error_get() != ERROR_DECIMAL_OVERFLOW) return __LINE__;

/*
// divide d1 by d2: d3 = d1 / d2
// return 0 on success, not 0 on error
sint8 decimal_div(const decimal *d1, const decimal *d2, decimal *d3);
*/
    return 0;
}
