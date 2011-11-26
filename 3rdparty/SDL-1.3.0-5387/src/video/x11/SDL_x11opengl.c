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

#include "SDL_x11video.h"

/* GLX implementation of SDL OpenGL support */

#if SDL_VIDEO_OPENGL_GLX
#include "SDL_loadso.h"

#if defined(__IRIX__)
/* IRIX doesn't have a GL library versioning system */
#define DEFAULT_OPENGL	"libGL.so"
#elif defined(__MACOSX__)
#define DEFAULT_OPENGL	"/usr/X11R6/lib/libGL.1.dylib"
#elif defined(__QNXNTO__)
#define DEFAULT_OPENGL	"libGL.so.3"
#else
#define DEFAULT_OPENGL	"libGL.so.1"
#endif

#ifndef GLX_NONE_EXT
#define GLX_NONE_EXT                       0x8000
#endif

#ifndef GLX_ARB_multisample
#define GLX_ARB_multisample
#define GLX_SAMPLE_BUFFERS_ARB             100000
#define GLX_SAMPLES_ARB                    100001
#endif

#ifndef GLX_EXT_visual_rating
#define GLX_EXT_visual_rating
#define GLX_VISUAL_CAVEAT_EXT              0x20
#define GLX_NONE_EXT                       0x8000
#define GLX_SLOW_VISUAL_EXT                0x8001
#define GLX_NON_CONFORMANT_VISUAL_EXT      0x800D
#endif

#ifndef GLX_ARB_create_context
#define GLX_ARB_create_context
#define GLX_CONTEXT_MAJOR_VERSION_ARB      0x2091
#define GLX_CONTEXT_MINOR_VERSION_ARB      0x2092
#define GLX_CONTEXT_FLAGS_ARB              0x2094
#define GLX_CONTEXT_DEBUG_BIT_ARB          0x0001
#define GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB 0x0002

/* Typedef for the GL 3.0 context creation function */
typedef GLXContext(*PFNGLXCREATECONTEXTATTRIBSARBPROC) (Display * dpy,
                                                        GLXFBConfig config,
                                                        GLXContext
                                                        share_context,
                                                        Bool direct,
                                                        const int
                                                        *attrib_list);
#endif

#define OPENGL_REQUIRS_DLOPEN
#if defined(OPENGL_REQUIRS_DLOPEN) && defined(SDL_LOADSO_DLOPEN)
#include <dlfcn.h>
#define GL_LoadObject(X)	dlopen(X, (RTLD_NOW|RTLD_GLOBAL))
#define GL_LoadFunction		dlsym
#define GL_UnloadObject		dlclose
#else
#define GL_LoadObject	SDL_LoadObject
#define GL_LoadFunction	SDL_LoadFunction
#define GL_UnloadObject	SDL_UnloadObject
#endif

static void X11_GL_InitExtensions(_THIS);


int
X11_GL_LoadLibrary(_THIS, const char *path)
{
    void *handle;

    /* Load the OpenGL library */
    if (path == NULL) {
        path = SDL_getenv("SDL_OPENGL_LIBRARY");
    }
    if (path == NULL) {
        path = DEFAULT_OPENGL;
    }
    _this->gl_config.dll_handle = SDL_LoadObject(path);
    if (!_this->gl_config.dll_handle) {
        return -1;
    }
    SDL_strlcpy(_this->gl_config.driver_path, path,
                SDL_arraysize(_this->gl_config.driver_path));

    /* Allocate OpenGL memory */
    _this->gl_data =
        (struct SDL_GLDriverData *) SDL_calloc(1,
                                               sizeof(struct
                                                      SDL_GLDriverData));
    if (!_this->gl_data) {
        SDL_OutOfMemory();
        return -1;
    }

    /* Load function pointers */
    handle = _this->gl_config.dll_handle;
    _this->gl_data->glXGetProcAddress =
        (void *(*)(const GLubyte *))
            GL_LoadFunction(handle, "glXGetProcAddressARB");
    _this->gl_data->glXChooseVisual =
        (XVisualInfo * (*)(Display *, int, int *))
            X11_GL_GetProcAddress(_this, "glXChooseVisual");
    _this->gl_data->glXCreateContext =
        (GLXContext(*)(Display *, XVisualInfo *, GLXContext, int))
            X11_GL_GetProcAddress(_this, "glXCreateContext");
    _this->gl_data->glXDestroyContext =
        (void (*)(Display *, GLXContext))
            X11_GL_GetProcAddress(_this, "glXDestroyContext");
    _this->gl_data->glXMakeCurrent =
        (int (*)(Display *, GLXDrawable, GLXContext))
            X11_GL_GetProcAddress(_this, "glXMakeCurrent");
    _this->gl_data->glXSwapBuffers =
        (void (*)(Display *, GLXDrawable))
            X11_GL_GetProcAddress(_this, "glXSwapBuffers");

    if (!_this->gl_data->glXChooseVisual ||
        !_this->gl_data->glXCreateContext ||
        !_this->gl_data->glXDestroyContext ||
        !_this->gl_data->glXMakeCurrent ||
        !_this->gl_data->glXSwapBuffers) {
        SDL_SetError("Could not retrieve OpenGL functions");
        return -1;
    }

    /* Initialize extensions */
    X11_GL_InitExtensions(_this);

    return 0;
}

