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

#ifndef _SDL_x11dyn_h
#define _SDL_x11dyn_h

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

/* Apparently some X11 systems can't include this multiple times... */
#ifndef SDL_INCLUDED_XLIBINT_H
#define SDL_INCLUDED_XLIBINT_H 1
#include <X11/Xlibint.h>
#endif

#include <X11/Xproto.h>
#include "../Xext/extensions/Xext.h"
#include "../Xext/extensions/extutil.h"

#ifndef NO_SHARED_MEMORY
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
#endif

#if SDL_VIDEO_DRIVER_X11_XRANDR
#include <X11/extensions/Xrandr.h>
#endif

#if SDL_VIDEO_DRIVER_X11_XINPUT
#include <X11/extensions/XInput.h>
#endif

/*
 * When using the "dynamic X11" functionality, we duplicate all the Xlib
 *  symbols that would be referenced by SDL inside of SDL itself.
 *  These duplicated symbols just serve as passthroughs to the functions
 *  in Xlib, that was dynamically loaded.
 *
 * This allows us to use Xlib as-is when linking against it directly, but
 *  also handles all the strange cases where there was code in the Xlib
 *  headers that may or may not exist or vary on a given platform.
 */
#ifdef __cplusplus
extern "C"
{
#endif

/* evil function signatures... */
    typedef Bool(*SDL_X11_XESetWireToEventRetType) (Display *, XEvent *,
                                                    xEvent *);
    typedef int (*SDL_X11_XSynchronizeRetType) (Display *);
    typedef Status(*SDL_X11_XESetEventToWireRetType) (Display *, XEvent *,
                                                      xEvent *);

    int SDL_X11_LoadSymbols(void);
    void SDL_X11_UnloadSymbols(void);

/* That's really annoying...make these function pointers no matter what. */
#ifdef X_HAVE_UTF8_STRING
    extern XIC(*pXCreateIC) (XIM, ...);
    extern char *(*pXGetICValues) (XIC, ...);
#endif

/* These SDL_X11_HAVE_* flags are here whether you have dynamic X11 or not. */
#define SDL_X11_MODULE(modname) extern int SDL_X11_HAVE_##modname;
#define SDL_X11_SYM(rc,fn,params,args,ret)
#include "SDL_x11sym.h"
#undef SDL_X11_MODULE
#undef SDL_X11_SYM


#ifdef __cplusplus
}
#endif

#endif                          /* !defined _SDL_x11dyn_h */
/* vi: set ts=4 sw=4 expandtab: */
