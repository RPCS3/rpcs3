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

#pragma once

//------------------------------------------------------------------
// ix86 instructions
//------------------------------------------------------------------

#include "PrecompiledHeader.h"
#include "System.h"
#include "ix86.h"

emitterT void WriteRmOffset(x86IntRegType to, s32 offset)
{
	if( (to&7) == ESP ) {
		if( offset == 0 ) {
			ModRM<I>( 0, 0, 4 );
			SibSB<I>( 0, ESP, 4 );
		}
		else if( offset <= 127 && offset >= -128 ) {
			ModRM<I>( 1, 0, 4 );
			SibSB<I>( 0, ESP, 4 );
			write8<I>(offset);
		}
		else {
			ModRM<I>( 2, 0, 4 );
			SibSB<I>( 0, ESP, 4 );
			write32<I>(offset);
		}
	}
	else {
		if( offset == 0 ) {
			ModRM<I>( 0, 0, to );
		}
		else if( offset <= 127 && offset >= -128 ) {
			ModRM<I>( 1, 0, to );
			write8<I>(offset);
		}
		else {
			ModRM<I>( 2, 0, to );
			write32<I>(offset);
		}
	}
}

emitterT void WriteRmOffsetFrom(x86IntRegType to, x86IntRegType from, int offset)
{
	if ((from&7) == ESP) {
		if( offset == 0 ) {
			ModRM<I>( 0, to, 0x4 );
			SibSB<I>( 0, 0x4, 0x4 );
		}
		else if( offset <= 127 && offset >= -128 ) {
			ModRM<I>( 1, to, 0x4 );
			SibSB<I>( 0, 0x4, 0x4 );
			write8<I>(offset);
		}
		else {
			ModRM<I>( 2, to, 0x4 );
			SibSB<I>( 0, 0x4, 0x4 );
			write32<I>(offset);
		}
	}
	else {
		if( offset == 0 ) {
			ModRM<I>( 0, to, from );
		}
		else if( offset <= 127 && offset >= -128 ) {
			ModRM<I>( 1, to, from );
			write8<I>(offset);
		}
		else {
			ModRM<I>( 2, to, from );
			write32<I>(offset);
		}
	}
}

emitterT void ModRM( s32 mod, s32 reg, s32 rm )
{	
	write8<I>( ( mod << 6 ) | ( (reg & 7) << 3 ) | ( rm & 7 ) );
}

emitterT void SibSB( s32 ss, s32 index, s32 base )
{
	write8<I>( ( ss << 6 ) | ( (index & 7) << 3 ) | ( base & 7 ) );
}

emitterT void SET8R( int cc, int to )
{
	RexB(0, to);
	write8<I>( 0x0F );
	write8<I>( cc );
	write8<I>( 0xC0 | ( to ) );
}

emitterT u8* J8Rel( int cc, int to )
{
	write8<I>( cc );
	write8<I>( to );
	return (u8*)(x86Ptr[I] - 1);
}

emitterT u16* J16Rel( int cc, u32 to )
{
	write16<I>( 0x0F66 );
	write8<I>( cc );
	write16<I>( to );
	return (u16*)( x86Ptr[I] - 2 );
}

emitterT u32* J32Rel( int cc, u32 to )
{
	write8<I>( 0x0F );
	write8<I>( cc );
	write32<I>( to );
	return (u32*)( x86Ptr[I] - 4 );
}

emitterT void CMOV32RtoR( int cc, int to, int from )
{
	RexRB(0, to, from);
	write8<I>( 0x0F );
	write8<I>( cc );
	ModRM<I>( 3, to, from );
}

emitterT void CMOV32MtoR( int cc, int to, uptr from )
{
	RexR(0, to);
	write8<I>( 0x0F );
	write8<I>( cc );
	ModRM<I>( 0, to, DISP32 );
	write32<I>( MEMADDR(from, 4) );
}

////////////////////////////////////////////////////
emitterT void ex86SetPtr( u8* ptr ) 
{
	x86Ptr[I] = ptr;
}

////////////////////////////////////////////////////
emitterT void ex86SetJ8( u8* j8 )
{
	u32 jump = ( x86Ptr[I] - j8 ) - 1;

	if ( jump > 0x7f ) {
		Console::Error( "j8 greater than 0x7f!!" );
		assert(0);
	}
	*j8 = (u8)jump;
}

emitterT void ex86SetJ8A( u8* j8 )
{
	u32 jump = ( x86Ptr[I] - j8 ) - 1;

	if ( jump > 0x7f ) {
		Console::Error( "j8 greater than 0x7f!!" );
		assert(0);
	}

	if( ((uptr)x86Ptr[I]&0xf) > 4 ) {

		uptr newjump = jump + 16-((uptr)x86Ptr[I]&0xf);

		if( newjump <= 0x7f ) {
			jump = newjump;
			while((uptr)x86Ptr[I]&0xf) *x86Ptr[I]++ = 0x90;
		}
	}
	*j8 = (u8)jump;
}

emitterT void ex86SetJ16( u16 *j16 )
{
	// doesn't work
	u32 jump = ( x86Ptr[I] - (u8*)j16 ) - 2;

	if ( jump > 0x7fff ) {
		Console::Error( "j16 greater than 0x7fff!!" );
		assert(0);
	}
	*j16 = (u16)jump;
}

emitterT void ex86SetJ16A( u16 *j16 )
{
	if( ((uptr)x86Ptr[I]&0xf) > 4 ) {
		while((uptr)x86Ptr[I]&0xf) *x86Ptr[I]++ = 0x90;
	}
	ex86SetJ16<I>(j16);
}

////////////////////////////////////////////////////
emitterT void ex86SetJ32( u32* j32 ) 
{
	*j32 = ( x86Ptr[I] - (u8*)j32 ) - 4;
}

emitterT void ex86SetJ32A( u32* j32 )
{
	while((uptr)x86Ptr[I]&0xf) *x86Ptr[I]++ = 0x90;
	ex86SetJ32<I>(j32);
}

////////////////////////////////////////////////////
emitterT void ex86Align( int bytes ) 
{
	// forward align
	x86Ptr[I] = (u8*)( ( (uptr)x86Ptr[I] + bytes - 1) & ~( bytes - 1 ) );
}

////////////////////////////////////////////////////
// Generates executable code to align to the given alignment (could be useful for the second leg
// of if/else conditionals, which usually fall through a jump target label).
emitterT void ex86AlignExecutable( int align )
{
	uptr newx86 = ( (uptr)x86Ptr[I] + align - 1) & ~( align - 1 );
	uptr bytes = ( newx86 - (uptr)x86Ptr[I] );

	switch( bytes )
	{
	case 0: break;

	case 1: eNOP<I>(); break;
	case 2: eMOV32RtoR<I>( ESI, ESI ); break;
	case 3: write8<I>(0x08D); write8<I>(0x024); write8<I>(0x024); break;
	case 5: eNOP<I>();	// falls through to 4...
	case 4: write8<I>(0x08D); write8<I>(0x064); write8<I>(0x024); write8<I>(0); break;
	case 6: write8<I>(0x08D); write8<I>(0x0B6); write32<I>(0); break;
	case 8: eNOP<I>();	// falls through to 7...
	case 7: write8<I>(0x08D); write8<I>(0x034); write8<I>(0x035); write32<I>(0); break;

	default:
		{
			// for larger alignments, just use a JMP...
			u8* aligned_target = eJMP8<I>(0);
			x86Ptr[I] = (u8*)newx86;
			ex86SetJ8<I>( aligned_target );
		}
	}

	jASSUME( x86Ptr[0] == (u8*)newx86 );
}

/********************/
/* IX86 intructions */
/********************/

emitterT void eSTC( void )
{
	write8<I>( 0xF9 );
}

emitterT void eCLC( void )
{
	write8<I>( 0xF8 );
}

// NOP 1-byte
emitterT void eNOP( void )
{
	write8<I>(0x90);
}


////////////////////////////////////
// mov instructions				/
////////////////////////////////////

/* mov r64 to r64 */
emitterT void eMOV64RtoR( x86IntRegType to, x86IntRegType from ) 
{
	RexRB(1, from, to);
	write8<I>( 0x89 );
	ModRM<I>( 3, from, to );
}

/* mov r64 to m64 */
emitterT void eMOV64RtoM( uptr to, x86IntRegType from ) 
{
	RexR(1, from);
	write8<I>( 0x89 );
	ModRM<I>( 0, from, DISP32 );
	write32<I>( (u32)MEMADDR(to, 4) );
}

/* mov m64 to r64 */
emitterT void eMOV64MtoR( x86IntRegType to, uptr from ) 
{
	RexR(1, to);
	write8<I>( 0x8B );
	ModRM<I>( 0, to, DISP32 );
	write32<I>( (u32)MEMADDR(from, 4) ); 
}

/* mov imm32 to m64 */
emitterT void eMOV64I32toM(uptr to, u32 from ) 
{
	Rex(1, 0, 0, 0);
	write8<I>( 0xC7 );
	ModRM<I>( 0, 0, DISP32 );
	write32<I>( MEMADDR(to, 8) );
	write32<I>( from ); 
}

// mov imm64 to r64
emitterT void eMOV64ItoR( x86IntRegType to, u64 from)
{
	RexB(1, to);
	write8<I>( 0xB8 | (to & 0x7) ); 
	write64<I>( from );
}

/* mov imm32 to r64 */
emitterT void eMOV64I32toR( x86IntRegType to, s32 from ) 
{
	RexB(1, to);
	write8<I>( 0xC7 ); 
	ModRM<I>( 0, 0, to );
	write32<I>( from );
}

// mov imm64 to [r64+off]
emitterT void eMOV64ItoRmOffset( x86IntRegType to, u32 from, int offset)
{
	RexB(1,to);
	write8<I>( 0xC7 );
	WriteRmOffset<I>(to, offset);
	write32<I>(from);
}

// mov [r64+offset] to r64
emitterT void eMOV64RmOffsettoR( x86IntRegType to, x86IntRegType from, int offset )
{
	RexRB(1, to, from);
	write8<I>( 0x8B );
	WriteRmOffsetFrom<I>(to, from, offset);
}

/* mov [r64][r64*scale] to r64 */
emitterT void eMOV64RmStoR( x86IntRegType to, x86IntRegType from, x86IntRegType from2, int scale) {
	RexRXB(1, to, from2, from);
	write8<I>( 0x8B );
	ModRM<I>( 0, to, 0x4 );
	SibSB<I>(scale, from2, from );
}

/* mov r64 to [r64+offset] */
emitterT void eMOV64RtoRmOffset( x86IntRegType to, x86IntRegType from, int offset )
{
	RexRB(1,from,to);
	write8<I>( 0x89 );
	WriteRmOffsetFrom<I>(from, to, offset);
}

/* mov r64 to [r64][r64*scale] */
emitterT void eMOV64RtoRmS( x86IntRegType to, x86IntRegType from, x86IntRegType from2, int scale) {
	RexRXB(1, to, from2, from);
	write8<I>( 0x89 );
	ModRM<I>( 0, to, 0x4 );
	SibSB<I>(scale, from2, from );
}


/* mov r32 to r32 */
emitterT void eMOV32RtoR( x86IntRegType to, x86IntRegType from ) 
{
	RexRB(0, from, to);
	write8<I>( 0x89 );
	ModRM<I>( 3, from, to );
}

/* mov r32 to m32 */
emitterT void eMOV32RtoM( uptr to, x86IntRegType from ) 
{
	RexR(0, from);
	if (from == EAX) {
		write8<I>(0xA3);
	} else {
		write8<I>( 0x89 );
		ModRM<I>( 0, from, DISP32 );
	}
	write32<I>( MEMADDR(to, 4) );
}

/* mov m32 to r32 */
emitterT void eMOV32MtoR( x86IntRegType to, uptr from ) 
{
	RexR(0, to);
	if (to == EAX) {
		write8<I>(0xA1);
	} else {
		write8<I>( 0x8B );
		ModRM<I>( 0, to, DISP32 );
	}
	write32<I>( MEMADDR(from, 4) ); 
}

