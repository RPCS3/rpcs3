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
#include "../SDL_pixels_c.h"
#include "../../events/SDL_keyboard_c.h"
#include "../../events/SDL_mouse_c.h"

#include "SDL_x11video.h"
#include "SDL_x11mouse.h"
#include "SDL_x11shape.h"

#ifdef SDL_VIDEO_DRIVER_PANDORA
#include "SDL_x11opengles.h"
#endif

#include "SDL_timer.h"
#include "SDL_syswm.h"

#define _NET_WM_STATE_REMOVE    0l
#define _NET_WM_STATE_ADD       1l
#define _NET_WM_STATE_TOGGLE    2l

static SDL_bool
X11_IsWindowOldFullscreen(_THIS, SDL_Window * window)
{
    SDL_VideoData *videodata = (SDL_VideoData *) _this->driverdata;

    /* ICCCM2.0-compliant window managers can handle fullscreen windows */
    if ((window->flags & SDL_WINDOW_FULLSCREEN) && !videodata->net_wm) {
        return SDL_TRUE;
    } else {
        return SDL_FALSE;
    }
}

static SDL_bool
X11_IsWindowMapped(_THIS, SDL_Window * window)
{
    SDL_WindowData *data = (SDL_WindowData *) window->driverdata;
    SDL_VideoData *videodata = (SDL_VideoData *) _this->driverdata;
    XWindowAttributes attr;

    XGetWindowAttributes(videodata->display, data->xwindow, &attr);
    if (attr.map_state != IsUnmapped) {
        return SDL_TRUE;
    } else {
        return SDL_FALSE;
    }
}

static int
X11_GetWMStateProperty(_THIS, SDL_Window * window, Atom atoms[3])
{
    SDL_VideoData *data = (SDL_VideoData *) _this->driverdata;
    int count = 0;

    if (window->flags & SDL_WINDOW_FULLSCREEN) {
        atoms[count++] = data->_NET_WM_STATE_FULLSCREEN;
    }
    if (window->flags & SDL_WINDOW_MAXIMIZED) {
        atoms[count++] = data->_NET_WM_STATE_MAXIMIZED_VERT;
        atoms[count++] = data->_NET_WM_STATE_MAXIMIZED_HORZ;
    }
    return count;
}

static void
X11_GetDisplaySize(_THIS, SDL_Window * window, int *w, int *h)
{
    SDL_VideoData *data = (SDL_VideoData *) _this->driverdata;
    SDL_DisplayData *displaydata =
        (SDL_DisplayData *) SDL_GetDisplayForWindow(window)->driverdata;
    XWindowAttributes attr;

    XGetWindowAttributes(data->display, RootWindow(data->display, displaydata->screen), &attr);
    if (window->flags & SDL_WINDOW_FULLSCREEN) {
        /* The bounds when this window is visible is the fullscreen mode */
        SDL_DisplayMode fullscreen_mode;
        if (SDL_GetWindowDisplayMode(window, &fullscreen_mode) == 0) {
            attr.width = fullscreen_mode.w;
            attr.height = fullscreen_mode.h;
        }
    }
    if (w) {
        *w = attr.width;
    }
    if (h) {
        *h = attr.height;
    }
}

