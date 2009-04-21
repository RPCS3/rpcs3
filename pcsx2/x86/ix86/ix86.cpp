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
 * ix86 core v0.9.0
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

#include "PrecompiledHeader.h"

#include "System.h"
#include "ix86_internal.h"

// ------------------------------------------------------------------------
// Notes on Thread Local Storage:
//  * TLS is pretty simple, and "just works" from a programmer perspective, with only
//    some minor additional computational overhead (see performance notes below).
//
//  * MSVC and GCC handle TLS differently internally, but behavior to the programmer is
//    generally identical.
//
// Performance Considerations:
//  * GCC's implementation involves an extra dereference from normal storage.
//
//  * MSVC's implementation involves *two* extra dereferences from normal storage because
//    it has to look up the TLS heap pointer from the Windows Thread Storage Area.  (in
//    generated ASM code, this dereference is denoted by access to the fs:[2ch] address).
//
//  * However, in either case, the optimizer usually optimizes it to a register so the
//    extra overhead is minimal over a series of instructions.  (Note!!  the Full Opt-
//    imization [/Ox] option effectively disables TLS optimizations in MSVC, causing
//    generally significant code bloat).
//


__threadlocal u8  *x86Ptr;
__threadlocal u8  *j8Ptr[32];
__threadlocal u32 *j32Ptr[32];

__threadlocal XMMSSEType g_xmmtypes[iREGCNT_XMM] = { XMMT_INT };

namespace x86Emitter {

const xAddressIndexerBase ptr;
const xAddressIndexer<u128> ptr128;
const xAddressIndexer<u64> ptr64;
const xAddressIndexer<u32> ptr32;
const xAddressIndexer<u16> ptr16;
const xAddressIndexer<u8> ptr8;	

// ------------------------------------------------------------------------

template< typename OperandType > const xRegister<OperandType> xRegister<OperandType>::Empty;
const xAddressReg xAddressReg::Empty;

const xRegisterSSE
	xmm0( 0 ), xmm1( 1 ),
	xmm2( 2 ), xmm3( 3 ),
	xmm4( 4 ), xmm5( 5 ),
	xmm6( 6 ), xmm7( 7 );

const xRegisterMMX
	mm0( 0 ), mm1( 1 ),
	mm2( 2 ), mm3( 3 ),
	mm4( 4 ), mm5( 5 ),
	mm6( 6 ), mm7( 7 );

const xRegister32
	eax( 0 ), ebx( 3 ),
	ecx( 1 ), edx( 2 ),
	esi( 6 ), edi( 7 ),
	ebp( 5 ), esp( 4 );

const xRegister16
	ax( 0 ), bx( 3 ),
	cx( 1 ), dx( 2 ),
	si( 6 ), di( 7 ),
	bp( 5 ), sp( 4 );

const xRegister8
	al( 0 ),
	dl( 2 ), bl( 3 ),
	ah( 4 ), ch( 5 ),
	dh( 6 ), bh( 7 );
	
const xRegisterCL cl;

namespace Internal
{
	// Performance note: VC++ wants to use byte/word register form for the following
	// ModRM/SibSB constructors when we use xWrite<u8>, and furthermore unrolls the
	// the shift using a series of ADDs for the following results:
	//   add cl,cl
	//   add cl,cl
	//   add cl,cl
	//   or  cl,bl
	//   add cl,cl
	//  ... etc.
	//
	// This is unquestionably bad optimization by Core2 standard, an generates tons of
	// register aliases and false dependencies. (although may have been ideal for early-
	// brand P4s with a broken barrel shifter?).  The workaround is to do our own manual
	// x86Ptr access and update using a u32 instead of u8.  Thanks to little endianness,
	// the same end result is achieved and no false dependencies are generated.  The draw-
	// back is that it clobbers 3 bytes past the end of the write, which could cause a 
	// headache for someone who himself is doing some kind of headache-inducing amount of
	// recompiler SMC.  So we don't do a work-around, and just hope for the compiler to
	// stop sucking someday instead. :)
	//
	// (btw, I know this isn't a critical performance item by any means, but it's
	//  annoying simply because it *should* be an easy thing to optimize)

	__forceinline void ModRM( uint mod, uint reg, uint rm )
	{
		xWrite<u8>( (mod << 6) | (reg << 3) | rm );
	}

	__forceinline void ModRM_Direct( uint reg, uint rm )
	{
		ModRM( Mod_Direct, reg, rm );
	}

