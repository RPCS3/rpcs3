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
#ifdef SDL_POWER_UIKIT

#import <UIKit/UIKit.h>

#include "SDL_power.h"
#include "SDL_timer.h"
#include "SDL_assert.h"

// turn off the battery monitor if it's been more than X ms since last check.
static const int BATTERY_MONITORING_TIMEOUT = 3000;
static Uint32 SDL_UIKitLastPowerInfoQuery = 0;

void
SDL_UIKit_UpdateBatteryMonitoring(void)
{
    if (SDL_UIKitLastPowerInfoQuery) {
        const Uint32 prev = SDL_UIKitLastPowerInfoQuery;
        const UInt32 now = SDL_GetTicks();
        const UInt32 ticks = now - prev;
        // if timer wrapped (now < prev), shut down, too.
        if ((now < prev) || (ticks >= BATTERY_MONITORING_TIMEOUT)) {
            UIDevice *uidev = [UIDevice currentDevice];
            SDL_assert([uidev isBatteryMonitoringEnabled] == YES);
            [uidev setBatteryMonitoringEnabled:NO];
            SDL_UIKitLastPowerInfoQuery = 0;
        }
    }
}

SDL_bool
SDL_GetPowerInfo_UIKit(SDL_PowerState * state, int *seconds, int *percent)
{
    UIDevice *uidev = [UIDevice currentDevice];

    if (!SDL_UIKitLastPowerInfoQuery) {
        SDL_assert([uidev isBatteryMonitoringEnabled] == NO);
        [uidev setBatteryMonitoringEnabled:YES];
    }

    // UIKit_GL_SwapWindow() (etc) will check this and disable the battery
    //  monitoring if the app hasn't queried it in the last X seconds.
    //  Apparently monitoring the battery burns battery life.  :)
    //  Apple's docs say not to monitor the battery unless you need it.
    SDL_UIKitLastPowerInfoQuery = SDL_GetTicks();

    *seconds = -1;   // no API to estimate this in UIKit.

    switch ([uidev batteryState])
    {
        case UIDeviceBatteryStateCharging:
            *state = SDL_POWERSTATE_CHARGING;
            break;

        case UIDeviceBatteryStateFull:
            *state = SDL_POWERSTATE_CHARGED;
            break;

        case UIDeviceBatteryStateUnplugged:
            *state = SDL_POWERSTATE_ON_BATTERY;
            break;

        case UIDeviceBatteryStateUnknown:
        default:
            *state = SDL_POWERSTATE_UNKNOWN;
            break;
    }

    const float level = [uidev batteryLevel];
    *percent = ( (level < 0.0f) ? -1 : (((int) (level + 0.5f)) * 100) );
    return SDL_TRUE;            /* always the definitive answer on iPhoneOS. */
}

#endif /* SDL_POWER_UIKIT */
#endif /* SDL_POWER_DISABLED */

/* vi: set ts=4 sw=4 expandtab: */
