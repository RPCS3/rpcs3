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

static __fi u32 setVifRowRegs(u32 reg, u32 data) {
	switch (reg) {
		case 0: vifRegs->r0 = data; break;
		case 1: vifRegs->r1 = data; break;
		case 2: vifRegs->r2 = data; break;
		case 3: vifRegs->r3 = data; break;
		jNO_DEFAULT;
	}
	return data;
}

static __fi u32 getVifRowRegs(u32 reg) {
	switch (reg) {
		case 0: return vifRegs->r0; break;
		case 1: return vifRegs->r1; break;
		case 2: return vifRegs->r2; break;
		case 3: return vifRegs->r3; break;
		jNO_DEFAULT;
	}
	return 0; // unreachable...
}

static __fi u32 getVifColRegs(u32 reg) {
	switch (reg) {
		case 0:  return vifRegs->c0; break;
		case 1:  return vifRegs->c1; break;
		case 2:  return vifRegs->c2; break;
		default: return vifRegs->c3; break;
	}
	return 0; // unreachable...
}

template< bool doMask >
static __ri void writeXYZW(u32 offnum, u32 &dest, u32 data) {
	u32 vifRowReg = getVifRowRegs(offnum);
	int n = 0;

	if (doMask) {
		switch (vif->cl) {
			case 0:  n = (vifRegs->mask >> (offnum * 2)) & 0x3;		   break;
			case 1:  n = (vifRegs->mask >> ( 8 + (offnum * 2))) & 0x3; break;
			case 2:  n = (vifRegs->mask >> (16 + (offnum * 2))) & 0x3; break;
			default: n = (vifRegs->mask >> (24 + (offnum * 2))) & 0x3; break;
		}
	}

	switch (n) {
		case 0:
			if ((vif->cmd & 0x6F) != 0x6f) {
				switch (vifRegs->mode) {
					case 1:  dest = data + vifRowReg; break;
					case 2:  dest = setVifRowRegs(offnum, vifRowReg + data); break;
					default: dest = data; break;
				}
			}
			else dest = data; // v4-5 Unpack Mode
			break;
		case 1: dest = vifRowReg; break;
		case 2: dest = getVifColRegs(vif->cl); break;
		case 3: break;
	}
}

template < bool doMask, class T >
static void __fastcall UNPACK_S(u32 *dest, const T *data)
{
	//S-# will always be a complete packet, no matter what. So we can skip the offset bits
	writeXYZW<doMask>(OFFSET_X, *dest++, *data);
	writeXYZW<doMask>(OFFSET_Y, *dest++, *data);
	writeXYZW<doMask>(OFFSET_Z, *dest++, *data);
	writeXYZW<doMask>(OFFSET_W, *dest  , *data);
}

// The PS2 console actually writes v1v0v1v0 for all V2 unpacks -- the second v1v0 pair
// being officially "indeterminate" but some games very much depend on it.
template <bool doMask, class T>
static void __fastcall UNPACK_V2(u32 *dest, const T *data)
{
	writeXYZW<doMask>(0, *dest++, *data);
	writeXYZW<doMask>(1, *dest++, *(data+1));
	writeXYZW<doMask>(2, *dest++, *data);
	writeXYZW<doMask>(3, *dest++, *(data+1));
}

// V3 and V4 unpacks both use the V4 unpack logic, even though most of the OFFSET_W fields
// during V3 unpacking end up being overwritten by the next unpack.  This is confirmed real
// hardware behavior that games such as Ape Escape 3 depend on.
template <bool doMask, class T>
static void __fastcall UNPACK_V4(u32 *dest, const T *data)
{
	writeXYZW<doMask>(OFFSET_X, *dest++, *data++);
	writeXYZW<doMask>(OFFSET_Y, *dest++, *data++);
	writeXYZW<doMask>(OFFSET_Z, *dest++, *data++);
	writeXYZW<doMask>(OFFSET_W, *dest  , *data);
}

