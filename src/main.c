#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "ChunkStructures.h"
#include "chunk.h"
#include "LinkedList.h"

#include "SDLFunctions.h"

int main(int argc, char **argv) {
    int ret = -1;

    if (argc < 2) {
        puts("Args not valid.\nSecond srgument must be path relative to png");
        return ret;
    }

    FILE *file = fopen(argv[1], "rb");
    if (!file) {
        puts("Errore nell'apertura del file");
        return ret;
    }

    chunk_item_t *chunks;
    ihdr_chunk_t *ihdr_chunk;
    unsigned char *pixels;
    if (png_parse(file, &chunks, &ihdr_chunk, &pixels) != 0) {
        puts("Errore nella lettura del png");
        goto end;
    }

    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;

    if (png_decoder_sdl_init(&window, &renderer, &texture, pixels, ihdr_chunk) != 0) {
        goto end;
    }

    ret = 0;
    int running = 1;

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        SDL_Rect target_rect = {100, 100, 520, 715};
        SDL_RenderCopy(renderer, texture, NULL, &target_rect);

        SDL_RenderPresent(renderer);
    }

    free(window);
    free(renderer);
    free(texture);
end:
    if (chunks) {
        chunk_item_t *chunk_item_to_remove = chunks;
        while (chunk_item_to_remove) {
            free(chunk_item_to_remove->chunk);
            free(chunk_item_to_remove);
            chunk_item_to_remove = (chunk_item_t *)chunk_item_to_remove->node.next;
        }
    }

    if (ihdr_chunk) {
        free(ihdr_chunk);
    }

    fclose(file);
    return ret;
}