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

//////////////////////////////////////////////////////////////////////////////////////////
// TEST instruction Implementation

template< typename ImmType >
class TestImpl
{
protected:
	static const uint OperandSize = sizeof(ImmType);
	static bool Is8BitOperand()	{ return OperandSize == 1; }
	static void prefix16()		{ if( OperandSize == 2 ) iWrite<u8>( 0x66 ); }

public:
	TestImpl() {}

	// ------------------------------------------------------------------------
	static __emitinline void Emit( const iRegister<ImmType>& to, const iRegister<ImmType>& from )
	{
		prefix16();
		iWrite<u8>( Is8BitOperand() ? 0x84 : 0x85 );
		ModRM_Direct( from.Id, to.Id );
	}

	// ------------------------------------------------------------------------
	static __emitinline void Emit( const iRegister<ImmType>& to, ImmType imm )
	{
		prefix16();
		
		if( to.IsAccumulator() )
			iWrite<u8>( Is8BitOperand() ? 0xa8 : 0xa9 );
		else
		{
			iWrite<u8>( Is8BitOperand() ? 0xf6 : 0xf7 );
			ModRM_Direct( 0, to.Id );
		}
		iWrite<ImmType>( imm );
	}

	// ------------------------------------------------------------------------
	static __emitinline void Emit( ModSibStrict<ImmType> dest, ImmType imm )
	{
		prefix16();
		iWrite<u8>( Is8BitOperand() ? 0xf6 : 0xf7 );
		EmitSibMagic( 0, dest );
		iWrite<ImmType>( imm );
	}
};

// -------------------------------------------------------------------
//
class TestImplAll
{
public:
	template< typename T >
	__forceinline void operator()( const iRegister<T>& to,	const iRegister<T>& from ) const	{ TestImpl<T>::Emit( to, from ); }

	template< typename T >
	__noinline void operator()( const ModSibStrict<T>& sibdest, T imm ) const	{ TestImpl<T>::Emit( sibdest, imm ); }
	template< typename T >
	void operator()( const iRegister<T>& to, T imm ) const						{ TestImpl<T>::Emit( to, imm ); }

	TestImplAll() {}		// Why does GCC need these?
};

