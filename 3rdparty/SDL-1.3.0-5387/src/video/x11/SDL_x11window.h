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

#ifndef _SDL_x11window_h
#define _SDL_x11window_h

typedef struct
{
    SDL_Window *window;
    Window xwindow;
    Visual *visual;
#ifndef NO_SHARED_MEMORY
    /* MIT shared memory extension information */
    SDL_bool use_mitshm;
    XShmSegmentInfo shminfo;
#endif
    XImage *ximage;
    GC gc;
    XIC ic;
    SDL_bool created;
    struct SDL_VideoData *videodata;
} SDL_WindowData;

extern int X11_CreateWindow(_THIS, SDL_Window * window);
extern int X11_CreateWindowFrom(_THIS, SDL_Window * window, const void *data);
extern char *X11_GetWindowTitle(_THIS, Window xwindow);
extern void X11_SetWindowTitle(_THIS, SDL_Window * window);
extern void X11_SetWindowIcon(_THIS, SDL_Window * window, SDL_Surface * icon);
extern void X11_SetWindowPosition(_THIS, SDL_Window * window);
extern void X11_SetWindowSize(_THIS, SDL_Window * window);
extern void X11_ShowWindow(_THIS, SDL_Window * window);
extern void X11_HideWindow(_THIS, SDL_Window * window);
extern void X11_RaiseWindow(_THIS, SDL_Window * window);
extern void X11_MaximizeWindow(_THIS, SDL_Window * window);
extern void X11_MinimizeWindow(_THIS, SDL_Window * window);
extern void X11_RestoreWindow(_THIS, SDL_Window * window);
extern void X11_SetWindowFullscreen(_THIS, SDL_Window * window, SDL_VideoDisplay * display, SDL_bool fullscreen);
extern void X11_SetWindowGrab(_THIS, SDL_Window * window);
extern void X11_DestroyWindow(_THIS, SDL_Window * window);
extern SDL_bool X11_GetWindowWMInfo(_THIS, SDL_Window * window,
                                    struct SDL_SysWMinfo *info);

#endif /* _SDL_x11window_h */

/* vi: set ts=4 sw=4 expandtab: */
