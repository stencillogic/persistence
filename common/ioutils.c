#include "common/ioutils.h"
#include "logging/logger.h"
#include <stddef.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <endian.h>
#include <assert.h>

#define IOUTILS_RECV_BUF_SIZE 8192u
#define IOUTILS_SEND_BUF_SIZE 8192u

uint8 g_ioutils_recv_buf[IOUTILS_RECV_BUF_SIZE];
uint8 g_ioutils_send_buf[IOUTILS_SEND_BUF_SIZE];

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
} g_ioutils_state = {-1, IOUTILS_RECV_BUF_SIZE, IOUTILS_SEND_BUF_SIZE, 0u, 0u, 0u, ENCODING_UNKNOWN, ENCODING_UNKNOWN, NULL, NULL, NULL, NULL};

void ioutils_set_sock(int sock)
{
    g_ioutils_state.sock = sock;
}

sint8 ioutils_send_fully(const void *data, uint64 sz)
{
    ssize_t written;
    uint64 total_written = 0u;

    while(sz > total_written && (written = send(g_ioutils_state.sock, data, sz - total_written, 0)) > 0) total_written += (uint64)written;
    if(written <= 0)
    {
        return 1;
    }
    return 0;
}

sint8 ioutils_write_fully(int fd, const void *data, uint64 sz)
{
    ssize_t written;
    uint64 total_written = 0u;

    while(sz > total_written && (written = write(fd, data, sz - total_written)) > 0) total_written += (uint64)written;
    if(written <= 0)
    {
        return 1;
    }
    return 0;
}

sint8 ioutils_read_portion()
{
    ssize_t readsz;
    uint32 leftsz = g_ioutils_state.recv_buf_size - g_ioutils_state.recv_buf_upper_bound;
    if(0u == leftsz)
    {
        leftsz = g_ioutils_state.recv_buf_size;
        g_ioutils_state.recv_buf_ptr = 0u;
        g_ioutils_state.recv_buf_upper_bound = 0u;
    }

    readsz = recv(g_ioutils_state.sock, g_ioutils_recv_buf + g_ioutils_state.recv_buf_upper_bound, leftsz, 0);
    if(readsz < 0)
    {
        logger_error(_ach("ioutils, failed to read from socket: %s"), strerror(errno));
        return 1;
    }
    if(0 == readsz)
    {
        logger_error(_ach("ioutils, connection was shut down: %s"), strerror(errno));
        return 1;
    }
    g_ioutils_state.recv_buf_upper_bound += (uint32)readsz;

    return 0;
}

sint8 ioutils_flush_send()
{
    sint8 res = ioutils_send_fully(g_ioutils_send_buf, g_ioutils_state.send_buf_ptr);
    if(0 == res)
    {
        g_ioutils_state.send_buf_ptr = 0u;
    }
    return res;
}

sint8 ioutils_get_uint8(uint8 *val)
{
    if(g_ioutils_state.recv_buf_upper_bound == g_ioutils_state.recv_buf_ptr)
    {
        if(ioutils_read_portion() != 0) return 1;
    }

    *val = g_ioutils_recv_buf[g_ioutils_state.recv_buf_ptr++];

    return 0;
}

sint8 ioutils_get(uint8 *buf, uint32 sz)
{
    uint32 cpsz, diff;

    while(sz > 0u)
    {
        diff = g_ioutils_state.recv_buf_upper_bound - g_ioutils_state.recv_buf_ptr;
        if(diff > 0u)
        {
            cpsz = diff > sz ? sz : diff;
            memcpy(buf, g_ioutils_recv_buf + g_ioutils_state.recv_buf_ptr, cpsz);
            buf += cpsz;
            g_ioutils_state.recv_buf_ptr += cpsz;
            sz -= cpsz;
        }
        else
        {
            if(ioutils_read_portion() != 0) return 1;
        }
    }
    return 0;
}

sint8 ioutils_get_uint16(uint16 *val)
{
    if(ioutils_get((void*)val, 2) != 0) return 1;
    *val = be16toh(*val);
    return 0;
}

