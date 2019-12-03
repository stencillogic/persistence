#ifndef _HTABLE_H
#define _HTABLE_H


// hash table


#include "defs/defs.h"


// entry description
typedef struct _htable_entry
{
    uint32    keylen;
    uint32    datalen;
    void      *key;
    void      *data;
} htable_entry;


// function calculating hash value of val of length len
// return hash value
typedef uint32 (* htable_hfun)(const void *val, uint32 len);


// return allocation size for table of n elements
size_t htable_get_alloc_sz(uint32 n);


// create hash table of n entries using buffer buf to store data
// and hashing algorithm defined by function f
// return NULL of error
handle htable_create(uint32 n, void *buf, htable_hfun f);


// add an entry to a table
// return 0 on success, non-0 when not enough memory
sint8 htable_add(handle ht, const htable_entry *entry);


// search for an entry by a key
// return NULL if not found
const htable_entry *htable_search(handle ht, const void *key, uint32 keylen);


// to destroy delete allocated memory


// hash functions:

// hash function for hashing strings
uint32 htable_strhash(const void *val, uint32 len);


#endif
