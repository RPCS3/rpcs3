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
// MMX / SSE Helper Functions!

template< typename T >
__emitinline void SimdPrefix( u8 opcode, u8 prefix )
{
	if( sizeof( T ) == 16 && prefix != 0 )
	{
		iWrite<u16>( 0x0f00 | prefix );
		iWrite<u8>( opcode );
	}
	else
		iWrite<u16>( (opcode<<8) | 0x0f );
}

template< u8 prefix, typename T, typename T2 >
__emitinline void writeXMMop( u8 opcode, const iRegister<T>& to, const iRegister<T2>& from )
{
	SimdPrefix<T>( opcode, prefix );
	ModRM_Direct( to.Id, from.Id );
}

template< u8 prefix, typename T >
void writeXMMop( u8 opcode, const iRegister<T>& reg, const ModSibBase& sib )
{
	SimdPrefix<T>( opcode, prefix );
	EmitSibMagic( reg.Id, sib );
}

template< u8 prefix, typename T >
__emitinline void writeXMMop( u8 opcode, const iRegister<T>& reg, const void* data )
{
	SimdPrefix<T>( opcode, prefix );
	iWriteDisp( reg.Id, data );
}

//////////////////////////////////////////////////////////////////////////////////////////
//
template< u8 Prefix, typename OperandType >
class MovapsImpl
{
public:
	// ------------------------------------------------------------------------
	static __emitinline void Emit( u8 opcode, const iRegisterSIMD<OperandType>& to, const iRegisterSIMD<OperandType> from )
	{
		if( to != from )
			writeXMMop<Prefix,OperandType>( opcode, to, from );
	}

	// ------------------------------------------------------------------------
	static __emitinline void Emit( u8 opcode, const iRegisterSIMD<OperandType>& to, const void* from )
	{
		writeXMMop<Prefix,OperandType>( opcode, to, from );
	}

	// ------------------------------------------------------------------------
	static __emitinline void Emit( u8 opcode, const iRegisterSIMD<OperandType>& to, const ModSibBase& from )
	{
		writeXMMop<Prefix,OperandType>( opcode, to, from );
	}

	// ------------------------------------------------------------------------
	// Generally a Movaps/dqa instruction form only.
	// Most SSE/MMX instructions don't have this form.
	static __emitinline void Emit( u8 opcode, const void* to, const iRegisterSIMD<OperandType>& from )
	{
		writeXMMop<Prefix,OperandType>( opcode, from, to );
	}

	// ------------------------------------------------------------------------
	// Generally a Movaps/dqa instruction form only.
	// Most SSE/MMX instructions don't have this form.
	static __emitinline void Emit( u8 opcode, const ModSibBase& to, const iRegisterSIMD<OperandType>& from )
	{
		writeXMMop<Prefix,OperandType>( opcode, from, to );
	}

};

// ------------------------------------------------------------------------
template< u8 Prefix, u8 Opcode, u8 OpcodeAlt >
class MovapsImplAll
{
protected:
	typedef MovapsImpl<Prefix, u128> m_128;

public:
	__forceinline void operator()( const iRegisterSSE& to, const iRegisterSSE& from ) const { m_128::Emit( Opcode, to, from ); }
	__forceinline void operator()( const iRegisterSSE& to, const void* from ) const { m_128::Emit( Opcode, to, from ); }
	__forceinline void operator()( const void* to, const iRegisterSSE& from ) const { m_128::Emit( OpcodeAlt, to, from ); }
	__noinline void operator()( const iRegisterSSE& to, const ModSibBase& from ) const { m_128::Emit( Opcode, to, from ); }
	__noinline void operator()( const ModSibBase& to, const iRegisterSSE& from ) const { m_128::Emit( OpcodeAlt, to, from ); }
	
	MovapsImplAll() {} //GCC.
};

