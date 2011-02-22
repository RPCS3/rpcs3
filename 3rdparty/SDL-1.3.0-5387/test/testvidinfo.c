
/* Simple program -- figure out what kind of video display we have */

#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SDL.h"

#define NUM_BLITS	10
#define NUM_UPDATES	500

#define FLAG_MASK	(SDL_HWSURFACE | SDL_FULLSCREEN | SDL_DOUBLEBUF | \
                         SDL_SRCCOLORKEY | SDL_SRCALPHA | SDL_RLEACCEL  | \
                         SDL_RLEACCELOK)

#if 0
void
PrintFlags(Uint32 flags)
{
    printf("0x%8.8x", (flags & FLAG_MASK));
    if (flags & SDL_HWSURFACE) {
        printf(" SDL_HWSURFACE");
    } else {
        printf(" SDL_SWSURFACE");
    }
    if (flags & SDL_FULLSCREEN) {
        printf(" | SDL_FULLSCREEN");
    }
    if (flags & SDL_DOUBLEBUF) {
        printf(" | SDL_DOUBLEBUF");
    }
    if (flags & SDL_SRCCOLORKEY) {
        printf(" | SDL_SRCCOLORKEY");
    }
    if (flags & SDL_SRCALPHA) {
        printf(" | SDL_SRCALPHA");
    }
    if (flags & SDL_RLEACCEL) {
        printf(" | SDL_RLEACCEL");
    }
    if (flags & SDL_RLEACCELOK) {
        printf(" | SDL_RLEACCELOK");
    }
}

int
RunBlitTests(SDL_Surface * screen, SDL_Surface * bmp, int blitcount)
{
    int i, j;
    int maxx;
    int maxy;
    SDL_Rect dst;

    maxx = (int) screen->w - bmp->w + 1;
    maxy = (int) screen->h - bmp->h + 1;
    for (i = 0; i < NUM_UPDATES; ++i) {
        for (j = 0; j < blitcount; ++j) {
            if (maxx) {
                dst.x = rand() % maxx;
            } else {
                dst.x = 0;
            }
            if (maxy) {
                dst.y = rand() % maxy;
            } else {
                dst.y = 0;
            }
            dst.w = bmp->w;
            dst.h = bmp->h;
            SDL_BlitSurface(bmp, NULL, screen, &dst);
        }
        SDL_Flip(screen);
    }

    return i;
}

