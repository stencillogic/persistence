#include "session/listener.h"
#include "common/error.h"
#include "config/config.h"
#include "logging/logger.h"
#include <sys/types.h>
#include <sys/socket.h>

#define LISTENER_BACKLOG 10

// creates listener, return 0 on normal termination and non 0 otherwise
sint8 listener_create()
{
    struct sockaddr_in serv_addr;
    int client_sock;
    struct sockaddr client_addr;
    socklen_t client_addr_size;

    int sock = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if(-1 == sock)
    {
        logger_error(_ach("Socket creation error: %s"), strerror(errno));
        return 1;
    }

    sint64 hport;
    sint8 res = config_get_int(CONFIG_LISTENER_TCP_PORT, &hport);
    assert(0 == res);

    memset(&serv_addr, 0, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(hport);

    if(-1 == bind(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)))
    {
        logger_error(_ach("Socket binding error: %s"), strerror(errno));
        return 1;
    }

    if(-1 == listen(sock, LISTENER_BACKLOG))
    {
        logger_error(_ach("Start litening failed: %s"), strerror(errno));
        return 1;
    }

    sint8 not_terminated = 1;
    while(not_terminated)
    {
        client_addr_size = sizeof(client_addr);
        client_sock = accept4(sock, (struct sockaddr *)&client_addr, &client_addr_size, SOCK_CLOEXEC);
        if(-1 == client_sock)
        {
            logger_error("Accepting connection from client: %s", strerror(errno));
        }
        else
        {
            pid_t pid = fork();
            if(pid < 0)
            {
                error_set(ERROR_SESSION_INIT_FAIL);
                const achar *errmes = error_msg();
                ioutils_send_fully();
            }
            else if(pid == 0)
            {
                execve();
            }
        }
    }

    return 0;
}
