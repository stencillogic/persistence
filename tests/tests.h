#ifndef _PERSISTENCE_TESTS_H
#define _PERSISTENCE_TESTS_H

// automated tests (invoked by 'make test')

// unit test auth_sha3_512() from auth/auth_sha3.h
// return 0 on success, non 0 on error
int test_auth_sha3_512();

// test grigorian calendar functions
// return 0 on success, non 0 on error
int test_grigorian_calendar();

#endif
