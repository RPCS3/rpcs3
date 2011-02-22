/********************************************************************************
 *                                                                              *
 * Test of the overlay used for moved pictures, test more closed to real life.  *
 * Running trojan moose :) Coded by Mike Gorchak.                               *
 *                                                                              *
 ********************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "SDL.h"

#define MOOSEPIC_W 64
#define MOOSEPIC_H 88

#define MOOSEFRAME_SIZE (MOOSEPIC_W * MOOSEPIC_H)
#define MOOSEFRAMES_COUNT 10

SDL_Color MooseColors[84] = {
    {49, 49, 49}
    , {66, 24, 0}
    , {66, 33, 0}
    , {66, 66, 66}
    ,
    {66, 115, 49}
    , {74, 33, 0}
    , {74, 41, 16}
    , {82, 33, 8}
    ,
    {82, 41, 8}
    , {82, 49, 16}
    , {82, 82, 82}
    , {90, 41, 8}
    ,
    {90, 41, 16}
    , {90, 57, 24}
    , {99, 49, 16}
    , {99, 66, 24}
    ,
    {99, 66, 33}
    , {99, 74, 33}
    , {107, 57, 24}
    , {107, 82, 41}
    ,
    {115, 57, 33}
    , {115, 66, 33}
    , {115, 66, 41}
    , {115, 74, 0}
    ,
    {115, 90, 49}
    , {115, 115, 115}
    , {123, 82, 0}
    , {123, 99, 57}
    ,
    {132, 66, 41}
    , {132, 74, 41}
    , {132, 90, 8}
    , {132, 99, 33}
    ,
    {132, 99, 66}
    , {132, 107, 66}
    , {140, 74, 49}
    , {140, 99, 16}
    ,
    {140, 107, 74}
    , {140, 115, 74}
    , {148, 107, 24}
    , {148, 115, 82}
    ,
    {148, 123, 74}
    , {148, 123, 90}
    , {156, 115, 33}
    , {156, 115, 90}
    ,
    {156, 123, 82}
    , {156, 132, 82}
    , {156, 132, 99}
    , {156, 156, 156}
    ,
    {165, 123, 49}
    , {165, 123, 90}
    , {165, 132, 82}
    , {165, 132, 90}
    ,
    {165, 132, 99}
    , {165, 140, 90}
    , {173, 132, 57}
    , {173, 132, 99}
    ,
    {173, 140, 107}
    , {173, 140, 115}
    , {173, 148, 99}
    , {173, 173, 173}
    ,
    {181, 140, 74}
    , {181, 148, 115}
    , {181, 148, 123}
    , {181, 156, 107}
    ,
    {189, 148, 123}
    , {189, 156, 82}
    , {189, 156, 123}
    , {189, 156, 132}
    ,
    {189, 189, 189}
    , {198, 156, 123}
    , {198, 165, 132}
    , {206, 165, 99}
    ,
    {206, 165, 132}
    , {206, 173, 140}
    , {206, 206, 206}
    , {214, 173, 115}
    ,
    {214, 173, 140}
    , {222, 181, 148}
    , {222, 189, 132}
    , {222, 189, 156}
    ,
    {222, 222, 222}
    , {231, 198, 165}
    , {231, 231, 231}
    , {239, 206, 173}
};


/* Call this instead of exit(), so we can clean up SDL: atexit() is evil. */
static void
quit(int rc)
{
    SDL_Quit();
    exit(rc);
}

/* All RGB2YUV conversion code and some other parts of code has been taken from testoverlay.c */

/* NOTE: These RGB conversion functions are not intended for speed,
         only as examples.
*/

void
RGBtoYUV(Uint8 * rgb, int *yuv, int monochrome, int luminance)
{
    if (monochrome) {
#if 1                           /* these are the two formulas that I found on the FourCC site... */
        yuv[0] = (int)(0.299 * rgb[0] + 0.587 * rgb[1] + 0.114 * rgb[2]);
        yuv[1] = 128;
        yuv[2] = 128;
#else
        yuv[0] = (int)(0.257 * rgb[0]) + (0.504 * rgb[1]) + (0.098 * rgb[2]) + 16;
        yuv[1] = 128;
        yuv[2] = 128;
#endif
    } else {
#if 1                           /* these are the two formulas that I found on the FourCC site... */
        yuv[0] = (int)(0.299 * rgb[0] + 0.587 * rgb[1] + 0.114 * rgb[2]);
        yuv[1] = (int)((rgb[2] - yuv[0]) * 0.565 + 128);
        yuv[2] = (int)((rgb[0] - yuv[0]) * 0.713 + 128);
#else
        yuv[0] = (0.257 * rgb[0]) + (0.504 * rgb[1]) + (0.098 * rgb[2]) + 16;
        yuv[1] = 128 - (0.148 * rgb[0]) - (0.291 * rgb[1]) + (0.439 * rgb[2]);
        yuv[2] = 128 + (0.439 * rgb[0]) - (0.368 * rgb[1]) - (0.071 * rgb[2]);
#endif
    }

    if (luminance != 100) {
        yuv[0] = yuv[0] * luminance / 100;
        if (yuv[0] > 255)
            yuv[0] = 255;
    }
}

