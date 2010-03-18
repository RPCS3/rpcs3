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

#include "PrecompiledHeader.h"
#include "Common.h"
#include "Vif_Dma.h"
#include "newVif.h"
#include "Vif_Unpack.inl"

__aligned16 nVifStruct	nVif[2];
__aligned16 nVifCall	nVifUpk[(2*2*16)  *4];		// ([USN][Masking][Unpack Type]) [curCycle]
__aligned16 u32			nVifMask[3][4][4] = {0};	// [MaskNumber][CycleNumber][Vector]

__aligned16 const u8 nVifT[16] = { 
	4, // S-32
	2, // S-16
	1, // S-8
	0, // ----
	8, // V2-32
	4, // V2-16
	2, // V2-8
	0, // ----
	12,// V3-32
	6, // V3-16
	3, // V3-8
	0, // ----
	16,// V4-32
	8, // V4-16
	4, // V4-8
	2, // V4-5
};

// ----------------------------------------------------------------------------
template< int idx, bool doMode, bool isFill, bool singleUnpack >
__releaseinline void __fastcall _nVifUnpackLoop(u8 *data, u32 size);

typedef void (__fastcall* Fnptr_VifUnpackLoop)(u8 *data, u32 size);

// Unpacks Until 'Num' is 0
static const __aligned16 Fnptr_VifUnpackLoop UnpackLoopTable[2][2][2] = {
	{{ _nVifUnpackLoop<0,0,0,0>, _nVifUnpackLoop<0,0,1,0> },
	{  _nVifUnpackLoop<0,1,0,0>, _nVifUnpackLoop<0,1,1,0> },},
	{{ _nVifUnpackLoop<1,0,0,0>, _nVifUnpackLoop<1,0,1,0> },
	{  _nVifUnpackLoop<1,1,0,0>, _nVifUnpackLoop<1,1,1,0> },},
};

// Unpacks until 1 normal write cycle unpack has been written to VU mem
static const __aligned16 Fnptr_VifUnpackLoop UnpackSingleTable[2][2][2] = {
	{{ _nVifUnpackLoop<0,0,0,1>, _nVifUnpackLoop<0,0,1,1> },
	{  _nVifUnpackLoop<0,1,0,1>, _nVifUnpackLoop<0,1,1,1> },},
	{{ _nVifUnpackLoop<1,0,0,1>, _nVifUnpackLoop<1,0,1,1> },
	{  _nVifUnpackLoop<1,1,0,1>, _nVifUnpackLoop<1,1,1,1> },},
};
// ----------------------------------------------------------------------------

void initNewVif(int idx) {
	nVif[idx].idx			= idx;
	nVif[idx].VU			= idx ? &VU1     : &VU0;
	nVif[idx].vif			= idx ? &vif1    : &vif0;
	nVif[idx].vifRegs		= idx ? vif1Regs : vif0Regs;
	nVif[idx].vuMemEnd		= idx ? ((u8*)(VU1.Mem + 0x4000)) : ((u8*)(VU0.Mem + 0x1000));
	nVif[idx].vuMemLimit	= idx ? 0x3ff0 : 0xff0;
	nVif[idx].vifCache		= NULL;
	nVif[idx].bSize			= 0;
	memzero(nVif[idx].buffer);

	VifUnpackSSE_Init();
	if (newVifDynaRec) dVifInit(idx);
}

void closeNewVif(int idx) { if (newVifDynaRec) dVifClose(idx); }
void resetNewVif(int idx) { closeNewVif(idx); initNewVif(idx); }

static _f u8* setVUptr(int vuidx, const u8* vuMemBase, int offset) {
	return (u8*)(vuMemBase + ( offset & (vuidx ? 0x3ff0 : 0xff0) ));
}

static _f void incVUptr(int vuidx, u8* &ptr, const u8* vuMemBase, int amount) {
	pxAssume( ((uptr)ptr & 0xf) == 0 );		// alignment check
	ptr			  += amount;
	vif->tag.addr += amount;
	int diff = ptr - (vuMemBase + (vuidx ? 0x4000 : 0x1000));
	if (diff >= 0) {
		ptr = (u8*)(vuMemBase + diff);
	}
}

static _f void incVUptrBy16(int vuidx, u8* &ptr, const u8* vuMemBase) {
	pxAssume( ((uptr)ptr & 0xf) == 0 );	// alignment check
	ptr			  += 16;
	vif->tag.addr += 16;
	if( ptr == (vuMemBase + (vuidx ? 0x4000 : 0x1000)) ) {
		ptr -= (vuidx ? 0x4000 : 0x1000);
	}
}

