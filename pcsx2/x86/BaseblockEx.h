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

#pragma once

#include "PrecompiledHeader.h"
#include <vector>
#include <map>
#include <utility>

// used to keep block information
#define BLOCKTYPE_DELAYSLOT	1		// if bit set, delay slot

// Every potential jump point in the PS2's addressable memory has a BASEBLOCK
// associated with it. So that means a BASEBLOCK for every 4 bytes of PS2
// addressable memory.  Yay!
struct BASEBLOCK
{
	u32 m_pFnptr;

	const __inline uptr GetFnptr() const { return m_pFnptr; }
	void __inline SetFnptr( uptr ptr ) { m_pFnptr = ptr; }
};

// extra block info (only valid for start of fn)
struct BASEBLOCKEX
{
	u32 startpc;
	uptr fnptr;
	u16 size;	// size in dwords
	u16 x86size;

#ifdef PCSX2_DEVBUILD
	u32 visited; // number of times called
	u64 ltime; // regs it assumes to have set already
#endif

};

class BaseBlocks
{
private:
	// switch to a hash map later?
	std::multimap<u32, uptr> links;
	typedef std::multimap<u32, uptr>::iterator linkiter_t;
	unsigned long size;
	uptr recompiler;
	std::vector<BASEBLOCKEX> blocks;

public:
	BaseBlocks(unsigned long size_, uptr recompiler_) :
		size(size_),
		recompiler(recompiler_),
		blocks(0)
	{
		blocks.reserve(size);
	}

	BASEBLOCKEX* New(u32 startpc, uptr fnptr);
	int LastIndex (u32 startpc) const;
	BASEBLOCKEX* GetByX86(uptr ip) const;

	inline int Index (u32 startpc) const
	{
		int idx = LastIndex(startpc);
		// fixme: I changed the parenthesis to be unambiguous, but this needs to be checked to see if ((x or y or z) and w)
		// is correct, or ((x or y) or (z and w)), or some other variation. --arcum42
		// Mixing &&'s and ||'s is not actually ambiguous; &&'s take precedence.  Reverted to old behavior -- ChickenLiver.
		if ((idx == -1) || (startpc < blocks[idx].startpc) ||
			((blocks[idx].size) && (startpc >= blocks[idx].startpc + blocks[idx].size * 4)))
			return -1;
		else
			return idx;
	}

	inline BASEBLOCKEX* operator[](int idx)
	{
		if (idx < 0 || idx >= (int)blocks.size())
			return 0;
		return &blocks[idx];
	}

	inline BASEBLOCKEX* Get(u32 startpc)
	{
		return (*this)[Index(startpc)];
	}

	inline void Remove(int idx)
	{
		//u32 startpc = blocks[idx].startpc;
		std::pair<linkiter_t, linkiter_t> range = links.equal_range(blocks[idx].startpc);
		for (linkiter_t i = range.first; i != range.second; ++i)
			*(u32*)i->second = recompiler - (i->second + 4);
		// TODO: remove links from this block?
		blocks.erase(blocks.begin() + idx);
	}

	void Link(u32 pc, uptr jumpptr);

	inline void Reset()
	{
		blocks.clear();
		links.clear();
	}
};

#define GET_BLOCKTYPE(b) ((b)->Type)
#define PC_GETBLOCK_(x, reclut) ((BASEBLOCK*)(reclut[((u32)(x)) >> 16] + (x)*(sizeof(BASEBLOCK)/4)))

static void recLUT_SetPage(uptr reclut[0x10000], uptr hwlut[0x10000],
						   BASEBLOCK *mapbase, uint pagebase, uint pageidx, uint mappage)
{
	uint page = pagebase + pageidx;

	jASSUME( page < 0x10000 );
	reclut[page] = (uptr)&mapbase[(mappage - page) << 14];
	if (hwlut)
		hwlut[page] = 0u - (pagebase << 16);
}

C_ASSERT( sizeof(BASEBLOCK) == 4 );