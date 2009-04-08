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
 * ix86 core v0.6.2
 * Authors: linuzappz <linuzappz@pcsx.net>
 *			alexey silinov
 *			goldfinger
 *			zerofrog(@gmail.com)
 *			cottonvibes(@gmail.com)
 */

#include "PrecompiledHeader.h"

#include "System.h"
#include "ix86_internal.h"

__threadlocal u8  *x86Ptr;
__threadlocal u8  *j8Ptr[32];
__threadlocal u32 *j32Ptr[32];

PCSX2_ALIGNED16(u32 p[4]);
PCSX2_ALIGNED16(u32 p2[4]);
PCSX2_ALIGNED16(float f[4]);

XMMSSEType g_xmmtypes[XMMREGS] = { XMMT_INT };

namespace x86Emitter {

x86IndexerType ptr;

//////////////////////////////////////////////////////////////////////////////////////////
//
const x86Register32 x86Register32::Empty( -1 );

const x86Register32 eax( 0 );
const x86Register32 ebx( 3 );
const x86Register32 ecx( 1 );
const x86Register32 edx( 2 );
const x86Register32 esi( 6 );
const x86Register32 edi( 7 );
const x86Register32 ebp( 5 );
const x86Register32 esp( 4 );

const x86Register16 ax( 0 );
const x86Register16 bx( 3 );
const x86Register16 cx( 1 );
const x86Register16 dx( 2 );
const x86Register16 si( 6 );
const x86Register16 di( 7 );
const x86Register16 bp( 5 );
const x86Register16 sp( 4 );

const x86Register8 al( 0 );
const x86Register8 cl( 1 );
const x86Register8 dl( 2 );
const x86Register8 bl( 3 );
const x86Register8 ah( 4 );
const x86Register8 ch( 5 );
const x86Register8 dh( 6 );
const x86Register8 bh( 7 );

//////////////////////////////////////////////////////////////////////////////////////////
// x86Register Method Implementations
//
x86ModRm x86Register32::operator+( const x86Register32& right ) const
{
	return x86ModRm( *this, right );
}

x86ModRm x86Register32::operator+( const x86ModRm& right ) const
{
	return right + *this;
}

x86ModRm x86Register32::operator+( s32 right ) const
{
	return x86ModRm( *this, right );
}

x86ModRm x86Register32::operator*( u32 right ) const
{
	return x86ModRm( Empty, *this, right );
}

//////////////////////////////////////////////////////////////////////////////////////////
// x86ModRm Method Implementations
//
x86ModRm& x86ModRm::Add( const x86IndexReg& src )
{
	if( src == Index )
	{
		Factor++;
	}
	else if( src == Base )
	{
		// Compound the existing register reference into the Index/Scale pair.
		Base = x86IndexReg::Empty;

		if( src == Index )
			Factor++;
		else
		{
			jASSUME( Index.IsEmpty() );		// or die if we already have an index!
			Index = src;
			Factor = 2;
		}
	}
	else if( Base.IsEmpty() )
		Base = src;
	else if( Index.IsEmpty() )
		Index = src;
	else
		assert( false );	// oops, only 2 regs allowed per ModRm!

	return *this;
}

x86ModRm& x86ModRm::Add( const x86ModRm& src )
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
		Factor = 1;
	}
	else if( Index == src.Index )
		Factor++;
	else
		assert( false );	// oops, only 2 regs allowed!

	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////
// ModSib Method Implementations
//

