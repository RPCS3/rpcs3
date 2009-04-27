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
 
class MemoryCard
{
protected:
	static wxFile cardfile[2];
	
public:
	static void Load( uint mcdId );
	static void Seek( wxFile& mcdfp, u32 adr );
	static void Create( const wxString& mcd );

public:
	static void Init();
	static void Shutdown();
	static void Unload( uint mcd );

	static bool IsPresent( uint mcdId );
	static void Read( uint mcdId, u8 *data, u32 adr, int size );
	static void Save( uint mcdId, const u8 *data, u32 adr, int size );
	static void Erase( uint mcdId, u32 adr );
	
	static bool HasChanged( uint mcdId );
	static u64 GetCRC( uint mcdId );
};

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
