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
#ifdef SDL_POWER_NINTENDODS

#include "SDL_power.h"

SDL_bool
SDL_GetPowerInfo_NintendoDS(SDL_PowerState * state, int *seconds,
                            int *percent)
{
    /* !!! FIXME: write me. */

    *state = SDL_POWERSTATE_UNKNOWN;
    *percent = -1;
    *seconds = -1;

    return SDL_TRUE;            /* always the definitive answer on Nintendo DS. */
}

#endif /* SDL_POWER_NINTENDODS */
#endif /* SDL_POWER_DISABLED */

/* vi: set ts=4 sw=4 expandtab: */
