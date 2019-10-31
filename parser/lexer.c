#include "parser/lexer.h"
#include "common/hash_map.h"
#include "session/pproto_server.h"
#include "common/error.h"
#include "common/string_literal.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>


#define LEXER_ERRMES_BUF_SZ     (1024)

// sql character types
typedef enum _lexer_char_type
{
   LEXER_CHAR_TYPE_UNDEFINED,
   LEXER_CHAR_TYPE_UCASE_LETTER,        // A-Z
   LEXER_CHAR_TYPE_LCASE_LETTER,        // a-z
   LEXER_CHAR_TYPE_DIGIT,               // 0-9
   LEXER_CHAR_TYPE_SPECIAL,             // `~.,/?;:'"\|!@#$%^&*()[]-_+<>{}= space, tab, CR, LF
   LEXER_CHAR_TYPE_OTHER,
   LEXER_CHAR_TYPE_EOS                  // no more input
} lexer_char_type;


// character description
typedef struct _lexer_char
{
    lexer_char_type type;
    uint8 ch_buf[2][ENCODING_MAXCHAR_LEN];
    char_info chi;
    char_info ach;
} lexer_char;


// first letter diapasones
int g_lexer_reserved_word_diap[27] =
{
//  A   B   C   D   E   F   G   H   I   J   K   L   M   N   O   P   Q   R   S   T   U   V   W   X   Y   Z   <top>
    1,  8,  9, 16, 25, 26, 31, 32, 33, 39, 40, 41, 43, 44, 49, 53,  0, 55, 59, 62, 65, 68, 71,  0,  0,  0,  0
};


// the order must correspnd to the oreder of enum lexer_reserved_word
// and sorted by ASCII code number (alphabetically)
const achar *g_lexer_reserved_words[] =
{
    NULL,
    _ach("ACTION"),
    _ach("ADD"),
    _ach("ALL"),
    _ach("ALTER"),
    _ach("AND"),
    _ach("AS"),
    _ach("ASC"),
    _ach("BY"),
    _ach("CASCADE"),
    _ach("CHARACTER"),
    _ach("CHECK"),
    _ach("COLUMN"),
    _ach("CONSTRAINT"),
    _ach("CREATE"),
    _ach("CROSS"),
    _ach("DATABASE"),
    _ach("DATE"),
    _ach("DECIMAL"),
    _ach("DEFAULT"),
    _ach("DELETE"),
    _ach("DESC"),
    _ach("DISTINCT"),
    _ach("DOUBLE"),
    _ach("DROP"),
    _ach("EXCEPT"),
    _ach("FIRST"),
    _ach("FLOAT"),
    _ach("FOREIGN"),
    _ach("FROM"),
    _ach("FULL"),
    _ach("GROUP"),
    _ach("HAVING"),
    _ach("INNER"),
    _ach("INSERT"),
    _ach("INTEGER"),
    _ach("INTERSECT"),
    _ach("INTO"),
    _ach("IS"),
    _ach("JOIN"),
    _ach("KEY"),
    _ach("LAST"),
    _ach("LEFT"),
    _ach("MODIFY"),
    _ach("NO"),
    _ach("NOT"),
    _ach("NULL"),
    _ach("NULLS"),
    _ach("NUMBER"),
    _ach("ON"),
    _ach("OR"),
    _ach("ORDER"),
    _ach("OUTER"),
    _ach("PRECISION"),
    _ach("PRIMARY"),
    _ach("REFERENCES"),
    _ach("RENAME"),
    _ach("RESTRICT"),
    _ach("RIGHT"),
    _ach("SELECT"),
    _ach("SET"),
    _ach("SMALLINT"),
    _ach("TABLE"),
    _ach("TIMESTAMP"),
    _ach("TO"),
    _ach("UNION"),
    _ach("UNIQUE"),
    _ach("UPDATE"),
    _ach("VALUES"),
    _ach("VARCHAR"),
    _ach("VARYING"),
    _ach("WHERE"),
};


