#include "session/pproto_server.h"
#include "logging/logger.h"
#include <stddef.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <endian.h>
#include <assert.h>


#define PPROTO_SERVER_RECV_BUF_SIZE 8192u
#define PPROTO_SERVER_SEND_BUF_SIZE 8192u

uint8 g_pproto_server_recv_buf[PPROTO_SERVER_RECV_BUF_SIZE];
uint8 g_pproto_server_send_buf[PPROTO_SERVER_SEND_BUF_SIZE];

struct {
    int sock;

    const uint32 recv_buf_size;
    const uint32 send_buf_size;

    uint32 recv_buf_ptr;
    uint32 recv_buf_upper_bound;
    uint32 send_buf_ptr;

    encoding client_encoding;
    encoding server_encoding;

    encoding_conversion_fun enc_client_to_srv_conversion;
    encoding_conversion_fun enc_srv_to_client_conversion;
    encoding_build_char_fun enc_client_build_char;
    encoding_char_len_fun   enc_srv_char_len;
} g_pproto_server_state = {-1, PPROTO_SERVER_RECV_BUF_SIZE, PPROTO_SERVER_SEND_BUF_SIZE, 0u, 0u, 0u, ENCODING_UNKNOWN, ENCODING_UNKNOWN, NULL, NULL, NULL, NULL};


void pproto_server_set_sock(int client_sock)
{
    g_pproto_server_state.sock = client_sock;
}

void pproto_server_set_encoding(encoding client_encoding)
{
    g_pproto_server_state.client_encoding = client_encoding;
}

sint8 pproto_server_send_fully(const void *data, uint64 sz)
{
    ssize_t written;
    uint64 total_written = 0u;

    while(sz > total_written && (written = send(g_pproto_server_state.sock, data, sz - total_written, 0)) > 0) total_written += (uint64)written;
    if(written <= 0)
    {
        return 1;
    }
    return 0;
}

sint8 pproto_server_read_portion()
{
    ssize_t readsz;
    uint32 leftsz = g_pproto_server_state.recv_buf_size - g_pproto_server_state.recv_buf_upper_bound;
    if(0u == leftsz)
    {
        leftsz = g_pproto_server_state.recv_buf_size;
        g_pproto_server_state.recv_buf_ptr = 0u;
        g_pproto_server_state.recv_buf_upper_bound = 0u;
    }

    readsz = recv(g_pproto_server_state.sock, g_pproto_server_recv_buf + g_pproto_server_state.recv_buf_upper_bound, leftsz, 0);
    if(readsz < 0)
    {
        logger_error(_ach("pproto_server, failed to read from socket: %s"), strerror(errno));
        return 1;
    }
    if(0 == readsz)
    {
        logger_error(_ach("pproto_server, connection was shut down: %s"), strerror(errno));
        return 1;
    }
    g_pproto_server_state.recv_buf_upper_bound += (uint32)readsz;

    return 0;
}

sint8 pproto_server_flush_send()
{
    sint8 res = pproto_server_send_fully(g_pproto_server_send_buf, g_pproto_server_state.send_buf_ptr);
    if(0 == res)
    {
        g_pproto_server_state.send_buf_ptr = 0u;
    }
    return res;
}

sint8 pproto_server_get_uint8(uint8 *val)
{
    if(g_pproto_server_state.recv_buf_upper_bound == g_pproto_server_state.recv_buf_ptr)
    {
        if(pproto_server_read_portion() != 0) return 1;
    }

    *val = g_pproto_server_recv_buf[g_pproto_server_state.recv_buf_ptr++];

    return 0;
}

sint8 pproto_server_get(uint8 *buf, uint32 sz)
{
    uint32 cpsz, diff;

    while(sz > 0u)
    {
        diff = g_pproto_server_state.recv_buf_upper_bound - g_pproto_server_state.recv_buf_ptr;
        if(diff > 0u)
        {
            cpsz = diff > sz ? sz : diff;
            memcpy(buf, g_pproto_server_recv_buf + g_pproto_server_state.recv_buf_ptr, cpsz);
            buf += cpsz;
            g_pproto_server_state.recv_buf_ptr += cpsz;
            sz -= cpsz;
        }
        else
        {
            if(pproto_server_read_portion() != 0) return 1;
        }
    }
    return 0;
}

