#include "common/strop.h"
#include "common/dateop.h"
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

struct
{
    encoding enc;
    encoding_conversion_fun conversion_fun;

    uint8 digit_buf[10][ENCODING_MAXCHAR_LEN];
    char_info digits[10];

    uint8 fmt_letters_buf[5][ENCODING_MAXCHAR_LEN];
    char_info fmt_letter_y;
    char_info fmt_letter_m;
    char_info fmt_letter_d;
    char_info fmt_letter_h;
    char_info fmt_letter_s;
} g_strop_state = { .enc = ENCODING_UNKNOWN, .conversion_fun = NULL};

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

    g_strop_state.fmt_letter_y.chr = g_strop_state.fmt_letters_buf[0];
    g_strop_state.fmt_letter_d.chr = g_strop_state.fmt_letters_buf[1];
    g_strop_state.fmt_letter_m.chr = g_strop_state.fmt_letters_buf[2];
    g_strop_state.fmt_letter_h.chr = g_strop_state.fmt_letters_buf[3];
    g_strop_state.fmt_letter_s.chr = g_strop_state.fmt_letters_buf[4];

    ach = _ach('y'); g_strop_state.conversion_fun((const_char_info *)&achr, &(g_strop_state.fmt_letter_y));
    ach = _ach('m'); g_strop_state.conversion_fun((const_char_info *)&achr, &(g_strop_state.fmt_letter_m));
    ach = _ach('d'); g_strop_state.conversion_fun((const_char_info *)&achr, &(g_strop_state.fmt_letter_d));
    ach = _ach('h'); g_strop_state.conversion_fun((const_char_info *)&achr, &(g_strop_state.fmt_letter_h));
    ach = _ach('s'); g_strop_state.conversion_fun((const_char_info *)&achr, &(g_strop_state.fmt_letter_s));
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

sint8 strop_fmt_date(uint8 *buf, uint32 *start, uint32 sz, const achar* fmt, uint64 date)
{
    uint32 ptr = (*start);
    uint8 finished = 0, cnt;
    achar ch, ach;
    uint8 chr_buf[ENCODING_MAXCHAR_LEN];
    char_info achr;
    char_info chr;

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
                ptr += g_strop_state.fmt_letter_h.length;
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