/* mov [r32] to r32 */
emitterT void eMOV32RmtoR( x86IntRegType to, x86IntRegType from ) {
	RexRB(0, to, from);
	write8<I>(0x8B);
	WriteRmOffsetFrom<I>(to, from, 0);
}

emitterT void eMOV32RmtoROffset( x86IntRegType to, x86IntRegType from, int offset ) {
	RexRB(0, to, from);
	write8<I>( 0x8B );
	WriteRmOffsetFrom<I>(to, from, offset);
}

/* mov [r32+r32*scale] to r32 */
emitterT void eMOV32RmStoR( x86IntRegType to, x86IntRegType from, x86IntRegType from2, int scale) {
	RexRXB(0,to,from2,from);
	write8<I>( 0x8B );
	ModRM<I>( 0, to, 0x4 );
	SibSB<I>(scale, from2, from );
}

// mov r32 to [r32<<scale+from2]
emitterT void eMOV32RmSOffsettoR( x86IntRegType to, x86IntRegType from1, int from2, int scale )
{
	RexRXB(0,to,from1,0);
	write8<I>( 0x8B );
	ModRM<I>( 0, to, 0x4 );
	ModRM<I>( scale, from1, 5);
	write32<I>(from2);
}

/* mov r32 to [r32] */
emitterT void eMOV32RtoRm( x86IntRegType to, x86IntRegType from ) {
	RexRB(0, from, to);
	if ((to&7) == ESP) {
		write8<I>( 0x89 );
		ModRM<I>( 0, from, 0x4 );
		SibSB<I>( 0, 0x4, 0x4 );
	} 
	else {
		write8<I>( 0x89 );
		ModRM<I>( 0, from, to );
	}
}

/* mov r32 to [r32][r32*scale] */
emitterT void eMOV32RtoRmS( x86IntRegType to, x86IntRegType from, x86IntRegType from2, int scale) {
	RexRXB(0, to, from2, from);
	write8<I>( 0x89 );
	ModRM<I>( 0, to, 0x4 );
	SibSB<I>(scale, from2, from );
}

/* mov imm32 to r32 */
emitterT void eMOV32ItoR( x86IntRegType to, u32 from ) 
{
	RexB(0, to);
	write8<I>( 0xB8 | (to & 0x7) ); 
	write32<I>( from );
}

/* mov imm32 to m32 */
emitterT void eMOV32ItoM(uptr to, u32 from ) 
{
	write8<I>( 0xC7 );
	ModRM<I>( 0, 0, DISP32 );
	write32<I>( MEMADDR(to, 8) );
	write32<I>( from ); 
}

// mov imm32 to [r32+off]
emitterT void eMOV32ItoRmOffset( x86IntRegType to, u32 from, int offset)
{
	RexB(0,to);
	write8<I>( 0xC7 );
	WriteRmOffset<I>(to, offset);
	write32<I>(from);
}

// mov r32 to [r32+off]
emitterT void eMOV32RtoRmOffset( x86IntRegType to, x86IntRegType from, int offset)
{
	RexRB(0,from,to);
	write8<I>( 0x89 );
	WriteRmOffsetFrom<I>(from, to, offset);
}

/* mov r16 to m16 */
emitterT void eMOV16RtoM(uptr to, x86IntRegType from ) 
{
	write8<I>( 0x66 );
	RexR(0,from);
	write8<I>( 0x89 );
	ModRM<I>( 0, from, DISP32 );
	write32<I>( MEMADDR(to, 4) );
}

/* mov m16 to r16 */
emitterT void eMOV16MtoR( x86IntRegType to, uptr from ) 
{
	write8<I>( 0x66 );
	RexR(0,to);
	write8<I>( 0x8B );
	ModRM<I>( 0, to, DISP32 );
	write32<I>( MEMADDR(from, 4) ); 
}

emitterT void eMOV16RmtoR( x86IntRegType to, x86IntRegType from) 
{
	write8<I>( 0x66 );
	RexRB(0,to,from);
	write8<I>( 0x8B );
	WriteRmOffsetFrom<I>(to, from, 0);
}

emitterT void eMOV16RmtoROffset( x86IntRegType to, x86IntRegType from, int offset )
{
	write8<I>( 0x66 );
	RexRB(0,to,from);
	write8<I>( 0x8B );
	WriteRmOffsetFrom<I>(to, from, offset);
}

emitterT void eMOV16RmSOffsettoR( x86IntRegType to, x86IntRegType from1, u32 from2, int scale )
{
	write8<I>(0x66);
	RexRXB(0,to,from1,0);
	write8<I>( 0x8B );
	ModRM<I>( 0, to, SIB );
	SibSB<I>( scale, from1, SIBDISP);
	write32<I>(from2);
}

emitterT void eMOV16RtoRm(x86IntRegType to, x86IntRegType from)
{
	write8<I>( 0x66 );
	RexRB(0,from,to);
	write8<I>( 0x89 );
	ModRM<I>( 0, from, to );
}

/* mov imm16 to m16 */
emitterT void eMOV16ItoM( uptr to, u16 from ) 
{
	write8<I>( 0x66 );
	write8<I>( 0xC7 );
	ModRM<I>( 0, 0, DISP32 );
	write32<I>( MEMADDR(to, 6) );
	write16<I>( from ); 
}

/* mov r16 to [r32][r32*scale] */
emitterT void eMOV16RtoRmS( x86IntRegType to, x86IntRegType from, x86IntRegType from2, int scale) {
	write8<I>( 0x66 );
	RexRXB(0,to,from2,from);
	write8<I>( 0x89 );
	ModRM<I>( 0, to, 0x4 );
	SibSB<I>(scale, from2, from );
}

emitterT void eMOV16ItoR( x86IntRegType to, u16 from )
{
	RexB(0, to);
	write16<I>( 0xB866 | ((to & 0x7)<<8) ); 
	write16<I>( from );
}

// mov imm16 to [r16+off]
emitterT void eMOV16ItoRmOffset( x86IntRegType to, u16 from, u32 offset)
{
	write8<I>(0x66);
	RexB(0,to);
	write8<I>( 0xC7 );
	WriteRmOffset<I>(to, offset);
	write16<I>(from);
}

// mov r16 to [r16+off]
emitterT void eMOV16RtoRmOffset( x86IntRegType to, x86IntRegType from, int offset)
{
	write8<I>(0x66);
	RexRB(0,from,to);
	write8<I>( 0x89 );
	WriteRmOffsetFrom<I>(from, to, offset);
}

/* mov r8 to m8 */
emitterT void eMOV8RtoM( uptr to, x86IntRegType from ) 
{
	RexR(0,from);
	write8<I>( 0x88 );
	ModRM<I>( 0, from, DISP32 );
	write32<I>( MEMADDR(to, 4) );
}

/* mov m8 to r8 */
emitterT void eMOV8MtoR( x86IntRegType to, uptr from ) 
{
	RexR(0,to);
	write8<I>( 0x8A );
	ModRM<I>( 0, to, DISP32 );
	write32<I>( MEMADDR(from, 4) ); 
}

/* mov [r32] to r8 */
emitterT void eMOV8RmtoR(x86IntRegType to, x86IntRegType from)
{
	RexRB(0,to,from);
	write8<I>( 0x8A );
	WriteRmOffsetFrom<I>(to, from, 0);
}

emitterT void eMOV8RmtoROffset(x86IntRegType to, x86IntRegType from, int offset)
{
	RexRB(0,to,from);
	write8<I>( 0x8A );
	WriteRmOffsetFrom<I>(to, from, offset);
}

emitterT void eMOV8RmSOffsettoR( x86IntRegType to, x86IntRegType from1, u32 from2, int scale )
{
	RexRXB(0,to,from1,0);
	write8<I>( 0x8A );
	ModRM<I>( 0, to, SIB );
	SibSB<I>( scale, from1, SIBDISP);
	write32<I>(from2);
}

emitterT void eMOV8RtoRm(x86IntRegType to, x86IntRegType from)
{
	RexRB(0,from,to);
	write8<I>( 0x88 );
	WriteRmOffsetFrom<I>(from, to, 0);
}

/* mov imm8 to m8 */
emitterT void eMOV8ItoM( uptr to, u8 from ) 
{
	write8<I>( 0xC6 );
	ModRM<I>( 0, 0, DISP32 );
	write32<I>( MEMADDR(to, 5) );
	write8<I>( from ); 
}

// mov imm8 to r8
emitterT void eMOV8ItoR( x86IntRegType to, u8 from )
{
	RexB(0, to);
	write8<I>( 0xB0 | (to & 0x7) ); 
	write8<I>( from );
}

// mov imm8 to [r8+off]
emitterT void eMOV8ItoRmOffset( x86IntRegType to, u8 from, int offset)
{
	assert( to != ESP );
	RexB(0,to);
	write8<I>( 0xC6 );
	WriteRmOffset<I>(to,offset);
	write8<I>(from);
}

// mov r8 to [r8+off]
emitterT void eMOV8RtoRmOffset( x86IntRegType to, x86IntRegType from, int offset)
{
	assert( to != ESP );
	RexRB(0,from,to);
	write8<I>( 0x88 );
	WriteRmOffsetFrom<I>(from,to,offset);
}

/* movsx r8 to r32 */
emitterT void eMOVSX32R8toR( x86IntRegType to, x86IntRegType from ) 
{
	RexRB(0,to,from);
	write16<I>( 0xBE0F ); 
	ModRM<I>( 3, to, from ); 
}

emitterT void eMOVSX32Rm8toR( x86IntRegType to, x86IntRegType from )
{
	RexRB(0,to,from);
	write16<I>( 0xBE0F ); 
	ModRM<I>( 0, to, from ); 
}

emitterT void eMOVSX32Rm8toROffset( x86IntRegType to, x86IntRegType from, int offset )
{
	RexRB(0,to,from);
	write16<I>( 0xBE0F ); 
	WriteRmOffsetFrom<I>(to,from,offset);
}

/* movsx m8 to r32 */
emitterT void eMOVSX32M8toR( x86IntRegType to, u32 from ) 
{
	RexR(0,to);
	write16<I>( 0xBE0F ); 
	ModRM<I>( 0, to, DISP32 );
	write32<I>( MEMADDR(from, 4) );
}

/* movsx r16 to r32 */
emitterT void eMOVSX32R16toR( x86IntRegType to, x86IntRegType from ) 
{
	RexRB(0,to,from);
	write16<I>( 0xBF0F ); 
	ModRM<I>( 3, to, from ); 
}

emitterT void eMOVSX32Rm16toR( x86IntRegType to, x86IntRegType from )
{
	RexRB(0,to,from);
	write16<I>( 0xBF0F ); 
	ModRM<I>( 0, to, from ); 
}

emitterT void eMOVSX32Rm16toROffset( x86IntRegType to, x86IntRegType from, int offset )
{
	RexRB(0,to,from);
	write16<I>( 0xBF0F );
	WriteRmOffsetFrom<I>(to,from,offset);
}

/* movsx m16 to r32 */
emitterT void eMOVSX32M16toR( x86IntRegType to, u32 from ) 
{
	RexR(0,to);
	write16<I>( 0xBF0F ); 
	ModRM<I>( 0, to, DISP32 );
	write32<I>( MEMADDR(from, 4) );
}

/* movzx r8 to r32 */
emitterT void eMOVZX32R8toR( x86IntRegType to, x86IntRegType from ) 
{
	RexRB(0,to,from);
	write16<I>( 0xB60F ); 
	ModRM<I>( 3, to, from ); 
}

emitterT void eMOVZX32Rm8toR( x86IntRegType to, x86IntRegType from )
{
	RexRB(0,to,from);
	write16<I>( 0xB60F ); 
	ModRM<I>( 0, to, from );
}

emitterT void eMOVZX32Rm8toROffset( x86IntRegType to, x86IntRegType from, int offset )
{
	RexRB(0,to,from);
	write16<I>( 0xB60F );
	WriteRmOffsetFrom<I>(to,from,offset);
}

/* movzx m8 to r32 */
emitterT void eMOVZX32M8toR( x86IntRegType to, u32 from ) 
{
	RexR(0,to);
	write16<I>( 0xB60F ); 
	ModRM<I>( 0, to, DISP32 );
	write32<I>( MEMADDR(from, 4) );
}