static int
SetupWindowData(_THIS, SDL_Window * window, Window w, BOOL created)
{
    SDL_VideoData *videodata = (SDL_VideoData *) _this->driverdata;
    SDL_WindowData *data;
    int numwindows = videodata->numwindows;
    int windowlistlength = videodata->windowlistlength;
    SDL_WindowData **windowlist = videodata->windowlist;

    /* Allocate the window data */
    data = (SDL_WindowData *) SDL_calloc(1, sizeof(*data));
    if (!data) {
        SDL_OutOfMemory();
        return -1;
    }
    data->window = window;
    data->xwindow = w;
#ifdef X_HAVE_UTF8_STRING
    if (SDL_X11_HAVE_UTF8) {
        data->ic =
            pXCreateIC(videodata->im, XNClientWindow, w, XNFocusWindow, w,
                       XNInputStyle, XIMPreeditNothing | XIMStatusNothing,
                       XNResourceName, videodata->classname, XNResourceClass,
                       videodata->classname, NULL);
    }
#endif
    data->created = created;
    data->videodata = videodata;

    /* Associate the data with the window */

    if (numwindows < windowlistlength) {
        windowlist[numwindows] = data;
        videodata->numwindows++;
    } else {
        windowlist =
            (SDL_WindowData **) SDL_realloc(windowlist,
                                            (numwindows +
                                             1) * sizeof(*windowlist));
        if (!windowlist) {
            SDL_OutOfMemory();
            SDL_free(data);
            return -1;
        }
        windowlist[numwindows] = data;
        videodata->numwindows++;
        videodata->windowlistlength++;
        videodata->windowlist = windowlist;
    }

    /* Fill in the SDL window with the window data */
    {
        XWindowAttributes attrib;

        XGetWindowAttributes(data->videodata->display, w, &attrib);
        window->x = attrib.x;
        window->y = attrib.y;
        window->w = attrib.width;
        window->h = attrib.height;
        if (attrib.map_state != IsUnmapped) {
            window->flags |= SDL_WINDOW_SHOWN;
        } else {
            window->flags &= ~SDL_WINDOW_SHOWN;
        }
        data->visual = attrib.visual;
    }

    {
        Atom _NET_WM_STATE = data->videodata->_NET_WM_STATE;
        Atom _NET_WM_STATE_MAXIMIZED_VERT = data->videodata->_NET_WM_STATE_MAXIMIZED_VERT;
        Atom _NET_WM_STATE_MAXIMIZED_HORZ = data->videodata->_NET_WM_STATE_MAXIMIZED_HORZ;
        Atom _NET_WM_STATE_FULLSCREEN = data->videodata->_NET_WM_STATE_FULLSCREEN;
        Atom actualType;
        int actualFormat;
        unsigned long i, numItems, bytesAfter;
        unsigned char *propertyValue = NULL;
        long maxLength = 1024;

        if (XGetWindowProperty(data->videodata->display, w, _NET_WM_STATE,
                               0l, maxLength, False, XA_ATOM, &actualType,
                               &actualFormat, &numItems, &bytesAfter,
                               &propertyValue) == Success) {
            Atom *atoms = (Atom *) propertyValue;
            int maximized = 0;
            int fullscreen = 0;

            for (i = 0; i < numItems; ++i) {
                if (atoms[i] == _NET_WM_STATE_MAXIMIZED_VERT) {
                    maximized |= 1;
                } else if (atoms[i] == _NET_WM_STATE_MAXIMIZED_HORZ) {
                    maximized |= 2;
                } else if ( atoms[i] == _NET_WM_STATE_FULLSCREEN) {
                    fullscreen = 1;
                }
            }
            if (maximized == 3) {
                window->flags |= SDL_WINDOW_MAXIMIZED;
            }  else if (fullscreen == 1) {
                window->flags |= SDL_WINDOW_FULLSCREEN;
            }
            XFree(propertyValue);
        }
    }

    /* FIXME: How can I tell?
       {
       DWORD style = GetWindowLong(hwnd, GWL_STYLE);
       if (style & WS_VISIBLE) {
       if (style & (WS_BORDER | WS_THICKFRAME)) {
       window->flags &= ~SDL_WINDOW_BORDERLESS;
       } else {
       window->flags |= SDL_WINDOW_BORDERLESS;
       }
       if (style & WS_THICKFRAME) {
       window->flags |= SDL_WINDOW_RESIZABLE;
       } else {
       window->flags &= ~SDL_WINDOW_RESIZABLE;
       }
       if (style & WS_MINIMIZE) {
       window->flags |= SDL_WINDOW_MINIMIZED;
       } else {
       window->flags &= ~SDL_WINDOW_MINIMIZED;
       }
       }
       if (GetFocus() == hwnd) {
       int index = data->videodata->keyboard;
       window->flags |= SDL_WINDOW_INPUT_FOCUS;
       SDL_SetKeyboardFocus(index, data->window);

       if (window->flags & SDL_WINDOW_INPUT_GRABBED) {
       RECT rect;
       GetClientRect(hwnd, &rect);
       ClientToScreen(hwnd, (LPPOINT) & rect);
       ClientToScreen(hwnd, (LPPOINT) & rect + 1);
       ClipCursor(&rect);
       }
       }
     */

    /* All done! */
    window->driverdata = data;
    return 0;
}

