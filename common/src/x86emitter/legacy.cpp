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
 * ix86 core v0.6.2
 * Authors: linuzappz <linuzappz@pcsx.net>
 *			alexey silinov
 *			goldfinger
 *			zerofrog(@gmail.com)
 *			cottonvibes(@gmail.com)
 */

//------------------------------------------------------------------
// ix86 legacy emitter functions
//------------------------------------------------------------------

#include "PrecompiledHeader.h"
#include "legacy_internal.h"

//////////////////////////////////////////////////////////////////////////////////////////
//
emitterT void ModRM( uint mod, uint reg, uint rm )
{
	// Note: Following assertions are for legacy support only.
	// The new emitter performs these sanity checks during operand construction, so these
	// assertions can probably be removed once all legacy emitter code has been removed.
	pxAssert( mod < 4 );
	pxAssert( reg < 8 );
	pxAssert( rm < 8 );
	xWrite8( (mod << 6) | (reg << 3) | rm );
}

emitterT void SibSB( uint ss, uint index, uint base )
{
	// Note: Following asserts are for legacy support only.
	// The new emitter performs these sanity checks during operand construction, so these
	// assertions can probably be removed once all legacy emitter code has been removed.
	pxAssert( ss < 4 );
	pxAssert( index < 8 );
	pxAssert( base < 8 );
	xWrite8( (ss << 6) | (index << 3) | base );
}

using namespace x86Emitter;

static ModSib32 _mhlp32( x86IntRegType to )
{
	return ptr32[xAddressReg( to )];
}

static ModSib32 _mhlp32( x86IntRegType to1, x86IntRegType to2 )
{
	return ptr32[xAddressReg( to1 ) + xAddressReg( to2 )];
}

static ModSib16 _mhlp16( x86IntRegType to )
{
	return ptr16[xAddressReg( to )];
}

static ModSib16 _mhlp16( x86IntRegType to1, x86IntRegType to2 )
{
	return ptr16[xAddressReg( to1 ) + xAddressReg( to2 )];
}

static ModSib8 _mhlp8( x86IntRegType to )
{
	return ptr8[xAddressReg( to )];
}

static ModSib8 _mhlp8( x86IntRegType to1, x86IntRegType to2 )
{
	return ptr8[xAddressReg( to1 ) + xAddressReg( to2 )];
}

