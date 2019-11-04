#ifndef _LEXER_H
#define _LEXER_H


// lexical analysator


#include "defs/defs.h"
#include "common/encoding.h"
#include "common/decimal.h"
#include "common/error.h"


#define LEXER_MAX_IDENTIFIER_LEN    (256)


typedef enum _lexer_lexem_type
{
    LEXEM_TYPE_RESERVED_WORD = 1,   // e.g. SELECT, CREATE, etc.
    LEXEM_TYPE_IDENTIFIER,          // e.g. column name
    LEXEM_TYPE_BIND_VAR,            // bind variable
    LEXEM_TYPE_STR_LITERAL,         // e.g. string
    LEXEM_TYPE_NUM_LITERAL,         // e.g. number
    LEXEM_TYPE_TOKEN,               // e.g. "(" ";" "+"
    LEXEM_TYPE_EOS                  // end of statement
} lexer_lexem_type;


#define LEXER_RESERVED_WORD_NUM     (71)

// the list must be sorted by ASCII code number (alphabetically)
typedef enum _lexer_reserved_word
{
    LEXER_RESERVED_WORD_ACTION = 1,
    LEXER_RESERVED_WORD_ADD,
    LEXER_RESERVED_WORD_ALL,
    LEXER_RESERVED_WORD_ALTER,
    LEXER_RESERVED_WORD_AND,
    LEXER_RESERVED_WORD_AS,
    LEXER_RESERVED_WORD_ASC,
    LEXER_RESERVED_WORD_BY,
    LEXER_RESERVED_WORD_CASCADE,
    LEXER_RESERVED_WORD_CHARACTER,
    LEXER_RESERVED_WORD_CHECK,
    LEXER_RESERVED_WORD_COLUMN,
    LEXER_RESERVED_WORD_CONSTRAINT,
    LEXER_RESERVED_WORD_CREATE,
    LEXER_RESERVED_WORD_CROSS,
    LEXER_RESERVED_WORD_DATABASE,
    LEXER_RESERVED_WORD_DATE,
    LEXER_RESERVED_WORD_DECIMAL,
    LEXER_RESERVED_WORD_DEFAULT,
    LEXER_RESERVED_WORD_DELETE,
    LEXER_RESERVED_WORD_DESC,
    LEXER_RESERVED_WORD_DISTINCT,
    LEXER_RESERVED_WORD_DOUBLE,
    LEXER_RESERVED_WORD_DROP,
    LEXER_RESERVED_WORD_EXCEPT,
    LEXER_RESERVED_WORD_FIRST,
    LEXER_RESERVED_WORD_FLOAT,
    LEXER_RESERVED_WORD_FOREIGN,
    LEXER_RESERVED_WORD_FROM,
    LEXER_RESERVED_WORD_FULL,
    LEXER_RESERVED_WORD_GROUP,
    LEXER_RESERVED_WORD_HAVING,
    LEXER_RESERVED_WORD_INNER,
    LEXER_RESERVED_WORD_INSERT,
    LEXER_RESERVED_WORD_INTEGER,
    LEXER_RESERVED_WORD_INTERSECT,
    LEXER_RESERVED_WORD_INTO,
    LEXER_RESERVED_WORD_IS,
    LEXER_RESERVED_WORD_JOIN,
    LEXER_RESERVED_WORD_KEY,
    LEXER_RESERVED_WORD_LAST,
    LEXER_RESERVED_WORD_LEFT,
    LEXER_RESERVED_WORD_MODIFY,
    LEXER_RESERVED_WORD_NO,
    LEXER_RESERVED_WORD_NOT,
    LEXER_RESERVED_WORD_NULL,
    LEXER_RESERVED_WORD_NULLS,
    LEXER_RESERVED_WORD_NUMBER,
    LEXER_RESERVED_WORD_ON,
    LEXER_RESERVED_WORD_OR,
    LEXER_RESERVED_WORD_ORDER,
    LEXER_RESERVED_WORD_OUTER,
    LEXER_RESERVED_WORD_PRECISION,
    LEXER_RESERVED_WORD_PRIMARY,
    LEXER_RESERVED_WORD_REFERENCES,
    LEXER_RESERVED_WORD_RENAME,
    LEXER_RESERVED_WORD_RESTRICT,
    LEXER_RESERVED_WORD_RIGHT,
    LEXER_RESERVED_WORD_SELECT,
    LEXER_RESERVED_WORD_SET,
    LEXER_RESERVED_WORD_SMALLINT,
    LEXER_RESERVED_WORD_TABLE,
    LEXER_RESERVED_WORD_TIMESTAMP,
    LEXER_RESERVED_WORD_TO,
    LEXER_RESERVED_WORD_UNION,
    LEXER_RESERVED_WORD_UNIQUE,
    LEXER_RESERVED_WORD_UPDATE,
    LEXER_RESERVED_WORD_VALUES,
    LEXER_RESERVED_WORD_VARCHAR,
    LEXER_RESERVED_WORD_VARYING,
    LEXER_RESERVED_WORD_WHERE,
} lexer_reserved_word;


