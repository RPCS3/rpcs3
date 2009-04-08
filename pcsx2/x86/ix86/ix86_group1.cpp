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

#include "PrecompiledHeader.h"
#include "ix86_internal.h"

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

//////////////////////////////////////////////////////////////////////////////////////////
// x86RegConverter - this class is used internally by the emitter as a helper for
// converting 8 and 16 register forms into 32 bit forms.  This way the end-user exposed API
// can use type-safe 8/16/32 bit register types, and the underlying code can use a single
// unified emitter to generate all function variations + prefixes and such. :)
//
class x86RegConverter : public x86Register32
{
public:
	x86RegConverter( x86Register32 src ) : x86Register32( src ) {}
	x86RegConverter( x86Register16 src ) : x86Register32( src.Id ) {}
	x86RegConverter( x86Register8 src )  : x86Register32( src.Id ) {}
};

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


static emitterT void Group1( Group1InstructionType inst, x86RegConverter to, x86RegConverter from, bool bit8form=false ) 
{
	write8( (bit8form ? 0 : 1) | (inst<<3) ); 
	ModRM( 3, from.Id, to.Id );
}

static emitterT void Group1( Group1InstructionType inst, const ModSib& sibdest, x86RegConverter from, bool bit8form=false )
{
	write8( (bit8form ? 0 : 1) | (inst<<3) ); 
	EmitSibMagic( from, sibdest );
}

static emitterT void Group1( Group1InstructionType inst, x86RegConverter to, const ModSib& sibsrc, bool bit8form=false )
{
	write8( (bit8form ? 2 : 3) | (inst<<3) );
	EmitSibMagic( to, sibsrc );
}

// Note: this function emits based on the operand size of imm, so 16 bit imms generate a 16 bit
// instruction (AX,BX,etc).
template< typename T >
static emitterT void Group1_Imm( Group1InstructionType inst, x86RegConverter to, T imm )
{
	bool bit8form = (sizeof(T) == 1);

	if( !bit8form && is_s8( imm ) )
	{
		write8( 0x83 );
		ModRM( 3, inst, to.Id );
		write8( (s8)imm );
	}
	else
	{
		if( to == eax )
			write8( (bit8form ? 4 : 5) | (inst<<3) );
		else
		{
			write8( bit8form ? 0x80 : 0x81 );
			ModRM( 3, inst, to.Id );
		}
		x86write<T>( imm );
	}
}

// Note: this function emits based on the operand size of imm, so 16 bit imms generate a 16 bit
// instruction (AX,BX,etc).
template< typename T >
static emitterT void Group1_Imm( Group1InstructionType inst, const ModSib& sibdest, T imm )
{
	bool bit8form = (sizeof(T) == 1);

	write8( bit8form ? 0x80 : (is_s8( imm ) ? 0x83 : 0x81) );

	EmitSibMagic( inst, sibdest );

	if( !bit8form && is_s8( imm ) )
		write8( (s8)imm );
	else
		x86write<T>( imm );
}

// 16 bit instruction prefix!
static __forceinline void prefix16() { write8(0x66); }

//////////////////////////////////////////////////////////////////////////////////////////
//
#define DEFINE_GROUP1_OPCODE( cod ) \
	emitterT void cod##32( x86Register32 to, x86Register32 from )  { Group1( G1Type_##cod, to,	from	); } \
	emitterT void cod##32( x86Register32 to, void* from )          { Group1( G1Type_##cod, to,	ptr[from]	); } \
	emitterT void cod##32( x86Register32 to, const ModSib& from )  { Group1( G1Type_##cod, to,	from	); } \
	emitterT void cod##32( x86Register32 to, u32 imm )             { Group1_Imm( G1Type_##cod, to,	imm	); } \
	emitterT void cod##32( const ModSib& to, x86Register32 from )  { Group1( G1Type_##cod, to,	from	); } \
	emitterT void cod##32( void* to, x86Register32 from )          { Group1( G1Type_##cod, ptr[to],	from	); } \
	emitterT void cod##32( void* to, u32 imm )                     { Group1_Imm( G1Type_##cod, ptr[to],	imm	); } \
	emitterT void cod##32( const ModSib& to, u32 imm )             { Group1_Imm( G1Type_##cod, to,	imm	); } \
 \
	emitterT void cod##16( x86Register16 to, x86Register16 from )  { prefix16(); Group1( G1Type_##cod, to,	from	); } \
	emitterT void cod##16( x86Register16 to, void* from )          { prefix16(); Group1( G1Type_##cod, to,	ptr[from]	); } \
	emitterT void cod##16( x86Register16 to, const ModSib& from )  { prefix16(); Group1( G1Type_##cod, to,	from	); } \
	emitterT void cod##16( x86Register16 to, u16 imm )             { prefix16(); Group1_Imm( G1Type_##cod, to,	imm	); } \
	emitterT void cod##16( const ModSib& to, x86Register16 from )  { prefix16(); Group1( G1Type_##cod, to,	from	); } \
	emitterT void cod##16( void* to, x86Register16 from )          { prefix16(); Group1( G1Type_##cod, ptr[to],	from	); } \
	emitterT void cod##16( void* to, u16 imm )                     { prefix16(); Group1_Imm( G1Type_##cod, ptr[to],	imm	); } \
	emitterT void cod##16( const ModSib& to, u16 imm )             { prefix16(); Group1_Imm( G1Type_##cod, to,	imm	); } \
 \
	emitterT void cod##8( x86Register8 to, x86Register8 from )     { Group1( G1Type_##cod, to,	from	, true ); } \
	emitterT void cod##8( x86Register8 to, void* from )            { Group1( G1Type_##cod, to,	ptr[from], true ); } \
	emitterT void cod##8( x86Register8 to, const ModSib& from )    { Group1( G1Type_##cod, to,	from	, true ); } \
	emitterT void cod##8( x86Register8 to, u8 imm )                { Group1_Imm( G1Type_##cod, to,	imm	); } \
	emitterT void cod##8( const ModSib& to, x86Register8 from )    { Group1( G1Type_##cod, to,	from	, true ); } \
	emitterT void cod##8( void* to, x86Register8 from )            { Group1( G1Type_##cod, ptr[to],	from	, true ); } \
	emitterT void cod##8( void* to, u8 imm )                       { Group1_Imm( G1Type_##cod, ptr[to],	imm	); } \
	emitterT void cod##8( const ModSib& to, u8 imm )               { Group1_Imm( G1Type_##cod, to,	imm	); }

