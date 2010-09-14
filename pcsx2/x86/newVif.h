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

#include "Vif.h"
#include "VU.h"

#include "x86emitter/x86emitter.h"
using namespace x86Emitter;

#define aMax(x, y) std::max(x,y)
#define aMin(x, y) std::min(x,y)

// newVif_HashBucket.h uses this typedef, so it has to be declared first.
typedef u32  (__fastcall *nVifCall)(void*, const void*);
typedef void (__fastcall *nVifrecCall)(uptr dest, uptr src);

#include "newVif_BlockBuffer.h"
#include "newVif_HashBucket.h"

extern void  mVUmergeRegs(const xRegisterSSE& dest, const xRegisterSSE& src,  int xyzw, bool modXYZW = 0);
extern void _nVifUnpack  (int idx, const u8* data, uint mode, bool isFill);
extern void  dVifReset   (int idx);
extern void  dVifClose   (int idx);
extern void  VifUnpackSSE_Init();

_vifT extern void  dVifUnpack  (const u8* data, bool isFill);

#define VUFT VIFUnpackFuncTable
#define	_v0 0
#define	_v1 0x55
#define	_v2 0xaa
#define	_v3 0xff
#define xmmCol0 xmm2
#define xmmCol1 xmm3
#define xmmCol2 xmm4
#define xmmCol3 xmm5
#define xmmRow  xmm6
#define xmmTemp xmm7

// nVifBlock - Ordered for Hashing; the 'num' field and the lower 6 bits of upkType are
//             used as the hash bucket selector.
//
struct __aligned16 nVifBlock {
	u8   num;		// [00] Num  Field
	u8   upkType;	// [01] Unpack Type [usn*1:mask*1:upk*4]
	u8   mode;		// [02] Mode Field
	u8   cl;		// [03] CL   Field
	u32  mask;		// [04] Mask Field
	u8   wl;		// [08] WL   Field
	u8	 padding[3];// [09] through [11]
	uptr startPtr;	// [12] Start Ptr of RecGen Code
}; // 16 bytes

#define _hSize 0x4000 // [usn*1:mask*1:upk*4:num*8] hash...
#define _cmpS  (sizeof(nVifBlock) - (4))
#define _tParams nVifBlock, _hSize, _cmpS
struct nVifStruct {

	// Buffer for partial transfers (should always be first to ensure alignment)
	// Maximum buffer size is 256 (vifRegs.Num max range) * 16 (quadword)
	__aligned16 u8			buffer[256*16];
	u32						bSize;			// Size of 'buffer'
	u32						bPtr;

	u32						idx;			// VIF0 or VIF1
	u8*						recPtr;			// Cur Pos to recompile to
	u8*						recEnd;			// 'Safe' End of Rec Cache
	BlockBuffer*			vifCache;		// Block Buffer
	HashBucket<_tParams>*	vifBlocks;		// Vif Blocks
	int						numBlocks;		// # of Blocks Recompiled
	
	nVifStruct()
	{
		vifCache	=  NULL;
		vifBlocks	=  NULL;
		numBlocks	=  0;
		recPtr		=  NULL;
		recEnd		=  NULL;
	}
};

extern __aligned16 nVifStruct nVif[2];
extern __aligned16 nVifCall nVifUpk[(2*2*16)*4]; // ([USN][Masking][Unpack Type]) [curCycle]
extern __aligned16 u32		nVifMask[3][4][4];	 // [MaskNumber][CycleNumber][Vector]

static const bool newVifDynaRec = 1; // Use code in newVif_Dynarec.inl
