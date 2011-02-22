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

/* Semaphores in the BeOS environment */

#include <be/kernel/OS.h>

#include "SDL_thread.h"


struct SDL_semaphore
{
    sem_id id;
};

/* Create a counting semaphore */
SDL_sem *
SDL_CreateSemaphore(Uint32 initial_value)
{
    SDL_sem *sem;

    sem = (SDL_sem *) SDL_malloc(sizeof(*sem));
    if (sem) {
        sem->id = create_sem(initial_value, "SDL semaphore");
        if (sem->id < B_NO_ERROR) {
            SDL_SetError("create_sem() failed");
            SDL_free(sem);
            sem = NULL;
        }
    } else {
        SDL_OutOfMemory();
    }
    return (sem);
}

/* Free the semaphore */
void
SDL_DestroySemaphore(SDL_sem * sem)
{
    if (sem) {
        if (sem->id >= B_NO_ERROR) {
            delete_sem(sem->id);
        }
        SDL_free(sem);
    }
}

int
SDL_SemWaitTimeout(SDL_sem * sem, Uint32 timeout)
{
    int32 val;
    int retval;

    if (!sem) {
        SDL_SetError("Passed a NULL semaphore");
        return -1;
    }

  tryagain:
    if (timeout == SDL_MUTEX_MAXWAIT) {
        val = acquire_sem(sem->id);
    } else {
        timeout *= 1000;        /* BeOS uses a timeout in microseconds */
        val = acquire_sem_etc(sem->id, 1, B_RELATIVE_TIMEOUT, timeout);
    }
    switch (val) {
    case B_INTERRUPTED:
        goto tryagain;
    case B_NO_ERROR:
        retval = 0;
        break;
    case B_TIMED_OUT:
        retval = SDL_MUTEX_TIMEDOUT;
        break;
    case B_WOULD_BLOCK:
        retval = SDL_MUTEX_TIMEDOUT;
        break;
    default:
        SDL_SetError("acquire_sem() failed");
        retval = -1;
        break;
    }

    return retval;
}

int
SDL_SemTryWait(SDL_sem * sem)
{
    return SDL_SemWaitTimeout(sem, 0);
}

int
SDL_SemWait(SDL_sem * sem)
{
    return SDL_SemWaitTimeout(sem, SDL_MUTEX_MAXWAIT);
}

/* Returns the current count of the semaphore */
Uint32
SDL_SemValue(SDL_sem * sem)
{
    int32 count;
    Uint32 value;

    value = 0;
    if (sem) {
        get_sem_count(sem->id, &count);
        if (count > 0) {
            value = (Uint32) count;
        }
    }
    return value;
}

/* Atomically increases the semaphore's count (not blocking) */
int
SDL_SemPost(SDL_sem * sem)
{
    if (!sem) {
        SDL_SetError("Passed a NULL semaphore");
        return -1;
    }

    if (release_sem(sem->id) != B_NO_ERROR) {
        SDL_SetError("release_sem() failed");
        return -1;
    }
    return 0;
}

/* vi: set ts=4 sw=4 expandtab: */
