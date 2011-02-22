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

#ifndef _SDL_directfb_wm_h
#define _SDL_directfb_wm_h

#include "SDL_DirectFB_video.h"

typedef struct _DFB_Theme DFB_Theme;
struct _DFB_Theme
{
    int left_size;
    int right_size;
    int top_size;
    int bottom_size;
    DFBColor frame_color;
    int caption_size;
    DFBColor caption_color;
    int font_size;
    DFBColor font_color;
    char *font;
    DFBColor close_color;
    DFBColor max_color;
};

extern void DirectFB_WM_AdjustWindowLayout(SDL_Window * window, int flags, int w, int h);
extern void DirectFB_WM_RedrawLayout(_THIS, SDL_Window * window);

extern int DirectFB_WM_ProcessEvent(_THIS, SDL_Window * window,
                                    DFBWindowEvent * evt);

extern DFBResult DirectFB_WM_GetClientSize(_THIS, SDL_Window * window,
                                           int *cw, int *ch);


#endif /* _SDL_directfb_wm_h */

/* vi: set ts=4 sw=4 expandtab: */
