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
#include "SDL_power.h"

/*
 * Returns SDL_TRUE if we have a definitive answer.
 * SDL_FALSE to try next implementation.
 */
typedef SDL_bool
    (*SDL_GetPowerInfo_Impl) (SDL_PowerState * state, int *seconds,
                              int *percent);

SDL_bool SDL_GetPowerInfo_Linux_proc_acpi(SDL_PowerState *, int *, int *);
SDL_bool SDL_GetPowerInfo_Linux_proc_apm(SDL_PowerState *, int *, int *);
SDL_bool SDL_GetPowerInfo_Windows(SDL_PowerState *, int *, int *);
SDL_bool SDL_GetPowerInfo_MacOSX(SDL_PowerState *, int *, int *);
SDL_bool SDL_GetPowerInfo_BeOS(SDL_PowerState *, int *, int *);
SDL_bool SDL_GetPowerInfo_NintendoDS(SDL_PowerState *, int *, int *);
SDL_bool SDL_GetPowerInfo_UIKit(SDL_PowerState *, int *, int *);

#ifndef SDL_POWER_DISABLED
#ifdef SDL_POWER_HARDWIRED
/* This is for things that _never_ have a battery */
static SDL_bool
SDL_GetPowerInfo_Hardwired(SDL_PowerState * state, int *seconds, int *percent)
{
    *seconds = -1;
    *percent = -1;
    *state = SDL_POWERSTATE_NO_BATTERY;
    return SDL_TRUE;
}
#endif
#endif


static SDL_GetPowerInfo_Impl implementations[] = {
#ifndef SDL_POWER_DISABLED
#ifdef SDL_POWER_LINUX          /* in order of preference. More than could work. */
    SDL_GetPowerInfo_Linux_proc_acpi,
    SDL_GetPowerInfo_Linux_proc_apm,
#endif
#ifdef SDL_POWER_WINDOWS        /* handles Win32, Win64, PocketPC. */
    SDL_GetPowerInfo_Windows,
#endif
#ifdef SDL_POWER_UIKIT          /* handles iPhone/iPad/etc */
    SDL_GetPowerInfo_UIKit,
#endif
#ifdef SDL_POWER_MACOSX         /* handles Mac OS X, Darwin. */
    SDL_GetPowerInfo_MacOSX,
#endif
#ifdef SDL_POWER_NINTENDODS     /* handles Nintendo DS. */
    SDL_GetPowerInfo_NintendoDS,
#endif
#ifdef SDL_POWER_BEOS           /* handles BeOS, Zeta, with euc.jp apm driver. */
    SDL_GetPowerInfo_BeOS,
#endif
#ifdef SDL_POWER_HARDWIRED
    SDL_GetPowerInfo_Hardwired,
#endif
#endif
};

SDL_PowerState
SDL_GetPowerInfo(int *seconds, int *percent)
{
    const int total = sizeof(implementations) / sizeof(implementations[0]);
    int _seconds, _percent;
    SDL_PowerState retval;
    int i;

    /* Make these never NULL for platform-specific implementations. */
    if (seconds == NULL) {
        seconds = &_seconds;
    }

    if (percent == NULL) {
        percent = &_percent;
    }

    for (i = 0; i < total; i++) {
        if (implementations[i] (&retval, seconds, percent)) {
            return retval;
        }
    }

    /* nothing was definitive. */
    *seconds = -1;
    *percent = -1;
    return SDL_POWERSTATE_UNKNOWN;
}

/* vi: set ts=4 sw=4 expandtab: */
