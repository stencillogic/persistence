#ifndef _SESSION_H
#define _SESSION_H

// Client session, it holds state, receives input and returns result to the client

// creates session
// returns session handle or NULL on error
handle session_create(handle connection);

#endif
