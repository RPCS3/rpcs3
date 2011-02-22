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

#ifndef _SDL_uikitopengles
#define _SDL_uikitopengles

#include "SDL_uikitvideo.h"

extern int UIKit_GL_MakeCurrent(_THIS, SDL_Window * window,
                                SDL_GLContext context);
extern void UIKit_GL_SwapWindow(_THIS, SDL_Window * window);
extern SDL_GLContext UIKit_GL_CreateContext(_THIS, SDL_Window * window);
extern void UIKit_GL_DeleteContext(_THIS, SDL_GLContext context);
extern void *UIKit_GL_GetProcAddress(_THIS, const char *proc);
extern int UIKit_GL_LoadLibrary(_THIS, const char *path);

#endif

/* vi: set ts=4 sw=4 expandtab: */
