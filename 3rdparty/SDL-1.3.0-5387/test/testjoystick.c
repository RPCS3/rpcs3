
/* Simple program to test the SDL joystick routines */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SDL.h"

#ifdef __IPHONEOS__
#define SCREEN_WIDTH	320
#define SCREEN_HEIGHT	480
#else
#define SCREEN_WIDTH	640
#define SCREEN_HEIGHT	480
#endif

void
WatchJoystick(SDL_Joystick * joystick)
{
    SDL_Surface *screen;
    const char *name;
    int i, done;
    SDL_Event event;
    int x, y, draw;
    SDL_Rect axis_area[6][2];

    /* Set a video mode to display joystick axis position */
    screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, 16, 0);
    if (screen == NULL) {
        fprintf(stderr, "Couldn't set video mode: %s\n", SDL_GetError());
        return;
    }

    /* Print info about the joystick we are watching */
    name = SDL_JoystickName(SDL_JoystickIndex(joystick));
    printf("Watching joystick %d: (%s)\n", SDL_JoystickIndex(joystick),
           name ? name : "Unknown Joystick");
    printf("Joystick has %d axes, %d hats, %d balls, and %d buttons\n",
           SDL_JoystickNumAxes(joystick), SDL_JoystickNumHats(joystick),
           SDL_JoystickNumBalls(joystick), SDL_JoystickNumButtons(joystick));

    /* Initialize drawing rectangles */
    memset(axis_area, 0, (sizeof axis_area));
    draw = 0;

    /* Loop, getting joystick events! */
    done = 0;
    while (!done) {
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_JOYAXISMOTION:
                printf("Joystick %d axis %d value: %d\n",
                       event.jaxis.which,
                       event.jaxis.axis, event.jaxis.value);
                break;
            case SDL_JOYHATMOTION:
                printf("Joystick %d hat %d value:",
                       event.jhat.which, event.jhat.hat);
                if (event.jhat.value == SDL_HAT_CENTERED)
                    printf(" centered");
                if (event.jhat.value & SDL_HAT_UP)
                    printf(" up");
                if (event.jhat.value & SDL_HAT_RIGHT)
                    printf(" right");
                if (event.jhat.value & SDL_HAT_DOWN)
                    printf(" down");
                if (event.jhat.value & SDL_HAT_LEFT)
                    printf(" left");
                printf("\n");
                break;
            case SDL_JOYBALLMOTION:
                printf("Joystick %d ball %d delta: (%d,%d)\n",
                       event.jball.which,
                       event.jball.ball, event.jball.xrel, event.jball.yrel);
                break;
            case SDL_JOYBUTTONDOWN:
                printf("Joystick %d button %d down\n",
                       event.jbutton.which, event.jbutton.button);
                break;
            case SDL_JOYBUTTONUP:
                printf("Joystick %d button %d up\n",
                       event.jbutton.which, event.jbutton.button);
                break;
            case SDL_KEYDOWN:
                if (event.key.keysym.sym != SDLK_ESCAPE) {
                    break;
                }
                /* Fall through to signal quit */
            case SDL_QUIT:
                done = 1;
                break;
            default:
                break;
            }
        }
        /* Update visual joystick state */
        for (i = 0; i < SDL_JoystickNumButtons(joystick); ++i) {
            SDL_Rect area;

            area.x = i * 34;
            area.y = SCREEN_HEIGHT - 34;
            area.w = 32;
            area.h = 32;
            if (SDL_JoystickGetButton(joystick, i) == SDL_PRESSED) {
                SDL_FillRect(screen, &area, 0xFFFF);
            } else {
                SDL_FillRect(screen, &area, 0x0000);
            }
            SDL_UpdateRects(screen, 1, &area);
        }

        for (i = 0;
             i < SDL_JoystickNumAxes(joystick) / 2
             && i < SDL_arraysize(axis_area); ++i) {
            /* Erase previous axes */
            SDL_FillRect(screen, &axis_area[i][draw], 0x0000);

            /* Draw the X/Y axis */
            draw = !draw;
            x = (((int) SDL_JoystickGetAxis(joystick, i * 2 + 0)) + 32768);
            x *= SCREEN_WIDTH;
            x /= 65535;
            if (x < 0) {
                x = 0;
            } else if (x > (SCREEN_WIDTH - 16)) {
                x = SCREEN_WIDTH - 16;
            }
            y = (((int) SDL_JoystickGetAxis(joystick, i * 2 + 1)) + 32768);
            y *= SCREEN_HEIGHT;
            y /= 65535;
            if (y < 0) {
                y = 0;
            } else if (y > (SCREEN_HEIGHT - 16)) {
                y = SCREEN_HEIGHT - 16;
            }

            axis_area[i][draw].x = (Sint16) x;
            axis_area[i][draw].y = (Sint16) y;
            axis_area[i][draw].w = 16;
            axis_area[i][draw].h = 16;
            SDL_FillRect(screen, &axis_area[i][draw], 0xFFFF);

            SDL_UpdateRects(screen, 2, axis_area[i]);
        }
    }
}

int
main(int argc, char *argv[])
{
    const char *name;
    int i;
    SDL_Joystick *joystick;

    /* Initialize SDL (Note: video is required to start event loop) */
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0) {
        fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
        exit(1);
    }

    /* Print information about the joysticks */
    printf("There are %d joysticks attached\n", SDL_NumJoysticks());
    for (i = 0; i < SDL_NumJoysticks(); ++i) {
        name = SDL_JoystickName(i);
        printf("Joystick %d: %s\n", i, name ? name : "Unknown Joystick");
        joystick = SDL_JoystickOpen(i);
        if (joystick == NULL) {
            fprintf(stderr, "SDL_JoystickOpen(%d) failed: %s\n", i,
                    SDL_GetError());
        } else {
            printf("       axes: %d\n", SDL_JoystickNumAxes(joystick));
            printf("      balls: %d\n", SDL_JoystickNumBalls(joystick));
            printf("       hats: %d\n", SDL_JoystickNumHats(joystick));
            printf("    buttons: %d\n", SDL_JoystickNumButtons(joystick));
            SDL_JoystickClose(joystick);
        }
    }

    if (argv[1]) {
        joystick = SDL_JoystickOpen(atoi(argv[1]));
        if (joystick == NULL) {
            printf("Couldn't open joystick %d: %s\n", atoi(argv[1]),
                   SDL_GetError());
        } else {
            WatchJoystick(joystick);
            SDL_JoystickClose(joystick);
        }
    }
    SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK);

    return (0);
}