int
X11_CreateWindow(_THIS, SDL_Window * window)
{
    SDL_VideoData *data = (SDL_VideoData *) _this->driverdata;
    SDL_DisplayData *displaydata =
        (SDL_DisplayData *) SDL_GetDisplayForWindow(window)->driverdata;
    Display *display = data->display;
    int screen = displaydata->screen;
    Visual *visual;
    int depth;
    XSetWindowAttributes xattr;
    Window w;
    XSizeHints *sizehints;
    XWMHints *wmhints;
    XClassHint *classhints;
    Atom _NET_WM_WINDOW_TYPE;
    Atom _NET_WM_WINDOW_TYPE_NORMAL;
    int wmstate_count;
    Atom wmstate_atoms[3];

#if SDL_VIDEO_DRIVER_X11_XINERAMA
/* FIXME
    if ( use_xinerama ) {
        x = xinerama_info.x_org;
        y = xinerama_info.y_org;
    }
*/
#endif
#if SDL_VIDEO_OPENGL_GLX
    if (window->flags & SDL_WINDOW_OPENGL) {
        XVisualInfo *vinfo;

        vinfo = X11_GL_GetVisual(_this, display, screen);
        if (!vinfo) {
            return -1;
        }
        visual = vinfo->visual;
        depth = vinfo->depth;
        XFree(vinfo);
    } else
#endif
#ifdef SDL_VIDEO_DRIVER_PANDORA
    if (window->flags & SDL_WINDOW_OPENGL) {
        XVisualInfo *vinfo;

        vinfo = X11_GLES_GetVisual(_this, display, screen);
        if (!vinfo) {
            return -1;
        }
        visual = vinfo->visual;
        depth = vinfo->depth;
        XFree(vinfo);
    } else
#endif
    {
        visual = displaydata->visual;
        depth = displaydata->depth;
    }

    xattr.override_redirect = False;
    xattr.background_pixel = 0;
    xattr.border_pixel = 0;
    xattr.colormap = XCreateColormap(display, RootWindow(display, screen), visual, AllocNone);

    w = XCreateWindow(display, RootWindow(display, screen),
                      window->x, window->y, window->w, window->h,
                      0, depth, InputOutput, visual,
                      (CWOverrideRedirect | CWBackPixel | CWBorderPixel |
                       CWColormap), &xattr);
    if (!w) {
        SDL_SetError("Couldn't create window");
        return -1;
    }
#if SDL_VIDEO_DRIVER_PANDORA
    /* Create the GLES window surface */
    _this->gles_data->egl_surface =
        _this->gles_data->eglCreateWindowSurface(_this->gles_data->
                                                 egl_display,
                                                 _this->gles_data->egl_config,
                                                 (NativeWindowType) w, NULL);

    if (_this->gles_data->egl_surface == EGL_NO_SURFACE) {
        SDL_SetError("Could not create GLES window surface");
        return -1;
    }
#endif

    sizehints = XAllocSizeHints();
    if (sizehints) {
        if (!(window->flags & SDL_WINDOW_RESIZABLE)) {
            sizehints->min_width = sizehints->max_width = window->w;
            sizehints->min_height = sizehints->max_height = window->h;
            sizehints->flags = PMaxSize | PMinSize;
        }
        sizehints->x = window->x;
        sizehints->y = window->y;
        sizehints->flags |= USPosition;
        XSetWMNormalHints(display, w, sizehints);
        XFree(sizehints);
    }

    if (window->flags & SDL_WINDOW_BORDERLESS) {
        SDL_bool set;
        Atom WM_HINTS;

        /* We haven't modified the window manager hints yet */
        set = SDL_FALSE;

        /* First try to set MWM hints */
        WM_HINTS = XInternAtom(display, "_MOTIF_WM_HINTS", True);
        if (WM_HINTS != None) {
            /* Hints used by Motif compliant window managers */
            struct
            {
                unsigned long flags;
                unsigned long functions;
                unsigned long decorations;
                long input_mode;
                unsigned long status;
            } MWMHints = {
            (1L << 1), 0, 0, 0, 0};

            XChangeProperty(display, w, WM_HINTS, WM_HINTS, 32,
                            PropModeReplace, (unsigned char *) &MWMHints,
                            sizeof(MWMHints) / 4);
            set = SDL_TRUE;
        }
        /* Now try to set KWM hints */
        WM_HINTS = XInternAtom(display, "KWM_WIN_DECORATION", True);
        if (WM_HINTS != None) {
            long KWMHints = 0;

            XChangeProperty(display, w, WM_HINTS, WM_HINTS, 32,
                            PropModeReplace,
                            (unsigned char *) &KWMHints,
                            sizeof(KWMHints) / 4);
            set = SDL_TRUE;
        }
        /* Now try to set GNOME hints */
        WM_HINTS = XInternAtom(display, "_WIN_HINTS", True);
        if (WM_HINTS != None) {
            long GNOMEHints = 0;

            XChangeProperty(display, w, WM_HINTS, WM_HINTS, 32,
                            PropModeReplace,
                            (unsigned char *) &GNOMEHints,
                            sizeof(GNOMEHints) / 4);
            set = SDL_TRUE;
        }
        /* Finally set the transient hints if necessary */
        if (!set) {
            XSetTransientForHint(display, w, RootWindow(display, screen));
        }
    } else {
        SDL_bool set;
        Atom WM_HINTS;

        /* We haven't modified the window manager hints yet */
        set = SDL_FALSE;

        /* First try to unset MWM hints */
        WM_HINTS = XInternAtom(display, "_MOTIF_WM_HINTS", True);
        if (WM_HINTS != None) {
            XDeleteProperty(display, w, WM_HINTS);
            set = SDL_TRUE;
        }
        /* Now try to unset KWM hints */
        WM_HINTS = XInternAtom(display, "KWM_WIN_DECORATION", True);
        if (WM_HINTS != None) {
            XDeleteProperty(display, w, WM_HINTS);
            set = SDL_TRUE;
        }
        /* Now try to unset GNOME hints */
        WM_HINTS = XInternAtom(display, "_WIN_HINTS", True);
        if (WM_HINTS != None) {
            XDeleteProperty(display, w, WM_HINTS);
            set = SDL_TRUE;
        }
        /* Finally unset the transient hints if necessary */
        if (!set) {
            XDeleteProperty(display, w, XA_WM_TRANSIENT_FOR);
        }
    }

    /* Set the input hints so we get keyboard input */
    wmhints = XAllocWMHints();
    if (wmhints) {
        wmhints->input = True;
        wmhints->flags = InputHint;
        XSetWMHints(display, w, wmhints);
        XFree(wmhints);
    }

    /* Set the class hints so we can get an icon (AfterStep) */
    classhints = XAllocClassHint();
    if (classhints != NULL) {
        classhints->res_name = data->classname;
        classhints->res_class = data->classname;
        XSetClassHint(display, w, classhints);
        XFree(classhints);
    }

    /* Set the window manager state */
    wmstate_count = X11_GetWMStateProperty(_this, window, wmstate_atoms);
    if (wmstate_count > 0) {
        XChangeProperty(display, w, data->_NET_WM_STATE, XA_ATOM, 32,
                        PropModeReplace,
                        (unsigned char *)wmstate_atoms, wmstate_count);
    } else {
        XDeleteProperty(display, w, data->_NET_WM_STATE);
    }

    /* Let the window manager know we're a "normal" window */
    _NET_WM_WINDOW_TYPE = XInternAtom(display, "_NET_WM_WINDOW_TYPE", False);
    _NET_WM_WINDOW_TYPE_NORMAL = XInternAtom(display, "_NET_WM_WINDOW_TYPE_NORMAL", False);
    XChangeProperty(display, w, _NET_WM_WINDOW_TYPE, XA_ATOM, 32,
                    PropModeReplace,
                    (unsigned char *)&_NET_WM_WINDOW_TYPE_NORMAL, 1);

    /* Allow the window to be deleted by the window manager */
    XSetWMProtocols(display, w, &data->WM_DELETE_WINDOW, 1);

    if (SetupWindowData(_this, window, w, SDL_TRUE) < 0) {
        XDestroyWindow(display, w);
        return -1;
    }

#ifdef X_HAVE_UTF8_STRING
    {
        Uint32 fevent = 0;
        pXGetICValues(((SDL_WindowData *) window->driverdata)->ic,
                      XNFilterEvents, &fevent, NULL);
        XSelectInput(display, w,
                     (FocusChangeMask | EnterWindowMask | LeaveWindowMask |
                      ExposureMask | ButtonPressMask | ButtonReleaseMask |
                      PointerMotionMask | KeyPressMask | KeyReleaseMask |
                      PropertyChangeMask | StructureNotifyMask |
                      KeymapStateMask | fevent));
    }
#else
    {
        XSelectInput(display, w,
                     (FocusChangeMask | EnterWindowMask | LeaveWindowMask |
                      ExposureMask | ButtonPressMask | ButtonReleaseMask |
                      PointerMotionMask | KeyPressMask | KeyReleaseMask |
                      PropertyChangeMask | StructureNotifyMask |
                      KeymapStateMask));
    }
#endif

    XFlush(display);

    return 0;
}

