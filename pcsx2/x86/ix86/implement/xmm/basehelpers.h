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

extern void SimdPrefix( u8 prefix, u16 opcode );

extern void xOpWrite0F( u8 prefix, u16 opcode, int instId, const ModSibBase& sib );
extern void xOpWrite0F( u8 prefix, u16 opcode, int instId, const void* data );
extern void xOpWrite0F( u16 opcode, int instId, const ModSibBase& sib );
extern void xOpWrite0F( u16 opcode, int instId, const void* data );

template< typename T2 > __emitinline
void xOpWrite0F( u8 prefix, u16 opcode, int instId, const xRegister<T2>& from )
{
	SimdPrefix( prefix, opcode );
	ModRM_Direct( instId, from.Id );
}

template< typename T2 > __emitinline
void xOpWrite0F( u16 opcode, int instId, const xRegister<T2>& from )
{
	xOpWrite0F( 0, opcode, instId, from );
}

template< typename T, typename T2 > __emitinline
void xOpWrite0F( u8 prefix, u16 opcode, const xRegister<T>& to, const xRegister<T2>& from, bool forcePrefix=false )
{
	xOpWrite0F( prefix, opcode, to.Id, from );
}

template< typename T > __noinline
void xOpWrite0F( u8 prefix, u16 opcode, const xRegister<T>& reg, const ModSibBase& sib, bool forcePrefix=false )
{
	xOpWrite0F( prefix, opcode, reg.Id, sib );
}

template< typename T > __emitinline
void xOpWrite0F( u8 prefix, u16 opcode, const xRegister<T>& reg, const void* data, bool forcePrefix=false )
{
	xOpWrite0F( prefix, opcode, reg.Id, data );
}

// ------------------------------------------------------------------------
//
template< typename T, typename T2 > __emitinline
void xOpWrite0F( u8 prefix, u16 opcode, const xRegister<T>& to, const xRegister<T2>& from, u8 imm8 )
{
	xOpWrite0F( prefix, opcode, to, from );
	xWrite<u8>( imm8 );
}

template< typename T > __noinline
void xOpWrite0F( u8 prefix, u16 opcode, const xRegister<T>& reg, const ModSibBase& sib, u8 imm8 )
{
	xOpWrite0F( prefix, opcode, reg, sib );
	xWrite<u8>( imm8 );
}

template< typename T > __emitinline
void xOpWrite0F( u8 prefix, u16 opcode, const xRegister<T>& reg, const void* data, u8 imm8 )
{
	xOpWrite0F( prefix, opcode, reg, data );
	xWrite<u8>( imm8 );
}

// ------------------------------------------------------------------------

template< typename T, typename T2 > __emitinline
void xOpWrite0F( u16 opcode, const xRegister<T>& to, const xRegister<T2>& from )
{
	xOpWrite0F( 0, opcode, to, from );
}

template< typename T > __noinline
void xOpWrite0F( u16 opcode, const xRegister<T>& reg, const ModSibBase& sib )
{
	xOpWrite0F( 0, opcode, reg, sib );
}

template< typename T > __emitinline
void xOpWrite0F( u16 opcode, const xRegister<T>& reg, const void* data )
{
	xOpWrite0F( 0, opcode, reg, data );
}

// ------------------------------------------------------------------------

template< typename T, typename T2 > __emitinline
void xOpWrite0F( u16 opcode, const xRegister<T>& to, const xRegister<T2>& from, u8 imm8 )
{
	xOpWrite0F( opcode, to, from );
	xWrite<u8>( imm8 );
}

template< typename T > __noinline
void xOpWrite0F( u16 opcode, const xRegister<T>& reg, const ModSibBase& sib, u8 imm8 )
{
	xOpWrite0F( opcode, reg, sib );
	xWrite<u8>( imm8 );
}

template< typename T > __emitinline
void xOpWrite0F( u16 opcode, const xRegister<T>& reg, const void* data, u8 imm8 )
{
	xOpWrite0F( opcode, reg, data );
	xWrite<u8>( imm8 );
}

// ------------------------------------------------------------------------
// For implementing SSE-only logic operations that have xmmreg,xmmreg/rm forms only,
// like ANDPS/ANDPD
//
template< u8 Prefix, u16 Opcode >
class SimdImpl_DestRegSSE
{
public:
	__forceinline void operator()( const xRegisterSSE& to, const xRegisterSSE& from ) const	{ xOpWrite0F( Prefix, Opcode, to, from ); }
	__forceinline void operator()( const xRegisterSSE& to, const void* from ) const			{ xOpWrite0F( Prefix, Opcode, to, from ); }
	__forceinline void operator()( const xRegisterSSE& to, const ModSibBase& from ) const	{ xOpWrite0F( Prefix, Opcode, to, from ); }

