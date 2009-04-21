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
// Used for PSRA, which lacks the Q form.
//
template< u16 OpcodeBase1, u8 Modcode >
class SimdImpl_ShiftWithoutQ
{
protected:
	template< u16 Opcode1, u16 OpcodeImm >
	class ShiftHelper
	{
	public:
		ShiftHelper() {}

		template< typename OperandType >
		__forceinline void operator()( const xRegisterSIMD<OperandType>& to, const xRegisterSIMD<OperandType>& from ) const
		{
			writeXMMop( 0x66, Opcode1, to, from );
		}

		template< typename OperandType >
		__forceinline void operator()( const xRegisterSIMD<OperandType>& to, const void* from ) const
		{
			writeXMMop( 0x66, Opcode1, to, from );
		}

		template< typename OperandType >
		__noinline void operator()( const xRegisterSIMD<OperandType>& to, const ModSibBase& from ) const
		{
			writeXMMop( 0x66, Opcode1, to, from );
		}

		template< typename OperandType >
		__emitinline void operator()( const xRegisterSIMD<OperandType>& to, u8 imm8 ) const
		{
			SimdPrefix( (sizeof( OperandType ) == 16) ? 0x66 : 0, OpcodeImm );
			ModRM( 3, (int)Modcode, to.Id );
			xWrite<u8>( imm8 );
		}
	};

public:
	const ShiftHelper<OpcodeBase1+1,0x71> W;
	const ShiftHelper<OpcodeBase1+2,0x72> D;

	SimdImpl_ShiftWithoutQ() {}
};

//////////////////////////////////////////////////////////////////////////////////////////
// Implements PSRL and PSLL
//
template< u16 OpcodeBase1, u8 Modcode >
class SimdImpl_Shift : public SimdImpl_ShiftWithoutQ<OpcodeBase1, Modcode>
{
public:
	const ShiftHelper<OpcodeBase1+3,0x73> Q;
	
	void DQ( const xRegisterSSE& to, u8 imm ) const
	{
		SimdPrefix( 0x66, 0x73 );
		ModRM( 3, (int)Modcode+1, to.Id );
		xWrite<u8>( imm );
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
	const SimdImpl_DestRegSSE<0xf2,OpcodeSSE> SD;
	SimdImpl_Sqrt() {}
};

//////////////////////////////////////////////////////////////////////////////////////////
//
class SimdImpl_AndNot
{
public:
	const SimdImpl_DestRegSSE<0x00,0x55> PS;
	const SimdImpl_DestRegSSE<0x66,0x55> PD;
	SimdImpl_AndNot() {}
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
