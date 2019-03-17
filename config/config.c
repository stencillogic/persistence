#include "config/config.h"
#include "common/error.h"
#include "logging/logger.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>

#define CONFIG_ENTRIES_NUM 3

typedef enum _config_option_type
{
    CONFIG_TYPE_STRING,
    CONFIG_TYPE_INT,
    CONFIG_TYPE_FLOAT
} config_option_type;

typedef struct _config_entry
{
    config_option option;
    const achar* option_name;
    config_option_type type;
    achar *str_value;
    sint64 int_value;
    float64 float_value;
} config_entry;

config_entry g_config[CONFIG_ENTRIES_NUM] =
{
    {CONFIG_LOGGING_MODE, _ach("logging_mode"), CONFIG_TYPE_STRING, _ach("warn"), 0L, 0.0},
    {CONFIG_LOG_DIR, _ach("log_dir"), CONFIG_TYPE_STRING, _ach("/var/log/persistence"), 0L, 0.0},
    {CONFIG_LOG_FILE_SIZE_THRESHOLD, _ach("log_file_size_threshold"), CONFIG_TYPE_INT, _ach(""), 1024*1024*10, 0.0}
};

/////////////////////////////////////

config_entry *config_get_entry(config_option option)
{
    if(option >= 0 && option < CONFIG_ENTRIES_NUM)
    {
        return g_config + option;
    }
    return NULL;
}

config_option config_find_option_by_name(achar *option_name)
{
    for(sint32 i = 0; i < CONFIG_ENTRIES_NUM; i++)
    {
        if(0 == strcmp(option_name, g_config[i].option_name))
        {
            return g_config[i].option;
        }
    }
    return (config_option)-1;
}

sint8 config_set_entry_value(config_option option, achar *str_value, sint32 value_len)
{
    config_entry *entry = config_get_entry(option);
    if(entry != NULL)
    {
        switch(entry->type)
        {
            case CONFIG_TYPE_STRING:
                entry->str_value = (achar*)malloc((value_len + 1)*sizeof(achar));
                if(NULL == entry->str_value)
                {
                    logger_error(_ach("Memory allocation failed: %s\n"), strerror(errno));
                    return 1;
                }
                memcpy(entry->str_value, str_value, value_len*sizeof(achar));
                entry->str_value[value_len] = _ach('\0');
                break;
            case CONFIG_TYPE_INT:
                entry->int_value = atol(str_value);
                break;
            case CONFIG_TYPE_FLOAT:
                entry->float_value = atof(str_value);
                break;
        }
    }
    return 0;
}

sint8 config_parse_line(achar *line, uint32 line_num, sint32 *option_name_start, sint32 *option_name_len, sint32 *option_value_start, sint32 *option_value_len)
{
    sint32 p = 0;
//    while(_ach(' ') == line[p] || _ach('\t') == line[p]) p++;

    if(_ach('\n') == line[p] || _ach('\0') == line[p] || _ach('#') == line[p]) return 1;    // skip this line

    sint32 s = 0;
    achar ch = line[p+s];
    while((ch >= _ach('a') && ch <= _ach('z')) || (ch >= _ach('A') && ch <= _ach('Z')) || (ch >= _ach('0') && ch <= _ach('9'))
            || _ach('.') == ch || _ach('_') == ch || _ach('-') == ch)
    {
        if(s >= *option_name_len)
        {
            logger_error(_ach("Option name is too long, at line %u pos %d"), line_num, p);
            return -1;
        }
        s++;
        ch = line[p+s];
    }

    if(0 == s)
    {
        logger_error(_ach("Option name is not specified, at line %u pos %d"), line_num, p);
        return -1;
    }

    *option_name_start = p;
    *option_name_len = s;
    p = s;

    while(_ach(' ') == line[p] || _ach('\t') == line[p]) p++;

    if(_ach('=') != line[p])
    {
        logger_error(_ach("'=' is expected, at line %u pos %d"), line_num, p);
        return -1;
    }
    p++;

    while(_ach(' ') == line[p] || _ach('\t') == line[p]) p++;

    s = 0;
    ch = line[p];
    while(!(_ach('\n') == ch || _ach('\0') == ch))
    {
        if(s >= *option_value_len)
        {
            logger_error(_ach("Option value is too long, at line %u pos %d"), line_num, p);
            return -1;
        }
        s++;
        ch = line[p+s];
    }

    if(0 == s)
    {
        logger_error(_ach("Option value is not specified, at line %u pos %d\n"), line_num, p);
        return -1;
    }

    *option_value_start = p;
    *option_value_len = s;

    return 0;
}

sint8 check_config_valid()
{
    const config_entry* entry;

    entry = config_get_entry(CONFIG_LOG_FILE_SIZE_THRESHOLD);
    if(4096 > entry->int_value)
    {
        logger_error(_ach("Value for option %s must be greater than or equal to 4096\n"), entry->option_name);
        return 1;
    }

    entry = config_get_entry(CONFIG_LOGGING_MODE);
    if(!(0 == strcmp(entry->str_value, _ach("debug"))
             || 0 == strcmp(entry->str_value, _ach("info"))
             || 0 == strcmp(entry->str_value, _ach("warn"))
             || 0 == strcmp(entry->str_value, _ach("error"))))
    {
        logger_error(_ach("Value for option %s must be one of 'debug', 'info', 'warn', 'error'\n"), entry->option_name);
        return 1;
    }

    return 0;
}

