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
	extern void iJccKnownTarget( JccComparisonType comparison, void* target, bool slideForward=false );


	// ----- Lea Instructions (Load Effective Address) -----
	// Note: alternate (void*) forms of these instructions are not provided since those
	// forms are functionally equivalent to Mov reg,imm, and thus better written as MOVs
	// instead.

	extern void iLEA( iRegister32 to, const ModSibBase& src, bool preserve_flags=false );
	extern void iLEA( iRegister16 to, const ModSibBase& src, bool preserve_flags=false );

	// ----- Push / Pop Instructions  -----
	// Note: pushad/popad implementations are intentionally left out.  The instructions are
	// invalid in x64, and are super slow on x32.  Use multiple Push/Pop instructions instead.

	extern void iPOP( const ModSibBase& from );
	extern void iPUSH( const ModSibBase& from );

	static __forceinline void iPOP( iRegister32 from )	{ write8( 0x58 | from.Id ); }
	static __forceinline void iPOP( void* from )		{ iPOP( ptr[from] ); }

	static __forceinline void iPUSH( u32 imm )			{ write8( 0x68 ); write32( imm ); }
	static __forceinline void iPUSH( iRegister32 from )	{ write8( 0x50 | from.Id ); }
	static __forceinline void iPUSH( void* from )		{ iPUSH( ptr[from] ); }

	// pushes the EFLAGS register onto the stack
	static __forceinline void iPUSHFD()	{ write8( 0x9C ); }
	// pops the EFLAGS register from the stack
	static __forceinline void iPOPFD()	{ write8( 0x9D ); }

	// ----- Miscellaneous Instructions  -----
	// Various Instructions with no parameter and no special encoding logic.

	__forceinline void iRET()	{ write8( 0xC3 ); }
	__forceinline void iCBW()	{ write16( 0x9866 );  }
	__forceinline void iCWD()	{ write8( 0x98 ); }
	__forceinline void iCDQ()	{ write8( 0x99 ); }
	__forceinline void iCWDE()	{ write8( 0x98 ); }

	__forceinline void iLAHF()	{ write8( 0x9f ); }
	__forceinline void iSAHF()	{ write8( 0x9e ); }

	__forceinline void iSTC()	{ write8( 0xF9 ); }
	__forceinline void iCLC()	{ write8( 0xF8 ); }

	// NOP 1-byte
	__forceinline void iNOP()	{ write8(0x90); }
	
	//////////////////////////////////////////////////////////////////////////////////////////
	// MUL / DIV instructions
	
	extern void iUMUL( const iRegister32& from );
	extern void iUMUL( const iRegister16& from );
	extern void iUMUL( const iRegister8& from );
	extern void iUMUL( const ModSibSized& from );

	extern void iUDIV( const iRegister32& from );
	extern void iUDIV( const iRegister16& from );
	extern void iUDIV( const iRegister8& from );
	extern void iUDIV( const ModSibSized& from );

	extern void iSDIV( const iRegister32& from );
	extern void iSDIV( const iRegister16& from );
	extern void iSDIV( const iRegister8& from );
	extern void iSDIV( const ModSibSized& from );

	extern void iSMUL( const iRegister32& from );
	extern void iSMUL( const iRegister32& to,	const iRegister32& from );
	extern void iSMUL( const iRegister32& to,	const void* src );
	extern void iSMUL( const iRegister32& to,	const iRegister32& from, s32 imm );
	extern void iSMUL( const iRegister32& to,	const ModSibBase& src );
	extern void iSMUL( const iRegister32& to,	const ModSibBase& src, s32 imm );

	extern void iSMUL( const iRegister16& from );
	extern void iSMUL( const iRegister16& to,	const iRegister16& from );
	extern void iSMUL( const iRegister16& to,	const void* src );
	extern void iSMUL( const iRegister16& to,	const iRegister16& from, s16 imm );
	extern void iSMUL( const iRegister16& to,	const ModSibBase& src );
	extern void iSMUL( const iRegister16& to,	const ModSibBase& src, s16 imm );

	extern void iSMUL( const iRegister8& from );
	extern void iSMUL( const ModSibSized& from );


	//////////////////////////////////////////////////////////////////////////////////////////
	// MOV instructions!
	// ---------- 32 Bit Interface -----------
	extern void iMOV( const iRegister32& to, const iRegister32& from );
	extern void iMOV( const ModSibBase& sibdest, const iRegister32& from );
	extern void iMOV( const iRegister32& to, const ModSibBase& sibsrc );
	extern void iMOV( const iRegister32& to, const void* src );
	extern void iMOV( void* dest, const iRegister32& from );

	// preserve_flags  - set to true to disable optimizations which could alter the state of
	//   the flags (namely replacing mov reg,0 with xor).
	extern void iMOV( const iRegister32& to, u32 imm, bool preserve_flags=false );
	extern void iMOV( const ModSibStrict<4>& sibdest, u32 imm );

	// ---------- 16 Bit Interface -----------
	extern void iMOV( const iRegister16& to, const iRegister16& from );
	extern void iMOV( const ModSibBase& sibdest, const iRegister16& from );
	extern void iMOV( const iRegister16& to, const ModSibBase& sibsrc );
	extern void iMOV( const iRegister16& to, const void* src );
	extern void iMOV( void* dest, const iRegister16& from );

	// preserve_flags  - set to true to disable optimizations which could alter the state of
	//   the flags (namely replacing mov reg,0 with xor).
	extern void iMOV( const iRegister16& to, u16 imm, bool preserve_flags=false );
	extern void iMOV( const ModSibStrict<2>& sibdest, u16 imm );

	// ---------- 8 Bit Interface -----------
	extern void iMOV( const iRegister8& to, const iRegister8& from );
	extern void iMOV( const ModSibBase& sibdest, const iRegister8& from );
	extern void iMOV( const iRegister8& to, const ModSibBase& sibsrc );
	extern void iMOV( const iRegister8& to, const void* src );
	extern void iMOV( void* dest, const iRegister8& from );

	extern void iMOV( const iRegister8& to, u8 imm, bool preserve_flags=false );
	extern void iMOV( const ModSibStrict<1>& sibdest, u8 imm );

	//////////////////////////////////////////////////////////////////////////////////////////
	// JMP / Jcc Instructions!

