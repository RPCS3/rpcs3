/* Simple program:  Move N sprites around on the screen as fast as possible */

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <fat.h>
#include <SDL/SDL.h>

#define NUM_SPRITES	10
#define MAX_SPEED 	1

SDL_Surface *sprite;
int numsprites;
SDL_Rect *sprite_rects;
SDL_Rect *positions;
SDL_Rect *velocities;
int sprites_visible;
int debug_flip;
Uint16 sprite_w, sprite_h;

/* Call this instead of exit(), so we can clean up SDL: atexit() is evil. */
static void
quit(int rc)
{
    SDL_Quit();
    exit(rc);
}

int
LoadSprite(char *file)
{
    SDL_Surface *temp;

    /* Load the sprite image */
    sprite = SDL_LoadBMP(file);
    if (sprite == NULL) {
        fprintf(stderr, "Couldn't load %s: %s", file, SDL_GetError());
        return (-1);
    }

    /* Set transparent pixel as the pixel at (0,0) */
    if (sprite->format->palette) {
        SDL_SetColorKey(sprite, (SDL_SRCCOLORKEY | SDL_RLEACCEL),
                        *(Uint8 *) sprite->pixels);
    }

    /* Convert sprite to video format */
    temp = SDL_DisplayFormat(sprite);
    SDL_FreeSurface(sprite);
    if (temp == NULL) {
        fprintf(stderr, "Couldn't convert background: %s\n", SDL_GetError());
        return (-1);
    }
    sprite = temp;

    /* We're ready to roll. :) */
    return (0);
}

void
MoveSprites(SDL_Surface * screen, Uint32 background)
{
    int i, nupdates;
    SDL_Rect area, *position, *velocity;

    nupdates = 0;
    /* Erase all the sprites if necessary */
    if (sprites_visible) {
        SDL_FillRect(screen, NULL, background);
    }

    /* Move the sprite, bounce at the wall, and draw */
    for (i = 0; i < numsprites; ++i) {
        position = &positions[i];
        velocity = &velocities[i];
        position->x += velocity->x;
        if ((position->x < 0) || (position->x >= (screen->w - sprite_w))) {
            velocity->x = -velocity->x;
            position->x += velocity->x;
        }
        position->y += velocity->y;
        if ((position->y < 0) || (position->y >= (screen->h - sprite_w))) {
            velocity->y = -velocity->y;
            position->y += velocity->y;
        }

        /* Blit the sprite onto the screen */
        area = *position;
        SDL_BlitSurface(sprite, NULL, screen, &area);
        sprite_rects[nupdates++] = area;
    }

    if (debug_flip) {
        if ((screen->flags & SDL_DOUBLEBUF) == SDL_DOUBLEBUF) {
            static int t = 0;

            Uint32 color = SDL_MapRGB(screen->format, 255, 0, 0);
            SDL_Rect r;
            r.x = t;
/* (sin((float) t * 2 * 3.1459) + 1.0) / 2.0 * (screen->w - 20); */
            r.y = 0;
            r.w = 20;
            r.h = screen->h;

            SDL_FillRect(screen, &r, color);
            t += 2;
        }
    }

    /* Update the screen! */
    if ((screen->flags & SDL_DOUBLEBUF) == SDL_DOUBLEBUF) {
        SDL_Flip(screen);
    } else {
        SDL_UpdateRects(screen, nupdates, sprite_rects);
    }
    sprites_visible = 1;
}

/* This is a way of telling whether or not to use hardware surfaces */
Uint32
FastestFlags(Uint32 flags, int width, int height, int bpp)
{
    const SDL_VideoInfo *info;

    /* Hardware acceleration is only used in fullscreen mode */
    flags |= SDL_FULLSCREEN;

    /* Check for various video capabilities */
    info = SDL_GetVideoInfo();
    if (info->blit_hw_CC && info->blit_fill) {
        /* We use accelerated colorkeying and color filling */
        flags |= SDL_HWSURFACE;
    }
    /* If we have enough video memory, and will use accelerated
       blits directly to it, then use page flipping.
     */
    if ((flags & SDL_HWSURFACE) == SDL_HWSURFACE) {
        /* Direct hardware blitting without double-buffering
           causes really bad flickering.
         */
        if (info->video_mem * 1024 > (height * width * bpp / 8)) {
            flags |= SDL_DOUBLEBUF;
        } else {
            flags &= ~SDL_HWSURFACE;
        }
    }

    /* Return the flags */
    return (flags);
}