int nVifUnpack(int idx, u8* data) {
	XMMRegisters::Freeze();
	nVifStruct& v = nVif[idx];
	vif		= v.vif;
	vifRegs = v.vifRegs;

	const int  ret    = aMin(vif->vifpacketsize, vif->tag.size);
	const bool isFill = (vifRegs->cycle.cl < vifRegs->cycle.wl);
	s32		   size   = ret << 2;

	if (ret == v.vif->tag.size) { // Full Transfer
		if (v.bSize) { // Last transfer was partial
			memcpy_fast(&v.buffer[v.bSize], data, size);
			v.bSize += size;
			data = v.buffer;
			size = v.bSize;
		}
		if (size > 0 || isFill) {
			if (newVifDynaRec)  dVifUnpack(idx, data, size, isFill);
			else			   _nVifUnpack(idx, data, size, isFill);
		}
		vif->tag.size = 0;
		vif->cmd = 0;
		v.bSize  = 0;
	}
	else { // Partial Transfer
		memcpy_fast(&v.buffer[v.bSize], data, size);
		v.bSize		  += size;
		vif->tag.size -= ret;
	}

	XMMRegisters::Thaw();
	return ret;
}

static void setMasks(int idx, const VIFregisters& v) {
	u32* row = idx ? g_vifmask.Row1 : g_vifmask.Row0;
	u32* col = idx ? g_vifmask.Col1 : g_vifmask.Col0;
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
				nVifMask[2][i/4][i%4] = newVifDynaRec ? row[i%4] : ((u32*)&v.r0)[(i%4)*4];
				break;
			case 2: // Col
				nVifMask[0][i/4][i%4] = 0;
				nVifMask[1][i/4][i%4] = 0;
				nVifMask[2][i/4][i%4] = newVifDynaRec ? col[i/4] : ((u32*)&v.c0)[(i/4)*4];
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

template< int idx, bool doMode, bool isFill, bool singleUnpack >
__releaseinline void __fastcall _nVifUnpackLoop(u8 *data, u32 size) {

	const int cycleSize = isFill ? vifRegs->cycle.cl : vifRegs->cycle.wl;
	const int blockSize = isFill ? vifRegs->cycle.wl : vifRegs->cycle.cl;
	const int skipSize  = blockSize - cycleSize;
	//DevCon.WriteLn("[%d][%d][%d][num=%d][upk=%d][cl=%d][bl=%d][skip=%d]", isFill, doMask, doMode, vifRegs->num, upkNum, vif->cl, blockSize, skipSize);

	if (vif->cmd & 0x10) setMasks(idx, *vifRegs);

	const int	usn		= !!(vif->usn);
	const int	upkNum	= vif->cmd & 0x1f;
	//const s8&	vift	= nVifT[upkNum]; // might be useful later when other SSE paths are finished.
	
	const nVifCall*	fnbase			= &nVifUpk[ ((usn*2*16) + upkNum) * (4*1) ];
	const VIFUnpackFuncTable& ft	= VIFfuncTable[upkNum];
	UNPACKFUNCTYPE func				= usn ? ft.funcU : ft.funcS;
	
	const u8* vuMemBase	= (idx ? VU1 : VU0).Mem;
	u8* dest			= setVUptr(idx, vuMemBase, vif->tag.addr);
	if (vif->cl >= blockSize)  vif->cl = 0;
	
	while (vifRegs->num) {
		if (vif->cl < cycleSize) { 
			if (size < ft.gsize) break;
			if (doMode) {
				//DevCon.WriteLn("Non SSE; unpackNum = %d", upkNum);
				func((u32*)dest, (u32*)data);
			}
			else {
				//DevCon.WriteLn("SSE Unpack!");
				fnbase[aMin(vif->cl, 3)](dest, data);
			}
			data += ft.gsize;
			size -= ft.gsize;
			vifRegs->num--;
			incVUptrBy16(idx, dest, vuMemBase);
			if (++vif->cl == blockSize) vif->cl = 0;
			if (singleUnpack) return;
		}
		else if (isFill) {
			//DevCon.WriteLn("isFill!");
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
}

_f void _nVifUnpack(int idx, u8 *data, u32 size, bool isFill) {

	if (useOldUnpack) {
		if (!idx) VIFunpack<0>((u32*)data, &vif0.tag, size>>2);
		else	  VIFunpack<1>((u32*)data, &vif1.tag, size>>2);
		return;
	}

	const bool doMode = !!vifRegs->mode;
	UnpackLoopTable[idx][doMode][isFill]( data, size );
}