sint8 pproto_server_get_uint16(uint16 *val)
{
    if(pproto_server_get((void*)val, 2) != 0) return 1;
    *val = be16toh(*val);
    return 0;
}

sint8 pproto_server_get_uint32(uint32 *val)
{
    if(pproto_server_get((void*)val, 4) != 0) return 1;
    *val = be32toh(*val);
    return 0;
}

sint8 pproto_server_get_uint64(uint64 *val)
{
    if(pproto_server_get((void*)val, 8) != 0) return 1;
    *val = be64toh(*val);
    return 0;
}

void pproto_server_set_client_encoding(encoding enc)
{
    assert(ENCODING_UNKNOWN != g_pproto_server_state.server_encoding);
    assert(encoding_is_convertable(g_pproto_server_state.server_encoding, enc));

    g_pproto_server_state.client_encoding = enc;

    g_pproto_server_state.enc_client_to_srv_conversion = encoding_get_conversion_fun(g_pproto_server_state.client_encoding, g_pproto_server_state.server_encoding);
    g_pproto_server_state.enc_srv_to_client_conversion = encoding_get_conversion_fun(g_pproto_server_state.server_encoding, g_pproto_server_state.client_encoding);
    g_pproto_server_state.enc_client_build_char = encoding_get_build_char_fun(g_pproto_server_state.client_encoding);
    g_pproto_server_state.enc_srv_char_len = encoding_get_char_len_fun(g_pproto_server_state.server_encoding);
}

void pproto_server_set_server_encoding(encoding enc)
{
    g_pproto_server_state.server_encoding = enc;
}

sint8 pproto_server_get_str(uint8 *str_buf, uint32 *sz, uint32 *charlen, uint8 is_nt)
{
    uint32 wr = 0u, chrcnt = 0u;
    uint8 completed = 0u;
    char_info server_chr;
    char_info client_chr;
    uint8 client_chr_buf[ENCODING_MAXCHAR_LEN];

    assert(*sz >= 2*ENCODING_MAXCHAR_LEN);
    assert(g_pproto_server_state.client_encoding != ENCODING_UNKNOWN);
    assert(g_pproto_server_state.server_encoding != ENCODING_UNKNOWN);
    assert(g_pproto_server_state.enc_client_build_char != NULL);
    assert(g_pproto_server_state.enc_client_to_srv_conversion != NULL);

    client_chr.chr = client_chr_buf;
    client_chr.ptr = 0u;
    client_chr.state = CHAR_STATE_INCOMPLETE;
    server_chr.chr = str_buf;     // will write directly in the destination buffer

    while(!completed)
    {
        if(g_pproto_server_state.recv_buf_upper_bound == g_pproto_server_state.recv_buf_ptr)
        {
            if(pproto_server_read_portion() != 0) return 1;
        }

        g_pproto_server_state.enc_client_build_char(&client_chr, g_pproto_server_recv_buf[g_pproto_server_state.recv_buf_ptr]);
        switch(client_chr.state)
        {
            case CHAR_STATE_INVALID:
                // invalid character
                return 1;

            case CHAR_STATE_COMPLETE:
                // completed character
                g_pproto_server_state.enc_client_to_srv_conversion((const_char_info*)&client_chr, &server_chr);
                wr += server_chr.length;
                if(*(sz) - wr < ENCODING_MAXCHAR_LEN)   // next char may not fit
                {
                    completed = 1u;
                }
                else
                {
                    if(is_nt > 0u && encoding_is_string_terminator((const_char_info*)&server_chr, g_pproto_server_state.server_encoding))
                    {
                        completed = 1u;
                    }
                    else
                    {
                        chrcnt++;
                    }
                }

                client_chr.ptr = 0u;
                client_chr.state = CHAR_STATE_INCOMPLETE;
                server_chr.chr += server_chr.length;
                break;
            case CHAR_STATE_INCOMPLETE:
                break;
            default:
                break;
        }

        g_pproto_server_state.recv_buf_ptr++;
    }
    *sz = wr;
    *charlen = chrcnt;

    return 0;
}

