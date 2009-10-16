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
#include "internal.h"

// defined in tools.cpp
extern __aligned16 u64 g_globalXMMData[2*iREGCNT_XMM];

// ------------------------------------------------------------------------
// Notes on Thread Local Storage:
//  * TLS is pretty simple, and "just works" from a programmer perspective, with only
//    some minor additional computational overhead (see performance notes below).
//
//  * MSVC and GCC handle TLS differently internally, but behavior to the programmer is
//    generally identical.
//
// Performance Considerations:
//  * GCC's implementation involves an extra dereference from normal storage (possibly
//    applies to x86-32 only -- x86-64 is untested).
//
//  * MSVC's implementation involves *two* extra dereferences from normal storage because
//    it has to look up the TLS heap pointer from the Windows Thread Storage Area.  (in
//    generated ASM code, this dereference is denoted by access to the fs:[2ch] address),
//
//  * However, in either case, the optimizer usually optimizes it to a register so the
//    extra overhead is minimal over a series of instructions.
//
// MSVC Notes:
//  * Important!! the Full Optimization [/Ox] option effectively disables TLS optimizations
//    in MSVC 2008 and earlier, causing generally significant code bloat.  Not tested in
//    VC2010 yet.
//
//  * VC2010 generally does a superior job of optimizing TLS across inlined functions and
//    class methods, compared to predecessors.
//


__threadlocal u8  *x86Ptr;

__threadlocal XMMSSEType g_xmmtypes[iREGCNT_XMM] = { XMMT_INT };

namespace x86Emitter {

__forceinline void xWrite8( u8 val )
{
	xWrite( val );
}

__forceinline void xWrite16( u16 val )
{
	xWrite( val );
}

__forceinline void xWrite32( u32 val )
{
	xWrite( val );
}

__forceinline void xWrite64( u64 val )
{
	xWrite( val );
}

const xAddressIndexerBase	ptr;
const xAddressIndexer<u128>	ptr128;
const xAddressIndexer<u64>	ptr64;
const xAddressIndexer<u32>	ptr32;
const xAddressIndexer<u16>	ptr16;
const xAddressIndexer<u8>	ptr8;

// ------------------------------------------------------------------------

template< typename OperandType > const xRegisterBase<OperandType> xRegisterBase<OperandType>::Empty;

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

const xAddressReg
	eax( 0 ), ebx( 3 ),
	ecx( 1 ), edx( 2 ),
	esp( 4 ), ebp( 5 ),
	esi( 6 ), edi( 7 );

const xRegister16
	ax( 0 ), bx( 3 ),
	cx( 1 ), dx( 2 ),
	sp( 4 ), bp( 5 ),
	si( 6 ), di( 7 );

const xRegister8
	al( 0 ),
	dl( 2 ), bl( 3 ),
	ah( 4 ), ch( 5 ),
	dh( 6 ), bh( 7 );

const xRegisterCL cl;

const char *const x86_regnames_gpr8[8] =
{
	"al", "cl", "dl", "bl",
	"ah", "ch", "dh", "bh"
};

const char *const x86_regnames_gpr16[8] =
{
	"ax", "cx", "dx", "bx",
	"sp", "bp", "si", "di"
};

const char *const x86_regnames_gpr32[8] =
{
	"eax", "ecx", "edx", "ebx",
	"esp", "ebp", "esi", "edi"
};

const char *const x86_regnames_sse[8] =
{
	"xmm0", "xmm1", "xmm2", "xmm3",
	"xmm4", "xmm5", "xmm6", "xmm7"
};

const char *const x86_regnames_mmx[8] =
{
	"mm0", "mm1", "mm2", "mm3",
	"mm4", "mm5", "mm6", "mm7"
};

//////////////////////////////////////////////////////////////////////////////////////////

namespace Internal
{

