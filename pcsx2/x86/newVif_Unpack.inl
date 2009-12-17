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

// newVif!
// authors: cottonvibes(@gmail.com)
//			Jake.Stine (@gmail.com)

#pragma once

static __aligned16 nVifStruct nVif[2];
static _f void _nVifUnpack(int idx, u8 *data, u32 size);

int nVifUnpack(int idx, u32 *data) {
	XMMRegisters::Freeze();
	int ret = aMin(vif1.vifpacketsize, vif1.tag.size);
	vif1.tag.size -= ret;
	_nVifUnpack(idx, (u8*)data, ret<<2);
	if (vif1.tag.size <= 0) {
		vif1.tag.size  = 0;
		vif1.cmd       = 0;
	}
	XMMRegisters::Thaw();
	return ret;
}

_f u8* setVUptr(int idx, int offset) {
	return (u8*)(nVif[idx].VU->Mem + (offset & nVif[idx].vuMemLimit));
}

_f void incVUptr(int idx, u8* &ptr, int amount) {
	ptr += amount;
	int diff = ptr - nVif[idx].vuMemEnd;
	if (diff >= 0) {
		ptr = nVif[idx].VU->Mem + diff;
	}
	if ((uptr)ptr & 0xf) DevCon.WriteLn("unaligned wtf :(");
}

static u32 oldMaskIdx = -1;
static u32 oldMask    =  0;

static void setMasks(int idx, const VIFregisters& v) {
	if (idx == oldMaskIdx && oldMask == v.mask) return;
	oldMaskIdx = idx;
	oldMask	   = v.mask;
	//DevCon.WriteLn("mask");
	for (int i = 0; i < 16; i++) {
		int m = (v.mask >> (i*2)) & 3;
		switch (m) {
			case 0: // Data
				nVifMask[0][i/4][i%4] = 0xffffffff;
				nVifMask[1][i/4][i%4] = 0;
				nVifMask[2][i/4][i%4] = 0;
				break;
			case 1: // Row
				nVifMask[0][i/4][i%4] = 0;
				nVifMask[1][i/4][i%4] = 0;
				nVifMask[2][i/4][i%4] = ((u32*)&v.r0)[(i%4)*4];
				break;
			case 2: // Col
				nVifMask[0][i/4][i%4] = 0;
				nVifMask[1][i/4][i%4] = 0;
				nVifMask[2][i/4][i%4] = ((u32*)&v.c0)[(i/4)*4];
				break;
			case 3: // Write Protect
				nVifMask[0][i/4][i%4] = 0;
				nVifMask[1][i/4][i%4] = 0xffffffff;
				nVifMask[2][i/4][i%4] = 0;
				break;
		}
	}
}

// ----------------------------------------------------------------------------
//  Unpacking Optimization notes:
// ----------------------------------------------------------------------------
// Some games send a LOT of single-cycle packets (God of War, SotC, TriAce games, etc),
// so we always need to be weary of keeping loop setup code optimized.  It's not always
// a "win" to move code outside the loop, like normally in most other loop scenarios.
//
// The biggest bottleneck of the current code is the call/ret needed to invoke the SSE
// unpackers.  A better option is to generate the entire vifRegs->num loop code as part
// of the SSE template, and inline the SSE code into the heart of it.  This both avoids
// the call/ret and opens the door for resolving some register dependency chains in the
// current emitted functions.  (this is what zero's SSE does to get it's final bit of
// speed advantage over the new vif). --air
//
// As a secondary optimization to above, special handlers could be generated for the
// cycleSize==1 case, which is used frequently enough, and results in enough code
// elimination that it would probably be a win in most cases (and for sure in many
// "slow" games that need it most). --air

