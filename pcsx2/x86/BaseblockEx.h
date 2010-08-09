/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2010  PCSX2 Dev Team
 *
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <map>			// used by BaseBlockEx
#include <utility>

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
protected:
	typedef std::multimap<u32, uptr>::iterator linkiter_t;

	// switch to a hash map later?
	std::multimap<u32, uptr> links;
	uptr recompiler;
	std::vector<BASEBLOCKEX> blocks;

public:
	BaseBlocks() :
		recompiler( NULL )
	,	blocks(0)
	{
		blocks.reserve(0x4000);
	}

	BaseBlocks(uptr recompiler_) :
		recompiler(recompiler_),
		blocks(0)
	{
		blocks.reserve(0x4000);
	}

	void SetJITCompile( void (*recompiler_)() )
	{
		recompiler = (uptr)recompiler_;
	}

	BASEBLOCKEX* New(u32 startpc, uptr fnptr);
	int LastIndex (u32 startpc) const;
	BASEBLOCKEX* GetByX86(uptr ip);

	__fi int Index (u32 startpc) const
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

	__fi BASEBLOCKEX* operator[](int idx)
	{
		if (idx < 0 || idx >= (int)blocks.size())
			return 0;
		return &blocks[idx];
	}

	__fi BASEBLOCKEX* Get(u32 startpc)
	{
		return (*this)[Index(startpc)];
	}

	__fi void Remove(int idx)
	{
		//u32 startpc = blocks[idx].startpc;
		std::pair<linkiter_t, linkiter_t> range = links.equal_range(blocks[idx].startpc);
		for (linkiter_t i = range.first; i != range.second; ++i)
			*(u32*)i->second = recompiler - (i->second + 4);

		if( IsDevBuild )
		{
			// Clear the first instruction to 0xcc (breakpoint), as a way to assert if some
			// static jumps get left behind to this block.  Note: Do not clear more than the
			// first byte, since this code is called during exception handlers and event handlers
			// both of which expect to be able to return to the recompiled code.

			BASEBLOCKEX effu( blocks[idx] );
			memset( (void*)effu.fnptr, 0xcc, 1 );
		}

		// TODO: remove links from this block?
		blocks.erase(blocks.begin() + idx);
	}

	void Link(u32 pc, s32* jumpptr);

	__fi void Reset()
	{
		blocks.clear();
		links.clear();
	}
};

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