// map ASCII code to lexer_char_type
lexer_char_type g_lexer_char_type[128] =
{
    LEXER_CHAR_TYPE_OTHER,              // 0        NUL     Null
    LEXER_CHAR_TYPE_OTHER,              // 1        SOH     Start of Header
    LEXER_CHAR_TYPE_OTHER,              // 2        STX     Start of Text
    LEXER_CHAR_TYPE_OTHER,              // 3        ETX     End of Text
    LEXER_CHAR_TYPE_OTHER,              // 4        EOT     End of Transmission
    LEXER_CHAR_TYPE_OTHER,              // 5        ENQ     Enquiry
    LEXER_CHAR_TYPE_OTHER,              // 6        ACK     Acknowledge
    LEXER_CHAR_TYPE_OTHER,              // 7        BEL     Bell
    LEXER_CHAR_TYPE_OTHER,              // 8        BS     Backspace
    LEXER_CHAR_TYPE_SPECIAL,            // 9        HT     Horizontal Tab
    LEXER_CHAR_TYPE_SPECIAL,            // 10       LF     Line Feed
    LEXER_CHAR_TYPE_OTHER,              // 11       VT     Vertical Tab
    LEXER_CHAR_TYPE_OTHER,              // 12       FF     Form Feed
    LEXER_CHAR_TYPE_SPECIAL,            // 13       CR     Carriage Return
    LEXER_CHAR_TYPE_OTHER,              // 14       SO     Shift Out
    LEXER_CHAR_TYPE_OTHER,              // 15       SI     Shift In
    LEXER_CHAR_TYPE_OTHER,              // 16       DLE     Data Link Escape
    LEXER_CHAR_TYPE_OTHER,              // 17       DC1     Device Control 1
    LEXER_CHAR_TYPE_OTHER,              // 18       DC2     Device Control 2
    LEXER_CHAR_TYPE_OTHER,              // 19       DC3     Device Control 3
    LEXER_CHAR_TYPE_OTHER,              // 20       DC4     Device Control 4
    LEXER_CHAR_TYPE_OTHER,              // 21       NAK     Negative Acknowledge
    LEXER_CHAR_TYPE_OTHER,              // 22       SYN     Synchronize
    LEXER_CHAR_TYPE_OTHER,              // 23       ETB     End of Transmission Block
    LEXER_CHAR_TYPE_OTHER,              // 24       CAN     Cancel
    LEXER_CHAR_TYPE_OTHER,              // 25       EM     End of Medium
    LEXER_CHAR_TYPE_OTHER,              // 26       SUB     Substitute
    LEXER_CHAR_TYPE_OTHER,              // 27       ESC     Escape
    LEXER_CHAR_TYPE_OTHER,              // 28       FS     File Separator
    LEXER_CHAR_TYPE_OTHER,              // 29       GS     Group Separator
    LEXER_CHAR_TYPE_OTHER,              // 30       RS     Record Separator
    LEXER_CHAR_TYPE_OTHER,              // 31       US     Unit Separator
    LEXER_CHAR_TYPE_SPECIAL,            // 32       space     Space
    LEXER_CHAR_TYPE_SPECIAL,            // 33       !     Exclamation mark
    LEXER_CHAR_TYPE_SPECIAL,            // 34       "     Double quote
    LEXER_CHAR_TYPE_SPECIAL,            // 35       #     Number
    LEXER_CHAR_TYPE_SPECIAL,            // 36       $     Dollar sign
    LEXER_CHAR_TYPE_SPECIAL,            // 37       %     Percent
    LEXER_CHAR_TYPE_SPECIAL,            // 38       &     Ampersand
    LEXER_CHAR_TYPE_SPECIAL,            // 39       '     Single quote
    LEXER_CHAR_TYPE_SPECIAL,            // 40       (     Left parenthesis
    LEXER_CHAR_TYPE_SPECIAL,            // 41       )     Right parenthesis
    LEXER_CHAR_TYPE_SPECIAL,            // 42       *     Asterisk
    LEXER_CHAR_TYPE_SPECIAL,            // 43       +     Plus
    LEXER_CHAR_TYPE_SPECIAL,            // 44       ,     Comma
    LEXER_CHAR_TYPE_SPECIAL,            // 45       -     Minus
    LEXER_CHAR_TYPE_SPECIAL,            // 46       .     Period
    LEXER_CHAR_TYPE_SPECIAL,            // 47       /     Slash
    LEXER_CHAR_TYPE_DIGIT,              // 48       0     Zero
    LEXER_CHAR_TYPE_DIGIT,              // 49       1     One
    LEXER_CHAR_TYPE_DIGIT,              // 50       2     Two
    LEXER_CHAR_TYPE_DIGIT,              // 51       3     Three
    LEXER_CHAR_TYPE_DIGIT,              // 52       4     Four
    LEXER_CHAR_TYPE_DIGIT,              // 53       5     Five
    LEXER_CHAR_TYPE_DIGIT,              // 54       6     Six
    LEXER_CHAR_TYPE_DIGIT,              // 55       7     Seven
    LEXER_CHAR_TYPE_DIGIT,              // 56       8     Eight
    LEXER_CHAR_TYPE_DIGIT,              // 57       9     Nine
    LEXER_CHAR_TYPE_SPECIAL,            // 58       :     Colon
    LEXER_CHAR_TYPE_SPECIAL,            // 59       ;     Semicolon
    LEXER_CHAR_TYPE_SPECIAL,            // 60       <     Less than
    LEXER_CHAR_TYPE_SPECIAL,            // 61       =     Equality sign
    LEXER_CHAR_TYPE_SPECIAL,            // 62       >     Greater than
    LEXER_CHAR_TYPE_SPECIAL,            // 63       ?     Question mark
    LEXER_CHAR_TYPE_SPECIAL,            // 64       @     At sign
    LEXER_CHAR_TYPE_UCASE_LETTER,       // 65       A     Capital A
    LEXER_CHAR_TYPE_UCASE_LETTER,       // 66       B     Capital B
    LEXER_CHAR_TYPE_UCASE_LETTER,       // 67       C     Capital C
    LEXER_CHAR_TYPE_UCASE_LETTER,       // 68       D     Capital D
    LEXER_CHAR_TYPE_UCASE_LETTER,       // 69       E     Capital E
    LEXER_CHAR_TYPE_UCASE_LETTER,       // 70       F     Capital F
    LEXER_CHAR_TYPE_UCASE_LETTER,       // 71       G     Capital G
    LEXER_CHAR_TYPE_UCASE_LETTER,       // 72       H     Capital H
    LEXER_CHAR_TYPE_UCASE_LETTER,       // 73       I     Capital I
    LEXER_CHAR_TYPE_UCASE_LETTER,       // 74       J     Capital J
    LEXER_CHAR_TYPE_UCASE_LETTER,       // 75       K     Capital K
    LEXER_CHAR_TYPE_UCASE_LETTER,       // 76       L     Capital L
    LEXER_CHAR_TYPE_UCASE_LETTER,       // 77       M     Capital M
    LEXER_CHAR_TYPE_UCASE_LETTER,       // 78       N     Capital N
    LEXER_CHAR_TYPE_UCASE_LETTER,       // 79       O     Capital O
    LEXER_CHAR_TYPE_UCASE_LETTER,       // 80       P     Capital P
    LEXER_CHAR_TYPE_UCASE_LETTER,       // 81       Q     Capital Q
    LEXER_CHAR_TYPE_UCASE_LETTER,       // 82       R     Capital R
    LEXER_CHAR_TYPE_UCASE_LETTER,       // 83       S     Capital S
    LEXER_CHAR_TYPE_UCASE_LETTER,       // 84       T     Capital T
    LEXER_CHAR_TYPE_UCASE_LETTER,       // 85       U     Capital U
    LEXER_CHAR_TYPE_UCASE_LETTER,       // 86       V     Capital V
    LEXER_CHAR_TYPE_UCASE_LETTER,       // 87       W     Capital W
    LEXER_CHAR_TYPE_UCASE_LETTER,       // 88       X     Capital X
    LEXER_CHAR_TYPE_UCASE_LETTER,       // 89       Y     Capital Y
    LEXER_CHAR_TYPE_UCASE_LETTER,       // 90       Z     Capital Z
    LEXER_CHAR_TYPE_SPECIAL,            // 91       [     Left square bracket
    LEXER_CHAR_TYPE_SPECIAL,            // 92       \     Backslash
    LEXER_CHAR_TYPE_SPECIAL,            // 93       ]     Right square bracket
    LEXER_CHAR_TYPE_SPECIAL,            // 94       ^     Caret / circumflex
    LEXER_CHAR_TYPE_SPECIAL,            // 95       _     Underscore
    LEXER_CHAR_TYPE_SPECIAL,            // 96       `     Grave / accent
    LEXER_CHAR_TYPE_LCASE_LETTER,       // 97       a     Small a
    LEXER_CHAR_TYPE_LCASE_LETTER,       // 98       b     Small b
    LEXER_CHAR_TYPE_LCASE_LETTER,       // 99       c     Small c
    LEXER_CHAR_TYPE_LCASE_LETTER,       // 100      d     Small d
    LEXER_CHAR_TYPE_LCASE_LETTER,       // 101      e     Small e
    LEXER_CHAR_TYPE_LCASE_LETTER,       // 102      f     Small f
    LEXER_CHAR_TYPE_LCASE_LETTER,       // 103      g     Small g
    LEXER_CHAR_TYPE_LCASE_LETTER,       // 104      h     Small h
    LEXER_CHAR_TYPE_LCASE_LETTER,       // 105      i     Small i
    LEXER_CHAR_TYPE_LCASE_LETTER,       // 106      j     Small j
    LEXER_CHAR_TYPE_LCASE_LETTER,       // 107      k     Small k
    LEXER_CHAR_TYPE_LCASE_LETTER,       // 108      l     Small l
    LEXER_CHAR_TYPE_LCASE_LETTER,       // 109      m     Small m
    LEXER_CHAR_TYPE_LCASE_LETTER,       // 110      n     Small n
    LEXER_CHAR_TYPE_LCASE_LETTER,       // 111      o     Small o
    LEXER_CHAR_TYPE_LCASE_LETTER,       // 112      p     Small p
    LEXER_CHAR_TYPE_LCASE_LETTER,       // 113      q     Small q
    LEXER_CHAR_TYPE_LCASE_LETTER,       // 114      r     Small r
    LEXER_CHAR_TYPE_LCASE_LETTER,       // 115      s     Small s
    LEXER_CHAR_TYPE_LCASE_LETTER,       // 116      t     Small t
    LEXER_CHAR_TYPE_LCASE_LETTER,       // 117      u     Small u
    LEXER_CHAR_TYPE_LCASE_LETTER,       // 118      v     Small v
    LEXER_CHAR_TYPE_LCASE_LETTER,       // 119      w     Small w
    LEXER_CHAR_TYPE_LCASE_LETTER,       // 120      x     Small x
    LEXER_CHAR_TYPE_LCASE_LETTER,       // 121      y     Small y
    LEXER_CHAR_TYPE_LCASE_LETTER,       // 122      z     Small z
    LEXER_CHAR_TYPE_SPECIAL,            // 123      {     Left curly bracket
    LEXER_CHAR_TYPE_SPECIAL,            // 124      |     Vertical bar
    LEXER_CHAR_TYPE_SPECIAL,            // 125      }     Right curly bracket
    LEXER_CHAR_TYPE_SPECIAL,            // 126      ~     Tilde
    LEXER_CHAR_TYPE_OTHER,              // 127      DEL     Delete
};

