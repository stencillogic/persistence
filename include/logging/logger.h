#ifndef _LOGGER_H
#define _LOGGER_H

// Logging

#include "defs/defs.h"

typedef enum _logging_level
{
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO = 1,
    LOG_LEVEL_WARN = 2,
    LOG_LEVEL_ERROR = 3
} logging_level;

// create logger with certain log file name, returns 0 on success, non 0 otherwise
sint8 logger_create(const achar *log_file_name);

// close logger
sint8 logger_close();

// return log level
logging_level logger_log_level();

// write debug log message
void logger_debug(achar *fmt, ...);

// write informational log message
void logger_info(achar *fmt, ...);

// write warning log message
void logger_warn(achar *fmt, ...);

// write error log message
void logger_error(achar *fmt, ...);

#endif
