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
#define emitterT template<int I/*Emitter Index*/> 

#define MEMADDR(addr, oplen)	(addr)

#define Rex(w,r,x,b) assert(0);
#define RexR(w, reg) if( w||(reg)>=8 ) assert(0);
#define RexB(w, base) if( w||(base)>=8 ) assert(0);
#define RexRB(w, reg, base) if( w||(reg) >= 8 || (base)>=8 ) assert(0);
#define RexRXB(w, reg, index, base) if( w||(reg) >= 8 || (index) >= 8 || (base) >= 8 ) assert(0);

//------------------------------------------------------------------
// write functions
//------------------------------------------------------------------
extern u8  *x86Ptr[EmitterId_Count];
extern u8  *j8Ptr[32];
extern u32 *j32Ptr[32];

emitterT void write8( u8 val ) {  
	*x86Ptr[I] = (u8)val; 
	x86Ptr[I]++; 
} 

emitterT void write16( u16 val ) { 
	*(u16*)x86Ptr[I] = (u16)val; 
	x86Ptr[I] += 2;  
} 

emitterT void write24( u32 val ) { 
	*x86Ptr[I]++ = (u8)(val & 0xff); 
	*x86Ptr[I]++ = (u8)((val >> 8) & 0xff); 
	*x86Ptr[I]++ = (u8)((val >> 16) & 0xff); 
} 

emitterT void write32( u32 val ) { 
	*(u32*)x86Ptr[I] = val; 
	x86Ptr[I] += 4; 
} 

emitterT void write64( u64 val ){ 
	*(u64*)x86Ptr[I] = val; 
	x86Ptr[I] += 8; 
}
//------------------------------------------------------------------
/*
//------------------------------------------------------------------
// jump/align functions
//------------------------------------------------------------------
emitterT void ex86SetPtr( u8 *ptr );
emitterT void ex86SetJ8( u8 *j8 );
emitterT void ex86SetJ8A( u8 *j8 );
emitterT void ex86SetJ16( u16 *j16 );
emitterT void ex86SetJ16A( u16 *j16 );
emitterT void ex86SetJ32( u32 *j32 );
emitterT void ex86SetJ32A( u32 *j32 );
emitterT void ex86Align( int bytes );
emitterT void ex86AlignExecutable( int align );
//------------------------------------------------------------------

//------------------------------------------------------------------
// General Emitter Helper functions
//------------------------------------------------------------------
emitterT void WriteRmOffset(x86IntRegType to, int offset);
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
*/
#define MMXONLY(code) code
#define _MM_MK_INSERTPS_NDX(srcField, dstField, zeroMask) (((srcField)<<6) | ((dstField)<<4) | (zeroMask))

#include "ix86_macros.h"
#include "ix86.inl"
#include "ix86_3dnow.inl"
#include "ix86_fpu.inl"
#include "ix86_mmx.inl"
#include "ix86_sse.inl"
