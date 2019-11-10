#include "tests.h"
#include "client/pproto_client.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>


int prepare_col_desc(uint8 *buf, uint8 dt_magic, uint64 len, uint8 p, uint8 s, const char *alias, uint8 nullable)
{
    uint32 l = strlen(alias);
    assert(l <= 255);

    int i = 0;
    uint64 sl = htobe64(len);
    buf[i++] = dt_magic;
    switch(dt_magic)
    {
        case PPROTO_UTEXT_STRING_MAGIC:
        case PPROTO_LTEXT_STRING_MAGIC:
            memcpy(buf + i, &sl, sizeof(sl));
            i += sizeof(sl);
            break;
        case PPROTO_NUMERIC_VALUE_MAGIC:
            buf[i++] = p;
            buf[i++] = s;
            break;
        case PPROTO_TIMESTAMP_VALUE_MAGIC:
        case PPROTO_TIMESTAMP_WITH_TZ_VALUE_MAGIC:
            buf[i++] = p;
            break;
    }

    buf[i++] = nullable;

    buf[i++] = PPROTO_UTEXT_STRING_MAGIC;
    buf[i++] = l;
    memcpy(buf + i, alias, l);
    buf[i + l] = 0;

    return i + l + 1;
}

int check_col_desc(pproto_col_desc *cd, uint8 dt, uint64 len, uint8 p, uint8 s, const char *alias, uint8 nullable)
{
    uint32 al = strlen(alias);

    if(cd->data_type != dt) return __LINE__;

    if(cd->data_type == CHARACTER_VARYING)
    {
        if(cd->data_type_len != len) return __LINE__;
    }
    else if(cd->data_type == DECIMAL)
    {
        if(cd->data_type_precision != p) return __LINE__;
        if(cd->data_type_scale != s) return __LINE__;
    }
    else if(cd->data_type == TIMESTAMP_WITH_TZ || cd->data_type == TIMESTAMP)
    {
        if(cd->data_type_precision != p) return __LINE__;
    }

    if(cd->nullable != nullable) return __LINE__;
    if(memcmp(cd->col_alias, alias, al)) return __LINE__;
    if(cd->col_alias_sz != al) return __LINE__;

    return 0;
}

