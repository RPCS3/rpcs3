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

// Note: This header is meant to be included from within the x86Emitter::Internal namespace.
// Instructions implemented in this header are as follows -->>

enum G2Type
{
	G2Type_ROL=0,
	G2Type_ROR,
	G2Type_RCL,
	G2Type_RCR,
	G2Type_SHL,
	G2Type_SHR,
	G2Type_Unused,
	G2Type_SAR
};

// -------------------------------------------------------------------
// Group 2 (shift) instructions have no Sib/ModRM forms.
// Optimization Note: For Imm forms, we ignore the instruction if the shift count is zero.
// This is a safe optimization since any zero-value shift does not affect any flags.
//
template< G2Type InstType >
class Group2ImplAll
{
public:
	template< typename T > __forceinline void operator()( const xRegister<T>& to,		__unused const xRegisterCL& from ) const
	{
		prefix16<T>();
		xWrite8( Is8BitOp<T>() ? 0xd2 : 0xd3 );
		EmitSibMagic( InstType, to );
	}

	template< typename T > __noinline void operator()( const ModSibStrict<T>& sibdest,	__unused const xRegisterCL& from ) const
	{
		prefix16<T>();
		xWrite8( Is8BitOp<T>() ? 0xd2 : 0xd3 );
		EmitSibMagic( InstType, sibdest );
	}

	template< typename T > __noinline void operator()( const ModSibStrict<T>& sibdest, u8 imm ) const
	{
		if( imm == 0 ) return;

		prefix16<T>();
		if( imm == 1 )
		{
			// special encoding of 1's
			xWrite8( Is8BitOp<T>() ? 0xd0 : 0xd1 );
			EmitSibMagic( InstType, sibdest );
		}
		else
		{
			xWrite8( Is8BitOp<T>() ? 0xc0 : 0xc1 );
			EmitSibMagic( InstType, sibdest );
			xWrite8( imm );
		}
	}
	
	template< typename T > __forceinline void operator()( const xRegister<T>& to, u8 imm ) const
	{
		if( imm == 0 ) return;

		prefix16<T>();
		if( imm == 1 )
		{
			// special encoding of 1's
			xWrite8( Is8BitOp<T>() ? 0xd0 : 0xd1 );
			EmitSibMagic( InstType, to );
		}
		else
		{
			xWrite8( Is8BitOp<T>() ? 0xc0 : 0xc1 );
			EmitSibMagic( InstType, to );
			xWrite8( imm );
		}
	}

	// ------------------------------------------------------------------------
	template< typename T > __noinline void operator()( const xDirectOrIndirect<T>& to, u8 imm ) const
	{
		_DoI_helpermess( *this, to, imm );
	}

	template< typename T > __noinline void operator()( const xDirectOrIndirect<T>& to, const xRegisterCL& from ) const
	{
		_DoI_helpermess( *this, to, from );
	}


	Group2ImplAll() {}		// I am a class with no members, so I need an explicit constructor!  Sense abounds.
};
