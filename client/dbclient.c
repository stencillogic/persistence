#include "client/dbclient.h"
#include "client/pproto_client.h"
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>


#define DBCLIENT_MAX_ERRMES     (1024)
#define DBCLIENT_MAX_COLUMNS    (1000)
#define DBCLIENT_ROW_NULLS_SZ   (128)


typedef enum
{
    DBCLIENT_STATE_DISCONNECTED,
    DBCLIENT_STATE_CONNECTED,
    DBCLIENT_STATE_AUTHENTICATED,
    DBCLIENT_STATE_STATEMENT,
    DBCLIENT_STATE_EXECUTION,
    DBCLIENT_STATE_RECORDSET,
    DBCLIENT_STATE_FETCH
} dbclient_state;


typedef sint8 (*pproto_client_read_value_fp)(int, void*);


struct _g_dbclient_state
{
    uint8                   initialized;
    pproto_client_read_value_fp    read_value[7];
    uint8                   bits[8];
} g_dbclient_state = {.initialized = 0};


typedef struct _dbclient_session
{
    int             sock;
    dbclient_state  state;
    encoding        enc;
    FILE           *err_stream;
    uint16          rs_col_num;
    uint16          col_idx;
    uint16          nullable_col_idx;
    handle          pproto_client_session;
    uint32          nulls_sz;
    char            errmes[DBCLIENT_MAX_ERRMES + 1];
    uint8           row_nulls[DBCLIENT_ROW_NULLS_SZ];
    pproto_col_desc rs_columns[DBCLIENT_MAX_COLUMNS];
} dbclient_session;


// convert socket error code to string
const char* dbclient_herrno_msg()
{
    switch(h_errno)
    {
        case HOST_NOT_FOUND:
              return "host is unknown";
        case NO_DATA:
            return "requested name does not have an IP address";
        case NO_RECOVERY:
            return "an nonrecoverable name server error occurred";
        case TRY_AGAIN:
            return "a temporary error, try again later";
    }
    return "unkonwn error";
}


// prepare error message for errno = errcode
void dbclient_std_error(dbclient_session *ss, const char *context, int errcode)
{
    size_t len = strlen(context);

    assert(len + 512 < DBCLIENT_MAX_ERRMES);

    if(len > DBCLIENT_MAX_ERRMES - 2) len = DBCLIENT_MAX_ERRMES - 2;
    memcpy(ss->errmes, context, len);
    ss->errmes[len++] = ':';
    ss->errmes[len++] = ' ';

    assert(0 == strerror_r(errcode, ss->errmes + len, DBCLIENT_MAX_ERRMES - len + 1));
    ss->errmes[DBCLIENT_MAX_ERRMES] = '\0';
}


// prepare error message in buffer
void dbclient_pproto_client_error(dbclient_session *ss, const char *context)
{
    size_t len = strlen(context), len2;
    const char *msg;

    assert(len + 10 < DBCLIENT_MAX_ERRMES);

    if(len > DBCLIENT_MAX_ERRMES - 2) len = DBCLIENT_MAX_ERRMES - 2;
    memcpy(ss->errmes, context, len);
    ss->errmes[len++] = ':';
    ss->errmes[len++] = ' ';

    msg = pproto_client_last_error_msg(ss->pproto_client_session);

    len2 = strlen(msg);
    if(len2 > DBCLIENT_MAX_ERRMES - len) len2 = DBCLIENT_MAX_ERRMES - len;
    memcpy(ss->errmes + len, msg, len2);
    len += len2;

    ss->errmes[len] = '\0';
}


// spill error to error stream if there is error stream
void dbclient_spill_errmes(dbclient_session *ss)
{
    if(NULL != ss->err_stream)
    {
        fputs(ss->errmes, ss->err_stream);
        fputc('\0', ss->err_stream);
    }
}


