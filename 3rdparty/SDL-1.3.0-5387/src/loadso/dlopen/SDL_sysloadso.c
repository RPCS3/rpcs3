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

#ifdef SDL_LOADSO_DLOPEN

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* System dependent library loading routines                           */

#include <stdio.h>
#include <dlfcn.h>

#include "SDL_loadso.h"

void *
SDL_LoadObject(const char *sofile)
{
    void *handle = dlopen(sofile, RTLD_NOW);
    const char *loaderror = (char *) dlerror();
    if (handle == NULL) {
        SDL_SetError("Failed loading %s: %s", sofile, loaderror);
    }
    return (handle);
}

void *
SDL_LoadFunction(void *handle, const char *name)
{
    void *symbol = dlsym(handle, name);
    if (symbol == NULL) {
        /* append an underscore for platforms that need that. */
        size_t len = 1 + SDL_strlen(name) + 1;
        char *_name = SDL_stack_alloc(char, len);
        _name[0] = '_';
        SDL_strlcpy(&_name[1], name, len);
        symbol = dlsym(handle, _name);
        SDL_stack_free(_name);
        if (symbol == NULL) {
            SDL_SetError("Failed loading %s: %s", name,
                         (const char *) dlerror());
        }
    }
    return (symbol);
}

void
SDL_UnloadObject(void *handle)
{
    if (handle != NULL) {
        dlclose(handle);
    }
}

#endif /* SDL_LOADSO_DLOPEN */

/* vi: set ts=4 sw=4 expandtab: */
