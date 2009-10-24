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

// [SSE-3]
const SimdImpl_DestRegSSE<0xf3,0x12> xMOVSLDUP;
// [SSE-3]
const SimdImpl_DestRegSSE<0xf3,0x16> xMOVSHDUP;

const SimdImpl_MoveSSE<0x00,true> xMOVAPS;

// Note: All implementations of Unaligned Movs will, when possible, use aligned movs instead.
// This happens when using Mem,Reg or Reg,Mem forms where the address is simple displacement
// which can be checked for alignment at runtime.
const SimdImpl_MoveSSE<0x00,false> xMOVUPS;

#ifdef ALWAYS_USE_MOVAPS
const SimdImpl_MoveSSE<0,true> xMOVDQA;
const SimdImpl_MoveSSE<0,true> xMOVAPD;

// Note: All implementations of Unaligned Movs will, when possible, use aligned movs instead.
// This happens when using Mem,Reg or Reg,Mem forms where the address is simple displacement
// which can be checked for alignment at runtime.
const SimdImpl_MoveSSE<0,false> xMOVDQU;
const SimdImpl_MoveSSE<0,false> xMOVUPD;
#else
const SimdImpl_MoveDQ<0x66, 0x6f, 0x7f> xMOVDQA;
const SimdImpl_MoveDQ<0xf3, 0x6f, 0x7f> xMOVDQU;
const SimdImpl_MoveSSE<0x66,true> xMOVAPD;
const SimdImpl_MoveSSE<0x66,false> xMOVUPD;
#endif

const MovhlImplAll<0x16>		xMOVH;
const MovhlImplAll<0x12>		xMOVL;
const MovhlImpl_RtoR<0x16>		xMOVLH;
const MovhlImpl_RtoR<0x12>		xMOVHL;

const SimdImpl_COMI<true>		xCOMI;
const SimdImpl_COMI<false>		xUCOMI;

const SimdImpl_MinMax<0x5f>		xMAX;
const SimdImpl_MinMax<0x5d>		xMIN;
const SimdImpl_Shuffle<0xc6>	xSHUF;

const SimdImpl_DestRegEither<0x66,0xdb> xPAND;
const SimdImpl_DestRegEither<0x66,0xdf> xPANDN;
const SimdImpl_DestRegEither<0x66,0xeb> xPOR;
const SimdImpl_DestRegEither<0x66,0xef> xPXOR;

// ------------------------------------------------------------------------

// [SSE-4.1] Performs a bitwise AND of dest against src, and sets the ZF flag
// only if all bits in the result are 0.  PTEST also sets the CF flag according
// to the following condition: (xmm2/m128 AND NOT xmm1) == 0;
const SimdImpl_DestRegSSE<0x66,0x1738>		xPTEST;

const SimdImpl_Compare<SSE2_Equal>			xCMPEQ;
const SimdImpl_Compare<SSE2_Less>			xCMPLT;
const SimdImpl_Compare<SSE2_LessOrEqual>	xCMPLE;
const SimdImpl_Compare<SSE2_Unordered>		xCMPUNORD;
const SimdImpl_Compare<SSE2_NotEqual>		xCMPNE;
const SimdImpl_Compare<SSE2_NotLess>		xCMPNLT;
const SimdImpl_Compare<SSE2_NotLessOrEqual> xCMPNLE;
const SimdImpl_Compare<SSE2_Ordered>		xCMPORD;

// ------------------------------------------------------------------------
// SSE Conversion Operations, as looney as they are.
// 
// These enforce pointer strictness for Indirect forms, due to the otherwise completely confusing
// nature of the functions.  (so if a function expects an m32, you must use (u32*) or ptr32[]).
//
const SimdImpl_DestRegStrict<0xf3,0xe6,xRegisterSSE,xRegisterSSE,u64>		xCVTDQ2PD;
const SimdImpl_DestRegStrict<0x00,0x5b,xRegisterSSE,xRegisterSSE,u128>		xCVTDQ2PS;

const SimdImpl_DestRegStrict<0xf2,0xe6,xRegisterSSE,xRegisterSSE,u128>		xCVTPD2DQ;
const SimdImpl_DestRegStrict<0x66,0x2d,xRegisterMMX,xRegisterSSE,u128>		xCVTPD2PI;
const SimdImpl_DestRegStrict<0x66,0x5a,xRegisterSSE,xRegisterSSE,u128>		xCVTPD2PS;

const SimdImpl_DestRegStrict<0x66,0x2a,xRegisterSSE,xRegisterMMX,u64>		xCVTPI2PD;
const SimdImpl_DestRegStrict<0x00,0x2a,xRegisterSSE,xRegisterMMX,u64>		xCVTPI2PS;

const SimdImpl_DestRegStrict<0x66,0x5b,xRegisterSSE,xRegisterSSE,u128>		xCVTPS2DQ;
const SimdImpl_DestRegStrict<0x00,0x5a,xRegisterSSE,xRegisterSSE,u64>		xCVTPS2PD;
const SimdImpl_DestRegStrict<0x00,0x2d,xRegisterMMX,xRegisterSSE,u64>		xCVTPS2PI;