dbclient_return_code dbclient_server_error(dbclient_session *ss)
{
    uint64 sz = DBCLIENT_MAX_ERRMES;
    uint64 len;

    if(pproto_client_read_str_begin(ss->pproto_client_session, &len) != 0)
    {
        dbclient_pproto_client_error(ss, "Retreiving error message from server");
        dbclient_spill_errmes(ss);
        return DBCLIENT_RETURN_ERROR;
    }

    if(pproto_client_read_str(ss->pproto_client_session, (uint8 *)ss->errmes, &sz) != 0)
    {
        dbclient_pproto_client_error(ss, "Retreiving error message from server");
        dbclient_spill_errmes(ss);
        return DBCLIENT_RETURN_ERROR;
    }

    if(pproto_client_read_str_end(ss->pproto_client_session) != 0)
    {
        dbclient_pproto_client_error(ss, "Retreiving error message from server");
        dbclient_spill_errmes(ss);
        return DBCLIENT_RETURN_ERROR;
    }

    ss->errmes[sz + 1] = '\0';

    return DBCLIENT_RETURN_SUCCESS;
}


// setup connection
int dbclient_make_connection(dbclient_session *ss, const char *host, uint16_t port)
{
    int sock;
    struct sockaddr_in name;
    struct hostent *hostinfo;

    sock = socket(PF_INET, SOCK_STREAM, 0);
    if(sock < 0)
    {
        dbclient_std_error(ss, "Creating socket", errno);
        return -1;
    }

    name.sin_family = AF_INET;
    name.sin_port = htons(port);
    hostinfo = gethostbyname(host);
    if(NULL == hostinfo)
    {
        assert(snprintf(ss->errmes, DBCLIENT_MAX_ERRMES + 1, "Host name resolution: %s", dbclient_herrno_msg()) > 0);
        ss->errmes[DBCLIENT_MAX_ERRMES] = '\0';
        return -1;
    }
    name.sin_addr = *(struct in_addr *)hostinfo->h_addr;

    if(-1 == connect(sock, (struct sockaddr*)&name, sizeof(name)))
    {
        dbclient_std_error(ss, "Connecting", errno);
        return -1;
    }

    return sock;
}


void dbclient_close_connection(dbclient_session *ss)
{
    if(0 != shutdown(ss->sock, SHUT_RDWR))
    {
        dbclient_std_error(ss, "Shutting down connection", errno);
        dbclient_spill_errmes(ss);
    }

    if(0 != close(ss->sock))
    {
        dbclient_std_error(ss, "Shutting down connection", errno);
        dbclient_spill_errmes(ss);
    }

    ss->state = DBCLIENT_STATE_DISCONNECTED;
}


char *dbclient_last_error(handle session)
{
    return ((dbclient_session *)session)->errmes;
}


// close connection and process errors
void dbclient_termination_with_err(dbclient_session *ss, const char* context)
{
    dbclient_close_connection(ss);
    dbclient_pproto_client_error(ss, context);
    dbclient_spill_errmes(ss);
}

void dbclient_process_err_msg_type(dbclient_session *ss, pproto_msg_type msg_type, const char* context)
{
    if(msg_type == PPROTO_ERROR_MSG)
    {
        dbclient_server_error(ss);
    }
    else if(msg_type == PPROTO_MSG_TYPE_ERR)
    {
        dbclient_pproto_client_error(ss, context);
    }
    else
    {
        assert(0 < snprintf(ss->errmes, DBCLIENT_MAX_ERRMES + 1, "%s: unexpected message type from server %x", context, msg_type));
        ss->errmes[DBCLIENT_MAX_ERRMES] = '\0';
    }
    dbclient_spill_errmes(ss);
}

size_t dbclient_get_session_state_sz()
{
    return sizeof(dbclient_session) + pproto_client_get_alloc_size();
}


