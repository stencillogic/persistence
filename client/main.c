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

#define PASSWORD_MAX_LEN 1024

char    *user = 0;
char    *password = 0;
char    *host = "localhost";
int     port = SERVER_DEFAULT_PORT;
int     p_option_present = 0;
char    password_buf[PASSWORD_MAX_LEN];

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
    return ENCODING_UTF8;
}

void enter_repl()
{
    // read statement/command
    // send statement
    // print result
    // repeat
}

int main(int argc, char **argv)
{
    puts("=== Persistence command line client v0.1 ===");
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

    printf("user: %s\n", user);
    printf("password: %s\n", password != 0 ? password : "none");
    printf("host: %s\n", host);
    printf("user: %d\n", port);

    if(strlen(user) > AUTH_USER_NAME_SZ-ENCODING_MAXCHAR_LEN)
    {
        fprintf(stderr, "User name %s is too long\n", user);
    }

    uint8 responce;
    int sock = make_connection();
    encoding enc = client_encoding();

    if(pproto_send_client_hello(enc) == 0
            && pproto_read_server_hello() == 0
            && pproto_read_auth_request() == 0)
    {
        auth_credentials cred;
        strncpy((char *)cred.user_name, user, AUTH_USER_NAME_SZ);
        auth_hash_pwd((uint8 *)password, strlen(password), cred.credentials);

        if(pproto_send_auth(&cred) == 0
                && pproto_read_auth_responce(&responce) == 0)
        {
            if(1 == responce)
            {
                enter_repl();
            }
            else
            {
                fputs("Access denied\n", stderr);
            }
        }
    }

	close(sock);

    return 0;
}