// map ASCII code to lexer_char_type
lexer_token g_lexer_ascii_to_token[128] =
{
    0,              // 0        NUL     Null
    0,              // 1        SOH     Start of Header
    0,              // 2        STX     Start of Text
    0,              // 3        ETX     End of Text
    0,              // 4        EOT     End of Transmission
    0,              // 5        ENQ     Enquiry
    0,              // 6        ACK     Acknowledge
    0,              // 7        BEL     Bell
    0,              // 8        BS     Backspace
    0,              // 9        HT     Horizontal Tab
    0,              // 10       LF     Line Feed
    0,              // 11       VT     Vertical Tab
    0,              // 12       FF     Form Feed
    0,              // 13       CR     Carriage Return
    0,              // 14       SO     Shift Out
    0,              // 15       SI     Shift In
    0,              // 16       DLE     Data Link Escape
    0,              // 17       DC1     Device Control 1
    0,              // 18       DC2     Device Control 2
    0,              // 19       DC3     Device Control 3
    0,              // 20       DC4     Device Control 4
    0,              // 21       NAK     Negative Acknowledge
    0,              // 22       SYN     Synchronize
    0,              // 23       ETB     End of Transmission Block
    0,              // 24       CAN     Cancel
    0,              // 25       EM     End of Medium
    0,              // 26       SUB     Substitute
    0,              // 27       ESC     Escape
    0,              // 28       FS     File Separator
    0,              // 29       GS     Group Separator
    0,              // 30       RS     Record Separator
    0,              // 31       US     Unit Separator
    LEXER_TOKEN_EXCL,            // 33       !     Exclamation mark
    LEXER_TOKEN_DOUBLE_QUOTE,            // 34       "     Double quote
    LEXER_TOKEN_NUMBER,            // 35       #     Number
    LEXER_TOKEN_DOLLAR,            // 36       $     Dollar sign
    LEXER_TOKEN_PERCENT,            // 37       %     Percent
    LEXER_TOKEN_AMPERSAND,            // 38       &     Ampersand
    LEXER_TOKEN_SINGLE_QUOTE,            // 39       '     Single quote
    LEXER_TOKEN_LPAR,            // 40       (     Left parenthesis
    LEXER_TOKEN_RPAR,            // 41       )     Right parenthesis
    LEXER_TOKEN_ASTERISK,            // 42       *     Asterisk
    LEXER_TOKEN_PLUS,            // 43       +     Plus
    LEXER_TOKEN_COMMA,            // 44       ,     Comma
    LEXER_TOKEN_MINUS,            // 45       -     Minus
    LEXER_TOKEN_DOT,            // 46       .     Period
    LEXER_TOKEN_SLASH,            // 47       /     Slash
    0,              // 48       0     Zero
    0,              // 49       1     One
    0,              // 50       2     Two
    0,              // 51       3     Three
    0,              // 52       4     Four
    0,              // 53       5     Five
    0,              // 54       6     Six
    0,              // 55       7     Seven
    0,              // 56       8     Eight
    0,              // 57       9     Nine
    LEXER_TOKEN_COLON,            // 58       :     Colon
    LEXER_TOKEN_SEMICOLON,            // 59       ;     Semicolon
    LEXER_TOKEN_LT,            // 60       <     Less than
    LEXER_TOKEN_EQ,            // 61       =     Equality sign
    LEXER_TOKEN_GT,            // 62       >     Greater than
    LEXER_TOKEN_QMARK,            // 63       ?     Question mark
    LEXER_TOKEN_AT,            // 64       @     At sign
    0,       // 65       A     Capital A
    0,       // 66       B     Capital B
    0,       // 67       C     Capital C
    0,       // 68       D     Capital D
    0,       // 69       E     Capital E
    0,       // 70       F     Capital F
    0,       // 71       G     Capital G
    0,       // 72       H     Capital H
    0,       // 73       I     Capital I
    0,       // 74       J     Capital J
    0,       // 75       K     Capital K
    0,       // 76       L     Capital L
    0,       // 77       M     Capital M
    0,       // 78       N     Capital N
    0,       // 79       O     Capital O
    0,       // 80       P     Capital P
    0,       // 81       Q     Capital Q
    0,       // 82       R     Capital R
    0,       // 83       S     Capital S
    0,       // 84       T     Capital T
    0,       // 85       U     Capital U
    0,       // 86       V     Capital V
    0,       // 87       W     Capital W
    0,       // 88       X     Capital X
    0,       // 89       Y     Capital Y
    0,       // 90       Z     Capital Z
    LEXER_TOKEN_LBRAK,            // 91       [     Left square bracket
    LEXER_TOKEN_BSLASH,            // 92       \     Backslash
    LEXER_TOKEN_RBRACK,            // 93       ]     Right square bracket
    LEXER_TOKEN_CARET,            // 94       ^     Caret / circumflex
    LEXER_TOKEN_UNDERSCORE,            // 95       _     Underscore
    LEXER_TOKEN_ACCENT,            // 96       `     Grave / accent
    0,       // 97       a     Small a
    0,       // 98       b     Small b
    0,       // 99       c     Small c
    0,       // 100      d     Small d
    0,       // 101      e     Small e
    0,       // 102      f     Small f
    0,       // 103      g     Small g
    0,       // 104      h     Small h
    0,       // 105      i     Small i
    0,       // 106      j     Small j
    0,       // 107      k     Small k
    0,       // 108      l     Small l
    0,       // 109      m     Small m
    0,       // 110      n     Small n
    0,       // 111      o     Small o
    0,       // 112      p     Small p
    0,       // 113      q     Small q
    0,       // 114      r     Small r
    0,       // 115      s     Small s
    0,       // 116      t     Small t
    0,       // 117      u     Small u
    0,       // 118      v     Small v
    0,       // 119      w     Small w
    0,       // 120      x     Small x
    0,       // 121      y     Small y
    0,       // 122      z     Small z
    LEXER_TOKEN_LBRACE,            // 123      {     Left curly bracket
    LEXER_TOKEN_BAR,            // 124      |     Vertical bar
    LEXER_TOKEN_RBRACE,            // 125      }     Right curly bracket
    LEXER_TOKEN_TILDE,            // 126      ~     Tilde
    0,              // 127      DEL     Delete
};


