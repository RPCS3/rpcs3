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

#ifndef __CONTROLLER_H__
#define __CONTROLLER_H__

#ifdef __LINUX__
#define MAX_KEYS 28
#else
#define MAX_KEYS 20
#endif

#ifdef _WIN32
#define MAX_SUB_KEYS 1
#else
#define MAX_SUB_KEYS 2
#endif

enum KeyType
{
	PAD_KEYBOARD = 0,
	PAD_JOYBUTTONS,
	PAD_JOYSTICK,
	PAD_POV,
	PAD_HAT,
	PAD_NULL = -1
};

extern void set_key(int pad, int index, int value);
extern int get_key(int pad, int index);

extern KeyType type_of_key(int pad, int index);
extern int pad_to_key(int pad, int index);
extern int key_to_joystick_id(int pad, int index);
extern int key_to_button(int pad, int index);
extern int key_to_axis(int pad, int index);
extern int key_to_pov_sign(int pad, int index);
extern int key_to_hat_dir(int pad, int index);

extern int button_to_key(int joy_id, int button_id);
extern int joystick_to_key(int joy_id, int axis_id);
extern int pov_to_key(int joy_id, int sign, int axis_id);
extern int hat_to_key(int joy_id, int dir, int axis_id);

extern int PadEnum[2][2];

typedef struct
{
	bool left, right, up, down;
} HatPins;

extern HatPins hat_position;

static __forceinline void set_hat_pins(int tilt_o_the_hat)
{
	hat_position.left = false;
	hat_position.right = false;
	hat_position.up = false;
	hat_position.down = false;

	switch (tilt_o_the_hat)
	{
		case SDL_HAT_UP:
			hat_position.up = true;
			break;
		case SDL_HAT_RIGHT:
			hat_position.right= true;
			break;
		case SDL_HAT_DOWN:
			hat_position.down = true;
			break;
		case SDL_HAT_LEFT:
			hat_position.left = true;
			break;
		case SDL_HAT_LEFTUP:
			hat_position.left = true;
			hat_position.up = true;
			break;
		case SDL_HAT_RIGHTUP:
			hat_position.right= true;
			hat_position.up = true;
			break;
		case SDL_HAT_LEFTDOWN:
			hat_position.left = true;
			hat_position.down = true;
			break;
		case SDL_HAT_RIGHTDOWN:
			hat_position.right= true;
			hat_position.down = true;
			break;
		default:
			break;
	}
}

typedef struct
{
	u32 keys[2 * MAX_SUB_KEYS][MAX_KEYS];
	u32 log;
	u32 options;  // upper 16 bits are for pad2
} PADconf;

typedef struct
{
	u8 x, y;
} PADAnalog;

extern PADconf conf;
extern PADAnalog g_lanalog[2], g_ranalog[2];

#endif