template< int idx, bool doMode, bool isFill >
__releaseinline void __fastcall _nVifUnpackLoop(u8 *data, u32 size) {

	const int cycleSize = isFill ? vifRegs->cycle.cl : vifRegs->cycle.wl;
	const int blockSize = isFill ? vifRegs->cycle.wl : vifRegs->cycle.cl;
	const int skipSize  = blockSize - cycleSize;

	//if (skipSize > 2)
	//DevCon.WriteLn("[num = %d][cl = %d][bl = %d][diff = %d]", vifRegs->num, vif->cl, blockSize, skipSize);

	if (vif->cmd & 0x10) setMasks(idx, *vifRegs);

	const int	usn		= !!(vif->usn);
	const int	upkNum	= vif->cmd & 0x1f;
	//const s8&	vift	= nVifT[upkNum];	// might be useful later when other SSE paths are finished.

	// Recompiled Unpacker, used when doMode is false.
	// Did a bunch of work to make it so I could optimize this index lookup to outside
	// the main loop but it was for naught -- too often the loop is only 1-2 iterations,
	// so this setup code ends up being slower (1 iter) or same speed (2 iters).
	const nVifCall*	fnbase			= &nVifUpk[ ((usn*2*16) + upkNum) * (4*1) ];

	// Interpreted Unpacker, used if doMode is true OR if isFill is true.  Lookup is
	// always performed for now, due to ft.gsize reference (seems faster than using
	// nVifT for now)
	const VIFUnpackFuncTable& ft	= VIFfuncTable[upkNum];
	UNPACKFUNCTYPE func				= usn ? ft.funcU : ft.funcS;

	u8* dest = setVUptr(idx, vif->tag.addr);

	if (vif->cl >= blockSize)  vif->cl = 0;

	while (vifRegs->num /*&& size*/) {
		if (vif->cl < cycleSize) { 
			if (doMode /*|| doMask*/) {
				//if (doMask)
				//DevCon.WriteLn("Non SSE; unpackNum = %d", upkNum);
				func((u32*)dest, (u32*)data);
			}
			else {
				//DevCon.WriteLn("SSE Unpack!");
				
				// Opt note: removing this min check (which isn't needed right now?) is +1%
				// or more.  Just something to keep in mind. :) --air
				fnbase[0/*aMin(vif->cl, 4)*/](dest, data);
			}
			data += ft.gsize;
			size -= ft.gsize;
			vifRegs->num--;
			incVUptr(idx, dest, 16);
			if (++vif->cl == blockSize) vif->cl = 0;
		}
		else if (isFill) {
			func((u32*)dest, (u32*)data);
			vifRegs->num--;
			incVUptr(idx, dest, 16);
			if (++vif->cl == blockSize) vif->cl = 0;
		}
		else {
			incVUptr(idx, dest, 16 * skipSize);
			vif->cl = 0;
		}
	}
	//if (size > 0) DevCon.WriteLn("size = %d", size);
}

typedef void (__fastcall* Fnptr_VifUnpackLoop)(u8 *data, u32 size);

static const __aligned16 Fnptr_VifUnpackLoop UnpackLoopTable[2][2][2] = 
{
	{
		{ _nVifUnpackLoop<0,false,false>, _nVifUnpackLoop<0,false,true> },
		{ _nVifUnpackLoop<0,true,false>, _nVifUnpackLoop<0,true,true> },
	},

	{
		{ _nVifUnpackLoop<1,false,false>, _nVifUnpackLoop<1,false,true> },
		{ _nVifUnpackLoop<1,true,false>, _nVifUnpackLoop<1,true,true> },
	},
	
};


static _f void _nVifUnpack(int idx, u8 *data, u32 size) {
	/*if (nVif[idx].vifRegs->cycle.cl >= nVif[idx].vifRegs->cycle.wl) { // skipping write
		if (!idx) VIFunpack<0>((u32*)data, &vif0.tag, size>>2);
		else	  VIFunpack<1>((u32*)data, &vif1.tag, size>>2);
		return;
	}
	else*/ { // filling write

		vif				  =  nVif[idx].vif;
		vifRegs			  =  nVif[idx].vifRegs;
		const bool doMode =   vifRegs->mode && !(vif->tag.cmd & 0x10);
		const bool isFill =  (vifRegs->cycle.cl < vifRegs->cycle.wl);

		UnpackLoopTable[idx][doMode][isFill]( data, size );

		//if (isFill)
		//DevCon.WriteLn("%s Write! [num = %d][%s]", (isFill?"Filling":"Skipping"), vifRegs->num, (vifRegs->num%3 ? "bad!" : "ok"));
		//DevCon.WriteLn("%s Write! [mask = %08x][type = %02d][num = %d]", (isFill?"Filling":"Skipping"), vifRegs->mask, upkNum, vifRegs->num);	
	}
}
