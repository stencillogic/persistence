#include "common/strop.h"
#include "common/dateop.h"
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "common/error.h"

#define TIMESTAMP_TZ_MASK (0xFFF0000000000000UL)

struct
{
    encoding enc;
    encoding_conversion_fun conversion_fun;

    uint8 digit_buf[10][ENCODING_MAXCHAR_LEN];
    char_info digits[10];

    uint8 fmt_letters_buf[13][ENCODING_MAXCHAR_LEN];
    char_info fmt_letter_y;
    char_info fmt_letter_m;
    char_info fmt_letter_d;
    char_info fmt_letter_h;
    char_info fmt_letter_s;
    char_info fmt_letter_e;
    char_info fmt_letter_t;
    char_info sign_minus;
    char_info sign_plus;
    char_info space;
    char_info colon;
    char_info decimal_separator;
    char_info question_mark;
    uint32 pow10[6];
} g_strop_state = { .enc = ENCODING_UNKNOWN, .conversion_fun = NULL, .pow10 = {1, 10, 100, 1000, 10000, 100000}};


void strop_set_encoding(encoding enc)
{
    g_strop_state.enc = enc;
    g_strop_state.conversion_fun = encoding_get_conversion_fun(ENCODING_ASCII, enc);

    achar ach;
    char_info achr;
    achr.chr = (uint8*)&ach;
    achr.length = 1;

    for(int i=0; i<10; i++)
    {
        ach = _ach('0') + i;
        g_strop_state.digits[i].chr = g_strop_state.digit_buf[i];
        g_strop_state.conversion_fun((const_char_info *)&achr, &(g_strop_state.digits[i]));
    }

    g_strop_state.fmt_letter_y.chr      = g_strop_state.fmt_letters_buf[0];
    g_strop_state.fmt_letter_d.chr      = g_strop_state.fmt_letters_buf[1];
    g_strop_state.fmt_letter_m.chr      = g_strop_state.fmt_letters_buf[2];
    g_strop_state.fmt_letter_h.chr      = g_strop_state.fmt_letters_buf[3];
    g_strop_state.fmt_letter_s.chr      = g_strop_state.fmt_letters_buf[4];
    g_strop_state.fmt_letter_e.chr      = g_strop_state.fmt_letters_buf[5];
    g_strop_state.fmt_letter_t.chr      = g_strop_state.fmt_letters_buf[6];
    g_strop_state.sign_minus.chr        = g_strop_state.fmt_letters_buf[7];
    g_strop_state.sign_plus.chr         = g_strop_state.fmt_letters_buf[8];
    g_strop_state.space.chr             = g_strop_state.fmt_letters_buf[9];
    g_strop_state.decimal_separator.chr = g_strop_state.fmt_letters_buf[10];
    g_strop_state.question_mark.chr     = g_strop_state.fmt_letters_buf[11];
    g_strop_state.colon.chr             = g_strop_state.fmt_letters_buf[12];

    ach = _ach('y'); g_strop_state.conversion_fun((const_char_info *)&achr, &(g_strop_state.fmt_letter_y));
    ach = _ach('m'); g_strop_state.conversion_fun((const_char_info *)&achr, &(g_strop_state.fmt_letter_m));
    ach = _ach('d'); g_strop_state.conversion_fun((const_char_info *)&achr, &(g_strop_state.fmt_letter_d));
    ach = _ach('h'); g_strop_state.conversion_fun((const_char_info *)&achr, &(g_strop_state.fmt_letter_h));
    ach = _ach('s'); g_strop_state.conversion_fun((const_char_info *)&achr, &(g_strop_state.fmt_letter_s));
    ach = _ach('e'); g_strop_state.conversion_fun((const_char_info *)&achr, &(g_strop_state.fmt_letter_e));
    ach = _ach('t'); g_strop_state.conversion_fun((const_char_info *)&achr, &(g_strop_state.fmt_letter_t));
    ach = _ach('-'); g_strop_state.conversion_fun((const_char_info *)&achr, &(g_strop_state.sign_minus));
    ach = _ach('+'); g_strop_state.conversion_fun((const_char_info *)&achr, &(g_strop_state.sign_plus));
    ach = _ach(' '); g_strop_state.conversion_fun((const_char_info *)&achr, &(g_strop_state.space));
    ach = _ach('.'); g_strop_state.conversion_fun((const_char_info *)&achr, &(g_strop_state.decimal_separator));
    ach = _ach('?'); g_strop_state.conversion_fun((const_char_info *)&achr, &(g_strop_state.question_mark));
    ach = _ach(':'); g_strop_state.conversion_fun((const_char_info *)&achr, &(g_strop_state.colon));
}


