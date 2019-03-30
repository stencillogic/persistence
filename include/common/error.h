#ifndef _ERROR_H
#define _ERROR_H

// Error messages and codes for db clients

#include "defs/defs.h"

typedef enum _error_code
{
    ERROR_NO_ERROR = 0,
    ERROR_SESSION_INIT_FAIL = 1,
    ERROR_PROTOCOL_VIOLATION = 2,
} error_code;

// return error code of last operation
error_code error_get();

// return null-terminalted error message of last operation
const achar* error_msg();

// set error code (and message)
void error_set(error_code code);

#endif