// lexer instance state
typedef struct _lexer_state
{
    // converter from server enc to ASCII
    void (*enc_conv) (const_char_info *src, char_info *dst);

    // last read lexem
    lexer_lexem lexem;

    // if num_mode = 1 parse only integers, otherwise decimal
    uint8 num_mode;

    // last read char
    lexer_char ch;

    // identifier as ASCII uppercase letters with '\0' at the end
    achar ucase_identifier[LEXER_MAX_IDENTIFIER_LEN + sizeof(achar)];

    achar errmes[LEXER_ERRMES_BUF_SZ];

    lexer_interface li;
} lexer_state;



// send parsing error to client
sint8 lexer_report_error(lexer_state *ls, const achar *msg, ...)
{
    va_list args;
    va_start(args, msg);

    vsnprintf(ls->errmes, LEXER_ERRMES_BUF_SZ, msg, args);
    ls->errmes[LEXER_ERRMES_BUF_SZ-1] = _ach('\0');

    va_end(args);

    if(ls->li.report_error(ERROR_SYNTAX_ERROR, ls->errmes) != 0) return -1;

    return 0;
}


// read the next character from the stream
// return 0 on sucess, -1 on error
sint8 lexer_next_ch(lexer_state *ls)
{
    sint8 eos;

    // read char
    if(ls->li.next_char(&ls->ch.chi, &eos) != 0) return -1;

    // determine type
    ls->enc_conv((const_char_info *)&ls->ch.chi, &ls->ch.ach);
    if(ls->ch.ach.state == CHAR_STATE_COMPLETE)
    {
        ls->ch.type = g_lexer_char_type[ls->ch.ach.chr[0]];

        if(ls->ch.ach.chr[0] == _ach('\n'))
        {
            ls->lexem.line += 1;
            ls->lexem.col = 1;
        }
        else
        {
            ls->lexem.col += 1;
        }
    }
    else if(eos)
    {
        ls->ch.type = LEXER_CHAR_TYPE_EOS;
    }
    else
    {
        ls->ch.type = LEXER_CHAR_TYPE_OTHER;
        ls->lexem.col += 1;
    }

    return 0;
}


