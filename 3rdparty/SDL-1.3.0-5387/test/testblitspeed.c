/*
 * Benchmarks surface-to-surface blits in various formats.
 *
 *  Written by Ryan C. Gordon.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SDL.h"

static SDL_Surface *dest = NULL;
static SDL_Surface *src = NULL;
static int testSeconds = 10;


static int
percent(int val, int total)
{
    return ((int) ((((float) val) / ((float) total)) * 100.0f));
}

static int
randRange(int lo, int hi)
{
    return (lo + (int) (((double) hi) * rand() / (RAND_MAX + 1.0)));
}

static void
copy_trunc_str(char *str, size_t strsize, const char *flagstr)
{
    if ((strlen(str) + strlen(flagstr)) >= (strsize - 1))
        strcpy(str + (strsize - 5), " ...");
    else
        strcat(str, flagstr);
}

static void
__append_sdl_surface_flag(SDL_Surface * _surface, char *str,
                          size_t strsize, Uint32 flag, const char *flagstr)
{
    if (_surface->flags & flag)
        copy_trunc_str(str, strsize, flagstr);
}


#define append_sdl_surface_flag(a, b, c, fl) __append_sdl_surface_flag(a, b, c, fl, " " #fl)
#define print_tf_state(str, val) printf("%s: {%s}\n", str, (val) ? "true" : "false" )

static void
output_videoinfo_details(void)
{
    const SDL_VideoInfo *info = SDL_GetVideoInfo();
    printf("SDL_GetVideoInfo():\n");
    if (info == NULL)
        printf("  (null.)\n");
    else {
        print_tf_state("  hardware surface available", info->hw_available);
        print_tf_state("  window manager available", info->wm_available);
        print_tf_state("  accelerated hardware->hardware blits",
                       info->blit_hw);
        print_tf_state("  accelerated hardware->hardware colorkey blits",
                       info->blit_hw_CC);
        print_tf_state("  accelerated hardware->hardware alpha blits",
                       info->blit_hw_A);
        print_tf_state("  accelerated software->hardware blits",
                       info->blit_sw);
        print_tf_state("  accelerated software->hardware colorkey blits",
                       info->blit_sw_CC);
        print_tf_state("  accelerated software->hardware alpha blits",
                       info->blit_sw_A);
        print_tf_state("  accelerated color fills", info->blit_fill);
        printf("  video memory: (%d)\n", info->video_mem);
    }

    printf("\n");
}

static void
output_surface_details(const char *name, SDL_Surface * surface)
{
    printf("Details for %s:\n", name);

    if (surface == NULL) {
        printf("-WARNING- You've got a NULL surface!");
    } else {
        char f[256];
        printf("  width      : %d\n", surface->w);
        printf("  height     : %d\n", surface->h);
        printf("  depth      : %d bits per pixel\n",
               surface->format->BitsPerPixel);
        printf("  pitch      : %d\n", (int) surface->pitch);

        printf("  red bits   : 0x%08X mask, %d shift, %d loss\n",
               (int) surface->format->Rmask,
               (int) surface->format->Rshift, (int) surface->format->Rloss);
        printf("  green bits : 0x%08X mask, %d shift, %d loss\n",
               (int) surface->format->Gmask,
               (int) surface->format->Gshift, (int) surface->format->Gloss);
        printf("  blue bits  : 0x%08X mask, %d shift, %d loss\n",
               (int) surface->format->Bmask,
               (int) surface->format->Bshift, (int) surface->format->Bloss);
        printf("  alpha bits : 0x%08X mask, %d shift, %d loss\n",
               (int) surface->format->Amask,
               (int) surface->format->Ashift, (int) surface->format->Aloss);

        f[0] = '\0';

        /*append_sdl_surface_flag(surface, f, sizeof (f), SDL_SWSURFACE); */
        if ((surface->flags & SDL_HWSURFACE) == 0)
            copy_trunc_str(f, sizeof(f), " SDL_SWSURFACE");

        append_sdl_surface_flag(surface, f, sizeof(f), SDL_HWSURFACE);
        append_sdl_surface_flag(surface, f, sizeof(f), SDL_ASYNCBLIT);
        append_sdl_surface_flag(surface, f, sizeof(f), SDL_ANYFORMAT);
        append_sdl_surface_flag(surface, f, sizeof(f), SDL_HWPALETTE);
        append_sdl_surface_flag(surface, f, sizeof(f), SDL_DOUBLEBUF);
        append_sdl_surface_flag(surface, f, sizeof(f), SDL_FULLSCREEN);
        append_sdl_surface_flag(surface, f, sizeof(f), SDL_OPENGL);
        append_sdl_surface_flag(surface, f, sizeof(f), SDL_RESIZABLE);
        append_sdl_surface_flag(surface, f, sizeof(f), SDL_NOFRAME);
        append_sdl_surface_flag(surface, f, sizeof(f), SDL_HWACCEL);
        append_sdl_surface_flag(surface, f, sizeof(f), SDL_SRCCOLORKEY);
        append_sdl_surface_flag(surface, f, sizeof(f), SDL_RLEACCELOK);
        append_sdl_surface_flag(surface, f, sizeof(f), SDL_RLEACCEL);
        append_sdl_surface_flag(surface, f, sizeof(f), SDL_SRCALPHA);
        append_sdl_surface_flag(surface, f, sizeof(f), SDL_PREALLOC);

        if (f[0] == '\0')
            strcpy(f, " (none)");

        printf("  flags      :%s\n", f);
    }

    printf("\n");
}

