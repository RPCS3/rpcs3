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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "onepad.h"
#include "controller.h"

HatPins hat_position = {false, false, false, false};

__forceinline void set_keyboad_key(int pad, int keysym, int index)
{
	conf->keysym_map[pad][keysym] = index;
}

__forceinline int get_keyboard_key(int pad, int keysym)
{
	// You must use find instead of []
	// [] will create an element if the key does not exist and return 0
	map<u32,u32>::iterator it = conf->keysym_map[pad].find(keysym);
	if (it != conf->keysym_map[pad].end())
		return it->second;
	else
		return -1;
}

__forceinline void set_key(int pad, int index, int value)
{
	conf->keys[pad][index] = value;
}

__forceinline int get_key(int pad, int index)
{
	return conf->keys[pad][index];
}

__forceinline KeyType type_of_joykey(int pad, int index)
{
	int key = get_key(pad, index);

	if	    (key >= 0x10000 && key < 0x20000) return PAD_JOYBUTTONS;
	else if (key >= 0x20000 && key < 0x30000) return PAD_AXIS;
	else if (key >= 0x30000 && key < 0x40000) return PAD_HAT;
	else return PAD_NULL;
}

__forceinline bool IsAnalogKey(int index)
{
	return ((index >= PAD_L_UP) && (index <= PAD_R_LEFT));
}

//*******************************************************
//			onepad key -> joy input
//*******************************************************
__forceinline int key_to_button(int pad, int index)
{
	return (get_key(pad, index) & 0xff);
}

__forceinline int key_to_axis(int pad, int index)
{
	return (get_key(pad, index) & 0xff);
}

__forceinline bool key_to_axis_sign(int pad, int index)
{
	return ((get_key(pad, index) >> 8) & 0x1);
}

__forceinline bool key_to_axis_type(int pad, int index)
{
	return ((get_key(pad, index) >> 9) & 0x1);
}

__forceinline int key_to_hat_dir(int pad, int index)
{
	return ((get_key(pad, index) >> 8) & 0xF);
}

//*******************************************************
//			joy input -> onepad key
//*******************************************************
__forceinline int button_to_key(int button_id)
{
	return (0x10000 | button_id);
}

__forceinline int axis_to_key(int full_axis, int sign, int axis_id)
{
	return (0x20000 | (full_axis  << 9) | (sign << 8) | axis_id);
}

__forceinline int hat_to_key(int dir, int axis_id)
{
	return (0x30000 | (dir << 8) | axis_id);
}
