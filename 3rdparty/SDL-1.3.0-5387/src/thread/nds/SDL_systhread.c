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

#ifdef SAVE_RCSID
static char rcsid =
    "@(#) $Id: SDL_systhread.c,v 1.2 2001/04/26 16:50:18 hercules Exp $";
#endif

/* Thread management routines for SDL */

#include "SDL_error.h"
#include "SDL_thread.h"
#include "../SDL_systhread.h"

int
SDL_SYS_CreateThread(SDL_Thread * thread, void *args)
{
    SDL_SetError("Threads are not supported on this platform");
    return (-1);
}

void
SDL_SYS_SetupThread(void)
{
    return;
}

SDL_threadID
SDL_ThreadID(void)
{
    return (0);
}

void
SDL_SYS_WaitThread(SDL_Thread * thread)
{
    return;
}

void
SDL_SYS_KillThread(SDL_Thread * thread)
{
    return;
}
