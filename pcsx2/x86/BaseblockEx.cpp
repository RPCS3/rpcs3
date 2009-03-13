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

#include "PrecompiledHeader.h"
#include "BaseblockEx.h"

BASEBLOCKEX* BaseBlocks::New(u32 startpc)
{
	if (blocks.size() == size)
		return 0;

	BASEBLOCKEX newblock;
	std::vector<BASEBLOCKEX>::iterator iter;
	memset(&newblock, 0, sizeof newblock);
	newblock.startpc = startpc;

	int imin = 0, imax = blocks.size(), imid;

	while (imin < imax) {
		imid = (imin+imax)>>1;

		if (blocks[imid].startpc > startpc)
			imax = imid;
		else
			imin = imid + 1;
	}

	assert(imin == blocks.size() || blocks[imin].startpc > startpc);
	iter = blocks.insert(blocks.begin() + imin, newblock);
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
