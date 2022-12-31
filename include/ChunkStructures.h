#include <stdlib.h>
#include "LinkedList.h"

#ifndef __CHUNK_STRUCTURES_H
#define __CHUNK_STRUCTURES_H


typedef struct chunk {
    unsigned int length;
    unsigned int crc;
    char type[5];
    char *data;
} chunk_t;

typedef struct ihdr_chunk {
    chunk_t *chunk;
    unsigned int width;
    unsigned int height;
    unsigned int bit_depth;
    unsigned int color_type;
    unsigned int compression_method;
    unsigned int filter_method;
    unsigned int interlace_method;

    unsigned int bytes_per_pixel;
    unsigned int stride;
} ihdr_chunk_t;

typedef struct chunk_item {
    list_node_t node;
    chunk_t *chunk;
} chunk_item_t;

chunk_item_t *chunk_item_new(chunk_t *chunk) {
    chunk_item_t *item = (chunk_item_t *)malloc(sizeof(chunk_item_t));
    if (!item) {
        return NULL;
    }
    item->chunk = chunk;
    return item;
}

#endif //__CHUNK_STRUCTURES_H