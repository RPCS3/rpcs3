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
// SimdImpl_PackedLogic - Implements logic forms for MMX/SSE instructions, and can be used for
// a few other various instruction too (anything which comes in simdreg,simdreg/ModRM forms).
//
template< u16 Opcode >
class SimdImpl_PackedLogic
{
public:
	template< typename T > __forceinline
	void operator()( const xRegisterSIMD<T>& to, const xRegisterSIMD<T>& from ) const	{ writeXMMop( 0x66, Opcode, to, from ); }
	template< typename T > __forceinline
	void operator()( const xRegisterSIMD<T>& to, const void* from ) const				{ writeXMMop( 0x66, Opcode, to, from ); }
	template< typename T > __forceinline
	void operator()( const xRegisterSIMD<T>& to, const ModSibBase& from ) const		{ writeXMMop( 0x66, Opcode, to, from ); }

	SimdImpl_PackedLogic() {} //GCWho?
};

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
	template< typename DestOperandType > __forceinline
	void operator()( const xRegisterSIMD<DestOperandType>& to, const xRegisterSIMD<DestOperandType>& from ) const	{ writeXMMop( Prefix, Opcode, to, from ); }
	template< typename DestOperandType > __forceinline
	void operator()( const xRegisterSIMD<DestOperandType>& to, const void* from ) const			{ writeXMMop( Prefix, Opcode, to, from ); }
	template< typename DestOperandType > __forceinline
	void operator()( const xRegisterSIMD<DestOperandType>& to, const ModSibBase& from ) const		{ writeXMMop( Prefix, Opcode, to, from ); }

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

// ------------------------------------------------------------------------
template< u16 OpcodeSSE >
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
template< u16 OpcodeSSE >
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
template< u8 AltPrefix, u16 OpcodeSSE >
class SimdImpl_SS_SD
{
public:
	const SimdImpl_DestRegSSE<0x00,OpcodeSSE> SS;
	const SimdImpl_DestRegSSE<AltPrefix,OpcodeSSE> SD;
	SimdImpl_SS_SD() {}
};

// ------------------------------------------------------------------------
// For instructions that have PS/SS form only (most commonly reciprocal Sqrt functions)
template< u16 OpcodeSSE >
class SimdImpl_rSqrt
{
public:
	const SimdImpl_DestRegSSE<0x00,OpcodeSSE> PS;
	const SimdImpl_DestRegSSE<0xf3,OpcodeSSE> SS;
	SimdImpl_rSqrt() {}
};

// ------------------------------------------------------------------------
// For instructions that have PS/SS/SD form only (most commonly Sqrt functions)
template< u16 OpcodeSSE >
class SimdImpl_Sqrt : public SimdImpl_rSqrt<OpcodeSSE>
{
public:
	const SimdImpl_DestRegSSE<0xf2,OpcodeSSE> SD;
	SimdImpl_Sqrt() {}
};

// ------------------------------------------------------------------------
template< u16 OpcodeSSE >
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
template< u16 Opcode1, u16 OpcodeImm, u8 Modcode >
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
// Used for PSRA
template< u16 OpcodeBase1, u8 Modcode >
class SimdImpl_ShiftWithoutQ
{
public:
	const SimdImpl_Shift<OpcodeBase1+1,0x71,Modcode> W;
	const SimdImpl_Shift<OpcodeBase1+2,0x72,Modcode> D;

	SimdImpl_ShiftWithoutQ() {}
};

// ------------------------------------------------------------------------
template< u16 OpcodeBase1, u8 Modcode >
class SimdImpl_ShiftAll : public SimdImpl_ShiftWithoutQ<OpcodeBase1, Modcode>
{
public:
	const SimdImpl_Shift<OpcodeBase1+3,0x73,Modcode> Q;
	
	void DQ( const xRegisterSSE& to, u8 imm ) const
	{
		SimdPrefix( 0x66, 0x73 );
		ModRM( 3, (int)Modcode+1, to.Id );
		xWrite<u8>( imm );
	}
	
