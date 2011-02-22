/********************************************************************************
 *                                                                              *
 * Running moose :) Coded by Mike Gorchak.                                      *
 *                                                                              *
 ********************************************************************************/

#include <stdlib.h>
#include <stdio.h>

#include "SDL.h"

#define MOOSEPIC_W 64
#define MOOSEPIC_H 88

#define MOOSEFRAME_SIZE (MOOSEPIC_W * MOOSEPIC_H)
#define MOOSEFRAMES_COUNT 10

SDL_Color MooseColors[84] = {
    {49, 49, 49}, {66, 24, 0}, {66, 33, 0}, {66, 66, 66},
    {66, 115, 49}, {74, 33, 0}, {74, 41, 16}, {82, 33, 8},
    {82, 41, 8}, {82, 49, 16}, {82, 82, 82}, {90, 41, 8},
    {90, 41, 16}, {90, 57, 24}, {99, 49, 16}, {99, 66, 24},
    {99, 66, 33}, {99, 74, 33}, {107, 57, 24}, {107, 82, 41},
    {115, 57, 33}, {115, 66, 33}, {115, 66, 41}, {115, 74, 0},
    {115, 90, 49}, {115, 115, 115}, {123, 82, 0}, {123, 99, 57},
    {132, 66, 41}, {132, 74, 41}, {132, 90, 8}, {132, 99, 33},
    {132, 99, 66}, {132, 107, 66}, {140, 74, 49}, {140, 99, 16},
    {140, 107, 74}, {140, 115, 74}, {148, 107, 24}, {148, 115, 82},
    {148, 123, 74}, {148, 123, 90}, {156, 115, 33}, {156, 115, 90},
    {156, 123, 82}, {156, 132, 82}, {156, 132, 99}, {156, 156, 156},
    {165, 123, 49}, {165, 123, 90}, {165, 132, 82}, {165, 132, 90},
    {165, 132, 99}, {165, 140, 90}, {173, 132, 57}, {173, 132, 99},
    {173, 140, 107}, {173, 140, 115}, {173, 148, 99}, {173, 173, 173},
    {181, 140, 74}, {181, 148, 115}, {181, 148, 123}, {181, 156, 107},
    {189, 148, 123}, {189, 156, 82}, {189, 156, 123}, {189, 156, 132},
    {189, 189, 189}, {198, 156, 123}, {198, 165, 132}, {206, 165, 99},
    {206, 165, 132}, {206, 173, 140}, {206, 206, 206}, {214, 173, 115},
    {214, 173, 140}, {222, 181, 148}, {222, 189, 132}, {222, 189, 156},
    {222, 222, 222}, {231, 198, 165}, {231, 231, 231}, {239, 206, 173}
};

Uint8 MooseFrames[MOOSEFRAMES_COUNT][MOOSEFRAME_SIZE];

void quit(int rc)
{
    SDL_Quit();
    exit(rc);
}

void UpdateTexture(SDL_Texture *texture, int frame)
{
    SDL_Color *color;
    Uint8 *src;
    Uint32 *dst;
    int row, col;
    void *pixels;
    int pitch;

    if (SDL_LockTexture(texture, NULL, &pixels, &pitch) < 0) {
        fprintf(stderr, "Couldn't lock texture: %s\n", SDL_GetError());
        quit(5);
    }
    src = MooseFrames[frame];
    for (row = 0; row < MOOSEPIC_H; ++row) {
        dst = (Uint32*)((Uint8*)pixels + row * pitch);
        for (col = 0; col < MOOSEPIC_W; ++col) {
            color = &MooseColors[*src++];
            *dst++ = (0xFF000000|(color->r<<16)|(color->g<<8)|color->b);
        }
    }
    SDL_UnlockTexture(texture);
}

int
main(int argc, char **argv)
{
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_RWops *handle;
    SDL_Texture *MooseTexture;
    SDL_Event event;
    SDL_bool done = SDL_FALSE;
    int frame;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
        return 1;
    }

    /* load the moose images */
    handle = SDL_RWFromFile("moose.dat", "rb");
    if (handle == NULL) {
        fprintf(stderr, "Can't find the file moose.dat !\n");
        quit(2);
    }
    SDL_RWread(handle, MooseFrames, MOOSEFRAME_SIZE, MOOSEFRAMES_COUNT);
    SDL_RWclose(handle);


    /* Create the window and renderer */
    window = SDL_CreateWindow("Happy Moose",
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              MOOSEPIC_W*4, MOOSEPIC_H*4,
                              SDL_WINDOW_SHOWN|SDL_WINDOW_RESIZABLE);
    if (!window) {
        fprintf(stderr, "Couldn't set create window: %s\n", SDL_GetError());
        quit(3);
    }

    renderer = SDL_CreateRenderer(window, -1, 0);
    if (!renderer) {
        fprintf(stderr, "Couldn't set create renderer: %s\n", SDL_GetError());
        quit(4);
    }

    MooseTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, MOOSEPIC_W, MOOSEPIC_H);
    if (!MooseTexture) {
        fprintf(stderr, "Couldn't set create texture: %s\n", SDL_GetError());
        quit(5);
    }

    /* Loop, waiting for QUIT or the escape key */
    frame = 0;
    while (!done) {
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    done = SDL_TRUE;
                }
                break;
            case SDL_QUIT:
                done = SDL_TRUE;
                break;
            }
        }

        frame = (frame + 1) % MOOSEFRAMES_COUNT;
        UpdateTexture(MooseTexture, frame);

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, MooseTexture, NULL, NULL);
        SDL_RenderPresent(renderer);
    }
    SDL_DestroyRenderer(renderer);

    quit(0);
    return 0;
}

/* vi: set ts=4 sw=4 expandtab: */
