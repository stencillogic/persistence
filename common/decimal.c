#include "common/decimal.h"
#include "common/error.h"
#include <assert.h>
#include <string.h>

// add abs values of two decimals: d3 = d1 + d2
// return 0 if no carry outside of d3, 1 otherwise
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

    return c;
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

// compare absolute values of two decimals with shifts n1, n2
// return positive if d1 > d2, negative if d1 < d2, 0 otherwise
inline sint16 decimal_abs_cmp_with_shift(const sint16 *d1, sint16 n1, const sint16 *d2, sint16 n2)
{
    sint16 t1 = DECIMAL_BASE / 10, t2 = DECIMAL_BASE / 10;
    sint16 s;
    sint16 v1, v2;

    s = n1 / DECIMAL_BASE_LOG10;
    while((s--) > 0) t1 /= 10;
    s = n2 / DECIMAL_BASE_LOG10;
    while((s--) > 0) t2 /= 10;

    for(n1--, n2--; n1 >= 0 && n2 >= 0; n1--, n2--)
    {
        v1 = (d1[n1 / DECIMAL_BASE_LOG10] / t1) % 10;
        v2 = (d2[n2 / DECIMAL_BASE_LOG10] / t2) % 10;
        if(v1 != v2) return v1 - v2;
        t1 /= 10;
        if(t1 == 0) t1 = DECIMAL_BASE / 10;
        t2 /= 10;
        if(t2 == 0) t2 = DECIMAL_BASE / 10;
    }
    return 0;
}

// subtract absolute value of d2 from d1
// d1 is supposed to be > d2
void decimal_abs_sub(const sint16 *d1, const sint16 *d2, sint16 *d3)
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
void decimal_mul_by_digit(const sint16 *d1, sint16 d, sint16 *d3)
{
    sint32 i, m;
    for(i = m = 0; i < DECIMAL_PARTS; i++)
    {
        m = d1[i] * d + m / DECIMAL_BASE;
        d3[i] = (sint16)(m % DECIMAL_BASE);
    }
    d3[DECIMAL_PARTS] = m / DECIMAL_BASE;
}

// shift decimal m to the right by n digits
void decimal_shift_right(sint16 *m, sint32 n)
{
    assert(n > 0 && n <= DECIMAL_POSITIONS);

    sint32 i, s, t, x = n % DECIMAL_BASE_LOG10;
    n = n / DECIMAL_BASE_LOG10;
    if(x == 0)
    {
        for(i = 0; i < DECIMAL_PARTS - n; i++)
        {
            m[i] = m[i + n];
        }
    }
    else
    {
        s = 10;
        t = DECIMAL_BASE / 10;
        while((--x) > 0)
        {
            s *= 10;
            t /= 10;
        }

        for(i = 0; i < DECIMAL_PARTS - n - 1; i++)
        {
            m[i] = (m[i + n + 1] % s) * t + m[i + n] / s;
        }
        m[i] = m[i + n] / s;
    }
    memset(m - n + DECIMAL_PARTS, 0, n * sizeof(sint16));
}

// shift decimal m to the left by n digits
void decimal_shift_left(sint16 *m, sint32 n)
{
    assert(n > 0 && n <= DECIMAL_POSITIONS);

    sint32 i, s, t, x = n % DECIMAL_BASE_LOG10;
    n = n / DECIMAL_BASE_LOG10;
    if(x == 0)
    {
        for(i = DECIMAL_PARTS - 1; i >= n; i--)
        {
            m[i] = m[i - n];
        }
    }
    else
    {
        s = 10;
        t = DECIMAL_BASE / 10;
        while((--x) > 0)
        {
            s *= 10;
            t /= 10;
        }

        for(i = DECIMAL_PARTS - 1; i > n; i--)
        {
            m[i] = (m[i - n] % t) * s + m[i - n - 1] / t;
        }
        m[i] = (m[i - n] % t) * s;
    }
    memset(m, 0, n * sizeof(sint16));
}

