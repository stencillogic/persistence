#ifndef _STREAM_H
#define _STREAM_H

#include "defs/defs.h"

// read character from the stream
// EOF is returned in case of end of input or error
wchar stream_getc(handle stream);

#endif
