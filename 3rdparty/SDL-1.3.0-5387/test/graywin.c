
/* Simple program:  Fill a colormap with gray and stripe it down the screen */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "SDL.h"

#ifdef TEST_VGA16               /* Define this if you want to test VGA 16-color video modes */
#define NUM_COLORS	16
#else
#define NUM_COLORS	256
#endif

/* Draw a randomly sized and colored box centered about (X,Y) */
void
DrawBox(SDL_Surface * screen, int X, int Y, int width, int height)
{
    static unsigned int seeded = 0;
    SDL_Rect area;
    Uint32 color;
    Uint32 randc;

    /* Seed the random number generator */
    if (seeded == 0) {
        srand((unsigned int)time(NULL));
        seeded = 1;
    }

    /* Get the bounds of the rectangle */
    area.w = (rand() % width);
    area.h = (rand() % height);
    area.x = X - (area.w / 2);
    area.y = Y - (area.h / 2);
    randc = (rand() % NUM_COLORS);

    if (screen->format->BytesPerPixel == 1) {
        color = randc;
    } else {
        color = SDL_MapRGB(screen->format, randc, randc, randc);
    }

    /* Do it! */
    SDL_FillRect(screen, &area, color);
    if (screen->flags & SDL_DOUBLEBUF) {
        SDL_Flip(screen);
    } else {
        SDL_UpdateRects(screen, 1, &area);
    }
}

void
DrawBackground(SDL_Surface * screen)
{
    int i, j, k;
    Uint8 *buffer;
    Uint16 *buffer16;
    Uint16 color;
    Uint8 gradient;

    /* Set the surface pixels and refresh! */
    /* Use two loops in case the surface is double-buffered (both sides) */

    for (j = 0; j < 2; ++j) {
        if (SDL_LockSurface(screen) < 0) {
            fprintf(stderr, "Couldn't lock display surface: %s\n",
                    SDL_GetError());
            return;
        }
        buffer = (Uint8 *) screen->pixels;

        if (screen->format->BytesPerPixel != 2) {
            for (i = 0; i < screen->h; ++i) {
                memset(buffer, (i * (NUM_COLORS - 1)) / screen->h,
                       screen->w * screen->format->BytesPerPixel);
                buffer += screen->pitch;
            }
        } else {
            for (i = 0; i < screen->h; ++i) {
                gradient = ((i * (NUM_COLORS - 1)) / screen->h);
                color =
                    SDL_MapRGB(screen->format, gradient, gradient, gradient);
                buffer16 = (Uint16 *) buffer;
                for (k = 0; k < screen->w; k++) {
                    *(buffer16 + k) = color;
                }
                buffer += screen->pitch;
            }
        }

        SDL_UnlockSurface(screen);
        if (screen->flags & SDL_DOUBLEBUF) {
            SDL_Flip(screen);
        } else {
            SDL_UpdateRect(screen, 0, 0, 0, 0);
            break;
        }
    }
}

SDL_Surface *
CreateScreen(Uint16 w, Uint16 h, Uint8 bpp, Uint32 flags)
{
    SDL_Surface *screen;
    int i;
    SDL_Color palette[NUM_COLORS];

    /* Set the video mode */
    screen = SDL_SetVideoMode(w, h, bpp, flags);
    if (screen == NULL) {
        fprintf(stderr, "Couldn't set display mode: %s\n", SDL_GetError());
        return (NULL);
    }
    fprintf(stderr, "Screen is in %s mode\n",
            (screen->flags & SDL_FULLSCREEN) ? "fullscreen" : "windowed");

    if (bpp == 8) {
        /* Set a gray colormap, reverse order from white to black */
        for (i = 0; i < NUM_COLORS; ++i) {
            palette[i].r = (NUM_COLORS - 1) - i * (256 / NUM_COLORS);
            palette[i].g = (NUM_COLORS - 1) - i * (256 / NUM_COLORS);
            palette[i].b = (NUM_COLORS - 1) - i * (256 / NUM_COLORS);
        }
        SDL_SetColors(screen, palette, 0, NUM_COLORS);
    }

    return (screen);
}

