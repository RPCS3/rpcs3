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

#ifndef _SDL_directfb_window_h
#define _SDL_directfb_window_h

#include "SDL_DirectFB_video.h"
#include "SDL_DirectFB_WM.h"

#define SDL_DFB_WINDOWDATA(win)  DFB_WindowData *windata = ((win) ? (DFB_WindowData *) ((win)->driverdata) : NULL)

typedef struct _DFB_WindowData DFB_WindowData;
struct _DFB_WindowData
{
    IDirectFBSurface 		*window_surface;	/* window surface */
    IDirectFBSurface 		*surface;			/* client drawing surface */
    IDirectFBWindow 		*dfbwin;
    IDirectFBEventBuffer 	*eventbuffer;
    //SDL_Window 				*sdlwin;
    SDL_Window	 			*next;
    Uint8 					opacity;
    DFBRectangle 			client;
    DFBDimension 			size;
    DFBRectangle 			restore;

    /* WM extras */
    int 					is_managed;
    int 					wm_needs_redraw;
    IDirectFBSurface 		*icon;
    IDirectFBFont 			*font;
    DFB_Theme 				theme;

    /* WM moving and sizing */
    int 					wm_grab;
    int 					wm_lastx;
    int 					wm_lasty;
};

extern int DirectFB_CreateWindow(_THIS, SDL_Window * window);
extern int DirectFB_CreateWindowFrom(_THIS, SDL_Window * window,
                                     const void *data);
extern void DirectFB_SetWindowTitle(_THIS, SDL_Window * window);
extern void DirectFB_SetWindowIcon(_THIS, SDL_Window * window,
                                   SDL_Surface * icon);

extern void DirectFB_SetWindowPosition(_THIS, SDL_Window * window);
extern void DirectFB_SetWindowSize(_THIS, SDL_Window * window);
extern void DirectFB_ShowWindow(_THIS, SDL_Window * window);
extern void DirectFB_HideWindow(_THIS, SDL_Window * window);
extern void DirectFB_RaiseWindow(_THIS, SDL_Window * window);
extern void DirectFB_MaximizeWindow(_THIS, SDL_Window * window);
extern void DirectFB_MinimizeWindow(_THIS, SDL_Window * window);
extern void DirectFB_RestoreWindow(_THIS, SDL_Window * window);
extern void DirectFB_SetWindowGrab(_THIS, SDL_Window * window);
extern void DirectFB_DestroyWindow(_THIS, SDL_Window * window);
extern SDL_bool DirectFB_GetWindowWMInfo(_THIS, SDL_Window * window,
                                         struct SDL_SysWMinfo *info);

extern void DirectFB_AdjustWindowSurface(SDL_Window * window);

#endif /* _SDL_directfb_window_h */

/* vi: set ts=4 sw=4 expandtab: */
