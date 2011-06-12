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

#include "analog.h"
static PADAnalog g_lanalog[NUM_OF_PADS], g_ranalog[NUM_OF_PADS];

namespace Analog
{
	u8 Pad(int pad, u8 index)
	{
		switch (index)
		{
			case PAD_R_LEFT:
			case PAD_R_RIGHT: return g_ranalog[pad].x;

			case PAD_R_DOWN:
			case PAD_R_UP: return g_ranalog[pad].y;

			case PAD_L_LEFT:
			case PAD_L_RIGHT: return g_lanalog[pad].x;

			case PAD_L_DOWN:
			case PAD_L_UP: return g_lanalog[pad].y;

			default: return 0;
		}
	}

	static void SetPad(u8 pad, int index, u8 value)
	{
		switch (index)
		{
			case PAD_R_LEFT:
			case PAD_R_RIGHT: g_ranalog[pad].x = value; break;

			case PAD_R_DOWN:
			case PAD_R_UP: g_ranalog[pad].y = value; break;

			case PAD_L_LEFT:
			case PAD_L_RIGHT: g_lanalog[pad].x = value; break;

			case PAD_L_DOWN:
			case PAD_L_UP:  g_lanalog[pad].y = value; break;

			default: break;
		}
	}

	void ResetPad( u8 pad, int key)
	{
		SetPad(pad, key, 0x80);
	}

	void Init()
	{
		for (u8 pad = 0; pad < 2; ++pad)
		{
			// no need to put the 2 part of the axis
			ResetPad(pad, PAD_R_LEFT);
			ResetPad(pad, PAD_R_DOWN);
			ResetPad(pad, PAD_L_LEFT);
			ResetPad(pad, PAD_L_DOWN);
		}
	}

	static bool ReversePad(u8 index)
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

	void ConfigurePad( u8 pad, int index, int value)
	{
		//                          Left -> -- -> Right
		// Value range :        FFFF8002 -> 0  -> 7FFE
		// Force range :			  80 -> 0  -> 7F
		// Normal mode : expect value 0  -> 80 -> FF
		// Reverse mode: expect value FF -> 7F -> 0
		u8 force = (value / 256);
		if (ReversePad(index)) SetPad(pad, index, 0x7F - force);
		else				   SetPad(pad, index, 0x80 + force);
	}
}
