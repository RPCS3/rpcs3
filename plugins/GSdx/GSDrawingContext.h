/*
 *	Copyright (C) 2007-2009 Gabest
 *	http://www.gabest.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once

#include "GS.h"
#include "GSLocalMemory.h"

__aligned(class, 32) GSDrawingContext
{
public:
	GIFRegXYOFFSET	XYOFFSET;
	GIFRegTEX0		TEX0;
	GIFRegTEX1		TEX1;
	GIFRegTEX2		TEX2;
	GIFRegCLAMP		CLAMP;
	GIFRegMIPTBP1	MIPTBP1;
	GIFRegMIPTBP2	MIPTBP2;
	GIFRegSCISSOR	SCISSOR;
	GIFRegALPHA		ALPHA;
	GIFRegTEST		TEST;
	GIFRegFBA		FBA;
	GIFRegFRAME		FRAME;
	GIFRegZBUF		ZBUF;

	struct
	{
		GSVector4 in;
		GSVector4 ofex;
		uint32 ofxy;
	} scissor;

	struct
	{
		GSOffset* fb;
		GSOffset* zb;
		GSOffset* tex;
		GSPixelOffset* fzb;
		GSPixelOffset4* fzb4;
	} offset;

	GSDrawingContext()
	{
		memset(&offset, 0, sizeof(offset));

		Reset();
	}

	void Reset()
	{
		memset(&XYOFFSET, 0, sizeof(XYOFFSET));
		memset(&TEX0, 0, sizeof(TEX0));
		memset(&TEX1, 0, sizeof(TEX1));
		memset(&TEX2, 0, sizeof(TEX2));
		memset(&CLAMP, 0, sizeof(CLAMP));
		memset(&MIPTBP1, 0, sizeof(MIPTBP1));
		memset(&MIPTBP2, 0, sizeof(MIPTBP2));
		memset(&SCISSOR, 0, sizeof(SCISSOR));
		memset(&ALPHA, 0, sizeof(ALPHA));
		memset(&TEST, 0, sizeof(TEST));
		memset(&FBA, 0, sizeof(FBA));
		memset(&FRAME, 0, sizeof(FRAME));
		memset(&ZBUF, 0, sizeof(ZBUF));
	}

	void UpdateScissor()
	{
		scissor.ofex = GSVector4(
			(int)((SCISSOR.SCAX0 << 4) + XYOFFSET.OFX),
			(int)((SCISSOR.SCAY0 << 4) + XYOFFSET.OFY),
			(int)((SCISSOR.SCAX1 << 4) + XYOFFSET.OFX),
			(int)((SCISSOR.SCAY1 << 4) + XYOFFSET.OFY));

		scissor.in = GSVector4(
			(int)SCISSOR.SCAX0,
			(int)SCISSOR.SCAY0,
			(int)SCISSOR.SCAX1 + 1,
			(int)SCISSOR.SCAY1 + 1);

		uint16 ofx = (uint16)XYOFFSET.OFX - 15;
		uint16 ofy = (uint16)XYOFFSET.OFY - 15;

		scissor.ofxy = ((ofy << 16) | ofx); // ceil(xy) => (xy - offset + 15) >> 4 => (xy - [offset - 15]) >> 4
	}

	bool DepthRead() const
	{
		return TEST.ZTE && TEST.ZTST >= 2;
	}

	bool DepthWrite() const
	{
		if(TEST.ATE && TEST.ATST == ATST_NEVER && TEST.AFAIL != AFAIL_ZB_ONLY) // alpha test, all pixels fail, z buffer is not updated
		{
			return false;
		}

		return ZBUF.ZMSK == 0 && TEST.ZTE != 0; // ZTE == 0 is bug on the real hardware, write is blocked then
	}
};
