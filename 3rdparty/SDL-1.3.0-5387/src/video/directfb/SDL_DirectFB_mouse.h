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

#ifndef _SDL_DirectFB_mouse_h
#define _SDL_DirectFB_mouse_h

#include <directfb.h>

#include "../SDL_sysvideo.h"

typedef struct _DFB_CursorData DFB_CursorData;
struct _DFB_CursorData
{
    IDirectFBSurface *surf;
    int 			hotx;
    int 			hoty;
};

#define SDL_DFB_CURSORDATA(curs)  DFB_CursorData *curdata = (DFB_CursorData *) ((curs) ? (curs)->driverdata : NULL)

extern void DirectFB_InitMouse(_THIS);
extern void DirectFB_QuitMouse(_THIS);

#endif /* _SDL_DirectFB_mouse_h */

/* vi: set ts=4 sw=4 expandtab: */
