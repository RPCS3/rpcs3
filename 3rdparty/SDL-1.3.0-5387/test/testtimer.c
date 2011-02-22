
/* Test program to check the resolution of the SDL timer on the current
   platform
*/

#include <stdlib.h>
#include <stdio.h>

#include "SDL.h"

#define DEFAULT_RESOLUTION	1

static int ticks = 0;

static Uint32 SDLCALL
ticktock(Uint32 interval)
{
    ++ticks;
    return (interval);
}

static Uint32 SDLCALL
callback(Uint32 interval, void *param)
{
    printf("Timer %d : param = %d\n", interval, (int) (uintptr_t) param);
    return interval;
}

int
main(int argc, char *argv[])
{
    int desired;
    SDL_TimerID t1, t2, t3;

    if (SDL_Init(SDL_INIT_TIMER) < 0) {
        fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
        return (1);
    }

    /* Start the timer */
    desired = 0;
    if (argv[1]) {
        desired = atoi(argv[1]);
    }
    if (desired == 0) {
        desired = DEFAULT_RESOLUTION;
    }
    SDL_SetTimer(desired, ticktock);

    /* Wait 10 seconds */
    printf("Waiting 10 seconds\n");
    SDL_Delay(10 * 1000);

    /* Stop the timer */
    SDL_SetTimer(0, NULL);

    /* Print the results */
    if (ticks) {
        fprintf(stderr,
                "Timer resolution: desired = %d ms, actual = %f ms\n",
                desired, (double) (10 * 1000) / ticks);
    }

    /* Test multiple timers */
    printf("Testing multiple timers...\n");
    t1 = SDL_AddTimer(100, callback, (void *) 1);
    if (!t1)
        fprintf(stderr, "Could not create timer 1: %s\n", SDL_GetError());
    t2 = SDL_AddTimer(50, callback, (void *) 2);
    if (!t2)
        fprintf(stderr, "Could not create timer 2: %s\n", SDL_GetError());
    t3 = SDL_AddTimer(233, callback, (void *) 3);
    if (!t3)
        fprintf(stderr, "Could not create timer 3: %s\n", SDL_GetError());

    /* Wait 10 seconds */
    printf("Waiting 10 seconds\n");
    SDL_Delay(10 * 1000);

    printf("Removing timer 1 and waiting 5 more seconds\n");
    SDL_RemoveTimer(t1);

    SDL_Delay(5 * 1000);

    SDL_RemoveTimer(t2);
    SDL_RemoveTimer(t3);

    SDL_Quit();
    return (0);
}