/* movzx r16 to r32 */
emitterT void eMOVZX32R16toR( x86IntRegType to, x86IntRegType from ) 
{
	RexRB(0,to,from);
	write16<I>( 0xB70F ); 
	ModRM<I>( 3, to, from ); 
}

emitterT void eMOVZX32Rm16toR( x86IntRegType to, x86IntRegType from )
{
	RexRB(0,to,from);
	write16<I>( 0xB70F ); 
	ModRM<I>( 0, to, from ); 
}

emitterT void eMOVZX32Rm16toROffset( x86IntRegType to, x86IntRegType from, int offset )
{
	RexRB(0,to,from);
	write16<I>( 0xB70F );
	WriteRmOffsetFrom<I>(to,from,offset);
}

/* movzx m16 to r32 */
emitterT void eMOVZX32M16toR( x86IntRegType to, u32 from ) 
{
	RexR(0,to);
	write16<I>( 0xB70F ); 
	ModRM<I>( 0, to, DISP32 );
	write32<I>( MEMADDR(from, 4) );
}

/* cmovbe r32 to r32 */
emitterT void eCMOVBE32RtoR( x86IntRegType to, x86IntRegType from )
{
	CMOV32RtoR<I>( 0x46, to, from );
}

/* cmovbe m32 to r32*/
emitterT void eCMOVBE32MtoR( x86IntRegType to, uptr from )
{
	CMOV32MtoR<I>( 0x46, to, from );
}

/* cmovb r32 to r32 */
emitterT void eCMOVB32RtoR( x86IntRegType to, x86IntRegType from )
{
	CMOV32RtoR<I>( 0x42, to, from );
}

/* cmovb m32 to r32*/
emitterT void eCMOVB32MtoR( x86IntRegType to, uptr from )
{
	CMOV32MtoR<I>( 0x42, to, from );
}

/* cmovae r32 to r32 */
emitterT void eCMOVAE32RtoR( x86IntRegType to, x86IntRegType from )
{
	CMOV32RtoR<I>( 0x43, to, from );
}

/* cmovae m32 to r32*/
emitterT void eCMOVAE32MtoR( x86IntRegType to, uptr from )
{
	CMOV32MtoR<I>( 0x43, to, from );
}

/* cmova r32 to r32 */
emitterT void eCMOVA32RtoR( x86IntRegType to, x86IntRegType from )
{
	CMOV32RtoR<I>( 0x47, to, from );
}

/* cmova m32 to r32*/
emitterT void eCMOVA32MtoR( x86IntRegType to, uptr from )
{
	CMOV32MtoR<I>( 0x47, to, from );
}

/* cmovo r32 to r32 */
emitterT void eCMOVO32RtoR( x86IntRegType to, x86IntRegType from )
{
	CMOV32RtoR<I>( 0x40, to, from );
}

/* cmovo m32 to r32 */
emitterT void eCMOVO32MtoR( x86IntRegType to, uptr from )
{
	CMOV32MtoR<I>( 0x40, to, from );
}

/* cmovp r32 to r32 */
emitterT void eCMOVP32RtoR( x86IntRegType to, x86IntRegType from )
{
	CMOV32RtoR<I>( 0x4A, to, from );
}

/* cmovp m32 to r32 */
emitterT void eCMOVP32MtoR( x86IntRegType to, uptr from )
{
	CMOV32MtoR<I>( 0x4A, to, from );
}

/* cmovs r32 to r32 */
emitterT void eCMOVS32RtoR( x86IntRegType to, x86IntRegType from )
{
	CMOV32RtoR<I>( 0x48, to, from );
}

/* cmovs m32 to r32 */
emitterT void eCMOVS32MtoR( x86IntRegType to, uptr from )
{
	CMOV32MtoR<I>( 0x48, to, from );
}

/* cmovno r32 to r32 */
emitterT void eCMOVNO32RtoR( x86IntRegType to, x86IntRegType from )
{
	CMOV32RtoR<I>( 0x41, to, from );
}

/* cmovno m32 to r32 */
emitterT void eCMOVNO32MtoR( x86IntRegType to, uptr from )
{
	CMOV32MtoR<I>( 0x41, to, from );
}

/* cmovnp r32 to r32 */
emitterT void eCMOVNP32RtoR( x86IntRegType to, x86IntRegType from )
{
	CMOV32RtoR<I>( 0x4B, to, from );
}

/* cmovnp m32 to r32 */
emitterT void eCMOVNP32MtoR( x86IntRegType to, uptr from )
{
	CMOV32MtoR<I>( 0x4B, to, from );
}

/* cmovns r32 to r32 */
emitterT void eCMOVNS32RtoR( x86IntRegType to, x86IntRegType from )
{
	CMOV32RtoR<I>( 0x49, to, from );
}

/* cmovns m32 to r32 */
emitterT void eCMOVNS32MtoR( x86IntRegType to, uptr from )
{
	CMOV32MtoR<I>( 0x49, to, from );
}

/* cmovne r32 to r32 */
emitterT void eCMOVNE32RtoR( x86IntRegType to, x86IntRegType from )
{
	CMOV32RtoR<I>( 0x45, to, from );
}

/* cmovne m32 to r32*/
emitterT void eCMOVNE32MtoR( x86IntRegType to, uptr from ) 
{
	CMOV32MtoR<I>( 0x45, to, from );
}

/* cmove r32 to r32*/
emitterT void eCMOVE32RtoR( x86IntRegType to, x86IntRegType from ) 
{
	CMOV32RtoR<I>( 0x44, to, from );
}

/* cmove m32 to r32*/
emitterT void eCMOVE32MtoR( x86IntRegType to, uptr from ) 
{
	CMOV32MtoR<I>( 0x44, to, from );
}

/* cmovg r32 to r32*/
emitterT void eCMOVG32RtoR( x86IntRegType to, x86IntRegType from ) 
{
	CMOV32RtoR<I>( 0x4F, to, from );
}

/* cmovg m32 to r32*/
emitterT void eCMOVG32MtoR( x86IntRegType to, uptr from ) 
{
	CMOV32MtoR<I>( 0x4F, to, from );
}

/* cmovge r32 to r32*/
emitterT void eCMOVGE32RtoR( x86IntRegType to, x86IntRegType from ) 
{
	CMOV32RtoR<I>( 0x4D, to, from );
}

/* cmovge m32 to r32*/
emitterT void eCMOVGE32MtoR( x86IntRegType to, uptr from ) 
{
	CMOV32MtoR<I>( 0x4D, to, from );
}

/* cmovl r32 to r32*/
emitterT void eCMOVL32RtoR( x86IntRegType to, x86IntRegType from ) 
{
	CMOV32RtoR<I>( 0x4C, to, from );
}

/* cmovl m32 to r32*/
emitterT void eCMOVL32MtoR( x86IntRegType to, uptr from ) 
{
	CMOV32MtoR<I>( 0x4C, to, from );
}

/* cmovle r32 to r32*/
emitterT void eCMOVLE32RtoR( x86IntRegType to, x86IntRegType from ) 
{
	CMOV32RtoR<I>( 0x4E, to, from );
}

/* cmovle m32 to r32*/
emitterT void eCMOVLE32MtoR( x86IntRegType to, uptr from ) 
{
	CMOV32MtoR<I>( 0x4E, to, from );
}

////////////////////////////////////
// arithmetic instructions		 /
////////////////////////////////////

/* add imm32 to r64 */
emitterT void eADD64ItoR( x86IntRegType to, u32 from ) 
{
	Rex(1, 0, 0, to >> 3);
	if ( to == EAX) {
		write8<I>( 0x05 ); 
	} 
	else {
		write8<I>( 0x81 ); 
		ModRM<I>( 3, 0, to );
	}
	write32<I>( from );
}

/* add m64 to r64 */
emitterT void eADD64MtoR( x86IntRegType to, uptr from ) 
{
	Rex(1, to >> 3, 0, 0);
	write8<I>( 0x03 ); 
	ModRM<I>( 0, to, DISP32 );
	write32<I>( MEMADDR(from, 4) );
}

/* add r64 to r64 */
emitterT void eADD64RtoR( x86IntRegType to, x86IntRegType from ) 
{
	RexRB(1, from, to);
	write8<I>( 0x01 ); 
	ModRM<I>( 3, from, to );
}

/* add imm32 to EAX */
emitterT void eADD32ItoEAX( s32 imm )
{
	write8<I>( 0x05 );
	write32<I>( imm );
}

/* add imm32 to r32 */
emitterT void eADD32ItoR( x86IntRegType to, s32 imm ) 
{
	RexB(0, to);
	if (imm <= 127 && imm >= -128)
	{
		write8<I>( 0x83 ); 
		ModRM<I>( 3, 0, to );
		write8<I>( (s8)imm ); 
	}
	else
	{
		if ( to == EAX ) {
			eADD32ItoEAX<I>(imm);
		}
		else {
			write8<I>( 0x81 ); 
			ModRM<I>( 3, 0, to );
			write32<I>( imm );
		}
	}
}

/* add imm32 to m32 */
emitterT void eADD32ItoM( uptr to, s32 imm ) 
{ 
	if(imm <= 127 && imm >= -128)
	{
	write8<I>( 0x83 ); 
	ModRM<I>( 0, 0, DISP32 );
	write32<I>( MEMADDR(to, 8) );
	write8<I>( imm );
	}
	else
	{
		write8<I>( 0x81 ); 
		ModRM<I>( 0, 0, DISP32 );
		write32<I>( MEMADDR(to, 8) );
		write32<I>( imm );
	}
}

// add imm32 to [r32+off]
emitterT void eADD32ItoRmOffset( x86IntRegType to, s32 imm, s32 offset)
{
	RexB(0,to);
	if(imm <= 127 && imm >= -128)
	{
		write8<I>( 0x83 );
		WriteRmOffset<I>(to,offset);
		write8<I>(imm);
	} 
	else 
	{
		write8<I>( 0x81 );
		WriteRmOffset<I>(to,offset);
		write32<I>(imm);
	}
}

/* add r32 to r32 */
emitterT void eADD32RtoR( x86IntRegType to, x86IntRegType from ) 
{
	RexRB(0, from, to);
	write8<I>( 0x01 ); 
	ModRM<I>( 3, from, to );
}

/* add r32 to m32 */
emitterT void eADD32RtoM(uptr to, x86IntRegType from ) 
{
	RexR(0,from);
	write8<I>( 0x01 ); 
	ModRM<I>( 0, from, DISP32 );
	write32<I>( MEMADDR(to, 4) );
}

/* add m32 to r32 */
emitterT void eADD32MtoR( x86IntRegType to, uptr from ) 
{
	RexR(0,to);
	write8<I>( 0x03 ); 
	ModRM<I>( 0, to, DISP32 );
	write32<I>( MEMADDR(from, 4) );
}

// add r16 to r16 
emitterT void eADD16RtoR( x86IntRegType to , x86IntRegType from )
{
	write8<I>(0x66);
	RexRB(0,to,from);
	write8<I>( 0x03 ); 
	ModRM<I>( 3, to, from );
}

/* add imm16 to r16 */
emitterT void eADD16ItoR( x86IntRegType to, s16 imm ) 
{
	write8<I>( 0x66 );
	RexB(0,to);

	if ( to == EAX) 
	{
		write8<I>( 0x05 ); 
		write16<I>( imm );
	}
	else if(imm <= 127 && imm >= -128)
	{
		write8<I>( 0x83 ); 
		ModRM<I>( 3, 0, to );
		write8<I>((u8)imm );
	}
	else
	{
		write8<I>( 0x81 ); 
		ModRM<I>( 3, 0, to );
		write16<I>( imm );
	}
}

/* add imm16 to m16 */
emitterT void eADD16ItoM( uptr to, s16 imm ) 
{
	write8<I>( 0x66 );
	if(imm <= 127 && imm >= -128)
	{
		write8<I>( 0x83 ); 
		ModRM<I>( 0, 0, DISP32 );
		write32<I>( MEMADDR(to, 6) );
		write8<I>((u8)imm );
	}
	else
	{
		write8<I>( 0x81 ); 
		ModRM<I>( 0, 0, DISP32 );
		write32<I>( MEMADDR(to, 6) );
		write16<I>( imm );
	}
}

