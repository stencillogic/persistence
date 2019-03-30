#include "common/encoding.h"
#include <stddef.h>
#include <string.h>
#include <assert.h>

#define ENCODING_NUM 3

int g_aux_utf8_length_states[5] = {CHAR_STATE_INVALID, CHAR_STATE_COMPLETE,
    CHAR_STATE_INCOMPLETE, CHAR_STATE_INCOMPLETE, CHAR_STATE_INCOMPLETE};

//
// Conversion functions
//

void encoding_ascii_to_utf8(const_char_info *src, char_info *dst)
{
    dst->chr[0] = src->chr[0];
    dst->length = 1;
}

void encoding_utf8_to_ascii(const_char_info *src, char_info *dst)
{
    dst->chr[0] = (0x00u == (0x80u & src->chr[0])) ? src->chr[0] : 0x3Fu;
    dst->length = 1;
}

void encoding_zero_conversion(const_char_info *src, char_info *dst)
{
    memcpy(dst->chr, src->chr, src->length);
    dst->length = src->length;
}

encoding_conversion_fun g_encoding_converter_matrix[ENCODING_NUM][ENCODING_NUM] =
{
    {NULL, NULL, NULL},
    {NULL, encoding_zero_conversion, encoding_ascii_to_utf8},
    {NULL, encoding_utf8_to_ascii, encoding_zero_conversion}
};

void encoding_ascii_char_len(const_char_info *chr)
{
    chr->length = 1u;
}

void encoding_utf8_char_len(const_char_info *chr)
{
    uint8 byte = chr->chr[0];
    chr->length = (0x00u == (0x80u & byte)) ? 1 :
        (0xC0u == (0xE0u & byte)) ? 2 :
        (0xE0u == (0xF0u & byte)) ? 3 :
        (0xF0u == (0XF8u & byte)) ? 4 : 0;
}


//
// Building charater from byte stream functions
//

void encoding_build_utf8_char(char_info *chr, uint8 byte)
{
    if(0 == chr->ptr)
    {
        chr->chr[0] = byte;
        chr->ptr++;

        chr->length = (0x00u == (0x80u & byte)) ? 1 :
            (0xC0u == (0xE0u & byte)) ? 2 :
            (0xE0u == (0xF0u & byte)) ? 3 :
            (0xF0u == (0XF8u & byte)) ? 4 : 0;

        chr->state = g_aux_utf8_length_states[chr->length];
    }
    else
    {
        if(0x80u != (0xC0u & byte))
        {
            chr->state = CHAR_STATE_INVALID;
        }
        else
        {
            chr->chr[chr->ptr] = byte;
            chr->ptr++;
            if(chr->length == chr->ptr) chr->state = CHAR_STATE_COMPLETE;
        }
    }
}

void encoding_build_ascii_char(char_info *chr, uint8 byte)
{
    chr->chr[0] = byte;
    chr->length = 1;
    chr->state = (0x00u == (0x80u & byte)) ? CHAR_STATE_COMPLETE : CHAR_STATE_INVALID;
}

//
// Encoding functions
//

struct _encoding_props
{
    const achar* name;
    uint8 is_varlen;
    uint8 charlen;      // for fixed size char encodings
    uint8 string_terminator[ENCODING_MAXCHAR_LEN];
    uint8 string_terminator_len;
    encoding_build_char_fun build_char_fun;
    encoding_char_len_fun char_len_fun;
} g_encoding_props[ENCODING_NUM];

void encoding_set_entry(int id, const achar* name, uint8 is_varlen, uint8 charlen,
        uint8 *string_terminator, uint8 string_terminator_len, encoding_build_char_fun build_char_fun,
        encoding_char_len_fun char_len_fun)
{
    assert(id >= 0 && id < ENCODING_NUM);
    assert(string_terminator_len <= ENCODING_MAXCHAR_LEN);

    g_encoding_props[id].name = name;
    g_encoding_props[id].is_varlen = is_varlen;
    g_encoding_props[id].charlen = charlen;
    memcpy(g_encoding_props[id].string_terminator, string_terminator, string_terminator_len);
    g_encoding_props[id].string_terminator_len = string_terminator_len;
    g_encoding_props[id].build_char_fun = build_char_fun;
    g_encoding_props[id].char_len_fun = char_len_fun;
}

void encoding_init()
{
    uint8 terminator[ENCODING_MAXCHAR_LEN];
    memset(terminator, 0, ENCODING_MAXCHAR_LEN);
    encoding_set_entry(0, _ach("Unknown"), 0, 0, 0, 0, NULL, NULL);
    encoding_set_entry(1, _ach("ASCII"),   0, 1, terminator, 1, encoding_build_ascii_char, encoding_ascii_char_len);
    encoding_set_entry(3, _ach("UTF-8"),   1, 0, terminator, 1, encoding_build_utf8_char, encoding_utf8_char_len);
}

const achar* encoding_name(encoding enc)
{
    if(enc >= 0 && enc < ENCODING_NUM)
        return g_encoding_props[enc].name;

    return NULL;
}

encoding encoding_idbyname(const achar* name)
{
    for(int i = 0; i < ENCODING_NUM; i++)
    {
        if(0 == strcmp(name, g_encoding_props[i].name))
        {
            return (encoding)i;
        }
    }
    return ENCODING_UNKNOWN;
}

encoding_build_char_fun encoding_get_build_char_fun(encoding enc)
{
    assert(enc >= 0 && enc < ENCODING_NUM);
    return g_encoding_props[enc].build_char_fun;
}

encoding_conversion_fun encoding_get_conversion_fun(encoding from, encoding to)
{
    assert(from >= 0 && from < ENCODING_NUM);
    assert(to >= 0 && to < ENCODING_NUM);
    return g_encoding_converter_matrix[from][to];
}

sint8 encoding_is_string_terminator(const_char_info *chr, encoding enc)
{
    return (chr->length == g_encoding_props[enc].string_terminator_len)
        && (0 == memcmp(chr->chr, g_encoding_props[enc].string_terminator, chr->length));
}

sint8 encoding_is_convertable(encoding enc1, encoding enc2)
{
    assert(enc1 >= 0 && enc1 < ENCODING_NUM);
    assert(enc2 >= 0 && enc2 < ENCODING_NUM);

    return (g_encoding_converter_matrix[enc1][enc2] != NULL
            && g_encoding_converter_matrix[enc2][enc1] != NULL);
}

encoding_char_len_fun encoding_get_char_len_fun(encoding enc)
{
    assert(enc >= 0 && enc < ENCODING_NUM);
    return g_encoding_props[enc].char_len_fun;
}