sint8 config_create()
{
    config_option option;
    const achar *config_path;
    achar str_buf[(CONFIG_FILE_MAX_LINE_LEN+1)*sizeof(achar)];
    FILE *fp;
    uint32 line_num = 0;
    sint32 ons, onl, ovs, ovl;
    sint8 res;

    if((config_path = getenv(_ach("PERSISTENCE_CONFIG_PATH"))) == NULL)
    {
        if((fp = fopen(_ach("persistence.cfg"), _ach("rt"))) == NULL)
        {
            fp = fopen(_ach("/opt/persistence/persistence.cfg"), _ach("rt"));
            if(NULL != fp)
            {
                logger_info("Loading configuration from /opt/persistence/persistence.cfg");
            }
        }
        else
        {
            logger_info("Loading configuration from ./persistence.cfg");
        }
    }
    else
    {
        fp = fopen(config_path, _ach("rt"));
        if(NULL != fp)
        {
            logger_info(_ach("Loading configuration from %s"), config_path);
        }
    }

    if(!fp)
    {
        logger_error(_ach("Configuration file opening: %s"), strerror(errno));
        error_set(ERROR_FAILED_TO_LOAD_CONFIG);
        return 1;
    }

    while(fgets(str_buf, (CONFIG_FILE_MAX_LINE_LEN+1)*sizeof(achar), fp) != NULL)
    {
        line_num++;
        if(strlen(str_buf) >= CONFIG_FILE_MAX_LINE_LEN)
        {
            logger_error(_ach("Line length exceeds %d symbols, at line %u"), CONFIG_FILE_MAX_LINE_LEN, line_num);
            error_set(ERROR_FAILED_TO_LOAD_CONFIG);
            fclose(fp);
            return 1;
        }

        onl = CONFIG_MAX_OPTION_NAME_LEN;
        ovl = CONFIG_FILE_MAX_LINE_LEN - onl - 1;
        res = config_parse_line(str_buf, line_num, &ons, &onl, &ovs, &ovl);
        switch(res)
        {
            case -1:
                error_set(ERROR_FAILED_TO_LOAD_CONFIG);
                fclose(fp);
                return 1;
            case 0:
                str_buf[ons + onl] = _ach('\0');
                str_buf[ovs + ovl] = _ach('\0');
                option = config_find_option_by_name(str_buf + ons);
                if((sint32)option < 0)
                {
                    logger_warn(_ach("Unknown option: %s. Skipped"), str_buf + ons);
                }

                if(config_set_entry_value(option, str_buf+ovs, ovl) > 0)
                {
                    error_set(ERROR_FAILED_TO_LOAD_CONFIG);
                    fclose(fp);
                }

                break;
            case 1:
                break;
        }
    }
    int serrno = errno;
    if(0 == feof(fp))
    {
        logger_error(_ach("Reading configuration file: %s\n"), strerror(serrno));
        error_set(ERROR_FAILED_TO_LOAD_CONFIG);
        fclose(fp);
        return 1;
    }

    if(EOF == fclose(fp))
    {
        logger_warn(_ach("Failed to close configuration file after successfull loading"));
    }

    logger_debug(_ach("Checking configuration validity"));

    if(check_config_valid())
    {
        error_set(ERROR_FAILED_TO_LOAD_CONFIG);
        return 1;
    }

    logger_debug(_ach("Configuration is loaded and validated successfully"));
    return 0;
}

sint8 config_get_int(config_option option, sint64 *value)
{
    config_entry *entry;
    if((entry = config_get_entry(option)) != NULL)
    {
        if(entry->type == CONFIG_TYPE_INT)
        {
            *value = entry->int_value;
            return 0;
        }
        else
        {
            error_set(ERROR_CONFIG_OPTION_WRONG_TYPE);
            return 1;
        }
    }
    error_set(ERROR_NO_SUCH_CONFIG_OPTION);
    return 1;
}

sint8 config_get_float(config_option option, float64 *value)
{
    config_entry *entry;
    if((entry = config_get_entry(option)) != NULL)
    {
        if(entry->type == CONFIG_TYPE_FLOAT)
        {
            *value = entry->float_value;
            return 0;
        }
        else
        {
            error_set(ERROR_CONFIG_OPTION_WRONG_TYPE);
            return 1;
        }
    }
    error_set(ERROR_NO_SUCH_CONFIG_OPTION);
    return 1;
}

const achar* config_get_str(config_option option)
{
    config_entry *entry;
    if((entry = config_get_entry(option)) != NULL)
    {
        if(entry->type == CONFIG_TYPE_STRING)
        {
            return entry->str_value;
        }
        else
        {
            error_set(ERROR_CONFIG_OPTION_WRONG_TYPE);
            return NULL;
        }
    }
    error_set(ERROR_NO_SUCH_CONFIG_OPTION);
    return NULL;
}
