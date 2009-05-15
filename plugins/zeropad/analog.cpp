/*  ZeroPAD - author: zerofrog(@gmail.com)
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
	u8 Pad(int padvalue, u8 i)
	{
		switch (padvalue)
		{
			case PAD_LX:
				return g_lanalog[i].x;
				break;
			
			case PAD_RX:
				return g_ranalog[i].x;
				break;
			
			case PAD_LY:
				return g_lanalog[i].y;
				break;
			
			case PAD_RY:
				return g_ranalog[i].y;
				break;
			
			default:
				return 0;
				break;
		}
	}
	
	void SetPad(int padvalue, u8 i, u8 value)
	{
		switch (padvalue)
		{
			case PAD_LX:
				g_lanalog[i].x = value;
				break;
			
			case PAD_RX:
				g_ranalog[i].x = value;
				break;
			
			case PAD_LY:
				g_lanalog[i].y = value;
				break;
			
			case PAD_RY:
				g_ranalog[i].y = value;
				break;
			
			default:
				break;
		}
	}
	
	void InvertPad(int padvalue, u8 i)
	{
		SetPad(padvalue, i, -Pad(padvalue, i));
	}
	
	void ResetPad(int padvalue, u8 i)
	{
		SetPad(padvalue, i, 0x80);
	}
	
	void Init()
	{
		for (int i = 0; i < 2; ++i)
		{
			ResetPad(PAD_LX, i);
			ResetPad(PAD_LY, i);
			ResetPad(PAD_RX, i);
			ResetPad(PAD_RY, i);
		}
	}
	
	bool RevertPad(u8 padvalue)
	{
		switch (padvalue)
		{
			case PAD_LX:
				return (conf.options & PADOPTION_REVERTLX);
				break;
			
			case PAD_RX:
				return (conf.options & PADOPTION_REVERTRX);
				break;
			
			case PAD_LY:
				return (conf.options & PADOPTION_REVERTLY);
				break;
			
			case PAD_RY:
				return (conf.options & PADOPTION_REVERTRY);
				break;
			
			default:
				return false;
				break;
		}
	}
	
	void ConfigurePad(int padvalue, u8 i, int value)
	{
		int temp = Pad(padvalue, i);
		SetPad(padvalue, i, value / 256);
		if (RevertPad(padvalue)) InvertPad(padvalue, i);
		SetPad(padvalue, i, Pad(padvalue, i) + 0x80);
		
		PAD_LOG("Setting pad[%d]@%d to %d from %d\n", padvalue, i, value, temp);
	}
	int AnalogToPad(int padvalue)
	{
		switch (padvalue)
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