/* add r16 to m16 */
emitterT void eADD16RtoM(uptr to, x86IntRegType from ) 
{
	write8<I>( 0x66 );
	RexR(0,from);
	write8<I>( 0x01 ); 
	ModRM<I>( 0, from, DISP32 );
	write32<I>( MEMADDR(to, 4) );
}

/* add m16 to r16 */
emitterT void eADD16MtoR( x86IntRegType to, uptr from ) 
{
	write8<I>( 0x66 );
	RexR(0,to);
	write8<I>( 0x03 ); 
	ModRM<I>( 0, to, DISP32 );
	write32<I>( MEMADDR(from, 4) );
}

// add m8 to r8
emitterT void eADD8MtoR( x86IntRegType to, uptr from )
{
	RexR(0,to);
	write8<I>( 0x02 ); 
	ModRM<I>( 0, to, DISP32 );
	write32<I>( MEMADDR(from, 4) );
}

/* adc imm32 to r32 */
emitterT void eADC32ItoR( x86IntRegType to, u32 from ) 
{
	RexB(0,to);
	if ( to == EAX ) {
		write8<I>( 0x15 );
	}
	else {
		write8<I>( 0x81 );
		ModRM<I>( 3, 2, to );
	}
	write32<I>( from ); 
}

/* adc imm32 to m32 */
emitterT void eADC32ItoM( uptr to, u32 from ) 
{
	write8<I>( 0x81 ); 
	ModRM<I>( 0, 2, DISP32 );
	write32<I>( MEMADDR(to, 8) );
	write32<I>( from );
}

/* adc r32 to r32 */
emitterT void eADC32RtoR( x86IntRegType to, x86IntRegType from ) 
{
	RexRB(0,from,to);
	write8<I>( 0x11 ); 
	ModRM<I>( 3, from, to );
}

/* adc m32 to r32 */
emitterT void eADC32MtoR( x86IntRegType to, uptr from ) 
{
	RexR(0,to);
	write8<I>( 0x13 ); 
	ModRM<I>( 0, to, DISP32 );
	write32<I>( MEMADDR(from, 4) ); 
}

// adc r32 to m32 
emitterT void eADC32RtoM( uptr to, x86IntRegType from )
{
	RexR(0,from);
	write8<I>( 0x11 ); 
	ModRM<I>( 0, from, DISP32 );
	write32<I>( MEMADDR(to, 4) );
}

/* inc r32 */
emitterT void eINC32R( x86IntRegType to ) 
{
	write8<I>( 0x40 + to );
}

/* inc m32 */
emitterT void eINC32M( u32 to ) 
{
	write8<I>( 0xFF );
	ModRM<I>( 0, 0, DISP32 );
	write32<I>( MEMADDR(to, 4) );
}

/* inc r16 */
emitterT void eINC16R( x86IntRegType to ) 
{
	write8<I>( 0x66 );
	write8<I>( 0x40 + to );
}

/* inc m16 */
emitterT void eINC16M( u32 to ) 
{
	write8<I>( 0x66 );
	write8<I>( 0xFF );
	ModRM<I>( 0, 0, DISP32 );
	write32<I>( MEMADDR(to, 4) );
}


/* sub imm32 to r64 */
emitterT void eSUB64ItoR( x86IntRegType to, u32 from ) 
{
	RexB(1, to);
	if ( to == EAX ) {
		write8<I>( 0x2D ); 
	} 
	else {
		write8<I>( 0x81 ); 
		ModRM<I>( 3, 5, to );
	}
	write32<I>( from ); 
}

/* sub r64 to r64 */
emitterT void eSUB64RtoR( x86IntRegType to, x86IntRegType from ) 
{
	RexRB(1, from, to);
	write8<I>( 0x29 ); 
	ModRM<I>( 3, from, to );
}

/* sub m64 to r64 */
emitterT void eSUB64MtoR( x86IntRegType to, uptr from ) 
{
	RexR(1, to);
	write8<I>( 0x2B ); 
	ModRM<I>( 0, to, DISP32 );
	write32<I>( MEMADDR(from, 4) ); 
}

/* sub imm32 to r32 */
emitterT void eSUB32ItoR( x86IntRegType to, u32 from ) 
{
	RexB(0,to);
	if ( to == EAX ) {
		write8<I>( 0x2D ); 
	}
	else {
		write8<I>( 0x81 ); 
		ModRM<I>( 3, 5, to );
	}
	write32<I>( from ); 
}

/* sub imm32 to m32 */
emitterT void eSUB32ItoM( uptr to, u32 from ) 
{
	write8<I>( 0x81 ); 
	ModRM<I>( 0, 5, DISP32 );
	write32<I>( MEMADDR(to, 8) );
	write32<I>( from );
}

/* sub r32 to r32 */
emitterT void eSUB32RtoR( x86IntRegType to, x86IntRegType from ) 
{
	RexRB(0, from, to);
	write8<I>( 0x29 ); 
	ModRM<I>( 3, from, to );
}

/* sub m32 to r32 */
emitterT void eSUB32MtoR( x86IntRegType to, uptr from ) 
{
	RexR(0,to);
	write8<I>( 0x2B ); 
	ModRM<I>( 0, to, DISP32 );
	write32<I>( MEMADDR(from, 4) ); 
}

// sub r32 to m32 
emitterT void eSUB32RtoM( uptr to, x86IntRegType from )
{
	RexR(0,from);
	write8<I>( 0x29 ); 
	ModRM<I>( 0, from, DISP32 );
	write32<I>( MEMADDR(to, 4) );
}

// sub r16 to r16 
emitterT void eSUB16RtoR( x86IntRegType to, u16 from )
{
	write8<I>(0x66);
	RexRB(0,to,from);
	write8<I>( 0x2b ); 
	ModRM<I>( 3, to, from );
}

/* sub imm16 to r16 */
emitterT void eSUB16ItoR( x86IntRegType to, u16 from ) {
	write8<I>( 0x66 );
	RexB(0,to);
	if ( to == EAX ) {
		write8<I>( 0x2D ); 
	} 
	else {
		write8<I>( 0x81 ); 
		ModRM<I>( 3, 5, to );
	}
	write16<I>( from ); 
}

/* sub imm16 to m16 */
emitterT void eSUB16ItoM( uptr to, u16 from ) {
	write8<I>( 0x66 ); 
	write8<I>( 0x81 ); 
	ModRM<I>( 0, 5, DISP32 );
	write32<I>( MEMADDR(to, 6) );
	write16<I>( from );
}

/* sub m16 to r16 */
emitterT void eSUB16MtoR( x86IntRegType to, uptr from ) {
	write8<I>( 0x66 ); 
	RexR(0,to);
	write8<I>( 0x2B ); 
	ModRM<I>( 0, to, DISP32 );
	write32<I>( MEMADDR(from, 4) ); 
}

/* sbb r64 to r64 */
emitterT void eSBB64RtoR( x86IntRegType to, x86IntRegType from ) {
	RexRB(1, from,to);
	write8<I>( 0x19 ); 
	ModRM<I>( 3, from, to );
}

/* sbb imm32 to r32 */
emitterT void eSBB32ItoR( x86IntRegType to, u32 from ) {
	RexB(0,to);
	if ( to == EAX ) {
		write8<I>( 0x1D );
	} 
	else {
		write8<I>( 0x81 );
		ModRM<I>( 3, 3, to );
	}
	write32<I>( from ); 
}

/* sbb imm32 to m32 */
emitterT void eSBB32ItoM( uptr to, u32 from ) {
	write8<I>( 0x81 );
	ModRM<I>( 0, 3, DISP32 );
	write32<I>( MEMADDR(to, 8) );
	write32<I>( from );
}

/* sbb r32 to r32 */
emitterT void eSBB32RtoR( x86IntRegType to, x86IntRegType from ) 
{
	RexRB(0,from,to);
	write8<I>( 0x19 ); 
	ModRM<I>( 3, from, to );
}

/* sbb m32 to r32 */
emitterT void eSBB32MtoR( x86IntRegType to, uptr from ) 
{
	RexR(0,to);
	write8<I>( 0x1B ); 
	ModRM<I>( 0, to, DISP32 );
	write32<I>( MEMADDR(from, 4) ); 
}

/* sbb r32 to m32 */
emitterT void eSBB32RtoM( uptr to, x86IntRegType from ) 
{
	RexR(0,from);
	write8<I>( 0x19 ); 
	ModRM<I>( 0, from, DISP32 );
	write32<I>( MEMADDR(to, 4) );
}

/* dec r32 */
emitterT void eDEC32R( x86IntRegType to ) 
{
	write8<I>( 0x48 + to );
}

/* dec m32 */
emitterT void eDEC32M( u32 to ) 
{
	write8<I>( 0xFF );
	ModRM<I>( 0, 1, DISP32 );
	write32<I>( MEMADDR(to, 4) );
}

/* dec r16 */
emitterT void eDEC16R( x86IntRegType to ) 
{
	write8<I>( 0x66 );
	write8<I>( 0x48 + to );
}

/* dec m16 */
emitterT void eDEC16M( u32 to ) 
{
	write8<I>( 0x66 );
	write8<I>( 0xFF );
	ModRM<I>( 0, 1, DISP32 );
	write32<I>( MEMADDR(to, 4) );
}

/* mul eax by r32 to edx:eax */
emitterT void eMUL32R( x86IntRegType from ) 
{
	RexB(0,from);
	write8<I>( 0xF7 ); 
	ModRM<I>( 3, 4, from );
}

/* imul eax by r32 to edx:eax */
emitterT void eIMUL32R( x86IntRegType from ) 
{
	RexB(0,from);
	write8<I>( 0xF7 ); 
	ModRM<I>( 3, 5, from );
}

/* mul eax by m32 to edx:eax */
emitterT void eMUL32M( u32 from ) 
{
	write8<I>( 0xF7 ); 
	ModRM<I>( 0, 4, DISP32 );
	write32<I>( MEMADDR(from, 4) ); 
}

/* imul eax by m32 to edx:eax */
emitterT void eIMUL32M( u32 from ) 
{
	write8<I>( 0xF7 ); 
	ModRM<I>( 0, 5, DISP32 );
	write32<I>( MEMADDR(from, 4) ); 
}

/* imul r32 by r32 to r32 */
emitterT void eIMUL32RtoR( x86IntRegType to, x86IntRegType from ) 
{
	RexRB(0,to,from);
	write16<I>( 0xAF0F ); 
	ModRM<I>( 3, to, from );
}

/* div eax by r32 to edx:eax */
emitterT void eDIV32R( x86IntRegType from ) 
{
	RexB(0,from);
	write8<I>( 0xF7 ); 
	ModRM<I>( 3, 6, from );
}

/* idiv eax by r32 to edx:eax */
emitterT void eIDIV32R( x86IntRegType from ) 
{
	RexB(0,from);
	write8<I>( 0xF7 ); 
	ModRM<I>( 3, 7, from );
}

/* div eax by m32 to edx:eax */
emitterT void eDIV32M( u32 from ) 
{
	write8<I>( 0xF7 ); 
	ModRM<I>( 0, 6, DISP32 );
	write32<I>( MEMADDR(from, 4) ); 
}

/* idiv eax by m32 to edx:eax */
emitterT void eIDIV32M( u32 from ) 
{
	write8<I>( 0xF7 ); 
	ModRM<I>( 0, 7, DISP32 );
	write32<I>( MEMADDR(from, 4) ); 
}

////////////////////////////////////
// shifting instructions			/
////////////////////////////////////

/* shl imm8 to r64 */
emitterT void eSHL64ItoR( x86IntRegType to, u8 from ) 
{
	RexB(1, to);
	if ( from == 1 )
	{
		write8<I>( 0xD1 );
		ModRM<I>( 3, 4, to );
		return;
	}
	write8<I>( 0xC1 ); 
	ModRM<I>( 3, 4, to );
	write8<I>( from ); 
}

/* shl cl to r64 */
emitterT void eSHL64CLtoR( x86IntRegType to ) 
{
	RexB(1, to);
	write8<I>( 0xD3 ); 
	ModRM<I>( 3, 4, to );
}

