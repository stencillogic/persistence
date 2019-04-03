#include "session/session.h"
#include "session/pproto_server.h"
#include "logging/logger.h"
#include "auth/auth_server.h"
#include <unistd.h>
#include <string.h>

//
//       session automaton
// -----------------------------------
//       |  condition
// state |----------------------------
//       | a | b | c | d | e | f | g |
// -----------------------------------
//   0   | 1 | 4 | 4 | 4 | 4 | 4 | 4 |
//   1   | 4 | 1 | 2 | 4 | 4 | 4 | 4 |
//   2   | 2 | 2 | 2 | 3 | 2 | 4 | 2 |
//   3   | 3 | 3 | 3 | 3 | 2 | 3 | 3 |
//   4   | 4 | 4 | 4 | 4 | 4 | 4 | 4 |
//
//
// States:
//  0 - initial state, client just connected
//  1 - hello from client received
//  2 - client authenticated
//  3 - sql being executed
//  4 - session terminated, client disconnected
//
// Conditions:
//  a - hello_message from client
//  b - auth_message from client, wrong credentials
//  c - auth_message from client, correct credentials
//  d - sql_request_message from client
//  e - cancel_message
//  f - goodbye_message
//  g - incorrect message

struct {
    int client_sock;
    encoding client_encoding;
    encoding server_encoding;
    uint32 user_id;
} g_session_state = {-1, ENCODING_UNKNOWN, ENCODING_UNKNOWN, 0};

encoding session_encoding()
{
    return g_session_state.client_encoding;
}

sint8 session_auth_client()
{
    sint8 res;
    auth_credentials cred;

    if(pproto_read_client_hello(&g_session_state.client_encoding) != 0)
    {
        return 1;
    }

    if(0 == g_session_state.client_encoding || NULL == encoding_name(g_session_state.client_encoding) ||
        encoding_is_convertable(g_session_state.client_encoding, g_session_state.server_encoding) == 0)
    {
        return 1;
    }

    pproto_set_encoding(g_session_state.client_encoding);

    if(pproto_send_server_hello() != 0)
    {
        return 1;
    }

    if(pproto_send_auth_request() != 0)
    {
        return 1;
    }

    while(1)
    {
        res = pproto_read_auth(&cred);
        sleep(1);
        if(0 == res)
        {
            if(auth_check_credentials(&cred, &g_session_state.user_id) == 1)
            {
                break;
            }
        }
        else if(-1 == res)
        {
            return 1;
        }

        if(pproto_send_auth_responce(0) != 0)
        {
            return 1;
        }
    }

    if(pproto_send_auth_responce(1))
    {
        return 1;
    }

    return 0;
}

sint8 session_enter_repl()
{
    pproto_msg_type msg_type;

    while(1)
    {
        msg_type = pproto_read_msg_type();
        logger_debug(_ach("session, message received, type: %d"), (int)msg_type);
        if(PPROTO_MSG_TYPE_ERR == msg_type)
        {
            return 1;
        }

        switch(msg_type)
        {
            case PPROTO_SQL_REQUEST_MSG:
                // TODO: process sql request
                break;
            case PPROTO_GOODBYE_MSG:
                pproto_send_goodbye();
                return 0;
            default:
                pproto_send_error(ERROR_PROTOCOL_VIOLATION);
                logger_warn(_ach("session, unexpected message type received: %d"), (int)msg_type);
                break;
        }
    }

    return 0;
}

sint8 session_create(int client_sock)
{
    g_session_state.client_sock = client_sock;

    encoding_init();
    pproto_set_sock(g_session_state.client_sock);

    if(session_auth_client() != 0) return 1;

    if(session_enter_repl() != 0) return 1;

    return 0;
}
