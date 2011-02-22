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

/**
 *  \file SDL_mouse.h
 *  
 *  Include file for SDL mouse event handling.
 *
 *  Please note that this ONLY discusses "mice" with the notion of the
 *  desktop GUI. You (usually) have one system cursor, and the OS hides
 *  the hardware details from you. If you plug in 10 mice, all ten move that
 *  one cursor. For many applications and games, this is perfect, and this
 *  API has served hundreds of SDL programs well since its birth.
 *
 *  It's not the whole picture, though. If you want more lowlevel control,
 *  SDL offers a different API, that gives you visibility into each input
 *  device, multi-touch interfaces, etc.
 *
 *  Those two APIs are incompatible, and you usually should not use both
 *  at the same time. But for legacy purposes, this API refers to a "mouse"
 *  when it actually means the system pointer and not a physical mouse.
 *
 *  The other API is in SDL_input.h
 */

#ifndef _SDL_mouse_h
#define _SDL_mouse_h

#include "SDL_stdinc.h"
#include "SDL_error.h"
#include "SDL_video.h"

#include "begin_code.h"
/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
/* *INDENT-OFF* */
extern "C" {
/* *INDENT-ON* */
#endif

typedef struct SDL_Cursor SDL_Cursor;   /* Implementation dependent */


/* Function prototypes */

/**
 *  \brief Get the window which currently has mouse focus.
 */
extern DECLSPEC SDL_Window * SDLCALL SDL_GetMouseFocus(void);

/**
 *  \brief Retrieve the current state of the mouse.
 *  
 *  The current button state is returned as a button bitmask, which can
 *  be tested using the SDL_BUTTON(X) macros, and x and y are set to the
 *  mouse cursor position relative to the focus window for the currently
 *  selected mouse.  You can pass NULL for either x or y.
 */
extern DECLSPEC Uint8 SDLCALL SDL_GetMouseState(int *x, int *y);

/**
 *  \brief Retrieve the relative state of the mouse.
 *
 *  The current button state is returned as a button bitmask, which can
 *  be tested using the SDL_BUTTON(X) macros, and x and y are set to the
 *  mouse deltas since the last call to SDL_GetRelativeMouseState().
 */
extern DECLSPEC Uint8 SDLCALL SDL_GetRelativeMouseState(int *x, int *y);

/**
 *  \brief Moves the mouse to the given position within the window.
 *  
 *  \param window The window to move the mouse into, or NULL for the current mouse focus
 *  \param x The x coordinate within the window
 *  \param y The y coordinate within the window
 *  
 *  \note This function generates a mouse motion event
 */
extern DECLSPEC void SDLCALL SDL_WarpMouseInWindow(SDL_Window * window,
                                                   int x, int y);

/**
 *  \brief Set relative mouse mode.
 *  
 *  \param enabled Whether or not to enable relative mode
 *
 *  \return 0 on success, or -1 if relative mode is not supported.
 *  
 *  While the mouse is in relative mode, the cursor is hidden, and the
 *  driver will try to report continuous motion in the current window.
 *  Only relative motion events will be delivered, the mouse position
 *  will not change.
 *  
 *  \note This function will flush any pending mouse motion.
 *  
 *  \sa SDL_GetRelativeMouseMode()
 */
extern DECLSPEC int SDLCALL SDL_SetRelativeMouseMode(SDL_bool enabled);

/**
 *  \brief Query whether relative mouse mode is enabled.
 *  
 *  \sa SDL_SetRelativeMouseMode()
 */
extern DECLSPEC SDL_bool SDLCALL SDL_GetRelativeMouseMode(void);

/**
 *  \brief Create a cursor, using the specified bitmap data and
 *         mask (in MSB format).
 *  
 *  The cursor width must be a multiple of 8 bits.
 *  
 *  The cursor is created in black and white according to the following:
 *  <table>
 *  <tr><td> data </td><td> mask </td><td> resulting pixel on screen </td></tr>
 *  <tr><td>  0   </td><td>  1   </td><td> White </td></tr>
 *  <tr><td>  1   </td><td>  1   </td><td> Black </td></tr>
 *  <tr><td>  0   </td><td>  0   </td><td> Transparent </td></tr>
 *  <tr><td>  1   </td><td>  0   </td><td> Inverted color if possible, black 
 *                                         if not. </td></tr>
 *  </table>
 *  
 *  \sa SDL_FreeCursor()
 */
extern DECLSPEC SDL_Cursor *SDLCALL SDL_CreateCursor(const Uint8 * data,
                                                     const Uint8 * mask,
                                                     int w, int h, int hot_x,
                                                     int hot_y);

/**
 *  \brief Set the active cursor.
 */
extern DECLSPEC void SDLCALL SDL_SetCursor(SDL_Cursor * cursor);

/**
 *  \brief Return the active cursor.
 */
extern DECLSPEC SDL_Cursor *SDLCALL SDL_GetCursor(void);

/**
 *  \brief Frees a cursor created with SDL_CreateCursor().
 *  
 *  \sa SDL_CreateCursor()
 */
extern DECLSPEC void SDLCALL SDL_FreeCursor(SDL_Cursor * cursor);

/**
 *  \brief Toggle whether or not the cursor is shown.
 *  
 *  \param toggle 1 to show the cursor, 0 to hide it, -1 to query the current 
 *                state.
 *  
 *  \return 1 if the cursor is shown, or 0 if the cursor is hidden.
 */
extern DECLSPEC int SDLCALL SDL_ShowCursor(int toggle);

/**
 *  Used as a mask when testing buttons in buttonstate.
 *   - Button 1:  Left mouse button
 *   - Button 2:  Middle mouse button
 *   - Button 3:  Right mouse button
 */
#define SDL_BUTTON(X)		(1 << ((X)-1))
#define SDL_BUTTON_LEFT		1
#define SDL_BUTTON_MIDDLE	2
#define SDL_BUTTON_RIGHT	3
#define SDL_BUTTON_X1		4
#define SDL_BUTTON_X2		5
#define SDL_BUTTON_LMASK	SDL_BUTTON(SDL_BUTTON_LEFT)
#define SDL_BUTTON_MMASK	SDL_BUTTON(SDL_BUTTON_MIDDLE)
#define SDL_BUTTON_RMASK	SDL_BUTTON(SDL_BUTTON_RIGHT)
#define SDL_BUTTON_X1MASK	SDL_BUTTON(SDL_BUTTON_X1)
#define SDL_BUTTON_X2MASK	SDL_BUTTON(SDL_BUTTON_X2)


/* Ends C function definitions when using C++ */
#ifdef __cplusplus
/* *INDENT-OFF* */
}
/* *INDENT-ON* */
#endif
#include "close_code.h"

#endif /* _SDL_mouse_h */

/* vi: set ts=4 sw=4 expandtab: */
