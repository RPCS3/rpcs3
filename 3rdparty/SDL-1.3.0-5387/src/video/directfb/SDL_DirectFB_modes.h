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

#ifndef _SDL_directfb_modes_h
#define _SDL_directfb_modes_h

#include <directfb.h>

#include "../SDL_sysvideo.h"

#define SDL_DFB_DISPLAYDATA(win)  DFB_DisplayData *dispdata = ((win) ? (DFB_DisplayData *) (win)->display->driverdata : NULL)

typedef struct _DFB_DisplayData DFB_DisplayData;
struct _DFB_DisplayData
{
    IDirectFBDisplayLayer 	*layer;
    DFBSurfacePixelFormat 	pixelformat;
    /* FIXME: support for multiple video layer. 
     * However, I do not know any card supporting 
     * more than one
     */
    DFBDisplayLayerID 		vidID;
    IDirectFBDisplayLayer 	*vidlayer;

    int 					vidIDinuse;

    int 					cw;
    int 					ch;
};


extern void DirectFB_InitModes(_THIS);
extern void DirectFB_GetDisplayModes(_THIS, SDL_VideoDisplay * display);
extern int DirectFB_SetDisplayMode(_THIS, SDL_VideoDisplay * display, SDL_DisplayMode * mode);
extern void DirectFB_QuitModes(_THIS);

extern void DirectFB_SetContext(_THIS, SDL_Window *window);

#endif /* _SDL_directfb_modes_h */

/* vi: set ts=4 sw=4 expandtab: */
