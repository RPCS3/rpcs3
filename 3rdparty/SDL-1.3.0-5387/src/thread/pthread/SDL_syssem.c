/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2011 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org
*/
#include "SDL_config.h"

#include <errno.h>
#include <pthread.h>
#include <semaphore.h>

#include "SDL_thread.h"
#include "SDL_timer.h"

/* Wrapper around POSIX 1003.1b semaphores */

#if defined(__MACOSX__) || defined(__IPHONEOS__)
/* Mac OS X doesn't support sem_getvalue() as of version 10.4 */
#include "../generic/SDL_syssem.c"
#else

struct SDL_semaphore
{
    sem_t sem;
};

/* Create a semaphore, initialized with value */
SDL_sem *
SDL_CreateSemaphore(Uint32 initial_value)
{
    SDL_sem *sem = (SDL_sem *) SDL_malloc(sizeof(SDL_sem));
    if (sem) {
        if (sem_init(&sem->sem, 0, initial_value) < 0) {
            SDL_SetError("sem_init() failed");
            SDL_free(sem);
            sem = NULL;
        }
    } else {
        SDL_OutOfMemory();
    }
    return sem;
}

void
SDL_DestroySemaphore(SDL_sem * sem)
{
    if (sem) {
        sem_destroy(&sem->sem);
        SDL_free(sem);
    }
}

int
SDL_SemTryWait(SDL_sem * sem)
{
    int retval;

    if (!sem) {
        SDL_SetError("Passed a NULL semaphore");
        return -1;
    }
    retval = SDL_MUTEX_TIMEDOUT;
    if (sem_trywait(&sem->sem) == 0) {
        retval = 0;
    }
    return retval;
}

int
SDL_SemWait(SDL_sem * sem)
{
    int retval;

    if (!sem) {
        SDL_SetError("Passed a NULL semaphore");
        return -1;
    }

    retval = sem_wait(&sem->sem);
    if (retval < 0) {
        SDL_SetError("sem_wait() failed");
    }
    return retval;
}

int
SDL_SemWaitTimeout(SDL_sem * sem, Uint32 timeout)
{
    int retval;
    struct timeval now;
    struct timespec ts_timeout;

    if (!sem) {
        SDL_SetError("Passed a NULL semaphore");
        return -1;
    }

    /* Try the easy cases first */
    if (timeout == 0) {
        return SDL_SemTryWait(sem);
    }
    if (timeout == SDL_MUTEX_MAXWAIT) {
        return SDL_SemWait(sem);
    }

    /* Setup the timeout. sem_timedwait doesn't wait for
    * a lapse of time, but until we reach a certain time.
    * This time is now plus the timeout.
    */
    gettimeofday(&now, NULL);

    /* Add our timeout to current time */
    now.tv_usec += (timeout % 1000) * 1000;
    now.tv_sec += timeout / 1000;

    /* Wrap the second if needed */
    if ( now.tv_usec >= 1000000 ) {
        now.tv_usec -= 1000000;
        now.tv_sec ++;
    }

    /* Convert to timespec */
    ts_timeout.tv_sec = now.tv_sec;
    ts_timeout.tv_nsec = now.tv_usec * 1000;

    /* Wait. */
    do {
        retval = sem_timedwait(&sem->sem, &ts_timeout);
    } while (retval < 0 && errno == EINTR);

    if (retval < 0) {
        SDL_SetError("sem_timedwait() failed");
    }

    return retval;
}

Uint32
SDL_SemValue(SDL_sem * sem)
{
    int ret = 0;
    if (sem) {
        sem_getvalue(&sem->sem, &ret);
        if (ret < 0) {
            ret = 0;
        }
    }
    return (Uint32) ret;
}

int
SDL_SemPost(SDL_sem * sem)
{
    int retval;

    if (!sem) {
        SDL_SetError("Passed a NULL semaphore");
        return -1;
    }

    retval = sem_post(&sem->sem);
    if (retval < 0) {
        SDL_SetError("sem_post() failed");
    }
    return retval;
}

#endif /* __MACOSX__ */
/* vi: set ts=4 sw=4 expandtab: */
