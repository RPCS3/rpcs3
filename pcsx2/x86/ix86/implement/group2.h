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

// Note: This header is meant to be included from within the x86Emitter::Internal namespace.
// Instructions implemented in this header are as follows -->>

enum G2Type
{
	G2Type_ROL=0,
	G2Type_ROR,
	G2Type_RCL,
	G2Type_RCR,
	G2Type_SHL,
	G2Type_SHR,
	G2Type_Unused,
	G2Type_SAR
};

// -------------------------------------------------------------------
// Group 2 (shift) instructions have no Sib/ModRM forms.
// Optimization Note: For Imm forms, we ignore the instruction if the shift count is zero.
// This is a safe optimization since any zero-value shift does not affect any flags.
//
template< G2Type InstType, typename ImmType >
class Group2Impl
{
protected:
	static const uint OperandSize = sizeof(ImmType);

	static bool Is8BitOperand()	{ return OperandSize == 1; }
	static void prefix16()		{ if( OperandSize == 2 ) iWrite<u8>( 0x66 ); }

public: 
	Group2Impl() {}		// For the love of GCC.

	static __emitinline void Emit( const iRegister<ImmType>& to ) 
	{
		prefix16();
		iWrite<u8>( Is8BitOperand() ? 0xd2 : 0xd3 );
		ModRM_Direct( InstType, to.Id );
	}

	static __emitinline void Emit( const iRegister<ImmType>& to, u8 imm ) 
	{
		if( imm == 0 ) return;

		prefix16();
		if( imm == 1 )
		{
			// special encoding of 1's
			iWrite<u8>( Is8BitOperand() ? 0xd0 : 0xd1 );
			ModRM_Direct( InstType, to.Id );
		}
		else
		{
			iWrite<u8>( Is8BitOperand() ? 0xc0 : 0xc1 );
			ModRM_Direct( InstType, to.Id );
			iWrite<u8>( imm );
		}
	}

	static __emitinline void Emit( const ModSibStrict<ImmType>& sibdest ) 
	{
		prefix16();
		iWrite<u8>( Is8BitOperand() ? 0xd2 : 0xd3 );
		EmitSibMagic( InstType, sibdest );
	}

	static __emitinline void Emit( const ModSibStrict<ImmType>& sibdest, u8 imm ) 
	{
		if( imm == 0 ) return;

		prefix16();
		if( imm == 1 )
		{
			// special encoding of 1's
			iWrite<u8>( Is8BitOperand() ? 0xd0 : 0xd1 );
			EmitSibMagic( InstType, sibdest );
		}
		else
		{
			iWrite<u8>( Is8BitOperand() ? 0xc0 : 0xc1 );
			EmitSibMagic( InstType, sibdest );
			iWrite<u8>( imm );
		}
	}
};

// -------------------------------------------------------------------
//
template< G2Type InstType >
class Group2ImplAll
{
	// Inlining Notes:
	//   I've set up the inlining to be as practical and intelligent as possible, which means
	//   forcing inlining for (void*) forms of ModRM, which thanks to constprop reduce to
	//   virtually no code.  In the case of (Reg, Imm) forms, the inlining is up to the dis-
	//   creation of the compiler.
	// 

	// (Note: I'm not going to macro this since it would likely clobber intellisense parameter resolution)

public:
	// ---------- 32 Bit Interface -----------
	template< typename T > __forceinline void operator()( const iRegister<T>& to,		__unused const iRegisterCL& from ) const
	{ Group2Impl<InstType,T>::Emit( to ); }

	template< typename T > __noinline void operator()( const ModSibStrict<T>& sibdest,	__unused const iRegisterCL& from ) const
	{ Group2Impl<InstType,T>::Emit( sibdest ); }

	template< typename T > __noinline void operator()( const ModSibStrict<T>& sibdest, u8 imm ) const
	{ Group2Impl<InstType,T>::Emit( sibdest, imm ); }

	template< typename T > void operator()( const iRegister<T>& to, u8 imm ) const
	{ Group2Impl<InstType,T>::Emit( to, imm ); }

	Group2ImplAll() {}		// I am a class with no members, so I need an explicit constructor!  Sense abounds.
};
