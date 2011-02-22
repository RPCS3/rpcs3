
/* Simple test of the SDL threading code */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "SDL.h"
#include "SDL_thread.h"

static int alive = 0;

/* Call this instead of exit(), so we can clean up SDL: atexit() is evil. */
static void
quit(int rc)
{
    SDL_Quit();
    exit(rc);
}

int SDLCALL
ThreadFunc(void *data)
{
    printf("Started thread %s: My thread id is %lu\n",
           (char *) data, SDL_ThreadID());
    while (alive) {
        printf("Thread '%s' is alive!\n", (char *) data);
        SDL_Delay(1 * 1000);
    }
    printf("Thread '%s' exiting!\n", (char *) data);
    return (0);
}

static void
killed(int sig)
{
    printf("Killed with SIGTERM, waiting 5 seconds to exit\n");
    SDL_Delay(5 * 1000);
    alive = 0;
    quit(0);
}

int
main(int argc, char *argv[])
{
    SDL_Thread *thread;

    /* Load the SDL library */
    if (SDL_Init(0) < 0) {
        fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
        return (1);
    }

    alive = 1;
    thread = SDL_CreateThread(ThreadFunc, "#1");
    if (thread == NULL) {
        fprintf(stderr, "Couldn't create thread: %s\n", SDL_GetError());
        quit(1);
    }
    SDL_Delay(5 * 1000);
    printf("Waiting for thread #1\n");
    alive = 0;
    SDL_WaitThread(thread, NULL);

    alive = 1;
    signal(SIGTERM, killed);
    thread = SDL_CreateThread(ThreadFunc, "#2");
    if (thread == NULL) {
        fprintf(stderr, "Couldn't create thread: %s\n", SDL_GetError());
        quit(1);
    }
    raise(SIGTERM);

    SDL_Quit();                 /* Never reached */
    return (0);                 /* Never reached */
}
