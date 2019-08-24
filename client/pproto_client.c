#include "client/pproto_client.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif

#include <endian.h>


#define PPROTO_CLIENT_RECV_BUF_SIZE (8192u)
#define PPROTO_CLIENT_SEND_BUF_SIZE (8192u)
#define PPROTO_MAX_ERRMES           (1024)


typedef struct
{
    int sock;
    uint32 recv_buf_size;
    uint32 send_buf_size;

    uint32 recv_buf_ptr;
    uint32 recv_buf_upper_bound;
    uint32 send_buf_ptr;

    uint8 next_chunk_len;
    char errmes[PPROTO_MAX_ERRMES];
    uint8 recv_buf[PPROTO_CLIENT_RECV_BUF_SIZE];
    uint8 send_buf[PPROTO_CLIENT_SEND_BUF_SIZE];
} pproto_client_state;


size_t pproto_client_get_alloc_size()
{
    return sizeof(pproto_client_state);
}

handle pproto_client_create(void *ss_buf, int sock)
{
    pproto_client_state *state = (pproto_client_state *)ss_buf;

    state->sock = sock;

    state->recv_buf_size = PPROTO_CLIENT_RECV_BUF_SIZE;
    state->send_buf_size = PPROTO_CLIENT_SEND_BUF_SIZE;

    state->recv_buf_ptr = 0;
    state->recv_buf_upper_bound = 0;
    state->send_buf_ptr = 0;

    return (handle)state;
}

void pproto_client_set_sock(handle ss, int sock)
{
    pproto_client_state *state = (pproto_client_state *)ss;
    state->sock = sock;
}

sint8 pproto_client_send_fully(handle ss, const void *data, uint64 sz)
{
    pproto_client_state *state = (pproto_client_state *)ss;
    ssize_t written;
    uint64 total_written = 0u;

    while(sz > total_written && (written = send(state->sock, data, sz - total_written, 0)) > 0)
    {
        total_written += (uint64)written;
    }

    if(written <= 0)
    {
        return 1;
    }

    return 0;
}

sint8 pproto_client_read_portion(handle ss)
{
    pproto_client_state *state = (pproto_client_state *)ss;
    ssize_t readsz;
    uint32 leftsz = state->recv_buf_size - state->recv_buf_upper_bound;

    if(0u == leftsz)
    {
        leftsz = state->recv_buf_size;
        state->recv_buf_ptr = 0u;
        state->recv_buf_upper_bound = 0u;
    }

    readsz = recv(state->sock, state->recv_buf + state->recv_buf_upper_bound, leftsz, 0);
    if(readsz <= 0)
    {
        return 1;
    }
    state->recv_buf_upper_bound += (uint32)readsz;

    return 0;
}

sint8 pproto_client_get(handle ss, uint8 *buf, uint32 sz)
{
    pproto_client_state *state = (pproto_client_state *)ss;
    uint32 cpsz, diff;

    while(sz > 0u)
    {
        diff = state->recv_buf_upper_bound - state->recv_buf_ptr;
        if(diff > 0u)
        {
            cpsz = diff > sz ? sz : diff;
            memcpy(buf, state->recv_buf + state->recv_buf_ptr, cpsz);
            buf += cpsz;
            state->recv_buf_ptr += cpsz;
            sz -= cpsz;
        }
        else
        {
            if(pproto_client_read_portion(ss) != 0) return 1;
        }
    }
    return 0;
}

sint8 pproto_client_flush_send(handle ss)
{
    pproto_client_state *state = (pproto_client_state *)ss;
    sint8 res = pproto_client_send_fully(ss, state->send_buf, state->send_buf_ptr);
    if(0 == res)
    {
        state->send_buf_ptr = 0u;
    }
    return res;
}


sint8 pproto_client_send(handle ss, const uint8 *buf, uint32 sz)
{
    pproto_client_state *state = (pproto_client_state *)ss;
    uint32 cpsz, diff, wr = 0u;

    while(sz > wr)
    {
        if(state->send_buf_ptr == state->send_buf_size)
        {
            if(pproto_client_send_fully(ss, state->send_buf, state->send_buf_ptr) != 0)
            {
                return 1;
            }
            state->send_buf_ptr = 0;
        }

        diff = state->send_buf_size - state->send_buf_ptr;
        cpsz = (diff > sz) ? sz : diff;
        memcpy(state->send_buf + state->send_buf_ptr, buf + wr, cpsz);
        wr += cpsz;
        state->send_buf_ptr += cpsz;
    }
    return 0;
}


