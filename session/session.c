#include "session/session.h"
#include "session/pproto_server.h"
#include "logging/logger.h"
#include "auth/auth_server.h"
#include "parser/parser.h"
#include "execution/execution.h"
#include "parser/lexer.h"
#include "common/string_literal.h"
#include <stdlib.h>
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
    int         client_sock;
    encoding    client_encoding;
    encoding    server_encoding;
    uint32      user_id;
    handle      lexer;
} g_session_state = {-1, ENCODING_UNKNOWN, ENCODING_UNKNOWN, 0};

encoding session_encoding()
{
    return g_session_state.client_encoding;
}

// implements authentication flow
// return 0 on successful auth, 1 on error, 2 if connection with client is closed
sint8 session_auth_client()
{
    sint8 res;
    auth_credentials cred;

    switch(pproto_server_read_msg_type())
    {
        case PPROTO_GOODBYE_MSG:
            return 2;
            break;
        case PPROTO_CLIENT_HELLO_MSG:
            break;
        default:
            return 1;
            break;
    }

    if(pproto_server_read_client_hello(&g_session_state.client_encoding) != 0)
    {
        return 1;
    }

    if(0 == g_session_state.client_encoding || NULL == encoding_name(g_session_state.client_encoding) ||
        encoding_is_convertable(g_session_state.client_encoding, g_session_state.server_encoding) == 0)
    {
        return 1;
    }

    pproto_server_set_encoding(g_session_state.client_encoding);

    if(pproto_server_send_server_hello() != 0)
    {
        return 1;
    }

    if(pproto_server_send_auth_request() != 0)
    {
        return 1;
    }

    while(1)
    {
        switch(pproto_server_read_msg_type())
        {
            case PPROTO_GOODBYE_MSG:
                return 2;
                break;
            case PPROTO_AUTH_MSG:
                break;
            default:
                return 1;
                break;
        }

        res = pproto_server_read_auth(&cred);
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

        if(pproto_server_send_auth_responce(0) != 0)
        {
            return 1;
        }
    }

    if(pproto_server_send_auth_responce(1))
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
        msg_type = pproto_server_read_msg_type();
        logger_debug(_ach("session, message received, type: %d"), (int)msg_type);
        if(PPROTO_MSG_TYPE_ERR == msg_type)
        {
            return 1;
        }

        switch(msg_type)
        {
            case PPROTO_SQL_REQUEST_MSG:
                if(execution_exec_statement(g_session_state.lexer))
                {
                    return 1;
                }
                break;
            case PPROTO_GOODBYE_MSG:
                pproto_server_send_goodbye();
                return 0;
            default:
                pproto_server_send_error(ERROR_PROTOCOL_VIOLATION, NULL);
                logger_error(_ach("session, unexpected message type received: %d"), (int)msg_type);
                return 1;
        }
    }

    return 0;
}

sint8 session_create_lexer()
{
    handle str_literal = string_literal_create(malloc(string_literal_alloc_sz()));
    if(!str_literal)
    {
        logger_error(_ach("session, lexer creation failed; out of memory"));
        return -1;
    }

    lexer_interface li;
    li.next_char = pproto_server_read_char;
    li.report_error = pproto_server_send_error;

    g_session_state.lexer = lexer_create(malloc(lexer_get_allocation_size()), g_session_state.server_encoding, str_literal, li);
    if(!g_session_state.lexer)
    {
        logger_error(_ach("session, lexer creation failed; out of memory"));
        return -1;
    }

    return 0;
}

sint8 session_create(int client_sock)
{
    g_session_state.client_sock = client_sock;

    encoding_init();
    pproto_server_set_sock(g_session_state.client_sock);

    // TODO: connect to tipi and determine server encoding

    if(session_create_lexer() != 0) return 1;

    switch(session_auth_client())
    {
        case 1:     // error
            return 1;
        case 0:     // client authenticted
            return session_enter_repl();
        default:    // session closed without error
            return 0;
    }

    return 0;
}
