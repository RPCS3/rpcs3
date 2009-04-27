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

enum G3Type
{
	G3Type_NOT	= 2,
	G3Type_NEG	= 3,
	G3Type_MUL	= 4,
	G3Type_iMUL	= 5,		// partial implementation, iMul has additional forms in ix86.cpp
	G3Type_DIV	= 6,
	G3Type_iDIV	= 7
};

//////////////////////////////////////////////////////////////////////////////////////////
//
template< G3Type InstType >
class xImpl_Group3
{
public:
	// ------------------------------------------------------------------------
	template< typename T > __emitinline void operator()( const xRegister<T>& from ) const
	{
		prefix16<T>();
		xWrite8(Is8BitOp<T>() ? 0xf6 : 0xf7 );
		EmitSibMagic( InstType, from );
	}

	// ------------------------------------------------------------------------
	template< typename T > __emitinline void operator()( const ModSibStrict<T>& from ) const
	{
		prefix16<T>();
		xWrite8( Is8BitOp<T>() ? 0xf6 : 0xf7 );
		EmitSibMagic( InstType, from );
	}
	
	xImpl_Group3() {}
};

// ------------------------------------------------------------------------
// This class combines x86 and SSE/SSE2 instructions for iMUL and iDIV.
//
template< G3Type InstType, u16 OpcodeSSE >
class ImplMulDivBase : public xImpl_Group3<InstType>
{
public:
	ImplMulDivBase() {}
	const SimdImpl_DestRegSSE<0x00,OpcodeSSE> PS;
	const SimdImpl_DestRegSSE<0x66,OpcodeSSE> PD;
	const SimdImpl_DestRegSSE<0xf3,OpcodeSSE> SS;
	const SimdImpl_DestRegSSE<0xf2,OpcodeSSE> SD;
};

//////////////////////////////////////////////////////////////////////////////////////////
//
class xImpl_iDiv : public ImplMulDivBase<G3Type_iDIV,0x5e>
{
public:
	using ImplMulDivBase<G3Type_iDIV,0x5e>::operator();
};

//////////////////////////////////////////////////////////////////////////////////////////
// 	The following iMul-specific forms are valid for 16 and 32 bit register operands only!
//
class xImpl_iMul : public ImplMulDivBase<G3Type_iMUL,0x59>
{
	template< typename T1, typename T2, typename ImmType >
	static __forceinline void ImmStyle( const T1& param1, const T2& param2, ImmType imm8 )
	{
		xOpWrite0F( (sizeof(ImmType) == 2) ? 0x66 : 0, is_s8( imm8 ) ? 0x6b : 0x69, param1, param2 );
		if( is_s8( imm8 ) )
			xWrite8( imm8 );
		else
			xWrite<ImmType>( imm8 );
	}

public:
	using ImplMulDivBase<G3Type_iMUL,0x59>::operator();
	
	__forceinline void operator()( const xRegister32& to,	const xRegister32& from ) const			{ xOpWrite0F( 0xaf, to, from ); }
	__forceinline void operator()( const xRegister32& to,	const void* src ) const					{ xOpWrite0F( 0xaf, to, src ); }
	__forceinline void operator()( const xRegister32& to,	const ModSibBase& src ) const			{ xOpWrite0F( 0xaf, to, src ); }
	__forceinline void operator()( const xRegister32& to,	const xRegister32& from, s32 imm ) const{ ImmStyle( to, from, imm ); }
	__forceinline void operator()( const xRegister32& to,	const ModSibBase& from, s32 imm ) const	{ ImmStyle( to, from, imm ); }

	__forceinline void operator()( const xRegister16& to,	const xRegister16& from ) const			{ xOpWrite0F( 0x66, 0xaf, to, from ); }
	__forceinline void operator()( const xRegister16& to,	const void* src ) const					{ xOpWrite0F( 0x66, 0xaf, to, src ); }
	__forceinline void operator()( const xRegister16& to,	const ModSibBase& src ) const			{ xOpWrite0F( 0x66, 0xaf, to, src ); }
	__forceinline void operator()( const xRegister16& to,	const xRegister16& from, s16 imm ) const{ ImmStyle( to, from, imm ); }
	__forceinline void operator()( const xRegister16& to,	const ModSibBase& from, s16 imm ) const	{ ImmStyle( to, from, imm ); }

	xImpl_iMul() {}
};
