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

// Implementations found here: TEST + BTS/BT/BTC/BTR + BSF/BSR! (for lack of better location)
// Note: This header is meant to be included from within the x86Emitter::Internal namespace.

//////////////////////////////////////////////////////////////////////////////////////////
// TEST instruction Implementation
//
class xImpl_Test
{
public:
	// ------------------------------------------------------------------------
	template< typename T > __forceinline
	void operator()( const xRegister<T>& to, const xRegister<T>& from ) const
	{
		prefix16<T>();
		xWrite8( Is8BitOp<T>() ? 0x84 : 0x85 );
		EmitSibMagic( from, to );
	}
	
	// ------------------------------------------------------------------------
	template< typename T > __forceinline
	void operator()( const ModSibStrict<T>& dest, int imm ) const
	{
		prefix16<T>();
		xWrite8( Is8BitOp<T>() ? 0xf6 : 0xf7 );
		EmitSibMagic( 0, dest );
		xWrite<T>( imm );
	}
	
	// ------------------------------------------------------------------------
	template< typename T > __forceinline
	void operator()( const xRegister<T>& to, int imm ) const
	{
		prefix16<T>();

		if( to.IsAccumulator() )
			xWrite8( Is8BitOp<T>() ? 0xa8 : 0xa9 );
		else
		{
			xWrite8( Is8BitOp<T>() ? 0xf6 : 0xf7 );
			EmitSibMagic( 0, to );
		}
		xWrite<T>( imm );
	}

	xImpl_Test() {}		// Why does GCC need these?
};

enum G8Type
{
	G8Type_BT = 4,
	G8Type_BTS,
	G8Type_BTR,
	G8Type_BTC,
};

//////////////////////////////////////////////////////////////////////////////////////////
// BSF / BSR -- 16/32 operands supported only.
//
// 0xbc [fwd] / 0xbd [rev]
//
template< u16 Opcode >
class xImpl_BitScan
{
public:
	xImpl_BitScan() {}

	__forceinline void operator()( const xRegister32& to, const xRegister32& from ) const	{ xOpWrite0F( Opcode, to, from ); }
	__forceinline void operator()( const xRegister16& to, const xRegister16& from ) const	{ xOpWrite0F( 0x66, Opcode, to, from ); }
	__forceinline void operator()( const xRegister32& to, const ModSibBase& sibsrc ) const	{ xOpWrite0F( Opcode, to, sibsrc ); }
	__forceinline void operator()( const xRegister16& to, const ModSibBase& sibsrc ) const	{ xOpWrite0F( 0x66, Opcode, to, sibsrc ); }
};

//////////////////////////////////////////////////////////////////////////////////////////
// Bit Test Instructions - Valid on 16/32 bit instructions only.
//
template< G8Type InstType >
class xImpl_Group8
{
	static const uint RegFormOp = 0xa3 | (InstType << 3);
public:
	__forceinline void operator()( const xRegister32& bitbase, const xRegister32& bitoffset ) const	{ xOpWrite0F( RegFormOp, bitbase, bitoffset ); }
	__forceinline void operator()( const xRegister16& bitbase, const xRegister16& bitoffset ) const	{ xOpWrite0F( 0x66, RegFormOp, bitbase, bitoffset ); }
	__forceinline void operator()( const ModSibBase& bitbase, const xRegister32& bitoffset ) const	{ xOpWrite0F( RegFormOp, bitoffset, bitbase ); }
	__forceinline void operator()( const ModSibBase& bitbase, const xRegister16& bitoffset ) const	{ xOpWrite0F( 0x66, RegFormOp, bitoffset, bitbase ); }

	__forceinline void operator()( const ModSibStrict<u32>& bitbase, u8 bitoffset ) const			{ xOpWrite0F( 0xba, InstType, bitbase, bitoffset ); }
	__forceinline void operator()( const ModSibStrict<u16>& bitbase, u8 bitoffset ) const			{ xOpWrite0F( 0x66, 0xba, InstType, bitbase, bitoffset ); }
	__forceinline void operator()( const xRegister<u32>& bitbase, u8 bitoffset ) const				{ xOpWrite0F( 0xba, InstType, bitbase, bitoffset ); }
	__forceinline void operator()( const xRegister<u16>& bitbase, u8 bitoffset ) const				{ xOpWrite0F( 0x66, 0xba, InstType, bitbase, bitoffset ); }

	xImpl_Group8() {}
};
