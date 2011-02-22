
/* Simple test of the SDL semaphore code */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "SDL.h"
#include "SDL_thread.h"

#define NUM_THREADS 10

static SDL_sem *sem;
int alive = 1;

int SDLCALL
ThreadFunc(void *data)
{
    int threadnum = (int) (uintptr_t) data;
    while (alive) {
        SDL_SemWait(sem);
        fprintf(stderr,
                "Thread number %d has got the semaphore (value = %d)!\n",
                threadnum, SDL_SemValue(sem));
        SDL_Delay(200);
        SDL_SemPost(sem);
        fprintf(stderr,
                "Thread number %d has released the semaphore (value = %d)!\n",
                threadnum, SDL_SemValue(sem));
        SDL_Delay(1);           /* For the scheduler */
    }
    printf("Thread number %d exiting.\n", threadnum);
    return 0;
}

static void
killed(int sig)
{
    alive = 0;
}

static void
TestWaitTimeout(void)
{
    Uint32 start_ticks;
    Uint32 end_ticks;
    Uint32 duration;

    sem = SDL_CreateSemaphore(0);
    printf("Waiting 2 seconds on semaphore\n");

    start_ticks = SDL_GetTicks();
    SDL_SemWaitTimeout(sem, 2000);
    end_ticks = SDL_GetTicks();

    duration = end_ticks - start_ticks;

    /* Accept a little offset in the effective wait */
    if (duration > 1900 && duration < 2050)
        printf("Wait done.\n");
    else
        fprintf(stderr, "Wait took %d milliseconds\n", duration);
}

int
main(int argc, char **argv)
{
    SDL_Thread *threads[NUM_THREADS];
    uintptr_t i;
    int init_sem;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s init_value\n", argv[0]);
        return (1);
    }

    /* Load the SDL library */
    if (SDL_Init(0) < 0) {
        fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
        return (1);
    }
    signal(SIGTERM, killed);
    signal(SIGINT, killed);

    init_sem = atoi(argv[1]);
    sem = SDL_CreateSemaphore(init_sem);

    printf("Running %d threads, semaphore value = %d\n", NUM_THREADS,
           init_sem);
    /* Create all the threads */
    for (i = 0; i < NUM_THREADS; ++i) {
        threads[i] = SDL_CreateThread(ThreadFunc, (void *) i);
    }

    /* Wait 10 seconds */
    SDL_Delay(10 * 1000);

    /* Wait for all threads to finish */
    printf("Waiting for threads to finish\n");
    alive = 0;
    for (i = 0; i < NUM_THREADS; ++i) {
        SDL_WaitThread(threads[i], NULL);
    }
    printf("Finished waiting for threads\n");

    SDL_DestroySemaphore(sem);

    TestWaitTimeout();

    SDL_Quit();
    return (0);
}