// format uint64 number to string, put in buf starting from ptr, adjust ptr to point at the end of formatted value
// return 0 on success, non-0 on error
sint8 strop_fmt_uint64(uint8* buf, uint32 *ptr, uint32 sz, uint64 number)
{
    uint32 charlen, dgt;
    uint64 pos = 1;

    if(0 == number)
    {
        charlen = g_strop_state.digits[0].length;
        if(sz - (*ptr) < charlen) return 1; // buf overflow
        memcpy(buf + (*ptr), g_strop_state.digit_buf[0], charlen);
        (*ptr) += charlen;
    }
    else
    {
        while((number / pos) / 10 != 0)
        {
            pos *= 10;
        }

        do
        {
            dgt = (number / pos) % 10;
            charlen = g_strop_state.digits[dgt].length;
            if(sz - (*ptr) < charlen) return 1; // buf overflow
            memcpy(buf + (*ptr), g_strop_state.digit_buf[dgt], charlen);
            (*ptr) += charlen;
            pos /= 10;
        }
        while(pos != 0);
    }

    return 0;
}

// format uint64 number to string, put in buf starting from ptr, adjust ptr to point at the end of formatted value
// if number length less than pad, add 0 on left size to pad to desired padding length
// return 0 on success, non-0 on buffer overflow
sint8 strop_fmt_uint64_padded(uint8* buf, uint32 *ptr, uint32 sz, uint64 number, uint32 pad)
{
    uint32 charlen, dgt;
    uint64 pos = 1;

    if(pad == 0) return 0;

    pad--;
    while((number / pos) / 10 != 0 && pad > 0)
    {
        pos *= 10;
        pad--;
    }

    charlen = g_strop_state.digits[0].length;
    while(pad > 0)
    {
        if(sz - (*ptr) < charlen) return 1; // buf overflow
        memcpy(buf + (*ptr), g_strop_state.digit_buf[0], charlen);
        (*ptr) += charlen;
        pad--;
    }

    do
    {
        dgt = (number / pos) % 10;
        charlen = g_strop_state.digits[dgt].length;
        if(sz - (*ptr) < charlen) return 1; // buf overflow
        memcpy(buf + (*ptr), g_strop_state.digit_buf[dgt], charlen);
        (*ptr) += charlen;
        pos /= 10;
    }
    while(pos != 0);

    return 0;
}

