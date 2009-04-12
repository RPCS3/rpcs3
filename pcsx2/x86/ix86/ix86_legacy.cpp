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

//------------------------------------------------------------------
// ix86 legacy emitter functions
//------------------------------------------------------------------

#include "PrecompiledHeader.h"
#include "System.h"
#include "ix86_internal.h"

using namespace x86Emitter;

// Note: the 'to' field can either be a register or a special opcode extension specifier
// depending on the opcode's encoding.

emitterT void WriteRmOffsetFrom(x86IntRegType to, x86IntRegType from, int offset)
{
	if ((from&7) == ESP) {
		if( offset == 0 ) {
			ModRM( 0, to, 0x4 );
			SibSB( 0, 0x4, 0x4 );
		}
		else if( is_s8( offset ) ) {
			ModRM( 1, to, 0x4 );
			SibSB( 0, 0x4, 0x4 );
			write8(offset);
		}
		else {
			ModRM( 2, to, 0x4 );
			SibSB( 0, 0x4, 0x4 );
			write32(offset);
		}
	}
	else {
		if( offset == 0 ) {
			ModRM( 0, to, from );
		}
		else if( is_s8( offset ) ) {
			ModRM( 1, to, from );
			write8(offset);
		}
		else {
			ModRM( 2, to, from );
			write32(offset);
		}
	}
}

emitterT void ModRM( s32 mod, s32 reg, s32 rm )
{	
	write8( ( mod << 6 ) | ( (reg & 7) << 3 ) | ( rm & 7 ) );
}

emitterT void SibSB( s32 ss, s32 index, s32 base )
{
	write8( ( ss << 6 ) | ( (index & 7) << 3 ) | ( base & 7 ) );
}

emitterT void SET8R( int cc, int to )
{
	RexB(0, to);
	write8( 0x0F );
	write8( cc );
	write8( 0xC0 | ( to ) );
}

emitterT u8* J8Rel( int cc, int to )
{
	write8( cc );
	write8( to );
	return (u8*)(x86Ptr - 1);
}

emitterT u16* J16Rel( int cc, u32 to )
{
	write16( 0x0F66 );
	write8( cc );
	write16( to );
	return (u16*)( x86Ptr - 2 );
}

emitterT u32* J32Rel( int cc, u32 to )
{
	write8( 0x0F );
	write8( cc );
	write32( to );
	return (u32*)( x86Ptr - 4 );
}

emitterT void CMOV32RtoR( int cc, int to, int from )
{
	RexRB(0, to, from);
	write8( 0x0F );
	write8( cc );
	ModRM( 3, to, from );
}

emitterT void CMOV32MtoR( int cc, int to, uptr from )
{
	RexR(0, to);
	write8( 0x0F );
	write8( cc );
	ModRM( 0, to, DISP32 );
	write32( MEMADDR(from, 4) );
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
		Console::Error( "j8 greater than 0x7f!!" );
		assert(0);
	}
	*j8 = (u8)jump;
}

