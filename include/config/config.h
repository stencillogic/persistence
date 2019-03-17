#ifndef _CONFIG_H
#define _CONFIG_H

// Configuration

#include "defs/defs.h"

// Config BNF:
//   <config> ::= <config_line> { "\n" <config_line> }
//   <config_line> ::= <comment_line> | <option_line>
//   <comment_line> ::= "#" { <any_char> }
//   <any_text> ::= any character except "\n" and "\0"
//   <option_line> ::= <option_name> { <space> } "=" { <space> } <option_value> | "\n"
//   <space> ::= "\s" | "\t"
//   <option_name> ::= <name_symbol> { <name_symbol> }
//   <name_symbol> ::= a..z | A..Z | 0..9 | "." | "_" | "-"
//   <option_value> ::= <value_symbol> { <value_symbol> }
//   <value_symbol> ::= any character ecxept "\n" and "\0"

#define CONFIG_FILE_MAX_LINE_LEN 4000
#define CONFIG_MAX_OPTION_NAME_LEN 100

typedef enum _config_option
{
    CONFIG_LOGGING_MODE = 0,
    CONFIG_LOG_DIR = 1,
    CONFIG_LOG_FILE_SIZE_THRESHOLD = 2,
} config_option;

// searches for configuration file and loads config
// returns 0 on success, non 0 on error
sint8 config_create();

// loads integer value from config instance
// returns 0 on success, non 0 on error
sint8 config_get_int(config_option option, sint64 *value);

// loads floating point value from config instance
// returns 0 on success, non 0 on error
sint8 config_get_float(config_option option, float64 *value);

// return null terminated string value or NULL on error
const achar* config_get_str(config_option option);

#endif
