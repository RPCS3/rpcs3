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

// Header: ix86_impl_movs.h -- covers mov, cmov, movsx/movzx, and SETcc (which shares
// with cmov many similarities).

// Note: This header is meant to be included from within the x86Emitter::Internal namespace.

//////////////////////////////////////////////////////////////////////////////////////////
// MOV instruction Implementation

template< typename ImmType >
class MovImpl
{
protected:
	static const uint OperandSize = sizeof(ImmType);
	static bool Is8BitOperand()	{ return OperandSize == 1; }
	static void prefix16()		{ if( OperandSize == 2 ) xWrite8( 0x66 ); }

public:
	MovImpl() {}
	
	// ------------------------------------------------------------------------
	static __emitinline void Emit( const xRegister<ImmType>& to, const xRegister<ImmType>& from )
	{
		if( to == from ) return;	// ignore redundant MOVs.

		prefix16();
		xWrite8( Is8BitOperand() ? 0x88 : 0x89 );
		EmitSibMagic( from, to );
	}

	// ------------------------------------------------------------------------
	static __emitinline void Emit( const ModSibBase& dest, const xRegister<ImmType>& from )
	{
		prefix16();

		// mov eax has a special from when writing directly to a DISP32 address
		// (sans any register index/base registers).

		if( from.IsAccumulator() && dest.Index.IsEmpty() && dest.Base.IsEmpty() )
		{
			xWrite8( Is8BitOperand() ? 0xa2 : 0xa3 );
			xWrite32( dest.Displacement );
		}
		else
		{
			xWrite8( Is8BitOperand() ? 0x88 : 0x89 );
			EmitSibMagic( from.Id, dest );
		}
	}

	// ------------------------------------------------------------------------
	static __emitinline void Emit( const xRegister<ImmType>& to, const ModSibBase& src )
	{
		prefix16();

		// mov eax has a special from when reading directly from a DISP32 address
		// (sans any register index/base registers).

		if( to.IsAccumulator() && src.Index.IsEmpty() && src.Base.IsEmpty() )
		{
			xWrite8( Is8BitOperand() ? 0xa0 : 0xa1 );
			xWrite32( src.Displacement );
		}
		else
		{
			xWrite8( Is8BitOperand() ? 0x8a : 0x8b );
			EmitSibMagic( to, src );
		}
	}

	// ------------------------------------------------------------------------
	static __emitinline void Emit( void* dest, const xRegister<ImmType>& from )
	{
		prefix16();

		// mov eax has a special from when writing directly to a DISP32 address

		if( from.IsAccumulator() )
		{
			xWrite8( Is8BitOperand() ? 0xa2 : 0xa3 );
			xWrite<s32>( (s32)dest );
		}
		else
		{
			xWrite8( Is8BitOperand() ? 0x88 : 0x89 );
			EmitSibMagic( from, dest );
		}
	}

	// ------------------------------------------------------------------------
	static __emitinline void Emit( const xRegister<ImmType>& to, const void* src )
	{
		prefix16();

		// mov eax has a special from when reading directly from a DISP32 address

		if( to.IsAccumulator() )
		{
			xWrite8( Is8BitOperand() ? 0xa0 : 0xa1 );
			xWrite<s32>( (s32)src );
		}
		else
		{
			xWrite8( Is8BitOperand() ? 0x8a : 0x8b );
			EmitSibMagic( to, src );
		}
	}

	// ------------------------------------------------------------------------
	static __emitinline void Emit( const xRegister<ImmType>& to, ImmType imm )
	{
		// Note: MOV does not have (reg16/32,imm8) forms.

		prefix16();
		xWrite8( (Is8BitOperand() ? 0xb0 : 0xb8) | to.Id ); 
		xWrite<ImmType>( imm );
	}

	// ------------------------------------------------------------------------
	static __emitinline void Emit( ModSibStrict<ImmType> dest, ImmType imm )
	{
		prefix16();
		xWrite8( Is8BitOperand() ? 0xc6 : 0xc7 );
		EmitSibMagic( 0, dest );
		xWrite<ImmType>( imm );
	}
};

