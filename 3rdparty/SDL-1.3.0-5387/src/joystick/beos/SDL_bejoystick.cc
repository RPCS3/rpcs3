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

#ifdef SDL_JOYSTICK_BEOS

/* This is the system specific header for the SDL joystick API */

#include <be/support/String.h>
#include <be/device/Joystick.h>

extern "C"
{

#include "SDL_joystick.h"
#include "../SDL_sysjoystick.h"
#include "../SDL_joystick_c.h"


/* The maximum number of joysticks we'll detect */
#define MAX_JOYSTICKS	16

/* A list of available joysticks */
    static char *SDL_joyport[MAX_JOYSTICKS];
    static char *SDL_joyname[MAX_JOYSTICKS];

/* The private structure used to keep track of a joystick */
    struct joystick_hwdata
    {
        BJoystick *stick;
        uint8 *new_hats;
        int16 *new_axes;
    };

/* Function to scan the system for joysticks.
 * This function should set SDL_numjoysticks to the number of available
 * joysticks.  Joystick 0 should be the system default joystick.
 * It should return 0, or -1 on an unrecoverable fatal error.
 */
    int SDL_SYS_JoystickInit(void)
    {
        BJoystick joystick;
        int numjoysticks;
        int i;
        int32 nports;
        char name[B_OS_NAME_LENGTH];

        /* Search for attached joysticks */
          nports = joystick.CountDevices();
          numjoysticks = 0;
          SDL_memset(SDL_joyport, 0, (sizeof SDL_joyport));
          SDL_memset(SDL_joyname, 0, (sizeof SDL_joyname));
        for (i = 0; (SDL_numjoysticks < MAX_JOYSTICKS) && (i < nports); ++i)
        {
            if (joystick.GetDeviceName(i, name) == B_OK) {
                if (joystick.Open(name) != B_ERROR) {
                    BString stick_name;
                      joystick.GetControllerName(&stick_name);
                      SDL_joyport[numjoysticks] = strdup(name);
                      SDL_joyname[numjoysticks] = strdup(stick_name.String());
                      numjoysticks++;
                      joystick.Close();
                }
            }
        }
        return (numjoysticks);
    }

/* Function to get the device-dependent name of a joystick */
    const char *SDL_SYS_JoystickName(int index)
    {
        return SDL_joyname[index];
    }

/* Function to open a joystick for use.
   The joystick to open is specified by the index field of the joystick.
   This should fill the nbuttons and naxes fields of the joystick structure.
   It returns 0, or -1 if there is an error.
 */
    int SDL_SYS_JoystickOpen(SDL_Joystick * joystick)
    {
        BJoystick *stick;

        /* Create the joystick data structure */
        joystick->hwdata = (struct joystick_hwdata *)
            SDL_malloc(sizeof(*joystick->hwdata));
        if (joystick->hwdata == NULL) {
            SDL_OutOfMemory();
            return (-1);
        }
        SDL_memset(joystick->hwdata, 0, sizeof(*joystick->hwdata));
        stick = new BJoystick;
        joystick->hwdata->stick = stick;

        /* Open the requested joystick for use */
        if (stick->Open(SDL_joyport[joystick->index]) == B_ERROR) {
            SDL_SetError("Unable to open joystick");
            SDL_SYS_JoystickClose(joystick);
            return (-1);
        }

        /* Set the joystick to calibrated mode */
        stick->EnableCalibration();

        /* Get the number of buttons, hats, and axes on the joystick */
        joystick->nbuttons = stick->CountButtons();
        joystick->naxes = stick->CountAxes();
        joystick->nhats = stick->CountHats();

        joystick->hwdata->new_axes = (int16 *)
            SDL_malloc(joystick->naxes * sizeof(int16));
        joystick->hwdata->new_hats = (uint8 *)
            SDL_malloc(joystick->nhats * sizeof(uint8));
        if (!joystick->hwdata->new_hats || !joystick->hwdata->new_axes) {
            SDL_OutOfMemory();
            SDL_SYS_JoystickClose(joystick);
            return (-1);
        }

        /* We're done! */
        return (0);
    }

/* Function to update the state of a joystick - called as a device poll.
 * This function shouldn't update the joystick structure directly,
 * but instead should call SDL_PrivateJoystick*() to deliver events
 * and update joystick device state.
 */
    void SDL_SYS_JoystickUpdate(SDL_Joystick * joystick)
    {
        static const Uint8 hat_map[9] = {
            SDL_HAT_CENTERED,
            SDL_HAT_UP,
            SDL_HAT_RIGHTUP,
            SDL_HAT_RIGHT,
            SDL_HAT_RIGHTDOWN,
            SDL_HAT_DOWN,
            SDL_HAT_LEFTDOWN,
            SDL_HAT_LEFT,
            SDL_HAT_LEFTUP
        };
        const int JITTER = (32768 / 10);        /* 10% jitter threshold (ok?) */

        BJoystick *stick;
        int i, change;
        int16 *axes;
        uint8 *hats;
        uint32 buttons;

        /* Set up data pointers */
        stick = joystick->hwdata->stick;
        axes = joystick->hwdata->new_axes;
        hats = joystick->hwdata->new_hats;

        /* Get the new joystick state */
        stick->Update();
        stick->GetAxisValues(axes);
        stick->GetHatValues(hats);
        buttons = stick->ButtonValues();

        /* Generate axis motion events */
        for (i = 0; i < joystick->naxes; ++i) {
            change = ((int32) axes[i] - joystick->axes[i]);
            if ((change > JITTER) || (change < -JITTER)) {
                SDL_PrivateJoystickAxis(joystick, i, axes[i]);
            }
        }

        /* Generate hat change events */
        for (i = 0; i < joystick->nhats; ++i) {
            if (hats[i] != joystick->hats[i]) {
                SDL_PrivateJoystickHat(joystick, i, hat_map[hats[i]]);
            }
        }

        /* Generate button events */
        for (i = 0; i < joystick->nbuttons; ++i) {
            if ((buttons & 0x01) != joystick->buttons[i]) {
                SDL_PrivateJoystickButton(joystick, i, (buttons & 0x01));
            }
            buttons >>= 1;
        }
    }

/* Function to close a joystick after use */
    void SDL_SYS_JoystickClose(SDL_Joystick * joystick)
    {
        if (joystick->hwdata) {
            joystick->hwdata->stick->Close();
            delete joystick->hwdata->stick;
            if (joystick->hwdata->new_hats) {
                SDL_free(joystick->hwdata->new_hats);
            }
            if (joystick->hwdata->new_axes) {
                SDL_free(joystick->hwdata->new_axes);
            }
            SDL_free(joystick->hwdata);
            joystick->hwdata = NULL;
        }
    }

/* Function to perform any system-specific joystick related cleanup */
    void SDL_SYS_JoystickQuit(void)
    {
        int i;

        for (i = 0; SDL_joyport[i]; ++i) {
            SDL_free(SDL_joyport[i]);
        }
        SDL_joyport[0] = NULL;

        for (i = 0; SDL_joyname[i]; ++i) {
            SDL_free(SDL_joyname[i]);
        }
        SDL_joyname[0] = NULL;
    }

};                              // extern "C"

#endif /* SDL_JOYSTICK_BEOS */
/* vi: set ts=4 sw=4 expandtab: */
