#include "common/htable.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>


typedef struct _htable_table_desc
{
    uint32          n;
    uint32          used;
    htable_hfun     hfun;
    htable_entry    tbl[];
} htable_table_desc;


// return allocation size for table of n elements
size_t htable_get_alloc_sz(uint32 n)
{
    return sizeof(htable_table_desc) + n*sizeof(htable_entry);
}


// create hash table of n entries using buffer buf to store data
// and hashing algorithm defined by function f
// return NULL of error
handle htable_create(uint32 n, void *buf, htable_hfun f)
{
    htable_table_desc *ht = (htable_table_desc *)buf;

    assert(NULL != f);

    if(NULL != buf)
    {
        ht->n = n;
        ht->hfun = f;
        ht->used = 0;
        memset(ht->tbl, 0, sizeof(htable_entry)*n);
    }

    return (handle)buf;
}


// add an entry to a table
// return 0 on success, non-0 when not enough memory
sint8 htable_add(handle hh, const htable_entry *entry)
{
    htable_table_desc *ht = (htable_table_desc *)hh;

    if(ht->used == ht->n) return -1;

    assert(NULL != entry);
    assert(NULL != entry->key);

    uint32 hash = ht->hfun(entry->key, entry->keylen) % ht->n;

    htable_entry *e = ht->tbl + hash;
    while(NULL != e->key)
    {
        if(e->keylen == entry->keylen && 0 == memcmp(e->key, entry->key, e->keylen))
        {
            break;
        }

        hash++;
        if(hash == ht->n) hash = 0;
        e = ht->tbl + hash;
    }

    ht->tbl[hash] = *entry;
    ht->used++;

    return 0;
}


// search for an entry by a key
// return NULL if not found
const htable_entry *htable_search(handle hh, const void *key, uint32 keylen)
{
    htable_table_desc   *ht = (htable_table_desc *)hh;
    const htable_entry  *e;
    uint32              checked = 0;
    uint32              hash;

    assert(NULL != key);

    hash = ht->hfun(key, keylen) % ht->n;

    do
    {
        e = ht->tbl + hash;
        if(NULL == e->key)
        {
            return NULL;
        }
        else
        {
            if(e->keylen != keylen || 0 != memcmp(e->key, key, keylen))
            {
                hash++;
                if(hash == ht->n) hash = 0;
            }
            else
            {
                return e;
            }
            checked++;
        }
    }
    while(checked < ht->used);

    return NULL;
}


// to destroy delete allocated memory


// hash functions:

// hash function for hashing strings
uint32 htable_strhash(const void *val, uint32 len)
{
    uint32 hash = 5381;
    uint8  c;

    while(len-- > 0)
    {
        c = *((uint8 *)val++);
        hash = ((hash << 5) + hash) + c;
    }

    return hash;
}
