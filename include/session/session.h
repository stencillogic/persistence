#ifndef _SESSION_H
#define _SESSION_H

// Client session, it holds state, receives input and returns result to the client

#include "common/error.h"

// creates session
// return 0 on success, not 0 otherwise
sint8 session_create(int client_sock);

// sends error to client
// return 0 on success, not 0 otherwise
sint8 session_error(error_code err);

// closes session
// return 0 on success, not 0 otherwise
sint8 session_close();

#endif
