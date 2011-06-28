/*  OnePAD - author: arcum42(@gmail.com)
 *  Copyright (C) 2011
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

#ifndef __KEYSTATUS_H__
#define __KEYSTATUS_H__

#include "onepad.h"

typedef struct
{
	u8 lx, ly;
	u8 rx, ry;
} PADAnalog;

#define MAX_ANALOG_VALUE	  	32766

class KeyStatus
{
	private:
		u16 m_button[2];
		u16 m_internal_button_kbd[2];
		u16 m_internal_button_joy[2];

		u8 m_button_pressure[2][MAX_KEYS];
		u8 m_internal_button_pressure[2][MAX_KEYS];

		bool m_state_acces[2];

		PADAnalog analog[2];
		PADAnalog m_internal_analog_kbd[2];
		PADAnalog m_internal_analog_joy[2];

		void analog_set(u32 pad, u32 index, u8 value);
		bool analog_is_reversed(u32 index);
		u8   analog_merge(u8 kbd, u8 joy);

	public:
		KeyStatus() { Init(); }
		void Init();

		void keyboard_state_acces(u32 pad) { m_state_acces[pad] = true; }
		void joystick_state_acces(u32 pad) { m_state_acces[pad] = false; }

		void press(u32 pad, u32 index, s32 value = 0);
		void release(u32 pad, u32 index);
		u16  get(u32 pad);

		void set_pressure(u32 pad, u32 index, u32 value);
		u8   get_pressure(u32 pad, u32 index);

		u8   analog_get(u32 pad, u32 index);

		void commit_status(u32 pad);
};

extern KeyStatus* key_status;

#endif
