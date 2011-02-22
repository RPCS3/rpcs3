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

#ifndef _SDL_windowsmodes_h
#define _SDL_windowsmodes_h

typedef struct
{
    TCHAR DeviceName[32];
} SDL_DisplayData;

typedef struct
{
    DEVMODE DeviceMode;
} SDL_DisplayModeData;

extern int WIN_InitModes(_THIS);
extern int WIN_GetDisplayBounds(_THIS, SDL_VideoDisplay * display, SDL_Rect * rect);
extern void WIN_GetDisplayModes(_THIS, SDL_VideoDisplay * display);
extern int WIN_SetDisplayMode(_THIS, SDL_VideoDisplay * display, SDL_DisplayMode * mode);
extern void WIN_QuitModes(_THIS);

#endif /* _SDL_windowsmodes_h */

/* vi: set ts=4 sw=4 expandtab: */
