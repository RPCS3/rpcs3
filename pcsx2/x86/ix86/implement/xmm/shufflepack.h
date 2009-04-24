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
//
template< u16 OpcodeSSE >
class SimdImpl_Shuffle
{
protected:
	template< u8 Prefix > struct Woot
	{
		__forceinline void operator()( const xRegisterSSE& to, const xRegisterSSE& from, u8 cmptype ) const	{ xOpWrite0F( Prefix, OpcodeSSE, to, from ); xWrite8( cmptype ); }
		__forceinline void operator()( const xRegisterSSE& to, const void* from, u8 cmptype ) const			{ xOpWrite0F( Prefix, OpcodeSSE, to, from ); xWrite8( cmptype ); }
		__forceinline void operator()( const xRegisterSSE& to, const ModSibBase& from, u8 cmptype ) const	{ xOpWrite0F( Prefix, OpcodeSSE, to, from ); xWrite8( cmptype ); }
		Woot() {}
	};

public:
	const Woot<0x00> PS;
	const Woot<0x66> PD;

	SimdImpl_Shuffle() {} //GCWhat?
};

//////////////////////////////////////////////////////////////////////////////////////////
//
class SimdImpl_PShuffle
{
public:
	SimdImpl_PShuffle() {}
	
	// Copies words from src and inserts them into dest at word locations selected with
	// the order operand (8 bit immediate).
	const SimdImpl_DestRegImmMMX<0x00,0x70> W;

	// Copies doublewords from src and inserts them into dest at dword locations selected
	// with the order operand (8 bit immediate).
	const SimdImpl_DestRegImmSSE<0x66,0x70> D;
	
	// Copies words from the low quadword of src and inserts them into the low quadword
	// of dest at word locations selected with the order operand (8 bit immediate).
	// The high quadword of src is copied to the high quadword of dest.
	const SimdImpl_DestRegImmSSE<0xf2,0x70> LW;

	// Copies words from the high quadword of src and inserts them into the high quadword
	// of dest at word locations selected with the order operand (8 bit immediate).
	// The low quadword of src is copied to the low quadword of dest.
	const SimdImpl_DestRegImmSSE<0xf3,0x70> HW;

	// [sSSE-3] Performs in-place shuffles of bytes in dest according to the shuffle
	// control mask in src.  If the most significant bit (bit[7]) of each byte of the
	// shuffle control mask is set, then constant zero is written in the result byte.
	// Each byte in the shuffle control mask forms an index to permute the corresponding
	// byte in dest. The value of each index is the least significant 4 bits (128-bit
	// operation) or 3 bits (64-bit operation) of the shuffle control byte.
	//
	// Operands can be MMX or XMM registers.
	const SimdImpl_DestRegEither<0x66,0x0038> B;
};

//////////////////////////////////////////////////////////////////////////////////////////
//
class SimdImpl_PUnpack
{
public:
	SimdImpl_PUnpack() {}
	
	// Unpack and interleave low-order bytes from src and dest into dest.
	const SimdImpl_DestRegEither<0x66,0x60> LBW;
	// Unpack and interleave low-order words from src and dest into dest.
	const SimdImpl_DestRegEither<0x66,0x61> LWD;
	// Unpack and interleave low-order doublewords from src and dest into dest.
	const SimdImpl_DestRegEither<0x66,0x62> LDQ;
	// Unpack and interleave low-order quadwords from src and dest into dest.
	const SimdImpl_DestRegSSE<0x66,0x6c> LQDQ;

	// Unpack and interleave high-order bytes from src and dest into dest.
	const SimdImpl_DestRegEither<0x66,0x68> HBW;
	// Unpack and interleave high-order words from src and dest into dest.
	const SimdImpl_DestRegEither<0x66,0x69> HWD;
	// Unpack and interleave high-order doublewords from src and dest into dest.
	const SimdImpl_DestRegEither<0x66,0x6a> HDQ;
	// Unpack and interleave high-order quadwords from src and dest into dest.
	const SimdImpl_DestRegSSE<0x66,0x6d> HQDQ;
};

//////////////////////////////////////////////////////////////////////////////////////////
// Pack with Signed or Unsigned Saturation
//
class SimdImpl_Pack
{
public:
	SimdImpl_Pack() {}

	// Converts packed signed word integers from src and dest into packed signed 
	// byte integers in dest, using signed saturation.
	const SimdImpl_DestRegEither<0x66,0x63> SSWB;

	// Converts packed signed dword integers from src and dest into packed signed 
	// word integers in dest, using signed saturation.
	const SimdImpl_DestRegEither<0x66,0x6b> SSDW;

	// Converts packed unsigned word integers from src and dest into packed unsigned 
	// byte integers in dest, using unsigned saturation.
	const SimdImpl_DestRegEither<0x66,0x67> USWB;

	// [SSE-4.1] Converts packed unsigned dword integers from src and dest into packed
	// unsigned word integers in dest, using signed saturation.
	const SimdImpl_DestRegSSE<0x66,0x2b38> USDW;
};


//////////////////////////////////////////////////////////////////////////////////////////
//
class SimdImpl_Unpack
{
public:
	SimdImpl_Unpack() {}

	// Unpacks the high doubleword [single-precision] values from src and dest into
	// dest, such that the result of dest looks like this:
	//    dest[0] <- dest[2]
	//    dest[1] <- src[2]
	//    dest[2] <- dest[3]
	//    dest[3] <- src[3]
	//
	const SimdImpl_DestRegSSE<0x00,0x15> HPS;

	// Unpacks the high quadword [double-precision] values from src and dest into
	// dest, such that the result of dest looks like this:
	//    dest.lo <- dest.hi
	//    dest.hi <- src.hi
	//
	const SimdImpl_DestRegSSE<0x66,0x15> HPD;

