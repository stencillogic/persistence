#ifndef _TILER_H
#define _TILER_H

#include "defs/defs.h"

typedef struct _tile_uid {void *p} tile_uid;

typedef struct _tile {
    wchar  table_name;
    uint64 file_num;
    void *data;
} tile;

handle create_tiler(uint64 mem_size);

tile* find_tile(tile_uid tile);

#endif