void *
X11_GL_GetProcAddress(_THIS, const char *proc)
{
    if (_this->gl_data->glXGetProcAddress) {
        return _this->gl_data->glXGetProcAddress((const GLubyte *) proc);
    }
    return GL_LoadFunction(_this->gl_config.dll_handle, proc);
}

void
X11_GL_UnloadLibrary(_THIS)
{
    /* Don't actually unload the library, since it may have registered
     * X11 shutdown hooks, per the notes at:
     * http://dri.sourceforge.net/doc/DRIuserguide.html
     */
#if 0
    GL_UnloadObject(_this->gl_config.dll_handle);
    _this->gl_config.dll_handle = NULL;
#endif

    /* Free OpenGL memory */
    SDL_free(_this->gl_data);
    _this->gl_data = NULL;
}

static SDL_bool
HasExtension(const char *extension, const char *extensions)
{
    const char *start;
    const char *where, *terminator;

    /* Extension names should not have spaces. */
    where = SDL_strchr(extension, ' ');
    if (where || *extension == '\0')
        return SDL_FALSE;

    if (!extensions)
        return SDL_FALSE;

    /* It takes a bit of care to be fool-proof about parsing the
     * OpenGL extensions string. Don't be fooled by sub-strings,
     * etc. */

    start = extensions;

    for (;;) {
        where = SDL_strstr(start, extension);
        if (!where)
            break;

        terminator = where + SDL_strlen(extension);
        if (where == start || *(where - 1) == ' ')
            if (*terminator == ' ' || *terminator == '\0')
                return SDL_TRUE;

        start = terminator;
    }
    return SDL_FALSE;
}

static void
X11_GL_InitExtensions(_THIS)
{
    Display *display = ((SDL_VideoData *) _this->driverdata)->display;
    int screen = DefaultScreen(display);
    XVisualInfo *vinfo;
    XSetWindowAttributes xattr;
    Window w;
    GLXContext context;
    const char *(*glXQueryExtensionsStringFunc) (Display *, int);
    const char *extensions;

    vinfo = X11_GL_GetVisual(_this, display, screen);
    if (!vinfo) {
        return;
    }
    xattr.background_pixel = 0;
    xattr.border_pixel = 0;
    xattr.colormap =
        XCreateColormap(display, RootWindow(display, screen), vinfo->visual,
                        AllocNone);
    w = XCreateWindow(display, RootWindow(display, screen), 0, 0, 32, 32, 0,
                      vinfo->depth, InputOutput, vinfo->visual,
                      (CWBackPixel | CWBorderPixel | CWColormap), &xattr);
    context = _this->gl_data->glXCreateContext(display, vinfo, NULL, True);
    if (context) {
        _this->gl_data->glXMakeCurrent(display, w, context);
    }
    XFree(vinfo);

    glXQueryExtensionsStringFunc =
        (const char *(*)(Display *, int)) X11_GL_GetProcAddress(_this,
                                                                "glXQueryExtensionsString");
    if (glXQueryExtensionsStringFunc) {
        extensions = glXQueryExtensionsStringFunc(display, screen);
    } else {
        extensions = NULL;
    }

    /* Check for SGI_swap_control */
    if (HasExtension("GLX_SGI_swap_control", extensions)) {
        _this->gl_data->glXSwapIntervalSGI =
            (int (*)(int)) X11_GL_GetProcAddress(_this, "glXSwapIntervalSGI");
    }

    /* Check for GLX_MESA_swap_control */
    if (HasExtension("GLX_MESA_swap_control", extensions)) {
        _this->gl_data->glXSwapIntervalMESA =
            (GLint(*)(unsigned)) X11_GL_GetProcAddress(_this,
                                                       "glXSwapIntervalMESA");
        _this->gl_data->glXGetSwapIntervalMESA =
            (GLint(*)(void)) X11_GL_GetProcAddress(_this,
                                                   "glXGetSwapIntervalMESA");
    }

    /* Check for GLX_EXT_visual_rating */
    if (HasExtension("GLX_EXT_visual_rating", extensions)) {
        _this->gl_data->HAS_GLX_EXT_visual_rating = SDL_TRUE;
    }

    if (context) {
        _this->gl_data->glXMakeCurrent(display, None, NULL);
        _this->gl_data->glXDestroyContext(display, context);
    }
    XDestroyWindow(display, w);
    X11_PumpEvents(_this);
}

