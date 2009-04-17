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

const x86IndexerType ptr;
const x86IndexerTypeExplicit<u32> ptr32;
const x86IndexerTypeExplicit<u16> ptr16;
const x86IndexerTypeExplicit<u8> ptr8;	

// ------------------------------------------------------------------------

template< typename OperandType > const iRegister<OperandType> iRegister<OperandType>::Empty;
const x86IndexReg x86IndexReg::Empty;

const iRegister32
	eax( 0 ), ebx( 3 ),
	ecx( 1 ), edx( 2 ),
	esi( 6 ), edi( 7 ),
	ebp( 5 ), esp( 4 );

const iRegister16
	ax( 0 ), bx( 3 ),
	cx( 1 ), dx( 2 ),
	si( 6 ), di( 7 ),
	bp( 5 ), sp( 4 );

const iRegister8
	al( 0 ),
	dl( 2 ), bl( 3 ),
	ah( 4 ), ch( 5 ),
	dh( 6 ), bh( 7 );
	
const iRegisterCL cl;

namespace Internal
{
	// Performance note: VC++ wants to use byte/word register form for the following
	// ModRM/SibSB constructors when we use iWrite<u8>, and furthermore unrolls the
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
		iWrite<u8>( (mod << 6) | (reg << 3) | rm );
		//*(u32*)x86Ptr = (mod << 6) | (reg << 3) | rm;
		//x86Ptr++;
	}

	__forceinline void ModRM_Direct( uint reg, uint rm )
	{
		ModRM( Mod_Direct, reg, rm );
	}

	__forceinline void SibSB( u32 ss, u32 index, u32 base )
	{
		iWrite<u8>( (ss << 6) | (index << 3) | base );
		//*(u32*)x86Ptr = (ss << 6) | (index << 3) | base;
		//x86Ptr++;
	}

	__forceinline void iWriteDisp( int regfield, s32 displacement )
	{
		ModRM( 0, regfield, ModRm_UseDisp32 );
		iWrite<s32>( displacement );
	}

	__forceinline void iWriteDisp( int regfield, const void* address )
	{
		iWriteDisp( regfield, (s32)address );
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
				iWriteDisp( regfield, info.Displacement );
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
				iWrite<s32>( info.Displacement );
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
				iWrite<s8>( info.Displacement );
			else
				iWrite<s32>( info.Displacement );
		}
	}
}

using namespace Internal;

const MovImplAll iMOV;
const TestImplAll iTEST;

const Group1ImplAll<G1Type_ADD> iADD;
const Group1ImplAll<G1Type_OR>  iOR;
const Group1ImplAll<G1Type_ADC> iADC;
const Group1ImplAll<G1Type_SBB> iSBB;
const Group1ImplAll<G1Type_AND> iAND;
const Group1ImplAll<G1Type_SUB> iSUB;
const Group1ImplAll<G1Type_XOR> iXOR;
const Group1ImplAll<G1Type_CMP> iCMP;

const Group2ImplAll<G2Type_ROL> iROL;
const Group2ImplAll<G2Type_ROR> iROR;
const Group2ImplAll<G2Type_RCL> iRCL;
const Group2ImplAll<G2Type_RCR> iRCR;
const Group2ImplAll<G2Type_SHL> iSHL;
const Group2ImplAll<G2Type_SHR> iSHR;
const Group2ImplAll<G2Type_SAR> iSAR;

const Group3ImplAll<G3Type_NOT> iNOT;
const Group3ImplAll<G3Type_NEG> iNEG;
const Group3ImplAll<G3Type_MUL> iUMUL;
const Group3ImplAll<G3Type_DIV> iUDIV;
const Group3ImplAll<G3Type_iDIV> iSDIV;

const IncDecImplAll<false> iINC;
const IncDecImplAll<true>  iDEC;

const MovExtendImplAll<false> iMOVZX;
const MovExtendImplAll<true>  iMOVSX;

const DwordShiftImplAll<false> iSHLD;
const DwordShiftImplAll<true>  iSHRD;

const Group8ImplAll<G8Type_BT> iBT;
const Group8ImplAll<G8Type_BTR> iBTR;
const Group8ImplAll<G8Type_BTS> iBTS;
const Group8ImplAll<G8Type_BTC> iBTC;

// ------------------------------------------------------------------------
const CMovImplGeneric iCMOV;

const CMovImplAll<Jcc_Above>			iCMOVA;
const CMovImplAll<Jcc_AboveOrEqual>		iCMOVAE;
const CMovImplAll<Jcc_Below>			iCMOVB;
const CMovImplAll<Jcc_BelowOrEqual>		iCMOVBE;

const CMovImplAll<Jcc_Greater>			iCMOVG;
const CMovImplAll<Jcc_GreaterOrEqual>	iCMOVGE;
const CMovImplAll<Jcc_Less>				iCMOVL;
const CMovImplAll<Jcc_LessOrEqual>		iCMOVLE;

