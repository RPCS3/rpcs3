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
#define MAX_KEYS 24
#else
#define MAX_KEYS 20
#endif

enum KeyType
{
	PAD_JOYBUTTONS = 0,
	PAD_AXIS,
	PAD_HAT,
	PAD_NULL = -1
};

extern void set_keyboad_key(int pad, int keysym, int index);
extern int get_keyboard_key(int pad, int keysym);
extern void set_key(int pad, int index, int value);
extern int get_key(int pad, int index);
extern bool IsAnalogKey(int index);

extern KeyType type_of_joykey(int pad, int index);
extern int key_to_button(int pad, int index);
extern int key_to_axis(int pad, int index);
extern bool key_to_axis_sign(int pad, int index);
extern bool key_to_axis_type(int pad, int index);
extern int key_to_hat_dir(int pad, int index);

extern int button_to_key(int button_id);
extern int axis_to_key(int full_axis, int sign, int axis_id);
extern int hat_to_key(int dir, int axis_id);

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

struct PADconf
{
	public:
	u32 keys[2][MAX_KEYS];
	u32 log;
	u32 options;  // upper 16 bits are for pad2
	u32 sensibility;
	u32 joyid_map;
	map<u32,u32> keysym_map[2];

	PADconf() { init(); }

	void init() {
		memset(&keys, 0, sizeof(keys));
		log = options = joyid_map = 0;
		sensibility = 500;
		for (int pad = 0; pad < 2 ; pad++)
			keysym_map[pad].clear();
	}

	void set_joyid(u32 pad, u32 joy_id) {
		int shift = 8 * pad;
		joyid_map &= ~(0xFF << shift); // clear
		joyid_map |= (joy_id & 0xFF) << shift; // set
	}

	u32 get_joyid(u32 pad) {
		int shift = 8 * pad;
		return ((joyid_map >> shift) & 0xFF);
	}
};
extern PADconf *conf;
#endif