void x86SetJ8A( u8* j8 )
{
	u32 jump = ( x86Ptr - j8 ) - 1;

	if ( jump > 0x7f ) {
		Console::Error( "j8 greater than 0x7f!!" );
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

////////////////////////////////////////////////////
// Generates executable code to align to the given alignment (could be useful for the second leg
// of if/else conditionals, which usually fall through a jump target label).
//
// Note: Left in for now just in case, but usefulness is moot.  Only K8's and older (non-Prescott)
// P4s benefit from this, and we don't optimize for those platforms anyway.
//
void x86AlignExecutable( int align )
{
	uptr newx86 = ( (uptr)x86Ptr + align - 1) & ~( align - 1 );
	uptr bytes = ( newx86 - (uptr)x86Ptr );

	switch( bytes )
	{
		case 0: break;

		case 1: NOP(); break;
		case 2: MOV32RtoR( ESI, ESI ); break;
		case 3: write8(0x08D); write8(0x024); write8(0x024); break;
		case 5: NOP();	// falls through to 4...
		case 4: write8(0x08D); write8(0x064); write8(0x024); write8(0); break;
		case 6: write8(0x08D); write8(0x0B6); write32(0); break;
		case 8: NOP();	// falls through to 7...
		case 7: write8(0x08D); write8(0x034); write8(0x035); write32(0); break;

		default:
		{
			// for larger alignments, just use a JMP...
			u8* aligned_target = JMP8(0);
			x86Ptr = (u8*)newx86;
			x86SetJ8( aligned_target );
		}
	}

	jASSUME( x86Ptr == (u8*)newx86 );
}

/********************/
/* IX86 instructions */
/********************/

emitterT void STC( void )
{
	write8( 0xF9 );
}

emitterT void CLC( void )
{
	write8( 0xF8 );
}

// NOP 1-byte
emitterT void NOP( void )
{
	write8(0x90);
}


////////////////////////////////////
// mov instructions				/
////////////////////////////////////

/* mov r32 to r32 */
emitterT void MOV32RtoR( x86IntRegType to, x86IntRegType from ) 
{
	if( to == from ) return;

	RexRB(0, from, to);
	write8( 0x89 );
	ModRM( 3, from, to );
}

/* mov r32 to m32 */
emitterT void MOV32RtoM( uptr to, x86IntRegType from ) 
{
	RexR(0, from);
	if (from == EAX) {
		write8(0xA3);
	} else {
		write8( 0x89 );
		ModRM( 0, from, DISP32 );
	}
	write32( MEMADDR(to, 4) );
}

/* mov m32 to r32 */
emitterT void MOV32MtoR( x86IntRegType to, uptr from ) 
{
	RexR(0, to);
	if (to == EAX) {
		write8(0xA1);
	} else {
		write8( 0x8B );
		ModRM( 0, to, DISP32 );
	}
	write32( MEMADDR(from, 4) ); 
}

emitterT void MOV32RmtoR( x86IntRegType to, x86IntRegType from, int offset )
{
	RexRB(0, to, from);
	write8( 0x8B );
	WriteRmOffsetFrom(to, from, offset);
}

/* mov [r32+r32*scale] to r32 */
emitterT void MOV32RmStoR( x86IntRegType to, x86IntRegType from, x86IntRegType from2, int scale )
{
	RexRXB(0,to,from2,from);
	write8( 0x8B );
	ModRM( 0, to, 0x4 );
	SibSB(scale, from2, from );
}

// mov r32 to [r32<<scale+from2]
emitterT void MOV32RmSOffsettoR( x86IntRegType to, x86IntRegType from1, int from2, int scale )
{
	RexRXB(0,to,from1,0);
	write8( 0x8B );
	ModRM( 0, to, 0x4 );
	ModRM( scale, from1, 5);
	write32(from2);
}

/* mov r32 to [r32][r32*scale] */
emitterT void MOV32RtoRmS( x86IntRegType to, x86IntRegType from, x86IntRegType from2, int scale )
{
	RexRXB(0, to, from2, from);
	write8( 0x89 );
	ModRM( 0, to, 0x4 );
	SibSB(scale, from2, from );
}

/* mov imm32 to r32 */
emitterT void MOV32ItoR( x86IntRegType to, u32 from ) 
{
	RexB(0, to);
	write8( 0xB8 | (to & 0x7) ); 
	write32( from );
}

/* mov imm32 to m32 */
emitterT void MOV32ItoM(uptr to, u32 from ) 
{
	write8( 0xC7 );
	ModRM( 0, 0, DISP32 );
	write32( MEMADDR(to, 8) );
	write32( from ); 
}

// mov imm32 to [r32+off]
emitterT void MOV32ItoRm( x86IntRegType to, u32 from, int offset)
{
	RexB(0,to);
	write8( 0xC7 );
	WriteRmOffsetFrom(0, to, offset);
	write32(from);
}

// mov r32 to [r32+off]
emitterT void MOV32RtoRm( x86IntRegType to, x86IntRegType from, int offset)
{
	RexRB(0,from,to);
	write8( 0x89 );
	WriteRmOffsetFrom(from, to, offset);
}


/* mov r16 to r16 */
emitterT void MOV16RtoR( x86IntRegType to, x86IntRegType from ) 
{
	if( to == from ) return;

	write8( 0x66 );
	RexRB(0, from, to);
	write8( 0x89 );
	ModRM( 3, from, to );
}

/* mov r16 to m16 */
emitterT void MOV16RtoM(uptr to, x86IntRegType from ) 
{
	write8( 0x66 );
	RexR(0,from);
	write8( 0x89 );
	ModRM( 0, from, DISP32 );
	write32( MEMADDR(to, 4) );
}

/* mov m16 to r16 */
emitterT void MOV16MtoR( x86IntRegType to, uptr from ) 
{
	write8( 0x66 );
	RexR(0,to);
	write8( 0x8B );
	ModRM( 0, to, DISP32 );
	write32( MEMADDR(from, 4) ); 
}

emitterT void MOV16RmtoR( x86IntRegType to, x86IntRegType from, int offset )
{
	write8( 0x66 );
	RexRB(0,to,from);
	write8( 0x8B );
	WriteRmOffsetFrom(to, from, offset);
}

emitterT void MOV16RmSOffsettoR( x86IntRegType to, x86IntRegType from1, u32 from2, int scale )
{
	write8(0x66);
	RexRXB(0,to,from1,0);
	write8( 0x8B );
	ModRM( 0, to, SIB );
	SibSB( scale, from1, SIBDISP);
	write32(from2);
}

/* mov imm16 to m16 */
emitterT void MOV16ItoM( uptr to, u16 from ) 
{
	write8( 0x66 );
	write8( 0xC7 );
	ModRM( 0, 0, DISP32 );
	write32( MEMADDR(to, 6) );
	write16( from ); 
}

/* mov r16 to [r32][r32*scale] */
emitterT void MOV16RtoRmS( x86IntRegType to, x86IntRegType from, x86IntRegType from2, int scale )
{
	write8( 0x66 );
	RexRXB(0,to,from2,from);
	write8( 0x89 );
	ModRM( 0, to, 0x4 );
	SibSB(scale, from2, from );
}

emitterT void MOV16ItoR( x86IntRegType to, u16 from )
{
	RexB(0, to);
	write16( 0xB866 | ((to & 0x7)<<8) ); 
	write16( from );
}

// mov imm16 to [r16+off]
emitterT void MOV16ItoRm( x86IntRegType to, u16 from, u32 offset=0 )
{
	write8(0x66);
	RexB(0,to);
	write8( 0xC7 );
	WriteRmOffsetFrom(0, to, offset);
	write16(from);
}

// mov r16 to [r16+off]
emitterT void MOV16RtoRm( x86IntRegType to, x86IntRegType from, int offset )
{
	write8(0x66);
	RexRB(0,from,to);
	write8( 0x89 );
	WriteRmOffsetFrom(from, to, offset);
}

/* mov r8 to m8 */
emitterT void MOV8RtoM( uptr to, x86IntRegType from ) 
{
	RexR(0,from);
	write8( 0x88 );
	ModRM( 0, from, DISP32 );
	write32( MEMADDR(to, 4) );
}

/* mov m8 to r8 */
emitterT void MOV8MtoR( x86IntRegType to, uptr from ) 
{
	RexR(0,to);
	write8( 0x8A );
	ModRM( 0, to, DISP32 );
	write32( MEMADDR(from, 4) ); 
}

emitterT void MOV8RmtoR(x86IntRegType to, x86IntRegType from, int offset)
{
	RexRB(0,to,from);
	write8( 0x8A );
	WriteRmOffsetFrom(to, from, offset);
}

emitterT void MOV8RmSOffsettoR( x86IntRegType to, x86IntRegType from1, u32 from2, int scale )
{
	RexRXB(0,to,from1,0);
	write8( 0x8A );
	ModRM( 0, to, SIB );
	SibSB( scale, from1, SIBDISP);
	write32(from2);
}

/* mov imm8 to m8 */
emitterT void MOV8ItoM( uptr to, u8 from ) 
{
	write8( 0xC6 );
	ModRM( 0, 0, DISP32 );
	write32( MEMADDR(to, 5) );
	write8( from ); 
}

// mov imm8 to r8
emitterT void MOV8ItoR( x86IntRegType to, u8 from )
{
	RexB(0, to);
	write8( 0xB0 | (to & 0x7) ); 
	write8( from );
}

// mov imm8 to [r8+off]
emitterT void MOV8ItoRm( x86IntRegType to, u8 from, int offset)
{
	assert( to != ESP );
	RexB(0,to);
	write8( 0xC6 );
	WriteRmOffsetFrom(0, to,offset);
	write8(from);
}

// mov r8 to [r8+off]
emitterT void MOV8RtoRm( x86IntRegType to, x86IntRegType from, int offset)
{
	assert( to != ESP );
	RexRB(0,from,to);
	write8( 0x88 );
	WriteRmOffsetFrom(from,to,offset);
}

/* movsx r8 to r32 */
emitterT void MOVSX32R8toR( x86IntRegType to, x86IntRegType from ) 
{
	RexRB(0,to,from);
	write16( 0xBE0F ); 
	ModRM( 3, to, from ); 
}

emitterT void MOVSX32Rm8toR( x86IntRegType to, x86IntRegType from, int offset )
{
	RexRB(0,to,from);
	write16( 0xBE0F ); 
	WriteRmOffsetFrom(to,from,offset);
}

/* movsx m8 to r32 */
emitterT void MOVSX32M8toR( x86IntRegType to, u32 from ) 
{
	RexR(0,to);
	write16( 0xBE0F ); 
	ModRM( 0, to, DISP32 );
	write32( MEMADDR(from, 4) );
}

/* movsx r16 to r32 */
emitterT void MOVSX32R16toR( x86IntRegType to, x86IntRegType from ) 
{
	RexRB(0,to,from);
	write16( 0xBF0F ); 
	ModRM( 3, to, from ); 
}

emitterT void MOVSX32Rm16toR( x86IntRegType to, x86IntRegType from, int offset )
{
	RexRB(0,to,from);
	write16( 0xBF0F );
	WriteRmOffsetFrom(to,from,offset);
}

/* movsx m16 to r32 */
emitterT void MOVSX32M16toR( x86IntRegType to, u32 from ) 
{
	RexR(0,to);
	write16( 0xBF0F ); 
	ModRM( 0, to, DISP32 );
	write32( MEMADDR(from, 4) );
}

/* movzx r8 to r32 */
emitterT void MOVZX32R8toR( x86IntRegType to, x86IntRegType from ) 
{
	RexRB(0,to,from);
	write16( 0xB60F ); 
	ModRM( 3, to, from ); 
}

emitterT void MOVZX32Rm8toR( x86IntRegType to, x86IntRegType from, int offset )
{
	RexRB(0,to,from);
	write16( 0xB60F );
	WriteRmOffsetFrom(to,from,offset);
}

/* movzx m8 to r32 */
emitterT void MOVZX32M8toR( x86IntRegType to, u32 from ) 
{
	RexR(0,to);
	write16( 0xB60F ); 
	ModRM( 0, to, DISP32 );
	write32( MEMADDR(from, 4) );
}

/* movzx r16 to r32 */
emitterT void MOVZX32R16toR( x86IntRegType to, x86IntRegType from ) 
{
	RexRB(0,to,from);
	write16( 0xB70F ); 
	ModRM( 3, to, from ); 
}

emitterT void MOVZX32Rm16toR( x86IntRegType to, x86IntRegType from, int offset )
{
	RexRB(0,to,from);
	write16( 0xB70F );
	WriteRmOffsetFrom(to,from,offset);
}

/* movzx m16 to r32 */
emitterT void MOVZX32M16toR( x86IntRegType to, u32 from ) 
{
	RexR(0,to);
	write16( 0xB70F ); 
	ModRM( 0, to, DISP32 );
	write32( MEMADDR(from, 4) );
}

/* cmovbe r32 to r32 */
emitterT void CMOVBE32RtoR( x86IntRegType to, x86IntRegType from )
{
	CMOV32RtoR( 0x46, to, from );
}

/* cmovbe m32 to r32*/
emitterT void CMOVBE32MtoR( x86IntRegType to, uptr from )
{
	CMOV32MtoR( 0x46, to, from );
}

/* cmovb r32 to r32 */
emitterT void CMOVB32RtoR( x86IntRegType to, x86IntRegType from )
{
	CMOV32RtoR( 0x42, to, from );
}

/* cmovb m32 to r32*/
emitterT void CMOVB32MtoR( x86IntRegType to, uptr from )
{
	CMOV32MtoR( 0x42, to, from );
}

/* cmovae r32 to r32 */
emitterT void CMOVAE32RtoR( x86IntRegType to, x86IntRegType from )
{
	CMOV32RtoR( 0x43, to, from );
}

/* cmovae m32 to r32*/
emitterT void CMOVAE32MtoR( x86IntRegType to, uptr from )
{
	CMOV32MtoR( 0x43, to, from );
}

/* cmova r32 to r32 */
emitterT void CMOVA32RtoR( x86IntRegType to, x86IntRegType from )
{
	CMOV32RtoR( 0x47, to, from );
}

/* cmova m32 to r32*/
emitterT void CMOVA32MtoR( x86IntRegType to, uptr from )
{
	CMOV32MtoR( 0x47, to, from );
}

/* cmovo r32 to r32 */
emitterT void CMOVO32RtoR( x86IntRegType to, x86IntRegType from )
{
	CMOV32RtoR( 0x40, to, from );
}

/* cmovo m32 to r32 */
emitterT void CMOVO32MtoR( x86IntRegType to, uptr from )
{
	CMOV32MtoR( 0x40, to, from );
}

/* cmovp r32 to r32 */
emitterT void CMOVP32RtoR( x86IntRegType to, x86IntRegType from )
{
	CMOV32RtoR( 0x4A, to, from );
}

/* cmovp m32 to r32 */
emitterT void CMOVP32MtoR( x86IntRegType to, uptr from )
{
	CMOV32MtoR( 0x4A, to, from );
}

/* cmovs r32 to r32 */
emitterT void CMOVS32RtoR( x86IntRegType to, x86IntRegType from )
{
	CMOV32RtoR( 0x48, to, from );
}

/* cmovs m32 to r32 */
emitterT void CMOVS32MtoR( x86IntRegType to, uptr from )
{
	CMOV32MtoR( 0x48, to, from );
}

/* cmovno r32 to r32 */
emitterT void CMOVNO32RtoR( x86IntRegType to, x86IntRegType from )
{
	CMOV32RtoR( 0x41, to, from );
}

/* cmovno m32 to r32 */
emitterT void CMOVNO32MtoR( x86IntRegType to, uptr from )
{
	CMOV32MtoR( 0x41, to, from );
}

/* cmovnp r32 to r32 */
emitterT void CMOVNP32RtoR( x86IntRegType to, x86IntRegType from )
{
	CMOV32RtoR( 0x4B, to, from );
}

/* cmovnp m32 to r32 */
emitterT void CMOVNP32MtoR( x86IntRegType to, uptr from )
{
	CMOV32MtoR( 0x4B, to, from );
}

/* cmovns r32 to r32 */
emitterT void CMOVNS32RtoR( x86IntRegType to, x86IntRegType from )
{
	CMOV32RtoR( 0x49, to, from );
}

/* cmovns m32 to r32 */
emitterT void CMOVNS32MtoR( x86IntRegType to, uptr from )
{
	CMOV32MtoR( 0x49, to, from );
}

/* cmovne r32 to r32 */
emitterT void CMOVNE32RtoR( x86IntRegType to, x86IntRegType from )
{
	CMOV32RtoR( 0x45, to, from );
}

/* cmovne m32 to r32*/
emitterT void CMOVNE32MtoR( x86IntRegType to, uptr from ) 
{
	CMOV32MtoR( 0x45, to, from );
}

/* cmove r32 to r32*/
emitterT void CMOVE32RtoR( x86IntRegType to, x86IntRegType from ) 
{
	CMOV32RtoR( 0x44, to, from );
}

/* cmove m32 to r32*/
emitterT void CMOVE32MtoR( x86IntRegType to, uptr from ) 
{
	CMOV32MtoR( 0x44, to, from );
}

/* cmovg r32 to r32*/
emitterT void CMOVG32RtoR( x86IntRegType to, x86IntRegType from ) 
{
	CMOV32RtoR( 0x4F, to, from );
}

/* cmovg m32 to r32*/
emitterT void CMOVG32MtoR( x86IntRegType to, uptr from ) 
{
	CMOV32MtoR( 0x4F, to, from );
}

/* cmovge r32 to r32*/
emitterT void CMOVGE32RtoR( x86IntRegType to, x86IntRegType from ) 
{
	CMOV32RtoR( 0x4D, to, from );
}

/* cmovge m32 to r32*/
emitterT void CMOVGE32MtoR( x86IntRegType to, uptr from ) 
{
	CMOV32MtoR( 0x4D, to, from );
}

/* cmovl r32 to r32*/
emitterT void CMOVL32RtoR( x86IntRegType to, x86IntRegType from ) 
{
	CMOV32RtoR( 0x4C, to, from );
}

/* cmovl m32 to r32*/
emitterT void CMOVL32MtoR( x86IntRegType to, uptr from ) 
{
	CMOV32MtoR( 0x4C, to, from );
}

/* cmovle r32 to r32*/
emitterT void CMOVLE32RtoR( x86IntRegType to, x86IntRegType from ) 
{
	CMOV32RtoR( 0x4E, to, from );
}

/* cmovle m32 to r32*/
emitterT void CMOVLE32MtoR( x86IntRegType to, uptr from ) 
{
	CMOV32MtoR( 0x4E, to, from );
}

////////////////////////////////////
// arithmetic instructions		 /
////////////////////////////////////

/* inc r32 */
emitterT void INC32R( x86IntRegType to ) 
{
	write8( 0x40 + to );
}

/* inc m32 */
emitterT void INC32M( u32 to ) 
{
	write8( 0xFF );
	ModRM( 0, 0, DISP32 );
	write32( MEMADDR(to, 4) );
}

/* inc r16 */
emitterT void INC16R( x86IntRegType to ) 
{
	write8( 0x66 );
	write8( 0x40 + to );
}

/* inc m16 */
emitterT void INC16M( u32 to ) 
{
	write8( 0x66 );
	write8( 0xFF );
	ModRM( 0, 0, DISP32 );
	write32( MEMADDR(to, 4) );
}

/* dec r32 */
emitterT void DEC32R( x86IntRegType to ) 
{
	write8( 0x48 + to );
}

/* dec m32 */
emitterT void DEC32M( u32 to ) 
{
	write8( 0xFF );
	ModRM( 0, 1, DISP32 );
	write32( MEMADDR(to, 4) );
}

/* dec r16 */
emitterT void DEC16R( x86IntRegType to ) 
{
	write8( 0x66 );
	write8( 0x48 + to );
}

/* dec m16 */
emitterT void DEC16M( u32 to ) 
{
	write8( 0x66 );
	write8( 0xFF );
	ModRM( 0, 1, DISP32 );
	write32( MEMADDR(to, 4) );
}

/* mul eax by r32 to edx:eax */
emitterT void MUL32R( x86IntRegType from ) 
{
	RexB(0,from);
	write8( 0xF7 ); 
	ModRM( 3, 4, from );
}

/* imul eax by r32 to edx:eax */
emitterT void IMUL32R( x86IntRegType from ) 
{
	RexB(0,from);
	write8( 0xF7 ); 
	ModRM( 3, 5, from );
}

/* mul eax by m32 to edx:eax */
emitterT void MUL32M( u32 from ) 
{
	write8( 0xF7 ); 
	ModRM( 0, 4, DISP32 );
	write32( MEMADDR(from, 4) ); 
}

/* imul eax by m32 to edx:eax */
emitterT void IMUL32M( u32 from ) 
{
	write8( 0xF7 ); 
	ModRM( 0, 5, DISP32 );
	write32( MEMADDR(from, 4) ); 
}

/* imul r32 by r32 to r32 */
emitterT void IMUL32RtoR( x86IntRegType to, x86IntRegType from ) 
{
	RexRB(0,to,from);
	write16( 0xAF0F ); 
	ModRM( 3, to, from );
}

/* div eax by r32 to edx:eax */
emitterT void DIV32R( x86IntRegType from ) 
{
	RexB(0,from);
	write8( 0xF7 ); 
	ModRM( 3, 6, from );
}

/* idiv eax by r32 to edx:eax */
emitterT void IDIV32R( x86IntRegType from ) 
{
	RexB(0,from);
	write8( 0xF7 ); 
	ModRM( 3, 7, from );
}

/* div eax by m32 to edx:eax */
emitterT void DIV32M( u32 from ) 
{
	write8( 0xF7 ); 
	ModRM( 0, 6, DISP32 );
	write32( MEMADDR(from, 4) ); 
}

/* idiv eax by m32 to edx:eax */
emitterT void IDIV32M( u32 from ) 
{
	write8( 0xF7 ); 
	ModRM( 0, 7, DISP32 );
	write32( MEMADDR(from, 4) ); 
}

////////////////////////////////////
// shifting instructions			/
////////////////////////////////////

/* shl imm8 to r32 */
emitterT void SHL32ItoR( x86IntRegType to, u8 from ) 
{
	RexB(0, to);
	if ( from == 1 )
	{
		write8( 0xD1 );
		write8( 0xE0 | (to & 0x7) );
		return;
	}
	write8( 0xC1 ); 
	ModRM( 3, 4, to );
	write8( from ); 
}

/* shl imm8 to m32 */
emitterT void SHL32ItoM( uptr to, u8 from ) 
{
	if ( from == 1 )
	{
		write8( 0xD1 );
		ModRM( 0, 4, DISP32 );
		write32( MEMADDR(to, 4) );
	}
	else
	{
		write8( 0xC1 ); 
		ModRM( 0, 4, DISP32 );
		write32( MEMADDR(to, 5) );
		write8( from ); 
	}
}

/* shl cl to r32 */
emitterT void SHL32CLtoR( x86IntRegType to ) 
{
	RexB(0,to);
	write8( 0xD3 ); 
	ModRM( 3, 4, to );
}

// shl imm8 to r16
emitterT void SHL16ItoR( x86IntRegType to, u8 from )
{
	write8(0x66);
	RexB(0,to);
	if ( from == 1 )
	{
		write8( 0xD1 );
		write8( 0xE0 | (to & 0x7) );
		return;
	}
	write8( 0xC1 ); 
	ModRM( 3, 4, to );
	write8( from ); 
}

// shl imm8 to r8
emitterT void SHL8ItoR( x86IntRegType to, u8 from )
{
	RexB(0,to);
	if ( from == 1 )
	{
		write8( 0xD0 );
		write8( 0xE0 | (to & 0x7) );
		return;
	}
	write8( 0xC0 ); 
	ModRM( 3, 4, to );
	write8( from ); 
}

/* shr imm8 to r32 */
emitterT void SHR32ItoR( x86IntRegType to, u8 from ) {
	RexB(0,to);
	if ( from == 1 )
	{
		write8( 0xD1 );
		write8( 0xE8 | (to & 0x7) );
	}
	else
	{
		write8( 0xC1 ); 
		ModRM( 3, 5, to );
		write8( from ); 
	}
}

/* shr imm8 to m32 */
emitterT void SHR32ItoM( uptr to, u8 from ) 
{
	if ( from == 1 )
	{
		write8( 0xD1 );
		ModRM( 0, 5, DISP32 );
		write32( MEMADDR(to, 4) );
	}
	else
	{
		write8( 0xC1 ); 
		ModRM( 0, 5, DISP32 );
		write32( MEMADDR(to, 5) );
		write8( from ); 
	}
}

/* shr cl to r32 */
emitterT void SHR32CLtoR( x86IntRegType to ) 
{
	RexB(0,to);
	write8( 0xD3 ); 
	ModRM( 3, 5, to );
}

// shr imm8 to r16
emitterT void SHR16ItoR( x86IntRegType to, u8 from )
{
	RexB(0,to);
	if ( from == 1 )
	{
		write8( 0xD1 );
		ModRM( 3, 5, to );
	}
	else
	{
		write8( 0xC1 ); 
		ModRM( 3, 5, to );
		write8( from ); 
	}
}

// shr imm8 to r8
emitterT void SHR8ItoR( x86IntRegType to, u8 from )
{
	RexB(0,to);
	if ( from == 1 )
	{
		write8( 0xD0 );
		write8( 0xE8 | (to & 0x7) );
	}
	else
	{
		write8( 0xC0 ); 
		ModRM( 3, 5, to );
		write8( from ); 
	}
}

/* sar imm8 to r32 */
emitterT void SAR32ItoR( x86IntRegType to, u8 from ) 
{
	RexB(0,to);
	if ( from == 1 )
	{
		write8( 0xD1 );
		ModRM( 3, 7, to );
		return;
	}
	write8( 0xC1 ); 
	ModRM( 3, 7, to );
	write8( from ); 
}

/* sar imm8 to m32 */
emitterT void SAR32ItoM( uptr to, u8 from )
{
	write8( 0xC1 ); 
	ModRM( 0, 7, DISP32 );
	write32( MEMADDR(to, 5) );
	write8( from );
}

/* sar cl to r32 */
emitterT void SAR32CLtoR( x86IntRegType to ) 
{
	RexB(0,to);
	write8( 0xD3 ); 
	ModRM( 3, 7, to );
}

// sar imm8 to r16
emitterT void SAR16ItoR( x86IntRegType to, u8 from )
{
	write8(0x66);
	RexB(0,to);
	if ( from == 1 )
	{
		write8( 0xD1 );
		ModRM( 3, 7, to );
		return;
	}
	write8( 0xC1 ); 
	ModRM( 3, 7, to );
	write8( from ); 
}

/*emitterT void ROR32ItoR( x86IntRegType to,u8 from )
{
	RexB(0,to);
	if ( from == 1 ) {
		write8( 0xd1 );
		write8( 0xc8 | to );
	} 
	else 
	{
		write8( 0xc1 );
		write8( 0xc8 | to );
		write8( from );
	}
}*/

emitterT void RCR32ItoR( x86IntRegType to, u8 from ) 
{
	RexB(0,to);
	if ( from == 1 ) {
		write8( 0xd1 );
		ModRM(3, 3, to);
	} 
	else 
	{
		write8( 0xc1 );
		ModRM(3, 3, to);
		write8( from );
	}
}

emitterT void RCR32ItoM( uptr to, u8 from ) 
{
	RexB(0,to);
	if ( from == 1 ) {
		write8( 0xd1 );
		ModRM( 0, 3, DISP32 );
		write32( MEMADDR(to, 8) );
	} 
	else 
	{
		write8( 0xc1 );
		ModRM( 0, 3, DISP32 );
		write32( MEMADDR(to, 8) );
		write8( from );
	}
}

// shld imm8 to r32
emitterT void SHLD32ItoR( x86IntRegType to, x86IntRegType from, u8 shift )
{
	RexRB(0,from,to);
	write8( 0x0F );
	write8( 0xA4 );
	ModRM( 3, from, to );
	write8( shift );
}

// shrd imm8 to r32
emitterT void SHRD32ItoR( x86IntRegType to, x86IntRegType from, u8 shift )
{
	RexRB(0,from,to);
	write8( 0x0F );
	write8( 0xAC );
	ModRM( 3, from, to );
	write8( shift );
}

////////////////////////////////////
// logical instructions			/
////////////////////////////////////

/* not r32 */
emitterT void NOT32R( x86IntRegType from ) 
{
	RexB(0,from);
	write8( 0xF7 ); 
	ModRM( 3, 2, from );
}

// not m32 
emitterT void NOT32M( u32 from )
{
	write8( 0xF7 ); 
	ModRM( 0, 2, DISP32 );
	write32( MEMADDR(from, 4)); 
}

/* neg r32 */
emitterT void NEG32R( x86IntRegType from ) 
{
	RexB(0,from);
	write8( 0xF7 ); 
	ModRM( 3, 3, from );
}

emitterT void NEG32M( u32 from )
{
	write8( 0xF7 ); 
	ModRM( 0, 3, DISP32 );
	write32( MEMADDR(from, 4)); 
}

/* neg r16 */
emitterT void NEG16R( x86IntRegType from ) 
{
	write8( 0x66 ); 
	RexB(0,from);
	write8( 0xF7 ); 
	ModRM( 3, 3, from );
}

////////////////////////////////////
// jump instructions				/
////////////////////////////////////

emitterT u8* JMP( uptr to ) {
	uptr jump = ( x86Ptr - (u8*)to ) - 1;

	if ( jump > 0x7f ) {
		assert( to <= 0xffffffff );
		return (u8*)JMP32( to );
	} 
	else {
		return (u8*)JMP8( to );
	}
}

/* jmp rel8 */
emitterT u8* JMP8( u8 to ) 
{
	write8( 0xEB ); 
	write8( to );
	return x86Ptr - 1;
}

/* jmp rel32 */
emitterT u32* JMP32( uptr to ) 
{
	assert( (sptr)to <= 0x7fffffff && (sptr)to >= -0x7fffffff );
	write8( 0xE9 ); 
	write32( to ); 
	return (u32*)(x86Ptr - 4 );
}

/* jmp r32/r64 */
emitterT void JMPR( x86IntRegType to ) 
{
	RexB(0, to);
	write8( 0xFF ); 
	ModRM( 3, 4, to );
}

// jmp m32 
emitterT void JMP32M( uptr to )
{
	write8( 0xFF ); 
	ModRM( 0, 4, DISP32 );
	write32( MEMADDR(to, 4)); 
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
	func -= ( (uptr)x86Ptr + 5 );
	assert( (sptr)func <= 0x7fffffff && (sptr)func >= -0x7fffffff );
	CALL32(func);
}

/* call rel32 */
emitterT void CALL32( u32 to )
{
	write8( 0xE8 ); 
	write32( to ); 
}

/* call r32 */
emitterT void CALL32R( x86IntRegType to ) 
{
	write8( 0xFF );
	ModRM( 3, 2, to );
}

/* call m32 */
emitterT void CALL32M( u32 to ) 
{
	write8( 0xFF );
	ModRM( 0, 2, DISP32 );
	write32( MEMADDR(to, 4) );
}

////////////////////////////////////
// misc instructions				/
////////////////////////////////////

/* test imm32 to r32 */
emitterT void TEST32ItoR( x86IntRegType to, u32 from ) 
{
	RexB(0,to);
	if ( to == EAX )
	{
		write8( 0xA9 );
	} 
	else 
	{
		write8( 0xF7 );
		ModRM( 3, 0, to );
	}
	write32( from ); 
}

emitterT void TEST32ItoM( uptr to, u32 from )
{
	write8( 0xF7 );
	ModRM( 0, 0, DISP32 );
	write32( MEMADDR(to, 8) );
	write32( from );
}

/* test r32 to r32 */
emitterT void TEST32RtoR( x86IntRegType to, x86IntRegType from ) 
{
	RexRB(0,from,to);
	write8( 0x85 );
	ModRM( 3, from, to );
}

// test imm32 to [r32]
emitterT void TEST32ItoRm( x86IntRegType to, u32 from )
{
	RexB(0,to);
	write8( 0xF7 );
	ModRM( 0, 0, to );
	write32(from);
}

// test imm16 to r16
emitterT void TEST16ItoR( x86IntRegType to, u16 from )
{
	write8(0x66);
	RexB(0,to);
	if ( to == EAX )
	{
		write8( 0xA9 );
	} 
	else 
	{
		write8( 0xF7 );
		ModRM( 3, 0, to );
	}
	write16( from ); 
}

// test r16 to r16
emitterT void TEST16RtoR( x86IntRegType to, x86IntRegType from )
{
	write8(0x66);
	RexRB(0,from,to);
	write8( 0x85 );
	ModRM( 3, from, to );
}

// test r8 to r8
emitterT void TEST8RtoR( x86IntRegType to, x86IntRegType from )
{
	RexRB(0, from, to);
	write8( 0x84 );
	ModRM( 3, from, to );
}


// test imm8 to r8
emitterT void TEST8ItoR( x86IntRegType to, u8 from )
{
	RexB(0,to);
	if ( to == EAX )
	{
		write8( 0xA8 );
	} 
	else 
	{
		write8( 0xF6 );
		ModRM( 3, 0, to );
	}
	write8( from ); 
}

// test imm8 to r8
emitterT void TEST8ItoM( uptr to, u8 from )
{
	write8( 0xF6 );
	ModRM( 0, 0, DISP32 );
	write32( MEMADDR(to, 5) );
	write8( from );
}

/* sets r8 */
emitterT void SETS8R( x86IntRegType to ) 
{ 
	SET8R( 0x98, to ); 
}

/* setl r8 */
emitterT void SETL8R( x86IntRegType to ) 
{ 
	SET8R( 0x9C, to ); 
}

// setge r8 
emitterT void SETGE8R( x86IntRegType to ) { SET8R(0x9d, to); }
// setg r8 
emitterT void SETG8R( x86IntRegType to ) { SET8R(0x9f, to); }
// seta r8 
emitterT void SETA8R( x86IntRegType to ) { SET8R(0x97, to); }
// setae r8 
emitterT void SETAE8R( x86IntRegType to ) { SET8R(0x99, to); }
/* setb r8 */
emitterT void SETB8R( x86IntRegType to ) { SET8R( 0x92, to ); }
/* setb r8 */
emitterT void SETNZ8R( x86IntRegType to )  { SET8R( 0x95, to ); }
// setz r8 
emitterT void SETZ8R( x86IntRegType to ) { SET8R(0x94, to); }
// sete r8 
emitterT void SETE8R( x86IntRegType to ) { SET8R(0x94, to); }

/* push imm32 */
emitterT void PUSH32I( u32 from ) { PUSH( from ); }

/* push r32 */
emitterT void PUSH32R( x86IntRegType from )  { PUSH( x86Register32( from ) ); }

/* push m32 */
emitterT void PUSH32M( u32 from )
{
	PUSH( ptr[from] );
}

/* pop r32 */
emitterT void POP32R( x86IntRegType from ) { POP( x86Register32( from ) ); }

/* pushfd */
emitterT void PUSHFD( void ) { write8( 0x9C ); }
/* popfd */
emitterT void POPFD( void ) { write8( 0x9D ); }

emitterT void RET( void ) { /*write8( 0xf3 );  <-- K8 opt?*/ write8( 0xC3 ); }

emitterT void CBW( void ) { write16( 0x9866 );  }
emitterT void CWD( void )  { write8( 0x98 ); }
emitterT void CDQ( void ) { write8( 0x99 ); }
emitterT void CWDE() { write8(0x98); }

emitterT void LAHF() { write8(0x9f); }
emitterT void SAHF() { write8(0x9e); }

emitterT void BT32ItoR( x86IntRegType to, u8 from ) 
{
	write16( 0xBA0F );
	ModRM(3, 4, to);
	write8( from );
}

emitterT void BTR32ItoR( x86IntRegType to, u8 from ) 
{
	write16( 0xBA0F );
	ModRM(3, 6, to);
	write8( from );
}

emitterT void BSRRtoR(x86IntRegType to, x86IntRegType from)
{
	write16( 0xBD0F );
	ModRM( 3, from, to );
}

emitterT void BSWAP32R( x86IntRegType to ) 
{
	write8( 0x0F );
	write8( 0xC8 + to );
}

emitterT void LEA32RtoR(x86IntRegType to, x86IntRegType from, s32 offset)
{
	LEA32( x86Register32( to ), ptr[x86IndexReg(from)+offset] );
}

emitterT void LEA32RRtoR(x86IntRegType to, x86IntRegType from0, x86IntRegType from1)
{ 
	LEA32( x86Register32( to ), ptr[x86IndexReg(from0)+x86IndexReg(from1)] );
}

// Don't inline recursive functions
emitterT void LEA32RStoR(x86IntRegType to, x86IntRegType from, u32 scale)
{
	LEA32( x86Register32( to ), ptr[x86IndexReg(from)*(1<<scale)] );
}

// to = from + offset
emitterT void LEA16RtoR(x86IntRegType to, x86IntRegType from, s16 offset)
{
	LEA16( x86Register16( to ), ptr[x86IndexReg(from)+offset] );
}

// to = from0 + from1
emitterT void LEA16RRtoR(x86IntRegType to, x86IntRegType from0, x86IntRegType from1)
{
	LEA16( x86Register16( to ), ptr[x86IndexReg(from0)+x86IndexReg(from1)] );
}

// to = from << scale (max is 3)
emitterT void LEA16RStoR(x86IntRegType to, x86IntRegType from, u32 scale)
{
	LEA16( x86Register16( to ), ptr[x86IndexReg(from)*(1<<scale)] );
}