const CMovImplAll<Jcc_Zero>				iCMOVZ;
const CMovImplAll<Jcc_Equal>			iCMOVE;
const CMovImplAll<Jcc_NotZero>			iCMOVNZ;
const CMovImplAll<Jcc_NotEqual>			iCMOVNE;

const CMovImplAll<Jcc_Overflow>			iCMOVO;
const CMovImplAll<Jcc_NotOverflow>		iCMOVNO;
const CMovImplAll<Jcc_Carry>			iCMOVC;
const CMovImplAll<Jcc_NotCarry>			iCMOVNC;

const CMovImplAll<Jcc_Signed>			iCMOVS;
const CMovImplAll<Jcc_Unsigned>			iCMOVNS;
const CMovImplAll<Jcc_ParityEven>		iCMOVPE;
const CMovImplAll<Jcc_ParityOdd>		iCMOVPO;

// ------------------------------------------------------------------------
const SetImplGeneric iSET;

const SetImplAll<Jcc_Above>				iSETA;
const SetImplAll<Jcc_AboveOrEqual>		iSETAE;
const SetImplAll<Jcc_Below>				iSETB;
const SetImplAll<Jcc_BelowOrEqual>		iSETBE;

const SetImplAll<Jcc_Greater>			iSETG;
const SetImplAll<Jcc_GreaterOrEqual>	iSETGE;
const SetImplAll<Jcc_Less>				iSETL;
const SetImplAll<Jcc_LessOrEqual>		iSETLE;

const SetImplAll<Jcc_Zero>				iSETZ;
const SetImplAll<Jcc_Equal>				iSETE;
const SetImplAll<Jcc_NotZero>			iSETNZ;
const SetImplAll<Jcc_NotEqual>			iSETNE;

const SetImplAll<Jcc_Overflow>			iSETO;
const SetImplAll<Jcc_NotOverflow>		iSETNO;
const SetImplAll<Jcc_Carry>				iSETC;
const SetImplAll<Jcc_NotCarry>			iSETNC;

const SetImplAll<Jcc_Signed>			iSETS;
const SetImplAll<Jcc_Unsigned>			iSETNS;
const SetImplAll<Jcc_ParityEven>		iSETPE;
const SetImplAll<Jcc_ParityOdd>			iSETPO;


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
			iWrite<u8>( 0xcc );
	}
	else
		x86Ptr += bytes;
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
static void EmitLeaMagic( iRegister<OperandType> to, const ModSibBase& src, bool preserve_flags )
{
	typedef iRegister<OperandType> ToReg;
	
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
			iMOV( to, src.Displacement );
			return;
		}
		else if( displacement_size == 0 )
		{
			iMOV( to, ToReg( src.Index.Id ) );
			return;
		}
		else
		{
			if( !preserve_flags )
			{
				// encode as MOV and ADD combo.  Make sure to use the immediate on the
				// ADD since it can encode as an 8-bit sign-extended value.
				
				iMOV( to, ToReg( src.Index.Id ) );
				iADD( to, src.Displacement );
				return;
			}
			else
			{
				// note: no need to do ebp+0 check since we encode all 0 displacements as
				// register assignments above (via MOV)

				iWrite<u8>( 0x8d );
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

				iMOV( to, ToReg( src.Index.Id ) );
				iSHL( to, src.Scale );
				return;
			}
			iWrite<u8>( 0x8d );
			ModRM( 0, to.Id, ModRm_UseSib );
			SibSB( src.Scale, src.Index.Id, ModRm_UseDisp32 );
			iWrite<u32>( src.Displacement );
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
						iMOV( to, ToReg( src.Base.Id ) );	// will do the trick!
						if( src.Displacement ) iADD( to, src.Displacement );
						return;
					}
					else if( src.Displacement == 0 )
					{
						iMOV( to, ToReg( src.Base.Id ) );
						iADD( to, ToReg( src.Index.Id ) );
						return;
					}
				}
				else if( (src.Index == esp) && (src.Displacement == 0) )
				{
					// special case handling of ESP as Index, which is replaceable with
					// a single MOV even when preserve_flags is set! :D
					
					iMOV( to, ToReg( src.Base.Id ) );
					return;
				}
			}

			if( src.Base == ebp && displacement_size == 0 )
				displacement_size = 1;		// forces [ebp] to be encoded as [ebp+0]!

			iWrite<u8>( 0x8d );
			ModRM( displacement_size, to.Id, ModRm_UseSib );
			SibSB( src.Scale, src.Index.Id, src.Base.Id );
		}
	}

	if( displacement_size != 0 )
	{
		if( displacement_size == 1 )
			iWrite<s8>( src.Displacement );
		else
			iWrite<s32>( src.Displacement );
	}
}