template< bool doMask >
static void __fastcall UNPACK_V4_5(u32 *dest, const u32 *data)
{
	//As with S-#, this will always be a complete packet
	writeXYZW<doMask>(OFFSET_X, *dest++,	((*data & 0x001f) << 3));
	writeXYZW<doMask>(OFFSET_Y, *dest++,	((*data & 0x03e0) >> 2));
	writeXYZW<doMask>(OFFSET_Z, *dest++,	((*data & 0x7c00) >> 7));
	writeXYZW<doMask>(OFFSET_W, *dest,		((*data & 0x8000) >> 8));
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

#define _upk			(UNPACKFUNCTYPE)
#define _odd			(UNPACKFUNCTYPE_ODD)
#define _unpk_s(bits)	(UNPACKFUNCTYPE_S##bits)
#define _unpk_u(bits)	(UNPACKFUNCTYPE_U##bits)

// 32-bits versions are unsigned-only!!
#define UnpackFuncPair32( vt, doMask ) \
	(UNPACKFUNCTYPE)_unpk_u(32) UNPACK_##vt<doMask, u32>, \
	(UNPACKFUNCTYPE)_unpk_u(32) UNPACK_##vt<doMask, u32>
	
#define UnpackFuncPair( vt, bits, doMask ) \
	(UNPACKFUNCTYPE)_unpk_u(bits) UNPACK_##vt<doMask, u##bits>, \
	(UNPACKFUNCTYPE)_unpk_s(bits) UNPACK_##vt<doMask, s##bits>

#define UnpackFuncSet( doMask )												  \
 	{	UnpackFuncPair32( S,		doMask ),   4, 4 },		/* 0x0 - S-32  */ \
	{	UnpackFuncPair	( S,  16,	doMask ),   2, 4 },		/* 0x1 - S-16  */ \
	{	UnpackFuncPair	( S,   8,	doMask ),   1, 4 },		/* 0x2 - S-8   */ \
	{	NULL, NULL, 0, 0 },									/* 0x3 (NULL)  */ \
	{	UnpackFuncPair32( V2,		doMask ),   8, 2 },		/* 0x4 - V2-32 */ \
	{	UnpackFuncPair	( V2, 16,	doMask ),   4, 2 },		/* 0x5 - V2-16 */ \
	{	UnpackFuncPair	( V2,  8,	doMask ),   2, 2 },		/* 0x6 - V2-8  */ \
	{	NULL, NULL, 0, 0 },									/* 0x7 (NULL)  */ \
	{	UnpackFuncPair32( V4,		doMask ),  12, 3 },		/* 0x8 - V3-32 */ \
	{	UnpackFuncPair	( V4, 16,	doMask ),   6, 3 },		/* 0x9 - V3-16 */ \
	{	UnpackFuncPair	( V4,  8,	doMask ),   3, 3 },		/* 0xA - V3-8  */ \
	{	NULL, NULL, 0, 0 },									/* 0xB (NULL)  */ \
	{	UnpackFuncPair32( V4,		doMask ),  16, 4 },		/* 0xC - V4-32 */ \
	{	UnpackFuncPair	( V4, 16,	doMask ),   8, 4 },		/* 0xD - V4-16 */ \
	{	UnpackFuncPair	( V4,  8,	doMask ),   4, 4 },		/* 0xE - V4-8  */ \
	{														/* 0xF - V4-5  */ \
		(UNPACKFUNCTYPE)_unpk_u(32)UNPACK_V4_5<doMask>,	\
		(UNPACKFUNCTYPE)_unpk_u(32)UNPACK_V4_5<doMask>,	\
		2, 4 \
	},

const __aligned16 VIFUnpackFuncTable VIFfuncTable[32] =
{
	UnpackFuncSet( false )
	UnpackFuncSet( true )
};

//----------------------------------------------------------------------------
// Unpack Setup Code
//----------------------------------------------------------------------------

_vifT void vifUnpackSetup(const u32 *data) {

	vifStruct& vifX = GetVifX;

	if ((vifXRegs.cycle.wl == 0) && (vifXRegs.cycle.wl < vifXRegs.cycle.cl)) {
        Console.WriteLn("Vif%d CL %d, WL %d", idx, vifXRegs.cycle.cl, vifXRegs.cycle.wl);
		vifX.cmd = 0;
        return; // Skipping write and 0 write-cycles, so do nothing!
	}

	
	//if (!idx) vif0FLUSH(); // Only VU0?

	vifX.usn   = (vifXRegs.code >> 14) & 0x01;
	int vifNum = (vifXRegs.code >> 16) & 0xff;

	if (vifNum == 0) vifNum = 256;
	vifXRegs.num =  vifNum;

	if (vifXRegs.cycle.wl <= vifXRegs.cycle.cl) {
		if (!idx) vif0.tag.size = ((vifNum * VIFfuncTable[ vif0.cmd & 0xf ].gsize) + 3) >> 2;
		else	  vif1.tag.size = ((vifNum * VIFfuncTable[ vif1.cmd & 0xf ].gsize) + 3) >> 2;
	}
	else {
		int n = vifXRegs.cycle.cl * (vifNum / vifXRegs.cycle.wl) +
		        _limit(vifNum % vifXRegs.cycle.wl, vifXRegs.cycle.cl);

		if (!idx) vif0.tag.size = ((n * VIFfuncTable[ vif0.cmd & 0xf ].gsize) + 3) >> 2;
		else	  vif1.tag.size = ((n * VIFfuncTable[ vif1.cmd & 0xf ].gsize) + 3) >> 2;
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
