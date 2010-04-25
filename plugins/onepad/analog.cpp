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

 #include "analog.h"
PADAnalog g_lanalog[NUM_OF_PADS], g_ranalog[NUM_OF_PADS];

namespace Analog
{
	u8 Pad(int pad, u8 index)
	{
		switch (index)
		{
			case PAD_LX:
				return g_lanalog[pad].x;
				break;

			case PAD_RX:
				return g_ranalog[pad].x;
				break;

			case PAD_LY:
				return g_lanalog[pad].y;
				break;

			case PAD_RY:
				return g_ranalog[pad].y;
				break;

			default:
				return 0;
				break;
		}
	}

	void SetPad(u8 pad, int index, u8 value)
	{
		switch (index)
		{
			case PAD_LX:
				g_lanalog[pad].x = value;
				break;

			case PAD_RX:
				g_ranalog[ pad].x = value;
				break;

			case PAD_LY:
				g_lanalog[ pad].y = value;
				break;

			case PAD_RY:
				g_ranalog[pad].y = value;
				break;

			default:
				break;
		}
	}

	void InvertPad(u8 pad, int key)
	{
		SetPad(pad, key, -Pad(pad, key));
	}

	void ResetPad( u8 pad, int key)
	{
		SetPad(pad, key, 0x80);
	}

	void Init()
	{
		for (u8 pad = 0; pad < 2; ++pad)
		{
			ResetPad(pad, PAD_LX);
			ResetPad(pad, PAD_LY);
			ResetPad(pad, PAD_RX);
			ResetPad(pad, PAD_RY);
		}
	}

	bool RevertPad(u8 index)
	{
		switch (index)
		{
			case PAD_LX:
				return ((conf.options & PADOPTION_REVERTLX) != 0);
				break;

			case PAD_RX:
				return ((conf.options & PADOPTION_REVERTRX) != 0);
				break;

			case PAD_LY:
				return ((conf.options & PADOPTION_REVERTLY) != 0);
				break;

			case PAD_RY:
				return ((conf.options & PADOPTION_REVERTRY) != 0);
				break;

			default:
				return false;
				break;
		}
	}

	void ConfigurePad( u8 pad, int index, int value)
	{
		Pad(pad, index);
		SetPad(pad, index, value / 256);
		if (RevertPad(index)) InvertPad(pad,index);
		SetPad(pad, index, Pad(pad, index) + 0x80);
	}

	int AnalogToPad(int index)
	{
		switch (index)
		{
			case PAD_R_LEFT:
				return PAD_RX;
				break;
			case PAD_R_UP:
				return PAD_RY;
				break;
			case PAD_L_LEFT:
				return PAD_LX;
				break;
			case PAD_L_UP:
				return PAD_LY;
				break;
			case PAD_R_DOWN:
				return PAD_RY;
				break;
			case PAD_R_RIGHT:
				return PAD_RX;
				break;
			case PAD_L_DOWN:
				return PAD_LY;
				break;
			case PAD_L_RIGHT:
				return PAD_LX;
				break;
		}
		return 0;
	}
}
