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

// This helper function is used for instructions which enter XMM form when the 0x66 prefix
// is specified (indicating alternate operand type selection).
template< typename OperandType >
static __forceinline void preXMM( u8 opcode )
{
	if( sizeof( OperandType ) == 16 )
		iWrite<u16>( 0x0f66 );
	else
		iWrite<u8>( 0x0f );
	iWrite<u8>( opcode );
}

// prefix - 0 indicates MMX, anything assumes XMM.
static __forceinline void SimdPrefix( u8 opcode, u8 prefix=0 )
{
	if( prefix != 0 )
	{
		iWrite<u16>( 0x0f00 | prefix );
		iWrite<u8>( opcode );
	}
	else
		iWrite<u16>( (opcode<<8) | 0x0f );
}

template< u8 prefix, typename T, typename T2 >
static __forceinline void writeXMMop( const iRegister<T>& to, const iRegister<T2>& from, u8 opcode )
{
	SimdPrefix( opcode, prefix );
	ModRM_Direct( to.Id, from.Id );
}

template< u8 prefix, typename T >
static __noinline void writeXMMop( const iRegister<T>& reg, const ModSibBase& sib, u8 opcode )
{
	SimdPrefix( opcode, prefix );
	EmitSibMagic( reg.Id, sib );
}

template< u8 prefix, typename T >
static __forceinline void writeXMMop( const iRegister<T>& reg, const void* data, u8 opcode )
{
	SimdPrefix( opcode, prefix );
	iWriteDisp( reg.Id, data );
}

// ------------------------------------------------------------------------
// MOVD has valid forms for MMX and XMM registers.
//
template< typename T >
static __forceinline void iMOVDZX( const iRegisterSIMD<T>& to, const iRegister32& from )
{
	preXMM<T>( 0x6e );
	ModRM_Direct( to.Id, from.Id );
}

template< typename T>
static __forceinline void iMOVDZX( const iRegisterSIMD<T>& to,	const void* src )
{
	preXMM<T>( 0x6e );
	iWriteDisp( to.Id, src );
}

template< typename T>
static __forceinline void iMOVDZX( const iRegisterSIMD<T>& to,	const ModSibBase& src )
{
	preXMM<T>( 0x6e );
	EmitSibMagic( to.Id, src );
}

template< typename T>
static __emitinline void iMOVD( const iRegister32& to, const iRegisterSIMD<T>& from )
{
	preXMM<T>( 0x7e );
	ModRM_Direct( from.Id, to.Id );
}

template< typename T>
static __forceinline void iMOVD( void* dest, const iRegisterSIMD<T>& from )
{
	preXMM<T>( 0x7e );
	iWriteDisp( from.Id, dest );
}

template< typename T>
static __noinline void iMOVD( const ModSibBase& dest, const iRegisterSIMD<T>& from )
{
	preXMM<T>( 0x7e );
	EmitSibMagic( from.Id, dest );
}
