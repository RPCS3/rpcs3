/* Simple program:  Fill the screen with colors as fast as possible */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "SDL.h"

/* Call this instead of exit(), so we can clean up SDL: atexit() is evil. */
static void
quit(int rc)
{
    SDL_Quit();
    exit(rc);
}

int
main(int argc, char *argv[])
{
    SDL_Surface *screen;
    int width, height;
    Uint8 video_bpp;
    Uint32 videoflags;
    Uint32 colors[3];
    int i, done;
    SDL_Event event;
    Uint32 then, now, frames;

    /* Initialize SDL */
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
        return (1);
    }

    width = 640;
    height = 480;
    video_bpp = 8;
    videoflags = 0;
    while (argc > 1) {
        --argc;
        if (strcmp(argv[argc - 1], "-width") == 0) {
            width = atoi(argv[argc]);
            --argc;
        } else if (strcmp(argv[argc - 1], "-height") == 0) {
            height = atoi(argv[argc]);
            --argc;
        } else if (strcmp(argv[argc - 1], "-bpp") == 0) {
            video_bpp = atoi(argv[argc]);
            --argc;
        } else if (strcmp(argv[argc], "-fullscreen") == 0) {
            videoflags ^= SDL_FULLSCREEN;
        } else {
            fprintf(stderr,
                    "Usage: %s [-width N] [-height N] [-bpp N] [-fullscreen]\n",
                    argv[0]);
            quit(1);
        }
    }

    /* Set video mode */
    screen = SDL_SetVideoMode(width, height, video_bpp, 0);
    if (!screen) {
        fprintf(stderr, "Couldn't set %dx%d video mode: %s\n",
                width, height, SDL_GetError());
        quit(2);
    }

    /* Get the colors */
    colors[0] = SDL_MapRGB(screen->format, 0xFF, 0x00, 0x00);
    colors[1] = SDL_MapRGB(screen->format, 0x00, 0xFF, 0x00);
    colors[2] = SDL_MapRGB(screen->format, 0x00, 0x00, 0xFF);

    /* Loop, filling and waiting for a keystroke */
    frames = 0;
    then = SDL_GetTicks();
    done = 0;
    while (!done) {
        /* Check for events */
        ++frames;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_MOUSEBUTTONDOWN:
                SDL_WarpMouse(screen->w / 2, screen->h / 2);
                break;
            case SDL_KEYDOWN:
                /* Any keypress quits the app... */
            case SDL_QUIT:
                done = 1;
                break;
            default:
                break;
            }
        }
        SDL_FillRect(screen, NULL, colors[frames%3]);
        SDL_Flip(screen);
    }

    /* Print out some timing information */
    now = SDL_GetTicks();
    if (now > then) {
        double fps = ((double) frames * 1000) / (now - then);
        printf("%2.2f frames per second\n", fps);
    }
    SDL_Quit();
    return (0);
}
