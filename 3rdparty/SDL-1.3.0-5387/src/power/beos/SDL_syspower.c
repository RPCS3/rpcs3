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
#ifdef SDL_POWER_BEOS

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <drivers/Drivers.h>

/* These values are from apm.h ... */
#define APM_DEVICE_PATH "/dev/misc/apm"
#define APM_FUNC_OFFSET 0x5300
#define APM_FUNC_GET_POWER_STATUS 10
#define APM_DEVICE_ALL 1
#define APM_BIOS_CALL (B_DEVICE_OP_CODES_END + 3)

#include "SDL_power.h"

SDL_bool
SDL_GetPowerInfo_BeOS(SDL_PowerState * state, int *seconds, int *percent)
{
    const int fd = open("/dev/misc/apm", O_RDONLY);
    SDL_bool need_details = SDL_FALSE;
    uint16 regs[6];
    uint8 ac_status;
    uint8 battery_status;
    uint8 battery_flags;
    uint8 battery_life;
    uint32 battery_time;

    if (fd == -1) {
        return SDL_FALSE;       /* maybe some other method will work? */
    }

    memset(regs, '\0', sizeof(regs));
    regs[0] = APM_FUNC_OFFSET + APM_FUNC_GET_POWER_STATUS;
    regs[1] = APM_DEVICE_ALL;
    rc = ioctl(fd, APM_BIOS_CALL, regs);
    close(fd);

    if (rc < 0) {
        return SDL_FALSE;
    }

    ac_status = regs[1] >> 8;
    battery_status = regs[1] & 0xFF;
    battery_flags = regs[2] >> 8;
    battery_life = regs[2] & 0xFF;
    battery_time = (uint32) regs[3];

    /* in theory, _something_ should be set in battery_flags, right? */
    if (battery_flags == 0x00) {        /* older APM BIOS? Less fields. */
        battery_time = 0xFFFF;
        if (battery_status == 0xFF) {
            battery_flags = 0xFF;
        } else {
            battery_flags = (1 << status.battery_status);
        }
    }

    if ((battery_time != 0xFFFF) && (battery_time & (1 << 15))) {
        /* time is in minutes, not seconds */
        battery_time = (battery_time & 0x7FFF) * 60;
    }

    if (battery_flags == 0xFF) {        /* unknown state */
        *state = SDL_POWERSTATE_UNKNOWN;
    } else if (battery_flags & (1 << 7)) {      /* no battery */
        *state = SDL_POWERSTATE_NO_BATTERY;
    } else if (battery_flags & (1 << 3)) {      /* charging */
        *state = SDL_POWERSTATE_CHARGING;
        need_details = SDL_TRUE;
    } else if (ac_status == 1) {
        *state = SDL_POWERSTATE_CHARGED;        /* on AC, not charging. */
        need_details = SDL_TRUE;
    } else {
        *state = SDL_POWERSTATE_ON_BATTERY;     /* not on AC. */
        need_details = SDL_TRUE;
    }

    *percent = -1;
    *seconds = -1;
    if (need_details) {
        const int pct = (int) battery_life;
        const int secs = (int) battery_time;

        if (pct != 255) {       /* 255 == unknown */
            *percent = (pct > 100) ? 100 : pct; /* clamp between 0%, 100% */
        }
        if (secs != 0xFFFF) {   /* 0xFFFF == unknown */
            *seconds = secs;
        }
    }

    return SDL_TRUE;            /* the definitive answer if APM driver replied. */
}

#endif /* SDL_POWER_BEOS */
#endif /* SDL_POWER_DISABLED */

/* vi: set ts=4 sw=4 expandtab: */
