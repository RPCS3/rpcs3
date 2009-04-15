/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2009  Pcsx2 Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/*
 * ix86 core v0.9.0
 *
 * Original Authors (v0.6.2 and prior):
 *		linuzappz <linuzappz@pcsx.net>
 *		alexey silinov
 *		goldfinger
 *		zerofrog(@gmail.com)
 *
 * Authors of v0.9.0:
 *		Jake.Stine(@gmail.com)
 *		cottonvibes(@gmail.com)
 *		sudonim(1@gmail.com)
 */

#include "PrecompiledHeader.h"

#include "System.h"
#include "ix86_internal.h"

namespace x86Emitter {

// ------------------------------------------------------------------------
void iSmartJump::SetTarget()
{
	jASSUME( !m_written );
	if( m_written )
		throw Exception::InvalidOperation( "Attempted to set SmartJump label multiple times." );

	m_target = iGetPtr();
	if( m_baseptr == NULL ) return;

	iSetPtr( m_baseptr );
	u8* const saveme = m_baseptr + GetMaxInstructionSize();
	iJccKnownTarget( m_cc, m_target, true );

	// Copy recompiled data inward if the jump instruction didn't fill the
	// alloted buffer (means that we optimized things to a j8!)

	const int spacer = (sptr)saveme - (sptr)iGetPtr();
	if( spacer != 0 )
	{
		u8* destpos = iGetPtr();
		const int copylen = (sptr)m_target - (sptr)saveme;

		memcpy_fast( destpos, saveme, copylen );
		iSetPtr( m_target - spacer );
	}

	m_written = true;
}

//////////////////////////////////////////////////////////////////////////////////////////
//

// ------------------------------------------------------------------------
// Writes a jump at the current x86Ptr, which targets a pre-established target address.
// (usually a backwards jump)
//
// slideForward - used internally by iSmartJump to indicate that the jump target is going
// to slide forward in the event of an 8 bit displacement.
//
__emitinline void iJccKnownTarget( JccComparisonType comparison, void* target, bool slideForward )
{
	// Calculate the potential j8 displacement first, assuming an instruction length of 2:
	sptr displacement8 = (sptr)target - ((sptr)iGetPtr() + 2);

	const int slideVal = slideForward ? ((comparison == Jcc_Unconditional) ? 3 : 4) : 0;
	displacement8 -= slideVal;

	// if the following assert fails it means we accidentally used slideForard on a backward
	// jump (which is an invalid operation since there's nothing to slide forward).
	if( slideForward ) jASSUME( displacement8 >= 0 );
	
	if( is_s8( displacement8 ) )
	{
		iWrite<u8>( (comparison == Jcc_Unconditional) ? 0xeb : (0x70 | comparison) );
		iWrite<s8>( displacement8 );
	}
	else
	{
		// Perform a 32 bit jump instead. :(

		if( comparison == Jcc_Unconditional )
			iWrite<u8>( 0xe9 );
		else
		{
			iWrite<u8>( 0x0f );
			iWrite<u8>( 0x80 | comparison );
		}
		iWrite<s32>( (sptr)target - ((sptr)iGetPtr() + 4) );
	}
}

__emitinline void iJcc( JccComparisonType comparison, void* target )
{
	iJccKnownTarget( comparison, target );
}

}