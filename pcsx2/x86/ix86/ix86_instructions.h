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

	extern const Internal::xImpl_G1Logic<Internal::G1Type_AND,0x54> xAND;
	extern const Internal::xImpl_G1Logic<Internal::G1Type_OR,0x56>  xOR;
	extern const Internal::xImpl_G1Logic<Internal::G1Type_XOR,0x57> xXOR;

	extern const Internal::xImpl_G1Arith<Internal::G1Type_ADD,0x58> xADD;
	extern const Internal::xImpl_G1Arith<Internal::G1Type_SUB,0x5c> xSUB;
	extern const Internal::xImpl_G1Compare xCMP;

	extern const Internal::xImpl_Group1<Internal::G1Type_ADC> xADC;
	extern const Internal::xImpl_Group1<Internal::G1Type_SBB> xSBB;

	// ------------------------------------------------------------------------
	// Group 2 Instruction Class
	//
	// Optimization Note: For Imm forms, we ignore the instruction if the shift count is
	// zero.  This is a safe optimization since any zero-value shift does not affect any
	// flags.

	extern const Internal::MovImplAll xMOV;
	extern const Internal::TestImplAll xTEST;

	extern const Internal::Group2ImplAll<Internal::G2Type_ROL> xROL;
	extern const Internal::Group2ImplAll<Internal::G2Type_ROR> xROR;
	extern const Internal::Group2ImplAll<Internal::G2Type_RCL> xRCL;
	extern const Internal::Group2ImplAll<Internal::G2Type_RCR> xRCR;
	extern const Internal::Group2ImplAll<Internal::G2Type_SHL> xSHL;
	extern const Internal::Group2ImplAll<Internal::G2Type_SHR> xSHR;
	extern const Internal::Group2ImplAll<Internal::G2Type_SAR> xSAR;

	// ------------------------------------------------------------------------
	// Group 3 Instruction Class

	extern const Internal::Group3ImplAll<Internal::G3Type_NOT> xNOT;
	extern const Internal::Group3ImplAll<Internal::G3Type_NEG> xNEG;
	extern const Internal::Group3ImplAll<Internal::G3Type_MUL> xUMUL;
	extern const Internal::Group3ImplAll<Internal::G3Type_DIV> xUDIV;
	extern const Internal::xImpl_Group3<Internal::G3Type_iDIV,0x5e> xDIV;
	extern const Internal::xImpl_iMul xMUL;

	extern const Internal::IncDecImplAll<false> xINC;
	extern const Internal::IncDecImplAll<true>  xDEC;

	extern const Internal::MovExtendImplAll<false> xMOVZX;
	extern const Internal::MovExtendImplAll<true>  xMOVSX;

	extern const Internal::DwordShiftImplAll<false> xSHLD;
	extern const Internal::DwordShiftImplAll<true>  xSHRD;

	extern const Internal::Group8ImplAll<Internal::G8Type_BT> xBT;
	extern const Internal::Group8ImplAll<Internal::G8Type_BTR> xBTR;
	extern const Internal::Group8ImplAll<Internal::G8Type_BTS> xBTS;
	extern const Internal::Group8ImplAll<Internal::G8Type_BTC> xBTC;

	extern const Internal::JmpCallImplAll<true> xJMP;
	extern const Internal::JmpCallImplAll<false> xCALL;

	extern const Internal::BitScanImplAll<false> xBSF;
	extern const Internal::BitScanImplAll<true> xBSR;

	// ------------------------------------------------------------------------
	extern const Internal::CMovImplGeneric xCMOV;

	extern const Internal::CMovImplAll<Jcc_Above>			xCMOVA;
	extern const Internal::CMovImplAll<Jcc_AboveOrEqual>	xCMOVAE;
	extern const Internal::CMovImplAll<Jcc_Below>			xCMOVB;
	extern const Internal::CMovImplAll<Jcc_BelowOrEqual>	xCMOVBE;

	extern const Internal::CMovImplAll<Jcc_Greater>			xCMOVG;
	extern const Internal::CMovImplAll<Jcc_GreaterOrEqual>	xCMOVGE;
	extern const Internal::CMovImplAll<Jcc_Less>			xCMOVL;
	extern const Internal::CMovImplAll<Jcc_LessOrEqual>		xCMOVLE;

	extern const Internal::CMovImplAll<Jcc_Zero>			xCMOVZ;
	extern const Internal::CMovImplAll<Jcc_Equal>			xCMOVE;
	extern const Internal::CMovImplAll<Jcc_NotZero>			xCMOVNZ;
	extern const Internal::CMovImplAll<Jcc_NotEqual>		xCMOVNE;

	extern const Internal::CMovImplAll<Jcc_Overflow>		xCMOVO;
	extern const Internal::CMovImplAll<Jcc_NotOverflow>		xCMOVNO;
	extern const Internal::CMovImplAll<Jcc_Carry>			xCMOVC;
	extern const Internal::CMovImplAll<Jcc_NotCarry>		xCMOVNC;

	extern const Internal::CMovImplAll<Jcc_Signed>			xCMOVS;
	extern const Internal::CMovImplAll<Jcc_Unsigned>		xCMOVNS;
	extern const Internal::CMovImplAll<Jcc_ParityEven>		xCMOVPE;
	extern const Internal::CMovImplAll<Jcc_ParityOdd>		xCMOVPO;

	// ------------------------------------------------------------------------
	extern const Internal::SetImplGeneric xSET;

	extern const Internal::SetImplAll<Jcc_Above>			xSETA;
	extern const Internal::SetImplAll<Jcc_AboveOrEqual>		xSETAE;
	extern const Internal::SetImplAll<Jcc_Below>			xSETB;
	extern const Internal::SetImplAll<Jcc_BelowOrEqual>		xSETBE;

	extern const Internal::SetImplAll<Jcc_Greater>			xSETG;
	extern const Internal::SetImplAll<Jcc_GreaterOrEqual>	xSETGE;
	extern const Internal::SetImplAll<Jcc_Less>				xSETL;
	extern const Internal::SetImplAll<Jcc_LessOrEqual>		xSETLE;

	extern const Internal::SetImplAll<Jcc_Zero>				xSETZ;
	extern const Internal::SetImplAll<Jcc_Equal>			xSETE;
	extern const Internal::SetImplAll<Jcc_NotZero>			xSETNZ;
	extern const Internal::SetImplAll<Jcc_NotEqual>			xSETNE;

	extern const Internal::SetImplAll<Jcc_Overflow>			xSETO;
	extern const Internal::SetImplAll<Jcc_NotOverflow>		xSETNO;
	extern const Internal::SetImplAll<Jcc_Carry>			xSETC;
	extern const Internal::SetImplAll<Jcc_NotCarry>			xSETNC;

	extern const Internal::SetImplAll<Jcc_Signed>			xSETS;
	extern const Internal::SetImplAll<Jcc_Unsigned>			xSETNS;
	extern const Internal::SetImplAll<Jcc_ParityEven>		xSETPE;
	extern const Internal::SetImplAll<Jcc_ParityOdd>		xSETPO;

	//////////////////////////////////////////////////////////////////////////////////////////
	// Miscellaneous Instructions
	// These are all defined inline or in ix86.cpp.
	//

	extern void xBSWAP( const xRegister32& to );

	// ----- Lea Instructions (Load Effective Address) -----
	// Note: alternate (void*) forms of these instructions are not provided since those
	// forms are functionally equivalent to Mov reg,imm, and thus better written as MOVs
	// instead.

	extern void xLEA( xRegister32 to, const ModSibBase& src, bool preserve_flags=false );
	extern void xLEA( xRegister16 to, const ModSibBase& src, bool preserve_flags=false );

	// ----- Push / Pop Instructions  -----
	// Note: pushad/popad implementations are intentionally left out.  The instructions are
	// invalid in x64, and are super slow on x32.  Use multiple Push/Pop instructions instead.

	extern void xPOP( const ModSibBase& from );
	extern void xPUSH( const ModSibBase& from );

	static __forceinline void xPOP( xRegister32 from )	{ write8( 0x58 | from.Id ); }
	static __forceinline void xPOP( void* from )		{ xPOP( ptr[from] ); }

	static __forceinline void xPUSH( u32 imm )			{ write8( 0x68 ); write32( imm ); }
	static __forceinline void xPUSH( xRegister32 from )	{ write8( 0x50 | from.Id ); }
	static __forceinline void xPUSH( void* from )		{ xPUSH( ptr[from] ); }

	// pushes the EFLAGS register onto the stack
	static __forceinline void xPUSHFD()	{ write8( 0x9C ); }
	// pops the EFLAGS register from the stack
	static __forceinline void xPOPFD()	{ write8( 0x9D ); }

	// ----- Miscellaneous Instructions  -----
	// Various Instructions with no parameter and no special encoding logic.

	__forceinline void xRET()	{ write8( 0xC3 ); }
	__forceinline void xCBW()	{ write16( 0x9866 );  }
	__forceinline void xCWD()	{ write8( 0x98 ); }
	__forceinline void xCDQ()	{ write8( 0x99 ); }
	__forceinline void xCWDE()	{ write8( 0x98 ); }

	__forceinline void xLAHF()	{ write8( 0x9f ); }
	__forceinline void xSAHF()	{ write8( 0x9e ); }

	__forceinline void xSTC()	{ write8( 0xF9 ); }
	__forceinline void xCLC()	{ write8( 0xF8 ); }

	// NOP 1-byte
	__forceinline void xNOP()	{ write8(0x90); }

	//////////////////////////////////////////////////////////////////////////////////////////
	// JMP / Jcc Instructions!

	extern void iJccKnownTarget( JccComparisonType comparison, void* target, bool slideForward=false );

