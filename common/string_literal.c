#include "common/string_literal.h"
#include <string.h>


// TODO: get rid of the size limit, e.g. write data to tipi


#define STRING_LITERAL_MAX_BUF_SZ   (1024)


typedef struct _string_literal
{
    uint64  sz;
    uint8   buf[STRING_LITERAL_MAX_BUF_SZ];
} string_literal;


// size for string_literal
size_t string_literal_alloc_sz()
{
    return sizeof(string_literal);
}

// create string literal instance
// return NULL on error
handle string_literal_create(void *buf)
{
    string_literal *sl = (string_literal *)buf;

    if(NULL == sl) return NULL;

    sl->sz = 0L;

    return (handle)sl;
}

// append buffer to string literal
// return 0 on success, 1 on error
sint8 string_literal_append_char(handle sh, uint8 *buf, uint32 sz)
{
    string_literal *sl = (string_literal *)sh;
    if(sz + sl->sz > STRING_LITERAL_MAX_BUF_SZ) return -1;

    memcpy(sl->buf + sl->sz, buf, sz);
    sl->sz += sz;
    return 0;
}


// truncate
sint8 string_literal_truncate(handle sh)
{
    string_literal *sl = (string_literal *)sh;
    sl->sz = 0;
    return 0;
}


// move ownership from one string literal to another
// from is trucated
// return 0 on success, non-0 on error
sint8 string_literal_move(handle from, handle to)
{
    // for now it is simple copy
    string_literal *sl_from= (string_literal *)from;
    string_literal *sl_to = (string_literal *)to;

    memcpy(sl_to, sl_from, sizeof(*to));
    return string_literal_truncate(from);
}
