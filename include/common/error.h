#ifndef _ERROR_H
#define _ERROR_H

// Error messages and codes

#include "defs/defs.h"

typedef enum _error_code
{
    ERROR_NO_ERROR=0,
    ERROR_NO_SUCH_CONFIG_OPTION,
    ERROR_CONFIG_OPTION_WRONG_TYPE,
    ERROR_FAILED_TO_LOAD_CONFIG,
    ERROR_CREATE_LOG,
    ERROR_WRITING_TO_LOG
} error_code;

// return error code of last operation
error_code error_get();

// return null-terminalted error message of last operation
const achar* error_msg();

// set error code (and message)
void error_set(error_code code);

#endif