static void
output_details(void)
{
    output_videoinfo_details();
    output_surface_details("Source Surface", src);
    output_surface_details("Destination Surface", dest);
}

static Uint32
blit(SDL_Surface * dst, SDL_Surface * src, int x, int y)
{
    Uint32 start = 0;
    SDL_Rect srcRect;
    SDL_Rect dstRect;

    srcRect.x = 0;
    srcRect.y = 0;
    dstRect.x = x;
    dstRect.y = y;
    dstRect.w = srcRect.w = src->w;     /* SDL will clip as appropriate. */
    dstRect.h = srcRect.h = src->h;

    start = SDL_GetTicks();
    SDL_BlitSurface(src, &srcRect, dst, &dstRect);
    return (SDL_GetTicks() - start);
}

static void
blitCentered(SDL_Surface * dst, SDL_Surface * src)
{
    int x = (dst->w - src->w) / 2;
    int y = (dst->h - src->h) / 2;
    blit(dst, src, x, y);
}

static int
atoi_hex(const char *str)
{
    if (str == NULL)
        return 0;

    if (strlen(str) > 2) {
        int retval = 0;
        if ((str[0] == '0') && (str[1] == 'x'))
            sscanf(str + 2, "%X", &retval);
        return (retval);
    }

    return (atoi(str));
}


