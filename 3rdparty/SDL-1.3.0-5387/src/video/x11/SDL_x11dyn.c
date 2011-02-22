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

#define DEBUG_DYNAMIC_X11 0

#include "SDL_x11dyn.h"

#if DEBUG_DYNAMIC_X11
#include <stdio.h>
#endif

#ifdef SDL_VIDEO_DRIVER_X11_DYNAMIC

#include "SDL_name.h"
#include "SDL_loadso.h"

typedef struct
{
    void *lib;
    const char *libname;
} x11dynlib;

#ifndef SDL_VIDEO_DRIVER_X11_DYNAMIC
#define SDL_VIDEO_DRIVER_X11_DYNAMIC NULL
#endif
#ifndef SDL_VIDEO_DRIVER_X11_DYNAMIC_XEXT
#define SDL_VIDEO_DRIVER_X11_DYNAMIC_XEXT NULL
#endif
#ifndef SDL_VIDEO_DRIVER_X11_DYNAMIC_XRENDER
#define SDL_VIDEO_DRIVER_X11_DYNAMIC_XRENDER NULL
#endif
#ifndef SDL_VIDEO_DRIVER_X11_DYNAMIC_XRANDR
#define SDL_VIDEO_DRIVER_X11_DYNAMIC_XRANDR NULL
#endif
#ifndef SDL_VIDEO_DRIVER_X11_DYNAMIC_XINPUT
#define SDL_VIDEO_DRIVER_X11_DYNAMIC_XINPUT NULL
#endif
#ifndef SDL_VIDEO_DRIVER_X11_DYNAMIC_XSS
#define SDL_VIDEO_DRIVER_X11_DYNAMIC_XSS NULL
#endif

static x11dynlib x11libs[] = {
    {NULL, SDL_VIDEO_DRIVER_X11_DYNAMIC},
    {NULL, SDL_VIDEO_DRIVER_X11_DYNAMIC_XEXT},
    {NULL, SDL_VIDEO_DRIVER_X11_DYNAMIC_XRENDER},
    {NULL, SDL_VIDEO_DRIVER_X11_DYNAMIC_XRANDR},
    {NULL, SDL_VIDEO_DRIVER_X11_DYNAMIC_XINPUT},
    {NULL, SDL_VIDEO_DRIVER_X11_DYNAMIC_XSS},
};

static void
X11_GetSym(const char *fnname, int *rc, void **fn)
{
    int i;
    for (i = 0; i < SDL_TABLESIZE(x11libs); i++) {
        if (x11libs[i].lib != NULL) {
            *fn = SDL_LoadFunction(x11libs[i].lib, fnname);
            if (*fn != NULL)
                break;
        }
    }

#if DEBUG_DYNAMIC_X11
    if (*fn != NULL)
        printf("X11: Found '%s' in %s (%p)\n", fnname, x11libs[i].libname,
               *fn);
    else
        printf("X11: Symbol '%s' NOT FOUND!\n", fnname);
#endif

    if (*fn == NULL)
        *rc = 0;                /* kill this module. */
}


/* Define all the function pointers and wrappers... */
#define SDL_X11_MODULE(modname)
#define SDL_X11_SYM(rc,fn,params,args,ret) \
	static rc (*p##fn) params = NULL; \
	rc fn params { ret p##fn args ; }
#include "SDL_x11sym.h"
#undef SDL_X11_MODULE
#undef SDL_X11_SYM
#endif /* SDL_VIDEO_DRIVER_X11_DYNAMIC */

/* Annoying varargs entry point... */
#ifdef X_HAVE_UTF8_STRING
XIC(*pXCreateIC) (XIM,...) = NULL;
char *(*pXGetICValues) (XIC, ...) = NULL;
#endif

/* These SDL_X11_HAVE_* flags are here whether you have dynamic X11 or not. */
#define SDL_X11_MODULE(modname) int SDL_X11_HAVE_##modname = 1;
#define SDL_X11_SYM(rc,fn,params,args,ret)
#include "SDL_x11sym.h"
#undef SDL_X11_MODULE
#undef SDL_X11_SYM


static int x11_load_refcount = 0;

void
SDL_X11_UnloadSymbols(void)
{
#ifdef SDL_VIDEO_DRIVER_X11_DYNAMIC
    /* Don't actually unload if more than one module is using the libs... */
    if (x11_load_refcount > 0) {
        if (--x11_load_refcount == 0) {
            int i;

            /* set all the function pointers to NULL. */
#define SDL_X11_MODULE(modname) SDL_X11_HAVE_##modname = 1;
#define SDL_X11_SYM(rc,fn,params,args,ret) p##fn = NULL;
#include "SDL_x11sym.h"
#undef SDL_X11_MODULE
#undef SDL_X11_SYM

#ifdef X_HAVE_UTF8_STRING
            pXCreateIC = NULL;
            pXGetICValues = NULL;
#endif

            for (i = 0; i < SDL_TABLESIZE(x11libs); i++) {
                if (x11libs[i].lib != NULL) {
                    SDL_UnloadObject(x11libs[i].lib);
                    x11libs[i].lib = NULL;
                }
            }
        }
    }
#endif
}

/* returns non-zero if all needed symbols were loaded. */
int
SDL_X11_LoadSymbols(void)
{
    int rc = 1;                 /* always succeed if not using Dynamic X11 stuff. */

#ifdef SDL_VIDEO_DRIVER_X11_DYNAMIC
    /* deal with multiple modules (dga, x11, etc) needing these symbols... */
    if (x11_load_refcount++ == 0) {
        int i;
        int *thismod = NULL;
        for (i = 0; i < SDL_TABLESIZE(x11libs); i++) {
            if (x11libs[i].libname != NULL) {
                x11libs[i].lib = SDL_LoadObject(x11libs[i].libname);
            }
        }
#define SDL_X11_MODULE(modname) thismod = &SDL_X11_HAVE_##modname;
#define SDL_X11_SYM(a,fn,x,y,z) X11_GetSym(#fn,thismod,(void**)&p##fn);
#include "SDL_x11sym.h"
#undef SDL_X11_MODULE
#undef SDL_X11_SYM

#ifdef X_HAVE_UTF8_STRING
        X11_GetSym("XCreateIC", &SDL_X11_HAVE_UTF8, (void **) &pXCreateIC);
        X11_GetSym("XGetICValues", &SDL_X11_HAVE_UTF8,
                   (void **) &pXGetICValues);
#endif

        if (SDL_X11_HAVE_BASEXLIB) {
            /* all required symbols loaded. */
            SDL_ClearError();
        } else {
            /* in case something got loaded... */
            SDL_X11_UnloadSymbols();
            rc = 0;
        }
    }
#else
#ifdef X_HAVE_UTF8_STRING
    pXCreateIC = XCreateIC;
    pXGetICValues = XGetICValues;
#endif
#endif

    return rc;
}

/* vi: set ts=4 sw=4 expandtab: */
