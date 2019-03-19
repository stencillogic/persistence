// client listener and session

#include <stdlib.h>
#include <stdio.h>
#include "config/config.h"
#include "logging/logger.h"
#include "common/error.h"
#include "session/listener.h"

void write_error_and_exit()
{
    fflush(stdout);
    fputs(_ach("Fatal, terminating\n"), stderr);
    exit(1);
}

int main(int argc, char **argv)
{
    puts("=== Persistence v0.1 ===");

    if(config_create())
    {
        write_error_and_exit();
    }

    if(logger_create(_ach("listener.log")))
    {
        write_error_and_exit();
    }

    if(listener_create())
    {
        write_error_and_exit();
    }

    return 0;
}
