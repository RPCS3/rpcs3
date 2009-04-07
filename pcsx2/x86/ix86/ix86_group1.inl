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

#pragma once

//------------------------------------------------------------------
// x86 Group 1 Instructions
//------------------------------------------------------------------
// Group 1 instructions all adhere to the same encoding scheme, and so they all
// share the same emitter which has been coded here.
//
// Group 1 Table:  [column value is the Reg field of the ModRM byte]
//
//    0    1    2    3    4    5    6    7
//   ADD  OR   ADC  SBB  AND  SUB  XOR  CMP
//

namespace x86Emitter {

static const int ModRm_UseSib = 4;		// same index value as ESP (used in RM field)
static const int ModRm_UseDisp32 = 5;	// same index value as EBP (used in Mod field)

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

	if( !NeedsSibMagic<I>( info ) )
	{
		// Use ModRm-only encoding, with the rm field holding an index/base register, if
		// one has been specified.  If neither register is specified then use Disp32 form,
		// which is encoded as "EBP w/o displacement" (which is why EBP must always be
		// encoded *with* a displacement of 0, if it would otherwise not have one).

		x86Register basereg = info.GetEitherReg();

		if( basereg.IsEmpty() )
			ModRM<I>( 0, regfield, ModRm_UseDisp32 );
		else
		{
			if( basereg == ebp && displacement_size == 0 )
				displacement_size = 1;		// forces [ebp] to be encoded as [ebp+0]!

			ModRM<I>( displacement_size, regfield, basereg.Id );
		}
	}
	else
	{
		ModRM<I>( displacement_size, regfield, ModRm_UseSib );
		SibSB<I>( info.Index.Id, info.Scale, info.Base.Id );
	}

