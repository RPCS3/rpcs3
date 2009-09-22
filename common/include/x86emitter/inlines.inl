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

/*
 * ix86 core v0.9.0
 *
 * Original Authors (v0.6.2 and prior):
 *		linuzappz <linuzappz@pcsx.net>
 *		alexey silinov
 *		goldfinger
 *		zerofrog(@gmail.com)
 *
 * Authors of v0.9.0:
 *		Jake.Stine(@gmail.com)
 *		cottonvibes(@gmail.com)
 *		sudonim(1@gmail.com)
 */

#pragma once

// This header module contains functions which, under most circumstances, inline
// nicely with constant propagation from the compiler, resulting in little or
// no actual codegen in the majority of emitter statements. (common forms include:
// RegToReg, PointerToReg, RegToPointer).  These cannot be included in the class
// definitions in the .h file because of inter-dependencies with other classes.
//   (score one for C++!!)
//
// In order for MSVC to work correctly with __forceinline on class members,
// however, we need to include these methods into all source files which might
// reference them.  Without this MSVC generates linker errors.  Or, in other words,
// global optimization fails to resolve the externals and junk.
//   (score one for MSVC!)

namespace x86Emitter
{
	extern const char *const x86_regnames_gpr8[8];
	extern const char *const x86_regnames_gpr16[8];
	extern const char *const x86_regnames_gpr32[8];

	extern const char *const x86_regnames_sse[8];
	extern const char *const x86_regnames_mmx[8];

	//////////////////////////////////////////////////////////////////////////////////////////
	// Diagnostic -- returns a string representation of this register.
	//
	template< typename T >
	const char* xGetRegName( const xRegister<T>& src )
	{
		if( src.IsEmpty() ) return "empty";
		
		switch( sizeof(T) )
		{
			case 1: return x86_regnames_gpr8[ src.Id ];
			case 2: return x86_regnames_gpr16[ src.Id ];
			case 4: return x86_regnames_gpr32[ src.Id ];
			
			jNO_DEFAULT
		}

		return "oops?";
	}

	template< typename T >
	const char* xGetRegName( const xRegisterSIMD<T>& src )
	{
		if( src.IsEmpty() ) return "empty";
		
		switch( sizeof(T) )
		{
			case 8: return x86_regnames_mmx[ src.Id ];
			case 16: return x86_regnames_sse[ src.Id ];
			
			jNO_DEFAULT
		}

		return "oops?";
	}

	//////////////////////////////////////////////////////////////////////////////////////////
	// x86Register Method Implementations
	//
	__forceinline xAddressInfo xAddressReg::operator+( const xAddressReg& right ) const
	{
		jASSUME( Id != -1 );
		return xAddressInfo( *this, right );
	}

	__forceinline xAddressInfo xAddressReg::operator+( const xAddressInfo& right ) const
	{
		jASSUME( Id != -1 );
		return right + *this;
	}

	__forceinline xAddressInfo xAddressReg::operator+( s32 right ) const
	{
		jASSUME( Id != -1 );
		return xAddressInfo( *this, right );
	}

	__forceinline xAddressInfo xAddressReg::operator+( const void* right ) const
	{
		jASSUME( Id != -1 );
		return xAddressInfo( *this, (s32)right );
	}

	// ------------------------------------------------------------------------
	__forceinline xAddressInfo xAddressReg::operator-( s32 right ) const
	{
		jASSUME( Id != -1 );
		return xAddressInfo( *this, -right );
	}

	__forceinline xAddressInfo xAddressReg::operator-( const void* right ) const
	{
		jASSUME( Id != -1 );
		return xAddressInfo( *this, -(s32)right );
	}

	// ------------------------------------------------------------------------
	__forceinline xAddressInfo xAddressReg::operator*( u32 right ) const
	{
		jASSUME( Id != -1 );
		return xAddressInfo( Empty, *this, right );
	}

	__forceinline xAddressInfo xAddressReg::operator<<( u32 shift ) const
	{
		jASSUME( Id != -1 );
		return xAddressInfo( Empty, *this, 1<<shift );
	}

	//////////////////////////////////////////////////////////////////////////////////////////
	// ModSib Method Implementations
	//

	// ------------------------------------------------------------------------
	__forceinline ModSibBase::ModSibBase( const xAddressInfo& src ) :
		Base( src.Base ),
		Index( src.Index ),
		Scale( src.Factor ),
		Displacement( src.Displacement )
	{
		Reduce();
	}

