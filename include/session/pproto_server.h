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
void pproto_server_set_encoding(encoding client_encoding);

// sends error defined by errcode and additional message to client
// msg is optional, can be NULL, expected encoding is UTF-8
// return 0 on success, non 0 otherwise
sint8 pproto_server_send_error(error_code errcode, const achar *msg);

// sends goodbye message to client
// return 0 on success, non 0 otherwise
sint8 pproto_server_send_goodbye();

sint8 pproto_server_send_text_string(const achar *str);

sint8 pproto_server_send_success_message_with_text();

sint8 pproto_server_send_success_message_without_text();

sint8 pproto_server_send_recordset();

sint8 pproto_server_send_recordset_row();

sint8 pproto_server_send_numeric_value();

sint8 pproto_server_send_timestamp_value();

sint8 pproto_server_send_smallint_value();

sint8 pproto_server_send_integer_value();

sint8 pproto_server_send_float_value();

sint8 pproto_server_send_double_precision_value();

sint8 pproto_server_send_date_value();


// sends server hello message
// return 0 on success, non 0 otherwise
sint8 pproto_server_send_server_hello();

// sends authentication request
// return 0 on success, non 0 otherwise
sint8 pproto_server_send_auth_request();

// sends authentication status: success if auth_status = 1, failure otherwise
// return 0 on success, non 0 otherwise
sint8 pproto_server_send_auth_responce(uint8 auth_status);

// read auth message from client, returns credentials
// return 0 on successful authentication, 1 on wrong credentials, -1 on error
sint8 pproto_server_read_auth(auth_credentials *cred);

// read sql message from client
// return 0 on success, non 0 otherwise
sint8 pproto_server_read_sql_request();

// get the next character sent by client client in server encoding
sint8 pproto_server_read_char(char_info *ch, sint8 *eos);

// read cancel message from client
// return 0 on success, non 0 otherwise
sint8 pproto_server_read_cancel();

// read hello message from client
// return 0 on success, non 0 otherwise
sint8 pproto_server_read_client_hello();

// return next message type or -1 on error
pproto_msg_type pproto_server_read_msg_type();

#endif
