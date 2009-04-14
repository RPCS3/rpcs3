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

__threadlocal XMMSSEType g_xmmtypes[XMMREGS] = { XMMT_INT };

namespace x86Emitter {

const x86IndexerType ptr;
const x86IndexerTypeExplicit<4> ptr32;
const x86IndexerTypeExplicit<2> ptr16;
const x86IndexerTypeExplicit<1> ptr8;	

// ------------------------------------------------------------------------

template< int OperandSize > const x86Register<OperandSize> x86Register<OperandSize>::Empty;
const x86IndexReg	x86IndexReg::Empty;

const x86Register32
	eax( 0 ), ebx( 3 ),
	ecx( 1 ), edx( 2 ),
	esi( 6 ), edi( 7 ),
	ebp( 5 ), esp( 4 );

const x86Register16
	ax( 0 ), bx( 3 ),
	cx( 1 ), dx( 2 ),
	si( 6 ), di( 7 ),
	bp( 5 ), sp( 4 );

const x86Register8
	al( 0 ), cl( 1 ),
	dl( 2 ), bl( 3 ),
	ah( 4 ), ch( 5 ),
	dh( 6 ), bh( 7 );

namespace Internal
{
	const Group1ImplAll<G1Type_ADD> ADD;
	const Group1ImplAll<G1Type_OR>  OR;
	const Group1ImplAll<G1Type_ADC> ADC;
	const Group1ImplAll<G1Type_SBB> SBB;
	const Group1ImplAll<G1Type_AND> AND;
	const Group1ImplAll<G1Type_SUB> SUB;
	const Group1ImplAll<G1Type_XOR> XOR;
	const Group1ImplAll<G1Type_CMP> CMP;

	const Group2ImplAll<G2Type_ROL> ROL;
	const Group2ImplAll<G2Type_ROR> ROR;
	const Group2ImplAll<G2Type_RCL> RCL;
	const Group2ImplAll<G2Type_RCR> RCR;
	const Group2ImplAll<G2Type_SHL> SHL;
	const Group2ImplAll<G2Type_SHR> SHR;
	const Group2ImplAll<G2Type_SAR> SAR;

	// Performance note: VC++ wants to use byte/word register form for the following
	// ModRM/SibSB constructors if we use iWrite<u8>, and furthermore unrolls the
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
	// the same end result is achieved and no false dependencies are generated.
	//
	// (btw, I know this isn't a critical performance item by any means, but it's
	//  annoying simply because it *should* be an easy thing to optimize)

	__forceinline void ModRM( uint mod, uint reg, uint rm )
	{
		*(u32*)x86Ptr = (mod << 6) | (reg << 3) | rm;
		x86Ptr++;
	}

	__forceinline void SibSB( u32 ss, u32 index, u32 base )
	{
		*(u32*)x86Ptr = (ss << 6) | (index << 3) | base;
		x86Ptr++;
	}

	// ------------------------------------------------------------------------
	// returns TRUE if this instruction requires SIB to be encoded, or FALSE if the
	// instruction ca be encoded as ModRm alone.
	static __forceinline bool NeedsSibMagic( const ModSibBase& info )
	{
		// If base register is ESP, then we need a SIB:
		if( info.Base.IsStackPointer() ) return true;

		// no registers? no sibs!
		// (ModSibBase::Reduce
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
	__forceinline void EmitSibMagic( uint regfield, const ModSibBase& info )
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
				ModRM( 0, regfield, ModRm_UseDisp32 );
				iWrite<u32>( info.Displacement );
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
				iWrite<u32>( info.Displacement );
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
			*(u32*)x86Ptr = info.Displacement;
			x86Ptr += (displacement_size == 1) ? 1 : 4;
		}
	}
}

