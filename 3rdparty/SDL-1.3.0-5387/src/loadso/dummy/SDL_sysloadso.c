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

#if defined(SDL_LOADSO_DUMMY) || defined(SDL_LOADSO_DISABLED)

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* System dependent library loading routines                           */

#include "SDL_loadso.h"

void *
SDL_LoadObject(const char *sofile)
{
    const char *loaderror = "SDL_LoadObject() not implemented";
    SDL_SetError("Failed loading %s: %s", sofile, loaderror);
    return (NULL);
}

void *
SDL_LoadFunction(void *handle, const char *name)
{
    const char *loaderror = "SDL_LoadFunction() not implemented";
    SDL_SetError("Failed loading %s: %s", name, loaderror);
    return (NULL);
}

void
SDL_UnloadObject(void *handle)
{
    /* no-op. */
}

#endif /* SDL_LOADSO_DUMMY || SDL_LOADSO_DISABLED */

/* vi: set ts=4 sw=4 expandtab: */
