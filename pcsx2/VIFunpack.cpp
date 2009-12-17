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


#include "PrecompiledHeader.h"
#include "Common.h"

#include <cmath>

#include "Vif.h"
#include "VifDma_internal.h"

enum UnpackOffset
{
	OFFSET_X = 0,
	OFFSET_Y = 1,
	OFFSET_Z = 2,
	OFFSET_W = 3
};

static __forceinline u32 setVifRowRegs(u32 reg, u32 data)
{
	switch (reg)
	{
		case 0:
			vifRegs->r0 = data;
			break;
		case 1:
			vifRegs->r1 = data;
			break;
		case 2:
			vifRegs->r2 = data;
			break;
		case 3:
			vifRegs->r3 = data;
			break;
			jNO_DEFAULT;
	}
	return data;
}

static __forceinline u32 getVifRowRegs(u32 reg)
{
	switch (reg)
	{
		case 0:
			return vifRegs->r0;
			break;
		case 1:
			return vifRegs->r1;
			break;
		case 2:
			return vifRegs->r2;
			break;
		case 3:
			return vifRegs->r3;
			break;
			jNO_DEFAULT;
	}
	return 0;	// unreachable...
}

static __forceinline u32 setVifColRegs(u32 reg, u32 data)
{
	switch (reg)
	{
		case 0:
			vifRegs->c0 = data;
			break;
		case 1:
			vifRegs->c1 = data;
			break;
		case 2:
			vifRegs->c2 = data;
			break;
		case 3:
			vifRegs->c3 = data;
			break;
			jNO_DEFAULT;
	}
	return data;
}

static __forceinline u32 getVifColRegs(u32 reg)
{
	switch (reg)
	{
		case 0:
			return vifRegs->c0;
			break;
		case 1:
			return vifRegs->c1;
			break;
		case 2:
			return vifRegs->c2;
			break;
		case 3:
			return vifRegs->c3;
			break;
			jNO_DEFAULT;
	}
	return 0;	// unreachable...
}

template< bool doMask >
static __releaseinline void writeXYZW(u32 offnum, u32 &dest, u32 data)
{
	int n;
	u32 vifRowReg = getVifRowRegs(offnum);

	if (doMask)
	{
		switch (vif->cl)
		{
			case 0:
				if (offnum == OFFSET_X)
					n = (vifRegs->mask) & 0x3;
				else
					n = (vifRegs->mask >> (offnum * 2)) & 0x3;
				break;
			case 1:
				n = (vifRegs->mask >> ( 8 + (offnum * 2))) & 0x3;
				break;
			case 2:
				n = (vifRegs->mask >> (16 + (offnum * 2))) & 0x3;
				break;
			default:
				n = (vifRegs->mask >> (24 + (offnum * 2))) & 0x3;
				break;
		}
	}
	else n = 0;

	switch (n)
	{
		case 0:
			if ((vif->cmd & 0x6F) == 0x6f)
			{
				dest = data;
			}
			else switch (vifRegs->mode)
			{
				case 1:
					dest = data + vifRowReg;
					break;
				case 2:
					// vifRowReg isn't used after this, or I would make it equal to dest here.
					dest = setVifRowRegs(offnum, vifRowReg + data);
					break;
				default:
					dest = data;
					break;
			}
			break;
		case 1:
			dest = vifRowReg;
			break;
		case 2:
			dest = getVifColRegs((vif->cl > 2) ? 3 : vif->cl);
			break;
		case 3:
			break;
	}
//	VIF_LOG("writeX %8.8x : Mode %d, r0 = %x, data %8.8x", *dest,vifRegs->mode,vifRegs->r0,data);
}

template < bool doMask, class T >
static __forceinline void __fastcall UNPACK_S(u32 *dest, T *data, int size)
{
	//S-# will always be a complete packet, no matter what. So we can skip the offset bits
	writeXYZW<doMask>(OFFSET_X, *dest++, *data);
	writeXYZW<doMask>(OFFSET_Y, *dest++, *data);
	writeXYZW<doMask>(OFFSET_Z, *dest++, *data);
	writeXYZW<doMask>(OFFSET_W, *dest  , *data);
}

