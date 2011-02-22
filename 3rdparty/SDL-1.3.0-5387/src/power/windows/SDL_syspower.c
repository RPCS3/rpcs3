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

#ifndef SDL_POWER_DISABLED
#ifdef SDL_POWER_WINDOWS

#include "../../core/windows/SDL_windows.h"

#include "SDL_power.h"

SDL_bool
SDL_GetPowerInfo_Windows(SDL_PowerState * state, int *seconds, int *percent)
{
#ifdef _WIN32_WCE
    SYSTEM_POWER_STATUS_EX status;
#else
    SYSTEM_POWER_STATUS status;
#endif
    SDL_bool need_details = SDL_FALSE;

    /* This API should exist back to Win95 and Windows CE. */
#ifdef _WIN32_WCE
    if (!GetSystemPowerStatusEx(&status, FALSE))
#else
    if (!GetSystemPowerStatus(&status))
#endif
    {
        /* !!! FIXME: push GetLastError() into SDL_GetError() */
        *state = SDL_POWERSTATE_UNKNOWN;
    } else if (status.BatteryFlag == 0xFF) {    /* unknown state */
        *state = SDL_POWERSTATE_UNKNOWN;
    } else if (status.BatteryFlag & (1 << 7)) { /* no battery */
        *state = SDL_POWERSTATE_NO_BATTERY;
    } else if (status.BatteryFlag & (1 << 3)) { /* charging */
        *state = SDL_POWERSTATE_CHARGING;
        need_details = SDL_TRUE;
    } else if (status.ACLineStatus == 1) {
        *state = SDL_POWERSTATE_CHARGED;        /* on AC, not charging. */
        need_details = SDL_TRUE;
    } else {
        *state = SDL_POWERSTATE_ON_BATTERY;     /* not on AC. */
        need_details = SDL_TRUE;
    }

    *percent = -1;
    *seconds = -1;
    if (need_details) {
        const int pct = (int) status.BatteryLifePercent;
        const int secs = (int) status.BatteryLifeTime;

        if (pct != 255) {       /* 255 == unknown */
            *percent = (pct > 100) ? 100 : pct; /* clamp between 0%, 100% */
        }
        if (secs != 0xFFFFFFFF) {       /* ((DWORD)-1) == unknown */
            *seconds = secs;
        }
    }

    return SDL_TRUE;            /* always the definitive answer on Windows. */
}

#endif /* SDL_POWER_WINDOWS */
#endif /* SDL_POWER_DISABLED */

/* vi: set ts=4 sw=4 expandtab: */
