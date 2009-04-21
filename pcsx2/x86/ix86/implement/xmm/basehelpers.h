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

// ------------------------------------------------------------------------
// xmm emitter helpers for xmm instruction with prefixes.
// These functions also support deducing the use of the prefix from the template parameters,
// since most xmm instructions use a prefix and most mmx instructions do not.  (some mov
// instructions violate this "guideline.")
//
template< typename T, typename T2 >
__emitinline void writeXMMop( u8 prefix, u16 opcode, const xRegister<T>& to, const xRegister<T2>& from, bool forcePrefix=false )
{
	SimdPrefix( (forcePrefix || (sizeof( T ) == 16)) ? prefix : 0, opcode );
	ModRM_Direct( to.Id, from.Id );
}

template< typename T >
__noinline void writeXMMop( u8 prefix, u16 opcode, const xRegister<T>& reg, const ModSibBase& sib, bool forcePrefix=false )
{
	SimdPrefix( (forcePrefix || (sizeof( T ) == 16)) ? prefix : 0, opcode );
	EmitSibMagic( reg.Id, sib );
}

template< typename T >
__emitinline void writeXMMop( u8 prefix, u16 opcode, const xRegister<T>& reg, const void* data, bool forcePrefix=false )
{
	SimdPrefix( (forcePrefix || (sizeof( T ) == 16)) ? prefix : 0, opcode );
	xWriteDisp( reg.Id, data );
}

// ------------------------------------------------------------------------
// xmm emitter helpers for xmm instructions *without* prefixes.
// These are normally used for special instructions that have MMX forms only (non-SSE), however
// some special forms of sse/xmm mov instructions also use them due to prefixing inconsistencies.
//
template< typename T, typename T2 >
__emitinline void writeXMMop( u16 opcode, const xRegister<T>& to, const xRegister<T2>& from )
{
	SimdPrefix( 0, opcode );
	ModRM_Direct( to.Id, from.Id );
}

template< typename T >
__noinline void writeXMMop( u16 opcode, const xRegister<T>& reg, const ModSibBase& sib )
{
	SimdPrefix( 0, opcode );
	EmitSibMagic( reg.Id, sib );
}

template< typename T >
__emitinline void writeXMMop( u16 opcode, const xRegister<T>& reg, const void* data )
{
	SimdPrefix( 0, opcode );
	xWriteDisp( reg.Id, data );
}

// ------------------------------------------------------------------------
// For implementing SSE-only logic operations that have xmmreg,xmmreg/rm forms only,
// like ANDPS/ANDPD
//
template< u8 Prefix, u16 Opcode >
class SimdImpl_DestRegSSE
{
public:
	__forceinline void operator()( const xRegisterSSE& to, const xRegisterSSE& from ) const	{ writeXMMop( Prefix, Opcode, to, from ); }
	__forceinline void operator()( const xRegisterSSE& to, const void* from ) const			{ writeXMMop( Prefix, Opcode, to, from ); }
	__forceinline void operator()( const xRegisterSSE& to, const ModSibBase& from ) const	{ writeXMMop( Prefix, Opcode, to, from ); }

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
	__forceinline void operator()( const xRegisterSSE& to, const xRegisterSSE& from, u8 imm ) const	{ writeXMMop( Prefix, Opcode, to, from ); xWrite<u8>( imm ); }
	__forceinline void operator()( const xRegisterSSE& to, const void* from, u8 imm ) const			{ writeXMMop( Prefix, Opcode, to, from ); xWrite<u8>( imm ); }
	__forceinline void operator()( const xRegisterSSE& to, const ModSibBase& from, u8 imm ) const	{ writeXMMop( Prefix, Opcode, to, from ); xWrite<u8>( imm ); }

	SimdImpl_DestRegImmSSE() {} //GCWho?
};

template< u8 Prefix, u16 Opcode >
class SimdImpl_DestRegImmMMX
{
public:
	__forceinline void operator()( const xRegisterMMX& to, const xRegisterMMX& from, u8 imm ) const	{ writeXMMop( Prefix, Opcode, to, from ); xWrite<u8>( imm ); }
	__forceinline void operator()( const xRegisterMMX& to, const void* from, u8 imm ) const			{ writeXMMop( Prefix, Opcode, to, from ); xWrite<u8>( imm ); }
	__forceinline void operator()( const xRegisterMMX& to, const ModSibBase& from, u8 imm ) const	{ writeXMMop( Prefix, Opcode, to, from ); xWrite<u8>( imm ); }

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
	template< typename T > __forceinline
	void operator()( const xRegisterSIMD<T>& to, const xRegisterSIMD<T>& from ) const	{ writeXMMop( Prefix, Opcode, to, from ); }
	template< typename T > __forceinline
	void operator()( const xRegisterSIMD<T>& to, const void* from ) const				{ writeXMMop( Prefix, Opcode, to, from ); }
	template< typename T > __forceinline
	void operator()( const xRegisterSIMD<T>& to, const ModSibBase& from ) const			{ writeXMMop( Prefix, Opcode, to, from ); }

	SimdImpl_DestRegEither() {} //GCWho?
};

// ------------------------------------------------------------------------
// For implementing MMX/SSE operations which the destination *must* be a register, but the source
// can be regDirect or ModRM (indirect).
//
template< u8 Prefix, u16 Opcode, typename DestRegType, typename SrcRegType, typename SrcOperandType >
class SimdImpl_DestRegStrict
{
public:
	__forceinline void operator()( const DestRegType& to, const SrcRegType& from ) const					{ writeXMMop( Prefix, Opcode, to, from, true ); }
	__forceinline void operator()( const DestRegType& to, const SrcOperandType* from ) const				{ writeXMMop( Prefix, Opcode, to, from, true ); }
	__forceinline void operator()( const DestRegType& to, const ModSibStrict<SrcOperandType>& from ) const	{ writeXMMop( Prefix, Opcode, to, from, true ); }

	SimdImpl_DestRegStrict() {} //GCWho?
};

