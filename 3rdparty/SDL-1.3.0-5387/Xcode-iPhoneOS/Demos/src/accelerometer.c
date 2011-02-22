/*
 *	accelerometer.c
 *	written by Holmes Futrell
 *	use however you want
 */

#include "SDL.h"
#include "math.h"
#include "common.h"

#define MILLESECONDS_PER_FRAME 16       /* about 60 frames per second */
#define DAMPING 0.5f;           /* after bouncing off a wall, damping coefficient determines final speed */
#define FRICTION 0.0008f        /* coefficient of acceleration that opposes direction of motion */
#define GRAVITY_CONSTANT 0.004f /* how sensitive the ship is to the accelerometer */

/*	If we aren't on an iPhone, then this definition ought to yield reasonable behavior */
#ifndef SDL_IPHONE_MAX_GFORCE
#define SDL_IPHONE_MAX_GFORCE 5.0f
#endif

static SDL_Joystick *accelerometer;     /* used for controlling the ship */

static struct
{
    float x, y;                 /* position of ship */
    float vx, vy;               /* velocity of ship (in pixels per millesecond) */
    SDL_Rect rect;              /* (drawn) position and size of ship */
} shipData;

static SDL_Texture *ship = 0;        /* texture for spaceship */
static SDL_Texture *space = 0;       /* texture for space (background */

void
render(SDL_Renderer *renderer)
{


    /* get joystick (accelerometer) axis values and normalize them */
    float ax = SDL_JoystickGetAxis(accelerometer, 0);
    float ay = -SDL_JoystickGetAxis(accelerometer, 1);

    /* ship screen constraints */
    Uint32 minx = 0.0f;
    Uint32 maxx = SCREEN_WIDTH - shipData.rect.w;
    Uint32 miny = 0.0f;
    Uint32 maxy = SCREEN_HEIGHT - shipData.rect.h;

#define SINT16_MAX ((float)(0x7FFF))

    /* update velocity from accelerometer
       the factor SDL_IPHONE_MAX_G_FORCE / SINT16_MAX converts between 
       SDL's units reported from the joytick, and units of g-force, as reported by the accelerometer
     */
    shipData.vx +=
        ax * SDL_IPHONE_MAX_GFORCE / SINT16_MAX * GRAVITY_CONSTANT *
        MILLESECONDS_PER_FRAME;
    shipData.vy +=
        ay * SDL_IPHONE_MAX_GFORCE / SINT16_MAX * GRAVITY_CONSTANT *
        MILLESECONDS_PER_FRAME;

    float speed = sqrt(shipData.vx * shipData.vx + shipData.vy * shipData.vy);

    if (speed > 0) {
        /* compensate for friction */
        float dirx = shipData.vx / speed;   /* normalized x velocity */
        float diry = shipData.vy / speed;   /* normalized y velocity */

        /* update velocity due to friction */
        if (speed - FRICTION * MILLESECONDS_PER_FRAME > 0) {
            /* apply friction */
            shipData.vx -= dirx * FRICTION * MILLESECONDS_PER_FRAME;
            shipData.vy -= diry * FRICTION * MILLESECONDS_PER_FRAME;
        } else {
            /* applying friction would MORE than stop the ship, so just stop the ship */
            shipData.vx = 0.0f;
            shipData.vy = 0.0f;
        }
    }

    /* update ship location */
    shipData.x += shipData.vx * MILLESECONDS_PER_FRAME;
    shipData.y += shipData.vy * MILLESECONDS_PER_FRAME;

    if (shipData.x > maxx) {
        shipData.x = maxx;
        shipData.vx = -shipData.vx * DAMPING;
    } else if (shipData.x < minx) {
        shipData.x = minx;
        shipData.vx = -shipData.vx * DAMPING;
    }
    if (shipData.y > maxy) {
        shipData.y = maxy;
        shipData.vy = -shipData.vy * DAMPING;
    } else if (shipData.y < miny) {
        shipData.y = miny;
        shipData.vy = -shipData.vy * DAMPING;
    }

    /* draw the background */
    SDL_RenderCopy(renderer, space, NULL, NULL);

    /* draw the ship */
    shipData.rect.x = shipData.x;
    shipData.rect.y = shipData.y;

    SDL_RenderCopy(renderer, ship, NULL, &shipData.rect);

    /* update screen */
    SDL_RenderPresent(renderer);

}