typedef enum _lexer_token
{
    LEXER_TOKEN_EXCL = 1,           //   !
    LEXER_TOKEN_DOUBLE_QUOTE,       //   "
    LEXER_TOKEN_NUMBER,             //   #
    LEXER_TOKEN_DOLLAR,             //   $
    LEXER_TOKEN_PERCENT,            //   %
    LEXER_TOKEN_AMPERSAND,          //   &
    LEXER_TOKEN_SINGLE_QUOTE,       //   '
    LEXER_TOKEN_LPAR,               //   (
    LEXER_TOKEN_RPAR,               //   )
    LEXER_TOKEN_ASTERISK,           //   *
    LEXER_TOKEN_PLUS,               //   +
    LEXER_TOKEN_COMMA,              //   ,
    LEXER_TOKEN_MINUS,              //   -
    LEXER_TOKEN_DOT,                //   .
    LEXER_TOKEN_SLASH,              //   /
    LEXER_TOKEN_COLON,              //   :
    LEXER_TOKEN_SEMICOLON,          //   ;
    LEXER_TOKEN_LT,                 //   <
    LEXER_TOKEN_EQ,                 //   =
    LEXER_TOKEN_GT,                 //   >
    LEXER_TOKEN_QMARK,              //   ?
    LEXER_TOKEN_AT,                 //   @
    LEXER_TOKEN_LBRAK,              //   [
    LEXER_TOKEN_BSLASH,             //   back slash
    LEXER_TOKEN_RBRACK,             //   ]
    LEXER_TOKEN_CARET,              //   ^
    LEXER_TOKEN_UNDERSCORE,         //   _
    LEXER_TOKEN_ACCENT,             //   `
    LEXER_TOKEN_LBRACE,             //   {
    LEXER_TOKEN_BAR,                //   |
    LEXER_TOKEN_RBRACE,             //   }
    LEXER_TOKEN_TILDE,              //   ~
} lexer_token;


typedef struct _lexer_lexem
{
    lexer_lexem_type    type;
    lexer_reserved_word reserved_word;
    lexer_token         token;
    uint8               identifier[LEXER_MAX_IDENTIFIER_LEN];
    decimal             num_literal;
    sint64              integer;
    handle              str_literal;
    uint16              identifier_len; // length of identifier
    uint64              line;           // lexem line
    uint64              col;            // lexem start position in line
} lexer_lexem;


// lexer interface
typedef struct _lexer_interface
{
    // function to fetch next char
    sint8 (*next_char)(char_info *ch, sint8 *eos);

    // function to report error
    sint8 (*report_error)(error_code error, const achar *msg);
} lexer_interface;


// return size of the buffer for lexer initialization
size_t lexer_get_allocation_size();


// create and return lexical tokenizer instance for parsing strings of certain encoding
// returns NULL on error
handle lexer_create(void *buf, encoding enc, handle str_literal, lexer_interface li);


// reset lexer to start parsing of a new statement
// return 0 on success, non-0 on error
sint8 lexer_reset(handle lexer);


// read and return next token
// return 0 on success, 1 on syntax error, -1 on error
sint8 lexer_next(handle lexer, lexer_lexem *lexem);


// read and return last read token
// return 0 on success, non-0 on error
sint8 lexer_current(handle lexer, lexer_lexem *lexem);


// if set = 1 make lexer parse only integers, otherwise full decimal is parsed (default)
void lexer_num_mode_integer(handle lexer, uint8 set);


#endif