int
X11_CreateWindowFrom(_THIS, SDL_Window * window, const void *data)
{
    Window w = (Window) data;

    window->title = X11_GetWindowTitle(_this, w);

    if (SetupWindowData(_this, window, w, SDL_FALSE) < 0) {
        return -1;
    }
    return 0;
}

char *
X11_GetWindowTitle(_THIS, Window xwindow)
{
    SDL_VideoData *data = (SDL_VideoData *) _this->driverdata;
    Display *display = data->display;
    int status, real_format;
    Atom real_type;
    unsigned long items_read, items_left;
    unsigned char *propdata;
    char *title = NULL;

    status = XGetWindowProperty(display, xwindow, data->_NET_WM_NAME,
                0L, 8192L, False, data->UTF8_STRING, &real_type, &real_format,
                &items_read, &items_left, &propdata);
    if (status == Success) {
        title = SDL_strdup(SDL_static_cast(char*, propdata));
        XFree(propdata);
    } else {
        status = XGetWindowProperty(display, xwindow, XA_WM_NAME,
                    0L, 8192L, False, XA_STRING, &real_type, &real_format,
                    &items_read, &items_left, &propdata);
        if (status == Success) {
            title = SDL_iconv_string("UTF-8", "", SDL_static_cast(char*, propdata), items_read+1);
        } else {
            title = SDL_strdup("");
        }
    }
    return title;
}

