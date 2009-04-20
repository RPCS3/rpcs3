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

// ------------------------------------------------------------------------
template< typename ImmType >
class Group3Impl
{
protected:
	static const uint OperandSize = sizeof(ImmType);

	static bool Is8BitOperand()	{ return OperandSize == 1; }
	static void prefix16()		{ if( OperandSize == 2 ) xWrite<u8>( 0x66 ); }

public: 
	Group3Impl() {}		// For the love of GCC.

	static __emitinline void Emit( G3Type InstType, const xRegister<ImmType>& from )
	{
		prefix16();
		xWrite<u8>(Is8BitOperand() ? 0xf6 : 0xf7 );
		ModRM_Direct( InstType, from.Id );
	}

	static __emitinline void Emit( G3Type InstType, const ModSibStrict<ImmType>& sibsrc )
	{
		prefix16();
		xWrite<u8>( Is8BitOperand() ? 0xf6 : 0xf7 );
		EmitSibMagic( InstType, sibsrc );
	}
};

// -------------------------------------------------------------------
//
template< G3Type InstType >
class Group3ImplAll
{
public:
	template< typename T >
	__forceinline void operator()( const xRegister<T>& from ) const	{ Group3Impl<T>::Emit( InstType, from ); }

	template< typename T >
	__noinline void operator()( const ModSibStrict<T>& from ) const { Group3Impl<T>::Emit( InstType, from ); }
	
	Group3ImplAll() {}
};

// ------------------------------------------------------------------------
// This class combines x86 and SSE/SSE2 instructions for iMUL and iDIV.
//
template< G3Type InstType, u8 OpcodeSSE >
class G3Impl_PlusSSE : public Group3ImplAll<InstType>
{
public:
	const SSELogicImpl<0x00,OpcodeSSE> PS;
	const SSELogicImpl<0x66,OpcodeSSE> PD;
	const SSELogicImpl<0xf3,OpcodeSSE> SS;
	const SSELogicImpl<0xf2,OpcodeSSE> SD;

	G3Impl_PlusSSE() {}
};

//////////////////////////////////////////////////////////////////////////////////////////
// 	The following iMul-specific forms are valid for 16 and 32 bit register operands only!

template< typename ImmType >
class iMulImpl
{
protected:
	static const uint OperandSize = sizeof(ImmType);
	static void prefix16() { if( OperandSize == 2 ) xWrite<u8>( 0x66 ); }

public:
	// ------------------------------------------------------------------------
	static __emitinline void Emit( const xRegister<ImmType>& to, const xRegister<ImmType>& from )
	{
		prefix16();
		write16( 0xaf0f );
		ModRM_Direct( to.Id, from.Id );
	}
	
	// ------------------------------------------------------------------------
	static __emitinline void Emit( const xRegister<ImmType>& to, const void* src )
	{
		prefix16();
		write16( 0xaf0f );
		xWriteDisp( to.Id, src );
	}

	// ------------------------------------------------------------------------
	static __emitinline void Emit( const xRegister<ImmType>& to, const ModSibBase& src )
	{
		prefix16();
		write16( 0xaf0f );
		EmitSibMagic( to.Id, src );
	}

	// ------------------------------------------------------------------------
	static __emitinline void Emit( const xRegister<ImmType>& to, const xRegister<ImmType>& from, ImmType imm )
	{
		prefix16();
		write16( is_s8( imm ) ? 0x6b : 0x69 );
		ModRM_Direct( to.Id, from.Id );
		if( is_s8( imm ) )
			write8( imm );
		else
			xWrite<ImmType>( imm );
	}

	// ------------------------------------------------------------------------
	static __emitinline void Emit( const xRegister<ImmType>& to, const void* src, ImmType imm )
	{
		prefix16();
		write16( is_s8( imm ) ? 0x6b : 0x69 );
		xWriteDisp( to.Id, src );
		if( is_s8( imm ) )
			write8( imm );
		else
			xWrite<ImmType>( imm );
	}

	// ------------------------------------------------------------------------
	static __emitinline void Emit( const xRegister<ImmType>& to, const ModSibBase& src, ImmType imm )
	{
		prefix16();
		write16( is_s8( imm ) ? 0x6b : 0x69 );
		EmitSibMagic( to.Id, src );
		if( is_s8( imm ) )
			write8( imm );
		else
			xWrite<ImmType>( imm );
	}
};

// ------------------------------------------------------------------------
class iMul_PlusSSE : public G3Impl_PlusSSE<G3Type_iMUL,0x59>
{
protected:
	typedef iMulImpl<u32> iMUL32;
	typedef iMulImpl<u16> iMUL16;

public:
	using G3Impl_PlusSSE<G3Type_iMUL,0x59>::operator();
	
	__forceinline void operator()( const xRegister32& to,	const xRegister32& from ) const			{ iMUL32::Emit( to, from ); }
	__forceinline void operator()( const xRegister32& to,	const void* src ) const					{ iMUL32::Emit( to, src ); }
	__forceinline void operator()( const xRegister32& to,	const xRegister32& from, s32 imm ) const{ iMUL32::Emit( to, from, imm ); }
	__noinline void operator()( const xRegister32& to,	const ModSibBase& src ) const				{ iMUL32::Emit( to, src ); }
	__noinline void operator()( const xRegister32& to,	const ModSibBase& from, s32 imm ) const		{ iMUL32::Emit( to, from, imm ); }

	__forceinline void operator()( const xRegister16& to,	const xRegister16& from ) const			{ iMUL16::Emit( to, from ); }
	__forceinline void operator()( const xRegister16& to,	const void* src ) const					{ iMUL16::Emit( to, src ); }
	__forceinline void operator()( const xRegister16& to,	const xRegister16& from, s16 imm ) const{ iMUL16::Emit( to, from, imm ); }
	__noinline void operator()( const xRegister16& to,	const ModSibBase& src ) const				{ iMUL16::Emit( to, src ); }
	__noinline void operator()( const xRegister16& to,	const ModSibBase& from, s16 imm ) const		{ iMUL16::Emit( to, from, imm ); }

	iMul_PlusSSE() {}
};
