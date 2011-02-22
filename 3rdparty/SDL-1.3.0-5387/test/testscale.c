/* Simple program:  Move N sprites around on the screen as fast as possible */

#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "SDL.h"
#include "common.h"

#define WINDOW_WIDTH    640
#define WINDOW_HEIGHT   480

static CommonState *state;

typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *background;
    SDL_Texture *sprite;
    SDL_Rect sprite_rect;
    int scale_direction;
} DrawState;

/* Call this instead of exit(), so we can clean up SDL: atexit() is evil. */
static void
quit(int rc)
{
    CommonQuit(state);
    exit(rc);
}

SDL_Texture *
LoadTexture(SDL_Renderer *renderer, char *file, SDL_bool transparent)
{
    SDL_Surface *temp;
    SDL_Texture *texture;

    /* Load the sprite image */
    temp = SDL_LoadBMP(file);
    if (temp == NULL) {
        fprintf(stderr, "Couldn't load %s: %s", file, SDL_GetError());
        return NULL;
    }

    /* Set transparent pixel as the pixel at (0,0) */
    if (transparent) {
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
    }

    /* Create textures from the image */
    texture = SDL_CreateTextureFromSurface(renderer, temp);
    if (!texture) {
        fprintf(stderr, "Couldn't create texture: %s\n", SDL_GetError());
        SDL_FreeSurface(temp);
        return NULL;
    }
    SDL_FreeSurface(temp);

    /* We're ready to roll. :) */
    return texture;
}

void
Draw(DrawState *s)
{
    SDL_Rect viewport;

    SDL_RenderGetViewport(s->renderer, &viewport);

    /* Draw the background */
    SDL_RenderCopy(s->renderer, s->background, NULL, NULL);

    /* Scale and draw the sprite */
    s->sprite_rect.w += s->scale_direction;
    s->sprite_rect.h += s->scale_direction;
    if (s->scale_direction > 0) {
        if (s->sprite_rect.w >= viewport.w || s->sprite_rect.h >= viewport.h) {
            s->scale_direction = -1;
        }
    } else {
        if (s->sprite_rect.w <= 1 || s->sprite_rect.h <= 1) {
            s->scale_direction = 1;
        }
    }
    s->sprite_rect.x = (viewport.w - s->sprite_rect.w) / 2;
    s->sprite_rect.y = (viewport.h - s->sprite_rect.h) / 2;

    SDL_RenderCopy(s->renderer, s->sprite, NULL, &s->sprite_rect);

    /* Update the screen! */
    SDL_RenderPresent(s->renderer);
}

int
main(int argc, char *argv[])
{
    DrawState *drawstates;
    int i, done;
    SDL_Event event;
    int frames;
    Uint32 then, now;

    /* Initialize test framework */
    state = CommonCreateState(argv, SDL_INIT_VIDEO);
    if (!state) {
        return 1;
    }
    for (i = 1; i < argc;) {
        int consumed;

        consumed = CommonArg(state, i);
        if (consumed == 0) {
            fprintf(stderr, "Usage: %s %s\n", argv[0], CommonUsage(state));
            return 1;
        }
        i += consumed;
    }
    if (!CommonInit(state)) {
        quit(2);
    }

    drawstates = SDL_stack_alloc(DrawState, state->num_windows);
    for (i = 0; i < state->num_windows; ++i) {
        DrawState *drawstate = &drawstates[i];

        drawstate->window = state->windows[i];
        drawstate->renderer = state->renderers[i];
        drawstate->sprite = LoadTexture(drawstate->renderer, "icon.bmp", SDL_TRUE);
        drawstate->background = LoadTexture(drawstate->renderer, "sample.bmp", SDL_FALSE);
        if (!drawstate->sprite || !drawstate->background) {
            quit(2);
        }
        SDL_QueryTexture(drawstate->sprite, NULL, NULL,
                         &drawstate->sprite_rect.w, &drawstate->sprite_rect.h);
        drawstate->scale_direction = 1;
    }

    /* Main render loop */
    frames = 0;
    then = SDL_GetTicks();
    done = 0;
    while (!done) {
        /* Check for events */
        ++frames;
        while (SDL_PollEvent(&event)) {
            CommonEvent(state, &event, &done);
        }
        for (i = 0; i < state->num_windows; ++i) {
            Draw(&drawstates[i]);
        }
    }

    /* Print out some timing information */
    now = SDL_GetTicks();
    if (now > then) {
        double fps = ((double) frames * 1000) / (now - then);
        printf("%2.2f frames per second\n", fps);
    }

    SDL_stack_free(drawstates);

    quit(0);
    return 0;
}

/* vi: set ts=4 sw=4 expandtab: */
