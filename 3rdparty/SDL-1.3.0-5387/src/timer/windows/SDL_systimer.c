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

#ifdef SDL_TIMER_WINDOWS

#include "../../core/windows/SDL_windows.h"
#include <mmsystem.h>

#include "SDL_timer.h"

#ifdef _WIN32_WCE
#error This is WinCE. Please use src/timer/wince/SDL_systimer.c instead.
#endif

#define TIME_WRAP_VALUE	(~(DWORD)0)

/* The first (low-resolution) ticks value of the application */
static DWORD start;

#ifndef USE_GETTICKCOUNT
/* Store if a high-resolution performance counter exists on the system */
static BOOL hires_timer_available;
/* The first high-resolution ticks value of the application */
static LARGE_INTEGER hires_start_ticks;
/* The number of ticks per second of the high-resolution performance counter */
static LARGE_INTEGER hires_ticks_per_second;
#endif

void
SDL_StartTicks(void)
{
    /* Set first ticks value */
#ifdef USE_GETTICKCOUNT
    start = GetTickCount();
#else
#if 0                           /* Apparently there are problems with QPC on Win2K */
    if (QueryPerformanceFrequency(&hires_ticks_per_second) == TRUE) {
        hires_timer_available = TRUE;
        QueryPerformanceCounter(&hires_start_ticks);
    } else
#endif
    {
        hires_timer_available = FALSE;
        timeBeginPeriod(1);     /* use 1 ms timer precision */
        start = timeGetTime();
    }
#endif
}

Uint32
SDL_GetTicks(void)
{
    DWORD now, ticks;
#ifndef USE_GETTICKCOUNT
    LARGE_INTEGER hires_now;
#endif

#ifdef USE_GETTICKCOUNT
    now = GetTickCount();
#else
    if (hires_timer_available) {
        QueryPerformanceCounter(&hires_now);

        hires_now.QuadPart -= hires_start_ticks.QuadPart;
        hires_now.QuadPart *= 1000;
        hires_now.QuadPart /= hires_ticks_per_second.QuadPart;

        return (DWORD) hires_now.QuadPart;
    } else {
        now = timeGetTime();
    }
#endif

    if (now < start) {
        ticks = (TIME_WRAP_VALUE - start) + now;
    } else {
        ticks = (now - start);
    }
    return (ticks);
}

void
SDL_Delay(Uint32 ms)
{
    Sleep(ms);
}

#endif /* SDL_TIMER_WINDOWS */

/* vi: set ts=4 sw=4 expandtab: */