XVisualInfo *
X11_GL_GetVisual(_THIS, Display * display, int screen)
{
    XVisualInfo *vinfo;

    /* 64 seems nice. */
    int attribs[64];
    int i = 0;

    /* Setup our GLX attributes according to the gl_config. */
    attribs[i++] = GLX_RGBA;
    attribs[i++] = GLX_RED_SIZE;
    attribs[i++] = _this->gl_config.red_size;
    attribs[i++] = GLX_GREEN_SIZE;
    attribs[i++] = _this->gl_config.green_size;
    attribs[i++] = GLX_BLUE_SIZE;
    attribs[i++] = _this->gl_config.blue_size;

    if (_this->gl_config.alpha_size) {
        attribs[i++] = GLX_ALPHA_SIZE;
        attribs[i++] = _this->gl_config.alpha_size;
    }

    if (_this->gl_config.buffer_size) {
        attribs[i++] = GLX_BUFFER_SIZE;
        attribs[i++] = _this->gl_config.buffer_size;
    }

    if (_this->gl_config.double_buffer) {
        attribs[i++] = GLX_DOUBLEBUFFER;
    }

    attribs[i++] = GLX_DEPTH_SIZE;
    attribs[i++] = _this->gl_config.depth_size;

    if (_this->gl_config.stencil_size) {
        attribs[i++] = GLX_STENCIL_SIZE;
        attribs[i++] = _this->gl_config.stencil_size;
    }

    if (_this->gl_config.accum_red_size) {
        attribs[i++] = GLX_ACCUM_RED_SIZE;
        attribs[i++] = _this->gl_config.accum_red_size;
    }

    if (_this->gl_config.accum_green_size) {
        attribs[i++] = GLX_ACCUM_GREEN_SIZE;
        attribs[i++] = _this->gl_config.accum_green_size;
    }

    if (_this->gl_config.accum_blue_size) {
        attribs[i++] = GLX_ACCUM_BLUE_SIZE;
        attribs[i++] = _this->gl_config.accum_blue_size;
    }

    if (_this->gl_config.accum_alpha_size) {
        attribs[i++] = GLX_ACCUM_ALPHA_SIZE;
        attribs[i++] = _this->gl_config.accum_alpha_size;
    }

    if (_this->gl_config.stereo) {
        attribs[i++] = GLX_STEREO;
    }

    if (_this->gl_config.multisamplebuffers) {
        attribs[i++] = GLX_SAMPLE_BUFFERS_ARB;
        attribs[i++] = _this->gl_config.multisamplebuffers;
    }

    if (_this->gl_config.multisamplesamples) {
        attribs[i++] = GLX_SAMPLES_ARB;
        attribs[i++] = _this->gl_config.multisamplesamples;
    }

    if (_this->gl_config.accelerated >= 0 &&
        _this->gl_data->HAS_GLX_EXT_visual_rating) {
        attribs[i++] = GLX_VISUAL_CAVEAT_EXT;
        attribs[i++] = _this->gl_config.accelerated ? GLX_NONE_EXT :
                                                      GLX_SLOW_VISUAL_EXT;
    }

    attribs[i++] = None;

    vinfo = _this->gl_data->glXChooseVisual(display, screen, attribs);
    if (!vinfo) {
        SDL_SetError("Couldn't find matching GLX visual");
    }
    return vinfo;
}