static int
setup_test(int argc, char **argv)
{
    const char *dumpfile = NULL;
    SDL_Surface *bmp = NULL;
    Uint32 dstbpp = 32;
    Uint32 dstrmask = 0x00FF0000;
    Uint32 dstgmask = 0x0000FF00;
    Uint32 dstbmask = 0x000000FF;
    Uint32 dstamask = 0x00000000;
    Uint32 dstflags = 0;
    int dstw = 640;
    int dsth = 480;
    Uint32 srcbpp = 32;
    Uint32 srcrmask = 0x00FF0000;
    Uint32 srcgmask = 0x0000FF00;
    Uint32 srcbmask = 0x000000FF;
    Uint32 srcamask = 0x00000000;
    Uint32 srcflags = 0;
    int srcw = 640;
    int srch = 480;
    Uint32 origsrcalphaflags = 0;
    Uint32 origdstalphaflags = 0;
    Uint32 srcalphaflags = 0;
    Uint32 dstalphaflags = 0;
    Uint8 origsrcalpha = 255;
    Uint8 origdstalpha = 255;
    Uint8 srcalpha = 255;
    Uint8 dstalpha = 255;
    int screenSurface = 0;
    int i = 0;

    for (i = 1; i < argc; i++) {
        const char *arg = argv[i];

        if (strcmp(arg, "--dstbpp") == 0)
            dstbpp = atoi(argv[++i]);
        else if (strcmp(arg, "--dstrmask") == 0)
            dstrmask = atoi_hex(argv[++i]);
        else if (strcmp(arg, "--dstgmask") == 0)
            dstgmask = atoi_hex(argv[++i]);
        else if (strcmp(arg, "--dstbmask") == 0)
            dstbmask = atoi_hex(argv[++i]);
        else if (strcmp(arg, "--dstamask") == 0)
            dstamask = atoi_hex(argv[++i]);
        else if (strcmp(arg, "--dstwidth") == 0)
            dstw = atoi(argv[++i]);
        else if (strcmp(arg, "--dstheight") == 0)
            dsth = atoi(argv[++i]);
        else if (strcmp(arg, "--dsthwsurface") == 0)
            dstflags |= SDL_HWSURFACE;
        else if (strcmp(arg, "--srcbpp") == 0)
            srcbpp = atoi(argv[++i]);
        else if (strcmp(arg, "--srcrmask") == 0)
            srcrmask = atoi_hex(argv[++i]);
        else if (strcmp(arg, "--srcgmask") == 0)
            srcgmask = atoi_hex(argv[++i]);
        else if (strcmp(arg, "--srcbmask") == 0)
            srcbmask = atoi_hex(argv[++i]);
        else if (strcmp(arg, "--srcamask") == 0)
            srcamask = atoi_hex(argv[++i]);
        else if (strcmp(arg, "--srcwidth") == 0)
            srcw = atoi(argv[++i]);
        else if (strcmp(arg, "--srcheight") == 0)
            srch = atoi(argv[++i]);
        else if (strcmp(arg, "--srchwsurface") == 0)
            srcflags |= SDL_HWSURFACE;
        else if (strcmp(arg, "--seconds") == 0)
            testSeconds = atoi(argv[++i]);
        else if (strcmp(arg, "--screen") == 0)
            screenSurface = 1;
        else if (strcmp(arg, "--dumpfile") == 0)
            dumpfile = argv[++i];
        /* !!! FIXME: set colorkey. */
        else if (0) {           /* !!! FIXME: we handle some commandlines elsewhere now */
            fprintf(stderr, "Unknown commandline option: %s\n", arg);
            return (0);
        }
    }

    if (SDL_Init(SDL_INIT_VIDEO) == -1) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return (0);
    }

    bmp = SDL_LoadBMP("sample.bmp");
    if (bmp == NULL) {
        fprintf(stderr, "SDL_LoadBMP failed: %s\n", SDL_GetError());
        SDL_Quit();
        return (0);
    }

    if ((dstflags & SDL_HWSURFACE) == 0)
        dstflags |= SDL_SWSURFACE;
    if ((srcflags & SDL_HWSURFACE) == 0)
        srcflags |= SDL_SWSURFACE;

    if (screenSurface)
        dest = SDL_SetVideoMode(dstw, dsth, dstbpp, dstflags);
    else {
        dest = SDL_CreateRGBSurface(dstflags, dstw, dsth, dstbpp,
                                    dstrmask, dstgmask, dstbmask, dstamask);
    }

    if (dest == NULL) {
        fprintf(stderr, "dest surface creation failed: %s\n", SDL_GetError());
        SDL_Quit();
        return (0);
    }

    src = SDL_CreateRGBSurface(srcflags, srcw, srch, srcbpp,
                               srcrmask, srcgmask, srcbmask, srcamask);
    if (src == NULL) {
        fprintf(stderr, "src surface creation failed: %s\n", SDL_GetError());
        SDL_Quit();
        return (0);
    }

    /* handle alpha settings... */
    srcalphaflags = (src->flags & SDL_SRCALPHA) | (src->flags & SDL_RLEACCEL);
    dstalphaflags =
        (dest->flags & SDL_SRCALPHA) | (dest->flags & SDL_RLEACCEL);
    origsrcalphaflags = srcalphaflags;
    origdstalphaflags = dstalphaflags;
    SDL_GetSurfaceAlphaMod(src, &srcalpha);
    SDL_GetSurfaceAlphaMod(dest, &dstalpha);
    origsrcalpha = srcalpha;
    origdstalpha = dstalpha;
    for (i = 1; i < argc; i++) {
        const char *arg = argv[i];

        if (strcmp(arg, "--srcalpha") == 0)
            srcalpha = atoi(argv[++i]);
        else if (strcmp(arg, "--dstalpha") == 0)
            dstalpha = atoi(argv[++i]);
        else if (strcmp(arg, "--srcsrcalpha") == 0)
            srcalphaflags |= SDL_SRCALPHA;
        else if (strcmp(arg, "--srcnosrcalpha") == 0)
            srcalphaflags &= ~SDL_SRCALPHA;
        else if (strcmp(arg, "--srcrleaccel") == 0)
            srcalphaflags |= SDL_RLEACCEL;
        else if (strcmp(arg, "--srcnorleaccel") == 0)
            srcalphaflags &= ~SDL_RLEACCEL;
        else if (strcmp(arg, "--dstsrcalpha") == 0)
            dstalphaflags |= SDL_SRCALPHA;
        else if (strcmp(arg, "--dstnosrcalpha") == 0)
            dstalphaflags &= ~SDL_SRCALPHA;
        else if (strcmp(arg, "--dstrleaccel") == 0)
            dstalphaflags |= SDL_RLEACCEL;
        else if (strcmp(arg, "--dstnorleaccel") == 0)
            dstalphaflags &= ~SDL_RLEACCEL;
    }
    if ((dstalphaflags != origdstalphaflags) || (origdstalpha != dstalpha))
        SDL_SetAlpha(dest, dstalphaflags, dstalpha);
    if ((srcalphaflags != origsrcalphaflags) || (origsrcalpha != srcalpha))
        SDL_SetAlpha(src, srcalphaflags, srcalpha);

    /* set some sane defaults so we can see if the blit code is broken... */
    SDL_FillRect(dest, NULL, SDL_MapRGB(dest->format, 0, 0, 0));
    SDL_FillRect(src, NULL, SDL_MapRGB(src->format, 0, 0, 0));

    blitCentered(src, bmp);
    SDL_FreeSurface(bmp);

    if (dumpfile)
        SDL_SaveBMP(src, dumpfile);     /* make sure initial convert is sane. */

    output_details();

    return (1);
}