// ------------------------------------------------------------------------
class MovImplAll
{
public:
	template< typename T >
	__forceinline void operator()( const xRegister<T>& to,	const xRegister<T>& from ) const	{ MovImpl<T>::Emit( to, from ); }
	template< typename T >
	__forceinline void operator()( const xRegister<T>& to,	const void* src ) const				{ MovImpl<T>::Emit( to, src ); }
	template< typename T >
	__forceinline void operator()( void* dest,				const xRegister<T>& from ) const	{ MovImpl<T>::Emit( dest, from ); }
	template< typename T >
	__noinline void operator()( const ModSibBase& sibdest,	const xRegister<T>& from ) const	{ MovImpl<T>::Emit( sibdest, from ); }
	template< typename T >
	__noinline void operator()( const xRegister<T>& to,		const ModSibBase& sibsrc ) const	{ MovImpl<T>::Emit( to, sibsrc ); }

	template< typename T >
	__noinline void operator()( const ModSibStrict<T>& sibdest, int imm ) const					{ MovImpl<T>::Emit( sibdest, imm ); }

	// preserve_flags  - set to true to disable optimizations which could alter the state of
	//   the flags (namely replacing mov reg,0 with xor).

	template< typename T >
	__emitinline void operator()( const xRegister<T>& to, int imm, bool preserve_flags=false ) const
	{
		if( !preserve_flags && (imm == 0) )
			xXOR( to, to );
		else
			MovImpl<T>::Emit( to, imm );
	}
	
	MovImplAll() {} // Satisfy GCC's whims.
};

#define ccSane() jASSUME( ccType >= 0 && ccType <= 0x0f )

//////////////////////////////////////////////////////////////////////////////////////////
// CMOV !!  [in all of it's disappointing lack-of glory]  .. and ..
// SETcc!!  [more glory, less lack!]
//
// CMOV Disclaimer: Caution!  This instruction can look exciting and cool, until you
// realize that it cannot load immediate values into registers. -_-
//
// I use explicit method declarations here instead of templates, in order to provide
// *only* 32 and 16 bit register operand forms (8 bit registers are not valid in CMOV).
//
class CMovImplGeneric
{
public:
	__forceinline void operator()( JccComparisonType ccType, const xRegister32& to, const xRegister32& from ) const		{ ccSane(); xOpWrite0F( 0x40 | ccType, to, from ); }
	__forceinline void operator()( JccComparisonType ccType, const xRegister32& to, const void* src ) const				{ ccSane(); xOpWrite0F( 0x40 | ccType, to, src ); }
	__forceinline void operator()( JccComparisonType ccType, const xRegister32& to, const ModSibBase& sibsrc ) const	{ ccSane(); xOpWrite0F( 0x40 | ccType, to, sibsrc ); }

	__forceinline void operator()( JccComparisonType ccType, const xRegister16& to, const xRegister16& from ) const		{ ccSane(); xOpWrite0F( 0x66, 0x40 | ccType, to, from ); }
	__forceinline void operator()( JccComparisonType ccType, const xRegister16& to, const void* src ) const				{ ccSane(); xOpWrite0F( 0x66, 0x40 | ccType, to, src ); }
	__forceinline void operator()( JccComparisonType ccType, const xRegister16& to, const ModSibBase& sibsrc ) const	{ ccSane(); xOpWrite0F( 0x66, 0x40 | ccType, to, sibsrc ); }

	CMovImplGeneric() {}		// don't ask.
};

// ------------------------------------------------------------------------
template< JccComparisonType ccType >
class CMovImplAll
{
	static const u16 Opcode = 0x40 | ccType;

public:
	__forceinline void operator()( const xRegister32& to, const xRegister32& from ) const	{ ccSane(); xOpWrite0F( Opcode, to, from ); }
	__forceinline void operator()( const xRegister32& to, const void* src ) const			{ ccSane(); xOpWrite0F( Opcode, to, src ); }
	__forceinline void operator()( const xRegister32& to, const ModSibBase& sibsrc ) const	{ ccSane(); xOpWrite0F( Opcode, to, sibsrc ); }

	__forceinline void operator()( const xRegister16& to, const xRegister16& from ) const	{ ccSane(); xOpWrite0F( 0x66, Opcode, to, from ); }
	__forceinline void operator()( const xRegister16& to, const void* src ) const			{ ccSane(); xOpWrite0F( 0x66, Opcode, to, src ); }
	__forceinline void operator()( const xRegister16& to, const ModSibBase& sibsrc ) const	{ ccSane(); xOpWrite0F( 0x66, Opcode, to, sibsrc ); }

