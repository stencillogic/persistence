// persistence command line utility

#include "defs/defs.h"
#include "client/pproto_client.h"
#include "client/dbclient.h"
#include "client/printing.h"
#include "common/strop.h"
#include "common/error.h"
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>


#define CLIENT_ENCODING_ENVVAR          ("PSQLENC")
#define CLIENT_SQL_STATEMENT_DELIMITER  (';')
#define CLIENT_PROGRESS_CHAR            ('.')
#define CLIENT_PASSWORD_MAX_LEN         (1024)
#define CLIENT_MAX_VALUE_LEN            (256)
#define CLIENT_STRBUF_SZ                (CLIENT_MAX_VALUE_LEN * 32)

#define CLIENT_DEFAULT_DECIMAL_FMT      ("dsdddddde")
#define CLIENT_DEFAULT_INTEGER_FMT      ("dddddddddd")
#define CLIENT_DEFAULT_SMALLINT_FMT     ("ddddd")
#define CLIENT_DEFAULT_FLOAT_FMT        ("dsdddddde")
#define CLIENT_DEFAULT_DOUBLE_FMT       ("dsdddddddddddde")
#define CLIENT_DEFAULT_DATE_FMT         ("yyyy-mm-dd hh:mi:ss")
#define CLIENT_DEFAULT_TS_FMT           ("yyyy-mm-dd hh:mi:ss.tttttt")
#define CLIENT_DEFAULT_TS_WITH_TZ_FMT   ("yyyy-mm-dd hh:mi:ss.tttttt+tz")

#define CLIENT_DEFAULT_STR_LEN                  (32)
#define CLIENT_DEFAULT_DECIMAL_LEN              (14)
#define CLIENT_DEFAULT_INTEGER_LEN              (10)
#define CLIENT_DEFAULT_SMALLINT_LEN             (5)
#define CLIENT_DEFAULT_FLOAT_LEN                (14)
#define CLIENT_DEFAULT_DOUBLE_LEN               (21)
#define CLIENT_DEFAULT_DATE_LEN                 (19)
#define CLIENT_DEFAULT_TIMESTAMP_LEN            (26)
#define CLIENT_DEFAULT_TIMESTAMP_WITH_TZ_LEN    (32)


char       *user = 0;
char       *password = 0;
char       *host = "localhost";
int         port = SERVER_DEFAULT_PORT;
int         p_option_present = 0;
char        password_buf[CLIENT_PASSWORD_MAX_LEN];
uint8       stmt_delimiter = CLIENT_SQL_STATEMENT_DELIMITER;
const char *prompt = "ptool> ";
uint8       strbuf[CLIENT_STRBUF_SZ];
char        decimal_separator = '.';
encoding    client_enc;


typedef struct _char_state
{
    char *line;
    char *lineptr;
    char *lineend;
    size_t linelen;
} char_state;


void init()
{
    const char *encname = getenv(CLIENT_ENCODING_ENVVAR);
    if(NULL != encname)
    {
        client_enc = encoding_idbyname(encname);
        if(client_enc == ENCODING_UNKNOWN)
        {
            printf("Value \"%s\" of environment variable %s is not recognized encoding name, ASCII will be used as default encoding\n", encname, CLIENT_ENCODING_ENVVAR);
            client_enc = ENCODING_ASCII;
        }
    }
    else
    {
        printf("%s environment variable is not set, ASCII will be used as default encoding\n", CLIENT_ENCODING_ENVVAR);
        client_enc = ENCODING_ASCII;
    }
    encoding_init();
    strop_set_encoding(client_enc);
}

void usage(FILE* fp)
{
    fputs("Usage: psql [-h | --help] [-u <user>] [-p [<password>]] [-H --host <host>] [-P --port port>]\n", fp);
}

void show_help()
{
    usage(stdout);
    puts("");
    puts("Options: ");
    puts("  -h --help      show this help");
    puts("  -u --user      user name");
    puts("  -p --password  ask for password or use option value as a password if specified");
    puts("  -s --server    server host and port");
}