sint8 pproto_client_send_string(handle ss, const char* str)
{
    uint8 len;
    uint64 total = strlen(str), totaln;
    uint8 magic = PPROTO_LTEXT_STRING_MAGIC;

    if(0 != pproto_client_send(ss, (const uint8 *)&magic, sizeof(magic))) return 1;

    totaln = htobe64(total);
    if(0 != pproto_client_send(ss, (const uint8 *)&totaln, sizeof(totaln))) return 1;

    while(total > 0)
    {
        len = (total > 255) ? 255 : (uint8)total;
        if(pproto_client_send(ss, &len, sizeof(len)) != 0) return 1;
        if(pproto_client_send(ss, (const uint8 *)str, len*sizeof(uint8)) != 0) return 1;
        str += len;
        total -= len;
    }

    len = 0;
    if(pproto_client_send(ss, &len, sizeof(len)) != 0) return 1;

    return 0;
}


sint8 pproto_client_read_recordset_col_desc(handle ss, pproto_col_desc *col_desc)
{
    uint8       dt_magic, dt_precision, dt_scale;
    uint64      dt_len, dt_lenn, len;
    uint64      sz=(MAX_TABLE_COL_NAME_LEN/255 + 1)*255;
    uint8       alias_buf[sz];
    uint8       flags;

    if(0 != pproto_client_get(ss, &dt_magic, sizeof(dt_magic))) return 1;

    switch(dt_magic)
    {
        case PPROTO_UTEXT_STRING_MAGIC:
        case PPROTO_LTEXT_STRING_MAGIC:
            col_desc->data_type = CHARACTER_VARYING;
            if(0 != pproto_client_get(ss, (uint8 *)&dt_lenn, sizeof(dt_len))) return 1;
            dt_len = be64toh(dt_lenn);
            col_desc->data_type_len = dt_len;
            break;

        case PPROTO_NUMERIC_VALUE_MAGIC:
            col_desc->data_type = DECIMAL;
            if(0 != pproto_client_get(ss, &dt_precision, sizeof(dt_precision))) return 1;
            if(0 != pproto_client_get(ss, &dt_scale, sizeof(dt_scale))) return 1;
            col_desc->data_type_precision = dt_precision;
            col_desc->data_type_scale = dt_scale;
            break;

        case PPROTO_INTEGER_VALUE_MAGIC:
            col_desc->data_type = INTEGER;
            break;
        case PPROTO_SMALLINT_VALUE_MAGIC:
            col_desc->data_type = SMALLINT;
            break;
        case PPROTO_FLOAT_VALUE_MAGIC:
            col_desc->data_type = FLOAT;
            break;
        case PPROTO_DOUBLE_PRECISION_VALUE_MAGIC:
            col_desc->data_type = DOUBLE_PRECISION;
            break;
        case PPROTO_DATE_VALUE_MAGIC:
            col_desc->data_type = DATE;
            break;

        case PPROTO_TIMESTAMP_VALUE_MAGIC:
            col_desc->data_type = TIMESTAMP;
            if(0 != pproto_client_get(ss, &dt_precision, sizeof(dt_precision))) return 1;
            col_desc->data_type_precision = dt_precision;
            break;
        case PPROTO_TIMESTAMP_WITH_TZ_VALUE_MAGIC:
            col_desc->data_type = TIMESTAMP_WITH_TZ;
            if(0 != pproto_client_get(ss, &dt_precision, sizeof(dt_precision))) return 1;
            col_desc->data_type_precision = dt_precision;
            break;

        default:
            errno = EPROTO;
            return 1;
    }

    if(0 != pproto_client_get(ss, &flags, sizeof(flags))) return 1;
    col_desc->nullable = (0 != (flags & PPROTO_COL_FLAG_NULLABLE)) ? 1 : 0;

    if(0 != pproto_client_read_str_begin(ss, &len)) return 1;
    if(0 != pproto_client_read_str(ss, alias_buf, &sz)) return 1;
    if(0 != pproto_client_read_str_end(ss)) return 1;

    if(sz > MAX_TABLE_COL_NAME_LEN) sz = MAX_TABLE_COL_NAME_LEN;

    memcpy(col_desc->col_alias, alias_buf, sz);
    col_desc->col_alias[sz] = '\0';
    col_desc->col_alias_sz = sz;

    return 0;
}



sint8 pproto_client_read_recordset_col_num(handle ss, uint16 *n)
{
    uint16 col_num;

    if(0 != pproto_client_get(ss, (uint8 *)&col_num, sizeof(col_num))) return 1;

    *n = be16toh(col_num);

    return 0;
}



sint8 pproto_client_recordset_start_row(handle ss, uint8 *nulls, uint32 nulls_sz)
{
    uint8 magic;

    if(0 != pproto_client_get(ss, &magic, sizeof(magic))) return -1;

    if(PPROTO_RECORDSET_END == magic)
    {
        return 0;
    }
    else if(PPROTO_RECORDSET_ROW_MAGIC == magic)
    {
        if(0 != pproto_client_get(ss, nulls, nulls_sz*sizeof(uint8))) return -1;
    }
    else
    {
        errno = EPROTO;
        return -1;
    }

    return 1;
}



