#include "logging/logger.h"
#include "config/config.h"
#include "common/error.h"
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>

#define MAX_LOG_MSG_LEN 4096

logging_level g_logger_log_level = LOG_LEVEL_DEBUG;
uint32 g_log_file_name_base = 0;
achar g_log_file_path[CONFIG_FILE_MAX_LINE_LEN + 258];
int g_current_log = -1;
achar g_log_msg_buf[MAX_LOG_MSG_LEN];
size_t g_log_file_size;
size_t g_log_file_size_threshold;

size_t logger_prepare_msg(logging_level level, const achar* fmt, va_list ap)
{
    size_t bufsz = MAX_LOG_MSG_LEN*sizeof(achar);
    time_t t;
    struct tm *ptm;
    size_t dttm_shift = 0;
    int res_len;
    if((time(&t) != -1) && ((ptm = localtime(&t)) != NULL))
    {
        dttm_shift = strftime(g_log_msg_buf, bufsz, _ach("%Y-%m-%d %H:%M:%S"), ptm);
    }

    const achar* lvl_str = _ach(", unkn,  ");
    switch(level)
    {
        case LOG_LEVEL_ERROR: lvl_str = _ach(", error, "); break;
        case LOG_LEVEL_WARN:  lvl_str = _ach(", warn,  "); break;
        case LOG_LEVEL_INFO:  lvl_str = _ach(", info,  "); break;
        case LOG_LEVEL_DEBUG: lvl_str = _ach(", debug, "); break;
    }
    memcpy(g_log_msg_buf + dttm_shift, lvl_str, 9*sizeof(achar));
    dttm_shift += 9;

    res_len = vsnprintf(g_log_msg_buf + dttm_shift, bufsz - dttm_shift, fmt, ap);

    if(res_len + dttm_shift == MAX_LOG_MSG_LEN - 1)
    {
        if(_ach('\n') != g_log_msg_buf[MAX_LOG_MSG_LEN - 2])
        {
            memcpy(g_log_msg_buf + MAX_LOG_MSG_LEN - 6, _ach("...\n"), 4*sizeof(achar));
        }
    }
    else
    {
        if(res_len < 0) res_len = 0;
        if(_ach('\n') != g_log_msg_buf[dttm_shift + res_len - 1])
        {
            g_log_msg_buf[dttm_shift + res_len] = _ach('\n');
            g_log_msg_buf[dttm_shift + res_len + 1] = _ach('\0');
            res_len += 1;
        }
    }

    return (res_len + dttm_shift)*sizeof(achar);
}

void logger_print_to_stream(FILE *fp, logging_level level, const achar *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    logger_prepare_msg(level, fmt, args);
    fputs(g_log_msg_buf, fp);
    va_end(args);
}

// close logger
sint8 logger_close()
{
    if(g_current_log != -1)
    {
        logger_print_to_stream(stdout, LOG_LEVEL_INFO, _ach("Closing current log file"));
        if(close(g_current_log) == -1)
        {
            logger_print_to_stream(stderr, LOG_LEVEL_ERROR, _ach("Failed to close current log file: %s"), strerror(errno));
            return 1;
        }
    }
    g_log_file_size = 0;
    return 0;
}

// close current log file (if opened) and open new one
sint8 reopen_log_file()
{
    if(logger_close())
    {
        return 1;
    }

    time_t t;
    struct tm *ptm;
    achar formatted_tm[100];
    size_t formatted_len = 0;
    if((time(&t) == -1) || ((ptm = localtime(&t)) == NULL))
    {
        logger_print_to_stream(stderr, LOG_LEVEL_ERROR, _ach("Failed to get current time"));
    }
    else
    {
        formatted_len = strftime(formatted_tm, 100, ".%Y%m%d-%H%M%S", ptm);
    }

    if(formatted_len == 0)
    {
        formatted_len = snprintf(formatted_tm, 100*sizeof(achar), _ach("%d"), rand());
    }

    size_t sz = 0;
    if(formatted_len > 0)
    {
        sz = formatted_len*sizeof(achar);
        memcpy(g_log_file_path + g_log_file_name_base, formatted_tm, sz);
    }
    g_log_file_path[g_log_file_name_base + sz] = _ach('\0');

    logger_print_to_stream(stdout, LOG_LEVEL_INFO, _ach("Opening new log file: %s"), g_log_file_path);

    g_current_log = open(g_log_file_path, O_APPEND|O_CREAT|O_WRONLY|O_CLOEXEC);
    if(-1 == g_current_log)
    {
        logger_print_to_stream(stderr, LOG_LEVEL_ERROR, _ach("Failed to open a new log file"));
        return 1;
    }
    return 0;
}