DEFINE_GROUP1_OPCODE( ADD )
DEFINE_GROUP1_OPCODE( CMP )
DEFINE_GROUP1_OPCODE( OR )
DEFINE_GROUP1_OPCODE( ADC )
DEFINE_GROUP1_OPCODE( SBB )
DEFINE_GROUP1_OPCODE( AND )
DEFINE_GROUP1_OPCODE( SUB )
DEFINE_GROUP1_OPCODE( XOR )

}		// end namespace x86Emitter


static __forceinline x86Emitter::x86Register32 _reghlp32( x86IntRegType src )
{
	return x86Emitter::x86Register32( src );
}

static __forceinline x86Emitter::x86Register16 _reghlp16( x86IntRegType src )
{
	return x86Emitter::x86Register16( src );
}

static __forceinline x86Emitter::x86Register8 _reghlp8( x86IntRegType src )
{
	return x86Emitter::x86Register8( src );
}

static __forceinline x86Emitter::ModSib _mrmhlp( x86IntRegType src )
{
	return x86Emitter::ModSib( x86Emitter::x86ModRm( _reghlp32(src) ) );
}

//////////////////////////////////////////////////////////////////////////////////////////
//
#define DEFINE_LEGACY_HELPER( cod, bits ) \
	emitterT void cod##bits##RtoR( x86IntRegType to, x86IntRegType from )	{ x86Emitter::cod##bits( _reghlp##bits(to), _reghlp##bits(from) ); } \
	emitterT void cod##bits##ItoR( x86IntRegType to, u##bits imm )			{ x86Emitter::cod##bits( _reghlp##bits(to), imm ); } \
	emitterT void cod##bits##MtoR( x86IntRegType to, uptr from )			{ x86Emitter::cod##bits( _reghlp##bits(to), (void*)from ); } \
	emitterT void cod##bits##RtoM( uptr to, x86IntRegType from )			{ x86Emitter::cod##bits( (void*)to, _reghlp##bits(from) ); } \
	emitterT void cod##bits##ItoM( uptr to, u##bits imm )					{ x86Emitter::cod##bits( (void*)to, imm ); } \
	emitterT void cod##bits##ItoRm( x86IntRegType to, u##bits imm, int offset )	{ x86Emitter::cod##bits( _mrmhlp(to) + offset, imm ); } \
	emitterT void cod##bits##RmtoR( x86IntRegType to, x86IntRegType from, int offset ) { x86Emitter::cod##bits( _reghlp##bits(to), _mrmhlp(from) + offset ); } \
	emitterT void cod##bits##RtoRm( x86IntRegType to, x86IntRegType from, int offset ) { x86Emitter::cod##bits( _mrmhlp(to) + offset, _reghlp##bits(from) ); }

#define DEFINE_GROUP1_OPCODE_LEGACY( cod ) \
	DEFINE_LEGACY_HELPER( cod, 32 ) \
	DEFINE_LEGACY_HELPER( cod, 16 ) \
	DEFINE_LEGACY_HELPER( cod, 8 )

DEFINE_GROUP1_OPCODE_LEGACY( ADD )
DEFINE_GROUP1_OPCODE_LEGACY( CMP )
DEFINE_GROUP1_OPCODE_LEGACY( OR )
DEFINE_GROUP1_OPCODE_LEGACY( ADC )
DEFINE_GROUP1_OPCODE_LEGACY( SBB )
DEFINE_GROUP1_OPCODE_LEGACY( AND )
DEFINE_GROUP1_OPCODE_LEGACY( SUB )
DEFINE_GROUP1_OPCODE_LEGACY( XOR )

// Special forms needed by the legacy emitter syntax:

emitterT void AND32I8toR( x86IntRegType to, s8 from ) 
{
	x86Emitter::AND32( _reghlp32(to), from );
}

emitterT void AND32I8toM( uptr to, s8 from ) 
{
	x86Emitter::AND32( (void*)to, from );
}
