#ifndef _PPROTO_SERVER_H
#define _PPROTO_SERVER_H

// server-side protocol

#include "common/encoding.h"
#include "common/error.h"
#include "auth/auth.h"

#define PPROTO_MAJOR_VERSION 0x0001u
#define PPROTO_MINOR_VERSION 0x0001u

// different magics
#define PPROTO_TEXT_STRING_TERMINATOR 0x00u
#define PPROTO_WTEXT_STRING_TERMINATOR 0x0000u
#define PPROTO_RECORDSET_END 0x88u
#define PPROTO_AUTH_SUCCESS 0xCCu
#define PPROTO_AUTH_FAIL 0xFFu
#define PPROTO_GOODBYE_MESSAGE 0xBEu
#define PPROTO_RECORDSET_ROW_MAGIC 0x06u

// data type magics
#define PPROTO_UTEXT_STRING_MAGIC 0x01u
#define PPROTO_LTEXT_STRING_MAGIC 0xFEu
#define PPROTO_NUMERIC_VALUE_MAGIC 0x02u
#define PPROTO_INTEGER_VALUE_MAGIC 0x03u
#define PPROTO_SMALLINT_VALUE_MAGIC 0x04u
#define PPROTO_FLOAT_VALUE_MAGIC 0x05u
#define PPROTO_DOUBLE_PRECISION_VALUE_MAGIC 0x06u
#define PPROTO_DATE_VALUE_MAGIC 0x07u
#define PPROTO_TIMESTAMP_VALUE_MAGIC 0x08u

// server message magics
#define PPROTO_SERVER_HELLO_MAGIC 0x1985u
#define PPROTO_ERROR_MSG_MAGIC 0x0Fu
#define PPROTO_SUCCESS_MESSAGE_WITH_TEXT_MAGIC 0xF01u
#define PPROTO_SUCCESS_MESSAGE_WITHOUT_TEXT_MAGIC 0xF02u
#define PPROTO_RECORDSET_MESSAGE_MAGIC 0xFFu
#define PPROTO_AUTH_REQUEST_MESSAGE_MAGIC 0x11u
#define PPROTO_AUTH_RESPONCE_MESSAGE_MAGIC 0x33u
#define PPROTO_PROGRESS_MESSAGE_MAGIC 0x44u

// client message magics
#define PPROTO_CLIENT_HELLO_MAGIC 0x1406u
#define PPROTO_AUTH_MESSAGE_MAGIC 0x22u
#define PPROTO_SQL_REQUEST_MESSAGE_MAGIC 0x55u
#define PPROTO_CANCEL_MESSAGE_MAGIC 0x57u

typedef enum _pproto_msg_type
{
    PPROTO_MSG_TYPE_ERR = -1,
    PPROTO_UNKNOWN_MSG = 0,
    PPROTO_SERVER_HELLO_MSG = 1,
    PPROTO_ERROR_MSG_MSG = 2,
    PPROTO_SUCCESS_WITH_TEXT_MSG = 3,
    PPROTO_SUCCESS_WITHOUT_TEXT_MSG = 4,
    PPROTO_RECORDSET_MSG = 5,
    PPROTO_AUTH_REQUEST_MSG = 6,
    PPROTO_AUTH_RESPONCE_MSG = 7,
    PPROTO_PROGRESS_MSG = 8,
    PPROTO_CLIENT_HELLO_MSG = 9,
    PPROTO_AUTH_MSG = 10,
    PPROTO_SQL_REQUEST_MSG = 11,
    PPROTO_CANCEL_MSG = 12,
    PPROTO_GOODBYE_MSG = 13
} pproto_msg_type;


// set socket to work with
void pproto_set_sock(int client_sock);

// set client encoding
void pproto_set_encoding(encoding client_encoding);

// sends error defined by errcode to client
// return 0 on success, non 0 otherwise
sint8 pproto_send_error(error_code errcode);

// sends goodbye message to client
// return 0 on success, non 0 otherwise
sint8 pproto_send_goodbye();

sint8 pproto_send_text_string(const achar *str);

sint8 pproto_send_success_message_with_text();

sint8 pproto_send_success_message_without_text();

sint8 pproto_send_recordset();

sint8 pproto_send_recordset_row();

sint8 pproto_send_numeric_value();

sint8 pproto_send_timestamp_value();

sint8 pproto_send_smallint_value();

sint8 pproto_send_integer_value();

sint8 pproto_send_float_value();

sint8 pproto_send_double_precision_value();

sint8 pproto_send_date_value();


// sends server hello message
// return 0 on success, non 0 otherwise
sint8 pproto_send_server_hello();

// sends authentication request
// return 0 on success, non 0 otherwise
sint8 pproto_send_auth_request();

// sends authentication status: success if auth_status = 1, failure otherwise
// return 0 on success, non 0 otherwise
sint8 pproto_send_auth_responce(uint8 auth_status);

// read auth message from client, returns credentials
// return 0 on successful authentication, 1 on wrong credentials, -1 on error
sint8 pproto_read_auth(auth_credentials *cred);

// read sql message from client
// return 0 on success, non 0 otherwise
sint8 pproto_read_sql_request();

// read cancel message from client
// return 0 on success, non 0 otherwise
sint8 pproto_read_cancel();

// read hello message from client
// return 0 on success, non 0 otherwise
sint8 pproto_read_client_hello();

// return next message type or -1 on error
pproto_msg_type pproto_read_msg_type();

#endif
