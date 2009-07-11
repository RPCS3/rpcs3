/*  OnePAD - author: arcum42(@gmail.com)
 *  Copyright (C) 2009
 *
 *  Based on ZeroPAD, author zerofrog@gmail.com
 *  Copyright (C) 2006-2007
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
 
#include "onepad.h"
#include "controller.h"

HatPins hat_position = {false, false, false, false};

__forceinline int set_key(int pad, int index, int value)
{
	conf.keys[pad][index] = value;
}

__forceinline int get_key(int pad, int index)
{
	return conf.keys[pad][index];
}

__forceinline KeyType type_of_key(int pad, int index)
 {
	 int key = conf.keys[pad][index];
	 
	if (key < 0x10000) return PAD_KEYBOARD;
	else if (key >= 0x10000 && key < 0x20000) return PAD_JOYBUTTONS;
	else if (key >= 0x20000 && key < 0x30000) return PAD_JOYSTICK;
	else if (key >= 0x30000 && key < 0x40000) return PAD_POV;
	else if (key >= 0x40000 && key < 0x50000) return PAD_HAT;
	else return PAD_NULL;
 }
 
  __forceinline int pad_to_key(int pad, int index)
 {
	return ((conf.keys[pad][index]) & 0xffff);
}
 
__forceinline int key_to_joystick_id(int pad, int index)
{
	return (((conf.keys[pad][index]) & 0xf000) >> 12);
}

__forceinline int key_to_button(int pad, int index)
{
	return ((conf.keys[pad][index]) & 0xff);
}

__forceinline int key_to_axis(int pad, int index)
{
	return ((conf.keys[pad][index]) & 0xff);
}

__forceinline int button_to_key(int joy_id, int button_id)
{
	return (0x10000 | ((joy_id) << 12) | (button_id));
}

__forceinline int joystick_to_key(int joy_id, int axis_id)
{
	return (0x20000 | ((joy_id) << 12) | (axis_id));
}

__forceinline int pov_to_key(int joy_id, int sign, int axis_id)
{
	return (0x30000 | ((joy_id) << 12) | ((sign) << 8) | (axis_id));
}

__forceinline int hat_to_key(int joy_id, int dir, int axis_id)
{
	return (0x40000 | ((joy_id) << 12) | ((dir) << 8) | (axis_id));
}

__forceinline int key_to_pov_sign(int pad, int index)
{
	return (((conf.keys[pad][index]) & 0x100) >> 8);
}

__forceinline int key_to_hat_dir(int pad, int index)
{
	return (((conf.keys[pad][index]) & ~ 0x40000) >> 8);
}
