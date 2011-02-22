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

#include <limits.h> /* For INT_MAX */

#include "SDL_events.h"
#include "SDL_x11video.h"


/* If you don't support UTF-8, you might use XA_STRING here */
#ifdef X_HAVE_UTF8_STRING
#define TEXT_FORMAT XInternAtom(display, "UTF8_STRING", False)
#else
#define TEXT_FORMAT XA_STRING
#endif

/* Get any application owned window handle for clipboard association */
static Window
GetWindow(_THIS)
{
    SDL_Window *window;

    window = _this->windows;
    if (window) {
        return ((SDL_WindowData *) window->driverdata)->xwindow;
    }
    return None;
}

int
X11_SetClipboardText(_THIS, const char *text)
{
    Display *display = ((SDL_VideoData *) _this->driverdata)->display;
    Atom format;
    Window window;

    /* Get the SDL window that will own the selection */
    window = GetWindow(_this);
    if (window == None) {
        SDL_SetError("Couldn't find a window to own the selection");
        return -1;
    }

    /* Save the selection on the root window */
    format = TEXT_FORMAT;
    XChangeProperty(display, DefaultRootWindow(display),
        XA_CUT_BUFFER0, format, 8, PropModeReplace,
        (const unsigned char *)text, SDL_strlen(text));

    if (XGetSelectionOwner(display, XA_PRIMARY) != window) {
        XSetSelectionOwner(display, XA_PRIMARY, window, CurrentTime);
    }
    return 0;
}

char *
X11_GetClipboardText(_THIS)
{
    SDL_VideoData *videodata = (SDL_VideoData *) _this->driverdata;
    Display *display = videodata->display;
    Atom format;
    Window window;
    Window owner;
    Atom selection;
    Atom seln_type;
    int seln_format;
    unsigned long nbytes;
    unsigned long overflow;
    unsigned char *src;
    char *text;

    text = NULL;

    /* Get the window that holds the selection */
    window = GetWindow(_this);
    format = TEXT_FORMAT;
    owner = XGetSelectionOwner(display, XA_PRIMARY);
    if ((owner == None) || (owner == window)) {
        owner = DefaultRootWindow(display);
        selection = XA_CUT_BUFFER0;
    } else {
        /* Request that the selection owner copy the data to our window */
        owner = window;
        selection = XInternAtom(display, "SDL_SELECTION", False);
        XConvertSelection(display, XA_PRIMARY, format, selection, owner,
            CurrentTime);

        /* FIXME: Should we have a timeout here? */
        videodata->selection_waiting = SDL_TRUE;
        while (videodata->selection_waiting) {
            SDL_PumpEvents();
        }
    }

    if (XGetWindowProperty(display, owner, selection, 0, INT_MAX/4, False,
            format, &seln_type, &seln_format, &nbytes, &overflow, &src)
            == Success) {
        if (seln_type == format) {
            text = (char *)SDL_malloc(nbytes+1);
            if (text) {
                SDL_memcpy(text, src, nbytes);
                text[nbytes] = '\0';
            }
        }
        XFree(src);
    }

    if (!text) {
        text = SDL_strdup("");
    }
    return text;
}

SDL_bool
X11_HasClipboardText(_THIS)
{
    /* Not an easy way to tell with X11, as far as I know... */
    char *text;
    SDL_bool retval;

    text = X11_GetClipboardText(_this);
    if (*text) {
        retval = SDL_TRUE;
    } else {
        retval = SDL_FALSE;
    }
    SDL_free(text);

    return retval;
}

/* vi: set ts=4 sw=4 expandtab: */
