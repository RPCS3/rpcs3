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
#include "internal.h"
#include "tools.h"

// Mask of valid bit fields for the target CPU.  Typically this is either 0xFFFF (SSE2
// or better) or 0xFFBF (SSE1 and earlier).  Code can ensure a safe/valid MXCSR by
// AND'ing this mask against an MXCSR prior to LDMXCSR.
SSE_MXCSR	MXCSR_Mask;

SSE_RoundMode SSE_MXCSR::GetRoundMode() const
{
	return (SSE_RoundMode)RoundingControl;
}

SSE_MXCSR& SSE_MXCSR::SetRoundMode( SSE_RoundMode mode )
{
	pxAssert( (uint)mode < 4 ); 
	RoundingControl = (u32)mode;
	return *this;
}

SSE_MXCSR& SSE_MXCSR::ClearExceptionFlags()
{
	bitmask &= ~0x3f;
	return *this;
}

SSE_MXCSR& SSE_MXCSR::EnableExceptions()
{
	bitmask &= ~(0x3f << 7);
	return *this;
}

SSE_MXCSR& SSE_MXCSR::DisableExceptions()
{
	bitmask |= 0x3f << 7;
	return *this;
}

// Applies the reserve bits mask for the current running cpu, as fetched from the CPU
// during CPU init/detection.
SSE_MXCSR& SSE_MXCSR::ApplyReserveMask()
{
	bitmask &= MXCSR_Mask.bitmask;
	return *this;
}

SSE_MXCSR::operator x86Emitter::ModSib32() const
{
	return &bitmask;
}

