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

#include "../SDL_sysvideo.h"

#include "SDL_androidvideo.h"
#include "SDL_androidwindow.h"

int
Android_CreateWindow(_THIS, SDL_Window * window)
{
    if (Android_Window) {
        SDL_SetError("Android only supports one window");
        return -1;
    }
    Android_Window = window;

    /* Adjust the window data to match the screen */
    window->x = 0;
    window->y = 0;
    window->w = Android_ScreenWidth;
    window->h = Android_ScreenHeight;

    window->flags &= ~SDL_WINDOW_RESIZABLE;     /* window is NEVER resizeable */
    window->flags |= SDL_WINDOW_FULLSCREEN;     /* window is always fullscreen */
    window->flags |= SDL_WINDOW_SHOWN;          /* only one window on Android */
    window->flags |= SDL_WINDOW_INPUT_FOCUS;    /* always has input focus */    

    return 0;
}

void
Android_SetWindowTitle(_THIS, SDL_Window * window)
{
    Android_JNI_SetActivityTitle(window->title);
}

void
Android_DestroyWindow(_THIS, SDL_Window * window)
{
    if (window == Android_Window) {
        Android_Window = NULL;
    }
}

/* vi: set ts=4 sw=4 expandtab: */
