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

enum G3Type
{
	G3Type_NOT	= 2,
	G3Type_NEG	= 3,
	G3Type_MUL	= 4,
	G3Type_iMUL	= 5,		// partial implementation, iMul has additional forms in ix86.cpp
	G3Type_DIV	= 6,
	G3Type_iDIV	= 7
};

// --------------------------------------------------------------------------------------
//  xImpl_Group3
// --------------------------------------------------------------------------------------
struct xImpl_Group3
{
	G3Type	InstType;

	void operator()( const xRegisterInt& from ) const;
	void operator()( const ModSib32orLess& from ) const;

#if 0
	template< typename T >
	void operator()( const xDirectOrIndirect<T>& from ) const
	{
		_DoI_helpermess( *this, from );
	}
#endif
};

// --------------------------------------------------------------------------------------
//  xImpl_MulDivBase
// --------------------------------------------------------------------------------------
// This class combines x86 and SSE/SSE2 instructions for iMUL and iDIV.
//
struct xImpl_MulDivBase
{
	G3Type	InstType;
	u16		OpcodeSSE;

	void operator()( const xRegisterInt& from ) const;
	void operator()( const ModSib32orLess& from ) const;

	const xImplSimd_DestRegSSE	PS;
	const xImplSimd_DestRegSSE	PD;
	const xImplSimd_DestRegSSE	SS;
	const xImplSimd_DestRegSSE	SD;
};

// --------------------------------------------------------------------------------------
//  xImpl_iDiv
// --------------------------------------------------------------------------------------
struct xImpl_iDiv
{
	void operator()( const xRegisterInt& from ) const;
	void operator()( const ModSib32orLess& from ) const;

	const xImplSimd_DestRegSSE	PS;
	const xImplSimd_DestRegSSE	PD;
	const xImplSimd_DestRegSSE	SS;
	const xImplSimd_DestRegSSE	SD;
};

// --------------------------------------------------------------------------------------
//  xImpl_iMul
// --------------------------------------------------------------------------------------
//
struct xImpl_iMul
{
	void operator()( const xRegisterInt& from ) const;
	void operator()( const ModSib32orLess& from ) const;

	// The following iMul-specific forms are valid for 16 and 32 bit register operands only!

	void operator()( const xRegister32& to,	const xRegister32& from ) const;
	void operator()( const xRegister32& to,	const ModSibBase& src ) const;
	void operator()( const xRegister16& to,	const xRegister16& from ) const;
	void operator()( const xRegister16& to,	const ModSibBase& src ) const;

	void operator()( const xRegister32& to,	const xRegister32& from, s32 imm ) const;
	void operator()( const xRegister32& to,	const ModSibBase& from, s32 imm ) const;
	void operator()( const xRegister16& to,	const xRegister16& from, s16 imm ) const;
	void operator()( const xRegister16& to,	const ModSibBase& from, s16 imm ) const;

	const xImplSimd_DestRegSSE	PS;
	const xImplSimd_DestRegSSE	PD;
	const xImplSimd_DestRegSSE	SS;
	const xImplSimd_DestRegSSE	SD;
};

}
