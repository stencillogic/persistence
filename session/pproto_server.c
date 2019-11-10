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


/////////////// defs


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
    uint32 last_chunk_len_ptr;
    uint32 chunk_len;

    encoding client_encoding;
    encoding server_encoding;

    encoding_conversion_fun enc_client_to_srv_conversion;
    encoding_conversion_fun enc_srv_to_client_conversion;
    encoding_conversion_fun enc_utf8_to_client_conversion;
    encoding_build_char_fun enc_client_build_char;
    encoding_char_len_fun   enc_srv_char_len;

    uint8   chunk_len_left;
} g_pproto_server_state = {-1, PPROTO_SERVER_RECV_BUF_SIZE, PPROTO_SERVER_SEND_BUF_SIZE, 0u, 0u, 0u, 0u, 0u, ENCODING_UNKNOWN, ENCODING_UNKNOWN, NULL, NULL, NULL, NULL, NULL, 0u};


/////////////// functions


void pproto_server_set_sock(int client_sock)
{
    g_pproto_server_state.sock = client_sock;
}


void pproto_server_set_encoding(encoding enc)
{
    g_pproto_server_state.server_encoding = enc;
}


sint8 pproto_server_send_fully(const void *data, uint64 sz)
{
    ssize_t written;
    uint64 total_written = 0u;

    while(sz > total_written && (written = send(g_pproto_server_state.sock, data, sz - total_written, 0)) > 0) total_written += (uint64)written;
    if(written <= 0)
    {
        logger_error(_ach("pproto_server, failed to write to socket: %s"), strerror(errno));
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
    g_pproto_server_state.enc_utf8_to_client_conversion = encoding_get_conversion_fun(ENCODING_UTF8, g_pproto_server_state.client_encoding);
    g_pproto_server_state.enc_client_build_char = encoding_get_build_char_fun(g_pproto_server_state.client_encoding);
    g_pproto_server_state.enc_srv_char_len = encoding_get_char_len_fun(g_pproto_server_state.server_encoding);
}


void pproto_server_set_server_encoding(encoding enc)
{
    g_pproto_server_state.server_encoding = enc;
}


sint8 pproto_server_send(const uint8 *buf, uint32 sz)
{
    uint32 cpsz, diff, wr = 0u;

    while(sz > wr)
    {
        if(g_pproto_server_state.send_buf_ptr == g_pproto_server_state.send_buf_size)
        {
            if(pproto_server_send_fully(g_pproto_server_send_buf, g_pproto_server_state.send_buf_ptr) != 0) return 1;
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


sint8 pproto_server_send_str_begin()
{
    assert(g_pproto_server_state.client_encoding != ENCODING_UNKNOWN);
    assert(g_pproto_server_state.server_encoding != ENCODING_UNKNOWN);
    assert(g_pproto_server_state.enc_srv_to_client_conversion != NULL);
    assert(g_pproto_server_state.enc_utf8_to_client_conversion != NULL);
    assert(g_pproto_server_state.enc_srv_char_len != NULL);
    assert(g_pproto_server_state.send_buf_size > 512);

    if(0 != pproto_server_send_uint8(PPROTO_UTEXT_STRING_MAGIC)) return 1;

    // make sure there is space in the user-space buffer
    if(g_pproto_server_state.send_buf_ptr >= g_pproto_server_state.send_buf_size - 257 - ENCODING_MAXCHAR_LEN)
    {
        if(pproto_server_send_fully(g_pproto_server_send_buf, g_pproto_server_state.send_buf_ptr) != 0) return 1;
        g_pproto_server_state.send_buf_ptr = 0;
    }

    // reserve space for string chunk length
    g_pproto_server_state.last_chunk_len_ptr = g_pproto_server_state.send_buf_ptr;
    g_pproto_server_state.send_buf_ptr++;

    // current chunk length
    g_pproto_server_state.chunk_len = 0;

    return 0;
}


sint8 pproto_server_send_str(const uint8 *str_buf, sint32 sz, uint8 srv_enc)
{
    assert(str_buf != NULL);

    const_char_info server_chr;
    char_info client_chr;
    encoding_conversion_fun conv_fun = (srv_enc != 0) ? g_pproto_server_state.enc_srv_to_client_conversion : g_pproto_server_state.enc_utf8_to_client_conversion;

    client_chr.chr = g_pproto_server_send_buf + g_pproto_server_state.send_buf_ptr;  // will write directly to send buffer

    server_chr.chr = str_buf;     // will read directly from the source buffer
    server_chr.ptr = 0u;
    server_chr.state = CHAR_STATE_COMPLETE;
    server_chr.length = 0u;

    if(g_pproto_server_state.send_buf_ptr >= g_pproto_server_state.send_buf_size - 256 - ENCODING_MAXCHAR_LEN)
    {
        // this may occur if someone operates on buffer while in the middle of sending string
        logger_error(_ach("pproto_server, incorrect buffer state while sending text string"));
        return 1;
    }

    while(sz > 0)
    {
        g_pproto_server_state.enc_srv_char_len(&server_chr);
        if(server_chr.length == 0)
        {
            logger_error(_ach("pproto_server, invalid character with length 0"));
            return 1;
        }

        conv_fun(&server_chr, &client_chr);
        client_chr.chr += client_chr.length;
        g_pproto_server_state.send_buf_ptr += client_chr.length;
        g_pproto_server_state.chunk_len += client_chr.length;

        if(g_pproto_server_state.chunk_len >= 255)
        {
            // start a new chunk
            g_pproto_server_state.chunk_len -= 255;
            g_pproto_server_send_buf[g_pproto_server_state.last_chunk_len_ptr] = 255;
            g_pproto_server_state.last_chunk_len_ptr = g_pproto_server_state.send_buf_ptr - g_pproto_server_state.chunk_len;

            if(g_pproto_server_state.chunk_len > 0)
            {
                // shift overflow part to reserve space for the new chunk's length
                memcpy(g_pproto_server_send_buf + g_pproto_server_state.last_chunk_len_ptr + 1,
                        g_pproto_server_send_buf + g_pproto_server_state.last_chunk_len_ptr,
                        g_pproto_server_state.chunk_len);
            }

            g_pproto_server_state.send_buf_ptr++;
            client_chr.chr++;

            // make sure next chunk fits
            if(g_pproto_server_state.send_buf_ptr >= g_pproto_server_state.send_buf_size - 256 - ENCODING_MAXCHAR_LEN)
            {
                // next chunk may not fit, flush completed chunks
                if(pproto_server_send_fully(g_pproto_server_send_buf, g_pproto_server_state.last_chunk_len_ptr) != 0)
                {
                    return 1;
                }

                g_pproto_server_state.send_buf_ptr = g_pproto_server_state.chunk_len + 1;  // newely started chunk + it's length
                memcpy(g_pproto_server_send_buf, g_pproto_server_send_buf + g_pproto_server_state.last_chunk_len_ptr, g_pproto_server_state.send_buf_ptr);
                client_chr.chr = g_pproto_server_send_buf + g_pproto_server_state.send_buf_ptr;
                g_pproto_server_state.last_chunk_len_ptr = 0;
            }
        }

        server_chr.chr += server_chr.length;
        sz -= server_chr.length;
    }

    return 0;
}


sint8 pproto_server_send_str_end()
{
    g_pproto_server_send_buf[g_pproto_server_state.last_chunk_len_ptr] = g_pproto_server_state.chunk_len;

    if(g_pproto_server_state.chunk_len > 0)
    {
        if(0 != pproto_server_send_uint8(0)) return 1;
        g_pproto_server_state.chunk_len = 0;
    }

    return pproto_server_flush_send();
}


sint8 pproto_server_read_str_begin(uint64 *len)
{
    uint8 str_type;

    if(0 != pproto_server_get_uint8(&str_type))
    {
        return 1;
    }

    if(PPROTO_LTEXT_STRING_MAGIC == str_type)
    {
        if(0 != pproto_server_get_uint64(len))
        {
            return 1;
        }
    }

    if(0 != pproto_server_get_uint8(&g_pproto_server_state.chunk_len_left))
    {
        return 1;
    }

    return 0;
}


sint8 pproto_server_read_str(uint8 *str_buf, uint64 *sz, uint64 *charlen)
{
    uint64      wr = 0u, chrcnt = 0u;
    uint8       completed = 0u;
    char_info   server_chr;
    char_info   client_chr;
    uint8       client_chr_buf[ENCODING_MAXCHAR_LEN];

    assert(*sz >= ENCODING_MAXCHAR_LEN);
    assert(g_pproto_server_state.client_encoding != ENCODING_UNKNOWN);
    assert(g_pproto_server_state.server_encoding != ENCODING_UNKNOWN);
    assert(g_pproto_server_state.enc_client_build_char != NULL);
    assert(g_pproto_server_state.enc_client_to_srv_conversion != NULL);

    client_chr.chr = client_chr_buf;
    client_chr.ptr = 0u;
    client_chr.state = CHAR_STATE_INCOMPLETE;
    server_chr.chr = str_buf;     // will write directly in the destination buffer

    if(g_pproto_server_state.chunk_len_left == 0)
    {
        completed = 1;
    }

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
                logger_error(_ach("pproto_server, reading text string: invalid character received from client"));
                return 1;

            case CHAR_STATE_COMPLETE:
                // completed character
                g_pproto_server_state.enc_client_to_srv_conversion((const_char_info*)&client_chr, &server_chr);
                wr += server_chr.length;
                if(*(sz) - wr < ENCODING_MAXCHAR_LEN)   // next char may not fit
                {
                    completed = 1u;
                }

                chrcnt++;
                client_chr.ptr = 0u;
                client_chr.state = CHAR_STATE_INCOMPLETE;
                server_chr.chr += server_chr.length;
                break;

            case CHAR_STATE_INCOMPLETE:
            default:
                break;
        }

        g_pproto_server_state.recv_buf_ptr++;
        g_pproto_server_state.chunk_len_left--;

        if(g_pproto_server_state.chunk_len_left == 0)
        {
            if(0 != pproto_server_get_uint8(&g_pproto_server_state.chunk_len_left))
            {
                return 1;
            }

            if(g_pproto_server_state.chunk_len_left == 0)
            {
                // NOTE: in case last char is not complete it will be dropped with no error
                completed = 1u;
            }
        }
    }

    *sz = wr;
    *charlen = chrcnt;

    return 0;
}


sint8 pproto_server_read_char(char_info *ch, sint8 *eos)
{
    char_info   client_chr;
    uint8       client_chr_buf[ENCODING_MAXCHAR_LEN];
    uint8       completed = 0;
    sint8       res;

    assert(g_pproto_server_state.client_encoding != ENCODING_UNKNOWN);
    assert(g_pproto_server_state.server_encoding != ENCODING_UNKNOWN);
    assert(g_pproto_server_state.enc_client_build_char != NULL);
    assert(g_pproto_server_state.enc_client_to_srv_conversion != NULL);

    client_chr.chr = client_chr_buf;
    client_chr.ptr = 0u;
    client_chr.state = CHAR_STATE_INCOMPLETE;

    if(g_pproto_server_state.chunk_len_left == 0)
    {
        *eos = 1;
        return 0;
    }
    else
    {
        *eos = 0;
    }

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
                logger_error(_ach("pproto_server, reading text string: invalid character received from client"));
                res = 1;
                completed = 1;
                break;

            case CHAR_STATE_COMPLETE:
                // completed character
                g_pproto_server_state.enc_client_to_srv_conversion((const_char_info*)&client_chr, ch);
                res = 0;
                completed = 1;
                break;

            case CHAR_STATE_INCOMPLETE:
            default:
                break;
        }

        g_pproto_server_state.recv_buf_ptr++;
        g_pproto_server_state.chunk_len_left--;

        if(g_pproto_server_state.chunk_len_left == 0)
        {
            if(0 != pproto_server_get_uint8(&g_pproto_server_state.chunk_len_left))
            {
                return 1;
            }

            if(g_pproto_server_state.chunk_len_left == 0)
            {
                // NOTE: if last char is not complete it will be dropped with no error message
                *eos = 1;
                return 0;
            }
        }
    }

    return res;
}