sint8 ioutils_get_uint32(uint32 *val)
{
    if(ioutils_get((void*)val, 4) != 0) return 1;
    *val = be32toh(*val);
    return 0;
}

sint8 ioutils_get_uint64(uint64 *val)
{
    if(ioutils_get((void*)val, 8) != 0) return 1;
    *val = be64toh(*val);
    return 0;
}

void ioutils_set_client_encoding(encoding enc)
{
    assert(ENCODING_UNKNOWN != g_ioutils_state.server_encoding);
    assert(encoding_is_convertable(g_ioutils_state.server_encoding, enc));

    g_ioutils_state.client_encoding = enc;

    g_ioutils_state.enc_client_to_srv_conversion = encoding_get_conversion_fun(g_ioutils_state.client_encoding, g_ioutils_state.server_encoding);
    g_ioutils_state.enc_srv_to_client_conversion = encoding_get_conversion_fun(g_ioutils_state.server_encoding, g_ioutils_state.client_encoding);
    g_ioutils_state.enc_client_build_char = encoding_get_build_char_fun(g_ioutils_state.client_encoding);
    g_ioutils_state.enc_srv_char_len = encoding_get_char_len_fun(g_ioutils_state.server_encoding);
}

void ioutils_set_server_encoding(encoding enc)
{
    g_ioutils_state.server_encoding = enc;
}