namespace x86Emitter {

using namespace Internal;

// ------------------------------------------------------------------------
// SimdPrefix - If the lower byte of the opcode is 0x38 or 0x3a, then the opcode is
// treated as a 16 bit value (in SSE 0x38 and 0x3a denote prefixes for extended SSE3/4
// instructions).  Any other lower value assumes the upper value is 0 and ignored.
// Non-zero upper bytes, when the lower byte is not the 0x38 or 0x3a prefix, will
// generate an assertion.
//
__emitinline void Internal::SimdPrefix( u8 prefix, u16 opcode )
{
	const bool is16BitOpcode = ((opcode & 0xff) == 0x38) || ((opcode & 0xff) == 0x3a);

	// If the lower byte is not a valid prefix and the upper byte is non-zero it
	// means we made a mistake!
	if( !is16BitOpcode ) pxAssert( (opcode >> 8) == 0 );

	if( prefix != 0 )
	{
		if( is16BitOpcode )
			xWrite32( (opcode<<16) | 0x0f00 | prefix );
		else
		{
			xWrite16( 0x0f00 | prefix );
			xWrite8( opcode );
		}
	}
	else
	{
		if( is16BitOpcode )
		{
			xWrite8( 0x0f );
			xWrite16( opcode );
		}
		else
			xWrite16( (opcode<<8) | 0x0f );
	}
}

const xImplSimd_DestRegEither xPAND		= { 0x66,0xdb };
const xImplSimd_DestRegEither xPANDN	= { 0x66,0xdf };
const xImplSimd_DestRegEither xPOR		= { 0x66,0xeb };
const xImplSimd_DestRegEither xPXOR		= { 0x66,0xef };

// [SSE-4.1] Performs a bitwise AND of dest against src, and sets the ZF flag
// only if all bits in the result are 0.  PTEST also sets the CF flag according
// to the following condition: (xmm2/m128 AND NOT xmm1) == 0;
const xImplSimd_DestRegSSE		xPTEST = { 0x66,0x1738 };

// =====================================================================================================
// SSE Conversion Operations, as looney as they are.
// =====================================================================================================
// These enforce pointer strictness for Indirect forms, due to the otherwise completely confusing
// nature of the functions.  (so if a function expects an m32, you must use (u32*) or ptr32[]).
//

__forceinline void xCVTDQ2PD( const xRegisterSSE& to, const xRegisterSSE& from )	{ OpWriteSSE( 0xf3, 0xe6 ); }
__forceinline void xCVTDQ2PD( const xRegisterSSE& to, const ModSib64& from )		{ OpWriteSSE( 0xf3, 0xe6 ); }
__forceinline void xCVTDQ2PS( const xRegisterSSE& to, const xRegisterSSE& from )	{ OpWriteSSE( 0x00, 0x5b ); }
__forceinline void xCVTDQ2PS( const xRegisterSSE& to, const ModSib128& from )		{ OpWriteSSE( 0x00, 0x5b ); }

__forceinline void xCVTPD2DQ( const xRegisterSSE& to, const xRegisterSSE& from )	{ OpWriteSSE( 0xf2, 0xe6 ); }
__forceinline void xCVTPD2DQ( const xRegisterSSE& to, const ModSib128& from )		{ OpWriteSSE( 0xf2, 0xe6 ); }
__forceinline void xCVTPD2PI( const xRegisterMMX& to, const xRegisterSSE& from )	{ OpWriteSSE( 0x66, 0x2d ); }
__forceinline void xCVTPD2PI( const xRegisterMMX& to, const ModSib128& from )		{ OpWriteSSE( 0x66, 0x2d ); }
__forceinline void xCVTPD2PS( const xRegisterSSE& to, const xRegisterSSE& from )	{ OpWriteSSE( 0x66, 0x5a ); }
__forceinline void xCVTPD2PS( const xRegisterSSE& to, const ModSib128& from )		{ OpWriteSSE( 0x66, 0x5a ); }

__forceinline void xCVTPI2PD( const xRegisterSSE& to, const xRegisterMMX& from )	{ OpWriteSSE( 0x66, 0x2a ); }
__forceinline void xCVTPI2PD( const xRegisterSSE& to, const ModSib64& from )		{ OpWriteSSE( 0x66, 0x2a ); }
__forceinline void xCVTPI2PS( const xRegisterSSE& to, const xRegisterMMX& from )	{ OpWriteSSE( 0x00, 0x2a ); }
__forceinline void xCVTPI2PS( const xRegisterSSE& to, const ModSib64& from )		{ OpWriteSSE( 0x00, 0x2a ); }

__forceinline void xCVTPS2DQ( const xRegisterSSE& to, const xRegisterSSE& from )	{ OpWriteSSE( 0x66, 0x5b ); }
__forceinline void xCVTPS2DQ( const xRegisterSSE& to, const ModSib128& from )		{ OpWriteSSE( 0x66, 0x5b ); }
__forceinline void xCVTPS2PD( const xRegisterSSE& to, const xRegisterSSE& from )	{ OpWriteSSE( 0x00, 0x5a ); }
__forceinline void xCVTPS2PD( const xRegisterSSE& to, const ModSib64& from )		{ OpWriteSSE( 0x00, 0x5a ); }
__forceinline void xCVTPS2PI( const xRegisterMMX& to, const xRegisterSSE& from )	{ OpWriteSSE( 0x00, 0x2d ); }
__forceinline void xCVTPS2PI( const xRegisterMMX& to, const ModSib64& from )		{ OpWriteSSE( 0x00, 0x2d ); }

__forceinline void xCVTSD2SI( const xRegister32& to, const xRegisterSSE& from )		{ OpWriteSSE( 0xf2, 0x2d ); }
__forceinline void xCVTSD2SI( const xRegister32& to, const ModSib64& from )			{ OpWriteSSE( 0xf2, 0x2d ); }
__forceinline void xCVTSD2SS( const xRegisterSSE& to, const xRegisterSSE& from )	{ OpWriteSSE( 0xf2, 0x5a ); }
__forceinline void xCVTSD2SS( const xRegisterSSE& to, const ModSib64& from )		{ OpWriteSSE( 0xf2, 0x5a ); }
__forceinline void xCVTSI2SD( const xRegisterMMX& to, const xRegister32& from )		{ OpWriteSSE( 0xf2, 0x2a ); }
__forceinline void xCVTSI2SD( const xRegisterMMX& to, const ModSib32& from )		{ OpWriteSSE( 0xf2, 0x2a ); }
__forceinline void xCVTSI2SS( const xRegisterSSE& to, const xRegister32& from )		{ OpWriteSSE( 0xf3, 0x2a ); }
__forceinline void xCVTSI2SS( const xRegisterSSE& to, const ModSib32& from )		{ OpWriteSSE( 0xf3, 0x2a ); }

__forceinline void xCVTSS2SD( const xRegisterSSE& to, const xRegisterSSE& from )	{ OpWriteSSE( 0xf3, 0x5a ); }
__forceinline void xCVTSS2SD( const xRegisterSSE& to, const ModSib32& from )		{ OpWriteSSE( 0xf3, 0x5a ); }
__forceinline void xCVTSS2SI( const xRegister32& to, const xRegisterSSE& from )		{ OpWriteSSE( 0xf3, 0x2d ); }
__forceinline void xCVTSS2SI( const xRegister32& to, const ModSib32& from )			{ OpWriteSSE( 0xf3, 0x2d ); }

__forceinline void xCVTTPD2DQ( const xRegisterSSE& to, const xRegisterSSE& from )	{ OpWriteSSE( 0x66, 0xe6 ); }
__forceinline void xCVTTPD2DQ( const xRegisterSSE& to, const ModSib128& from )		{ OpWriteSSE( 0x66, 0xe6 ); }
__forceinline void xCVTTPD2PI( const xRegisterMMX& to, const xRegisterSSE& from )	{ OpWriteSSE( 0x66, 0x2c ); }
__forceinline void xCVTTPD2PI( const xRegisterMMX& to, const ModSib128& from )		{ OpWriteSSE( 0x66, 0x2c ); }
__forceinline void xCVTTPS2DQ( const xRegisterSSE& to, const xRegisterSSE& from )	{ OpWriteSSE( 0xf3, 0x5b ); }
__forceinline void xCVTTPS2DQ( const xRegisterSSE& to, const ModSib128& from )		{ OpWriteSSE( 0xf3, 0x5b ); }
__forceinline void xCVTTPS2PI( const xRegisterMMX& to, const xRegisterSSE& from )	{ OpWriteSSE( 0x00, 0x2c ); }
__forceinline void xCVTTPS2PI( const xRegisterMMX& to, const ModSib64& from )		{ OpWriteSSE( 0x00, 0x2c ); }

__forceinline void xCVTTSD2SI( const xRegister32& to, const xRegisterSSE& from )	{ OpWriteSSE( 0xf2, 0x2c ); }
__forceinline void xCVTTSD2SI( const xRegister32& to, const ModSib64& from )		{ OpWriteSSE( 0xf2, 0x2c ); }
__forceinline void xCVTTSS2SI( const xRegister32& to, const xRegisterSSE& from )	{ OpWriteSSE( 0xf3, 0x2c ); }
__forceinline void xCVTTSS2SI( const xRegister32& to, const ModSib32& from )		{ OpWriteSSE( 0xf3, 0x2c ); }


// ------------------------------------------------------------------------

void xImplSimd_DestRegSSE::operator()( const xRegisterSSE& to, const xRegisterSSE& from ) const				{ OpWriteSSE( Prefix, Opcode ); }
void xImplSimd_DestRegSSE::operator()( const xRegisterSSE& to, const ModSibBase& from ) const				{ OpWriteSSE( Prefix, Opcode ); }

void xImplSimd_DestRegImmSSE::operator()( const xRegisterSSE& to, const xRegisterSSE& from, u8 imm ) const	{ xOpWrite0F( Prefix, Opcode, to, from, imm ); }
void xImplSimd_DestRegImmSSE::operator()( const xRegisterSSE& to, const ModSibBase& from, u8 imm ) const	{ xOpWrite0F( Prefix, Opcode, to, from, imm ); }

void xImplSimd_DestRegImmMMX::operator()( const xRegisterMMX& to, const xRegisterMMX& from, u8 imm ) const	{ xOpWrite0F( Opcode, to, from, imm ); }
void xImplSimd_DestRegImmMMX::operator()( const xRegisterMMX& to, const ModSibBase& from, u8 imm ) const	{ xOpWrite0F( Opcode, to, from, imm ); }

void xImplSimd_DestRegEither::operator()( const xRegisterSSE& to, const xRegisterSSE& from ) const			{ OpWriteSSE( Prefix, Opcode ); }
void xImplSimd_DestRegEither::operator()( const xRegisterSSE& to, const ModSibBase& from ) const			{ OpWriteSSE( Prefix, Opcode ); }

void xImplSimd_DestRegEither::operator()( const xRegisterMMX& to, const xRegisterMMX& from ) const			{ OpWriteSSE( 0x00, Opcode ); }
void xImplSimd_DestRegEither::operator()( const xRegisterMMX& to, const ModSibBase& from ) const			{ OpWriteSSE( 0x00, Opcode ); }

// =====================================================================================================
//  SIMD Arithmetic Instructions
// =====================================================================================================

void _SimdShiftHelper::operator()( const xRegisterSSE& to, const xRegisterSSE& from ) const			{ OpWriteSSE( Prefix, Opcode ); }
void _SimdShiftHelper::operator()( const xRegisterSSE& to, const ModSibBase& from ) const			{ OpWriteSSE( Prefix, Opcode ); }

void _SimdShiftHelper::operator()( const xRegisterMMX& to, const xRegisterMMX& from ) const			{ OpWriteSSE( 0x00, Opcode ); }
void _SimdShiftHelper::operator()( const xRegisterMMX& to, const ModSibBase& from ) const			{ OpWriteSSE( 0x00, Opcode ); }

void _SimdShiftHelper::operator()( const xRegisterSSE& to, u8 imm8 ) const
{
	SimdPrefix( 0x66, OpcodeImm );
	EmitSibMagic( (int)Modcode, to );
	xWrite8( imm8 );
}

void _SimdShiftHelper::operator()( const xRegisterMMX& to, u8 imm8 ) const
{
	SimdPrefix( 0x00, OpcodeImm );
	EmitSibMagic( (int)Modcode, to );
	xWrite8( imm8 );
}

void xImplSimd_Shift::DQ( const xRegisterSSE& to, u8 imm8 ) const
{
	xOpWrite0F( 0x66, 0x73, (int)Q.Modcode+1, to, imm8 );
}


const xImplSimd_ShiftWithoutQ xPSRA =
{
	{ 0x66, 0xe1, 0x71, 4 },	// W
	{ 0x66, 0xe2, 0x72, 4 }		// D
};

const xImplSimd_Shift xPSRL =
{
	{ 0x66, 0xd1, 0x71, 2 },	// W
	{ 0x66, 0xd2, 0x72, 2 },	// D
	{ 0x66, 0xd3, 0x73, 2 },	// Q
};

const xImplSimd_Shift xPSLL =
{
	{ 0x66, 0xf1, 0x71, 6 },	// W
	{ 0x66, 0xf2, 0x72, 6 },	// D
	{ 0x66, 0xf3, 0x73, 6 },	// Q
};

const xImplSimd_AddSub xPADD =
{
	{ 0x66, 0xdc+0x20 },	// B
	{ 0x66, 0xdc+0x21 },	// W
	{ 0x66, 0xdc+0x22 },	// D
	{ 0x66, 0xd4 },			// Q

	{ 0x66, 0xdc+0x10 },	// SB
	{ 0x66, 0xdc+0x11 },	// SW
	{ 0x66, 0xdc },			// USB
	{ 0x66, 0xdc+1 },		// USW
};

const xImplSimd_AddSub xPSUB =
{
	{ 0x66, 0xd8+0x20 },	// B
	{ 0x66, 0xd8+0x21 },	// W
	{ 0x66, 0xd8+0x22 },	// D
	{ 0x66, 0xfb },			// Q

	{ 0x66, 0xd8+0x10 },	// SB
	{ 0x66, 0xd8+0x11 },	// SW
	{ 0x66, 0xd8 },			// USB
	{ 0x66, 0xd8+1 },		// USW
};

const xImplSimd_PMul xPMUL =
{
	{ 0x66, 0xd5 },		// LW
	{ 0x66, 0xe5 },		// HW
	{ 0x66, 0xe4 },		// HUW
	{ 0x66, 0xf4 },		// UDQ

	{ 0x66, 0x0b38 },	// HRSW
	{ 0x66, 0x4038 },	// LD
	{ 0x66, 0x2838 },	// DQ
};

const xImplSimd_rSqrt xRSQRT =
{
	{ 0x00, 0x52 },		// PS
	{ 0xf3, 0x52 }		// SS
};

const xImplSimd_rSqrt xRCP =
{
	{ 0x00, 0x53 },		// PS
	{ 0xf3, 0x53 }		// SS
};

const xImplSimd_Sqrt xSQRT =
{
	{ 0x00, 0x51 },		// PS
	{ 0xf3, 0x51 },		// SS
	{ 0xf2, 0x51 }		// SS
};

const xImplSimd_AndNot xANDN =
{
	{ 0x00, 0x55 },		// PS
	{ 0x66, 0x55 }		// PD
};

const xImplSimd_PAbsolute xPABS = 
{
	{ 0x66, 0x1c38 },	// B
	{ 0x66, 0x1d38 },	// W
	{ 0x66, 0x1e38 }	// D
};

const xImplSimd_PSign xPSIGN =
{
	{ 0x66, 0x0838 },	// B
	{ 0x66, 0x0938 },	// W
	{ 0x66, 0x0a38 },	// D
};

const xImplSimd_PMultAdd xPMADD =
{
	{ 0x66, 0xf5 },		// WD
	{ 0x66, 0xf438 },	// UBSW
};

const xImplSimd_HorizAdd xHADD =
{
	{ 0xf2, 0x7c },		// PS
	{ 0x66, 0x7c },		// PD
};

const xImplSimd_DotProduct xDP =
{
	{ 0x66,0x403a },	// PS
	{ 0x66,0x413a },	// PD
};

const xImplSimd_Round xROUND =
{
	{ 0x66,0x083a },	// PS
	{ 0x66,0x093a },	// PD
	{ 0x66,0x0a3a },	// SS
	{ 0x66,0x0b3a },	// SD
};

// =====================================================================================================
//  SIMD Comparison Instructions
// =====================================================================================================

void xImplSimd_Compare::PS( const xRegisterSSE& to, const xRegisterSSE& from ) const	{ xOpWrite0F( 0x00, 0xc2, to, from, (u8)CType ); }
void xImplSimd_Compare::PS( const xRegisterSSE& to, const ModSibBase& from ) const	{ xOpWrite0F( 0x00, 0xc2, to, from, (u8)CType ); }

void xImplSimd_Compare::PD( const xRegisterSSE& to, const xRegisterSSE& from ) const	{ xOpWrite0F( 0x66, 0xc2, to, from, (u8)CType ); }
void xImplSimd_Compare::PD( const xRegisterSSE& to, const ModSibBase& from ) const	{ xOpWrite0F( 0x66, 0xc2, to, from, (u8)CType ); }

void xImplSimd_Compare::SS( const xRegisterSSE& to, const xRegisterSSE& from ) const	{ xOpWrite0F( 0xf3, 0xc2, to, from, (u8)CType ); }
void xImplSimd_Compare::SS( const xRegisterSSE& to, const ModSibBase& from ) const	{ xOpWrite0F( 0xf3, 0xc2, to, from, (u8)CType ); }

void xImplSimd_Compare::SD( const xRegisterSSE& to, const xRegisterSSE& from ) const	{ xOpWrite0F( 0xf2, 0xc2, to, from, (u8)CType ); }
void xImplSimd_Compare::SD( const xRegisterSSE& to, const ModSibBase& from ) const	{ xOpWrite0F( 0xf2, 0xc2, to, from, (u8)CType ); }

const xImplSimd_MinMax xMIN = 
{
	{ 0x00, 0x5d },		// PS
	{ 0x66, 0x5d },		// PD
	{ 0xf3, 0x5d },		// SS
	{ 0xf2, 0x5d },		// SD
};

const xImplSimd_MinMax xMAX = 
{
	{ 0x00, 0x5f },		// PS
	{ 0x66, 0x5f },		// PD
	{ 0xf3, 0x5f },		// SS
	{ 0xf2, 0x5f },		// SD
};

// [TODO] : Merge this into the xCMP class, so that they are notation as: xCMP.EQ

const xImplSimd_Compare xCMPEQ		= { SSE2_Equal };
const xImplSimd_Compare xCMPLT		= { SSE2_Less };
const xImplSimd_Compare xCMPLE		= { SSE2_LessOrEqual };
const xImplSimd_Compare xCMPUNORD	= { SSE2_LessOrEqual };
const xImplSimd_Compare xCMPNE		= { SSE2_NotEqual };
const xImplSimd_Compare xCMPNLT		= { SSE2_NotLess };
const xImplSimd_Compare xCMPNLE		= { SSE2_NotLessOrEqual };
const xImplSimd_Compare xCMPORD		= { SSE2_Ordered };

const xImplSimd_COMI xCOMI =
{
	{ 0x00, 0x2f },		// SS
	{ 0x66, 0x2f },		// SD
};

const xImplSimd_COMI xUCOMI =
{
	{ 0x00, 0x2e },		// SS
	{ 0x66, 0x2e },		// SD
};

const xImplSimd_PCompare xPCMP =
{
	{ 0x66, 0x74 },		// EQB
	{ 0x66, 0x75 },		// EQW
	{ 0x66, 0x76 },		// EQD

	{ 0x66, 0x64 },		// GTB
	{ 0x66, 0x65 },		// GTW
	{ 0x66, 0x66 },		// GTD
};

const xImplSimd_PMinMax xPMIN = 
{
	{ 0x66, 0xda },			// UB
	{ 0x66, 0xea },			// SW
	{ 0x66, 0x3838 },		// SB
	{ 0x66, 0x3938 },		// SD

	{ 0x66, 0x3a38 },		// UW
	{ 0x66, 0x3b38 },		// UD
};

const xImplSimd_PMinMax xPMAX = 
{
	{ 0x66, 0xde },			// UB
	{ 0x66, 0xee },			// SW
	{ 0x66, 0x3c38 },		// SB
	{ 0x66, 0x3d38 },		// SD

	{ 0x66, 0x3e38 },		// UW
	{ 0x66, 0x3f38 },		// UD
};

// =====================================================================================================
//  SIMD Shuffle/Pack  (Shuffle puck?)
// =====================================================================================================

__forceinline void xImplSimd_Shuffle::_selector_assertion_check( u8 selector ) const
{
	pxAssertMsg( (selector & ~3) == 0,
		"Invalid immediate operand on SSE Shuffle: Upper 6 bits of the SSE Shuffle-PD Selector are reserved and must be zero."
	);
}

void xImplSimd_Shuffle::PS( const xRegisterSSE& to, const xRegisterSSE& from, u8 selector ) const
{
	xOpWrite0F( 0xc6, to, from, selector );
}

void xImplSimd_Shuffle::PS( const xRegisterSSE& to, const ModSibBase& from, u8 selector ) const
{
	xOpWrite0F( 0xc6, to, from, selector );
}

void xImplSimd_Shuffle::PD( const xRegisterSSE& to, const xRegisterSSE& from, u8 selector ) const
{
	_selector_assertion_check( selector );
	xOpWrite0F( 0x66, 0xc6, to, from, selector & 0x3 );
}

void xImplSimd_Shuffle::PD( const xRegisterSSE& to, const ModSibBase& from, u8 selector ) const
{
	_selector_assertion_check( selector );
	xOpWrite0F( 0x66, 0xc6, to, from, selector & 0x3 );
}

void xImplSimd_InsertExtractHelper::operator()( const xRegisterSSE& to, const xRegister32& from, u8 imm8 ) const
{
	xOpWrite0F( 0x66, Opcode, to, from, imm8 );
}

void xImplSimd_InsertExtractHelper::operator()( const xRegisterSSE& to, const ModSibBase& from, u8 imm8 ) const
{
	xOpWrite0F( 0x66, Opcode, to, from, imm8 );
}

void xImplSimd_PInsert::W( const xRegisterSSE& to, const xRegister32& from, u8 imm8 ) const		{ xOpWrite0F( 0x66, 0xc4, to, from, imm8 ); }
void xImplSimd_PInsert::W( const xRegisterSSE& to, const ModSibBase& from, u8 imm8 ) const		{ xOpWrite0F( 0x66, 0xc4, to, from, imm8 ); }
void xImplSimd_PInsert::W( const xRegisterMMX& to, const xRegister32& from, u8 imm8 ) const		{ xOpWrite0F( 0xc4, to, from, imm8 ); }
void xImplSimd_PInsert::W( const xRegisterMMX& to, const ModSibBase& from, u8 imm8 ) const		{ xOpWrite0F( 0xc4, to, from, imm8 ); }

void SimdImpl_PExtract::W( const xRegister32& to, const xRegisterSSE& from, u8 imm8 ) const		{ xOpWrite0F( 0x66, 0xc5, to, from, imm8 ); }
void SimdImpl_PExtract::W( const xRegister32& to, const xRegisterMMX& from, u8 imm8 ) const		{ xOpWrite0F( 0xc5, to, from, imm8 ); }
void SimdImpl_PExtract::W( const ModSibBase& dest, const xRegisterSSE& from, u8 imm8 ) const	{ xOpWrite0F( 0x66, 0x153a, from, dest, imm8 ); }

const xImplSimd_Shuffle xSHUF;

const xImplSimd_PShuffle xPSHUF =
{
	{ 0x00, 0x70 },		// W
	{ 0x66, 0x70 },		// D
	{ 0xf2, 0x70 },		// LW
	{ 0xf3, 0x70 },		// HW
	
	{ 0x66, 0x0038 },	// B
};

const SimdImpl_PUnpack xPUNPCK =
{
	{ 0x66, 0x60 },		// LBW
	{ 0x66, 0x61 },		// LWD
	{ 0x66, 0x62 },		// LDQ
	{ 0x66, 0x6c },		// LQDQ

	{ 0x66, 0x68 },		// HBW
	{ 0x66, 0x69 },		// HWD
	{ 0x66, 0x6a },		// HDQ
	{ 0x66, 0x6d },		// HQDQ
};

const SimdImpl_Pack xPACK =
{
	{ 0x66, 0x63 },		// SSWB
	{ 0x66, 0x6b },		// SSDW
	{ 0x66, 0x67 },		// USWB
	{ 0x66, 0x2b38 },	// USDW
};

const xImplSimd_Unpack xUNPCK =
{
	{ 0x00, 0x15 },		// HPS
	{ 0x66, 0x15 },		// HPD
	{ 0x00, 0x14 },		// LPS
	{ 0x66, 0x14 },		// LPD
};

const xImplSimd_PInsert xPINSR =
{
	{ 0x203a },			// B
	{ 0x223a },			// D
};

const SimdImpl_PExtract xPEXTR =
{
	{ 0x143a },			// B
	{ 0x163a },			// D
};

// =====================================================================================================
//  SIMD Move And Blend Instructions
// =====================================================================================================

void xImplSimd_MovHL::PS( const xRegisterSSE& to, const ModSibBase& from ) const			{ xOpWrite0F( Opcode, to, from ); }
void xImplSimd_MovHL::PS( const ModSibBase& to, const xRegisterSSE& from ) const			{ xOpWrite0F( Opcode+1, from, to ); }

void xImplSimd_MovHL::PD( const xRegisterSSE& to, const ModSibBase& from ) const			{ xOpWrite0F( 0x66, Opcode, to, from ); }
void xImplSimd_MovHL::PD( const ModSibBase& to, const xRegisterSSE& from ) const			{ xOpWrite0F( 0x66, Opcode+1, from, to ); }

void xImplSimd_MovHL_RtoR::PS( const xRegisterSSE& to, const xRegisterSSE& from ) const		{ xOpWrite0F( Opcode, to, from ); }
void xImplSimd_MovHL_RtoR::PD( const xRegisterSSE& to, const xRegisterSSE& from ) const		{ xOpWrite0F( 0x66, Opcode, to, from ); }

static const u16 MovPS_OpAligned		= 0x28;		// Aligned [aps] form
static const u16 MovPS_OpUnaligned		= 0x10;		// unaligned [ups] form

void xImplSimd_MoveSSE::operator()( const xRegisterSSE& to, const xRegisterSSE& from ) const
{
	if( to != from ) xOpWrite0F( Prefix, MovPS_OpAligned, to, from );
}

void xImplSimd_MoveSSE::operator()( const xRegisterSSE& to, const ModSibBase& from ) const
{
	// ModSib form is aligned if it's displacement-only and the displacement is aligned:
	bool isReallyAligned = isAligned || ( ((from.Displacement & 0x0f) == 0) && from.Index.IsEmpty() && from.Base.IsEmpty() );

	xOpWrite0F( Prefix, isReallyAligned ? MovPS_OpAligned : MovPS_OpUnaligned, to, from );
}

void xImplSimd_MoveSSE::operator()( const ModSibBase& to, const xRegisterSSE& from ) const
{
	// ModSib form is aligned if it's displacement-only and the displacement is aligned:
	bool isReallyAligned = isAligned || ( (to.Displacement & 0x0f) == 0 && to.Index.IsEmpty() && to.Base.IsEmpty() );
	xOpWrite0F( Prefix, isReallyAligned ? MovPS_OpAligned+1 : MovPS_OpUnaligned+1, from, to );
}

static const u8 MovDQ_PrefixAligned		= 0x66;		// Aligned [dqa] form
static const u8 MovDQ_PrefixUnaligned	= 0xf3;		// unaligned [dqu] form

void xImplSimd_MoveDQ::operator()( const xRegisterSSE& to, const xRegisterSSE& from ) const
{
	if( to != from ) xOpWrite0F( MovDQ_PrefixAligned, 0x6f, to, from );
}

void xImplSimd_MoveDQ::operator()( const xRegisterSSE& to, const ModSibBase& from ) const
{
	// ModSib form is aligned if it's displacement-only and the displacement is aligned:
	bool isReallyAligned = isAligned || ( (from.Displacement & 0x0f) == 0 && from.Index.IsEmpty() && from.Base.IsEmpty() );
	xOpWrite0F( isReallyAligned ? MovDQ_PrefixAligned : MovDQ_PrefixUnaligned, 0x6f, to, from );
}

void xImplSimd_MoveDQ::operator()( const ModSibBase& to, const xRegisterSSE& from ) const
{
	// ModSib form is aligned if it's displacement-only and the displacement is aligned:
	bool isReallyAligned = isAligned || ( (to.Displacement & 0x0f) == 0 && to.Index.IsEmpty() && to.Base.IsEmpty() );

	// use opcode 0x7f : alternate ModRM encoding (reverse src/dst)
	xOpWrite0F( isReallyAligned ? MovDQ_PrefixAligned : MovDQ_PrefixUnaligned, 0x7f, from, to );
}

void xImplSimd_PMove::BW( const xRegisterSSE& to, const xRegisterSSE& from ) const		{ OpWriteSSE( 0x66, OpcodeBase ); }
void xImplSimd_PMove::BW( const xRegisterSSE& to, const ModSibStrict<u64>& from ) const	{ OpWriteSSE( 0x66, OpcodeBase ); }

void xImplSimd_PMove::BD( const xRegisterSSE& to, const xRegisterSSE& from ) const		{ OpWriteSSE( 0x66, OpcodeBase+0x100 ); }
void xImplSimd_PMove::BD( const xRegisterSSE& to, const ModSibStrict<u32>& from ) const	{ OpWriteSSE( 0x66, OpcodeBase+0x100 ); }

void xImplSimd_PMove::BQ( const xRegisterSSE& to, const xRegisterSSE& from ) const		{ OpWriteSSE( 0x66, OpcodeBase+0x200 ); }
void xImplSimd_PMove::BQ( const xRegisterSSE& to, const ModSibStrict<u16>& from ) const	{ OpWriteSSE( 0x66, OpcodeBase+0x200 ); }

void xImplSimd_PMove::WD( const xRegisterSSE& to, const xRegisterSSE& from ) const		{ OpWriteSSE( 0x66, OpcodeBase+0x300 ); }
void xImplSimd_PMove::WD( const xRegisterSSE& to, const ModSibStrict<u64>& from ) const	{ OpWriteSSE( 0x66, OpcodeBase+0x300 ); }

void xImplSimd_PMove::WQ( const xRegisterSSE& to, const xRegisterSSE& from ) const		{ OpWriteSSE( 0x66, OpcodeBase+0x400 ); }
void xImplSimd_PMove::WQ( const xRegisterSSE& to, const ModSibStrict<u32>& from ) const	{ OpWriteSSE( 0x66, OpcodeBase+0x400 ); }

void xImplSimd_PMove::DQ( const xRegisterSSE& to, const xRegisterSSE& from ) const		{ OpWriteSSE( 0x66, OpcodeBase+0x500 ); }
void xImplSimd_PMove::DQ( const xRegisterSSE& to, const ModSibStrict<u64>& from ) const	{ OpWriteSSE( 0x66, OpcodeBase+0x500 ); }


const xImplSimd_MoveSSE xMOVAPS = { 0x00, true };
const xImplSimd_MoveSSE xMOVUPS = { 0x00, false };

#ifdef ALWAYS_USE_MOVAPS
	const xImplSimd_MoveSSE xMOVDQA	= { 0x00, true };
	const xImplSimd_MoveSSE xMOVAPD	= { 0x00, true };