void dbclient_init()
{
    g_dbclient_state.bits[0] = 0x01;
    g_dbclient_state.bits[1] = 0x02;
    g_dbclient_state.bits[2] = 0x04;
    g_dbclient_state.bits[3] = 0x08;
    g_dbclient_state.bits[4] = 0x10;
    g_dbclient_state.bits[5] = 0x20;
    g_dbclient_state.bits[6] = 0x40;
    g_dbclient_state.bits[7] = 0x80;
    g_dbclient_state.read_value[0] = (pproto_client_read_value_fp)pproto_client_read_date_value;
    g_dbclient_state.read_value[1] = (pproto_client_read_value_fp)pproto_client_read_decimal_value;
    g_dbclient_state.read_value[2] = (pproto_client_read_value_fp)pproto_client_read_double_value;
    g_dbclient_state.read_value[3] = (pproto_client_read_value_fp)pproto_client_read_float_value;
    g_dbclient_state.read_value[4] = (pproto_client_read_value_fp)pproto_client_read_integer_value;
    g_dbclient_state.read_value[5] = (pproto_client_read_value_fp)pproto_client_read_smallint_value;
    g_dbclient_state.read_value[6] = (pproto_client_read_value_fp)pproto_client_read_timestamp_value;
    g_dbclient_state.initialized = 1;
}


handle dbclient_allocate_session(void *ssbuf, encoding enc, FILE* err_stream)
{
    if(0 == g_dbclient_state.initialized)
    {
        return NULL;
    }

    dbclient_session *ss = (dbclient_session *)ssbuf;
    ss->state = DBCLIENT_STATE_DISCONNECTED;
    ss->enc = enc;
    ss->err_stream = err_stream;

    pproto_client_create((uint8*)ssbuf + sizeof(dbclient_session), -1);

    return (handle)ss;
}


dbclient_return_code dbclient_connect(handle session, const char *host, uint16_t port)
{
    pproto_msg_type msg_type;
    dbclient_session *ss = (dbclient_session *)session;
    uint16 vminor, vmajor;

    if(DBCLIENT_STATE_DISCONNECTED != ss->state)
    {
        strncpy(ss->errmes, "Client must be in disconnected state", DBCLIENT_MAX_ERRMES + 1);
        dbclient_spill_errmes(ss);
        return DBCLIENT_RETURN_ERROR;
    }

    ss->sock = dbclient_make_connection(ss, host, port);
    if(ss->sock < 0)
    {
        dbclient_spill_errmes(ss);
        return DBCLIENT_RETURN_ERROR;
    }

    pproto_client_set_sock(ss->pproto_client_session, ss->sock);

    if(pproto_client_send_hello(ss->pproto_client_session, ss->enc) != 0)
    {
        dbclient_termination_with_err(ss, "Sending hello to server");
        return DBCLIENT_RETURN_ERROR;
    }

    msg_type = pproto_client_read_msg_type(ss->pproto_client_session);
    if(msg_type != PPROTO_SERVER_HELLO_MSG)
    {
        dbclient_process_err_msg_type(ss, msg_type, "Reading server hello");
        dbclient_close_connection(ss);
        return DBCLIENT_RETURN_ERROR;
    }

    if(0 != pproto_client_read_server_hello(ss->pproto_client_session, &vmajor, &vminor))
    {
        dbclient_termination_with_err(ss, "Reading server hello");
        return DBCLIENT_RETURN_ERROR;
    }

    if(vmajor != PPROTO_MAJOR_VERSION)
    {
        dbclient_close_connection(ss);
        snprintf(ss->errmes, DBCLIENT_MAX_ERRMES + 1, "Major protocol version differs, client^ %d, server: %d", PPROTO_MAJOR_VERSION, vmajor);
        dbclient_spill_errmes(ss);
        return DBCLIENT_RETURN_ERROR;
    }

    ss->state = DBCLIENT_STATE_CONNECTED;

    return DBCLIENT_RETURN_SUCCESS;
}


