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

/* Clipboard event handling code for SDL */

#include "SDL_events.h"
#include "SDL_events_c.h"
#include "SDL_clipboardevents_c.h"


int
SDL_SendClipboardUpdate(void)
{
    int posted;

    /* Post the event, if desired */
    posted = 0;
    if (SDL_GetEventState(SDL_CLIPBOARDUPDATE) == SDL_ENABLE) {
        SDL_Event event;
        event.type = SDL_CLIPBOARDUPDATE;

        posted = (SDL_PushEvent(&event) > 0);
    }
    return (posted);
}

/* vi: set ts=4 sw=4 expandtab: */