sint8 pproto_client_read_str_begin(handle ss, uint64 *len)
{
    pproto_client_state *state = (pproto_client_state *)ss;
    uint8 str_type;
    uint64 str_len;

    if(0 != pproto_client_get(ss, &str_type, sizeof(str_type))) return 1;

    if(PPROTO_LTEXT_STRING_MAGIC == str_type)
    {
        if(0 != pproto_client_get(ss, (uint8 *)&str_len, sizeof(str_len))) return 1;

        *len = be64toh(str_len);
    }

    if(pproto_client_get(ss, &state->next_chunk_len, sizeof(state->next_chunk_len)) != 0)
    {
        return 1;
    }

    return 0;
}



sint8 pproto_client_read_str(handle ss, uint8 *strbuf, uint64 *sz)
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



sint8 pproto_client_read_date_value(handle ss, uint64 *date)
{
    uint64 dtn;

    if(0 != pproto_client_get(ss, (uint8 *)&dtn, sizeof(dtn))) return 1;

    *date = be64toh(dtn);

    return 0;
}



sint8 pproto_client_read_decimal_value(handle ss, decimal *d)
{
    uint8 b, m[DECIMAL_PARTS * 2], len, i = 0;
    uint16 p;

    memset(d, 0, sizeof(decimal));

    if(0 != pproto_client_get(ss, &b, sizeof(b))) return 1;

    len = b & 0x3F;
    if(0 == len)
    {
        d->sign = DECIMAL_SIGN_POS;
        return 0;
    }

    if(len > DECIMAL_PARTS * 2)
    {
        errno = EPROTO;
        return 1;
    }

    if(0 != pproto_client_get(ss, m, len)) return 1;

    for(i=0; i<len; i++)
    {
        d->m[i/2] |= ((uint16)m[i]) << (i%2 * 8);
    }

    i = len / 2 - 1 + len % 2;
    d->n = i * DECIMAL_BASE_LOG10;
    p = d->m[i];
    while(p > 0)
    {
        d->n++;
        p /= 10;
    }

    d->sign = (b & 0x80) ? DECIMAL_SIGN_NEG : DECIMAL_SIGN_POS;

    if(b & 0x40)
    {
        if(0 != pproto_client_get(ss, (uint8 *)&d->e, sizeof(uint8))) return 1;
    }

    return 0;
}



sint8 pproto_client_read_double_value(handle ss, float64 *d)
{
    uint64 l;

    if(0 != pproto_client_get(ss, (uint8 *)&l, sizeof(l))) return 1;

    l = be64toh(l);
    memcpy(d, &l, sizeof(*d));

    return 0;
}



sint8 pproto_client_read_float_value(handle ss, float32 *f)
{
    uint32 l;

    if(0 != pproto_client_get(ss, (uint8 *)&l, sizeof(l))) return 1;

    l = be32toh(l);
    memcpy(f, &l, sizeof(*f));

    return 0;
}



sint8 pproto_client_read_integer_value(handle ss, sint32 *i)
{
    uint32 l;

    if(0 != pproto_client_get(ss, (uint8 *)&l, sizeof(l))) return 1;

    *i = (sint32)be32toh(l);

    return 0;
}



sint8 pproto_client_read_smallint_value(handle ss, sint16 *s)
{
    uint16 l;

    if(0 != pproto_client_get(ss, (uint8 *)&l, sizeof(l))) return 1;

    *s = (sint16)be16toh(l);

    return 0;
}



sint8 pproto_client_read_timestamp_value(handle ss, uint64 *ts)
{
    uint64 l;

    if(0 != pproto_client_get(ss, (uint8 *)&l, sizeof(l))) return 1;

    *ts = (uint64)be64toh(l);

    return 0;
}



sint8 pproto_client_read_timestamp_with_tz_value(handle ss, uint64 *ts, sint16 *tz)
{
    uint64 l;
    uint16 t;

    if(0 != pproto_client_get(ss, (uint8 *)&l, sizeof(l))) return 1;
    if(0 != pproto_client_get(ss, (uint8 *)&t, sizeof(t))) return 1;

    *ts = (uint64)be64toh(l);
    *tz = (uint16)be16toh(t);

    return 0;
}


