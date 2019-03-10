#ifndef _LEXER_H
#define _LEXER_H

typedef enum _sql_lexem_type
{
    RESERVED_WORD,  // e.g. SELECT, CREATE, etc.
    IDENTIFIER,     // e.g. column name
    BIND_VAR,       // bind variable
    LITERAL,        // e.g. string or number
    TOKEN           // e.g. "(" ";" "+"
} sql_lexem_type;

typedef struct _sql_lexem
{
    sql_lexem_type type;
    wchar          *value;
    uint32         value_size;
} sql_lexem;

// create and return lexical tokenizer
// returns NULL on error
handle lexer_create(handle stream);

// read and return next token
sql_lexem lexer_next(handle lexer);

// returns position of last read lexem
uint64 lexer_pos(handle lexer);

#endif
