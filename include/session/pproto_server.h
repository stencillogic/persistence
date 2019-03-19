#ifndef _PPROTO_SERVER_H
#define _PPROTO_SERVER_H

// Interaction protocol, server side
//
// Server message BNF:
//   <server_message> ::= <error_message> | <success_message> | <recordset_message> | <progress_message> | <hello_message> |
//                        <auth_request_message> | <auth_responce_message> | <goodbye_message>
//
//   <error_message> ::= <error_msg_magic> <text_string>
//     <error_msg_magic> ::= 0x0F
//
//     <text_string> ::= <text_string_magic> { <character> } <text_string_terminator>
//     <text_string_magic> ::= 0x01
//     <character> ::= any byte except <string_terminator>
//     <text_string_terminator> ::= 0x00 | 0x0000
//
//   <success_message> ::= <success_message_with_text> | <success_message_without_text>
//     <success_message_with_text> ::= <success_message_with_text_magic> <text_string>
//     <success_message_with_text_magic> ::= 0xF01
//     <success_message_without_text> ::= <success_message_without_text_magic>
//     <success_message_without_text_magic> ::= 0xF02
//
//   <recordset_message> ::= <recordset_message_magic> <recordset_descriptor> { <recordset_row> } <recordset_end>
//     <recordset_message_magic> ::= 0xFF
//
//     <recordset_descriptor> ::= <col_num> { <col_descriptor> }
//     <col_descriptor> ::= <data_type> <col_alias>
//     <data_type> ::= <data_type_code> [ <data_type_length> | <data_type_precision> <data_type_scale> ]
//     <data_type_code> ::= one of _column_datatype enum values, uint8
//     <data_type_length> ::= uint64 value in network order - max length defined for data type
//     <data_type_precision> ::= 1-byte value of data type precision
//     <data_type_scale> ::= 1-byte value of data type scale
//     <col_alias> ::= <text_string>
//
//     <recordset_row> ::= <recordset_row_magic> { <cell_value> }
//     <recordset_row_magic> ::= 0x06
//     <cell_value> ::= <text_string> | <numeric_value> | <timestamp_value> | <smallint_value> | <integer_value> | <float_value> | <double_precision_value> | <date_value>
//     <numeric_value> ::= <numeric_value_magic> [ <numeric_value_value> ]
//     <timestamp_value> ::= <timestamp_value_magic> [ <timestamp_value_value> ]
//     <smallint_value> ::= <smallint_value_magic> [ <smallint_value_value> ]
//     <integer_value> ::= <integer_value_magic> [ <integer_value_value> ]
//     <float_value> ::= <float_value_magic> [ <float_value_value> ]
//     <double_precision_value> ::= <double_precision_value_magic> [ <double_precision_value_value> ]
//     <date_value> ::= <date_value_magic> [ <date_value_value> ]
//
//     <numeric_value_magic> ::= 0x02
//     <timestamp_value_magic> ::= 0x08
//     <smallint_value_magic> ::= 0x04
//     <integer_value_magic> ::= 0x03
//     <float_value_magic> ::= 0x05
//     <double_precision_value_magic> ::= 0x06
//     <date_value_magic> ::= 0x07
//
//     <numeric_value_value> ::= bytes representing numeric value
//     <timestamp_value_value> ::= uint64 value of microseconds
//     <smallint_value_value> ::= sint16 value
//     <integer_value_value> ::= sint32 value
//     <float_value_value> ::= float32 value
//     <double_precision_value_value> ::= float64 value
//     <date_value_value> ::= uint64 value of seconds
//
//     <recordset_end> ::= 0x88
//
//   <progress_message> ::= 0x44
//
//   <hello_message> ::= <hello_message_magic> <protocol_version> [ <text_string> ]
//     <hello_message_magic> ::= 0x1985 (network order)
//     <protocol_version> ::= <major_protocol_version> <minor_protocol_version>
//     <major_protocol_version> ::= uint16 in network order
//     <minor_protocol_version> ::= uint16 in network order
//
//   <auth_request_message> ::= <auth_request_message_magic> <salt_key>
//     <auth_request_message_magic> ::= 0x11
//     <salt_key> ::= uint64 value in network byte order
//
//   <auth_responce_message> ::= <auth_responce_message_magic> ( <auth_success> | <auth_fail> )
//     <auth_responce_message_magic> ::= 0x33
//     <auth_success> ::= 0xCC
//     <auth_fail> ::= 0xFF
//
//   <goodbye_message> ::= 0xBEEE (network order)
//
// Client message BNF:
//
//   <client_message> ::= <hello_message> | <auth_message> | <sql_request_message> | <cancel_message> | <goodbye_message>
//
//   <hello_message> ::= <hello_message_magic>
//     <hello_message_magic> ::= 0x1406 (network order)
//
//   <auth_message> ::= <auth_message_magic> | <credentials>
//     <auth_message_magic> ::= 0x22
//     <credentials> ::= <user_name> <encrypted_password>
//     <user_name> ::= <text_string>
//     <encrypted_password> ::= 64 uint8 values in network order (512 bit sha-3)
//
//   <sql_request_message> ::= <sql_request_message_magic> <sql_request>
//     <sql_request_message_magic> ::= 0x55
//     <sql_request> ::= <text_string>
//
//   <cancel_message> ::= <cancel_message_magic>
//     <cancel_message_magic> ::= 0x57
//
//   <goodbye_message> ::= 0xBEEE (network order)
//

