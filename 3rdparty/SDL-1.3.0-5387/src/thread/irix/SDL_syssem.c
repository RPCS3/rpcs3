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

#include "SDL_thread.h"
#include "SDL_timer.h"


#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <errno.h>

#include "SDL_error.h"
#include "SDL_thread.h"


struct SDL_semaphore
{
    int id;
};

/* Not defined by many operating systems, use configure to detect */
/*
#if !defined(HAVE_SEMUN)
union semun {
	int val;
	struct semid_ds *buf;
	ushort *array;
};
#endif
*/

static struct sembuf op_trywait[2] = {
    {0, -1, (IPC_NOWAIT | SEM_UNDO)}    /* Decrement semaphore, no block */
};

static struct sembuf op_wait[2] = {
    {0, -1, SEM_UNDO}           /* Decrement semaphore */
};

static struct sembuf op_post[1] = {
    {0, 1, (IPC_NOWAIT | SEM_UNDO)}     /* Increment semaphore */
};

/* Create a blockable semaphore */
SDL_sem *
SDL_CreateSemaphore(Uint32 initial_value)
{
    extern int _creating_thread_lock;   /* SDL_threads.c */
    SDL_sem *sem;
    union semun init;

    sem = (SDL_sem *) SDL_malloc(sizeof(*sem));
    if (sem == NULL) {
        SDL_OutOfMemory();
        return (NULL);
    }
    sem->id = semget(IPC_PRIVATE, 1, (0600 | IPC_CREAT));
    if (sem->id < 0) {
        SDL_SetError("Couldn't create semaphore");
        SDL_free(sem);
        return (NULL);
    }
    init.val = initial_value;   /* Initialize semaphore */
    semctl(sem->id, 0, SETVAL, init);
    return (sem);
}

void
SDL_DestroySemaphore(SDL_sem * sem)
{
    if (sem) {
#ifdef __IRIX__
        semctl(sem->id, 0, IPC_RMID);
#else
        union semun dummy;
        dummy.val = 0;
        semctl(sem->id, 0, IPC_RMID, dummy);
#endif
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

    retval = 0;
  tryagain:
    if (semop(sem->id, op_trywait, 1) < 0) {
        if (errno == EINTR) {
            goto tryagain;
        }
        retval = SDL_MUTEX_TIMEDOUT;
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

    retval = 0;
  tryagain:
    if (semop(sem->id, op_wait, 1) < 0) {
        if (errno == EINTR) {
            goto tryagain;
        }
        SDL_SetError("Semaphore operation error");
        retval = -1;
    }
    return retval;
}

int
SDL_SemWaitTimeout(SDL_sem * sem, Uint32 timeout)
{
    int retval;

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

    /* Ack!  We have to busy wait... */
    timeout += SDL_GetTicks();
    do {
        retval = SDL_SemTryWait(sem);
        if (retval == 0) {
            break;
        }
        SDL_Delay(1);
    } while (SDL_GetTicks() < timeout);

    return retval;
}

Uint32
SDL_SemValue(SDL_sem * sem)
{
    int semval;
    Uint32 value;

    value = 0;
    if (sem) {
      tryagain:
#ifdef __IRIX__
        semval = semctl(sem->id, 0, GETVAL);
#else
        {
            union semun arg;
            arg.val = 0;
            semval = semctl(sem->id, 0, GETVAL, arg);
        }
#endif
        if (semval < 0) {
            if (errno == EINTR) {
                goto tryagain;
            }
        } else {
            value = (Uint32) semval;
        }
    }
    return value;
}

int
SDL_SemPost(SDL_sem * sem)
{
    int retval;

    if (!sem) {
        SDL_SetError("Passed a NULL semaphore");
        return -1;
    }

    retval = 0;
  tryagain:
    if (semop(sem->id, op_post, 1) < 0) {
        if (errno == EINTR) {
            goto tryagain;
        }
        SDL_SetError("Semaphore operation error");
        retval = -1;
    }
    return retval;
}

/* vi: set ts=4 sw=4 expandtab: */
