#include "tests.h"
#include "common/htable.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int test_htable_functions()
{
    puts("Testing htable functions");

    uint32 n = 7;
    htable_entry e[8];
    const htable_entry *pe;
    uint32 i;

    void *buf = malloc(htable_get_alloc_sz(n));
    handle ht = htable_create(n, buf, htable_strhash);
    if(NULL == ht) return __LINE__;

    e[0].key = "a";
    e[0].keylen = 1;
    e[1].key = "aa";
    e[1].keylen = 2;
    e[2].key = "ab";
    e[2].keylen = 2;
    e[3].key = "aaaa";
    e[3].keylen = 4;
    e[4].key = "abc";
    e[4].keylen = 3;
    e[5].key = "";
    e[5].keylen = 0;
    e[6].key = "z";
    e[6].keylen = 1;
    e[7].key = "xxx xxx\0 123";
    e[7].keylen = 12;

    e[0].data = "data 1";
    e[0].datalen = 6;
    e[1].data = "data 2";
    e[1].datalen = 6;
    e[2].data = "data 3";
    e[2].datalen = 6;
    e[3].data = "data 4";
    e[3].datalen = 6;
    e[4].data = "data 5";
    e[4].datalen = 6;
    e[5].data = "data 6";
    e[5].datalen = 6;
    e[6].data = "data 7";
    e[6].datalen = 6;
    e[7].data = "data 8";
    e[7].datalen = 6;

    for(i=0; i<n; i++)
    {
        if(0 != htable_add(ht, e + i)) return 100000000 * i + __LINE__;
    }

    if(0 == htable_add(ht, e + i)) return __LINE__;

    if(NULL != htable_search(ht, "aaa", 3)) return __LINE__;

    for(i=0; i<n; i++)
    {
        pe = htable_search(ht, e[i].key, e[i].keylen);

        if(NULL == pe) return 100000000 * i + __LINE__;

        if(pe->keylen != e[i].keylen) return 100000000 * i + __LINE__;
        if(memcmp(pe->key, e[i].key, pe->keylen)) return 100000000 * i + __LINE__;

        if(pe->datalen != e[i].datalen) return 100000000 * i + __LINE__;
        if(memcmp(pe->data, e[i].data, pe->datalen)) return 100000000 * i + __LINE__;
    }


    free(ht);

    n = 100003;
    ht = htable_create(n, malloc(htable_get_alloc_sz(n)), htable_strhash);
    if(NULL == ht) return __LINE__;


    free(ht);

    return 0;
}
