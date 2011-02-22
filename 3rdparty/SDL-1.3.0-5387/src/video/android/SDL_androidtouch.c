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

#include <android/log.h>

#include "SDL_events.h"
#include "../../events/SDL_mouse_c.h"

#include "SDL_androidtouch.h"


#define ACTION_DOWN 0
#define ACTION_UP 1
#define ACTION_MOVE 2
#define ACTION_CANCEL 3
#define ACTION_OUTSIDE 4

void Android_OnTouch(int action, float x, float y, float p)
{
    if (!Android_Window) {
        return;
    }

    if ((action != ACTION_CANCEL) && (action != ACTION_OUTSIDE)) {
        SDL_SetMouseFocus(Android_Window);
        SDL_SendMouseMotion(Android_Window, 0, (int)x, (int)y);
        switch(action) {
        case ACTION_DOWN:
            SDL_SendMouseButton(Android_Window, SDL_PRESSED, SDL_BUTTON_LEFT);
            break;
        case ACTION_UP:
            SDL_SendMouseButton(Android_Window, SDL_RELEASED, SDL_BUTTON_LEFT);
            break;
        }
    } else {
        SDL_SetMouseFocus(NULL);
    }
}

/* vi: set ts=4 sw=4 expandtab: */
