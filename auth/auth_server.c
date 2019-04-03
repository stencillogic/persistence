#include "auth/auth_server.h"


uint8 auth_check_credentials(auth_credentials *credentials, uint32 *user_id)
{
    // TODO: perform credential checks
    *user_id = 1;
    return 1;
}