void
X11_SetWindowTitle(_THIS, SDL_Window * window)
{
    SDL_WindowData *data = (SDL_WindowData *) window->driverdata;
    Display *display = data->videodata->display;
    XTextProperty titleprop, iconprop;
    Status status;
    const char *title = window->title;
    const char *icon = NULL;

#ifdef X_HAVE_UTF8_STRING
    Atom _NET_WM_NAME = data->videodata->_NET_WM_NAME;
    Atom _NET_WM_ICON_NAME = data->videodata->_NET_WM_ICON_NAME;
#endif

    if (title != NULL) {
        char *title_locale = SDL_iconv_utf8_locale(title);
        if (!title_locale) {
            SDL_OutOfMemory();
            return;
        }
        status = XStringListToTextProperty(&title_locale, 1, &titleprop);
        SDL_free(title_locale);
        if (status) {
            XSetTextProperty(display, data->xwindow, &titleprop, XA_WM_NAME);
            XFree(titleprop.value);
        }
#ifdef X_HAVE_UTF8_STRING
        if (SDL_X11_HAVE_UTF8) {
            status =
                Xutf8TextListToTextProperty(display, (char **) &title, 1,
                                            XUTF8StringStyle, &titleprop);
            if (status == Success) {
                XSetTextProperty(display, data->xwindow, &titleprop,
                                 _NET_WM_NAME);
                XFree(titleprop.value);
            }
        }
#endif
    }
    if (icon != NULL) {
        char *icon_locale = SDL_iconv_utf8_locale(icon);
        if (!icon_locale) {
            SDL_OutOfMemory();
            return;
        }
        status = XStringListToTextProperty(&icon_locale, 1, &iconprop);
        SDL_free(icon_locale);
        if (status) {
            XSetTextProperty(display, data->xwindow, &iconprop,
                             XA_WM_ICON_NAME);
            XFree(iconprop.value);
        }
#ifdef X_HAVE_UTF8_STRING
        if (SDL_X11_HAVE_UTF8) {
            status =
                Xutf8TextListToTextProperty(display, (char **) &icon, 1,
                                            XUTF8StringStyle, &iconprop);
            if (status == Success) {
                XSetTextProperty(display, data->xwindow, &iconprop,
                                 _NET_WM_ICON_NAME);
                XFree(iconprop.value);
            }
        }
#endif
    }
    XFlush(display);
}

