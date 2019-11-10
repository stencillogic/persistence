#ifndef _ENCODING_H
#define _ENCODING_H


// work with encoding: character operations, validation, conversion, etc.


#include "defs/defs.h"


#define ENCODING_MAXCHAR_LEN    (4)     // longest possible character in all supported encodings
#define ENCODING_NUM            (3)     // number of encodings in encoding enum


typedef enum _encoding
{
    ENCODING_UNKNOWN = 0,
    ENCODING_ASCII = 1,
    ENCODING_UTF8 = 2
} encoding;

// aux struct for holding state while reading characters from byte stream
typedef enum _encoding_char_state
{
    CHAR_STATE_INCOMPLETE = 0,
    CHAR_STATE_COMPLETE = 1,
    CHAR_STATE_INVALID = -1
} encoding_char_state;

typedef struct _char_info
{
    uint8 *chr;
    uint8 length;
    uint8 ptr;
    encoding_char_state state;
} char_info;

typedef struct _const_char_info
{
    const uint8 *chr;
    uint8 length;
    uint8 ptr;
    encoding_char_state state;
} const_char_info;


// initialize encodings
void encoding_init();


// calculates character length for chr
typedef void (*encoding_char_len_fun)(const_char_info *chr);

// get character length function
encoding_char_len_fun encoding_get_char_len_fun(encoding enc);


// convert character from encoding to encoding
// if src or dst is invalid behaviour is undetermined
// if src can't be converted to dst, then dst->state is set to CHAR_STATE_INVALID and the question mark is placed as a dst character
// if character converted successfully then dst->State is CHAR_STATE_COMPLETE
typedef void (*encoding_conversion_fun)(const_char_info *src, char_info *dst);

// get conversion function
encoding_conversion_fun encoding_get_conversion_fun(encoding from, encoding to);



// build character chr from stream of byte
// if chr pointer is corrupt or invalid behaviour is undetermined
typedef void (*encoding_build_char_fun)(char_info *chr, uint8 byte);

// get encoding_build_char function for certain encoding
encoding_build_char_fun encoding_get_build_char_fun(encoding enc);



// return 1 if chr is '\0', otherwise return 0
// if chr is invalid behaviour is undetermined
sint8 encoding_is_string_terminator(const_char_info *chr, encoding enc);

// return string representation of encoding name
const achar* encoding_name(encoding enc);

// return encoding by it's name, 0 if name is not recognized
encoding encoding_idbyname(const achar* name);

// return 1 if encodings are mutually convertable, 0 otherwise
sint8 encoding_is_convertable(encoding enc1, encoding enc2);


#endif
