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

/* These are functions that need to be implemented by a port of SDL */

#ifndef _SDL_systhread_h
#define _SDL_systhread_h

#include "SDL_thread.h"

/* This function creates a thread, passing args to SDL_RunThread(),
   saves a system-dependent thread id in thread->id, and returns 0
   on success.
*/
#ifdef SDL_PASSED_BEGINTHREAD_ENDTHREAD
extern int SDL_SYS_CreateThread(SDL_Thread * thread, void *args,
                                pfnSDL_CurrentBeginThread pfnBeginThread,
                                pfnSDL_CurrentEndThread pfnEndThread);
#else
extern int SDL_SYS_CreateThread(SDL_Thread * thread, void *args);
#endif

/* This function does any necessary setup in the child thread */
extern void SDL_SYS_SetupThread(void);

/* This function waits for the thread to finish and frees any data
   allocated by SDL_SYS_CreateThread()
 */
extern void SDL_SYS_WaitThread(SDL_Thread * thread);

#endif /* _SDL_systhread_h */

/* vi: set ts=4 sw=4 expandtab: */