int
RunModeTests(SDL_Surface * screen)
{
    Uint32 then, now;
    Uint32 frames;
    float seconds;
    int i;
    Uint8 r, g, b;
    SDL_Surface *bmp, *bmpcc, *tmp;
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_KEYDOWN)
            return 0;
    }

    /* First test fills and screen update speed */
    printf("Running color fill and fullscreen update test\n");
    then = SDL_GetTicks();
    frames = 0;
    for (i = 0; i < 256; ++i) {
        r = i;
        g = 0;
        b = 0;
        SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, r, g, b));
        SDL_Flip(screen);
        ++frames;
    }
    for (i = 0; i < 256; ++i) {
        r = 0;
        g = i;
        b = 0;
        SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, r, g, b));
        SDL_Flip(screen);
        ++frames;
    }
    for (i = 0; i < 256; ++i) {
        r = 0;
        g = 0;
        b = i;
        SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, r, g, b));
        SDL_Flip(screen);
        ++frames;
    }
    now = SDL_GetTicks();
    seconds = (float) (now - then) / 1000.0f;
    if (seconds > 0.0f) {
        printf("%d fills and flips in %2.2f seconds, %2.2f FPS\n", frames,
               seconds, (float) frames / seconds);
    } else {
        printf("%d fills and flips in zero seconds!n", frames);
    }

    /* clear the screen after fill test */
    SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0, 0, 0));
    SDL_Flip(screen);

    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_KEYDOWN)
            return 0;
    }

    /* run the generic blit test */
    bmp = SDL_LoadBMP("sample.bmp");
    if (!bmp) {
        printf("Couldn't load sample.bmp: %s\n", SDL_GetError());
        return 0;
    }
    printf("Running freshly loaded blit test: %dx%d at %d bpp, flags: ",
           bmp->w, bmp->h, bmp->format->BitsPerPixel);
    PrintFlags(bmp->flags);
    printf("\n");
    then = SDL_GetTicks();
    frames = RunBlitTests(screen, bmp, NUM_BLITS);
    now = SDL_GetTicks();
    seconds = (float) (now - then) / 1000.0f;
    if (seconds > 0.0f) {
        printf("%d blits / %d updates in %2.2f seconds, %2.2f FPS\n",
               NUM_BLITS * frames, frames, seconds, (float) frames / seconds);
    } else {
        printf("%d blits / %d updates in zero seconds!\n",
               NUM_BLITS * frames, frames);
    }

    /* clear the screen after blit test */
    SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0, 0, 0));
    SDL_Flip(screen);

    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_KEYDOWN)
            return 0;
    }

    /* run the colorkeyed blit test */
    bmpcc = SDL_LoadBMP("sample.bmp");
    if (!bmpcc) {
        printf("Couldn't load sample.bmp: %s\n", SDL_GetError());
        return 0;
    }
    printf("Running freshly loaded cc blit test: %dx%d at %d bpp, flags: ",
           bmpcc->w, bmpcc->h, bmpcc->format->BitsPerPixel);
    SDL_SetColorKey(bmpcc, SDL_SRCCOLORKEY | SDL_RLEACCEL,
                    *(Uint8 *) bmpcc->pixels);

    PrintFlags(bmpcc->flags);
    printf("\n");
    then = SDL_GetTicks();
    frames = RunBlitTests(screen, bmpcc, NUM_BLITS);
    now = SDL_GetTicks();
    seconds = (float) (now - then) / 1000.0f;
    if (seconds > 0.0f) {
        printf("%d cc blits / %d updates in %2.2f seconds, %2.2f FPS\n",
               NUM_BLITS * frames, frames, seconds, (float) frames / seconds);
    } else {
        printf("%d cc blits / %d updates in zero seconds!\n",
               NUM_BLITS * frames, frames);
    }

    /* clear the screen after cc blit test */
    SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0, 0, 0));
    SDL_Flip(screen);

    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_KEYDOWN)
            return 0;
    }

    /* run the generic blit test */
    tmp = bmp;
    bmp = SDL_DisplayFormat(bmp);
    SDL_FreeSurface(tmp);
    if (!bmp) {
        printf("Couldn't convert sample.bmp: %s\n", SDL_GetError());
        return 0;
    }
    printf("Running display format blit test: %dx%d at %d bpp, flags: ",
           bmp->w, bmp->h, bmp->format->BitsPerPixel);
    PrintFlags(bmp->flags);
    printf("\n");
    then = SDL_GetTicks();
    frames = RunBlitTests(screen, bmp, NUM_BLITS);
    now = SDL_GetTicks();
    seconds = (float) (now - then) / 1000.0f;
    if (seconds > 0.0f) {
        printf("%d blits / %d updates in %2.2f seconds, %2.2f FPS\n",
               NUM_BLITS * frames, frames, seconds, (float) frames / seconds);
    } else {
        printf("%d blits / %d updates in zero seconds!\n",
               NUM_BLITS * frames, frames);
    }

    /* clear the screen after blit test */
    SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0, 0, 0));
    SDL_Flip(screen);

    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_KEYDOWN)
            return 0;
    }

    /* run the colorkeyed blit test */
    tmp = bmpcc;
    bmpcc = SDL_DisplayFormat(bmpcc);
    SDL_FreeSurface(tmp);
    if (!bmpcc) {
        printf("Couldn't convert sample.bmp: %s\n", SDL_GetError());
        return 0;
    }
    printf("Running display format cc blit test: %dx%d at %d bpp, flags: ",
           bmpcc->w, bmpcc->h, bmpcc->format->BitsPerPixel);
    PrintFlags(bmpcc->flags);
    printf("\n");
    then = SDL_GetTicks();
    frames = RunBlitTests(screen, bmpcc, NUM_BLITS);
    now = SDL_GetTicks();
    seconds = (float) (now - then) / 1000.0f;
    if (seconds > 0.0f) {
        printf("%d cc blits / %d updates in %2.2f seconds, %2.2f FPS\n",
               NUM_BLITS * frames, frames, seconds, (float) frames / seconds);
    } else {
        printf("%d cc blits / %d updates in zero seconds!\n",
               NUM_BLITS * frames, frames);
    }

    /* clear the screen after cc blit test */
    SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0, 0, 0));
    SDL_Flip(screen);

    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_KEYDOWN)
            return 0;
    }

    /* run the alpha blit test only if screen bpp>8 */
    if (bmp->format->BitsPerPixel > 8) {
        SDL_FreeSurface(bmp);
        bmp = SDL_LoadBMP("sample.bmp");
        SDL_SetAlpha(bmp, SDL_SRCALPHA, 85);    /* 85 - 33% alpha */
        tmp = bmp;
        bmp = SDL_DisplayFormat(bmp);
        SDL_FreeSurface(tmp);
        if (!bmp) {
            printf("Couldn't convert sample.bmp: %s\n", SDL_GetError());
            return 0;
        }
        printf
            ("Running display format alpha blit test: %dx%d at %d bpp, flags: ",
             bmp->w, bmp->h, bmp->format->BitsPerPixel);
        PrintFlags(bmp->flags);
        printf("\n");
        then = SDL_GetTicks();
        frames = RunBlitTests(screen, bmp, NUM_BLITS);
        now = SDL_GetTicks();
        seconds = (float) (now - then) / 1000.0f;
        if (seconds > 0.0f) {
            printf
                ("%d alpha blits / %d updates in %2.2f seconds, %2.2f FPS\n",
                 NUM_BLITS * frames, frames, seconds,
                 (float) frames / seconds);
        } else {
            printf("%d alpha blits / %d updates in zero seconds!\n",
                   NUM_BLITS * frames, frames);
        }
    }

    /* clear the screen after alpha blit test */
    SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0, 0, 0));
    SDL_Flip(screen);

    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_KEYDOWN)
            return 0;
    }

    /* run the cc+alpha blit test only if screen bpp>8 */
    if (bmp->format->BitsPerPixel > 8) {
        SDL_FreeSurface(bmpcc);
        bmpcc = SDL_LoadBMP("sample.bmp");
        SDL_SetAlpha(bmpcc, SDL_SRCALPHA, 85);  /* 85 - 33% alpha */
        SDL_SetColorKey(bmpcc, SDL_SRCCOLORKEY | SDL_RLEACCEL,
                        *(Uint8 *) bmpcc->pixels);
        tmp = bmpcc;
        bmpcc = SDL_DisplayFormat(bmpcc);
        SDL_FreeSurface(tmp);
        if (!bmpcc) {
            printf("Couldn't convert sample.bmp: %s\n", SDL_GetError());
            return 0;
        }
        printf
            ("Running display format cc+alpha blit test: %dx%d at %d bpp, flags: ",
             bmpcc->w, bmpcc->h, bmpcc->format->BitsPerPixel);
        PrintFlags(bmpcc->flags);
        printf("\n");
        then = SDL_GetTicks();
        frames = RunBlitTests(screen, bmpcc, NUM_BLITS);
        now = SDL_GetTicks();
        seconds = (float) (now - then) / 1000.0f;
        if (seconds > 0.0f) {
            printf
                ("%d cc+alpha blits / %d updates in %2.2f seconds, %2.2f FPS\n",
                 NUM_BLITS * frames, frames, seconds,
                 (float) frames / seconds);
        } else {
            printf("%d cc+alpha blits / %d updates in zero seconds!\n",
                   NUM_BLITS * frames, frames);
        }
    }

    SDL_FreeSurface(bmpcc);
    SDL_FreeSurface(bmp);

    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_KEYDOWN)
            return 0;
    }
    return 1;
}

