#ifndef _DECIMAL_H
#define _DECIMAL_H

// decimal precision arithmetics and decimal data type

#include "defs/defs.h"

#define DECIMAL_PARTS (10)
#define DECIMAL_BASE  (10000)
#define DECIMAL_SIGN_POS (1)
#define DECIMAL_SIGN_NEG (-1)

typedef struct _decimal
{
    sint8  sign;
    sint16 m[DECIMAL_PARTS];
} decimal;

// add two decimals: d3 = d1 + d2
// return 0 on success, not 0 on error
sint8 decimal_add(const decimal *d1, const decimal *d2, decimal *d3);

// subtract d2 from d1: d3 = d1 + d2
// return 0 on success, not 0 on error
sint8 decimal_sub(const decimal *d1, const decimal *d2, decimal *d3);

// multiply two decimals: d3 = d1 * d2
// return 0 on success, not 0 on error
sint8 decimal_mul(const decimal *d1, const decimal *d2, decimal *d3);

// divide d1 by d2: d3 = d1 / d2
// return 0 on success, not 0 on error
sint8 decimal_div(const decimal *d1, const decimal *d2, decimal *d3);

// compare two decimals
// return positive if d1 > d2, negative if d1 < d2, 0 otherwise
sint16 decimal_cmp(const decimal *d1, const decimal *d2);

#endif
