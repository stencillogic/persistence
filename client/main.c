// persistence command line utility

#include "defs/defs.h"
#include "client/pproto_client.h"
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
#define CLIENT_RS_PADDING               (1)
#define CLIENT_MAX_COLUMNS              (1024)
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
uint8       nulls[128];
encoding    client_enc;

struct rs_columns
{
    uint32 width;
    pproto_col_desc desc;
} g_rs_columns[CLIENT_MAX_COLUMNS];

typedef struct _char_state
{
    char *line;
    char *lineptr;
    char *lineend;
    size_t linelen;
} char_state;

union _value_buf
{
    decimal d;
    sint32  i;
    sint16  s;
    float32 f32;
    float64 f64;
    uint64  dt;
    uint64  ts;
    struct
    {
        uint64 ts;
        sint16 tz;
    } ts_with_tz;
} value_buf;

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

// convert socket error code to string
const char* herrno_msg()
{
    switch(h_errno)
    {
        case HOST_NOT_FOUND:
              return "host is unknown";
        case NO_DATA:
            return "requested name does not have an IP address";
        case NO_RECOVERY:
            return "an nonrecoverable name server error occurred";
        case TRY_AGAIN:
            return "a temporary error, try again later";
    }
    return "unkonwn error";
}

int make_connection()
{
    int sock;
    struct sockaddr_in name;
    struct hostent *hostinfo;

    sock = socket(PF_INET, SOCK_STREAM, 0);
    if(sock < 0)
    {
          perror("Creating socket");
          exit(1);
    }

      name.sin_family = AF_INET;
    name.sin_port = htons(port);
    hostinfo = gethostbyname(host);
    if(NULL == hostinfo)
    {
        fprintf(stderr, "Host name resolution: %s.\n", herrno_msg());
        exit(1);
    }
    name.sin_addr = *(struct in_addr *)hostinfo->h_addr;

    if(-1 == connect(sock, (struct sockaddr*)&name, sizeof(name)))
    {
        perror("Connecting");
        exit(1);
    }

    return sock;
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

void process_server_error(error_code errcode)
{
    fprintf(stderr, "Error from server: %s\n", error_msg(errcode));
    fflush(stderr);
}

// calculate output column width
uint32 calc_col_width(const pproto_col_desc *cd)
{
    uint32 w = cd->col_alias_sz;
    switch(cd->data_type)
    {
        case CHARACTER_VARYING:
            if(w < cd->data_type_len) w = cd->data_type_len;
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

    return w + 2 * CLIENT_RS_PADDING;
}

void table_print_separator(uint16 col_num)
{
    fputc(' ', stdout);
    for(uint16 c = 0; c < col_num; c++)
    {
        strbuf[0] = '+';
        memset(strbuf + 1, '-', g_rs_columns[c].width);
        strbuf[g_rs_columns[c].width + 1] = '\0';
        fputs((const char*)strbuf, stdout);
    }
    fputs("+\n", stdout);
}

void table_print_cell(uint8 *value, uint32 w, uint32 c)
{
    fputs(" | ", stdout);
    fprintf(stdout, "\e[s");
    if(w + 2 * CLIENT_RS_PADDING > g_rs_columns[c].width) w = g_rs_columns[c].width - 2 * CLIENT_RS_PADDING;
    fwrite(value, w, 1, stdout);
    fprintf(stdout, "\e[u\e");
}

void table_finish_row()
{
    fputs(" |\n", stdout);
}


sint8 read_and_format_value(uint16 c)
{
    uint32 sz = 0;
    switch(g_rs_columns[c].desc.data_type)
    {
        case CHARACTER_VARYING:
            if(pproto_read_str_begin() != 0) return 1;
            sz = g_rs_columns[c].width;
            if(pproto_read_str(strbuf, &sz) != 0) return 1;    // aborting if error
            break;
        case DECIMAL:
            if(pproto_read_decimal_value(&value_buf.d) != 0) return 1;
            if(strop_fmt_decimal_pse(strbuf, &sz, CLIENT_STRBUF_SZ, g_rs_columns[c].desc.data_type_precision, g_rs_columns[c].desc.data_type_scale, 0, &value_buf.d) != 0) return 1;
            break;
        case INTEGER:
            if(pproto_read_integer_value(&value_buf.i) != 0) return 1;
            snprintf((char*)strbuf, CLIENT_STRBUF_SZ, "%11d", value_buf.i);
            break;
        case SMALLINT:
            if(pproto_read_smallint_value(&value_buf.s) != 0) return 1;
            snprintf((char*)strbuf, CLIENT_STRBUF_SZ, "%6d", value_buf.s);
            break;
        case FLOAT:
            if(pproto_read_float_value(&value_buf.f32) != 0) return 1;
            snprintf((char*)strbuf, CLIENT_STRBUF_SZ, "%1.6e", value_buf.f32);
            break;
        case DOUBLE_PRECISION:
            if(pproto_read_double_value(&value_buf.f64) != 0) return 1;
            snprintf((char*)strbuf, CLIENT_STRBUF_SZ, "%1.12e", value_buf.f64);
            break;
        case DATE:
            if(pproto_read_date_value(&value_buf.dt) != 0) return 1;
            if(strop_fmt_date(strbuf, &sz, CLIENT_STRBUF_SZ, CLIENT_DEFAULT_DATE_FMT, value_buf.dt) != 0) return 1;
            break;
        case TIMESTAMP:
            if(pproto_read_timestamp_value(&value_buf.ts) != 0) return 1;
            if(strop_fmt_timestamp(strbuf, &sz, CLIENT_STRBUF_SZ, CLIENT_DEFAULT_TS_FMT, value_buf.ts) != 0) return 1;
            break;
        case TIMESTAMP_WITH_TZ:
            if(pproto_read_timestamp_with_tz_value(&value_buf.ts_with_tz.ts, &value_buf.ts_with_tz.tz) != 0) return 1;
            if(strop_fmt_timestamp_with_tz(strbuf, &sz, CLIENT_STRBUF_SZ, CLIENT_DEFAULT_TS_WITH_TZ_FMT, value_buf.ts_with_tz.ts, value_buf.ts_with_tz.tz) != 0) return 1;
            break;
    }
    return 0;
}

sint8 get_and_print_recordset()
{
    pproto_msg_type msg_type = pproto_read_msg_type();
    error_code errcode;
    uint16 col_num, c;
    uint32 sz;
    sint8  status;
    pproto_col_desc col_desc;

    switch(msg_type)
    {
        case PPROTO_MSG_TYPE_ERR:
            if(pproto_read_error(&errcode) != 0) return 1;
            process_server_error(errcode);
            break;
        case PPROTO_SUCCESS_WITH_TEXT_MSG:
            fputs("Success: ", stdout);
            if(pproto_read_str_begin() != 0) return 1;
            do
            {
                sz = CLIENT_STRBUF_SZ;
                if(pproto_read_str(strbuf, &sz) != 0) return 1;    // aborting if error
                fwrite(strbuf, 1, sz, stdout);
            } while(sz == CLIENT_STRBUF_SZ);
            fputc('\n', stdout);
            fflush(stdout);
            break;
        case PPROTO_SUCCESS_WITHOUT_TEXT_MSG:
            fputs("Success\n", stdout);
            fflush(stdout);
            break;
        case PPROTO_RECORDSET_MSG:
            //
            // +-------------+-------------+
            // | col alias 1 | col alias 2 | ...
            // +-------------+-------------+
            // | val1        | val 2       | ...
            // | val3        | val 4       | ...
            // +-------------+-------------+
            //
            if(pproto_read_recordset_col_num(&col_num) != 0) return 1;

            if(col_num > CLIENT_MAX_COLUMNS)
            {
                fprintf(stderr, "recordset continas more than %d columns. This client supports only %d columns.\n", CLIENT_MAX_COLUMNS, CLIENT_MAX_COLUMNS);
                col_num = CLIENT_MAX_COLUMNS;
            }

            fputs("\n ", stdout);
            for(c = 0; c < col_num; c++)
            {
                if(pproto_read_recordset_col_desc(&col_desc) != 0) return 1;
                g_rs_columns[c].desc = col_desc;
                g_rs_columns[c].width = calc_col_width(&col_desc);
            }

            // table heading
            table_print_separator(col_num);
            for(c = 0; c < col_num; c++)
            {
                table_print_cell(g_rs_columns[c].desc.col_alias, g_rs_columns[c].desc.col_alias_sz, c);
            }
            table_finish_row();
            table_print_separator(col_num);

            // table body
            status = pproto_recordset_start_row(nulls);
            while(status)
            {
                if(status == -1) return 1;

                for(c = 0; c < col_num; c++)
                {
                    sz = CLIENT_MAX_VALUE_LEN;
                    read_and_format_value(c);
                    table_print_cell(strbuf, sz, c);
                }
                table_finish_row();

                status = pproto_recordset_start_row(nulls);
            }
            table_print_separator(col_num);

            break;
        case PPROTO_PROGRESS_MSG:
            fputc(CLIENT_PROGRESS_CHAR, stdout);
            break;
        default:
            fprintf(stderr, "Unexpected message type from server: %x\n", msg_type);
            return 1;
    }

    return 0;
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
                exit(1);
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
            exit(1);
        }
        memcpy(&termios_no_echo, &termios_original, sizeof(termios_original));
        termios_no_echo.c_lflag ^= ECHO;
        if(tcsetattr(0, TCSANOW, &termios_no_echo))
        {
            perror("Failed to set terminal settings");
            exit(1);
        }

        if(0 != fgets(password_buf, CLIENT_PASSWORD_MAX_LEN, stdin))
        {
            password_buf[strlen(password_buf)-1] = '\0';
            password = password_buf;
        }

        if(tcsetattr(0, TCSANOW, &termios_original))
        {
            perror("Failed to restore terminal settings");
            exit(1);
        }
    }

    if(strlen(user) > AUTH_USER_NAME_SZ - ENCODING_MAXCHAR_LEN)
    {
        fprintf(stderr, "User name %s is too long\n", user);
        exit(1);
    }

    enum client_states
    {
        PCLIENT_STATE_HELLO,
        PCLIENT_STATE_READ_ERR_AND_EXIT,
        PCLIENT_STATE_UNEXPECTED_MSG_TYPE,
        PCLIENT_STATE_AUTH,
        PCLIENT_STATE_AUTH_RESPONCE,
        PCLIENT_STATE_USER_INPUT
    };

    int sock = make_connection();
    pproto_set_sock(sock);
    encoding enc = client_enc;

    if(pproto_send_client_hello(enc) != 0) exit(1);

    pproto_msg_type msg_type;
    error_code errcode;
    uint8 state = PCLIENT_STATE_HELLO;
    uint8 responce;
    uint8 in_comment = 0, in_string = 0;
    char ch;
    int aborted = 0;
    char_state chst;
    memset(&chst, 0, sizeof(chst));

    while(!aborted)
    {
        switch(state)
        {
            case PCLIENT_STATE_HELLO: // read server hello
                msg_type = pproto_read_msg_type();
                if(msg_type == PPROTO_MSG_TYPE_ERR)
                {
                    state = PCLIENT_STATE_READ_ERR_AND_EXIT;
                }
                else if(msg_type == PPROTO_SERVER_HELLO_MSG)
                {
                    if(pproto_read_server_hello() != 0)
                    {
                        aborted = 1;
                    }
                    else state = PCLIENT_STATE_AUTH;
                }
                else state = PCLIENT_STATE_UNEXPECTED_MSG_TYPE;
                break;
            case PCLIENT_STATE_READ_ERR_AND_EXIT: // read error and exit
                if(pproto_read_error(&errcode) == 0)
                {
                    fprintf(stderr, "Error from server: %s\n", error_msg(errcode));
                }
                aborted = 1;
                break;
            case PCLIENT_STATE_UNEXPECTED_MSG_TYPE: // process protocol violation
                fprintf(stderr, "Unexpected message type from server: %x\n", msg_type);
                aborted = 1;
                break;
            case PCLIENT_STATE_AUTH: // read server auth
                msg_type = pproto_read_msg_type();
                if(msg_type == PPROTO_MSG_TYPE_ERR)
                {
                    state = PCLIENT_STATE_READ_ERR_AND_EXIT;
                }
                else if(msg_type == PPROTO_AUTH_REQUEST_MSG)
                {
                    if(pproto_read_auth_request() != 0)
                    {
                        aborted = 1;
                    }
                    else
                    {
                        auth_credentials cred;
                        strncpy((char *)cred.user_name, user, AUTH_USER_NAME_SZ);
                        auth_hash_pwd((uint8 *)password, strlen(password), cred.credentials);

                        if(pproto_send_auth(&cred) != 0)
                        {
                            aborted = 1;
                        }
                        else state = PCLIENT_STATE_AUTH_RESPONCE;
                    }
                }
                else state = PCLIENT_STATE_UNEXPECTED_MSG_TYPE;
                break;
            case PCLIENT_STATE_AUTH_RESPONCE: // read auth responce
                msg_type = pproto_read_msg_type();
                if(msg_type == PPROTO_MSG_TYPE_ERR)
                {
                    state = PCLIENT_STATE_READ_ERR_AND_EXIT;
                }
                else if(msg_type == PPROTO_AUTH_RESPONCE_MSG)
                {
                    if(pproto_read_auth_responce(&responce) != 0)
                    {
                        aborted = 1;
                    }
                    else
                    {
                        if(1 == responce) state = PCLIENT_STATE_USER_INPUT;
                        else
                        {
                            fputs("Invalid credentials specified\n", stderr);
                            aborted = 1;
                        }
                    }
                }
                else state = PCLIENT_STATE_UNEXPECTED_MSG_TYPE;
                break;
            case PCLIENT_STATE_USER_INPUT: // request input from user (read and exec sql statements)
                if(pproto_sql_stmt_begin() != 0)
                {
                    aborted = 1;
                }

                while(!aborted)    // entering repl loop
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
                            if(pproto_sql_stmt_finish() != 0)
                            {
                                aborted = 1;
                            }

                            get_and_print_recordset();

                            if(pproto_sql_stmt_begin() != 0)
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
                    if(pproto_send_sql_stmt((const uint8 *)&ch, sizeof(char)) != 0)
                    {
                        aborted = 1;
                    }
                }
                break;
        }
    }

    fflush(NULL);
    close(sock);

    return aborted;
}
