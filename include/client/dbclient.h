#ifndef _DBCLIENT_H
#define _DBCLIENT_H

// Client side functions (wrapper of low-level pproto calls)
//
// DBClient has underlying state machine:
//
// X - not initialized
// D - disconnected
// C - connected
// A - authenticated
// S - sending statement
// E - statement execution
// R - recordset ready
// F - fetching rows
//
// func                            | X | D | C | A | S | E | R | F |
// -----------------------------------------------------------------
// dbclient_get_session_state_sz   | X |   |   |   |   |   |   |   |
// dbclient_allocate_session       | D |   |   |   |   |   |   |   |
// dbclient_connect                |   | C |   |   |   |   |   |   |
// dbclient_authenticate           |   |   | A |   |   |   |   |   |
// dbclient_begin_statement        |   |   |   | S |   |   |   |   |
// dbclient_statement              |   |   |   |   | S |   |   |   |
// dbclient_finish_statement       |   |   |   |   | E |   |   |   |
// dbclient_execution_status       |   |   |   |   |   | E |   |   |
// dbclient_cancel_statement       |   |   |   |   | A | A | A | A |
// dbclient_begin_recordset        |   |   |   |   |   | R |   |   |
// dbclient_get_column_count       |   |   |   |   |   |   | R | F |
// dbclient_get_column_desc        |   |   |   |   |   |   | R | F |
// dbclient_fetch_row              |   |   |   |   |   |   | F | F |
// dbclient_next_col_str           |   |   |   |   |   |   |   | F |
// dbclient_close_recordset        |   |   |   |   |   |   | A | A |
// dbclient_close_session          |   | D | D | D | D | D | D | D |
// -----------------------------------------------------------------


#include "defs/defs.h"
#include "common/encoding.h"
#include "common/decimal.h"
#include "client/pproto_client.h"
#include <stdio.h>
#include <arpa/inet.h>

// function return codes
typedef enum
{
    DBCLIENT_RETURN_ERROR = -1,         // client-side error occured
    DBCLIENT_RETURN_SUCCESS = 0,        // successful result / statement execution finished
    DBCLIENT_RETURN_AUTH_FAILED = 2,    // authentication failed
    DBCLIENT_RETURN_IN_PROGRESS = 3,    // statement execution is in progress
    DBCLIENT_RETURN_NO_MORE_ROWS = 4,   // end of recordset reached
    DBCLIENT_RETURN_SUCCESS_MSG = 5,    // success with message
    DBCLIENT_RETURN_SUCCESS_RS = 6      // success with resulting recordset
} dbclient_return_code;

// cell value
typedef union
{
    uint8   isnull;
    decimal d;
    sint32  i;
    sint16  s;
    float32 f32;
    float64 f64;
    uint64  dt;
    uint64  ts;
    struct
    {
        sint16 tz;
        uint64 ts;
    } ts_with_tz;
    struct
    {
        uint8 *buf;
        uint64 sz;
    } str;
} dbclient_value;


// return memory size required to allocate session
size_t dbclient_get_session_state_sz();

// initialize dbclient subsystem
void dbclient_init();

// allocate session using buffer ssbuf, client encoding enc
// if err_stream is not null errors will be spliied to err_stream in all future calls
// err_stream can be NULL, but some messages valuable for diagnostic can be lost
// return session handle or NULL on error
handle dbclient_allocate_session(void *ssbuf, encoding enc, FILE* err_stream);

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

#endif
