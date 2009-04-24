/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2009  Pcsx2 Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#pragma once

//////////////////////////////////////////////////////////////////////////////////////////
// ShiftHelper -- It's out here because C++ child class template semantics are generally
// not cross-compiler friendly.
//
template< u16 Opcode1, u16 OpcodeImm, u8 Modcode >
class _SimdShiftHelper
{
public:
	_SimdShiftHelper() {}

	__forceinline void operator()( const xRegisterSSE& to, const xRegisterSSE& from ) const { xOpWrite0F( 0x66, Opcode1, to, from ); }
	__forceinline void operator()( const xRegisterSSE& to, const void* from ) const			{ xOpWrite0F( 0x66, Opcode1, to, from ); }
	__forceinline void operator()( const xRegisterSSE& to, const ModSibBase& from ) const	{ xOpWrite0F( 0x66, Opcode1, to, from ); }

	__forceinline void operator()( const xRegisterMMX& to, const xRegisterMMX& from ) const { xOpWrite0F( Opcode1, to, from ); }
	__forceinline void operator()( const xRegisterMMX& to, const void* from ) const			{ xOpWrite0F( Opcode1, to, from ); }
	__forceinline void operator()( const xRegisterMMX& to, const ModSibBase& from ) const	{ xOpWrite0F( Opcode1, to, from ); }


	__emitinline void operator()( const xRegisterSSE& to, u8 imm8 ) const
	{
		SimdPrefix( 0x66, OpcodeImm );
		EmitSibMagic( (int)Modcode, to );
		xWrite8( imm8 );
	}

	__emitinline void operator()( const xRegisterMMX& to, u8 imm8 ) const
	{
		SimdPrefix( 0x00, OpcodeImm );
		EmitSibMagic( (int)Modcode, to );
		xWrite8( imm8 );
	}
};

//////////////////////////////////////////////////////////////////////////////////////////
// Used for PSRA, which lacks the Q form.
//
template< u16 OpcodeBase1, u8 Modcode >
class SimdImpl_ShiftWithoutQ
{
public:
	const _SimdShiftHelper<OpcodeBase1+1,0x71,Modcode> W;
	const _SimdShiftHelper<OpcodeBase1+2,0x72,Modcode> D;

	SimdImpl_ShiftWithoutQ() {}
};

//////////////////////////////////////////////////////////////////////////////////////////
// Implements PSRL and PSLL
//
template< u16 OpcodeBase1, u8 Modcode >
class SimdImpl_Shift : public SimdImpl_ShiftWithoutQ<OpcodeBase1, Modcode>
{
public:
	const _SimdShiftHelper<OpcodeBase1+3,0x73,Modcode> Q;
	
	void DQ( const xRegisterSSE& to, u8 imm8 ) const
	{
		SimdPrefix( 0x66, 0x73 );
		ModRM( 3, (int)Modcode+1, to.Id );
		xWrite8( imm8 );
	}
	
	SimdImpl_Shift() {}
};


//////////////////////////////////////////////////////////////////////////////////////////
//
template< u16 OpcodeB, u16 OpcodeQ >
class SimdImpl_AddSub
{
public:
	const SimdImpl_DestRegEither<0x66,OpcodeB+0x20> B;
	const SimdImpl_DestRegEither<0x66,OpcodeB+0x21> W;
	const SimdImpl_DestRegEither<0x66,OpcodeB+0x22> D;
	const SimdImpl_DestRegEither<0x66,OpcodeQ> Q;

	// Add/Sub packed signed byte [8bit] integers from src into dest, and saturate the results.
	const SimdImpl_DestRegEither<0x66,OpcodeB+0x10> SB;

	// Add/Sub packed signed word [16bit] integers from src into dest, and saturate the results.
	const SimdImpl_DestRegEither<0x66,OpcodeB+0x11> SW;

	// Add/Sub packed unsigned byte [8bit] integers from src into dest, and saturate the results.
	const SimdImpl_DestRegEither<0x66,OpcodeB> USB;

