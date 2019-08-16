#ifndef _PPROTO_CLIENT_H
#define _PPROTO_CLIENT_H

// Client side protocol

#include "common/encoding.h"
#include "common/error.h"
#include "table/table.h"
#include "auth/auth.h"
#include "common/decimal.h"

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
    PPROTO_ERROR_MSG = 2,
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


// recordset description
typedef struct _pproto_col_desc
{
    uint8               nullable;
    uint8               data_type_precision;
    uint8               data_type_scale;
    uint16              col_alias_sz;
    column_datatype     data_type;
    uint64              data_type_len;
    uint8               col_alias[MAX_TABLE_COL_NAME_LEN];
} pproto_col_desc;


// read column descriptor in recordset
// return 0 on success, non 0 otherwise
sint8 pproto_read_recordset_col_desc(int sock, pproto_col_desc *col_desc);

// read number of columns in recordset
// return 0 on success, non 0 otherwise
sint8 pproto_read_recordset_col_num(int sock, uint16 *n);

// begin reading row in recordset
// fills bitmap "nulls" with null value mask (0 - value is null for column, 1 - value is present)
// return 0 if no rows present anymore, 1 if there is row, -1 on error
sint8 pproto_recordset_start_row(int sock, uint8 *nulls);


// initiate string read
// return 0 on success, non 0 otherwise
sint8 pproto_read_str_begin(int sock);

// read string data by chunks into strbuf of length sz
// set sz to actually read bytes size
// return 0 on success, non 0 otherwise
sint8 pproto_read_str(int sock, uint8 *strbuf, uint32 *sz);

// finish reading string
// set sz to actually read bytes size
// return 0 on success, non 0 otherwise
sint8 pproto_read_str_end(int sock);

// read date value
// return 0 on success, non 0 otherwise
sint8 pproto_read_date_value(int sock, uint64 *date);

// read decimal value
// return 0 on success, non 0 otherwise
sint8 pproto_read_decimal_value(int sock, decimal *d);

// read 8-byte float value
// return 0 on success, non 0 otherwise
sint8 pproto_read_double_value(int sock, float64 *d);

// read 4-byte float value
// return 0 on success, non 0 otherwise
sint8 pproto_read_float_value(int sock, float32 *f);

// read 4-byte integer
// return 0 on success, non 0 otherwise
sint8 pproto_read_integer_value(int sock, sint32 *i);

// read 2-byte integer
// return 0 on success, non 0 otherwise
sint8 pproto_read_smallint_value(int sock, sint16 *s);

// read timestamp value
// return 0 on success, non 0 otherwise
sint8 pproto_read_timestamp_value(int sock, uint64 *ts);

// read timestamp with timezone value
// return 0 on success, non 0 otherwise
sint8 pproto_read_timestamp_with_tz_value(int sock, uint64 *ts, sint16 *tz);

// return next message type or -1 on error
pproto_msg_type pproto_read_msg_type(int sock);

// read server hello message
// return 0 on success, non 0 otherwise
sint8 pproto_read_server_hello(int sock);

// read authentication request
// return 0 on success, non 0 otherwise
sint8 pproto_read_auth_request(int sock);

// read authentication status: success if auth_status = 1, failure otherwise
// return 0 on success, non 0 otherwise
sint8 pproto_read_auth_responce(int sock, uint8 *auth_status);

// reads error to errcode
// return 0 on success, non 0 otherwise
sint8 pproto_read_error(int sock, error_code *errcode);


// send client hello message
// return 0 on success, non 0 otherwise
sint8 pproto_send_client_hello(int sock, encoding client_encoding);

// send auth message with auth credentials
// return 0 on successful authentication, 1 on wrong credentials, -1 on error
sint8 pproto_send_auth(int sock, auth_credentials *cred);

// sends goodbye message to client
// return 0 on success, non 0 otherwise
sint8 pproto_send_goodbye(int sock);


// start sending sql statement
// return 0 on success, 1 on error
sint8 pproto_sql_stmt_begin(int sock);

// send sql statement data
// return 0 on success, 1 on error
sint8 pproto_send_sql_stmt(int sock, const uint8* data, uint32 sz);

// finish sending sql statement
// return 0 on success, 1 on error
sint8 pproto_sql_stmt_finish(int sock);

// send cancel message to cancel running statement
// return 0 on success, non 0 otherwise
sint8 pproto_send_cancel(int sock);

// return string of the last error
const char *pproto_last_error_msg();

#endif
