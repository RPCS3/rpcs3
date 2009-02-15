/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2009  Pcsx2 Team
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

#ifndef _MEMORYCARD_H_
#define _MEMORYCARD_H_

static const int MCD_SIZE = 1024 *  8  * 16;
static const int MC2_SIZE = 1024 * 528 * 16;
 
extern void MemoryCard_Init();
extern void MemoryCard_Shutdown();

extern bool TestMcdIsPresent(int mcd);
extern FILE *LoadMcd(int mcd);
extern void ReadMcd(int mcd, u8 *data, u32 adr, int size);
extern void SaveMcd(int mcd, const u8 *data, u32 adr, int size);
extern void EraseMcd(int mcd, u32 adr);
extern void CreateMcd(const char *mcd);

#if 0		// unused code?
struct McdBlock
{
	s8 Title[48];
	s8 ID[14];
	s8 Name[16];
	int IconCount;
	u16 Icon[16*16*3];
	u8 Flags;
};

void GetMcdBlockInfo(int mcd, int block, McdBlock *info);
#endif

#endif
