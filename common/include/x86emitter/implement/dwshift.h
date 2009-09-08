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

// Implementations here cover SHLD and SHRD.
// Note: This header is meant to be included from within the x86Emitter::Internal namespace.

//////////////////////////////////////////////////////////////////////////////////////////
// I use explicit method declarations here instead of templates, in order to provide
// *only* 32 and 16 bit register operand forms (8 bit registers are not valid in SHLD/SHRD).
//
// Optimization Note: Imm shifts by 0 are ignore (no code generated).  This is a safe optimization
// because shifts by 0 do *not* affect flags status.
//
template< bool isShiftRight >
class DwordShiftImplAll
{
	static const u8 m_shiftop = isShiftRight ? 0x8 : 0;

public:
	// ---------- 32 Bit Interface -----------
	__forceinline void operator()( const xRegister32& to,	const xRegister32& from, __unused const xRegisterCL& clreg ) const	{ xOpWrite0F( 0xa5 | m_shiftop, to, from ); }
	__forceinline void operator()( void* dest,				const xRegister32& from, __unused const xRegisterCL& clreg ) const	{ xOpWrite0F( 0xa5 | m_shiftop, from, dest ); }
	__forceinline void operator()( const ModSibBase& dest,	const xRegister32& from, __unused const xRegisterCL& clreg ) const	{ xOpWrite0F( 0xa5 | m_shiftop, from, dest ); }
	__forceinline void operator()( const xRegister32& to,	const xRegister32& from, u8 shiftcnt ) const						{ if( shiftcnt != 0 ) xOpWrite0F( 0xa4 | m_shiftop, to, from ); }
	__forceinline void operator()( void* dest,				const xRegister32& from, u8 shiftcnt ) const						{ if( shiftcnt != 0 ) xOpWrite0F( 0xa4 | m_shiftop, from, dest, shiftcnt ); }
	__forceinline void operator()( const ModSibBase& dest,	const xRegister32& from, u8 shiftcnt ) const						{ if( shiftcnt != 0 ) xOpWrite0F( 0xa4 | m_shiftop, from, dest, shiftcnt ); }

	// ---------- 16 Bit Interface -----------
	__forceinline void operator()( const xRegister16& to,	const xRegister16& from, __unused const xRegisterCL& clreg ) const	{ xOpWrite0F( 0x66, 0xa5 | m_shiftop, to, from ); }
	__forceinline void operator()( void* dest,				const xRegister16& from, __unused const xRegisterCL& clreg ) const	{ xOpWrite0F( 0x66, 0xa5 | m_shiftop, from, dest ); }
	__forceinline void operator()( const ModSibBase& dest,	const xRegister16& from, __unused const xRegisterCL& clreg ) const	{ xOpWrite0F( 0x66, 0xa5 | m_shiftop, from, dest ); }
	__forceinline void operator()( const xRegister16& to,	const xRegister16& from, u8 shiftcnt ) const						{ if( shiftcnt != 0 ) xOpWrite0F( 0x66, 0xa4 | m_shiftop, to, from ); }
	__forceinline void operator()( void* dest,				const xRegister16& from, u8 shiftcnt ) const						{ if( shiftcnt != 0 ) xOpWrite0F( 0x66, 0xa4 | m_shiftop, from, dest, shiftcnt ); }
	__forceinline void operator()( const ModSibBase& dest,	const xRegister16& from, u8 shiftcnt ) const						{ if( shiftcnt != 0 ) xOpWrite0F( 0x66, 0xa4 | m_shiftop, from, dest, shiftcnt ); }

	DwordShiftImplAll() {}		// Why does GCC need these?
};

