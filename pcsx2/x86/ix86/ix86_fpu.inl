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
//#include "PrecompiledHeader.h"

//------------------------------------------------------------------
// FPU instructions
//------------------------------------------------------------------

/* fild m32 to fpu reg stack */
emitterT void eFILD32( u32 from )
{
	write8<I>( 0xDB );
	ModRM<I>( 0, 0x0, DISP32 );
	write32<I>( MEMADDR(from, 4) ); 
}

/* fistp m32 from fpu reg stack */
emitterT void eFISTP32( u32 from ) 
{
	write8<I>( 0xDB );
	ModRM<I>( 0, 0x3, DISP32 );
	write32<I>( MEMADDR(from, 4) ); 
}

/* fld m32 to fpu reg stack */
emitterT void eFLD32( u32 from )
{
	write8<I>( 0xD9 );
	ModRM<I>( 0, 0x0, DISP32 );
	write32<I>( MEMADDR(from, 4) ); 
}

// fld st(i)
emitterT void eFLD(int st)	{ write16<I>(0xc0d9+(st<<8)); }
emitterT void eFLD1()		{ write16<I>(0xe8d9); }
emitterT void eFLDL2E()		{ write16<I>(0xead9); }

/* fst m32 from fpu reg stack */
emitterT void eFST32( u32 to ) 
{
   write8<I>( 0xD9 );
   ModRM<I>( 0, 0x2, DISP32 );
   write32<I>( MEMADDR(to, 4) ); 
}

/* fstp m32 from fpu reg stack */
emitterT void eFSTP32( u32 to )
{
	write8<I>( 0xD9 );
	ModRM<I>( 0, 0x3, DISP32 );
	write32<I>( MEMADDR(to, 4) ); 
}

// fstp st(i)
emitterT void eFSTP(int st)	{ write16<I>(0xd8dd+(st<<8)); }

/* fldcw fpu control word from m16 */
emitterT void eFLDCW( u32 from )
{
	write8<I>( 0xD9 );
	ModRM<I>( 0, 0x5, DISP32 );
	write32<I>( MEMADDR(from, 4) ); 
}

/* fnstcw fpu control word to m16 */
emitterT void eFNSTCW( u32 to ) 
{
	write8<I>( 0xD9 );
	ModRM<I>( 0, 0x7, DISP32 );
	write32<I>( MEMADDR(to, 4) ); 
}

emitterT void eFNSTSWtoAX()	{ write16<I>(0xE0DF); }
emitterT void eFXAM()		{ write16<I>(0xe5d9); }
emitterT void eFDECSTP()	{ write16<I>(0xf6d9); }
emitterT void eFRNDINT()	{ write16<I>(0xfcd9); }
emitterT void eFXCH(int st)	{ write16<I>(0xc8d9+(st<<8)); }
emitterT void eF2XM1()		{ write16<I>(0xf0d9); }
emitterT void eFSCALE()		{ write16<I>(0xfdd9); }
emitterT void eFPATAN(void)	{ write16<I>(0xf3d9); }
emitterT void eFSIN(void)	{ write16<I>(0xfed9); }

/* fadd ST(src) to fpu reg stack ST(0) */
emitterT void eFADD32Rto0( x86IntRegType src )
{
   write8<I>( 0xD8 );
   write8<I>( 0xC0 + src );
}

/* fadd ST(0) to fpu reg stack ST(src) */
emitterT void eFADD320toR( x86IntRegType src )
{
   write8<I>( 0xDC );
   write8<I>( 0xC0 + src );
}

/* fsub ST(src) to fpu reg stack ST(0) */
emitterT void eFSUB32Rto0( x86IntRegType src )
{
   write8<I>( 0xD8 );
   write8<I>( 0xE0 + src );
}

/* fsub ST(0) to fpu reg stack ST(src) */
emitterT void eFSUB320toR( x86IntRegType src )
{
   write8<I>( 0xDC );
   write8<I>( 0xE8 + src );
}

/* fsubp -> substract ST(0) from ST(1), store in ST(1) and POP stack */
emitterT void eFSUBP( void )
{
   write8<I>( 0xDE );
   write8<I>( 0xE9 );
}

/* fmul ST(src) to fpu reg stack ST(0) */
emitterT void eFMUL32Rto0( x86IntRegType src )
{
   write8<I>( 0xD8 );
   write8<I>( 0xC8 + src );
}

/* fmul ST(0) to fpu reg stack ST(src) */
emitterT void eFMUL320toR( x86IntRegType src )
{
   write8<I>( 0xDC );
   write8<I>( 0xC8 + src );
}