using namespace Internal;

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
			MOV( to, src.Displacement );
			return;
		}
		else if( displacement_size == 0 )
		{
			MOV( to, ToReg( src.Index.Id ) );
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

				MOV( to, ToReg( src.Index.Id ) );
				SHL( to, src.Scale );
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
			if( src.Base == ebp && displacement_size == 0 )
				displacement_size = 1;		// forces [ebp] to be encoded as [ebp+0]!

			iWrite<u8>( 0x8d );
			ModRM( displacement_size, to.Id, ModRm_UseSib );
			SibSB( src.Scale, src.Index.Id, src.Base.Id );
			
			/*switch( displacement_size )
			{
				case 0: break;
				case 1: emit.write<u8>( src.Displacement );  break;
				case 2: emit.write<u32>( src.Displacement ); break;
				jNO_DEFAULT
			}*/
		}
	}

	if( displacement_size != 0 )
	{
		*(u32*)x86Ptr = src.Displacement;
		x86Ptr += (displacement_size == 1) ? 1 : 4;
	}
}

__emitinline void LEA( x86Register32 to, const ModSibBase& src, bool preserve_flags )
{
	EmitLeaMagic( to, src, preserve_flags );
}


__emitinline void LEA( x86Register16 to, const ModSibBase& src, bool preserve_flags )
{
	write8( 0x66 );
	EmitLeaMagic( to, src, preserve_flags );
}

//////////////////////////////////////////////////////////////////////////////////////////
// MOV instruction Implementation

template< typename ImmType, typename SibMagicType >
class MovImpl
{
public: 
	static const uint OperandSize = sizeof(ImmType);

protected:
	static bool Is8BitOperand() { return OperandSize == 1; }
	static void prefix16() { if( OperandSize == 2 ) iWrite<u8>( 0x66 ); }

public:
	static __forceinline void Emit( const x86Register<OperandSize>& to, const x86Register<OperandSize>& from )
	{
		if( to == from ) return;	// ignore redundant MOVs.

		prefix16();
		iWrite<u8>( Is8BitOperand() ? 0x88 : 0x89 );
		ModRM( 3, from.Id, to.Id );
	}

	static __forceinline void Emit( const ModSibBase& dest, const x86Register<OperandSize>& from )
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
			SibMagicType::Emit( from.Id, dest );
		}
	}

	static __forceinline void Emit( const x86Register<OperandSize>& to, const ModSibBase& src )
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
			SibMagicType::Emit( to.Id, src );
		}
	}

	static __forceinline void Emit( const x86Register<OperandSize>& to, ImmType imm )
	{
		// Note: MOV does not have (reg16/32,imm8) forms.

		prefix16();
		iWrite<u8>( (Is8BitOperand() ? 0xb0 : 0xb8) | to.Id ); 
		iWrite<ImmType>( imm );
	}

	static __forceinline void Emit( ModSibStrict<OperandSize> dest, ImmType imm )
	{
		prefix16();
		iWrite<u8>( Is8BitOperand() ? 0xc6 : 0xc7 );
		SibMagicType::Emit( 0, dest );
		iWrite<ImmType>( imm );
	}
};

namespace Internal
{
	typedef MovImpl<u32,SibMagic> MOV32;
	typedef MovImpl<u16,SibMagic> MOV16;
	typedef MovImpl<u8,SibMagic>  MOV8;

	typedef MovImpl<u32,SibMagicInline> MOV32i;
	typedef MovImpl<u16,SibMagicInline> MOV16i;
	typedef MovImpl<u8,SibMagicInline>  MOV8i;
}

// Inlining Notes:
//   I've set up the inlining to be as practical and intelligent as possible, which means
//   forcing inlining for (void*) forms of ModRM, which thanks to constprop reduce to
//   virtually no code.  In the case of (Reg, Imm) forms, the inlinign is up to the dis-
//   cretion of the compiler.
// 

// TODO : Turn this into a macro after it's been debugged and accuracy-approved! :D

// ---------- 32 Bit Interface -----------
__forceinline void MOV( const x86Register32& to,	const x86Register32& from )	{ MOV32i::Emit( to, from ); }
__forceinline void MOV( const x86Register32& to,	const void* src )			{ MOV32i::Emit( to, ptr32[src] ); }
__forceinline void MOV( const void* dest,			const x86Register32& from )	{ MOV32i::Emit( ptr32[dest], from ); }
__noinline void MOV( const ModSibBase& sibdest,		const x86Register32& from )	{ MOV32::Emit( sibdest, from ); }
__noinline void MOV( const x86Register32& to,		const ModSibBase& sibsrc )	{ MOV32::Emit( to, sibsrc ); }
__noinline void MOV( const ModSibStrict<4>& sibdest,u32 imm )					{ MOV32::Emit( sibdest, imm ); }