void
X11_SetWindowIcon(_THIS, SDL_Window * window, SDL_Surface * icon)
{
    SDL_WindowData *data = (SDL_WindowData *) window->driverdata;
    Display *display = data->videodata->display;
    Atom _NET_WM_ICON = data->videodata->_NET_WM_ICON;

    if (icon) {
        SDL_PixelFormat format;
        SDL_Surface *surface;
        int propsize;
        long *propdata;

        /* Convert the icon to ARGB for modern window managers */
        SDL_InitFormat(&format, SDL_PIXELFORMAT_ARGB8888);
        surface = SDL_ConvertSurface(icon, &format, 0);
        if (!surface) {
            return;
        }

        /* Set the _NET_WM_ICON property */
        propsize = 2 + (icon->w * icon->h);
        propdata = SDL_malloc(propsize * sizeof(long));
        if (propdata) {
            int x, y;
            Uint32 *src;
            long *dst;

            propdata[0] = icon->w;
            propdata[1] = icon->h;
            dst = &propdata[2];
            for (y = 0; y < icon->h; ++y) {
                src = (Uint32*)((Uint8*)surface->pixels + y * surface->pitch);
                for (x = 0; x < icon->w; ++x) {
                    *dst++ = *src++;
                }
            }
            XChangeProperty(display, data->xwindow, _NET_WM_ICON, XA_CARDINAL,
                            32, PropModeReplace, (unsigned char *) propdata,
                            propsize);
        }
        SDL_FreeSurface(surface);
    } else {
        XDeleteProperty(display, data->xwindow, _NET_WM_ICON);
    }
    XFlush(display);
}

void
X11_SetWindowPosition(_THIS, SDL_Window * window)
{
    SDL_WindowData *data = (SDL_WindowData *) window->driverdata;
    Display *display = data->videodata->display;
    SDL_bool oldstyle_fullscreen;
    int x, y;

    /* ICCCM2.0-compliant window managers can handle fullscreen windows */
    oldstyle_fullscreen = X11_IsWindowOldFullscreen(_this, window);

    if (oldstyle_fullscreen
        || SDL_WINDOWPOS_ISCENTERED(window->x)) {
        X11_GetDisplaySize(_this, window, &x, NULL);
        x = (x - window->w) / 2;
    } else {
        x = window->x;
    }
    if (oldstyle_fullscreen
        || SDL_WINDOWPOS_ISCENTERED(window->y)) {
        X11_GetDisplaySize(_this, window, NULL, &y);
        y = (y - window->h) / 2;
    } else {
        y = window->y;
    }
    XMoveWindow(display, data->xwindow, x, y);
    XFlush(display);
}

void
X11_SetWindowSize(_THIS, SDL_Window * window)
{
    SDL_WindowData *data = (SDL_WindowData *) window->driverdata;
    Display *display = data->videodata->display;

    if (SDL_IsShapedWindow(window))
        X11_ResizeWindowShape(window);
    XResizeWindow(display, data->xwindow, window->w, window->h);
    XFlush(display);
}

void
X11_ShowWindow(_THIS, SDL_Window * window)
{
    SDL_WindowData *data = (SDL_WindowData *) window->driverdata;
    Display *display = data->videodata->display;

    XMapRaised(display, data->xwindow);
    XFlush(display);
}

void
X11_HideWindow(_THIS, SDL_Window * window)
{
    SDL_WindowData *data = (SDL_WindowData *) window->driverdata;
    Display *display = data->videodata->display;

    XUnmapWindow(display, data->xwindow);
    XFlush(display);
}

void
X11_RaiseWindow(_THIS, SDL_Window * window)
{
    SDL_WindowData *data = (SDL_WindowData *) window->driverdata;
    Display *display = data->videodata->display;

    XRaiseWindow(display, data->xwindow);
    XFlush(display);
}

