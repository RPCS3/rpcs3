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

void initNewVif(int idx) {
	nVif[idx].idx		= idx;
	nVif[idx].VU		= idx ? &VU1     : &VU0;
	nVif[idx].vif		= idx ? &vif1    : &vif0;
	nVif[idx].vifRegs	= idx ? vif1Regs : vif0Regs;
	nVif[idx].vuMemEnd  = idx ? ((u8*)(VU1.Mem + 0x4000)) : ((u8*)(VU0.Mem + 0x1000));
	nVif[idx].vuMemLimit= idx ? 0x3ff0 : 0xff0;
	nVif[idx].vifCache	= NULL;

	HostSys::MemProtectStatic(nVifUpkExec, Protect_ReadWrite, false);
	memset8<0xcc>( nVifUpkExec );

	xSetPtr( nVifUpkExec );

	for (int a = 0; a < 2; a++) {
	for (int b = 0; b < 2; b++) {
	for (int c = 0; c < 4; c++) {
		nVifGen(a, b, c);
	}}}

	HostSys::MemProtectStatic(nVifUpkExec, Protect_ReadOnly, true);
	if (newVifDynaRec)  dVifInit(idx);
}

int nVifUnpack(int idx, u32 *data) {
	XMMRegisters::Freeze();
	int ret = aMin(vif1.vifpacketsize, vif1.tag.size);
	vif1.tag.size -= ret;
	if (newVifDynaRec)	dVifUnpack(idx, (u8*)data, ret<<2);
	else			   _nVifUnpack(idx, (u8*)data, ret<<2);
	if (vif1.tag.size <= 0) {
		vif1.tag.size  = 0;
		vif1.cmd       = 0;
	}
	XMMRegisters::Thaw();
	return ret;
}

_f u8* setVUptr(int vuidx, const u8* vuMemBase, int offset) {
	return (u8*)(vuMemBase + ( offset & (vuidx ? 0x3ff0 : 0xff0) ));
}

_f void incVUptr(int vuidx, u8* &ptr, const u8* vuMemBase, int amount) {
	pxAssert( ((uptr)ptr & 0xf) == 0 );		// alignment check
	ptr += amount;
	int diff = ptr - (vuMemBase + (vuidx ? 0x4000 : 0x1000));
	if (diff >= 0) {
		ptr = (u8*)(vuMemBase + diff);
	}
}

_f void incVUptrBy16(int vuidx, u8* &ptr, const u8* vuMemBase) {
	pxAssert( ((uptr)ptr & 0xf) == 0 );	// alignment check
	ptr += 16;
	if( ptr == (vuMemBase + (vuidx ? 0x4000 : 0x1000)) )
		ptr -= (vuidx ? 0x4000 : 0x1000);
}

static u32 oldMaskIdx = -1;
static u32 oldMask    =  0;

static void setMasks(int idx, const VIFregisters& v) {
	//if (idx == oldMaskIdx && oldMask == v.mask) return;
	//oldMaskIdx = idx;
	//oldMask	   = v.mask;
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
	//DevCon.WriteLn("[%d][%d][%d][num=%d][upk=%d][cl=%d][bl=%d][skip=%d]", isFill, doMask, doMode, vifRegs->num, upkNum, vif->cl, blockSize, skipSize);

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

	// Cache vuMemBase to a local var because the VU1's is a dereferenced pointer that
	// mucks up compiler optimizations on the internal loops. >_< --air
	const u8* vuMemBase	= (idx ? VU1 : VU0).Mem;
	u8* dest			= setVUptr(idx, vuMemBase, vif->tag.addr);
	if (vif->cl >= blockSize)  vif->cl = 0;

	while (vifRegs->num && size) {
		if (vif->cl < cycleSize) { 
			if (doMode) {
				//DevCon.WriteLn("Non SSE; unpackNum = %d", upkNum);
				func((u32*)dest, (u32*)data);
			}
			else {
				//DevCon.WriteLn("SSE Unpack!");
				fnbase[aMin(vif->cl, 3)](dest, data);
			}
			data += ft.gsize;
			//if( IsDebugBuild ) size -= ft.gsize; // only used below for assertion checking

			vifRegs->num--;
			incVUptrBy16(idx, dest, vuMemBase);
			if (++vif->cl == blockSize) vif->cl = 0;
		}
		else if (isFill) {
			func((u32*)dest, (u32*)data);
			vifRegs->num--;
			incVUptrBy16(idx, dest, vuMemBase);
			if (++vif->cl == blockSize) vif->cl = 0;
		}
		else {
			incVUptr(idx, dest, vuMemBase, 16 * skipSize);
			vif->cl = 0;
		}
	}

	// Spams in many games?  (Suikoden3 / TriAce games, prolly more) Bad news or just wacky VIF hack mess?
	//pxAssert( size == 0, "Mismatched VIFunpack size specified." );
}

typedef void (__fastcall* Fnptr_VifUnpackLoop)(u8 *data, u32 size);

static const __aligned16 Fnptr_VifUnpackLoop UnpackLoopTable[2][2][2] = 
{
	{
		{ _nVifUnpackLoop<0,false,false>, _nVifUnpackLoop<0,false,true> },
		{ _nVifUnpackLoop<0,true, false>, _nVifUnpackLoop<0,true, true> },
	},
	{
		{ _nVifUnpackLoop<1,false,false>, _nVifUnpackLoop<1,false,true> },
		{ _nVifUnpackLoop<1,true, false>, _nVifUnpackLoop<1,true, true> },
	},
	
};

static _f void _nVifUnpack(int idx, u8 *data, u32 size) {

	if (useOldUnpack) {
		if (!idx) VIFunpack<0>((u32*)data, &vif0.tag, size>>2);
		else	  VIFunpack<1>((u32*)data, &vif1.tag, size>>2);
		return;
	}

	vif				  =  nVif[idx].vif;
	vifRegs			  =  nVif[idx].vifRegs;
	const bool doMode =   vifRegs->mode && !(vif->tag.cmd & 0x10);
	const bool isFill =  (vifRegs->cycle.cl < vifRegs->cycle.wl);

	UnpackLoopTable[idx][doMode][isFill]( data, size );

	//if (isFill)
	//DevCon.WriteLn("%s Write! [num = %d][%s]", (isFill?"Filling":"Skipping"), vifRegs->num, (vifRegs->num%3 ? "bad!" : "ok"));
	//DevCon.WriteLn("%s Write! [mask = %08x][type = %02d][num = %d]", (isFill?"Filling":"Skipping"), vifRegs->mask, upkNum, vifRegs->num);	
}