/* fdiv ST(src) to fpu reg stack ST(0) */
emitterT void eFDIV32Rto0( x86IntRegType src )
{
   write8<I>( 0xD8 );
   write8<I>( 0xF0 + src );
}

/* fdiv ST(0) to fpu reg stack ST(src) */
emitterT void eFDIV320toR( x86IntRegType src )
{
   write8<I>( 0xDC );
   write8<I>( 0xF8 + src );
}

emitterT void eFDIV320toRP( x86IntRegType src )
{
	write8<I>( 0xDE );
	write8<I>( 0xF8 + src );
}

/* fadd m32 to fpu reg stack */
emitterT void eFADD32( u32 from ) 
{
	write8<I>( 0xD8 );
	ModRM<I>( 0, 0x0, DISP32 );
	write32<I>( MEMADDR(from, 4) ); 
}

/* fsub m32 to fpu reg stack */
emitterT void eFSUB32( u32 from ) 
{
	write8<I>( 0xD8 );
	ModRM<I>( 0, 0x4, DISP32 );
	write32<I>( MEMADDR(from, 4) ); 
}

/* fmul m32 to fpu reg stack */
emitterT void eFMUL32( u32 from )
{
	write8<I>( 0xD8 );
	ModRM<I>( 0, 0x1, DISP32 );
	write32<I>( MEMADDR(from, 4) ); 
}

/* fdiv m32 to fpu reg stack */
emitterT void eFDIV32( u32 from ) 
{
	write8<I>( 0xD8 );
	ModRM<I>( 0, 0x6, DISP32 );
	write32<I>( MEMADDR(from, 4) ); 
}

/* fabs fpu reg stack */
emitterT void eFABS( void )
{
	write16<I>( 0xE1D9 );
}

/* fsqrt fpu reg stack */
emitterT void eFSQRT( void ) 
{
	write16<I>( 0xFAD9 );
}

/* fchs fpu reg stack */
emitterT void eFCHS( void ) 
{
	write16<I>( 0xE0D9 );
}

/* fcomi st, st(i) */
emitterT void eFCOMI( x86IntRegType src )
{
	write8<I>( 0xDB );
	write8<I>( 0xF0 + src ); 
}

/* fcomip st, st(i) */
emitterT void eFCOMIP( x86IntRegType src )
{
	write8<I>( 0xDF );
	write8<I>( 0xF0 + src ); 
}

/* fucomi st, st(i) */
emitterT void eFUCOMI( x86IntRegType src )
{
	write8<I>( 0xDB );
	write8<I>( 0xE8 + src ); 
}

/* fucomip st, st(i) */
emitterT void eFUCOMIP( x86IntRegType src )
{
	write8<I>( 0xDF );
	write8<I>( 0xE8 + src ); 
}

/* fcom m32 to fpu reg stack */
emitterT void eFCOM32( u32 from ) 
{
	write8<I>( 0xD8 );
	ModRM<I>( 0, 0x2, DISP32 );
	write32<I>( MEMADDR(from, 4) ); 
}

/* fcomp m32 to fpu reg stack */
emitterT void eFCOMP32( u32 from )
{
	write8<I>( 0xD8 );
	ModRM<I>( 0, 0x3, DISP32 );
	write32<I>( MEMADDR(from, 4) ); 
}

#define FCMOV32( low, high ) \
   { \
	   write8<I>( low ); \
	   write8<I>( high + from );  \
   }

emitterT void eFCMOVB32( x86IntRegType from )     { FCMOV32( 0xDA, 0xC0 ); }
emitterT void eFCMOVE32( x86IntRegType from )     { FCMOV32( 0xDA, 0xC8 ); }
emitterT void eFCMOVBE32( x86IntRegType from )    { FCMOV32( 0xDA, 0xD0 ); }
emitterT void eFCMOVU32( x86IntRegType from )     { FCMOV32( 0xDA, 0xD8 ); }
emitterT void eFCMOVNB32( x86IntRegType from )    { FCMOV32( 0xDB, 0xC0 ); }
emitterT void eFCMOVNE32( x86IntRegType from )    { FCMOV32( 0xDB, 0xC8 ); }
emitterT void eFCMOVNBE32( x86IntRegType from )   { FCMOV32( 0xDB, 0xD0 ); }
emitterT void eFCMOVNU32( x86IntRegType from )    { FCMOV32( 0xDB, 0xD8 ); }
