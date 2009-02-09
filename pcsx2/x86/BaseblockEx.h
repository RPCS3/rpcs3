/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2008  Pcsx2 Team
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

#ifndef _BASEBLOCKEX_H_
#define _BASEBLOCKEX_H_

// used to keep block information
#define BLOCKTYPE_STARTPC	4		// startpc offset
#define BLOCKTYPE_DELAYSLOT	1		// if bit set, delay slot

// Every potential jump point in the PS2's addressable memory has a BASEBLOCK
// associated with it. So that means a BASEBLOCK for every 4 bytes of PS2
// addressable memory.  Yay!
struct BASEBLOCK
{
	u32 m_pFnptr : 28;
	u32 uType : 4;
	u32 startpc;

	const uptr GetFnptr() const { return ((u32)m_pFnptr)<<4; }
	void SetFnptr( uptr ptr )
	{
		// 16 byte alignments only, please!
		jASSUME( (ptr & 0xf) == 0 );
		m_pFnptr = ptr>>4;
	}
};

// extra block info (only valid for start of fn)
// The only "important" piece of information is size.  Startpc is used as a debug/check
// var to make sure the baseblock is sane.  (and it's used for some FFX hack involving
// a big snake in a sewer, but no one knows if the hack is relevant anymore).
struct BASEBLOCKEX
{
	u16 size; // size in dwords	
	u16 dummy;
	u32 startpc; // for debugging?

#ifdef PCSX2_DEVBUILD
	u32 visited; // number of times called
	LARGE_INTEGER ltime; // regs it assumes to have set already
#endif

};

// This is an asinine macro that bases indexing on sizeof(BASEBLOCK) for no reason. (air)
#define GET_BLOCKTYPE(b) ((b)->Type)
#define PC_GETBLOCK_(x, reclut) ((BASEBLOCK*)(reclut[((u32)(x)) >> 16] + (sizeof(BASEBLOCK)/4)*((x) & 0xffff)))

// This is needed because of the retarded GETBLOCK macro above.
C_ASSERT( sizeof(BASEBLOCK) == 8 );

// 0 - ee, 1 - iop
extern void AddBaseBlockEx(BASEBLOCKEX*, int cpu);
extern void RemoveBaseBlockEx(BASEBLOCKEX*, int cpu);
extern BASEBLOCKEX* GetBaseBlockEx(u32 startpc, int cpu);
extern void ResetBaseBlockEx(int cpu);

extern BASEBLOCKEX** GetAllBaseBlocks(int* pnum, int cpu);


#endif