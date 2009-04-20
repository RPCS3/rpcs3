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

// Implementations found here: CALL and JMP!  (unconditional only)
// Note: This header is meant to be included from within the x86Emitter::Internal namespace.

template< typename ImmType >
class JmpCallImpl
{
protected:
	static const uint OperandSize = sizeof(ImmType);

	static void prefix16()		{ if( OperandSize == 2 ) xWrite<u8>( 0x66 ); }

public: 
	JmpCallImpl() {}		// For the love of GCC.

	static __emitinline void Emit( bool isJmp, const xRegister<ImmType>& absreg )
	{
		prefix16();
		xWrite<u8>( 0xff );
		ModRM_Direct( isJmp ? 4 : 2, absreg.Id );
	}

	static __emitinline void Emit( bool isJmp, const ModSibStrict<ImmType>& src )
	{
		prefix16();
		xWrite<u8>( 0xff );
		EmitSibMagic( isJmp ? 4 : 2, src );
	}
};

// ------------------------------------------------------------------------
template< bool isJmp >
class JmpCallImplAll
{
protected:
	typedef JmpCallImpl<u32> m_32;
	typedef JmpCallImpl<u16> m_16;
	
public:
	JmpCallImplAll() {}

	__forceinline void operator()( const xRegister32& absreg ) const	{ m_32::Emit( isJmp, absreg ); }
	__forceinline void operator()( const ModSibStrict<u32>& src ) const	{ m_32::Emit( isJmp, src ); }

	__forceinline void operator()( const xRegister16& absreg ) const	{ m_16::Emit( isJmp, absreg ); }
	__forceinline void operator()( const ModSibStrict<u16>& src ) const	{ m_16::Emit( isJmp, src ); }
	
	// Special form for calling functions.  This form automatically resolves the
	// correct displacement based on the size of the instruction being generated.
	template< typename T > 
	__forceinline void operator()( const T* func ) const
	{
		if( isJmp )
			iJccKnownTarget( Jcc_Unconditional, (void*)(uptr)func );
		else
		{
			// calls are relative to the instruction after this one, and length is
			// always 5 bytes (16 bit calls are bad mojo, so no bother to do special logic).
			
			sptr dest = (sptr)func - ((sptr)iGetPtr() + 5);
			xWrite<u8>( 0xe8 );
			xWrite<u32>( dest );
		}
	}

};
