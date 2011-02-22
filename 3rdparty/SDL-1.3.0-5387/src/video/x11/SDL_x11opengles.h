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

#include <GLES/gl.h>
#include <GLES/egl.h>
#include <dlfcn.h>
#if defined(__OpenBSD__) && !defined(__ELF__)
#define dlsym(x,y) dlsym(x, "_" y)
#endif

#include "../SDL_sysvideo.h"

typedef struct SDL_PrivateGLESData
{
    int egl_active;             /* to stop switching drivers while we have a valid context */
    XVisualInfo *egl_visualinfo;
    EGLDisplay egl_display;
    EGLContext egl_context;     /* Current GLES context */
    EGLSurface egl_surface;
    EGLConfig egl_config;

      EGLDisplay(*eglGetDisplay) (NativeDisplayType display);
      EGLBoolean(*eglInitialize) (EGLDisplay dpy, EGLint * major,
                                  EGLint * minor);
      EGLBoolean(*eglTerminate) (EGLDisplay dpy);

    void *(*eglGetProcAddress) (const GLubyte * procName);

      EGLBoolean(*eglChooseConfig) (EGLDisplay dpy,
                                    const EGLint * attrib_list,
                                    EGLConfig * configs,
                                    EGLint config_size, EGLint * num_config);

      EGLContext(*eglCreateContext) (EGLDisplay dpy,
                                     EGLConfig config,
                                     EGLContext share_list,
                                     const EGLint * attrib_list);

      EGLBoolean(*eglDestroyContext) (EGLDisplay dpy, EGLContext ctx);

      EGLSurface(*eglCreateWindowSurface) (EGLDisplay dpy,
                                           EGLConfig config,
                                           NativeWindowType window,
                                           const EGLint * attrib_list);
      EGLBoolean(*eglDestroySurface) (EGLDisplay dpy, EGLSurface surface);

      EGLBoolean(*eglMakeCurrent) (EGLDisplay dpy, EGLSurface draw,
                                   EGLSurface read, EGLContext ctx);

      EGLBoolean(*eglSwapBuffers) (EGLDisplay dpy, EGLSurface draw);

    const char *(*eglQueryString) (EGLDisplay dpy, EGLint name);

      EGLBoolean(*eglGetConfigAttrib) (EGLDisplay dpy, EGLConfig config,
                                       EGLint attribute, EGLint * value);

} SDL_PrivateGLESData;

/* OpenGLES functions */
extern SDL_GLContext X11_GLES_CreateContext(_THIS, SDL_Window * window);
extern XVisualInfo *X11_GLES_GetVisual(_THIS, Display * display, int screen);
extern int X11_GLES_MakeCurrent(_THIS, SDL_Window * window,
                                SDL_GLContext context);
extern int X11_GLES_GetAttribute(_THIS, SDL_GLattr attrib, int *value);
extern int X11_GLES_LoadLibrary(_THIS, const char *path);
extern void *X11_GLES_GetProcAddress(_THIS, const char *proc);
extern void X11_GLES_UnloadLibrary(_THIS);

extern int X11_GLES_SetSwapInterval(_THIS, int interval);
extern int X11_GLES_GetSwapInterval(_THIS);
extern void X11_GLES_SwapWindow(_THIS, SDL_Window * window);
extern void X11_GLES_DeleteContext(_THIS, SDL_GLContext context);