	CMovImplAll() {}		// don't ask.
};

// ------------------------------------------------------------------------
class SetImplGeneric
{
	// note: SETcc are 0x90, with 0 in the Reg field of ModRM.
public:
	__forceinline void operator()( JccComparisonType ccType, const xRegister8& to ) const		{ ccSane(); xOpWrite0F( 0x90 | ccType, 0, to ); }
	__forceinline void operator()( JccComparisonType ccType, void* dest ) const					{ ccSane(); xOpWrite0F( 0x90 | ccType, 0, dest ); }
	__noinline void operator()( JccComparisonType ccType, const ModSibStrict<u8>& dest ) const	{ ccSane(); xOpWrite0F( 0x90 | ccType, 0, dest ); }

	SetImplGeneric() {}		// if you do, ask GCC.
};

// ------------------------------------------------------------------------
template< JccComparisonType ccType >
class SetImplAll
{
	static const u16 Opcode = 0x90 | ccType;		// SETcc are 0x90 base opcode, with 0 in the Reg field of ModRM.

public:
	__forceinline void operator()( const xRegister8& to ) const			{ ccSane(); xOpWrite0F( Opcode, 0, to ); }
	__forceinline void operator()( void* dest ) const					{ ccSane(); xOpWrite0F( Opcode, 0, dest ); }
	__noinline void operator()( const ModSibStrict<u8>& dest ) const	{ ccSane(); xOpWrite0F( Opcode, 0, dest ); }

	SetImplAll() {}		// if you do, ask GCC.
};


//////////////////////////////////////////////////////////////////////////////////////////
// Mov with sign/zero extension implementations (movsx / movzx)
//
template< typename DestImmType, typename SrcImmType >
class MovExtendImpl
{
protected:
	static const uint DestOperandSize = sizeof( DestImmType );
	static const uint SrcOperandSize = sizeof( SrcImmType );

	static bool Is8BitOperand()	{ return SrcOperandSize == 1; }
	static void prefix16()		{ if( DestOperandSize == 2 ) xWrite8( 0x66 ); }
	static __forceinline void emit_base( bool SignExtend )
	{
		prefix16();
		xWrite8( 0x0f );
		xWrite8( 0xb6 | (Is8BitOperand() ? 0 : 1) | (SignExtend ? 8 : 0 ) );
	}

public: 
	MovExtendImpl() {}		// For the love of GCC.

	static __emitinline void Emit( const xRegister<DestImmType>& to, const xRegister<SrcImmType>& from, bool SignExtend )
	{
		emit_base( SignExtend );
		EmitSibMagic( to, from );
	}

	static __emitinline void Emit( const xRegister<DestImmType>& to, const ModSibStrict<SrcImmType>& sibsrc, bool SignExtend )
	{
		emit_base( SignExtend );
		EmitSibMagic( to, sibsrc );
	}
};

// ------------------------------------------------------------------------
template< bool SignExtend >
class MovExtendImplAll
{
protected:
	typedef MovExtendImpl<u32, u16> m_16to32;
	typedef MovExtendImpl<u32, u8> m_8to32;
	typedef MovExtendImpl<u16, u8> m_8to16;

public:
	__forceinline void operator()( const xRegister32& to, const xRegister16& from )	const		{ m_16to32::Emit( to, from, SignExtend ); }
	__noinline void operator()( const xRegister32& to, const ModSibStrict<u16>& sibsrc ) const	{ m_16to32::Emit( to, sibsrc, SignExtend ); }

	__forceinline void operator()( const xRegister32& to, const xRegister8& from ) const		{ m_8to32::Emit( to, from, SignExtend ); }
	__noinline void operator()( const xRegister32& to, const ModSibStrict<u8>& sibsrc ) const	{ m_8to32::Emit( to, sibsrc, SignExtend ); }

	__forceinline void operator()( const xRegister16& to, const xRegister8& from ) const		{ m_8to16::Emit( to, from, SignExtend ); }
	__noinline void operator()( const xRegister16& to, const ModSibStrict<u8>& sibsrc ) const	{ m_8to16::Emit( to, sibsrc, SignExtend ); }

	MovExtendImplAll() {}		// don't ask.
};

