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

// newVif!
// authors: cottonvibes(@gmail.com)
//			Jake.Stine (@gmail.com)

#include "PrecompiledHeader.h"
#include "Common.h"
#include "Vif_Dma.h"
#include "newVif.h"
#include "MTVU.h"

__aligned16 nVifStruct	nVif[2];

// Interpreter-style SSE unpacks.  Array layout matches the interpreter C unpacks.
//  ([USN][Masking][Unpack Type]) [curCycle]
__aligned16 nVifCall	nVifUpk[(2*2*16)  *4];

// This is used by the interpreted SSE unpacks only.  Recompiled SSE unpacks
// and the interpreted C unpacks use the vif.MaskRow/MaskCol members directly.
//  [MaskNumber][CycleNumber][Vector]
__aligned16 u32			nVifMask[3][4][4] = {0};

// Number of bytes of data in the source stream needed for each vector.
// [equivalent to ((32 >> VL) * (VN+1)) / 8]
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
template< int idx, bool doMode, bool isFill >
__ri void __fastcall _nVifUnpackLoop(const u8* data);

typedef void __fastcall FnType_VifUnpackLoop(const u8* data);
typedef FnType_VifUnpackLoop* Fnptr_VifUnpackLoop;

// Unpacks Until 'Num' is 0
static const __aligned16 Fnptr_VifUnpackLoop UnpackLoopTable[2][2][2] = {
	{{ _nVifUnpackLoop<0,0,0>, _nVifUnpackLoop<0,0,1> },
	{  _nVifUnpackLoop<0,1,0>, _nVifUnpackLoop<0,1,1> },},
	{{ _nVifUnpackLoop<1,0,0>, _nVifUnpackLoop<1,0,1> },
	{  _nVifUnpackLoop<1,1,0>, _nVifUnpackLoop<1,1,1> },},
};
// ----------------------------------------------------------------------------

nVifStruct::nVifStruct()
{
	vifBlocks	=  NULL;
	numBlocks	=  0;

	recReserveSizeMB = 8;
}

void reserveNewVif(int idx)
{
}

void resetNewVif(int idx)
{
	// Safety Reset : Reassign all VIF structure info, just in case the VU1 pointers have
	// changed for some reason.

	nVif[idx].idx   = idx;
	nVif[idx].bSize = 0;
	memzero(nVif[idx].buffer);

	if (newVifDynaRec) dVifReset(idx);
}

void closeNewVif(int idx) {
}

void releaseNewVif(int idx) {
}

static __fi u8* getVUptr(uint idx, int offset) {
	return (u8*)(vuRegs[idx].Mem + ( offset & (idx ? 0x3ff0 : 0xff0) ));
}


_vifT int nVifUnpack(const u8* data) {
	nVifStruct&   v       = nVif[idx];
	vifStruct&    vif     = GetVifX;
	VIFregisters& vifRegs = vifXRegs;

	const uint ret    = aMin(vif.vifpacketsize, vif.tag.size);
	const bool isFill = (vifRegs.cycle.cl < vifRegs.cycle.wl);
	s32		   size   = ret << 2;

	if (ret == vif.tag.size) { // Full Transfer
		if (v.bSize) { // Last transfer was partial
			memcpy_fast(&v.buffer[v.bSize], data, size);
			v.bSize		+= size;
			size        = v.bSize;
			data		= v.buffer;

			vif.cl		= 0;
			vifRegs.num	= (vifXRegs.code >> 16) & 0xff;		// grab NUM form the original VIFcode input.
			if (!vifRegs.num) vifRegs.num = 256;
		}

		if (!idx || !THREAD_VU1) {
			if (newVifDynaRec)	dVifUnpack<idx>(data, isFill);
			else			   _nVifUnpack(idx, data, vifRegs.mode, isFill);
		}
		else vu1Thread.VifUnpack(vif, vifRegs, (u8*)data, size);

		vif.pass		= 0;
		vif.tag.size	= 0;
		vif.cmd			= 0;
		vifRegs.num		= 0;
		v.bSize			= 0;
	}
	else { // Partial Transfer
		memcpy_fast(&v.buffer[v.bSize], data, size);
		v.bSize		 += size;
		vif.tag.size -= ret;

		const u8&	vSize	= nVifT[vif.cmd & 0x0f];

		// We need to provide accurate accounting of the NUM register, in case games decided
		// to read back from it mid-transfer.  Since so few games actually use partial transfers
		// of VIF unpacks, this code should not be any bottleneck.

		// We can optimize the calculation either way as some games have big partial chunks (Guitar Hero). 
		// Skipping writes are easy, filling is a bit more complex, so for now until we can 
		// be sure its right (if it happens) it just prints debug stuff and processes the old way.
		if (!isFill) {
			vifRegs.num -= (size / vSize);
		}
		else {
			int guessedsize = (size / vSize);
			guessedsize = vifRegs.num - (((guessedsize / vifRegs.cycle.cl) * (vifRegs.cycle.wl - vifRegs.cycle.cl)) + guessedsize);

			while (size >= vSize) {
				--vifRegs.num;
				++vif.cl;

				if (isFill) {
					if (vif.cl <= vifRegs.cycle.cl)			size -= vSize;
					else if (vif.cl == vifRegs.cycle.wl)	vif.cl = 0;
				}
				else {
					size -= vSize;
					if (vif.cl >= vifRegs.cycle.wl) vif.cl = 0;
				}
			}
			DevCon.Warning("Fill!! Partial num left = %x, guessed %x", vifRegs.num, guessedsize);
		}
	}

	return ret;
}

