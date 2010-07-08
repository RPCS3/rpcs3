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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#pragma once

#define NUM_OF_PADS 2

#include "zeropad.h"

namespace Analog
{
	extern void Init();
	extern u8 Pad(int padvalue, u8 i);
	extern void SetPad(int padvalue, u8 i, u8 value);
	extern void InvertPad(int padvalue, u8 i);
	extern bool RevertPad(u8 padvalue);
	extern void ResetPad(int padvalue, u8 i);
	extern void ConfigurePad(int padvalue, u8 i, int value);
	extern int KeypadToPad(u8 keypress);
	extern int AnalogToPad(int padvalue);
}
