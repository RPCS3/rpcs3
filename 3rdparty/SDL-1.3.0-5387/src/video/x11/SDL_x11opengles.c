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

    Open Pandora SDL driver
    Copyright (C) 2009 David Carré
    (cpasjuste@gmail.com)
*/
#include "SDL_config.h"

#if SDL_VIDEO_OPENGL_ES

#include "SDL_x11video.h"
#include "SDL_x11opengles.h"

#define DEFAULT_OPENGL	"/usr/lib/libGLES_CM.so"

#define LOAD_FUNC(NAME) \
	*((void**)&_this->gles_data->NAME) = dlsym(handle, #NAME); \
	if (!_this->gles_data->NAME) \
	{ \
		SDL_SetError("Could not retrieve EGL function " #NAME); \
		return -1; \
	}

/* GLES implementation of SDL OpenGL support */

void *
X11_GLES_GetProcAddress(_THIS, const char *proc)
{
    static char procname[1024];
    void *handle;
    void *retval;

    handle = _this->gl_config.dll_handle;
    if (_this->gles_data->eglGetProcAddress) {
        retval = _this->gles_data->eglGetProcAddress(proc);
        if (retval) {
            return retval;
        }
    }
#if defined(__OpenBSD__) && !defined(__ELF__)
#undef dlsym(x,y);
#endif
    retval = dlsym(handle, proc);
    if (!retval && strlen(proc) <= 1022) {
        procname[0] = '_';
        strcpy(procname + 1, proc);
        retval = dlsym(handle, procname);
    }
    return retval;
}

void
X11_GLES_UnloadLibrary(_THIS)
{
    if (_this->gl_config.driver_loaded) {
        _this->gles_data->eglTerminate(_this->gles_data->egl_display);

        dlclose(_this->gl_config.dll_handle);

        _this->gles_data->eglGetProcAddress = NULL;
        _this->gles_data->eglChooseConfig = NULL;
        _this->gles_data->eglCreateContext = NULL;
        _this->gles_data->eglCreateWindowSurface = NULL;
        _this->gles_data->eglDestroyContext = NULL;
        _this->gles_data->eglDestroySurface = NULL;
        _this->gles_data->eglMakeCurrent = NULL;
        _this->gles_data->eglSwapBuffers = NULL;
        _this->gles_data->eglGetDisplay = NULL;
        _this->gles_data->eglTerminate = NULL;

        _this->gl_config.dll_handle = NULL;
        _this->gl_config.driver_loaded = 0;
    }
}

int
X11_GLES_LoadLibrary(_THIS, const char *path)
{
    void *handle;
    int dlopen_flags;

    SDL_VideoData *data = (SDL_VideoData *) _this->driverdata;

    if (_this->gles_data->egl_active) {
        SDL_SetError("OpenGL ES context already created");
        return -1;
    }
#ifdef RTLD_GLOBAL
    dlopen_flags = RTLD_LAZY | RTLD_GLOBAL;
#else
    dlopen_flags = RTLD_LAZY;
#endif
    handle = dlopen(path, dlopen_flags);
    /* Catch the case where the application isn't linked with EGL */
    if ((dlsym(handle, "eglChooseConfig") == NULL) && (path == NULL)) {

        dlclose(handle);
        path = getenv("SDL_VIDEO_GL_DRIVER");
        if (path == NULL) {
            path = DEFAULT_OPENGL;
        }
        handle = dlopen(path, dlopen_flags);
    }

    if (handle == NULL) {
        SDL_SetError("Could not load OpenGL ES/EGL library");
        return -1;
    }

    /* Unload the old driver and reset the pointers */
    X11_GLES_UnloadLibrary(_this);

    /* Load new function pointers */
    LOAD_FUNC(eglGetDisplay);
    LOAD_FUNC(eglInitialize);
    LOAD_FUNC(eglTerminate);
    LOAD_FUNC(eglGetProcAddress);
    LOAD_FUNC(eglChooseConfig);
    LOAD_FUNC(eglGetConfigAttrib);
    LOAD_FUNC(eglCreateContext);
    LOAD_FUNC(eglDestroyContext);
    LOAD_FUNC(eglCreateWindowSurface);
    LOAD_FUNC(eglDestroySurface);
    LOAD_FUNC(eglMakeCurrent);
    LOAD_FUNC(eglSwapBuffers);

    _this->gles_data->egl_display =
        _this->gles_data->eglGetDisplay((NativeDisplayType) data->display);

    if (!_this->gles_data->egl_display) {
        SDL_SetError("Could not get EGL display");
        return -1;
    }

    if (_this->gles_data->
        eglInitialize(_this->gles_data->egl_display, NULL,
                      NULL) != EGL_TRUE) {
        SDL_SetError("Could not initialize EGL");
        return -1;
    }

    _this->gl_config.dll_handle = handle;
    _this->gl_config.driver_loaded = 1;

    if (path) {
        strncpy(_this->gl_config.driver_path, path,
                sizeof(_this->gl_config.driver_path) - 1);
    } else {
        strcpy(_this->gl_config.driver_path, "");
    }
    return 0;
}

XVisualInfo *
X11_GLES_GetVisual(_THIS, Display * display, int screen)
{
    /* 64 seems nice. */
    EGLint attribs[64];
    EGLint found_configs = 0;
    VisualID visual_id;
    int i;

    /* load the gl driver from a default path */
    if (!_this->gl_config.driver_loaded) {
        /* no driver has been loaded, use default (ourselves) */
        if (X11_GLES_LoadLibrary(_this, NULL) < 0) {
            return NULL;
        }
    }

    i = 0;
    attribs[i++] = EGL_RED_SIZE;
    attribs[i++] = _this->gl_config.red_size;
    attribs[i++] = EGL_GREEN_SIZE;
    attribs[i++] = _this->gl_config.green_size;
    attribs[i++] = EGL_BLUE_SIZE;
    attribs[i++] = _this->gl_config.blue_size;

    if (_this->gl_config.alpha_size) {
        attribs[i++] = EGL_ALPHA_SIZE;
        attribs[i++] = _this->gl_config.alpha_size;
    }

    if (_this->gl_config.buffer_size) {
        attribs[i++] = EGL_BUFFER_SIZE;
        attribs[i++] = _this->gl_config.buffer_size;
    }

    attribs[i++] = EGL_DEPTH_SIZE;
    attribs[i++] = _this->gl_config.depth_size;

    if (_this->gl_config.stencil_size) {
        attribs[i++] = EGL_STENCIL_SIZE;
        attribs[i++] = _this->gl_config.stencil_size;
    }

    if (_this->gl_config.multisamplebuffers) {
        attribs[i++] = EGL_SAMPLE_BUFFERS;
        attribs[i++] = _this->gl_config.multisamplebuffers;
    }

    if (_this->gl_config.multisamplesamples) {
        attribs[i++] = EGL_SAMPLES;
        attribs[i++] = _this->gl_config.multisamplesamples;
    }

    attribs[i++] = EGL_NONE;

    if (_this->gles_data->eglChooseConfig(_this->gles_data->egl_display,
                                          attribs,
                                          &_this->gles_data->egl_config, 1,
                                          &found_configs) == EGL_FALSE ||
        found_configs == 0) {
        SDL_SetError("Couldn't find matching EGL config");
        return NULL;
    }

    if (_this->gles_data->eglGetConfigAttrib(_this->gles_data->egl_display,
                                             _this->gles_data->egl_config,
                                             EGL_NATIVE_VISUAL_ID,
                                             (EGLint *) & visual_id) ==
        EGL_FALSE || !visual_id) {
        /* Use the default visual when all else fails */
        XVisualInfo vi_in;
        int out_count;
        vi_in.screen = screen;

        _this->gles_data->egl_visualinfo = XGetVisualInfo(display,
                                                          VisualScreenMask,
                                                          &vi_in, &out_count);
    } else {
        XVisualInfo vi_in;
        int out_count;

        vi_in.screen = screen;
        vi_in.visualid = visual_id;
        _this->gles_data->egl_visualinfo = XGetVisualInfo(display,
                                                          VisualScreenMask |
                                                          VisualIDMask,
                                                          &vi_in, &out_count);
    }

    return _this->gles_data->egl_visualinfo;
}

SDL_GLContext
X11_GLES_CreateContext(_THIS, SDL_Window * window)
{
    int retval;
    SDL_WindowData *data = (SDL_WindowData *) window->driverdata;
    Display *display = data->videodata->display;

    XSync(display, False);


    _this->gles_data->egl_context =
        _this->gles_data->eglCreateContext(_this->gles_data->egl_display,
                                           _this->gles_data->egl_config,
                                           EGL_NO_CONTEXT, NULL);
    XSync(display, False);

    if (_this->gles_data->egl_context == EGL_NO_CONTEXT) {
        SDL_SetError("Could not create EGL context");
        return NULL;
    }

    _this->gles_data->egl_active = 1;

    if (_this->gles_data->egl_active)
        retval = 1;
    else
        retval = 0;

    return (retval);
}

int
X11_GLES_MakeCurrent(_THIS, SDL_Window * window, SDL_GLContext context)
{
    int retval;

//    SDL_WindowData *data = (SDL_WindowData *) window->driverdata;
//    Display *display = data->videodata->display;

    retval = 1;
    if (!_this->gles_data->eglMakeCurrent(_this->gles_data->egl_display,
                                          _this->gles_data->egl_surface,
                                          _this->gles_data->egl_surface,
                                          _this->gles_data->egl_context)) {
        SDL_SetError("Unable to make EGL context current");
        retval = -1;
    }
//    XSync(display, False);

    return (retval);
}

static int swapinterval = -1;
int
X11_GLES_SetSwapInterval(_THIS, int interval)
{
    return 0;
}

int
X11_GLES_GetSwapInterval(_THIS)
{
    return 0;
}

void
X11_GLES_SwapWindow(_THIS, SDL_Window * window)
{
    _this->gles_data->eglSwapBuffers(_this->gles_data->egl_display,
                                     _this->gles_data->egl_surface);
}

void
X11_GLES_DeleteContext(_THIS, SDL_GLContext context)
{
    /* Clean up GLES and EGL */
    if (_this->gles_data->egl_context != EGL_NO_CONTEXT ||
        _this->gles_data->egl_surface != EGL_NO_SURFACE) {
        _this->gles_data->eglMakeCurrent(_this->gles_data->egl_display,
                                         EGL_NO_SURFACE, EGL_NO_SURFACE,
                                         EGL_NO_CONTEXT);

        if (_this->gles_data->egl_context != EGL_NO_CONTEXT) {
            _this->gles_data->eglDestroyContext(_this->gles_data->egl_display,
                                                _this->gles_data->
                                                egl_context);
            _this->gles_data->egl_context = EGL_NO_CONTEXT;
        }

        if (_this->gles_data->egl_surface != EGL_NO_SURFACE) {
            _this->gles_data->eglDestroySurface(_this->gles_data->egl_display,
                                                _this->gles_data->
                                                egl_surface);
            _this->gles_data->egl_surface = EGL_NO_SURFACE;
        }
    }
    _this->gles_data->egl_active = 0;

/* crappy fix */
    X11_GLES_UnloadLibrary(_this);

}

#endif /* SDL_VIDEO_OPENGL_ES */

/* vi: set ts=4 sw=4 expandtab: */