int
main(int argc, char *argv[])
{
    SDL_Surface *screen;
    Uint8 *mem;
    int width, height;
    Uint8 video_bpp;
    Uint32 videoflags;
    Uint32 background;
    int i, done;
    SDL_Event event;
    Uint32 then, now, frames;

    consoleDemoInit();
    puts("Hello world!  Initializing FAT...");
    fatInitDefault();
    /* Initialize SDL */
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
        return (1);
    }
    puts("* initialized SDL");

    numsprites = NUM_SPRITES;
    videoflags = SDL_SWSURFACE /*| SDL_ANYFORMAT */ ;
    width = 256;
    height = 192;
    video_bpp = 15;
    debug_flip = 0;
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
            videoflags &= ~SDL_ANYFORMAT;
            --argc;
        } else if (strcmp(argv[argc], "-fast") == 0) {
            videoflags = FastestFlags(videoflags, width, height, video_bpp);
        } else if (strcmp(argv[argc], "-hw") == 0) {
            videoflags ^= SDL_HWSURFACE;
        } else if (strcmp(argv[argc], "-flip") == 0) {
            videoflags ^= SDL_DOUBLEBUF;
        } else if (strcmp(argv[argc], "-debugflip") == 0) {
            debug_flip ^= 1;
        } else if (strcmp(argv[argc], "-fullscreen") == 0) {
            videoflags ^= SDL_FULLSCREEN;
        } else if (isdigit(argv[argc][0])) {
            numsprites = atoi(argv[argc]);
        } else {
            fprintf(stderr,
                    "Usage: %s [-bpp N] [-hw] [-flip] [-fast] [-fullscreen] [numsprites]\n",
                    argv[0]);
            quit(1);
        }
    }

    /* Set video mode */
    screen = SDL_SetVideoMode(width, height, video_bpp, videoflags);
    if (!screen) {
        fprintf(stderr, "Couldn't set %dx%d video mode: %s\n",
                width, height, SDL_GetError());
        quit(2);
    }
    screen->flags &= ~SDL_PREALLOC;
    puts("* set video mode");

    /* Load the sprite */
    if (LoadSprite("icon.bmp") < 0) {
        quit(1);
    }
    puts("* loaded sprite");

    /* Allocate memory for the sprite info */
    mem = (Uint8 *) malloc(4 * sizeof(SDL_Rect) * numsprites);
    if (mem == NULL) {
        SDL_FreeSurface(sprite);
        fprintf(stderr, "Out of memory!\n");
        quit(2);
    }
    sprite_rects = (SDL_Rect *) mem;
    positions = sprite_rects;
    sprite_rects += numsprites;
    velocities = sprite_rects;
    sprite_rects += numsprites;
    sprite_w = sprite->w;
    sprite_h = sprite->h;
    srand(time(NULL));
    for (i = 0; i < numsprites; ++i) {
        positions[i].x = rand() % (screen->w - sprite_w);
        positions[i].y = rand() % (screen->h - sprite_h);
        positions[i].w = sprite->w;
        positions[i].h = sprite->h;
        velocities[i].x = 0;
        velocities[i].y = 0;
        while (!velocities[i].x && !velocities[i].y) {
            velocities[i].x = (rand() % (MAX_SPEED * 2 + 1)) - MAX_SPEED;
            velocities[i].y = (rand() % (MAX_SPEED * 2 + 1)) - MAX_SPEED;
        }
    }
    background = SDL_MapRGB(screen->format, 0x00, 0x00, 0x00);

    /* Print out information about our surfaces */
    printf("Screen is at %d bits per pixel\n", screen->format->BitsPerPixel);
    if ((screen->flags & SDL_HWSURFACE) == SDL_HWSURFACE) {
        printf("Screen is in video memory\n");
    } else {
        printf("Screen is in system memory\n");
    }
    if ((screen->flags & SDL_DOUBLEBUF) == SDL_DOUBLEBUF) {
        printf("Screen has double-buffering enabled\n");
    }
    if ((sprite->flags & SDL_HWSURFACE) == SDL_HWSURFACE) {
        printf("Sprite is in video memory\n");
    } else {
        printf("Sprite is in system memory\n");
    }
    /* Run a sample blit to trigger blit acceleration */
    {
        SDL_Rect dst;
        dst.x = 0;
        dst.y = 0;
        dst.w = sprite->w;
        dst.h = sprite->h;
        SDL_BlitSurface(sprite, NULL, screen, &dst);
        SDL_FillRect(screen, &dst, background);
    }
    if ((sprite->flags & SDL_HWACCEL) == SDL_HWACCEL) {
        printf("Sprite blit uses hardware acceleration\n");
    }
    if ((sprite->flags & SDL_RLEACCEL) == SDL_RLEACCEL) {
        printf("Sprite blit uses RLE acceleration\n");
    }

    /* Loop, blitting sprites and waiting for a keystroke */
    frames = 0;
    then = SDL_GetTicks();
    done = 0;
    sprites_visible = 0;
    puts("hello!");
    while (!done) {
        /* Check for events */
        ++frames;
        printf(".");
        swiWaitForVBlank();
        MoveSprites(screen, background);
    }
    puts("goodbye!");
    SDL_FreeSurface(sprite);
    free(mem);

    /* Print out some timing information */
    now = SDL_GetTicks();
    if (now > then) {
        printf("%2.2f frames per second\n",
               ((double) frames * 1000) / (now - then));
    }
    SDL_Quit();
    return (0);
}
