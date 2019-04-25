#include "common/decimal.h"
#include "common/error.h"
#include <assert.h>

// add abs values of two decimals: d3 = d1 + d2
// return 0 on success, not 0 on error
sint8 decimal_abs_add(const decimal *d1, const decimal *d2, decimal *d3)
{
    uint32 s, c = 0;

    for(uint32 i = 0; i < DECIMAL_PARTS; i++)
    {
        s = (uint32)d1->m[i] + (uint32)d2->m[i] + c;
        if(s >= DECIMAL_BASE)
        {
            s -= DECIMAL_BASE;
            c = 1;
        }
        else
        {
            c = 0;
        }
        d3->m[i] = (sint16)s;
    }

    if(c > 0)
    {
        error_set(ERROR_DECIMAL_OVERFLOW);
        return 1;
    }

    return 0;
}

/*
inline void decimal_negate(decimal *d)
{
    for(uint32 i=0; i < DECIMAL_PARTS; i++)
        d->m[i] = DECIMAL_BASE - d->m[i];
}

// subtract two decimals: d3 = d1 - d2
// return 0 on success, not 0 on error
sint8 decimal_sub(const decimal *d1, decimal *d2, decimal *d3)
{
    uint32 s, c = 0;

    if(d2->sign != d1->sign)
    {
        d3->sign = d1->sign;
        return decimal_add(d1, d2, d3);
    }
    else
    {
        decimal_negate(d2);

        for(uint32 i = 0, j = 0; i < DECIMAL_PARTS; i++)
        {
            s = (uint32)d1->m[i] + (uint32)d2->m[i] + c;
            if(s > DECIMAL_BASE)
            {
                s -= DECIMAL_BASE;
                c = 1;
                j = i;
            }
            else
            {
                j = c = 0;
            }
            d3->m[i] = (sint16)s;
        }

        if(c > 0)
        {
            // result is positive decimal
            while(j < DECIMAL_PARTS)
            {
                d3->m[j] -= 1;
                j++;
            }
            d3->sign = DECIMAL_SIGN_POS;
        }
        else
        {
            decimal_negate(d3);
            d3->sign = DECIMAL_SIGN_NEG;
        }

        decimal_negate(d2);
    }

    return 0;
}
*/

// multiply two decimals: d3 = d1 * d2
// return 0 on success, not 0 on error
sint8 decimal_mul(const decimal *d1, const decimal *d2, decimal *d3)
{
    sint32 m, k = 0;
    sint16 d1mi;
    uint16 m3[DECIMAL_PARTS*2];

    assert(sizeof(m3) == sizeof(d3->m)*2);

    memset(m3, 0, sizeof(m3));

    for(uint32 i = 0; i < DECIMAL_PARTS; i++)
    {
        if((d1mi = d1->m[i]) == 0) continue;

        for(uint32 j = 0, k = 0; j < DECIMAL_PARTS; j++)
        {
            m = d1mi * d2->m[j] + m3[i+j] + k;

            if(i + j >= DECIMAL_PARTS && m > 0)
            {
                error_set(ERROR_DECIMAL_OVERFLOW);
                return 1;
            }

            m3[i+j] = (sint16)(m % DECIMAL_BASE);
            k = m / DECIMAL_BASE;
        }
    }

    if(k > 0)
    {
        error_set(ERROR_DECIMAL_OVERFLOW);
        return 1;
    }

    memcpy(d3->m, m3, sizeof(d3->m));
    d3->sign = ((d1->sign == d2->sign) ? DECIMAL_SIGN_POS : DECIMAL_SIGN_NEG);

    return 0;
}

// divide d1 by d2: d3 = d1 / d2
// return 0 on success, not 0 on error
sint8 decimal_div(const decimal *d1, const decimal *d2, decimal *d3)
{
    return 0;
}

// compare absolute values of two decimals
// return positive if d1 > d2, negative if d1 < d2, 0 otherwise
inline sint16 decimal_abs_cmp(const decimal *d1, const decimal *d2)
{
    for(sint32 i = DECIMAL_PARTS-1; i >= 0; i--)
    {
        if(d1->m[i] != d2->m[i]) return d1->m[i] - d2->m[i];
    }
    return 0;
}

// subtract absolute value of d2 from d1
// d1 is supposed to be > d2
inline void decimal_abs_sub(const decimal *d1, const decimal *d2, decimal *d3)
{
    sint16 c = 0;
    for(uint32 i = 0; i < DECIMAL_PARTS; i++)
    {
        if(d1->m[i] < d2->m[i] + c)
        {
            d3->m[i] = d1->m[i] + DECIMAL_BASE - d2->m[i] - c;
            c = 1;
        }
        else
        {
            d3->m[i] = d1->m[i] - d2->m[i] - c;
            c = 0;
        }
    }
    assert(0 == c);
}

// subtract two decimals: d3 = d1 - d2
// return 0 on success, not 0 on error
sint8 decimal_sub(const decimal *d1, const decimal *d2, decimal *d3)
{
    if(d1->sign != d2->sign)
    {
        d3->sign = d1->sign;
        return decimal_abs_add(d1, d2, d3);
    }

    sint16 cmp = decimal_abs_cmp(d1, d2);
    if(cmp > 0)
    {
        d3->sign = d1->sign;
        decimal_abs_sub(d1, d2, d3);
    }
    else if(cmp < 0)
    {
        d3->sign = d2->sign;
        decimal_abs_sub(d2, d1, d3);
    }
    else
    {
        d3->sign = DECIMAL_SIGN_POS;
        for(uint32 i=0; i<DECIMAL_PARTS; i++)
            d3->m[i] = 0;
    }

    return 0;
}

// add two decimals: d3 = d1 + d2
// return 0 on success, not 0 on error
sint8 decimal_add(const decimal *d1, const decimal *d2, decimal *d3)
{
    if(d1->sign != d2->sign)
    {
        sint16 cmp = decimal_abs_cmp(d1, d2);
        if(cmp > 0)
        {
            d3->sign = d1->sign;
            decimal_abs_sub(d1, d2, d3);
        }
        else if(cmp < 0)
        {
            d3->sign = d2->sign;
            decimal_abs_sub(d2, d1, d3);
        }
        else
        {
            d3->sign = DECIMAL_SIGN_POS;
            for(uint32 i=0; i<DECIMAL_PARTS; i++)
                d3->m[i] = 0;
        }
    }
    else
    {
        d3->sign = d1->sign;
        return decimal_abs_add(d1, d2, d3);
    }
    return 0;
}

// compare two decimals
// return positive if d1 > d2, negative if d1 < d2, 0 otherwise
sint16 decimal_cmp(const decimal *d1, const decimal *d2)
{
    if(d1->sign != d2->sign) return d1->sign;

    return decimal_abs_cmp(d1, d2) * d1->sign;
}
