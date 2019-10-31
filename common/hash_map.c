#include "common/hash_map.h"


typedef struct _hmap_state
{
    size_t el_sz;
    uint32 n;
    void *data;
    uint8 buf[1];
} hmap_state;


// calculate size required to hold n elements of size el_sz
size_t hmap_get_allocation_sz(uint32 n, size_t el_sz)
{
    return sizeof(hmap_state) + el_sz * n;
}


// create a hash map of fixed size and return handle
// returns NULL on error
handle hmap_create(uint32 n, size_t el_sz, void *buf)
{
    hmap_state *hs =  (hmap_state *)buf;

    hs->n = n;
    hs->el_sz = el_sz;
    hs->data = hs->buf;

    return (handle)buf;
}


// put element in hash map "hmap", returns 0 on success, 1 on error
// value must have size of el_sz
uint8 hmap_put(handle hmap, void *key, size_t key_size, void *value)
{

}


// returns 0 if key is found in hmap and value is loaded in "value", unless "value" == NULL
// returns 1 if key is not found in a hash map
// if value != NULL then it must provide size of el_sz
uint8 hmap_get(handle hmap, void *key, size_t key_size, void *value);
