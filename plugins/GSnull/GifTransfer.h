/*  GSnull
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

#ifndef __GIF_TRANSFER_H__
#define __GIF_TRANSFER_H__

#include <assert.h>
#include <algorithm>
#include "Registers.h"

enum GIF_FLG
{
	GIF_FLG_PACKED	= 0,
	GIF_FLG_REGLIST	= 1,
	GIF_FLG_IMAGE	= 2,
	GIF_FLG_IMAGE2	= 3
};

//
// GIFTag
union GIFTag
{
	u64 ai64[2];
	u32 ai32[4];

	struct
	{
		u32 NLOOP : 15;
		u32 EOP : 1;
		u32 _PAD1 : 16;
		u32 _PAD2 : 14;
		u32 PRE : 1;
		u32 PRIM : 11;
		u32 FLG : 2; // enum GIF_FLG
		u32 NREG : 4;
		u64 REGS : 64;
	};

	void set(const u32 *data)
	{
		for (int i = 0; i <= 3; i++)
		{
			ai32[i] = data[i];
		}
	}

	GIFTag(u32 *data)
	{
		set(data);
	}

	GIFTag(){ ai64[0] = 0; ai64[1] = 0; }
};

// EE part. Data transfer packet description

typedef struct
{
	u32 mode;
	int reg;
	u64 regs;
	u32 nloop;
	int eop;
	int nreg;
	u32 adonly;
	GIFTag tag;

	void setTag(const u32 *data)
	{
		tag.set(data);

		nloop	= tag.NLOOP;
		eop		= tag.EOP;
		mode	= tag.FLG;
		//adonly = false;

		// Hmm....
		nreg	= tag.NREG << 2;
		if (nreg == 0) nreg = 64;
		regs = tag.REGS;
		reg = 0;
		//if ((nreg == 4) && (regs == GIF_REG_A_D)) adonly = true;

		//      GSLog::Writeln"GIFtag: %8.8lx_%8.8lx_%8.8lx_%8.8lx: EOP=%d, NLOOP=%x, FLG=%x, NREG=%d, PRE=%d",
		//                      data[3], data[2], data[1], data[0],
		//                      path->eop, path->nloop, mode, path->nreg, tag.PRE);
	}

	u32 GetReg()
	{
		return (regs >> reg) & 0xf;
	}

	bool StepReg()
	{
		reg += 4;

		if (reg == nreg)
		{
			reg = 0;
			nloop--;

			if (nloop == 0) return false;
		}

		return true;
	}

} pathInfo;

extern void _GSgifPacket(pathInfo *path, const u32 *pMem);
extern void _GSgifRegList(pathInfo *path, const u32 *pMem);
template<int index> 
extern void _GSgifTransfer(const u32 *pMem, u32 size);
extern GIFRegHandler GIFPackedRegHandlers[];
extern GIFRegHandler GIFRegHandlers[];
#endif // GIFTRANSFER_H_INCLUDED
