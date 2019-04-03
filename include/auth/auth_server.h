#ifndef _AUTH_SERVER_H
#define _AUTH_SERVER_H

// server-side authentication functions

#include "auth/auth.h"

// performs authentication, determines and returns user_id of authenticated user
// returns 1 on successfull authentication, 0 otherwise
uint8 auth_check_credentials(auth_credentials *credentials, uint32 *user_id);

#endif
