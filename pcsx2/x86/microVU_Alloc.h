/*  Pcsx2 - Pc Ps2 Emulator
*  Copyright (C) 2009  Pcsx2-Playground Team
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

union regInfo {
	u32 reg;
	struct {
		u8 x;
		u8 y;
		u8 z;
		u8 w;
	};
};

template<u32 pSize>
struct microAllocInfo {
	regInfo VF[32];
	regInfo Acc;
	u8 VI[32];
	u8 i;
	u8 q;
	u8 p;
	u8 r;
	u16 info[pSize];// bit 0 = NOP?
					// bit 1 = Read Fd from backup memory?
					// bit 2 = Read Fs from backup memory?
					// bit 3 = Read Ft from backup memory?
					// bit 4 = ACC1 or ACC2?
					// bit 5 = Read Q1/P1 or backup?
					// bit 6 = Write to Q2/P2?
					// bit 7 = Write Fd/Acc/Result to backup memory?
					// bit 8 = Update Status Flags?
					// bit 9 = Update Mac Flags?
					// bit 10 = Used with bit 11 to make a 2-bit key for status/mac flag instance
					// bit 11 = (00 = instance #0, 01 = instance #1, 10 = instance #2, 11 = instance #3)
	u32 curPC;
};

