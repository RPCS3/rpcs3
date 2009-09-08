/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2009  PCSX2 Dev Team
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

#include "PrecompiledHeader.h"
#include "legacy_internal.h"

//------------------------------------------------------------------
// FPU instructions
//------------------------------------------------------------------

/* fild m32 to fpu reg stack */
emitterT void FILD32( u32 from )
{
	xWrite8( 0xDB );
	ModRM( 0, 0x0, DISP32 );
	xWrite32( MEMADDR(from, 4) ); 
}

/* fistp m32 from fpu reg stack */
emitterT void FISTP32( u32 from ) 
{
	xWrite8( 0xDB );
	ModRM( 0, 0x3, DISP32 );
	xWrite32( MEMADDR(from, 4) ); 
}

/* fld m32 to fpu reg stack */
emitterT void FLD32( u32 from )
{
	xWrite8( 0xD9 );
	ModRM( 0, 0x0, DISP32 );
	xWrite32( MEMADDR(from, 4) ); 
}

// fld st(i)
emitterT void FLD(int st)	{ xWrite16(0xc0d9+(st<<8)); }
emitterT void FLD1()		{ xWrite16(0xe8d9); }
emitterT void FLDL2E()		{ xWrite16(0xead9); }

/* fst m32 from fpu reg stack */
emitterT void FST32( u32 to ) 
{
   xWrite8( 0xD9 );
   ModRM( 0, 0x2, DISP32 );
   xWrite32( MEMADDR(to, 4) ); 
}

/* fstp m32 from fpu reg stack */
emitterT void FSTP32( u32 to )
{
	xWrite8( 0xD9 );
	ModRM( 0, 0x3, DISP32 );
	xWrite32( MEMADDR(to, 4) ); 
}

// fstp st(i)
emitterT void FSTP(int st)	{ xWrite16(0xd8dd+(st<<8)); }

/* fldcw fpu control word from m16 */
emitterT void FLDCW( u32 from )
{
	xWrite8( 0xD9 );
	ModRM( 0, 0x5, DISP32 );
	xWrite32( MEMADDR(from, 4) ); 
}

/* fnstcw fpu control word to m16 */
emitterT void FNSTCW( u32 to ) 
{
	xWrite8( 0xD9 );
	ModRM( 0, 0x7, DISP32 );
	xWrite32( MEMADDR(to, 4) ); 
}

emitterT void FNSTSWtoAX()	{ xWrite16(0xE0DF); }
emitterT void FXAM()		{ xWrite16(0xe5d9); }
emitterT void FDECSTP()	{ xWrite16(0xf6d9); }
emitterT void FRNDINT()	{ xWrite16(0xfcd9); }
emitterT void FXCH(int st)	{ xWrite16(0xc8d9+(st<<8)); }
emitterT void F2XM1()		{ xWrite16(0xf0d9); }
emitterT void FSCALE()		{ xWrite16(0xfdd9); }
emitterT void FPATAN(void)	{ xWrite16(0xf3d9); }
emitterT void FSIN(void)	{ xWrite16(0xfed9); }

/* fadd ST(src) to fpu reg stack ST(0) */
emitterT void FADD32Rto0( x86IntRegType src )
{
   xWrite8( 0xD8 );
   xWrite8( 0xC0 + src );
}

/* fadd ST(0) to fpu reg stack ST(src) */
emitterT void FADD320toR( x86IntRegType src )
{
   xWrite8( 0xDC );
   xWrite8( 0xC0 + src );
}

/* fsub ST(src) to fpu reg stack ST(0) */
emitterT void FSUB32Rto0( x86IntRegType src )
{
   xWrite8( 0xD8 );
   xWrite8( 0xE0 + src );
}

/* fsub ST(0) to fpu reg stack ST(src) */
emitterT void FSUB320toR( x86IntRegType src )
{
   xWrite8( 0xDC );
   xWrite8( 0xE8 + src );
}

/* fsubp -> substract ST(0) from ST(1), store in ST(1) and POP stack */
emitterT void FSUBP( void )
{
   xWrite8( 0xDE );
   xWrite8( 0xE9 );
}

/* fmul ST(src) to fpu reg stack ST(0) */
emitterT void FMUL32Rto0( x86IntRegType src )
{
   xWrite8( 0xD8 );
   xWrite8( 0xC8 + src );
}

