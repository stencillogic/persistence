#include "tests.h"
#include "client/dbclient.h"
#include "common/encoding.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>


#ifndef TEST_DBCLIENT_SERVER_PORT
#define TEST_DBCLIENT_SERVER_PORT   (3000)
#endif



// test client-side db driver functions
int test_dbclient_functions_sub(FILE *err_stream)
{
    size_t ss_buf_sz;
    void *ss_buf;
    handle ss;
    encoding enc = ENCODING_UTF8;


    puts("Tesing client connection and auth");


    ss_buf_sz = dbclient_get_session_state_sz();

    ss_buf = malloc(ss_buf_sz);
    assert(ss_buf != NULL);

    ss = dbclient_allocate_session(ss_buf, enc, err_stream);
    if(NULL == ss) return __LINE__;


    puts("Tesing sql statement");


    puts("Tesing recordset processing");


    free(ss_buf);

    return 0;
}

int test_dbclient_functions()
{
    int line;

    // try non-initialized dbclient
    handle ss = dbclient_allocate_session(NULL, 0, stderr);
    if(NULL != ss) return __LINE__;

    dbclient_init();

    line = test_dbclient_functions_sub(NULL);
    if(line != 0) return line;

    line = test_dbclient_functions_sub(stderr);
    if(line != 0) return line;

    return 0;
}
/*
// setup session with server <host:port> using encoding enc
// return DBCLIENT_RETURN_SUCCESS on successful completion or DBCLIENT_RETURN_ERROR on error
dbclient_return_code dbclient_connect(handle session, const char *host, uint16_t port);

// try authentication in session ss with user and password
// return DBCLIENT_RETURN_SUCCESS on successful completion or DBCLIENT_RETURN_ERROR on error
// or DBCLIENT_RETURN_AUTH_FAILED on authentication failure (invalid user / password)
dbclient_return_code dbclient_authenticate(handle session, const char *user, const char *password);

// begin sql statement
// return DBCLIENT_RETURN_SUCCESS on successful completion or DBCLIENT_RETURN_ERROR on error
dbclient_return_code dbclient_begin_statement(handle session);

// send part of or whole sql statement to server
// return DBCLIENT_RETURN_SUCCESS on successful completion or DBCLIENT_RETURN_ERROR on error
dbclient_return_code dbclient_statement(handle session, const uint8 *buf, uint32 len);

// tell server that statement is complete and to start execution
// return DBCLIENT_RETURN_SUCCESS on successful completion or DBCLIENT_RETURN_ERROR on error
dbclient_return_code dbclient_finish_statement(handle session);

// get execution status from the server
// return DBCLIENT_RETURN_SUCCESS if statement completed successfully, DBCLIENT_RETURN_ERROR on error
// or DBCLIENT_RETURN_IN_PROGRESS if statement is in progress
// or DBCLIENT_RETURN_SUCCESS_MSG if statement completed successfully and there is text message from server
// or DBCLIENT_RETURN_SUCCESS_RS if statement completed successfully and there is resulting recordset
dbclient_return_code dbclient_execution_status(handle session);

// stop statement execution (or close recordset if statement is complete)
// return DBCLIENT_RETURN_SUCCESS on successful completion or DBCLIENT_RETURN_ERROR on error
dbclient_return_code dbclient_cancel_statement(handle session);

// begin reading resulting recordset after statement is complete
// return DBCLIENT_RETURN_SUCCESS on successful completion or DBCLIENT_RETURN_ERROR on error
dbclient_return_code dbclient_begin_recordset(handle session);

// get the number of columns in the recordset
// return DBCLIENT_RETURN_SUCCESS on successful completion or DBCLIENT_RETURN_ERROR on error
dbclient_return_code dbclient_get_column_count(handle session, uint16 *col_num);

// get description of certain column by column index c
// return column description or NULL on error
const pproto_col_desc *dbclient_get_column_desc(handle session, uint16 c);

// start fetching row
// return DBCLIENT_RETURN_SUCCESS on successful completion or DBCLIENT_RETURN_ERROR on error
// or DBCLIENT_RETURN_NO_MORE_ROWS if there are no more rows in the recordset
dbclient_return_code dbclient_fetch_row(handle session);

// return value of the next column in a row
// return DBCLIENT_RETURN_SUCCESS on successful completion or DBCLIENT_RETURN_ERROR on error
dbclient_return_code dbclient_next_col_val(handle session, dbclient_value *val);

// return DBCLIENT_RETURN_SUCCESS on successful completion or DBCLIENT_RETURN_ERROR on error
dbclient_return_code dbclient_close_recordset(handle session);

// return DBCLIENT_RETURN_SUCCESS on successful completion or DBCLIENT_RETURN_ERROR on error
dbclient_return_code dbclient_close_session(handle session, uint8 force);

// return message of last error
char *dbclient_last_error(handle session);
*/