// return size of the buffer for lexer initialization
size_t lexer_get_allocation_size()
{
    return sizeof(lexer_state);
}



// create and return lexical tokenizer instance
// returns NULL on error
handle lexer_create(void *buf, encoding enc, handle str_literal, lexer_interface li)
{
    lexer_state *ls = (lexer_state *)buf;

    if(NULL == ls) return NULL;

    ls->enc_conv = encoding_get_conversion_fun(enc, ENCODING_ASCII);
    ls->lexem.str_literal = str_literal;
    ls->li = li;

    assert(NULL != ls->enc_conv);

    ls->ch.chi.chr = ls->ch.ch_buf[0];
    ls->ch.ach.chr = ls->ch.ch_buf[1];
    ls->ch.type = LEXER_CHAR_TYPE_UNDEFINED;
    memset(&ls->lexem, 0, sizeof(ls->lexem));
    ls->lexem.line = 1;
    ls->lexem.col = 0;
    ls->num_mode = 0;

    if(lexer_next_ch(ls) != 0) return NULL;

    return (handle)ls;
}


// determine if identifier is a reserved word
void lexer_match_rword(lexer_state *ls)
{
    if(ls->ucase_identifier[0] == _ach('\0')) return;

    achar c = ls->ucase_identifier[0];
    int start_diap = g_lexer_reserved_word_diap[c - 64];
    int end_diap = g_lexer_reserved_word_diap[c - 63];

    for(int i=start_diap; i<end_diap; i++)
    {
        if(!strncmp(ls->ucase_identifier, g_lexer_reserved_words[i], LEXER_MAX_IDENTIFIER_LEN))
        {
            ls->lexem.type = LEXEM_TYPE_RESERVED_WORD;
            ls->lexem.reserved_word = i;
        }
    }
}


