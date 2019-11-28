#ifndef _PERSISTENCE_TESTS_H
#define _PERSISTENCE_TESTS_H

// automated tests (invoked by 'make test')

// All test functions below return 0 on success or __LINE__ on error

// unit test auth_sha3_512() from auth/auth_sha3.h
int test_auth_sha3_512();

// test grigorian calendar functions
int test_grigorian_calendar();

// test strop functions
int test_strop_functions();

// test dateop functions
int test_dateop_functions();

// test encoding functions
int test_encoding_functions();

// test decimal functions
int test_decimal_functions();

// test client-side low level protocol functions
int test_pproto_client_functions();

// test client-side db driver functions
int test_dbclient_functions();

// test string_literal functions
int test_string_literal_functions();

// test client-side db driver functions
int test_lexer_functions();

// test pproto_server functions
int test_pproto_server_functions();

// test parser functions
int test_parser_functions();

// test stack functions
int test_stack_functions();

// test expression functions
int test_expression_functions();

#endif
