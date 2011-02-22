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

/* This is the system specific header for the SDL joystick API */

#include "SDL_joystick.h"
#include "../SDL_sysjoystick.h"
#include "../SDL_joystick_c.h"
#import "SDLUIAccelerationDelegate.h"

const char *accelerometerName = "iPhone accelerometer";

/* Function to scan the system for joysticks.
 * This function should set SDL_numjoysticks to the number of available
 * joysticks.  Joystick 0 should be the system default joystick.
 * It should return 0, or -1 on an unrecoverable fatal error.
 */
int
SDL_SYS_JoystickInit(void)
{
    SDL_numjoysticks = 1;
    return (1);
}

/* Function to get the device-dependent name of a joystick */
const char *
SDL_SYS_JoystickName(int index)
{
	switch(index) {
		case 0:
			return accelerometerName;
		default:
			SDL_SetError("No joystick available with that index");
			return NULL;
	}
}

/* Function to open a joystick for use.
   The joystick to open is specified by the index field of the joystick.
   This should fill the nbuttons and naxes fields of the joystick structure.
   It returns 0, or -1 if there is an error.
 */
int
SDL_SYS_JoystickOpen(SDL_Joystick * joystick)
{
	if (joystick->index == 0) {
		joystick->naxes = 3;
		joystick->nhats = 0;
		joystick->nballs = 0;
		joystick->nbuttons = 0;
		joystick->name  = accelerometerName;
		[[SDLUIAccelerationDelegate sharedDelegate] startup];
		return 0;
	}
	else {
		SDL_SetError("No joystick available with that index");
		return (-1);
	}
	
}

/* Function to update the state of a joystick - called as a device poll.
 * This function shouldn't update the joystick structure directly,
 * but instead should call SDL_PrivateJoystick*() to deliver events
 * and update joystick device state.
 */
void
SDL_SYS_JoystickUpdate(SDL_Joystick * joystick)
{
	
	Sint16 orientation[3];
	
	if ([[SDLUIAccelerationDelegate sharedDelegate] hasNewData]) {
	
		[[SDLUIAccelerationDelegate sharedDelegate] getLastOrientation: orientation];
		[[SDLUIAccelerationDelegate sharedDelegate] setHasNewData: NO];
		
		SDL_PrivateJoystickAxis(joystick, 0, orientation[0]);
		SDL_PrivateJoystickAxis(joystick, 1, orientation[1]);
		SDL_PrivateJoystickAxis(joystick, 2, orientation[2]);

	}
	
    return;
}

/* Function to close a joystick after use */
void
SDL_SYS_JoystickClose(SDL_Joystick * joystick)
{
	if (joystick->index == 0 && [[SDLUIAccelerationDelegate sharedDelegate] isRunning]) {
		[[SDLUIAccelerationDelegate sharedDelegate] shutdown];
	}
	SDL_SetError("No joystick open with that index");

    return;
}

/* Function to perform any system-specific joystick related cleanup */
void
SDL_SYS_JoystickQuit(void)
{
    return;
}
/* vi: set ts=4 sw=4 expandtab: */
