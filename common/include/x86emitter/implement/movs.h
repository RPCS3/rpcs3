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

class MovImplAll
{
public:
	// ------------------------------------------------------------------------
	template< typename T > __forceinline void operator()( const xRegister<T>& to, const xRegister<T>& from ) const
	{
		if( to == from ) return;	// ignore redundant MOVs.

		prefix16<T>();
		xWrite8( Is8BitOp<T>() ? 0x88 : 0x89 );
		EmitSibMagic( from, to );
	}

	// ------------------------------------------------------------------------
	template< typename T > __noinline void operator()( const ModSibBase& dest, const xRegister<T>& from ) const
	{
		prefix16<T>();

		// mov eax has a special from when writing directly to a DISP32 address
		// (sans any register index/base registers).

		if( from.IsAccumulator() && dest.Index.IsEmpty() && dest.Base.IsEmpty() )
		{
			xWrite8( Is8BitOp<T>() ? 0xa2 : 0xa3 );
			xWrite32( dest.Displacement );
		}
		else
		{
			xWrite8( Is8BitOp<T>() ? 0x88 : 0x89 );
			EmitSibMagic( from.Id, dest );
		}
	}

	// ------------------------------------------------------------------------
	template< typename T > __noinline void operator()( const xRegister<T>& to, const ModSibBase& src ) const
	{
		prefix16<T>();

		// mov eax has a special from when reading directly from a DISP32 address
		// (sans any register index/base registers).

		if( to.IsAccumulator() && src.Index.IsEmpty() && src.Base.IsEmpty() )
		{
			xWrite8( Is8BitOp<T>() ? 0xa0 : 0xa1 );
			xWrite32( src.Displacement );
		}
		else
		{
			xWrite8( Is8BitOp<T>() ? 0x8a : 0x8b );
			EmitSibMagic( to, src );
		}
	}

	// ------------------------------------------------------------------------
	template< typename T > __noinline void operator()( const ModSibStrict<T>& dest, int imm ) const
	{
		prefix16<T>();
		xWrite8( Is8BitOp<T>() ? 0xc6 : 0xc7 );
		EmitSibMagic( 0, dest );
		xWrite<T>( imm );
	}

	// ------------------------------------------------------------------------
	// preserve_flags  - set to true to disable optimizations which could alter the state of
	//   the flags (namely replacing mov reg,0 with xor).
	template< typename T > __emitinline void operator()( const xRegister<T>& to, int imm, bool preserve_flags=false ) const
	{
		if( !preserve_flags && (imm == 0) )
			xXOR( to, to );
		else
		{
			// Note: MOV does not have (reg16/32,imm8) forms.

			prefix16<T>();
			xWrite8( (Is8BitOp<T>() ? 0xb0 : 0xb8) | to.Id ); 
			xWrite<T>( imm );
		}
	}

	// ------------------------------------------------------------------------
	template< typename T > __noinline void operator()( const ModSibBase& to, const xImmReg<T>& immOrReg ) const
	{
		_DoI_helpermess( *this, to, immOrReg );
	}

	template< typename T > __noinline void operator()( const xDirectOrIndirect<T>& to, const xImmReg<T>& immOrReg ) const
	{
		_DoI_helpermess( *this, to, immOrReg );
	}

	template< typename T > __noinline void operator()( const xDirectOrIndirect<T>& to, int imm ) const
	{
		_DoI_helpermess( *this, to, imm );
	}

	template< typename T > __noinline void operator()( const xDirectOrIndirect<T>& to, const xDirectOrIndirect<T>& from ) const
	{
		if( to == from ) return;
		_DoI_helpermess( *this, to, from );
	}

	template< typename T > __noinline void operator()( const xRegister<T>& to, const xDirectOrIndirect<T>& from ) const
	{
		_DoI_helpermess( *this, xDirectOrIndirect<T>( to ), from );
	}
	
