#include "common/error.h"

#define ERROR_CODE_NUM 6

achar *g_error_msg[] =
{
    _ach("no error"),
    _ach("there is no such configuration option"),
    _ach("the configuration option has different data type"),
    _ach("failed to load configuration file"),
    _ach("failed to initiate logging"),
    _ach("failed to write tol log")
};

error_code g_current_error_code = 0;

// return error code of last operation
error_code error_get()
{
    return g_current_error_code;
}

// return null-terminalted error message of last operation
const achar* error_msg()
{
    return g_error_msg[g_current_error_code];
}

// set error code (and message)
void error_set(error_code code)
{
    g_current_error_code = (code < 0 || code >= ERROR_CODE_NUM) ? 0 : code;
}
