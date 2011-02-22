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

#ifdef SDL_LOADSO_BEOS

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* System dependent library loading routines                           */

#include <stdio.h>
#include <be/kernel/image.h>

#include "SDL_loadso.h"

void *
SDL_LoadObject(const char *sofile)
{
    void *handle = NULL;
    image_id library_id = load_add_on(sofile);
    if (library_id < 0) {
        SDL_SetError(strerror((int) library_id));
    } else {
        handle = (void *) (library_id);
    }
    return (handle);
}

void *
SDL_LoadFunction(void *handle, const char *name)
{
    void *sym = NULL;
    image_id library_id = (image_id) handle;
    status_t rc =
        get_image_symbol(library_id, name, B_SYMBOL_TYPE_TEXT, &sym);
    if (rc != B_NO_ERROR) {
        SDL_SetError(strerror(rc));
    }
    return (sym);
}

void
SDL_UnloadObject(void *handle)
{
    image_id library_id;
    if (handle != NULL) {
        library_id = (image_id) handle;
        unload_add_on(library_id);
    }
}

#endif /* SDL_LOADSO_BEOS */

/* vi: set ts=4 sw=4 expandtab: */
