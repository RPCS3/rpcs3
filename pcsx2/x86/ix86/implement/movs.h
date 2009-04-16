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

// Header: ix86_impl_movs.h -- covers cmov and movsx/movzx.
// Note: This header is meant to be included from within the x86Emitter::Internal namespace.

//////////////////////////////////////////////////////////////////////////////////////////
// CMOV !!  [in all of it's disappointing lack-of glory]
//
template< int OperandSize >
class CMovImpl
{
protected:
	static bool Is8BitOperand()	{ return OperandSize == 1; }
	static void prefix16()		{ if( OperandSize == 2 ) iWrite<u8>( 0x66 ); }
	
	static __forceinline void emit_base( JccComparisonType cc )
	{
		jASSUME( cc >= 0 && cc <= 0x0f );
		prefix16();
		write8( 0x0f );
		write8( 0x40 | cc );	
	}

public:
	CMovImpl() {}

	static __emitinline void Emit( JccComparisonType cc, const iRegister<OperandSize>& to, const iRegister<OperandSize>& from )
	{
		if( to == from ) return;
		emit_base( cc );
		ModRM( ModRm_Direct, to.Id, from.Id );
	}

	static __emitinline void Emit( JccComparisonType cc, const iRegister<OperandSize>& to, const void* src )
	{
		emit_base( cc );
		iWriteDisp( to.Id, src );
	}

	static __emitinline void Emit( JccComparisonType cc, const iRegister<OperandSize>& to, const ModSibBase& sibsrc )
	{
		emit_base( cc );
		EmitSibMagic( to.Id, sibsrc );
	}

};

// ------------------------------------------------------------------------
class CMovImplGeneric
{
protected:
	typedef CMovImpl<4> m_32;
	typedef CMovImpl<2> m_16;

public:
	__forceinline void operator()( JccComparisonType ccType, const iRegister32& to, const iRegister32& from ) const		{ m_32::Emit( ccType, to, from ); }
	__forceinline void operator()( JccComparisonType ccType, const iRegister32& to, const void* src ) const				{ m_32::Emit( ccType, to, src ); }
	__noinline void operator()( JccComparisonType ccType, const iRegister32& to, const ModSibBase& sibsrc ) const		{ m_32::Emit( ccType, to, sibsrc ); }

	__forceinline void operator()( JccComparisonType ccType, const iRegister16& to, const iRegister16& from ) const		{ m_16::Emit( ccType, to, from ); }
	__forceinline void operator()( JccComparisonType ccType, const iRegister16& to, const void* src ) const				{ m_16::Emit( ccType, to, src ); }
	__noinline void operator()( JccComparisonType ccType, const iRegister16& to, const ModSibBase& sibsrc ) const		{ m_16::Emit( ccType, to, sibsrc ); }

	CMovImplGeneric() {}		// don't ask.
};

// ------------------------------------------------------------------------
template< JccComparisonType ccType >
class CMovImplAll
{
protected:
	typedef CMovImpl<4> m_32;
	typedef CMovImpl<2> m_16;

public:
	__forceinline void operator()( const iRegister32& to, const iRegister32& from ) const	{ m_32::Emit( ccType, to, from ); }
	__forceinline void operator()( const iRegister32& to, const void* src ) const			{ m_32::Emit( ccType, to, src ); }
	__noinline void operator()( const iRegister32& to, const ModSibBase& sibsrc ) const		{ m_32::Emit( ccType, to, sibsrc ); }

	__forceinline void operator()( const iRegister16& to, const iRegister16& from ) const	{ m_16::Emit( ccType, to, from ); }
	__forceinline void operator()( const iRegister16& to, const void* src ) const			{ m_16::Emit( ccType, to, src ); }
	__noinline void operator()( const iRegister16& to, const ModSibBase& sibsrc ) const		{ m_16::Emit( ccType, to, sibsrc ); }

	CMovImplAll() {}		// don't ask.
};

//////////////////////////////////////////////////////////////////////////////////////////
// Mov with sign/zero extension implementations (movsx / movzx)
//
template< int DestOperandSize, int SrcOperandSize >
class MovExtendImpl
{
protected:
	static bool Is8BitOperand()	{ return SrcOperandSize == 1; }
	static void prefix16()		{ if( DestOperandSize == 2 ) iWrite<u8>( 0x66 ); }
	static __forceinline void emit_base( bool SignExtend )
	{
		prefix16();
		iWrite<u8>( 0x0f );
		iWrite<u8>( 0xb6 | (Is8BitOperand() ? 0 : 1) | (SignExtend ? 8 : 0 ) );
	}

public: 
	MovExtendImpl() {}		// For the love of GCC.

	static __emitinline void Emit( const iRegister<DestOperandSize>& to, const iRegister<SrcOperandSize>& from, bool SignExtend )
	{
		emit_base( SignExtend );
		ModRM_Direct( to.Id, from.Id );
	}

	static __emitinline void Emit( const iRegister<DestOperandSize>& to, const ModSibStrict<SrcOperandSize>& sibsrc, bool SignExtend )
	{
		emit_base( SignExtend );
		EmitSibMagic( to.Id, sibsrc );
	}
};

// ------------------------------------------------------------------------
template< bool SignExtend >
class MovExtendImplAll
{
protected:
	typedef MovExtendImpl<4, 2> m_16to32;
	typedef MovExtendImpl<4, 1> m_8to32;
	typedef MovExtendImpl<2, 1> m_8to16;

public:
	__forceinline void operator()( const iRegister32& to, const iRegister16& from )	const		{ m_16to32::Emit( to, from, SignExtend ); }
	__noinline void operator()( const iRegister32& to, const ModSibStrict<2>& sibsrc ) const	{ m_16to32::Emit( to, sibsrc, SignExtend ); }

	__forceinline void operator()( const iRegister32& to, const iRegister8& from ) const		{ m_8to32::Emit( to, from, SignExtend ); }
	__noinline void operator()( const iRegister32& to, const ModSibStrict<1>& sibsrc ) const	{ m_8to32::Emit( to, sibsrc, SignExtend ); }

	__forceinline void operator()( const iRegister16& to, const iRegister8& from ) const		{ m_8to16::Emit( to, from, SignExtend ); }
	__noinline void operator()( const iRegister16& to, const ModSibStrict<1>& sibsrc ) const	{ m_8to16::Emit( to, sibsrc, SignExtend ); }

	MovExtendImplAll() {}		// don't ask.
};

