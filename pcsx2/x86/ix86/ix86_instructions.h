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
	// ------------------------------------------------------------------------
	// Group 1 Instruction Class

	extern const Internal::Group1ImplAll<Internal::G1Type_ADD> iADD;
	extern const Internal::Group1ImplAll<Internal::G1Type_OR>  iOR;
	extern const Internal::Group1ImplAll<Internal::G1Type_ADC> iADC;
	extern const Internal::Group1ImplAll<Internal::G1Type_SBB> iSBB;
	extern const Internal::Group1ImplAll<Internal::G1Type_AND> iAND;
	extern const Internal::Group1ImplAll<Internal::G1Type_SUB> iSUB;
	extern const Internal::Group1ImplAll<Internal::G1Type_XOR> iXOR;
	extern const Internal::Group1ImplAll<Internal::G1Type_CMP> iCMP;

	// ------------------------------------------------------------------------
	// Group 2 Instruction Class
	//
	// Optimization Note: For Imm forms, we ignore the instruction if the shift count is
	// zero.  This is a safe optimization since any zero-value shift does not affect any
	// flags.

	extern const Internal::MovImplAll iMOV;
	extern const Internal::TestImplAll iTEST;

	extern const Internal::Group2ImplAll<Internal::G2Type_ROL> iROL;
	extern const Internal::Group2ImplAll<Internal::G2Type_ROR> iROR;
	extern const Internal::Group2ImplAll<Internal::G2Type_RCL> iRCL;
	extern const Internal::Group2ImplAll<Internal::G2Type_RCR> iRCR;
	extern const Internal::Group2ImplAll<Internal::G2Type_SHL> iSHL;
	extern const Internal::Group2ImplAll<Internal::G2Type_SHR> iSHR;
	extern const Internal::Group2ImplAll<Internal::G2Type_SAR> iSAR;

	// ------------------------------------------------------------------------
	// Group 3 Instruction Class

	extern const Internal::Group3ImplAll<Internal::G3Type_NOT> iNOT;
	extern const Internal::Group3ImplAll<Internal::G3Type_NEG> iNEG;
	extern const Internal::Group3ImplAll<Internal::G3Type_MUL> iUMUL;
	extern const Internal::Group3ImplAll<Internal::G3Type_DIV> iUDIV;
	extern const Internal::Group3ImplAll<Internal::G3Type_iDIV> iSDIV;

	extern const Internal::IncDecImplAll<false> iINC;
	extern const Internal::IncDecImplAll<true>  iDEC;

	extern const Internal::MovExtendImplAll<false> iMOVZX;
	extern const Internal::MovExtendImplAll<true>  iMOVSX;

	extern const Internal::DwordShiftImplAll<false> iSHLD;
	extern const Internal::DwordShiftImplAll<true>  iSHRD;

	extern const Internal::Group8ImplAll<Internal::G8Type_BT> iBT;
	extern const Internal::Group8ImplAll<Internal::G8Type_BTR> iBTR;
	extern const Internal::Group8ImplAll<Internal::G8Type_BTS> iBTS;
	extern const Internal::Group8ImplAll<Internal::G8Type_BTC> iBTC;

	extern const Internal::JmpCallImplAll<true> iJMP;
	extern const Internal::JmpCallImplAll<false> iCALL;

	extern const Internal::BitScanImplAll<false> iBSF;
	extern const Internal::BitScanImplAll<true> iBSR;

	// ------------------------------------------------------------------------
	extern const Internal::CMovImplGeneric iCMOV;

	extern const Internal::CMovImplAll<Jcc_Above>			iCMOVA;
	extern const Internal::CMovImplAll<Jcc_AboveOrEqual>	iCMOVAE;
	extern const Internal::CMovImplAll<Jcc_Below>			iCMOVB;
	extern const Internal::CMovImplAll<Jcc_BelowOrEqual>	iCMOVBE;

	extern const Internal::CMovImplAll<Jcc_Greater>			iCMOVG;
	extern const Internal::CMovImplAll<Jcc_GreaterOrEqual>	iCMOVGE;
	extern const Internal::CMovImplAll<Jcc_Less>			iCMOVL;
	extern const Internal::CMovImplAll<Jcc_LessOrEqual>		iCMOVLE;

	extern const Internal::CMovImplAll<Jcc_Zero>			iCMOVZ;
	extern const Internal::CMovImplAll<Jcc_Equal>			iCMOVE;
	extern const Internal::CMovImplAll<Jcc_NotZero>			iCMOVNZ;
	extern const Internal::CMovImplAll<Jcc_NotEqual>		iCMOVNE;

	extern const Internal::CMovImplAll<Jcc_Overflow>		iCMOVO;
	extern const Internal::CMovImplAll<Jcc_NotOverflow>		iCMOVNO;
	extern const Internal::CMovImplAll<Jcc_Carry>			iCMOVC;
	extern const Internal::CMovImplAll<Jcc_NotCarry>		iCMOVNC;

	extern const Internal::CMovImplAll<Jcc_Signed>			iCMOVS;
	extern const Internal::CMovImplAll<Jcc_Unsigned>		iCMOVNS;
	extern const Internal::CMovImplAll<Jcc_ParityEven>		iCMOVPE;
	extern const Internal::CMovImplAll<Jcc_ParityOdd>		iCMOVPO;

	// ------------------------------------------------------------------------
	extern const Internal::SetImplGeneric iSET;

	extern const Internal::SetImplAll<Jcc_Above>			iSETA;
	extern const Internal::SetImplAll<Jcc_AboveOrEqual>		iSETAE;
	extern const Internal::SetImplAll<Jcc_Below>			iSETB;
	extern const Internal::SetImplAll<Jcc_BelowOrEqual>		iSETBE;

	extern const Internal::SetImplAll<Jcc_Greater>			iSETG;
	extern const Internal::SetImplAll<Jcc_GreaterOrEqual>	iSETGE;
	extern const Internal::SetImplAll<Jcc_Less>				iSETL;
	extern const Internal::SetImplAll<Jcc_LessOrEqual>		iSETLE;

	extern const Internal::SetImplAll<Jcc_Zero>				iSETZ;
	extern const Internal::SetImplAll<Jcc_Equal>			iSETE;
	extern const Internal::SetImplAll<Jcc_NotZero>			iSETNZ;
	extern const Internal::SetImplAll<Jcc_NotEqual>			iSETNE;

	extern const Internal::SetImplAll<Jcc_Overflow>			iSETO;
	extern const Internal::SetImplAll<Jcc_NotOverflow>		iSETNO;
	extern const Internal::SetImplAll<Jcc_Carry>			iSETC;
	extern const Internal::SetImplAll<Jcc_NotCarry>			iSETNC;

	extern const Internal::SetImplAll<Jcc_Signed>			iSETS;
	extern const Internal::SetImplAll<Jcc_Unsigned>			iSETNS;
	extern const Internal::SetImplAll<Jcc_ParityEven>		iSETPE;
	extern const Internal::SetImplAll<Jcc_ParityOdd>		iSETPO;

	//////////////////////////////////////////////////////////////////////////////////////////
	// Miscellaneous Instructions
	// These are all defined inline or in ix86.cpp.
	//

	extern void iBSWAP( const iRegister32& to );

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
	
	extern void iSMUL( const iRegister32& to,	const iRegister32& from );
	extern void iSMUL( const iRegister32& to,	const void* src );
	extern void iSMUL( const iRegister32& to,	const iRegister32& from, s32 imm );
	extern void iSMUL( const iRegister32& to,	const ModSibBase& src );
	extern void iSMUL( const iRegister32& to,	const ModSibBase& src, s32 imm );

	extern void iSMUL( const iRegister16& to,	const iRegister16& from );
	extern void iSMUL( const iRegister16& to,	const void* src );
	extern void iSMUL( const iRegister16& to,	const iRegister16& from, s16 imm );
	extern void iSMUL( const iRegister16& to,	const ModSibBase& src );
	extern void iSMUL( const iRegister16& to,	const ModSibBase& src, s16 imm );

	template< typename T >
	__forceinline void iSMUL( const iRegister<T>& from )	{ Internal::Group3Impl<T>::Emit( Internal::G3Type_iMUL, from ); }
	template< typename T >
	__noinline void iSMUL( const ModSibStrict<T>& from )	{ Internal::Group3Impl<T>::Emit( Internal::G3Type_iMUL, from ); }

	//////////////////////////////////////////////////////////////////////////////////////////
	// JMP / Jcc Instructions!

	extern void iJccKnownTarget( JccComparisonType comparison, void* target, bool slideForward=false );

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

	//////////////////////////////////////////////////////////////////////////////////////////
	// MMX Mov Instructions (MOVD, MOVQ, MOVSS).
	//
	// Notes:
	//  * Some of the functions have been renamed to more clearly reflect what they actually
	//    do.  Namely we've affixed "ZX" to several MOVs that take a register as a destination
	//    since that's what they do (MOVD clears upper 32/96 bits, etc).
	//
	
	// ------------------------------------------------------------------------
	// MOVD has valid forms for MMX and XMM registers.
	//
	template< typename T >
	__emitinline void iMOVDZX( const iRegisterSIMD<T>& to, const iRegister32& from )
	{
		Internal::writeXMMop<0x66>( 0x6e, to, from );
	}

	template< typename T >
	__emitinline void iMOVDZX( const iRegisterSIMD<T>& to, const void* src )
	{
		Internal::writeXMMop<0x66>( 0x6e, to, src );
	}

	template< typename T >
	void iMOVDZX( const iRegisterSIMD<T>& to, const ModSibBase& src )
	{
		Internal::writeXMMop<0x66>( 0x6e, to, src );
	}

	template< typename T >
	__emitinline void iMOVD( const iRegister32& to, const iRegisterSIMD<T>& from )
	{
		Internal::writeXMMop<0x66>( 0x7e, from, to );
	}

	template< typename T >
	__emitinline void iMOVD( void* dest, const iRegisterSIMD<T>& from )
	{
		Internal::writeXMMop<0x66>( 0x7e, from, dest );
	}

	template< typename T >
	void iMOVD( const ModSibBase& dest, const iRegisterSIMD<T>& from )
	{
		Internal::writeXMMop<0x66>( 0x7e, from, dest );
	}

	// ------------------------------------------------------------------------
	
	

	// ------------------------------------------------------------------------
	
	extern void iMOVQ( const iRegisterMMX& to, const iRegisterMMX& from );
	extern void iMOVQ( const iRegisterMMX& to, const iRegisterSSE& from );
	extern void iMOVQ( const iRegisterSSE& to, const iRegisterMMX& from );

	extern void iMOVQ( void* dest, const iRegisterSSE& from );
	extern void iMOVQ( const ModSibBase& dest, const iRegisterSSE& from );
	extern void iMOVQ( void* dest, const iRegisterMMX& from );
	extern void iMOVQ( const ModSibBase& dest, const iRegisterMMX& from );
	extern void iMOVQ( const iRegisterMMX& to, const void* src );
	extern void iMOVQ( const iRegisterMMX& to, const ModSibBase& src );

	extern void iMOVQZX( const iRegisterSSE& to, const void* src );
	extern void iMOVQZX( const iRegisterSSE& to, const ModSibBase& src );
	extern void iMOVQZX( const iRegisterSSE& to, const iRegisterSSE& from );
	
	extern void iMOVSS( const iRegisterSSE& to, const iRegisterSSE& from );
	extern void iMOVSS( const void* to, const iRegisterSSE& from );
	extern void iMOVSS( const ModSibBase& to, const iRegisterSSE& from );
	extern void iMOVSD( const iRegisterSSE& to, const iRegisterSSE& from );
	extern void iMOVSD( const void* to, const iRegisterSSE& from );
	extern void iMOVSD( const ModSibBase& to, const iRegisterSSE& from );

	extern void iMOVSSZX( const iRegisterSSE& to, const void* from );
	extern void iMOVSSZX( const iRegisterSSE& to, const ModSibBase& from );
	extern void iMOVSDZX( const iRegisterSSE& to, const void* from );
	extern void iMOVSDZX( const iRegisterSSE& to, const ModSibBase& from );

}