sint8 pproto_server_send_str(const uint8 *str_buf, uint8 is_nt)
{
    assert(g_pproto_server_state.client_encoding != ENCODING_UNKNOWN);
    assert(g_pproto_server_state.server_encoding != ENCODING_UNKNOWN);
    assert(g_pproto_server_state.enc_srv_to_client_conversion != NULL);
    assert(g_pproto_server_state.enc_srv_char_len != NULL);
    assert(str_buf != NULL);

    const_char_info server_chr;
    char_info client_chr;

    client_chr.chr = g_pproto_server_send_buf + g_pproto_server_state.send_buf_ptr;  // will write directly to send buffer

    server_chr.chr = str_buf;     // will read directly from the source buffer
    server_chr.ptr = 0u;
    server_chr.state = CHAR_STATE_COMPLETE;
    server_chr.length = 0u;

    if(0 != is_nt)
    {
        do
        {
            server_chr.chr += server_chr.length;
            g_pproto_server_state.enc_srv_char_len(&server_chr);

            if(g_pproto_server_state.send_buf_ptr >= g_pproto_server_state.send_buf_size - ENCODING_MAXCHAR_LEN)
            {
                if(pproto_server_send_fully(g_pproto_server_send_buf, g_pproto_server_state.send_buf_ptr) != 0)
                {
                    logger_error(_ach("pproto_server, failed to send data to client: %s"), strerror(errno));
                    return 1;
                }
                g_pproto_server_state.send_buf_ptr = 0;
                client_chr.chr = g_pproto_server_send_buf;
            }
            g_pproto_server_state.enc_srv_to_client_conversion(&server_chr, &client_chr);
            client_chr.chr += client_chr.length;
            g_pproto_server_state.send_buf_ptr += client_chr.length;
        }
        while(0 == encoding_is_string_terminator(&server_chr, g_pproto_server_state.server_encoding));
    }
    else
    {
        g_pproto_server_state.enc_srv_char_len(&server_chr);
        while(0 == encoding_is_string_terminator(&server_chr, g_pproto_server_state.server_encoding))
        {
            if(g_pproto_server_state.send_buf_ptr >= g_pproto_server_state.send_buf_size - ENCODING_MAXCHAR_LEN)
            {
                if(pproto_server_send_fully(g_pproto_server_send_buf, g_pproto_server_state.send_buf_ptr) != 0)
                {
                    logger_error(_ach("pproto_server, failed to send data to client: %s"), strerror(errno));
                    return 1;
                }
                g_pproto_server_state.send_buf_ptr = 0;
                client_chr.chr = g_pproto_server_send_buf;
            }
            g_pproto_server_state.enc_srv_to_client_conversion(&server_chr, &client_chr);
            client_chr.chr += client_chr.length;
            g_pproto_server_state.send_buf_ptr += client_chr.length;

            server_chr.chr += server_chr.length;
            g_pproto_server_state.enc_srv_char_len(&server_chr);
        }
    }

    return 0;
}

sint8 pproto_server_send(const uint8 *buf, uint32 sz)
{
    uint32 cpsz, diff, wr = 0u;

    while(sz > wr)
    {
        if(g_pproto_server_state.send_buf_ptr == g_pproto_server_state.send_buf_size)
        {
            if(pproto_server_send_fully(g_pproto_server_send_buf, g_pproto_server_state.send_buf_ptr) != 0)
            {
                logger_error(_ach("pproto_server, failed to send data to client: %s"), strerror(errno));
                return 1;
            }
            g_pproto_server_state.send_buf_ptr = 0;
        }

        diff = g_pproto_server_state.send_buf_size - g_pproto_server_state.send_buf_ptr;
        cpsz = (diff > sz) ? sz : diff;
        memcpy(g_pproto_server_send_buf + g_pproto_server_state.send_buf_ptr, buf + wr, cpsz);
        wr += cpsz;
        g_pproto_server_state.send_buf_ptr += cpsz;
    }
    return 0;
}

sint8 pproto_server_send_uint8(uint8 val)
{
   return pproto_server_send(&val, sizeof(uint8));
}

sint8 pproto_server_send_uint16(uint16 val)
{
   val = htobe16(val);
   return pproto_server_send((uint8 *)&val, sizeof(uint16));
}

sint8 pproto_server_send_uint32(uint32 val)
{
   val = htobe32(val);
   return pproto_server_send((uint8 *)&val, sizeof(uint32));
}