/* shr imm8 to r64 */
emitterT void eSHR64ItoR( x86IntRegType to, u8 from ) 
{
	RexB(1,to);
	if ( from == 1 ) {
		write8<I>( 0xD1 );
		ModRM<I>( 3, 5, to );
		return;
	}
	write8<I>( 0xC1 ); 
	ModRM<I>( 3, 5, to );
	write8<I>( from ); 
}

/* shr cl to r64 */
emitterT void eSHR64CLtoR( x86IntRegType to ) 
{
	RexB(1, to);
	write8<I>( 0xD3 ); 
	ModRM<I>( 3, 5, to );
}

/* shl imm8 to r32 */
emitterT void eSHL32ItoR( x86IntRegType to, u8 from ) 
{
	RexB(0, to);
	if ( from == 1 )
	{
		write8<I>( 0xD1 );
		write8<I>( 0xE0 | (to & 0x7) );
		return;
	}
	write8<I>( 0xC1 ); 
	ModRM<I>( 3, 4, to );
	write8<I>( from ); 
}

/* shl imm8 to m32 */
emitterT void eSHL32ItoM( uptr to, u8 from ) 
{
	if ( from == 1 )
	{
		write8<I>( 0xD1 );
		ModRM<I>( 0, 4, DISP32 );
		write32<I>( MEMADDR(to, 4) );
	}
	else
	{
		write8<I>( 0xC1 ); 
		ModRM<I>( 0, 4, DISP32 );
		write32<I>( MEMADDR(to, 5) );
		write8<I>( from ); 
	}
}

/* shl cl to r32 */
emitterT void eSHL32CLtoR( x86IntRegType to ) 
{
	RexB(0,to);
	write8<I>( 0xD3 ); 
	ModRM<I>( 3, 4, to );
}

// shl imm8 to r16
emitterT void eSHL16ItoR( x86IntRegType to, u8 from )
{
	write8<I>(0x66);
	RexB(0,to);
	if ( from == 1 )
	{
		write8<I>( 0xD1 );
		write8<I>( 0xE0 | (to & 0x7) );
		return;
	}
	write8<I>( 0xC1 ); 
	ModRM<I>( 3, 4, to );
	write8<I>( from ); 
}

// shl imm8 to r8
emitterT void eSHL8ItoR( x86IntRegType to, u8 from )
{
	RexB(0,to);
	if ( from == 1 )
	{
		write8<I>( 0xD0 );
		write8<I>( 0xE0 | (to & 0x7) );
		return;
	}
	write8<I>( 0xC0 ); 
	ModRM<I>( 3, 4, to );
	write8<I>( from ); 
}

/* shr imm8 to r32 */
emitterT void eSHR32ItoR( x86IntRegType to, u8 from ) {
	RexB(0,to);
	if ( from == 1 )
	{
		write8<I>( 0xD1 );
		write8<I>( 0xE8 | (to & 0x7) );
	}
	else
	{
		write8<I>( 0xC1 ); 
		ModRM<I>( 3, 5, to );
		write8<I>( from ); 
	}
}

/* shr imm8 to m32 */
emitterT void eSHR32ItoM( uptr to, u8 from ) 
{
	if ( from == 1 )
	{
		write8<I>( 0xD1 );
		ModRM<I>( 0, 5, DISP32 );
		write32<I>( MEMADDR(to, 4) );
	}
	else
	{
		write8<I>( 0xC1 ); 
		ModRM<I>( 0, 5, DISP32 );
		write32<I>( MEMADDR(to, 5) );
		write8<I>( from ); 
	}
}

/* shr cl to r32 */
emitterT void eSHR32CLtoR( x86IntRegType to ) 
{
	RexB(0,to);
	write8<I>( 0xD3 ); 
	ModRM<I>( 3, 5, to );
}

// shr imm8 to r16
emitterT void eSHR16ItoR( x86IntRegType to, u8 from )
{
	RexB(0,to);
	if ( from == 1 )
	{
		write8<I>( 0xD1 );
		ModRM<I>( 3, 5, to );
	}
	else
	{
		write8<I>( 0xC1 ); 
		ModRM<I>( 3, 5, to );
		write8<I>( from ); 
	}
}

// shr imm8 to r8
emitterT void eSHR8ItoR( x86IntRegType to, u8 from )
{
	RexB(0,to);
	if ( from == 1 )
	{
		write8<I>( 0xD0 );
		write8<I>( 0xE8 | (to & 0x7) );
	}
	else
	{
		write8<I>( 0xC0 ); 
		ModRM<I>( 3, 5, to );
		write8<I>( from ); 
	}
}

/* sar imm8 to r64 */
emitterT void eSAR64ItoR( x86IntRegType to, u8 from ) 
{
	RexB(1,to);
	if ( from == 1 )
	{
		write8<I>( 0xD1 );
		ModRM<I>( 3, 7, to );
		return;
	}
	write8<I>( 0xC1 ); 
	ModRM<I>( 3, 7, to );
	write8<I>( from ); 
}

/* sar cl to r64 */
emitterT void eSAR64CLtoR( x86IntRegType to ) 
{
	RexB(1, to);
	write8<I>( 0xD3 ); 
	ModRM<I>( 3, 7, to );
}

/* sar imm8 to r32 */
emitterT void eSAR32ItoR( x86IntRegType to, u8 from ) 
{
	RexB(0,to);
	if ( from == 1 )
	{
		write8<I>( 0xD1 );
		ModRM<I>( 3, 7, to );
		return;
	}
	write8<I>( 0xC1 ); 
	ModRM<I>( 3, 7, to );
	write8<I>( from ); 
}

/* sar imm8 to m32 */
emitterT void eSAR32ItoM( uptr to, u8 from )
{
	write8<I>( 0xC1 ); 
	ModRM<I>( 0, 7, DISP32 );
	write32<I>( MEMADDR(to, 5) );
	write8<I>( from );
}

/* sar cl to r32 */
emitterT void eSAR32CLtoR( x86IntRegType to ) 
{
	RexB(0,to);
	write8<I>( 0xD3 ); 
	ModRM<I>( 3, 7, to );
}

// sar imm8 to r16
emitterT void eSAR16ItoR( x86IntRegType to, u8 from )
{
	write8<I>(0x66);
	RexB(0,to);
	if ( from == 1 )
	{
		write8<I>( 0xD1 );
		ModRM<I>( 3, 7, to );
		return;
	}
	write8<I>( 0xC1 ); 
	ModRM<I>( 3, 7, to );
	write8<I>( from ); 
}

emitterT void eROR32ItoR( x86IntRegType to,u8 from )
{
	RexB(0,to);
	if ( from == 1 ) {
		write8<I>( 0xd1 );
		write8<I>( 0xc8 | to );
	} 
	else 
	{
		write8<I>( 0xc1 );
		write8<I>( 0xc8 | to );
		write8<I>( from );
	}
}

emitterT void eRCR32ItoR( x86IntRegType to, u8 from ) 
{
	RexB(0,to);
	if ( from == 1 ) {
		write8<I>( 0xd1 );
		ModRM<I>(3, 3, to);
	} 
	else 
	{
		write8<I>( 0xc1 );
		ModRM<I>(3, 3, to);
		write8<I>( from );
	}
}

emitterT void eRCR32ItoM( uptr to, u8 from ) 
{
	RexB(0,to);
	if ( from == 1 ) {
		write8<I>( 0xd1 );
		ModRM<I>( 0, 3, DISP32 );
		write32<I>( MEMADDR(to, 8) );
	} 
	else 
	{
		write8<I>( 0xc1 );
		ModRM<I>( 0, 3, DISP32 );
		write32<I>( MEMADDR(to, 8) );
		write8<I>( from );
	}
}

// shld imm8 to r32
emitterT void eSHLD32ItoR( x86IntRegType to, x86IntRegType from, u8 shift )
{
	RexRB(0,from,to);
	write8<I>( 0x0F );
	write8<I>( 0xA4 );
	ModRM<I>( 3, from, to );
	write8<I>( shift );
}

// shrd imm8 to r32
emitterT void eSHRD32ItoR( x86IntRegType to, x86IntRegType from, u8 shift )
{
	RexRB(0,from,to);
	write8<I>( 0x0F );
	write8<I>( 0xAC );
	ModRM<I>( 3, from, to );
	write8<I>( shift );
}

////////////////////////////////////
// logical instructions			/
////////////////////////////////////

/* or imm32 to r32 */
emitterT void eOR64ItoR( x86IntRegType to, u32 from ) 
{
	RexB(1, to);
	if ( to == EAX ) {
		write8<I>( 0x0D ); 
	} 
	else {
		write8<I>( 0x81 ); 
		ModRM<I>( 3, 1, to );
	}
	write32<I>( from ); 
}

/* or m64 to r64 */
emitterT void eOR64MtoR( x86IntRegType to, uptr from ) 
{
	RexR(1, to);
	write8<I>( 0x0B ); 
	ModRM<I>( 0, to, DISP32 );
	write32<I>( MEMADDR(from, 4) ); 
}

/* or r64 to r64 */
emitterT void eOR64RtoR( x86IntRegType to, x86IntRegType from ) 
{
	RexRB(1, from, to);
	write8<I>( 0x09 ); 
	ModRM<I>( 3, from, to );
}

// or r32 to m64
emitterT void eOR64RtoM(uptr to, x86IntRegType from ) 
{
	RexR(1,from);
	write8<I>( 0x09 ); 
	ModRM<I>( 0, from, DISP32 );
	write32<I>( MEMADDR(to, 4) );
}

/* or imm32 to r32 */
emitterT void eOR32ItoR( x86IntRegType to, u32 from ) 
{
	RexB(0,to);
	if ( to == EAX ) {
		write8<I>( 0x0D ); 
	}
	else {
		write8<I>( 0x81 ); 
		ModRM<I>( 3, 1, to );
	}
	write32<I>( from ); 
}

/* or imm32 to m32 */
emitterT void eOR32ItoM(uptr to, u32 from ) 
{
	write8<I>( 0x81 ); 
	ModRM<I>( 0, 1, DISP32 );
	write32<I>( MEMADDR(to, 8) );
	write32<I>( from ); 
}

/* or r32 to r32 */
emitterT void eOR32RtoR( x86IntRegType to, x86IntRegType from ) 
{
	RexRB(0,from,to);
	write8<I>( 0x09 ); 
	ModRM<I>( 3, from, to );
}

/* or r32 to m32 */
emitterT void eOR32RtoM(uptr to, x86IntRegType from ) 
{
	RexR(0,from);
	write8<I>( 0x09 ); 
	ModRM<I>( 0, from, DISP32 );
	write32<I>( MEMADDR(to, 4) );
}

/* or m32 to r32 */
emitterT void eOR32MtoR( x86IntRegType to, uptr from ) 
{
	RexR(0,to);
	write8<I>( 0x0B ); 
	ModRM<I>( 0, to, DISP32 );
	write32<I>( MEMADDR(from, 4) ); 
}

// or r16 to r16
emitterT void eOR16RtoR( x86IntRegType to, x86IntRegType from )
{
	write8<I>(0x66);
	RexRB(0,from,to);
	write8<I>( 0x09 ); 
	ModRM<I>( 3, from, to );
}

// or imm16 to r16
emitterT void eOR16ItoR( x86IntRegType to, u16 from )
{
	write8<I>(0x66);
	RexB(0,to);
	if ( to == EAX ) {
		write8<I>( 0x0D ); 
	}
	else {
		write8<I>( 0x81 ); 
		ModRM<I>( 3, 1, to );
	}
	write16<I>( from ); 
}

// or imm16 to m316
emitterT void eOR16ItoM( uptr to, u16 from )
{
	write8<I>(0x66);
	write8<I>( 0x81 ); 
	ModRM<I>( 0, 1, DISP32 );
	write32<I>( MEMADDR(to, 6) );
	write16<I>( from ); 
}

/* or m16 to r16 */
emitterT void eOR16MtoR( x86IntRegType to, uptr from ) 
{
	write8<I>(0x66);
	RexR(0,to);
	write8<I>( 0x0B ); 
	ModRM<I>( 0, to, DISP32 );
	write32<I>( MEMADDR(from, 4) ); 
}