int
main(int argc, char *argv[])
{
    SDL_Surface *screen;
    Uint32 videoflags;
    int done;
    SDL_Event event;
    int width, height, bpp;

    /* Initialize SDL */
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
        exit(1);
    }

    /* See if we try to get a hardware colormap */
    width = 640;
    height = 480;
    bpp = 8;
    videoflags = SDL_SWSURFACE;
    while (argc > 1) {
        --argc;
        if (argv[argc - 1] && (strcmp(argv[argc - 1], "-width") == 0)) {
            width = atoi(argv[argc]);
            --argc;
        } else if (argv[argc - 1]
                   && (strcmp(argv[argc - 1], "-height") == 0)) {
            height = atoi(argv[argc]);
            --argc;
        } else if (argv[argc - 1] && (strcmp(argv[argc - 1], "-bpp") == 0)) {
            bpp = atoi(argv[argc]);
            --argc;
        } else if (argv[argc] && (strcmp(argv[argc], "-hw") == 0)) {
            videoflags |= SDL_HWSURFACE;
        } else if (argv[argc] && (strcmp(argv[argc], "-hwpalette") == 0)) {
            videoflags |= SDL_HWPALETTE;
        } else if (argv[argc] && (strcmp(argv[argc], "-flip") == 0)) {
            videoflags |= SDL_DOUBLEBUF;
        } else if (argv[argc] && (strcmp(argv[argc], "-noframe") == 0)) {
            videoflags |= SDL_NOFRAME;
        } else if (argv[argc] && (strcmp(argv[argc], "-resize") == 0)) {
            videoflags |= SDL_RESIZABLE;
        } else if (argv[argc] && (strcmp(argv[argc], "-fullscreen") == 0)) {
            videoflags |= SDL_FULLSCREEN;
        } else {
            fprintf(stderr,
                    "Usage: %s [-width] [-height] [-bpp] [-hw] [-hwpalette] [-flip] [-noframe] [-fullscreen] [-resize]\n",
                    argv[0]);
            exit(1);
        }
    }

    /* Set a video mode */
    screen = CreateScreen(width, height, bpp, videoflags);
    if (screen == NULL) {
        exit(2);
    }

    DrawBackground(screen);

    /* Wait for a keystroke */
    done = 0;
    while (!done && SDL_WaitEvent(&event)) {
        switch (event.type) {
        case SDL_MOUSEBUTTONDOWN:
            DrawBox(screen, event.button.x, event.button.y, width, height);
            break;
        case SDL_KEYDOWN:
            /* Ignore ALT-TAB for windows */
            if ((event.key.keysym.sym == SDLK_LALT) ||
                (event.key.keysym.sym == SDLK_TAB)) {
                break;
            }
            /* Center the mouse on <SPACE> */
            if (event.key.keysym.sym == SDLK_SPACE) {
                SDL_WarpMouse(width / 2, height / 2);
                break;
            }
            /* Toggle fullscreen mode on <RETURN> */
            if (event.key.keysym.sym == SDLK_RETURN) {
                videoflags ^= SDL_FULLSCREEN;
                screen = CreateScreen(screen->w, screen->h,
                                      screen->format->BitsPerPixel,
                                      videoflags);
                if (screen == NULL) {
                    fprintf(stderr, "Couldn't toggle fullscreen mode\n");
                    done = 1;
                }
                DrawBackground(screen);
                break;
            }
            /* Any other key quits the application... */
        case SDL_QUIT:
            done = 1;
            break;
        case SDL_VIDEOEXPOSE:
            DrawBackground(screen);
            break;
        case SDL_VIDEORESIZE:
            printf("Screen resized to %dx%d\n", event.resize.w,
                   event.resize.h);
            screen =
                CreateScreen(event.resize.w, event.resize.h,
                             screen->format->BitsPerPixel, videoflags);
            if (screen == NULL) {
                fprintf(stderr, "Couldn't resize video mode\n");
                done = 1;
            }
            DrawBackground(screen);
            break;
        default:
            break;
        }
    }
    SDL_Quit();
    return (0);
}
