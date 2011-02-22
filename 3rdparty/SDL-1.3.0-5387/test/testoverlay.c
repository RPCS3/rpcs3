
/* Bring up a window and play with it */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define BENCHMARK_SDL

#define NOTICE(X)	printf("%s", X);

#define WINDOW_WIDTH  640
#define WINDOW_HEIGHT 480

#include "SDL.h"

SDL_Surface *screen, *pic;
SDL_Overlay *overlay;
int scale;
int monochrome;
int luminance;
int w, h;

/* Call this instead of exit(), so we can clean up SDL: atexit() is evil. */
static void
quit(int rc)
{
    SDL_Quit();
    exit(rc);
}

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
        yuv[0] = (int)((0.257 * rgb[0]) + (0.504 * rgb[1]) + (0.098 * rgb[2]) + 16);
        yuv[1] = 128;
        yuv[2] = 128;
#endif
    } else {
#if 1                           /* these are the two formulas that I found on the FourCC site... */
        yuv[0] = (int)(0.299 * rgb[0] + 0.587 * rgb[1] + 0.114 * rgb[2]);
        yuv[1] = (int)((rgb[2] - yuv[0]) * 0.565 + 128);
        yuv[2] = (int)((rgb[0] - yuv[0]) * 0.713 + 128);
#else
        yuv[0] = (int)((0.257 * rgb[0]) + (0.504 * rgb[1]) + (0.098 * rgb[2]) + 16);
        yuv[1] = (int)(128 - (0.148 * rgb[0]) - (0.291 * rgb[1]) + (0.439 * rgb[2]));
        yuv[2] = (int)(128 + (0.439 * rgb[0]) - (0.368 * rgb[1]) - (0.071 * rgb[2]));
#endif
    }

    if (luminance != 100) {
        yuv[0] = yuv[0] * luminance / 100;
        if (yuv[0] > 255)
            yuv[0] = 255;
    }

    /* clamp values...if you need to, we don't seem to have a need */
    /*
       for(i=0;i<3;i++)
       {
       if(yuv[i]<0)
       yuv[i]=0;
       if(yuv[i]>255)
       yuv[i]=255;
       }
     */
}

void
ConvertRGBtoYV12(SDL_Surface * s, SDL_Overlay * o, int monochrome,
                 int luminance)
{
    int x, y;
    int yuv[3];
    Uint8 *p, *op[3];

    SDL_LockSurface(s);
    SDL_LockYUVOverlay(o);

    /* Black initialization */
    /*
       memset(o->pixels[0],0,o->pitches[0]*o->h);
       memset(o->pixels[1],128,o->pitches[1]*((o->h+1)/2));
       memset(o->pixels[2],128,o->pitches[2]*((o->h+1)/2));
     */

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
                *(op[1]++) = yuv[2];
                *(op[2]++) = yuv[1];
            }
            p += s->format->BytesPerPixel;
        }
    }

    SDL_UnlockYUVOverlay(o);
    SDL_UnlockSurface(s);
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

    /* Black initialization */
    /*
       memset(o->pixels[0],0,o->pitches[0]*o->h);
       memset(o->pixels[1],128,o->pitches[1]*((o->h+1)/2));
       memset(o->pixels[2],128,o->pitches[2]*((o->h+1)/2));
     */

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

void
Draw()
{
    SDL_Rect rect;
    int i;
    int disp;

    if (!scale) {
        rect.w = overlay->w;
        rect.h = overlay->h;
        for (i = 0; i < h - rect.h && i < w - rect.w; i++) {
            rect.x = i;
            rect.y = i;
            SDL_DisplayYUVOverlay(overlay, &rect);
        }
    } else {
        rect.w = overlay->w / 2;
        rect.h = overlay->h / 2;
        rect.x = (w - rect.w) / 2;
        rect.y = (h - rect.h) / 2;
        disp = rect.y - 1;
        for (i = 0; i < disp; i++) {
            rect.w += 2;
            rect.h += 2;
            rect.x--;
            rect.y--;
            SDL_DisplayYUVOverlay(overlay, &rect);
        }
    }
    printf("Displayed %d times.\n", i);
}