// create logger, returns 0 on success, non 0 otherwise
sint8 logger_create(const achar *log_file_name)
{
    const achar* mode = config_get_str(CONFIG_LOGGING_MODE);
    assert(mode != NULL);
    if(0 == strcmp(mode, _ach("debug"))) g_logger_log_level = LOG_LEVEL_DEBUG;
    if(0 == strcmp(mode, _ach("info"))) g_logger_log_level = LOG_LEVEL_INFO;
    if(0 == strcmp(mode, _ach("warn"))) g_logger_log_level = LOG_LEVEL_WARN;
    if(0 == strcmp(mode, _ach("error"))) g_logger_log_level = LOG_LEVEL_ERROR;

    assert(config_get_int(CONFIG_LOG_FILE_SIZE_THRESHOLD, (sint64*)&g_log_file_size_threshold) == 0);

    const achar *log_dir = config_get_str(CONFIG_LOG_DIR);

    struct stat sb;
    if (!(stat(log_dir, &sb) == 0 && S_ISDIR(sb.st_mode)))
    {
        logger_print_to_stream(stderr, LOG_LEVEL_ERROR, _ach("Log directory: %s"), strerror(errno));
        return 1;
    }

    size_t log_dir_len = strlen(log_dir)*sizeof(achar);
    size_t log_file_name_len = strlen(log_file_name)*sizeof(achar);

    assert(log_file_name_len <= 150*sizeof(achar));

    if(log_dir_len > 0)
    {
        memcpy(g_log_file_path, log_dir, log_dir_len);
        if(log_dir[log_dir_len - 1] != _ach('/'))
        {
            g_log_file_path[log_dir_len] = _ach('/');
            log_dir_len += 1;
        }
    }
    memcpy(g_log_file_path + log_dir_len, log_file_name, log_file_name_len);
    g_log_file_name_base = log_dir_len + log_file_name_len;

    return reopen_log_file();
}

logging_level logger_log_level()
{
    return g_logger_log_level;
}

void logger_write(logging_level level, const achar* fmt, va_list ap)
{
    if(g_logger_log_level <= level)
    {
        size_t written, total_written = 0, left;
        int fd;

        left = logger_prepare_msg(level, fmt, ap);

        fd = (-1 == g_current_log) ? ( level == LOG_LEVEL_ERROR ? STDERR_FILENO : STDOUT_FILENO ) : g_current_log;

        while(left > total_written && (written = write(fd, g_log_msg_buf, left - total_written)) > 0) total_written += written;
        if(written <= 0)
        {
            logger_print_to_stream(stderr, LOG_LEVEL_ERROR, _ach("Writing to log file: %s"), strerror(errno));
        }

        g_log_file_size += total_written;
        if(-1 != g_current_log && g_log_file_size >= g_log_file_size_threshold)
        {
            if(reopen_log_file()) {
                g_log_file_size = 0;  // if failed to open new log reset written size for current
            }
        }
    }
}

// write debug log message
void logger_debug(achar *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    logger_write(LOG_LEVEL_DEBUG, fmt, args);
    va_end(args);
}

// write informational log message
void logger_info(achar *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    logger_write(LOG_LEVEL_INFO, fmt, args);
    va_end(args);
}

// write warning log message
void logger_warn(achar *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    logger_write(LOG_LEVEL_WARN, fmt, args);
    va_end(args);
}

// write error log message
void logger_error(achar *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    logger_write(LOG_LEVEL_ERROR, fmt, args);
    va_end(args);
}