__emitinline void iLEA( iRegister32 to, const ModSibBase& src, bool preserve_flags )
{
	EmitLeaMagic( to, src, preserve_flags );
}


__emitinline void iLEA( iRegister16 to, const ModSibBase& src, bool preserve_flags )
{
	write8( 0x66 );
	EmitLeaMagic( to, src, preserve_flags );
}

//////////////////////////////////////////////////////////////////////////////////////////
// 	The following iMul-specific forms are valid for 16 and 32 bit register operands only!

template< typename ImmType >
class iMulImpl
{
protected:
	static const uint OperandSize = sizeof(ImmType);
	static void prefix16() { if( OperandSize == 2 ) iWrite<u8>( 0x66 ); }

public:
	// ------------------------------------------------------------------------
	static __emitinline void Emit( const iRegister<ImmType>& to, const iRegister<ImmType>& from )
	{
		prefix16();
		write16( 0xaf0f );
		ModRM_Direct( to.Id, from.Id );
	}
	
	// ------------------------------------------------------------------------
	static __emitinline void Emit( const iRegister<ImmType>& to, const void* src )
	{
		prefix16();
		write16( 0xaf0f );
		iWriteDisp( to.Id, src );
	}

	// ------------------------------------------------------------------------
	static __emitinline void Emit( const iRegister<ImmType>& to, const ModSibBase& src )
	{
		prefix16();
		write16( 0xaf0f );
		EmitSibMagic( to.Id, src );
	}

	// ------------------------------------------------------------------------
	static __emitinline void Emit( const iRegister<ImmType>& to, const iRegister<ImmType>& from, ImmType imm )
	{
		prefix16();
		write16( is_s8( imm ) ? 0x6b : 0x69 );
		ModRM_Direct( to.Id, from.Id );
		if( is_s8( imm ) )
			write8( imm );
		else
			iWrite<ImmType>( imm );
	}

	// ------------------------------------------------------------------------
	static __emitinline void Emit( const iRegister<ImmType>& to, const void* src, ImmType imm )
	{
		prefix16();
		write16( is_s8( imm ) ? 0x6b : 0x69 );
		iWriteDisp( to.Id, src );
		if( is_s8( imm ) )
			write8( imm );
		else
			iWrite<ImmType>( imm );
	}

	// ------------------------------------------------------------------------
	static __emitinline void Emit( const iRegister<ImmType>& to, const ModSibBase& src, ImmType imm )
	{
		prefix16();
		write16( is_s8( imm ) ? 0x6b : 0x69 );
		EmitSibMagic( to.Id, src );
		if( is_s8( imm ) )
			write8( imm );
		else
			iWrite<ImmType>( imm );
	}
};

// ------------------------------------------------------------------------
// iMUL's special forms (unique to iMUL alone), and valid for 32/16 bit operands only,
// thus noi templates are used.

namespace Internal
{
	typedef iMulImpl<u32> iMUL32;
	typedef iMulImpl<u16> iMUL16;
}

__forceinline void iSMUL( const iRegister32& to,	const iRegister32& from )			{ iMUL32::Emit( to, from ); }
__forceinline void iSMUL( const iRegister32& to,	const void* src )					{ iMUL32::Emit( to, src ); }
__forceinline void iSMUL( const iRegister32& to,	const iRegister32& from, s32 imm )	{ iMUL32::Emit( to, from, imm ); }
__noinline void iSMUL( const iRegister32& to,	const ModSibBase& src )					{ iMUL32::Emit( to, src ); }
__noinline void iSMUL( const iRegister32& to,	const ModSibBase& from, s32 imm )		{ iMUL32::Emit( to, from, imm ); }

__forceinline void iSMUL( const iRegister16& to,	const iRegister16& from )			{ iMUL16::Emit( to, from ); }
__forceinline void iSMUL( const iRegister16& to,	const void* src )					{ iMUL16::Emit( to, src ); }
__forceinline void iSMUL( const iRegister16& to,	const iRegister16& from, s16 imm )	{ iMUL16::Emit( to, from, imm ); }
__noinline void iSMUL( const iRegister16& to,	const ModSibBase& src )					{ iMUL16::Emit( to, src ); }
__noinline void iSMUL( const iRegister16& to,	const ModSibBase& from, s16 imm )		{ iMUL16::Emit( to, from, imm ); }



//////////////////////////////////////////////////////////////////////////////////////////
// Push / Pop Emitters
//
// Note: pushad/popad implementations are intentionally left out.  The instructions are
// invalid in x64, and are super slow on x32.  Use multiple Push/Pop instructions instead.

__emitinline void iPOP( const ModSibBase& from )
{
	iWrite<u8>( 0x8f );
	EmitSibMagic( 0, from );
}

__emitinline void iPUSH( const ModSibBase& from )
{
	iWrite<u8>( 0xff );
	EmitSibMagic( 6, from );
}


}
