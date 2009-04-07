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
 * ix86 definitions v0.6.2
 *  Authors: linuzappz <linuzappz@pcsx.net>
 *           alexey silinov
 *           goldfinger
 *           shadow < shadow@pcsx2.net >
 *			 cottonvibes(@gmail.com)
 */

#pragma once
#define _ix86_included_		// used for sanity checks by headers dependent on this one.

#include "ix86_types.h"

//------------------------------------------------------------------
// Helper Macros
//------------------------------------------------------------------
#define emitterT static __forceinline

#define MEMADDR(addr, oplen)	(addr)

#define Rex(w,r,x,b) assert(0)
#define RexR(w, reg) assert( !(w || (reg)>=8) )
#define RexB(w, base) assert( !(w || (base)>=8) )
#define RexRB(w, reg, base) assert( !(w || (reg) >= 8 || (base)>=8) )
#define RexRXB(w, reg, index, base) assert( !(w || (reg) >= 8 || (index) >= 8 || (base) >= 8) )

// We use int param for offsets and then test them for validity in the recompiler.
// This helps catch programmer errors better than using an auto-truncated s8 parameter.
#define assertOffset8(ofs) assert( ofs < 128 && ofs >= -128 )

#ifdef _MSC_VER
#define __threadlocal __declspec(thread)
#else
#define __threadlocal __thread
#endif

//------------------------------------------------------------------
// write functions
//------------------------------------------------------------------
extern __threadlocal u8  *x86Ptr;
extern __threadlocal u8  *j8Ptr[32];
extern __threadlocal u32 *j32Ptr[32];

emitterT void write8( u8 val )
{
	*x86Ptr = (u8)val;
	x86Ptr++; 
}

emitterT void write16( u16 val )
{ 
	*(u16*)x86Ptr = val; 
	x86Ptr += 2;  
} 

emitterT void write24( u32 val )
{ 
	*x86Ptr++ = (u8)(val & 0xff); 
	*x86Ptr++ = (u8)((val >> 8) & 0xff); 
	*x86Ptr++ = (u8)((val >> 16) & 0xff); 
} 

emitterT void write32( u32 val )
{ 
	*(u32*)x86Ptr = val; 
	x86Ptr += 4; 
} 

emitterT void write64( u64 val ){ 
	*(u64*)x86Ptr = val; 
	x86Ptr += 8; 
}
//------------------------------------------------------------------

//------------------------------------------------------------------
// jump/align functions
//------------------------------------------------------------------
emitterT void x86SetPtr( u8 *ptr );
emitterT void x86SetJ8( u8 *j8 );
emitterT void x86SetJ8A( u8 *j8 );
emitterT void x86SetJ16( u16 *j16 );
emitterT void x86SetJ16A( u16 *j16 );
emitterT void x86SetJ32( u32 *j32 );
emitterT void x86SetJ32A( u32 *j32 );
emitterT void x86Align( int bytes );
emitterT void x86AlignExecutable( int align );
//------------------------------------------------------------------

//------------------------------------------------------------------
// General Emitter Helper functions
//------------------------------------------------------------------
emitterT void WriteRmOffsetFrom(x86IntRegType to, x86IntRegType from, int offset);
emitterT void ModRM( int mod, int reg, int rm );
emitterT void SibSB( int ss, int index, int base );
emitterT void SET8R( int cc, int to );
emitterT void CMOV32RtoR( int cc, int to, int from );
emitterT void CMOV32MtoR( int cc, int to, uptr from );
emitterT u8*  J8Rel( int cc, int to );
emitterT u32* J32Rel( int cc, u32 to );
emitterT u64  GetCPUTick( void );
//------------------------------------------------------------------

emitterT void MOV32RtoR( x86IntRegType to, x86IntRegType from );
emitterT u32* JMP32( uptr to );
emitterT u8* JMP8( u8 to );
emitterT void CALL32( u32 to );
emitterT void LEA32RStoR(x86IntRegType to, x86IntRegType from, u32 scale);
emitterT void NOP( void );
emitterT void AND32ItoM( uptr to, u32 from );
emitterT void LEA32RRtoR(x86IntRegType to, x86IntRegType from0, x86IntRegType from1);
emitterT void LEA32RStoR(x86IntRegType to, x86IntRegType from, u32 scale);



#define MMXONLY(code) code
#define _MM_MK_INSERTPS_NDX(srcField, dstField, zeroMask) (((srcField)<<6) | ((dstField)<<4) | (zeroMask))

#include "ix86.inl"
#include "ix86_3dnow.inl"
#include "ix86_fpu.inl"
#include "ix86_mmx.inl"
#include "ix86_sse.inl"