//////////////////////////////////////////////////////////////////////////////////////////
//
#define DEFINE_LEGACY_HELPER( cod, bits ) \
	emitterT void cod##bits##RtoR( x86IntRegType to, x86IntRegType from )	{ x##cod( xRegister##bits(to), xRegister##bits(from) ); } \
	emitterT void cod##bits##ItoR( x86IntRegType to, u##bits imm )			{ x##cod( xRegister##bits(to), imm ); } \
	emitterT void cod##bits##MtoR( x86IntRegType to, uptr from )			{ x##cod( xRegister##bits(to), (void*)from ); } \
	emitterT void cod##bits##RtoM( uptr to, x86IntRegType from )			{ x##cod( (void*)to, xRegister##bits(from) ); } \
	emitterT void cod##bits##ItoM( uptr to, u##bits imm )					{ x##cod( ptr##bits[to], imm ); }  \
	emitterT void cod##bits##ItoRm( x86IntRegType to, u##bits imm, int offset )	{ x##cod( _mhlp##bits(to) + offset, imm ); } \
	emitterT void cod##bits##RmtoR( x86IntRegType to, x86IntRegType from, int offset ) { x##cod( xRegister##bits(to), _mhlp##bits(from) + offset ); } \
	emitterT void cod##bits##RtoRm( x86IntRegType to, x86IntRegType from, int offset ) { x##cod( _mhlp##bits(to) + offset, xRegister##bits(from) ); } \
	emitterT void cod##bits##RtoRmS( x86IntRegType to1, x86IntRegType to2, x86IntRegType from, int offset ) \
	{ x##cod( _mhlp##bits(to1,to2) + offset, xRegister##bits(from) ); } \
	emitterT void cod##bits##RmStoR( x86IntRegType to, x86IntRegType from1, x86IntRegType from2, int offset ) \
	{ x##cod( xRegister##bits(to), _mhlp##bits(from1,from2) + offset ); }

#define DEFINE_LEGACY_SHIFT_HELPER( cod, bits ) \
	emitterT void cod##bits##CLtoR( x86IntRegType to )						{ x##cod( xRegister##bits(to), cl ); } \
	emitterT void cod##bits##ItoR( x86IntRegType to, u8 imm )				{ x##cod( xRegister##bits(to), imm ); } \
	emitterT void cod##bits##CLtoM( uptr to )								{ x##cod( ptr##bits[to], cl ); } \
	emitterT void cod##bits##ItoM( uptr to, u8 imm )						{ x##cod( ptr##bits[to], imm ); }  \
	emitterT void cod##bits##ItoRm( x86IntRegType to, u8 imm, int offset )	{ x##cod( _mhlp##bits(to) + offset, imm ); } \
	emitterT void cod##bits##CLtoRm( x86IntRegType to, int offset )			{ x##cod( _mhlp##bits(to) + offset, cl ); }

#define DEFINE_LEGACY_ONEREG_HELPER( cod, bits ) \
	emitterT void cod##bits##R( x86IntRegType to )					{ x##cod( xRegister##bits(to) ); } \
	emitterT void cod##bits##M( uptr to )							{ x##cod( ptr##bits[to] ); } \
	emitterT void cod##bits##Rm( x86IntRegType to, uptr offset )	{ x##cod( _mhlp##bits(to) + offset ); }

#define DEFINE_OPCODE_LEGACY( cod ) \
	DEFINE_LEGACY_HELPER( cod, 32 ) \
	DEFINE_LEGACY_HELPER( cod, 16 ) \
	DEFINE_LEGACY_HELPER( cod, 8 )

#define DEFINE_OPCODE_SHIFT_LEGACY( cod ) \
	DEFINE_LEGACY_SHIFT_HELPER( cod, 32 ) \
	DEFINE_LEGACY_SHIFT_HELPER( cod, 16 ) \
	DEFINE_LEGACY_SHIFT_HELPER( cod, 8 )

#define DEFINE_OPCODE_ONEREG_LEGACY( cod ) \
	DEFINE_LEGACY_ONEREG_HELPER( cod, 32 ) \
	DEFINE_LEGACY_ONEREG_HELPER( cod, 16 ) \
	DEFINE_LEGACY_ONEREG_HELPER( cod, 8 )

//////////////////////////////////////////////////////////////////////////////////////////
//
DEFINE_OPCODE_LEGACY( ADD )
DEFINE_OPCODE_LEGACY( CMP )
DEFINE_OPCODE_LEGACY( OR )
DEFINE_OPCODE_LEGACY( ADC )
DEFINE_OPCODE_LEGACY( SBB )
DEFINE_OPCODE_LEGACY( AND )
DEFINE_OPCODE_LEGACY( SUB )
DEFINE_OPCODE_LEGACY( XOR )

DEFINE_OPCODE_SHIFT_LEGACY( ROL )
DEFINE_OPCODE_SHIFT_LEGACY( ROR )
DEFINE_OPCODE_SHIFT_LEGACY( RCL )
DEFINE_OPCODE_SHIFT_LEGACY( RCR )
DEFINE_OPCODE_SHIFT_LEGACY( SHL )
DEFINE_OPCODE_SHIFT_LEGACY( SHR )
DEFINE_OPCODE_SHIFT_LEGACY( SAR )

DEFINE_OPCODE_LEGACY( MOV )

DEFINE_OPCODE_ONEREG_LEGACY( INC )
DEFINE_OPCODE_ONEREG_LEGACY( DEC )
DEFINE_OPCODE_ONEREG_LEGACY( NOT )
DEFINE_OPCODE_ONEREG_LEGACY( NEG )


// ------------------------------------------------------------------------
#define DEFINE_LEGACY_MOVEXTEND( form, destbits, srcbits ) \
	emitterT void MOV##form##destbits##R##srcbits##toR( x86IntRegType to, x86IntRegType from )				{ xMOV##form( xRegister##destbits( to ), xRegister##srcbits( from ) ); } \
	emitterT void MOV##form##destbits##Rm##srcbits##toR( x86IntRegType to, x86IntRegType from, int offset )	{ xMOV##form( xRegister##destbits( to ), ptr##srcbits[xAddressReg( from ) + offset] ); } \
	emitterT void MOV##form##destbits##M##srcbits##toR( x86IntRegType to, u32 from )						{ xMOV##form( xRegister##destbits( to ), ptr##srcbits[from] ); }

DEFINE_LEGACY_MOVEXTEND( SX, 32, 16 )
DEFINE_LEGACY_MOVEXTEND( ZX, 32, 16 )
DEFINE_LEGACY_MOVEXTEND( SX, 32, 8 )
DEFINE_LEGACY_MOVEXTEND( ZX, 32, 8 )

DEFINE_LEGACY_MOVEXTEND( SX, 16, 8 )
DEFINE_LEGACY_MOVEXTEND( ZX, 16, 8 )

emitterT void TEST32ItoR( x86IntRegType to, u32 from )				{ xTEST( xRegister32(to), from ); }
emitterT void TEST32ItoM( uptr to, u32 from )						{ xTEST( ptr32[to], from ); }
emitterT void TEST32RtoR( x86IntRegType to, x86IntRegType from )	{ xTEST( xRegister32(to), xRegister32(from) ); }
emitterT void TEST32ItoRm( x86IntRegType to, u32 from )				{ xTEST( ptr32[xAddressReg(to)], from ); }

emitterT void TEST16ItoR( x86IntRegType to, u16 from )				{ xTEST( xRegister16(to), from ); }
emitterT void TEST16ItoM( uptr to, u16 from )						{ xTEST( ptr16[to], from ); }
emitterT void TEST16RtoR( x86IntRegType to, x86IntRegType from )	{ xTEST( xRegister16(to), xRegister16(from) ); }
emitterT void TEST16ItoRm( x86IntRegType to, u16 from )				{ xTEST( ptr16[xAddressReg(to)], from ); }

emitterT void TEST8ItoR( x86IntRegType to, u8 from )				{ xTEST( xRegister8(to), from ); }
emitterT void TEST8ItoM( uptr to, u8 from )							{ xTEST( ptr8[to], from ); }
emitterT void TEST8RtoR( x86IntRegType to, x86IntRegType from )		{ xTEST( xRegister8(to), xRegister8(from) ); }
emitterT void TEST8ItoRm( x86IntRegType to, u8 from )				{ xTEST( ptr8[xAddressReg(to)], from ); }

// mov r32 to [r32<<scale+from2]
emitterT void MOV32RmSOffsettoR( x86IntRegType to, x86IntRegType from1, s32 from2, int scale )
{
	xMOV( xRegister32(to), ptr[(xAddressReg(from1)<<scale) + from2] );
}

emitterT void MOV16RmSOffsettoR( x86IntRegType to, x86IntRegType from1, s32 from2, int scale )
{
	xMOV( xRegister16(to), ptr[(xAddressReg(from1)<<scale) + from2] );
}

emitterT void MOV8RmSOffsettoR( x86IntRegType to, x86IntRegType from1, s32 from2, int scale )
{
	xMOV( xRegister8(to), ptr[(xAddressReg(from1)<<scale) + from2] );
}

emitterT void AND32I8toR( x86IntRegType to, s8 from )
{
	xAND( xRegister32(to), from );
}

emitterT void AND32I8toM( uptr to, s8 from )
{
	xAND( ptr8[to], from );
}

/* cmove r32 to r32*/
emitterT void CMOVE32RtoR( x86IntRegType to, x86IntRegType from )
{
	xCMOVE( xRegister32(to), xRegister32(from) );
}

// shld imm8 to r32
emitterT void SHLD32ItoR( x86IntRegType to, x86IntRegType from, u8 shift )
{
	xSHLD( xRegister32(to), xRegister32(from), shift );
}

// shrd imm8 to r32
emitterT void SHRD32ItoR( x86IntRegType to, x86IntRegType from, u8 shift )
{
	xSHRD( xRegister32(to), xRegister32(from), shift );
}

/* mul eax by r32 to edx:eax */
emitterT void MUL32R( x86IntRegType from )		{ xUMUL( xRegister32(from) ); }
/* imul eax by r32 to edx:eax */
emitterT void IMUL32R( x86IntRegType from )		{ xMUL( xRegister32(from) ); }
/* mul eax by m32 to edx:eax */
emitterT void MUL32M( u32 from )				{ xUMUL( ptr32[from] ); }
/* imul eax by m32 to edx:eax */
emitterT void IMUL32M( u32 from )				{ xMUL( ptr32[from] ); }

/* imul r32 by r32 to r32 */
emitterT void IMUL32RtoR( x86IntRegType to, x86IntRegType from )
{
	xMUL( xRegister32(to), xRegister32(from) );
}

/* div eax by r32 to edx:eax */
emitterT void DIV32R( x86IntRegType from )		{ xUDIV( xRegister32(from) ); }
/* idiv eax by r32 to edx:eax */
emitterT void IDIV32R( x86IntRegType from )		{ xDIV( xRegister32(from) ); }
/* div eax by m32 to edx:eax */
emitterT void DIV32M( u32 from )				{ xUDIV( ptr32[from] ); }
/* idiv eax by m32 to edx:eax */
emitterT void IDIV32M( u32 from )				{ xDIV( ptr32[from] ); }


emitterT void LEA32RtoR(x86IntRegType to, x86IntRegType from, s32 offset)
{
	xLEA( xRegister32( to ), ptr[xAddressReg(from)+offset] );
}

emitterT void LEA32RRtoR(x86IntRegType to, x86IntRegType from0, x86IntRegType from1)
{
	xLEA( xRegister32( to ), ptr[xAddressReg(from0)+xAddressReg(from1)] );
}

// Don't inline recursive functions
emitterT void LEA32RStoR(x86IntRegType to, x86IntRegType from, u32 scale)
{
	xLEA( xRegister32( to ), ptr[xAddressReg(from)*(1<<scale)] );
}

// to = from + offset
emitterT void LEA16RtoR(x86IntRegType to, x86IntRegType from, s16 offset)
{
	xLEA( xRegister16( to ), ptr[xAddressReg(from)+offset] );
}

// to = from0 + from1
emitterT void LEA16RRtoR(x86IntRegType to, x86IntRegType from0, x86IntRegType from1)
{
	xLEA( xRegister16( to ), ptr[xAddressReg(from0)+xAddressReg(from1)] );
}

// to = from << scale (max is 3)
emitterT void LEA16RStoR(x86IntRegType to, x86IntRegType from, u32 scale)
{
	xLEA( xRegister16( to ), ptr[xAddressReg(from)*(1<<scale)] );
}

emitterT void BT32ItoR( x86IntRegType to, u8 from )		{ xBT( xRegister32(to), from ); }
emitterT void BTR32ItoR( x86IntRegType to, u8 from )	{ xBTR( xRegister32(to), from ); }

emitterT void SETS8R( x86IntRegType to )	{ xSETS( xRegister8(to) ); }
emitterT void SETL8R( x86IntRegType to )	{ xSETL( xRegister8(to) ); }
emitterT void SETGE8R( x86IntRegType to )	{ xSETGE( xRegister8(to) ); }
emitterT void SETG8R( x86IntRegType to )	{ xSETG( xRegister8(to) ); }
emitterT void SETA8R( x86IntRegType to )	{ xSETA( xRegister8(to) ); }
emitterT void SETAE8R( x86IntRegType to )	{ xSETAE( xRegister8(to) ); }
emitterT void SETB8R( x86IntRegType to )	{ xSETB( xRegister8(to) ); }
emitterT void SETNZ8R( x86IntRegType to )	{ xSETNZ( xRegister8(to) ); }
emitterT void SETZ8R( x86IntRegType to )	{ xSETZ( xRegister8(to) ); }
emitterT void SETE8R( x86IntRegType to )	{ xSETE( xRegister8(to) ); }

/* push imm32 */
emitterT void PUSH32I( u32 from ) { xPUSH( from ); }

/* push r32 */
emitterT void PUSH32R( x86IntRegType from )  { xPUSH( xRegister32( from ) ); }

/* push m32 */
emitterT void PUSH32M( u32 from )
{
	xPUSH( ptr[from] );
}

/* pop r32 */
emitterT void POP32R( x86IntRegType from ) { xPOP( xRegister32( from ) ); }
emitterT void PUSHFD( void ) { xPUSHFD(); }
emitterT void POPFD( void ) { xPOPFD(); }

emitterT void RET( void ) { xRET(); }

emitterT void CBW( void ) { xCBW();  }
emitterT void CWD( void )  { xCWD(); }
emitterT void CDQ( void ) { xCDQ(); }
emitterT void CWDE() { xCWDE(); }

emitterT void LAHF() { xLAHF(); }
emitterT void SAHF() { xSAHF(); }

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
// From here on are instructions that have NOT been implemented in the new emitter.
//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

emitterT u8* J8Rel( int cc, int to )
{
	xWrite8( cc );
	xWrite8( to );
	return (u8*)(x86Ptr - 1);
}

emitterT u16* J16Rel( int cc, u32 to )
{
	xWrite16( 0x0F66 );
	xWrite8( cc );
	xWrite16( to );
	return (u16*)( x86Ptr - 2 );
}

emitterT u32* J32Rel( int cc, u32 to )
{
	xWrite8( 0x0F );
	xWrite8( cc );
	xWrite32( to );
	return (u32*)( x86Ptr - 4 );
}

////////////////////////////////////////////////////
emitterT void x86SetPtr( u8* ptr )
{
	x86Ptr = ptr;
}

//////////////////////////////////////////////////////////////////////////////////////////
// Jump Label API (as rough as it might be)
//
// I don't auto-inline these because of the console logging in case of error, which tends
// to cause quite a bit of code bloat.
//
void x86SetJ8( u8* j8 )
{
	u32 jump = ( x86Ptr - j8 ) - 1;

	if ( jump > 0x7f ) {
		Console.Error( "j8 greater than 0x7f!!" );
		assert(0);
	}
	*j8 = (u8)jump;
}

void x86SetJ8A( u8* j8 )
{
	u32 jump = ( x86Ptr - j8 ) - 1;

	if ( jump > 0x7f ) {
		Console.Error( "j8 greater than 0x7f!!" );
		assert(0);
	}

	if( ((uptr)x86Ptr&0xf) > 4 ) {

		uptr newjump = jump + 16-((uptr)x86Ptr&0xf);

		if( newjump <= 0x7f ) {
			jump = newjump;
			while((uptr)x86Ptr&0xf) *x86Ptr++ = 0x90;
		}
	}
	*j8 = (u8)jump;
}

////////////////////////////////////////////////////
emitterT void x86SetJ32( u32* j32 )
{
	*j32 = ( x86Ptr - (u8*)j32 ) - 4;
}

emitterT void x86SetJ32A( u32* j32 )
{
	while((uptr)x86Ptr&0xf) *x86Ptr++ = 0x90;
	x86SetJ32(j32);
}

////////////////////////////////////////////////////
emitterT void x86Align( int bytes )
{
	// forward align
	x86Ptr = (u8*)( ( (uptr)x86Ptr + bytes - 1) & ~( bytes - 1 ) );
}

/********************/
/* IX86 instructions */
/********************/

emitterT void STC( void ) { xSTC(); }
emitterT void CLC( void ) { xCLC(); }
emitterT void NOP( void ) { xNOP(); }

////////////////////////////////////
// jump instructions				/
////////////////////////////////////

/* jmp rel8 */
emitterT u8* JMP8( u8 to )
{
	xWrite8( 0xEB );
	xWrite8( to );
	return x86Ptr - 1;
}

/* jmp rel32 */
emitterT u32* JMP32( uptr to )
{
	assert( (sptr)to <= 0x7fffffff && (sptr)to >= -0x7fffffff );
	xWrite8( 0xE9 );
	xWrite32( to );
	return (u32*)(x86Ptr - 4 );
}

/* jmp r32/r64 */
emitterT void JMPR( x86IntRegType to )
{
	xJMP( xRegister32(to) );
}

// jmp m32
emitterT void JMP32M( uptr to )
{
	xJMP( ptr32[to] );
}

/* jp rel8 */
emitterT u8* JP8( u8 to ) {
	return J8Rel( 0x7A, to );
}

/* jnp rel8 */
emitterT u8* JNP8( u8 to ) {
	return J8Rel( 0x7B, to );
}

/* je rel8 */
emitterT u8* JE8( u8 to ) {
	return J8Rel( 0x74, to );
}

/* jz rel8 */
emitterT u8* JZ8( u8 to )
{
	return J8Rel( 0x74, to );
}

/* js rel8 */
emitterT u8* JS8( u8 to )
{
	return J8Rel( 0x78, to );
}

/* jns rel8 */
emitterT u8* JNS8( u8 to )
{
	return J8Rel( 0x79, to );
}

/* jg rel8 */
emitterT u8* JG8( u8 to )
{
	return J8Rel( 0x7F, to );
}

/* jge rel8 */
emitterT u8* JGE8( u8 to )
{
	return J8Rel( 0x7D, to );
}

/* jl rel8 */
emitterT u8* JL8( u8 to )
{
	return J8Rel( 0x7C, to );
}

/* ja rel8 */
emitterT u8* JA8( u8 to )
{
	return J8Rel( 0x77, to );
}

emitterT u8* JAE8( u8 to )
{
	return J8Rel( 0x73, to );
}

/* jb rel8 */
emitterT u8* JB8( u8 to )
{
	return J8Rel( 0x72, to );
}

/* jbe rel8 */
emitterT u8* JBE8( u8 to )
{
	return J8Rel( 0x76, to );
}

/* jle rel8 */
emitterT u8* JLE8( u8 to )
{
	return J8Rel( 0x7E, to );
}

/* jne rel8 */
emitterT u8* JNE8( u8 to )
{
	return J8Rel( 0x75, to );
}

/* jnz rel8 */
emitterT u8* JNZ8( u8 to )
{
	return J8Rel( 0x75, to );
}

/* jng rel8 */
emitterT u8* JNG8( u8 to )
{
	return J8Rel( 0x7E, to );
}

/* jnge rel8 */
emitterT u8* JNGE8( u8 to )
{
	return J8Rel( 0x7C, to );
}

/* jnl rel8 */
emitterT u8* JNL8( u8 to )
{
	return J8Rel( 0x7D, to );
}

/* jnle rel8 */
emitterT u8* JNLE8( u8 to )
{
	return J8Rel( 0x7F, to );
}

/* jo rel8 */
emitterT u8* JO8( u8 to )
{
	return J8Rel( 0x70, to );
}

/* jno rel8 */
emitterT u8* JNO8( u8 to )
{
	return J8Rel( 0x71, to );
}
// jb rel32
emitterT u32* JB32( u32 to )
{
	return J32Rel( 0x82, to );
}

/* je rel32 */
emitterT u32* JE32( u32 to )
{
	return J32Rel( 0x84, to );
}

/* jz rel32 */
emitterT u32* JZ32( u32 to )
{
	return J32Rel( 0x84, to );
}

/* js rel32 */
emitterT u32* JS32( u32 to )
{
	return J32Rel( 0x88, to );
}

/* jns rel32 */
emitterT u32* JNS32( u32 to )
{
	return J32Rel( 0x89, to );
}

/* jg rel32 */
emitterT u32* JG32( u32 to )
{
	return J32Rel( 0x8F, to );
}

/* jge rel32 */
emitterT u32* JGE32( u32 to )
{
	return J32Rel( 0x8D, to );
}

/* jl rel32 */
emitterT u32* JL32( u32 to )
{
	return J32Rel( 0x8C, to );
}

/* jle rel32 */
emitterT u32* JLE32( u32 to )
{
	return J32Rel( 0x8E, to );
}

/* ja rel32 */
emitterT u32* JA32( u32 to )
{
	return J32Rel( 0x87, to );
}

/* jae rel32 */
emitterT u32* JAE32( u32 to )
{
	return J32Rel( 0x83, to );
}

/* jne rel32 */
emitterT u32* JNE32( u32 to )
{
	return J32Rel( 0x85, to );
}

/* jnz rel32 */
emitterT u32* JNZ32( u32 to )
{
	return J32Rel( 0x85, to );
}

/* jng rel32 */
emitterT u32* JNG32( u32 to )
{
	return J32Rel( 0x8E, to );
}

/* jnge rel32 */
emitterT u32* JNGE32( u32 to )
{
	return J32Rel( 0x8C, to );
}

/* jnl rel32 */
emitterT u32* JNL32( u32 to )
{
	return J32Rel( 0x8D, to );
}

/* jnle rel32 */
emitterT u32* JNLE32( u32 to )
{
	return J32Rel( 0x8F, to );
}

/* jo rel32 */
emitterT u32* JO32( u32 to )
{
	return J32Rel( 0x80, to );
}

/* jno rel32 */
emitterT u32* JNO32( u32 to )
{
	return J32Rel( 0x81, to );
}



/* call func */
emitterT void CALLFunc( uptr func )
{
	xCALL( (void*)func );
}

/* call r32 */
emitterT void CALL32R( x86IntRegType to )
{
	xCALL( xRegister32( to ) );
}

/* call m32 */
emitterT void CALL32M( u32 to )
{
	xCALL( ptr32[to] );
}

emitterT void BSRRtoR(x86IntRegType to, x86IntRegType from)
{
	xBSR( xRegister32(to), xRegister32(from) );
}

emitterT void BSWAP32R( x86IntRegType to )
{
	xBSWAP( xRegister32(to) );
}