dbclient_return_code dbclient_authenticate(handle session, const char *user, const char *password)
{
    pproto_msg_type msg_type;
    auth_credentials cred;
    uint8 responce;
    dbclient_session *ss = (dbclient_session *)session;

    if(DBCLIENT_STATE_CONNECTED != ss->state)
    {
        strncpy(ss->errmes, "Client must be connected and not authenticated", DBCLIENT_MAX_ERRMES + 1);
        dbclient_spill_errmes(ss);
        return DBCLIENT_RETURN_ERROR;
    }

    msg_type = pproto_client_read_msg_type(ss->pproto_client_session);
    if(msg_type == PPROTO_AUTH_REQUEST_MSG)
    {
        strncpy((char *)cred.user_name, user, AUTH_USER_NAME_SZ);
        auth_hash_pwd((uint8 *)password, strlen(password), cred.credentials);

        if(pproto_client_send_auth(ss->pproto_client_session, &cred) != 0)
        {
            dbclient_pproto_client_error(ss, "Authenticating to server");
            dbclient_spill_errmes(ss);
            return DBCLIENT_RETURN_ERROR;
        }
    }
    else
    {
        dbclient_process_err_msg_type(ss, msg_type, "Reading auth request");
        return DBCLIENT_RETURN_ERROR;
    }

    msg_type = pproto_client_read_msg_type(ss->pproto_client_session);
    if(msg_type == PPROTO_AUTH_RESPONCE_MSG)
    {
        if(pproto_client_read_auth_responce(ss->pproto_client_session, &responce) != 0)
        {
            dbclient_pproto_client_error(ss, "Reading auth responce");
            dbclient_spill_errmes(ss);
            return DBCLIENT_RETURN_ERROR;
        }
        else
        {
            if(1 == responce)
            {
                ss->state = DBCLIENT_STATE_AUTHENTICATED;
            }
            else
            {
                return DBCLIENT_RETURN_AUTH_FAILED;
            }
        }
    }
    else
    {
        dbclient_process_err_msg_type(ss, msg_type, "Reading auth request");
        return DBCLIENT_RETURN_ERROR;
    }

    return DBCLIENT_RETURN_SUCCESS;
}


dbclient_return_code dbclient_begin_statement(handle session)
{
    dbclient_session *ss = (dbclient_session *)session;

    if(DBCLIENT_STATE_CONNECTED != ss->state)
    {
        strncpy(ss->errmes, "Client must be authenticated and not executing statement", DBCLIENT_MAX_ERRMES + 1);
        dbclient_spill_errmes(ss);
        return DBCLIENT_RETURN_ERROR;
    }

    if(pproto_client_sql_stmt_begin(ss->pproto_client_session) != 0)
    {
        dbclient_pproto_client_error(ss, "Sending statement text to server");
        dbclient_spill_errmes(ss);
        return DBCLIENT_RETURN_ERROR;
    }

    ss->state = DBCLIENT_STATE_STATEMENT;

    return DBCLIENT_RETURN_SUCCESS;
}


dbclient_return_code dbclient_statement(handle session, const uint8 *buf, uint32 len)
{
    dbclient_session *ss = (dbclient_session *)session;
    if(DBCLIENT_STATE_STATEMENT != ss->state)
    {
        strncpy(ss->errmes, "Client must begin statement", DBCLIENT_MAX_ERRMES + 1);
        dbclient_spill_errmes(ss);
        return DBCLIENT_RETURN_ERROR;
    }

    sint8 res = pproto_client_poll(ss->pproto_client_session);
    if(0 == res)
    {
        if(pproto_client_send_sql_stmt(ss->pproto_client_session, buf, len) != 0)
        {
            dbclient_pproto_client_error(ss, "Sending statement text to server");
            dbclient_spill_errmes(ss);
            return DBCLIENT_RETURN_ERROR;
        }

        return DBCLIENT_RETURN_SUCCESS;
    }
    else if(res == 1)
    {
        pproto_msg_type msg_type = pproto_client_read_msg_type(ss->pproto_client_session);
        dbclient_process_err_msg_type(ss, msg_type, "Reading server reply");
        dbclient_close_connection(ss);
        return DBCLIENT_RETURN_ERROR;
    }

    dbclient_pproto_client_error(ss, "Polling for server reply");
    dbclient_spill_errmes(ss);
    return DBCLIENT_RETURN_ERROR;
}


