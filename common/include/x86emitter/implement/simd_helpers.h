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

// =====================================================================================================
//  xImpl_SIMD Types (template free!)
// =====================================================================================================

// ------------------------------------------------------------------------
// For implementing SSE-only logic operations that have xmmreg,xmmreg/rm forms only,
// like ANDPS/ANDPD
//
struct xImplSimd_DestRegSSE
{
	u8		Prefix;
	u16		Opcode;
	
	void operator()( const xRegisterSSE& to, const xRegisterSSE& from ) const;
	void operator()( const xRegisterSSE& to, const ModSibBase& from ) const;
};

// ------------------------------------------------------------------------
// For implementing SSE-only logic operations that have xmmreg,reg/rm,imm forms only
// (PSHUFD / PSHUFHW / etc).
//
struct xImplSimd_DestRegImmSSE
{
	u8		Prefix;
	u16		Opcode;

	void operator()( const xRegisterSSE& to, const xRegisterSSE& from, u8 imm ) const;
	void operator()( const xRegisterSSE& to, const ModSibBase& from, u8 imm ) const;
};

struct xImplSimd_DestRegImmMMX
{
	u8		Prefix;
	u16		Opcode;

	void operator()( const xRegisterMMX& to, const xRegisterMMX& from, u8 imm ) const;
	void operator()( const xRegisterMMX& to, const ModSibBase& from, u8 imm ) const;
};

// ------------------------------------------------------------------------
// For implementing MMX/SSE operations that have reg,reg/rm forms only,
// but accept either MM or XMM destinations (most PADD/PSUB and other P arithmetic ops).
//
struct xImplSimd_DestRegEither
{
	u8		Prefix;
	u16		Opcode;

	void operator()( const xRegisterSSE& to, const xRegisterSSE& from ) const;
	void operator()( const xRegisterSSE& to, const ModSibBase& from ) const;

	void operator()( const xRegisterMMX& to, const xRegisterMMX& from ) const;
	void operator()( const xRegisterMMX& to, const ModSibBase& from ) const;
};

}	// end namespace x86Emitter