	// Unpacks the low doubleword [single-precision] values from src and dest into
	// dest, such that the result of dest looks like this:
	//    dest[3] <- src[1]
	//    dest[2] <- dest[1]
	//    dest[1] <- src[0]
	//    dest[0] <- dest[0]
	//
	const SimdImpl_DestRegSSE<0x00,0x14> LPS;

	// Unpacks the low quadword [double-precision] values from src and dest into
	// dest, effectively moving the low portion of src into the upper portion of dest.
	// The result of dest is loaded as such:
	//    dest.hi <- src.lo
	//    dest.lo <- dest.lo  [remains unchanged!]
	//
	const SimdImpl_DestRegSSE<0x66,0x14> LPD;
};

//////////////////////////////////////////////////////////////////////////////////////////
// PINSRW/B/D [all but Word form are SSE4.1 only!]
//
class SimdImpl_PInsert
{
protected:
	template< u16 Opcode >
	class ByteDwordForms
	{
	public:
		ByteDwordForms() {}
		
		__forceinline void operator()( const xRegisterSSE& to, const xRegister32& from, u8 imm8 ) const
		{
			xOpWrite0F( 0x66, (Opcode<<8) | 0x3a, to, from, imm8 );
		}

		__forceinline void operator()( const xRegisterSSE& to, const void* from, u8 imm8 ) const
		{
			xOpWrite0F( 0x66, (Opcode<<8) | 0x3a, to, from, imm8 );
		}

		__forceinline void operator()( const xRegisterSSE& to, const ModSibBase& from, u8 imm8 ) const
		{
			xOpWrite0F( 0x66, (Opcode<<8) | 0x3a, to, from, imm8 );
		}
	};
	
public:
	SimdImpl_PInsert() {}

	// Operation can be performed on either MMX or SSE src operands.
	__forceinline void W( const xRegisterSSE& to, const xRegister32& from, u8 imm8 ) const	{ xOpWrite0F( 0x66, 0xc4, to, from, imm8 ); }
	__forceinline void W( const xRegisterSSE& to, const void* from, u8 imm8 ) const			{ xOpWrite0F( 0x66, 0xc4, to, from, imm8 ); }
	__forceinline void W( const xRegisterSSE& to, const ModSibBase& from, u8 imm8 ) const	{ xOpWrite0F( 0x66, 0xc4, to, from, imm8 ); }

	__forceinline void W( const xRegisterMMX& to, const xRegister32& from, u8 imm8 ) const	{ xOpWrite0F( 0xc4, to, from, imm8 ); }
	__forceinline void W( const xRegisterMMX& to, const void* from, u8 imm8 ) const			{ xOpWrite0F( 0xc4, to, from, imm8 ); }
	__forceinline void W( const xRegisterMMX& to, const ModSibBase& from, u8 imm8 ) const	{ xOpWrite0F( 0xc4, to, from, imm8 ); }

	// [SSE-4.1] 
	const ByteDwordForms<0x20> B;

	// [SSE-4.1]
	const ByteDwordForms<0x22> D;
};


//////////////////////////////////////////////////////////////////////////////////////////
// PEXTRW/B/D [all but Word form are SSE4.1 only!]
//
// Note: Word form's indirect memory form is only available in SSE4.1.
//
class SimdImpl_PExtract
{
protected:
	template< u16 Opcode >
	class ByteDwordForms
	{
	public:
		ByteDwordForms() {}

		__forceinline void operator()( const xRegister32& to, const xRegisterSSE& from, u8 imm8 ) const
		{
			xOpWrite0F( 0x66, (Opcode<<8) | 0x3a, to, from, imm8 );
		}

		__forceinline void operator()( void* dest, const xRegisterSSE& from, u8 imm8 ) const
		{
			xOpWrite0F( 0x66, (Opcode<<8) | 0x3a, from, dest, imm8 );
		}

		__forceinline void operator()( const ModSibBase& dest, const xRegisterSSE& from, u8 imm8 ) const
		{
			xOpWrite0F( 0x66, (Opcode<<8) | 0x3a, from, dest, imm8 );
		}
	};

public:
	SimdImpl_PExtract() {}

	// Copies the word element specified by imm8 from src to dest.  The upper bits
	// of dest are zero-extended (cleared).  This can be used to extract any single packed
	// word value from src into an x86 32 bit register.
	//
	// [SSE-4.1] Note: Indirect memory forms of this instruction are an SSE-4.1 extension!
	//
	__forceinline void W( const xRegister32& to, const xRegisterSSE& from, u8 imm8 ) const		{ xOpWrite0F( 0x66, 0xc5, to, from, imm8 ); }
	__forceinline void W( const xRegister32& to, const xRegisterMMX& from, u8 imm8 ) const		{ xOpWrite0F( 0xc5, to, from, imm8 ); }

	__forceinline void W( void* dest, const xRegisterSSE& from, u8 imm8 ) const					{ xOpWrite0F( 0x66, 0x153a, from, dest, imm8 ); }
	__forceinline void W( const ModSibBase& dest, const xRegisterSSE& from, u8 imm8 ) const		{ xOpWrite0F( 0x66, 0x153a, from, dest, imm8 ); }

	// [SSE-4.1] Copies the byte element specified by imm8 from src to dest.  The upper bits
	// of dest are zero-extended (cleared).  This can be used to extract any single packed
	// byte value from src into an x86 32 bit register.
	const ByteDwordForms<0x14> B;

	// [SSE-4.1] Copies the dword element specified by imm8 from src to dest.  This can be
	// used to extract any single packed dword value from src into an x86 32 bit register.
	const ByteDwordForms<0x16> D;
};

