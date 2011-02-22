/* Simple program:  Create a native window and attach an SDL renderer */

#include <stdio.h>

#include "testnative.h"

#define WINDOW_W    640
#define WINDOW_H    480
#define NUM_SPRITES 100
#define MAX_SPEED 	1

static NativeWindowFactory *factories[] = {
#ifdef TEST_NATIVE_WINDOWS
    &WindowsWindowFactory,
#endif
#ifdef TEST_NATIVE_X11
    &X11WindowFactory,
#endif
#ifdef TEST_NATIVE_COCOA
    &CocoaWindowFactory,
#endif
    NULL
};
static NativeWindowFactory *factory = NULL;
static void *native_window;
static SDL_Rect *positions, *velocities;

/* Call this instead of exit(), so we can clean up SDL: atexit() is evil. */
static void
quit(int rc)
{
    SDL_VideoQuit();
    if (native_window) {
        factory->DestroyNativeWindow(native_window);
    }
    exit(rc);
}

SDL_Texture *
LoadSprite(SDL_Window * window, char *file)
{
    SDL_Surface *temp;
    SDL_Texture *sprite;

    /* Load the sprite image */
    temp = SDL_LoadBMP(file);
    if (temp == NULL) {
        fprintf(stderr, "Couldn't load %s: %s", file, SDL_GetError());
        return 0;
    }

    /* Set transparent pixel as the pixel at (0,0) */
    if (temp->format->palette) {
        SDL_SetColorKey(temp, SDL_SRCCOLORKEY, *(Uint8 *) temp->pixels);
    }

    /* Create textures from the image */
    SDL_SelectRenderer(window);
    sprite = SDL_CreateTextureFromSurface(0, temp);
    if (!sprite) {
        fprintf(stderr, "Couldn't create texture: %s\n", SDL_GetError());
        SDL_FreeSurface(temp);
        return 0;
    }
    SDL_FreeSurface(temp);

    /* We're ready to roll. :) */
    return sprite;
}

void
MoveSprites(SDL_Window * window, SDL_Texture * sprite)
{
    int i, n;
    int window_w, window_h;
    int sprite_w, sprite_h;
    SDL_Rect *position, *velocity;

    SDL_SelectRenderer(window);

    /* Query the sizes */
    SDL_GetWindowSize(window, &window_w, &window_h);
    SDL_QueryTexture(sprite, NULL, NULL, &sprite_w, &sprite_h);

    /* Move the sprite, bounce at the wall, and draw */
    n = 0;
    SDL_SetRenderDrawColor(0xA0, 0xA0, 0xA0, 0xFF);
    SDL_RenderClear();
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
        SDL_RenderCopy(sprite, NULL, position);
    }

    /* Update the screen! */
    SDL_RenderPresent();
}

int
main(int argc, char *argv[])
{
    int i, done;
    const char *driver;
    SDL_Window *window;
    SDL_Texture *sprite;
    int window_w, window_h;
    int sprite_w, sprite_h;
    SDL_Event event;

    if (SDL_VideoInit(NULL, 0) < 0) {
        fprintf(stderr, "Couldn't initialize SDL video: %s\n",
                SDL_GetError());
        exit(1);
    }
    driver = SDL_GetCurrentVideoDriver();

    /* Find a native window driver and create a native window */
    for (i = 0; factories[i]; ++i) {
        if (SDL_strcmp(driver, factories[i]->tag) == 0) {
            factory = factories[i];
            break;
        }
    }
    if (!factory) {
        fprintf(stderr, "Couldn't find native window code for %s driver\n",
                driver);
        quit(2);
    }
    printf("Creating native window for %s driver\n", driver);
    native_window = factory->CreateNativeWindow(WINDOW_W, WINDOW_H);
    if (!native_window) {
        fprintf(stderr, "Couldn't create native window\n");
        quit(3);
    }
    window = SDL_CreateWindowFrom(native_window);
    if (!window) {
        fprintf(stderr, "Couldn't create SDL window: %s\n", SDL_GetError());
        quit(4);
    }
    SDL_SetWindowTitle(window, "SDL Native Window Test");

    /* Create the renderer */
    if (SDL_CreateRenderer(window, -1, 0) < 0) {
        fprintf(stderr, "Couldn't create renderer: %s\n", SDL_GetError());
        quit(5);
    }

    /* Clear the window, load the sprite and go! */
    SDL_SelectRenderer(window);
    SDL_SetRenderDrawColor(0xA0, 0xA0, 0xA0, 0xFF);
    SDL_RenderClear();

    sprite = LoadSprite(window, "icon.bmp");
    if (!sprite) {
        quit(6);
    }

    /* Allocate memory for the sprite info */
    SDL_GetWindowSize(window, &window_w, &window_h);
    SDL_QueryTexture(sprite, NULL, NULL, &sprite_w, &sprite_h);
    positions = (SDL_Rect *) SDL_malloc(NUM_SPRITES * sizeof(SDL_Rect));
    velocities = (SDL_Rect *) SDL_malloc(NUM_SPRITES * sizeof(SDL_Rect));
    if (!positions || !velocities) {
        fprintf(stderr, "Out of memory!\n");
        quit(2);
    }
    srand(time(NULL));
    for (i = 0; i < NUM_SPRITES; ++i) {
        positions[i].x = rand() % (window_w - sprite_w);
        positions[i].y = rand() % (window_h - sprite_h);
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
            switch (event.type) {
            case SDL_WINDOWEVENT:
                switch (event.window.event) {
                case SDL_WINDOWEVENT_EXPOSED:
                    SDL_SelectRenderer(event.window.windowID);
                    SDL_SetRenderDrawColor(0xA0, 0xA0, 0xA0, 0xFF);
                    SDL_RenderClear();
                    break;
                }
                break;
            case SDL_QUIT:
                done = 1;
                break;
            default:
                break;
            }
        }
        MoveSprites(window, sprite);
    }

    quit(0);
}

/* vi: set ts=4 sw=4 expandtab: */
