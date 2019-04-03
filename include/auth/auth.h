#ifndef _AUTH_H
#define _AUTH_H

// authentication data structures and functions

#include "defs/defs.h"

// length and size limits
#define AUTH_USER_NAME_SZ 260
#define AUTH_CREDENTIAL_SZ 512

typedef struct _auth_credentials
{
    uint8 user_name[AUTH_USER_NAME_SZ];
    uint8 credentials[AUTH_CREDENTIAL_SZ];
} auth_credentials;

void auth_hash_pwd(uint8* password, uint32 password_len, uint8 *credentials);

#endif
