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
sint8 string_literal_append_char(handle sh, const uint8 *buf, uint32 sz)
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

    memcpy(sl_to->buf, sl_from->buf, sl_from->sz);
    sl_to->sz = sl_from->sz;

    return string_literal_truncate(from);
}


sint8 string_literal_read(handle sh, uint8 *buf, uint64 *sz)
{
    string_literal *sl = (string_literal *)sh;

    if(sl->sz < *sz) *sz = sl->sz;
    memcpy(buf, sl->buf, *sz);

    return 0;
}

sint8 string_literal_byte_compare(handle sh1, handle sh2, sint8 *ret)
{
    string_literal *sl1 = (string_literal *)sh1;
    string_literal *sl2 = (string_literal *)sh2;
    int res;

    if(sl1->sz == sl2->sz)
    {
        res = memcmp(sl1->buf, sl2->buf, sl1->sz);
        *ret = (res > 0) ? 1 : ((res < 0) ? -1 : 0);
    }
    else
    {
        if(sl1->sz > sl2->sz)
        {
            res = memcmp(sl1->buf, sl2->buf, sl2->sz);
            *ret = (res >= 0) ? 1 : -1;
        }
        else
        {
            res = memcmp(sl1->buf, sl2->buf, sl1->sz);
            *ret = (res > 0) ? 1 : -1;
        }
    }

    return 0;
}

sint8 string_literal_byte_length(handle sh, uint64 *len)
{
    string_literal *sl = (string_literal *)sh;
    *len = sl->sz;
    return 0;
}