/* fmul ST(0) to fpu reg stack ST(src) */
emitterT void FMUL320toR( x86IntRegType src )
{
   xWrite8( 0xDC );
   xWrite8( 0xC8 + src );
}

/* fdiv ST(src) to fpu reg stack ST(0) */
emitterT void FDIV32Rto0( x86IntRegType src )
{
   xWrite8( 0xD8 );
   xWrite8( 0xF0 + src );
}

/* fdiv ST(0) to fpu reg stack ST(src) */
emitterT void FDIV320toR( x86IntRegType src )
{
   xWrite8( 0xDC );
   xWrite8( 0xF8 + src );
}

emitterT void FDIV320toRP( x86IntRegType src )
{
	xWrite8( 0xDE );
	xWrite8( 0xF8 + src );
}

/* fadd m32 to fpu reg stack */
emitterT void FADD32( u32 from ) 
{
	xWrite8( 0xD8 );
	ModRM( 0, 0x0, DISP32 );
	xWrite32( MEMADDR(from, 4) ); 
}

/* fsub m32 to fpu reg stack */
emitterT void FSUB32( u32 from ) 
{
	xWrite8( 0xD8 );
	ModRM( 0, 0x4, DISP32 );
	xWrite32( MEMADDR(from, 4) ); 
}

/* fmul m32 to fpu reg stack */
emitterT void FMUL32( u32 from )
{
	xWrite8( 0xD8 );
	ModRM( 0, 0x1, DISP32 );
	xWrite32( MEMADDR(from, 4) ); 
}

/* fdiv m32 to fpu reg stack */
emitterT void FDIV32( u32 from ) 
{
	xWrite8( 0xD8 );
	ModRM( 0, 0x6, DISP32 );
	xWrite32( MEMADDR(from, 4) ); 
}

/* fabs fpu reg stack */
emitterT void FABS( void )
{
	xWrite16( 0xE1D9 );
}

/* fsqrt fpu reg stack */
emitterT void FSQRT( void ) 
{
	xWrite16( 0xFAD9 );
}

/* fchs fpu reg stack */
emitterT void FCHS( void ) 
{
	xWrite16( 0xE0D9 );
}

/* fcomi st, st(i) */
emitterT void FCOMI( x86IntRegType src )
{
	xWrite8( 0xDB );
	xWrite8( 0xF0 + src ); 
}

/* fcomip st, st(i) */
emitterT void FCOMIP( x86IntRegType src )
{
	xWrite8( 0xDF );
	xWrite8( 0xF0 + src ); 
}

/* fucomi st, st(i) */
emitterT void FUCOMI( x86IntRegType src )
{
	xWrite8( 0xDB );
	xWrite8( 0xE8 + src ); 
}

/* fucomip st, st(i) */
emitterT void FUCOMIP( x86IntRegType src )
{
	xWrite8( 0xDF );
	xWrite8( 0xE8 + src ); 
}

/* fcom m32 to fpu reg stack */
emitterT void FCOM32( u32 from ) 
{
	xWrite8( 0xD8 );
	ModRM( 0, 0x2, DISP32 );
	xWrite32( MEMADDR(from, 4) ); 
}

/* fcomp m32 to fpu reg stack */
emitterT void FCOMP32( u32 from )
{
	xWrite8( 0xD8 );
	ModRM( 0, 0x3, DISP32 );
	xWrite32( MEMADDR(from, 4) ); 
}

#define FCMOV32( low, high ) \
   { \
	   xWrite8( low ); \
	   xWrite8( high + from );  \
   }

emitterT void FCMOVB32( x86IntRegType from )     { FCMOV32( 0xDA, 0xC0 ); }
emitterT void FCMOVE32( x86IntRegType from )     { FCMOV32( 0xDA, 0xC8 ); }
emitterT void FCMOVBE32( x86IntRegType from )    { FCMOV32( 0xDA, 0xD0 ); }
emitterT void FCMOVU32( x86IntRegType from )     { FCMOV32( 0xDA, 0xD8 ); }
emitterT void FCMOVNB32( x86IntRegType from )    { FCMOV32( 0xDB, 0xC0 ); }
emitterT void FCMOVNE32( x86IntRegType from )    { FCMOV32( 0xDB, 0xC8 ); }
emitterT void FCMOVNBE32( x86IntRegType from )   { FCMOV32( 0xDB, 0xD0 ); }
emitterT void FCMOVNU32( x86IntRegType from )    { FCMOV32( 0xDB, 0xD8 ); }
