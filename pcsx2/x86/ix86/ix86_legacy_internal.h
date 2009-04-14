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

#include "ix86_internal.h"

//------------------------------------------------------------------
// Legacy Helper Macros and Functions (depreciated)
//------------------------------------------------------------------

#include "ix86_legacy_types.h"
#include "ix86_legacy_instructions.h"

#define MEMADDR(addr, oplen)	(addr)

#define Rex(w,r,x,b) assert(0)
#define RexR(w, reg) assert( !(w || (reg)>=8) )
#define RexB(w, base) assert( !(w || (base)>=8) )
#define RexRB(w, reg, base) assert( !(w || (reg) >= 8 || (base)>=8) )
#define RexRXB(w, reg, index, base) assert( !(w || (reg) >= 8 || (index) >= 8 || (base) >= 8) )

#define _MM_MK_INSERTPS_NDX(srcField, dstField, zeroMask) (((srcField)<<6) | ((dstField)<<4) | (zeroMask))

extern void WriteRmOffsetFrom(x86IntRegType to, x86IntRegType from, int offset);
extern void ModRM( uint mod, uint reg, uint rm );
extern void SibSB( uint ss, uint index, uint base );
extern void SET8R( int cc, int to );
extern u8*  J8Rel( int cc, int to );
extern u32* J32Rel( int cc, u32 to );
extern u64  GetCPUTick( void );


//////////////////////////////////////////////////////////////////////////////////////////
//
emitterT void ModRM( uint mod, uint reg, uint rm )
{
	// Note: Following ASSUMEs are for legacy support only.
	// The new emitter performs these sanity checks during operand construction, so these
	// assertions can probably be removed once all legacy emitter code has been removed.
	jASSUME( mod < 4 );
	jASSUME( reg < 8 );
	jASSUME( rm < 8 );
	//write8( (mod << 6) | (reg << 3) | rm );

	*(u32*)x86Ptr = (mod << 6) | (reg << 3) | rm;
	x86Ptr++;
	
}

emitterT void SibSB( uint ss, uint index, uint base )
{
	// Note: Following ASSUMEs are for legacy support only.
	// The new emitter performs these sanity checks during operand construction, so these
	// assertions can probably be removed once all legacy emitter code has been removed.
	jASSUME( ss < 4 );
	jASSUME( index < 8 );
	jASSUME( base < 8 );
	//write8( (ss << 6) | (index << 3) | base );

	*(u32*)x86Ptr = (ss << 6) | (index << 3) | base;
	x86Ptr++;
}