#define DEFINE_FORWARD_JUMP( label, cond ) \
	template< typename OperandType > \
	class xForward##label : public xForwardJump<OperandType> \
	{ \
	public: \
		xForward##label() : xForwardJump<OperandType>( cond ) {} \
	};

	// ------------------------------------------------------------------------
	// Note: typedefs below  are defined individually in order to appease Intellisense
	// resolution.  Including them into the class definition macro above breaks it.

	typedef xForwardJump<s8>  xForwardJump8;
	typedef xForwardJump<s32> xForwardJump32;

	DEFINE_FORWARD_JUMP( JA,	Jcc_Above );
	DEFINE_FORWARD_JUMP( JB,	Jcc_Below );
	DEFINE_FORWARD_JUMP( JAE,	Jcc_AboveOrEqual );
	DEFINE_FORWARD_JUMP( JBE,	Jcc_BelowOrEqual );

	typedef xForwardJA<s8>		xForwardJA8;
	typedef xForwardJA<s32>		xForwardJA32;
	typedef xForwardJB<s8>		xForwardJB8;
	typedef xForwardJB<s32>		xForwardJB32;
	typedef xForwardJAE<s8>		xForwardJAE8;
	typedef xForwardJAE<s32>	xForwardJAE32;
	typedef xForwardJBE<s8>		xForwardJBE8;
	typedef xForwardJBE<s32>	xForwardJBE32;

	DEFINE_FORWARD_JUMP( JG,	Jcc_Greater );
	DEFINE_FORWARD_JUMP( JL,	Jcc_Less );
	DEFINE_FORWARD_JUMP( JGE,	Jcc_GreaterOrEqual );
	DEFINE_FORWARD_JUMP( JLE,	Jcc_LessOrEqual );

	typedef xForwardJG<s8>		xForwardJG8;
	typedef xForwardJG<s32>		xForwardJG32;
	typedef xForwardJL<s8>		xForwardJL8;
	typedef xForwardJL<s32>		xForwardJL32;
	typedef xForwardJGE<s8>		xForwardJGE8;
	typedef xForwardJGE<s32>	xForwardJGE32;
	typedef xForwardJLE<s8>		xForwardJLE8;
	typedef xForwardJLE<s32>	xForwardJLE32;

	DEFINE_FORWARD_JUMP( JZ,	Jcc_Zero );
	DEFINE_FORWARD_JUMP( JE,	Jcc_Equal );
	DEFINE_FORWARD_JUMP( JNZ,	Jcc_NotZero );
	DEFINE_FORWARD_JUMP( JNE,	Jcc_NotEqual );

	typedef xForwardJZ<s8>		xForwardJZ8;
	typedef xForwardJZ<s32>		xForwardJZ32;
	typedef xForwardJE<s8>		xForwardJE8;
	typedef xForwardJE<s32>		xForwardJE32;
	typedef xForwardJNZ<s8>		xForwardJNZ8;
	typedef xForwardJNZ<s32>	xForwardJNZ32;
	typedef xForwardJNE<s8>		xForwardJNE8;
	typedef xForwardJNE<s32>	xForwardJNE32;

	DEFINE_FORWARD_JUMP( JS,	Jcc_Signed );
	DEFINE_FORWARD_JUMP( JNS,	Jcc_Unsigned );

	typedef xForwardJS<s8>		xForwardJS8;
	typedef xForwardJS<s32>		xForwardJS32;
	typedef xForwardJNS<s8>		xForwardJNS8;
	typedef xForwardJNS<s32>	xForwardJNS32;

	DEFINE_FORWARD_JUMP( JO,	Jcc_Overflow );
	DEFINE_FORWARD_JUMP( JNO,	Jcc_NotOverflow );

	typedef xForwardJO<s8>		xForwardJO8;
	typedef xForwardJO<s32>		xForwardJO32;
	typedef xForwardJNO<s8>		xForwardJNO8;
	typedef xForwardJNO<s32>	xForwardJNO32;

	DEFINE_FORWARD_JUMP( JC,	Jcc_Carry );
	DEFINE_FORWARD_JUMP( JNC,	Jcc_NotCarry );

	typedef xForwardJC<s8>		xForwardJC8;
	typedef xForwardJC<s32>		xForwardJC32;
	typedef xForwardJNC<s8>		xForwardJNC8;
	typedef xForwardJNC<s32>	xForwardJNC32;

	DEFINE_FORWARD_JUMP( JPE,	Jcc_ParityEven );
	DEFINE_FORWARD_JUMP( JPO,	Jcc_ParityOdd );
	
	typedef xForwardJPE<s8>		xForwardJPE8;
	typedef xForwardJPE<s32>	xForwardJPE32;
	typedef xForwardJPO<s8>		xForwardJPO8;
	typedef xForwardJPO<s32>	xForwardJPO32;

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
	__emitinline void xMOVDZX( const xRegisterSIMD<T>& to, const xRegister32& from )
	{
		Internal::writeXMMop( 0x66, 0x6e, to, from );
	}

	template< typename T >
	__emitinline void xMOVDZX( const xRegisterSIMD<T>& to, const void* src )
	{
		Internal::writeXMMop( 0x66, 0x6e, to, src );
	}

	template< typename T >
	void xMOVDZX( const xRegisterSIMD<T>& to, const ModSibBase& src )
	{
		Internal::writeXMMop( 0x66, 0x6e, to, src );
	}

	template< typename T >
	__emitinline void xMOVD( const xRegister32& to, const xRegisterSIMD<T>& from )
	{
		Internal::writeXMMop( 0x66, 0x7e, from, to );
	}

	template< typename T >
	__emitinline void xMOVD( void* dest, const xRegisterSIMD<T>& from )
	{
		Internal::writeXMMop( 0x66, 0x7e, from, dest );
	}

	template< typename T >
	void xMOVD( const ModSibBase& dest, const xRegisterSIMD<T>& from )
	{
		Internal::writeXMMop( 0x66, 0x7e, from, dest );
	}


	// ------------------------------------------------------------------------

	// xMASKMOV:
	// Selectively write bytes from mm1/xmm1 to memory location using the byte mask in mm2/xmm2.
	// The default memory location is specified by DS:EDI.  The most significant bit in each byte
	// of the mask operand determines whether the corresponding byte in the source operand is
	// written to the corresponding byte location in memory.

	template< typename T >
	static __forceinline void xMASKMOV( const xRegisterSIMD<T>& to, const xRegisterSIMD<T>& from )	{ Internal::writeXMMop( 0x66, 0xf7, to, from ); }

	// xPMOVMSKB:
	// Creates a mask made up of the most significant bit of each byte of the source 
	// operand and stores the result in the low byte or word of the destination operand.
	// Upper bits of the destination are cleared to zero.
	//
	// When operating on a 64-bit (MMX) source, the byte mask is 8 bits; when operating on
	// 128-bit (SSE) source, the byte mask is 16-bits.
	//
	template< typename T >
	static __forceinline void xPMOVMSKB( const xRegister32& to, const xRegisterSIMD<T>& from )	{ Internal::writeXMMop( 0x66, 0xd7, to, from ); }
	
	// ------------------------------------------------------------------------
	
	extern void xMOVQ( const xRegisterMMX& to, const xRegisterMMX& from );
	extern void xMOVQ( const xRegisterMMX& to, const xRegisterSSE& from );
	extern void xMOVQ( const xRegisterSSE& to, const xRegisterMMX& from );

	extern void xMOVQ( void* dest, const xRegisterSSE& from );
	extern void xMOVQ( const ModSibBase& dest, const xRegisterSSE& from );
	extern void xMOVQ( void* dest, const xRegisterMMX& from );
	extern void xMOVQ( const ModSibBase& dest, const xRegisterMMX& from );
	extern void xMOVQ( const xRegisterMMX& to, const void* src );
	extern void xMOVQ( const xRegisterMMX& to, const ModSibBase& src );

	extern void xMOVQZX( const xRegisterSSE& to, const void* src );
	extern void xMOVQZX( const xRegisterSSE& to, const ModSibBase& src );
	extern void xMOVQZX( const xRegisterSSE& to, const xRegisterSSE& from );
	
	extern void xMOVSS( const xRegisterSSE& to, const xRegisterSSE& from );
	extern void xMOVSS( const void* to, const xRegisterSSE& from );
	extern void xMOVSS( const ModSibBase& to, const xRegisterSSE& from );
	extern void xMOVSD( const xRegisterSSE& to, const xRegisterSSE& from );
	extern void xMOVSD( const void* to, const xRegisterSSE& from );
	extern void xMOVSD( const ModSibBase& to, const xRegisterSSE& from );

	extern void xMOVSSZX( const xRegisterSSE& to, const void* from );
	extern void xMOVSSZX( const xRegisterSSE& to, const ModSibBase& from );
	extern void xMOVSDZX( const xRegisterSSE& to, const void* from );
	extern void xMOVSDZX( const xRegisterSSE& to, const ModSibBase& from );

	extern void xMOVNTDQA( const xRegisterSSE& to, const void* from );
	extern void xMOVNTDQA( const xRegisterSSE& to, const ModSibBase& from );
	extern void xMOVNTDQ( void* to, const xRegisterSSE& from );
	extern void xMOVNTDQA( const ModSibBase& to, const xRegisterSSE& from );

	extern void xMOVNTPD( void* to, const xRegisterSSE& from );
	extern void xMOVNTPD( const ModSibBase& to, const xRegisterSSE& from );
	extern void xMOVNTPS( void* to, const xRegisterSSE& from );
	extern void xMOVNTPS( const ModSibBase& to, const xRegisterSSE& from );
	extern void xMOVNTQ( void* to, const xRegisterMMX& from );
	extern void xMOVNTQ( const ModSibBase& to, const xRegisterMMX& from );

	// ------------------------------------------------------------------------

	extern const Internal::MovapsImplAll<0, 0x28, 0x29> xMOVAPS;
	extern const Internal::MovapsImplAll<0, 0x10, 0x11> xMOVUPS;

	extern const Internal::MovapsImplAll<0x66, 0x28, 0x29> xMOVAPD;
	extern const Internal::MovapsImplAll<0x66, 0x10, 0x11> xMOVUPD;

