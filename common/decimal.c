#include "common/decimal.h"
#include "common/error.h"
#include <assert.h>
#include <string.h>

// add abs values of two decimals: d3 = d1 + d2
// return 0 on success, not 0 on error
inline sint8 decimal_abs_add(const sint16 *d1, const sint16 *d2, sint16 *d3)
{
    uint32 s, c = 0;

    for(uint32 i = 0; i < DECIMAL_PARTS; i++)
    {
        s = (uint32)d1[i] + (uint32)d2[i] + c;
        if(s >= DECIMAL_BASE)
        {
            s -= DECIMAL_BASE;
            c = 1;
        }
        else
        {
            c = 0;
        }
        d3[i] = (sint16)s;
    }

    if(c > 0)
    {
        error_set(ERROR_DECIMAL_OVERFLOW);
        return 1;
    }

    return 0;
}

// compare absolute values of two decimals
// return positive if d1 > d2, negative if d1 < d2, 0 otherwise
inline sint16 decimal_abs_cmp(const sint16 *d1, const sint16 *d2)
{
    for(sint32 i = DECIMAL_PARTS-1; i >= 0; i--)
    {
        if(d1[i] != d2[i]) return d1[i] - d2[i];
    }
    return 0;
}

// subtract absolute value of d2 from d1
// d1 is supposed to be > d2
inline void decimal_abs_sub(const sint16 *d1, const sint16 *d2, sint16 *d3)
{
    sint16 c = 0;
    for(uint32 i = 0; i < DECIMAL_PARTS; i++)
    {
        if(d1[i] < d2[i] + c)
        {
            d3[i] = d1[i] + DECIMAL_BASE - d2[i] - c;
            c = 1;
        }
        else
        {
            d3[i] = d1[i] - d2[i] - c;
            c = 0;
        }
    }
    assert(0 == c);
}

// multiply d1 by digit d and put result to d3 with overflow
inline void decimal_mul_by_digit(const sint16 *d1, sint16 d, sint16 *d3)
{
    sint32 i, m;
    for(i = m = 0; i < DECIMAL_PARTS; i++)
    {
        m = d1[i] * d + m / DECIMAL_BASE;
        d3[i] = (sint16)(m % DECIMAL_BASE);
    }
    d3[DECIMAL_PARTS] = m / DECIMAL_BASE;
}