void parse_args(int argc, char **argv)
{
    int c;

    while(1)
    {
        int option_index = 0;
        static struct option long_options[] =
        {
            {"help",    no_argument,        0,  'h'},
            {"user",    required_argument,  0,  'u'},
            {"password",optional_argument,  0,  'p'},
            {"host",    required_argument,  0,  'H'},
            {"port",    required_argument,  0,  'P'},
            {0,         0,                  0,  0}
        };

        c = getopt_long(argc, argv, "hu:p::s:H:P:",
                 long_options, &option_index);
        if(c == -1)
        {
            break;
        }

        switch(c)
        {
            case 'h':
                show_help();
                exit(0);
                break;
            case 'u':
                if(0 == optarg)
                {
                    usage(stderr);
                    exit(1);
                }
                user = optarg;
                break;
            case 'p':
                p_option_present = 1;
                password = optarg;
                break;
            case 'H':
                if(0 == optarg)
                {
                    usage(stderr);
                    exit(1);
                }
                host = optarg;
                break;
            case 'P':
                if(0 == optarg)
                {
                    usage(stderr);
                    exit(1);
                }
                port = atoi(optarg);
                if(port <=0 || port >= 65536)
                {
                    fputs("Invalid port number specified\n", stderr);
                    exit(1);
                }
                break;
            case '?':
            default:
                usage(stderr);
                exit(1);
                break;
        }
    }

    if(optind < argc)
    {
        usage(stderr);
        exit(1);
    }
}

// read next char from stdin
char next_char(char_state *chst)
{
    while(chst->lineptr >= chst->lineend)
    {
        fputs(prompt, stdout);
        if(getline(&chst->line, &chst->linelen, stdin) == -1)
        {
            perror("Failed to read input");
        }
        else
        {
            chst->lineptr = chst->line;
            chst->lineend = chst->line + chst->linelen;
        }
    }

    return *(chst->lineptr++);
}

// calculate output column width
uint32 calc_col_width(const pproto_col_desc *cd)
{
    uint32 w = cd->col_alias_sz;
    switch(cd->data_type)
    {
        case CHARACTER_VARYING:
            w = cd->data_type_len;
            if(w > CLIENT_DEFAULT_STR_LEN) w = CLIENT_DEFAULT_STR_LEN;
            if(w < cd->col_alias_sz) w = cd->col_alias_sz;
            break;
        case DECIMAL:
            if(cd->data_type_precision > 0)
            {
                if(w < cd->data_type_precision + sizeof(decimal_separator))
                {
                    w = cd->data_type_precision + sizeof(decimal_separator);
                }
            }
            else
            {
                w = CLIENT_DEFAULT_DECIMAL_LEN;
            }
            break;
        case INTEGER:
            if(w < CLIENT_DEFAULT_INTEGER_LEN) w = CLIENT_DEFAULT_INTEGER_LEN;
            break;
        case SMALLINT:
            if(w < CLIENT_DEFAULT_SMALLINT_LEN) w = CLIENT_DEFAULT_SMALLINT_LEN;
            break;
        case FLOAT:
            if(w < CLIENT_DEFAULT_FLOAT_LEN) w = CLIENT_DEFAULT_FLOAT_LEN;
            break;
        case DOUBLE_PRECISION:
            if(w < CLIENT_DEFAULT_DOUBLE_LEN) w = CLIENT_DEFAULT_DOUBLE_LEN;
            break;
        case DATE:
            if(w < CLIENT_DEFAULT_DATE_LEN) w = CLIENT_DEFAULT_DATE_LEN;
            break;
        case TIMESTAMP:
            if(w < CLIENT_DEFAULT_TIMESTAMP_LEN) w = CLIENT_DEFAULT_TIMESTAMP_LEN;
            break;
        case TIMESTAMP_WITH_TZ:
            if(w < CLIENT_DEFAULT_TIMESTAMP_WITH_TZ_LEN) w = CLIENT_DEFAULT_TIMESTAMP_WITH_TZ_LEN;
            break;
    }

    if(w > CLIENT_MAX_VALUE_LEN) w = CLIENT_MAX_VALUE_LEN;

    return w + 2 * table_padding();
}

