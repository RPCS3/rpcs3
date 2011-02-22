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

#ifndef _SDL_mouse_c_h
#define _SDL_mouse_c_h

#include "SDL_mouse.h"

struct SDL_Cursor
{
    struct SDL_Cursor *next;
    void *driverdata;
};

typedef struct
{
    /* Create a cursor from a surface */
    SDL_Cursor *(*CreateCursor) (SDL_Surface * surface, int hot_x, int hot_y);

    /* Show the specified cursor, or hide if cursor is NULL */
    int (*ShowCursor) (SDL_Cursor * cursor);

    /* This is called when a mouse motion event occurs */
    void (*MoveCursor) (SDL_Cursor * cursor);

    /* Free a window manager cursor */
    void (*FreeCursor) (SDL_Cursor * cursor);

    /* Warp the mouse to (x,y) */
    void (*WarpMouse) (SDL_Window * window, int x, int y);

    /* Data common to all mice */
    SDL_Window *focus;
    int x;
    int y;
    int xdelta;
    int ydelta;
    int last_x, last_y;         /* the last reported x and y coordinates */
    Uint8 buttonstate;
    SDL_bool relative_mode;

    SDL_Cursor *cursors;
    SDL_Cursor *def_cursor;
    SDL_Cursor *cur_cursor;
    SDL_bool cursor_shown;
} SDL_Mouse;


/* Initialize the mouse subsystem */
extern int SDL_MouseInit(void);

/* Get the mouse state structure */
SDL_Mouse *SDL_GetMouse(void);

/* Set the mouse focus window */
extern void SDL_SetMouseFocus(SDL_Window * window);

/* Send a mouse motion event */
extern int SDL_SendMouseMotion(SDL_Window * window, int relative, int x, int y);

/* Send a mouse button event */
extern int SDL_SendMouseButton(SDL_Window * window, Uint8 state, Uint8 button);

/* Send a mouse wheel event */
extern int SDL_SendMouseWheel(SDL_Window * window, int x, int y);

/* Shutdown the mouse subsystem */
extern void SDL_MouseQuit(void);

#endif /* _SDL_mouse_c_h */

/* vi: set ts=4 sw=4 expandtab: */