	SimdImpl_DestRegSSE() {} //GCWho?
};

// ------------------------------------------------------------------------
// For implementing SSE-only logic operations that have xmmreg,reg/rm,imm forms only
// (PSHUFD / PSHUFHW / etc).
//
template< u8 Prefix, u16 Opcode >
class SimdImpl_DestRegImmSSE
{
public:
	__forceinline void operator()( const xRegisterSSE& to, const xRegisterSSE& from, u8 imm ) const	{ xOpWrite0F( Prefix, Opcode, to, from, imm ); }
	__forceinline void operator()( const xRegisterSSE& to, const void* from, u8 imm ) const			{ xOpWrite0F( Prefix, Opcode, to, from, imm ); }
	__forceinline void operator()( const xRegisterSSE& to, const ModSibBase& from, u8 imm ) const	{ xOpWrite0F( Prefix, Opcode, to, from, imm ); }

	SimdImpl_DestRegImmSSE() {} //GCWho?
};

template< u8 Prefix, u16 Opcode >
class SimdImpl_DestRegImmMMX
{
public:
	__forceinline void operator()( const xRegisterMMX& to, const xRegisterMMX& from, u8 imm ) const	{ xOpWrite0F( Opcode, to, from, imm ); }
	__forceinline void operator()( const xRegisterMMX& to, const void* from, u8 imm ) const			{ xOpWrite0F( Opcode, to, from, imm ); }
	__forceinline void operator()( const xRegisterMMX& to, const ModSibBase& from, u8 imm ) const	{ xOpWrite0F( Opcode, to, from, imm ); }

	SimdImpl_DestRegImmMMX() {} //GCWho?
};

// ------------------------------------------------------------------------
// For implementing MMX/SSE operations that have reg,reg/rm forms only,
// but accept either MM or XMM destinations (most PADD/PSUB and other P srithmetic ops).
//
template< u8 Prefix, u16 Opcode >
class SimdImpl_DestRegEither
{
public:
	__forceinline void operator()( const xRegisterSSE& to, const xRegisterSSE& from ) const	{ xOpWrite0F( Prefix, Opcode, to, from ); }
	__forceinline void operator()( const xRegisterSSE& to, const void* from ) const			{ xOpWrite0F( Prefix, Opcode, to, from ); }
	__forceinline void operator()( const xRegisterSSE& to, const ModSibBase& from ) const	{ xOpWrite0F( Prefix, Opcode, to, from ); }

	__forceinline void operator()( const xRegisterMMX& to, const xRegisterMMX& from ) const	{ xOpWrite0F( Opcode, to, from ); }
	__forceinline void operator()( const xRegisterMMX& to, const void* from ) const			{ xOpWrite0F( Opcode, to, from ); }
	__forceinline void operator()( const xRegisterMMX& to, const ModSibBase& from ) const	{ xOpWrite0F( Opcode, to, from ); }

	SimdImpl_DestRegEither() {} //GCWho?
};

// ------------------------------------------------------------------------
// For implementing MMX/SSE operations where the destination *must* be a register, but the
// source can be Direct or Indirect (ModRM/SibSB).  The SrcOperandType template parameter
// is used to enforce type strictness of the (void*) parameter and ModSib<> parameter, so
// that the programmer must be explicit in specifying desired operand size.
//
// IMPORTANT: This helper assumes the prefix opcode is written *always* -- regardless of
// MMX or XMM register status.
//
template< u8 Prefix, u16 Opcode, typename DestRegType, typename SrcRegType, typename SrcOperandType >
class SimdImpl_DestRegStrict
{
public:
	__forceinline void operator()( const DestRegType& to, const SrcRegType& from ) const					{ xOpWrite0F( Prefix, Opcode, to, from ); }
	__forceinline void operator()( const DestRegType& to, const SrcOperandType* from ) const				{ xOpWrite0F( Prefix, Opcode, to, from ); }
	__forceinline void operator()( const DestRegType& to, const ModSibStrict<SrcOperandType>& from ) const	{ xOpWrite0F( Prefix, Opcode, to, from ); }

	SimdImpl_DestRegStrict() {} //GCWho?
};