	template< typename T > __noinline void operator()( const xDirectOrIndirect<T>& to, const xRegister<T>& from ) const
	{
		_DoI_helpermess( *this, to, xDirectOrIndirect<T>( from ) );
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
	__noinline void operator()( JccComparisonType ccType, const xRegister32& to, const ModSibBase& sibsrc ) const		{ ccSane(); xOpWrite0F( 0x40 | ccType, to, sibsrc ); }
	//__noinline void operator()( JccComparisonType ccType, const xDirectOrIndirect32& to, const xDirectOrIndirect32& from ) const { ccSane(); _DoI_helpermess( *this, to, from ); }	// too.. lazy.. to fix.

	__forceinline void operator()( JccComparisonType ccType, const xRegister16& to, const xRegister16& from ) const		{ ccSane(); xOpWrite0F( 0x66, 0x40 | ccType, to, from ); }
	__noinline void operator()( JccComparisonType ccType, const xRegister16& to, const ModSibBase& sibsrc ) const	{ ccSane(); xOpWrite0F( 0x66, 0x40 | ccType, to, sibsrc ); }
	//__noinline void operator()( JccComparisonType ccType, const xDirectOrIndirect16& to, const xDirectOrIndirect16& from ) const { ccSane(); _DoI_helpermess( *this, to, from ); }

	CMovImplGeneric() {}		// don't ask.
};

// ------------------------------------------------------------------------
template< JccComparisonType ccType >
class CMovImplAll
{
	static const u16 Opcode = 0x40 | ccType;

public:
	__forceinline void operator()( const xRegister32& to, const xRegister32& from ) const	{ ccSane(); xOpWrite0F( Opcode, to, from ); }
	__noinline void operator()( const xRegister32& to, const ModSibBase& sibsrc ) const		{ ccSane(); xOpWrite0F( Opcode, to, sibsrc ); }
	__noinline void operator()( const xDirectOrIndirect32& to, const xDirectOrIndirect32& from ) const { ccSane(); _DoI_helpermess( *this, to, from ); }

	__forceinline void operator()( const xRegister16& to, const xRegister16& from ) const	{ ccSane(); xOpWrite0F( 0x66, Opcode, to, from ); }
	__noinline void operator()( const xRegister16& to, const ModSibBase& sibsrc ) const		{ ccSane(); xOpWrite0F( 0x66, Opcode, to, sibsrc ); }
	__noinline void operator()( const xDirectOrIndirect16& to, const xDirectOrIndirect16& from ) const { ccSane(); _DoI_helpermess( *this, to, from ); }

	CMovImplAll() {}		// don't ask.
};

// ------------------------------------------------------------------------
class SetImplGeneric
{
	// note: SETcc are 0x90, with 0 in the Reg field of ModRM.
public:
	__forceinline void operator()( JccComparisonType ccType, const xRegister8& to ) const		{ ccSane(); xOpWrite0F( 0x90 | ccType, 0, to ); }
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
	__noinline void operator()( const ModSibStrict<u8>& dest ) const	{ ccSane(); xOpWrite0F( Opcode, 0, dest ); }
	__noinline void operator()( const xDirectOrIndirect8& dest ) const	{ ccSane(); _DoI_helpermess( *this, dest ); }
	
	SetImplAll() {}		// if you do, ask GCC.
};


//////////////////////////////////////////////////////////////////////////////////////////
// Mov with sign/zero extension implementations (movsx / movzx)
//

// ------------------------------------------------------------------------
template< bool SignExtend >
class MovExtendImplAll
{
protected:
	static const u16 Opcode = 0xb6 | (SignExtend ? 8 : 0 );

public:
	__forceinline void operator()( const xRegister32& to, const xRegister16& from )	const		{ xOpWrite0F( Opcode+1, to, from ); }
	__noinline void operator()( const xRegister32& to, const ModSibStrict<u16>& sibsrc ) const	{ xOpWrite0F( Opcode+1, to, sibsrc ); }
	__noinline void operator()( const xRegister32& to, const xDirectOrIndirect16& src ) const	{ _DoI_helpermess( *this, to, src ); }
	
	__forceinline void operator()( const xRegister32& to, const xRegister8& from ) const		{ xOpWrite0F( Opcode, to, from ); }
	__noinline void operator()( const xRegister32& to, const ModSibStrict<u8>& sibsrc ) const	{ xOpWrite0F( Opcode, to, sibsrc ); }
	__noinline void operator()( const xRegister32& to, const xDirectOrIndirect8& src ) const	{ _DoI_helpermess( *this, to, src ); }

	__forceinline void operator()( const xRegister16& to, const xRegister8& from ) const		{ xOpWrite0F( 0x66, Opcode, to, from ); }
	__noinline void operator()( const xRegister16& to, const ModSibStrict<u8>& sibsrc ) const	{ xOpWrite0F( 0x66, Opcode, to, sibsrc ); }
	__noinline void operator()( const xRegister16& to, const xDirectOrIndirect8& src ) const	{ _DoI_helpermess( *this, to, src ); }

	MovExtendImplAll() {}		// don't ask.
};

