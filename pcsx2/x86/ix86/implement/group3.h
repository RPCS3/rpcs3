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

template< typename ImmType >
class Group3Impl : public ImplementationHelper<ImmType>
{
public: 
	Group3Impl() {}		// For the love of GCC.

	static __emitinline void Emit( G3Type InstType, const iRegister<ImmType>& from )
	{
		prefix16();
		iWrite<u8>( Is8BitOperand() ? 0xf6 : 0xf7 );
		ModRM_Direct( InstType, from.Id );
	}

	static __emitinline void Emit( G3Type InstType, const ModSibStrict<ImmType>& sibsrc )
	{
		prefix16();
		iWrite<u8>( Is8BitOperand() ? 0xf6 : 0xf7 );
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
	__forceinline void operator()( const iRegister<T>& from ) const	{ Group3Impl<T>::Emit( InstType, from ); }

	template< typename T >
	__noinline void operator()( const ModSibStrict<T>& from ) const { Group3Impl<T>::Emit( InstType, from ); }
};