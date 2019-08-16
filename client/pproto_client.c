#include "client/pproto_client.h"



sint8 pproto_read_recordset_col_desc(int sock, pproto_col_desc *col_desc)
{
    return 0;
}



sint8 pproto_read_recordset_col_num(int sock, uint16 *n)
{
    return 0;
}



sint8 pproto_recordset_start_row(int sock, uint8 *nulls)
{
    return 0;
}



sint8 pproto_read_str_begin(int sock)
{
    return 0;
}



sint8 pproto_read_str(int sock, uint8 *strbuf, uint32 *sz)
{
    return 0;
}



sint8 pproto_read_str_end(int sock)
{
    return 0;
}



sint8 pproto_read_date_value(int sock, uint64 *date)
{
    return 0;
}



sint8 pproto_read_decimal_value(int sock, decimal *d)
{
    return 0;
}



sint8 pproto_read_double_value(int sock, float64 *d)
{
    return 0;
}



sint8 pproto_read_float_value(int sock, float32 *f)
{
    return 0;
}



sint8 pproto_read_integer_value(int sock, sint32 *i)
{
    return 0;
}



sint8 pproto_read_smallint_value(int sock, sint16 *s)
{
    return 0;
}



sint8 pproto_read_timestamp_value(int sock, uint64 *ts)
{
    return 0;
}



sint8 pproto_read_timestamp_with_tz_value(int sock, uint64 *ts, sint16 *tz)
{
    return 0;
}


pproto_msg_type pproto_read_msg_type(int sock)
{
    return 0;
}



sint8 pproto_read_server_hello(int sock)
{
    return 0;
}



sint8 pproto_read_auth_request(int sock)
{
    return 0;
}



sint8 pproto_read_auth_responce(int sock, uint8 *auth_status)
{
    return 0;
}



sint8 pproto_read_error(int sock, error_code *errcode)
{
    return 0;
}




sint8 pproto_send_client_hello(int sock, encoding client_encoding)
{
    return 0;
}



sint8 pproto_send_auth(int sock, auth_credentials *cred)
{
    return 0;
}



sint8 pproto_send_goodbye(int sock)
{
    return 0;
}



sint8 pproto_sql_stmt_begin(int sock)
{
    return 0;
}



sint8 pproto_send_sql_stmt(int sock, const uint8* data, uint32 sz)
{
    return 0;
}



sint8 pproto_sql_stmt_finish(int sock)
{
    return 0;
}



sint8 pproto_send_cancel(int sock)
{
    return 0;
}



const char *pproto_last_error_msg()
{
    return "No error";
}

