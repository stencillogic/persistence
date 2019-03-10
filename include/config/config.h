#ifndef _CONFIG_H
#define _CONFIG_H

// Configuration

#include "defs/defs.h"

typedef enum _config_option
{
    logging_mode = 0
} config_option;

// searches for configuration file and loads config
// returns 0 on success, non 0 on error
uint8 config_create();

// loads integer value from config instance
// returns 0 on success, non 0 on error
uint8 config_get_int(config_option option, sint64 *value);

// loads floating point value from config instance
// returns 0 on success, non 0 on error
uint8 config_get_float(config_option option, sint64 *value);

// return null terminated string value or NULL on error
const wchar* config_get_str(config_option option);

#endif
