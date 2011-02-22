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

#ifndef _SDL_keyboard_c_h
#define _SDL_keyboard_c_h

#include "SDL_keycode.h"
#include "SDL_events.h"

/* Initialize the keyboard subsystem */
extern int SDL_KeyboardInit(void);

/* Clear the state of the keyboard */
extern void SDL_ResetKeyboard(void);

/* Get the default keymap */
extern void SDL_GetDefaultKeymap(SDL_Keycode * keymap);

/* Set the mapping of scancode to key codes */
extern void SDL_SetKeymap(int start, SDL_Keycode * keys, int length);

/* Set a platform-dependent key name, overriding the default platform-agnostic
   name. Encoded as UTF-8. The string is not copied, thus the pointer given to
   this function must stay valid forever (or at least until the call to
   VideoQuit()). */
extern void SDL_SetScancodeName(SDL_Scancode scancode, const char *name);

/* Set the keyboard focus window */
extern void SDL_SetKeyboardFocus(SDL_Window * window);

/* Send a keyboard key event */
extern int SDL_SendKeyboardKey(Uint8 state, SDL_Scancode scancode);

/* Send keyboard text input */
extern int SDL_SendKeyboardText(const char *text);

/* Send editing text for selected range from start to end */
extern int SDL_SendEditingText(const char *text, int start, int end);

/* Shutdown the keyboard subsystem */
extern void SDL_KeyboardQuit(void);

#endif /* _SDL_keyboard_c_h */

/* vi: set ts=4 sw=4 expandtab: */
