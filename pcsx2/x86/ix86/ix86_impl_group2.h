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
template< typename ImmType, G2Type InstType >
class Group2Impl
{
public: 
	static const uint OperandSize = sizeof(ImmType);

	Group2Impl() {}		// For the love of GCC.

protected:
	static bool Is8BitOperand()	{ return OperandSize == 1; }
	static void prefix16()		{ if( OperandSize == 2 ) iWrite<u8>( 0x66 ); }

public:
	static __emitinline void Emit( const iRegister<OperandSize>& to ) 
	{
		prefix16();
		iWrite<u8>( Is8BitOperand() ? 0xd2 : 0xd3 );
		ModRM( 3, InstType, to.Id );
	}

	static __emitinline void Emit( const iRegister<OperandSize>& to, u8 imm ) 
	{
		if( imm == 0 ) return;

		prefix16();
		if( imm == 1 )
		{
			// special encoding of 1's
			iWrite<u8>( Is8BitOperand() ? 0xd0 : 0xd1 );
			ModRM( 3, InstType, to.Id );
		}
		else
		{
			iWrite<u8>( Is8BitOperand() ? 0xc0 : 0xc1 );
			ModRM( 3, InstType, to.Id );
			iWrite<u8>( imm );
		}
	}

	static __emitinline void Emit( const ModSibStrict<OperandSize>& sibdest ) 
	{
		prefix16();
		iWrite<u8>( Is8BitOperand() ? 0xd2 : 0xd3 );
		EmitSibMagic( InstType, sibdest );
	}

	static __emitinline void Emit( const ModSibStrict<OperandSize>& sibdest, u8 imm ) 
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
protected:
	typedef Group2Impl<u32, InstType> m_32;
	typedef Group2Impl<u16, InstType> m_16;
	typedef Group2Impl<u8, InstType>  m_8;

	// Inlining Notes:
	//   I've set up the inlining to be as practical and intelligent as possible, which means
	//   forcing inlining for (void*) forms of ModRM, which thanks to constprop reduce to
	//   virtually no code.  In the case of (Reg, Imm) forms, the inlining is up to the dis-
	//   creation of the compiler.
	// 

	// (Note: I'm not going to macro this since it would likely clobber intellisense parameter resolution)

public:
	// ---------- 32 Bit Interface -----------
	__forceinline void operator()( const iRegister32& to,		__unused const iRegisterCL& from ) const{ m_32::Emit( to ); }
	__noinline void operator()( const ModSibStrict<4>& sibdest,	__unused const iRegisterCL& from ) const{ m_32::Emit( sibdest ); }
	__noinline void operator()( const ModSibStrict<4>& sibdest, u8 imm ) const					{ m_32::Emit( sibdest, imm ); }
	void operator()( const iRegister32& to, u8 imm ) const										{ m_32::Emit( to, imm ); }

	// ---------- 16 Bit Interface -----------
	__forceinline void operator()( const iRegister16& to,		__unused const iRegisterCL& from ) const{ m_16::Emit( to ); }
	__noinline void operator()( const ModSibStrict<2>& sibdest,	__unused const iRegisterCL& from ) const{ m_16::Emit( sibdest ); }
	__noinline void operator()( const ModSibStrict<2>& sibdest, u8 imm ) const					{ m_16::Emit( sibdest, imm ); }
	void operator()( const iRegister16& to, u8 imm ) const										{ m_16::Emit( to, imm ); }

	// ---------- 8 Bit Interface -----------
	__forceinline void operator()( const iRegister8& to,		__unused const iRegisterCL& from ) const{ m_8::Emit( to ); }
	__noinline void operator()( const ModSibStrict<1>& sibdest,	__unused const iRegisterCL& from ) const{ m_8::Emit( sibdest ); }
	__noinline void operator()( const ModSibStrict<1>& sibdest, u8 imm ) const					{ m_8::Emit( sibdest, imm ); }
	void operator()( const iRegister8& to, u8 imm ) const										{ m_8::Emit( to, imm ); }

	Group2ImplAll() {}		// I am a class with no members, so I need an explicit constructor!  Sense abounds.
};
