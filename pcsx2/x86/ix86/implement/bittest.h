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

// Implementations found here: BTS/BT/BTC/BTR plus BSF/BSR!
// Note: This header is meant to be included from within the x86Emitter::Internal namespace.

// These instructions are in the 'Group8' as per Intel's manual, but since they all have
// a unified purpose, I've named them for that instead.

enum G8Type
{
	G8Type_BT = 4,
	G8Type_BTS,
	G8Type_BTR,
	G8Type_BTC,
};

//////////////////////////////////////////////////////////////////////////////////////////
// Notes: Bit Test instructions are valid on 16/32 bit operands only.
//
template< G8Type InstType, typename ImmType >
class Group8Impl
{
protected:
	static const uint OperandSize = sizeof(ImmType);

	static void prefix16()		{ if( OperandSize == 2 ) iWrite<u8>( 0x66 ); }

public: 
	Group8Impl() {}		// For the love of GCC.

	// ------------------------------------------------------------------------
	static __emitinline void Emit( const iRegister<ImmType>& bitbase, const iRegister<ImmType>& bitoffset )
	{
		prefix16();
		iWrite<u8>( 0x0f );
		iWrite<u8>( 0xa3 | (InstType << 2) );
		ModRM_Direct( bitoffset.Id, bitbase.Id );
	}

	// ------------------------------------------------------------------------
#ifndef __LINUX__
	static __emitinline void Emit( void* bitbase, const iRegister<ImmType>& bitoffset )
	{
		prefix16();
		iWrite<u8>( 0x0f );
		iWrite<u8>( 0xa3 | (InstType << 2) );
		iWriteDisp( bitoffset.Id, bitbase.Id );
	}
#endif
	// ------------------------------------------------------------------------
	static __emitinline void Emit( const ModSibBase& bitbase, const iRegister<ImmType>& bitoffset )
	{
		prefix16();
		iWrite<u8>( 0x0f );
		iWrite<u8>( 0xa3 | (InstType << 2) );
		EmitSibMagic( bitoffset.Id, bitbase );
	}

	// ------------------------------------------------------------------------
	static __emitinline void Emit( const iRegister<ImmType>& bitbase, u8 immoffset )
	{
		prefix16();
		iWrite<u16>( 0xba0f );
		ModRM_Direct( InstType, bitbase.Id );
		iWrite<u8>( immoffset );
	}

	// ------------------------------------------------------------------------
	static __emitinline void Emit( const ModSibStrict<ImmType>& bitbase, u8 immoffset )
	{
		prefix16();
		iWrite<u16>( 0xba0f );
		EmitSibMagic( InstType, bitbase );
		iWrite<u8>( immoffset );
	}
};

// -------------------------------------------------------------------
//
template< G8Type InstType >
class Group8ImplAll
{
protected:
	typedef Group8Impl<InstType,u32> m_32;
	typedef Group8Impl<InstType,u32> m_16;

public:
	__forceinline void operator()( const iRegister32& bitbase,	const iRegister32& bitoffset ) const	{ m_32::Emit( bitbase, bitoffset ); }
	__forceinline void operator()( const iRegister16& bitbase,	const iRegister16& bitoffset ) const	{ m_16::Emit( bitbase, bitoffset ); }
	__forceinline void operator()( void* bitbase,				const iRegister32& bitoffset ) const	{ m_32::Emit( bitbase, bitoffset ); }
	__forceinline void operator()( void* bitbase,				const iRegister16& bitoffset ) const	{ m_16::Emit( bitbase, bitoffset ); }
	__noinline void operator()( const ModSibBase& bitbase,		const iRegister32& bitoffset ) const	{ m_32::Emit( bitbase, bitoffset ); }
	__noinline void operator()( const ModSibBase& bitbase,		const iRegister16& bitoffset ) const	{ m_16::Emit( bitbase, bitoffset ); }

	// Note on Imm forms : use int as the source operand since it's "reasonably inert" from a compiler
	// perspective.  (using uint tends to make the compiler try and fail to match signed immediates with
	// one of the other overloads).

	__noinline void operator()( const ModSibStrict<u32>& bitbase, u8 immoffset ) const	{ m_32::Emit( bitbase, immoffset ); }
	__noinline void operator()( const ModSibStrict<u16>& bitbase, u8 immoffset ) const	{ m_16::Emit( bitbase, immoffset ); }
	void operator()( const iRegister<u32>& bitbase, u8 immoffset ) const				{ m_32::Emit( bitbase, immoffset ); }
	void operator()( const iRegister<u16>& bitbase, u8 immoffset ) const				{ m_16::Emit( bitbase, immoffset ); }

	Group8ImplAll() {}
};


//////////////////////////////////////////////////////////////////////////////////////////
// BSF / BSR -- 16/32 operands supported only.
//
template< bool isReverse, typename ImmType >
class BitScanImpl
{
protected:
	static const uint OperandSize = sizeof(ImmType);
	static void prefix16()		{ if( OperandSize == 2 ) iWrite<u8>( 0x66 ); }
	static void emitbase()
	{
		prefix16();
		iWrite<u8>( 0x0f );
		iWrite<u8>( isReverse ? 0xbd : 0xbc );
	}

public: 
	BitScanImpl() {}		// For the love of GCC.

	// ------------------------------------------------------------------------
	static __emitinline void Emit( const iRegister<ImmType>& to, const iRegister<ImmType>& from )
	{
		emitbase();
		ModRM_Direct( to.Id, from.Id );
	}

	// ------------------------------------------------------------------------
	static __emitinline void Emit( const iRegister<ImmType>& to, const void* src )
	{
		emitbase();
		iWriteDisp( to.Id, src );
	}

	// ------------------------------------------------------------------------
	static __emitinline void Emit( const iRegister<ImmType>& to, const ModSibBase& sibsrc )
	{
		emitbase();
		EmitSibMagic( to.Id, sibsrc );
	}
};


// -------------------------------------------------------------------
// BSF/BSR  -- 16 and 32 bit operand forms only!
//
template< bool isReverse >
class BitScanImplAll
{
protected:
	typedef BitScanImpl<isReverse,u32> m_32;
	typedef BitScanImpl<isReverse,u32> m_16;

public:
	__forceinline void operator()( const iRegister32& to, const iRegister32& from ) const	{ m_32::Emit( to, from ); }
	__forceinline void operator()( const iRegister16& to, const iRegister16& from ) const	{ m_16::Emit( to, from ); }
	__forceinline void operator()( const iRegister32& to, const void* src ) const			{ m_32::Emit( to, src ); }
	__forceinline void operator()( const iRegister16& to, const void* src ) const			{ m_16::Emit( to, src ); }
	__noinline void operator()( const iRegister32& to, const ModSibBase& sibsrc ) const		{ m_32::Emit( to, sibsrc ); }
	__noinline void operator()( const iRegister16& to, const ModSibBase& sibsrc ) const		{ m_16::Emit( to, sibsrc ); }

	BitScanImplAll() {}
};