#define DEFINE_FORWARD_JUMP( label, cond ) \
	template< typename OperandType > \
	class iForward##label : public iForwardJump<OperandType> \
	{ \
	public: \
		iForward##label() : iForwardJump<OperandType>( cond ) {} \
	};

	// ------------------------------------------------------------------------
	// Note: typedefs below  are defined individually in order to appease Intellisense
	// resolution.  Including them into the class definition macro above breaks it.

	typedef iForwardJump<s8>  iForwardJump8;
	typedef iForwardJump<s32> iForwardJump32;
	

	DEFINE_FORWARD_JUMP( JA,	Jcc_Above );
	DEFINE_FORWARD_JUMP( JB,	Jcc_Below );
	DEFINE_FORWARD_JUMP( JAE,	Jcc_AboveOrEqual );
	DEFINE_FORWARD_JUMP( JBE,	Jcc_BelowOrEqual );

	typedef iForwardJA<s8>		iForwardJA8;
	typedef iForwardJA<s32>		iForwardJA32;
	typedef iForwardJB<s8>		iForwardJB8;
	typedef iForwardJB<s32>		iForwardJB32;
	typedef iForwardJAE<s8>		iForwardJAE8;
	typedef iForwardJAE<s32>	iForwardJAE32;
	typedef iForwardJBE<s8>		iForwardJBE8;
	typedef iForwardJBE<s32>	iForwardJBE32;

	DEFINE_FORWARD_JUMP( JG,	Jcc_Greater );
	DEFINE_FORWARD_JUMP( JL,	Jcc_Less );
	DEFINE_FORWARD_JUMP( JGE,	Jcc_GreaterOrEqual );
	DEFINE_FORWARD_JUMP( JLE,	Jcc_LessOrEqual );

	typedef iForwardJG<s8>		iForwardJG8;
	typedef iForwardJG<s32>		iForwardJG32;
	typedef iForwardJL<s8>		iForwardJL8;
	typedef iForwardJL<s32>		iForwardJL32;
	typedef iForwardJGE<s8>		iForwardJGE8;
	typedef iForwardJGE<s32>	iForwardJGE32;
	typedef iForwardJLE<s8>		iForwardJLE8;
	typedef iForwardJLE<s32>	iForwardJLE32;

	DEFINE_FORWARD_JUMP( JZ,	Jcc_Zero );
	DEFINE_FORWARD_JUMP( JE,	Jcc_Equal );
	DEFINE_FORWARD_JUMP( JNZ,	Jcc_NotZero );
	DEFINE_FORWARD_JUMP( JNE,	Jcc_NotEqual );

	typedef iForwardJZ<s8>		iForwardJZ8;
	typedef iForwardJZ<s32>		iForwardJZ32;
	typedef iForwardJE<s8>		iForwardJE8;
	typedef iForwardJE<s32>		iForwardJE32;
	typedef iForwardJNZ<s8>		iForwardJNZ8;
	typedef iForwardJNZ<s32>	iForwardJNZ32;
	typedef iForwardJNE<s8>		iForwardJNE8;
	typedef iForwardJNE<s32>	iForwardJNE32;

	DEFINE_FORWARD_JUMP( JS,	Jcc_Signed );
	DEFINE_FORWARD_JUMP( JNS,	Jcc_Unsigned );

	typedef iForwardJS<s8>		iForwardJS8;
	typedef iForwardJS<s32>		iForwardJS32;
	typedef iForwardJNS<s8>		iForwardJNS8;
	typedef iForwardJNS<s32>	iForwardJNS32;

	DEFINE_FORWARD_JUMP( JO,	Jcc_Overflow );
	DEFINE_FORWARD_JUMP( JNO,	Jcc_NotOverflow );

	typedef iForwardJO<s8>		iForwardJO8;
	typedef iForwardJO<s32>		iForwardJO32;
	typedef iForwardJNO<s8>		iForwardJNO8;
	typedef iForwardJNO<s32>	iForwardJNO32;

	DEFINE_FORWARD_JUMP( JC,	Jcc_Carry );
	DEFINE_FORWARD_JUMP( JNC,	Jcc_NotCarry );

	typedef iForwardJC<s8>		iForwardJC8;
	typedef iForwardJC<s32>		iForwardJC32;
	typedef iForwardJNC<s8>		iForwardJNC8;
	typedef iForwardJNC<s32>	iForwardJNC32;

	DEFINE_FORWARD_JUMP( JPE,	Jcc_ParityEven );
	DEFINE_FORWARD_JUMP( JPO,	Jcc_ParityOdd );
	
	typedef iForwardJPE<s8>		iForwardJPE8;
	typedef iForwardJPE<s32>	iForwardJPE32;
	typedef iForwardJPO<s8>		iForwardJPO8;
	typedef iForwardJPO<s32>	iForwardJPO32;
}

