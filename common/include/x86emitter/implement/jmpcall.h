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

// Implementations found here: CALL and JMP!  (unconditional only)
// Note: This header is meant to be included from within the x86Emitter::Internal namespace.

// ------------------------------------------------------------------------
template< bool isJmp >
class xImpl_JmpCall
{
public:
	xImpl_JmpCall() {}

	__forceinline void operator()( const xRegister32& absreg ) const	{ xOpWrite( 0x00, 0xff, isJmp ? 4 : 2, absreg ); }
	__forceinline void operator()( const ModSibStrict<u32>& src ) const	{ xOpWrite( 0x00, 0xff, isJmp ? 4 : 2, src ); }

	__forceinline void operator()( const xRegister16& absreg ) const	{ xOpWrite( 0x66, 0xff, isJmp ? 4 : 2, absreg ); }
	__forceinline void operator()( const ModSibStrict<u16>& src ) const	{ xOpWrite( 0x66, 0xff, isJmp ? 4 : 2, src ); }
	
	// Special form for calling functions.  This form automatically resolves the
	// correct displacement based on the size of the instruction being generated.
	template< typename T > 
	__forceinline void operator()( T* func ) const
	{
		if( isJmp )
			xJccKnownTarget( Jcc_Unconditional, (void*)(uptr)func, false );	// double cast to/from (uptr) needed to appease GCC
		else
		{
			// calls are relative to the instruction after this one, and length is
			// always 5 bytes (16 bit calls are bad mojo, so no bother to do special logic).
			
			sptr dest = (sptr)func - ((sptr)xGetPtr() + 5);
			xWrite8( 0xe8 );
			xWrite32( dest );
		}
	}
};

