#include "session/pproto_server.h"
#include "common/ioutils.h"
#include "logging/logger.h"
#include <string.h>


void pproto_server_set_sock(int client_sock)
{
    ioutils_set_sock(client_sock);
}

void pproto_server_set_encoding(encoding client_encoding)
{
    ioutils_set_client_encoding(client_encoding);
}

sint8 pproto_server_read_text_string(uint8 *buf, uint32 *sz, uint32 *charlen)
{
    uint8 magic, is_nt;
    uint32 len;

    if(ioutils_get_uint8(&magic))
    {
        logger_error(_ach("pproto_server, failed to read text string: io error"));
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
            logger_error(_ach("pproto_server, reading limited text string: io error"));
            return 1;
        }

        if(len > *sz)
        {
            logger_error(_ach("pproto_server, reading limited text string: string size of %u bytes is \
                        greater than expected size of %u bytes"), len, *sz);
            return 1;
        }
    }
    else
    {
        logger_error(_ach("pproto_server, failed to read text string: invalid magic"));
        return 1;
    }

    if(ioutils_get_str(buf, &len, charlen, is_nt) != 0)
    {
        logger_error(_ach("pproto_server, reading text string: io error"));
        return 1;
    }

    return 0;
}

sint8 pproto_server_read_str_begin(uint64 *len, uint8 next_chunk_len)
{
    uint8 str_type;

    if(0 != ioutils_get_uint8(&str_type))
    {
        logger_error(_ach("pproto_server, reading text string type: io error"));
        return 1;
    }

    if(PPROTO_LTEXT_STRING_MAGIC == str_type)
    {
        if(0 != ioutils_get_uint64(len))
        {
            logger_error(_ach("pproto_server, reading text string length: io error"));
            return 1;
        }
    }

    if(0 != ioutils_get_uint8(&next_chunk_len))
    {
        logger_error(_ach("pproto_server, reading text string chunk length: io error"));
        return 1;
    }

    return 0;
}



sint8 pproto_server_read_str(uint8 *strbuf, uint64 *sz)
{
    pproto_client_state *state = (pproto_client_state *)ss;
    uint64 read = 0;

    if(state->next_chunk_len == 0)
    {
        *sz = 0;
        return 0;
    }

    if((*sz) < (uint64)state->next_chunk_len)
    {
        errno = ENOBUFS;
        return 1;
    }

    do
    {
        if(0 != pproto_client_get(ss, strbuf + read, state->next_chunk_len)) return 1;

        read += state->next_chunk_len;

        if(pproto_client_get(ss, &state->next_chunk_len, sizeof(state->next_chunk_len)) != 0) return 1;
    }
    while(state->next_chunk_len != 0 && state->next_chunk_len + read <= (*sz));

    (*sz) = read;

    return 0;
}



sint8 pproto_client_read_str_end(handle ss)
{
    pproto_client_state *state = (pproto_client_state *)ss;
    uint8 magic = PPROTO_CANCEL_MESSAGE_MAGIC;
    uint8 strbuf[255];
    uint64 sz;

    if(state->next_chunk_len != 0)
    {
        if(pproto_client_send(ss, &magic, sizeof(magic)) != 0) return 1;
        if(pproto_client_flush_send(ss) != 0) return 1;

        do
        {
            sz = 255;
            if(0 != pproto_client_read_str(ss, strbuf, &sz)) return 1;
        }
        while(sz != 0);
    }
    return 0;
}

sint8 pproto_server_read_auth(auth_credentials *cred)
{
    uint8 user_name[AUTH_USER_NAME_SZ + ENCODING_MAXCHAR_LEN];
    uint32 sz = sizeof(user_name), charlen;

    if(pproto_server_read_text_string(user_name, &sz, &charlen) != 0)
    {
        logger_error(_ach("pproto_server, failed to read auth message user name: io error"));
        return 1;
    }

    if(sz > AUTH_USER_NAME_SZ || 0 == sz)
    {
        logger_error(_ach("pproto_server, auth message user name is invalid: io error"));
        return 1;
    }
    else
    {
        memcpy(cred->user_name, user_name, sz);
    }

    if(ioutils_get(cred->credentials, 64) != 0)
    {
        logger_error(_ach("pproto_server, reading auth message credentials: io error"));
        return 1;
    }

    return 0;
}

sint8 pproto_server_send_error(error_code errcode)
{
    error_set(errcode);
    if(ioutils_send_str((uint8 *)error_msg(), 1) != 0
            || ioutils_flush_send() != 0)
    {
        logger_error(_ach("pproto_server, failed to send error message: io error"));
        return 1;
    }

    return 0;
}