sint8 pproto_server_send_uint64(uint64 val)
{
   val = htobe64(val);
   return pproto_server_send((uint8 *)&val, sizeof(uint64));
}

sint8 pproto_server_read_text_string(uint8 *buf, uint32 *sz, uint32 *charlen)
{
    uint8 magic, is_nt;
    uint32 len;

    if(pproto_server_get_uint8(&magic))
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
        if(pproto_server_get_uint32(&len) != 0)
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

    if(pproto_server_get_str(buf, &len, charlen, is_nt) != 0)
    {
        logger_error(_ach("pproto_server, reading text string: io error"));
        return 1;
    }

    return 0;
}

sint8 pproto_server_read_str_begin(uint64 *len, uint8 next_chunk_len)
{
    uint8 str_type;

    if(0 != pproto_server_get_uint8(&str_type))
    {
        logger_error(_ach("pproto_server, reading text string type: io error"));
        return 1;
    }

    if(PPROTO_LTEXT_STRING_MAGIC == str_type)
    {
        if(0 != pproto_server_get_uint64(len))
        {
            logger_error(_ach("pproto_server, reading text string length: io error"));
            return 1;
        }
    }

    if(0 != pproto_server_get_uint8(&next_chunk_len))
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

    if(pproto_server_get(cred->credentials, 64) != 0)
    {
        logger_error(_ach("pproto_server, reading auth message credentials: io error"));
        return 1;
    }

    return 0;
}

sint8 pproto_server_send_error(error_code errcode)
{
    error_set(errcode);
    if(pproto_server_send_str((uint8 *)error_msg(), 1) != 0
            || pproto_server_flush_send() != 0)
    {
        logger_error(_ach("pproto_server, failed to send error message: io error"));
        return 1;
    }

    return 0;
}

sint8 pproto_server_read_client_hello(encoding *client_encoding)
{
    uint16 val;

    if(pproto_server_get_uint16(&val) != 0)
    {
        logger_error(_ach("pproto_server, failed to read client hello message: io error"));
        return 1;
    }

    *client_encoding = val;

    return 0;
}

sint8 pproto_server_send_auth_request()
{
    if(pproto_server_send_uint8(PPROTO_AUTH_REQUEST_MESSAGE_MAGIC) != 0
            || pproto_server_flush_send() != 0)
    {
        logger_error(_ach("pproto_server, failed to send auth request message: io error"));
        return 1;
    }

    return 0;
}

sint8 pproto_server_send_auth_responce(uint8 auth_status)
{
    uint8 status = (1 == auth_status) ? PPROTO_AUTH_SUCCESS : PPROTO_AUTH_FAIL;

    if(pproto_server_send_uint8(PPROTO_AUTH_RESPONCE_MESSAGE_MAGIC) != 0
            || pproto_server_send_uint8(status) != 0
            || pproto_server_flush_send() != 0)
    {
        logger_error(_ach("pproto_server, failed to send auth responce message: io error"));
        return 1;
    }

    return 0;
}

sint8 pproto_server_send_goodbye()
{
    if(pproto_server_send_uint8(PPROTO_GOODBYE_MESSAGE) != 0
            || pproto_server_flush_send() != 0)
    {
        logger_error(_ach("pproto_server, failed to send goodbye message: io error"));
        return 1;
    }

    return 0;
}

pproto_msg_type pproto_server_read_msg_type()
{
    uint8 val;
    if(pproto_server_get_uint8(&val) != 0)
    {
        logger_error(_ach("pproto_server, failed to read next message magic: io error"));
        return PPROTO_MSG_TYPE_ERR;
    }

    if(val == (uint8)(PPROTO_CLIENT_HELLO_MAGIC >> 8))
    {
        if(pproto_server_get_uint8(&val) != 0)
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
    if(pproto_server_send_uint16(PPROTO_SERVER_HELLO_MAGIC) != 0
            || pproto_server_send_uint16(PPROTO_MAJOR_VERSION) != 0
            || pproto_server_send_uint16(PPROTO_MINOR_VERSION) != 0
            || pproto_server_flush_send() != 0)
    {
        logger_error(_ach("pproto_server, failed to send server hello message: io error"));
        return 1;
    }
    return 0;
}