sint8 read_and_format_value(handle ss, uint16 c)
{
    uint32 sz = 0;
    dbclient_value val;
    val.str.buf = strbuf;
    val.str.sz = table_get_col_width(c);
    const pproto_col_desc *desc = dbclient_get_column_desc(ss, c);

    if(DBCLIENT_RETURN_SUCCESS != dbclient_next_col_val(ss, &val))
    {
        return 1;
    }

    switch(desc->data_type)
    {
        case CHARACTER_VARYING:
            strbuf[val.str.sz] = '\0';
            break;
        case DECIMAL:
            if(desc->data_type_precision > 0)
            {
                if(strop_fmt_decimal_pse(strbuf, &sz, CLIENT_STRBUF_SZ, desc->data_type_precision, desc->data_type_scale, 0, &val.d) != 0) return 1;
            }
            else
            {
                if(strop_fmt_decimal_pse(strbuf, &sz, CLIENT_STRBUF_SZ, 1, 6, 1, &val.d) != 0) return 1;
            }
            break;
        case INTEGER:
            snprintf((char*)strbuf, CLIENT_STRBUF_SZ, "%11d", val.i);
            break;
        case SMALLINT:
            snprintf((char*)strbuf, CLIENT_STRBUF_SZ, "%6d", val.s);
            break;
        case FLOAT:
            snprintf((char*)strbuf, CLIENT_STRBUF_SZ, "%1.6e", val.f32);
            break;
        case DOUBLE_PRECISION:
            snprintf((char*)strbuf, CLIENT_STRBUF_SZ, "%1.12e", val.f64);
            break;
        case DATE:
            if(strop_fmt_date(strbuf, &sz, CLIENT_STRBUF_SZ, CLIENT_DEFAULT_DATE_FMT, val.dt) != 0) return 1;
            break;
        case TIMESTAMP:
            if(strop_fmt_timestamp(strbuf, &sz, CLIENT_STRBUF_SZ, CLIENT_DEFAULT_TS_FMT, val.ts) != 0) return 1;
            break;
        case TIMESTAMP_WITH_TZ:
            if(strop_fmt_timestamp_with_tz(strbuf, &sz, CLIENT_STRBUF_SZ, CLIENT_DEFAULT_TS_WITH_TZ_FMT, val.ts_with_tz.ts, val.ts_with_tz.tz) != 0) return 1;
            break;
    }

    return 0;
}

sint8 get_and_print_recordset(handle ss)
{
    uint16 col_num, c;
    uint32 sz;
    sint8  status;
    const pproto_col_desc *col_desc;

    //
    // +-------------+-------------+
    // | col alias 1 | col alias 2 | ...
    // +-------------+-------------+
    // | val1        | val 2       | ...
    // | val3        | val 4       | ...
    // +-------------+-------------+
    //
    if(DBCLIENT_RETURN_SUCCESS == dbclient_begin_recordset(ss))
    {
        if(DBCLIENT_RETURN_SUCCESS == dbclient_get_column_count(ss, &col_num))
        {
            table_set_col_num(col_num);
            for(c = 0; c < col_num; c++)
            {
                col_desc = dbclient_get_column_desc(ss, c);
                table_set_col_width(c, calc_col_width(col_desc));
            }

            // table heading
            table_print_separator();
            for(c = 0; c < col_num; c++)
            {
                col_desc = dbclient_get_column_desc(ss, c);
                table_print_cell((const char *)col_desc->col_alias, col_desc->col_alias_sz, c);
            }
            table_finish_row();
            table_print_separator(col_num);

            // table body
            status = dbclient_fetch_row(ss);
            while(DBCLIENT_RETURN_SUCCESS == status)
            {
                for(c = 0; c < col_num; c++)
                {
                    sz = CLIENT_MAX_VALUE_LEN;
                    if(read_and_format_value(ss, c) != 0)
                    {
                        return 1;
                    }
                    table_print_cell((const char *)strbuf, sz, c);
                }
                table_finish_row();

                status = dbclient_fetch_row(ss);
            }

            if(DBCLIENT_RETURN_NO_MORE_ROWS == status)
            {
                table_print_separator(col_num);
            }
        }
    }

    dbclient_close_recordset(ss);

    return 0;
}

