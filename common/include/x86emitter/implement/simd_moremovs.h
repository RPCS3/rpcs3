/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2010  PCSX2 Dev Team
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

namespace x86Emitter {

// --------------------------------------------------------------------------------------
//  xImplSimd_MovHL
// --------------------------------------------------------------------------------------
// Moves to/from high/low portions of an xmm register.
// These instructions cannot be used in reg/reg form.
//
struct xImplSimd_MovHL
{
	u16	Opcode;

	void PS( const xRegisterSSE& to, const xIndirectVoid& from ) const;
	void PS( const xIndirectVoid& to, const xRegisterSSE& from ) const;

	void PD( const xRegisterSSE& to, const xIndirectVoid& from ) const;
	void PD( const xIndirectVoid& to, const xRegisterSSE& from ) const;
};

// --------------------------------------------------------------------------------------
//  xImplSimd_MovHL_RtoR
// --------------------------------------------------------------------------------------
// RegtoReg forms of MOVHL/MOVLH -- these are the same opcodes as MOVH/MOVL but
// do something kinda different! Fun!
//
struct xImplSimd_MovHL_RtoR
{
	u16	Opcode;

	void PS( const xRegisterSSE& to, const xRegisterSSE& from ) const;
	void PD( const xRegisterSSE& to, const xRegisterSSE& from ) const;
};

// --------------------------------------------------------------------------------------
//  xImplSimd_MoveSSE
// --------------------------------------------------------------------------------------
// Legends in their own right: MOVAPS / MOVAPD / MOVUPS / MOVUPD
//
// All implementations of Unaligned Movs will, when possible, use aligned movs instead.
// This happens when using Mem,Reg or Reg,Mem forms where the address is simple displacement
// which can be checked for alignment at runtime.
//
struct xImplSimd_MoveSSE
{
	u8		Prefix;
	bool	isAligned;

	void operator()( const xRegisterSSE& to, const xRegisterSSE& from ) const;
	void operator()( const xRegisterSSE& to, const xIndirectVoid& from ) const;
	void operator()( const xIndirectVoid& to, const xRegisterSSE& from ) const;
};

// --------------------------------------------------------------------------------------
//  xImplSimd_MoveDQ
// --------------------------------------------------------------------------------------
// Implementations for MOVDQA / MOVDQU
//
// All implementations of Unaligned Movs will, when possible, use aligned movs instead.
// This happens when using Mem,Reg or Reg,Mem forms where the address is simple displacement
// which can be checked for alignment at runtime.

struct xImplSimd_MoveDQ
{
	u8		Prefix;
	bool	isAligned;

	void operator()( const xRegisterSSE& to, const xRegisterSSE& from ) const;
	void operator()( const xRegisterSSE& to, const xIndirectVoid& from ) const;
	void operator()( const xIndirectVoid& to, const xRegisterSSE& from ) const;
};

// --------------------------------------------------------------------------------------
//  xImplSimd_Blend
// --------------------------------------------------------------------------------------
// Blend - Conditional copying of values in src into dest.
//
struct xImplSimd_Blend
{
	// [SSE-4.1] Conditionally copies dword values from src to dest, depending on the
	// mask bits in the immediate operand (bits [3:0]).  Each mask bit corresponds to a
	// dword element in a 128-bit operand.
	//
	// If a mask bit is 1, then the corresponding dword in the source operand is copied
	// to dest, else the dword element in dest is left unchanged.
	//
	xImplSimd_DestRegImmSSE	PS;

	// [SSE-4.1] Conditionally copies quadword values from src to dest, depending on the
	// mask bits in the immediate operand (bits [1:0]).  Each mask bit corresponds to a
	// quadword element in a 128-bit operand.
	//
	// If a mask bit is 1, then the corresponding dword in the source operand is copied
	// to dest, else the dword element in dest is left unchanged.
	//
	xImplSimd_DestRegImmSSE	PD;

	// [SSE-4.1] Conditionally copies dword values from src to dest, depending on the
	// mask (bits [3:0]) in XMM0 (yes, the fixed register).  Each mask bit corresponds
	// to a dword element in the 128-bit operand.
	//
	// If a mask bit is 1, then the corresponding dword in the source operand is copied
	// to dest, else the dword element in dest is left unchanged.
	//
	xImplSimd_DestRegSSE	VPS;

	// [SSE-4.1] Conditionally copies quadword values from src to dest, depending on the
	// mask (bits [1:0]) in XMM0 (yes, the fixed register).  Each mask bit corresponds
	// to a quadword element in the 128-bit operand.
	//
	// If a mask bit is 1, then the corresponding dword in the source operand is copied
	// to dest, else the dword element in dest is left unchanged.
	//
	xImplSimd_DestRegSSE	VPD;
};

// --------------------------------------------------------------------------------------
//  xImplSimd_PMove
// --------------------------------------------------------------------------------------
// Packed Move with Sign or Zero extension.
//
struct xImplSimd_PMove
{
	u16		OpcodeBase;

	// [SSE-4.1] Zero/Sign-extend the low byte values in src into word integers
	// and store them in dest.
	void BW( const xRegisterSSE& to, const xRegisterSSE& from ) const;
	void BW( const xRegisterSSE& to, const xIndirect64& from ) const;

	// [SSE-4.1] Zero/Sign-extend the low byte values in src into dword integers
	// and store them in dest.
	void BD( const xRegisterSSE& to, const xRegisterSSE& from ) const;
	void BD( const xRegisterSSE& to, const xIndirect32& from ) const;

	// [SSE-4.1] Zero/Sign-extend the low byte values in src into qword integers
	// and store them in dest.
	void BQ( const xRegisterSSE& to, const xRegisterSSE& from ) const;
	void BQ( const xRegisterSSE& to, const xIndirect16& from ) const;

	// [SSE-4.1] Zero/Sign-extend the low word values in src into dword integers
	// and store them in dest.
	void WD( const xRegisterSSE& to, const xRegisterSSE& from ) const;
	void WD( const xRegisterSSE& to, const xIndirect64& from ) const;

	// [SSE-4.1] Zero/Sign-extend the low word values in src into qword integers
	// and store them in dest.
	void WQ( const xRegisterSSE& to, const xRegisterSSE& from ) const;
	void WQ( const xRegisterSSE& to, const xIndirect32& from ) const;

	// [SSE-4.1] Zero/Sign-extend the low dword values in src into qword integers
	// and store them in dest.
	void DQ( const xRegisterSSE& to, const xRegisterSSE& from ) const;
	void DQ( const xRegisterSSE& to, const xIndirect64& from ) const;
};

}

