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

#ifndef _SDL_power_h
#define _SDL_power_h

/**
 *  \file SDL_power.h
 *  
 *  Header for the SDL power management routines.
 */

#include "SDL_stdinc.h"

#include "begin_code.h"
/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
/* *INDENT-OFF* */
extern "C" {
/* *INDENT-ON* */
#endif

/**
 *  \brief The basic state for the system's power supply.
 */
typedef enum
{
    SDL_POWERSTATE_UNKNOWN,      /**< cannot determine power status */
    SDL_POWERSTATE_ON_BATTERY,   /**< Not plugged in, running on the battery */
    SDL_POWERSTATE_NO_BATTERY,   /**< Plugged in, no battery available */
    SDL_POWERSTATE_CHARGING,     /**< Plugged in, charging battery */
    SDL_POWERSTATE_CHARGED       /**< Plugged in, battery charged */
} SDL_PowerState;


/**
 *  \brief Get the current power supply details.
 *  
 *  \param secs Seconds of battery life left. You can pass a NULL here if
 *              you don't care. Will return -1 if we can't determine a
 *              value, or we're not running on a battery.
 *  
 *  \param pct Percentage of battery life left, between 0 and 100. You can
 *             pass a NULL here if you don't care. Will return -1 if we
 *             can't determine a value, or we're not running on a battery.
 *  
 *  \return The state of the battery (if any).
 */
extern DECLSPEC SDL_PowerState SDLCALL SDL_GetPowerInfo(int *secs, int *pct);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
/* *INDENT-OFF* */
}
/* *INDENT-ON* */
#endif
#include "close_code.h"

#endif /* _SDL_power_h */

/* vi: set ts=4 sw=4 expandtab: */
