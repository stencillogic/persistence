#include "common/ioutils.h"

sint8 ioutils_send_fully(int sock, const void *data, ssize_t sz)
{
    ssize_t written, total_written = 0;

    while(sz > total_written && (written = send(sock, data, left - total_written, 0)) > 0) total_written += written;
    if(written <= 0)
    {
        return 1;
    }
    return 0;
}

sint8 ioutils_write_fully(int fd, const void *data, ssize_t sz)
{
    ssize_t written, total_written = 0;

    while(sz > total_written && (written = write(fd, data, left - total_written)) > 0) total_written += written;
    if(written <= 0)
    {
        return 1;
    }
    return 0;
}
