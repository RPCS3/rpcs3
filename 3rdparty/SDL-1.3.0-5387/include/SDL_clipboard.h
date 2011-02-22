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
 * \file SDL_clipboard.h
 *
 * Include file for SDL clipboard handling
 */

#ifndef _SDL_clipboard_h
#define _SDL_clipboard_h

#include "SDL_stdinc.h"

#include "begin_code.h"
/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
/* *INDENT-OFF* */
extern "C" {
/* *INDENT-ON* */
#endif

/* Function prototypes */

/**
 * \brief Put UTF-8 text into the clipboard
 *
 * \sa SDL_GetClipboardText()
 */
extern DECLSPEC int SDLCALL SDL_SetClipboardText(const char *text);

/**
 * \brief Get UTF-8 text from the clipboard, which must be freed with SDL_free()
 *
 * \sa SDL_SetClipboardText()
 */
extern DECLSPEC char * SDLCALL SDL_GetClipboardText(void);

/**
 * \brief Returns whether the clipboard has text
 *
 * \sa SDL_GetClipboardText()
 */
extern DECLSPEC SDL_bool SDLCALL SDL_HasClipboardText(void);


/* Ends C function definitions when using C++ */
#ifdef __cplusplus
/* *INDENT-OFF* */
}
/* *INDENT-ON* */
#endif
#include "close_code.h"

#endif /* _SDL_clipboard_h */

/* vi: set ts=4 sw=4 expandtab: */