template int nVifUnpack<0>(const u8* data);
template int nVifUnpack<1>(const u8* data);

// This is used by the interpreted SSE unpacks only.  Recompiled SSE unpacks
// and the interpreted C unpacks use the vif.MaskRow/MaskCol members directly.
static void setMasks(const vifStruct& vif, const VIFregisters& v) {
	for (int i = 0; i < 16; i++) {
		int m = (v.mask >> (i*2)) & 3;
		switch (m) {
			case 0: // Data
				nVifMask[0][i/4][i%4] = 0xffffffff;
				nVifMask[1][i/4][i%4] = 0;
				nVifMask[2][i/4][i%4] = 0;
				break;
			case 1: // MaskRow
				nVifMask[0][i/4][i%4] = 0;
				nVifMask[1][i/4][i%4] = 0;
				nVifMask[2][i/4][i%4] = vif.MaskRow._u32[i%4];
				break;
			case 2: // MaskCol
				nVifMask[0][i/4][i%4] = 0;
				nVifMask[1][i/4][i%4] = 0;
				nVifMask[2][i/4][i%4] = vif.MaskCol._u32[i/4];
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
// unpackers.  A better option is to generate the entire vifRegs.num loop code as part
// of the SSE template, and inline the SSE code into the heart of it.  This both avoids
// the call/ret and opens the door for resolving some register dependency chains in the
// current emitted functions.  (this is what zero's SSE does to get it's final bit of
// speed advantage over the new vif). --air
//
// The BEST optimizatin strategy here is to use data available to us from the UNPACK dispatch
// -- namely the unpack type and mask flag -- in combination mode and usn values -- to
// generate ~600 special versions of this function.  But since it's an interpreter, who gives
// a crap?  Really? :p
//

// size - size of the packet fragment incoming from DMAC.
template< int idx, bool doMode, bool isFill >
__ri void __fastcall _nVifUnpackLoop(const u8* data) {

	vifStruct&    vif     = MTVU_VifX;
	VIFregisters& vifRegs = MTVU_VifXRegs;

	// skipSize used for skipping writes only
	const int skipSize  = (vifRegs.cycle.cl - vifRegs.cycle.wl) * 16;

	//DevCon.WriteLn("[%d][%d][%d][num=%d][upk=%d][cl=%d][bl=%d][skip=%d]", isFill, doMask, doMode, vifRegs.num, upkNum, vif.cl, blockSize, skipSize);

	if (!doMode && (vif.cmd & 0x10)) setMasks(vif, vifRegs);

	const int	usn		= !!vif.usn;
	const int	upkNum	= vif.cmd & 0x1f;
	const u8&	vSize	= nVifT[upkNum & 0x0f];
	//uint vl = vif.cmd & 0x03;
	//uint vn = (vif.cmd >> 2) & 0x3;
	//uint vSize = ((32 >> vl) * (vn+1)) / 8;		// size of data (in bytes) used for each write cycle

	const nVifCall*	fnbase  = &nVifUpk[ ((usn*2*16) + upkNum) * (4*1) ];
	const UNPACKFUNCTYPE ft = VIFfuncTable[idx][doMode ? vifRegs.mode : 0][ ((usn*2*16) + upkNum) ];

	pxAssume (vif.cl == 0);
	pxAssume (vifRegs.cycle.wl > 0);

	do {
		u8* dest = getVUptr(idx, vif.tag.addr);

		if (doMode) {
		//if (1) {
			ft(dest, data);
		}
		else {
			//DevCon.WriteLn("SSE Unpack!");
			uint cl3 = aMin(vif.cl,3);
			fnbase[cl3](dest, data);
		}

		vif.tag.addr += 16;
		--vifRegs.num;
		++vif.cl;

		if (isFill) {
			//DevCon.WriteLn("isFill!");
			if (vif.cl <= vifRegs.cycle.cl)			data += vSize;
			else if (vif.cl == vifRegs.cycle.wl)	vif.cl = 0;
		}
		else
		{
			data += vSize;

			if (vif.cl >= vifRegs.cycle.wl) {
				vif.tag.addr += skipSize;
				vif.cl = 0;
			}
		}
	} while (vifRegs.num);
}

__fi void _nVifUnpack(int idx, const u8* data, uint mode, bool isFill) {

	UnpackLoopTable[idx][!!mode][isFill]( data );
}