	const xImplSimd_MoveSSE xMOVDQU	= { 0x00, false };
	const xImplSimd_MoveSSE xMOVUPD	= { 0x00, false };
#else
	const xImplSimd_MoveDQ xMOVDQA	= { 0x66, true };
	const xImplSimd_MoveSSE xMOVAPD	= { 0x66, true };

	const xImplSimd_MoveDQ xMOVDQU	= { 0xf3, false };
	const xImplSimd_MoveSSE xMOVUPD	= { 0x66, false };
#endif


const xImplSimd_MovHL xMOVH = { 0x16 };
const xImplSimd_MovHL xMOVL = { 0x12 };

const xImplSimd_MovHL_RtoR xMOVLH = { 0x16 };
const xImplSimd_MovHL_RtoR xMOVHL = { 0x12 };

const xImplSimd_Blend xBLEND =
{
	{ 0x66, 0x0c3a },		// PS
	{ 0x66, 0x0d3a },		// PD
	{ 0x66, 0x1438 },		// VPS
	{ 0x66, 0x1538 },		// VPD
};

const xImplSimd_PMove xPMOVSX = { 0x2038 };
const xImplSimd_PMove xPMOVZX = { 0x3038 };

// [SSE-3]
const xImplSimd_DestRegSSE xMOVSLDUP = { 0xf3,0x12 };

// [SSE-3]
const xImplSimd_DestRegSSE xMOVSHDUP = { 0xf3,0x16 };

//////////////////////////////////////////////////////////////////////////////////////////
// MMX Mov Instructions (MOVD, MOVQ, MOVSS).
//
// Notes:
//  * Some of the functions have been renamed to more clearly reflect what they actually
//    do.  Namely we've affixed "ZX" to several MOVs that take a register as a destination
//    since that's what they do (MOVD clears upper 32/96 bits, etc).
//
//  * MOVD has valid forms for MMX and XMM registers.
//

__forceinline void xMOVDZX( const xRegisterSSE& to, const xRegister32& from )	{ xOpWrite0F( 0x66, 0x6e, to, from ); }
__forceinline void xMOVDZX( const xRegisterSSE& to, const ModSibBase& src )		{ xOpWrite0F( 0x66, 0x6e, to, src ); }

__forceinline void xMOVDZX( const xRegisterMMX& to, const xRegister32& from )	{ xOpWrite0F( 0x6e, to, from ); }
__forceinline void xMOVDZX( const xRegisterMMX& to, const ModSibBase& src )		{ xOpWrite0F( 0x6e, to, src ); }

__forceinline void xMOVD( const xRegister32& to, const xRegisterSSE& from )		{ xOpWrite0F( 0x66, 0x7e, from, to ); }
__forceinline void xMOVD( const ModSibBase& dest, const xRegisterSSE& from )	{ xOpWrite0F( 0x66, 0x7e, from, dest ); }

__forceinline void xMOVD( const xRegister32& to, const xRegisterMMX& from )		{ xOpWrite0F( 0x7e, from, to ); }
__forceinline void xMOVD( const ModSibBase& dest, const xRegisterMMX& from )	{ xOpWrite0F( 0x7e, from, dest ); }


// Moves from XMM to XMM, with the *upper 64 bits* of the destination register
// being cleared to zero.
__forceinline void xMOVQZX( const xRegisterSSE& to, const xRegisterSSE& from )	{ xOpWrite0F( 0xf3, 0x7e, to, from ); }

// Moves from XMM to XMM, with the *upper 64 bits* of the destination register
// being cleared to zero.
__forceinline void xMOVQZX( const xRegisterSSE& to, const ModSibBase& src )		{ xOpWrite0F( 0xf3, 0x7e, to, src ); }

// Moves from XMM to XMM, with the *upper 64 bits* of the destination register
// being cleared to zero.
__forceinline void xMOVQZX( const xRegisterSSE& to, const void* src )			{ xOpWrite0F( 0xf3, 0x7e, to, src ); }

// Moves lower quad of XMM to ptr64 (no bits are cleared)
__forceinline void xMOVQ( const ModSibBase& dest, const xRegisterSSE& from )	{ xOpWrite0F( 0x66, 0xd6, from, dest ); }

__forceinline void xMOVQ( const xRegisterMMX& to, const xRegisterMMX& from )	{ if( to != from ) xOpWrite0F( 0x6f, to, from ); }
__forceinline void xMOVQ( const xRegisterMMX& to, const ModSibBase& src )		{ xOpWrite0F( 0x6f, to, src ); }
__forceinline void xMOVQ( const ModSibBase& dest, const xRegisterMMX& from )	{ xOpWrite0F( 0x7f, from, dest ); }

// This form of xMOVQ is Intel's adeptly named 'MOVQ2DQ'
__forceinline void xMOVQ( const xRegisterSSE& to, const xRegisterMMX& from )	{ xOpWrite0F( 0xf3, 0xd6, to, from ); }

// This form of xMOVQ is Intel's adeptly named 'MOVDQ2Q'
__forceinline void xMOVQ( const xRegisterMMX& to, const xRegisterSSE& from )
{
	// Manual implementation of this form of MOVQ, since its parameters are unique in a way
	// that breaks the template inference of writeXMMop();

	SimdPrefix( 0xf2, 0xd6 );
	EmitSibMagic( to, from );
}

//////////////////////////////////////////////////////////////////////////////////////////
//

#define IMPLEMENT_xMOVS( ssd, prefix ) \
	__forceinline void xMOV##ssd( const xRegisterSSE& to, const xRegisterSSE& from )	{ if( to != from ) xOpWrite0F( prefix, 0x10, to, from ); } \
	__forceinline void xMOV##ssd##ZX( const xRegisterSSE& to, const ModSibBase& from )	{ xOpWrite0F( prefix, 0x10, to, from ); } \
	__forceinline void xMOV##ssd( const ModSibBase& to, const xRegisterSSE& from )		{ xOpWrite0F( prefix, 0x11, from, to ); }

IMPLEMENT_xMOVS( SS, 0xf3 )
IMPLEMENT_xMOVS( SD, 0xf2 )

//////////////////////////////////////////////////////////////////////////////////////////
// Non-temporal movs only support a register as a target (ie, load form only, no stores)
//

__forceinline void xMOVNTDQA( const xRegisterSSE& to, const ModSibBase& from )
{
	xWrite32( 0x2A380f66 );
	EmitSibMagic( to.Id, from );
}

__forceinline void xMOVNTDQA( const ModSibBase& to, const xRegisterSSE& from )	{ xOpWrite0F( 0x66, 0xe7, from, to ); }

__forceinline void xMOVNTPD( const ModSibBase& to, const xRegisterSSE& from )	{ xOpWrite0F( 0x66, 0x2b, from, to ); }
__forceinline void xMOVNTPS( const ModSibBase& to, const xRegisterSSE& from )	{ xOpWrite0F( 0x2b, from, to ); }

__forceinline void xMOVNTQ( const ModSibBase& to, const xRegisterMMX& from )	{ xOpWrite0F( 0xe7, from, to ); }

// ------------------------------------------------------------------------

__forceinline void xMOVMSKPS( const xRegister32& to, const xRegisterSSE& from)	{ xOpWrite0F( 0x50, to, from ); }
__forceinline void xMOVMSKPD( const xRegister32& to, const xRegisterSSE& from)	{ xOpWrite0F( 0x66, 0x50, to, from, true ); }

// xMASKMOV:
// Selectively write bytes from mm1/xmm1 to memory location using the byte mask in mm2/xmm2.
// The default memory location is specified by DS:EDI.  The most significant bit in each byte
// of the mask operand determines whether the corresponding byte in the source operand is
// written to the corresponding byte location in memory.
__forceinline void xMASKMOV( const xRegisterSSE& to, const xRegisterSSE& from )		{ xOpWrite0F( 0x66, 0xf7, to, from ); }
__forceinline void xMASKMOV( const xRegisterMMX& to, const xRegisterMMX& from )		{ xOpWrite0F( 0xf7, to, from ); }

// xPMOVMSKB:
// Creates a mask made up of the most significant bit of each byte of the source 
// operand and stores the result in the low byte or word of the destination operand.
// Upper bits of the destination are cleared to zero.
//
// When operating on a 64-bit (MMX) source, the byte mask is 8 bits; when operating on
// 128-bit (SSE) source, the byte mask is 16-bits.
//
__forceinline void xPMOVMSKB( const xRegister32& to, const xRegisterSSE& from )		{ xOpWrite0F( 0x66, 0xd7, to, from ); }
__forceinline void xPMOVMSKB( const xRegister32& to, const xRegisterMMX& from )		{ xOpWrite0F( 0xd7, to, from ); }

// [sSSE-3] Concatenates dest and source operands into an intermediate composite,
// shifts the composite at byte granularity to the right by a constant immediate,
// and extracts the right-aligned result into the destination.
//
__forceinline void xPALIGNR( const xRegisterSSE& to, const xRegisterSSE& from, u8 imm8 )	{ xOpWrite0F( 0x66, 0x0f3a, to, from, imm8 ); }
__forceinline void xPALIGNR( const xRegisterMMX& to, const xRegisterMMX& from, u8 imm8 )	{ xOpWrite0F( 0x0f3a, to, from, imm8 ); }


// --------------------------------------------------------------------------------------
//  INSERTPS / EXTRACTPS   [SSE4.1 only!]
// --------------------------------------------------------------------------------------
// [TODO] these might be served better as classes, especially if other instructions use
// the M32,sse,imm form (I forget offhand if any do).


// [SSE-4.1] Insert a single-precision floating-point value from src into a specified
// location in dest, and selectively zero out the data elements in dest according to
// the mask  field in the immediate byte. The source operand can be a memory location
// (32 bits) or an XMM register (lower 32 bits used).
//
// Imm8 provides three fields:
//  * COUNT_S: The value of Imm8[7:6] selects the dword element from src.  It is 0 if
//    the source is a memory operand.
//  * COUNT_D: The value of Imm8[5:4] selects the target dword element in dest.
//  * ZMASK: Each bit of Imm8[3:0] selects a dword element in dest to  be written
//    with 0.0 if set to 1.
//
__emitinline void xINSERTPS( const xRegisterSSE& to, const xRegisterSSE& from, u8 imm8 )		{ xOpWrite0F( 0x66, 0x213a, to, from, imm8 ); }
__emitinline void xINSERTPS( const xRegisterSSE& to, const ModSibStrict<u32>& from, u8 imm8 )	{ xOpWrite0F( 0x66, 0x213a, to, from, imm8 ); }

// [SSE-4.1] Extract a single-precision floating-point value from src at an offset
// determined by imm8[1-0]*32. The extracted single precision floating-point value
// is stored into the low 32-bits of dest (or at a 32-bit memory pointer).
//
__emitinline void xEXTRACTPS( const xRegister32& to, const xRegisterSSE& from, u8 imm8 )		{ xOpWrite0F( 0x66, 0x173a, to, from, imm8 ); }
__emitinline void xEXTRACTPS( const ModSibStrict<u32>& dest, const xRegisterSSE& from, u8 imm8 ){ xOpWrite0F( 0x66, 0x173a, from, dest, imm8 ); }


// =====================================================================================================
//  Ungrouped Instructions!
// =====================================================================================================

// Converts from MMX register mode to FPU register mode.  The cpu enters MMX register mode
// when ever MMX instructions are run, and if FPU instructions are run without using EMMS,
// the FPU results will be invalid.
__forceinline void xEMMS()	{ xWrite16( 0x770F ); }

// [3DNow] Same as EMMS, but an AMD special version which may (or may not) leave MMX regs
// in an undefined state (which is fine, since presumably you're done using them anyway).
// This instruction is thus faster than EMMS on K8s, but all newer AMD cpus use the same
// logic for either EMMS or FEMMS.
// Conclusion: Obsolete.  Just use EMMS instead.
__forceinline void xFEMMS()	{ xWrite16( 0x0E0F ); }


// Store Streaming SIMD Extension Control/Status to Mem32.
__emitinline void xSTMXCSR( const ModSib32& dest )
{
	SimdPrefix( 0, 0xae );
	EmitSibMagic( 3, dest );
}

// Load Streaming SIMD Extension Control/Status from Mem32.
__emitinline void xLDMXCSR( const ModSib32& src )
{
	SimdPrefix( 0, 0xae );
	EmitSibMagic( 2, src );
}

// Save x87 FPU, MMX Technology, and SSE State to buffer
// Target buffer must be at least 512 bytes in length to hold the result.
__emitinline void xFXSAVE( const ModSibBase& dest )
{
	SimdPrefix( 0, 0xae );
	EmitSibMagic( 0, dest );
}

// Restore x87 FPU, MMX , XMM, and MXCSR State.
// Source buffer should be 512 bytes in length.
__emitinline void xFXRSTOR( const ModSibBase& src )
{
	SimdPrefix( 0, 0xae );
	EmitSibMagic( 1, src );
}

}