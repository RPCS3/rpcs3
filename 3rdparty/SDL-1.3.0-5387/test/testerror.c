
/* Simple test of the SDL threading code and error handling */

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
    /* Set the child thread error string */
    SDL_SetError("Thread %s (%lu) had a problem: %s",
                 (char *) data, SDL_ThreadID(), "nevermind");
    while (alive) {
        printf("Thread '%s' is alive!\n", (char *) data);
        SDL_Delay(1 * 1000);
    }
    printf("Child thread error string: %s\n", SDL_GetError());
    return (0);
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

    /* Set the error value for the main thread */
    SDL_SetError("No worries");

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

    printf("Main thread error string: %s\n", SDL_GetError());

    SDL_Quit();
    return (0);
}
