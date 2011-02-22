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

#ifndef SDL_JOYSTICK_DINPUT_H

/* DirectInput joystick driver; written by Glenn Maynard, based on Andrei de
 * A. Formiga's WINMM driver. 
 *
 * Hats and sliders are completely untested; the app I'm writing this for mostly
 * doesn't use them and I don't own any joysticks with them. 
 *
 * We don't bother to use event notification here.  It doesn't seem to work
 * with polled devices, and it's fine to call IDirectInputDevice2_GetDeviceData and
 * let it return 0 events. */

#include "../../core/windows/SDL_windows.h"

#define DIRECTINPUT_VERSION 0x0700      /* Need version 7 for force feedback. */
#include <dinput.h>


#define MAX_INPUTS	256     /* each joystick can have up to 256 inputs */


/* local types */
typedef enum Type
{ BUTTON, AXIS, HAT } Type;

typedef struct input_t
{
    /* DirectInput offset for this input type: */
    DWORD ofs;

    /* Button, axis or hat: */
    Type type;

    /* SDL input offset: */
    Uint8 num;
} input_t;

/* The private structure used to keep track of a joystick */
struct joystick_hwdata
{
    LPDIRECTINPUTDEVICE2 InputDevice;
    DIDEVCAPS Capabilities;
    int buffered;

    input_t Inputs[MAX_INPUTS];
    int NumInputs;
};

#endif /* SDL_JOYSTICK_DINPUT_H */