sint8 strop_fmt_dttm_all(uint8 *buf, uint32 *start, uint32 sz, const achar* fmt, uint64 date, uint32 microseconds, sint16 tzminutes)
{
    uint32 ptr = (*start);
    uint8 finished = 0, cnt;
    achar ch, ach;
    uint8 chr_buf[ENCODING_MAXCHAR_LEN];
    char_info achr;
    char_info chr;
    const char_info *sign;

    chr.chr = chr_buf;
    achr.chr = (uint8*)&ach;
    achr.length = 1;

    assert(sz - ptr > 0 && buf != NULL && fmt != NULL);

    ch = *(fmt++);
    while(!finished)
    {
        if(ch == _ach('y'))
        {
            cnt = 0;
            do
            {
                ch = *(fmt++);
                cnt++;
            }
            while(ch == _ach('y') && cnt < 4);

            if(cnt == 4)
            {
                // yyyy
                if(0 != strop_fmt_uint64_padded(buf, &ptr, sz, dateop_extract_year(date), 4)) return 1;
            }
            else
            {
                while(cnt > 0)
                {
                    if(sz - ptr < g_strop_state.fmt_letter_y.length) return 1; // buf overflow
                    memcpy(buf + ptr, g_strop_state.fmt_letter_y.chr, g_strop_state.fmt_letter_y.length);
                    ptr += g_strop_state.fmt_letter_y.length;
                    cnt --;
                }
            }
        }
        else if(ch == _ach('m'))
        {
            ch = *(fmt++);
            if(ch == _ach('m'))
            {
                // month
                if(0 != strop_fmt_uint64_padded(buf, &ptr, sz, dateop_extract_month_of_year(date), 2)) return 1;
                ch = *(fmt++);
            }
            else if(ch == _ach('i'))
            {
                // minutes
                if(0 != strop_fmt_uint64_padded(buf, &ptr, sz, dateop_extract_minutes(date), 2)) return 1;
                ch = *(fmt++);
            }
            else
            {
                if(sz - ptr < g_strop_state.fmt_letter_m.length) return 1; // buf overflow
                memcpy(buf + ptr, g_strop_state.fmt_letter_m.chr, g_strop_state.fmt_letter_m.length);
                ptr += g_strop_state.fmt_letter_m.length;
            }
        }
        else if(ch == _ach('d'))
        {
            ch = *(fmt++);
            if(ch == _ach('d'))
            {
                // day of month
                if(0 != strop_fmt_uint64_padded(buf, &ptr, sz, dateop_extract_day_of_month(date), 2)) return 1;
                ch = *(fmt++);
            }
            else
            {
                if(sz - ptr < g_strop_state.fmt_letter_d.length) return 1; // buf overflow
                memcpy(buf + ptr, g_strop_state.fmt_letter_d.chr, g_strop_state.fmt_letter_d.length);
                ptr += g_strop_state.fmt_letter_d.length;
            }
        }
        else if(ch == _ach('h'))
        {
            ch = *(fmt++);
            if(ch == _ach('h'))
            {
                // hour
                if(0 != strop_fmt_uint64_padded(buf, &ptr, sz, (uint64)dateop_extract_hour(date), 2)) return 1;
                ch = *(fmt++);
            }
            else
            {
                if(sz - ptr < g_strop_state.fmt_letter_h.length) return 1; // buf overflow
                memcpy(buf + ptr, g_strop_state.fmt_letter_h.chr, g_strop_state.fmt_letter_h.length);
                ptr += g_strop_state.fmt_letter_h.length;
            }
        }
        else if(ch == _ach('s'))
        {
            ch = *(fmt++);
            if(ch == _ach('s'))
            {
                // second
                if(0 != strop_fmt_uint64_padded(buf, &ptr, sz, (uint64)dateop_extract_seconds(date), 2)) return 1;
                ch = *(fmt++);
            }
            else
            {
                if(sz - ptr < g_strop_state.fmt_letter_s.length) return 1; // buf overflow
                memcpy(buf + ptr, g_strop_state.fmt_letter_s.chr, g_strop_state.fmt_letter_s.length);
                ptr += g_strop_state.fmt_letter_s.length;
            }
        }
        else if(ch == _ach('f'))
        {
            cnt = 0;
            do
            {
                ch = *(fmt++);
                cnt++;
            }
            while(ch == _ach('f') && cnt < 6);

            if(0 != strop_fmt_uint64_padded(buf, &ptr, sz, microseconds / g_strop_state.pow10[6 - cnt], cnt)) return 1;
        }
        else if(ch == _ach('t'))
        {
            ch = *(fmt++);
            if(ch == _ach('z'))
            {
                if(tzminutes >= 0)
                {
                    sign = &g_strop_state.sign_plus;
                }
                else
                {
                    sign = &g_strop_state.sign_minus;
                    tzminutes = -tzminutes;
                }

                if(sz - ptr < sign->length) return 1; // buf overflow
                memcpy(buf + ptr, sign->chr, sign->length);
                ptr += sign->length;

                // tz hours
                if(0 != strop_fmt_uint64_padded(buf, &ptr, sz, tzminutes/60, 2)) return 1;

                // :
                if(sz - ptr < g_strop_state.colon.length) return 1; // buf overflow
                memcpy(buf + ptr, g_strop_state.colon.chr, g_strop_state.colon.length);
                ptr += g_strop_state.colon.length;

                // tz minutes
                if(0 != strop_fmt_uint64_padded(buf, &ptr, sz, tzminutes%60, 2)) return 1;

                ch = *(fmt++);
            }
            else
            {
                if(sz - ptr < g_strop_state.fmt_letter_t.length) return 1; // buf overflow
                memcpy(buf + ptr, g_strop_state.fmt_letter_t.chr, g_strop_state.fmt_letter_t.length);
                ptr += g_strop_state.fmt_letter_t.length;
            }
        }
        else if(ch == _ach('\0'))
        {
            finished = 1;
        }
        else
        {
            // simply write char to buf
            ach = ch;
            g_strop_state.conversion_fun((const_char_info*)&achr, &chr);
            if(sz - ptr < chr.length) return 1; // buf overflow
            memcpy(buf + ptr, chr_buf, chr.length);
            ptr += chr.length;

            ch = *(fmt++);
        }
    }

    (*start) = ptr;

    return 0;
}

