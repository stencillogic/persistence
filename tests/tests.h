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

#endif
