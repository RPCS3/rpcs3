/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2011 Sam Lantinga

    This library is SDL_free software; you can redistribute it and/or
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

#include "../video/SDL_sysvideo.h"

/* Useful functions and variables from SDL_sysevents.c */

#ifdef __BEOS__                 /* The Be event loop runs in a separate thread */
#define MUST_THREAD_EVENTS
#endif

#ifdef __WIN32__              /* Windows doesn't allow a separate event thread */
#define CANT_THREAD_EVENTS
#endif

/* vi: set ts=4 sw=4 expandtab: */
