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

#ifndef _SDL_cocoaopengl_h
#define _SDL_cocoaopengl_h

#if SDL_VIDEO_OPENGL_CGL

/* Define this if you want to be able to toggle fullscreen mode seamlessly */
#define FULLSCREEN_TOGGLEABLE

struct SDL_GLDriverData
{
    int initialized;
};

/* OpenGL functions */
extern int Cocoa_GL_LoadLibrary(_THIS, const char *path);
extern void *Cocoa_GL_GetProcAddress(_THIS, const char *proc);
extern void Cocoa_GL_UnloadLibrary(_THIS);
extern SDL_GLContext Cocoa_GL_CreateContext(_THIS, SDL_Window * window);
extern int Cocoa_GL_MakeCurrent(_THIS, SDL_Window * window,
                                SDL_GLContext context);
extern int Cocoa_GL_SetSwapInterval(_THIS, int interval);
extern int Cocoa_GL_GetSwapInterval(_THIS);
extern void Cocoa_GL_SwapWindow(_THIS, SDL_Window * window);
extern void Cocoa_GL_DeleteContext(_THIS, SDL_GLContext context);

#endif /* SDL_VIDEO_OPENGL_CGL */

#endif /* _SDL_cocoaopengl_h */

/* vi: set ts=4 sw=4 expandtab: */
