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

// Implementations found here: CALL and JMP!  (unconditional only)

namespace x86Emitter {

#ifdef __GNUG__
	// GCC has a bug that causes the templated function handler for Jmp/Call emitters to generate
	// bad asm code.  (error is something like "7#*_uber_379s_mangled_$&02_name is already defined!")
	// Using GCC's always_inline attribute fixes it.  This differs from __forceinline in that it
	// inlines *even in debug builds* which is (usually) undesirable.
	//  ... except when it avoids compiler bugs.
#	define __always_inline_tmpl_fail	__attribute__((always_inline))
#else
#	define __always_inline_tmpl_fail
#endif

extern void xJccKnownTarget( JccComparisonType comparison, const void* target, bool slideForward );

// ------------------------------------------------------------------------
struct xImpl_JmpCall
{
	bool	isJmp;

	void operator()( const xRegister32& absreg ) const;
	void operator()( const ModSib32& src ) const;

	void operator()( const xRegister16& absreg ) const;
	void operator()( const ModSib16& src ) const;

	// Special form for calling functions.  This form automatically resolves the
	// correct displacement based on the size of the instruction being generated.
	template< typename T > __forceinline __always_inline_tmpl_fail
	void operator()( T* func ) const
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

}	// End namespace x86Emitter

