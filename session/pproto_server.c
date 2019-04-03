#include "session/pproto_server.h"
#include "common/ioutils.h"
#include "logging/logger.h"
#include <string.h>


void pproto_set_sock(int client_sock)
{
    ioutils_set_sock(client_sock);
}

void pproto_set_encoding(encoding client_encoding)
{
    ioutils_set_client_encoding(client_encoding);
}

sint8 pproto_read_text_string(uint8 *buf, uint32 *sz, uint32 *charlen)
{
    uint8 magic, is_nt;
    uint32 len;

    if(ioutils_get_uint8(&magic))
    {
        logger_error(_ach("pproto, failed to read text string: io error"));
    }

    if(PPROTO_UTEXT_STRING_MAGIC == magic)
    {
        len = *sz;
        is_nt = 1;
    }
    else if (PPROTO_LTEXT_STRING_MAGIC == magic)
    {
        is_nt = 0;
        if(ioutils_get_uint32(&len) != 0)
        {
            logger_error(_ach("pproto, reading limited text string: io error"));
            return 1;
        }

        if(len > *sz)
        {
            logger_error(_ach("pproto, reading limited text string: string size of %u bytes is \
                        greater than expected size of %u bytes"), len, *sz);
            return 1;
        }
    }
    else
    {
        logger_error(_ach("pproto, failed to read text string: invalid magic"));
        return 1;
    }

    if(ioutils_get_str(buf, &len, charlen, is_nt) != 0)
    {
        logger_error(_ach("pproto, reading text string: io error"));
        return 1;
    }

    return 0;
}

sint8 pproto_read_auth(auth_credentials *cred)
{
    uint8 magic;
    uint8 user_name[AUTH_USER_NAME_SZ + ENCODING_MAXCHAR_LEN];
    uint32 sz = sizeof(user_name), charlen;
    if(ioutils_get_uint8(&magic) != 0)
    {
        logger_error(_ach("pproto, failed to read auth message: io error"));
        return 1;
    }

    if(PPROTO_AUTH_MESSAGE_MAGIC != magic)
    {
        logger_error(_ach("pproto, failed to read auth message: message magic missmatch"));
        return 1;
    }

    if(pproto_read_text_string(user_name, &sz, &charlen) != 0)
    {
        logger_error(_ach("pproto, failed to read auth message user name: io error"));
        return 1;
    }

    if(sz > AUTH_USER_NAME_SZ || 0 == sz)
    {
        logger_error(_ach("pproto, auth message user name is invalid: io error"));
        return 1;
    }
    else
    {
        memcpy(cred->user_name, user_name, sz);
    }

    if(ioutils_get(cred->credentials, 64) != 0)
    {
        logger_error(_ach("pproto, reading auth message credentials: io error"));
        return 1;
    }

    return 0;
}

sint8 pproto_send_error(error_code errcode)
{
    error_set(errcode);
    if(ioutils_send_str((uint8 *)error_msg(), 1) != 0
            || ioutils_flush_send() != 0)
    {
        logger_error(_ach("pproto, failed to send error message: io error"));
        return 1;
    }

    return 0;
}

sint8 pproto_read_client_hello(encoding *client_encoding)
{
    uint16 val;

    if(ioutils_get_uint16(&val) != 0)
    {
        logger_error(_ach("pproto, failed to read client hello message: io error"));
        return 1;
    }

    if(PPROTO_CLIENT_HELLO_MAGIC != val)
    {
        logger_error(_ach("pproto, failed to read client hello message: message magic missmatch"));
        return 1;
    }

    if(ioutils_get_uint16(&val) != 0)
    {
        logger_error(_ach("pproto, failed to read client hello message: io error"));
        return 1;
    }

    *client_encoding = val;

    return 0;
}

sint8 pproto_send_auth_request()
{
    if(ioutils_send_uint8(PPROTO_AUTH_REQUEST_MESSAGE_MAGIC) != 0
            || ioutils_flush_send() != 0)
    {
        logger_error(_ach("pproto, failed to send auth request message: io error"));
        return 1;
    }

    return 0;
}

sint8 pproto_send_auth_responce(uint8 auth_status)
{
    uint8 status = (1 == auth_status) ? PPROTO_AUTH_SUCCESS : PPROTO_AUTH_FAIL;

    if(ioutils_send_uint8(PPROTO_AUTH_RESPONCE_MESSAGE_MAGIC) != 0
            || ioutils_send_uint8(status) != 0
            || ioutils_flush_send() != 0)
    {
        logger_error(_ach("pproto, failed to send auth responce message: io error"));
        return 1;
    }

    return 0;
}

sint8 pproto_send_goodbye()
{
    if(ioutils_send_uint16(PPROTO_GOODBYE_MESSAGE) != 0
            || ioutils_flush_send() != 0)
    {
        logger_error(_ach("pproto, failed to send goodbye message: io error"));
        return 1;
    }

    return 0;
}

pproto_msg_type pproto_read_msg_type()
{
    uint8 val;
    if(ioutils_get_uint8(&val) != 0)
    {
        logger_error(_ach("pproto, failed to read next message magic: io error"));
        return PPROTO_MSG_TYPE_ERR;
    }

    switch(val)
    {
        case PPROTO_SQL_REQUEST_MESSAGE_MAGIC:
            return PPROTO_SQL_REQUEST_MSG;
        case PPROTO_CANCEL_MESSAGE_MAGIC:
            return PPROTO_CANCEL_MSG;
        case PPROTO_GOODBYE_MESSAGE:
            return PPROTO_GOODBYE_MSG;
        default:
            return 0;
    }
    return 0;
}

sint8 pproto_send_server_hello()
{
    if(ioutils_send_uint16(PPROTO_SERVER_HELLO_MAGIC) != 0
            || ioutils_send_uint16(PPROTO_MAJOR_VERSION) != 0
            || ioutils_send_uint16(PPROTO_MINOR_VERSION) != 0
            || ioutils_flush_send() != 0)
    {
        logger_error(_ach("pproto, failed to server hello message: io error"));
        return 1;
    }
    return 0;
}