void
ConvertRGBtoYV12(Uint8 *rgb, Uint8 *out, int w, int h,
                 int monochrome, int luminance)
{
    int x, y;
    int yuv[3];
    Uint8 *op[3];

    op[0] = out;
    op[1] = op[0] + w*h;
    op[2] = op[1] + w*h/4;
    for (y = 0; y < h; ++y) {
        for (x = 0; x < w; ++x) {
            RGBtoYUV(rgb, yuv, monochrome, luminance);
            *(op[0]++) = yuv[0];
            if (x % 2 == 0 && y % 2 == 0) {
                *(op[1]++) = yuv[2];
                *(op[2]++) = yuv[1];
            }
            rgb += 3;
        }
    }
}

void
ConvertRGBtoIYUV(SDL_Surface * s, SDL_Overlay * o, int monochrome,
                 int luminance)
{
    int x, y;
    int yuv[3];
    Uint8 *p, *op[3];

    SDL_LockSurface(s);
    SDL_LockYUVOverlay(o);

    /* Convert */
    for (y = 0; y < s->h && y < o->h; y++) {
        p = ((Uint8 *) s->pixels) + s->pitch * y;
        op[0] = o->pixels[0] + o->pitches[0] * y;
        op[1] = o->pixels[1] + o->pitches[1] * (y / 2);
        op[2] = o->pixels[2] + o->pitches[2] * (y / 2);
        for (x = 0; x < s->w && x < o->w; x++) {
            RGBtoYUV(p, yuv, monochrome, luminance);
            *(op[0]++) = yuv[0];
            if (x % 2 == 0 && y % 2 == 0) {
                *(op[1]++) = yuv[1];
                *(op[2]++) = yuv[2];
            }
            p += s->format->BytesPerPixel;
        }
    }

    SDL_UnlockYUVOverlay(o);
    SDL_UnlockSurface(s);
}

void
ConvertRGBtoUYVY(SDL_Surface * s, SDL_Overlay * o, int monochrome,
                 int luminance)
{
    int x, y;
    int yuv[3];
    Uint8 *p, *op;

    SDL_LockSurface(s);
    SDL_LockYUVOverlay(o);

    for (y = 0; y < s->h && y < o->h; y++) {
        p = ((Uint8 *) s->pixels) + s->pitch * y;
        op = o->pixels[0] + o->pitches[0] * y;
        for (x = 0; x < s->w && x < o->w; x++) {
            RGBtoYUV(p, yuv, monochrome, luminance);
            if (x % 2 == 0) {
                *(op++) = yuv[1];
                *(op++) = yuv[0];
                *(op++) = yuv[2];
            } else
                *(op++) = yuv[0];

            p += s->format->BytesPerPixel;
        }
    }

    SDL_UnlockYUVOverlay(o);
    SDL_UnlockSurface(s);
}

void
ConvertRGBtoYVYU(SDL_Surface * s, SDL_Overlay * o, int monochrome,
                 int luminance)
{
    int x, y;
    int yuv[3];
    Uint8 *p, *op;

    SDL_LockSurface(s);
    SDL_LockYUVOverlay(o);

    for (y = 0; y < s->h && y < o->h; y++) {
        p = ((Uint8 *) s->pixels) + s->pitch * y;
        op = o->pixels[0] + o->pitches[0] * y;
        for (x = 0; x < s->w && x < o->w; x++) {
            RGBtoYUV(p, yuv, monochrome, luminance);
            if (x % 2 == 0) {
                *(op++) = yuv[0];
                *(op++) = yuv[2];
                op[1] = yuv[1];
            } else {
                *op = yuv[0];
                op += 2;
            }

            p += s->format->BytesPerPixel;
        }
    }

    SDL_UnlockYUVOverlay(o);
    SDL_UnlockSurface(s);
}