static void
test_blit_speed(void)
{
    Uint32 clearColor = SDL_MapRGB(dest->format, 0, 0, 0);
    Uint32 iterations = 0;
    Uint32 elasped = 0;
    Uint32 end = 0;
    Uint32 now = 0;
    Uint32 last = 0;
    int testms = testSeconds * 1000;
    int wmax = (dest->w - src->w);
    int hmax = (dest->h - src->h);
    int isScreen = (SDL_GetVideoSurface() == dest);
    SDL_Event event;

    printf("Testing blit speed for %d seconds...\n", testSeconds);

    now = SDL_GetTicks();
    end = now + testms;

    do {
        /* pump the event queue occasionally to keep OS happy... */
        if (now - last > 1000) {
            last = now;
            while (SDL_PollEvent(&event)) {     /* no-op. */
            }
        }

        iterations++;
        elasped += blit(dest, src, randRange(0, wmax), randRange(0, hmax));
        if (isScreen) {
            SDL_Flip(dest);     /* show it! */
            SDL_FillRect(dest, NULL, clearColor);       /* blank it for next time! */
        }

        now = SDL_GetTicks();
    } while (now < end);

    printf("Non-blitting crap accounted for %d percent of this run.\n",
           percent(testms - elasped, testms));

    printf("%d blits took %d ms (%d fps).\n",
           (int) iterations,
           (int) elasped,
           (int) (((float) iterations) / (((float) elasped) / 1000.0f)));
}

int
main(int argc, char **argv)
{
    int initialized = setup_test(argc, argv);
    if (initialized) {
        test_blit_speed();
        SDL_Quit();
    }
    return (!initialized);
}

/* end of testblitspeed.c ... */