#define PPROTO_VERSION 0x0101u

// different magics
#define PPROTO_TEXT_STRING_TERMINATOR 0x00
#define PPROTO_WTEXT_STRING_TERMINATOR 0x0000
#define PPROTO_RECORDSET_END 0x88
#define PPROTO_AUTH_SUCCESS 0xCC
#define PPROTO_AUTH_FAIL 0xFF
#define PPROTO_GOODBYE_MESSAGE 0xBEEE
#define PPROTO_RECORDSET_ROW_MAGIC 0x06

// data type magics
#define PPROTO_TEXT_STRING_MAGIC 0x01
#define PPROTO_NUMERIC_VALUE_MAGIC 0x02
#define PPROTO_INTEGER_VALUE_MAGIC 0x03
#define PPROTO_SMALLINT_VALUE_MAGIC 0x04
#define PPROTO_FLOAT_VALUE_MAGIC 0x05
#define PPROTO_DOUBLE_PRECISION_VALUE_MAGIC 0x06
#define PPROTO_DATE_VALUE_MAGIC 0x07
#define PPROTO_TIMESTAMP_VALUE_MAGIC 0x08

// server message magics
#define PPROTO_SERVER_HELLO_MAGIC 0x1985
#define PPROTO_ERROR_MSG_MAGIC 0x0F
#define PPROTO_SUCCESS_MESSAGE_WITH_TEXT_MAGIC 0xF01
#define PPROTO_SUCCESS_MESSAGE_WITHOUT_TEXT_MAGIC 0xF02
#define PPROTO_RECORDSET_MESSAGE_MAGIC 0xFF
#define PPROTO_AUTH_REQUEST_MESSAGE_MAGIC 0x11
#define PPROTO_AUTH_RESPONCE_MESSAGE_MAGIC 0x33
#define PPROTO_PROGRESS_MESSAGE_MAGIC 0x44

// client message magics
#define PPROTO_CLIENT_HELLO_MAGIC 0x1406
#define PPROTO_AUTH_MESSAGE_MAGIC 0x22
#define PPROTO_SQL_REQUEST_MESSAGE_MAGIC 0x55
#define PPROTO_CANCEL_MESSAGE_MAGIC 0x57

sint8 pproto_parse_message();

sint8 pproto_send_error();
sint8 pproto_send_text_string();
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
sint8 pproto_send_hello();
sint8 pproto_send_auth_request();
sint8 pproto_send_auth_responce();

sint8 pproto_parse_auth_message();
sint8 pproto_parse_sql_request_message();
sint8 pproto_parse_cancel_message();
sint8 pproto_parse_hello_message();

#endif
