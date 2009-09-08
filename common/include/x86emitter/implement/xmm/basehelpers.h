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

//////////////////////////////////////////////////////////////////////////////////////////
// MMX / SSE Helper Functions!

// ------------------------------------------------------------------------
// For implementing SSE-only logic operations that have xmmreg,xmmreg/rm forms only,
// like ANDPS/ANDPD
//
template< u8 Prefix, u16 Opcode >
class SimdImpl_DestRegSSE
{
public:
	__forceinline void operator()( const xRegisterSSE& to, const xRegisterSSE& from ) const	{ xOpWrite0F( Prefix, Opcode, to, from ); }
	__forceinline void operator()( const xRegisterSSE& to, const ModSibBase& from ) const	{ xOpWrite0F( Prefix, Opcode, to, from ); }

	SimdImpl_DestRegSSE() {} //GCWho?
};

// ------------------------------------------------------------------------
// For implementing SSE-only logic operations that have xmmreg,reg/rm,imm forms only
// (PSHUFD / PSHUFHW / etc).
//
template< u8 Prefix, u16 Opcode >
class SimdImpl_DestRegImmSSE
{
public:
	__forceinline void operator()( const xRegisterSSE& to, const xRegisterSSE& from, u8 imm ) const	{ xOpWrite0F( Prefix, Opcode, to, from, imm ); }
	__forceinline void operator()( const xRegisterSSE& to, const ModSibBase& from, u8 imm ) const	{ xOpWrite0F( Prefix, Opcode, to, from, imm ); }

	SimdImpl_DestRegImmSSE() {} //GCWho?
};

template< u8 Prefix, u16 Opcode >
class SimdImpl_DestRegImmMMX
{
public:
	__forceinline void operator()( const xRegisterMMX& to, const xRegisterMMX& from, u8 imm ) const	{ xOpWrite0F( Opcode, to, from, imm ); }
	__forceinline void operator()( const xRegisterMMX& to, const ModSibBase& from, u8 imm ) const	{ xOpWrite0F( Opcode, to, from, imm ); }

	SimdImpl_DestRegImmMMX() {} //GCWho?
};

// ------------------------------------------------------------------------
// For implementing MMX/SSE operations that have reg,reg/rm forms only,
// but accept either MM or XMM destinations (most PADD/PSUB and other P srithmetic ops).
//
template< u8 Prefix, u16 Opcode >
class SimdImpl_DestRegEither
{
public:
	__forceinline void operator()( const xRegisterSSE& to, const xRegisterSSE& from ) const	{ xOpWrite0F( Prefix, Opcode, to, from ); }
	__forceinline void operator()( const xRegisterSSE& to, const ModSibBase& from ) const	{ xOpWrite0F( Prefix, Opcode, to, from ); }

	__forceinline void operator()( const xRegisterMMX& to, const xRegisterMMX& from ) const	{ xOpWrite0F( Opcode, to, from ); }
	__forceinline void operator()( const xRegisterMMX& to, const ModSibBase& from ) const	{ xOpWrite0F( Opcode, to, from ); }

	SimdImpl_DestRegEither() {} //GCWho?
};

// ------------------------------------------------------------------------
// For implementing MMX/SSE operations where the destination *must* be a register, but the
// source can be Direct or Indirect (ModRM/SibSB).  The SrcOperandType template parameter
// is used to enforce type strictness of the (void*) parameter and ModSib<> parameter, so
// that the programmer must be explicit in specifying desired operand size.
//
// IMPORTANT: This helper assumes the prefix opcode is written *always* -- regardless of
// MMX or XMM register status.
//
template< u8 Prefix, u16 Opcode, typename DestRegType, typename SrcRegType, typename SrcOperandType >
class SimdImpl_DestRegStrict
{
public:
	__forceinline void operator()( const DestRegType& to, const SrcRegType& from ) const					{ xOpWrite0F( Prefix, Opcode, to, from ); }
	__forceinline void operator()( const DestRegType& to, const ModSibStrict<SrcOperandType>& from ) const	{ xOpWrite0F( Prefix, Opcode, to, from ); }

	SimdImpl_DestRegStrict() {} //GCWho?
};

