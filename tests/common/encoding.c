#include "tests.h"
#include "common/encoding.h"
#include <stdio.h>
#include <string.h>

int test_encoding_functions()
{
    uint8 chr_buf[ENCODING_MAXCHAR_LEN];
    char_info chr;
    chr.chr = chr_buf;
    uint8 chr2_buf[ENCODING_MAXCHAR_LEN];
    char_info chr2;
    chr2.chr = chr2_buf;

    encoding_init();

    puts("Testing character legth functions");
    encoding_char_len_fun charlen_fun = encoding_get_char_len_fun(ENCODING_UTF8);
    if(NULL == charlen_fun) return __LINE__;

    chr.chr[0] = 0x00;
    charlen_fun((const_char_info*)&chr);
    if(chr.length != 1) return __LINE__;

    chr.chr[0] = 0xFF;  // illegal
    charlen_fun((const_char_info*)&chr);
    if(chr.length != 0) return __LINE__;

    chr.chr[0] = 0x24;
    charlen_fun((const_char_info*)&chr);
    if(chr.length != 1) return __LINE__;

    chr.chr[0] = 0xC2;
    chr.chr[1] = 0xA2;
    charlen_fun((const_char_info*)&chr);
    if(chr.length != 2) return __LINE__;

    chr.chr[0] = 0xE0;
    chr.chr[1] = 0xA4;
    chr.chr[2] = 0xB9;
    charlen_fun((const_char_info*)&chr);
    if(chr.length != 3) return __LINE__;

    chr.chr[0] = 0xF0;
    chr.chr[1] = 0x90;
    chr.chr[2] = 0x8D;
    chr.chr[3] = 0x88;
    charlen_fun((const_char_info*)&chr);
    if(chr.length != 4) return __LINE__;


    charlen_fun = encoding_get_char_len_fun(ENCODING_ASCII);
    if(NULL == charlen_fun) return __LINE__;

    chr.chr[0] = 0x00;
    charlen_fun((const_char_info*)&chr);
    if(chr.length != 1) return __LINE__;

    chr.chr[0] = 0x7F;
    charlen_fun((const_char_info*)&chr);
    if(chr.length != 1) return __LINE__;

    chr.chr[0] = 0xFF;  // illegal ASCII
    charlen_fun((const_char_info*)&chr);
    if(chr.length != 1) return __LINE__;


    puts("Testing character conversion functions");

    if(encoding_is_convertable(ENCODING_ASCII, ENCODING_UTF8) != 1) return __LINE__;
    if(encoding_is_convertable(ENCODING_UTF8, ENCODING_ASCII) != 1) return __LINE__;

    encoding_conversion_fun ascii_to_utf8, utf8_to_ascii;

    ascii_to_utf8 = encoding_get_conversion_fun(ENCODING_ASCII, ENCODING_UTF8);
    utf8_to_ascii = encoding_get_conversion_fun(ENCODING_UTF8, ENCODING_ASCII);
    if(ascii_to_utf8 == NULL) return __LINE__;
    if(utf8_to_ascii == NULL) return __LINE__;
    if(NULL != encoding_get_conversion_fun(ENCODING_ASCII, ENCODING_UNKNOWN)) return __LINE__;
    if(NULL != encoding_get_conversion_fun(ENCODING_UTF8, ENCODING_UNKNOWN)) return __LINE__;

    chr.chr[0] = 0x7Fu;
    chr.length = 1;
    chr2.length = 0;
    chr2.chr[0] = 0;
    ascii_to_utf8((const_char_info*)&chr, &chr2);
    if(chr2.length != 1 || chr2.chr[0] != chr.chr[0]) return __LINE__;

    chr.chr[0] = 0x8Fu;
    chr.length = 1;
    chr2.length = 0;
    chr2.chr[0] = 0;
    ascii_to_utf8((const_char_info*)&chr, &chr2);
    if(chr2.length != 1 || chr2.chr[0] != (uint8)'?') return __LINE__;

    chr.chr[0] = 0x00u;
    chr.length = 1;
    chr2.length = 0;
    chr2.chr[0] = 0;
    ascii_to_utf8((const_char_info*)&chr, &chr2);
    if(chr2.length != 1 || chr2.chr[0] != chr.chr[0]) return __LINE__;


    chr.chr[0] = 0x7Fu;
    chr.length = 1;
    chr2.length = 0;
    chr2.chr[0] = 0;
    utf8_to_ascii((const_char_info*)&chr, &chr2);
    if(chr2.length != 1 || chr2.chr[0] != chr.chr[0]) return __LINE__;

    chr.chr[0] = 0x00u;
    chr.length = 1;
    chr2.length = 0;
    chr2.chr[0] = 0;
    utf8_to_ascii((const_char_info*)&chr, &chr2);
    if(chr2.length != 1 || chr2.chr[0] != chr.chr[0]) return __LINE__;

    chr.chr[0] = 0xC2;
    chr.chr[1] = 0xA2;
    chr.length = 2;
    chr2.length = 0;
    chr2.chr[0] = 0;
    utf8_to_ascii((const_char_info*)&chr, &chr2);
    if(chr2.length != 1 || chr2.chr[0] != (uint8)'?') return __LINE__;

    chr.chr[0] = 0xE0;
    chr.chr[1] = 0xA4;
    chr.chr[2] = 0xB9;
    chr.length = 3;
    utf8_to_ascii((const_char_info*)&chr, &chr2);
    if(chr2.length != 1 || chr2.chr[0] != (uint8)'?') return __LINE__;

    chr.chr[0] = 0xF0;
    chr.chr[1] = 0x90;
    chr.chr[2] = 0x8D;
    chr.chr[3] = 0x88;
    chr.length = 4;
    utf8_to_ascii((const_char_info*)&chr, &chr2);
    if(chr2.length != 1 || chr2.chr[0] != (uint8)'?') return __LINE__;


    puts("Testing terminator character test functions");

    chr.chr[0] = 0x00u;
    chr.length = 1;
    if(!encoding_is_string_terminator((const_char_info *)&chr, ENCODING_ASCII)) return __LINE__;
    if(!encoding_is_string_terminator((const_char_info *)&chr, ENCODING_UTF8)) return __LINE__;


    puts("Testing encoding names");

    const achar* encname;
    for(uint32 enc = 0; enc < ENCODING_NUM; enc++)
    {
        encname = encoding_name(enc);
        if(encname == NULL) return __LINE__*1000 + enc;
        for(uint32 enc2 = 0; enc2 < ENCODING_NUM; enc2++)
        {
            if((enc2 != enc) && (strcmp(encname, encoding_name(enc2)) == 0)) return __LINE__*1000 + enc;
        }
        if(encoding_idbyname(encname) != enc) return __LINE__*1000 + enc;
    }


    puts("Testing character building functions");

    // build character chr from stream of byte
    // if chr pointer is corrupt or invalid behaviour is undetermined
    typedef void (*encoding_build_char_fun)(char_info *chr, uint8 byte);

    encoding_build_char_fun ascii_build = encoding_get_build_char_fun(ENCODING_ASCII);
    encoding_build_char_fun utf8_build =  encoding_get_build_char_fun(ENCODING_UTF8);
    if(ascii_build == NULL) return __LINE__;
    if(utf8_build == NULL) return __LINE__;
    if(NULL != encoding_get_build_char_fun(ENCODING_UNKNOWN)) return __LINE__;

    memset(chr_buf, 0, ENCODING_MAXCHAR_LEN);
    chr.length = 0;
    chr.ptr = 0;
    chr.state = CHAR_STATE_INCOMPLETE;
    ascii_build(&chr, 0x7Fu);
    if(chr.chr[0] != 0x7Fu) return __LINE__;
    if(chr.length != 1) return __LINE__;
    if(chr.state != CHAR_STATE_COMPLETE) return __LINE__;

    memset(chr_buf, 0, ENCODING_MAXCHAR_LEN);
    chr.length = 0;
    chr.ptr = 0;
    chr.state = CHAR_STATE_INCOMPLETE;
    ascii_build(&chr, 0x8Fu);
    if(chr.state != CHAR_STATE_INVALID) return __LINE__;

    memset(chr_buf, 0, ENCODING_MAXCHAR_LEN);
    chr.length = 0;
    chr.ptr = 0;
    chr.state = CHAR_STATE_INCOMPLETE;
    utf8_build(&chr, 0x7Fu);
    if(chr.chr[0] != 0x7Fu) return __LINE__;
    if(chr.length != 1) return __LINE__;
    if(chr.state != CHAR_STATE_COMPLETE) return __LINE__;

    memset(chr_buf, 0, ENCODING_MAXCHAR_LEN);
    chr.length = 0;
    chr.ptr = 0;
    chr.state = CHAR_STATE_INCOMPLETE;
    utf8_build(&chr, 0xFFu);
    if(chr.state != CHAR_STATE_INVALID) return __LINE__;

    memset(chr_buf, 0, ENCODING_MAXCHAR_LEN);
    chr.length = 0;
    chr.ptr = 0;
    chr.state = CHAR_STATE_INCOMPLETE;
    utf8_build(&chr, 0xC2u);
    if(chr.state != CHAR_STATE_INCOMPLETE) return __LINE__;
    utf8_build(&chr, 0xA2u);
    if(chr.chr[0] != 0xC2u || chr.chr[1] != 0xA2u) return __LINE__;
    if(chr.length != 2) return __LINE__;
    if(chr.state != CHAR_STATE_COMPLETE) return __LINE__;

    memset(chr_buf, 0, ENCODING_MAXCHAR_LEN);
    chr.length = 0;
    chr.ptr = 0;
    chr.state = CHAR_STATE_INCOMPLETE;
    utf8_build(&chr, 0xC2u);
    utf8_build(&chr, 0xF0u);
    if(chr.state != CHAR_STATE_INVALID) return __LINE__;

    memset(chr_buf, 0, ENCODING_MAXCHAR_LEN);
    chr.length = 0;
    chr.ptr = 0;
    chr.state = CHAR_STATE_INCOMPLETE;
    utf8_build(&chr, 0xE0u);
    if(chr.state != CHAR_STATE_INCOMPLETE) return __LINE__;
    utf8_build(&chr, 0xA4u);
    if(chr.state != CHAR_STATE_INCOMPLETE) return __LINE__;
    utf8_build(&chr, 0xB9u);
    if(chr.chr[0] != 0xE0u || chr.chr[1] != 0xA4u || chr.chr[2] != 0xB9u) return __LINE__;
    if(chr.length != 3) return __LINE__;
    if(chr.state != CHAR_STATE_COMPLETE) return __LINE__;


    memset(chr_buf, 0, ENCODING_MAXCHAR_LEN);
    chr.length = 0;
    chr.ptr = 0;
    chr.state = CHAR_STATE_INCOMPLETE;
    utf8_build(&chr, 0xF0u);
    if(chr.state != CHAR_STATE_INCOMPLETE) return __LINE__;
    utf8_build(&chr, 0x90u);
    if(chr.state != CHAR_STATE_INCOMPLETE) return __LINE__;
    utf8_build(&chr, 0x8Du);
    if(chr.state != CHAR_STATE_INCOMPLETE) return __LINE__;
    utf8_build(&chr, 0x88u);
    if(chr.chr[0] != 0xF0u || chr.chr[1] != 0x90u || chr.chr[2] != 0x8Du || chr.chr[3] != 0x88u) return __LINE__;
    if(chr.length != 4) return __LINE__;
    if(chr.state != CHAR_STATE_COMPLETE) return __LINE__;

    return 0;
}