	// Add/Sub packed unsigned word [16bit] integers from src into dest, and saturate the results.
	const SimdImpl_DestRegEither<0x66,OpcodeB+1> USW;

	SimdImpl_AddSub() {}
};

//////////////////////////////////////////////////////////////////////////////////////////
//
class SimdImpl_PMul
{
public:
	const SimdImpl_DestRegEither<0x66,0xd5> LW;
	const SimdImpl_DestRegEither<0x66,0xe5> HW;
	const SimdImpl_DestRegEither<0x66,0xe4> HUW;
	const SimdImpl_DestRegEither<0x66,0xf4> UDQ;

	// [SSE-3] PMULHRSW multiplies vertically each signed 16-bit integer from dest with the
	// corresponding signed 16-bit integer of source, producing intermediate signed 32-bit
	// integers. Each intermediate 32-bit integer is truncated to the 18 most significant
	// bits. Rounding is always performed by adding 1 to the least significant bit of the
	// 18-bit intermediate result. The final result is obtained by selecting the 16 bits
	// immediately to the right of the most significant bit of each 18-bit intermediate
	// result and packed to the destination operand.
	//
	// Both operands can be MMX or XMM registers.  Source can be register or memory.
	//
	const SimdImpl_DestRegEither<0x66,0x0b38> HRSW;
	
	// [SSE-4.1] Multiply the packed dword signed integers in dest with src, and store
	// the low 32 bits of each product in xmm1.
	const SimdImpl_DestRegSSE<0x66,0x4038> LD;
	
	// [SSE-4.1] Multiply the packed signed dword integers in dest with src.
	const SimdImpl_DestRegSSE<0x66,0x2838> DQ;
	
	SimdImpl_PMul() {}
};

//////////////////////////////////////////////////////////////////////////////////////////
// For instructions that have PS/SS form only (most commonly reciprocal Sqrt functions)
//
template< u16 OpcodeSSE >
class SimdImpl_rSqrt
{
public:
	const SimdImpl_DestRegSSE<0x00,OpcodeSSE> PS;
	const SimdImpl_DestRegSSE<0xf3,OpcodeSSE> SS;
	SimdImpl_rSqrt() {}
};

//////////////////////////////////////////////////////////////////////////////////////////
// SQRT has PS/SS/SD forms, but not the PD form.
//
template< u16 OpcodeSSE >
class SimdImpl_Sqrt : public SimdImpl_rSqrt<OpcodeSSE>
{
public:
	SimdImpl_Sqrt() {}
	const SimdImpl_DestRegSSE<0xf2,OpcodeSSE> SD;
};

//////////////////////////////////////////////////////////////////////////////////////////
//
class SimdImpl_AndNot
{
public:
	SimdImpl_AndNot() {}
	const SimdImpl_DestRegSSE<0x00,0x55> PS;
	const SimdImpl_DestRegSSE<0x66,0x55> PD;
};

//////////////////////////////////////////////////////////////////////////////////////////
// Packed absolute value. [sSSE3 only]
//
class SimdImpl_PAbsolute
{
public:
	SimdImpl_PAbsolute() {}
	
	// [sSSE-3] Computes the absolute value of bytes in the src, and stores the result
	// in dest, as UNSIGNED.
	const SimdImpl_DestRegEither<0x66, 0x1c38> B;

	// [sSSE-3] Computes the absolute value of word in the src, and stores the result
	// in dest, as UNSIGNED.
	const SimdImpl_DestRegEither<0x66, 0x1d38> W;

	// [sSSE-3] Computes the absolute value of doublewords in the src, and stores the
	// result in dest, as UNSIGNED.
	const SimdImpl_DestRegEither<0x66, 0x1e38> D;
};

//////////////////////////////////////////////////////////////////////////////////////////
// Packed Sign [sSSE3 only] - Negate/zero/preserve packed integers in dest depending on the
// corresponding sign in src.
//
class SimdImpl_PSign
{
public:
	SimdImpl_PSign() {}

