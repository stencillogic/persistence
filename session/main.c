// client listener and session

#include <stdlib.h>
#include <stdio.h>
#include "config/config.h"
#include "logging/logger.h"
#include "common/error.h"
#include "session/listener.h"

void write_error_and_exit()
{
    fputs("Error: ", stderr);
    fputs((const char*)error_msg(), stderr);
    fputc('\n', stderr);
    exit(error_code());
}

int main(int argc, char **argv)
{
    puts("=== Persistence v0.1 ===");

    if(config_create())
    {
        write_error_and_exit();
    }

    if(logger_create())
    {
        write_error_and_exit();
    }

    if(listener_create())
    {
        write_error_and_exit();
    }

    return 0;
}