// or r16 to m16 
emitterT void eOR16RtoM( uptr to, x86IntRegType from )
{
	write8<I>(0x66);
	RexR(0,from);
	write8<I>( 0x09 ); 
	ModRM<I>( 0, from, DISP32 );
	write32<I>( MEMADDR(to, 4) );
}

// or r8 to r8
emitterT void eOR8RtoR( x86IntRegType to, x86IntRegType from )
{
	RexRB(0,from,to);
	write8<I>( 0x08 ); 
	ModRM<I>( 3, from, to );
}

// or r8 to m8
emitterT void eOR8RtoM( uptr to, x86IntRegType from )
{
	RexR(0,from);
	write8<I>( 0x08 ); 
	ModRM<I>( 0, from, DISP32 );
	write32<I>( MEMADDR(to, 4) ); 
}

// or imm8 to m8
emitterT void eOR8ItoM( uptr to, u8 from )
{
	write8<I>( 0x80 ); 
	ModRM<I>( 0, 1, DISP32 );
	write32<I>( MEMADDR(to, 5) );
	write8<I>( from ); 
}

// or m8 to r8
emitterT void eOR8MtoR( x86IntRegType to, uptr from )
{
	RexR(0,to);
	write8<I>( 0x0A ); 
	ModRM<I>( 0, to, DISP32 );
	write32<I>( MEMADDR(from, 4) ); 
}

/* xor imm32 to r64 */
emitterT void eXOR64ItoR( x86IntRegType to, u32 from ) 
{
	RexB(1,to);
	if ( to == EAX ) {
		write8<I>( 0x35 ); 
	} else {
		write8<I>( 0x81 ); 
		ModRM<I>( 3, 6, to );
	}
	write32<I>( from ); 
}

/* xor r64 to r64 */
emitterT void eXOR64RtoR( x86IntRegType to, x86IntRegType from ) 
{
	RexRB(1, from, to);
	write8<I>( 0x31 ); 
	ModRM<I>( 3, from, to );
}

/* xor m64 to r64 */
emitterT void eXOR64MtoR( x86IntRegType to, uptr from ) 
{
	RexR(1, to);
	write8<I>( 0x33 ); 
	ModRM<I>( 0, to, DISP32 );
	write32<I>( MEMADDR(from, 4) ); 
}

/* xor r64 to m64 */
emitterT void eXOR64RtoM( uptr to, x86IntRegType from ) 
{
	RexR(1,from);
	write8<I>( 0x31 ); 
	ModRM<I>( 0, from, DISP32 );
	write32<I>( MEMADDR(to, 4) );
}

/* xor imm32 to r32 */
emitterT void eXOR32ItoR( x86IntRegType to, u32 from ) 
{
	RexB(0,to);
	if ( to == EAX ) {
		write8<I>( 0x35 ); 
	}
	else  {
		write8<I>( 0x81 ); 
		ModRM<I>( 3, 6, to );
	}
	write32<I>( from ); 
}

/* xor imm32 to m32 */
emitterT void eXOR32ItoM( uptr to, u32 from ) 
{
	write8<I>( 0x81 ); 
	ModRM<I>( 0, 6, DISP32 );
	write32<I>( MEMADDR(to, 8) ); 
	write32<I>( from ); 
}

/* xor r32 to r32 */
emitterT void eXOR32RtoR( x86IntRegType to, x86IntRegType from ) 
{
	RexRB(0,from,to);
	write8<I>( 0x31 ); 
	ModRM<I>( 3, from, to );
}

/* xor r16 to r16 */
emitterT void eXOR16RtoR( x86IntRegType to, x86IntRegType from ) 
{
	write8<I>( 0x66 );
	RexRB(0,from,to);
	write8<I>( 0x31 ); 
	ModRM<I>( 3, from, to );
}

/* xor r32 to m32 */
emitterT void eXOR32RtoM( uptr to, x86IntRegType from ) 
{
	RexR(0,from);
	write8<I>( 0x31 ); 
	ModRM<I>( 0, from, DISP32 );
	write32<I>( MEMADDR(to, 4) );
}

/* xor m32 to r32 */
emitterT void eXOR32MtoR( x86IntRegType to, uptr from ) 
{
	RexR(0,to);
	write8<I>( 0x33 ); 
	ModRM<I>( 0, to, DISP32 );
	write32<I>( MEMADDR(from, 4) ); 
}

// xor imm16 to r16
emitterT void eXOR16ItoR( x86IntRegType to, u16 from )
{
	write8<I>(0x66);
	RexB(0,to);
	if ( to == EAX ) {
		write8<I>( 0x35 ); 
	}
	else  {
		write8<I>( 0x81 ); 
		ModRM<I>( 3, 6, to );
	}
	write16<I>( from ); 
}

// xor r16 to m16
emitterT void eXOR16RtoM( uptr to, x86IntRegType from )
{
	write8<I>(0x66);
	RexR(0,from);
	write8<I>( 0x31 ); 
	ModRM<I>( 0, from, DISP32 );
	write32<I>( MEMADDR(to, 4) );
}

/* and imm32 to r64 */
emitterT void eAND64I32toR( x86IntRegType to, u32 from ) 
{
	RexB(1, to);
	if ( to == EAX ) {
		write8<I>( 0x25 ); 
	} else {
		write8<I>( 0x81 ); 
		ModRM<I>( 3, 0x4, to );
	}
	write32<I>( from ); 
}

/* and m64 to r64 */
emitterT void eAND64MtoR( x86IntRegType to, uptr from ) 
{
	RexR(1, to);
	write8<I>( 0x23 ); 
	ModRM<I>( 0, to, DISP32 );
	write32<I>( MEMADDR(from, 4) ); 
}

/* and r64 to m64 */
emitterT void eAND64RtoM( uptr to, x86IntRegType from ) 
{
	RexR(1, from);
	write8<I>( 0x21 ); 
	ModRM<I>( 0, from, DISP32 );
	write32<I>( MEMADDR(to, 4) ); 
}

/* and r64 to r64 */
emitterT void eAND64RtoR( x86IntRegType to, x86IntRegType from ) 
{
	RexRB(1, from, to);
	write8<I>( 0x21 ); 
	ModRM<I>( 3, from, to );
}

/* and imm32 to m64 */
emitterT void eAND64I32toM( uptr to, u32 from ) 
{
	Rex(1,0,0,0);
	write8<I>( 0x81 ); 
	ModRM<I>( 0, 0x4, DISP32 );
	write32<I>( MEMADDR(to, 8) );
	write32<I>( from ); 
}

/* and imm32 to r32 */
emitterT void eAND32ItoR( x86IntRegType to, u32 from ) 
{
	RexB(0,to);
	if(from < 0x80) {
		eAND32I8toR<I>(to, (u8)from);
	}
	else {
		if ( to == EAX ) {
			write8<I>( 0x25 ); 
		} 
		else {
			write8<I>( 0x81 ); 
			ModRM<I>( 3, 0x4, to );
		}
		write32<I>( from );
	}
}

/* and sign ext imm8 to r32 */
emitterT void eAND32I8toR( x86IntRegType to, u8 from ) 
{
	RexB(0,to);
	write8<I>( 0x83 ); 
	ModRM<I>( 3, 0x4, to );
	write8<I>( from ); 
}

/* and imm32 to m32 */
emitterT void eAND32ItoM( uptr to, u32 from ) 
{
	if(from < 0x80) {
		eAND32I8toM<I>(to, (u8)from);
	}
	else {
		write8<I>( 0x81 ); 
		ModRM<I>( 0, 0x4, DISP32 );
		write32<I>( MEMADDR(to, 8) );
		write32<I>( from ); 
	}
}


/* and sign ext imm8 to m32 */
emitterT void eAND32I8toM( uptr to, u8 from ) 
{
	write8<I>( 0x83 ); 
	ModRM<I>( 0, 0x4, DISP32 );
	write32<I>( MEMADDR(to, 5) );
	write8<I>( from ); 
}

/* and r32 to r32 */
emitterT void eAND32RtoR( x86IntRegType to, x86IntRegType from ) 
{
	RexRB(0,from,to);
	write8<I>( 0x21 ); 
	ModRM<I>( 3, from, to );
}

/* and r32 to m32 */
emitterT void eAND32RtoM( uptr to, x86IntRegType from ) 
{
	RexR(0,from);
	write8<I>( 0x21 ); 
	ModRM<I>( 0, from, DISP32 );
	write32<I>( MEMADDR(to, 4) ); 
}

/* and m32 to r32 */
emitterT void eAND32MtoR( x86IntRegType to, uptr from ) 
{
	RexR(0,to);
	write8<I>( 0x23 ); 
	ModRM<I>( 0, to, DISP32 );
	write32<I>( MEMADDR(from, 4) ); 
}

// Warning: Untested form of AND.
emitterT void eAND32RmtoR( x86IntRegType to, x86IntRegType from )
{
	RexRB(0,to,from);
	write8<I>( 0x23 ); 
	ModRM<I>( 0, to, from ); 
}

// Warning: Untested form of AND.
emitterT void eAND32RmtoROffset( x86IntRegType to, x86IntRegType from, int offset )
{
	RexRB(0,to,from);
	write8<I>( 0x23 );
	WriteRmOffsetFrom<I>(to,from,offset);
}

// and r16 to r16
emitterT void eAND16RtoR( x86IntRegType to, x86IntRegType from )
{
	write8<I>(0x66);
	RexRB(0,to,from);
	write8<I>( 0x23 ); 
	ModRM<I>( 3, to, from );
}

/* and imm16 to r16 */
emitterT void eAND16ItoR( x86IntRegType to, u16 from ) 
{
	write8<I>(0x66);
	RexB(0,to);

	if ( to == EAX ) {
		write8<I>( 0x25 ); 
		write16<I>( from ); 
	} 
	else if ( from < 0x80 ) {
		write8<I>( 0x83 ); 
		ModRM<I>( 3, 0x4, to );
		write8<I>((u8)from ); 
	} 
	else {
		write8<I>( 0x81 ); 
		ModRM<I>( 3, 0x4, to );
		write16<I>( from ); 
	}
}

/* and imm16 to m16 */
emitterT void eAND16ItoM( uptr to, u16 from ) 
{
	write8<I>(0x66);
	if ( from < 0x80 ) {
		write8<I>( 0x83 ); 
		ModRM<I>( 0, 0x4, DISP32 );
		write32<I>( MEMADDR(to, 6) );
		write8<I>((u8)from );
	} 
	else 
	{
		write8<I>( 0x81 );
		ModRM<I>( 0, 0x4, DISP32 );
		write32<I>( MEMADDR(to, 6) );
		write16<I>( from );

	}
}

/* and r16 to m16 */
emitterT void eAND16RtoM( uptr to, x86IntRegType from ) 
{
	write8<I>( 0x66 );
	RexR(0,from);
	write8<I>( 0x21 ); 
	ModRM<I>( 0, from, DISP32 );
	write32<I>( MEMADDR(to, 4) ); 
}

/* and m16 to r16 */
emitterT void eAND16MtoR( x86IntRegType to, uptr from ) 
{
	write8<I>( 0x66 );
	RexR(0,to);
	write8<I>( 0x23 ); 
	ModRM<I>( 0, to, DISP32 );
	write32<I>( MEMADDR(from, 4)); 
}

/* and imm8 to r8 */
emitterT void eAND8ItoR( x86IntRegType to, u8 from ) 
{
	RexB(0,to);
	if ( to == EAX ) {
		write8<I>( 0x24 ); 
	} 
	else {
		write8<I>( 0x80 ); 
		ModRM<I>( 3, 0x4, to );
	}
	write8<I>( from ); 
}

/* and imm8 to m8 */
emitterT void eAND8ItoM( uptr to, u8 from ) 
{
	write8<I>( 0x80 ); 
	ModRM<I>( 0, 0x4, DISP32 );
	write32<I>( MEMADDR(to, 5) );
	write8<I>( from ); 
}

// and r8 to r8
emitterT void eAND8RtoR( x86IntRegType to, x86IntRegType from )
{
	RexRB(0,to,from);
	write8<I>( 0x22 ); 
	ModRM<I>( 3, to, from );
}

