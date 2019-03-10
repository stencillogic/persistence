#ifndef _HASH_MAP_H
#define _HASH_MAP_H

// create a hash map and return handle
// returns NULL on error
handle hmap_create(uint32 nbuckets);

// put element in hash map "hmap", returns 0 on success, 1 on error
uint8 hmap_put(handle hmap, uint8 *key, uint32 key_size, uint16 value);

// returns 0 if key is found in hmap and value is loaded in "value", unless "value" == NULL
// returns 1 if key is not found in a hash map
uint8 hmap_get(handle hmap, uint8 *key, uint32 key_size, uint16 *value);

#endif
