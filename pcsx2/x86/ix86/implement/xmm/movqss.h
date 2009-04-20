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
// SimdImpl_PackedLogic - Implements logic forms for MMX/SSE instructions, and can be used for
// a few other various instruction too (anything which comes in simdreg,simdreg/ModRM forms).
//
template< u8 Opcode >
class SimdImpl_PackedLogic
{
public:
	template< typename T >
	__forceinline void operator()( const xRegisterSIMD<T>& to, const xRegisterSIMD<T>& from ) const	{ writeXMMop( 0x66, Opcode, to, from ); }
	template< typename T >
	__forceinline void operator()( const xRegisterSIMD<T>& to, const void* from ) const				{ writeXMMop( 0x66, Opcode, to, from ); }
	template< typename T >
	__noinline void operator()( const xRegisterSIMD<T>& to, const ModSibBase& from ) const			{ writeXMMop( 0x66, Opcode, to, from ); }

	SimdImpl_PackedLogic() {} //GCWho?
};

// ------------------------------------------------------------------------
// For implementing SSE-only logic operations that have reg,reg/rm forms only,
// like ANDPS/ANDPD
//
template< u8 Prefix, u8 Opcode >
class SimdImpl_DestRegSSE
{
public:
	__forceinline void operator()( const xRegisterSSE& to, const xRegisterSSE& from ) const	{ writeXMMop( Prefix, Opcode, to, from ); }
	__forceinline void operator()( const xRegisterSSE& to, const void* from ) const			{ writeXMMop( Prefix, Opcode, to, from ); }
	__noinline void operator()( const xRegisterSSE& to, const ModSibBase& from ) const		{ writeXMMop( Prefix, Opcode, to, from ); }

	SimdImpl_DestRegSSE() {} //GCWho?
};

// ------------------------------------------------------------------------
// For implementing MMX/SSE operations that have reg,reg/rm forms only,
// but accept either MM or XMM destinations (most PADD/PSUB and other P srithmetic ops).
//
template< u8 Prefix, u8 Opcode >
class SimdImpl_DestRegEither
{
public:
	template< typename DestOperandType >
	__forceinline void operator()( const xRegisterSIMD<DestOperandType>& to, const xRegisterSIMD<DestOperandType>& from ) const	{ writeXMMop( Prefix, Opcode, to, from ); }
	template< typename DestOperandType >
	__forceinline void operator()( const xRegisterSIMD<DestOperandType>& to, const void* from ) const			{ writeXMMop( Prefix, Opcode, to, from ); }
	template< typename DestOperandType >
	__noinline void operator()( const xRegisterSIMD<DestOperandType>& to, const ModSibBase& from ) const		{ writeXMMop( Prefix, Opcode, to, from ); }

	SimdImpl_DestRegEither() {} //GCWho?
};

// ------------------------------------------------------------------------
// For implementing MMX/SSE operations which the destination *must* be a register, but the source
// can be regDirect or ModRM (indirect).
//
template< u8 Prefix, u8 Opcode, typename DestRegType, typename SrcRegType, typename SrcOperandType >
class SimdImpl_DestRegStrict
{
public:
	__forceinline void operator()( const DestRegType& to, const SrcRegType& from ) const				{ writeXMMop( Prefix, Opcode, to, from, true ); }
	__forceinline void operator()( const DestRegType& to, const SrcOperandType* from ) const			{ writeXMMop( Prefix, Opcode, to, from, true ); }
	__noinline void operator()( const DestRegType& to, const ModSibStrict<SrcOperandType>& from ) const	{ writeXMMop( Prefix, Opcode, to, from, true ); }

	SimdImpl_DestRegStrict() {} //GCWho?
};

// ------------------------------------------------------------------------
template< u8 OpcodeSSE >
class SimdImpl_PSPD_SSSD
{
public:
	const SimdImpl_DestRegSSE<0x00,OpcodeSSE> PS;		// packed single precision
	const SimdImpl_DestRegSSE<0x66,OpcodeSSE> PD;		// packed double precision
	const SimdImpl_DestRegSSE<0xf3,OpcodeSSE> SS;		// scalar single precision
	const SimdImpl_DestRegSSE<0xf2,OpcodeSSE> SD;		// scalar double precision
	
	SimdImpl_PSPD_SSSD() {}  //GChow?
};

// ------------------------------------------------------------------------
//
template< u8 OpcodeSSE >
class SimdImpl_AndNot
{
public:
	const SimdImpl_DestRegSSE<0x00,OpcodeSSE> PS;
	const SimdImpl_DestRegSSE<0x66,OpcodeSSE> PD;
	SimdImpl_AndNot() {}
};