static void
PrintUsage(char *argv0)
{
    fprintf(stderr, "Usage: %s [arg] [arg] [arg] ...\n", argv0);
    fprintf(stderr, "Where 'arg' is one of:\n");
    fprintf(stderr, "	-delay <seconds>\n");
    fprintf(stderr, "	-width <pixels>\n");
    fprintf(stderr, "	-height <pixels>\n");
    fprintf(stderr, "	-bpp <bits>\n");
    fprintf(stderr,
            "	-format <fmt> (one of the: YV12, IYUV, YUY2, UYVY, YVYU)\n");
    fprintf(stderr, "	-hw\n");
    fprintf(stderr, "	-flip\n");
    fprintf(stderr,
            "	-scale (test scaling features, from 50%% upto window size)\n");
    fprintf(stderr, "	-mono (use monochromatic RGB2YUV conversion)\n");
    fprintf(stderr,
            "	-lum <perc> (use luminance correction during RGB2YUV conversion,\n");
    fprintf(stderr,
            "	             from 0%% to unlimited, normal is 100%%)\n");
    fprintf(stderr, "	-help (shows this help)\n");
    fprintf(stderr, "	-fullscreen (test overlay in fullscreen mode)\n");
}

int
main(int argc, char **argv)
{
    char *argv0 = argv[0];
    int flip;
    int delay;
    int desired_bpp;
    Uint32 video_flags, overlay_format;
    char *bmpfile;
#ifdef BENCHMARK_SDL
    Uint32 then, now;
#endif
    int i;

    /* Set default options and check command-line */
    flip = 0;
    scale = 0;
    monochrome = 0;
    luminance = 100;
    delay = 1;
    w = WINDOW_WIDTH;
    h = WINDOW_HEIGHT;
    desired_bpp = 0;
    video_flags = 0;
    overlay_format = SDL_YV12_OVERLAY;

    while (argc > 1) {
        if (strcmp(argv[1], "-delay") == 0) {
            if (argv[2]) {
                delay = atoi(argv[2]);
                argv += 2;
                argc -= 2;
            } else {
                fprintf(stderr, "The -delay option requires an argument\n");
                return (1);
            }
        } else if (strcmp(argv[1], "-width") == 0) {
            if (argv[2] && ((w = atoi(argv[2])) > 0)) {
                argv += 2;
                argc -= 2;
            } else {
                fprintf(stderr, "The -width option requires an argument\n");
                return (1);
            }
        } else if (strcmp(argv[1], "-height") == 0) {
            if (argv[2] && ((h = atoi(argv[2])) > 0)) {
                argv += 2;
                argc -= 2;
            } else {
                fprintf(stderr, "The -height option requires an argument\n");
                return (1);
            }
        } else if (strcmp(argv[1], "-bpp") == 0) {
            if (argv[2]) {
                desired_bpp = atoi(argv[2]);
                argv += 2;
                argc -= 2;
            } else {
                fprintf(stderr, "The -bpp option requires an argument\n");
                return (1);
            }
        } else if (strcmp(argv[1], "-lum") == 0) {
            if (argv[2]) {
                luminance = atoi(argv[2]);
                argv += 2;
                argc -= 2;
            } else {
                fprintf(stderr, "The -lum option requires an argument\n");
                return (1);
            }
        } else if (strcmp(argv[1], "-format") == 0) {
            if (argv[2]) {
                if (!strcmp(argv[2], "YV12"))
                    overlay_format = SDL_YV12_OVERLAY;
                else if (!strcmp(argv[2], "IYUV"))
                    overlay_format = SDL_IYUV_OVERLAY;
                else if (!strcmp(argv[2], "YUY2"))
                    overlay_format = SDL_YUY2_OVERLAY;
                else if (!strcmp(argv[2], "UYVY"))
                    overlay_format = SDL_UYVY_OVERLAY;
                else if (!strcmp(argv[2], "YVYU"))
                    overlay_format = SDL_YVYU_OVERLAY;
                else {
                    fprintf(stderr,
                            "The -format option %s is not recognized\n",
                            argv[2]);
                    return (1);
                }
                argv += 2;
                argc -= 2;
            } else {
                fprintf(stderr, "The -format option requires an argument\n");
                return (1);
            }
        } else if (strcmp(argv[1], "-hw") == 0) {
            video_flags |= SDL_HWSURFACE;
            argv += 1;
            argc -= 1;
        } else if (strcmp(argv[1], "-flip") == 0) {
            video_flags |= SDL_DOUBLEBUF;
            argv += 1;
            argc -= 1;
        } else if (strcmp(argv[1], "-scale") == 0) {
            scale = 1;
            argv += 1;
            argc -= 1;
        } else if (strcmp(argv[1], "-mono") == 0) {
            monochrome = 1;
            argv += 1;
            argc -= 1;
        } else if ((strcmp(argv[1], "-help") == 0)
                   || (strcmp(argv[1], "-h") == 0)) {
            PrintUsage(argv0);
            return (1);
        } else if (strcmp(argv[1], "-fullscreen") == 0) {
            video_flags |= SDL_FULLSCREEN;
            argv += 1;
            argc -= 1;
        } else
            break;
    }
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
        return (1);
    }

    /* Initialize the display */
    screen = SDL_SetVideoMode(w, h, desired_bpp, video_flags);
    if (screen == NULL) {
        fprintf(stderr, "Couldn't set %dx%dx%d video mode: %s\n",
                w, h, desired_bpp, SDL_GetError());
        quit(1);
    }
    printf("Set%s %dx%dx%d mode\n",
           screen->flags & SDL_FULLSCREEN ? " fullscreen" : "",
           screen->w, screen->h, screen->format->BitsPerPixel);
    printf("(video surface located in %s memory)\n",
           (screen->flags & SDL_HWSURFACE) ? "video" : "system");
    if (screen->flags & SDL_DOUBLEBUF) {
        printf("Double-buffering enabled\n");
        flip = 1;
    }

    /* Set the window manager title bar */
    SDL_WM_SetCaption("SDL test overlay", "testoverlay");

    /* Load picture */
    bmpfile = (argv[1] ? argv[1] : "sample.bmp");
    pic = SDL_LoadBMP(bmpfile);
    if (pic == NULL) {
        fprintf(stderr, "Couldn't load %s: %s\n", bmpfile, SDL_GetError());
        quit(1);
    }

    /* Convert the picture to 32bits, for easy conversion */
    {
        SDL_Surface *newsurf;
        SDL_PixelFormat format;

        format.palette = NULL;
        format.BitsPerPixel = 32;
        format.BytesPerPixel = 4;
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
        format.Rshift = 0;
        format.Gshift = 8;
        format.Bshift = 16;
#else
        format.Rshift = 24;
        format.Gshift = 16;
        format.Bshift = 8;
#endif
        format.Ashift = 0;
        format.Rmask = 0xff << format.Rshift;
        format.Gmask = 0xff << format.Gshift;
        format.Bmask = 0xff << format.Bshift;
        format.Amask = 0;
        format.Rloss = 0;
        format.Gloss = 0;
        format.Bloss = 0;
        format.Aloss = 8;

        newsurf = SDL_ConvertSurface(pic, &format, SDL_SWSURFACE);
        if (!newsurf) {
            fprintf(stderr, "Couldn't convert picture to 32bits RGB: %s\n",
                    SDL_GetError());
            quit(1);
        }
        SDL_FreeSurface(pic);
        pic = newsurf;
    }

    /* Create the overlay */
    overlay = SDL_CreateYUVOverlay(pic->w, pic->h, overlay_format, screen);
    if (overlay == NULL) {
        fprintf(stderr, "Couldn't create overlay: %s\n", SDL_GetError());
        quit(1);
    }
    printf("Created %dx%dx%d %s %s overlay\n", overlay->w, overlay->h,
           overlay->planes, overlay->hw_overlay ? "hardware" : "software",
           overlay->format == SDL_YV12_OVERLAY ? "YV12" : overlay->format ==
           SDL_IYUV_OVERLAY ? "IYUV" : overlay->format ==
           SDL_YUY2_OVERLAY ? "YUY2" : overlay->format ==
           SDL_UYVY_OVERLAY ? "UYVY" : overlay->format ==
           SDL_YVYU_OVERLAY ? "YVYU" : "Unknown");
    for (i = 0; i < overlay->planes; i++) {
        printf("  plane %d: pitch=%d\n", i, overlay->pitches[i]);
    }

    /* Convert to YUV, and draw to the overlay */
