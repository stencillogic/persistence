#ifndef _PPROTO_CLIENT_H
#define _PPROTO_CLIENT_H


// Client side low-level protocol interface


#include "common/pproto.h"
#include "common/encoding.h"
#include "common/error.h"
#include "auth/auth.h"
#include "common/decimal.h"


// get allocation size required to create pproto session
size_t pproto_client_get_alloc_size();

// create pproto session
handle pproto_client_create(void *ss_buf, int sock);

// set socket
void pproto_client_set_sock(handle ss, int sock);

// read column descriptor in recordset
// return 0 on success, non 0 otherwise
sint8 pproto_client_read_recordset_col_desc(handle ss, pproto_col_desc *col_desc);

// read number of columns in recordset
// return 0 on success, non 0 otherwise
sint8 pproto_client_read_recordset_col_num(handle ss, uint16 *n);

// begin reading row in recordset
// fills bitmap "nulls" with null value mask (0 - value is null for column, 1 - value is present)
// return 0 if no rows present anymore, 1 if there is row, -1 on error
sint8 pproto_client_recordset_start_row(handle ss, uint8 *nulls, uint32 nulls_sz);


// initiate string read
// if string length is known len is updated with the total string length
// return 0 on success, non 0 otherwise
sint8 pproto_client_read_str_begin(handle ss, uint64 *len);

// read string data by chunks into strbuf of length sz
// set sz to actually read bytes size
// return 0 on success, non 0 otherwise
sint8 pproto_client_read_str(handle ss, uint8 *strbuf, uint64 *sz);

// finish reading string
// set sz to actually read bytes size
// return 0 on success, non 0 otherwise
sint8 pproto_client_read_str_end(handle ss);

// read date value
// return 0 on success, non 0 otherwise
sint8 pproto_client_read_date_value(handle ss, uint64 *date);

// read decimal value
// return 0 on success, non 0 otherwise
sint8 pproto_client_read_decimal_value(handle ss, decimal *d);

// read 8-byte float value
// return 0 on success, non 0 otherwise
sint8 pproto_client_read_double_value(handle ss, float64 *d);

// read 4-byte float value
// return 0 on success, non 0 otherwise
sint8 pproto_client_read_float_value(handle ss, float32 *f);

// read 4-byte integer
// return 0 on success, non 0 otherwise
sint8 pproto_client_read_integer_value(handle ss, sint32 *i);

// read 2-byte integer
// return 0 on success, non 0 otherwise
sint8 pproto_client_read_smallint_value(handle ss, sint16 *s);

// read timestamp value
// return 0 on success, non 0 otherwise
sint8 pproto_client_read_timestamp_value(handle ss, uint64 *ts);

// read timestamp with timezone value
// return 0 on success, non 0 otherwise
sint8 pproto_client_read_timestamp_with_tz_value(handle ss, uint64 *ts, sint16 *tz);

// return next message type or -1 on error
pproto_msg_type pproto_client_read_msg_type(handle ss);

// check if htere is data ready to read
// return 0 on no data, 1 on data ready to read, -1 on error
sint8 pproto_client_poll(handle ss);

// read authentication status: success if auth_status = 1, failure otherwise
// return 0 on success, non 0 otherwise
sint8 pproto_client_read_auth_responce(handle ss, uint8 *auth_status);


// send client hello message
// return 0 on success, non 0 otherwise
sint8 pproto_client_send_hello(handle ss, encoding client_encoding);

// send auth message with auth credentials
// return 0 on successful authentication, 1 on wrong credentials, -1 on error
sint8 pproto_client_send_auth(handle ss, auth_credentials *cred);

// sends goodbye message to client
// return 0 on success, non 0 otherwise
sint8 pproto_client_send_goodbye(handle ss);


// start sending sql statement
// return 0 on success, 1 on error
sint8 pproto_client_sql_stmt_begin(handle ss);

// send sql statement data
// return 0 on success, 1 on error
sint8 pproto_client_send_sql_stmt(handle ss, const uint8* data, uint32 sz);

// finish sending sql statement
// return 0 on success, 1 on error
sint8 pproto_client_sql_stmt_finish(handle ss);

// send cancel message to cancel running statement
// return 0 on success, non 0 otherwise
sint8 pproto_client_send_cancel(handle ss);

// return string of the last error
const char *pproto_client_last_error_msg(handle ss);

// read protocol major and minor versions
// return 0 on success, non-0 on error
sint8 pproto_client_read_server_hello(handle ss, uint16 *vmajor, uint16 *vminor);


#endif