SDL_GLContext
X11_GL_CreateContext(_THIS, SDL_Window * window)
{
    SDL_WindowData *data = (SDL_WindowData *) window->driverdata;
    Display *display = data->videodata->display;
    int screen =
        ((SDL_DisplayData *) SDL_GetDisplayForWindow(window)->driverdata)->screen;
    XWindowAttributes xattr;
    XVisualInfo v, *vinfo;
    int n;
    GLXContext context = NULL;

    /* We do this to create a clean separation between X and GLX errors. */
    XSync(display, False);
    XGetWindowAttributes(display, data->xwindow, &xattr);
    v.screen = screen;
    v.visualid = XVisualIDFromVisual(xattr.visual);
    vinfo = XGetVisualInfo(display, VisualScreenMask | VisualIDMask, &v, &n);
    if (vinfo) {
        if (_this->gl_config.major_version < 3) {
            context =
                _this->gl_data->glXCreateContext(display, vinfo, NULL, True);
        } else {
            /* If we want a GL 3.0 context or later we need to get a temporary
               context to grab the new context creation function */
            GLXContext temp_context =
                _this->gl_data->glXCreateContext(display, vinfo, NULL, True);
            if (!temp_context) {
                SDL_SetError("Could not create GL context");
                return NULL;
            } else {
                int attribs[] = {
                    GLX_CONTEXT_MAJOR_VERSION_ARB,
                    _this->gl_config.major_version,
                    GLX_CONTEXT_MINOR_VERSION_ARB,
                    _this->gl_config.minor_version,
                    //FIXME GREGORY
                    // Request a debug context to ease opengl development
                    GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_DEBUG_BIT_ARB,
                    0
                };

                /* Get a pointer to the context creation function for GL 3.0 */
                PFNGLXCREATECONTEXTATTRIBSARBPROC glXCreateContextAttribs =
                    (PFNGLXCREATECONTEXTATTRIBSARBPROC) _this->gl_data->
                    glXGetProcAddress((GLubyte *)
                                      "glXCreateContextAttribsARB");
                if (!glXCreateContextAttribs) {
                    SDL_SetError("GL 3.x is not supported");
                    context = temp_context;
                } else {
                    /* Create a GL 3.x context */
                    GLXFBConfig *framebuffer_config = NULL;
                    int fbcount = 0;
                    GLXFBConfig *(*glXChooseFBConfig) (Display * disp,
                                                       int screen,
                                                       const int *attrib_list,
                                                       int *nelements);

                    glXChooseFBConfig =
                        (GLXFBConfig *
                         (*)(Display *, int, const int *,
                             int *)) _this->gl_data->
                        glXGetProcAddress((GLubyte *) "glXChooseFBConfig");

                    if (!glXChooseFBConfig
                        || !(framebuffer_config =
                             glXChooseFBConfig(display,
                                               DefaultScreen(display), NULL,
                                               &fbcount))) {
                        SDL_SetError
                            ("No good framebuffers found. GL 3.x disabled");
                        context = temp_context;
                    } else {
                        context =
                            glXCreateContextAttribs(display,
                                                    framebuffer_config[0],
                                                    NULL, True, attribs);
                        _this->gl_data->glXDestroyContext(display,
                                                          temp_context);
                    }
                }
            }
        }
        XFree(vinfo);
    }
    XSync(display, False);

    if (!context) {
        SDL_SetError("Could not create GL context");
        return NULL;
    }

    if (X11_GL_MakeCurrent(_this, window, context) < 0) {
        X11_GL_DeleteContext(_this, context);
        return NULL;
    }

    return context;
}

int
X11_GL_MakeCurrent(_THIS, SDL_Window * window, SDL_GLContext context)
{
    Display *display = ((SDL_VideoData *) _this->driverdata)->display;
    Window drawable =
        (window ? ((SDL_WindowData *) window->driverdata)->xwindow : None);
    GLXContext glx_context = (GLXContext) context;
    int status;

    status = 0;
    if (!_this->gl_data->glXMakeCurrent(display, drawable, glx_context)) {
        SDL_SetError("Unable to make GL context current");
        status = -1;
    }
    XSync(display, False);

    return (status);
}

/* 
   0 is a valid argument to glxSwapIntervalMESA and setting it to 0
   with the MESA version of the extension will undo the effect of a
   previous call with a value that is greater than zero (or at least
   that is what the FM says. OTOH, 0 is an invalid argument to
   glxSwapIntervalSGI and it returns an error if you call it with 0 as
   an argument.
*/

static int swapinterval = -1;
int
X11_GL_SetSwapInterval(_THIS, int interval)
{
    int status;

    if (_this->gl_data->glXSwapIntervalMESA) {
        status = _this->gl_data->glXSwapIntervalMESA(interval);
        if (status != 0) {
            SDL_SetError("glxSwapIntervalMESA failed");
            status = -1;
        } else {
            swapinterval = interval;
        }
    } else if (_this->gl_data->glXSwapIntervalSGI) {
        status = _this->gl_data->glXSwapIntervalSGI(interval);
        if (status != 0) {
            SDL_SetError("glxSwapIntervalSGI failed");
            status = -1;
        } else {
            swapinterval = interval;
        }
    } else {
        SDL_Unsupported();
        status = -1;
    }
    return status;
}

int
X11_GL_GetSwapInterval(_THIS)
{
    if (_this->gl_data->glXGetSwapIntervalMESA) {
        return _this->gl_data->glXGetSwapIntervalMESA();
    } else {
        return swapinterval;
    }
}

void
X11_GL_SwapWindow(_THIS, SDL_Window * window)
{
    SDL_WindowData *data = (SDL_WindowData *) window->driverdata;
    Display *display = data->videodata->display;

    _this->gl_data->glXSwapBuffers(display, data->xwindow);
}

void
X11_GL_DeleteContext(_THIS, SDL_GLContext context)
{
    Display *display = ((SDL_VideoData *) _this->driverdata)->display;
    GLXContext glx_context = (GLXContext) context;

    _this->gl_data->glXDestroyContext(display, glx_context);
    XSync(display, False);
}

#endif /* SDL_VIDEO_OPENGL_GLX */

/* vi: set ts=4 sw=4 expandtab: */