sint8 pproto_server_read_client_hello(encoding *client_encoding)
{
    uint16 val;

    if(ioutils_get_uint16(&val) != 0)
    {
        logger_error(_ach("pproto_server, failed to read client hello message: io error"));
        return 1;
    }

    *client_encoding = val;

    return 0;
}

sint8 pproto_server_send_auth_request()
{
    if(ioutils_send_uint8(PPROTO_AUTH_REQUEST_MESSAGE_MAGIC) != 0
            || ioutils_flush_send() != 0)
    {
        logger_error(_ach("pproto_server, failed to send auth request message: io error"));
        return 1;
    }

    return 0;
}

sint8 pproto_server_send_auth_responce(uint8 auth_status)
{
    uint8 status = (1 == auth_status) ? PPROTO_AUTH_SUCCESS : PPROTO_AUTH_FAIL;

    if(ioutils_send_uint8(PPROTO_AUTH_RESPONCE_MESSAGE_MAGIC) != 0
            || ioutils_send_uint8(status) != 0
            || ioutils_flush_send() != 0)
    {
        logger_error(_ach("pproto_server, failed to send auth responce message: io error"));
        return 1;
    }

    return 0;
}

sint8 pproto_server_send_goodbye()
{
    if(ioutils_send_uint8(PPROTO_GOODBYE_MESSAGE) != 0
            || ioutils_flush_send() != 0)
    {
        logger_error(_ach("pproto_server, failed to send goodbye message: io error"));
        return 1;
    }

    return 0;
}

pproto_msg_type pproto_server_read_msg_type()
{
    uint8 val;
    if(ioutils_get_uint8(&val) != 0)
    {
        logger_error(_ach("pproto_server, failed to read next message magic: io error"));
        return PPROTO_MSG_TYPE_ERR;
    }

    if(val == (uint8)(PPROTO_CLIENT_HELLO_MAGIC >> 8))
    {
        if(ioutils_get_uint8(&val) != 0)
        {
            logger_error(_ach("pproto_server, failed to read next message magic: io error"));
            return PPROTO_MSG_TYPE_ERR;
        }

        if(val == (uint8)PPROTO_CLIENT_HELLO_MAGIC)
        {
            return PPROTO_CLIENT_HELLO_MSG;
        }
    }

    switch(val)
    {
        case PPROTO_SQL_REQUEST_MESSAGE_MAGIC:
            return PPROTO_SQL_REQUEST_MSG;
        case PPROTO_CANCEL_MESSAGE_MAGIC:
            return PPROTO_CANCEL_MSG;
        case PPROTO_GOODBYE_MESSAGE:
            return PPROTO_GOODBYE_MSG;
        case PPROTO_ERROR_MSG_MAGIC:
            return PPROTO_ERROR_MSG;
        case PPROTO_SUCCESS_MESSAGE_WITH_TEXT_MAGIC:
            return PPROTO_SUCCESS_WITH_TEXT_MSG;
        case PPROTO_SUCCESS_MESSAGE_WITHOUT_TEXT_MAGIC:
            return PPROTO_SUCCESS_WITHOUT_TEXT_MSG;
        case PPROTO_RECORDSET_MESSAGE_MAGIC:
            return PPROTO_RECORDSET_MSG;
        case PPROTO_AUTH_REQUEST_MESSAGE_MAGIC:
            return PPROTO_AUTH_REQUEST_MSG;
        case PPROTO_AUTH_RESPONCE_MESSAGE_MAGIC:
            return PPROTO_AUTH_RESPONCE_MSG;
        case PPROTO_PROGRESS_MESSAGE_MAGIC:
            return PPROTO_PROGRESS_MSG;
        case PPROTO_AUTH_MESSAGE_MAGIC:
            return PPROTO_AUTH_MSG;
        default:
            return PPROTO_UNKNOWN_MSG;
    }

    return PPROTO_UNKNOWN_MSG;
}

sint8 pproto_server_send_server_hello()
{
    if(ioutils_send_uint16(PPROTO_SERVER_HELLO_MAGIC) != 0
            || ioutils_send_uint16(PPROTO_MAJOR_VERSION) != 0
            || ioutils_send_uint16(PPROTO_MINOR_VERSION) != 0
            || ioutils_flush_send() != 0)
    {
        logger_error(_ach("pproto_server, failed to send server hello message: io error"));
        return 1;
    }
    return 0;
}