// return number of digits taken in mantissa
sint16 decimal_num_digits(const sint16 *m)
{
    sint16 n = DECIMAL_POSITIONS, t;
    const sint16 *p = m + DECIMAL_PARTS - 1;

    while(p >= m && 0 == (*p))
    {
        p--;
        n -= DECIMAL_BASE_LOG10;
    }

    if(n > 0)
    {
        t = *p;
        n -= DECIMAL_BASE_LOG10;
        do
        {
            n += 1;
            t /= 10;
        }
        while(t > 0);
    }

    return n;
}

// multiply two decimals: d3 = d1 * d2
// return 0 on success, not 0 on error
sint8 decimal_mul(const decimal *d1, const decimal *d2, decimal *d3)
{
    sint32 m, k = 0, n, nd, i, j, e = (sint32)d1->e + (sint32)d2->e;
    sint16 d1mi;
    sint16 m3[DECIMAL_PARTS*2];

    assert(sizeof(m3) == sizeof(d3->m)*2);

    memset(m3, 0, sizeof(m3));

    for(i = 0; i < DECIMAL_PARTS; i++)
    {
        if((d1mi = d1->m[i]) == 0) continue;

        for(j = 0, k = 0; j < DECIMAL_PARTS; j++)
        {
            m = d1mi * d2->m[j] + m3[i + j] + k;

            m3[i + j] = (sint16)(m % DECIMAL_BASE);
            k = m / DECIMAL_BASE;
        }
        m3[i + j] = (sint16)m3[i + j] + k;
    }

    n = (sint32)decimal_num_digits(m3 + DECIMAL_PARTS);
    if(n > 0)
    {
        n += DECIMAL_POSITIONS;
    }
    else
    {
        n = (sint32)decimal_num_digits(m3);
    }

    // take care if result is not fitting in d3->m without truncating
    if(n > DECIMAL_POSITIONS)
    {
        // save as much digits as we can
        e += n - DECIMAL_POSITIONS;
        nd = n % DECIMAL_BASE_LOG10;
        n = (n + DECIMAL_BASE_LOG10 - 1) / DECIMAL_BASE_LOG10 - DECIMAL_PARTS;

        decimal_shift_left(m3 + n, DECIMAL_BASE_LOG10 - nd);

        k = DECIMAL_BASE;
        while(nd > 0)
        {
            k /= 10;
            nd--;
        }
        m3[n] += m3[n - 1] / k;

        d3->n = DECIMAL_POSITIONS;
    }
    else
    {
        d3->n = n;
        n = 0;
    }

    if(e > DECIMAL_MAX_EXPONENT || e < DECIMAL_MIN_EXPONENT)
    {
        error_set(ERROR_DECIMAL_OVERFLOW);
        return 1;
    }

    memcpy(d3->m, m3 + n, sizeof(d3->m));
    d3->e = (sint8)e;
    d3->sign = ((d1->sign == d2->sign || d1->n == 0) ? DECIMAL_SIGN_POS : DECIMAL_SIGN_NEG);

    return 0;
}


