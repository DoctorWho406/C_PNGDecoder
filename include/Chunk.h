#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#define ZLIB_BUF_SIZE 16 * 1024

#include "ChunkStructures.h"

#ifndef __CHUNK_H
#define __CHUNK_H

void change_endian(unsigned int *value) {
    *value = ((*value << 8) & 0xFF00FF00) | ((*value >> 8) & 0xFF00FF);
    *value = (*value << 16) | ((*value >> 16) & 0xFFFF);
}

unsigned int chars_to_int(const char *string, size_t size, size_t offset) {
    unsigned int result = 0;
    for (size_t i = offset; i < size + offset; ++i) {
        result |= string[i] << (8 * (i - offset));
    }
    return result;
}

int png_verify_signature(FILE *file) {
    void *signature = malloc(8);
    if (!signature) {
        puts("Error while malloc signature");
        return 0;
    }
    fread(signature, sizeof(signature), 1, file);

    const char *png_signature = "\x89PNG\r\n\x1a\n";
    int ret = 0;
    ret = memcmp(png_signature, signature, 8) == 0;
    free(signature);
    return ret;
}

chunk_t *read_chunk(const FILE *file) {
    chunk_t *chunk = (chunk_t *)malloc(sizeof(chunk_t));
    if (!chunk) {
        return NULL;
    }

    fread(&(chunk->length), sizeof(chunk->length), 1, file);
    change_endian(&(chunk->length));

    fread(&(chunk->type), 4, 1, file);
    chunk->type[4] = '\0';

    chunk->data = (char *)malloc(chunk->length);
    if (!chunk->data) {
        puts("Error while malloc data");
        free(chunk);
        return NULL;
    }
    fread(chunk->data, chunk->length, 1, file);

    // compute the crc32 given a buffer and its size (same as python zlib.crc32(buffer))
    unsigned int checksum = crc32(crc32(0, chunk->type, 4), chunk->data, chunk->length);

    fread(&(chunk->crc), sizeof(chunk->crc), 1, file);
    change_endian(&(chunk->crc));

    if (checksum != chunk->crc) {
        puts("Checksum failed");
        free(chunk);
        return NULL;
    }

    return chunk;
}

ihdr_chunk_t *read_ihdr_chunk(const chunk_t *chunk) {
    ihdr_chunk_t *ihdr_chunk = (ihdr_chunk_t *)malloc(sizeof(ihdr_chunk_t));
    if (!ihdr_chunk) {
        puts("Error while malloc ihdr chunk");
        return NULL;
    }

    ihdr_chunk->width = chars_to_int(chunk->data, 4, 0);
    change_endian(&(ihdr_chunk->width));
    ihdr_chunk->height = chars_to_int(chunk->data, 4, 4);
    change_endian(&(ihdr_chunk->height));
    ihdr_chunk->bit_depth = chunk->data[8];
    ihdr_chunk->color_type = chunk->data[9];
    ihdr_chunk->compression_method = chunk->data[10];
    ihdr_chunk->filter_method = chunk->data[11];
    ihdr_chunk->interlace_method = chunk->data[12];

    switch (ihdr_chunk->color_type) {
        case 0:
            ihdr_chunk->bytes_per_pixel = 1;
            break;
        case 2:
            ihdr_chunk->bytes_per_pixel = 3;
            break;
        case 3:
            ihdr_chunk->bytes_per_pixel = 1;
            break;
        case 4:
            ihdr_chunk->bytes_per_pixel = 2;
            break;
        case 6:
            ihdr_chunk->bytes_per_pixel = 4;
            break;
        default:
            puts("IHDR Color type is not valid.");
            free(ihdr_chunk);
            return NULL;
            break;
    }
    ihdr_chunk->stride = ihdr_chunk->width * ihdr_chunk->bytes_per_pixel;

    return ihdr_chunk;
}

unsigned char *read_idat_chunk(const ihdr_chunk_t *ihdr_chunk, unsigned char *idat_data, size_t idata_data_length) {
    size_t uncompressed_size = ihdr_chunk->height * (1 + ihdr_chunk->stride);
    size_t compressed_max_size = compressBound(uncompressed_size);
    unsigned char *compressed_idat_data = (unsigned char *)malloc(idata_data_length);
    if (!compressed_idat_data) {
        puts("Error while malloc idat data");
        return NULL;
    }
    if (!memcpy(compressed_idat_data, idat_data, idata_data_length)) {
        puts("Error while memcpy idat data");
        free(compressed_idat_data);
        return NULL;
    }
    idat_data = (unsigned char *)realloc(idat_data, uncompressed_size);
    if (!idat_data) {
        puts("Error while realloc idat data");
        free(compressed_idat_data);
        return NULL;
    }
    int result = uncompress(idat_data, &uncompressed_size, compressed_idat_data, idata_data_length);
    if (result != Z_OK) {
        printf("Errore while uncompressing data: %d\n", result);
        free(compressed_idat_data);
        return NULL;
    }
    free(compressed_idat_data);

    return idat_data;
}

unsigned char paeth_predictor(const unsigned char a, const unsigned char b, const unsigned char c) {
    int p = (int)a + (int)b - (int)c;
    int pa = abs(p - (int)a);
    int pb = abs(p - (int)b);
    int pc = abs(p - (int)c);

    if (pa <= pb && pa <= pc) {
        return a;
    } else if (pb <= pc) {
        return b;
    } else {
        return c;
    }
}

unsigned char recon_a(const unsigned char *pixels, const int scanline, const int byte, const ihdr_chunk_t *ihdr_chunk) {
    return  byte >= ihdr_chunk->bytes_per_pixel ? pixels[scanline * ihdr_chunk->stride + byte - ihdr_chunk->bytes_per_pixel] : 0;
}

