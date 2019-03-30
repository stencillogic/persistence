#ifndef _IOUTILS_H
#define _IOUTILS_H

// I/O utility functions

#include "defs/defs.h"
#include "common/encoding.h"

// set communication socket
void ioutils_set_sock(int sock);

// send data to socket (omitting process-space buffering)
// return 0 on success, not 0 on error
sint8 ioutils_send_fully(const void *data, uint64 sz);

// write data to file (omitting process-space buffering)
// return 0 on success, not 0 on error
sint8 ioutils_write_fully(int fd, const void *data, uint64 sz);


// reads sz bytes to buffer buf from socket/client buffer
// return 0 on success, not 0 on error
sint8 ioutils_get(uint8 *buf, uint32 sz);

// read uint from socket/client buffer
// return 0 on success, not 0 on error
sint8 ioutils_get_uint8(uint8 *val);
sint8 ioutils_get_uint16(uint16 *val);
sint8 ioutils_get_uint32(uint32 *val);
sint8 ioutils_get_uint64(uint64 *val);


// flush process-space send buffer
// return 0 on success, not 0 on error
sint8 ioutils_flush_send();

// send arbitraty data in buf of size sz to socket sz using process-space buffering
// return 0 on success, not 0 on error
sint8 ioutils_send(const uint8 *buf, uint32 sz);

// send uint to socket using process-space buffer
// return 0 on success, not 0 on error
sint8 ioutils_send_uint8(uint8 val);
sint8 ioutils_send_uint16(uint16 val);
sint8 ioutils_send_uint32(uint32 val);
sint8 ioutils_send_uint64(uint64 val);

//
// character string io functions
//

// set current client encoding for use by ioutils_get_str
void ioutils_set_client_encoding(encoding enc);

// set internal/database encoding
void ioutils_set_server_encoding(encoding enc);

// reads at max uint32 bytes or until terminating character is met if is_nt is not 0 from socket/client buffer
// puts string in server encoding in str_buf and adds string terminator ('\0') if whole string fits in the str_buf
// return 0 on success, not 0 on error, and
// sz is set to number of bytes being read excluding '\0' at the end
// charlen is set to a number of characters being read excluding '\0'
sint8 ioutils_get_str(uint8 *str_buf, uint32 *sz, uint32 *charlen, uint8 is_nt);

// send string to client in client encoding (with null terminator at the end if is_nt is not 0)
// str_buf must be null-terminated string in server encoding
// return 0 on success, not 0 on error
sint8 ioutils_send_str(const uint8 *str_buf, uint8 is_nt);

#endif
