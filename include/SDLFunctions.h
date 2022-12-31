#define SDL_MAIN_HANDLED
#include <SDL.h>

#include "ChunkStructures.h"

#ifndef __SDL_FUNCTIONS_H
#define __SDL_FUNCTIONS_H

int png_decoder_sdl_init(SDL_Window **window, SDL_Renderer **renderer, SDL_Texture **texture, const unsigned char *pixels, ihdr_chunk_t *ihdr_chunk) {
    int ret = -1;
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) != 0) {
        SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
        goto png_decoder_sdl_quit;
    }
    SDL_Log("SDL is active!");

    *window = SDL_CreateWindow("SDL is active!", 50, 50, 1024, 1024, 0);
    if (!window) {
        SDL_Log("Unable to create window: %s", SDL_GetError());
        goto png_decoder_sdl_quit_window;
    }
    SDL_Log("SDL Window is create!");

    *renderer = SDL_CreateRenderer(*window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        SDL_Log("Unable to create renderer: %s", SDL_GetError());
        goto png_decoder_sdl_quit_renderer;
    }
    SDL_Log("SDL Rendere is create!");

    *texture = SDL_CreateTexture(*renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, ihdr_chunk->width, ihdr_chunk->height);
    if (!texture) {
        SDL_Log("Unable to create texture: %s", SDL_GetError());
        goto png_decoder_sdl_quit_texture;
    }
    SDL_Log("SDL Texture is create Width size %ux%u", ihdr_chunk->width, ihdr_chunk->height);

    ret = 0;

    SDL_UpdateTexture(*texture, NULL, pixels, ihdr_chunk->width * ihdr_chunk->bytes_per_pixel);
    SDL_SetTextureAlphaMod(*texture, 255);
    SDL_SetTextureBlendMode(*texture, SDL_BLENDMODE_BLEND);

    free(pixels);

    goto png_decoder_sdl_quit;

png_decoder_sdl_quit_texture:
    SDL_DestroyRenderer(*renderer);

png_decoder_sdl_quit_renderer:
    SDL_DestroyWindow(*window);

png_decoder_sdl_quit_window:
    SDL_Quit();

png_decoder_sdl_quit:
    return ret;
}

#endif //__SDL_FUNCTIONS_H