static void
SetWindowMaximized(_THIS, SDL_Window * window, SDL_bool maximized)
{
    SDL_WindowData *data = (SDL_WindowData *) window->driverdata;
    SDL_DisplayData *displaydata =
        (SDL_DisplayData *) SDL_GetDisplayForWindow(window)->driverdata;
    Display *display = data->videodata->display;
    Atom _NET_WM_STATE = data->videodata->_NET_WM_STATE;
    Atom _NET_WM_STATE_MAXIMIZED_VERT = data->videodata->_NET_WM_STATE_MAXIMIZED_VERT;
    Atom _NET_WM_STATE_MAXIMIZED_HORZ = data->videodata->_NET_WM_STATE_MAXIMIZED_HORZ;
    Atom _NET_WM_STATE_FULLSCREEN = data->videodata->_NET_WM_STATE_FULLSCREEN;

    if (X11_IsWindowMapped(_this, window)) {
        XEvent e;

        SDL_zero(e);
        e.xany.type = ClientMessage;
        e.xclient.message_type = _NET_WM_STATE;
        e.xclient.format = 32;
        e.xclient.window = data->xwindow;
        e.xclient.data.l[0] =
            maximized ? _NET_WM_STATE_ADD : _NET_WM_STATE_REMOVE;
        e.xclient.data.l[1] = _NET_WM_STATE_MAXIMIZED_VERT;
        e.xclient.data.l[2] = _NET_WM_STATE_MAXIMIZED_HORZ;
        e.xclient.data.l[3] = 0l;

        XSendEvent(display, RootWindow(display, displaydata->screen), 0,
                   SubstructureNotifyMask | SubstructureRedirectMask, &e);
    } else {
        int count = 0;
        Atom atoms[3];

        if (window->flags & SDL_WINDOW_FULLSCREEN) {
            atoms[count++] = _NET_WM_STATE_FULLSCREEN;
        }
        if (maximized) {
            atoms[count++] = _NET_WM_STATE_MAXIMIZED_VERT;
            atoms[count++] = _NET_WM_STATE_MAXIMIZED_HORZ;
        }
        if (count > 0) {
            XChangeProperty(display, data->xwindow, _NET_WM_STATE, XA_ATOM, 32,
                            PropModeReplace, (unsigned char *)atoms, count);
        } else {
            XDeleteProperty(display, data->xwindow, _NET_WM_STATE);
        }
    }
    XFlush(display);
}

void
X11_MaximizeWindow(_THIS, SDL_Window * window)
{
    SetWindowMaximized(_this, window, SDL_TRUE);
}

void
X11_MinimizeWindow(_THIS, SDL_Window * window)
{
    SDL_WindowData *data = (SDL_WindowData *) window->driverdata;
    SDL_DisplayData *displaydata =
        (SDL_DisplayData *) SDL_GetDisplayForWindow(window)->driverdata;
    Display *display = data->videodata->display;
 
    XIconifyWindow(display, data->xwindow, displaydata->screen);
    XFlush(display);
}

void
X11_RestoreWindow(_THIS, SDL_Window * window)
{
    SetWindowMaximized(_this, window, SDL_FALSE);
    X11_ShowWindow(_this, window);
}

void
X11_SetWindowFullscreen(_THIS, SDL_Window * window, SDL_VideoDisplay * _display, SDL_bool fullscreen)
{
    SDL_WindowData *data = (SDL_WindowData *) window->driverdata;
    SDL_DisplayData *displaydata = (SDL_DisplayData *) _display->driverdata;
    Display *display = data->videodata->display;
    Atom _NET_WM_STATE = data->videodata->_NET_WM_STATE;
    Atom _NET_WM_STATE_MAXIMIZED_VERT = data->videodata->_NET_WM_STATE_MAXIMIZED_VERT;
    Atom _NET_WM_STATE_MAXIMIZED_HORZ = data->videodata->_NET_WM_STATE_MAXIMIZED_HORZ;
    Atom _NET_WM_STATE_FULLSCREEN = data->videodata->_NET_WM_STATE_FULLSCREEN;

    if (X11_IsWindowMapped(_this, window)) {
        XEvent e;

        SDL_zero(e);
        e.xany.type = ClientMessage;
        e.xclient.message_type = _NET_WM_STATE;
        e.xclient.format = 32;
        e.xclient.window = data->xwindow;
        e.xclient.data.l[0] =
            fullscreen ? _NET_WM_STATE_ADD : _NET_WM_STATE_REMOVE;
        e.xclient.data.l[1] = _NET_WM_STATE_FULLSCREEN;
        e.xclient.data.l[3] = 0l;

        XSendEvent(display, RootWindow(display, displaydata->screen), 0,
                   SubstructureNotifyMask | SubstructureRedirectMask, &e);
    } else {
        int count = 0;
        Atom atoms[3];

        if (fullscreen) {
            atoms[count++] = _NET_WM_STATE_FULLSCREEN;
        }
        if (window->flags & SDL_WINDOW_MAXIMIZED) {
            atoms[count++] = _NET_WM_STATE_MAXIMIZED_VERT;
            atoms[count++] = _NET_WM_STATE_MAXIMIZED_HORZ;
        }
        if (count > 0) {
            XChangeProperty(display, data->xwindow, _NET_WM_STATE, XA_ATOM, 32,
                            PropModeReplace, (unsigned char *)atoms, count);
        } else {
            XDeleteProperty(display, data->xwindow, _NET_WM_STATE);
        }
    }
    XFlush(display);
}