#ifdef ALWAYS_USE_MOVAPS
	extern const Internal::MovapsImplAll<0x66, 0x6f, 0x7f> xMOVDQA;
	extern const Internal::MovapsImplAll<0xf3, 0x6f, 0x7f> xMOVDQU;
#else
	extern const Internal::MovapsImplAll<0, 0x28, 0x29> xMOVDQA;
	extern const Internal::MovapsImplAll<0, 0x10, 0x11> xMOVDQU;
#endif

	extern const Internal::MovhlImpl_RtoR<0x16> xMOVLH;
	extern const Internal::MovhlImpl_RtoR<0x12> xMOVHL;

	extern const Internal::MovhlImplAll<0x16> xMOVH;
	extern const Internal::MovhlImplAll<0x12> xMOVL;

	// ------------------------------------------------------------------------
	
	extern const Internal::SimdImpl_PackedLogic<0xdb> xPAND;
	extern const Internal::SimdImpl_PackedLogic<0xdf> xPANDN;
	extern const Internal::SimdImpl_PackedLogic<0xeb> xPOR;
	extern const Internal::SimdImpl_PackedLogic<0xef> xPXOR;

	extern const Internal::SimdImpl_AndNot<0x55> xANDN;

	extern const Internal::SimdImpl_SS_SD<0x66,0x2e> xUCOMI;
	extern const Internal::SimdImpl_rSqrt<0x53> xRCP;
	extern const Internal::SimdImpl_rSqrt<0x52> xRSQRT;
	extern const Internal::SimdImpl_Sqrt<0x51> xSQRT;
	
	extern const Internal::SimdImpl_PSPD_SSSD<0x5f> xMAX;
	extern const Internal::SimdImpl_PSPD_SSSD<0x5d> xMIN;
	extern const Internal::SimdImpl_Shuffle<0xc6> xSHUF;

	// ------------------------------------------------------------------------
	
	extern const Internal::SimdImpl_Compare<SSE2_Equal>		xCMPEQ;
	extern const Internal::SimdImpl_Compare<SSE2_Less>		xCMPLT;
	extern const Internal::SimdImpl_Compare<SSE2_LessOrEqual>	xCMPLE;
	extern const Internal::SimdImpl_Compare<SSE2_Unordered>	xCMPUNORD;
	extern const Internal::SimdImpl_Compare<SSE2_NotEqual>	xCMPNE;
	extern const Internal::SimdImpl_Compare<SSE2_NotLess>		xCMPNLT;
	extern const Internal::SimdImpl_Compare<SSE2_NotLessOrEqual> xCMPNLE;
	extern const Internal::SimdImpl_Compare<SSE2_Ordered>		xCMPORD;

	// ------------------------------------------------------------------------
	// OMG Evil.  I went cross-eyed an hour ago doing this.
	//
	extern const Internal::SimdImpl_DestRegStrict<0xf3,0xe6,xRegisterSSE,xRegisterSSE,u64>	xCVTDQ2PD;
	extern const Internal::SimdImpl_DestRegStrict<0x00,0x5b,xRegisterSSE,xRegisterSSE,u128>	xCVTDQ2PS;

	extern const Internal::SimdImpl_DestRegStrict<0xf2,0xe6,xRegisterSSE,xRegisterSSE,u128>	xCVTPD2DQ;
	extern const Internal::SimdImpl_DestRegStrict<0x66,0x2d,xRegisterMMX,xRegisterSSE,u128>	xCVTPD2PI;
	extern const Internal::SimdImpl_DestRegStrict<0x66,0x5a,xRegisterSSE,xRegisterSSE,u128>	xCVTPD2PS;

	extern const Internal::SimdImpl_DestRegStrict<0x66,0x2a,xRegisterSSE,xRegisterMMX,u64>	xCVTPI2PD;
	extern const Internal::SimdImpl_DestRegStrict<0x00,0x2a,xRegisterSSE,xRegisterMMX,u64>	xCVTPI2PS;

	extern const Internal::SimdImpl_DestRegStrict<0x66,0x5b,xRegisterSSE,xRegisterSSE,u128>	xCVTPS2DQ;
	extern const Internal::SimdImpl_DestRegStrict<0x00,0x5a,xRegisterSSE,xRegisterSSE,u64>	xCVTPS2PD;
	extern const Internal::SimdImpl_DestRegStrict<0x00,0x2d,xRegisterMMX,xRegisterSSE,u64>	xCVTPS2PI;

	extern const Internal::SimdImpl_DestRegStrict<0xf2,0x2d,xRegister32, xRegisterSSE,u64>	xCVTSD2SI;
	extern const Internal::SimdImpl_DestRegStrict<0xf2,0x5a,xRegisterSSE,xRegisterSSE,u64>	xCVTSD2SS;
	extern const Internal::SimdImpl_DestRegStrict<0xf2,0x2a,xRegisterMMX,xRegister32, u32>	xCVTSI2SD;
	extern const Internal::SimdImpl_DestRegStrict<0xf3,0x2a,xRegisterSSE,xRegister32, u32>	xCVTSI2SS;

	extern const Internal::SimdImpl_DestRegStrict<0xf3,0x5a,xRegisterSSE,xRegisterSSE,u32>	xCVTSS2SD;
	extern const Internal::SimdImpl_DestRegStrict<0xf3,0x2d,xRegister32, xRegisterSSE,u32>	xCVTSS2SI;

	extern const Internal::SimdImpl_DestRegStrict<0x66,0xe6,xRegisterSSE,xRegisterSSE,u128>	xCVTTPD2DQ;
	extern const Internal::SimdImpl_DestRegStrict<0x66,0x2c,xRegisterMMX,xRegisterSSE,u128>	xCVTTPD2PI;
	extern const Internal::SimdImpl_DestRegStrict<0xf3,0x5b,xRegisterSSE,xRegisterSSE,u128>	xCVTTPS2DQ;
	extern const Internal::SimdImpl_DestRegStrict<0x00,0x2c,xRegisterMMX,xRegisterSSE,u64>	xCVTTPS2PI;

	extern const Internal::SimdImpl_DestRegStrict<0xf2,0x2c,xRegister32, xRegisterSSE,u64>	xCVTTSD2SI;
	extern const Internal::SimdImpl_DestRegStrict<0xf3,0x2c,xRegister32, xRegisterSSE,u32>	xCVTTSS2SI;
	
	// ------------------------------------------------------------------------
	
	extern const Internal::SimdImpl_ShiftAll<0xd0, 2> xPSRL;
	extern const Internal::SimdImpl_ShiftAll<0xf0, 6> xPSLL;
	extern const Internal::SimdImpl_ShiftWithoutQ<0xe0, 4> xPSRA;

	extern const Internal::SimdImpl_AddSub<0xdc, 0xd4> xPADD;
	extern const Internal::SimdImpl_AddSub<0xd8, 0xfb> xPSUB;
	extern const Internal::SimdImpl_PMinMax<0xde,0x3c> xPMAX;
	extern const Internal::SimdImpl_PMinMax<0xda,0x38> xPMIN;

	extern const Internal::SimdImpl_PMul xPMUL;
	extern const Internal::SimdImpl_PCompare xPCMP;
	extern const Internal::SimdImpl_PShuffle xPSHUF;
	extern const Internal::SimdImpl_PUnpack xPUNPCK;
	extern const Internal::SimdImpl_Unpack xUNPCK;
	extern const Internal::SimdImpl_Pack xPACK;
}

