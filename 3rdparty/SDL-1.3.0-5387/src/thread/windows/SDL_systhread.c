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

/* Win32 thread management routines for SDL */

#include "SDL_thread.h"
#include "../SDL_thread_c.h"
#include "../SDL_systhread.h"
#include "SDL_systhread_c.h"

#ifndef SDL_PASSED_BEGINTHREAD_ENDTHREAD
#ifndef _WIN32_WCE
/* We'll use the C library from this DLL */
#include <process.h>
#endif

#if __GNUC__
typedef uintptr_t (__cdecl * pfnSDL_CurrentBeginThread) (void *, unsigned,
                                                             unsigned
                                                             (__stdcall *
                                                              func) (void *),
                                                             void *arg,
                                                             unsigned,
                                                             unsigned
                                                             *threadID);
typedef void (__cdecl * pfnSDL_CurrentEndThread) (unsigned code);
#elif defined(__WATCOMC__)
/* This is for Watcom targets except OS2 */
#if __WATCOMC__ < 1240
#define __watcall
#endif
typedef unsigned long (__watcall * pfnSDL_CurrentBeginThread) (void *,
                                                               unsigned,
                                                               unsigned
                                                               (__stdcall *
                                                                func) (void
                                                                       *),
                                                               void *arg,
                                                               unsigned,
                                                               unsigned
                                                               *threadID);
typedef void (__watcall * pfnSDL_CurrentEndThread) (unsigned code);
#else
typedef uintptr_t(__cdecl * pfnSDL_CurrentBeginThread) (void *, unsigned,
                                                        unsigned (__stdcall *
                                                                  func) (void
                                                                         *),
                                                        void *arg, unsigned,
                                                        unsigned *threadID);
typedef void (__cdecl * pfnSDL_CurrentEndThread) (unsigned code);
#endif
#endif /* !SDL_PASSED_BEGINTHREAD_ENDTHREAD */


typedef struct ThreadStartParms
{
    void *args;
    pfnSDL_CurrentEndThread pfnCurrentEndThread;
} tThreadStartParms, *pThreadStartParms;

static DWORD __stdcall
RunThread(void *data)
{
    pThreadStartParms pThreadParms = (pThreadStartParms) data;
    pfnSDL_CurrentEndThread pfnCurrentEndThread = NULL;

    // Call the thread function!
    SDL_RunThread(pThreadParms->args);

    // Get the current endthread we have to use!
    if (pThreadParms) {
        pfnCurrentEndThread = pThreadParms->pfnCurrentEndThread;
        SDL_free(pThreadParms);
    }
    // Call endthread!
    if (pfnCurrentEndThread)
        (*pfnCurrentEndThread) (0);
    return (0);
}

#ifdef SDL_PASSED_BEGINTHREAD_ENDTHREAD
int
SDL_SYS_CreateThread(SDL_Thread * thread, void *args,
                     pfnSDL_CurrentBeginThread pfnBeginThread,
                     pfnSDL_CurrentEndThread pfnEndThread)
{
#else
int
SDL_SYS_CreateThread(SDL_Thread * thread, void *args)
{
#ifdef _WIN32_WCE
    pfnSDL_CurrentBeginThread pfnBeginThread = NULL;
    pfnSDL_CurrentEndThread pfnEndThread = NULL;
#else
    pfnSDL_CurrentBeginThread pfnBeginThread = _beginthreadex;
    pfnSDL_CurrentEndThread pfnEndThread = _endthreadex;
#endif
#endif /* SDL_PASSED_BEGINTHREAD_ENDTHREAD */
    unsigned threadid;
    pThreadStartParms pThreadParms =
        (pThreadStartParms) SDL_malloc(sizeof(tThreadStartParms));
    if (!pThreadParms) {
        SDL_OutOfMemory();
        return (-1);
    }
    // Save the function which we will have to call to clear the RTL of calling app!
    pThreadParms->pfnCurrentEndThread = pfnEndThread;
    // Also save the real parameters we have to pass to thread function
    pThreadParms->args = args;

    if (pfnBeginThread) {
        thread->handle =
            (SYS_ThreadHandle) pfnBeginThread(NULL, 0, RunThread,
                                              pThreadParms, 0, &threadid);
    } else {
        thread->handle =
            CreateThread(NULL, 0, RunThread, pThreadParms, 0, &threadid);
    }
    if (thread->handle == NULL) {
        SDL_SetError("Not enough resources to create thread");
        return (-1);
    }
    return (0);
}

void
SDL_SYS_SetupThread(void)
{
    return;
}

SDL_threadID
SDL_ThreadID(void)
{
    return ((SDL_threadID) GetCurrentThreadId());
}

void
SDL_SYS_WaitThread(SDL_Thread * thread)
{
    WaitForSingleObject(thread->handle, INFINITE);
    CloseHandle(thread->handle);
}

/* vi: set ts=4 sw=4 expandtab: */