int test_pproto_client_functions()
{
    uint8 buf[1024], auth_status;
    pproto_msg_type msg_type;
    auth_credentials cred;
    int sv[2];
    uint64 len, sz, i64;
    int i, ret;
    uint8 nulls[2];
    pproto_col_desc cd;
    uint16 col_num;
    uint16 vminor, vmajor;

    decimal d;
    float32 f32;
    float64 f64;
    uint32 i32;
    uint16 s16;
    uint64 ts, dt;
    uint16 tz;

    if(sizeof(float64) != sizeof(uint64)) return __LINE__;
    if(sizeof(float32) != sizeof(uint32)) return __LINE__;

    puts("Tesing client protocol connection and auth");


    size_t ss_buf_sz = pproto_client_get_alloc_size();

    // sanity check
    if(!(ss_buf_sz > 0 && ss_buf_sz < 1024*1024)) return __LINE__;

    void *ss_buf = malloc(ss_buf_sz);
    if(NULL == ss_buf) return __LINE__;

    if(0 != socketpair(AF_UNIX, SOCK_STREAM, 0, sv)) return __LINE__;

    // set timeouts
    struct timeval tv;
    tv.tv_sec = 20;
    tv.tv_usec = 0;
    if(0 != setsockopt(sv[0], SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv))) return __LINE__;
    if(0 != setsockopt(sv[1], SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv))) return __LINE__;
    if(0 != setsockopt(sv[0], SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof(tv))) return __LINE__;
    if(0 != setsockopt(sv[1], SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof(tv))) return __LINE__;

    handle ss = pproto_client_create(ss_buf, sv[0]);
    if(NULL == ss) return __LINE__;

    if(0 != pproto_client_send_hello(ss, ENCODING_UTF8)) return __LINE__;

    if(4 != recv(sv[1], buf, 4, 0)) return __LINE__;
    if(buf[0] != 0x14 ||
       buf[1] != 0x06 ||
       buf[2] != 0x00 ||
       buf[3] != 0x02) return __LINE__;

    buf[0] = 0x19;
    buf[1] = 0x85;
    if(2 != send(sv[1], buf, 2, 0)) return __LINE__;
    msg_type = pproto_client_read_msg_type(ss);
    if(PPROTO_SERVER_HELLO_MSG != msg_type) return __LINE__;

    buf[0] = PPROTO_AUTH_REQUEST_MESSAGE_MAGIC;
    if(1 != send(sv[1], buf, 1, 0)) return __LINE__;
    msg_type = pproto_client_read_msg_type(ss);
    if(PPROTO_AUTH_REQUEST_MSG != msg_type) return __LINE__;

    buf[0] = 0xAA;
    if(1 != send(sv[1], buf, 1, 0)) return __LINE__;
    msg_type = pproto_client_read_msg_type(ss);
    if(PPROTO_UNKNOWN_MSG != msg_type) return __LINE__;

    vmajor = vminor = 0;
    buf[0] = 1;
    buf[1] = 2;
    buf[2] = 3;
    buf[3] = 4;
    if(4 != send(sv[1], buf, 4, 0)) return __LINE__;
    if(0 != pproto_client_read_server_hello(ss, &vmajor, &vminor)) return __LINE__;
    if(vmajor != 258 || vminor != 772) return __LINE__;


    strncpy((char *)cred.user_name, "test", AUTH_USER_NAME_SZ);
    memset(cred.credentials, 0xcc, AUTH_CREDENTIAL_SZ);
    if(0 != pproto_client_send_auth(ss, &cred)) return __LINE__;
    if(16 + AUTH_CREDENTIAL_SZ  != recv(sv[1], buf, 16 + AUTH_CREDENTIAL_SZ, 0)) return __LINE__;
    if(buf[0] != PPROTO_AUTH_MESSAGE_MAGIC ||
       buf[1] != PPROTO_LTEXT_STRING_MAGIC ||
       buf[2] != 0 ||
       buf[3] != 0 ||
       buf[4] != 0 ||
       buf[5] != 0 ||
       buf[6] != 0 ||
       buf[7] != 0 ||
       buf[8] != 0 ||
       buf[9] != 4 ||
       buf[10] != 4 ||
       buf[11] != 't' ||
       buf[12] != 'e' ||
       buf[13] != 's' ||
       buf[14] != 't' ||
       buf[15] != 0 ) return __LINE__;
    for(int i=0; i<AUTH_CREDENTIAL_SZ; i++)
    {
        if(buf[16 + i] != 0xcc) return __LINE__;
    }

    buf[0] = 0xFF;
    buf[1] = 0xCC;
    if(2 != send(sv[1], buf, 2, 0)) return __LINE__;
    if(0 != pproto_client_read_auth_responce(ss, &auth_status)) return __LINE__;
    if(auth_status != 0) return __LINE__;
    if(0 != pproto_client_read_auth_responce(ss, &auth_status)) return __LINE__;
    if(auth_status != 1) return __LINE__;


    puts("Tesing client protocol sending sql statement");


    memset(buf, 's', 256);
    if(0 != pproto_client_sql_stmt_begin(ss)) return __LINE__;
    if(0 != pproto_client_send_sql_stmt(ss, (const uint8 *)buf, 256)) return __LINE__;
    if(0 != pproto_client_sql_stmt_finish(ss)) return __LINE__;
    if(0 != pproto_client_send_cancel(ss)) return __LINE__;

    memset(buf, 0, sizeof(buf));
    if(262 != recv(sv[1], buf, 262, 0)) return __LINE__;
    if(buf[0] != PPROTO_SQL_REQUEST_MESSAGE_MAGIC ||
       buf[1] != PPROTO_UTEXT_STRING_MAGIC ||
       buf[2] != 255 ||
       buf[258] != 1 ||
       buf[259] != 's' ||
       buf[260] != 0 ||
       buf[261] != PPROTO_CANCEL_MESSAGE_MAGIC) return __LINE__;
    for(int i=3; i<257; i++)
    {
        if(buf[i] != 's') return __LINE__;
    }


    puts("Tesing client protocol reading string values");


    // limited string
    buf[0] = PPROTO_LTEXT_STRING_MAGIC;
    buf[1] = 0;
    buf[2] = 0;
    buf[3] = 0;
    buf[4] = 0;
    buf[5] = 0;
    buf[6] = 0;
    buf[7] = 1;
    buf[8] = 255;
    buf[9] = 255;
    i = 10;
    memset(buf+i, '1', 255);
    i += 255;
    buf[i++] = 255;
    memset(buf+i, '2', 255);
    i+=255;
    buf[i++] = 1;
    buf[i++] = '3';
    buf[i++] = 0;
    if(i != send(sv[1], buf, i, 0)) return __LINE__;
    len = 0;
    if(0 != pproto_client_read_str_begin(ss, &len)) return __LINE__;
    if(len != 255+255+1) return __LINE__;
    sz = 1024;
    memset(buf, 0, sz);
    if(0 != pproto_client_read_str(ss, buf, &sz)) return __LINE__;
    if(sz != 255+255+1) return __LINE__;
    if(buf[0] != '1' || buf[254] != '1') return __LINE__;
    if(buf[255] != '2' || buf[255+254] != '2') return __LINE__;
    if(buf[255 + 255] != '3') return __LINE__;
    if(0 != pproto_client_read_str_end(ss)) return __LINE__;

    buf[0] = PPROTO_LTEXT_STRING_MAGIC;
    buf[1] = 0;
    buf[2] = 0;
    buf[3] = 0;
    buf[4] = 0;
    buf[5] = 0;
    buf[6] = 0;
    buf[7] = 1;
    buf[8] = 255;
    buf[9] = 255;
    i = 10;
    memset(buf+i, '1', 255);
    i += 255;
    buf[i++] = 255;
    memset(buf+i, '2', 255);
    i+=255;
    buf[i++] = 1;
    buf[i++] = '3';
    buf[i++] = 0;
    if(i != send(sv[1], buf, i, 0)) return __LINE__;
    len = 0;
    if(0 != pproto_client_read_str_begin(ss, &len)) return __LINE__;
    if(len != 255+255+1) return __LINE__;
    sz = 254;
    memset(buf, 0, sz);
    if(0 == pproto_client_read_str(ss, buf, &sz)) return __LINE__;
    sz = 255+254;
    memset(buf, 0, sz);
    if(0 != pproto_client_read_str(ss, buf, &sz)) return __LINE__;
    if(255 != sz) return __LINE__;
    if(buf[0] != '1' || buf[254] != '1') return __LINE__;
    if(0 != pproto_client_read_str_end(ss)) return __LINE__;
    if(1 != recv(sv[1], buf, 1, 0)) return __LINE__;
    if(buf[0] != PPROTO_CANCEL_MESSAGE_MAGIC) return __LINE__;

    // unlimited string test
    buf[0] = PPROTO_UTEXT_STRING_MAGIC;
    buf[1] = 255;
    i = 2;
    memset(buf+i, '1', 255);
    i += 255;
    buf[i++] = 255;
    memset(buf+i, '2', 255);
    i+=255;
    buf[i++] = 1;
    buf[i++] = '3';
    buf[i++] = 0;
    if(i != send(sv[1], buf, i, 0)) return __LINE__;
    len = 0;
    if(0 != pproto_client_read_str_begin(ss, &len)) return __LINE__;
    if(len != 0) return __LINE__;
    sz = 1024;
    memset(buf, 0, sz);
    if(0 != pproto_client_read_str(ss, buf, &sz)) return __LINE__;
    if(sz != 255+255+1) return __LINE__;
    if(buf[0] != '1' || buf[254] != '1') return __LINE__;
    if(buf[255] != '2' || buf[255+254] != '2') return __LINE__;
    if(buf[255 + 255] != '3') return __LINE__;
    if(0 != pproto_client_read_str_end(ss)) return __LINE__;

    // empty string
    buf[0] = PPROTO_UTEXT_STRING_MAGIC;
    buf[1] = 0;
    if(2 != send(sv[1], buf, 2, 0)) return __LINE__;
    len = 0;
    if(0 != pproto_client_read_str_begin(ss, &len)) return __LINE__;
    if(len != 0) return __LINE__;
    sz = 1024;
    if(0 != pproto_client_read_str(ss, buf, &sz)) return __LINE__;
    if(sz != 0) return __LINE__;
    if(0 != pproto_client_read_str_end(ss)) return __LINE__;


    puts("Tesing client protocol reading numeric values");


    buf[0] = 0x80 | 0x40 | 0x05;
    buf[1] = 1;
    buf[2] = 2;
    buf[3] = 3;
    buf[4] = 4;
    buf[5] = 5;
    buf[6] = 255;
    if(7 != send(sv[1], buf, 7, 0)) return __LINE__;
    if(0 != pproto_client_read_decimal_value(ss, &d)) return __LINE__;