/*
uint64 strop_len_utf8(const uint8 *str)
{
    return
}

uint64 strop_len(const uint8 *str)
{

}

// compare two strings
// return an -1, 0, 1 if, respectively, str1 < str2, str1 == str2, str1 > str2
sint8 strop_cmp(const uint8 *str1, const uint8 *str2)
{

}
*/

// format decimal value according to fmt and put result to buf + start
// return 0 on success, non-0 on error
// start value will be updated to point after formatted number
// format:
//   d - decimal digit
//   s - decimal separator
//   e - exponent
sint8 strop_fmt_decimal(uint8 *buf, uint32 *start, uint32 sz, const achar* fmt, decimal *d)
{
    sint32 n = 0, m = 0;
    sint8 finished = 0, state = 0;
    achar ch;

    // read and validate format
    while(!finished)
    {
        ch = *(fmt++);
        if(_ach('d') == ch)
        {
            if(state == 0)  // reading whole decimal positions
            {
                n++;
            }
            else if(state == 1) // reading decimal positions after decimal separator
            {
                m++;
            }
            else    // error
            {
                error_set(ERROR_INVALID_DECIMAL_FORMAT);
                return 1;
            }
        }
        else if(_ach('s') == ch)
        {
            if(state == 0)  // decimal separator
            {
                state = 1;
            }
            else
            {
                error_set(ERROR_INVALID_DECIMAL_FORMAT);
                return 1;
            }
        }
        else if(_ach('e') == ch)
        {
            if(state < 2)   // scientific format
            {
                state = 2;
            }
            else
            {
                error_set(ERROR_INVALID_DECIMAL_FORMAT);
                return 1;
            }
        }
        else if(_ach('\0') == ch)
        {
            finished = 1;
        }
        else
        {
            error_set(ERROR_INVALID_DECIMAL_FORMAT);
            return 1;
        }
    }

    return strop_fmt_decimal_pse(buf, start, sz, n + m, m, (state == 2 ? 1 : 0), d);
}