	SimdImpl_ShiftAll() {}
};

//////////////////////////////////////////////////////////////////////////////////////////
//
template< u16 OpcodeB, u16 OpcodeQ >
class SimdImpl_AddSub
{
public:
	const SimdImpl_DestRegEither<0x66,OpcodeB+0x20> B;
	const SimdImpl_DestRegEither<0x66,OpcodeB+0x21> W;
	const SimdImpl_DestRegEither<0x66,OpcodeB+0x22> D;
	const SimdImpl_DestRegEither<0x66,OpcodeQ> Q;

	// Add/Sub packed signed byte [8bit] integers from src into dest, and saturate the results.
	const SimdImpl_DestRegEither<0x66,OpcodeB+0x10> SB;

	// Add/Sub packed signed word [16bit] integers from src into dest, and saturate the results.
	const SimdImpl_DestRegEither<0x66,OpcodeB+0x11> SW;

	// Add/Sub packed unsigned byte [8bit] integers from src into dest, and saturate the results.
	const SimdImpl_DestRegEither<0x66,OpcodeB> USB;

	// Add/Sub packed unsigned word [16bit] integers from src into dest, and saturate the results.
	const SimdImpl_DestRegEither<0x66,OpcodeB+1> USW;

	SimdImpl_AddSub() {}
};

//////////////////////////////////////////////////////////////////////////////////////////
//
class SimdImpl_PMul
{
public:
	const SimdImpl_DestRegEither<0x66,0xd5> LW;
	const SimdImpl_DestRegEither<0x66,0xe5> HW;
	const SimdImpl_DestRegEither<0x66,0xe4> HUW;
	const SimdImpl_DestRegEither<0x66,0xf4> UDQ;

	// [SSE-3] PMULHRSW multiplies vertically each signed 16-bit integer from dest with the
	// corresponding signed 16-bit integer of source, producing intermediate signed 32-bit
	// integers. Each intermediate 32-bit integer is truncated to the 18 most significant
	// bits. Rounding is always performed by adding 1 to the least significant bit of the
	// 18-bit intermediate result. The final result is obtained by selecting the 16 bits
	// immediately to the right of the most significant bit of each 18-bit intermediate
	// result and packed to the destination operand.
	//
	// Both operands can be MMX or XMM registers.  Source can be register or memory.
	//
	const SimdImpl_DestRegEither<0x66,0x0b38> HRSW;
	
	// [SSE-4.1] Multiply the packed dword signed integers in dest with src, and store
	// the low 32 bits of each product in xmm1.
	const SimdImpl_DestRegSSE<0x66,0x4038> LD;
	
	// [SSE-4.1] Multiply the packed signed dword integers in dest with src.
	const SimdImpl_DestRegSSE<0x66,0x2838> DQ;
	
	SimdImpl_PMul() {}
};

//////////////////////////////////////////////////////////////////////////////////////////
//
class SimdImpl_PCompare
{
public:
	SimdImpl_PCompare() {}
	
	// Compare packed bytes for equality.
	// If a data element in dest is equal to the corresponding date element src, the
	// corresponding data element in dest is set to all 1s; otherwise, it is set to all 0s.
	const SimdImpl_DestRegEither<0x66,0x74> EQB;

	// Compare packed words for equality.
	// If a data element in dest is equal to the corresponding date element src, the
	// corresponding data element in dest is set to all 1s; otherwise, it is set to all 0s.
	const SimdImpl_DestRegEither<0x66,0x75> EQW;

	// Compare packed doublewords [32-bits] for equality.
	// If a data element in dest is equal to the corresponding date element src, the
	// corresponding data element in dest is set to all 1s; otherwise, it is set to all 0s.
	const SimdImpl_DestRegEither<0x66,0x76> EQD;

