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

/*
 * ix86 definitions v0.9.0
 *
 * Original Authors (v0.6.2 and prior):
 *		linuzappz <linuzappz@pcsx.net>
 *		alexey silinov
 *		goldfinger
 *		zerofrog(@gmail.com)
 *
 * Authors of v0.9.0:
 *		Jake.Stine(@gmail.com)
 *		cottonvibes(@gmail.com)
 *		sudonim(1@gmail.com)
 */

#pragma once

namespace x86Emitter
{
	// ----- Lea Instructions (Load Effective Address) -----
	// Note: alternate (void*) forms of these instructions are not provided since those
	// forms are functionally equivalent to Mov reg,imm, and thus better written as MOVs
	// instead.

	extern void LEA( x86Register32 to, const ModSibBase& src );
	extern void LEA( x86Register16 to, const ModSibBase& src );

	// ----- Push / Pop Instructions  -----

	extern void POP( x86Register32 from );
	extern void POP( const ModSibBase& from );

	extern void PUSH( u32 imm );
	extern void PUSH( x86Register32 from );
	extern void PUSH( const ModSibBase& from );

	static __forceinline void POP( void* from )  { POP( ptr[from] ); }
	static __forceinline void PUSH( void* from ) { PUSH( ptr[from] ); }

	// ------------------------------------------------------------------------
	using Internal::ADD;
	using Internal::OR;
	using Internal::ADC;
	using Internal::SBB;
	using Internal::AND;
	using Internal::SUB;
	using Internal::XOR;
	using Internal::CMP;

	using Internal::ROL;
	using Internal::ROR;
	using Internal::RCL;
	using Internal::RCR;
	using Internal::SHL;
	using Internal::SHR;
	using Internal::SAR;

	// ---------- 32 Bit Interface -----------
	extern void MOV( const x86Register32& to, const x86Register32& from );
	extern void MOV( const ModSibBase& sibdest, const x86Register32& from );
	extern void MOV( const x86Register32& to, const ModSibBase& sibsrc );
	extern void MOV( const x86Register32& to, const void* src );
	extern void MOV( const void* dest, const x86Register32& from );

	extern void MOV( const x86Register32& to, u32 imm );
	extern void MOV( const ModSibStrict<4>& sibdest, u32 imm );

	// ---------- 16 Bit Interface -----------
	extern void MOV( const x86Register16& to, const x86Register16& from );
	extern void MOV( const ModSibBase& sibdest, const x86Register16& from );
	extern void MOV( const x86Register16& to, const ModSibBase& sibsrc );
	extern void MOV( const x86Register16& to, const void* src );
	extern void MOV( const void* dest, const x86Register16& from );

	extern void MOV( const x86Register16& to, u16 imm );
	extern void MOV( const ModSibStrict<2>& sibdest, u16 imm );

	// ---------- 8 Bit Interface -----------
	extern void MOV( const x86Register8& to, const x86Register8& from );
	extern void MOV( const ModSibBase& sibdest, const x86Register8& from );
	extern void MOV( const x86Register8& to, const ModSibBase& sibsrc );
	extern void MOV( const x86Register8& to, const void* src );
	extern void MOV( const void* dest, const x86Register8& from );

	extern void MOV( const x86Register8& to, u8 imm );
	extern void MOV( const ModSibStrict<1>& sibdest, u8 imm );

}

