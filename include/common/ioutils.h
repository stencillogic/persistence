#ifndef _IOUTILS_H
#define _IOUTILS_H

// I/O utility functions

// send data to socket
// return 0 on success, not 0 on error
sint8 ioutils_send_fully(int sock, const void *data, uint64 sz);

// write data to file
// return 0 on success, not 0 on error
sint8 ioutils_write_fully(int fd, const void *data, uint64 sz);

#endif