unsigned char recon_b(const unsigned char *pixels, const int scanline, const int byte, const ihdr_chunk_t *ihdr_chunk) {
    return scanline > 0 ? pixels[(scanline - 1) * ihdr_chunk->stride + byte] : 0;
}

unsigned char recon_c(const unsigned char *pixels, const int scanline, const int byte, const ihdr_chunk_t *ihdr_chunk) {
    return (scanline > 0 && byte >= ihdr_chunk->bytes_per_pixel) ? pixels[(scanline - 1) * ihdr_chunk->stride + byte - ihdr_chunk->bytes_per_pixel] : 0;
}

unsigned char *get_pixels(const ihdr_chunk_t *ihdr_chunk, const unsigned char *idat_data) {
    unsigned char *pixels = (unsigned char *)malloc(ihdr_chunk->height * ihdr_chunk->stride * sizeof(unsigned char));
    if (!pixels) {
        puts("Error while malloc pixels");
        return NULL;
    }

    size_t i = 0, pixels_index = 0;
    unsigned char pixel = 0;
    for (size_t scanline = 0; scanline < ihdr_chunk->height; scanline++) {
        unsigned int filter_type = idat_data[i++];
        for (size_t byte = 0; byte < ihdr_chunk->stride; byte++) {
            unsigned char filter_x = idat_data[i++];
            switch (filter_type) {
                case 0:
                    pixel = filter_x;
                    break;
                case 1:
                    pixel = filter_x + recon_a(pixels, scanline, byte, ihdr_chunk);
                    break;
                case 2:
                    pixel = filter_x + recon_b(pixels, scanline, byte, ihdr_chunk);
                    break;
                case 3:
                    pixel = filter_x + (recon_a(pixels, scanline, byte, ihdr_chunk) + recon_b(pixels, scanline, byte, ihdr_chunk)) / 2;
                    break;
                case 4:
                    pixel = filter_x + paeth_predictor(recon_a(pixels, scanline, byte, ihdr_chunk), recon_b(pixels, scanline, byte, ihdr_chunk), recon_c(pixels, scanline, byte, ihdr_chunk));
                    break;
                default:
                    printf("Filter type %u is not valid.\n", filter_type);
                    free(pixels);
                    return NULL;
                    break;
            }
            pixels[pixels_index++] = pixel;
        }
    }

    return pixels;
}

int png_parse(FILE *file, chunk_item_t **chunks, ihdr_chunk_t **ihdr_chunk, unsigned char **pixels) {
    int ret = -1;
    if (!png_verify_signature(file)) {
        puts("Invalid PNG Signature");
        return ret;
    }

    *chunks = NULL;
    chunk_item_t *chunk_item = NULL;
    chunk_t *chunk = NULL;

    while (1) {
        chunk = read_chunk(file);
        if (!chunk) {
            puts("Error parsing CHUNKS");
            return ret;
        }
        chunk_item = chunk_item_new(chunk);
        if (!chunk_item) {
            puts("Error creating list");
            return ret;
        }

        chunk_item = (chunk_item_t *)list_append((list_node_t **)chunks, (list_node_t *)chunk_item);
        if (!chunk_item) {
            puts("Error creating list");
            return ret;
        }

        if (memcmp(chunk->type, "IEND", 4) == 0) {
            break;
        }
    }

    chunk_item = *chunks;
    while (chunk_item) {
        if (memcmp(chunk_item->chunk->type, "IHDR", 4) == 0) {
            *ihdr_chunk = read_ihdr_chunk(chunk_item->chunk);
            break;
        }
        chunk_item = (chunk_item_t *)chunk_item->node.next;
    }

    if (!(*ihdr_chunk)) {
        puts("Error parsing IHDR CHUNKS");
        return ret;
    }

    if ((*ihdr_chunk)->compression_method != 0) {
        puts("Invalid compression method");
        return ret;
    }
    if ((*ihdr_chunk)->filter_method != 0) {
        puts("Invalid filter method");
        return ret;
    }
    if ((*ihdr_chunk)->color_type != 6) {
        puts("Only support truecolor with alpha");
        return ret;
    }
    if ((*ihdr_chunk)->bit_depth != 8) {
        puts("Only support a bit depth of 8");
        return ret;
    }
    if ((*ihdr_chunk)->interlace_method != 0) {
        puts("Only support no interlacing");
        return ret;
    }

    size_t data_size = 0;
    unsigned char *data = NULL;

    chunk_item = chunks;
    while (chunk_item) {
        if (memcmp(chunk_item->chunk->type, "IDAT", 4) == 0) {
            data = realloc(data, data_size + chunk_item->chunk->length);
            if (!data) {
                puts("Errore while realloc data");
                return ret;
            }
            data = memcpy(data + data_size, chunk_item->chunk->data, chunk_item->chunk->length);
            if (!data) {
                puts("Errore while memcpy data");
                free(data);
                return ret;
            }
            data_size = chunk_item->chunk->length;
        }
        chunk_item = (chunk_item_t *)chunk_item->node.next;
    }

    data = read_idat_chunk(*ihdr_chunk, data, data_size);
    if (!data) {
        puts("Error parsing IHDR CHUNKS");
        free(data);
        return ret;
    }

    *pixels = get_pixels(*ihdr_chunk, data);
    if (!pixels) {
        puts("Error reading pixels");
        free(data);
        return ret;
    }

    free(data);
    return 0;
}

#endif //__CHUNK_H