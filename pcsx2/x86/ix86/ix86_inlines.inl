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

#include "System.h"

namespace x86Emitter
{
	//////////////////////////////////////////////////////////////////////////////////////////
	// x86Register Method Implementations
	//
	__forceinline x86AddressInfo x86IndexReg::operator+( const x86IndexReg& right ) const
	{
		return x86AddressInfo( *this, right );
	}

	__forceinline x86AddressInfo x86IndexReg::operator+( const x86AddressInfo& right ) const
	{
		return right + *this;
	}

	__forceinline x86AddressInfo x86IndexReg::operator+( s32 right ) const
	{
		return x86AddressInfo( *this, right );
	}

	__forceinline x86AddressInfo x86IndexReg::operator*( u32 right ) const
	{
		return x86AddressInfo( Empty, *this, right );
	}

	__forceinline x86AddressInfo x86IndexReg::operator<<( u32 shift ) const
	{
		return x86AddressInfo( Empty, *this, 1<<shift );
	}

	//////////////////////////////////////////////////////////////////////////////////////////
	// ModSib Method Implementations
	//

	// ------------------------------------------------------------------------
	__forceinline ModSibBase::ModSibBase( const x86AddressInfo& src ) :
		Base( src.Base ),
		Index( src.Index ),
		Scale( src.Factor ),
		Displacement( src.Displacement )
	{
		Reduce();
	}

	// ------------------------------------------------------------------------
	__forceinline ModSibBase::ModSibBase( x86IndexReg base, x86IndexReg index, int scale, s32 displacement ) :
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
	// Generates a 'reduced' ModSib form, which has valid Base, Index, and Scale values.
	// Necessary because by default ModSib compounds registers into Index when possible.
	//
	// If the ModSib is in illegal form ([Base + Index*5] for example) then an assertion
	// followed by an InvalidParameter Exception will be tossed around in haphazard 
	// fashion.
	__forceinline void ModSibBase::Reduce()
	{
		// If no index reg, then load the base register into the index slot.
		if( Index.IsEmpty() )
		{
			Index = Base;
			Scale = 0;
			Base = x86IndexReg::Empty;
			return;
		}
		
		// The Scale has a series of valid forms, all shown here:
		
		switch( Scale )
		{
			case 0: break;
			case 1: Scale = 0; break;
			case 2: Scale = 1; break;

			case 3:				// becomes [reg*2+reg]
				jASSUME( Base.IsEmpty() );
				Base = Index;
				Scale = 1;
			break;
			
			case 4: Scale = 2; break;

			case 5:				// becomes [reg*4+reg]
				jASSUME( Base.IsEmpty() );
				Base = Index;
				Scale = 2;
			break;
			
			case 6:				// invalid!
				assert( false );
			break;
			
			case 7:				// so invalid!
				assert( false );
			break;
			
			case 8: Scale = 3; break;
			case 9:				// becomes [reg*8+reg]
				jASSUME( Base.IsEmpty() );
				Base = Index;
				Scale = 3;
			break;
		}
		
		if( Index.IsStackPointer() )
		{
			// esp cannot be encoded as the index, so move it to the Base, if possible.
			jASSUME( Scale == 0 );
			jASSUME( Base.IsEmpty() );
			
			Base = Index;
			// noe: leave index assigned to esp also (generates correct encoding later)
			//Index = x86IndexReg::Empty;
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////////
	// x86AddressInfo Method Implementations
	//
	__forceinline x86AddressInfo& x86AddressInfo::Add( const x86IndexReg& src )
	{
		if( src == Index )
		{
			Factor++;
		}
		else if( src == Base )
		{
			// Compound the existing register reference into the Index/Scale pair.
			Base = x86IndexReg::Empty;

			if( src == Index )
				Factor++;
			else
			{
				jASSUME( Index.IsEmpty() );		// or die if we already have an index!
				Index = src;
				Factor = 2;
			}
		}
		else if( Base.IsEmpty() )
			Base = src;
		else if( Index.IsEmpty() )
			Index = src;
		else
			assert( false );	// oops, only 2 regs allowed per ModRm!

		return *this;
	}

	// ------------------------------------------------------------------------
	__forceinline x86AddressInfo& x86AddressInfo::Add( const x86AddressInfo& src )
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
			assert( false );	// oops, only 2 regs allowed!

		return *this;
	}
	
	//////////////////////////////////////////////////////////////////////////////////////////
	//
	
	// ------------------------------------------------------------------------
	template< typename OperandType >
	iForwardJump<OperandType>::iForwardJump( JccComparisonType cctype ) :
		BasePtr( (s8*)iGetPtr() +
			((OperandSize == 1) ? 2 :		// j8's are always 2 bytes.
			((cctype==Jcc_Unconditional) ? 5 : 6 ))	// j32's are either 5 or 6 bytes
		)
	{
		jASSUME( cctype != Jcc_Unknown );
		jASSUME( OperandSize == 1 || OperandSize == 4 );
		
		if( OperandSize == 1 )
			iWrite<u8>( (cctype == Jcc_Unconditional) ? 0xeb : (0x70 | cctype) );
		else
		{
			if( cctype == Jcc_Unconditional )
				iWrite<u8>( 0xe9 );
			else
			{
				iWrite<u8>( 0x0f );
				iWrite<u8>( 0x80 | cctype );
			}
		}

		iAdvancePtr( OperandSize );
	}

	// ------------------------------------------------------------------------
	template< typename OperandType >
	void iForwardJump<OperandType>::SetTarget() const
	{
		jASSUME( BasePtr != NULL );

		sptr displacement = (sptr)iGetPtr() - (sptr)BasePtr;
		if( OperandSize == 1 )
		{
			if( !is_s8( displacement ) )
			{
				assert( false );
				Console::Error( "Emitter Error: Invalid short jump displacement = 0x%x", params (int)displacement );
			}
			BasePtr[-1] = (s8)displacement;
		}
		else
		{
			// full displacement, no sanity checks needed :D
			((s32*)BasePtr)[-1] = displacement;
		}
	}

}