void
initializeTextures(SDL_Renderer *renderer)
{

    SDL_Surface *bmp_surface;

    /* load the ship */
    bmp_surface = SDL_LoadBMP("ship.bmp");
    if (bmp_surface == NULL) {
        fatalError("could not ship.bmp");
    }
    /* set blue to transparent on the ship */
    SDL_SetColorKey(bmp_surface, 1,
                    SDL_MapRGB(bmp_surface->format, 0, 0, 255));

    /* create ship texture from surface */
    ship = SDL_CreateTextureFromSurface(renderer, bmp_surface);
    if (ship == 0) {
        fatalError("could not create ship texture");
    }
    SDL_SetTextureBlendMode(ship, SDL_BLENDMODE_BLEND);

    /* set the width and height of the ship from the surface dimensions */
    shipData.rect.w = bmp_surface->w;
    shipData.rect.h = bmp_surface->h;

    SDL_FreeSurface(bmp_surface);

    /* load the space background */
    bmp_surface = SDL_LoadBMP("space.bmp");
    if (bmp_surface == NULL) {
        fatalError("could not load space.bmp");
    }
    /* create space texture from surface */
    space = SDL_CreateTextureFromSurface(renderer, bmp_surface);
    if (space == 0) {
        fatalError("could not create space texture");
    }
    SDL_FreeSurface(bmp_surface);

}



int
main(int argc, char *argv[])
{

    SDL_Window *window;         /* main window */
	SDL_Renderer *renderer;
    Uint32 startFrame;          /* time frame began to process */
    Uint32 endFrame;            /* time frame ended processing */
    Uint32 delay;               /* time to pause waiting to draw next frame */
    int done;                   /* should we clean up and exit? */

    /* initialize SDL */
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0) {
        fatalError("Could not initialize SDL");
    }

    /* create main window and renderer */
    window = SDL_CreateWindow(NULL, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
                                SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN |
                                SDL_WINDOW_BORDERLESS);
    renderer = SDL_CreateRenderer(window, 0, 0);

    /* print out some info about joysticks and try to open accelerometer for use */
    printf("There are %d joysticks available\n", SDL_NumJoysticks());
    printf("Default joystick (index 0) is %s\n", SDL_JoystickName(0));
    accelerometer = SDL_JoystickOpen(0);
    if (accelerometer == NULL) {
        fatalError("Could not open joystick (accelerometer)");
    }
    printf("joystick number of axis = %d\n",
           SDL_JoystickNumAxes(accelerometer));
    printf("joystick number of hats = %d\n",
           SDL_JoystickNumHats(accelerometer));
    printf("joystick number of balls = %d\n",
           SDL_JoystickNumBalls(accelerometer));
    printf("joystick number of buttons = %d\n",
           SDL_JoystickNumButtons(accelerometer));

    /* load graphics */
    initializeTextures(renderer);

    /* setup ship */
    shipData.x = (SCREEN_WIDTH - shipData.rect.w) / 2;
    shipData.y = (SCREEN_HEIGHT - shipData.rect.h) / 2;
    shipData.vx = 0.0f;
    shipData.vy = 0.0f;

    done = 0;
    /* enter main loop */
    while (!done) {
        startFrame = SDL_GetTicks();
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                done = 1;
            }
        }
        render(renderer);
        endFrame = SDL_GetTicks();

        /* figure out how much time we have left, and then sleep */
        delay = MILLESECONDS_PER_FRAME - (endFrame - startFrame);
        if (delay < 0) {
            delay = 0;
        } else if (delay > MILLESECONDS_PER_FRAME) {
            delay = MILLESECONDS_PER_FRAME;
        }
        SDL_Delay(delay);
    }

    /* delete textures */
    SDL_DestroyTexture(ship);
    SDL_DestroyTexture(space);

    /* shutdown SDL */
    SDL_Quit();

    return 0;

}