sint8 lexer_next_identifier(lexer_state *ls)
{
    sint32 ucase_identifier_len = -1;

    memset(ls->ucase_identifier, 0, sizeof(ls->ucase_identifier));

    if(ls->ch.type == LEXER_CHAR_TYPE_LCASE_LETTER ||
            ls->ch.type == LEXER_CHAR_TYPE_UCASE_LETTER)
    {
        // can be reserved word, start recording
        ucase_identifier_len = 0;
    }

    do
    {
        if(ls->lexem.identifier_len + ls->ch.chi.length > LEXER_MAX_IDENTIFIER_LEN)
        {
            if(lexer_report_error(ls, _ach("identifier is too long at line %d, column %d"), ls->lexem.line, ls->lexem.col) != 0) return -1;
            return 1;
        }
        else
        {
            memcpy(ls->lexem.identifier + ls->lexem.identifier_len, ls->ch.chi.chr, ls->ch.chi.length);
            ls->lexem.identifier_len += ls->ch.chi.length;
        }

        if(ucase_identifier_len >= 0)
        {
            if(ls->ch.type == LEXER_CHAR_TYPE_UCASE_LETTER)
            {
                ls->ucase_identifier[ucase_identifier_len] = ls->ch.ach.chr[0];
            }
            else if(ls->ch.type == LEXER_CHAR_TYPE_LCASE_LETTER)
            {
                ls->ucase_identifier[ucase_identifier_len] = ls->ch.ach.chr[0] - 32;
            }
            else
            {
                ucase_identifier_len = -2;
            }
            ucase_identifier_len ++;
        }

        if(lexer_next_ch(ls) != 0) return -1;
    }
    while(ls->ch.type == LEXER_CHAR_TYPE_LCASE_LETTER ||
            ls->ch.type == LEXER_CHAR_TYPE_UCASE_LETTER ||
            ls->ch.type == LEXER_CHAR_TYPE_DIGIT ||
            ls->ch.type == LEXER_CHAR_TYPE_OTHER ||
            (ls->ch.type == LEXER_CHAR_TYPE_SPECIAL && ls->ch.ach.chr[0] == _ach('_')));

    return 0;
}


