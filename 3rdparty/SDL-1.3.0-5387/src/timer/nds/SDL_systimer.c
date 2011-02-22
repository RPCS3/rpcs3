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

#ifdef SDL_TIMER_NDS

#include <nds.h>
#include <nds/timers.h>

#include "SDL_timer.h"

/* Will wrap afetr 49 days. Shouldn't be an issue. */
static volatile Uint32 timer_ticks;

static void
NDS_TimerInterrupt(void)
{
    timer_ticks++;
}

void
SDL_StartTicks(void)
{
    timer_ticks = 0;

	/* Set timer 2 to fire every ms. */
	timerStart(2, ClockDivider_1024, TIMER_FREQ_1024(1000), NDS_TimerInterrupt);
}

Uint32
SDL_GetTicks(void)
{
    return timer_ticks;
}

void
SDL_Delay(Uint32 ms)
{
    Uint32 start = SDL_GetTicks();
    while (1) {
        if ((SDL_GetTicks() - start) >= ms)
            break;
    }
}

#endif /* SDL_TIMER_NDS */

/* vi: set ts=4 sw=4 expandtab: */