const SimdImpl_DestRegStrict<0xf2,0x2d,xRegister32, xRegisterSSE,u64>		xCVTSD2SI;
const SimdImpl_DestRegStrict<0xf2,0x5a,xRegisterSSE,xRegisterSSE,u64>		xCVTSD2SS;
const SimdImpl_DestRegStrict<0xf2,0x2a,xRegisterMMX,xRegister32, u32>		xCVTSI2SD;
const SimdImpl_DestRegStrict<0xf3,0x2a,xRegisterSSE,xRegister32, u32>		xCVTSI2SS;

const SimdImpl_DestRegStrict<0xf3,0x5a,xRegisterSSE,xRegisterSSE,u32>		xCVTSS2SD;
const SimdImpl_DestRegStrict<0xf3,0x2d,xRegister32, xRegisterSSE,u32>		xCVTSS2SI;

const SimdImpl_DestRegStrict<0x66,0xe6,xRegisterSSE,xRegisterSSE,u128>		xCVTTPD2DQ;
const SimdImpl_DestRegStrict<0x66,0x2c,xRegisterMMX,xRegisterSSE,u128>		xCVTTPD2PI;
const SimdImpl_DestRegStrict<0xf3,0x5b,xRegisterSSE,xRegisterSSE,u128>		xCVTTPS2DQ;
const SimdImpl_DestRegStrict<0x00,0x2c,xRegisterMMX,xRegisterSSE,u64>		xCVTTPS2PI;

const SimdImpl_DestRegStrict<0xf2,0x2c,xRegister32, xRegisterSSE,u64>		xCVTTSD2SI;
const SimdImpl_DestRegStrict<0xf3,0x2c,xRegister32, xRegisterSSE,u32>		xCVTTSS2SI;

// ------------------------------------------------------------------------

void xImplSimd_DestRegSSE::operator()( const xRegisterSSE& to, const xRegisterSSE& from ) const				{ OpWriteSSE( Prefix, Opcode ); }
void xImplSimd_DestRegSSE::operator()( const xRegisterSSE& to, const ModSibBase& from ) const				{ OpWriteSSE( Prefix, Opcode ); }

void xImplSimd_DestRegImmSSE::operator()( const xRegisterSSE& to, const xRegisterSSE& from, u8 imm ) const	{ xOpWrite0F( Prefix, Opcode, to, from, imm ); }
void xImplSimd_DestRegImmSSE::operator()( const xRegisterSSE& to, const ModSibBase& from, u8 imm ) const	{ xOpWrite0F( Prefix, Opcode, to, from, imm ); }

void xImplSimd_DestRegImmMMX::operator()( const xRegisterMMX& to, const xRegisterMMX& from, u8 imm ) const	{ xOpWrite0F( Opcode, to, from, imm ); }
void xImplSimd_DestRegImmMMX::operator()( const xRegisterMMX& to, const ModSibBase& from, u8 imm ) const	{ xOpWrite0F( Opcode, to, from, imm ); }

void xImplSimd_DestRegEither::operator()( const xRegisterSSE& to, const xRegisterSSE& from ) const			{ OpWriteSSE( Prefix, Opcode ); }
void xImplSimd_DestRegEither::operator()( const xRegisterSSE& to, const ModSibBase& from ) const			{ OpWriteSSE( Prefix, Opcode ); }

void xImplSimd_DestRegEither::operator()( const xRegisterMMX& to, const xRegisterMMX& from ) const			{ OpWriteMMX( Opcode ); }
void xImplSimd_DestRegEither::operator()( const xRegisterMMX& to, const ModSibBase& from ) const			{ OpWriteMMX( Opcode ); }

// =====================================================================================================
//  SIMD Arithmetic Instructions
// =====================================================================================================

void _SimdShiftHelper::operator()( const xRegisterSSE& to, const xRegisterSSE& from ) const			{ OpWriteSSE( Prefix, Opcode ); }
void _SimdShiftHelper::operator()( const xRegisterSSE& to, const ModSibBase& from ) const			{ OpWriteSSE( Prefix, Opcode ); }

void _SimdShiftHelper::operator()( const xRegisterMMX& to, const xRegisterMMX& from ) const			{ OpWriteMMX( Opcode ); }
void _SimdShiftHelper::operator()( const xRegisterMMX& to, const ModSibBase& from ) const			{ OpWriteMMX( Opcode ); }

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

const SimdImpl_PMinMax<0xde,0x3c> xPMAX;
const SimdImpl_PMinMax<0xda,0x38> xPMIN;
const SimdImpl_PCompare xPCMP;
const SimdImpl_PShuffle xPSHUF;
const SimdImpl_PUnpack xPUNPCK;
const SimdImpl_Unpack xUNPCK;
const SimdImpl_Pack xPACK;
const SimdImpl_PInsert xPINSR;
const SimdImpl_PExtract xPEXTR;
const SimdImpl_Blend xBLEND;

const SimdImpl_PMove<true> xPMOVSX;
const SimdImpl_PMove<false> xPMOVZX;


//////////////////////////////////////////////////////////////////////////////////////////
//

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
__emitinline void xSTMXCSR( u32* dest )
{
	SimdPrefix( 0, 0xae );
	EmitSibMagic( 3, dest );
}

// Load Streaming SIMD Extension Control/Status from Mem32.
__emitinline void xLDMXCSR( const u32* src )
{
	SimdPrefix( 0, 0xae );
	EmitSibMagic( 2, src );
}

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


//////////////////////////////////////////////////////////////////////////////////////////
// INSERTPS / EXTRACTPS   [SSE4.1 only!]
//
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

}