	// ------------------------------------------------------------------------
	__forceinline ModSibBase::ModSibBase( xAddressReg base, xAddressReg index, int scale, s32 displacement ) :
		Base( base ),
		Index( index ),
		Scale( scale ),
		Displacement( displacement )
	{
		Reduce();
	}

	// ------------------------------------------------------------------------
	__forceinline ModSibBase::ModSibBase( s32 displacement ) :
		Base(),
		Index(),
		Scale(0),
		Displacement( displacement )
	{
		// no reduction necessary :D
	}

	// ------------------------------------------------------------------------
	__forceinline ModSibBase::ModSibBase( const void* target ) :
		Base(),
		Index(),
		Scale(0),
		Displacement( (s32)target )
	{
		// no reduction necessary :D
	}

	//////////////////////////////////////////////////////////////////////////////////////////
	// xAddressInfo Method Implementations
	//
	__forceinline xAddressInfo& xAddressInfo::Add( const xAddressReg& src )
	{
		if( src == Index )
		{
			Factor++;
		}
		else if( src == Base )
		{
			// Compound the existing register reference into the Index/Scale pair.
			Base = xAddressReg::Empty;

			if( src == Index )
				Factor++;
			else
			{
				DevAssert( Index.IsEmpty(), "x86Emitter: Only one scaled index register is allowed in an address modifier." );
				Index = src;
				Factor = 2;
			}
		}
		else if( Base.IsEmpty() )
			Base = src;
		else if( Index.IsEmpty() )
			Index = src;
		else
			wxASSERT_MSG( false, L"x86Emitter: address modifiers cannot have more than two index registers." );	// oops, only 2 regs allowed per ModRm!

		return *this;
	}

	// ------------------------------------------------------------------------
	__forceinline xAddressInfo& xAddressInfo::Add( const xAddressInfo& src )
	{
		Add( src.Base );
		Add( src.Displacement );

		// If the factor is 1, we can just treat index like a base register also.
		if( src.Factor == 1 )
		{
			Add( src.Index );
		}
		else if( Index.IsEmpty() )
		{
			Index = src.Index;
			Factor = 1;
		}
		else if( Index == src.Index )
			Factor++;
		else
			wxASSERT_MSG( false, L"x86Emitter: address modifiers cannot have more than two index registers." );	// oops, only 2 regs allowed per ModRm!

		return *this;
	}
	
	//////////////////////////////////////////////////////////////////////////////////////////
	//
	
	// ------------------------------------------------------------------------
	template< typename OperandType >
	xForwardJump<OperandType>::xForwardJump( JccComparisonType cctype ) :
		BasePtr( (s8*)xGetPtr() +
			((OperandSize == 1) ? 2 :		// j8's are always 2 bytes.
			((cctype==Jcc_Unconditional) ? 5 : 6 ))	// j32's are either 5 or 6 bytes
		)
	{
		jASSUME( cctype != Jcc_Unknown );
		jASSUME( OperandSize == 1 || OperandSize == 4 );
		
		if( OperandSize == 1 )
			xWrite8( (cctype == Jcc_Unconditional) ? 0xeb : (0x70 | cctype) );
		else
		{
			if( cctype == Jcc_Unconditional )
				xWrite8( 0xe9 );
			else
			{
				xWrite8( 0x0f );
				xWrite8( 0x80 | cctype );
			}
		}

		xAdvancePtr( OperandSize );
	}

	// ------------------------------------------------------------------------
	template< typename OperandType >
	void xForwardJump<OperandType>::SetTarget() const
	{
		jASSUME( BasePtr != NULL );

		sptr displacement = (sptr)xGetPtr() - (sptr)BasePtr;
		if( OperandSize == 1 )
		{
			if( !is_s8( displacement ) )
			{
				wxASSERT( false );
// Don't ask. --arcum42
#if !defined(__LINUX__) || !defined(DEBUG)

				Console::Error( "Emitter Error: Invalid short jump displacement = 0x%x", (int)displacement );
#endif
			}
			BasePtr[-1] = (s8)displacement;
		}
		else
		{
			// full displacement, no sanity checks needed :D
			((s32*)BasePtr)[-1] = displacement;
		}
	}

	// ------------------------------------------------------------------------
	// returns the inverted conditional type for this Jcc condition.  Ie, JNS will become JS.
	//
	static __forceinline JccComparisonType xInvertCond( JccComparisonType src )
	{
		jASSUME( src != Jcc_Unknown );
		if( Jcc_Unconditional == src ) return Jcc_Unconditional;

		// x86 conditionals are clever!  To invert conditional types, just invert the lower bit:
		return (JccComparisonType)((int)src ^ 1);
	}
}