sint8 ioutils_get_str(uint8 *str_buf, uint32 *sz, uint32 *charlen, uint8 is_nt)
{
    uint32 wr = 0u, chrcnt = 0u;
    uint8 completed = 0u;
    char_info server_chr;
    char_info client_chr;
    uint8 client_chr_buf[ENCODING_MAXCHAR_LEN];

    assert(*sz >= 2*ENCODING_MAXCHAR_LEN);
    assert(g_ioutils_state.client_encoding != ENCODING_UNKNOWN);
    assert(g_ioutils_state.server_encoding != ENCODING_UNKNOWN);
    assert(g_ioutils_state.enc_client_build_char != NULL);
    assert(g_ioutils_state.enc_client_to_srv_conversion != NULL);

    client_chr.chr = client_chr_buf;
    client_chr.ptr = 0u;
    client_chr.state = CHAR_STATE_INCOMPLETE;
    server_chr.chr = str_buf;     // will write directly in the destination buffer

    while(!completed)
    {
        if(g_ioutils_state.recv_buf_upper_bound == g_ioutils_state.recv_buf_ptr)
        {
            if(ioutils_read_portion() != 0) return 1;
        }

        g_ioutils_state.enc_client_build_char(&client_chr, g_ioutils_recv_buf[g_ioutils_state.recv_buf_ptr]);
        switch(client_chr.state)
        {
            case CHAR_STATE_INVALID:
                // invalid character
                return 1;

            case CHAR_STATE_COMPLETE:
                // completed character
                g_ioutils_state.enc_client_to_srv_conversion((const_char_info*)&client_chr, &server_chr);
                wr += server_chr.length;
                if(*(sz) - wr < ENCODING_MAXCHAR_LEN)   // next char may not fit
                {
                    completed = 1u;
                }
                else
                {
                    if(is_nt > 0u && encoding_is_string_terminator((const_char_info*)&server_chr, g_ioutils_state.server_encoding))
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

        g_ioutils_state.recv_buf_ptr++;
    }
    *sz = wr;
    *charlen = chrcnt;

    return 0;
}

sint8 ioutils_send_str(const uint8 *str_buf, uint8 is_nt)
{
    assert(g_ioutils_state.client_encoding != ENCODING_UNKNOWN);
    assert(g_ioutils_state.server_encoding != ENCODING_UNKNOWN);
    assert(g_ioutils_state.enc_srv_to_client_conversion != NULL);
    assert(g_ioutils_state.enc_srv_char_len != NULL);
    assert(str_buf != NULL);

    const_char_info server_chr;
    char_info client_chr;

    client_chr.chr = g_ioutils_send_buf + g_ioutils_state.send_buf_ptr;  // will write directly to send buffer

    server_chr.chr = str_buf;     // will read directly from the source buffer
    server_chr.ptr = 0u;
    server_chr.state = CHAR_STATE_COMPLETE;
    server_chr.length = 0u;

    if(0 != is_nt)
    {
        do
        {
            server_chr.chr += server_chr.length;
            g_ioutils_state.enc_srv_char_len(&server_chr);

            if(g_ioutils_state.send_buf_ptr >= g_ioutils_state.send_buf_size - ENCODING_MAXCHAR_LEN)
            {
                if(ioutils_send_fully(g_ioutils_send_buf, g_ioutils_state.send_buf_ptr) != 0)
                {
                    logger_error(_ach("ioutils, failed to send data to client: %s"), strerror(errno));
                    return 1;
                }
                g_ioutils_state.send_buf_ptr = 0;
                client_chr.chr = g_ioutils_send_buf;
            }
            g_ioutils_state.enc_srv_to_client_conversion(&server_chr, &client_chr);
            client_chr.chr += client_chr.length;
            g_ioutils_state.send_buf_ptr += client_chr.length;
        }
        while(0 == encoding_is_string_terminator(&server_chr, g_ioutils_state.server_encoding));
    }
    else
    {
        g_ioutils_state.enc_srv_char_len(&server_chr);
        while(0 == encoding_is_string_terminator(&server_chr, g_ioutils_state.server_encoding))
        {
            if(g_ioutils_state.send_buf_ptr >= g_ioutils_state.send_buf_size - ENCODING_MAXCHAR_LEN)
            {
                if(ioutils_send_fully(g_ioutils_send_buf, g_ioutils_state.send_buf_ptr) != 0)
                {
                    logger_error(_ach("ioutils, failed to send data to client: %s"), strerror(errno));
                    return 1;
                }
                g_ioutils_state.send_buf_ptr = 0;
                client_chr.chr = g_ioutils_send_buf;
            }
            g_ioutils_state.enc_srv_to_client_conversion(&server_chr, &client_chr);
            client_chr.chr += client_chr.length;
            g_ioutils_state.send_buf_ptr += client_chr.length;

            server_chr.chr += server_chr.length;
            g_ioutils_state.enc_srv_char_len(&server_chr);
        }
    }

    return 0;
}

sint8 ioutils_send(const uint8 *buf, uint32 sz)
{
    uint32 cpsz, diff, wr = 0u;

    while(sz > wr)
    {
        if(g_ioutils_state.send_buf_ptr == g_ioutils_state.send_buf_size)
        {
            if(ioutils_send_fully(g_ioutils_send_buf, g_ioutils_state.send_buf_ptr) != 0)
            {
                logger_error(_ach("ioutils, failed to send data to client: %s"), strerror(errno));
                return 1;
            }
            g_ioutils_state.send_buf_ptr = 0;
        }

        diff = g_ioutils_state.send_buf_size - g_ioutils_state.send_buf_ptr;
        cpsz = (diff > sz) ? sz : diff;
        memcpy(g_ioutils_send_buf + g_ioutils_state.send_buf_ptr, buf + wr, cpsz);
        wr += cpsz;
    }
    return 0;
}

sint8 ioutils_send_uint8(uint8 val)
{
   return ioutils_send(&val, sizeof(uint8));
}

sint8 ioutils_send_uint16(uint16 val)
{
   val = htobe16(val);
   return ioutils_send((uint8 *)&val, sizeof(uint16));
}

sint8 ioutils_send_uint32(uint32 val)
{
   val = htobe32(val);
   return ioutils_send((uint8 *)&val, sizeof(uint32));
}

sint8 ioutils_send_uint64(uint64 val)
{
   val = htobe64(val);
   return ioutils_send((uint8 *)&val, sizeof(uint64));
}
