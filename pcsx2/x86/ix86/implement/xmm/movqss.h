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

extern void SimdPrefix( u8 prefix, u8 opcode );

// ------------------------------------------------------------------------
// xmm emitter helpers for xmm instruction with prefixes.
// These functions also support deducing the use of the prefix from the template parameters,
// since most xmm instructions use a prefix and most mmx instructions do not.  (some mov
// instructions violate this "guideline.")
//
template< typename T, typename T2 >
__emitinline void writeXMMop( u8 prefix, u8 opcode, const xRegister<T>& to, const xRegister<T2>& from, bool forcePrefix=false )
{
	SimdPrefix( (forcePrefix || (sizeof( T ) == 16)) ? prefix : 0, opcode );
	ModRM_Direct( to.Id, from.Id );
}

template< typename T >
void writeXMMop( u8 prefix, u8 opcode, const xRegister<T>& reg, const ModSibBase& sib, bool forcePrefix=false )
{
	SimdPrefix( (forcePrefix || (sizeof( T ) == 16)) ? prefix : 0, opcode );
	EmitSibMagic( reg.Id, sib );
}

template< typename T >
__emitinline void writeXMMop( u8 prefix, u8 opcode, const xRegister<T>& reg, const void* data, bool forcePrefix=false )
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
__emitinline void writeXMMop( u8 opcode, const xRegister<T>& to, const xRegister<T2>& from )
{
	SimdPrefix( 0, opcode );
	ModRM_Direct( to.Id, from.Id );
}

template< typename T >
void writeXMMop( u8 opcode, const xRegister<T>& reg, const ModSibBase& sib )
{
	SimdPrefix( 0, opcode );
	EmitSibMagic( reg.Id, sib );
}

template< typename T >
__emitinline void writeXMMop( u8 opcode, const xRegister<T>& reg, const void* data )
{
	SimdPrefix( 0, opcode );
	xWriteDisp( reg.Id, data );
}

//////////////////////////////////////////////////////////////////////////////////////////
// Moves to/from high/low portions of an xmm register.
// These instructions cannot be used in reg/reg form.
//
template< u8 Opcode >
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
template< u8 Opcode >
class MovhlImpl_RtoR
{
public:
	__forceinline void PS( const xRegisterSSE& to, const xRegisterSSE& from ) const			{ writeXMMop( Opcode, to, from ); }
	__forceinline void PD( const xRegisterSSE& to, const xRegisterSSE& from ) const			{ writeXMMop( 0x66, Opcode, to, from ); }

	MovhlImpl_RtoR() {} //GCC.
};

// ------------------------------------------------------------------------
template< u8 Prefix, u8 Opcode, u8 OpcodeAlt >
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
// PLogicImplAll - Implements logic forms for MMX/SSE instructions, and can be used for
// a few other various instruction too (anything which comes in simdreg,simdreg/ModRM forms).
//
template< u8 Opcode >
class PLogicImplAll
{
public:
	template< typename T >
	__forceinline void operator()( const xRegisterSIMD<T>& to, const xRegisterSIMD<T>& from ) const	{ writeXMMop( 0x66, Opcode, to, from ); }
	template< typename T >
	__forceinline void operator()( const xRegisterSIMD<T>& to, const void* from ) const				{ writeXMMop( 0x66, Opcode, to, from ); }
	template< typename T >
	__noinline void operator()( const xRegisterSIMD<T>& to, const ModSibBase& from ) const			{ writeXMMop( 0x66, Opcode, to, from ); }

	PLogicImplAll() {} //GCWho?
};

// ------------------------------------------------------------------------
// For implementing SSE-only logic operations, like ANDPS/ANDPD
//
template< u8 Prefix, u8 Opcode >
class SSELogicImpl
{
public:
	__forceinline void operator()( const xRegisterSSE& to, const xRegisterSSE& from ) const	{ writeXMMop( Prefix, Opcode, to, from ); }
	__forceinline void operator()( const xRegisterSSE& to, const void* from ) const			{ writeXMMop( Prefix, Opcode, to, from ); }
	__noinline void operator()( const xRegisterSSE& to, const ModSibBase& from ) const		{ writeXMMop( Prefix, Opcode, to, from ); }

	SSELogicImpl() {} //GCWho?
};