	template< typename T >
	const char* xGetRegName( const xRegister<T>& src )
	{
		if( src.IsEmpty() ) return "empty";
		switch( sizeof(T) )
		{
			case 1: return x86_regnames_gpr8[ src.Id ];
			case 2: return x86_regnames_gpr16[ src.Id ];
			case 4: return x86_regnames_gpr32[ src.Id ];
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////////
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

	static __forceinline void ModRM( uint mod, uint reg, uint rm )
	{
		xWrite8( (mod << 6) | (reg << 3) | rm );
	}

	static __forceinline void SibSB( u32 ss, u32 index, u32 base )
	{
		xWrite8( (ss << 6) | (index << 3) | base );
	}

	__forceinline void EmitSibMagic( uint regfield, const void* address )
	{
		ModRM( 0, regfield, ModRm_UseDisp32 );
		xWrite<s32>( (s32)address );
	}

	//////////////////////////////////////////////////////////////////////////////////////////
	// emitter helpers for xmm instruction with prefixes, most of which are using
	// the basic opcode format (items inside braces denote optional or conditional
	// emission):
	//
	//   [Prefix] / 0x0f / [OpcodePrefix] / Opcode / ModRM+[SibSB]
	//
	// Prefixes are typically 0x66, 0xf2, or 0xf3.  OpcodePrefixes are either 0x38 or
	// 0x3a [and other value will result in assertion failue].
	//
	__emitinline void xOpWrite0F( u8 prefix, u16 opcode, int instId, const ModSibBase& sib )
	{
		SimdPrefix( prefix, opcode );
		EmitSibMagic( instId, sib );
	}

	__emitinline void xOpWrite0F( u8 prefix, u16 opcode, int instId, const void* data )
	{
		SimdPrefix( prefix, opcode );
		EmitSibMagic( instId, data );
	}

	__emitinline void xOpWrite0F( u16 opcode, int instId, const ModSibBase& sib )
	{
		xOpWrite0F( 0, opcode, instId, sib );
	}


	//////////////////////////////////////////////////////////////////////////////////////////
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
	__noinline void EmitSibMagic( uint regfield, const ModSibBase& info )
	{
		pxAssertDev( regfield < 8, "Invalid x86 register identifier." );

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
				EmitSibMagic( regfield, (void*)info.Displacement );
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
const xImpl_Test xTEST;

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

const xImpl_Group3<G3Type_NOT> xNOT;
const xImpl_Group3<G3Type_NEG> xNEG;
const xImpl_Group3<G3Type_MUL> xUMUL;
const xImpl_Group3<G3Type_DIV> xUDIV;
const xImpl_iDiv xDIV;
const xImpl_iMul xMUL;

const xImpl_IncDec<false> xINC;
const xImpl_IncDec<true>  xDEC;

const MovExtendImplAll<false> xMOVZX;
const MovExtendImplAll<true>  xMOVSX;

const DwordShiftImplAll<false> xSHLD;
const DwordShiftImplAll<true>  xSHRD;

const xImpl_Group8<G8Type_BT> xBT;
const xImpl_Group8<G8Type_BTR> xBTR;
const xImpl_Group8<G8Type_BTS> xBTS;
const xImpl_Group8<G8Type_BTC> xBTC;

const xImpl_BitScan<0xbc> xBSF;
const xImpl_BitScan<0xbd> xBSR;

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
__emitinline void xSetPtr( void* ptr )
{
	x86Ptr = (u8*)ptr;
}

// ------------------------------------------------------------------------
// Retrieves the current emitter buffer target address.
// This is provided instead of using x86Ptr directly, since we may in the future find
// a need to change the storage class system for the x86Ptr 'under the hood.'
__emitinline u8* xGetPtr()
{
	return x86Ptr;
}

// ------------------------------------------------------------------------
__emitinline void xAlignPtr( uint bytes )
{
	// forward align
	x86Ptr = (u8*)( ( (uptr)x86Ptr + bytes - 1) & ~(bytes - 1) );
}

// ------------------------------------------------------------------------
__emitinline void xAdvancePtr( uint bytes )
{
	if( IsDevBuild )
	{
		// common debugger courtesy: advance with INT3 as filler.
		for( uint i=0; i<bytes; i++ )
			xWrite8( 0xcc );
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

		pxAssert( Scale == 0 );		// esp can't have an index modifier!
		pxAssert( Base.IsEmpty() );	// base must be empty or else!

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
			pxAssertDev( Base.IsEmpty(), "Cannot scale an Index register by 3 when Base is not empty!" );
			Base = Index;
			Scale = 1;
		break;

		case 4: Scale = 2; break;

		case 5:				// becomes [reg*4+reg]
			pxAssertDev( Base.IsEmpty(), "Cannot scale an Index register by 5 when Base is not empty!" );
			Base = Index;
			Scale = 2;
		break;

		case 6:				// invalid!
			pxFail( "x86 asm cannot scale a register by 6." );
		break;

		case 7:				// so invalid!
			pxFail( "x86 asm cannot scale a register by 7." );
		break;

		case 8: Scale = 3; break;
		case 9:				// becomes [reg*8+reg]
			pxAssertDev( Base.IsEmpty(), "Cannot scale an Index register by 9 when Base is not empty!" );
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

				xWrite8( 0x8d );
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
			xWrite8( 0x8d );
			ModRM( 0, to.Id, ModRm_UseSib );
			SibSB( src.Scale, src.Index.Id, ModRm_UseDisp32 );
			xWrite32( src.Displacement );
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

			xWrite8( 0x8d );
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
	xWrite8( 0x66 );
	EmitLeaMagic( to, src, preserve_flags );
}


//////////////////////////////////////////////////////////////////////////////////////////
// Push / Pop Emitters
//
// Note: pushad/popad implementations are intentionally left out.  The instructions are
// invalid in x64, and are super slow on x32.  Use multiple Push/Pop instructions instead.

__emitinline void xPOP( const ModSibBase& from )
{
	xWrite8( 0x8f );
	EmitSibMagic( 0, from );
}

__emitinline void xPUSH( const ModSibBase& from )
{
	xWrite8( 0xff );
	EmitSibMagic( 6, from );
}

__forceinline void xPOP( xRegister32 from )		{ xWrite8( 0x58 | from.Id ); }

__forceinline void xPUSH( u32 imm )				{ xWrite8( 0x68 ); xWrite32( imm ); }
__forceinline void xPUSH( xRegister32 from )	{ xWrite8( 0x50 | from.Id ); }

// pushes the EFLAGS register onto the stack
__forceinline void xPUSHFD()					{ xWrite8( 0x9C ); }
// pops the EFLAGS register from the stack
__forceinline void xPOPFD()						{ xWrite8( 0x9D ); }


//////////////////////////////////////////////////////////////////////////////////////////
//

__forceinline void xRET()	{ xWrite8( 0xC3 ); }
__forceinline void xCBW()	{ xWrite16( 0x9866 );  }
__forceinline void xCWD()	{ xWrite8( 0x98 ); }
__forceinline void xCDQ()	{ xWrite8( 0x99 ); }
__forceinline void xCWDE()	{ xWrite8( 0x98 ); }

__forceinline void xLAHF()	{ xWrite8( 0x9f ); }
__forceinline void xSAHF()	{ xWrite8( 0x9e ); }

__forceinline void xSTC()	{ xWrite8( 0xF9 ); }
__forceinline void xCLC()	{ xWrite8( 0xF8 ); }

// NOP 1-byte
__forceinline void xNOP()	{ xWrite8(0x90); }

__emitinline void xBSWAP( const xRegister32& to )
{
	xWrite8( 0x0F );
	xWrite8( 0xC8 | to.Id );
}

__emitinline void xStoreReg( const xRegisterSSE& src )
{
	xMOVDQA( &g_globalXMMData[src.Id*2], src );
}

__emitinline void xRestoreReg( const xRegisterSSE& dest )
{
	xMOVDQA( dest, &g_globalXMMData[dest.Id*2] );
}

}
