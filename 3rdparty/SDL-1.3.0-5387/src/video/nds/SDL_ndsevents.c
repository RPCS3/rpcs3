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

/* Being a null driver, there's no event stream. We just define stubs for
   most of the API. */

#include <stdio.h>
#include <stdlib.h>
#include <nds.h>

#include "../../events/SDL_events_c.h"

#include "SDL_ndsvideo.h"
#include "SDL_ndsevents_c.h"

void
NDS_PumpEvents(_THIS)
{
    scanKeys();
    /* TODO: defer click-age */
    if (keysDown() & KEY_TOUCH) {
        SDL_SendMouseButton(0, SDL_PRESSED, 0);
    } else if (keysUp() & KEY_TOUCH) {
        SDL_SendMouseButton(0, SDL_RELEASED, 0);
    }
    if (keysHeld() & KEY_TOUCH) {
        touchPosition t = touchReadXY();
        SDL_SendMouseMotion(0, 0, t.px, t.py);
    }
}

/* vi: set ts=4 sw=4 expandtab: */
