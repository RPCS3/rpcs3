/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2009  PCSX2 Dev Team
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


#include "PrecompiledHeader.h"
#include "BaseblockEx.h"

BASEBLOCKEX* BaseBlocks::New(u32 startpc, uptr fnptr)
{
	BASEBLOCKEX newblock;
	std::vector<BASEBLOCKEX>::iterator iter;
	memset(&newblock, 0, sizeof newblock);
	newblock.startpc = startpc;
	newblock.fnptr = fnptr;

	int imin = 0, imax = blocks.size(), imid;

	while (imin < imax) {
		imid = (imin+imax)>>1;

		if (blocks[imid].startpc > startpc)
			imax = imid;
		else
			imin = imid + 1;
	}

	pxAssert(imin == blocks.size() || blocks[imin].startpc > startpc);
	iter = blocks.insert(blocks.begin() + imin, newblock);

	std::pair<linkiter_t, linkiter_t> range = links.equal_range(startpc);
	for (linkiter_t i = range.first; i != range.second; ++i)
		*(u32*)i->second = fnptr - (i->second + 4);

	return &*iter;
}

int BaseBlocks::LastIndex(u32 startpc) const
{
	if (0 == blocks.size())
		return -1;

	int imin = 0, imax = blocks.size() - 1, imid;

	while(imin != imax) {
		imid = (imin+imax+1)>>1;

		if (blocks[imid].startpc > startpc)
			imax = imid - 1;
		else
			imin = imid;
	}

	return imin;
}

BASEBLOCKEX* BaseBlocks::GetByX86(uptr ip)
{
	if (0 == blocks.size())
		return 0;

	int imin = 0, imax = blocks.size() - 1, imid;

	while(imin != imax) {
		imid = (imin+imax+1)>>1;

		if (blocks[imid].fnptr > ip)
			imax = imid - 1;
		else
			imin = imid;
	}

	if (ip < blocks[imin].fnptr ||
		ip >= blocks[imin].fnptr + blocks[imin].x86size)
		return 0;

	return &blocks[imin];
}

void BaseBlocks::Link(u32 pc, s32* jumpptr)
{
	BASEBLOCKEX *targetblock = Get(pc);
	if (targetblock && targetblock->startpc == pc)
		*jumpptr = (s32)(targetblock->fnptr - (sptr)(jumpptr + 1));
	else
		*jumpptr = (s32)(recompiler - (sptr)(jumpptr + 1));
	links.insert(std::pair<u32, uptr>(pc, (uptr)jumpptr));
}

