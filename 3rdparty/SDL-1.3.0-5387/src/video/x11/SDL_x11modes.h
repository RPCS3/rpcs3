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

#ifndef _SDL_x11modes_h
#define _SDL_x11modes_h

typedef struct
{
    int screen;
    Visual *visual;
    int depth;
    int scanline_pad;

    int use_xinerama;
    int use_xrandr;
    int use_vidmode;

#if SDL_VIDEO_DRIVER_X11_XINERAMA
      SDL_NAME(XineramaScreenInfo) xinerama_info;
#endif
#if SDL_VIDEO_DRIVER_X11_XRANDR
    XRRScreenConfiguration *screen_config;
    int saved_size;
    Rotation saved_rotation;
    short saved_rate;
#endif
#if SDL_VIDEO_DRIVER_X11_VIDMODE
      SDL_NAME(XF86VidModeModeInfo) saved_mode;
    struct
    {
        int x, y;
    } saved_view;
#endif

} SDL_DisplayData;

extern int X11_InitModes(_THIS);
extern void X11_GetDisplayModes(_THIS, SDL_VideoDisplay * display);
extern int X11_SetDisplayMode(_THIS, SDL_VideoDisplay * display, SDL_DisplayMode * mode);
extern void X11_QuitModes(_THIS);

/* Some utility functions for working with visuals */
extern int X11_GetVisualInfoFromVisual(Display * display, Visual * visual,
                                       XVisualInfo * vinfo);
extern Uint32 X11_GetPixelFormatFromVisualInfo(Display * display,
                                               XVisualInfo * vinfo);

#endif /* _SDL_x11modes_h */

/* vi: set ts=4 sw=4 expandtab: */