// ------------------------------------------------------------------------
// For instructions that have SS/SD form only (UCOMI, etc)
// AltPrefix - prefixed used for doubles (SD form).
template< u8 AltPrefix, u8 OpcodeSSE >
class SimdImpl_SS_SD
{
public:
	const SimdImpl_DestRegSSE<0x00,OpcodeSSE> SS;
	const SimdImpl_DestRegSSE<AltPrefix,OpcodeSSE> SD;
	SimdImpl_SS_SD() {}
};

// ------------------------------------------------------------------------
// For instructions that have PS/SS form only (most commonly reciprocal Sqrt functions)
template< u8 OpcodeSSE >
class SimdImpl_rSqrt
{
public:
	const SimdImpl_DestRegSSE<0x00,OpcodeSSE> PS;
	const SimdImpl_DestRegSSE<0xf3,OpcodeSSE> SS;
	SimdImpl_rSqrt() {}
};

// ------------------------------------------------------------------------
// For instructions that have PS/SS/SD form only (most commonly Sqrt functions)
template< u8 OpcodeSSE >
class SimdImpl_Sqrt : public SimdImpl_rSqrt<OpcodeSSE>
{
public:
	const SimdImpl_DestRegSSE<0xf2,OpcodeSSE> SD;
	SimdImpl_Sqrt() {}
};

// ------------------------------------------------------------------------
template< u8 OpcodeSSE >
class SimdImpl_Shuffle
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

	SimdImpl_Shuffle() {} //GCWhat?
};

// ------------------------------------------------------------------------
template< SSE2_ComparisonType CType >
class SimdImpl_Compare
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
	SimdImpl_Compare() {} //GCWhat?
};


//////////////////////////////////////////////////////////////////////////////////////////
//
//
template< u8 Opcode1, u8 OpcodeImm, u8 Modcode >
class SimdImpl_Shift
{
public:
	SimdImpl_Shift() {}

	template< typename OperandType >
	__forceinline void operator()( const xRegisterSIMD<OperandType>& to, const xRegisterSIMD<OperandType>& from ) const
	{
		writeXMMop( 0x66, Opcode1, to, from );
	}

	template< typename OperandType >
	__forceinline void operator()( const xRegisterSIMD<OperandType>& to, const void* from ) const
	{
		writeXMMop( 0x66, Opcode1, to, from );
	}

	template< typename OperandType >
	__noinline void operator()( const xRegisterSIMD<OperandType>& to, const ModSibBase& from ) const
	{
		writeXMMop( 0x66, Opcode1, to, from );
	}

	template< typename OperandType >
	__emitinline void operator()( const xRegisterSIMD<OperandType>& to, u8 imm ) const
	{
		SimdPrefix( (sizeof( OperandType ) == 16) ? 0x66 : 0, OpcodeImm );
		ModRM( 3, (int)Modcode, to.Id );
		xWrite<u8>( imm );
	}
};

// ------------------------------------------------------------------------
template< u8 OpcodeBase1, u8 OpcodeBaseImm, u8 Modcode >
class SimdImpl_ShiftAll
{
public:
	const SimdImpl_Shift<OpcodeBase1+1,OpcodeBaseImm+1,Modcode> W;
	const SimdImpl_Shift<OpcodeBase1+2,OpcodeBaseImm+2,Modcode> D;
	const SimdImpl_Shift<OpcodeBase1+3,OpcodeBaseImm+3,Modcode> Q;
	
	void DQ( const xRegisterSSE& to, u8 imm ) const
	{
		SimdPrefix( 0x66, OpcodeBaseImm+3 );
		ModRM( 3, (int)Modcode+1, to.Id );
		xWrite<u8>( imm );
	}
	
	SimdImpl_ShiftAll() {}
};

//////////////////////////////////////////////////////////////////////////////////////////
//
template< u8 OpcodeB, u8 OpcodeS, u8 OpcodeUS, u8 OpcodeQ >
class SimdImpl_AddSub
{
public:
	const SimdImpl_DestRegEither<0x66,OpcodeB> B;
	const SimdImpl_DestRegEither<0x66,OpcodeB+1> W;
	const SimdImpl_DestRegEither<0x66,OpcodeB+2> D;
	const SimdImpl_DestRegEither<0x66,OpcodeQ> Q;

	// Add/Sub packed signed byte [8bit] integers from src into dest, and saturate the results.
	const SimdImpl_DestRegEither<0x66,OpcodeS> SB;

	// Add/Sub packed signed word [16bit] integers from src into dest, and saturate the results.
	const SimdImpl_DestRegEither<0x66,OpcodeS+1> SW;

	// Add/Sub packed unsigned byte [8bit] integers from src into dest, and saturate the results.
	const SimdImpl_DestRegEither<0x66,OpcodeUS> USB;

	// Add/Sub packed unsigned word [16bit] integers from src into dest, and saturate the results.
	const SimdImpl_DestRegEither<0x66,OpcodeUS+1> USW;

	SimdImpl_AddSub() {}
};