//  printf("n: %d, e: %d, sign: %d, m={%x %x %x %x %x %x %x %x %x %x}\n", d.n, d.e, d.sign, d.m[9], d.m[8], d.m[7], d.m[6], d.m[5], d.m[4], d.m[3], d.m[2], d.m[1], d.m[0] );
    if(d.sign != DECIMAL_SIGN_NEG) return __LINE__;
    if(d.e != -1) return __LINE__;
    if(d.n != 9) return __LINE__;
    if(d.m[0] != 0x0201 || d.m[1] != 0x0403 || d.m[2] != 0x0005) return __LINE__;

    buf[0] = 0x06;
    buf[1] = 0x01;
    buf[2] = 0x02;
    buf[3] = 0x01;
    buf[4] = 0x02;
    buf[5] = 0x01;
    buf[6] = 0x11;
    if(7 != send(sv[1], buf, 7, 0)) return __LINE__;
    if(0 != pproto_client_read_decimal_value(ss, &d)) return __LINE__;
    if(d.sign != DECIMAL_SIGN_POS) return __LINE__;
    if(d.e != 0) return __LINE__;
    if(d.n != 12) return __LINE__;
    if(d.m[0] != 513 || d.m[1] != 513 || d.m[2] != 4353) return __LINE__;

    f64 = -1.23456e-223;
    memcpy(&i64, &f64, sizeof(i64));
    i64 = htobe64((uint64)i64);
    if(sizeof(i64) != send(sv[1], &i64, sizeof(i64), 0)) return __LINE__;
    f64 = 0.0;
    if(0 != pproto_client_read_double_value(ss, &f64)) return __LINE__;
    if(f64 != -1.23456e-223) return __LINE__;

    f32 = -1.23456e-12f;
    memcpy(&i32, &f32, sizeof(i32));
    i32 = htobe32(i32);
    if(sizeof(i32) != send(sv[1], &i32, sizeof(i32), 0)) return __LINE__;
    f32 = 0.0f;
    if(0 != pproto_client_read_float_value(ss, &f32)) return __LINE__;
    if(f32 != -1.23456e-12f) return __LINE__;

    i32 = htobe32((uint32)-104010301);
    if(sizeof(i32) != send(sv[1], &i32, sizeof(i32), 0)) return __LINE__;
    i32 = 0;
    if(0 != pproto_client_read_integer_value(ss, &i32)) return __LINE__;
    if((sint32)i32 != -104010301) return __LINE__;

    s16 = htobe16((uint64)(-31010));
    if(sizeof(s16) != send(sv[1], &s16, sizeof(s16), 0)) return __LINE__;
    s16 = 0;
    if(0 != pproto_client_read_smallint_value(ss, &s16)) return __LINE__;
    if((sint16)s16 != -31010) return __LINE__;


    puts("Tesing client protocol reading date and timestamp values");

    i64 = htobe64(63718704000UL);
    if(sizeof(i64) != send(sv[1], &i64, sizeof(i64), 0)) return __LINE__;
    dt = 0;
    if(0 != pproto_client_read_date_value(ss, &dt)) return __LINE__;
    if(dt != 63718704000UL) return __LINE__;

    i64 = htobe64(63718704000UL);
    if(sizeof(i64) != send(sv[1], &i64, sizeof(i64), 0)) return __LINE__;
    ts = 0;
    if(0 != pproto_client_read_timestamp_value(ss, &ts)) return __LINE__;
    if(ts != 63718704000UL) return __LINE__;

    i64 = htobe64(63718704000UL);
    s16 = htobe16(567);
    if(sizeof(i64) != send(sv[1], &i64, sizeof(i64), 0)) return __LINE__;
    if(sizeof(s16) != send(sv[1], &s16, sizeof(s16), 0)) return __LINE__;
    ts = 0;
    tz = 0;
    if(0 != pproto_client_read_timestamp_with_tz_value(ss, &ts, &tz)) return __LINE__;
    if(ts != 63718704000UL) return __LINE__;
    if(tz != 567) return __LINE__;


    puts("Tesing client protocol recordset functions");


    buf[0] = 0;
    buf[1] = 10;
    i = 2;
    i += prepare_col_desc(buf + i, PPROTO_UTEXT_STRING_MAGIC, 10, 0, 0, "col1", 0);
    i += prepare_col_desc(buf + i, PPROTO_LTEXT_STRING_MAGIC, 10, 0, 0, "col2", 1);
    i += prepare_col_desc(buf + i, PPROTO_NUMERIC_VALUE_MAGIC, 0, 10, 2, "col3", 0);
    i += prepare_col_desc(buf + i, PPROTO_INTEGER_VALUE_MAGIC, 0, 0, 0, "col4", 1);
    i += prepare_col_desc(buf + i, PPROTO_SMALLINT_VALUE_MAGIC, 0, 0, 0, "col5", 0);
    i += prepare_col_desc(buf + i, PPROTO_FLOAT_VALUE_MAGIC, 0, 0, 0, "col6", 1);
    i += prepare_col_desc(buf + i, PPROTO_DOUBLE_PRECISION_VALUE_MAGIC, 0, 0, 0, "col7", 0);
    i += prepare_col_desc(buf + i, PPROTO_DATE_VALUE_MAGIC, 0, 0, 0, "col8", 0);
    i += prepare_col_desc(buf + i, PPROTO_TIMESTAMP_VALUE_MAGIC, 0, 6, 0, "col9", 1);
    i += prepare_col_desc(buf + i, PPROTO_TIMESTAMP_WITH_TZ_VALUE_MAGIC, 0, 6, 0, "col10", 0);
    if(i != send(sv[1], buf, i, 0)) return __LINE__;
    if(0 != pproto_client_read_recordset_col_num(ss, &col_num)) return __LINE__;
    if(10 != col_num) return __LINE__;
    if(0 != pproto_client_read_recordset_col_desc(ss, &cd)) return __LINE__;
    if(0 != (ret = check_col_desc(&cd, CHARACTER_VARYING, 10, 0, 0, "col1", 0))) return __LINE__*1000000 + ret;
    if(0 != pproto_client_read_recordset_col_desc(ss, &cd)) return __LINE__;
    if(0 != (ret = check_col_desc(&cd, CHARACTER_VARYING, 10, 0, 0, "col2", 1))) return __LINE__*1000000 + ret;
    if(0 != pproto_client_read_recordset_col_desc(ss, &cd)) return __LINE__;
    if(0 != (ret = check_col_desc(&cd, DECIMAL, 0, 10, 2, "col3", 0))) return __LINE__*1000000 + ret;
    if(0 != pproto_client_read_recordset_col_desc(ss, &cd)) return __LINE__;
    if(0 != (ret = check_col_desc(&cd, INTEGER, 0, 0, 0, "col4", 1))) return __LINE__*1000000 + ret;
    if(0 != pproto_client_read_recordset_col_desc(ss, &cd)) return __LINE__;
    if(0 != (ret = check_col_desc(&cd, SMALLINT, 0, 0, 0, "col5", 0))) return __LINE__*1000000 + ret;
    if(0 != pproto_client_read_recordset_col_desc(ss, &cd)) return __LINE__;
    if(0 != (ret = check_col_desc(&cd, FLOAT, 0, 0, 0, "col6", 1))) return __LINE__*1000000 + ret;
    if(0 != pproto_client_read_recordset_col_desc(ss, &cd)) return __LINE__;
    if(0 != (ret = check_col_desc(&cd, DOUBLE_PRECISION, 0, 0, 0, "col7", 0))) return __LINE__*1000000 + ret;
    if(0 != pproto_client_read_recordset_col_desc(ss, &cd)) return __LINE__;
    if(0 != (ret = check_col_desc(&cd, DATE, 0, 0, 0, "col8", 0))) return __LINE__*1000000 + ret;
    if(0 != pproto_client_read_recordset_col_desc(ss, &cd)) return __LINE__;
    if(0 != (ret = check_col_desc(&cd, TIMESTAMP, 0, 6, 0, "col9", 1))) return __LINE__*1000000 + ret;
    if(0 != pproto_client_read_recordset_col_desc(ss, &cd)) return __LINE__;
    if(0 != (ret = check_col_desc(&cd, TIMESTAMP_WITH_TZ, 0, 6, 0, "col10", 0))) return __LINE__*1000000 + ret;


    buf[0] = PPROTO_RECORDSET_ROW_MAGIC;
    buf[1] = 0xCC;
    buf[2] = 0xDD;
    if(3 != send(sv[1], buf, 3, 0)) return __LINE__;
    if(1 != pproto_client_recordset_start_row(ss, nulls, 2)) return __LINE__;
    if(nulls[0] != 0xCC || nulls[1] != 0xDD) return __LINE__;

    buf[0] = PPROTO_RECORDSET_END;
    if(1 != send(sv[1], buf, 1, 0)) return __LINE__;
    if(0 != pproto_client_recordset_start_row(ss, nulls, 2)) return __LINE__;

    buf[0] = PPROTO_GOODBYE_MESSAGE;
    if(1 != send(sv[1], buf, 1, 0)) return __LINE__;
    if(-1 != pproto_client_recordset_start_row(ss, nulls, 2)) return __LINE__;


    puts("Tesing client protocol other functions");


    if(0 != pproto_client_send_goodbye(ss)) return __LINE__;
    if(1 != recv(sv[1], buf, 1, 0)) return __LINE__;
    if(buf[0] != PPROTO_GOODBYE_MESSAGE) return __LINE__;

    if(NULL == pproto_client_last_error_msg(ss)) return __LINE__;

    if(0 != pproto_client_poll(ss)) return __LINE__;
    buf[0] = 1;
    if(1 != send(sv[1], buf, 1, 0)) return __LINE__;
    if(1 != pproto_client_poll(ss)) return __LINE__;
    if(1 != pproto_client_poll(ss)) return __LINE__;


    ///////////////////////////////////////// cleanup

    if(0 != close(sv[0])) return 1;
    if(0 != close(sv[1])) return 1;

    return 0;
}
