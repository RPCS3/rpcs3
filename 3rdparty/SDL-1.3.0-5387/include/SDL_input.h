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
 *  \file SDL_input.h
 *  
 *  Include file for lowlevel SDL input device handling.
 *
 *  This talks about individual devices, and not the system cursor. If you
 *  just want to know when the user moves the pointer somewhere in your
 *  window, this is NOT the API you want. This one handles things like
 *  multi-touch, drawing tablets, and multiple, separate mice.
 *
 *  The other API is in SDL_mouse.h
 */

#ifndef _SDL_input_h
#define _SDL_input_h

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


/* Function prototypes */

/* !!! FIXME: real documentation
 * - Redetect devices
 * - This invalidates all existing device information from previous queries!
 * - There is an implicit (re)detect upon SDL_Init().
 */
extern DECLSPEC int SDLCALL SDL_RedetectInputDevices(void);

/**
 *  \brief Get the number of mouse input devices available.
 */
extern DECLSPEC int SDLCALL SDL_GetNumInputDevices(void);

/**
 *  \brief Gets the name of a device with the given index.
 *  
 *  \param index is the index of the device, whose name is to be returned.
 *  
 *  \return the name of the device with the specified index
 */
extern DECLSPEC const char *SDLCALL SDL_GetInputDeviceName(int index);


extern DECLSPEC int SDLCALL SDL_IsDeviceDisconnected(int index);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
/* *INDENT-OFF* */
}
/* *INDENT-ON* */
#endif
#include "close_code.h"

#endif /* _SDL_mouse_h */

/* vi: set ts=4 sw=4 expandtab: */
