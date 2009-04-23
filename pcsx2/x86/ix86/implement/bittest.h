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

// Implementations found here: BTS/BT/BTC/BTR plus BSF/BSR!
// Note: This header is meant to be included from within the x86Emitter::Internal namespace.

// These instructions are in the 'Group8' as per Intel's manual, but since they all have
// a unified purpose, I've named them for that instead.

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
class BitScanImpl
{
public:
	BitScanImpl() {}

	__forceinline void operator()( const xRegister32& to, const xRegister32& from ) const	{ xOpWrite0F( Opcode, to, from ); }
	__forceinline void operator()( const xRegister16& to, const xRegister16& from ) const	{ xOpWrite0F( 0x66, Opcode, to, from ); }
	__forceinline void operator()( const xRegister32& to, const void* src ) const			{ xOpWrite0F( Opcode, to, src ); }
	__forceinline void operator()( const xRegister16& to, const void* src ) const			{ xOpWrite0F( 0x66, Opcode, to, src ); }
	__forceinline void operator()( const xRegister32& to, const ModSibBase& sibsrc ) const	{ xOpWrite0F( Opcode, to, sibsrc ); }
	__forceinline void operator()( const xRegister16& to, const ModSibBase& sibsrc ) const	{ xOpWrite0F( 0x66, Opcode, to, sibsrc ); }
};

//////////////////////////////////////////////////////////////////////////////////////////
// Bit Test Instructions - Valid on 16/32 bit instructions only.
//
template< G8Type InstType >
class Group8Impl : public BitScanImpl<0xa3 | (InstType << 2)>
{
public:
	using BitScanImpl<0xa3 | (InstType << 2)>::operator();

	__forceinline void operator()( const ModSibStrict<u32>& bitbase, u8 bitoffset ) const	{ xOpWrite0F( 0xba, InstType, bitbase );		xWrite<u8>( bitoffset ); }
	__forceinline void operator()( const ModSibStrict<u16>& bitbase, u8 bitoffset ) const	{ xOpWrite0F( 0x66, 0xba, InstType, bitbase );	xWrite<u8>( bitoffset ); }
	void operator()( const xRegister<u32>& bitbase, u8 bitoffset ) const					{ xOpWrite0F( 0xba, InstType, bitbase );		xWrite<u8>( bitoffset ); }
	void operator()( const xRegister<u16>& bitbase, u8 bitoffset ) const					{ xOpWrite0F( 0x66, 0xba, InstType, bitbase );	xWrite<u8>( bitoffset ); }

	Group8Impl() {}
};

