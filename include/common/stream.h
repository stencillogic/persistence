#ifndef _STREAM_H
#define _STREAM_H

// THis file defines interface for data streaming

#include "defs/defs.h"

// write sz bytes from biffer buf to stream
// return 0 on success, non-0 on error
typedef sint8 (* stream_write_fp)(uint8 *buf, uint32 sz);

// read sz bytes from stream to biffer buf
// return 0 on success, non-0 on error
typedef sint8 (* stream_read_fp)(uint8 *buf, uint32 sz);

// stream
typedef struct _stream_desc
{
    stream_write_fp stream_write;
    stream_read_fp stream_read;
} stream_desc;


#endif
