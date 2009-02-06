/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2008  Pcsx2 Team
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
#include "ix86.h"

/********************/
/* FPU instructions */
/********************/

/* fild m32 to fpu reg stack */
void FILD32( u32 from )
{
	write8( 0xDB );
	ModRM( 0, 0x0, DISP32 );
	write32( MEMADDR(from, 4) ); 
}

/* fistp m32 from fpu reg stack */
void FISTP32( u32 from ) 
{
	write8( 0xDB );
	ModRM( 0, 0x3, DISP32 );
	write32( MEMADDR(from, 4) ); 
}

/* fld m32 to fpu reg stack */
void FLD32( u32 from )
{
	write8( 0xD9 );
	ModRM( 0, 0x0, DISP32 );
	write32( MEMADDR(from, 4) ); 
}

// fld st(i)
void FLD(int st) { write16(0xc0d9+(st<<8)); }

void FLD1() { write16(0xe8d9); }
void FLDL2E() { write16(0xead9); }

/* fst m32 from fpu reg stack */
void FST32( u32 to ) 
{
   write8( 0xD9 );
   ModRM( 0, 0x2, DISP32 );
   write32( MEMADDR(to, 4) ); 
}

/* fstp m32 from fpu reg stack */
void FSTP32( u32 to )
{
	write8( 0xD9 );
	ModRM( 0, 0x3, DISP32 );
	write32( MEMADDR(to, 4) ); 
}

// fstp st(i)
void FSTP(int st) { write16(0xd8dd+(st<<8)); }

/* fldcw fpu control word from m16 */
void FLDCW( u32 from )
{
	write8( 0xD9 );
	ModRM( 0, 0x5, DISP32 );
	write32( MEMADDR(from, 4) ); 
}

/* fnstcw fpu control word to m16 */
void FNSTCW( u32 to ) 
{
	write8( 0xD9 );
	ModRM( 0, 0x7, DISP32 );
	write32( MEMADDR(to, 4) ); 
}

void FNSTSWtoAX( void ) 
{
	write16( 0xE0DF );
}

void FXAM()
{
	write16(0xe5d9);
}

void FDECSTP() { write16(0xf6d9); }
void FRNDINT() { write16(0xfcd9); }
void FXCH(int st) { write16(0xc8d9+(st<<8)); }
void F2XM1() { write16(0xf0d9); }
void FSCALE() { write16(0xfdd9); }

/* fadd ST(src) to fpu reg stack ST(0) */
void FADD32Rto0( x86IntRegType src )
{
   write8( 0xD8 );
   write8( 0xC0 + src );
}

/* fadd ST(0) to fpu reg stack ST(src) */
void FADD320toR( x86IntRegType src )
{
   write8( 0xDC );
   write8( 0xC0 + src );
}

/* fsub ST(src) to fpu reg stack ST(0) */
void FSUB32Rto0( x86IntRegType src )
{
   write8( 0xD8 );
   write8( 0xE0 + src );
}

/* fsub ST(0) to fpu reg stack ST(src) */
void FSUB320toR( x86IntRegType src )
{
   write8( 0xDC );
   write8( 0xE8 + src );
}

/* fsubp -> substract ST(0) from ST(1), store in ST(1) and POP stack */
void FSUBP( void )
{
   write8( 0xDE );
   write8( 0xE9 );
}

/* fmul ST(src) to fpu reg stack ST(0) */
void FMUL32Rto0( x86IntRegType src )
{
   write8( 0xD8 );
   write8( 0xC8 + src );
}

/* fmul ST(0) to fpu reg stack ST(src) */
void FMUL320toR( x86IntRegType src )
{
   write8( 0xDC );
   write8( 0xC8 + src );
}

/* fdiv ST(src) to fpu reg stack ST(0) */
void FDIV32Rto0( x86IntRegType src )
{
   write8( 0xD8 );
   write8( 0xF0 + src );
}

/* fdiv ST(0) to fpu reg stack ST(src) */
void FDIV320toR( x86IntRegType src )
{
   write8( 0xDC );
   write8( 0xF8 + src );
}

void FDIV320toRP( x86IntRegType src )
{
	write8( 0xDE );
	write8( 0xF8 + src );
}

/* fadd m32 to fpu reg stack */
void FADD32( u32 from ) 
{
	write8( 0xD8 );
	ModRM( 0, 0x0, DISP32 );
	write32( MEMADDR(from, 4) ); 
}

/* fsub m32 to fpu reg stack */
void FSUB32( u32 from ) 
{
	write8( 0xD8 );
	ModRM( 0, 0x4, DISP32 );
	write32( MEMADDR(from, 4) ); 
}

/* fmul m32 to fpu reg stack */
void FMUL32( u32 from )
{
	write8( 0xD8 );
	ModRM( 0, 0x1, DISP32 );
	write32( MEMADDR(from, 4) ); 
}

/* fdiv m32 to fpu reg stack */
void FDIV32( u32 from ) 
{
	write8( 0xD8 );
	ModRM( 0, 0x6, DISP32 );
	write32( MEMADDR(from, 4) ); 
}

/* fabs fpu reg stack */
void FABS( void )
{
	write16( 0xE1D9 );
}

/* fsqrt fpu reg stack */
void FSQRT( void ) 
{
	write16( 0xFAD9 );
}

void FPATAN(void) { write16(0xf3d9); }
void FSIN(void) { write16(0xfed9); }

/* fchs fpu reg stack */
void FCHS( void ) 
{
	write16( 0xE0D9 );
}

/* fcomi st, st(i) */
void FCOMI( x86IntRegType src )
{
	write8( 0xDB );
	write8( 0xF0 + src ); 
}

/* fcomip st, st(i) */
void FCOMIP( x86IntRegType src )
{
	write8( 0xDF );
	write8( 0xF0 + src ); 
}

/* fucomi st, st(i) */
void FUCOMI( x86IntRegType src )
{
	write8( 0xDB );
	write8( 0xE8 + src ); 
}

/* fucomip st, st(i) */
void FUCOMIP( x86IntRegType src )
{
	write8( 0xDF );
	write8( 0xE8 + src ); 
}

/* fcom m32 to fpu reg stack */
void FCOM32( u32 from ) 
{
	write8( 0xD8 );
	ModRM( 0, 0x2, DISP32 );
	write32( MEMADDR(from, 4) ); 
}

/* fcomp m32 to fpu reg stack */
void FCOMP32( u32 from )
{
	write8( 0xD8 );
	ModRM( 0, 0x3, DISP32 );
	write32( MEMADDR(from, 4) ); 
}

#define FCMOV32( low, high ) \
   { \
	   write8( low ); \
	   write8( high + from );  \
   }

void FCMOVB32( x86IntRegType from )     { FCMOV32( 0xDA, 0xC0 ); }
void FCMOVE32( x86IntRegType from )     { FCMOV32( 0xDA, 0xC8 ); }
void FCMOVBE32( x86IntRegType from )    { FCMOV32( 0xDA, 0xD0 ); }
void FCMOVU32( x86IntRegType from )     { FCMOV32( 0xDA, 0xD8 ); }
void FCMOVNB32( x86IntRegType from )    { FCMOV32( 0xDB, 0xC0 ); }
void FCMOVNE32( x86IntRegType from )    { FCMOV32( 0xDB, 0xC8 ); }
void FCMOVNBE32( x86IntRegType from )   { FCMOV32( 0xDB, 0xD0 ); }
void FCMOVNU32( x86IntRegType from )    { FCMOV32( 0xDB, 0xD8 ); }
