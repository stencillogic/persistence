// persistence command line utility

#include "defs/defs.h"
#include "client/pproto_client.h"
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define CLIENT_SQL_STATEMENT_DELIMITER ";"
#define CLIENT_PASSWORD_MAX_LEN 1024
#define CLIENT_STRBUF_SZ 8192

#define CLIENT_MAX_COL_STR_LEN          256
#define CLIENT_MAX_COL_DECIMAL_LEN      40
#define CLIENT_MAX_COL_INTEGER_LEN      10
#define CLIENT_MAX_COL_SMALLINT_LEN     5
#define CLIENT_MAX_COL_FLOAT_LEN        10
#define CLIENT_MAX_COL_DOUBLE_PRECISION_LEN  15
#define CLIENT_MAX_COL_DATE_LEN         50
#define CLIENT_MAX_COL_TIMESTAMP_LEN    50

char    *user = 0;
char    *password = 0;
char    *host = "localhost";
int     port = SERVER_DEFAULT_PORT;
int     p_option_present = 0;
char    password_buf[CLIENT_PASSWORD_MAX_LEN];
uint8   stmt_delimiter = CLIENT_SQL_STATEMENT_DELIMITER;
uint8   stmt_delimiter_len = 1;
const char *prompt = "ptool> ";
uint8   strbuf[CLIENT_STRBUF_SZ];

uint32  col_str_len = CLIENT_MAX_COL_STR_LEN;
uint32  col_decimal_len = CLIENT_MAX_COL_DECIMAL_LEN;
uint32  col_integer_len = CLIENT_MAX_COL_INTEGER_LEN;
uint32  col_smallint_len = CLIENT_MAX_COL_SMALLINT_LEN;
uint32  col_float_len = CLIENT_MAX_COL_FLOAT_LEN;
uint32  col_double_len = CLIENT_MAX_COL_DOUBLE_PRECISION_LEN;
uint32  col_date_len = CLIENT_MAX_COL_DATE_LEN;
uint32  col_ts_len = CLIENT_MAX_COL_TIMESTAMP_LEN;

typedef struct _char_state
{
    char *line;
    char *lineptr;
    char *lineend;
    size_t linelen;
} char_state;


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

encoding client_encoding()
{
    // TODO: determine encoding
    return ENCODING_UTF8;
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

void process_server_error(errcode)
{
    frpintf(stderr, "Error from server: %s\n", error_msg(errcode));
    fflush(stderr);
}

// calculate output column width
uint32 calc_col_width(struct pproto_col_desc *cd)
{
    uint32 w = cd->col_alias_sz;
    switch(cd->data_type)
    {
        case CHARACTER_VARYING:
            if(w < cd->data_type_len) w = cd->data_type_len;
            if(w > col_str_len) w = col_str_len;
            break;
        case DECIMAL:
            if(w < cd->data_type_precision + sizeof(decimal_separator)) w = cd->data_type_precision + sizeof(decimal_separator);
            if(w > col_decimal_len) w = col_decimal_len;
            break;
        case INTEGER:
            if(w < 10) w = 10;
            if(w > col_integer_len) w = col_integer_len;
            break;
        case SMALLINT:
            if(w < 5) w = 5;
            if(w > col_smallint_len) w = col_smallint_len;
            break;
        case FLOAT:
            if(w < 10) w = 10;
            if(w > col_float_len) w = col_float_len;
            break;
        case DOUBLE_PRECISION:
            if(w < 15) w = 15;
            if(w > col_double_len) w = col_double_len;
            break;
        case DATE:
            if(w < 50) w = 50;
            if(w > col_date_len) w = col_date_len;
            break;
        case TIMESTAMP: // yyyy-mm-dd hh:mi:ss.tttttt+tz
            if(w < 50) w = 50;
            if(w > col_ts_len) w = col_ts_len;
            break;
    }
    return w;
}

void get_and_print_recordset()
{
    pproto_msg_type msg_type = pproto_read_msg_type();
    error_code errcode;
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
                sz = STRBUF_SZ;
                if(pproto_read_str(strbuf, &sz) != 0) return 1;    // aborting if error
                fwrite(strbuf, 1, sz, stdout);
            } while(sz == STRBUF_SZ);
            fputc('\n', stdout);
            fflush(stdout);
            break;
        case PPROTO_SUCCESS_WITHOUT_TEXT_MSG:
            fputs("Success\n", stdout);
            fflush(stdout);
            break;
        case PPROTO_RECORDSET_MSG:
            if(pproto_read_recordset_descriptor(&rs_descriptor) != 0) return 1;
            for(uint16 c = 0; c < rs_descriptor.col_num; c++)
            {
                col_width = calc_col_width(col + c);
            }
            break;
        case PPROTO_PROGRESS_MSG:
            // TODO: display progress
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

        if(0 != fgets(password_buf, PASSWORD_MAX_LEN, stdin))
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

    if(strlen(user) > AUTH_USER_NAME_SZ-ENCODING_MAXCHAR_LEN)
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
    encoding enc = client_encoding();

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
                    frpintf(stderr, "Error from server: %s\n", error_msg(errcode));
                }
                aborted = 1;
                break;
            case PCLIENT_STATE_UNEXPECTED_MSG_TYPE: // process protocol violation
                fprintf(stderr, "Unexpected message type from server: %x\n", msg_type);
                aborted = 1;
                break;
            case PCLIENT_STATE_AUTH: // read server auth
                msg_type = pproto_read_msg_type();
                if(msg_type == PPROTO_PPROTO_MSG_TYPE_ERR)
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
                if(msg_type == PPROTO_PPROTO_MSG_TYPE_ERR)
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
                        if(ch == ';')
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
                    if(pproto_send_sql_stmt(&ch, sizeof(char)) != 0)
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