	// Compare packed signed bytes for greater than.
	// If a data element in dest is greater than the corresponding date element src, the
	// corresponding data element in dest is set to all 1s; otherwise, it is set to all 0s.
	const SimdImpl_DestRegEither<0x66,0x64> GTB;

	// Compare packed signed words for greater than.
	// If a data element in dest is greater than the corresponding date element src, the
	// corresponding data element in dest is set to all 1s; otherwise, it is set to all 0s.
	const SimdImpl_DestRegEither<0x66,0x65> GTW;

	// Compare packed signed doublewords [32-bits] for greater than.
	// If a data element in dest is greater than the corresponding date element src, the
	// corresponding data element in dest is set to all 1s; otherwise, it is set to all 0s.
	const SimdImpl_DestRegEither<0x66,0x66> GTD;
};

//////////////////////////////////////////////////////////////////////////////////////////
// 
template< u8 Opcode1, u16 Opcode2 >
class SimdImpl_PMinMax
{
public:
	SimdImpl_PMinMax() {}
	
	// Compare packed unsigned byte integers in dest to src and store packed min/max
	// values in dest.
	// Operation can be performed on either MMX or SSE operands.
	const SimdImpl_DestRegEither<0x66,Opcode1> UB;

	// Compare packed signed word integers in dest to src and store packed min/max
	// values in dest.
	// Operation can be performed on either MMX or SSE operands.
	const SimdImpl_DestRegEither<0x66,Opcode1+0x10> SW;

	// [SSE-4.1] Compare packed signed byte integers in dest to src and store 
	// packed min/max values in dest. (SSE operands only)
	const SimdImpl_DestRegSSE<0x66,(Opcode2<<8)|0x38> SB;

	// [SSE-4.1] Compare packed signed doubleword integers in dest to src and store 
	// packed min/max values in dest. (SSE operands only)
	const SimdImpl_DestRegSSE<0x66,((Opcode2+1)<<8)|0x38> SD;

	// [SSE-4.1] Compare packed unsigned word integers in dest to src and store 
	// packed min/max values in dest. (SSE operands only)
	const SimdImpl_DestRegSSE<0x66,((Opcode2+2)<<8)|0x38> UW;

	// [SSE-4.1] Compare packed unsigned doubleword integers in dest to src and store 
	// packed min/max values in dest. (SSE operands only)
	const SimdImpl_DestRegSSE<0x66,((Opcode2+3)<<8)|0x38> UD;
};

//////////////////////////////////////////////////////////////////////////////////////////
//
class SimdImpl_PShuffle
{
public:
	SimdImpl_PShuffle() {}
	
	// Copies words from src and inserts them into dest at word locations selected with
	// the order operand (8 bit immediate).
	const SimdImpl_DestRegImmMMX<0x00,0x70> W;

	// Copies doublewords from src and inserts them into dest at dword locations selected
	// with the order operand (8 bit immediate).
	const SimdImpl_DestRegImmSSE<0x66,0x70> D;
	
	// Copies words from the low quadword of src and inserts them into the low quadword
	// of dest at word locations selected with the order operand (8 bit immediate).
	// The high quadword of src is copied to the high quadword of dest.
	const SimdImpl_DestRegImmSSE<0xf2,0x70> LW;

	// Copies words from the high quadword of src and inserts them into the high quadword
	// of dest at word locations selected with the order operand (8 bit immediate).
	// The low quadword of src is copied to the low quadword of dest.
	const SimdImpl_DestRegImmSSE<0xf3,0x70> HW;

	// [sSSE-3] Performs in-place shuffles of bytes in dest according to the shuffle
	// control mask in src.  If the most significant bit (bit[7]) of each byte of the
	// shuffle control mask is set, then constant zero is written in the result byte.
	// Each byte in the shuffle control mask forms an index to permute the corresponding
	// byte in dest. The value of each index is the least significant 4 bits (128-bit
	// operation) or 3 bits (64-bit operation) of the shuffle control byte.
	//
	// Operands can be MMX or XMM registers.
	const SimdImpl_DestRegEither<0x66,0x0038> B;
};

