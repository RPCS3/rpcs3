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

template< typename ImmType >
class IncDecImpl
{
protected:
	static const uint OperandSize = sizeof(ImmType);

	static bool Is8BitOperand()	{ return OperandSize == 1; }
	static void prefix16()		{ if( OperandSize == 2 ) iWrite<u8>( 0x66 ); }

public: 
	IncDecImpl() {}		// For the love of GCC.

	static __emitinline void Emit( bool isDec, const iRegister<ImmType>& to )
	{
		// There is no valid 8-bit form of direct register inc/dec, so fall
		// back on Mod/RM format instead:
		if (Is8BitOperand() )
		{
			write8( 0xfe );
			ModRM_Direct( isDec ? 1 : 0, to.Id );
		}
		else
		{
			prefix16();
			write8( (isDec ? 0x48 : 0x40) | to.Id );
		}
	}

	static __emitinline void Emit( bool isDec, const ModSibStrict<ImmType>& dest )
	{
		prefix16();
		write8( Is8BitOperand() ? 0xfe : 0xff );
		EmitSibMagic( isDec ? 1 : 0, dest );
	}
};

// ------------------------------------------------------------------------
template< bool isDec >
class IncDecImplAll
{
protected:
	typedef IncDecImpl<u32> m_32;
	typedef IncDecImpl<u16> m_16;
	typedef IncDecImpl<u8> m_8;

public:
	__forceinline void operator()( const iRegister32& to )	const		{ m_32::Emit( isDec, to ); }
	__noinline void operator()( const ModSibStrict<u32>& sibdest ) const{ m_32::Emit( isDec, sibdest ); }

	__forceinline void operator()( const iRegister16& to )	const		{ m_16::Emit( isDec, to ); }
	__noinline void operator()( const ModSibStrict<u16>& sibdest ) const{ m_16::Emit( isDec, sibdest ); }

	__forceinline void operator()( const iRegister8& to )	const		{ m_8::Emit( isDec, to ); }
	__noinline void operator()( const ModSibStrict<u8>& sibdest ) const	{ m_8::Emit( isDec, sibdest ); }

	IncDecImplAll() {}		// don't ask.
};