#ifdef BENCHMARK_SDL
    then = SDL_GetTicks();
#endif
    switch (overlay->format) {
    case SDL_YV12_OVERLAY:
        ConvertRGBtoYV12(pic, overlay, monochrome, luminance);
        break;
    case SDL_UYVY_OVERLAY:
        ConvertRGBtoUYVY(pic, overlay, monochrome, luminance);
        break;
    case SDL_YVYU_OVERLAY:
        ConvertRGBtoYVYU(pic, overlay, monochrome, luminance);
        break;
    case SDL_YUY2_OVERLAY:
        ConvertRGBtoYUY2(pic, overlay, monochrome, luminance);
        break;
    case SDL_IYUV_OVERLAY:
        ConvertRGBtoIYUV(pic, overlay, monochrome, luminance);
        break;
    default:
        printf("cannot convert RGB picture to obtained YUV format!\n");
        quit(1);
        break;
    }
#ifdef BENCHMARK_SDL
    now = SDL_GetTicks();
    printf("Conversion Time: %d milliseconds\n", now - then);
#endif

    /* Do all the drawing work */
#ifdef BENCHMARK_SDL
    then = SDL_GetTicks();
#endif
    Draw();
#ifdef BENCHMARK_SDL
    now = SDL_GetTicks();
    printf("Time: %d milliseconds\n", now - then);
#endif
    SDL_Delay(delay * 1000);
    SDL_Quit();
    return (0);
}