dbclient_return_code dbclient_finish_statement(handle session)
{
    dbclient_session *ss = (dbclient_session *)session;
    if(DBCLIENT_STATE_CONNECTED != ss->state)
    {
        strncpy(ss->errmes, "Client must be authenticated", DBCLIENT_MAX_ERRMES + 1);
        dbclient_spill_errmes(ss);
        return DBCLIENT_RETURN_ERROR;
    }

    if(pproto_client_sql_stmt_finish(ss->pproto_client_session) != 0)
    {
        dbclient_pproto_client_error(ss, "Sending statement text to server");
        dbclient_spill_errmes(ss);
        return DBCLIENT_RETURN_ERROR;
    }

    ss->state = DBCLIENT_STATE_EXECUTION;

    return DBCLIENT_RETURN_SUCCESS;
}


dbclient_return_code dbclient_execution_status(handle session)
{
    dbclient_session *ss = (dbclient_session *)session;
    pproto_msg_type msg_type;

    if(DBCLIENT_STATE_EXECUTION != ss->state)
    {
        strncpy(ss->errmes, "Statement execution must be in progress", DBCLIENT_MAX_ERRMES + 1);
        dbclient_spill_errmes(ss);
        return DBCLIENT_RETURN_ERROR;
    }

    msg_type = pproto_client_read_msg_type(ss->pproto_client_session);
    if(msg_type == PPROTO_PROGRESS_MSG)
    {
        return DBCLIENT_RETURN_IN_PROGRESS;
    }
    else if(msg_type == PPROTO_RECORDSET_MSG)
    {
        return DBCLIENT_RETURN_SUCCESS_RS;
    }
    else if(msg_type == PPROTO_SUCCESS_WITH_TEXT_MSG)
    {
        return DBCLIENT_RETURN_SUCCESS_MSG;
    }
    else if(msg_type == PPROTO_SUCCESS_WITHOUT_TEXT_MSG)
    {
        return DBCLIENT_RETURN_SUCCESS;
    }
    else
    {
        dbclient_process_err_msg_type(ss, msg_type, "Retreiving execution status");
        return DBCLIENT_RETURN_ERROR;
    }

    return DBCLIENT_RETURN_ERROR;
}


dbclient_return_code dbclient_cancel_statement(handle session)
{
    dbclient_session *ss = (dbclient_session *)session;
    if(!(DBCLIENT_STATE_STATEMENT == ss->state ||
         DBCLIENT_STATE_EXECUTION == ss->state ||
         DBCLIENT_STATE_RECORDSET == ss->state ||
         DBCLIENT_STATE_FETCH == ss->state))
    {
        strncpy(ss->errmes, "Client must be authenticated", DBCLIENT_MAX_ERRMES + 1);
        dbclient_spill_errmes(ss);
        return DBCLIENT_RETURN_ERROR;
    }

    if(pproto_client_send_cancel(ss->pproto_client_session) != 0)
    {
        dbclient_pproto_client_error(ss, "Canceling statement");
        dbclient_spill_errmes(ss);
        return DBCLIENT_RETURN_ERROR;
    }

    ss->state = DBCLIENT_STATE_AUTHENTICATED;

    return DBCLIENT_RETURN_SUCCESS;
}


