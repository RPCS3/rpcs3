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

    SDL1.3 DirectFB driver by couriersud@arcor.de
	
*/

#include "SDL_config.h"

#include "SDL_DirectFB_video.h"
#include "SDL_DirectFB_dyn.h"

#ifdef SDL_VIDEO_DRIVER_DIRECTFB_DYNAMIC
#include "SDL_name.h"
#include "SDL_loadso.h"

#define DFB_SYM(ret, name, args, al, func) ret (*name) args;
static struct _SDL_DirectFB_Symbols
{
    DFB_SYMS 
    const unsigned int *directfb_major_version;
    const unsigned int *directfb_minor_version;
    const unsigned int *directfb_micro_version;
} SDL_DirectFB_Symbols;
#undef DFB_SYM

#define DFB_SYM(ret, name, args, al, func) ret name args { func SDL_DirectFB_Symbols.name al  ; }
DFB_SYMS
#undef DFB_SYM

static void *handle = NULL;

int
SDL_DirectFB_LoadLibrary(void)
{
    int retval = 0;

    if (handle == NULL) {
        handle = SDL_LoadObject(SDL_VIDEO_DRIVER_DIRECTFB_DYNAMIC);
        if (handle != NULL) {
            retval = 1;
#define DFB_SYM(ret, name, args, al, func) if (!(SDL_DirectFB_Symbols.name = SDL_LoadFunction(handle, # name))) retval = 0;
            DFB_SYMS
#undef DFB_SYM
            if (!
                    (SDL_DirectFB_Symbols.directfb_major_version =
                     SDL_LoadFunction(handle, "directfb_major_version")))
                retval = 0;
            if (!
                (SDL_DirectFB_Symbols.directfb_minor_version =
                 SDL_LoadFunction(handle, "directfb_minor_version")))
                retval = 0;
            if (!
                (SDL_DirectFB_Symbols.directfb_micro_version =
                 SDL_LoadFunction(handle, "directfb_micro_version")))
                retval = 0;
        }
    }
    if (retval) {
        const char *stemp = DirectFBCheckVersion(DIRECTFB_MAJOR_VERSION,
                                                 DIRECTFB_MINOR_VERSION,
                                                 DIRECTFB_MICRO_VERSION);
        /* Version Check */
        if (stemp != NULL) {
            fprintf(stderr,
                    "DirectFB Lib: Version mismatch. Compiled: %d.%d.%d Library %d.%d.%d\n",
                    DIRECTFB_MAJOR_VERSION, DIRECTFB_MINOR_VERSION,
                    DIRECTFB_MICRO_VERSION,
                    *SDL_DirectFB_Symbols.directfb_major_version,
                    *SDL_DirectFB_Symbols.directfb_minor_version,
                    *SDL_DirectFB_Symbols.directfb_micro_version);
            retval = 0;
        }
    }
    if (!retval)
        SDL_DirectFB_UnLoadLibrary();
    return retval;
}

void
SDL_DirectFB_UnLoadLibrary(void)
{
    if (handle != NULL) {
        SDL_UnloadObject(handle);
        handle = NULL;
    }
}

#else
int
SDL_DirectFB_LoadLibrary(void)
{
    return 1;
}

void
SDL_DirectFB_UnLoadLibrary(void)
{
}
#endif
