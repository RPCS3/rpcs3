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

// Implementations here cover SHLD and SHRD.
// Note: This header is meant to be included from within the x86Emitter::Internal namespace.


// -------------------------------------------------------------------
// Optimization Note: Imm shifts by 0 are ignore (no code generated).  This is a safe optimization
// because shifts by 0 do *not* affect flags status.

template< typename ImmType, bool isShiftRight >
class DwordShiftImpl
{
protected:
	static const uint OperandSize = sizeof(ImmType);

	static bool Is8BitOperand()	{ return OperandSize == 1; }
	static void prefix16()		{ if( OperandSize == 2 ) xWrite<u8>( 0x66 ); }

	static void basesibform( bool isCL )
	{
		prefix16();
		write8( 0x0f );
		write8( (isCL ? 0xa5 : 0xa4) | (isShiftRight ? 0x8 : 0) );
	}
	
public: 
	DwordShiftImpl() {}		// because GCC doesn't like static classes

	// ------------------------------------------------------------------------
	static __emitinline void Emit( const xRegister<ImmType>& to, const xRegister<ImmType>& from ) 
	{
		prefix16();
		write16( 0xa50f | (isShiftRight ? 0x800 : 0) );
		ModRM_Direct( from.Id, to.Id );
	}

	// ------------------------------------------------------------------------
	static __emitinline void Emit( const xRegister<ImmType>& to, const xRegister<ImmType>& from, u8 imm ) 
	{
		if( imm == 0 ) return;
		prefix16();
		write16( 0xa40f | (isShiftRight ? 0x800 : 0) );
		ModRM_Direct( from.Id, to.Id );
		write8( imm );
	}

	// ------------------------------------------------------------------------
	static __emitinline void Emit( const ModSibBase& sibdest, const xRegister<ImmType>& from, __unused const xRegisterCL& clreg ) 
	{
		basesibform();
		EmitSibMagic( from.Id, sibdest );
	}

	// ------------------------------------------------------------------------
	static __emitinline void Emit( const ModSibBase& sibdest, const xRegister<ImmType>& from, u8 imm ) 
	{
		basesibform();
		EmitSibMagic( from.Id, sibdest );
		write8( imm );
	}

	// ------------------------------------------------------------------------
	// dest data type is inferred from the 'from' register, so we can do void* resolution :)
	static __emitinline void Emit( void* dest, const xRegister<ImmType>& from, __unused const xRegisterCL& clreg ) 
	{
		basesibform();
		xWriteDisp( from.Id, dest );
	}

	// ------------------------------------------------------------------------
	// dest data type is inferred from the 'from' register, so we can do void* resolution :)
	static __emitinline void Emit( void* dest, const xRegister<ImmType>& from, u8 imm ) 
	{
		basesibform();
		xWriteDisp( from.Id, dest );
		write8( imm );
	}
};


// -------------------------------------------------------------------
// I use explicit method declarations here instead of templates, in order to provide
// *only* 32 and 16 bit register operand forms (8 bit registers are not valid in SHLD/SHRD).
//
template< bool isShiftRight >
class DwordShiftImplAll
{
protected:
	typedef DwordShiftImpl<u32, isShiftRight> m_32;
	typedef DwordShiftImpl<u16, isShiftRight> m_16;

public:
	// ---------- 32 Bit Interface -----------
	__forceinline void operator()( const xRegister32& to,	const xRegister32& from, __unused const xRegisterCL& clreg ) const { m_32::Emit( to, from ); }
	__forceinline void operator()( void* dest,				const xRegister32& from, __unused const xRegisterCL& clreg ) const { m_32::Emit( dest, from ); }
	__noinline void operator()( const ModSibBase& sibdest,	const xRegister32& from, __unused const xRegisterCL& clreg ) const { m_32::Emit( sibdest, from ); }
	__forceinline void operator()( const xRegister32& to,	const xRegister32& from, u8 imm ) const	{ m_32::Emit( to, from, imm ); }
	__forceinline void operator()( void* dest,				const xRegister32& from, u8 imm ) const	{ m_32::Emit( dest, from, imm ); }
	__noinline void operator()( const ModSibBase& sibdest,	const xRegister32& from, u8 imm ) const	{ m_32::Emit( sibdest, from ); }

	// ---------- 16 Bit Interface -----------
	__forceinline void operator()( const xRegister16& to,	const xRegister16& from, __unused const xRegisterCL& clreg ) const { m_16::Emit( to, from ); }
	__forceinline void operator()( void* dest,				const xRegister16& from, __unused const xRegisterCL& clreg ) const { m_16::Emit( dest, from ); }
	__noinline void operator()( const ModSibBase& sibdest,	const xRegister16& from, __unused const xRegisterCL& clreg ) const { m_16::Emit( sibdest, from ); }
	__forceinline void operator()( const xRegister16& to,	const xRegister16& from, u8 imm ) const	{ m_16::Emit( to, from, imm ); }
	__forceinline void operator()( void* dest,				const xRegister16& from, u8 imm ) const	{ m_16::Emit( dest, from, imm ); }
	__noinline void operator()( const ModSibBase& sibdest,	const xRegister16& from, u8 imm ) const	{ m_16::Emit( sibdest, from ); }

	DwordShiftImplAll() {}		// Why does GCC need these?
};

