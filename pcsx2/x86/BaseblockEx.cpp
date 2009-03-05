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

#define _SECURE_SCL 0

#include <vector>

using namespace std;

struct BASEBLOCKS
{
	// 0 - ee, 1 - iop
	void Add(BASEBLOCKEX*);
	void Remove(BASEBLOCKEX*);
	int Get(u32 startpc);
	void Reset();

	BASEBLOCKEX** GetAll(int* pnum);

	vector<BASEBLOCKEX*> blocks;
};

void BASEBLOCKS::Add(BASEBLOCKEX* pex)
{
	assert( pex != NULL );

	switch(blocks.size()) {
		case 0:
			blocks.push_back(pex);
			return;
		case 1:
			assert( blocks.front()->startpc != pex->startpc );

			if( blocks.front()->startpc < pex->startpc ) {
				blocks.push_back(pex);
			}
			else blocks.insert(blocks.begin(), pex);

			return;

		default:
		{
			int imin = 0, imax = blocks.size(), imid;

			while(imin < imax) {
				imid = (imin+imax)>>1;

				if( blocks[imid]->startpc > pex->startpc ) imax = imid;
				else imin = imid+1;
			}

			assert( imin == blocks.size() || blocks[imin]->startpc > pex->startpc );
			blocks.insert(blocks.begin()+imin, pex);

			return;
		}
	}
}

int BASEBLOCKS::Get(u32 startpc)
{
	switch(blocks.size()) {
		case 0:
			return -1;
		case 1:
			if (blocks.front()->startpc + blocks.front()->size*4 <= startpc)
				return -1;
			else
				return 0;
		/*case 2:
			return blocks.front()->startpc < startpc;*/

		default:
		{
			int imin = 0, imax = blocks.size()-1, imid;

			while(imin < imax) {
				imid = (imin+imax)>>1;

				if( blocks[imid]->startpc > startpc ) imax = imid;
				else if( blocks[imid]->startpc == startpc ) return imid;
				else imin = imid+1;
			}

			//assert( blocks[imin]->startpc == startpc );
			if (startpc < blocks[imin]->startpc ||
				startpc >= blocks[imin]->startpc + blocks[imin]->size*4)
				return -1;
			else
				return imin;
		}
	}
}

void BASEBLOCKS::Remove(BASEBLOCKEX* pex)
{
	assert( pex != NULL );
	int i = Get(pex->startpc);
	assert( blocks[i] == pex ); 
	blocks.erase(blocks.begin()+i);
}

void BASEBLOCKS::Reset()
{
	blocks.resize(0);
	blocks.reserve(512);
}

BASEBLOCKEX** BASEBLOCKS::GetAll(int* pnum)
{
	assert( pnum != NULL );
	*pnum = blocks.size();
	return &blocks[0];
}

static BASEBLOCKS s_vecBaseBlocksEx[2];

void AddBaseBlockEx(BASEBLOCKEX* pex, int cpu)
{
	s_vecBaseBlocksEx[cpu].Add(pex);
}

BASEBLOCKEX* GetBaseBlockEx(u32 startpc, int cpu)
{
	int i = s_vecBaseBlocksEx[cpu].Get(startpc);
	if (i < 0)
		return 0;
	else
		return s_vecBaseBlocksEx[cpu].blocks[i];
}

void RemoveBaseBlockEx(BASEBLOCKEX* pex, int cpu)
{
	s_vecBaseBlocksEx[cpu].Remove(pex);
}

void ResetBaseBlockEx(int cpu)
{
	s_vecBaseBlocksEx[cpu].Reset();
}

BASEBLOCKEX** GetAllBaseBlocks(int* pnum, int cpu)
{
	return s_vecBaseBlocksEx[cpu].GetAll(pnum);
}
