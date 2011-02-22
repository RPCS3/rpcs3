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

#ifndef _SDL_x11opengl_h
#define _SDL_x11opengl_h

#if SDL_VIDEO_OPENGL_GLX
#include "SDL_opengl.h"
#include <GL/glx.h>

struct SDL_GLDriverData
{
    SDL_bool HAS_GLX_EXT_visual_rating;

    void *(*glXGetProcAddress) (const GLubyte * procName);

    XVisualInfo *(*glXChooseVisual)
      (Display * dpy, int screen, int *attribList);

      GLXContext(*glXCreateContext)
      (Display * dpy, XVisualInfo * vis, GLXContext shareList, Bool direct);

    void (*glXDestroyContext)
      (Display * dpy, GLXContext ctx);

      Bool(*glXMakeCurrent)
      (Display * dpy, GLXDrawable drawable, GLXContext ctx);

    void (*glXSwapBuffers)
      (Display * dpy, GLXDrawable drawable);

    int (*glXSwapIntervalSGI) (int interval);
      GLint(*glXSwapIntervalMESA) (unsigned interval);
      GLint(*glXGetSwapIntervalMESA) (void);
};

/* OpenGL functions */
extern int X11_GL_LoadLibrary(_THIS, const char *path);
extern void *X11_GL_GetProcAddress(_THIS, const char *proc);
extern void X11_GL_UnloadLibrary(_THIS);
extern XVisualInfo *X11_GL_GetVisual(_THIS, Display * display, int screen);
extern SDL_GLContext X11_GL_CreateContext(_THIS, SDL_Window * window);
extern int X11_GL_MakeCurrent(_THIS, SDL_Window * window,
                              SDL_GLContext context);
extern int X11_GL_SetSwapInterval(_THIS, int interval);
extern int X11_GL_GetSwapInterval(_THIS);
extern void X11_GL_SwapWindow(_THIS, SDL_Window * window);
extern void X11_GL_DeleteContext(_THIS, SDL_GLContext context);

#endif /* SDL_VIDEO_OPENGL_GLX */

#endif /* _SDL_x11opengl_h */

/* vi: set ts=4 sw=4 expandtab: */