dbclient_return_code dbclient_begin_recordset(handle session)
{
    dbclient_session *ss = (dbclient_session *)session;
    uint16 c;

    if(DBCLIENT_STATE_EXECUTION != ss->state)
    {
        strncpy(ss->errmes, "No available recordset", DBCLIENT_MAX_ERRMES + 1);
        dbclient_spill_errmes(ss);
        return DBCLIENT_RETURN_ERROR;
    }

    if(pproto_client_read_recordset_col_num(ss->pproto_client_session, &(ss->rs_col_num)) != 0)
    {
        dbclient_pproto_client_error(ss, "Retreiving recordset description");
        dbclient_spill_errmes(ss);
        return DBCLIENT_RETURN_ERROR;
    }

    if(ss->rs_col_num > DBCLIENT_MAX_COLUMNS)
    {
        snprintf(ss->errmes, DBCLIENT_MAX_ERRMES + 1,
                "Recordset continas %d columns. Only %d columns will be displayed.\n",
                ss->rs_col_num, DBCLIENT_MAX_COLUMNS);
        dbclient_spill_errmes(ss);
        ss->rs_col_num = DBCLIENT_MAX_COLUMNS;
    }

    ss->nulls_sz = 0;
    for(c = 0; c < ss->rs_col_num; c++)
    {
        if(pproto_client_read_recordset_col_desc(ss->pproto_client_session, &(ss->rs_columns[c])) != 0)
        {
            dbclient_pproto_client_error(ss, "Retreiving recordset description");
            dbclient_spill_errmes(ss);
            return DBCLIENT_RETURN_ERROR;
        }

        if(1 == ss->rs_columns[c].nullable)
        {
            ss->nulls_sz++;
        }
    }
    ss->nulls_sz += 7;
    ss->nulls_sz /= 8;

    ss->state = DBCLIENT_STATE_RECORDSET;

    return DBCLIENT_RETURN_SUCCESS;
}


dbclient_return_code dbclient_get_column_count(handle session, uint16 *col_num)
{
    dbclient_session *ss = (dbclient_session *)session;

    if(!(DBCLIENT_STATE_RECORDSET == ss->state ||
         DBCLIENT_STATE_FETCH == ss->state))
    {
        strncpy(ss->errmes, "No available recordset", DBCLIENT_MAX_ERRMES + 1);
        dbclient_spill_errmes(ss);
        return DBCLIENT_RETURN_ERROR;
    }

    (*col_num) = ss->rs_col_num;

    return DBCLIENT_RETURN_SUCCESS;
}


const pproto_col_desc *dbclient_get_column_desc(handle session, uint16 c)
{
    dbclient_session *ss = (dbclient_session *)session;

    if(!(DBCLIENT_STATE_RECORDSET == ss->state ||
         DBCLIENT_STATE_FETCH == ss->state))
    {
        strncpy(ss->errmes, "No available recordset", DBCLIENT_MAX_ERRMES + 1);
        dbclient_spill_errmes(ss);
        return NULL;
    }

    if(c > ss->rs_col_num)
    {
        strncpy(ss->errmes, "No available recordset", DBCLIENT_MAX_ERRMES + 1);
        dbclient_spill_errmes(ss);
        return NULL;
    }

    return ss->rs_columns + c;
}


dbclient_return_code dbclient_fetch_row(handle session)
{
    dbclient_return_code ret = DBCLIENT_RETURN_ERROR;
    dbclient_session *ss = (dbclient_session *)session;

    if(!(DBCLIENT_STATE_RECORDSET == ss->state ||
         DBCLIENT_STATE_FETCH == ss->state))
    {
        strncpy(ss->errmes, "No available recordset", DBCLIENT_MAX_ERRMES + 1);
        dbclient_spill_errmes(ss);
        return DBCLIENT_RETURN_ERROR;
    }

    switch(pproto_client_recordset_start_row(ss->pproto_client_session, ss->row_nulls, ss->nulls_sz))
    {
        case 0:
            ret = DBCLIENT_RETURN_NO_MORE_ROWS;
            break;
        case 1:
            ret = DBCLIENT_RETURN_SUCCESS;
            break;
        case -1:
            dbclient_pproto_client_error(ss, "Fetching the row");
            dbclient_spill_errmes(ss);
            ret = DBCLIENT_RETURN_ERROR;
            break;
        default:
            break;
    }

    ss->col_idx = ss->nullable_col_idx = 0;
    ss->state = DBCLIENT_STATE_FETCH;

    return ret;
}


