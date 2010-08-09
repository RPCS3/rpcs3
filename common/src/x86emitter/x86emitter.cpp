/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2010  PCSX2 Dev Team
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
 * ix86 core v0.9.1
 *
 * Original Authors (v0.6.2 and prior):
 *		linuzappz <linuzappz@pcsx.net>
 *		alexey silinov
 *		goldfinger
 *		zerofrog(@gmail.com)
 *
 * Authors of v0.9.1:
 *		Jake.Stine(@gmail.com)
 *		cottonvibes(@gmail.com)
 *		sudonim(1@gmail.com)
 */

#include "PrecompiledHeader.h"
#include "internal.h"
#include "tools.h"

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


__tls_emit u8*			x86Ptr;
__tls_emit XMMSSEType	g_xmmtypes[iREGCNT_XMM] = { XMMT_INT };

namespace x86Emitter {

template void xWrite<u8>( u8 val );
template void xWrite<u16>( u16 val );
template void xWrite<u32>( u32 val );
template void xWrite<u64>( u64 val );
template void xWrite<u128>( u128 val );

__fi void xWrite8( u8 val )
{
	xWrite( val );
}

__fi void xWrite16( u16 val )
{
	xWrite( val );
}

__fi void xWrite32( u32 val )
{
	xWrite( val );
}

__fi void xWrite64( u64 val )
{
	xWrite( val );
}

// Empty initializers are due to frivolously pointless GCC errors (it demands the
// objects be initialized even though they have no actual variable members).

const xAddressIndexer<xIndirectVoid>		ptr		= { };
const xAddressIndexer<xIndirect128>		ptr128	= { };
const xAddressIndexer<xIndirect64>		ptr64	= { };
const xAddressIndexer<xIndirect32>		ptr32	= { };
const xAddressIndexer<xIndirect16>		ptr16	= { };
const xAddressIndexer<xIndirect8>		ptr8	= { };

// ------------------------------------------------------------------------

const xRegisterEmpty xEmptyReg = { };

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

const char* xRegisterBase::GetName()
{
	if( Id == xRegId_Invalid ) return "invalid";
	if( Id == xRegId_Empty ) return "empty";

	// bad error?  Return a "big" error string.  Might break formatting of register tables
	// but that's the least of your worries if you see this baby.
	if( Id >= 8 || Id <= -3 ) return "!Register index out of range!";

	switch( GetOperandSize() )
	{
		case 1: return x86_regnames_gpr8[ Id ];
		case 2: return x86_regnames_gpr16[ Id ];
		case 4: return x86_regnames_gpr32[ Id ];
		case 8: return x86_regnames_mmx[ Id ];
		case 16: return x86_regnames_sse[ Id ];
	}

	return "oops?";
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

static __fi void ModRM( uint mod, uint reg, uint rm )
{
	xWrite8( (mod << 6) | (reg << 3) | rm );
}

static __fi void SibSB( u32 ss, u32 index, u32 base )
{
	xWrite8( (ss << 6) | (index << 3) | base );
}

void EmitSibMagic( uint regfield, const void* address )
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
__emitinline void xOpWrite0F( u8 prefix, u16 opcode, int instId, const xIndirectVoid& sib )
{
	SimdPrefix( prefix, opcode );
	EmitSibMagic( instId, sib );
}

__emitinline void xOpWrite0F( u8 prefix, u16 opcode, int instId, const void* data )
{
	SimdPrefix( prefix, opcode );
	EmitSibMagic( instId, data );
}

__emitinline void xOpWrite0F( u16 opcode, int instId, const xIndirectVoid& sib )
{
	xOpWrite0F( 0, opcode, instId, sib );
}


//////////////////////////////////////////////////////////////////////////////////////////
// returns TRUE if this instruction requires SIB to be encoded, or FALSE if the
// instruction ca be encoded as ModRm alone.
static __fi bool NeedsSibMagic( const xIndirectVoid& info )
{
	// no registers? no sibs!
	// (xIndirectVoid::Reduce always places a register in Index, and optionally leaves
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
void EmitSibMagic( uint regfield, const xIndirectVoid& info )
{
	pxAssertDev( regfield < 8, "Invalid x86 register identifier." );

	int displacement_size = (info.Displacement == 0) ? 0 :
		( ( info.IsByteSizeDisp() ) ? 1 : 2 );

	assert(!info.Base.IsEmpty() || !info.Index.IsEmpty() || displacement_size == 2);

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

// Writes a ModRM byte for "Direct" register access forms, which is used for all
// instructions taking a form of [reg,reg].
void EmitSibMagic( uint reg1, const xRegisterBase& reg2 )
{
	xWrite8( (Mod_Direct << 6) | (reg1 << 3) | reg2.Id );
}

void EmitSibMagic( const xRegisterBase& reg1, const xRegisterBase& reg2 )
{
	xWrite8( (Mod_Direct << 6) | (reg1.Id << 3) | reg2.Id );
}

void EmitSibMagic( const xRegisterBase& reg1, const void* src )
{
	EmitSibMagic( reg1.Id, src );
}

void EmitSibMagic( const xRegisterBase& reg1, const xIndirectVoid& sib )
{
	EmitSibMagic( reg1.Id, sib );
}

// --------------------------------------------------------------------------------------
//  xSetPtr / xAlignPtr / xGetPtr / xAdvancePtr
// --------------------------------------------------------------------------------------

// Assigns the current emitter buffer target address.
// This is provided instead of using x86Ptr directly, since we may in the future find
// a need to change the storage class system for the x86Ptr 'under the hood.'
__emitinline void xSetPtr( void* ptr )
{
	x86Ptr = (u8*)ptr;
}

// Retrieves the current emitter buffer target address.
// This is provided instead of using x86Ptr directly, since we may in the future find
// a need to change the storage class system for the x86Ptr 'under the hood.'
__emitinline u8* xGetPtr()
{
	return x86Ptr;
}

__emitinline void xAlignPtr( uint bytes )
{
	// forward align
	x86Ptr = (u8*)( ( (uptr)x86Ptr + bytes - 1) & ~(bytes - 1) );
}

// Performs best-case alignment for the target CPU, for use prior to starting a new
// function.  This is not meant to be used prior to jump targets, since it doesn't
// add padding (additionally, speed benefit from jump alignment is minimal, and often
// a loss).
__emitinline void xAlignCallTarget()
{
	// Core2/i7 CPUs prefer unaligned addresses.  Checking for SSSE3 is a decent filter.
	// (also align in debug modes for disasm convenience)

	if( IsDebugBuild || !x86caps.hasSupplementalStreamingSIMD3Extensions )
	{
		// - P4's and earlier prefer 16 byte alignment.
		// - AMD Athlons and Phenoms prefer 8 byte alignment, but I don't have an easy
		//   heuristic for it yet.
		// - AMD Phenom IIs are unknown (either prefer 8 byte, or unaligned).

		xAlignPtr( 16 );
	}
}

__emitinline u8* xGetAlignedCallTarget()
{
	xAlignCallTarget();
	return x86Ptr;
}

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

// --------------------------------------------------------------------------------------
//  xAddressReg  (operator overloads)
// --------------------------------------------------------------------------------------
xAddressVoid xAddressReg::operator+( const xAddressReg& right ) const
{
	pxAssertMsg( right.Id != -1 || Id != -1, "Uninitialized x86 register." );
	return xAddressVoid( *this, right );
}

xAddressVoid xAddressReg::operator+( s32 right ) const
{
	pxAssertMsg( Id != -1, "Uninitialized x86 register." );
	return xAddressVoid( *this, right );
}

xAddressVoid xAddressReg::operator+( const void* right ) const
{
	pxAssertMsg( Id != -1, "Uninitialized x86 register." );
	return xAddressVoid( *this, (sptr)right );
}

xAddressVoid xAddressReg::operator-( s32 right ) const
{
	pxAssertMsg( Id != -1, "Uninitialized x86 register." );
	return xAddressVoid( *this, -right );
}

xAddressVoid xAddressReg::operator-( const void* right ) const
{
	pxAssertMsg( Id != -1, "Uninitialized x86 register." );
	return xAddressVoid( *this, -(s32)right );
}

xAddressVoid xAddressReg::operator*( u32 factor ) const
{
	pxAssertMsg( Id != -1, "Uninitialized x86 register." );
	return xAddressVoid( xEmptyReg, *this, factor );
}

xAddressVoid xAddressReg::operator<<( u32 shift ) const
{
	pxAssertMsg( Id != -1, "Uninitialized x86 register." );
	return xAddressVoid( xEmptyReg, *this, 1<<shift );
}


// --------------------------------------------------------------------------------------
//  xAddressVoid  (method implementations)
// --------------------------------------------------------------------------------------

xAddressVoid::xAddressVoid( const xAddressReg& base, const xAddressReg& index, int factor, s32 displacement )
{
	Base		= base;
	Index		= index;
	Factor		= factor;
	Displacement= displacement;

	pxAssertMsg( base.Id != xRegId_Invalid, "Uninitialized x86 register." );
	pxAssertMsg( index.Id != xRegId_Invalid, "Uninitialized x86 register." );
}

xAddressVoid::xAddressVoid( const xAddressReg& index, int displacement )
{
	Base		= xEmptyReg;
	Index		= index;
	Factor		= 0;
	Displacement= displacement;

	pxAssertMsg( index.Id != xRegId_Invalid, "Uninitialized x86 register." );
}

xAddressVoid::xAddressVoid( s32 displacement )
{
	Base		= xEmptyReg;
	Index		= xEmptyReg;
	Factor		= 0;
	Displacement= displacement;
}

xAddressVoid::xAddressVoid( const void* displacement )
{
	Base		= xEmptyReg;
	Index		= xEmptyReg;
	Factor		= 0;
	Displacement= (s32)displacement;
}

xAddressVoid& xAddressVoid::Add( const xAddressReg& src )
{
	if( src == Index )
	{
		Factor++;
	}
	else if( src == Base )
	{
		// Compound the existing register reference into the Index/Scale pair.
		Base = xEmptyReg;

		if( src == Index )
			Factor++;
		else
		{
			pxAssertDev( Index.IsEmpty(), "x86Emitter: Only one scaled index register is allowed in an address modifier." );
			Index = src;
			Factor = 2;
		}
	}
	else if( Base.IsEmpty() )
		Base = src;
	else if( Index.IsEmpty() )
		Index = src;
	else
		pxFailDev( L"x86Emitter: address modifiers cannot have more than two index registers." );	// oops, only 2 regs allowed per ModRm!

	return *this;
}

xAddressVoid& xAddressVoid::Add( const xAddressVoid& src )
{
	Add( src.Base );
	Add( src.Displacement );

	// If the factor is 1, we can just treat index like a base register also.
	if( src.Factor == 1 )
	{
		Add( src.Index );
	}
	else if( Index.IsEmpty() )
	{
		Index = src.Index;
		Factor = src.Factor;
	}
	else if( Index == src.Index )
	{
		Factor += src.Factor;
	}
	else
		pxFailDev( L"x86Emitter: address modifiers cannot have more than two index registers." );	// oops, only 2 regs allowed per ModRm!

	return *this;
}

xIndirectVoid::xIndirectVoid( const xAddressVoid& src )
{
	Base		= src.Base;
	Index		= src.Index;
	Scale		= src.Factor;
	Displacement= src.Displacement;

	Reduce();
}

xIndirectVoid::xIndirectVoid( s32 disp )
{
	Base		= xEmptyReg;
	Index		= xEmptyReg;
	Scale		= 0;
	Displacement= disp;

	// no reduction necessary :D
}

xIndirectVoid::xIndirectVoid( xAddressReg base, xAddressReg index, int scale, s32 displacement )
{
	Base		= base;
	Index		= index;
	Scale		= scale;
	Displacement= displacement;

	Reduce();
}

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
void xIndirectVoid::Reduce()
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
			Base = xEmptyReg;
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

uint xIndirectVoid::GetOperandSize() const
{
	pxFail( "Invalid operation on xIndirectVoid" );
	return 0;
}

xIndirectVoid& xIndirectVoid::Add( s32 imm )
{
	Displacement += imm;
	return *this;
}

// ------------------------------------------------------------------------
// Internal implementation of EmitSibMagic which has been custom tailored
// to optimize special forms of the Lea instructions accordingly, such
// as when a LEA can be replaced with a "MOV reg,imm" or "MOV reg,reg".
//
// preserve_flags - set to ture to disable use of SHL on [Index*Base] forms
// of LEA, which alters flags states.
//
static void EmitLeaMagic( const xRegisterInt& to, const xIndirectVoid& src, bool preserve_flags )
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
			xMOV( to, src.Displacement );
			return;
		}
		else if( displacement_size == 0 )
		{
			_xMovRtoR( to, src.Index );
			return;
		}
		else
		{
			if( !preserve_flags )
			{
				// encode as MOV and ADD combo.  Make sure to use the immediate on the
				// ADD since it can encode as an 8-bit sign-extended value.

				_xMovRtoR( to, src.Index );
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

				_xMovRtoR( to, src.Index );
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
						_xMovRtoR( to, src.Base );	// will do the trick!
						if( src.Displacement ) xADD( to, src.Displacement );
						return;
					}
					else if( src.Displacement == 0 )
					{
						_xMovRtoR( to, src.Base );
						_g1_EmitOp( G1Type_ADD, to, src.Index );
						return;
					}
				}
				else if( (src.Index == esp) && (src.Displacement == 0) )
				{
					// special case handling of ESP as Index, which is replaceable with
					// a single MOV even when preserve_flags is set! :D

					_xMovRtoR( to, src.Base );
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

__emitinline void xLEA( xRegister32 to, const xIndirectVoid& src, bool preserve_flags )
{
	EmitLeaMagic( to, src, preserve_flags );
}


__emitinline void xLEA( xRegister16 to, const xIndirectVoid& src, bool preserve_flags )
{
	xWrite8( 0x66 );
	EmitLeaMagic( to, src, preserve_flags );
}

// =====================================================================================================
//  TEST / INC / DEC
// =====================================================================================================
void xImpl_Test::operator()( const xRegister8& to, const xRegister8& from ) const
{
	xWrite8( 0x84 );
	EmitSibMagic( from, to );
}

void xImpl_Test::operator()( const xRegister16& to, const xRegister16& from ) const
{
	to.prefix16();
	xWrite8( 0x85 );
	EmitSibMagic( from, to );
}

void xImpl_Test::operator()( const xRegister32& to, const xRegister32& from ) const
{
	xWrite8( 0x85 );
	EmitSibMagic( from, to );
}

void xImpl_Test::operator()( const xIndirect32orLess& dest, int imm ) const
{
	dest.prefix16();
	xWrite8( dest.Is8BitOp() ? 0xf6 : 0xf7 );
	EmitSibMagic( 0, dest );
	dest.xWriteImm( imm );
}

void xImpl_Test::operator()( const xRegisterInt& to, int imm ) const
{
	to.prefix16();

	if( to.IsAccumulator() )
		xWrite8( to.Is8BitOp() ? 0xa8 : 0xa9 );
	else
	{
		xWrite8( to.Is8BitOp() ? 0xf6 : 0xf7 );
		EmitSibMagic( 0, to );
	}
	to.xWriteImm( imm );
}

void xImpl_BitScan::operator()( const xRegister32& to, const xRegister32& from ) const		{ xOpWrite0F( Opcode, to, from ); }
void xImpl_BitScan::operator()( const xRegister16& to, const xRegister16& from ) const		{ xOpWrite0F( 0x66, Opcode, to, from ); }
void xImpl_BitScan::operator()( const xRegister16or32& to, const xIndirectVoid& sibsrc ) const
{
	xOpWrite0F( (to->GetOperandSize() == 2) ? 0x66 : 0x00, Opcode, to, sibsrc );
}

void xImpl_IncDec::operator()( const xRegisterInt& to ) const
{
	if( to.Is8BitOp() )
	{
		xWrite8( 0xfe );
		EmitSibMagic( isDec ? 1 : 0, to );
	}
	else
	{
		to.prefix16();
		xWrite8( (isDec ? 0x48 : 0x40) | to.Id );
	}
}

void xImpl_IncDec::operator()( const xIndirect32orLess& to ) const
{
	to.prefix16();
	xWrite8( to.Is8BitOp() ? 0xfe : 0xff );
	EmitSibMagic( isDec ? 1 : 0, to );
}

void xImpl_DwordShift::operator()( const xRegister32& to,	const xRegister32& from, const xRegisterCL& /* clreg */ ) const	{ xOpWrite0F( OpcodeBase+1, to, from ); }
void xImpl_DwordShift::operator()( const xRegister16& to,	const xRegister16& from, const xRegisterCL& /* clreg */ ) const	{ xOpWrite0F( 0x66, OpcodeBase+1, to, from ); }
void xImpl_DwordShift::operator()( const xRegister32& to,	const xRegister32& from, u8 shiftcnt ) const
{
	if( shiftcnt != 0 )
		xOpWrite0F( OpcodeBase, to, from );
}
void xImpl_DwordShift::operator()( const xRegister16& to,	const xRegister16& from, u8 shiftcnt ) const
{
	if( shiftcnt != 0 )
		xOpWrite0F( 0x66, OpcodeBase, to, from );
}

void xImpl_DwordShift::operator()( const xIndirectVoid& dest, const xRegister16or32& from, const xRegisterCL& /* clreg */ ) const
{
	xOpWrite0F( (from->GetOperandSize() == 2) ? 0x66 : 0x00, OpcodeBase, from, dest );
}

void xImpl_DwordShift::operator()( const xIndirectVoid& dest, const xRegister16or32& from, u8 shiftcnt ) const
{
	if( shiftcnt != 0 )
		xOpWrite0F( (from->GetOperandSize() == 2) ? 0x66 : 0x00, OpcodeBase, from, dest, shiftcnt );
}

const xImpl_Test		xTEST	= { };

const xImpl_BitScan		xBSF	= { 0xbc };
const xImpl_BitScan		xBSR	= { 0xbd };

const xImpl_IncDec		xINC 	= { false };
const xImpl_IncDec		xDEC	= { true };

const xImpl_DwordShift	xSHLD	= { 0xa4 };
const xImpl_DwordShift	xSHRD	= { 0xac };

//////////////////////////////////////////////////////////////////////////////////////////
// Push / Pop Emitters
//
// Note: pushad/popad implementations are intentionally left out.  The instructions are
// invalid in x64, and are super slow on x32.  Use multiple Push/Pop instructions instead.

__emitinline void xPOP( const xIndirectVoid& from )
{
	xWrite8( 0x8f );
	EmitSibMagic( 0, from );
}

__emitinline void xPUSH( const xIndirectVoid& from )
{
	xWrite8( 0xff );
	EmitSibMagic( 6, from );
}

__fi void xPOP( xRegister32 from )		{ xWrite8( 0x58 | from.Id ); }

__fi void xPUSH( u32 imm )				{ xWrite8( 0x68 ); xWrite32( imm ); }
__fi void xPUSH( xRegister32 from )	{ xWrite8( 0x50 | from.Id ); }

// pushes the EFLAGS register onto the stack
__fi void xPUSHFD()					{ xWrite8( 0x9C ); }
// pops the EFLAGS register from the stack
__fi void xPOPFD()						{ xWrite8( 0x9D ); }


//////////////////////////////////////////////////////////////////////////////////////////
//

__fi void xLEAVE()	{ xWrite8( 0xC9 ); }
__fi void xRET()	{ xWrite8( 0xC3 ); }
__fi void xCBW()	{ xWrite16( 0x9866 );  }
__fi void xCWD()	{ xWrite8( 0x98 ); }
__fi void xCDQ()	{ xWrite8( 0x99 ); }
__fi void xCWDE()	{ xWrite8( 0x98 ); }

__fi void xLAHF()	{ xWrite8( 0x9f ); }
__fi void xSAHF()	{ xWrite8( 0x9e ); }

__fi void xSTC()	{ xWrite8( 0xF9 ); }
__fi void xCLC()	{ xWrite8( 0xF8 ); }

// NOP 1-byte
__fi void xNOP()	{ xWrite8(0x90); }

__fi void xINT( u8 imm )
{
	if (imm == 3)
		xWrite8(0xcc);
	else
	{
		xWrite8(0xcd);
		xWrite8(imm);
	}
}

__fi void xINTO()	{ xWrite8(0xce); }

__emitinline void xBSWAP( const xRegister32& to )
{
	xWrite8( 0x0F );
	xWrite8( 0xC8 | to.Id );
}

static __aligned16 u64 xmm_data[iREGCNT_XMM*2];

__emitinline void xStoreReg( const xRegisterSSE& src )
{
	xMOVDQA( ptr[&xmm_data[src.Id*2]], src );
}

__emitinline void xRestoreReg( const xRegisterSSE& dest )
{
	xMOVDQA( dest, ptr[&xmm_data[dest.Id*2]] );
}

}