sint8 lexer_next_num_literal(lexer_state *ls)
{
    sint32 pos = 0, f = 0;
    sint8 d, e = 0;

    ls->lexem.integer = 0L;
    ls->lexem.num_literal.sign = DECIMAL_SIGN_POS;
    do
    {
        d = ls->ch.ach.chr[0] - 48;

        ls->lexem.num_literal.m[pos] *= 10;
        ls->lexem.num_literal.m[pos] += d;
        ls->lexem.num_literal.n++;
        f++;
        if(f >= DECIMAL_BASE_LOG10)
        {
            pos++;
            ls->lexem.num_literal.m[pos] = ls->lexem.num_literal.m[pos - 1];
            ls->lexem.num_literal.m[pos - 1] = 0;
            f = 0;
        }
        if(pos >= DECIMAL_PARTS)
        {
            // overflow
            if(lexer_report_error(ls, _ach("numeric mantissa overflow at line %d, column %d"), ls->lexem.line, ls->lexem.col) != 0) return -1;
            return 1;
        }

        if(ls->num_mode == 1)
        {
            ls->lexem.integer *= 10;
            ls->lexem.integer += d;

            if(ls->lexem.integer < 0)
            {
                // overflow
                if(lexer_report_error(ls, _ach("integer is too big at line %d, column %d"), ls->lexem.line, ls->lexem.col) != 0) return -1;
                return 1;
            }
        }

        if(lexer_next_ch(ls) != 0) return -1;
    }
    while(ls->ch.type == LEXER_CHAR_TYPE_DIGIT);

    if(ls->num_mode == 1)
    {
        return 0;
    }

    if(ls->ch.type == LEXER_CHAR_TYPE_SPECIAL && ls->ch.ach.chr[0] == _ach('.'))
    {
        if(lexer_next_ch(ls) != 0) return -1;
        while(ls->ch.type == LEXER_CHAR_TYPE_DIGIT)
        {
            e++;
            d = ls->ch.ach.chr[0] - 48;

            ls->lexem.num_literal.m[pos] *= 10;
            ls->lexem.num_literal.m[pos] += d;
            ls->lexem.num_literal.n++;
            f++;
            if(f >= DECIMAL_BASE_LOG10)
            {
                pos++;
                ls->lexem.num_literal.m[pos] = ls->lexem.num_literal.m[pos - 1];
                ls->lexem.num_literal.m[pos - 1] = 0;
                f = 0;
            }
            if(pos >= DECIMAL_PARTS)
            {
                // overflow
                if(lexer_report_error(ls, _ach("numeric mantissa overflow at line %d, column %d"), ls->lexem.line, ls->lexem.col) != 0) return -1;
                return 1;
            }

            if(lexer_next_ch(ls) != 0) return -1;
        }
    }

    if((ls->ch.type == LEXER_CHAR_TYPE_LCASE_LETTER && ls->ch.ach.chr[0] == _ach('e')) ||
            (ls->ch.type == LEXER_CHAR_TYPE_UCASE_LETTER && ls->ch.ach.chr[0] == _ach('E')))
    {
        if(lexer_next_ch(ls) != 0) return -1;

        if(ls->ch.type == LEXER_CHAR_TYPE_SPECIAL && ls->ch.ach.chr[0] == _ach('+'))
        {
            f = 1;
        }
        else if(ls->ch.type == LEXER_CHAR_TYPE_SPECIAL && ls->ch.ach.chr[0] == _ach('-'))
        {
            f = -1;
        }
        else
        {
            if(lexer_report_error(ls, _ach("+ or - is expected at line %d, column %d"), ls->lexem.line, ls->lexem.col) != 0) return -1;
            return 1;
        }

        if(lexer_next_ch(ls) != 0) return -1;

        d = 0;
        pos = 0;
        while(ls->ch.type == LEXER_CHAR_TYPE_DIGIT)
        {
            d *= 10;
            d += ls->ch.ach.chr[0] - 48;
            if(pos >= 3)
            {
                if(lexer_report_error(ls, _ach("numeric exponent overflow at line %d, column %d"), ls->lexem.line, ls->lexem.col) != 0) return -1;
                return 1;
            }
        }

        if(d < -128 + e)
        {
            if(lexer_report_error(ls, _ach("numeric exponent overflow at line %d, column %d"), ls->lexem.line, ls->lexem.col) != 0) return -1;
            return 1;
        }

        ls->lexem.num_literal.e = d - e;
    }

    return 0;
}


