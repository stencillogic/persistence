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
    puts("Testing decimal multiplication performance");

    decimal decimal_set[10000], *deci, *decj;
    int num_to_read = 2000;
    int rd;
    int fd = open("/home/ivan/projects/cpp/test/gen_decimal_bank/decimal_set", O_RDONLY);
    if(fd == -1) { perror("opening"); return __LINE__; }
    for(int i=0; i<num_to_read; i++)
    {
        decimal_set[i].sign = DECIMAL_SIGN_POS;
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

    memset(d1.m, 0, sizeof(d1.m)); d1.sign = DECIMAL_SIGN_POS;
    memset(d2.m, 0, sizeof(d2.m)); d2.sign = DECIMAL_SIGN_POS;
    memset(d3.m, 0, sizeof(d3.m)); d3.sign = DECIMAL_SIGN_POS;
    memset(ref.m, 0, sizeof(ref.m)); ref.sign = DECIMAL_SIGN_POS;

    // division by zero
    if(decimal_div(&d1, &d2, &d3) == 0) return __LINE__;
    if(error_get() != ERROR_DIVISION_BY_ZERO) return __LINE__;

    d2.m[0] = 1;
    if(decimal_div(&d1, &d2, &d3) != 0) return __LINE__;
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;

    d1.sign = DECIMAL_SIGN_NEG;
    if(decimal_div(&d1, &d2, &d3) != 0) return __LINE__;
    if(d3.sign != DECIMAL_SIGN_POS) return __LINE__;

    d2.sign = DECIMAL_SIGN_NEG;
    if(decimal_div(&d1, &d2, &d3) != 0) return __LINE__;
    if(d3.sign != DECIMAL_SIGN_POS) return __LINE__;

    d1.sign = DECIMAL_SIGN_POS;
    if(decimal_div(&d1, &d2, &d3) != 0) return __LINE__;
    if(d3.sign != DECIMAL_SIGN_POS) return __LINE__;

    // division by single digit
    ref.sign = d1.sign = d2.sign = DECIMAL_SIGN_POS;
    d2.m[0] = 3;
    d1.m[0] = 9998;
    ref.m[0] = 3332;
    if(decimal_div(&d1, &d2, &d3) != 0) return __LINE__;
//    printf("d3.m = %c %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d\n", (d3.sign == DECIMAL_SIGN_NEG ? '-' : '+'), d3.m[9], d3.m[8], d3.m[7], d3.m[6], d3.m[5], d3.m[4], d3.m[3], d3.m[2], d3.m[1], d3.m[0]);
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;

    d1.m[1] = 9;
    d2.m[0] = 33;
    ref.m[0] = 3030;
    if(decimal_div(&d1, &d2, &d3) != 0) return __LINE__;
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;

    for(int i=0; i<DECIMAL_PARTS; i++)
    {
        d1.m[i] = 9999;
        ref.m[i] = 3333;
    }
    d2.m[0] = 3;
    if(decimal_div(&d1, &d2, &d3) != 0) return __LINE__;
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;

    for(int i=0; i<DECIMAL_PARTS; i++)
    {
        d1.m[i] = 3333;
        ref.m[i] = (i%3 == 1 ? 6667 : (i%3 == 2 ? 3333 : 0));
    }
    d2.m[0] = 9999;
    if(decimal_div(&d1, &d2, &d3) != 0) return __LINE__;
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;

    // signs again but with non zero quotinent
    ref.sign = d2.sign = DECIMAL_SIGN_NEG;
    if(decimal_div(&d1, &d2, &d3) != 0) return __LINE__;
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;

    ref.sign = d1.sign = DECIMAL_SIGN_NEG;
    d2.sign = DECIMAL_SIGN_POS;
    if(decimal_div(&d1, &d2, &d3) != 0) return __LINE__;
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;

    d2.sign = d1.sign = DECIMAL_SIGN_NEG;
    ref.sign = DECIMAL_SIGN_POS;
    if(decimal_div(&d1, &d2, &d3) != 0) return __LINE__;
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;

    // sign with zero quotinent and nonzero divident
    memset(ref.m, 0, sizeof(ref.m)); ref.sign = DECIMAL_SIGN_POS;
    memset(d1.m, 0, sizeof(d1.m)); d1.sign = DECIMAL_SIGN_POS;
    memset(d2.m, 0, sizeof(d2.m)); d2.sign = DECIMAL_SIGN_POS;
    memset(d3.m, 0, sizeof(d3.m)); d3.sign = DECIMAL_SIGN_POS;
    memset(ref.m, 0, sizeof(ref.m)); ref.sign = DECIMAL_SIGN_POS;
    d1.m[0] = 3333;
    d2.m[0] = 3334;
    if(decimal_div(&d1, &d2, &d3) != 0) return __LINE__;
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;

    // signs again but with non zero quotinent
    d2.sign = DECIMAL_SIGN_NEG;
    if(decimal_div(&d1, &d2, &d3) != 0) return __LINE__;
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;

    d1.sign = DECIMAL_SIGN_NEG;
    if(decimal_div(&d1, &d2, &d3) != 0) return __LINE__;
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;

    d2.sign = DECIMAL_SIGN_POS;
    if(decimal_div(&d1, &d2, &d3) != 0) return __LINE__;
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;

    // division by divisor with two or more digits
    ref.sign = d1.sign = d2.sign = DECIMAL_SIGN_POS;
    d1.m[3] = 1111; d1.m[2] = d1.m[1] = d1.m[0] = 9999;
    d2.m[1] = d2.m[0] = 3333;
    ref.m[0] = 0;
    ref.m[1] = 3336;
    if(decimal_div(&d1, &d2, &d3) != 0) return __LINE__;
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;

    ref.sign = d1.sign = d2.sign = DECIMAL_SIGN_POS;
    d1.m[3] = d1.m[2] = d1.m[1] = d1.m[0] = 9999;
    d2.m[1] = d2.m[0] = 3333;
    ref.m[2] = ref.m[0] = 3;
    ref.m[1] = 0;
    if(decimal_div(&d1, &d2, &d3) != 0) return __LINE__;
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;

    ref.sign = d1.sign = d2.sign = DECIMAL_SIGN_POS;
    d1.m[3] = d1.m[2] = d1.m[1] = d1.m[0] = 9999;
    d2.m[1] = d2.m[0] = 6666;
    ref.m[2] = ref.m[0] = 1;
    ref.m[1] = 5000;
    if(decimal_div(&d1, &d2, &d3) != 0) return __LINE__;
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;

    ref.sign = d1.sign = d2.sign = DECIMAL_SIGN_POS;
    d1.m[3] = d1.m[2] = 0;
    d1.m[0] = 6665;
    d1.m[1] = d2.m[1] = d2.m[0] = 6666;
    ref.m[2] = ref.m[1] = ref.m[0] = 0;     // 6666 6665 / 6666 6666
    if(decimal_div(&d1, &d2, &d3) != 0) return __LINE__;
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;

    // special cases for 100% coverage of Knuth's div
    d1.m[4] = 6666;
    d1.m[3] = 6666;
    d1.m[2] = 0;
    d1.m[1] = 0;
    d1.m[0] = 0;
    d2.m[2] = 6666;
    d2.m[1] = 6666;
    d2.m[0] = 9999;
    ref.m[1] = 9999;
    ref.m[0] = 9998;
    if(decimal_div(&d1, &d2, &d3) != 0) return __LINE__;
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;

    d1.m[4] = 0;
    d1.m[3] = 0;
    d1.m[2] = 4999;
    d1.m[1] = 9997;
    d1.m[0] = 0;
    d2.m[2] = 0;
    d2.m[1] = 5003;
    d2.m[0] = 9999;
    ref.m[1] = 0;
    ref.m[0] = 9992;
    if(decimal_div(&d1, &d2, &d3) != 0) return __LINE__;
//    printf("d3.m = %c %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d\n", (d3.sign == DECIMAL_SIGN_NEG ? '-' : '+'), d3.m[9], d3.m[8], d3.m[7], d3.m[6], d3.m[5], d3.m[4], d3.m[3], d3.m[2], d3.m[1], d3.m[0]);
    if(decimal_cmp(&d3, &ref) != 0) return __LINE__;

    return 0;
}