void
X11_SetWindowGrab(_THIS, SDL_Window * window)
{
    SDL_WindowData *data = (SDL_WindowData *) window->driverdata;
    Display *display = data->videodata->display;
    SDL_bool oldstyle_fullscreen;

    /* ICCCM2.0-compliant window managers can handle fullscreen windows */
    oldstyle_fullscreen = X11_IsWindowOldFullscreen(_this, window);

    if (((window->flags & SDL_WINDOW_INPUT_GRABBED) || oldstyle_fullscreen)
        && (window->flags & SDL_WINDOW_INPUT_FOCUS)) {
        /* Try to grab the mouse */
        for (;;) {
            int result =
                XGrabPointer(display, data->xwindow, True, 0, GrabModeAsync,
                             GrabModeAsync, data->xwindow, None, CurrentTime);
            if (result == GrabSuccess) {
                break;
            }
            SDL_Delay(100);
        }

        /* Raise the window if we grab the mouse */
        XRaiseWindow(display, data->xwindow);

        /* Now grab the keyboard */
        XGrabKeyboard(display, data->xwindow, True, GrabModeAsync,
                      GrabModeAsync, CurrentTime);
    } else {
        XUngrabPointer(display, CurrentTime);
        XUngrabKeyboard(display, CurrentTime);
    }
}

void
X11_DestroyWindow(_THIS, SDL_Window * window)
{
    SDL_WindowData *data = (SDL_WindowData *) window->driverdata;
    window->driverdata = NULL;

    if (data) {
        SDL_VideoData *videodata = (SDL_VideoData *) data->videodata;
        Display *display = videodata->display;
        int numwindows = videodata->numwindows;
        SDL_WindowData **windowlist = videodata->windowlist;
        int i;

        if (windowlist) {
            for (i = 0; i < numwindows; ++i) {
                if (windowlist[i] && (windowlist[i]->window == window)) {
                    windowlist[i] = windowlist[numwindows - 1];
                    windowlist[numwindows - 1] = NULL;
                    videodata->numwindows--;
                    break;
                }
            }
        }
#ifdef X_HAVE_UTF8_STRING
        if (data->ic) {
            XDestroyIC(data->ic);
        }
#endif
        if (data->created) {
            XDestroyWindow(display, data->xwindow);
            XFlush(display);
        }
        SDL_free(data);
    }
}

SDL_bool
X11_GetWindowWMInfo(_THIS, SDL_Window * window, SDL_SysWMinfo * info)
{
    SDL_WindowData *data = (SDL_WindowData *) window->driverdata;
    Display *display = data->videodata->display;

    if (info->version.major == SDL_MAJOR_VERSION &&
        info->version.minor == SDL_MINOR_VERSION) {
        info->subsystem = SDL_SYSWM_X11;
        info->info.x11.display = display;
        info->info.x11.window = data->xwindow;
        return SDL_TRUE;
    } else {
        SDL_SetError("Application not compiled with SDL %d.%d\n",
                     SDL_MAJOR_VERSION, SDL_MINOR_VERSION);
        return SDL_FALSE;
    }
}

/* vi: set ts=4 sw=4 expandtab: */