sint8 lexer_next_str_literal(lexer_state *ls)
{
    do
    {
        if(string_literal_append_char(ls->lexem.str_literal, ls->ch.chi.chr, ls->ch.chi.length) != 0)
        {
            if(lexer_report_error(ls, _ach("string literal is too long at line %d, column %d"), ls->lexem.line, ls->lexem.col) != 0) return -1;
            return 1;
        }

        if(lexer_next_ch(ls) != 0) return -1;
        if(ls->ch.type == LEXEM_TYPE_TOKEN && ls->ch.ach.chr[0] == _ach('\''))
        {
            if(lexer_next_ch(ls) != 0) return -1;
            if(!(ls->ch.type == LEXEM_TYPE_TOKEN && ls->ch.ach.chr[0] == _ach('\'')))
            {
                return 0;
            }
        }
    }
    while(1);

    return 0;
}


// read and return next token
// return 0 on success, 1 on syntax error, -1 on error
sint8 lexer_next(handle lexer, lexer_lexem *lexem)
{
    lexer_state *ls = (lexer_state *)lexer;
    uint8 ch;
    sint8 res;

    memset(&ls->lexem, 0, sizeof(ls->lexem));

    if(ls->ch.type == LEXER_CHAR_TYPE_LCASE_LETTER || ls->ch.type == LEXER_CHAR_TYPE_UCASE_LETTER)
    {
        ls->lexem.type = LEXEM_TYPE_IDENTIFIER;
        if((res = lexer_next_identifier(ls)) != 0) return res;

        // check for reserved word
        lexer_match_rword(ls);

        return 0;
    }
    else if(ls->ch.type == LEXER_CHAR_TYPE_OTHER)
    {
        ls->lexem.type = LEXEM_TYPE_IDENTIFIER;
        if((res = lexer_next_identifier(ls)) != 0) return res;
    }
    else if(ls->ch.type == LEXER_CHAR_TYPE_SPECIAL)
    {
        ch = ls->ch.ach.chr[0];
        if(ch == _ach('_'))
        {
            ls->lexem.type = LEXEM_TYPE_IDENTIFIER;
            return lexer_next_identifier(ls);
        }
        else if(ch == _ach('\''))
        {
            ls->lexem.type = LEXEM_TYPE_STR_LITERAL;
            return lexer_next_str_literal(ls);
        }
        else
        {
            ls->lexem.type = LEXEM_TYPE_TOKEN;
            ls->lexem.token = g_lexer_ascii_to_token[ch];
            return 0;
        }
    }
    else if(ls->ch.type == LEXER_CHAR_TYPE_DIGIT)
    {
        ls->lexem.type = LEXEM_TYPE_NUM_LITERAL;
        return lexer_next_num_literal(ls);
    }
    else if(ls->ch.type == LEXER_CHAR_TYPE_EOS)
    {
        ls->lexem.type = LEXEM_TYPE_EOS;
        return 0;
    }
    else
    {
        return -1;
    }

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


// if set = 1 make lexer parse only integers, otherwise full decimal is parsed (default)
void lexer_num_mode_integer(handle lexer, uint8 set)
{
    lexer_state *ls = (lexer_state *)lexer;
    ls->num_mode = set;
}

