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

#ifndef _SDL_windowswindow_h
#define _SDL_windowswindow_h

#ifdef _WIN32_WCE
#define SHFS_SHOWTASKBAR        0x0001
#define SHFS_HIDETASKBAR        0x0002
#define SHFS_SHOWSIPBUTTON      0x0004
#define SHFS_HIDESIPBUTTON      0x0008
#define SHFS_SHOWSTARTICON      0x0010
#define SHFS_HIDESTARTICON      0x0020
#endif

typedef struct
{
    SDL_Window *window;
    HWND hwnd;
    HDC hdc;
    HDC mdc;
    HBITMAP hbm;
    WNDPROC wndproc;
    SDL_bool created;
    int mouse_pressed;
    int windowed_x;
    int windowed_y;
    struct SDL_VideoData *videodata;
} SDL_WindowData;

extern int WIN_CreateWindow(_THIS, SDL_Window * window);
extern int WIN_CreateWindowFrom(_THIS, SDL_Window * window, const void *data);
extern void WIN_SetWindowTitle(_THIS, SDL_Window * window);
extern void WIN_SetWindowIcon(_THIS, SDL_Window * window, SDL_Surface * icon);
extern void WIN_SetWindowPosition(_THIS, SDL_Window * window);
extern void WIN_SetWindowSize(_THIS, SDL_Window * window);
extern void WIN_ShowWindow(_THIS, SDL_Window * window);
extern void WIN_HideWindow(_THIS, SDL_Window * window);
extern void WIN_RaiseWindow(_THIS, SDL_Window * window);
extern void WIN_MaximizeWindow(_THIS, SDL_Window * window);
extern void WIN_MinimizeWindow(_THIS, SDL_Window * window);
extern void WIN_RestoreWindow(_THIS, SDL_Window * window);
extern void WIN_SetWindowFullscreen(_THIS, SDL_Window * window, SDL_VideoDisplay * display, SDL_bool fullscreen);
extern void WIN_SetWindowGrab(_THIS, SDL_Window * window);
extern void WIN_DestroyWindow(_THIS, SDL_Window * window);
extern SDL_bool WIN_GetWindowWMInfo(_THIS, SDL_Window * window,
                                    struct SDL_SysWMinfo *info);

#endif /* _SDL_windowswindow_h */

/* vi: set ts=4 sw=4 expandtab: */
