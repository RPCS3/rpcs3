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

#include "PrecompiledHeader.h"
#include "Common.h"
#include "Vif.h"
#include "Vif_Dma.h"

enum UnpackOffset {
	OFFSET_X = 0,
	OFFSET_Y = 1,
	OFFSET_Z = 2,
	OFFSET_W = 3
};

static __fi u32 setVifRow(vifStruct& vif, u32 reg, u32 data) {
	vif.MaskRow._u32[reg] = data;
	return data;
}

// cycle derives from vif.cl
// mode derives from vifRegs.mode
template< uint idx, uint mode, bool doMask >
static __ri void writeXYZW(u32 offnum, u32 &dest, u32 data) {
	int n = 0;

	vifStruct& vif = GetVifX;

	if (doMask) {
		const VIFregisters& regs = vifXRegs;
		switch (vif.cl) {
			case 0:  n = (regs.mask >> (offnum * 2)) & 0x3;		break;
			case 1:  n = (regs.mask >> ( 8 + (offnum * 2))) & 0x3;	break;
			case 2:  n = (regs.mask >> (16 + (offnum * 2))) & 0x3;	break;
			default: n = (regs.mask >> (24 + (offnum * 2))) & 0x3;	break;
		}
	}

	// Four possible types of masking are handled below:
	//   0 - Data
	//   1 - MaskRow
	//   2 - MaskCol
	//   3 - Write protect

	switch (n) {
		case 0:
			switch (mode) {
				case 1:  dest = data + vif.MaskRow._u32[offnum]; break;
				case 2:  dest = setVifRow(vif, offnum, vif.MaskRow._u32[offnum] + data); break;
				default: dest = data; break;
			}
			break;
		case 1: dest = vif.MaskRow._u32[offnum]; break;
		case 2: dest = vif.MaskCol._u32[min(vif.cl,3)]; break;
		case 3: break;
	}
}
#define tParam idx,mode,doMask

template < uint idx, uint mode, bool doMask, class T >
static void __fastcall UNPACK_S(u32* dest, const T* src)
{
	u32 data = *src;

	//S-# will always be a complete packet, no matter what. So we can skip the offset bits
	writeXYZW<tParam>(OFFSET_X, *(dest+0), data);
	writeXYZW<tParam>(OFFSET_Y, *(dest+1), data);
	writeXYZW<tParam>(OFFSET_Z, *(dest+2), data);
	writeXYZW<tParam>(OFFSET_W, *(dest+3), data);
}

// The PS2 console actually writes v1v0v1v0 for all V2 unpacks -- the second v1v0 pair
// being officially "indeterminate" but some games very much depend on it.
template < uint idx, uint mode, bool doMask, class T >
static void __fastcall UNPACK_V2(u32* dest, const T* src)
{
	writeXYZW<tParam>(OFFSET_X, *(dest+0), *(src+0));
	writeXYZW<tParam>(OFFSET_Y, *(dest+1), *(src+1));
	writeXYZW<tParam>(OFFSET_Z, *(dest+2), *(src+0));
	writeXYZW<tParam>(OFFSET_W, *(dest+3), *(src+1));
}

// V3 and V4 unpacks both use the V4 unpack logic, even though most of the OFFSET_W fields
// during V3 unpacking end up being overwritten by the next unpack.  This is confirmed real
// hardware behavior that games such as Ape Escape 3 depend on.
template < uint idx, uint mode, bool doMask, class T >
static void __fastcall UNPACK_V4(u32* dest, const T* src)
{
	writeXYZW<tParam>(OFFSET_X, *(dest+0), *(src+0));
	writeXYZW<tParam>(OFFSET_Y, *(dest+1), *(src+1));
	writeXYZW<tParam>(OFFSET_Z, *(dest+2), *(src+2));
	writeXYZW<tParam>(OFFSET_W, *(dest+3), *(src+3));
}

// V4_5 unpacks do not support the MODE register, and act as mode==0 always.
template< uint idx, bool doMask >
static void __fastcall UNPACK_V4_5(u32 *dest, const u32* src)
{
	u32 data = *src;

	writeXYZW<idx,0,doMask>(OFFSET_X, *(dest+0),	((data & 0x001f) << 3));
	writeXYZW<idx,0,doMask>(OFFSET_Y, *(dest+1),	((data & 0x03e0) >> 2));
	writeXYZW<idx,0,doMask>(OFFSET_Z, *(dest+2),	((data & 0x7c00) >> 7));
	writeXYZW<idx,0,doMask>(OFFSET_W, *(dest+3),	((data & 0x8000) >> 8));
}

// =====================================================================================================

// --------------------------------------------------------------------------------------
//  Main table for function unpacking.
// --------------------------------------------------------------------------------------
// The extra data bsize/dsize/etc are all duplicated between the doMask enabled and
// disabled versions.  This is probably simpler and more efficient than bothering
// to generate separate tables.
//
// The double-cast function pointer nonsense is to appease GCC, which gives some rather
// cryptic error about being unable to deduce the type parameters (I think it's a bug
// relating to __fastcall, which I recall having some other places as well).  It's fixed
// by explicitly casting the function to itself prior to casting it to what we need it
// to be cast as. --air
//

