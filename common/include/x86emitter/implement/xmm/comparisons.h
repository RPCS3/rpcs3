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

#pragma once


//////////////////////////////////////////////////////////////////////////////////////////
//
template< u16 OpcodeSSE >
class SimdImpl_MinMax
{
public:
	const SimdImpl_DestRegSSE<0x00,OpcodeSSE> PS;		// packed single precision
	const SimdImpl_DestRegSSE<0x66,OpcodeSSE> PD;		// packed double precision
	const SimdImpl_DestRegSSE<0xf3,OpcodeSSE> SS;		// scalar single precision
	const SimdImpl_DestRegSSE<0xf2,OpcodeSSE> SD;		// scalar double precision

	SimdImpl_MinMax() {}  //GChow?
};

//////////////////////////////////////////////////////////////////////////////////////////
//
template< SSE2_ComparisonType CType >
class SimdImpl_Compare
{
protected:
	template< u8 Prefix > struct Woot
	{
		__forceinline void operator()( const xRegisterSSE& to, const xRegisterSSE& from ) const	{ xOpWrite0F( Prefix, 0xc2, to, from ); xWrite8( CType ); }
		__forceinline void operator()( const xRegisterSSE& to, const ModSibBase& from ) const	{ xOpWrite0F( Prefix, 0xc2, to, from ); xWrite8( CType ); }
		Woot() {}
	};

public:
	const Woot<0x00> PS;
	const Woot<0x66> PD;
	const Woot<0xf3> SS;
	const Woot<0xf2> SD;
	SimdImpl_Compare() {} //GCWhat?
};

//////////////////////////////////////////////////////////////////////////////////////////
// Compare scalar floating point values and set EFLAGS (Ordered or Unordered)
//
template< bool Ordered >
class SimdImpl_COMI
{
protected:
	static const u16 OpcodeSSE = Ordered ? 0x2f : 0x2e;

public:
	const SimdImpl_DestRegSSE<0x00,OpcodeSSE> SS;
	const SimdImpl_DestRegSSE<0x66,OpcodeSSE> SD;
	
	SimdImpl_COMI() {}
};


//////////////////////////////////////////////////////////////////////////////////////////
//
class SimdImpl_PCompare
{
public:
	SimdImpl_PCompare() {}
	
	// Compare packed bytes for equality.
	// If a data element in dest is equal to the corresponding date element src, the
	// corresponding data element in dest is set to all 1s; otherwise, it is set to all 0s.
	const SimdImpl_DestRegEither<0x66,0x74> EQB;

	// Compare packed words for equality.
	// If a data element in dest is equal to the corresponding date element src, the
	// corresponding data element in dest is set to all 1s; otherwise, it is set to all 0s.
	const SimdImpl_DestRegEither<0x66,0x75> EQW;

	// Compare packed doublewords [32-bits] for equality.
	// If a data element in dest is equal to the corresponding date element src, the
	// corresponding data element in dest is set to all 1s; otherwise, it is set to all 0s.
	const SimdImpl_DestRegEither<0x66,0x76> EQD;

	// Compare packed signed bytes for greater than.
	// If a data element in dest is greater than the corresponding date element src, the
	// corresponding data element in dest is set to all 1s; otherwise, it is set to all 0s.
	const SimdImpl_DestRegEither<0x66,0x64> GTB;

	// Compare packed signed words for greater than.
	// If a data element in dest is greater than the corresponding date element src, the
	// corresponding data element in dest is set to all 1s; otherwise, it is set to all 0s.
	const SimdImpl_DestRegEither<0x66,0x65> GTW;

	// Compare packed signed doublewords [32-bits] for greater than.
	// If a data element in dest is greater than the corresponding date element src, the
	// corresponding data element in dest is set to all 1s; otherwise, it is set to all 0s.
	const SimdImpl_DestRegEither<0x66,0x66> GTD;
};

//////////////////////////////////////////////////////////////////////////////////////////
// 
template< u8 Opcode1, u16 Opcode2 >
class SimdImpl_PMinMax
{
public:
	SimdImpl_PMinMax() {}
	
	// Compare packed unsigned byte integers in dest to src and store packed min/max
	// values in dest.
	// Operation can be performed on either MMX or SSE operands.
	const SimdImpl_DestRegEither<0x66,Opcode1> UB;

	// Compare packed signed word integers in dest to src and store packed min/max
	// values in dest.
	// Operation can be performed on either MMX or SSE operands.
	const SimdImpl_DestRegEither<0x66,Opcode1+0x10> SW;

	// [SSE-4.1] Compare packed signed byte integers in dest to src and store 
	// packed min/max values in dest. (SSE operands only)
	const SimdImpl_DestRegSSE<0x66,(Opcode2<<8)|0x38> SB;

	// [SSE-4.1] Compare packed signed doubleword integers in dest to src and store 
	// packed min/max values in dest. (SSE operands only)
	const SimdImpl_DestRegSSE<0x66,((Opcode2+1)<<8)|0x38> SD;

	// [SSE-4.1] Compare packed unsigned word integers in dest to src and store 
	// packed min/max values in dest. (SSE operands only)
	const SimdImpl_DestRegSSE<0x66,((Opcode2+2)<<8)|0x38> UW;

	// [SSE-4.1] Compare packed unsigned doubleword integers in dest to src and store 
	// packed min/max values in dest. (SSE operands only)
	const SimdImpl_DestRegSSE<0x66,((Opcode2+3)<<8)|0x38> UD;
};
