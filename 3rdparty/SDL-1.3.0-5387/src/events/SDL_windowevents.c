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

/* Window event handling code for SDL */

#include "SDL_events.h"
#include "SDL_events_c.h"
#include "SDL_mouse_c.h"
#include "../video/SDL_sysvideo.h"


static int
RemovePendingSizeEvents(void * userdata, SDL_Event *event)
{
    SDL_Event *new_event = (SDL_Event *)userdata;

    if (event->type == SDL_WINDOWEVENT &&
        (event->window.event == SDL_WINDOWEVENT_RESIZED ||
         event->window.event == SDL_WINDOWEVENT_SIZE_CHANGED) &&
        event->window.windowID == new_event->window.windowID) {
        /* We're about to post a new size event, drop the old one */
        return 0;
    }
    return 1;
}

static int
RemovePendingMoveEvents(void * userdata, SDL_Event *event)
{
    SDL_Event *new_event = (SDL_Event *)userdata;

    if (event->type == SDL_WINDOWEVENT &&
        event->window.event == SDL_WINDOWEVENT_MOVED &&
        event->window.windowID == new_event->window.windowID) {
        /* We're about to post a new move event, drop the old one */
        return 0;
    }
    return 1;
}

int
SDL_SendWindowEvent(SDL_Window * window, Uint8 windowevent, int data1,
                    int data2)
{
    int posted;

    if (!window) {
        return 0;
    }
    switch (windowevent) {
    case SDL_WINDOWEVENT_SHOWN:
        if (window->flags & SDL_WINDOW_SHOWN) {
            return 0;
        }
        window->flags &= ~SDL_WINDOW_HIDDEN;
        window->flags |= SDL_WINDOW_SHOWN;
        SDL_OnWindowShown(window);
        break;
    case SDL_WINDOWEVENT_HIDDEN:
        if (!(window->flags & SDL_WINDOW_SHOWN)) {
            return 0;
        }
        window->flags &= ~SDL_WINDOW_SHOWN;
        window->flags |= SDL_WINDOW_HIDDEN;
        SDL_OnWindowHidden(window);
        break;
    case SDL_WINDOWEVENT_MOVED:
        if (SDL_WINDOWPOS_ISUNDEFINED(data1) ||
            SDL_WINDOWPOS_ISUNDEFINED(data2)) {
            return 0;
        }
        if (data1 == window->x && data2 == window->y) {
            return 0;
        }
        window->x = data1;
        window->y = data2;
        break;
    case SDL_WINDOWEVENT_RESIZED:
        if (data1 == window->w && data2 == window->h) {
            return 0;
        }
        window->w = data1;
        window->h = data2;
        SDL_OnWindowResized(window);
        break;
    case SDL_WINDOWEVENT_MINIMIZED:
        if (window->flags & SDL_WINDOW_MINIMIZED) {
            return 0;
        }
        window->flags |= SDL_WINDOW_MINIMIZED;
        SDL_OnWindowMinimized(window);
        break;
    case SDL_WINDOWEVENT_MAXIMIZED:
        if (window->flags & SDL_WINDOW_MAXIMIZED) {
            return 0;
        }
        window->flags |= SDL_WINDOW_MAXIMIZED;
        break;
    case SDL_WINDOWEVENT_RESTORED:
        if (!(window->flags & (SDL_WINDOW_MINIMIZED | SDL_WINDOW_MAXIMIZED))) {
            return 0;
        }
        window->flags &= ~(SDL_WINDOW_MINIMIZED | SDL_WINDOW_MAXIMIZED);
        SDL_OnWindowRestored(window);
        break;
    case SDL_WINDOWEVENT_ENTER:
        if (window->flags & SDL_WINDOW_MOUSE_FOCUS) {
            return 0;
        }
        window->flags |= SDL_WINDOW_MOUSE_FOCUS;
        break;
    case SDL_WINDOWEVENT_LEAVE:
        if (!(window->flags & SDL_WINDOW_MOUSE_FOCUS)) {
            return 0;
        }
        window->flags &= ~SDL_WINDOW_MOUSE_FOCUS;
        break;
    case SDL_WINDOWEVENT_FOCUS_GAINED:
        if (window->flags & SDL_WINDOW_INPUT_FOCUS) {
            return 0;
        }
        window->flags |= SDL_WINDOW_INPUT_FOCUS;
        SDL_OnWindowFocusGained(window);
        break;
    case SDL_WINDOWEVENT_FOCUS_LOST:
        if (!(window->flags & SDL_WINDOW_INPUT_FOCUS)) {
            return 0;
        }
        window->flags &= ~SDL_WINDOW_INPUT_FOCUS;
        SDL_OnWindowFocusLost(window);
        break;
    }

    /* Post the event, if desired */
    posted = 0;
    if (SDL_GetEventState(SDL_WINDOWEVENT) == SDL_ENABLE) {
        SDL_Event event;
        event.type = SDL_WINDOWEVENT;
        event.window.event = windowevent;
        event.window.data1 = data1;
        event.window.data2 = data2;
        event.window.windowID = window->id;

        /* Fixes queue overflow with resize events that aren't processed */
        if (windowevent == SDL_WINDOWEVENT_RESIZED) {
            SDL_FilterEvents(RemovePendingSizeEvents, &event);
        }
        if (windowevent == SDL_WINDOWEVENT_MOVED) {
            SDL_FilterEvents(RemovePendingMoveEvents, &event);
        }

        posted = (SDL_PushEvent(&event) > 0);
    }
	
	if (windowevent == SDL_WINDOWEVENT_CLOSE) {
		if ( !window->prev && !window->next ) {
			// This is the last window in the list so send the SDL_QUIT event
			SDL_SendQuit();
		}
	}

    return (posted);
}

/* vi: set ts=4 sw=4 expandtab: */
