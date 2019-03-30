#ifndef _AUTH_H
#define _AUTH_H

typedef enum _auth_method
{
    AUTH_BUILT_IN = 1
} auth_method;



// loads credentials from the buffer
// returns 1 on successfull authentication, 0 otherwise
uint8 auth_check_credentials(const void *credentials, uint64 sz);

// erases credentials, leaving authenticated user id
void auth_deallocate_credentials(handle credentials);

// returns currently authenticated user id or -1 if not authenticated
sint32 auth_get_user_id();

#endif