template <bool doMask, class T>
static __forceinline void __fastcall UNPACK_V2(u32 *dest, T *data, int size)
{
	if (vifRegs->offset == OFFSET_X)
	{
		if (size > 0)
		{
			writeXYZW<doMask>(vifRegs->offset, *dest++, *data++);
			vifRegs->offset = OFFSET_Y;
			size--;
		}
	}

	if (vifRegs->offset == OFFSET_Y)
	{
		if (size > 0)
		{
			writeXYZW<doMask>(vifRegs->offset, *dest++, *data);
			vifRegs->offset = OFFSET_Z;
			size--;
		}
	}

	if (vifRegs->offset == OFFSET_Z)
	{
		writeXYZW<doMask>(vifRegs->offset, *dest++, *dest-2);
		vifRegs->offset = OFFSET_W;
	}

	if (vifRegs->offset == OFFSET_W)
	{
		writeXYZW<doMask>(vifRegs->offset, *dest, *data);
		vifRegs->offset = OFFSET_X;
	}
}

template <bool doMask, class T>
static __forceinline void __fastcall UNPACK_V3(u32 *dest, T *data, int size)
{
	if(vifRegs->offset == OFFSET_X)
	{
		if (size > 0)
		{
			writeXYZW<doMask>(vifRegs->offset, *dest++, *data++);
			vifRegs->offset = OFFSET_Y;
			size--;
		}
	}

	if(vifRegs->offset == OFFSET_Y)
	{
		if (size > 0)
		{
			writeXYZW<doMask>(vifRegs->offset, *dest++, *data++);
			vifRegs->offset = OFFSET_Z;
			size--;
		}
	}

	if(vifRegs->offset == OFFSET_Z)
	{
		if (size > 0)
		{
			writeXYZW<doMask>(vifRegs->offset, *dest++, *data++);
			vifRegs->offset = OFFSET_W;
			size--;
		}
	}

	if(vifRegs->offset == OFFSET_W)
	{
		//V3-# does some bizzare thing with alignment, every 6qw of data the W becomes 0 (strange console!)
		//Ape Escape doesnt seem to like it tho (what the hell?) gonna have to investigate
		writeXYZW<doMask>(vifRegs->offset, *dest, *data);
		vifRegs->offset = OFFSET_X;
	}
}

template <bool doMask, class T>
static __forceinline void __fastcall UNPACK_V4(u32 *dest, T *data , int size)
{
	while (size > 0)
	{
		writeXYZW<doMask>(vifRegs->offset, *dest++, *data++);
		vifRegs->offset++;
		size--;
	}

	if (vifRegs->offset > OFFSET_W) vifRegs->offset = OFFSET_X;
}

template< bool doMask >
static __releaseinline void __fastcall UNPACK_V4_5(u32 *dest, u32 *data, int size)
{
	//As with S-#, this will always be a complete packet
	writeXYZW<doMask>(OFFSET_X, *dest++,	((*data & 0x001f) << 3));
	writeXYZW<doMask>(OFFSET_Y, *dest++,	((*data & 0x03e0) >> 2));
	writeXYZW<doMask>(OFFSET_Z, *dest++,	((*data & 0x7c00) >> 7));
	writeXYZW<doMask>(OFFSET_W, *dest,		((*data & 0x8000) >> 8));
}

// =====================================================================================================

template < bool doMask, int size, class T >
static void __fastcall fUNPACK_S(u32 *dest, T *data)
{
	UNPACK_S<doMask>( dest, data, size );
}

template <bool doMask, int size, class T>
static void __fastcall fUNPACK_V2(u32 *dest, T *data)
{
	UNPACK_V2<doMask>( dest, data, size );
}

template <bool doMask, int size, class T>
static void __fastcall fUNPACK_V3(u32 *dest, T *data)
{
	UNPACK_V3<doMask>( dest, data, size );
}

template <bool doMask, int size, class T>
static void __fastcall fUNPACK_V4(u32 *dest, T *data)
{
	UNPACK_V4<doMask>( dest, data, size );
}

template< bool doMask >
static void __fastcall fUNPACK_V4_5(u32 *dest, u32 *data)
{
	UNPACK_V4_5<doMask>(dest, data, 0);		// size is ignored.
}

#define _upk (UNPACKFUNCTYPE)
#define _odd (UNPACKFUNCTYPE_ODD)
#define _unpk_s(bits) (UNPACKFUNCTYPE_S##bits)
#define _odd_s(bits) (UNPACKFUNCTYPE_ODD_S##bits)
#define _unpk_u(bits) (UNPACKFUNCTYPE_U##bits)
#define _odd_u(bits) (UNPACKFUNCTYPE_ODD_U##bits)

