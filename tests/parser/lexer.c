#include "tests.h"
#include "parser/lexer.h"
#include "common/string_literal.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


struct
{
    uint64                  cur_char;
    const achar             *stmt;
    encoding_build_char_fun build_char;
    const achar             *expected_errmsg;
    error_code              expected_errcode;
} g_test_lexer_state = {0, NULL, NULL, NULL, ERROR_SYNTAX_ERROR};


sint8 test_lexer_char_feeder(char_info *ch, sint8 *eos)
{
    if(g_test_lexer_state.cur_char == strlen(g_test_lexer_state.stmt))
    {
        *eos = 1;
    }
    else
    {
        ch->length = 0u;
        ch->ptr = 0u;
        ch->state = CHAR_STATE_INCOMPLETE;
        *eos = 0;
        do
        {
            g_test_lexer_state.build_char(ch, g_test_lexer_state.stmt[g_test_lexer_state.cur_char++]);
        }
        while(ch->state != CHAR_STATE_COMPLETE);
    }

    return 0;
}


sint8 test_lexer_error_reporter(error_code error, const achar *msg)
{
    if(strcmp(msg, g_test_lexer_state.expected_errmsg)) return -1;
    if(error != g_test_lexer_state.expected_errcode) return -1;
    return 0;
}


int test_lexer_functions()
{
    puts("Starting test test_lexer_functions");

    uint8 buf[1024];        // all-purpose buf
    uint64 buf_sz;
    lexer_lexem lexem;
    encoding_init();
    g_test_lexer_state.build_char = encoding_get_build_char_fun(ENCODING_UTF8);
    g_test_lexer_state.stmt = _ach(" \t\r\n\t SELECT \t from \r\n aS   Where \n\n PRIMARY ");
    g_test_lexer_state.cur_char = 0;


    puts("Testing lexer_create");

    lexer_interface li;
    li.next_char = test_lexer_char_feeder;
    li.report_error = test_lexer_error_reporter;

    handle strlit = string_literal_create(malloc(string_literal_alloc_sz()));
    if(NULL == strlit) return __LINE__;

    handle lexer = lexer_create(malloc(lexer_get_allocation_size()), ENCODING_UTF8, strlit, li);
    if(NULL == lexer) return __LINE__;

    if(0 != lexer_reset(lexer)) return __LINE__;


    puts("Testing lexer_next for reserved words");

    // some of reserved words
    if(lexer_next(lexer, &lexem) != 0) return __LINE__;
//    printf("%d, %ld, %ld\n", lexem.type, lexem.line, lexem.col); fflush(NULL);
//    for(int i=0; i<lexem.identifier_len; i++) printf("%c", lexem.identifier[i]); puts(""); fflush(NULL);
    if(lexem.type != LEXEM_TYPE_RESERVED_WORD || lexem.reserved_word != LEXER_RESERVED_WORD_SELECT) return __LINE__;
    if(lexem.line != 2) return __LINE__;
    if(lexem.col != 3) return __LINE__;

    if(lexer_next(lexer, &lexem) != 0) return __LINE__;
    if(lexem.type != LEXEM_TYPE_RESERVED_WORD || lexem.reserved_word != LEXER_RESERVED_WORD_FROM) return __LINE__;
    if(lexem.line != 2) return __LINE__;
    if(lexem.col != 12) return __LINE__;

    if(lexer_next(lexer, &lexem) != 0) return __LINE__;
    if(lexem.type != LEXEM_TYPE_RESERVED_WORD || lexem.reserved_word != LEXER_RESERVED_WORD_AS) return __LINE__;
    if(lexem.line != 3) return __LINE__;
    if(lexem.col != 2) return __LINE__;

    if(lexer_next(lexer, &lexem) != 0) return __LINE__;
    if(lexem.type != LEXEM_TYPE_RESERVED_WORD || lexem.reserved_word != LEXER_RESERVED_WORD_WHERE) return __LINE__;
    if(lexem.line != 3) return __LINE__;
    if(lexem.col != 7) return __LINE__;

    if(lexer_next(lexer, &lexem) != 0) return __LINE__;
    if(lexem.type != LEXEM_TYPE_RESERVED_WORD || lexem.reserved_word != LEXER_RESERVED_WORD_PRIMARY) return __LINE__;
    if(lexem.line != 5) return __LINE__;
    if(lexem.col != 2) return __LINE__;

    if(lexer_next(lexer, &lexem) != 0) return __LINE__;
    if(lexem.type != LEXEM_TYPE_EOS) return __LINE__;


    puts("Testing lexer_next for identifiers");

    // different identifiers
    uint8 ident[LEXER_MAX_IDENTIFIER_LEN];
    const achar *identifier_list[] =  { "λ_ФФФ", "_indentifier_123", "a1", "col", "_", "c" };
    g_test_lexer_state.stmt = _ach("λ_ФФФ _indentifier_123 a1.col _ 1c");
    g_test_lexer_state.cur_char = 0;
    if(0 != lexer_reset(lexer)) return __LINE__;

    for(int i=0; i<6; i++)
    {
        if(lexer_next(lexer, &lexem) != 0) return i*1000000 + __LINE__;
        if(i == 3 || i ==5)
        {
            if(lexer_next(lexer, &lexem) != 0) return i*1000000 + __LINE__;
        }
        if(lexem.type != LEXEM_TYPE_IDENTIFIER) return i*1000000 + __LINE__;
        if(lexem.identifier_len > LEXER_MAX_IDENTIFIER_LEN) return i*1000000 + __LINE__;
        memset(ident, 0, sizeof(ident));
        memcpy(ident, identifier_list[i], strlen(identifier_list[i]));
        if(memcmp(lexem.identifier, ident, lexem.identifier_len)) return i*1000000 + __LINE__;
    }

    if(lexer_next(lexer, &lexem) != 0) return __LINE__;
    if(lexem.type != LEXEM_TYPE_EOS) return __LINE__;


    puts("Testing lexer_next for tokens");

    // all tokens
    const achar *tokens = _ach("!\"#$%&()*+,-./:;<=>?@[\\]^`{|}~");
    int base = LEXER_TOKEN_EXCL;
    g_test_lexer_state.stmt = tokens;
    g_test_lexer_state.cur_char = 0;
    if(0 != lexer_reset(lexer)) return __LINE__;

    for(int i=0; i<strlen(tokens); i++)
    {
        if(lexer_next(lexer, &lexem) != 0) return i*1000000 + __LINE__;
        if(lexem.type != LEXEM_TYPE_TOKEN) return i*1000000 + __LINE__;

        if(i == 6 || i == 25) base++;   // ' and _ are omitted
        if(lexem.token != i + base) return i*1000000 + __LINE__;
    }

    if(lexer_next(lexer, &lexem) != 0) return __LINE__;
    if(lexem.type != LEXEM_TYPE_EOS) return __LINE__;


    puts("Testing lexer_next for string literals");

    // unfinished string
    g_test_lexer_state.stmt = _ach(" \t\ncol = 'unfinished string literal");
    g_test_lexer_state.cur_char = 0;
    if(0 != lexer_reset(lexer)) return __LINE__;

    if(lexer_next(lexer, &lexem) != 0) return __LINE__;
    if(lexem.type != LEXEM_TYPE_IDENTIFIER) return __LINE__;
    if(lexer_next(lexer, &lexem) != 0) return __LINE__;
    if(lexem.type != LEXEM_TYPE_TOKEN) return __LINE__;

    g_test_lexer_state.expected_errmsg = _ach("string literal has no closing ' at line 2, column 7");
    if(lexer_next(lexer, &lexem) != 1) return __LINE__;

    // empty string
    g_test_lexer_state.stmt = _ach(" \t\n '' \n  \r ");
    g_test_lexer_state.cur_char = 0;
    if(0 != lexer_reset(lexer)) return __LINE__;

    if(lexer_next(lexer, &lexem) != 0) return __LINE__;
    if(lexem.type != LEXEM_TYPE_STR_LITERAL) return __LINE__;
    buf_sz = 1024;
    if(string_literal_read(lexem.str_literal, buf, &buf_sz) != 0) return __LINE__;
    if(buf_sz != 0) return __LINE__;

    if(lexer_next(lexer, &lexem) != 0) return __LINE__;
    if(lexem.type != LEXEM_TYPE_EOS) return __LINE__;

    // string with escaped '
    g_test_lexer_state.stmt = _ach("''''");
    g_test_lexer_state.cur_char = 0;
    if(0 != lexer_reset(lexer)) return __LINE__;

    if(lexer_next(lexer, &lexem) != 0) return __LINE__;
    if(lexem.type != LEXEM_TYPE_STR_LITERAL) return __LINE__;
    buf_sz = 1024;
    if(string_literal_read(lexem.str_literal, buf, &buf_sz) != 0) return __LINE__;
    if(buf_sz != 1) return __LINE__;
    if(buf[0] != (uint8)_ach('\'')) return __LINE__;

    if(lexer_next(lexer, &lexem) != 0) return __LINE__;
    if(lexem.type != LEXEM_TYPE_EOS) return __LINE__;

    const achar *str = _ach("some 'ͼͼͼ'");
    g_test_lexer_state.stmt = _ach("'some ''ͼͼͼ'''");
    g_test_lexer_state.cur_char = 0;
    if(0 != lexer_reset(lexer)) return __LINE__;

    if(lexer_next(lexer, &lexem) != 0) return __LINE__;
    if(lexem.type != LEXEM_TYPE_STR_LITERAL) return __LINE__;
    buf_sz = 1024;
    if(string_literal_read(lexem.str_literal, buf, &buf_sz) != 0) return __LINE__;
    if(buf_sz != strlen(str)) return __LINE__;
    if(memcmp(buf, str, buf_sz)) return __LINE__;

    if(lexer_next(lexer, &lexem) != 0) return __LINE__;
    if(lexem.type != LEXEM_TYPE_EOS) return __LINE__;


    puts("Testing lexer_next for decimals");

    // integer with no exponent
    g_test_lexer_state.stmt = _ach("\n12345678\t");
    g_test_lexer_state.cur_char = 0;
    if(0 != lexer_reset(lexer)) return __LINE__;

    if(lexer_next(lexer, &lexem) != 0) return __LINE__;
    if(lexem.type != LEXEM_TYPE_NUM_LITERAL) return __LINE__;
    if(lexem.num_literal.m[1] != 1234 || lexem.num_literal.m[0] != 5678) return __LINE__;
    if(lexem.num_literal.sign != DECIMAL_SIGN_POS || lexem.num_literal.e != 0) return __LINE__;

    if(lexer_next(lexer, &lexem) != 0) return __LINE__;
    if(lexem.type != LEXEM_TYPE_EOS) return __LINE__;

    // floating with no exponent
    g_test_lexer_state.stmt = _ach("1234.5678");
    g_test_lexer_state.cur_char = 0;
    if(0 != lexer_reset(lexer)) return __LINE__;

    if(lexer_next(lexer, &lexem) != 0) return __LINE__;
    if(lexem.type != LEXEM_TYPE_NUM_LITERAL) return __LINE__;
    if(lexem.num_literal.m[1] != 1234 || lexem.num_literal.m[0] != 5678) return __LINE__;
    if(lexem.num_literal.sign != DECIMAL_SIGN_POS || lexem.num_literal.e != -4) return __LINE__;

    if(lexer_next(lexer, &lexem) != 0) return __LINE__;
    if(lexem.type != LEXEM_TYPE_EOS) return __LINE__;

    // floating with exponent
    g_test_lexer_state.stmt = _ach("1234.5678e-124");
    g_test_lexer_state.cur_char = 0;
    if(0 != lexer_reset(lexer)) return __LINE__;

    if(lexer_next(lexer, &lexem) != 0) return __LINE__;
// decimal d3;
// memcpy(&d3, &lexem.num_literal, sizeof(decimal));
// printf("d3.m = %c %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d e=%d, n=%d\n", (d3.sign == DECIMAL_SIGN_NEG ? '-' : '+'), d3.m[9], d3.m[8], d3.m[7], d3.m[6], d3.m[5], d3.m[4], d3.m[3], d3.m[2], d3.m[1], d3.m[0], d3.e, d3.n);
    if(lexem.type != LEXEM_TYPE_NUM_LITERAL) return __LINE__;
    if(lexem.num_literal.m[1] != 1234 || lexem.num_literal.m[0] != 5678) return __LINE__;
    if(lexem.num_literal.sign != DECIMAL_SIGN_POS || lexem.num_literal.e != -128) return __LINE__;

    if(lexer_next(lexer, &lexem) != 0) return __LINE__;
    if(lexem.type != LEXEM_TYPE_EOS) return __LINE__;


    g_test_lexer_state.stmt = _ach("123456.E-124");
    g_test_lexer_state.cur_char = 0;
    if(0 != lexer_reset(lexer)) return __LINE__;

    if(lexer_next(lexer, &lexem) != 0) return __LINE__;
    if(lexem.type != LEXEM_TYPE_NUM_LITERAL) return __LINE__;
    if(lexem.num_literal.m[1] != 12 || lexem.num_literal.m[0] != 3456) return __LINE__;
    if(lexem.num_literal.sign != DECIMAL_SIGN_POS || lexem.num_literal.e != -124) return __LINE__;

    if(lexer_next(lexer, &lexem) != 0) return __LINE__;
    if(lexem.type != LEXEM_TYPE_EOS) return __LINE__;


    g_test_lexer_state.stmt = _ach("1234.5678e+131");
    g_test_lexer_state.cur_char = 0;
    if(0 != lexer_reset(lexer)) return __LINE__;

    if(lexer_next(lexer, &lexem) != 0) return __LINE__;
    if(lexem.type != LEXEM_TYPE_NUM_LITERAL) return __LINE__;
    if(lexem.num_literal.m[1] != 1234 || lexem.num_literal.m[0] != 5678) return __LINE__;
    if(lexem.num_literal.sign != DECIMAL_SIGN_POS || lexem.num_literal.e != 127) return __LINE__;

    if(lexer_next(lexer, &lexem) != 0) return __LINE__;
    if(lexem.type != LEXEM_TYPE_EOS) return __LINE__;


    // edge case: large mantissa
    g_test_lexer_state.stmt = _ach("1234567812345678123456781234567812345678e+10");
    g_test_lexer_state.cur_char = 0;
    if(0 != lexer_reset(lexer)) return __LINE__;

    if(lexer_next(lexer, &lexem) != 0) return __LINE__;
    if(lexem.type != LEXEM_TYPE_NUM_LITERAL) return __LINE__;
    if(lexem.num_literal.m[9] != 1234 || lexem.num_literal.m[0] != 5678) return __LINE__;
    if(lexem.num_literal.sign != DECIMAL_SIGN_POS || lexem.num_literal.e != 10) return __LINE__;

    if(lexer_next(lexer, &lexem) != 0) return __LINE__;
    if(lexem.type != LEXEM_TYPE_EOS) return __LINE__;


    // exponent overflow
    g_test_lexer_state.stmt = _ach("1234.5678e-125");
    g_test_lexer_state.cur_char = 0;
    g_test_lexer_state.expected_errmsg = _ach("numeric exponent overflow at line 1, column 1");
    if(0 != lexer_reset(lexer)) return __LINE__;

    if(lexer_next(lexer, &lexem) != 1) return __LINE__;


    g_test_lexer_state.stmt = _ach("1234.5678e+132");
    g_test_lexer_state.cur_char = 0;
    if(0 != lexer_reset(lexer)) return __LINE__;

    if(lexer_next(lexer, &lexem) != 1) return __LINE__;


    g_test_lexer_state.stmt = _ach("12345678e+132345");
    g_test_lexer_state.cur_char = 0;
    if(0 != lexer_reset(lexer)) return __LINE__;

    if(lexer_next(lexer, &lexem) != 1) return __LINE__;


    // mantissa overflow
    g_test_lexer_state.stmt = _ach("12345678123456781234567812345678123456780e+10");
    g_test_lexer_state.cur_char = 0;
    g_test_lexer_state.expected_errmsg = _ach("numeric mantissa overflow at line 1, column 1");
    if(0 != lexer_reset(lexer)) return __LINE__;

    if(lexer_next(lexer, &lexem) != 1) return __LINE__;


    // integer mode
    g_test_lexer_state.stmt = _ach("922337203685477579.123");
    g_test_lexer_state.cur_char = 0;
    if(0 != lexer_reset(lexer)) return __LINE__;
    lexer_num_mode_integer(lexer, 1);

    if(lexer_next(lexer, &lexem) != 0) return __LINE__;
    if(lexem.type != LEXEM_TYPE_NUM_LITERAL) return __LINE__;
    if(lexem.integer != 922337203685477579L) return __LINE__;

    if(lexer_next(lexer, &lexem) != 0) return __LINE__;
    if(lexem.type != LEXEM_TYPE_TOKEN || lexem.token != LEXER_TOKEN_DOT ) return __LINE__;

    g_test_lexer_state.stmt = _ach("922337203685477579e+123");
    g_test_lexer_state.cur_char = 0;
    if(0 != lexer_reset(lexer)) return __LINE__;
    lexer_num_mode_integer(lexer, 1);

    if(lexer_next(lexer, &lexem) != 0) return __LINE__;
    if(lexem.type != LEXEM_TYPE_NUM_LITERAL) return __LINE__;
    if(lexem.integer != 922337203685477579L) return __LINE__;

    if(lexer_next(lexer, &lexem) != 0) return __LINE__;


    // integer mode overflow
    g_test_lexer_state.stmt = _ach(" 9223372036854775808.0 ");
    g_test_lexer_state.cur_char = 0;
    g_test_lexer_state.expected_errmsg = _ach("integer is too big at line 1, column 2");
    if(0 != lexer_reset(lexer)) return __LINE__;
    lexer_num_mode_integer(lexer, 1);

    if(lexer_next(lexer, &lexem) != 1) return __LINE__;


    return 0;
}
