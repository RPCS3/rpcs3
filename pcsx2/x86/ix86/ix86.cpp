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
const x86IndexerTypeExplicit<4> ptr32;
const x86IndexerTypeExplicit<2> ptr16;
const x86IndexerTypeExplicit<1> ptr8;	

// ------------------------------------------------------------------------

template< int OperandSize > const iRegister<OperandSize> iRegister<OperandSize>::Empty;
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

const MovExtendImplAll<false> iMOVZX;
const MovExtendImplAll<true>  iMOVSX;

const Internal::DwordShiftImplAll<false> iSHLD;
const Internal::DwordShiftImplAll<true>  iSHRD;

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
template< typename ToReg >
static void EmitLeaMagic( ToReg to, const ModSibBase& src, bool preserve_flags )
{
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
						iADD( to, src.Displacement );
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
// MOV instruction Implementation

template< typename ImmType >
class MovImpl
{
public: 
	static const uint OperandSize = sizeof(ImmType);

protected:
	static bool Is8BitOperand() { return OperandSize == 1; }
	static void prefix16() { if( OperandSize == 2 ) iWrite<u8>( 0x66 ); }

public:
	// ------------------------------------------------------------------------
	static __forceinline void Emit( const iRegister<OperandSize>& to, const iRegister<OperandSize>& from )
	{
		if( to == from ) return;	// ignore redundant MOVs.

		prefix16();
		iWrite<u8>( Is8BitOperand() ? 0x88 : 0x89 );
		ModRM( 3, from.Id, to.Id );
	}

	// ------------------------------------------------------------------------
	static __forceinline void Emit( const ModSibBase& dest, const iRegister<OperandSize>& from )
	{
		prefix16();

		// mov eax has a special from when writing directly to a DISP32 address
		// (sans any register index/base registers).

		if( from.IsAccumulator() && dest.Index.IsEmpty() && dest.Base.IsEmpty() )
		{
			iWrite<u8>( Is8BitOperand() ? 0xa2 : 0xa3 );
			iWrite<u32>( dest.Displacement );
		}
		else
		{
			iWrite<u8>( Is8BitOperand() ? 0x88 : 0x89 );
			EmitSibMagic( from.Id, dest );
		}
	}

	// ------------------------------------------------------------------------
	static __forceinline void Emit( const iRegister<OperandSize>& to, const ModSibBase& src )
	{
		prefix16();

		// mov eax has a special from when reading directly from a DISP32 address
		// (sans any register index/base registers).

		if( to.IsAccumulator() && src.Index.IsEmpty() && src.Base.IsEmpty() )
		{
			iWrite<u8>( Is8BitOperand() ? 0xa0 : 0xa1 );
			iWrite<u32>( src.Displacement );
		}
		else
		{
			iWrite<u8>( Is8BitOperand() ? 0x8a : 0x8b );
			EmitSibMagic( to.Id, src );
		}
	}

	// ------------------------------------------------------------------------
	static __forceinline void Emit( void* dest, const iRegister<OperandSize>& from )
	{
		prefix16();

		// mov eax has a special from when writing directly to a DISP32 address

		if( from.IsAccumulator() )
		{
			iWrite<u8>( Is8BitOperand() ? 0xa2 : 0xa3 );
			iWrite<s32>( (s32)dest );
		}
		else
		{
			iWrite<u8>( Is8BitOperand() ? 0x88 : 0x89 );
			iWriteDisp( from.Id, dest );
		}
	}

	// ------------------------------------------------------------------------
	static __forceinline void Emit( const iRegister<OperandSize>& to, const void* src )
	{
		prefix16();

		// mov eax has a special from when reading directly from a DISP32 address

		if( to.IsAccumulator() )
		{
			iWrite<u8>( Is8BitOperand() ? 0xa0 : 0xa1 );
			iWrite<s32>( (s32)src );
		}
		else
		{
			iWrite<u8>( Is8BitOperand() ? 0x8a : 0x8b );
			iWriteDisp( to.Id, src );
		}
	}

	// ------------------------------------------------------------------------
	static __forceinline void Emit( const iRegister<OperandSize>& to, ImmType imm )
	{
		// Note: MOV does not have (reg16/32,imm8) forms.

		prefix16();
		iWrite<u8>( (Is8BitOperand() ? 0xb0 : 0xb8) | to.Id ); 
		iWrite<ImmType>( imm );
	}

	// ------------------------------------------------------------------------
	static __forceinline void Emit( ModSibStrict<OperandSize> dest, ImmType imm )
	{
		prefix16();
		iWrite<u8>( Is8BitOperand() ? 0xc6 : 0xc7 );
		EmitSibMagic( 0, dest );
		iWrite<ImmType>( imm );
	}
};

namespace Internal
{
	typedef MovImpl<u32> MOV32;
	typedef MovImpl<u16> MOV16;
	typedef MovImpl<u8>  MOV8;
}

// Inlining Notes:
//   I've set up the inlining to be as practical and intelligent as possible, which means
//   forcing inlining for (void*) forms of ModRM, which thanks to constprop reduce to
//   virtually no code.  In the case of (Reg, Imm) forms, the inlinign is up to the dis-
//   cretion of the compiler.
// 

// TODO : Turn this into a macro after it's been debugged and accuracy-approved! :D

// ---------- 32 Bit Interface -----------
__forceinline void iMOV( const iRegister32& to,	const iRegister32& from )		{ MOV32::Emit( to, from ); }
__forceinline void iMOV( const iRegister32& to,	const void* src )				{ MOV32::Emit( to, ptr32[src] ); }
__forceinline void iMOV( void* dest,			const iRegister32& from )		{ MOV32::Emit( ptr32[dest], from ); }
__noinline void iMOV( const ModSibBase& sibdest,	const iRegister32& from )	{ MOV32::Emit( sibdest, from ); }
__noinline void iMOV( const iRegister32& to,		const ModSibBase& sibsrc )	{ MOV32::Emit( to, sibsrc ); }
__noinline void iMOV( const ModSibStrict<4>& sibdest,u32 imm )					{ MOV32::Emit( sibdest, imm ); }

void iMOV( const iRegister32& to, u32 imm, bool preserve_flags )
{
	if( !preserve_flags && (imm == 0) )
		iXOR( to, to );
	else
		MOV32::Emit( to, imm );
}


// ---------- 16 Bit Interface -----------
__forceinline void iMOV( const iRegister16& to,	const iRegister16& from )		{ MOV16::Emit( to, from ); }
__forceinline void iMOV( const iRegister16& to,	const void* src )				{ MOV16::Emit( to, ptr16[src] ); }
__forceinline void iMOV( void* dest,			const iRegister16& from )		{ MOV16::Emit( ptr16[dest], from ); }
__noinline void iMOV( const ModSibBase& sibdest,	const iRegister16& from )	{ MOV16::Emit( sibdest, from ); }
__noinline void iMOV( const iRegister16& to,		const ModSibBase& sibsrc )	{ MOV16::Emit( to, sibsrc ); }
__noinline void iMOV( const ModSibStrict<2>& sibdest,u16 imm )					{ MOV16::Emit( sibdest, imm ); }

void iMOV( const iRegister16& to, u16 imm, bool preserve_flags )
{
	if( !preserve_flags && (imm == 0) )
		iXOR( to, to );
	else
		MOV16::Emit( to, imm );
}

// ---------- 8 Bit Interface -----------
__forceinline void iMOV( const iRegister8& to,	const iRegister8& from )		{ MOV8::Emit( to, from ); }
__forceinline void iMOV( const iRegister8& to,	const void* src )				{ MOV8::Emit( to, ptr8[src] ); }
__forceinline void iMOV( void* dest,			const iRegister8& from )		{ MOV8::Emit( ptr8[dest], from ); }
__noinline void iMOV( const ModSibBase& sibdest,	const iRegister8& from )	{ MOV8::Emit( sibdest, from ); }
__noinline void iMOV( const iRegister8& to,		const ModSibBase& sibsrc )		{ MOV8::Emit( to, sibsrc ); }
__noinline void iMOV( const ModSibStrict<1>& sibdest,u8 imm )					{ MOV8::Emit( sibdest, imm ); }

void iMOV( const iRegister8& to, u8 imm, bool preserve_flags )
{
	if( !preserve_flags && (imm == 0) )
		iXOR( to, to );
	else
		MOV8::Emit( to, imm );
}

//////////////////////////////////////////////////////////////////////////////////////////
// DIV/MUL/IDIV/IMUL instructions  (Implemented!)

// F6 is r8, F7 is r32.
// MUL is 4, DIV is 6.

enum MulDivType
{
	MDT_Mul		= 4,
	MDT_iMul	= 5,
	MDT_Div		= 6,
	MDT_iDiv	= 7
};

// ------------------------------------------------------------------------
// EAX form emitter for Mul/Div/iMUL/iDIV
//
template< int OperandSize >
static __forceinline void EmitMulDiv_OneRegForm( MulDivType InstType, const iRegister<OperandSize>& from )
{
	if( OperandSize == 2 ) iWrite<u8>( 0x66 );
	iWrite<u8>( (OperandSize == 1) ? 0xf6 : 0xf7 );
	ModRM( ModRm_Direct, InstType, from.Id );
}

static __forceinline void EmitMulDiv_OneRegForm( MulDivType InstType, const ModSibSized& sibsrc )
{
	if( sibsrc.OperandSize == 2 ) iWrite<u8>( 0x66 );
	iWrite<u8>( (sibsrc.OperandSize == 1) ? 0xf6 : 0xf7 );
	EmitSibMagic( InstType, sibsrc );
}

//////////////////////////////////////////////////////////////////////////////////////////
// 	All ioMul forms are valid for 16 and 32 bit register operands only!

template< typename ImmType >
class iMulImpl
{
public: 
	static const uint OperandSize = sizeof(ImmType);

protected:
	static void prefix16() { if( OperandSize == 2 ) iWrite<u8>( 0x66 ); }

public:
	// ------------------------------------------------------------------------
	static __forceinline void Emit( const iRegister<OperandSize>& to, const iRegister<OperandSize>& from )
	{
		prefix16();
		write16( 0xaf0f );
		ModRM( ModRm_Direct, to.Id, from.Id );
	}
	
	// ------------------------------------------------------------------------
	static __forceinline void Emit( const iRegister<OperandSize>& to, const void* src )
	{
		prefix16();
		write16( 0xaf0f );
		iWriteDisp( to.Id, src );
	}

	// ------------------------------------------------------------------------
	static __forceinline void Emit( const iRegister<OperandSize>& to, const ModSibBase& src )
	{
		prefix16();
		write16( 0xaf0f );
		EmitSibMagic( to.Id, src );
	}

	// ------------------------------------------------------------------------
	static __forceinline void Emit( const iRegister<OperandSize>& to, const iRegister<OperandSize>& from, ImmType imm )
	{
		prefix16();
		write16( is_s8( imm ) ? 0x6b : 0x69 );
		ModRM( ModRm_Direct, to.Id, from.Id );
		if( is_s8( imm ) )
			write8( imm );
		else
			iWrite<ImmType>( imm );
	}

	// ------------------------------------------------------------------------
	static __forceinline void Emit( const iRegister<OperandSize>& to, const void* src, ImmType imm )
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
	static __forceinline void Emit( const iRegister<OperandSize>& to, const ModSibBase& src, ImmType imm )
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

namespace Internal
{
	typedef iMulImpl<u32> iMUL32;
	typedef iMulImpl<u16> iMUL16;
}

__forceinline void iUMUL( const iRegister32& from )		{ EmitMulDiv_OneRegForm( MDT_Mul, from ); }
__forceinline void iUMUL( const iRegister16& from )		{ EmitMulDiv_OneRegForm( MDT_Mul, from ); }
__forceinline void iUMUL( const iRegister8& from )		{ EmitMulDiv_OneRegForm( MDT_Mul, from ); }
__noinline void iUMUL( const ModSibSized& from )		{ EmitMulDiv_OneRegForm( MDT_Mul, from ); }

__forceinline void iUDIV( const iRegister32& from )		{ EmitMulDiv_OneRegForm( MDT_Div, from ); }
__forceinline void iUDIV( const iRegister16& from )		{ EmitMulDiv_OneRegForm( MDT_Div, from ); }
__forceinline void iUDIV( const iRegister8& from )		{ EmitMulDiv_OneRegForm( MDT_Div, from ); }
__noinline void iUDIV( const ModSibSized& from )		{ EmitMulDiv_OneRegForm( MDT_Div, from ); }

__forceinline void iSDIV( const iRegister32& from )		{ EmitMulDiv_OneRegForm( MDT_iDiv, from ); }
__forceinline void iSDIV( const iRegister16& from )		{ EmitMulDiv_OneRegForm( MDT_iDiv, from ); }
__forceinline void iSDIV( const iRegister8& from )		{ EmitMulDiv_OneRegForm( MDT_iDiv, from ); }
__noinline void iSDIV( const ModSibSized& from )		{ EmitMulDiv_OneRegForm( MDT_iDiv, from ); }

__forceinline void iSMUL( const iRegister32& from )										{ EmitMulDiv_OneRegForm( MDT_iMul, from ); }
__forceinline void iSMUL( const iRegister32& to,	const iRegister32& from )			{ iMUL32::Emit( to, from ); }
__forceinline void iSMUL( const iRegister32& to,	const void* src )					{ iMUL32::Emit( to, src ); }
__forceinline void iSMUL( const iRegister32& to,	const iRegister32& from, s32 imm )	{ iMUL32::Emit( to, from, imm ); }
__noinline void iSMUL( const iRegister32& to,	const ModSibBase& src )					{ iMUL32::Emit( to, src ); }
__noinline void iSMUL( const iRegister32& to,	const ModSibBase& from, s32 imm )		{ iMUL32::Emit( to, from, imm ); }

__forceinline void iSMUL( const iRegister16& from )										{ EmitMulDiv_OneRegForm( MDT_iMul, from ); }
__forceinline void iSMUL( const iRegister16& to,	const iRegister16& from )			{ iMUL16::Emit( to, from ); }
__forceinline void iSMUL( const iRegister16& to,	const void* src )					{ iMUL16::Emit( to, src ); }
__forceinline void iSMUL( const iRegister16& to,	const iRegister16& from, s16 imm )	{ iMUL16::Emit( to, from, imm ); }
__noinline void iSMUL( const iRegister16& to,	const ModSibBase& src )					{ iMUL16::Emit( to, src ); }
__noinline void iSMUL( const iRegister16& to,	const ModSibBase& from, s16 imm )		{ iMUL16::Emit( to, from, imm ); }

__forceinline void iSMUL( const iRegister8& from )		{ EmitMulDiv_OneRegForm( MDT_iMul, from ); }
__noinline void iSMUL( const ModSibSized& from )		{ EmitMulDiv_OneRegForm( MDT_iMul, from ); }


//////////////////////////////////////////////////////////////////////////////////////////
// Push / Pop Emitters
//
// Note: pushad/popad implementations are intentionally left out.  The instructions are
// invalid in x64, and are super slow on x32.  Use multiple Push/Pop instructions instead.

__emitinline void iPOP( const ModSibBase& from )
{
	iWrite<u8>( 0x8f );
	Internal::EmitSibMagic( 0, from );
}

__emitinline void iPUSH( const ModSibBase& from )
{
	iWrite<u8>( 0xff );
	Internal::EmitSibMagic( 6, from );
}


}