sint8 strop_fmt_decimal_pse(uint8 *buf, uint32 *start, uint32 sz, uint8 precision, uint8 scale, uint8 exp, decimal *d)
{
    uint32 ptr = (*start);
    sint32 p, k, n = precision - scale, m = scale;
    sint32 i, digit, e;
    char_info *sign;

    if(d->n > 0)
    {
        // we go through the digits starting from the most significant one, down to less significant
        // and put separator/exponent where we need to

        i = (d->n - 1) / DECIMAL_BASE_LOG10;
        k = d->m[i];
        p = 1;
        while(k / p >= 10) p *= 10;

        if(n >= (d->n + d->e) || exp > 0)
        {
            // sign
            if(d->sign == DECIMAL_SIGN_NEG)
            {
                if(sz - ptr < g_strop_state.sign_minus.length) return 1; // buf overflow
                memcpy(buf + ptr, g_strop_state.sign_minus.chr, g_strop_state.sign_minus.length);
                ptr += g_strop_state.sign_minus.length;
            }
            else
            {
                if(sz - ptr < g_strop_state.space.length) return 1; // buf overflow
                memcpy(buf + ptr, g_strop_state.space.chr, g_strop_state.space.length);
                ptr += g_strop_state.space.length;
            }

            if(exp > 0)
            {
                e = d->n + d->e - n;
            }
            else
            {
                if(n > 0 && d->n + d->e <= 0)
                {
                    // d < 1 and whole part requested
                    if(sz - ptr < g_strop_state.digits[0].length) return 1; // buf overflow
                    memcpy(buf + ptr, g_strop_state.digits[0].chr, g_strop_state.digits[0].length);
                    ptr += g_strop_state.digits[0].length;
                }

                n = (d->n + d->e);
            }

            do
            {
                while(n > 0)
                {
                    if(i >= 0)
                    {
                        digit = (d->m[i] / p) % 10;
                        p /= 10;
                        if(p == 0)
                        {
                            i--;
                            p = DECIMAL_BASE / 10;
                        }
                    }
                    else
                    {
                        digit = 0;
                    }

                    if(sz - ptr < g_strop_state.digits[digit].length) return 1; // buf overflow
                    memcpy(buf + ptr, g_strop_state.digits[digit].chr, g_strop_state.digits[digit].length);
                    ptr += g_strop_state.digits[digit].length;

                    n--;
                }

                if(m > 0)
                {
                    if(sz - ptr < g_strop_state.decimal_separator.length) return 1; // buf overflow
                    memcpy(buf + ptr, g_strop_state.decimal_separator.chr, g_strop_state.decimal_separator.length);
                    ptr += g_strop_state.decimal_separator.length;

                    // note "n = (d->n + d->e);" above can produce negative n
                    while(n < 0 && m > 0)
                    {
                        // d < 1 and whole part requested
                        if(sz - ptr < g_strop_state.digits[0].length) return 1; // buf overflow
                        memcpy(buf + ptr, g_strop_state.digits[0].chr, g_strop_state.digits[0].length);
                        ptr += g_strop_state.digits[0].length;
                        n++;
                        m--;
                    }

                    n = m;
                    m = 0;
                }
            }
            while(n > 0);

            if(exp > 0)
            {
                if(sz - ptr < g_strop_state.fmt_letter_e.length) return 1; // buf overflow
                memcpy(buf + ptr, g_strop_state.fmt_letter_e.chr, g_strop_state.fmt_letter_e.length);
                ptr += g_strop_state.fmt_letter_e.length;

                sign = (e >= 0) ? &g_strop_state.sign_plus : &g_strop_state.sign_minus;
                if(sz - ptr < sign->length) return 1; // buf overflow
                memcpy(buf + ptr, sign->chr, sign->length);
                ptr += sign->length;

                if(0 != strop_fmt_uint64(buf, &ptr, sz, (uint64)(e >= 0 ? e : -e))) return 1;
            }
        }
        else
        {
            // format requested less significant number than decimal contains
            // we can't print that without loosing data
            if(sz - ptr < g_strop_state.question_mark.length) return 1; // buf overflow
            memcpy(buf + ptr, g_strop_state.question_mark.chr, g_strop_state.question_mark.length);
            ptr += g_strop_state.question_mark.length;
        }
    }
    else
    {
        // format zero
        if(n > 0)
        {
            if(sz - ptr < g_strop_state.digits[0].length) return 1; // buf overflow
            memcpy(buf + ptr, g_strop_state.digits[0].chr, g_strop_state.digits[0].length);
            ptr += g_strop_state.digits[0].length;
        }

        if(m > 0)
        {
            if(sz - ptr < g_strop_state.decimal_separator.length) return 1; // buf overflow
            memcpy(buf + ptr, g_strop_state.decimal_separator.chr, g_strop_state.decimal_separator.length);
            ptr += g_strop_state.decimal_separator.length;

            if(sz - ptr < m * g_strop_state.digits[0].length) return 1; // buf overflow

            do
            {
                memcpy(buf + ptr, g_strop_state.digits[0].chr, g_strop_state.digits[0].length);
                ptr += g_strop_state.digits[0].length;
                m--;
            }
            while(m > 0);
        }

        if(exp > 0)
        {
            // add exponent
            if(sz - ptr < g_strop_state.digits[0].length + g_strop_state.fmt_letter_e.length + g_strop_state.sign_plus.length) return 1; // buf overflow

            memcpy(buf + ptr, g_strop_state.fmt_letter_e.chr, g_strop_state.fmt_letter_e.length);
            ptr += g_strop_state.fmt_letter_e.length;

            memcpy(buf + ptr, g_strop_state.sign_plus.chr, g_strop_state.sign_plus.length);
            ptr += g_strop_state.sign_plus.length;

            memcpy(buf + ptr, g_strop_state.digits[0].chr, g_strop_state.digits[0].length);
            ptr += g_strop_state.digits[0].length;
        }
    }

    (*start) = ptr;
    return 0;
}

sint8 strop_fmt_date(uint8 *buf, uint32 *start, uint32 sz, const achar* fmt, uint64 date)
{
    return strop_fmt_dttm_all(buf, start, sz, fmt, date, 0, 0);
}

sint8 strop_fmt_timestamp(uint8 *buf, uint32 *start, uint32 sz, const achar* fmt, uint64 ts)
{
    return strop_fmt_dttm_all(buf, start, sz, fmt, ts/1000000, ts%1000000, 0);
}

sint8 strop_fmt_timestamp_with_tz(uint8 *buf, uint32 *start, uint32 sz, const achar* fmt, uint64 ts, sint16 tz)
{
    return strop_fmt_dttm_all(buf, start, sz, fmt, ts/1000000, ts%1000000, tz);
}

