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
__emitinline void SimdPrefix( u8 opcode, u8 prefix=0 )
{
	if( sizeof( T ) == 16 && prefix != 0 )
	{
		iWrite<u16>( 0x0f00 | prefix );
		iWrite<u8>( opcode );
	}
	else
		iWrite<u16>( (opcode<<8) | 0x0f );
}

// ------------------------------------------------------------------------
// xmm emitter helpers for xmm instruction with prefixes.
// These functions also support deducing the use of the prefix from the template parameters,
// since most xmm instructions use a prefix and most mmx instructions do not.  (some mov
// instructions violate this "guideline.")
//
template< typename T, typename T2 >
__emitinline void writeXMMop( u8 prefix, u8 opcode, const iRegister<T>& to, const iRegister<T2>& from )
{
	SimdPrefix<T>( opcode, prefix );
	ModRM_Direct( to.Id, from.Id );
}

template< typename T >
void writeXMMop( u8 prefix, u8 opcode, const iRegister<T>& reg, const ModSibBase& sib )
{
	SimdPrefix<T>( opcode, prefix );
	EmitSibMagic( reg.Id, sib );
}

template< typename T >
__emitinline void writeXMMop( u8 prefix, u8 opcode, const iRegister<T>& reg, const void* data )
{
	SimdPrefix<T>( opcode, prefix );
	iWriteDisp( reg.Id, data );
}

// ------------------------------------------------------------------------
// xmm emitter helpers for xmm instructions *without* prefixes.
// These are normally used for special instructions that have MMX forms only (non-SSE), however
// some special forms of sse/xmm mov instructions also use them due to prefixing inconsistencies.
//
template< typename T, typename T2 >
__emitinline void writeXMMop( u8 opcode, const iRegister<T>& to, const iRegister<T2>& from )
{
	SimdPrefix<T>( opcode );
	ModRM_Direct( to.Id, from.Id );
}

template< typename T >
void writeXMMop( u8 opcode, const iRegister<T>& reg, const ModSibBase& sib )
{
	SimdPrefix<T>( opcode );
	EmitSibMagic( reg.Id, sib );
}

template< typename T >
__emitinline void writeXMMop( u8 opcode, const iRegister<T>& reg, const void* data )
{
	SimdPrefix<T>( opcode );
	iWriteDisp( reg.Id, data );
}

//////////////////////////////////////////////////////////////////////////////////////////
// Moves to/from high/low portions of an xmm register.
// These instructions cannot be used in reg/reg form.
//
template< u8 Prefix, u8 Opcode >
class MovhlImplAll
{
public:
	__forceinline void operator()( const iRegisterSSE& to, const void* from ) const			{ writeXMMop( Prefix, Opcode, to, from ); }
	__forceinline void operator()( const void* to, const iRegisterSSE& from ) const			{ writeXMMop( Prefix, Opcode+1, from, to ); }
	__noinline void operator()( const iRegisterSSE& to, const ModSibBase& from ) const		{ writeXMMop( Prefix, Opcode, to, from ); }
	__noinline void operator()( const ModSibBase& to, const iRegisterSSE& from ) const		{ writeXMMop( Prefix, Opcode+1, from, to ); }

	MovhlImplAll() {} //GCC.
};

template< u8 Prefix, u8 Opcode, u8 OpcodeAlt >
class MovapsImplAll
{
public:
	__forceinline void operator()( const iRegisterSSE& to, const iRegisterSSE& from ) const	{ if( to != from ) writeXMMop( Prefix, Opcode, to, from ); }
	__forceinline void operator()( const iRegisterSSE& to, const void* from ) const			{ writeXMMop( Prefix, Opcode, to, from ); }
	__forceinline void operator()( const void* to, const iRegisterSSE& from ) const			{ writeXMMop( Prefix, OpcodeAlt, from, to ); }
	__noinline void operator()( const iRegisterSSE& to, const ModSibBase& from ) const		{ writeXMMop( Prefix, Opcode, to, from ); }
	__noinline void operator()( const ModSibBase& to, const iRegisterSSE& from ) const		{ writeXMMop( Prefix, OpcodeAlt, from, to ); }
	
	MovapsImplAll() {} //GCC.
};

//////////////////////////////////////////////////////////////////////////////////////////
// PLogicImplAll - Implements logic forms for MMX/SSE instructions, and can be used for
// a few other various instruction too (anything which comes in simdreg,simdreg/ModRM forms).
//
template< u8 Opcode >
class PLogicImplAll
{
public:
	template< typename T >
	__forceinline void operator()( const iRegisterSIMD<T>& to, const iRegisterSIMD<T>& from ) const	{ writeXMMop( 0x66, Opcode, to, from ); }
	template< typename T >
	__forceinline void operator()( const iRegisterSIMD<T>& to, const void* from ) const		{ writeXMMop( 0x66, Opcode, to, from ); }
	template< typename T >
	__noinline void operator()( const iRegisterSIMD<T>& to, const ModSibBase& from ) const		{ writeXMMop( 0x66, Opcode, to, from ); }

	PLogicImplAll() {} //GCWho?
};

// ------------------------------------------------------------------------
// For implementing SSE-only logic operations, like ANDPS/ANDPD
//
template< u8 Prefix, u8 Opcode >
class SSELogicImpl
{
public:
	__forceinline void operator()( const iRegisterSSE& to, const iRegisterSSE& from ) const	{ writeXMMop( Prefix, Opcode, to, from ); }
	__forceinline void operator()( const iRegisterSSE& to, const void* from ) const			{ writeXMMop( Prefix, Opcode, to, from ); }
	__noinline void operator()( const iRegisterSSE& to, const ModSibBase& from ) const			{ writeXMMop( Prefix, Opcode, to, from ); }

	SSELogicImpl() {} //GCWho?
};


// ------------------------------------------------------------------------
// For implementing SSE-only comparison operations, like CMPEQPS.
//
enum SSE2_ComparisonType
{
	SSE2_Equal = 0,
	SSE2_Less,
	SSE2_LessOrEqual,
	SSE2_Unordered,
	SSE2_NotEqual,
	SSE2_NotLess,
	SSE2_NotLessOrEqual,
	SSE2_Ordered
};

template< u8 Prefix >
class SSECompareImplGeneric
{
public:
	__forceinline void operator()( const iRegisterSSE& to, const iRegisterSSE& from, u8 cmptype ) const	{ writeXMMop( Prefix, 0xc2, to, from ); iWrite( cmptype ); }
	__forceinline void operator()( const iRegisterSSE& to, const void* from, u8 cmptype ) const			{ writeXMMop( Prefix, 0xc2, to, from ); iWrite( cmptype ); }
	__noinline void operator()( const iRegisterSSE& to, const ModSibBase& from, u8 cmptype ) const		{ writeXMMop( Prefix, 0xc2, to, from ); iWrite( cmptype ); }

	SSECompareImplGeneric() {} //GCWhat?
};

template< u8 Prefix, u8 Opcode, SSE2_ComparisonType CType >
class SSECompareImpl
{
public:
	__forceinline void operator()( const iRegisterSSE& to, const iRegisterSSE& from ) const	{ writeXMMop( Prefix, 0xc2, to, from ); iWrite( CType ); }
	__forceinline void operator()( const iRegisterSSE& to, const void* from ) const			{ writeXMMop( Prefix, 0xc2, to, from ); iWrite( CType ); }
	__noinline void operator()( const iRegisterSSE& to, const ModSibBase& from ) const		{ writeXMMop( Prefix, 0xc2, to, from ); iWrite( CType ); }

	SSECompareImpl() {} //GCWhat?
};