// ------------------------------------------------------------------------
// For implementing MMX/SSE operations which the destination *must* be a register, but the source
// can be regDirect or ModRM (indirect).
//
template< u8 Prefix, u8 Opcode, typename DestRegType, typename SrcRegType, typename SrcOperandType >
class SSEImpl_DestRegForm
{
public:
	__forceinline void operator()( const DestRegType& to, const SrcRegType& from ) const				{ writeXMMop( Prefix, Opcode, to, from, true ); }
	__forceinline void operator()( const DestRegType& to, const SrcOperandType* from ) const			{ writeXMMop( Prefix, Opcode, to, from, true ); }
	__noinline void operator()( const DestRegType& to, const ModSibStrict<SrcOperandType>& from ) const	{ writeXMMop( Prefix, Opcode, to, from, true ); }

	SSEImpl_DestRegForm() {} //GCWho?
};

// ------------------------------------------------------------------------
template< u8 OpcodeSSE >
class SSEImpl_PSPD_SSSD
{
public:
	const SSELogicImpl<0x00,OpcodeSSE> PS;		// packed single precision
	const SSELogicImpl<0x66,OpcodeSSE> PD;		// packed double precision
	const SSELogicImpl<0xf3,OpcodeSSE> SS;		// scalar single precision
	const SSELogicImpl<0xf2,OpcodeSSE> SD;		// scalar double precision
	
	SSEImpl_PSPD_SSSD() {}  //GChow?
};

// ------------------------------------------------------------------------
//
template< u8 OpcodeSSE >
class SSEAndNotImpl
{
public:
	const SSELogicImpl<0x00,OpcodeSSE> PS;
	const SSELogicImpl<0x66,OpcodeSSE> PD;
	SSEAndNotImpl() {}
};

// ------------------------------------------------------------------------
// For instructions that have SS/SD form only (UCOMI, etc)
// AltPrefix - prefixed used for doubles (SD form).
template< u8 AltPrefix, u8 OpcodeSSE >
class SSEImpl_SS_SD
{
public:
	const SSELogicImpl<0x00,OpcodeSSE> SS;
	const SSELogicImpl<AltPrefix,OpcodeSSE> SD;
	SSEImpl_SS_SD() {}
};

// ------------------------------------------------------------------------
// For instructions that have PS/SS form only (most commonly reciprocal Sqrt functions)
template< u8 OpcodeSSE >
class SSE_rSqrtImpl
{
public:
	const SSELogicImpl<0x00,OpcodeSSE> PS;
	const SSELogicImpl<0xf3,OpcodeSSE> SS;
	SSE_rSqrtImpl() {}
};

// ------------------------------------------------------------------------
// For instructions that have PS/SS/SD form only (most commonly Sqrt functions)
template< u8 OpcodeSSE >
class SSE_SqrtImpl : public SSE_rSqrtImpl<OpcodeSSE>
{
public:
	const SSELogicImpl<0xf2,OpcodeSSE> SD;
	SSE_SqrtImpl() {}
};

// ------------------------------------------------------------------------
template< u8 OpcodeSSE >
class SSEImpl_Shuffle
{
protected:
	template< u8 Prefix > struct Woot
	{
		__forceinline void operator()( const xRegisterSSE& to, const xRegisterSSE& from, u8 cmptype ) const	{ writeXMMop( Prefix, OpcodeSSE, to, from ); xWrite<u8>( cmptype ); }
		__forceinline void operator()( const xRegisterSSE& to, const void* from, u8 cmptype ) const			{ writeXMMop( Prefix, OpcodeSSE, to, from ); xWrite<u8>( cmptype ); }
		__noinline void operator()( const xRegisterSSE& to, const ModSibBase& from, u8 cmptype ) const		{ writeXMMop( Prefix, OpcodeSSE, to, from ); xWrite<u8>( cmptype ); }
		Woot() {}
	};

public:
	const Woot<0x00> PS;
	const Woot<0x66> PD;

	SSEImpl_Shuffle() {} //GCWhat?
};

// ------------------------------------------------------------------------
template< SSE2_ComparisonType CType >
class SSECompareImpl
{
protected:
	template< u8 Prefix > struct Woot
	{
		__forceinline void operator()( const xRegisterSSE& to, const xRegisterSSE& from ) const	{ writeXMMop( Prefix, 0xc2, to, from ); xWrite<u8>( CType ); }
		__forceinline void operator()( const xRegisterSSE& to, const void* from ) const			{ writeXMMop( Prefix, 0xc2, to, from ); xWrite<u8>( CType ); }
		__noinline void operator()( const xRegisterSSE& to, const ModSibBase& from ) const		{ writeXMMop( Prefix, 0xc2, to, from ); xWrite<u8>( CType ); }
		Woot() {}
	};

public:
	const Woot<0x00> PS;
	const Woot<0x66> PD;
	const Woot<0xf3> SS;
	const Woot<0xf2> SD;
	SSECompareImpl() {} //GCWhat?
};