	__forceinline void SibSB( u32 ss, u32 index, u32 base )
	{
		xWrite<u8>( (ss << 6) | (index << 3) | base );
	}

	__forceinline void xWriteDisp( int regfield, s32 displacement )
	{
		ModRM( 0, regfield, ModRm_UseDisp32 );
		xWrite<s32>( displacement );
	}

	__forceinline void xWriteDisp( int regfield, const void* address )
	{
		xWriteDisp( regfield, (s32)address );
	}

	// ------------------------------------------------------------------------
	// returns TRUE if this instruction requires SIB to be encoded, or FALSE if the
	// instruction ca be encoded as ModRm alone.
	static __forceinline bool NeedsSibMagic( const ModSibBase& info )
	{
		// no registers? no sibs!
		// (ModSibBase::Reduce always places a register in Index, and optionally leaves
		// Base empty if only register is specified)
		if( info.Index.IsEmpty() ) return false;

		// A scaled register needs a SIB
		if( info.Scale != 0 ) return true;

		// two registers needs a SIB
		if( !info.Base.IsEmpty() ) return true;

		return false;
	}

	//////////////////////////////////////////////////////////////////////////////////////////
	// Conditionally generates Sib encoding information!
	//
	// regfield - register field to be written to the ModRm.  This is either a register specifier
	//   or an opcode extension.  In either case, the instruction determines the value for us.
	//
	void EmitSibMagic( uint regfield, const ModSibBase& info )
	{
		jASSUME( regfield < 8 );

		int displacement_size = (info.Displacement == 0) ? 0 : 
			( ( info.IsByteSizeDisp() ) ? 1 : 2 );

		if( !NeedsSibMagic( info ) )
		{
			// Use ModRm-only encoding, with the rm field holding an index/base register, if
			// one has been specified.  If neither register is specified then use Disp32 form,
			// which is encoded as "EBP w/o displacement" (which is why EBP must always be
			// encoded *with* a displacement of 0, if it would otherwise not have one).

			if( info.Index.IsEmpty() )
			{
				xWriteDisp( regfield, info.Displacement );
				return;
			}
			else
			{
				if( info.Index == ebp && displacement_size == 0 )
					displacement_size = 1;		// forces [ebp] to be encoded as [ebp+0]!

				ModRM( displacement_size, regfield, info.Index.Id );
			}
		}
		else
		{
			// In order to encode "just" index*scale (and no base), we have to encode
			// it as a special [index*scale + displacement] form, which is done by
			// specifying EBP as the base register and setting the displacement field
			// to zero. (same as ModRm w/o SIB form above, basically, except the
			// ModRm_UseDisp flag is specified in the SIB instead of the ModRM field).

			if( info.Base.IsEmpty() )
			{
				ModRM( 0, regfield, ModRm_UseSib );
				SibSB( info.Scale, info.Index.Id, ModRm_UseDisp32 );
				xWrite<s32>( info.Displacement );
				return;
			}
			else
			{
				if( info.Base == ebp && displacement_size == 0 )
					displacement_size = 1;		// forces [ebp] to be encoded as [ebp+0]!

				ModRM( displacement_size, regfield, ModRm_UseSib );
				SibSB( info.Scale, info.Index.Id, info.Base.Id );
			}
		}

		if( displacement_size != 0 )
		{
			if( displacement_size == 1 )
				xWrite<s8>( info.Displacement );
			else
				xWrite<s32>( info.Displacement );
		}
	}
}

using namespace Internal;

const MovImplAll xMOV;
const TestImplAll xTEST;

const xImpl_G1Logic<G1Type_AND,0x54> xAND;
const xImpl_G1Logic<G1Type_OR,0x56>  xOR;
const xImpl_G1Logic<G1Type_XOR,0x57> xXOR;

const xImpl_G1Arith<G1Type_ADD,0x58> xADD;
const xImpl_G1Arith<G1Type_SUB,0x5c> xSUB;

const xImpl_Group1<G1Type_ADC> xADC;
const xImpl_Group1<G1Type_SBB> xSBB;
const xImpl_G1Compare xCMP;

const Group2ImplAll<G2Type_ROL> xROL;
const Group2ImplAll<G2Type_ROR> xROR;
const Group2ImplAll<G2Type_RCL> xRCL;
const Group2ImplAll<G2Type_RCR> xRCR;
const Group2ImplAll<G2Type_SHL> xSHL;
const Group2ImplAll<G2Type_SHR> xSHR;
const Group2ImplAll<G2Type_SAR> xSAR;

const Group3ImplAll<G3Type_NOT> xNOT;
const Group3ImplAll<G3Type_NEG> xNEG;
const Group3ImplAll<G3Type_MUL> xUMUL;
const Group3ImplAll<G3Type_DIV> xUDIV;
const xImpl_Group3<G3Type_iDIV,0x5e> xDIV;
const xImpl_iMul xMUL;

const IncDecImplAll<false> xINC;
const IncDecImplAll<true>  xDEC;

const MovExtendImplAll<false> xMOVZX;
const MovExtendImplAll<true>  xMOVSX;

const DwordShiftImplAll<false> xSHLD;
const DwordShiftImplAll<true>  xSHRD;

const Group8ImplAll<G8Type_BT> xBT;
const Group8ImplAll<G8Type_BTR> xBTR;
const Group8ImplAll<G8Type_BTS> xBTS;
const Group8ImplAll<G8Type_BTC> xBTC;

const BitScanImplAll<false> xBSF;
const BitScanImplAll<true> xBSR;

// ------------------------------------------------------------------------
const CMovImplGeneric xCMOV;

const CMovImplAll<Jcc_Above>			xCMOVA;
const CMovImplAll<Jcc_AboveOrEqual>		xCMOVAE;
const CMovImplAll<Jcc_Below>			xCMOVB;
const CMovImplAll<Jcc_BelowOrEqual>		xCMOVBE;

const CMovImplAll<Jcc_Greater>			xCMOVG;
const CMovImplAll<Jcc_GreaterOrEqual>	xCMOVGE;
const CMovImplAll<Jcc_Less>				xCMOVL;
const CMovImplAll<Jcc_LessOrEqual>		xCMOVLE;

const CMovImplAll<Jcc_Zero>				xCMOVZ;
const CMovImplAll<Jcc_Equal>			xCMOVE;
const CMovImplAll<Jcc_NotZero>			xCMOVNZ;
const CMovImplAll<Jcc_NotEqual>			xCMOVNE;

const CMovImplAll<Jcc_Overflow>			xCMOVO;
const CMovImplAll<Jcc_NotOverflow>		xCMOVNO;
const CMovImplAll<Jcc_Carry>			xCMOVC;
const CMovImplAll<Jcc_NotCarry>			xCMOVNC;

const CMovImplAll<Jcc_Signed>			xCMOVS;
const CMovImplAll<Jcc_Unsigned>			xCMOVNS;
const CMovImplAll<Jcc_ParityEven>		xCMOVPE;
const CMovImplAll<Jcc_ParityOdd>		xCMOVPO;

// ------------------------------------------------------------------------
const SetImplGeneric xSET;

const SetImplAll<Jcc_Above>				xSETA;
const SetImplAll<Jcc_AboveOrEqual>		xSETAE;
const SetImplAll<Jcc_Below>				xSETB;
const SetImplAll<Jcc_BelowOrEqual>		xSETBE;

const SetImplAll<Jcc_Greater>			xSETG;
const SetImplAll<Jcc_GreaterOrEqual>	xSETGE;
const SetImplAll<Jcc_Less>				xSETL;
const SetImplAll<Jcc_LessOrEqual>		xSETLE;

const SetImplAll<Jcc_Zero>				xSETZ;
const SetImplAll<Jcc_Equal>				xSETE;
const SetImplAll<Jcc_NotZero>			xSETNZ;
const SetImplAll<Jcc_NotEqual>			xSETNE;

const SetImplAll<Jcc_Overflow>			xSETO;
const SetImplAll<Jcc_NotOverflow>		xSETNO;
const SetImplAll<Jcc_Carry>				xSETC;
const SetImplAll<Jcc_NotCarry>			xSETNC;

const SetImplAll<Jcc_Signed>			xSETS;
const SetImplAll<Jcc_Unsigned>			xSETNS;
const SetImplAll<Jcc_ParityEven>		xSETPE;
const SetImplAll<Jcc_ParityOdd>			xSETPO;


// ------------------------------------------------------------------------
// Assigns the current emitter buffer target address.
// This is provided instead of using x86Ptr directly, since we may in the future find
// a need to change the storage class system for the x86Ptr 'under the hood.'
__emitinline void iSetPtr( void* ptr ) 
{
	x86Ptr = (u8*)ptr;
}

// ------------------------------------------------------------------------
// Retrieves the current emitter buffer target address.
// This is provided instead of using x86Ptr directly, since we may in the future find
// a need to change the storage class system for the x86Ptr 'under the hood.'
__emitinline u8* iGetPtr()
{
	return x86Ptr;
}

// ------------------------------------------------------------------------
__emitinline void iAlignPtr( uint bytes ) 
{
	// forward align
	x86Ptr = (u8*)( ( (uptr)x86Ptr + bytes - 1) & ~(bytes - 1) );
}

// ------------------------------------------------------------------------
__emitinline void iAdvancePtr( uint bytes )
{
	if( IsDevBuild )
	{
		// common debugger courtesy: advance with INT3 as filler.
		for( uint i=0; i<bytes; i++ )
			xWrite<u8>( 0xcc );
	}
	else
		x86Ptr += bytes;
}

// ------------------------------------------------------------------------
// Generates a 'reduced' ModSib form, which has valid Base, Index, and Scale values.
// Necessary because by default ModSib compounds registers into Index when possible.
//
// If the ModSib is in illegal form ([Base + Index*5] for example) then an assertion
// followed by an InvalidParameter Exception will be tossed around in haphazard 
// fashion.
//
// Optimization Note: Currently VC does a piss poor job of inlining this, even though
// constant propagation *should* resove it to little or no code (VC's constprop fails
// on C++ class initializers).  There is a work around [using array initializers instead]
// but it's too much trouble for code that isn't performance critical anyway. 
// And, with luck, maybe VC10 will optimize it better and make it a non-issue. :D
//
void ModSibBase::Reduce()
{
	if( Index.IsStackPointer() )
	{
		// esp cannot be encoded as the index, so move it to the Base, if possible.
		// note: intentionally leave index assigned to esp also (generates correct
		// encoding later, since ESP cannot be encoded 'alone')

		jASSUME( Scale == 0 );		// esp can't have an index modifier!
		jASSUME( Base.IsEmpty() );	// base must be empty or else!

		Base = Index;
		return;
	}

	// If no index reg, then load the base register into the index slot.
	if( Index.IsEmpty() )
	{
		Index = Base;
		Scale = 0;
		if( !Base.IsStackPointer() )	// prevent ESP from being encoded 'alone'
			Base = xAddressReg::Empty;
		return;
	}


	// The Scale has a series of valid forms, all shown here:
	
	switch( Scale )
	{
		case 0: break;
		case 1: Scale = 0; break;
		case 2: Scale = 1; break;

		case 3:				// becomes [reg*2+reg]
			jASSUME( Base.IsEmpty() );
			Base = Index;
			Scale = 1;
		break;
		
		case 4: Scale = 2; break;

		case 5:				// becomes [reg*4+reg]
			jASSUME( Base.IsEmpty() );
			Base = Index;
			Scale = 2;
		break;
		
		case 6:				// invalid!
			assert( false );
		break;
		
		case 7:				// so invalid!
			assert( false );
		break;
		
		case 8: Scale = 3; break;
		case 9:				// becomes [reg*8+reg]
			jASSUME( Base.IsEmpty() );
			Base = Index;
			Scale = 3;
		break;
	}
}


// ------------------------------------------------------------------------
// Internal implementation of EmitSibMagic which has been custom tailored
// to optimize special forms of the Lea instructions accordingly, such
// as when a LEA can be replaced with a "MOV reg,imm" or "MOV reg,reg".
//
// preserve_flags - set to ture to disable use of SHL on [Index*Base] forms
// of LEA, which alters flags states.
//
template< typename OperandType >
static void EmitLeaMagic( xRegister<OperandType> to, const ModSibBase& src, bool preserve_flags )
{
	typedef xRegister<OperandType> ToReg;
	
	int displacement_size = (src.Displacement == 0) ? 0 : 
		( ( src.IsByteSizeDisp() ) ? 1 : 2 );

	// See EmitSibMagic for commenting on SIB encoding.

	if( !NeedsSibMagic( src ) )
	{
		// LEA Land: means we have either 1-register encoding or just an offset.
		// offset is encodable as an immediate MOV, and a register is encodable
		// as a register MOV.

		if( src.Index.IsEmpty() )
		{
			xMOV( to, src.Displacement );
			return;
		}
		else if( displacement_size == 0 )
		{
			xMOV( to, ToReg( src.Index.Id ) );
			return;
		}
		else
		{
			if( !preserve_flags )
			{
				// encode as MOV and ADD combo.  Make sure to use the immediate on the
				// ADD since it can encode as an 8-bit sign-extended value.
				
				xMOV( to, ToReg( src.Index.Id ) );
				xADD( to, src.Displacement );
				return;
			}
			else
			{
				// note: no need to do ebp+0 check since we encode all 0 displacements as
				// register assignments above (via MOV)

				xWrite<u8>( 0x8d );
				ModRM( displacement_size, to.Id, src.Index.Id );
			}
		}
	}
	else
	{
		if( src.Base.IsEmpty() )
		{
			if( !preserve_flags && (displacement_size == 0) )
			{
				// Encode [Index*Scale] as a combination of Mov and Shl.
				// This is more efficient because of the bloated LEA format which requires
				// a 32 bit displacement, and the compact nature of the alternative.
				//
				// (this does not apply to older model P4s with the broken barrel shifter,
				//  but we currently aren't optimizing for that target anyway).

				xMOV( to, ToReg( src.Index.Id ) );
				xSHL( to, src.Scale );
				return;
			}
			xWrite<u8>( 0x8d );
			ModRM( 0, to.Id, ModRm_UseSib );
			SibSB( src.Scale, src.Index.Id, ModRm_UseDisp32 );
			xWrite<u32>( src.Displacement );
			return;
		}
		else
		{
			if( src.Scale == 0 )
			{
				if( !preserve_flags )
				{
					if( src.Index == esp )
					{
						// ESP is not encodable as an index (ix86 ignores it), thus:
						xMOV( to, ToReg( src.Base.Id ) );	// will do the trick!
						if( src.Displacement ) xADD( to, src.Displacement );
						return;
					}
					else if( src.Displacement == 0 )
					{
						xMOV( to, ToReg( src.Base.Id ) );
						xADD( to, ToReg( src.Index.Id ) );
						return;
					}
				}
				else if( (src.Index == esp) && (src.Displacement == 0) )
				{
					// special case handling of ESP as Index, which is replaceable with
					// a single MOV even when preserve_flags is set! :D
					
					xMOV( to, ToReg( src.Base.Id ) );
					return;
				}
			}

			if( src.Base == ebp && displacement_size == 0 )
				displacement_size = 1;		// forces [ebp] to be encoded as [ebp+0]!

			xWrite<u8>( 0x8d );
			ModRM( displacement_size, to.Id, ModRm_UseSib );
			SibSB( src.Scale, src.Index.Id, src.Base.Id );
		}
	}

	if( displacement_size != 0 )
	{
		if( displacement_size == 1 )
			xWrite<s8>( src.Displacement );
		else
			xWrite<s32>( src.Displacement );
	}
}

__emitinline void xLEA( xRegister32 to, const ModSibBase& src, bool preserve_flags )
{
	EmitLeaMagic( to, src, preserve_flags );
}


__emitinline void xLEA( xRegister16 to, const ModSibBase& src, bool preserve_flags )
{
	write8( 0x66 );
	EmitLeaMagic( to, src, preserve_flags );
}


//////////////////////////////////////////////////////////////////////////////////////////
// Push / Pop Emitters
//
// Note: pushad/popad implementations are intentionally left out.  The instructions are
// invalid in x64, and are super slow on x32.  Use multiple Push/Pop instructions instead.

__emitinline void xPOP( const ModSibBase& from )
{
	xWrite<u8>( 0x8f );
	EmitSibMagic( 0, from );
}

__emitinline void xPUSH( const ModSibBase& from )
{
	xWrite<u8>( 0xff );
	EmitSibMagic( 6, from );
}

//////////////////////////////////////////////////////////////////////////////////////////
//
__emitinline void xBSWAP( const xRegister32& to )
{
	write8( 0x0F );
	write8( 0xC8 | to.Id );
}


//////////////////////////////////////////////////////////////////////////////////////////
// MMX / XMM Instructions
// (these will get put in their own file later)

// If the upper 8 bits of opcode are zero, the opcode is treated as a u8.
// The upper bits are non-zero, the opcode is assumed 16 bit (and the upper bits are checked aginst
// 0x38, which is the only valid high word for 16 bit opcodes as such)
__emitinline void Internal::SimdPrefix( u8 prefix, u16 opcode )
{
	if( prefix != 0 )
	{
		if( (opcode & 0xff00) != 0 )
		{
			jASSUME( (opcode & 0xff00) == 0x3800 );
			xWrite<u32>( (opcode<<16) | (0x0f00 | prefix) );
		}
		else
		{
			xWrite<u16>( 0x0f00 | prefix );
			xWrite<u8>( opcode );
		}
	}
	else
	{
		if( (opcode & 0xff00) != 0 )
		{
			jASSUME( (opcode & 0xff00) == 0x3800 );
			xWrite<u16>( opcode );
		}
		else
			xWrite<u16>( (opcode<<8) | 0x0f );
	}
}

const MovapsImplAll< 0, 0x28, 0x29 > xMOVAPS; 
const MovapsImplAll< 0, 0x10, 0x11 > xMOVUPS;
const MovapsImplAll< 0x66, 0x28, 0x29 > xMOVAPD;
const MovapsImplAll< 0x66, 0x10, 0x11 > xMOVUPD;

#ifdef ALWAYS_USE_MOVAPS
const MovapsImplAll< 0x66, 0x6f, 0x7f > xMOVDQA;
const MovapsImplAll< 0xf3, 0x6f, 0x7f > xMOVDQU;
#else
const MovapsImplAll< 0, 0x28, 0x29 > xMOVDQA;
const MovapsImplAll< 0, 0x10, 0x11 > xMOVDQU;
#endif

const MovhlImplAll<0x16> xMOVH;
const MovhlImplAll<0x12> xMOVL;
const MovhlImpl_RtoR<0x16> xMOVLH;
const MovhlImpl_RtoR<0x12> xMOVHL;

const SimdImpl_PackedLogic<0xdb> xPAND;
const SimdImpl_PackedLogic<0xdf> xPANDN;
const SimdImpl_PackedLogic<0xeb> xPOR;
const SimdImpl_PackedLogic<0xef> xPXOR;

const SimdImpl_AndNot<0x55> xANDN;

const SimdImpl_SS_SD<0x66,0x2e> xUCOMI;
const SimdImpl_rSqrt<0x53> xRCP;
const SimdImpl_rSqrt<0x52> xRSQRT;
const SimdImpl_Sqrt<0x51> xSQRT;

const SimdImpl_PSPD_SSSD<0x5f> xMAX;
const SimdImpl_PSPD_SSSD<0x5d> xMIN;
const SimdImpl_Shuffle<0xc6> xSHUF;

// ------------------------------------------------------------------------

const SimdImpl_Compare<SSE2_Equal>		xCMPEQ;
const SimdImpl_Compare<SSE2_Less>			xCMPLT;
const SimdImpl_Compare<SSE2_LessOrEqual>	xCMPLE;
const SimdImpl_Compare<SSE2_Unordered>	xCMPUNORD;
const SimdImpl_Compare<SSE2_NotEqual>		xCMPNE;
const SimdImpl_Compare<SSE2_NotLess>		xCMPNLT;
const SimdImpl_Compare<SSE2_NotLessOrEqual> xCMPNLE;
const SimdImpl_Compare<SSE2_Ordered>		xCMPORD;

// ------------------------------------------------------------------------
// SSE Conversion Operations, as looney as they are.
// 
// These enforce pointer strictness for Indirect forms, due to the otherwise completely confusing
// nature of the functions.  (so if a function expects an m32, you must use (u32*) or ptr32[]).
//
const SimdImpl_DestRegStrict<0xf3,0xe6,xRegisterSSE,xRegisterSSE,u64>		xCVTDQ2PD;
const SimdImpl_DestRegStrict<0x00,0x5b,xRegisterSSE,xRegisterSSE,u128>		xCVTDQ2PS;

const SimdImpl_DestRegStrict<0xf2,0xe6,xRegisterSSE,xRegisterSSE,u128>		xCVTPD2DQ;
const SimdImpl_DestRegStrict<0x66,0x2d,xRegisterMMX,xRegisterSSE,u128>		xCVTPD2PI;
const SimdImpl_DestRegStrict<0x66,0x5a,xRegisterSSE,xRegisterSSE,u128>		xCVTPD2PS;

const SimdImpl_DestRegStrict<0x66,0x2a,xRegisterSSE,xRegisterMMX,u64>		xCVTPI2PD;
const SimdImpl_DestRegStrict<0x00,0x2a,xRegisterSSE,xRegisterMMX,u64>		xCVTPI2PS;

const SimdImpl_DestRegStrict<0x66,0x5b,xRegisterSSE,xRegisterSSE,u128>		xCVTPS2DQ;
const SimdImpl_DestRegStrict<0x00,0x5a,xRegisterSSE,xRegisterSSE,u64>		xCVTPS2PD;
const SimdImpl_DestRegStrict<0x00,0x2d,xRegisterMMX,xRegisterSSE,u64>		xCVTPS2PI;

const SimdImpl_DestRegStrict<0xf2,0x2d,xRegister32, xRegisterSSE,u64>		xCVTSD2SI;
const SimdImpl_DestRegStrict<0xf2,0x5a,xRegisterSSE,xRegisterSSE,u64>		xCVTSD2SS;
const SimdImpl_DestRegStrict<0xf2,0x2a,xRegisterMMX,xRegister32, u32>		xCVTSI2SD;
const SimdImpl_DestRegStrict<0xf3,0x2a,xRegisterSSE,xRegister32, u32>		xCVTSI2SS;

const SimdImpl_DestRegStrict<0xf3,0x5a,xRegisterSSE,xRegisterSSE,u32>		xCVTSS2SD;
const SimdImpl_DestRegStrict<0xf3,0x2d,xRegister32, xRegisterSSE,u32>		xCVTSS2SI;

const SimdImpl_DestRegStrict<0x66,0xe6,xRegisterSSE,xRegisterSSE,u128>		xCVTTPD2DQ;
const SimdImpl_DestRegStrict<0x66,0x2c,xRegisterMMX,xRegisterSSE,u128>		xCVTTPD2PI;
const SimdImpl_DestRegStrict<0xf3,0x5b,xRegisterSSE,xRegisterSSE,u128>		xCVTTPS2DQ;
const SimdImpl_DestRegStrict<0x00,0x2c,xRegisterMMX,xRegisterSSE,u64>		xCVTTPS2PI;

const SimdImpl_DestRegStrict<0xf2,0x2c,xRegister32, xRegisterSSE,u64>		xCVTTSD2SI;
const SimdImpl_DestRegStrict<0xf3,0x2c,xRegister32, xRegisterSSE,u32>		xCVTTSS2SI;

// ------------------------------------------------------------------------

const SimdImpl_ShiftAll<0xd0, 2> xPSRL;
const SimdImpl_ShiftAll<0xf0, 6> xPSLL;
const SimdImpl_ShiftWithoutQ<0xe0, 4> xPSRA;

const SimdImpl_AddSub<0xdc, 0xd4> xPADD;
const SimdImpl_AddSub<0xd8, 0xfb> xPSUB;
const SimdImpl_PMinMax<0xde,0x3c> xPMAX;
const SimdImpl_PMinMax<0xda,0x38> xPMIN;

const SimdImpl_PMul xPMUL;
const SimdImpl_PCompare xPCMP;
const SimdImpl_PShuffle xPSHUF;
const SimdImpl_PUnpack xPUNPCK;
const SimdImpl_Unpack xUNPCK;
const SimdImpl_Pack xPACK;


//////////////////////////////////////////////////////////////////////////////////////////
//


// Moves from XMM to XMM, with the *upper 64 bits* of the destination register
// being cleared to zero.
__forceinline void xMOVQZX( const xRegisterSSE& to, const xRegisterSSE& from )	{ writeXMMop( 0xf3, 0x7e, to, from ); }

// Moves from XMM to XMM, with the *upper 64 bits* of the destination register
// being cleared to zero.
__forceinline void xMOVQZX( const xRegisterSSE& to, const ModSibBase& src )		{ writeXMMop( 0xf3, 0x7e, to, src ); }

// Moves from XMM to XMM, with the *upper 64 bits* of the destination register
// being cleared to zero.
__forceinline void xMOVQZX( const xRegisterSSE& to, const void* src )			{ writeXMMop( 0xf3, 0x7e, to, src ); }

// Moves lower quad of XMM to ptr64 (no bits are cleared)
__forceinline void xMOVQ( const ModSibBase& dest, const xRegisterSSE& from )	{ writeXMMop( 0x66, 0xd6, from, dest ); }
// Moves lower quad of XMM to ptr64 (no bits are cleared)
__forceinline void xMOVQ( void* dest, const xRegisterSSE& from )				{ writeXMMop( 0x66, 0xd6, from, dest ); }

__forceinline void xMOVQ( const xRegisterMMX& to, const xRegisterMMX& from )	{ if( to != from ) writeXMMop( 0x6f, to, from ); }
__forceinline void xMOVQ( const xRegisterMMX& to, const ModSibBase& src )		{ writeXMMop( 0x6f, to, src ); }
__forceinline void xMOVQ( const xRegisterMMX& to, const void* src )				{ writeXMMop( 0x6f, to, src ); }
__forceinline void xMOVQ( const ModSibBase& dest, const xRegisterMMX& from )	{ writeXMMop( 0x7f, from, dest ); }
__forceinline void xMOVQ( void* dest, const xRegisterMMX& from )				{ writeXMMop( 0x7f, from, dest ); }

// This form of xMOVQ is Intel's adeptly named 'MOVQ2DQ'
__forceinline void xMOVQ( const xRegisterSSE& to, const xRegisterMMX& from )	{ writeXMMop( 0xf3, 0xd6, to, from ); }

// This form of xMOVQ is Intel's adeptly named 'MOVDQ2Q'
__forceinline void xMOVQ( const xRegisterMMX& to, const xRegisterSSE& from )
{
	// Manual implementation of this form of MOVQ, since its parameters are unique in a way
	// that breaks the template inference of writeXMMop();

	SimdPrefix( 0xf2, 0xd6 );
	ModRM_Direct( to.Id, from.Id );
}

//////////////////////////////////////////////////////////////////////////////////////////
//

#define IMPLEMENT_xMOVS( ssd, prefix ) \
	__forceinline void xMOV##ssd( const xRegisterSSE& to, const xRegisterSSE& from )	{ if( to != from ) writeXMMop( prefix, 0x10, to, from ); } \
	__forceinline void xMOV##ssd##ZX( const xRegisterSSE& to, const void* from )		{ writeXMMop( prefix, 0x10, to, from ); } \
	__forceinline void xMOV##ssd##ZX( const xRegisterSSE& to, const ModSibBase& from )	{ writeXMMop( prefix, 0x10, to, from ); } \
	__forceinline void xMOV##ssd( const void* to, const xRegisterSSE& from )			{ writeXMMop( prefix, 0x11, from, to ); } \
	__forceinline void xMOV##ssd( const ModSibBase& to, const xRegisterSSE& from )		{ writeXMMop( prefix, 0x11, from, to ); }

IMPLEMENT_xMOVS( SS, 0xf3 )
IMPLEMENT_xMOVS( SD, 0xf2 )

//////////////////////////////////////////////////////////////////////////////////////////
// Non-temporal movs only support a register as a target (ie, load form only, no stores)
//

__forceinline void xMOVNTDQA( const xRegisterSSE& to, const void* from )
{
	xWrite<u32>( 0x2A380f66 );
	xWriteDisp( to.Id, from );
}

__noinline void xMOVNTDQA( const xRegisterSSE& to, const ModSibBase& from )
{
	xWrite<u32>( 0x2A380f66 );
	EmitSibMagic( to.Id, from );
}

__forceinline void xMOVNTDQ( void* to, const xRegisterSSE& from )			{ writeXMMop( 0x66, 0xe7, from, to ); }
__noinline void xMOVNTDQA( const ModSibBase& to, const xRegisterSSE& from )	{ writeXMMop( 0x66, 0xe7, from, to ); }

__forceinline void xMOVNTPD( void* to, const xRegisterSSE& from )			{ writeXMMop( 0x66, 0x2b, from, to ); }
__noinline void xMOVNTPD( const ModSibBase& to, const xRegisterSSE& from )	{ writeXMMop( 0x66, 0x2b, from, to ); }
__forceinline void xMOVNTPS( void* to, const xRegisterSSE& from )			{ writeXMMop( 0x2b, from, to ); }
__noinline void xMOVNTPS( const ModSibBase& to, const xRegisterSSE& from )	{ writeXMMop( 0x2b, from, to ); }

__forceinline void xMOVNTQ( void* to, const xRegisterMMX& from )			{ writeXMMop( 0xe7, from, to ); }
__noinline void xMOVNTQ( const ModSibBase& to, const xRegisterMMX& from )	{ writeXMMop( 0xe7, from, to ); }


}