//////////////////////////////////////////////////////////////////////////////////////////
//
class SimdImpl_PUnpack
{
public:
	SimdImpl_PUnpack() {}
	
	// Unpack and interleave low-order bytes from src and dest into dest.
	const SimdImpl_DestRegEither<0x66,0x60> LBW;
	// Unpack and interleave low-order words from src and dest into dest.
	const SimdImpl_DestRegEither<0x66,0x61> LWD;
	// Unpack and interleave low-order doublewords from src and dest into dest.
	const SimdImpl_DestRegEither<0x66,0x62> LDQ;
	// Unpack and interleave low-order quadwords from src and dest into dest.
	const SimdImpl_DestRegSSE<0x66,0x6c> LQDQ;

	// Unpack and interleave high-order bytes from src and dest into dest.
	const SimdImpl_DestRegEither<0x66,0x68> HBW;
	// Unpack and interleave high-order words from src and dest into dest.
	const SimdImpl_DestRegEither<0x66,0x69> HWD;
	// Unpack and interleave high-order doublewords from src and dest into dest.
	const SimdImpl_DestRegEither<0x66,0x6a> HDQ;
	// Unpack and interleave high-order quadwords from src and dest into dest.
	const SimdImpl_DestRegSSE<0x66,0x6d> HQDQ;
};

//////////////////////////////////////////////////////////////////////////////////////////
// Pack with Signed or Unsigned Saturation
//
class SimdImpl_Pack
{
public:
	SimdImpl_Pack() {}

	// Converts packed signed word integers from src and dest into packed signed 
	// byte integers in dest, using signed saturation.
	const SimdImpl_DestRegEither<0x66,0x63> SSWB;

	// Converts packed signed dword integers from src and dest into packed signed 
	// word integers in dest, using signed saturation.
	const SimdImpl_DestRegEither<0x66,0x6b> SSDW;

	// Converts packed unsigned word integers from src and dest into packed unsigned 
	// byte integers in dest, using unsigned saturation.
	const SimdImpl_DestRegEither<0x66,0x67> USWB;

	// [SSE-4.1] Converts packed unsigned dword integers from src and dest into packed
	// unsigned word integers in dest, using signed saturation.
	const SimdImpl_DestRegSSE<0x66,0x2b38> USDW;
};


//////////////////////////////////////////////////////////////////////////////////////////
//
class SimdImpl_Unpack
{
public:
	SimdImpl_Unpack() {}

	// Unpacks the high doubleword [single-precision] values from src and dest into
	// dest, such that the result of dest looks like this:
	//    dest[0] <- dest[2]
	//    dest[1] <- src[2]
	//    dest[2] <- dest[3]
	//    dest[3] <- src[3]
	//
	const SimdImpl_DestRegSSE<0x00,0x15> HPS;

	// Unpacks the high quadword [double-precision] values from src and dest into
	// dest, such that the result of dest looks like this:
	//    dest.lo <- dest.hi
	//    dest.hi <- src.hi
	//
	const SimdImpl_DestRegSSE<0x66,0x15> HPD;

	// Unpacks the low doubleword [single-precision] values from src and dest into
	// dest, such that the result of dest looks like this:
	//    dest[3] <- src[1]
	//    dest[2] <- dest[1]
	//    dest[1] <- src[0]
	//    dest[0] <- dest[0]
	//
	const SimdImpl_DestRegSSE<0x00,0x14> LPS;

	// Unpacks the low quadword [double-precision] values from src and dest into
	// dest, effectively moving the low portion of src into the upper portion of dest.
	// The result of dest is loaded as such:
	//    dest.hi <- src.lo
	//    dest.lo <- dest.lo  [remains unchanged!]
	//
	const SimdImpl_DestRegSSE<0x66,0x14> LPD;
};