	switch( displacement_size )
	{
	case 0: break;
	case 1: write8<I>( info.Displacement );  break;
	case 2: write32<I>( info.Displacement ); break;
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
	EmitSibMagic<I>( regfield.Id, info );
}

enum Group1InstructionType
{
	G1Type_ADD=0,
	G1Type_OR,
	G1Type_ADC,
	G1Type_SBB,
	G1Type_AND,
	G1Type_SUB,
	G1Type_XOR,
	G1Type_CMP
};


emitterT void Group1_32( Group1InstructionType inst, x86Register to, x86Register from ) 
{
	write8<I>( 0x01 | (inst<<3) ); 
	ModRM<I>( 3, from.Id, to.Id );
}

emitterT void Group1_32( Group1InstructionType inst, x86Register to, u32 imm )
{
	if( is_s8( imm ) )
	{
		write8<I>( 0x83 );
		ModRM<I>( 3, inst, to.Id );
		write8<I>( (s8)imm );
	}
	else
	{
		if( to == eax )
			write8<I>( 0x05 | (inst<<3) );
		else
		{
			write8<I>( 0x81 );
			ModRM<I>( 3, inst, to.Id );
		}
		write32<I>( imm );
	}
}

emitterT void Group1_32( Group1InstructionType inst, const ModSib& sibdest, u32 imm )
{
	write8<I>( is_s8( imm ) ? 0x83 : 0x81 );

	EmitSibMagic<I>( inst, sibdest );

	if( is_s8( imm ) )
		write8<I>( (s8)imm );
	else
		write32<I>( imm );
}

emitterT void Group1_32( Group1InstructionType inst, const ModSib& sibdest, x86Register from )
{
	write8<I>( 0x01 | (inst<<3) ); 
	EmitSibMagic<I>( from, sibdest );
}

/* add m32 to r32 */
emitterT void Group1_32( Group1InstructionType inst, x86Register to, const ModSib& sibsrc ) 
{
	write8<I>( 0x03 | (inst<<3) );
	EmitSibMagic<I>( to, sibsrc );
}

emitterT void Group1_8( Group1InstructionType inst, x86Register to, s8 imm )
{
	if( to == eax )
	{
		write8<I>( 0x04 | (inst<<3) );
		write8<I>( imm );
	}
	else
	{
		write8<I>( 0x80 );
		ModRM<I>( 3, inst, to.Id );
		write8<I>( imm );
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
//
#define DEFINE_GROUP1_OPCODE( lwr, cod ) \
	emitterT void lwr##32( x86Register to, x86Register from )      { Group1_32<I>( G1Type_##cod, to,		from		); } \
	emitterT void lwr##32( x86Register to, u32 imm )               { Group1_32<I>( G1Type_##cod, to,		imm			); } \
	emitterT void lwr##32( x86Register to, void* from )            { Group1_32<I>( G1Type_##cod, to,		ptr[from]	); } \
	emitterT void lwr##32( void* to, x86Register from )            { Group1_32<I>( G1Type_##cod, ptr[to],	from		); } \
	emitterT void lwr##32( void* to, u32 imm )                     { Group1_32<I>( G1Type_##cod, ptr[to],	imm			); } \
	emitterT void lwr##32( x86Register to, const x86ModRm& from )  { Group1_32<I>( G1Type_##cod, to,		ptr[from]	); } \
	emitterT void lwr##32( const x86ModRm& to, x86Register from )  { Group1_32<I>( G1Type_##cod, ptr[to],	from		); } \
	emitterT void lwr##32( const x86ModRm& to, u32 imm )           { Group1_32<I>( G1Type_##cod, ptr[to],	imm			); }

DEFINE_GROUP1_OPCODE( add, ADD );
DEFINE_GROUP1_OPCODE( cmp, CMP );
DEFINE_GROUP1_OPCODE( or,  OR );
DEFINE_GROUP1_OPCODE( adc, ADC );
DEFINE_GROUP1_OPCODE( sbb, SBB );
DEFINE_GROUP1_OPCODE( and, AND );
DEFINE_GROUP1_OPCODE( sub, SUB );
DEFINE_GROUP1_OPCODE( xor, XOR );

}		// end namespace x86Emitter


static __forceinline x86Emitter::x86Register _reghlp( x86IntRegType src )
{
	return x86Emitter::x86Register( src );
}


static __forceinline x86Emitter::x86ModRm _mrmhlp( x86IntRegType src )
{
	return x86Emitter::x86ModRm( _reghlp(src) );
}

//////////////////////////////////////////////////////////////////////////////////////////
//
#define DEFINE_GROUP1_OPCODE_LEGACY( lwr, cod ) \
	emitterT void e##cod##32RtoR( x86IntRegType to, x86IntRegType from )	{ x86Emitter::lwr##32<I>( _reghlp(to), _reghlp(from) ); } \
	emitterT void e##cod##32ItoR( x86IntRegType to, u32 imm )				{ x86Emitter::lwr##32<I>( _reghlp(to), imm ); } \
	emitterT void e##cod##32MtoR( x86IntRegType to, uptr from )				{ x86Emitter::lwr##32<I>( _reghlp(to), (void*)from ); } \
	emitterT void e##cod##32RtoM( uptr to, x86IntRegType from )				{ x86Emitter::lwr##32<I>( (void*)to, _reghlp(from) ); } \
	emitterT void e##cod##32ItoM( uptr to, u32 imm )						{ x86Emitter::lwr##32<I>( (void*)to, imm ); } \
	emitterT void e##cod##32ItoRm( x86IntRegType to, u32 imm, int offset=0 ){ x86Emitter::lwr##32<I>( _mrmhlp(to) + offset, imm ); } \
	emitterT void e##cod##32RmtoR( x86IntRegType to, x86IntRegType from, int offset=0 ) { x86Emitter::lwr##32<I>( _reghlp(to), _mrmhlp(from) + offset ); } \
	emitterT void e##cod##32RtoRm( x86IntRegType to, x86IntRegType from, int offset=0 ) { x86Emitter::lwr##32<I>( _mrmhlp(to) + offset, _reghlp(from) ); }

DEFINE_GROUP1_OPCODE_LEGACY( add, ADD );
DEFINE_GROUP1_OPCODE_LEGACY( cmp, CMP );
DEFINE_GROUP1_OPCODE_LEGACY( or, OR );
DEFINE_GROUP1_OPCODE_LEGACY( adc, ADC );
DEFINE_GROUP1_OPCODE_LEGACY( sbb, SBB );
DEFINE_GROUP1_OPCODE_LEGACY( and, AND );
DEFINE_GROUP1_OPCODE_LEGACY( sub, SUB );
DEFINE_GROUP1_OPCODE_LEGACY( xor, XOR );

emitterT void eAND32I8toR( x86IntRegType to, s8 from ) 
{
	x86Emitter::and32<I>( _reghlp(to), from );
}

emitterT void eAND32I8toM( uptr to, s8 from ) 
{
	x86Emitter::and32<I>( (void*)to, from );
}
