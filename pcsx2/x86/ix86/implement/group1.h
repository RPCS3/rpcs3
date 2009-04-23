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

// Note: This header is meant to be included from within the x86Emitter::Internal namespace.
// Instructions implemented in this header are as follows -->>

enum G1Type
{
	G1Type_ADD=0,
	G1Type_OR,
	G1Type_ADC,
	G1Type_SBB,
	G1Type_AND,
	G1Type_SUB,
	G1Type_XOR,
	G1Type_CMP
};

// -------------------------------------------------------------------
//
template< G1Type InstType >
class xImpl_Group1
{
public:
	// ------------------------------------------------------------------------
	template< typename T > __forceinline void operator()( const xRegister<T>& to, const xRegister<T>& from ) const
	{
		prefix16<T>();
		xWrite<u8>( (Is8BitOp<T>() ? 0 : 1) | (InstType<<3) );
		ModRM_Direct( from.Id, to.Id );
	}

	// ------------------------------------------------------------------------
	template< typename T > __forceinline void operator()( const xRegister<T>& to, const void* src ) const
	{
		prefix16<T>();
		xWrite<u8>( (Is8BitOp<T>() ? 2 : 3) | (InstType<<3) );
		xWriteDisp( to.Id, src );
	}
	
	// ------------------------------------------------------------------------
	template< typename T > __forceinline void operator()( void* dest, const xRegister<T>& from ) const
	{
		prefix16<T>();
		xWrite<u8>( (Is8BitOp<T>() ? 0 : 1) | (InstType<<3) ); 
		xWriteDisp( from.Id, dest );
	}

	// ------------------------------------------------------------------------
	template< typename T > __noinline void operator()( const ModSibBase& sibdest, const xRegister<T>& from ) const
	{
		prefix16<T>();
		xWrite<u8>( (Is8BitOp<T>() ? 0 : 1) | (InstType<<3) ); 
		EmitSibMagic( from.Id, sibdest );
	}

	// ------------------------------------------------------------------------
	template< typename T > __noinline void operator()( const xRegister<T>& to, const ModSibBase& sibsrc ) const
	{
		prefix16<T>();
		xWrite<u8>( (Is8BitOp<T>() ? 2 : 3) | (InstType<<3) );
		EmitSibMagic( to.Id, sibsrc );
	}

	// ------------------------------------------------------------------------
	// Note on Imm forms : use int as the source operand since it's "reasonably inert" from a compiler
	// perspective.  (using uint tends to make the compiler try and fail to match signed immediates with
	// one of the other overloads).
	
	template< typename T > __noinline void operator()( const ModSibStrict<T>& sibdest, int imm ) const
	{
		if( Is8BitOp<T>() )
		{
			xWrite<u8>( 0x80 );
			EmitSibMagic( InstType, sibdest );
			xWrite<s8>( imm );
		}
		else
		{		
			prefix16<T>();
			xWrite<u8>( is_s8( imm ) ? 0x83 : 0x81 );
			EmitSibMagic( InstType, sibdest );
			if( is_s8( imm ) )
				xWrite<s8>( imm );
			else
				xWrite<T>( imm );
		}
	}

	// ------------------------------------------------------------------------
	template< typename T > __forceinline void operator()( const xRegister<T>& to, int imm ) const
	{
		prefix16<T>();
		if( !Is8BitOp<T>() && is_s8( imm ) )
		{
			xWrite<u8>( 0x83 );
			ModRM_Direct( InstType, to.Id );
			xWrite<s8>( imm );
		}
		else
		{
			if( to.IsAccumulator() )
				xWrite<u8>( (Is8BitOp<T>() ? 4 : 5) | (InstType<<3) );
			else
			{
				xWrite<u8>( Is8BitOp<T>() ? 0x80 : 0x81 );
				ModRM_Direct( InstType, to.Id );
			}
			xWrite<T>( imm );
		}
	}

	xImpl_Group1() {}		// Why does GCC need these?
};

// ------------------------------------------------------------------------
// This class combines x86 with SSE/SSE2 logic operations (ADD, OR, and NOT).
// Note: ANDN [AndNot] is handled below separately.
//
template< G1Type InstType, u16 OpcodeSSE >
class xImpl_G1Logic : public xImpl_Group1<InstType>
{
public:
	using xImpl_Group1<InstType>::operator();

	const SimdImpl_DestRegSSE<0x00,OpcodeSSE> PS;		// packed single precision
	const SimdImpl_DestRegSSE<0x66,OpcodeSSE> PD;		// packed double precision

	xImpl_G1Logic() {}
};

// ------------------------------------------------------------------------
// This class combines x86 with SSE/SSE2 arithmetic operations (ADD/SUB).
//
template< G1Type InstType, u16 OpcodeSSE >
class xImpl_G1Arith : public xImpl_G1Logic<InstType, OpcodeSSE >
{
public:
	using xImpl_Group1<InstType>::operator();

	const SimdImpl_DestRegSSE<0xf3,OpcodeSSE> SS;		// scalar single precision
	const SimdImpl_DestRegSSE<0xf2,OpcodeSSE> SD;		// scalar double precision

	xImpl_G1Arith() {}
};

// ------------------------------------------------------------------------
class xImpl_G1Compare : xImpl_Group1< G1Type_CMP >
{
protected:
	template< u8 Prefix > struct Woot
	{
		__forceinline void operator()( const xRegisterSSE& to, const xRegisterSSE& from, SSE2_ComparisonType cmptype ) const{ xOpWrite0F( Prefix, 0xc2, to, from ); xWrite<u8>( cmptype ); }
		__forceinline void operator()( const xRegisterSSE& to, const void* from, SSE2_ComparisonType cmptype ) const		{ xOpWrite0F( Prefix, 0xc2, to, from ); xWrite<u8>( cmptype ); }
		__forceinline void operator()( const xRegisterSSE& to, const ModSibBase& from, SSE2_ComparisonType cmptype ) const	{ xOpWrite0F( Prefix, 0xc2, to, from ); xWrite<u8>( cmptype ); }
		Woot() {}
	};

public:
	using xImpl_Group1< G1Type_CMP >::operator();

	const Woot<0x00> PS;
	const Woot<0x66> PD;
	const Woot<0xf3> SS;
	const Woot<0xf2> SD;

	xImpl_G1Compare() {} //GCWhat?
};