	// [sSSE-3] negates each byte element of dest if the signed integer value of the
	// corresponding data element in src is less than zero. If the signed integer value
	// of a data element in src is positive, the corresponding data element in dest is
	// unchanged. If a data element in src is zero, the corresponding data element in
	// dest is set to zero.
	const SimdImpl_DestRegEither<0x66, 0x0838> B;

	// [sSSE-3] negates each word element of dest if the signed integer value of the
	// corresponding data element in src is less than zero. If the signed integer value
	// of a data element in src is positive, the corresponding data element in dest is
	// unchanged. If a data element in src is zero, the corresponding data element in
	// dest is set to zero.
	const SimdImpl_DestRegEither<0x66, 0x0938> W;

	// [sSSE-3] negates each doubleword element of dest if the signed integer value
	// of the corresponding data element in src is less than zero. If the signed integer
	// value of a data element in src is positive, the corresponding data element in dest
	// is unchanged. If a data element in src is zero, the corresponding data element in
	// dest is set to zero.
	const SimdImpl_DestRegEither<0x66, 0x0a38> D;

};

//////////////////////////////////////////////////////////////////////////////////////////
// Packed Multiply and Add!!
//
class SimdImpl_PMultAdd
{
public:
	SimdImpl_PMultAdd() {}

	// Multiplies the individual signed words of dest by the corresponding signed words
	// of src, producing temporary signed, doubleword results. The adjacent doubleword
	// results are then summed and stored in the destination operand.
	//
	//   DEST[31:0]  = ( DEST[15:0]  * SRC[15:0])  + (DEST[31:16] * SRC[31:16] );
	//   DEST[63:32] = ( DEST[47:32] * SRC[47:32]) + (DEST[63:48] * SRC[63:48] );
	//   [.. repeat in the case of XMM src/dest operands ..]
	//
	const SimdImpl_DestRegEither<0x66, 0xf5> WD;

	// [sSSE-3] multiplies vertically each unsigned byte of dest with the corresponding
	// signed byte of src, producing intermediate signed 16-bit integers. Each adjacent
	// pair of signed words is added and the saturated result is packed to dest.
	// For example, the lowest-order bytes (bits 7-0) in src and dest are multiplied
	// and the intermediate signed word result is added with the corresponding
	// intermediate result from the 2nd lowest-order bytes (bits 15-8) of the operands;
	// the sign-saturated result is stored in the lowest word of dest (bits 15-0).
	// The same operation is performed on the other pairs of adjacent bytes.
	//
	// In Coder Speak:
	//   DEST[15-0]  = SaturateToSignedWord( SRC[15-8]  * DEST[15-8]  + SRC[7-0]   * DEST[7-0]   );
	//   DEST[31-16] = SaturateToSignedWord( SRC[31-24] * DEST[31-24] + SRC[23-16] * DEST[23-16] );
	//   [.. repeat for each 16 bits up to 64 (mmx) or 128 (xmm) ..]
	//
	const SimdImpl_DestRegEither<0x66, 0xf438> UBSW;
};

//////////////////////////////////////////////////////////////////////////////////////////
// Packed Horizontal Add [SSE3 only]
//
class SimdImpl_HorizAdd
{
public:
	SimdImpl_HorizAdd() {}
	
	// [SSE-3] Horizontal Add of Packed Data.  A three step process:
	// * Adds the single-precision floating-point values in the first and second dwords of
	//   dest and stores the result in the first dword of dest.
	// * Adds single-precision floating-point values in the third and fourth dword of dest
	//   stores the result in the second dword of dest.
	// * Adds single-precision floating-point values in the first and second dword of *src*
	//   and stores the result in the third dword of dest.
	const SimdImpl_DestRegSSE<0xf2, 0x7c> PS;
	
	// [SSE-3] Horizontal Add of Packed Data.  A two step process:
	// * Adds the double-precision floating-point values in the high and low quadwords of
	//   dest and stores the result in the low quadword of dest.
	// * Adds the double-precision floating-point values in the high and low quadwords of
	//   *src* stores the result in the high quadword of dest.
	const SimdImpl_DestRegSSE<0x66, 0x7c> PD;
};

