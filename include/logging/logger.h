#ifndef _LOGGER_H
#define _LOGGER_H

// Logging

// create logger, returns 0 on success, non 0 otherwise
uint8 logger_create();

// write debug log message
void logger_debug(wchar *fmt, ...);

// write informational log message
void logger_info(wchar *fmt, ...);

// write warning log message
void logger_warn(wchar *fmt, ...);

// write error log message
void logger_error(wchar *fmt, ...);

#endif