void
RunVideoTests()
{
    static const struct
    {
        int w, h, bpp;
    } mode_list[] = {
        {
        640, 480, 8}, {
        640, 480, 16}, {
        640, 480, 32}, {
        800, 600, 8}, {
        800, 600, 16}, {
        800, 600, 32}, {
        1024, 768, 8}, {
        1024, 768, 16}, {
        1024, 768, 32}
    };
    static const Uint32 flags[] = {
        (SDL_SWSURFACE),
        (SDL_SWSURFACE | SDL_FULLSCREEN),
        (SDL_HWSURFACE | SDL_FULLSCREEN),
        (SDL_HWSURFACE | SDL_FULLSCREEN | SDL_DOUBLEBUF)
    };
    int i, j;
    SDL_Surface *screen;

    /* Test out several different video mode combinations */
    SDL_WM_SetCaption("SDL Video Benchmark", "vidtest");
    SDL_ShowCursor(0);
    for (i = 0; i < SDL_TABLESIZE(mode_list); ++i) {
        for (j = 0; j < SDL_TABLESIZE(flags); ++j) {
            printf("===================================\n");
            printf("Setting video mode: %dx%d at %d bpp, flags: ",
                   mode_list[i].w, mode_list[i].h, mode_list[i].bpp);
            PrintFlags(flags[j]);
            printf("\n");
            screen = SDL_SetVideoMode(mode_list[i].w,
                                      mode_list[i].h,
                                      mode_list[i].bpp, flags[j]);
            if (!screen) {
                printf("Setting video mode failed: %s\n", SDL_GetError());
                continue;
            }
            if ((screen->flags & FLAG_MASK) != flags[j]) {
                printf("Flags didn't match: ");
                PrintFlags(screen->flags);
                printf("\n");
                continue;
            }
            if (!RunModeTests(screen)) {
                return;
            }
        }
    }
}
#endif