// --------------------------------------------------------------------------------------
//  Main table for function unpacking. 
// --------------------------------------------------------------------------------------
// The extra data bsize/dsize/etc are all duplicated between the doMask enabled and
// disabled versions.  This is probably simpler and more efficient than bothering
// to generate separate tables.

// 32-bits versions are unsigned-only!!
#define UnpackFuncPair32( sizefac, vt, doMask ) \
	(UNPACKFUNCTYPE)_unpk_u(32) fUNPACK_##vt<doMask, sizefac, u32>, \
	(UNPACKFUNCTYPE)_unpk_u(32) fUNPACK_##vt<doMask, sizefac, u32>, \
	(UNPACKFUNCTYPE_ODD)_odd_u(32) UNPACK_##vt<doMask, u32>, \
	(UNPACKFUNCTYPE_ODD)_odd_u(32) UNPACK_##vt<doMask, u32>,

#define UnpackFuncPair( sizefac, vt, bits, doMask ) \
	(UNPACKFUNCTYPE)_unpk_u(bits) fUNPACK_##vt<doMask, sizefac, u##bits>, \
	(UNPACKFUNCTYPE)_unpk_s(bits) fUNPACK_##vt<doMask, sizefac, s##bits>, \
	(UNPACKFUNCTYPE_ODD)_odd_u(bits) UNPACK_##vt<doMask, u##bits>, \
	(UNPACKFUNCTYPE_ODD)_odd_s(bits) UNPACK_##vt<doMask, s##bits>,

#define UnpackFuncSet( doMask ) \
 	{	UnpackFuncPair32( 4, S, doMask )		/* 0x0 - S-32 */ \
		1, 4, 4, 4 }, \
	{	UnpackFuncPair	( 4, S, 16, doMask )	/* 0x1 - S-16 */ \
		2, 2, 2, 4 }, \
	{	UnpackFuncPair	( 4, S, 8, doMask )		/* 0x2 - S-8 */ \
		4, 1, 1, 4 }, \
	{ NULL, NULL, NULL, NULL, 0, 0, 0, 0 },		/* 0x3 (NULL) */ \
 \
	{	UnpackFuncPair32( 2, V2, doMask )		/* 0x4 - V2-32 */ \
		24, 4, 8, 2 }, \
	{	UnpackFuncPair	( 2, V2, 16, doMask )	/* 0x5 - V2-16 */ \
		12, 2, 4, 2 }, \
	{	UnpackFuncPair	( 2, V2, 8, doMask )	/* 0x6 - V2-8 */ \
		 6, 1, 2, 2 }, \
	{ NULL, NULL, NULL, NULL,0, 0, 0, 0 },		/* 0x7 (NULL) */ \
 \
	{	UnpackFuncPair32( 3, V3, doMask )		/* 0x8 - V3-32 */ \
		36, 4, 12, 3 }, \
	{	UnpackFuncPair	( 3, V3, 16, doMask )	/* 0x9 - V3-16 */ \
		18, 2, 6, 3 }, \
	{	UnpackFuncPair	( 3, V3, 8, doMask )	/* 0xA - V3-8 */ \
		 9, 1, 3, 3 }, \
	{ NULL, NULL, NULL, NULL,0, 0, 0, 0 },		/* 0xB (NULL) */ \
 \
	{	UnpackFuncPair32( 4, V4, doMask )		/* 0xC - V4-32 */ \
		48, 4, 16, 4 }, \
	{	UnpackFuncPair	( 4, V4, 16, doMask )	/* 0xD - V4-16 */ \
		24, 2, 8, 4 }, \
	{	UnpackFuncPair	( 4, V4, 8, doMask )	/* 0xE - V4-8 */ \
		12, 1, 4, 4 }, \
	{										/* 0xF - V4-5 */ \
		(UNPACKFUNCTYPE)_unpk_u(32) fUNPACK_V4_5<doMask>,		(UNPACKFUNCTYPE)_unpk_u(32) fUNPACK_V4_5<doMask>, \
		(UNPACKFUNCTYPE_ODD)_odd_u(32) UNPACK_V4_5<doMask>,		(UNPACKFUNCTYPE_ODD)_odd_u(32) UNPACK_V4_5<doMask>, \
		6, 2, 2, 4 },

const __aligned16 VIFUnpackFuncTable VIFfuncTable[32] =
{
	UnpackFuncSet( false )
	UnpackFuncSet( true )
};
