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

enum G2Type
{
	G2Type_ROL=0,
	G2Type_ROR,
	G2Type_RCL,
	G2Type_RCR,
	G2Type_SHL,
	G2Type_SHR,
	G2Type_Unused,
	G2Type_SAR
};

// --------------------------------------------------------------------------------------
//  xImpl_Group2
// --------------------------------------------------------------------------------------
// Group 2 (shift) instructions have no Sib/ModRM forms.
// Optimization Note: For Imm forms, we ignore the instruction if the shift count is zero.
// This is a safe optimization since any zero-value shift does not affect any flags.
//
struct xImpl_Group2
{
	G2Type InstType;

	void operator()( const xRegisterInt& to, const xRegisterCL& from ) const;
	void operator()( const ModSib32orLess& to, const xRegisterCL& from ) const;
	void operator()( const xRegisterInt& to, u8 imm ) const;
	void operator()( const ModSib32orLess& to, u8 imm ) const;

#if 0
	// ------------------------------------------------------------------------
	template< typename T > __noinline void operator()( const xDirectOrIndirect<T>& to, u8 imm ) const
	{
		_DoI_helpermess( *this, to, imm );
	}

	template< typename T > __noinline void operator()( const xDirectOrIndirect<T>& to, const xRegisterCL& from ) const
	{
		_DoI_helpermess( *this, to, from );
	}
#endif
};

} // End namespace x86Emitter
