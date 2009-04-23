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
		Woot() {}
		__forceinline void operator()( const xRegisterSSE& to, const void* from ) const			{ xOpWrite0F( Prefix, Opcode, to, from ); }
		__forceinline void operator()( const void* to, const xRegisterSSE& from ) const			{ xOpWrite0F( Prefix, Opcode+1, from, to ); }
		__forceinline void operator()( const xRegisterSSE& to, const ModSibBase& from ) const	{ xOpWrite0F( Prefix, Opcode, to, from ); }
		__forceinline void operator()( const ModSibBase& to, const xRegisterSSE& from ) const	{ xOpWrite0F( Prefix, Opcode+1, from, to ); }
	};

public:
	const Woot<0x00> PS;
	const Woot<0x66> PD;

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
	__forceinline void PS( const xRegisterSSE& to, const xRegisterSSE& from ) const			{ xOpWrite0F( Opcode, to, from ); }
	__forceinline void PD( const xRegisterSSE& to, const xRegisterSSE& from ) const			{ xOpWrite0F( 0x66, Opcode, to, from ); }

	MovhlImpl_RtoR() {} //GCC.
};

//////////////////////////////////////////////////////////////////////////////////////////
// Legends in their own right: MOVAPS / MOVAPD / MOVUPS / MOVUPD
//
// All implementations of Unaligned Movs will, when possible, use aligned movs instead.
// This happens when using Mem,Reg or Reg,Mem forms where the address is simple displacement
// which can be checked for alignment at runtime.
// 
template< u8 Prefix, bool isAligned >
class SimdImpl_MoveSSE
{
	static const u16 OpcodeA = 0x28;		// Aligned [aps] form
	static const u16 OpcodeU = 0x10;		// unaligned [ups] form

public:
	SimdImpl_MoveSSE() {} //GCC.

	__forceinline void operator()( const xRegisterSSE& to, const xRegisterSSE& from ) const
	{
		if( to != from ) xOpWrite0F( Prefix, OpcodeA, to, from );
	}

	__forceinline void operator()( const xRegisterSSE& to, const void* from ) const	
	{
		xOpWrite0F( Prefix, (isAligned || ((uptr)from & 0x0f) == 0) ? OpcodeA : OpcodeU, to, from );
	}

	__forceinline void operator()( void* to, const xRegisterSSE& from ) const
	{
		xOpWrite0F( Prefix, (isAligned || ((uptr)to & 0x0f) == 0) ? OpcodeA+1 : OpcodeU+1, from, to );
	}

	__forceinline void operator()( const xRegisterSSE& to, const ModSibBase& from ) const
	{
		// ModSib form is aligned if it's displacement-only and the displacement is aligned:
		bool isReallyAligned = isAligned || ( ((from.Displacement & 0x0f) == 0) && from.Index.IsEmpty() && from.Base.IsEmpty() );
		xOpWrite0F( Prefix, isReallyAligned ? OpcodeA : OpcodeU, to, from );
	}

	__forceinline void operator()( const ModSibBase& to, const xRegisterSSE& from ) const
	{
		// ModSib form is aligned if it's displacement-only and the displacement is aligned:
		bool isReallyAligned = isAligned || ( (to.Displacement & 0x0f) == 0 && to.Index.IsEmpty() && to.Base.IsEmpty() );
		xOpWrite0F( Prefix, isReallyAligned ? OpcodeA+1 : OpcodeU+1, from, to );
	}
};

//////////////////////////////////////////////////////////////////////////////////////////
// Implementations for MOVDQA / MOVDQU
//
template< u8 Prefix, bool isAligned >
class SimdImpl_MoveDQ
{
	static const u8 PrefixA = 0x66;		// Aligned [aps] form
	static const u8 PrefixU = 0xf3;		// unaligned [ups] form

	static const u16 Opcode = 0x6f;
	static const u16 Opcode_Alt = 0x7f; // alternate ModRM encoding (reverse src/dst)

public:
	SimdImpl_MoveDQ() {} //GCC.

	__forceinline void operator()( const xRegisterSSE& to, const xRegisterSSE& from ) const
	{
		if( to != from ) xOpWrite0F( PrefixA, Opcode, to, from );
	}
#ifndef __LINUX__ // Ifdef till Jake fixes; you can't use & on a const void*!
	__forceinline void operator()( const xRegisterSSE& to, const void* from ) const	
	{
		xOpWrite0F( (isAligned || (from & 0x0f) == 0) ? PrefixA : PrefixU, Opcode, to, from );
	}