pproto_msg_type pproto_client_read_msg_type(handle ss)
{
    uint8 byte;

    if(pproto_client_get(ss, &byte, sizeof(byte)) != 0)
    {
        return PPROTO_MSG_TYPE_ERR;
    }

    if((uint8)(PPROTO_SERVER_HELLO_MAGIC >> 8) == byte)
    {
        if(pproto_client_get(ss, &byte, sizeof(byte)) != 0)
        {
            return PPROTO_MSG_TYPE_ERR;
        }

        if((uint8)(PPROTO_SERVER_HELLO_MAGIC) == byte)
        {
            return PPROTO_SERVER_HELLO_MSG;
        }
    }

    switch(byte)
    {
        case PPROTO_ERROR_MSG_MAGIC:
            return PPROTO_ERROR_MSG;
            break;
        case PPROTO_SUCCESS_MESSAGE_WITH_TEXT_MAGIC:
            return PPROTO_SUCCESS_WITH_TEXT_MSG;
            break;
        case PPROTO_SUCCESS_MESSAGE_WITHOUT_TEXT_MAGIC:
            return PPROTO_SUCCESS_WITHOUT_TEXT_MSG;
            break;
        case PPROTO_RECORDSET_MESSAGE_MAGIC:
            return PPROTO_RECORDSET_MSG;
            break;
        case PPROTO_AUTH_REQUEST_MESSAGE_MAGIC:
            return PPROTO_AUTH_REQUEST_MSG;
            break;
        case PPROTO_AUTH_RESPONCE_MESSAGE_MAGIC:
            return PPROTO_AUTH_RESPONCE_MSG;
            break;
        case PPROTO_PROGRESS_MESSAGE_MAGIC:
            return PPROTO_PROGRESS_MSG;
            break;
        case PPROTO_GOODBYE_MESSAGE:
            return PPROTO_GOODBYE_MSG;
            break;
        default:
            return PPROTO_UNKNOWN_MSG;
    }

    return 0;
}


sint8 pproto_client_read_auth_responce(handle ss, uint8 *auth_status)
{
    uint8 status;

    if(0 == pproto_client_get(ss, &status, sizeof(status)))
    {
        *auth_status = (PPROTO_AUTH_SUCCESS == status) ? 1 : 0;
        return 0;
    }

    return 1;
}


sint8 pproto_client_send_hello(handle ss, encoding client_encoding)
{
    uint16 magic = (uint16)htobe16(PPROTO_CLIENT_HELLO_MAGIC);
    uint16 enc = (uint16)htobe16((uint16)client_encoding);

    if(0 != pproto_client_send(ss, (uint8 *)&magic, sizeof(magic))) return 1;
    if(0 != pproto_client_send(ss, (uint8 *)&enc, sizeof(enc))) return 1;

    return pproto_client_flush_send(ss);
}


sint8 pproto_client_send_auth(handle ss, auth_credentials *cred)
{
    uint8 magic = PPROTO_AUTH_MESSAGE_MAGIC;

    if(0 != pproto_client_send(ss, &magic, sizeof(magic))) return 1;

    if(0 != pproto_client_send_string(ss, (const char*)cred->user_name)) return 1;

    if(0 != pproto_client_send(ss, cred->credentials, AUTH_CREDENTIAL_SZ * sizeof(uint8))) return 1;

    return pproto_client_flush_send(ss);
}


sint8 pproto_client_send_goodbye(handle ss)
{
    uint8 magic = PPROTO_GOODBYE_MESSAGE;

    if(0 != pproto_client_send(ss, &magic, sizeof(magic))) return 1;

    return pproto_client_flush_send(ss);
}


sint8 pproto_client_sql_stmt_begin(handle ss)
{
    uint8 magics[2] = {PPROTO_SQL_REQUEST_MESSAGE_MAGIC, PPROTO_UTEXT_STRING_MAGIC};

    return pproto_client_send(ss, magics, sizeof(magics));
}


sint8 pproto_client_send_sql_stmt(handle ss, const uint8* data, uint32 sz)
{
    uint8 len;

    while(sz > 0)
    {
        len = (sz > 255 ? 255 : sz);
        if(pproto_client_send(ss, (const uint8 *)&len, sizeof(len)) != 0) return 1;
        if(pproto_client_send(ss, data, len*sizeof(uint8)) != 0) return 1;
        data += len;
        sz -= len;
    }

    return 0;
}


sint8 pproto_client_sql_stmt_finish(handle ss)
{
    uint8 terminator = 0;
    if(0 != pproto_client_send(ss, &terminator, sizeof(terminator))) return 1;

    return pproto_client_flush_send(ss);
}


sint8 pproto_client_send_cancel(handle ss)
{
    uint8 magic = PPROTO_CANCEL_MESSAGE_MAGIC;
    if(0!= pproto_client_send(ss, &magic, sizeof(magic))) return 1;

    return pproto_client_flush_send(ss);
}


const char *pproto_client_last_error_msg(handle ss)
{
    pproto_client_state *state = (pproto_client_state *)ss;

    assert(0 == strerror_r(errno, state->errmes, PPROTO_MAX_ERRMES));

    state->errmes[PPROTO_MAX_ERRMES - 1] = '\0';

    return state->errmes;
}

