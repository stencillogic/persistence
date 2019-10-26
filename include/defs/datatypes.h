#ifndef _DATATYPES_H
#define _DATATYPES_H

#include <uchar.h>

// strings
typedef char16_t wchar;     // not used actually
typedef char     achar;     // must be utf-8, as constant strings are treated as utf-8

// integers
typedef char  sint8;
typedef short sint16;
typedef int   sint32;
typedef long  sint64;

typedef unsigned char  uint8;
typedef unsigned short uint16;
typedef unsigned int   uint32;
typedef unsigned long  uint64;

// floats
typedef float  float32;
typedef double float64;

// pointers
typedef void* handle;

#endif
