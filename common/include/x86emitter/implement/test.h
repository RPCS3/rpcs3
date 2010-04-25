/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2009  PCSX2 Dev Team
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

// Implementations found here: TEST + BTS/BT/BTC/BTR + BSF/BSR! (for lack of better location)

namespace x86Emitter {

// --------------------------------------------------------------------------------------
//  xImpl_Test
// --------------------------------------------------------------------------------------
//
struct xImpl_Test
{
	void operator()( const xRegister8& to, const xRegister8& from ) const;
	void operator()( const xRegister16& to, const xRegister16& from ) const;
	void operator()( const xRegister32& to, const xRegister32& from ) const;
	void operator()( const ModSib32orLess& dest, int imm ) const;
	void operator()( const xRegisterInt& to, int imm ) const;
};

enum G8Type
{
	G8Type_BT = 4,
	G8Type_BTS,
	G8Type_BTR,
	G8Type_BTC,
};

// --------------------------------------------------------------------------------------
//  BSF / BSR
// --------------------------------------------------------------------------------------
// 16/32 operands are available.  No 8 bit ones, not that any of you cared, I bet.
//
struct xImpl_BitScan
{
	// 0xbc [fwd] / 0xbd [rev]
	u16		Opcode;

	void operator()( const xRegister32& to, const xRegister32& from ) const;
	void operator()( const xRegister16& to, const xRegister16& from ) const;
	void operator()( const xRegister16or32& to, const ModSibBase& sibsrc ) const;
};

// --------------------------------------------------------------------------------------
//  xImpl_Group8
// --------------------------------------------------------------------------------------
// Bit Test Instructions - Valid on 16/32 bit instructions only.
//
struct xImpl_Group8
{
	G8Type	InstType;

	void operator()( const xRegister32& bitbase, const xRegister32& bitoffset ) const;
	void operator()( const xRegister16& bitbase, const xRegister16& bitoffset ) const;
	void operator()( const xRegister16or32& bitbase, u8 bitoffset ) const;

	void operator()( const ModSibBase& bitbase, const xRegister16or32& bitoffset ) const;
	void operator()( const ModSib32& bitbase, u8 bitoffset ) const;
	void operator()( const ModSib16& bitbase, u8 bitoffset ) const;
};

}	// End namespace x86Emitter

