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

#ifdef SDL_LOADSO_WINDOWS

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* System dependent library loading routines                           */

#include "../../core/windows/SDL_windows.h"

#include "SDL_loadso.h"

void *
SDL_LoadObject(const char *sofile)
{
    LPTSTR tstr = WIN_UTF8ToString(sofile);
    void *handle = (void *) LoadLibrary(tstr);
    SDL_free(tstr);

    /* Generate an error message if all loads failed */
    if (handle == NULL) {
        char errbuf[512];
        SDL_strlcpy(errbuf, "Failed loading ", SDL_arraysize(errbuf));
        SDL_strlcat(errbuf, sofile, SDL_arraysize(errbuf));
        WIN_SetError(errbuf);
    }
    return handle;
}

void *
SDL_LoadFunction(void *handle, const char *name)
{
#ifdef _WIN32_WCE
    LPTSTR tstr = WIN_UTF8ToString(name);
    void *symbol = (void *) GetProcAddress((HMODULE) handle, tstr);
    SDL_free(tstr);
#else
    void *symbol = (void *) GetProcAddress((HMODULE) handle, name);
#endif

    if (symbol == NULL) {
        char errbuf[512];
        SDL_strlcpy(errbuf, "Failed loading ", SDL_arraysize(errbuf));
        SDL_strlcat(errbuf, name, SDL_arraysize(errbuf));
        WIN_SetError(errbuf);
    }
    return symbol;
}

void
SDL_UnloadObject(void *handle)
{
    if (handle != NULL) {
        FreeLibrary((HMODULE) handle);
    }
}

#endif /* SDL_LOADSO_WINDOWS */

/* vi: set ts=4 sw=4 expandtab: */