// divide d1 by d2: d3 = d1 / d2
// return 0 on success, not 0 on error
sint8 decimal_div(const decimal *d1, const decimal *d2, decimal *d3)
{
    // Knuth's division
    sint32 d, c, j, n, m, e, qh, l, k, rh;
    sint16 buf[DECIMAL_PARTS * 3 + 3], v1, v2;
    sint16 *n1 = buf + 2, *n2 = buf + DECIMAL_PARTS * 2 + 3, *n1j;

    if(d2->n == 0)
    {
        error_set(ERROR_DIVISION_BY_ZERO);
        return 1;
    }
    n = (d2->n - 1) / DECIMAL_BASE_LOG10;

    if(d1->n == 0)
    {
        memset(d3->m, 0, sizeof(d3->m));
        d3->e = d3->n = 0;
        d3->sign = DECIMAL_SIGN_POS;

        return 0;   // d1 / d2 = 0
    }
    m = (d1->n - 1) / DECIMAL_BASE_LOG10;

    j = DECIMAL_PARTS - 1;

    if(n == 0)
    {
        // division by single digit
        d = *(d2->m);
        rh = 0;
        k = m;
        if(d1->m[k] < d)
        {
            rh = d1->m[k];
            k--;
        }

        while(j >= 0)
        {
            qh = rh * DECIMAL_BASE + ((k >= 0) ? d1->m[k] : 0);
            rh = qh % d;
            d3->m[j] = qh / d;

            if(rh == 0 && k <= 0)
            {
                break;
            }

            k--;
            j--;
        }

        while(j > 0)
        {
            j--;
            d3->m[j] = 0;
        }
    }
    else
    {
        // normalize: n1 = d1 * d, n2 = d2 * d
        d = DECIMAL_BASE / (d2->m[n] + 1);  // factor d: d * d2[most significant] is close to DECIMAL_BASE

        memset(buf, 0, sizeof(buf));

        if(d == 1)
        {
            memcpy(n1 + DECIMAL_PARTS - m - 1, d1->m, sizeof(d1->m));
            memcpy(n2, d2->m, sizeof(d2->m));
        }
        else
        {
            decimal_mul_by_digit(d1->m, d, n1 + DECIMAL_PARTS - m - 1);
            decimal_mul_by_digit(d2->m, d, n2);
        }

        v1 = n2[n];
        v2 = n2[n - 1];

        if(n1[j + n + 1] * DECIMAL_BASE + n1[j + n] < v1)
        {
            n1--;
            if(n1[j + n + 1] * DECIMAL_BASE + n1[j + n] < v1)
            {
                n1--;
            }
        }

        do
        {
            n1j = n1 + j;
            qh = (n1j[n + 1] * DECIMAL_BASE + n1j[n]);
            rh = qh % v1;
            qh /= v1;

            if(qh >= DECIMAL_BASE || (qh * v2 > DECIMAL_BASE * rh + n1j[n - 1]))
            {
                qh--;
                rh += v1;
                if(rh < DECIMAL_BASE)
                {
                    if(qh >= DECIMAL_BASE || (qh * v2 > DECIMAL_BASE * rh + n1j[n - 1]))
                    {
                        qh--;
                    }
                }
            }

            // n1_j = n1_j - n2 * qh
            for(l = c = k = 0; l <= n + 1; l++)
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

    // exponent
    j = 0;
    d = d3->m[DECIMAL_PARTS - 1];
    while(d > 0)
    {
        d /= 10;
        j++;
    }

    e = (sint32)d1->e - (sint32)d2->e - (DECIMAL_PARTS - m + n - (d1->m[m] >= d2->m[n] ? 1 : 0)) * DECIMAL_BASE_LOG10;

    if(e > DECIMAL_MAX_EXPONENT || e < DECIMAL_MIN_EXPONENT)
    {
        error_set(ERROR_DECIMAL_OVERFLOW);
        return 1;
    }

    d3->e = (sint8)e;

    d3->n = DECIMAL_POSITIONS - DECIMAL_BASE_LOG10 + j;

    d3->sign = ((d1->sign == d2->sign) ? DECIMAL_SIGN_POS : DECIMAL_SIGN_NEG);

    return 0;
}

// add two decimals if op >= 0: d3 = d1 + d2, subtract if op < 0
// return 0 on success, not 0 on error
sint8 decimal_add_sub(const decimal *d1, const decimal *d2, decimal *d3, sint8 op)
{
    sint32 shift, free, e;
    sint16 cmp;
    const decimal *n1, *n2;
    decimal s1, s2;

    // one of decimals is zero
    if(0 == d1->n)
    {
        memcpy(d3, d2, sizeof(*d3));
        if(op < 0)  // 0 - d2
        {
            d3->sign = -d3->sign;
        }
        return 0;
    }
    if(0 == d2->n)
    {
        memcpy(d3, d1, sizeof(*d3));
        return 0;
    }

    // assign d1 and d2 to n1 and n2 such that n1 has more significant digits than n2
    // (we want to save more digits while not sacrificing any significant digits)
    if((sint32)d1->e + (sint32)d1->n < (sint32)d2->e + (sint32)d2->n)
    {
        n1 = d2;
        n2 = d1;
    }
    else
    {
        n1 = d1;
        n2 = d2;
    }
    shift = (sint32)n1->e - (sint32)n2->e;
    e = (sint32)n1->e;

    // bring n1 and n2 to common exponent
    if(shift > 0)
    {
        memcpy(&s1, n1, sizeof(s1));
        memcpy(&s2, n2, sizeof(s2));

        free = DECIMAL_POSITIONS - s1.n;

        if(shift > free)
        {
            if(shift - free > (sint32)s2.n)
            {
                // n2 is too small
                memcpy(d3, n1, sizeof(*d3));
                return 0;
            }
            else
            {
                if(free > 0)
                {
                    decimal_shift_left(s1.m, free);
                }
                decimal_shift_right(s2.m, shift - free);
                e -= free;
            }
        }
        else
        {
            decimal_shift_left(s1.m, shift);
            e -= shift;
        }
        n1 = &s1;
        n2 = &s2;
    }
    else if (shift < 0)
    {
        memcpy(&s2, n2, sizeof(s2));
        decimal_shift_left(s2.m, -shift);
        n2 = &s2;
    }

    if((n1->sign != n2->sign && op >= 0) || (op < 0 && n1->sign == n2->sign))
    {
        // subtract
        cmp = decimal_abs_cmp(n1->m, n2->m);
        if(cmp > 0)
        {
            d3->sign = n1->sign;
            d3->e = e;
            decimal_abs_sub(n1->m, n2->m, d3->m);
            d3->n = decimal_num_digits(d3->m);
        }
        else if(cmp < 0)
        {
            d3->sign = n2->sign;
            d3->e = e;
            decimal_abs_sub(n2->m, n1->m, d3->m);
            d3->n = decimal_num_digits(d3->m);
        }
        else
        {
            d3->sign = DECIMAL_SIGN_POS;
            d3->e = d3->n = 0;
            memset(d3->m, 0, sizeof(d3->m));
        }
    }
    else
    {
        // add
        d3->sign = d1->sign;
        d3->e = e;
        if(decimal_abs_add(n1->m, n2->m, d3->m) > 0)
        {
            if(e == DECIMAL_MAX_EXPONENT)
            {
                error_set(ERROR_DECIMAL_OVERFLOW);
                return 1;
            }
            d3->e++;
            decimal_shift_right(d3->m, 1);
            d3->m[DECIMAL_PARTS - 1] += DECIMAL_BASE / 10;
            d3->n = DECIMAL_POSITIONS;
        }
        else
        {
            d3->n = decimal_num_digits(d3->m);
        }
    }
    return 0;
}

// subtract two decimals: d3 = d1 - d2
// return 0 on success, not 0 on error
inline sint8 decimal_sub(const decimal *d1, const decimal *d2, decimal *d3)
{
   return decimal_add_sub(d1, d2, d3, -1);
}

// add two decimals: d3 = d1 + d2
// return 0 on success, not 0 on error
inline sint8 decimal_add(const decimal *d1, const decimal *d2, decimal *d3)
{
   return decimal_add_sub(d1, d2, d3, 1);
}

// compare two decimals
// return positive if d1 > d2, negative if d1 < d2, 0 otherwise
sint16 decimal_cmp(const decimal *d1, const decimal *d2)
{
    if(d1->sign != d2->sign) return d1->sign;

    sint32 diff = (sint32)d1->e + (sint32)d1->n - (sint32)d2->e - (sint32)d2->n;

    if(diff > 0) return 1;
    if(diff < 0) return -1;

    if(d1->n != d2->n)
    {
        return decimal_abs_cmp_with_shift(d1->m, d1->n, d2->m, d2->n) * d1->sign;
    }

    return decimal_abs_cmp(d1->m, d2->m) * d1->sign;
}
