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

#include "x86types.h"
#include "instructions.h"

#define OpWriteSSE( pre, op )		xOpWrite0F( pre, op, to, from )

namespace x86Emitter {

extern void SimdPrefix( u8 prefix, u16 opcode );
extern void EmitSibMagic( uint regfield, const void* address );
extern void EmitSibMagic( uint regfield, const ModSibBase& info );
extern void EmitSibMagic( uint reg1, const xRegisterBase& reg2 );
extern void EmitSibMagic( const xRegisterBase& reg1, const xRegisterBase& reg2 );
extern void EmitSibMagic( const xRegisterBase& reg1, const void* src );
extern void EmitSibMagic( const xRegisterBase& reg1, const ModSibBase& sib );

extern void _xMovRtoR( const xRegisterInt& to, const xRegisterInt& from );
extern void _g1_EmitOp( G1Type InstType, const xRegisterInt& to, const xRegisterInt& from );

template< typename T > inline
void xWrite( T val )
{
	*(T*)x86Ptr = val;
	x86Ptr += sizeof(T);
}

template< typename T1, typename T2 > __emitinline
void xOpWrite( u8 prefix, u8 opcode, const T1& param1, const T2& param2 )
{
	if( prefix != 0 )
		xWrite16( (opcode<<8) | prefix );
	else
		xWrite8( opcode );

	EmitSibMagic( param1, param2 );
}

template< typename T1, typename T2 > __emitinline
void xOpWrite0F( u8 prefix, u16 opcode, const T1& param1, const T2& param2 )
{
	SimdPrefix( prefix, opcode );
	EmitSibMagic( param1, param2 );
}

template< typename T1, typename T2 > __emitinline
void xOpWrite0F( u8 prefix, u16 opcode, const T1& param1, const T2& param2, u8 imm8 )
{
	xOpWrite0F( prefix, opcode, param1, param2 );
	xWrite8( imm8 );
}

template< typename T1, typename T2 > __emitinline
void xOpWrite0F( u16 opcode, const T1& param1, const T2& param2 )			{ xOpWrite0F( 0, opcode, param1, param2 ); }

template< typename T1, typename T2 > __emitinline
void xOpWrite0F( u16 opcode, const T1& param1, const T2& param2, u8 imm8 )	{ xOpWrite0F( 0, opcode, param1, param2, imm8 ); }

}

