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

enum G1Type
{
	G1Type_ADD=0,
	G1Type_OR,
	G1Type_ADC,
	G1Type_SBB,
	G1Type_AND,
	G1Type_SUB,
	G1Type_XOR,
	G1Type_CMP
};

// -------------------------------------------------------------------
template< typename ImmType, G1Type InstType >
class Group1Impl
{
public: 
	static const uint OperandSize = sizeof(ImmType);

	Group1Impl() {}		// because GCC doesn't like static classes

protected:
	static bool Is8BitOperand()	{ return OperandSize == 1; }
	static void prefix16()		{ if( OperandSize == 2 ) iWrite<u8>( 0x66 ); }

public:
	static __emitinline void Emit( const iRegister<OperandSize>& to, const iRegister<OperandSize>& from ) 
	{
		prefix16();
		iWrite<u8>( (Is8BitOperand() ? 0 : 1) | (InstType<<3) ); 
		ModRM_Direct( from.Id, to.Id );
	}

	static __emitinline void Emit( const ModSibBase& sibdest, const iRegister<OperandSize>& from ) 
	{
		prefix16();
		iWrite<u8>( (Is8BitOperand() ? 0 : 1) | (InstType<<3) ); 
		EmitSibMagic( from.Id, sibdest );
	}

	static __emitinline void Emit( const iRegister<OperandSize>& to, const ModSibBase& sibsrc ) 
	{
		prefix16();
		iWrite<u8>( (Is8BitOperand() ? 2 : 3) | (InstType<<3) );
		EmitSibMagic( to.Id, sibsrc );
	}

	static __emitinline void Emit( void* dest, const iRegister<OperandSize>& from ) 
	{
		prefix16();
		iWrite<u8>( (Is8BitOperand() ? 0 : 1) | (InstType<<3) ); 
		iWriteDisp( from.Id, dest );
	}

	static __emitinline void Emit( const iRegister<OperandSize>& to, const void* src ) 
	{
		prefix16();
		iWrite<u8>( (Is8BitOperand() ? 2 : 3) | (InstType<<3) );
		iWriteDisp( to.Id, src );
	}

	static __emitinline void Emit( const iRegister<OperandSize>& to, ImmType imm ) 
	{
		prefix16();
		if( !Is8BitOperand() && is_s8( imm ) )
		{
			iWrite<u8>( 0x83 );
			ModRM_Direct( InstType, to.Id );
			iWrite<s8>( imm );
		}
		else
		{
			if( to.IsAccumulator() )
				iWrite<u8>( (Is8BitOperand() ? 4 : 5) | (InstType<<3) );
			else
			{
				iWrite<u8>( Is8BitOperand() ? 0x80 : 0x81 );
				ModRM_Direct( InstType, to.Id );
			}
			iWrite<ImmType>( imm );
		}
	}

	static __emitinline void Emit( const ModSibStrict<OperandSize>& sibdest, ImmType imm ) 
	{
		if( Is8BitOperand() )
		{
			iWrite<u8>( 0x80 );
			EmitSibMagic( InstType, sibdest );
			iWrite<ImmType>( imm );
		}
		else
		{		
			prefix16();
			iWrite<u8>( is_s8( imm ) ? 0x83 : 0x81 );
			EmitSibMagic( InstType, sibdest );
			if( is_s8( imm ) )
				iWrite<s8>( imm );
			else
				iWrite<ImmType>( imm );
		}
	}
};


// -------------------------------------------------------------------
//
template< G1Type InstType >
class Group1ImplAll
{
protected:
	typedef Group1Impl<u32, InstType> m_32;
	typedef Group1Impl<u16, InstType> m_16;
	typedef Group1Impl<u8, InstType>  m_8;

	// (Note: I'm not going to macro this since it would likely clobber intellisense parameter resolution)

public:
	// ---------- 32 Bit Interface -----------
	__forceinline void operator()( const iRegister32& to,	const iRegister32& from ) const	{ m_32::Emit( to, from ); }
	__forceinline void operator()( const iRegister32& to,	const void* src ) const			{ m_32::Emit( to, src ); }
	__forceinline void operator()( void* dest,				const iRegister32& from ) const	{ m_32::Emit( dest, from ); }
	__noinline void operator()( const ModSibBase& sibdest,	const iRegister32& from ) const	{ m_32::Emit( sibdest, from ); }
	__noinline void operator()( const iRegister32& to,		const ModSibBase& sibsrc ) const{ m_32::Emit( to, sibsrc ); }
	__noinline void operator()( const ModSibStrict<4>& sibdest, u32 imm ) const				{ m_32::Emit( sibdest, imm ); }

	void operator()( const iRegister32& to, u32 imm ) const									{ m_32::Emit( to, imm ); }

	// ---------- 16 Bit Interface -----------
	__forceinline void operator()( const iRegister16& to,	const iRegister16& from ) const	{ m_16::Emit( to, from ); }
	__forceinline void operator()( const iRegister16& to,	const void* src ) const			{ m_16::Emit( to, src ); }
	__forceinline void operator()( void* dest,				const iRegister16& from ) const	{ m_16::Emit( dest, from ); }
	__noinline void operator()( const ModSibBase& sibdest,	const iRegister16& from ) const	{ m_16::Emit( sibdest, from ); }
	__noinline void operator()( const iRegister16& to,		const ModSibBase& sibsrc ) const{ m_16::Emit( to, sibsrc ); }
	__noinline void operator()( const ModSibStrict<2>& sibdest, u16 imm ) const				{ m_16::Emit( sibdest, imm ); }

	void operator()( const iRegister16& to, u16 imm ) const									{ m_16::Emit( to, imm ); }

	// ---------- 8 Bit Interface -----------
	__forceinline void operator()( const iRegister8& to,	const iRegister8& from ) const	{ m_8::Emit( to, from ); }
	__forceinline void operator()( const iRegister8& to,	const void* src ) const			{ m_8::Emit( to, src ); }
	__forceinline void operator()( void* dest,				const iRegister8& from ) const	{ m_8::Emit( dest, from ); }
	__noinline void operator()( const ModSibBase& sibdest,	const iRegister8& from ) const	{ m_8::Emit( sibdest, from ); }
	__noinline void operator()( const iRegister8& to,		const ModSibBase& sibsrc ) const{ m_8::Emit( to, sibsrc ); }
	__noinline void operator()( const ModSibStrict<1>& sibdest, u8 imm ) const				{ m_8::Emit( sibdest, imm ); }

	void operator()( const iRegister8& to, u8 imm ) const									{ m_8::Emit( to, imm ); }

	Group1ImplAll() {}		// Why does GCC need these?
};

