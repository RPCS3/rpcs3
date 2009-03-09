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
					// bit 1 = Used with bit 2 to make a 2-bit key for ACC write instance
					// bit 2 = (00 = instance #0, 01 = instance #1, 10 = instance #2, 11 = instance #3)
					// bit 3 = Used with bit 4 to make a 2-bit key for ACC read instance
					// bit 4 = (00 = instance #0, 01 = instance #1, 10 = instance #2, 11 = instance #3)
					// bit 5 = Write to Q1 or Q2?
					// bit 6 = Read Q1 or Q2?
					// bit 7 = Read/Write to P1 or P2?
					// bit 8 = Update Mac Flags?
					// bit 9 = Update Status Flags?
					// bit 10 = Used with bit 11 to make a 2-bit key for mac flag instance
					// bit 11 = (00 = instance #0, 01 = instance #1, 10 = instance #2, 11 = instance #3)
					// bit 12 = Used with bit 13 to make a 2-bit key for status flag instance
					// bit 13 = (00 = instance #0, 01 = instance #1, 10 = instance #2, 11 = instance #3)
					// bit 14 = Read VI(Fs) from backup memory?
					// bit 15 = Read VI(Ft) from backup memory?
	u32 curPC;
};

