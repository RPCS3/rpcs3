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

#ifdef SDL_TIMER_WINCE

#include "../../core/windows/SDL_windows.h"

#include "SDL_timer.h"

static Uint64 start_date;
static Uint64 start_ticks;

static Uint64
wce_ticks(void)
{
    return ((Uint64) GetTickCount());
}

static Uint64
wce_date(void)
{
    union
    {
        FILETIME ftime;
        Uint64 itime;
    } ftime;
    SYSTEMTIME stime;

    GetSystemTime(&stime);
    SystemTimeToFileTime(&stime, &ftime.ftime);
    ftime.itime /= 10000;       // Convert 100ns intervals to 1ms intervals
    // Remove ms portion, which can't be relied on
    ftime.itime -= (ftime.itime % 1000);
    return (ftime.itime);
}

static Sint32
wce_rel_ticks(void)
{
    return ((Sint32) (wce_ticks() - start_ticks));
}

static Sint32
wce_rel_date(void)
{
    return ((Sint32) (wce_date() - start_date));
}

/* Recard start-time of application for reference */
void
SDL_StartTicks(void)
{
    start_date = wce_date();
    start_ticks = wce_ticks();
}

/* Return time in ms relative to when SDL was started */
Uint32
SDL_GetTicks()
{
    Sint32 offset = wce_rel_date() - wce_rel_ticks();
    if ((offset < -1000) || (offset > 1000)) {
//    fprintf(stderr,"Time desync(%+d), resyncing\n",offset/1000);
        start_ticks -= offset;
    }

    return ((Uint32) wce_rel_ticks());
}

/* Give up approx. givem milliseconds to the OS. */
void
SDL_Delay(Uint32 ms)
{
    Sleep(ms);
}

#endif /* SDL_TIMER_WINCE */

/* vi: set ts=4 sw=4 expandtab: */
