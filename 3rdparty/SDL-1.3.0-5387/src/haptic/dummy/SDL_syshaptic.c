/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 2008 Edgar Simo

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

#if defined(SDL_HAPTIC_DUMMY) || defined(SDL_HAPTIC_DISABLED)

#include "SDL_haptic.h"
#include "../SDL_syshaptic.h"


static int
SDL_SYS_LogicError(void)
{
    SDL_SetError("Logic error: No haptic devices available.");
    return 0;
}


int
SDL_SYS_HapticInit(void)
{
    return 0;
}


const char *
SDL_SYS_HapticName(int index)
{
    SDL_SYS_LogicError();
    return NULL;
}


int
SDL_SYS_HapticOpen(SDL_Haptic * haptic)
{
    SDL_SYS_LogicError();
    return -1;
}


int
SDL_SYS_HapticMouse(void)
{
    return -1;
}


int
SDL_SYS_JoystickIsHaptic(SDL_Joystick * joystick)
{
    return 0;
}


int
SDL_SYS_HapticOpenFromJoystick(SDL_Haptic * haptic, SDL_Joystick * joystick)
{
    SDL_SYS_LogicError();
    return -1;
}


int
SDL_SYS_JoystickSameHaptic(SDL_Haptic * haptic, SDL_Joystick * joystick)
{
    return 0;
}


void
SDL_SYS_HapticClose(SDL_Haptic * haptic)
{
    return;
}


void
SDL_SYS_HapticQuit(void)
{
    return;
}


int
SDL_SYS_HapticNewEffect(SDL_Haptic * haptic,
                        struct haptic_effect *effect, SDL_HapticEffect * base)
{
    SDL_SYS_LogicError();
    return -1;
}


int
SDL_SYS_HapticUpdateEffect(SDL_Haptic * haptic,
                           struct haptic_effect *effect,
                           SDL_HapticEffect * data)
{
    SDL_SYS_LogicError();
    return -1;
}


int
SDL_SYS_HapticRunEffect(SDL_Haptic * haptic, struct haptic_effect *effect,
                        Uint32 iterations)
{
    SDL_SYS_LogicError();
    return -1;
}


int
SDL_SYS_HapticStopEffect(SDL_Haptic * haptic, struct haptic_effect *effect)
{
    SDL_SYS_LogicError();
    return -1;
}


void
SDL_SYS_HapticDestroyEffect(SDL_Haptic * haptic, struct haptic_effect *effect)
{
    SDL_SYS_LogicError();
    return;
}


int
SDL_SYS_HapticGetEffectStatus(SDL_Haptic * haptic,
                              struct haptic_effect *effect)
{
    SDL_SYS_LogicError();
    return -1;
}


int
SDL_SYS_HapticSetGain(SDL_Haptic * haptic, int gain)
{
    SDL_SYS_LogicError();
    return -1;
}


int
SDL_SYS_HapticSetAutocenter(SDL_Haptic * haptic, int autocenter)
{
    SDL_SYS_LogicError();
    return -1;
}

int
SDL_SYS_HapticPause(SDL_Haptic * haptic)
{
    SDL_SYS_LogicError();
    return -1;
}

int
SDL_SYS_HapticUnpause(SDL_Haptic * haptic)
{
    SDL_SYS_LogicError();
    return -1;
}

int
SDL_SYS_HapticStopAll(SDL_Haptic * haptic)
{
    SDL_SYS_LogicError();
    return -1;
}



#endif /* SDL_HAPTIC_DUMMY || SDL_HAPTIC_DISABLED */
