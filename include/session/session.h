#ifndef _SESSION_H
#define _SESSION_H

// Client session, it holds state, receives input and returns result to the client

#include "common/error.h"
#include "common/encoding.h"

//
//       session automaton
// -----------------------------------
//       |  condition
// state |----------------------------
//       | a | b | c | d | e | f | g |
// -----------------------------------
//   0   | 1 | 4 | 4 | 4 | 4 | 4 | 4 |
//   1   | 1 | 1 | 2 | 4 | 4 | 4 | 4 |
//   2   | 2 | 2 | 2 | 3 | 2 | 4 | 2 |
//   3   | 3 | 3 | 3 | 3 | 2 | 3 | 3 |
//   4   | 4 | 4 | 4 | 4 | 4 | 4 | 4 |
//
//
// States:
//  0 - initial state, client just connected
//  1 - hello from client received
//  2 - client authenticated
//  3 - sql being executed
//  4 - session terminated, client disconnected
//
// Conditions:
//  a - hello_message from client
//  b - auth_message from client, wrong credentials
//  c - auth_message from client, correct credentials
//  d - sql_request_message from client
//  e - cancel_message
//  f - goodbye_message
//  g - incorrect message

// creates session
// return 0 on success, not 0 otherwise
sint8 session_create(int client_sock);

// return client encoding, 0 if no encoding is set yet
encoding session_encoding();

#endif