/* and r8 to m8 */
emitterT void eAND8RtoM( uptr to, x86IntRegType from ) 
{
	RexR(0,from);
	write8<I>( 0x20 ); 
	ModRM<I>( 0, from, DISP32 );
	write32<I>( MEMADDR(to, 4) ); 
}

/* and m8 to r8 */
emitterT void eAND8MtoR( x86IntRegType to, uptr from ) 
{
	RexR(0,to);
	write8<I>( 0x22 ); 
	ModRM<I>( 0, to, DISP32 );
	write32<I>( MEMADDR(from, 4)); 
}

/* not r64 */
emitterT void eNOT64R( x86IntRegType from ) 
{
	RexB(1, from);
	write8<I>( 0xF7 ); 
	ModRM<I>( 3, 2, from );
}

/* not r32 */
emitterT void eNOT32R( x86IntRegType from ) 
{
	RexB(0,from);
	write8<I>( 0xF7 ); 
	ModRM<I>( 3, 2, from );
}

// not m32 
emitterT void eNOT32M( u32 from )
{
	write8<I>( 0xF7 ); 
	ModRM<I>( 0, 2, DISP32 );
	write32<I>( MEMADDR(from, 4)); 
}

/* neg r64 */
emitterT void eNEG64R( x86IntRegType from ) 
{
	RexB(1, from);
	write8<I>( 0xF7 ); 
	ModRM<I>( 3, 3, from );
}

/* neg r32 */
emitterT void eNEG32R( x86IntRegType from ) 
{
	RexB(0,from);
	write8<I>( 0xF7 ); 
	ModRM<I>( 3, 3, from );
}

emitterT void eNEG32M( u32 from )
{
	write8<I>( 0xF7 ); 
	ModRM<I>( 0, 3, DISP32 );
	write32<I>( MEMADDR(from, 4)); 
}

/* neg r16 */
emitterT void eNEG16R( x86IntRegType from ) 
{
	write8<I>( 0x66 ); 
	RexB(0,from);
	write8<I>( 0xF7 ); 
	ModRM<I>( 3, 3, from );
}

////////////////////////////////////
// jump instructions				/
////////////////////////////////////

emitterT u8* JMP( uptr to ) {
	uptr jump = ( x86Ptr[0] - (u8*)to ) - 1;

	if ( jump > 0x7f ) {
		assert( to <= 0xffffffff );
		return (u8*)eJMP32<I>( to );
	} 
	else {
		return (u8*)eJMP8<I>( to );
	}
}

/* jmp rel8 */
emitterT u8* eJMP8( u8 to ) 
{
	write8<I>( 0xEB ); 
	write8<I>( to );
	return x86Ptr[I] - 1;
}

/* jmp rel32 */
emitterT u32* eJMP32( uptr to ) 
{
	assert( (sptr)to <= 0x7fffffff && (sptr)to >= -0x7fffffff );
	write8<I>( 0xE9 ); 
	write32<I>( to ); 
	return (u32*)(x86Ptr[I] - 4 );
}

/* jmp r32/r64 */
emitterT void eJMPR( x86IntRegType to ) 
{
	RexB(0, to);
	write8<I>( 0xFF ); 
	ModRM<I>( 3, 4, to );
}

// jmp m32 
emitterT void eJMP32M( uptr to )
{
	write8<I>( 0xFF ); 
	ModRM<I>( 0, 4, DISP32 );
	write32<I>( MEMADDR(to, 4)); 
}

/* jp rel8 */
emitterT u8* eJP8( u8 to ) {
	return J8Rel<I>( 0x7A, to );
}

/* jnp rel8 */
emitterT u8* eJNP8( u8 to ) {
	return J8Rel<I>( 0x7B, to );
}

/* je rel8 */
emitterT u8* eJE8( u8 to ) {
	return J8Rel<I>( 0x74, to );
}

/* jz rel8 */
emitterT u8* eJZ8( u8 to ) 
{
	return J8Rel<I>( 0x74, to ); 
}

/* js rel8 */
emitterT u8* eJS8( u8 to ) 
{ 
	return J8Rel<I>( 0x78, to );
}

/* jns rel8 */
emitterT u8* eJNS8( u8 to ) 
{ 
	return J8Rel<I>( 0x79, to );
}

/* jg rel8 */
emitterT u8* eJG8( u8 to ) 
{ 
	return J8Rel<I>( 0x7F, to );
}

/* jge rel8 */
emitterT u8* eJGE8( u8 to ) 
{ 
	return J8Rel<I>( 0x7D, to ); 
}

/* jl rel8 */
emitterT u8* eJL8( u8 to ) 
{ 
	return J8Rel<I>( 0x7C, to ); 
}

/* ja rel8 */
emitterT u8* eJA8( u8 to ) 
{ 
	return J8Rel<I>( 0x77, to ); 
}

emitterT u8* eJAE8( u8 to ) 
{ 
	return J8Rel<I>( 0x73, to ); 
}

/* jb rel8 */
emitterT u8* eJB8( u8 to ) 
{ 
	return J8Rel<I>( 0x72, to ); 
}

/* jbe rel8 */
emitterT u8* eJBE8( u8 to ) 
{ 
	return J8Rel<I>( 0x76, to ); 
}

/* jle rel8 */
emitterT u8* eJLE8( u8 to ) 
{ 
	return J8Rel<I>( 0x7E, to ); 
}

/* jne rel8 */
emitterT u8* eJNE8( u8 to ) 
{ 
	return J8Rel<I>( 0x75, to ); 
}

/* jnz rel8 */
emitterT u8* eJNZ8( u8 to ) 
{ 
	return J8Rel<I>( 0x75, to ); 
}

/* jng rel8 */
emitterT u8* eJNG8( u8 to ) 
{ 
	return J8Rel<I>( 0x7E, to ); 
}

/* jnge rel8 */
emitterT u8* eJNGE8( u8 to ) 
{ 
	return J8Rel<I>( 0x7C, to ); 
}

/* jnl rel8 */
emitterT u8* eJNL8( u8 to ) 
{ 
	return J8Rel<I>( 0x7D, to ); 
}

/* jnle rel8 */
emitterT u8* eJNLE8( u8 to ) 
{ 
	return J8Rel<I>( 0x7F, to ); 
}

/* jo rel8 */
emitterT u8* eJO8( u8 to ) 
{ 
	return J8Rel<I>( 0x70, to ); 
}

/* jno rel8 */
emitterT u8* eJNO8( u8 to ) 
{ 
	return J8Rel<I>( 0x71, to ); 
}
/* Untested and slower, use 32bit versions instead
// ja rel16 
emitterT u16* eJA16( u16 to )
{
return J16Rel<I>( 0x87, to );
}

// jb rel16 
emitterT u16* eJB16( u16 to )
{
return J16Rel<I>( 0x82, to );
}

// je rel16 
emitterT u16* eJE16( u16 to )
{
return J16Rel<I>( 0x84, to );
}

// jz rel16 
emitterT u16* eJZ16( u16 to )
{
return J16Rel<I>( 0x84, to );
}
*/
// jb rel32 
emitterT u32* eJB32( u32 to )
{
	return J32Rel<I>( 0x82, to );
}

/* je rel32 */
emitterT u32* eJE32( u32 to ) 
{
	return J32Rel<I>( 0x84, to );
}

/* jz rel32 */
emitterT u32* eJZ32( u32 to ) 
{
	return J32Rel<I>( 0x84, to ); 
}

/* js rel32 */
emitterT u32* eJS32( u32 to ) 
{ 
	return J32Rel<I>( 0x88, to );
}

/* jns rel32 */
emitterT u32* eJNS32( u32 to ) 
{ 
	return J32Rel<I>( 0x89, to );
}

/* jg rel32 */
emitterT u32* eJG32( u32 to ) 
{ 
	return J32Rel<I>( 0x8F, to );
}

/* jge rel32 */
emitterT u32* eJGE32( u32 to ) 
{ 
	return J32Rel<I>( 0x8D, to ); 
}

/* jl rel32 */
emitterT u32* eJL32( u32 to ) 
{ 
	return J32Rel<I>( 0x8C, to ); 
}

/* jle rel32 */
emitterT u32* eJLE32( u32 to ) 
{ 
	return J32Rel<I>( 0x8E, to ); 
}

/* ja rel32 */
emitterT u32* eJA32( u32 to ) 
{ 
	return J32Rel<I>( 0x87, to ); 
}

/* jae rel32 */
emitterT u32* eJAE32( u32 to ) 
{ 
	return J32Rel<I>( 0x83, to ); 
}

/* jne rel32 */
emitterT u32* eJNE32( u32 to ) 
{ 
	return J32Rel<I>( 0x85, to ); 
}

/* jnz rel32 */
emitterT u32* eJNZ32( u32 to ) 
{ 
	return J32Rel<I>( 0x85, to ); 
}

/* jng rel32 */
emitterT u32* eJNG32( u32 to ) 
{ 
	return J32Rel<I>( 0x8E, to ); 
}

/* jnge rel32 */
emitterT u32* eJNGE32( u32 to ) 
{ 
	return J32Rel<I>( 0x8C, to ); 
}

/* jnl rel32 */
emitterT u32* eJNL32( u32 to ) 
{ 
	return J32Rel<I>( 0x8D, to ); 
}

/* jnle rel32 */
emitterT u32* eJNLE32( u32 to ) 
{ 
	return J32Rel<I>( 0x8F, to ); 
}

/* jo rel32 */
emitterT u32* eJO32( u32 to ) 
{ 
	return J32Rel<I>( 0x80, to ); 
}

/* jno rel32 */
emitterT u32* eJNO32( u32 to ) 
{ 
	return J32Rel<I>( 0x81, to ); 
}



/* call func */
emitterT void eCALLFunc( uptr func ) 
{
	func -= ( (uptr)x86Ptr[0] + 5 );
	assert( (sptr)func <= 0x7fffffff && (sptr)func >= -0x7fffffff );
	eCALL32<I>(func);
}

/* call rel32 */
emitterT void eCALL32( u32 to )
{
	write8<I>( 0xE8 ); 
	write32<I>( to ); 
}

/* call r32 */
emitterT void eCALL32R( x86IntRegType to ) 
{
	write8<I>( 0xFF );
	ModRM<I>( 3, 2, to );
}

/* call r64 */
emitterT void eCALL64R( x86IntRegType to ) 
{
	RexB(0, to);
	write8<I>( 0xFF );
	ModRM<I>( 3, 2, to );
}

/* call m32 */
emitterT void eCALL32M( u32 to ) 
{
	write8<I>( 0xFF );
	ModRM<I>( 0, 2, DISP32 );
	write32<I>( MEMADDR(to, 4) );
}

////////////////////////////////////
// misc instructions				/
////////////////////////////////////

/* cmp imm32 to r64 */
emitterT void eCMP64I32toR( x86IntRegType to, u32 from ) 
{
	RexB(1, to);
	if ( to == EAX ) {
		write8<I>( 0x3D );
	} 
	else {
		write8<I>( 0x81 );
		ModRM<I>( 3, 7, to );
	}
	write32<I>( from ); 
}

/* cmp m64 to r64 */
emitterT void eCMP64MtoR( x86IntRegType to, uptr from ) 
{
	RexR(1, to);
	write8<I>( 0x3B );
	ModRM<I>( 0, to, DISP32 );
	write32<I>( MEMADDR(from, 4) ); 
}

// cmp r64 to r64 
emitterT void eCMP64RtoR( x86IntRegType to, x86IntRegType from )
{
	RexRB(1,from,to);
	write8<I>( 0x39 );
	ModRM<I>( 3, from, to );
}

/* cmp imm32 to r32 */
emitterT void eCMP32ItoR( x86IntRegType to, u32 from ) 
{
	RexB(0,to);
	if ( to == EAX ) {
		write8<I>( 0x3D );
	} 
	else {
		write8<I>( 0x81 );
		ModRM<I>( 3, 7, to );
	}
	write32<I>( from ); 
}

/* cmp imm32 to m32 */
emitterT void eCMP32ItoM( uptr to, u32 from ) 
{
	write8<I>( 0x81 ); 
	ModRM<I>( 0, 7, DISP32 );
	write32<I>( MEMADDR(to, 8) ); 
	write32<I>( from ); 
}

