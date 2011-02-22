/* Simple program:  Move N sprites around on the screen as fast as possible */

#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "SDL.h"

#define WINDOW_WIDTH    640
#define WINDOW_HEIGHT   480
#define NUM_SPRITES     100
#define MAX_SPEED       1

static SDL_Texture *sprite;
static SDL_Rect positions[NUM_SPRITES];
static SDL_Rect velocities[NUM_SPRITES];
static int sprite_w, sprite_h;

/* Call this instead of exit(), so we can clean up SDL: atexit() is evil. */
static void
quit(int rc)
{
    exit(rc);
}

int
LoadSprite(char *file, SDL_Renderer *renderer)
{
    SDL_Surface *temp;

    /* Load the sprite image */
    temp = SDL_LoadBMP(file);
    if (temp == NULL) {
        fprintf(stderr, "Couldn't load %s: %s", file, SDL_GetError());
        return (-1);
    }
    sprite_w = temp->w;
    sprite_h = temp->h;

    /* Set transparent pixel as the pixel at (0,0) */
    if (temp->format->palette) {
        SDL_SetColorKey(temp, SDL_TRUE, *(Uint8 *) temp->pixels);
    } else {
        switch (temp->format->BitsPerPixel) {
        case 15:
            SDL_SetColorKey(temp, SDL_TRUE,
                            (*(Uint16 *) temp->pixels) & 0x00007FFF);
            break;
        case 16:
            SDL_SetColorKey(temp, SDL_TRUE, *(Uint16 *) temp->pixels);
            break;
        case 24:
            SDL_SetColorKey(temp, SDL_TRUE,
                            (*(Uint32 *) temp->pixels) & 0x00FFFFFF);
            break;
        case 32:
            SDL_SetColorKey(temp, SDL_TRUE, *(Uint32 *) temp->pixels);
            break;
        }
    }

    /* Create textures from the image */
    sprite = SDL_CreateTextureFromSurface(renderer, temp);
    if (!sprite) {
        fprintf(stderr, "Couldn't create texture: %s\n", SDL_GetError());
        SDL_FreeSurface(temp);
        return (-1);
    }
    SDL_FreeSurface(temp);

    /* We're ready to roll. :) */
    return (0);
}

void
MoveSprites(SDL_Window * window, SDL_Renderer * renderer, SDL_Texture * sprite)
{
    int i;
    int window_w = WINDOW_WIDTH;
    int window_h = WINDOW_HEIGHT;
    SDL_Rect *position, *velocity;

    /* Draw a gray background */
    SDL_SetRenderDrawColor(renderer, 0xA0, 0xA0, 0xA0, 0xFF);
    SDL_RenderClear(renderer);

    /* Move the sprite, bounce at the wall, and draw */
    for (i = 0; i < NUM_SPRITES; ++i) {
        position = &positions[i];
        velocity = &velocities[i];
        position->x += velocity->x;
        if ((position->x < 0) || (position->x >= (window_w - sprite_w))) {
            velocity->x = -velocity->x;
            position->x += velocity->x;
        }
        position->y += velocity->y;
        if ((position->y < 0) || (position->y >= (window_h - sprite_h))) {
            velocity->y = -velocity->y;
            position->y += velocity->y;
        }

        /* Blit the sprite onto the screen */
        SDL_RenderCopy(renderer, sprite, NULL, position);
    }

    /* Update the screen! */
    SDL_RenderPresent(renderer);
}

int
main(int argc, char *argv[])
{
    SDL_Window *window;
    SDL_Renderer *renderer;
    int i, done;
    SDL_Event event;

    window = SDL_CreateWindow("Happy Smileys",
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              WINDOW_WIDTH, WINDOW_HEIGHT,
                              SDL_WINDOW_SHOWN);
    if (!window) {
        quit(2);
    }

    renderer = SDL_CreateRenderer(window, -1, 0);
    if (!renderer) {
        quit(2);
    }

    if (LoadSprite("icon.bmp", renderer) < 0) {
        quit(2);
    }

    /* Initialize the sprite positions */
    srand(time(NULL));
    for (i = 0; i < NUM_SPRITES; ++i) {
        positions[i].x = rand() % (WINDOW_WIDTH - sprite_w);
        positions[i].y = rand() % (WINDOW_HEIGHT - sprite_h);
        positions[i].w = sprite_w;
        positions[i].h = sprite_h;
        velocities[i].x = 0;
        velocities[i].y = 0;
        while (!velocities[i].x && !velocities[i].y) {
            velocities[i].x = (rand() % (MAX_SPEED * 2 + 1)) - MAX_SPEED;
            velocities[i].y = (rand() % (MAX_SPEED * 2 + 1)) - MAX_SPEED;
        }
    }

    /* Main render loop */
    done = 0;
    while (!done) {
        /* Check for events */
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || event.type == SDL_KEYDOWN) {
                done = 1;
            }
        }
        MoveSprites(window, renderer, sprite);
    }

    quit(0);
}

/* vi: set ts=4 sw=4 expandtab: */