int
main(int argc, char *argv[])
{
    const SDL_VideoInfo *info;
    int i, d, n;
    const char *driver;
    SDL_DisplayMode mode;
    int bpp;
    Uint32 Rmask, Gmask, Bmask, Amask;
    int nmodes;

    /* Print available video drivers */
    n = SDL_GetNumVideoDrivers();
    if (n == 0) {
        printf("No built-in video drivers\n");
    } else {
        printf("Built-in video drivers:");
        for (i = 0; i < n; ++i) {
            if (i > 0) {
                printf(",");
            }
            printf(" %s", SDL_GetVideoDriver(i));
        }
        printf("\n");
    }

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
        exit(1);
    }
    driver = SDL_GetCurrentVideoDriver();
    if (driver) {
        printf("Video driver: %s\n", driver);
    }
    printf("Number of displays: %d\n", SDL_GetNumVideoDisplays());
    for (d = 0; d < SDL_GetNumVideoDisplays(); ++d) {
        SDL_Rect bounds;

        SDL_GetDisplayBounds(d, &bounds);
        printf("Display %d: %dx%d at %d,%d\n", d,
               bounds.w, bounds.h, bounds.x, bounds.y);

        SDL_GetDesktopDisplayMode(d, &mode);
        SDL_PixelFormatEnumToMasks(mode.format, &bpp, &Rmask, &Gmask, &Bmask,
                                   &Amask);
        printf("  Current mode: %dx%d@%dHz, %d bits-per-pixel\n", mode.w,
               mode.h, mode.refresh_rate, bpp);
        if (Rmask || Gmask || Bmask) {
            printf("      Red Mask = 0x%.8x\n", Rmask);
            printf("      Green Mask = 0x%.8x\n", Gmask);
            printf("      Blue Mask = 0x%.8x\n", Bmask);
            if (Amask)
                printf("      Alpha Mask = 0x%.8x\n", Amask);
        }

        /* Print available fullscreen video modes */
        nmodes = SDL_GetNumDisplayModes(d);
        if (nmodes == 0) {
            printf("No available fullscreen video modes\n");
        } else {
            printf("  Fullscreen video modes:\n");
            for (i = 0; i < nmodes; ++i) {
                SDL_GetDisplayMode(d, i, &mode);
                SDL_PixelFormatEnumToMasks(mode.format, &bpp, &Rmask,
                                           &Gmask, &Bmask, &Amask);
                printf("    Mode %d: %dx%d@%dHz, %d bits-per-pixel\n", i,
                       mode.w, mode.h, mode.refresh_rate, bpp);
                if (Rmask || Gmask || Bmask) {
                    printf("        Red Mask = 0x%.8x\n", Rmask);
                    printf("        Green Mask = 0x%.8x\n", Gmask);
                    printf("        Blue Mask = 0x%.8x\n", Bmask);
                    if (Amask)
                        printf("        Alpha Mask = 0x%.8x\n", Amask);
                }
            }
        }
    }

    info = SDL_GetVideoInfo();
    if (info->wm_available) {
        printf("A window manager is available\n");
    }
    if (info->hw_available) {
        printf("Hardware surfaces are available (%dK video memory)\n",
               info->video_mem);
    }
    if (info->blit_hw) {
        printf("Copy blits between hardware surfaces are accelerated\n");
    }
    if (info->blit_hw_CC) {
        printf("Colorkey blits between hardware surfaces are accelerated\n");
    }
    if (info->blit_hw_A) {
        printf("Alpha blits between hardware surfaces are accelerated\n");
    }
    if (info->blit_sw) {
        printf
            ("Copy blits from software surfaces to hardware surfaces are accelerated\n");
    }
    if (info->blit_sw_CC) {
        printf
            ("Colorkey blits from software surfaces to hardware surfaces are accelerated\n");
    }
    if (info->blit_sw_A) {
        printf
            ("Alpha blits from software surfaces to hardware surfaces are accelerated\n");
    }
    if (info->blit_fill) {
        printf("Color fills on hardware surfaces are accelerated\n");
    }
    printf("Current resolution: %dx%d\n", info->current_w, info->current_h);
#if 0
    if (argv[1] && (strcmp(argv[1], "-benchmark") == 0)) {
        RunVideoTests();
    }
#endif

    SDL_Quit();
    return (0);
}