#define _upk				(UNPACKFUNCTYPE)
#define _unpk(usn, bits)	(UNPACKFUNCTYPE_##usn##bits)

#define UnpackFuncSet( vt, idx, mode, usn, doMask ) \
	(UNPACKFUNCTYPE)_unpk(u,32)		UNPACK_##vt<idx, mode, doMask, u32>, \
	(UNPACKFUNCTYPE)_unpk(usn,16)	UNPACK_##vt<idx, mode, doMask, usn##16>, \
	(UNPACKFUNCTYPE)_unpk(usn,8)	UNPACK_##vt<idx, mode, doMask, usn##8> \

#define UnpackV4_5set(idx, doMask) \
	(UNPACKFUNCTYPE)_unpk(u,32) UNPACK_V4_5<idx, doMask> \

#define UnpackModeSet(idx, mode) \
	UnpackFuncSet( S,  idx, mode, s, 0 ), NULL,  \
	UnpackFuncSet( V2, idx, mode, s, 0 ), NULL,  \
	UnpackFuncSet( V4, idx, mode, s, 0 ), NULL,  \
	UnpackFuncSet( V4, idx, mode, s, 0 ), UnpackV4_5set(idx, 0), \
 \
	UnpackFuncSet( S,  idx, mode, s, 1 ), NULL,  \
	UnpackFuncSet( V2, idx, mode, s, 1 ), NULL,  \
	UnpackFuncSet( V4, idx, mode, s, 1 ), NULL,  \
	UnpackFuncSet( V4, idx, mode, s, 1 ), UnpackV4_5set(idx, 1), \
 \
	UnpackFuncSet( S,  idx, mode, u, 0 ), NULL,  \
	UnpackFuncSet( V2, idx, mode, u, 0 ), NULL,  \
	UnpackFuncSet( V4, idx, mode, u, 0 ), NULL,  \
	UnpackFuncSet( V4, idx, mode, u, 0 ), UnpackV4_5set(idx, 0), \
 \
	UnpackFuncSet( S,  idx, mode, u, 1 ), NULL,  \
	UnpackFuncSet( V2, idx, mode, u, 1 ), NULL,  \
	UnpackFuncSet( V4, idx, mode, u, 1 ), NULL,  \
	UnpackFuncSet( V4, idx, mode, u, 1 ), UnpackV4_5set(idx, 1)

__aligned16 const UNPACKFUNCTYPE VIFfuncTable[2][3][4 * 4 * 2 * 2] =
{
	{
		{ UnpackModeSet(0,0) },
		{ UnpackModeSet(0,1) },
		{ UnpackModeSet(0,2) }
	},

	{
		{ UnpackModeSet(1,0) },
		{ UnpackModeSet(1,1) },
		{ UnpackModeSet(1,2) }
	}
};

//----------------------------------------------------------------------------
// Unpack Setup Code
//----------------------------------------------------------------------------

_vifT void vifUnpackSetup(const u32 *data) {

	vifStruct& vifX = GetVifX;

	if ((vifXRegs.cycle.wl == 0) && (vifXRegs.cycle.wl < vifXRegs.cycle.cl)) {
        DevCon.WriteLn("Vif%d CL %d, WL %d", idx, vifXRegs.cycle.cl, vifXRegs.cycle.wl);
		vifX.cmd = 0;
        return; // Skipping write and 0 write-cycles, so do nothing!
	}

	
	//if (!idx) vif0FLUSH(); // Only VU0?

	vifX.usn   = (vifXRegs.code >> 14) & 0x01;
	int vifNum = (vifXRegs.code >> 16) & 0xff;

	if (vifNum == 0) vifNum = 256;
	vifXRegs.num =  vifNum;

	// Traditional-style way of calculating the gsize, based on VN/VL parameters.
	// Useful when VN/VL are known template params, but currently they are not so we use
	// the LUT instead (for now).
	//uint vl = vifX.cmd & 0x03;
	//uint vn = (vifX.cmd >> 2) & 0x3;
	//uint gsize = ((32 >> vl) * (vn+1)) / 8;

	const u8& gsize = nVifT[vifX.cmd & 0x0f];

	if (vifXRegs.cycle.wl <= vifXRegs.cycle.cl) {
		vifX.tag.size = ((vifNum * gsize) + 3) / 4;
	}
	else {
		int n = vifXRegs.cycle.cl * (vifNum / vifXRegs.cycle.wl) +
		        _limit(vifNum % vifXRegs.cycle.wl, vifXRegs.cycle.cl);

		vifX.tag.size = ((n * gsize) + 3) >> 2;
	}

	u32 addr = vifXRegs.code;
	if (idx && ((addr>>15)&1)) addr += vif1Regs.tops;
	vifX.tag.addr = (addr<<4) & (idx ? 0x3ff0 : 0xff0);

	VIF_LOG("Unpack VIF%x, QWC %x tagsize %x", idx, vifNum, vif0.tag.size);

	vifX.cl			 = 0;
	vifX.tag.cmd	 = vifX.cmd;
}

template void vifUnpackSetup<0>(const u32 *data);
template void vifUnpackSetup<1>(const u32 *data);
