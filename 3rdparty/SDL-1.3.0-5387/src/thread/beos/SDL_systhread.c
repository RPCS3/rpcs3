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

/* BeOS thread management routines for SDL */

#include <stdio.h>
#include <signal.h>
#include <be/kernel/OS.h>

#include "SDL_mutex.h"
#include "SDL_thread.h"
#include "../SDL_thread_c.h"
#include "../SDL_systhread.h"


static int sig_list[] = {
    SIGHUP, SIGINT, SIGQUIT, SIGPIPE, SIGALRM, SIGTERM, SIGWINCH, 0
};

void
SDL_MaskSignals(sigset_t * omask)
{
    sigset_t mask;
    int i;

    sigemptyset(&mask);
    for (i = 0; sig_list[i]; ++i) {
        sigaddset(&mask, sig_list[i]);
    }
    sigprocmask(SIG_BLOCK, &mask, omask);
}

void
SDL_UnmaskSignals(sigset_t * omask)
{
    sigprocmask(SIG_SETMASK, omask, NULL);
}

static int32
RunThread(void *data)
{
    SDL_RunThread(data);
    return (0);
}

int
SDL_SYS_CreateThread(SDL_Thread * thread, void *args)
{
    /* Create the thread and go! */
    thread->handle = spawn_thread(RunThread, "SDL", B_NORMAL_PRIORITY, args);
    if ((thread->handle == B_NO_MORE_THREADS) ||
        (thread->handle == B_NO_MEMORY)) {
        SDL_SetError("Not enough resources to create thread");
        return (-1);
    }
    resume_thread(thread->handle);
    return (0);
}

void
SDL_SYS_SetupThread(void)
{
    /* Mask asynchronous signals for this thread */
    SDL_MaskSignals(NULL);
}

SDL_threadID
SDL_ThreadID(void)
{
    return ((SDL_threadID) find_thread(NULL));
}

void
SDL_SYS_WaitThread(SDL_Thread * thread)
{
    status_t the_status;

    wait_for_thread(thread->handle, &the_status);
}

/* vi: set ts=4 sw=4 expandtab: */