sint8 pproto_server_read_str_end()
{
    uint8 strbuf[255];

    if(g_pproto_server_state.chunk_len_left > 0)
    {
        if(pproto_server_send_uint8(PPROTO_CANCEL_MESSAGE_MAGIC) != 0) return 1;
        if(pproto_server_flush_send() != 0) return 1;

        do
        {
            if(0 != pproto_server_get(strbuf, g_pproto_server_state.chunk_len_left)) return 1;
            if(0 != pproto_server_get_uint8(&g_pproto_server_state.chunk_len_left)) return 1;
        }
        while(g_pproto_server_state.chunk_len_left != 0);
    }

    return 0;
}


sint8 pproto_server_read_auth(auth_credentials *cred)
{
    uint8 user_name[AUTH_USER_NAME_SZ + ENCODING_MAXCHAR_LEN];
    uint64 sz = sizeof(user_name);
    uint64 charlen;
    uint64 strlen = 0;

    if(pproto_server_read_str_begin(&strlen) != 0
        || pproto_server_read_str(user_name, &sz, &charlen) != 0
        || pproto_server_read_str_end() != 0)
    {
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

    if(pproto_server_get(cred->credentials, AUTH_CREDENTIAL_SZ * sizeof(uint8)) != 0)
    {
        return 1;
    }

    return 0;
}


sint8 pproto_server_send_error(error_code errcode, const achar* msg)
{
    error_set(errcode);
    if(pproto_server_send_str_begin() != 0
            || pproto_server_send_str((uint8 *)error_msg(), strlen(error_msg()), 0) != 0)
    {
        return 1;
    }

    if(msg != NULL)
    {
        if(pproto_server_send_str((uint8 *)_ach(": "), strlen(_ach(": ")), 0) != 0
                || pproto_server_send_str((uint8 *)msg, strlen(msg), 0) != 0)
        {
            return 1;
        }
    }

    if(0 != pproto_server_send_str_end() != 0)
    {
        return 1;
    }

    return 0;
}


sint8 pproto_server_read_client_hello(encoding *client_encoding)
{
    uint16 val;

    if(pproto_server_get_uint16(&val) != 0)
    {
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
        return 1;
    }

    return 0;
}


sint8 pproto_server_send_goodbye()
{
    if(pproto_server_send_uint8(PPROTO_GOODBYE_MESSAGE) != 0
            || pproto_server_flush_send() != 0)
    {
        return 1;
    }

    return 0;
}


pproto_msg_type pproto_server_read_msg_type()
{
    uint8 val;
    if(pproto_server_get_uint8(&val) != 0)
    {
        return PPROTO_MSG_TYPE_ERR;
    }

    if(val == (uint8)(PPROTO_CLIENT_HELLO_MAGIC >> 8))
    {
        if(pproto_server_get_uint8(&val) != 0)
        {
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
        return 1;
    }
    return 0;
}
