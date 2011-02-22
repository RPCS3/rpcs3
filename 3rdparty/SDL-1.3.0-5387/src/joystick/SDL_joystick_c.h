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

/* Useful functions and variables from SDL_joystick.c */
#include "SDL_joystick.h"

/* The number of available joysticks on the system */
extern Uint8 SDL_numjoysticks;

/* Initialization and shutdown functions */
extern int SDL_JoystickInit(void);
extern void SDL_JoystickQuit(void);

/* Internal event queueing functions */
extern int SDL_PrivateJoystickAxis(SDL_Joystick * joystick,
                                   Uint8 axis, Sint16 value);
extern int SDL_PrivateJoystickBall(SDL_Joystick * joystick,
                                   Uint8 ball, Sint16 xrel, Sint16 yrel);
extern int SDL_PrivateJoystickHat(SDL_Joystick * joystick,
                                  Uint8 hat, Uint8 value);
extern int SDL_PrivateJoystickButton(SDL_Joystick * joystick,
                                     Uint8 button, Uint8 state);

/* Internal sanity checking functions */
extern int SDL_PrivateJoystickValid(SDL_Joystick ** joystick);

/* vi: set ts=4 sw=4 expandtab: */
