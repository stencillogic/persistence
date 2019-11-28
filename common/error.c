#include "common/error.h"

#define ERROR_CODE_NUM 9

achar *g_error_msg[] =
{
    _ach("ECODE=00001: no error"),
    _ach("ECODE=00002: session initialization failed"),
    _ach("ECODE=00003: protocol violation"),
    _ach("ECODE=00004: numeric overflow"),
    _ach("ECODE=00005: division by zero"),
    _ach("ECODE=00006: invalid decimal format"),
    _ach("ECODE=00007: syntax error"),
    _ach("ECODE=00008: datatype mismatch"),
    _ach("ECODE=00009: out of memory"),
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

