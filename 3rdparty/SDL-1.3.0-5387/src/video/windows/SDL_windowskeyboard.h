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

#ifndef _SDL_windowskeyboard_h
#define _SDL_windowskeyboard_h

extern BYTE alpha_scancodes[26];
extern BYTE keypad_scancodes[10];

extern void WIN_InitKeyboard(_THIS);
extern void WIN_UpdateKeymap(void);
extern void WIN_QuitKeyboard(_THIS);

extern void WIN_StartTextInput(_THIS);
extern void WIN_StopTextInput(_THIS);
extern void WIN_SetTextInputRect(_THIS, SDL_Rect *rect);

extern SDL_bool IME_HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM *lParam, struct SDL_VideoData *videodata);

#endif /* _SDL_windowskeyboard_h */

/* vi: set ts=4 sw=4 expandtab: */
