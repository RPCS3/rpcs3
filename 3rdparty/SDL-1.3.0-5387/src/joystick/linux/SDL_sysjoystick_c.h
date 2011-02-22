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

#if SDL_INPUT_LINUXEV
#include <linux/input.h>
#endif

/* The private structure used to keep track of a joystick */
struct joystick_hwdata
{
    int fd;
    char *fname;                /* Used in haptic subsystem */

    /* The current linux joystick driver maps hats to two axes */
    struct hwdata_hat
    {
        int axis[2];
    } *hats;
    /* The current linux joystick driver maps balls to two axes */
    struct hwdata_ball
    {
        int axis[2];
    } *balls;

    /* Support for the Linux 2.4 unified input interface */
#if SDL_INPUT_LINUXEV
    SDL_bool is_hid;
    Uint8 key_map[KEY_MAX - BTN_MISC];
    Uint8 abs_map[ABS_MAX];
    struct axis_correct
    {
        int used;
        int coef[3];
    } abs_correct[ABS_MAX];
#endif
};