void MOV( const x86Register32& to, u32 imm, bool preserve_flags )
{
	if( !preserve_flags && (imm == 0) )
		XOR( to, to );
	else
		MOV32i::Emit( to, imm );
}


// ---------- 16 Bit Interface -----------
__forceinline void MOV( const x86Register16& to,	const x86Register16& from )	{ MOV16i::Emit( to, from ); }
__forceinline void MOV( const x86Register16& to,	const void* src )			{ MOV16i::Emit( to, ptr16[src] ); }
__forceinline void MOV( const void* dest,			const x86Register16& from )	{ MOV16i::Emit( ptr16[dest], from ); }
__noinline void MOV( const ModSibBase& sibdest,		const x86Register16& from )	{ MOV16::Emit( sibdest, from ); }
__noinline void MOV( const x86Register16& to,		const ModSibBase& sibsrc )	{ MOV16::Emit( to, sibsrc ); }
__noinline void MOV( const ModSibStrict<2>& sibdest,u16 imm )					{ MOV16::Emit( sibdest, imm ); }

void MOV( const x86Register16& to, u16 imm, bool preserve_flags )
{
	if( !preserve_flags && (imm == 0) )
		XOR( to, to );
	else
		MOV16i::Emit( to, imm );
}

// ---------- 8 Bit Interface -----------
__forceinline void MOV( const x86Register8& to,		const x86Register8& from )	{ MOV8i::Emit( to, from ); }
__forceinline void MOV( const x86Register8& to,		const void* src )			{ MOV8i::Emit( to, ptr8[src] ); }
__forceinline void MOV( const void* dest,			const x86Register8& from )	{ MOV8i::Emit( ptr8[dest], from ); }
__noinline void MOV( const ModSibBase& sibdest,		const x86Register8& from )	{ MOV8::Emit( sibdest, from ); }
__noinline void MOV( const x86Register8& to,		const ModSibBase& sibsrc )	{ MOV8::Emit( to, sibsrc ); }
__noinline void MOV( const ModSibStrict<1>& sibdest,u8 imm )					{ MOV8::Emit( sibdest, imm ); }

void MOV( const x86Register8& to, u8 imm, bool preserve_flags )
{
	if( !preserve_flags && (imm == 0) )
		XOR( to, to );
	else
		MOV8i::Emit( to, imm );
}


//////////////////////////////////////////////////////////////////////////////////////////
// Miscellaneous Section!
// Various Instructions with no parameter and no special encoding logic.
//
__forceinline void RET()	{ write8( 0xC3 ); }
__forceinline void CBW()	{ write16( 0x9866 );  }
__forceinline void CWD()	{ write8( 0x98 ); }
__forceinline void CDQ()	{ write8( 0x99 ); }
__forceinline void CWDE()	{ write8( 0x98 ); }

__forceinline void LAHF()	{ write8( 0x9f ); }
__forceinline void SAHF()	{ write8( 0x9e ); }


//////////////////////////////////////////////////////////////////////////////////////////
// Push / Pop Emitters
//
// Note: pushad/popad implementations are intentionally left out.  The instructions are
// invalid in x64, and are super slow on x32.  Use multiple Push/Pop instructions instead.


__forceinline void POP( x86Register32 from )	{ write8( 0x58 | from.Id ); }

__emitinline void POP( const ModSibBase& from )
{
	iWrite<u8>( 0x8f ); Internal::EmitSibMagic( 0, from );
}

__forceinline void PUSH( u32 imm )				{ write8( 0x68 ); write32( imm ); }
__forceinline void PUSH( x86Register32 from )	{ write8( 0x50 | from.Id ); }

__emitinline void PUSH( const ModSibBase& from )
{
	iWrite<u8>( 0xff ); Internal::EmitSibMagic( 6, from );
}

// pushes the EFLAGS register onto the stack
__forceinline void PUSHFD() { write8( 0x9C ); }
// pops the EFLAGS register from the stack
__forceinline void POPFD() { write8( 0x9D ); }

}
