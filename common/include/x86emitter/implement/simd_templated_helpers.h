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

//////////////////////////////////////////////////////////////////////////////////////////
// MMX / SSE Helper Functions!

// ------------------------------------------------------------------------
// For implementing SSE-only logic operations that have xmmreg,xmmreg/rm forms only,
// like ANDPS/ANDPD
//
template< u8 Prefix, u16 Opcode >
class SimdImpl_DestRegSSE
{
public:
	__forceinline void operator()( const xRegisterSSE& to, const xRegisterSSE& from ) const	{ xOpWrite0F( Prefix, Opcode, to, from ); }
	__forceinline void operator()( const xRegisterSSE& to, const ModSibBase& from ) const
	{
		bool isReallyAligned = ((from.Displacement & 0x0f) == 0) && from.Index.IsEmpty() && from.Base.IsEmpty();
		pxAssertDev( isReallyAligned, "Alignment check failed on SSE indirect load." );
		xOpWrite0F( Prefix, Opcode, to, from );
	}

	SimdImpl_DestRegSSE() {} //GCWho?
};