/* cmp r32 to r32 */
emitterT void eCMP32RtoR( x86IntRegType to, x86IntRegType from ) 
{
	RexRB(0,from,to);
	write8<I>( 0x39 );
	ModRM<I>( 3, from, to );
}

/* cmp m32 to r32 */
emitterT void eCMP32MtoR( x86IntRegType to, uptr from ) 
{
	RexR(0,to);
	write8<I>( 0x3B );
	ModRM<I>( 0, to, DISP32 );
	write32<I>( MEMADDR(from, 4) ); 
}

// cmp imm8 to [r32]
emitterT void eCMP32I8toRm( x86IntRegType to, u8 from)
{
	RexB(0,to);
	write8<I>( 0x83 );
	ModRM<I>( 0, 7, to );
	write8<I>(from);
}

// cmp imm32 to [r32+off]
emitterT void eCMP32I8toRmOffset8( x86IntRegType to, u8 from, u8 off)
{
	RexB(0,to);
	write8<I>( 0x83 );
	ModRM<I>( 1, 7, to );
	write8<I>(off);
	write8<I>(from);
}

// cmp imm8 to [r32]
emitterT void eCMP32I8toM( uptr to, u8 from)
{
	write8<I>( 0x83 );
	ModRM<I>( 0, 7, DISP32 );
	write32<I>( MEMADDR(to, 5) ); 
	write8<I>( from ); 
}

/* cmp imm16 to r16 */
emitterT void eCMP16ItoR( x86IntRegType to, u16 from ) 
{
	write8<I>( 0x66 ); 
	RexB(0,to);
	if ( to == EAX )
	{
		write8<I>( 0x3D );
	} 
	else 
	{
		write8<I>( 0x81 );
		ModRM<I>( 3, 7, to );
	}
	write16<I>( from ); 
}

/* cmp imm16 to m16 */
emitterT void eCMP16ItoM( uptr to, u16 from ) 
{
	write8<I>( 0x66 ); 
	write8<I>( 0x81 ); 
	ModRM<I>( 0, 7, DISP32 );
	write32<I>( MEMADDR(to, 6) ); 
	write16<I>( from ); 
}

/* cmp r16 to r16 */
emitterT void eCMP16RtoR( x86IntRegType to, x86IntRegType from ) 
{
	write8<I>( 0x66 ); 
	RexRB(0,from,to);
	write8<I>( 0x39 );
	ModRM<I>( 3, from, to );
}

/* cmp m16 to r16 */
emitterT void eCMP16MtoR( x86IntRegType to, uptr from ) 
{
	write8<I>( 0x66 ); 
	RexR(0,to);
	write8<I>( 0x3B );
	ModRM<I>( 0, to, DISP32 );
	write32<I>( MEMADDR(from, 4) ); 
}

// cmp imm8 to r8
emitterT void eCMP8ItoR( x86IntRegType to, u8 from )
{
	RexB(0,to);
	if ( to == EAX )
	{
		write8<I>( 0x3C );
	} 
	else 
	{
		write8<I>( 0x80 );
		ModRM<I>( 3, 7, to );
	}
	write8<I>( from ); 
}

// cmp m8 to r8
emitterT void eCMP8MtoR( x86IntRegType to, uptr from )
{
	RexR(0,to);
	write8<I>( 0x3A );
	ModRM<I>( 0, to, DISP32 );
	write32<I>( MEMADDR(from, 4) ); 
}

/* test imm32 to r32 */
emitterT void eTEST32ItoR( x86IntRegType to, u32 from ) 
{
	RexB(0,to);
	if ( to == EAX )
	{
		write8<I>( 0xA9 );
	} 
	else 
	{
		write8<I>( 0xF7 );
		ModRM<I>( 3, 0, to );
	}
	write32<I>( from ); 
}

emitterT void eTEST32ItoM( uptr to, u32 from )
{
	write8<I>( 0xF7 );
	ModRM<I>( 0, 0, DISP32 );
	write32<I>( MEMADDR(to, 8) );
	write32<I>( from );
}

/* test r32 to r32 */
emitterT void eTEST32RtoR( x86IntRegType to, x86IntRegType from ) 
{
	RexRB(0,from,to);
	write8<I>( 0x85 );
	ModRM<I>( 3, from, to );
}

// test imm32 to [r32]
emitterT void eTEST32ItoRm( x86IntRegType to, u32 from )
{
	RexB(0,to);
	write8<I>( 0xF7 );
	ModRM<I>( 0, 0, to );
	write32<I>(from);
}

// test imm16 to r16
emitterT void eTEST16ItoR( x86IntRegType to, u16 from )
{
	write8<I>(0x66);
	RexB(0,to);
	if ( to == EAX )
	{
		write8<I>( 0xA9 );
	} 
	else 
	{
		write8<I>( 0xF7 );
		ModRM<I>( 3, 0, to );
	}
	write16<I>( from ); 
}

// test r16 to r16
emitterT void eTEST16RtoR( x86IntRegType to, x86IntRegType from )
{
	write8<I>(0x66);
	RexRB(0,from,to);
	write8<I>( 0x85 );
	ModRM<I>( 3, from, to );
}

// test r8 to r8
emitterT void eTEST8RtoR( x86IntRegType to, x86IntRegType from )
{
	RexRB(0, from, to);
	write8<I>( 0x84 );
	ModRM<I>( 3, from, to );
}


// test imm8 to r8
emitterT void eTEST8ItoR( x86IntRegType to, u8 from )
{
	RexB(0,to);
	if ( to == EAX )
	{
		write8<I>( 0xA8 );
	} 
	else 
	{
		write8<I>( 0xF6 );
		ModRM<I>( 3, 0, to );
	}
	write8<I>( from ); 
}

// test imm8 to r8
emitterT void eTEST8ItoM( uptr to, u8 from )
{
	write8<I>( 0xF6 );
	ModRM<I>( 0, 0, DISP32 );
	write32<I>( MEMADDR(to, 5) );
	write8<I>( from );
}

/* sets r8 */
emitterT void eSETS8R( x86IntRegType to ) 
{ 
	SET8R<I>( 0x98, to ); 
}

/* setl r8 */
emitterT void eSETL8R( x86IntRegType to ) 
{ 
	SET8R<I>( 0x9C, to ); 
}

// setge r8 
emitterT void eSETGE8R( x86IntRegType to ) { SET8R<I>(0x9d, to); }
// setg r8 
emitterT void eSETG8R( x86IntRegType to ) { SET8R<I>(0x9f, to); }
// seta r8 
emitterT void eSETA8R( x86IntRegType to ) { SET8R<I>(0x97, to); }
// setae r8 
emitterT void eSETAE8R( x86IntRegType to ) { SET8R<I>(0x99, to); }
/* setb r8 */
emitterT void eSETB8R( x86IntRegType to ) { SET8R<I>( 0x92, to ); }
/* setb r8 */
emitterT void eSETNZ8R( x86IntRegType to )  { SET8R<I>( 0x95, to ); }
// setz r8 
emitterT void eSETZ8R( x86IntRegType to ) { SET8R<I>(0x94, to); }
// sete r8 
emitterT void eSETE8R( x86IntRegType to ) { SET8R<I>(0x94, to); }

/* push imm32 */
emitterT void ePUSH32I( u32 from ) 
{;
write8<I>( 0x68 ); 
write32<I>( from ); 
}

/* push r32 */
emitterT void ePUSH32R( x86IntRegType from )  { write8<I>( 0x50 | from ); }

/* push m32 */
emitterT void ePUSH32M( u32 from ) 
{
	write8<I>( 0xFF );
	ModRM<I>( 0, 6, DISP32 );
	write32<I>( MEMADDR(from, 4) ); 
}

/* pop r32 */
emitterT void ePOP32R( x86IntRegType from ) { write8<I>( 0x58 | from ); }

/* pushad */
emitterT void ePUSHA32( void ) { write8<I>( 0x60 ); }

/* popad */
emitterT void ePOPA32( void ) { write8<I>( 0x61 ); }

emitterT void ePUSHR(x86IntRegType from) { ePUSH32R<I>(from); }
emitterT void ePOPR(x86IntRegType from) { ePOP32R<I>(from); }


/* pushfd */
emitterT void ePUSHFD( void ) { write8<I>( 0x9C ); }
/* popfd */
emitterT void ePOPFD( void ) { write8<I>( 0x9D ); }

emitterT void eRET( void ) { /*write8<I>( 0xf3 );  <-- K8 opt?*/ write8<I>( 0xC3 ); }

emitterT void eCBW( void ) { write16<I>( 0x9866 );  }
emitterT void eCWD( void )  { write8<I>( 0x98 ); }
emitterT void eCDQ( void ) { write8<I>( 0x99 ); }
emitterT void eCWDE() { write8<I>(0x98); }

emitterT void eLAHF() { write8<I>(0x9f); }
emitterT void eSAHF() { write8<I>(0x9e); }

emitterT void eBT32ItoR( x86IntRegType to, u8 from ) 
{
	write16<I>( 0xBA0F );
	ModRM<I>(3, 4, to);
	write8<I>( from );
}

emitterT void eBTR32ItoR( x86IntRegType to, u8 from ) 
{
	write16<I>( 0xBA0F );
	ModRM<I>(3, 6, to);
	write8<I>( from );
}

emitterT void eBSRRtoR(x86IntRegType to, x86IntRegType from)
{
	write16<I>( 0xBD0F );
	ModRM<I>( 3, from, to );
}

emitterT void eBSWAP32R( x86IntRegType to ) 
{
	write8<I>( 0x0F );
	write8<I>( 0xC8 + to );
}

// to = from + offset
emitterT void eLEA16RtoR(x86IntRegType to, x86IntRegType from, u16 offset)
{
	write8<I>(0x66);
	eLEA32RtoR<I>(to, from, offset);
}

emitterT void eLEA32RtoR(x86IntRegType to, x86IntRegType from, s32 offset)
{
	RexRB(0,to,from);
	write8<I>(0x8d);

	if( (from&7) == ESP ) {
		if( offset == 0 ) {
			ModRM<I>(1, to, from);
			write8<I>(0x24);
		}
		else if( offset <= 127 && offset >= -128 ) {
			ModRM<I>(1, to, from);
			write8<I>(0x24);
			write8<I>(offset);
		}
		else {
			ModRM<I>(2, to, from);
			write8<I>(0x24);
			write32<I>(offset);
		}
	}
	else {
		if( offset == 0 && from != EBP && from!=ESP ) {
			ModRM<I>(0, to, from);
		}
		else if( offset <= 127 && offset >= -128 ) {
			ModRM<I>(1, to, from);
			write8<I>(offset);
		}
		else {
			ModRM<I>(2, to, from);
			write32<I>(offset);
		}
	}
}

// to = from0 + from1
emitterT void eLEA16RRtoR(x86IntRegType to, x86IntRegType from0, x86IntRegType from1)
{
	write8<I>(0x66);
	eLEA32RRtoR<I>(to, from0, from1);
}

emitterT void eLEA32RRtoR(x86IntRegType to, x86IntRegType from0, x86IntRegType from1)
{ 
	RexRXB(0, to, from0, from1);
	write8<I>(0x8d);

	if( (from1&7) == EBP ) {
		ModRM<I>(1, to, 4);
		ModRM<I>(0, from0, from1);
		write8<I>(0);
	}
	else {
		ModRM<I>(0, to, 4);
		ModRM<I>(0, from0, from1);
	}
}

// to = from << scale (max is 3)
emitterT void eLEA16RStoR(x86IntRegType to, x86IntRegType from, u32 scale)
{
	write8<I>(0x66);
	eLEA32RStoR<I>(to, from, scale);
}

// Don't inline recursive functions
emitterT void eLEA32RStoR(x86IntRegType to, x86IntRegType from, u32 scale)
{
	if( to == from ) {
		eSHL32ItoR<I>(to, scale);
		return;
	}

	if( from != ESP ) {
		RexRXB(0,to,from,0);
		write8<I>(0x8d);
		ModRM<I>(0, to, 4);
		ModRM<I>(scale, from, 5);
		write32<I>(0);
	}
	else {
		assert( to != ESP );
		eMOV32RtoR<I>(to, from);
		eLEA32RStoR<I>(to, to, scale);
	}
}
