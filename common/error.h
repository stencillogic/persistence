#ifndef _ERROR_H
#define _ERROR_H

// Error messages and codes

// return error code of last operation
uint32 error_code();

// return null-terminalted error message of last operation
const wchar* error_msg();

// set error code (and message)
void error_set(uint32 code);

#endif