//////////////////////////////////////////////////////////////////////////////////////////
// DotProduct calculation (SSE4.1 only!)
//
class SimdImpl_DotProduct
{
public:
	SimdImpl_DotProduct() {}

	// [SSE-4.1] Conditionally multiplies the packed single precision floating-point
	// values in dest with the packed single-precision floats in src depending on a
	// mask extracted from the high 4 bits of the immediate byte. If a condition mask
	// bit in Imm8[7:4] is zero, the corresponding multiplication is replaced by a value
	// of 0.0.	The four resulting single-precision values are summed into an inter-
	// mediate result. 
	//
	// The intermediate result is conditionally broadcasted to the destination using a
	// broadcast mask specified by bits [3:0] of the immediate byte. If a broadcast
	// mask bit is 1, the intermediate result is copied to the corresponding dword
	// element in dest.  If a broadcast mask bit is zero, the corresponding element in
	// the destination is set to zero.
	//
	SimdImpl_DestRegImmSSE<0x66,0x403a> PS;

	// [SSE-4.1]
	SimdImpl_DestRegImmSSE<0x66,0x413a> PD;
};

//////////////////////////////////////////////////////////////////////////////////////////
// Rounds floating point values (packed or single scalar) by an arbitrary rounding mode.
// (SSE4.1 only!)
class SimdImpl_Round
{
public:
	SimdImpl_Round() {}

	// [SSE-4.1] Rounds the 4 packed single-precision src values and stores them in dest.
	//
	// Imm8 specifies control fields for the rounding operation:
	//   Bit  3 - processor behavior for a precision exception (0: normal, 1: inexact)
	//   Bit  2 - If enabled, use MXCSR.RC, else use RC specified in bits 1:0 of this Imm8.
	//   Bits 1:0 - Specifies a rounding mode for this instruction only.
	//
	// Rounding Mode Reference:
	//   0 - Nearest, 1 - Negative Infinity, 2 - Positive infinity, 3 - Truncate.
	//
	const SimdImpl_DestRegImmSSE<0x66,0x083a> PS;

	// [SSE-4.1] Rounds the 2 packed double-precision src values and stores them in dest.
	//
	// Imm8 specifies control fields for the rounding operation:
	//   Bit  3 - processor behavior for a precision exception (0: normal, 1: inexact)
	//   Bit  2 - If enabled, use MXCSR.RC, else use RC specified in bits 1:0 of this Imm8.
	//   Bits 1:0 - Specifies a rounding mode for this instruction only.
	//
	// Rounding Mode Reference:
	//   0 - Nearest, 1 - Negative Infinity, 2 - Positive infinity, 3 - Truncate.
	//
	const SimdImpl_DestRegImmSSE<0x66,0x093a> PD;

	// [SSE-4.1] Rounds the single-precision src value and stores in dest.
	//
	// Imm8 specifies control fields for the rounding operation:
	//   Bit  3 - processor behavior for a precision exception (0: normal, 1: inexact)
	//   Bit  2 - If enabled, use MXCSR.RC, else use RC specified in bits 1:0 of this Imm8.
	//   Bits 1:0 - Specifies a rounding mode for this instruction only.
	//
	// Rounding Mode Reference:
	//   0 - Nearest, 1 - Negative Infinity, 2 - Positive infinity, 3 - Truncate.
	//
	const SimdImpl_DestRegImmSSE<0x66,0x0a3a> SS;

	// [SSE-4.1] Rounds the double-precision src value and stores in dest.
	//
	// Imm8 specifies control fields for the rounding operation:
	//   Bit  3 - processor behavior for a precision exception (0: normal, 1: inexact)
	//   Bit  2 - If enabled, use MXCSR.RC, else use RC specified in bits 1:0 of this Imm8.
	//   Bits 1:0 - Specifies a rounding mode for this instruction only.
	//
	// Rounding Mode Reference:
	//   0 - Nearest, 1 - Negative Infinity, 2 - Positive infinity, 3 - Truncate.
	//
	const SimdImpl_DestRegImmSSE<0x66,0x0b3a> SD;
};
