#include "parser/lexer.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>


// lexer instance state
typedef struct _lexer_state
{
    // encoding conversion function
    void (*enc_conv) (const_char_info *src, char_info *dst);

    // buffer to hold characters
    uint8 l_buf[127][ENCODING_MAXCHAR_LEN];

    // ascii symbols converted to lexer enc
    char_info l_[127];

    // last read lexem
    lexer_lexem lexem;
} lexer_state;



// return size of the buffer for lexer initialization
size_t lexer_get_allocation_size()
{
    return sizeof(lexer_state);
}


// create and return lexical tokenizer instance
// returns NULL on error
handle lexer_create(void *buf, size_t buf_sz, encoding enc)
{
    lexer_state *ls = (lexer_state *)buf;

    ls->enc_conv = encoding_get_conversion_fun(ENCODING_ASCII, enc);

    assert(NULL != ls->enc_conv);

    char_info ascii[127];
    uint8 ascii_buf[ENCODING_MAXCHAR_LEN];
    for(uint8 i=0; i<127; i++)
    {
        ascii[i].length = 1;
        ascii[i].chr = ascii_buf;
        ascii_buf[0] = (uint8)i;

        ls->l_[i].chr = ls->l_buf[i];

        ls->enc_conv((const_char_info*)ascii + i, ls->l_ + i);
    }

    return (handle)ls;
}


// read and return next token
// return 0 on success, non-0 on error
sint8 lexer_next(handle lexer, lexer_lexem *lexem)
{
    return 0;
}


// read and return next token
// return 0 on success, non-0 on error
sint8 lexer_current(handle lexer, lexer_lexem *lexem)
{
    lexer_state *ls = (lexer_state *)lexer;
    memcpy(lexem, &ls->lexem, sizeof(lexer_lexem));
    return 0;
}


// destroy lexer
// return 0 on success, non-0 on error
sint8 lexer_destroy(handle lexer)
{
    return 0;
}