// ------------------------------------------------------------------------
// Generates a 'reduced' ModSib form, which has valid Base, Index, and Scale values.
// Necessary because by default ModSib compounds registers into Index when possible.
//
void ModSib::Reduce()
{
	// If no index reg, then load the base register into the index slot.
	if( Index.IsEmpty() )
	{
		Index = Base;
		Scale = 0;
		Base = x86IndexReg::Empty;
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

ModSib::ModSib( const x86ModRm& src ) :
	Base( src.Base ),
	Index( src.Index ),
	Scale( src.Factor ),
	Displacement( src.Displacement )
{
	Reduce();
}

ModSib::ModSib( x86IndexReg base, x86IndexReg index, int scale, s32 displacement ) :
	Base( base ),
	Index( index ),
	Scale( scale ),
	Displacement( displacement )
{
	Reduce();
}

ModSib::ModSib( s32 displacement ) :
	Base(),
	Index(),
	Scale(0),
	Displacement( displacement )
{
}

// ------------------------------------------------------------------------
// returns TRUE if this instruction requires SIB to be encoded, or FALSE if the
// instruction ca be encoded as ModRm alone.
bool NeedsSibMagic( const ModSib& info )
{
	// no registers? no sibs!
	if( info.Index.IsEmpty() ) return false;

	// A scaled register needs a SIB
	if( info.Scale != 0 ) return true;

	// two registers needs a SIB
	if( !info.Base.IsEmpty() ) return true;

	// If index register is ESP, then we need a SIB:
	// (the ModSib::Reduce() ensures that stand-alone ESP will be in the
	// index position for us)
	if( info.Index == esp ) return true;

	return false;
}

// ------------------------------------------------------------------------
// Conditionally generates Sib encoding information!
//
// regfield - register field to be written to the ModRm.  This is either a register specifier
//   or an opcode extension.  In either case, the instruction determines the value for us.
//
void EmitSibMagic( int regfield, const ModSib& info )
{
	int displacement_size = (info.Displacement == 0) ? 0 : 
		( ( info.IsByteSizeDisp() ) ? 1 : 2 );

	if( !NeedsSibMagic( info ) )
	{
		// Use ModRm-only encoding, with the rm field holding an index/base register, if
		// one has been specified.  If neither register is specified then use Disp32 form,
		// which is encoded as "EBP w/o displacement" (which is why EBP must always be
		// encoded *with* a displacement of 0, if it would otherwise not have one).

		if( info.Index.IsEmpty() )
			ModRM( 0, regfield, ModRm_UseDisp32 );
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
			displacement_size = 2;
		}
		else
		{
			if( info.Base == ebp && displacement_size == 0 )
				displacement_size = 1;		// forces [ebp] to be encoded as [ebp+0]!

			ModRM( displacement_size, regfield, ModRm_UseSib );
			SibSB( info.Scale, info.Index.Id, info.Base.Id );
		}
	}

	switch( displacement_size )
	{
		case 0: break;
		case 1: write8( info.Displacement );  break;
		case 2: write32( info.Displacement ); break;
		jNO_DEFAULT
	}
}

// ------------------------------------------------------------------------
// Conditionally generates Sib encoding information!
//
// regfield - register field to be written to the ModRm.  This is either a register specifier
//   or an opcode extension.  In either case, the instruction determines the value for us.
//
emitterT void EmitSibMagic( x86Register32 regfield, const ModSib& info )
{
	EmitSibMagic( regfield.Id, info );
}

template< typename ToReg >
static void EmitLeaMagic( ToReg to, const ModSib& src, bool is16bit=false )
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
			if( is16bit )
				MOV16ItoR( to.Id, src.Displacement );
			else
				MOV32ItoR( to.Id, src.Displacement );
			return;
		}
		else if( displacement_size == 0 )
		{
			if( is16bit )
				MOV16RtoR( to.Id, src.Index.Id );
			else
				MOV32RtoR( to.Id, src.Index.Id );
			return;
		}
		else
		{
			// note: no need to do ebp+0 check since we encode all 0 displacements as
			// register assignments above (via MOV)

			write8( 0x8d );
			ModRM( displacement_size, to.Id, src.Index.Id );
		}
	}
	else
	{
		if( src.Base.IsEmpty() )
		{
			if( displacement_size == 0 )
			{
				// Encode [Index*Scale] as a combination of Mov and Shl.
				// This is more efficient because of the bloated format which requires
				// a 32 bit displacement.

				if( is16bit )
				{
					MOV16RtoR( to.Id, src.Index.Id );
					SHL16ItoR( to.Id, src.Scale );
				}
				else
				{
					MOV32RtoR( to.Id, src.Index.Id );
					SHL32ItoR( to.Id, src.Scale );
				}
				return;
			}

			write8( 0x8d );
			ModRM( 0, to.Id, ModRm_UseSib );
			SibSB( src.Scale, src.Index.Id, ModRm_UseDisp32 );
			displacement_size = 2;		// force 32bit displacement.
		}
		else
		{
			if( src.Base == ebp && displacement_size == 0 )
				displacement_size = 1;		// forces [ebp] to be encoded as [ebp+0]!

			write8( 0x8d );
			ModRM( displacement_size, to.Id, ModRm_UseSib );
			SibSB( src.Scale, src.Index.Id, src.Base.Id );
		}
	}
	
	switch( displacement_size )
	{
		case 0: break;
		case 1: write8( src.Displacement );  break;
		case 2: write32( src.Displacement ); break;
		jNO_DEFAULT
	}

}

emitterT void LEA32( x86Register32 to, const ModSib& src )
{
	EmitLeaMagic( to, src );
}


emitterT void LEA16( x86Register16 to, const ModSib& src )
{
	// fixme: is this right?  Does Lea16 use 32 bit displacement and ModRM form?
	
	write8( 0x66 );
	EmitLeaMagic( to, src );
}

//////////////////////////////////////////////////////////////////////////////////////////
// Miscellaneous Section!
// Various Instructions with no parameter and no special encoding logic.
//
emitterT void RET()		{ write8( 0xC3 ); }
emitterT void CBW()		{ write16( 0x9866 );  }
emitterT void CWD()		{ write8( 0x98 ); }
emitterT void CDQ()		{ write8( 0x99 ); }
emitterT void CWDE()	{ write8( 0x98 ); }

emitterT void LAHF()	{ write8( 0x9f ); }
emitterT void SAHF()	{ write8( 0x9e ); }


//////////////////////////////////////////////////////////////////////////////////////////
// Push / Pop Emitters
//
// fixme?  push/pop instructions always push and pop aligned to whatever mode the cpu
// is running in.  So even thought these say push32, they would essentially be push64 on
// an x64 build.  Should I rename them accordingly?  --air
//
// Note: pushad/popad implementations are intentionally left out.  The instructions are
// invalid in x64, and are super slow on x32.  Use multiple Push/Pop instructions instead.


emitterT void POP( x86Register32 from )
{
	write8( 0x58 | from.Id );
}

emitterT void POP( const ModSib& from )
{
	write8( 0x8f ); EmitSibMagic( 0, from );
}

emitterT void PUSH( u32 imm )
{
	write8( 0x68 ); write32( imm );
}

emitterT void PUSH( x86Register32 from )
{
	write8( 0x50 | from.Id );
}

emitterT void PUSH( const ModSib& from )
{
	write8( 0xff ); EmitSibMagic( 6, from );
}

// pushes the EFLAGS register onto the stack
emitterT void PUSHFD() { write8( 0x9C ); }
// pops the EFLAGS register from the stack
emitterT void POPFD() { write8( 0x9D ); }

}
