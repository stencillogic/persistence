#ifndef _pproto_server_SERVER_H
#define _pproto_server_SERVER_H


// server-side protocol


#include "common/pproto.h"
#include "common/encoding.h"
#include "common/error.h"
#include "auth/auth.h"



// set socket to work with
void pproto_server_set_sock(int client_sock);

// set client encoding
void pproto_server_set_client_encoding(encoding enc);

// set server encoding
void pproto_server_set_encoding(encoding enc);


////////////////// connection setup and management



// read hello message from client
// return 0 on success, non 0 otherwise
sint8 pproto_server_read_client_hello(encoding *client_enc);

// sends server hello message
// return 0 on success, non 0 otherwise
sint8 pproto_server_send_server_hello();

// sends authentication request
// return 0 on success, non 0 otherwise
sint8 pproto_server_send_auth_request();

// read auth message from client, returns credentials
// return 0 on success, non 0 otherwise
sint8 pproto_server_read_auth(auth_credentials *cred);

// sends authentication status: success if auth_status = 1, failure otherwise
// return 0 on success, non 0 otherwise
sint8 pproto_server_send_auth_responce(uint8 auth_status);

// sends goodbye message to client
// return 0 on success, non 0 otherwise
sint8 pproto_server_send_goodbye();



////////////////// text string processing



// begin reading text string from client
// if length is known len will be set to the length
// return 0 on success, non 0 otherwise
sint8 pproto_server_read_str_begin(uint64 *len);

// read text string chunk in str_buf of size sz
// sz will be updated with total bytes read, and charlen will be set to numbe of characters read
// if not more data sz is set to 0
// return 0 on success, non 0 otherwise
sint8 pproto_server_read_str(uint8 *str_buf, uint64 *sz, uint64 *charlen);

// get the next character in server encoding sent by client
// if no more data eos is set to 1
// return 0 on success, non 0 otherwise
sint8 pproto_server_read_char(char_info *ch, sint8 *eos);

// finish reading the string from client
// if string is not fully read, cancel message will be sent to client and rest of the string will be skipped
// return 0 on success, non 0 otherwise
sint8 pproto_server_read_str_end();

// begin sending text string to client
// return 0 on success, non 0 otherwise
sint8 pproto_server_send_str_begin();

// send text string chunk in str_buf of size sz
// if srv_enc is not 0 server encoding is expected, otherwise UTF-8 is assumed (source code enc)
// data is not guarantied to be flushed after call finishes
// return 0 on success, non 0 otherwise
sint8 pproto_server_send_str(const uint8 *str_buf, sint32 sz, uint8 srv_enc);

// finish sending the string to client
// data is flushed after call finishes
// return 0 on success, non 0 otherwise
sint8 pproto_server_send_str_end();



////////////////// other


// return next message type or -1 on error
pproto_msg_type pproto_server_read_msg_type();

// sends error defined by errcode and additional message to client
// msg is optional, can be NULL, expected encoding is UTF-8 (source code enc)
// return 0 on success, non 0 otherwise
sint8 pproto_server_send_error(error_code errcode, const achar *msg);


#endif
