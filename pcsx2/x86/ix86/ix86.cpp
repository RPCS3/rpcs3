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
const x86Register x86Register::Empty( -1 );

const x86Register eax( 0 );
const x86Register ebx( 3 );
const x86Register ecx( 1 );
const x86Register edx( 2 );
const x86Register esi( 6 );
const x86Register edi( 7 );
const x86Register ebp( 5 );
const x86Register esp( 4 );

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
x86ModRm x86Register::operator+( const x86Register& right ) const
{
	return x86ModRm( *this, right );
}

x86ModRm x86Register::operator+( const x86ModRm& right ) const
{
	return right + *this;
}

//////////////////////////////////////////////////////////////////////////////////////////
// x86ModRm Method Implementations
//
x86ModRm& x86ModRm::Add( const x86Register& src )
{
	if( src == Index )
	{
		Factor++;
	}
	else if( src == Base )
	{
		// Compound the existing register reference into the Index/Scale pair.
		Base = x86Register::Empty;

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
	// If no index reg, then nothing for us to do...
	if( Index.IsEmpty() || Scale == 0 ) return;
	
	// The Scale has a series of valid forms, all shown here:
	
	switch( Scale )
	{
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

ModSib::ModSib( x86Register base, x86Register index, int scale, s32 displacement ) :
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

x86Register ModSib::GetEitherReg() const
{
	return Base.IsEmpty() ? Base : Index;
}

// ------------------------------------------------------------------------
// returns TRUE if this instruction requires SIB to be encoded, or FALSE if the
// instruction ca be encoded as ModRm alone.
emitterT bool NeedsSibMagic( const ModSib& info )
{
	// no registers? no sibs!
	if( info.Base.IsEmpty() && info.Index.IsEmpty() ) return false;

	// A scaled register needs a SIB
	if( info.Scale != 0 && !info.Index.IsEmpty() ) return true;

	// two registers needs a SIB
	if( !info.Base.IsEmpty() && !info.Index.IsEmpty() ) return true;

	// If register is ESP, then we need a SIB:
	if( info.Base == esp || info.Index == esp ) return true;

	return false;
}

// ------------------------------------------------------------------------
// Conditionally generates Sib encoding information!
//
// regfield - register field to be written to the ModRm.  This is either a register specifier
//   or an opcode extension.  In either case, the instruction determines the value for us.
//
emitterT void EmitSibMagic( int regfield, const ModSib& info )
{
	int displacement_size = (info.Displacement == 0) ? 0 : 
		( ( info.IsByteSizeDisp() ) ? 1 : 2 );

	if( !NeedsSibMagic( info ) )
	{
		// Use ModRm-only encoding, with the rm field holding an index/base register, if
		// one has been specified.  If neither register is specified then use Disp32 form,
		// which is encoded as "EBP w/o displacement" (which is why EBP must always be
		// encoded *with* a displacement of 0, if it would otherwise not have one).

		x86Register basereg = info.GetEitherReg();

		if( basereg.IsEmpty() )
			ModRM( 0, regfield, ModRm_UseDisp32 );
		else
		{
			if( basereg == ebp && displacement_size == 0 )
				displacement_size = 1;		// forces [ebp] to be encoded as [ebp+0]!

			ModRM( displacement_size, regfield, basereg.Id );
		}
	}
	else
	{
		ModRM( displacement_size, regfield, ModRm_UseSib );
		SibSB( info.Index.Id, info.Scale, info.Base.Id );
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
emitterT void EmitSibMagic( x86Register regfield, const ModSib& info )
{
	EmitSibMagic( regfield.Id, info );
}

}
