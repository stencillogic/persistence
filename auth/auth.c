#include "auth/auth.h"
#include "auth/auth_sha3.h"


void auth_hash_pwd(uint8* password, uint32 password_len, uint8 *credentials)
{
    auth_sha3_512(password, password_len, credentials);
}
