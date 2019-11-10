#include "tests.h"
#include "session/pproto_server.h"
#include "logging/logger.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>


int test_pproto_server_functions()
{
    int sv[2];
    int i, j;
    const achar *str;
    char_info ch;
    uint8 chrinfo_buf[ENCODING_MAXCHAR_LEN];
    uint64 sz, charlen;
    uint8 buf[1024], cmpbuf[1024];
    uint8 cur_char;
    sint8 eos;
    encoding client_enc = ENCODING_UTF8, enc;
    pproto_msg_type msg_type;
    auth_credentials cred;
    int optval;

    ch.chr = chrinfo_buf;

    logger_create("test");

    puts("Starting test test_pproto_server_functions");

    if(0 != socketpair(AF_UNIX, SOCK_STREAM, 0, sv)) return __LINE__;

    // set timouts on read and write
    struct timeval tv;
    tv.tv_sec = 20;
    tv.tv_usec = 0;
    if(0 != setsockopt(sv[0], SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv))) return __LINE__;
    if(0 != setsockopt(sv[1], SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv))) return __LINE__;
    if(0 != setsockopt(sv[0], SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof(tv))) return __LINE__;
    if(0 != setsockopt(sv[1], SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof(tv))) return __LINE__;

    // set buf larger than client-side buf of pproto_server
    optval = 65536;
    if(0 != setsockopt(sv[0], SOL_SOCKET, SO_SNDBUF, (const char*)&optval, sizeof(optval))) return __LINE__;

    pproto_server_set_sock(sv[0]);

    encoding_init();
    pproto_server_set_encoding(ENCODING_UTF8);


    puts("Testing connection setup and management");

    // read client hello
    buf[0] = (uint8)((PPROTO_CLIENT_HELLO_MAGIC & 0xFF00) >> 8);
    buf[1] = (uint8)((PPROTO_CLIENT_HELLO_MAGIC & 0x00FF));
    buf[2] = (uint8)((client_enc & 0xFF00) >> 8);
    buf[3] = (uint8)((client_enc & 0x00FF));

    if(4 != send(sv[1], buf, 4, 0)) return __LINE__;
    msg_type = pproto_server_read_msg_type();
    if(msg_type != PPROTO_CLIENT_HELLO_MSG) return __LINE__;

    if(0 != pproto_server_read_client_hello(&enc)) return __LINE__;
    if(enc != client_enc) return __LINE__;
    pproto_server_set_client_encoding(client_enc);

    // sends server hello message
    if(0 != pproto_server_send_server_hello()) return __LINE__;
    if(6 != recv(sv[1], buf, 6, 0)) return __LINE__;
    if(buf[0] != 0x19 || buf[1] != 0x85) return __LINE__;
    if(buf[2] != 0x00 || buf[3] != 0x01) return __LINE__;
    if(buf[4] != 0x00 || buf[5] != 0x01) return __LINE__;


    // sends authentication request
    if(0 != pproto_server_send_auth_request()) return __LINE__;
    if(1 != recv(sv[1], buf, 1, 0)) return __LINE__;
    if(buf[0] != PPROTO_AUTH_REQUEST_MESSAGE_MAGIC) return __LINE__;


    // read auth message from client
    i = 0;
    buf[i++] = PPROTO_UTEXT_STRING_MAGIC;
    buf[i++] = 255;
    for(j=0; j<255; j++) buf[i++] = _ach('A');
    buf[i++] = 5;
    for(j=0; j<5; j++) buf[i++] = _ach('B');
    buf[i++] = 0x00;
    memset(buf + i, 0xCC, AUTH_CREDENTIAL_SZ);
    if(i + AUTH_CREDENTIAL_SZ != send(sv[1], buf, i + AUTH_CREDENTIAL_SZ, 0)) return __LINE__;
    if(0 != pproto_server_read_auth(&cred)) return __LINE__;
    for(i=0; i<255; i++)
    {
        if(cred.user_name[i] != _ach('A')) return __LINE__;
    }
    for(i=0; i<5; i++)
    {
        if(cred.user_name[255+i] != _ach('B')) return __LINE__;
    }

    // username is too long: 1
    i = 0;
    buf[i++] = PPROTO_UTEXT_STRING_MAGIC;
    buf[i++] = 255;
    for(j=0; j<255; j++) buf[i++] = _ach('A');
    buf[i++] = 6;
    for(j=0; j<6; j++) buf[i++] = 'B';
    buf[i++] = 0x00;
    memset(buf + i, 0xCC, AUTH_CREDENTIAL_SZ);
    if(i + AUTH_CREDENTIAL_SZ != send(sv[1], buf, i + AUTH_CREDENTIAL_SZ, 0)) return __LINE__;
    if(0 == pproto_server_read_auth(&cred)) return __LINE__;
    // flush unread buf
    for(j=0; j<AUTH_CREDENTIAL_SZ; j++)
    {
        if(pproto_server_read_msg_type() == -1) return __LINE__;
    }

    // username is too long: 2
    i = 0;
    buf[i++] = PPROTO_UTEXT_STRING_MAGIC;
    buf[i++] = 255;
    for(j=0; j<255; j++) buf[i++] = _ach('A');
    buf[i++] = 255;
    for(j=0; j<255; j++) buf[i++] = 'B';
    buf[i++] = 0x00;
    memset(buf + i, 0xCC, AUTH_CREDENTIAL_SZ);
    if(i + AUTH_CREDENTIAL_SZ != send(sv[1], buf, i + AUTH_CREDENTIAL_SZ, 0)) return __LINE__;
    if(0 == pproto_server_read_auth(&cred)) return __LINE__;
    if(1 != recv(sv[1], buf, 1, 0)) return __LINE__;
    if(buf[0] != PPROTO_CANCEL_MESSAGE_MAGIC) return __LINE__;
    // flush unread buf
    for(j=0; j<AUTH_CREDENTIAL_SZ; j++)
    {
        if(pproto_server_read_msg_type() == -1) return __LINE__;
    }

    // no username
    i = 0;
    buf[i++] = PPROTO_UTEXT_STRING_MAGIC;
    buf[i++] = 0x00;
    memset(buf + i, 0xCC, AUTH_CREDENTIAL_SZ);
    if(i + AUTH_CREDENTIAL_SZ != send(sv[1], buf, i + AUTH_CREDENTIAL_SZ, 0)) return __LINE__;
    if(0 == pproto_server_read_auth(&cred)) return __LINE__;
    // flush unread buf
    for(j=0; j<AUTH_CREDENTIAL_SZ; j++)
    {
        if(pproto_server_read_msg_type() == -1) return __LINE__;
    }


    // sends authentication status: success if auth_status = 1, failure otherwise
    if(0 != pproto_server_send_auth_responce(1)) return __LINE__;
    if(2 != recv(sv[1], buf, 2, 0)) return __LINE__;
    if(PPROTO_AUTH_RESPONCE_MESSAGE_MAGIC != buf[0]) return __LINE__;
    if(PPROTO_AUTH_SUCCESS != buf[1]) return __LINE__;

    if(0 != pproto_server_send_auth_responce(0)) return __LINE__;
    if(2 != recv(sv[1], buf, 2, 0)) return __LINE__;
    if(PPROTO_AUTH_RESPONCE_MESSAGE_MAGIC != buf[0]) return __LINE__;
    if(PPROTO_AUTH_FAIL != buf[1]) return __LINE__;


    // sends goodbye message to client
    if(0 != pproto_server_send_goodbye()) return __LINE__;
    if(1 != recv(sv[1], buf, 1, 0)) return __LINE__;
    if(PPROTO_GOODBYE_MESSAGE != buf[0]) return __LINE__;


    puts("Testing text string reading");

    // limited string
    i=0;
    buf[i++] = PPROTO_LTEXT_STRING_MAGIC;
    buf[i++] = 0xAA;
    buf[i++] = 0xBB;
    buf[i++] = 0xCC;
    buf[i++] = 0xDD;
    buf[i++] = 0xEE;
    buf[i++] = 0xFF;
    buf[i++] = 0xAA;
    buf[i++] = 0xBB;
    buf[i++] = 0x01;
    buf[i++] = _ach('!');
    buf[i++] = 0x00;
    if(i != send(sv[1], buf, i, 0)) return __LINE__;
    if(pproto_server_read_str_begin(&sz) != 0) return __LINE__;
    if(sz != 0xAABBCCDDEEFFAABB) return __LINE__;
    sz = 1024;
    memset(buf, 0, sz);
    if(pproto_server_read_str(buf, &sz, &charlen) != 0) return __LINE__;
    if(sz != 1) return __LINE__;
    if(charlen != 1) return __LINE__;
    if(buf[0] != _ach('!')) return __LINE__;
    if(pproto_server_read_str_end() != 0) return __LINE__;

    // repeat after end of string
    i=0;
    buf[i++] = PPROTO_UTEXT_STRING_MAGIC;
    buf[i++] = 0x01;
    buf[i++] = _ach('!');
    buf[i++] = 0x00;
    if(i != send(sv[1], buf, i, 0)) return __LINE__;
    if(pproto_server_read_str_begin(&sz) != 0) return __LINE__;
    sz = 1024;
    charlen = 0;
    if(pproto_server_read_str(buf, &sz, &charlen) != 0) return __LINE__;
    if(sz != 1) return __LINE__;
    if(charlen != 1) return __LINE__;
    if(buf[0] != _ach('!')) return __LINE__;
    sz = 1024;
    charlen = 0;
    memset(buf, 0, sz);
    if(pproto_server_read_str(buf, &sz, &charlen) != 0) return __LINE__;
    if(sz != 0) return __LINE__;
    if(charlen != 0) return __LINE__;
    if(pproto_server_read_str_end() != 0) return __LINE__;

    // small server buf
    i=0;
    buf[i++] = PPROTO_UTEXT_STRING_MAGIC;
    buf[i++] = 100;
    for(j=0; j<100; j++) buf[i++] = _ach('x');
    buf[i++] = 0;
    if(i != send(sv[1], buf, i, 0)) return __LINE__;
    if(pproto_server_read_str_begin(&sz) != 0) return __LINE__;
    sz = 55;
    charlen = 0;
    memset(buf, 0, 100);
    if(pproto_server_read_str(buf, &sz, &charlen) != 0) return __LINE__;
    if(sz != 52) return __LINE__;
    if(charlen != 52) return __LINE__;
    for(j=0; j<sz; j++)
    {
        if(buf[j] != _ach('x')) return __LINE__;
    }
    sz = 55;
    charlen = 0;
    memset(buf, 0, 100);
    if(pproto_server_read_str(buf, &sz, &charlen) != 0) return __LINE__;
    if(sz != 48) return __LINE__;
    if(charlen != 48) return __LINE__;
    for(j=0; j<sz; j++)
    {
        if(buf[j] != _ach('x')) return __LINE__;
    }
    if(pproto_server_read_str_end() != 0) return __LINE__;

    // really long string
    buf[0] = PPROTO_UTEXT_STRING_MAGIC;
    buf[1] = 255;
    if(2 != send(sv[1], buf, 2, 0)) return __LINE__;
    if(pproto_server_read_str_begin(&sz) != 0) return __LINE__;
    for(i=0; i<65536; i++)
    {
        for(j=0; j<255; j++)
            buf[j] = _ach('a');
        buf[255] = 100;
        for(j=256; j<356; j++)
            buf[j] = _ach('b');
        buf[356] = (i == 65535) ? 0 : 255;
        if(357 != send(sv[1], buf, 357, 0)) return __LINE__;

        sz = 358;
        charlen = 0;
        memset(buf, 0, 1024);
        if(pproto_server_read_str(buf, &sz, &charlen) != 0) return __LINE__;
        if(sz != 355) return __LINE__;
        if(charlen != 355) return __LINE__;
        for(j=0; j<255; j++)
        {
            if(buf[j] != _ach('a')) return __LINE__;
        }
        for(j=255; j<355; j++)
        {
            if(buf[j] != _ach('b')) return __LINE__;
        }
    }
    if(pproto_server_read_str_end() != 0) return __LINE__;

    // char len vs buf len
    i = 0;
    buf[i++] = PPROTO_UTEXT_STRING_MAGIC;
    str = _ach("ФЫВА");
    buf[i++] = strlen(str);
    memcpy(buf + i, str, buf[1]);
    i += buf[1];
    buf[i++] = 0;

    if(i != send(sv[1], buf, i, 0)) return __LINE__;
    if(pproto_server_read_str_begin(&sz) != 0) return __LINE__;
    sz = 1024;
    charlen = 0;
    memset(buf, 0, sz);
    if(pproto_server_read_str(buf, &sz, &charlen) != 0) return __LINE__;
    if(sz != 8) return __LINE__;
    if(charlen != 4) return __LINE__;
    if(strncmp((const char *)buf, str, sz)) return __LINE__;
    if(pproto_server_read_str_end() != 0) return __LINE__;

    // different encoding
    pproto_server_set_encoding(ENCODING_ASCII);
    pproto_server_set_client_encoding(ENCODING_UTF8);

    i = 0;
    buf[i++] = PPROTO_UTEXT_STRING_MAGIC;
    str = _ach("123 ФЫВА asd");
    buf[i++] = strlen(str);
    memcpy(buf + i, str, buf[1]);
    i += buf[1];
    buf[i++] = 0;

    if(i != send(sv[1], buf, i, 0)) return __LINE__;
    if(pproto_server_read_str_begin(&sz) != 0) return __LINE__;
    sz = 1024;
    charlen = 0;
    memset(buf, 0, sz);
    if(pproto_server_read_str(buf, &sz, &charlen) != 0) return __LINE__;
    if(sz != 12) return __LINE__;
    if(charlen != 12) return __LINE__;
    if(strncmp((const char *)buf, "123 ???? asd", sz)) return __LINE__;
    if(pproto_server_read_str_end() != 0) return __LINE__;

    // char-by-char reading
    pproto_server_set_encoding(ENCODING_UTF8);
    pproto_server_set_client_encoding(ENCODING_UTF8);

    i = 0;
    buf[i++] = PPROTO_UTEXT_STRING_MAGIC;
    str = _ach("123 ФЫВА asd");
    buf[i++] = strlen(str);
    memcpy(buf + i, str, buf[1]);
    i += buf[1];
    buf[i++] = 0;

    if(i != send(sv[1], buf, i, 0)) return __LINE__;
    if(pproto_server_read_str_begin(&sz) != 0) return __LINE__;
    for(i=0; i<12; i++)
    {
        ch.length = 0;
        ch.ptr = 0;
        memset(ch.chr, 0, ENCODING_MAXCHAR_LEN);
        ch.state = CHAR_STATE_INCOMPLETE;

        if(pproto_server_read_char(&ch, &eos) != 0) return __LINE__;

        if(ch.state != CHAR_STATE_COMPLETE) return __LINE__;

        if(i == 4 && !(ch.chr[0] == 208 && ch.chr[1] == 164)) return __LINE__;  // Ф
        if(i == 10 && ch.chr[0] != _ach('s')) return __LINE__;

        if(i >= 4 && i <= 7)
        {
            if(ch.length != 2) return __LINE__;
        }
        else
        {
            if(ch.length != 1) return __LINE__;
        }

        if(i == 11)
        {
            if(eos != 1) return __LINE__;
        }
        else
        {
            if(eos != 0) return __LINE__;
        }
    }
    // try after end of string
    if(pproto_server_read_char(&ch, &eos) != 0) return __LINE__;
    if(eos != 1) return __LINE__;
    if(pproto_server_read_str_end() != 0) return __LINE__;


    puts("Testing text string sending");

    if(0 != pproto_server_send_str_begin()) return __LINE__;
    memset(buf, _ach('a'), 100);
    memset(buf + 100, _ach('b'), 100);
    memset(buf + 200, _ach('c'), 99);
    buf[299] = 'x';
    if(0 != pproto_server_send_str(buf, 300, 1)) return __LINE__;
    if(0 != pproto_server_send_str_end()) return __LINE__;
    memset(buf, 0, 1024);
    if(304 != recv(sv[1], buf, 304, 0)) return __LINE__;
    if(buf[0] != PPROTO_UTEXT_STRING_MAGIC) return __LINE__;
    if(buf[1] != 255) return __LINE__;
    for(i=0; i<100; i++)
    {
        if(buf[2 + i] != _ach('a')) return __LINE__;
    }
    for(i=100; i<200; i++)
    {
        if(buf[2 + i] != _ach('b')) return __LINE__;
    }
    for(i=200; i<255; i++)
    {
        if(buf[2 + i] != _ach('c')) return __LINE__;
    }
    if(buf[257] != 45) return __LINE__;
    for(i=258; i<302; i++)
    {
        if(buf[i] != _ach('c')) return __LINE__;
    }
    if(buf[302] != _ach('x')) return __LINE__;
    if(buf[303] != 0) return __LINE__;

    // empty string
    if(0 != pproto_server_send_str_begin()) return __LINE__;
    if(0 != pproto_server_send_str_end()) return __LINE__;
    memset(buf, 0xff, 2);
    if(2 != recv(sv[1], buf, 2, 0)) return __LINE__;
    if(buf[0] != PPROTO_UTEXT_STRING_MAGIC) return __LINE__;
    if(buf[1] != 0) return __LINE__;

    if(0 != pproto_server_send_str_begin()) return __LINE__;
    if(0 != pproto_server_send_str(buf, 0, 1)) return __LINE__;
    if(0 != pproto_server_send_str(buf, 0, 1)) return __LINE__;
    if(0 != pproto_server_send_str_end()) return __LINE__;
    memset(buf, 0xff, 2);
    if(2 != recv(sv[1], buf, 2, 0)) return __LINE__;
    if(buf[0] != PPROTO_UTEXT_STRING_MAGIC) return __LINE__;
    if(buf[1] != 0) return __LINE__;

    // long string
    if(0 != pproto_server_send_str_begin()) return __LINE__;
    for(i=0; i<32; i++)
    {
        memset(buf, _ach('A') + i, 1024);
        if(0 != pproto_server_send_str(buf, 1024, 1)) return __LINE__;
    }
    if(0 != pproto_server_send_str_end()) return __LINE__;
    memset(buf, 0, 2);
    if(1 != recv(sv[1], buf, 1, 0)) return __LINE__;
    if(buf[0] != PPROTO_UTEXT_STRING_MAGIC) return __LINE__;
    cur_char = _ach('A');
    j = 0;
    do
    {
        if(1 != recv(sv[1], buf, 1, 0)) return __LINE__;
        sz = buf[0];
        if(sz > 0)
        {
            if(sz != recv(sv[1], buf, sz, 0)) return __LINE__;
            for(i=0; i<sz; i++)
            {
                if(buf[i] != cur_char) return __LINE__;
                j++;
                if(j == 1024)
                {
                    cur_char++;
                    j = 0;
                }
            }
        }
    }
    while(sz != 0);

    // different encoding
    pproto_server_set_encoding(ENCODING_UTF8);
    pproto_server_set_client_encoding(ENCODING_ASCII);

    if(0 != pproto_server_send_str_begin()) return __LINE__;
    str = "ФЫВА test";
    if(0 != pproto_server_send_str((const uint8 *)str, strlen(str), 1)) return __LINE__;
    if(0 != pproto_server_send_str_end()) return __LINE__;
    memset(buf, 0, 1024);
    if(12 != recv(sv[1], buf, 12, 0)) return __LINE__;
    if(buf[0] != PPROTO_UTEXT_STRING_MAGIC) return __LINE__;
    if(buf[1] != 9) return __LINE__;
    if(memcmp(buf+2, _ach("???? test"), 9)) return __LINE__;
    if(buf[11] != 0) return __LINE__;

    // source code encoding (srv_enc = 0)
    pproto_server_set_encoding(ENCODING_ASCII);
    pproto_server_set_client_encoding(ENCODING_UTF8);

    if(0 != pproto_server_send_str_begin()) return __LINE__;
    str = "ФЫВА test";
    if(0 != pproto_server_send_str((const uint8 *)str, strlen(str), 0)) return __LINE__;
    if(0 != pproto_server_send_str_end()) return __LINE__;
    memset(buf, 0, 1024);
    if(16 != recv(sv[1], buf, 16, 0)) return __LINE__;
    if(buf[0] != PPROTO_UTEXT_STRING_MAGIC) return __LINE__;
    if(buf[1] != 13) return __LINE__;
    if(memcmp(buf+2, _ach("ФЫВА test"), 13)) return __LINE__;
    if(buf[15] != 0) return __LINE__;


    puts("Testing other functions");

    buf[0] = PPROTO_SQL_REQUEST_MESSAGE_MAGIC;
    if(1 != send(sv[1], buf, 1, 0)) return __LINE__;
    msg_type = pproto_server_read_msg_type();
    if(msg_type != PPROTO_SQL_REQUEST_MSG) return __LINE__;

    // with message
    pproto_server_set_encoding(ENCODING_UTF8);
    pproto_server_set_client_encoding(ENCODING_UTF8);

    error_set(ERROR_SYNTAX_ERROR);
    sz = strlen(error_msg());
    memcpy(cmpbuf, error_msg(), sz);
    memcpy(cmpbuf + sz, _ach(": "), strlen(_ach(": ")));
    sz += strlen(_ach(": "));
    str = _ach("ФЫВА utf-8 msg");
    memcpy(cmpbuf + sz, str, strlen(str));
    sz += strlen(str);
    cmpbuf[sz] = _ach('\0');

    error_set(ERROR_NO_ERROR);
    if(pproto_server_send_error(ERROR_SYNTAX_ERROR, str)) return __LINE__;
    if(sz + 3 != recv(sv[1], buf, sz + 3, 0)) return __LINE__;
    if(buf[0] != PPROTO_UTEXT_STRING_MAGIC) return __LINE__;
    if(buf[1] != sz) return __LINE__;
    if(memcmp(buf+2, cmpbuf, sz)) return __LINE__;
    if(buf[sz + 2] != 0) return __LINE__;


    // without message
    error_set(ERROR_SYNTAX_ERROR);
    sz = strlen(error_msg());
    memcpy(cmpbuf, error_msg(), sz);
    cmpbuf[sz] = _ach('\0');

    error_set(ERROR_NO_ERROR);
    if(pproto_server_send_error(ERROR_SYNTAX_ERROR, NULL)) return __LINE__;
    if(sz + 3 != recv(sv[1], buf, sz + 3, 0)) return __LINE__;
    if(buf[0] != PPROTO_UTEXT_STRING_MAGIC) return __LINE__;
    if(buf[1] != sz) return __LINE__;
    if(memcmp(buf+2, cmpbuf, sz)) return __LINE__;
    if(buf[sz + 2] != 0) return __LINE__;


    return 0;
}
