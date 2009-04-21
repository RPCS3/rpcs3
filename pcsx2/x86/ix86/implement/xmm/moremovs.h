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
// Moves to/from high/low portions of an xmm register.
// These instructions cannot be used in reg/reg form.
//
template< u16 Opcode >
class MovhlImplAll
{
protected:
	template< u8 Prefix >
	struct Woot
	{
		__forceinline void operator()( const xRegisterSSE& to, const void* from ) const			{ writeXMMop( Prefix, Opcode, to, from ); }
		__forceinline void operator()( const void* to, const xRegisterSSE& from ) const			{ writeXMMop( Prefix, Opcode+1, from, to ); }
		__noinline void operator()( const xRegisterSSE& to, const ModSibBase& from ) const		{ writeXMMop( Prefix, Opcode, to, from ); }
		__noinline void operator()( const ModSibBase& to, const xRegisterSSE& from ) const		{ writeXMMop( Prefix, Opcode+1, from, to ); }
	};

public:
	Woot<0x00> PS;
	Woot<0x66> PD;

	MovhlImplAll() {} //GCC.
};

// ------------------------------------------------------------------------
// RegtoReg forms of MOVHL/MOVLH -- these are the same opcodes as MOVH/MOVL but
// do something kinda different! Fun!
//
template< u16 Opcode >
class MovhlImpl_RtoR
{
public:
	__forceinline void PS( const xRegisterSSE& to, const xRegisterSSE& from ) const			{ writeXMMop( Opcode, to, from ); }
	__forceinline void PD( const xRegisterSSE& to, const xRegisterSSE& from ) const			{ writeXMMop( 0x66, Opcode, to, from ); }

	MovhlImpl_RtoR() {} //GCC.
};

// ------------------------------------------------------------------------
template< u8 Prefix, u16 Opcode, u16 OpcodeAlt >
class MovapsImplAll
{
public:
	__forceinline void operator()( const xRegisterSSE& to, const xRegisterSSE& from ) const	{ if( to != from ) writeXMMop( Prefix, Opcode, to, from ); }
	__forceinline void operator()( const xRegisterSSE& to, const void* from ) const			{ writeXMMop( Prefix, Opcode, to, from ); }
	__forceinline void operator()( const void* to, const xRegisterSSE& from ) const			{ writeXMMop( Prefix, OpcodeAlt, from, to ); }
	__noinline void operator()( const xRegisterSSE& to, const ModSibBase& from ) const		{ writeXMMop( Prefix, Opcode, to, from ); }
	__noinline void operator()( const ModSibBase& to, const xRegisterSSE& from ) const		{ writeXMMop( Prefix, OpcodeAlt, from, to ); }
	
	MovapsImplAll() {} //GCC.
};

//////////////////////////////////////////////////////////////////////////////////////////
//
template< u8 AltPrefix, u16 OpcodeSSE >
class SimdImpl_UcomI
{
public:
	const SimdImpl_DestRegSSE<0x00,OpcodeSSE> SS;
	const SimdImpl_DestRegSSE<AltPrefix,OpcodeSSE> SD;
	SimdImpl_UcomI() {}
};