// multiply two decimals: d3 = d1 * d2
// return 0 on success, not 0 on error
sint8 decimal_mul(const decimal *d1, const decimal *d2, decimal *d3)
{
    sint32 m, k = 0;
    sint16 d1mi;
    uint16 m3[DECIMAL_PARTS*2];
    uint32 i, j;

    assert(sizeof(m3) == sizeof(d3->m)*2);

    memset(m3, 0, sizeof(m3));

    for(i = 0; i < DECIMAL_PARTS; i++)
    {
        if((d1mi = d1->m[i]) == 0) continue;

        for(j = 0, k = 0; j < DECIMAL_PARTS; j++)
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
    // Knuth's division
    sint32 d, c, j, n, m, qh, l, k, rh;
    sint16 buf[(DECIMAL_PARTS+1)*2];
    sint16 *n1 = buf, *n2 = buf + DECIMAL_PARTS + 1, *n1j;

    n = DECIMAL_PARTS - 1;
    while(n >= 0 && d2->m[n] == 0) n--;
    if(n < 0)
    {
        error_set(ERROR_DIVISION_BY_ZERO);
        return 1;
    }

    memset(d3->m, 0, sizeof(d3->m));
    d3->sign = DECIMAL_SIGN_POS;

    m = DECIMAL_PARTS - 1;
    while(m >= 0 && d1->m[m] == 0) m--;
    if(m < 0)
    {
        return 0;   // d3 == 0
    }
    m -= n;

    if(n == 0)
    {
        // division by single digit
        d = *(d2->m);
        rh = 0;
        if(d1->m[m] < d)
        {
            rh = d1->m[m];
            m--;
        }

        while(m >= 0)
        {
            qh = rh * DECIMAL_BASE + d1->m[m];
            rh = qh % d;
            d3->m[m] = qh / d;
            m--;
        }
    }
    else
    {
        // normalize: n1 = d1 * d, n2 = d2 * d
        d = DECIMAL_BASE / (d2->m[n] + 1);  // factor d: d * d2[most significant] close to DECIMAL_BASE
        memset(buf, 0, sizeof(buf));
        if(d == 1)
        {
            memcpy(n1, d1->m, sizeof(d1->m));
            memcpy(n2, d2->m, sizeof(d2->m));
        }
        else
        {
            decimal_mul_by_digit(d1->m, d, n1);
            decimal_mul_by_digit(d2->m, d, n2);
        }

        j = m;
        do
        {
            n1j = n1 + j;
            qh = (n1j[n + 1] * DECIMAL_BASE + n1j[n]);
            rh = qh % n2[n];
            qh /= n2[n];

            if(qh >= DECIMAL_BASE || (qh * n2[n-1] > DECIMAL_BASE * rh + n1j[n - 1]))
            {
                qh--;
                rh += n2[n];
                if(rh < DECIMAL_BASE)
                {
                    if(qh >= DECIMAL_BASE || (qh * n2[n-1] > DECIMAL_BASE * rh + n1j[n - 1]))
                    {
                        qh--;
                        rh += n2[n];
                    }
                }
            }

            // n1_j = n1_j - n2 * qh
            for(l = c = k = 0; l <= n+1; l++)
            {
                k = n2[l] * qh + k / DECIMAL_BASE;
                n1j[l] -= k % DECIMAL_BASE + c;
                if(n1j[l] < 0)
                {
                    n1j[l] += DECIMAL_BASE;
                    c = 1;
                }
                else
                {
                    c = 0;
                }
            }

            d3->m[j] = qh;

            if(c > 0)
            {
                // compensate
                d3->m[j]--;
                for(l = c = 0; l <= n + 1; l++)
                {
                    n1j[l] += n2[l] + c;
                    if(n1j[l] >= DECIMAL_BASE)
                    {
                        n1j[l] -= DECIMAL_BASE;
                        c = 1;
                    }
                    else
                    {
                        c = 0;
                    }
                }
                assert(c > 0);
            }

            j--;
        }
        while(j >= 0);
    }

    // determine sign
    j = DECIMAL_PARTS - 1;
    while(j >= 0 && d3->m[j] == 0) j--;
    d3->sign = ((d1->sign == d2->sign || j < 0) ? DECIMAL_SIGN_POS : DECIMAL_SIGN_NEG);

    return 0;
}

// subtract two decimals: d3 = d1 - d2
// return 0 on success, not 0 on error
sint8 decimal_sub(const decimal *d1, const decimal *d2, decimal *d3)
{
    if(d1->sign != d2->sign)
    {
        d3->sign = d1->sign;
        return decimal_abs_add(d1->m, d2->m, d3->m);
    }

    sint16 cmp = decimal_abs_cmp(d1->m, d2->m);
    if(cmp > 0)
    {
        d3->sign = d1->sign;
        decimal_abs_sub(d1->m, d2->m, d3->m);
    }
    else if(cmp < 0)
    {
        d3->sign = d2->sign;
        decimal_abs_sub(d2->m, d1->m, d3->m);
    }
    else
    {
        d3->sign = DECIMAL_SIGN_POS;
        memset(d3->m, 0, sizeof(d3->m));
    }

    return 0;
}

// add two decimals: d3 = d1 + d2
// return 0 on success, not 0 on error
sint8 decimal_add(const decimal *d1, const decimal *d2, decimal *d3)
{
    if(d1->sign != d2->sign)
    {
        sint16 cmp = decimal_abs_cmp(d1->m, d2->m);
        if(cmp > 0)
        {
            d3->sign = d1->sign;
            decimal_abs_sub(d1->m, d2->m, d3->m);
        }
        else if(cmp < 0)
        {
            d3->sign = d2->sign;
            decimal_abs_sub(d2->m, d1->m, d3->m);
        }
        else
        {
            d3->sign = DECIMAL_SIGN_POS;
            memset(d3->m, 0, sizeof(d3->m));
        }
    }
    else
    {
        d3->sign = d1->sign;
        return decimal_abs_add(d1->m, d2->m, d3->m);
    }
    return 0;
}

// compare two decimals
// return positive if d1 > d2, negative if d1 < d2, 0 otherwise
sint16 decimal_cmp(const decimal *d1, const decimal *d2)
{
    if(d1->sign != d2->sign) return d1->sign;

    return decimal_abs_cmp(d1->m, d2->m) * d1->sign;
}
