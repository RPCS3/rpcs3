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

#define emitterT __forceinline

using x86Emitter::xWrite8;
using x86Emitter::xWrite16;
using x86Emitter::xWrite32;
using x86Emitter::xWrite64;

#include "ix86_legacy_types.h"
#include "ix86_legacy_instructions.h"

#define MEMADDR(addr, oplen)	(addr)

#define Rex(w,r,x,b) assert(0)
#define RexR(w, reg) assert( !(w || (reg)>=8) )
#define RexB(w, base) assert( !(w || (base)>=8) )
#define RexRB(w, reg, base) assert( !(w || (reg) >= 8 || (base)>=8) )
#define RexRXB(w, reg, index, base) assert( !(w || (reg) >= 8 || (index) >= 8 || (base) >= 8) )

#define _MM_MK_INSERTPS_NDX(srcField, dstField, zeroMask) (((srcField)<<6) | ((dstField)<<4) | (zeroMask))

extern void ModRM( uint mod, uint reg, uint rm );
extern void SibSB( uint ss, uint index, uint base );
extern void SET8R( int cc, int to );
extern u8*  J8Rel( int cc, int to );
extern u32* J32Rel( int cc, u32 to );