sint8 wait_for_execution_completion(handle ss)
{
    dbclient_return_code status = dbclient_execution_status(ss);

    while(DBCLIENT_RETURN_IN_PROGRESS == status)
    {
        putc(CLIENT_PROGRESS_CHAR, stdout);
        status = dbclient_execution_status(ss);
    }
    puts("");

    return status;
}

int main(int argc, char **argv)
{
    puts("=== Persistence command line tool v0.1 ===");
    init();
    parse_args(argc, argv);

    if(0 == user)
    {
        if(0 == (user = getenv("LOGNAME")))
        {
            if(0 == (user = getlogin()))
            {
                perror("Falied to determine current user name");
                return 1;
            }
        }
    }

    if(0 == password && p_option_present)
    {
        fputs("password: ", stdout);

        struct termios termios_original, termios_no_echo;
        if(tcgetattr(0, &termios_original) != 0)
        {
            perror("Failed to get terminal settings");
            return 1;
        }
        memcpy(&termios_no_echo, &termios_original, sizeof(termios_original));
        termios_no_echo.c_lflag ^= ECHO;
        if(tcsetattr(0, TCSANOW, &termios_no_echo))
        {
            perror("Failed to set terminal settings");
            return 1;
        }

        if(0 != fgets(password_buf, CLIENT_PASSWORD_MAX_LEN, stdin))
        {
            password_buf[strlen(password_buf)-1] = '\0';
            password = password_buf;
        }

        if(tcsetattr(0, TCSANOW, &termios_original))
        {
            perror("Failed to restore terminal settings");
            return 1;
        }
    }

    if(strlen(user) > AUTH_USER_NAME_SZ - ENCODING_MAXCHAR_LEN)
    {
        fprintf(stderr, "User name %s is too long\n", user);
        return 1;
    }

    dbclient_init();

    size_t ss_sz = dbclient_get_session_state_sz();
    void *ss_buf = malloc(ss_sz);
    if(NULL == ss_buf)
    {
        perror("Allocation db session");
        return 1;
    }

    handle ss = dbclient_allocate_session(ss_buf, client_enc, stderr);

    if(DBCLIENT_RETURN_SUCCESS != dbclient_connect(ss, host, port)) return 1;

    if(DBCLIENT_RETURN_SUCCESS != dbclient_authenticate(ss, user, password)) return 1;

    uint8 in_comment = 0, in_string = 0;
    char ch;
    int aborted = 0;
    char_state chst;
    memset(&chst, 0, sizeof(chst));

    if(DBCLIENT_RETURN_SUCCESS != dbclient_begin_statement(ss)) return 1;

    while(!aborted)
    {
        ch = next_char(&chst);
        if(in_string == 2)  // check for escape sequence for ' inside string
        {
            if(ch == '\'') in_string = 1;
            else in_string = 0;
        }
        if(in_comment == 1) // previous ch == '-'
        {
            if(ch == '-') in_comment = 2;
            else in_comment = 0;
        }

        if(in_string == 0 && in_comment == 0)
        {
            if(ch == stmt_delimiter)
            {
                // statement completed
                if(DBCLIENT_RETURN_SUCCESS != dbclient_finish_statement(ss))
                {
                    aborted = 1;
                }

                switch(wait_for_execution_completion(ss))
                {
                    case DBCLIENT_RETURN_SUCCESS_RS:
                        get_and_print_recordset(ss);
                        break;
                    case DBCLIENT_RETURN_SUCCESS_MSG:

                    case DBCLIENT_RETURN_SUCCESS:
                        puts("Complete");
                        break;
                }

                if(DBCLIENT_RETURN_SUCCESS != dbclient_begin_statement(ss))
                {
                    aborted = 1;
                }
            }
            else if(ch == '\'')
            {
                in_string = 1;
            }
            else if(ch == '-') in_comment = 1;
        }
        else if(in_comment == 2)
        {
            if(ch == '\n') in_comment = 0;
        }
        else if(in_string == 1)
        {
            if(ch == '\'') in_string = 2;   // end of string or '' (need to check next char)
        }

        // send statement char to server
        if(DBCLIENT_RETURN_SUCCESS != dbclient_statement(ss, (const uint8 *)&ch, sizeof(char)))
        {
            aborted = 1;
        }
    }

    fflush(NULL);

    return aborted;
}
