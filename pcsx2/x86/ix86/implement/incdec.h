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

#pragma once

// Implementations found here: Increment and Decrement Instructions!
// Note: This header is meant to be included from within the x86Emitter::Internal namespace.


template< bool isDec >
class xImpl_IncDec
{
public:
	template< typename T >
	__forceinline void operator()( const xRegister<T>& to )	const
	{
		if( Is8BitOp<T>() )
		{
			xWrite8( 0xfe );
			EmitSibMagic( isDec ? 1 : 0, to );
		}
		else
		{
			prefix16<T>();
			xWrite8( (isDec ? 0x48 : 0x40) | to.Id );
		}
	}

	template< typename T >
	__forceinline void operator()( const ModSibStrict<T>& sibdest ) const
	{
		prefix16<T>();
		xWrite8( Is8BitOp<T>() ? 0xfe : 0xff );
		EmitSibMagic( isDec ? 1 : 0, sibdest );
	}

	xImpl_IncDec() {}		// don't ask.
};
