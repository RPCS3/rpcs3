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

#include "SDL_haptic.h"


/*
 * Number of haptic devices on the system.
 */
extern Uint8 SDL_numhaptics;


struct haptic_effect
{
    SDL_HapticEffect effect;    /* The current event */
    struct haptic_hweffect *hweffect;   /* The hardware behind the event */
};

/*
 * The real SDL_Haptic struct.
 */
struct _SDL_Haptic
{
    Uint8 index;                /* Stores index it is attached to */

    struct haptic_effect *effects;      /* Allocated effects */
    int neffects;               /* Maximum amount of effects */
    int nplaying;               /* Maximum amount of effects to play at the same time */
    unsigned int supported;     /* Supported effects */
    int naxes;                  /* Number of axes on the device. */

    struct haptic_hwdata *hwdata;       /* Driver dependent */
    int ref_count;              /* Count for multiple opens */

    int rumble_id;              /* ID of rumble effect for simple rumble API. */
    SDL_HapticEffect rumble_effect; /* Rumble effect. */
};

/* 
 * Scans the system for haptic devices.
 *
 * Returns 0 on success, -1 on error.
 */
extern int SDL_SYS_HapticInit(void);

/*
 * Gets the device dependent name of the haptic device
 */
extern const char *SDL_SYS_HapticName(int index);

/*
 * Opens the haptic device for usage.  The haptic device should have
 * the index value set previously.
 *
 * Returns 0 on success, -1 on error.
 */
extern int SDL_SYS_HapticOpen(SDL_Haptic * haptic);

/*
 * Returns the index of the haptic core pointer or -1 if none is found.
 */
int SDL_SYS_HapticMouse(void);

/*
 * Checks to see if the joystick has haptic capabilities.
 *
 * Returns >0 if haptic capabilities are detected, 0 if haptic
 * capabilities aren't detected and -1 on error.
 */
extern int SDL_SYS_JoystickIsHaptic(SDL_Joystick * joystick);

/*
 * Opens the haptic device for usage using the same device as
 * the joystick.
 *
 * Returns 0 on success, -1 on error.
 */
extern int SDL_SYS_HapticOpenFromJoystick(SDL_Haptic * haptic,
                                          SDL_Joystick * joystick);
/*
 * Checks to see if haptic device and joystick device are the same.
 *
 * Returns 1 if they are the same, 0 if they aren't.
 */
extern int SDL_SYS_JoystickSameHaptic(SDL_Haptic * haptic,
                                      SDL_Joystick * joystick);

/*
 * Closes a haptic device after usage.
 */
extern void SDL_SYS_HapticClose(SDL_Haptic * haptic);

/*
 * Performs a cleanup on the haptic subsystem.
 */
extern void SDL_SYS_HapticQuit(void);

/*
 * Creates a new haptic effect on the haptic device using base
 * as a template for the effect.
 *
 * Returns 0 on success, -1 on error.
 */
extern int SDL_SYS_HapticNewEffect(SDL_Haptic * haptic,
                                   struct haptic_effect *effect,
                                   SDL_HapticEffect * base);

/*
 * Updates the haptic effect on the haptic device using data
 * as a template.
 *
 * Returns 0 on success, -1 on error.
 */
extern int SDL_SYS_HapticUpdateEffect(SDL_Haptic * haptic,
                                      struct haptic_effect *effect,
                                      SDL_HapticEffect * data);

/*
 * Runs the effect on the haptic device.
 *
 * Returns 0 on success, -1 on error.
 */
extern int SDL_SYS_HapticRunEffect(SDL_Haptic * haptic,
                                   struct haptic_effect *effect,
                                   Uint32 iterations);

/*
 * Stops the effect on the haptic device.
 *
 * Returns 0 on success, -1 on error.
 */
extern int SDL_SYS_HapticStopEffect(SDL_Haptic * haptic,
                                    struct haptic_effect *effect);

/*
 * Cleanups up the effect on the haptic device.
 */
extern void SDL_SYS_HapticDestroyEffect(SDL_Haptic * haptic,
                                        struct haptic_effect *effect);

/*
 * Queries the device for the status of effect.
 *
 * Returns 0 if device is stopped, >0 if device is playing and
 * -1 on error.
 */
extern int SDL_SYS_HapticGetEffectStatus(SDL_Haptic * haptic,
                                         struct haptic_effect *effect);

/*
 * Sets the global gain of the haptic device.
 *
 * Returns 0 on success, -1 on error.
 */
extern int SDL_SYS_HapticSetGain(SDL_Haptic * haptic, int gain);

/*
 * Sets the autocenter feature of the haptic device.
 *
 * Returns 0 on success, -1 on error.
 */
extern int SDL_SYS_HapticSetAutocenter(SDL_Haptic * haptic, int autocenter);

/*
 * Pauses the haptic device.
 *
 * Returns 0 on success, -1 on error.
 */
extern int SDL_SYS_HapticPause(SDL_Haptic * haptic);

/*
 * Unpauses the haptic device.
 *
 * Returns 0 on success, -1 on error.
 */
extern int SDL_SYS_HapticUnpause(SDL_Haptic * haptic);

/*
 * Stops all the currently playing haptic effects on the device.
 *
 * Returns 0 on success, -1 on error.
 */
extern int SDL_SYS_HapticStopAll(SDL_Haptic * haptic);