dbclient_return_code dbclient_next_col_val(handle session, dbclient_value *val)
{
    dbclient_session *ss = (dbclient_session *)session;
    column_datatype dt;
    uint16 col_idx, i;
    sint8 ret;
    uint64 len;

    if(DBCLIENT_STATE_FETCH != ss->state)
    {
        strncpy(ss->errmes, "No fetched row", DBCLIENT_MAX_ERRMES + 1);
        dbclient_spill_errmes(ss);
        return DBCLIENT_RETURN_ERROR;
    }

    if(ss->col_idx == ss->rs_col_num)
    {
        strncpy(ss->errmes, "No more columns available", DBCLIENT_MAX_ERRMES + 1);
        dbclient_spill_errmes(ss);
        return DBCLIENT_RETURN_ERROR;
    }

    val->isnull = 0;
    col_idx = ss->col_idx;
    if(ss->rs_columns[col_idx].nullable)
    {
        i = ss->nullable_col_idx;
        if(ss->row_nulls[i / 8] & g_dbclient_state.bits[i % 8])
        {
            val->isnull = 1;
        }
        ss->nullable_col_idx++;
    }

    if(0 == val->isnull)
    {
        dt = ss->rs_columns[col_idx].data_type;
        switch(dt)
        {
            case CHARACTER_VARYING:
                if(0 == (ret = pproto_client_read_str_begin(ss->pproto_client_session, &len)))
                {
                    if(0 == (ret = pproto_client_read_str(ss->pproto_client_session, val->str.buf, &(val->str.sz))))
                    {
                        ret = pproto_client_read_str_end(ss->pproto_client_session);
                    }
                }
                break;
            case TIMESTAMP_WITH_TZ:
                ret = pproto_client_read_timestamp_with_tz_value(ss->pproto_client_session, &val->ts_with_tz.ts, &val->ts_with_tz.tz);
                break;
            default:
                ret = g_dbclient_state.read_value[dt](ss->sock, (void *)val);
                break;
        }

        if(ret != 0)
        {
            dbclient_pproto_client_error(ss, "Fetching value");
            dbclient_spill_errmes(ss);
            return DBCLIENT_RETURN_ERROR;
        }
    }

    ss->col_idx++;

    return DBCLIENT_RETURN_SUCCESS;
}


dbclient_return_code dbclient_close_recordset(handle session)
{
    dbclient_session *ss = (dbclient_session *)session;

    if(!(DBCLIENT_STATE_RECORDSET == ss->state ||
         DBCLIENT_STATE_FETCH == ss->state))
    {
        strncpy(ss->errmes, "No open recordset", DBCLIENT_MAX_ERRMES + 1);
        dbclient_spill_errmes(ss);
        return DBCLIENT_RETURN_ERROR;
    }

    if(pproto_client_send_cancel(ss->pproto_client_session) != 0)
    {
        dbclient_pproto_client_error(ss, "Closing recordset");
        dbclient_spill_errmes(ss);
        return DBCLIENT_RETURN_ERROR;
    }

    ss->state = DBCLIENT_STATE_AUTHENTICATED;

    return DBCLIENT_RETURN_SUCCESS;
}


dbclient_return_code dbclient_close_session(handle session, uint8 force)
{
    dbclient_session *ss = (dbclient_session *)session;

    if(DBCLIENT_STATE_DISCONNECTED != ss->state)
    {
        if(pproto_client_send_goodbye(ss->pproto_client_session) != 0)
        {
            dbclient_pproto_client_error(ss, "Closing session");
            dbclient_spill_errmes(ss);
            if(0 == force)
            {
                return DBCLIENT_RETURN_ERROR;
            }
        }

        dbclient_close_connection(ss);

        ss->state = DBCLIENT_STATE_DISCONNECTED;
    }

    return DBCLIENT_RETURN_SUCCESS;
}

