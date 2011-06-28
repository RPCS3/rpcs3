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

#include "KeyStatus.h"

void KeyStatus::Init()
{
	for (int pad = 0; pad < 2; pad++) {
		m_button[pad] = 0xFFFF;
		m_internal_button_kbd[pad] = 0xFFFF;
		m_internal_button_joy[pad] = 0xFFFF;

		for (int index = 0; index < MAX_KEYS; index++) {
			m_button_pressure[pad][index] = 0xFF;
			m_internal_button_pressure[pad][index] = 0xFF;
		}

		analog[pad].lx = 0x80;
		analog[pad].ly = 0x80;
		analog[pad].rx = 0x80;
		analog[pad].ry = 0x80;
		m_internal_analog_kbd[pad].lx = 0x80;
		m_internal_analog_kbd[pad].ly = 0x80;
		m_internal_analog_kbd[pad].rx = 0x80;
		m_internal_analog_kbd[pad].ry = 0x80;
		m_internal_analog_joy[pad].lx = 0x80;
		m_internal_analog_joy[pad].ly = 0x80;
		m_internal_analog_joy[pad].rx = 0x80;
		m_internal_analog_joy[pad].ry = 0x80;
	}
}

void KeyStatus::press(u32 pad, u32 index, s32 value)
{
	if (!IsAnalogKey(index)) {
		if (m_state_acces[pad])
			clear_bit(m_internal_button_kbd[pad], index);
		else
			clear_bit(m_internal_button_joy[pad], index);
	} else {
		// clamp value
		if (value > MAX_ANALOG_VALUE)
			value = MAX_ANALOG_VALUE;
		else if (value < -MAX_ANALOG_VALUE)
			value = -MAX_ANALOG_VALUE;

		//                          Left -> -- -> Right
		// Value range :        FFFF8002 -> 0  -> 7FFE
		// Force range :			  80 -> 0  -> 7F
		// Normal mode : expect value 0  -> 80 -> FF
		// Reverse mode: expect value FF -> 7F -> 0
		u8 force = (value / 256);
		if (analog_is_reversed(index)) analog_set(pad, index, 0x7F - force);
		else						   analog_set(pad, index, 0x80 + force);
	}
}

void KeyStatus::release(u32 pad, u32 index)
{
	if (!IsAnalogKey(index)) {
		if (m_state_acces[pad])
			set_bit(m_internal_button_kbd[pad], index);
		else
			set_bit(m_internal_button_joy[pad], index);
	} else {
		analog_set(pad, index, 0x80);
	}
}

u16 KeyStatus::get(u32 pad)
{
	return m_button[pad];
}

void KeyStatus::set_pressure(u32 pad, u32 index, u32 value)
{
	m_internal_button_pressure[pad][index] = value;
}

u8 KeyStatus::get_pressure(u32 pad, u32 index)
{
	return m_button_pressure[pad][index];
}

void KeyStatus::analog_set(u32 pad, u32 index, u8 value)
{
	PADAnalog* m_internal_analog_ref;
	if (m_state_acces[pad])
		m_internal_analog_ref = &m_internal_analog_kbd[pad];
	else
		m_internal_analog_ref = &m_internal_analog_joy[pad];

	switch (index)
	{
		case PAD_R_LEFT:
		case PAD_R_RIGHT: m_internal_analog_ref->rx = value; break;

		case PAD_R_DOWN:
		case PAD_R_UP: m_internal_analog_ref->ry = value; break;

		case PAD_L_LEFT:
		case PAD_L_RIGHT: m_internal_analog_ref->lx = value; break;

		case PAD_L_DOWN:
		case PAD_L_UP: m_internal_analog_ref->ly  = value; break;

		default: break;
	}
}

bool KeyStatus::analog_is_reversed(u32 index)
{
	switch (index)
	{
		case PAD_L_RIGHT:
		case PAD_L_LEFT:
			return ((conf->options & PADOPTION_REVERSELX) != 0);

		case PAD_R_LEFT:
		case PAD_R_RIGHT:
			return ((conf->options & PADOPTION_REVERSERX) != 0);

		case PAD_L_UP:
		case PAD_L_DOWN:
			return ((conf->options & PADOPTION_REVERSELY) != 0);

		case PAD_R_DOWN:
		case PAD_R_UP:
			return ((conf->options & PADOPTION_REVERSERY) != 0);

		default: return false;
	}
}

u8 KeyStatus::analog_get(u32 pad, u32 index)
{
	switch (index)
	{
		case PAD_R_LEFT:
		case PAD_R_RIGHT: return analog[pad].rx;

		case PAD_R_DOWN:
		case PAD_R_UP: return analog[pad].ry;

		case PAD_L_LEFT:
		case PAD_L_RIGHT: return analog[pad].lx;

		case PAD_L_DOWN:
		case PAD_L_UP: return analog[pad].ly;

		default: return 0;
	}
}

u8 KeyStatus::analog_merge(u8 kbd, u8 joy)
{
	if (kbd != 0x80)
		return kbd;
	else
		return joy;
}

void KeyStatus::commit_status(u32 pad)
{
	m_button[pad] = m_internal_button_kbd[pad] & m_internal_button_joy[pad];

	for (int index = 0; index < MAX_KEYS; index++)
		m_button_pressure[pad][index] = m_internal_button_pressure[pad][index];

	analog[pad].lx = analog_merge(m_internal_analog_kbd[pad].lx, m_internal_analog_joy[pad].lx);
	analog[pad].ly = analog_merge(m_internal_analog_kbd[pad].ly, m_internal_analog_joy[pad].ly);
	analog[pad].rx = analog_merge(m_internal_analog_kbd[pad].rx, m_internal_analog_joy[pad].rx);
	analog[pad].ry = analog_merge(m_internal_analog_kbd[pad].ry, m_internal_analog_joy[pad].ry);
}