void
ConvertRGBtoYUY2(SDL_Surface * s, SDL_Overlay * o, int monochrome,
                 int luminance)
{
    int x, y;
    int yuv[3];
    Uint8 *p, *op;

    SDL_LockSurface(s);
    SDL_LockYUVOverlay(o);

    for (y = 0; y < s->h && y < o->h; y++) {
        p = ((Uint8 *) s->pixels) + s->pitch * y;
        op = o->pixels[0] + o->pitches[0] * y;
        for (x = 0; x < s->w && x < o->w; x++) {
            RGBtoYUV(p, yuv, monochrome, luminance);
            if (x % 2 == 0) {
                *(op++) = yuv[0];
                *(op++) = yuv[1];
                op[1] = yuv[2];
            } else {
                *op = yuv[0];
                op += 2;
            }

            p += s->format->BytesPerPixel;
        }
    }

    SDL_UnlockYUVOverlay(o);
    SDL_UnlockSurface(s);
}

static void
PrintUsage(char *argv0)
{
    fprintf(stderr, "Usage: %s [arg] [arg] [arg] ...\n", argv0);
    fprintf(stderr, "\n");
    fprintf(stderr, "Where 'arg' is any of the following options:\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "    -fps <frames per second>\n");
    fprintf(stderr, "    -nodelay\n");
    fprintf(stderr, "    -format <fmt> (one of the: YV12, IYUV, YUY2, UYVY, YVYU)\n");
    fprintf(stderr, "    -scale <scale factor> (initial scale of the overlay)\n");
    fprintf(stderr, "    -help (shows this help)\n");
    fprintf(stderr, "\n");
    fprintf(stderr,
            "Press ESC to exit, or SPACE to freeze the movie while application running.\n");
    fprintf(stderr, "\n");
}

int
main(int argc, char **argv)
{
    Uint8 *RawMooseData;
    SDL_RWops *handle;
    int window_w;
    int window_h;
    SDL_Window *window;
    SDL_Renderer *renderer;
    Uint8 MooseFrame[MOOSEFRAMES_COUNT][MOOSEFRAME_SIZE*2];
    SDL_Texture *MooseTexture;
    SDL_Rect displayrect;
    SDL_Event event;
    int paused = 0;
    int i, j;
    int fps = 12;
    int fpsdelay;
    int nodelay = 0;
    Uint32 pixel_format = SDL_PIXELFORMAT_YV12;
    int scale = 5;
    SDL_bool done = SDL_FALSE;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE) < 0) {
        fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
        return 3;
    }

    while (argc > 1) {
        if (strcmp(argv[1], "-fps") == 0) {
            if (argv[2]) {
                fps = atoi(argv[2]);
                if (fps == 0) {
                    fprintf(stderr,
                            "The -fps option requires an argument [from 1 to 1000], default is 12.\n");
                    quit(10);
                }
                if ((fps < 0) || (fps > 1000)) {
                    fprintf(stderr,
                            "The -fps option must be in range from 1 to 1000, default is 12.\n");
                    quit(10);
                }
                argv += 2;
                argc -= 2;
            } else {
                fprintf(stderr,
                        "The -fps option requires an argument [from 1 to 1000], default is 12.\n");
                quit(10);
            }
        } else if (strcmp(argv[1], "-nodelay") == 0) {
            nodelay = 1;
            argv += 1;
            argc -= 1;
        } else if (strcmp(argv[1], "-format") == 0) {
            if (argv[2]) {
                if (!strcmp(argv[2], "YV12"))
                    pixel_format = SDL_PIXELFORMAT_YV12;
                else if (!strcmp(argv[2], "IYUV"))
                    pixel_format = SDL_PIXELFORMAT_IYUV;
                else if (!strcmp(argv[2], "YUY2"))
                    pixel_format = SDL_PIXELFORMAT_YUY2;
                else if (!strcmp(argv[2], "UYVY"))
                    pixel_format = SDL_PIXELFORMAT_UYVY;
                else if (!strcmp(argv[2], "YVYU"))
                    pixel_format = SDL_PIXELFORMAT_YVYU;
                else {
                    fprintf(stderr,
                            "The -format option %s is not recognized, see help for info.\n",
                            argv[2]);
                    quit(10);
                }
                argv += 2;
                argc -= 2;
            } else {
                fprintf(stderr,
                        "The -format option requires an argument, default is YUY2.\n");
                quit(10);
            }
        } else if (strcmp(argv[1], "-scale") == 0) {
            if (argv[2]) {
                scale = atoi(argv[2]);
                if (scale == 0) {
                    fprintf(stderr,
                            "The -scale option requires an argument [from 1 to 50], default is 5.\n");
                    quit(10);
                }
                if ((scale < 0) || (scale > 50)) {
                    fprintf(stderr,
                            "The -scale option must be in range from 1 to 50, default is 5.\n");
                    quit(10);
                }
                argv += 2;
                argc -= 2;
            } else {
                fprintf(stderr,
                        "The -fps option requires an argument [from 1 to 1000], default is 12.\n");
                quit(10);
            }
        } else if ((strcmp(argv[1], "-help") == 0)
                   || (strcmp(argv[1], "-h") == 0)) {
            PrintUsage(argv[0]);
            quit(0);
        } else {
            fprintf(stderr, "Unrecognized option: %s.\n", argv[1]);
            quit(10);
        }
        break;
    }

    RawMooseData = (Uint8 *) malloc(MOOSEFRAME_SIZE * MOOSEFRAMES_COUNT);
    if (RawMooseData == NULL) {
        fprintf(stderr, "Can't allocate memory for movie !\n");
        free(RawMooseData);
        quit(1);
    }

    /* load the trojan moose images */
    handle = SDL_RWFromFile("moose.dat", "rb");
    if (handle == NULL) {
        fprintf(stderr, "Can't find the file moose.dat !\n");
        free(RawMooseData);
        quit(2);
    }

    SDL_RWread(handle, RawMooseData, MOOSEFRAME_SIZE, MOOSEFRAMES_COUNT);

    SDL_RWclose(handle);

    /* Create the window and renderer */
    window_w = MOOSEPIC_W * scale;
    window_h = MOOSEPIC_H * scale;
    window = SDL_CreateWindow("Happy Moose",
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              window_w, window_h,
                              SDL_WINDOW_SHOWN|SDL_WINDOW_RESIZABLE);
    if (!window) {
        fprintf(stderr, "Couldn't set create window: %s\n", SDL_GetError());
        free(RawMooseData);
        quit(4);
    }

    renderer = SDL_CreateRenderer(window, -1, 0);
    if (!renderer) {
        fprintf(stderr, "Couldn't set create renderer: %s\n", SDL_GetError());
        free(RawMooseData);
        quit(4);
    }

    MooseTexture = SDL_CreateTexture(renderer, pixel_format, SDL_TEXTUREACCESS_STREAMING, MOOSEPIC_W, MOOSEPIC_H);
    if (!MooseTexture) {
        fprintf(stderr, "Couldn't set create texture: %s\n", SDL_GetError());
        free(RawMooseData);
        quit(5);
    }

    for (i = 0; i < MOOSEFRAMES_COUNT; i++) {
        Uint8 MooseFrameRGB[MOOSEFRAME_SIZE*3];
        Uint8 *rgb;
        Uint8 *frame;

        rgb = MooseFrameRGB;
        frame = RawMooseData + i * MOOSEFRAME_SIZE;
        for (j = 0; j < MOOSEFRAME_SIZE; ++j) {
            rgb[0] = MooseColors[frame[j]].r;
            rgb[1] = MooseColors[frame[j]].g;
            rgb[2] = MooseColors[frame[j]].b;
            rgb += 3;
        }
        ConvertRGBtoYV12(MooseFrameRGB, MooseFrame[i], MOOSEPIC_W, MOOSEPIC_H, 0, 100);
    }

    free(RawMooseData);

    /* set the start frame */
    i = 0;
    if (nodelay) {
        fpsdelay = 0;
    } else {
        fpsdelay = 1000 / fps;
    }

    displayrect.x = 0;
    displayrect.y = 0;
    displayrect.w = window_w;
    displayrect.h = window_h;

    /* Ignore key up events, they don't even get filtered */
    SDL_EventState(SDL_KEYUP, SDL_IGNORE);

    /* Loop, waiting for QUIT or RESIZE */
    while (!done) {
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                    SDL_RenderSetViewport(renderer, NULL);
                    displayrect.w = window_w = event.window.data1;
                    displayrect.h = window_h = event.window.data2;
                }
                break;
            case SDL_MOUSEBUTTONDOWN:
                displayrect.x = event.button.x - window_w / 2;
                displayrect.y = event.button.y - window_h / 2;
                break;
            case SDL_MOUSEMOTION:
                if (event.motion.state) {
                    displayrect.x = event.motion.x - window_w / 2;
                    displayrect.y = event.motion.y - window_h / 2;
                }
                break;
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_SPACE) {
                    paused = !paused;
                    break;
                }
                if (event.key.keysym.sym != SDLK_ESCAPE) {
                    break;
                }
            case SDL_QUIT:
                done = SDL_TRUE;
                break;
            }
        }
        SDL_Delay(fpsdelay);

        if (!paused) {
            i = (i + 1) % MOOSEFRAMES_COUNT;

            SDL_UpdateTexture(MooseTexture, NULL, MooseFrame[i], MOOSEPIC_W*SDL_BYTESPERPIXEL(pixel_format));
        }
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, MooseTexture, NULL, &displayrect);
        SDL_RenderPresent(renderer);
    }
    SDL_DestroyRenderer(renderer);
    quit(0);
    return 0;
}

/* vi: set ts=4 sw=4 expandtab: */