	__forceinline void operator()( const void* to, const xRegisterSSE& from ) const
	{
		xOpWrite0F( (isAligned || (from & 0x0f) == 0) ? PrefixA : PrefixU, Opcode_Alt, to, from );
	}
#endif
	__forceinline void operator()( const xRegisterSSE& to, const ModSibBase& from ) const
	{
		// ModSib form is aligned if it's displacement-only and the displacement is aligned:
		bool isReallyAligned = isAligned || ( (from.Displacement & 0x0f) == 0 && from.Index.IsEmpty() && from.Base.IsEmpty() );
		xOpWrite0F( isReallyAligned ? PrefixA : PrefixU, Opcode, to, from );
	}

#ifndef __LINUX__ // II'll ifdef this one, too. xOpWrite0F doesn't take ModSibBase & xRegisterSSE in that order.
	__forceinline void operator()( const ModSibBase& to, const xRegisterSSE& from ) const
	{
		// ModSib form is aligned if it's displacement-only and the displacement is aligned:
		bool isReallyAligned = isAligned || ( (to.Displacement & 0x0f) == 0 && to.Index.IsEmpty() && to.Base.IsEmpty() );
		xOpWrite0F( isReallyAligned ? PrefixA : PrefixU, Opcode_Alt, to, from );
	}
#endif
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

//////////////////////////////////////////////////////////////////////////////////////////
// Blend - Conditional copying of values in src into dest.
//
class SimdImpl_Blend
{
public:
	// [SSE-4.1] Conditionally copies dword values from src to dest, depending on the
	// mask bits in the immediate operand (bits [3:0]).  Each mask bit corresponds to a
	// dword element in a 128-bit operand. 
	//
	// If a mask bit is 1, then the corresponding dword in the source operand is copied
	// to dest, else the dword element in dest is left unchanged.
	//
	SimdImpl_DestRegImmSSE<0x66,0x0c3a> PS;

	// [SSE-4.1] Conditionally copies quadword values from src to dest, depending on the
	// mask bits in the immediate operand (bits [1:0]).  Each mask bit corresponds to a
	// quadword element in a 128-bit operand. 
	//
	// If a mask bit is 1, then the corresponding dword in the source operand is copied
	// to dest, else the dword element in dest is left unchanged.
	//
	SimdImpl_DestRegImmSSE<0x66,0x0d3a> PD;
	
	// [SSE-4.1] Conditionally copies dword values from src to dest, depending on the
	// mask (bits [3:0]) in XMM0 (yes, the fixed register).  Each mask bit corresponds
	// to a dword element in the 128-bit operand. 
	//
	// If a mask bit is 1, then the corresponding dword in the source operand is copied
	// to dest, else the dword element in dest is left unchanged.
	//
	SimdImpl_DestRegSSE<0x66,0x1438> VPS;
	
	// [SSE-4.1] Conditionally copies quadword values from src to dest, depending on the
	// mask (bits [1:0]) in XMM0 (yes, the fixed register).  Each mask bit corresponds
	// to a quadword element in the 128-bit operand. 
	//
	// If a mask bit is 1, then the corresponding dword in the source operand is copied
	// to dest, else the dword element in dest is left unchanged.
	//
	SimdImpl_DestRegSSE<0x66,0x1538> VPD;
};

//////////////////////////////////////////////////////////////////////////////////////////
// Packed Move with Sign or Zero extension.
//
template< bool SignExtend >
class SimdImpl_PMove
{
	static const u16 OpcodeBase = SignExtend ? 0x2038 : 0x3038;

public:
	// [SSE-4.1] Zero/Sign-extend the low byte values in src into word integers
	// and store them in dest.
	SimdImpl_DestRegStrict<0x66,OpcodeBase,xRegisterSSE,xRegisterSSE,u64> BW;

	// [SSE-4.1] Zero/Sign-extend the low byte values in src into dword integers
	// and store them in dest.
	SimdImpl_DestRegStrict<0x66,OpcodeBase+0x100,xRegisterSSE,xRegisterSSE,u32> BD;

	// [SSE-4.1] Zero/Sign-extend the low byte values in src into qword integers
	// and store them in dest.
	SimdImpl_DestRegStrict<0x66,OpcodeBase+0x200,xRegisterSSE,xRegisterSSE,u16> BQ;
	
	// [SSE-4.1] Zero/Sign-extend the low word values in src into dword integers
	// and store them in dest.
	SimdImpl_DestRegStrict<0x66,OpcodeBase+0x300,xRegisterSSE,xRegisterSSE,u64> WD;

	// [SSE-4.1] Zero/Sign-extend the low word values in src into qword integers
	// and store them in dest.
	SimdImpl_DestRegStrict<0x66,OpcodeBase+0x400,xRegisterSSE,xRegisterSSE,u32> WQ;

	// [SSE-4.1] Zero/Sign-extend the low dword values in src into qword integers
	// and store them in dest.
	SimdImpl_DestRegStrict<0x66,OpcodeBase+0x500,xRegisterSSE,xRegisterSSE,u64> DQ;
};

