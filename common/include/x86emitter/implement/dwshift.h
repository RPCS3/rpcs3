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

namespace x86Emitter {

// Implementations here cover SHLD and SHRD.

// --------------------------------------------------------------------------------------
//  xImpl_DowrdShift
// --------------------------------------------------------------------------------------
// I use explicit method declarations here instead of templates, in order to provide
// *only* 32 and 16 bit register operand forms (8 bit registers are not valid in SHLD/SHRD).
//
// Optimization Note: Imm shifts by 0 are ignore (no code generated).  This is a safe optimization
// because shifts by 0 do *not* affect flags status (intel docs cited).
//
struct xImpl_DwordShift
{
	u16		OpcodeBase;

	void operator()( const xRegister32& to,	const xRegister32& from, const xRegisterCL& clreg ) const;
	void operator()( const xRegister16& to,	const xRegister16& from, const xRegisterCL& clreg ) const;
	void operator()( const xRegister32& to,	const xRegister32& from, u8 shiftcnt ) const;
	void operator()( const xRegister16& to,	const xRegister16& from, u8 shiftcnt ) const;

	void operator()( const ModSibBase& dest,const xRegister16or32& from, const xRegisterCL& clreg ) const;
	void operator()( const ModSibBase& dest,const xRegister16or32& from, u8 shiftcnt ) const;
};

}	// End namespace x86Emitter
