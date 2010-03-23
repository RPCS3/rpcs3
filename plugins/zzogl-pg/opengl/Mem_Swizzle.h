/*  ZeroGS KOSMOS
 *  Copyright (C) 2005-2006 zerofrog@gmail.com
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef MEM_SWIZZLE_H_INCLUDED
#define MEM_SWIZZLE_H_INCLUDED

#include "GS.h"
#include "Mem.h"

// special swizzle macros - which I converted to functions.

static __forceinline __fastcall void SwizzleBlock24(u8 *dst, u8 *src, int pitch, u32 WriteMask = 0xffffffff) 
{ 
	u8* pnewsrc = src; 
	u32* pblock = tempblock; 
	
	for(int by = 0; by < 7; ++by, pblock += 8, pnewsrc += pitch-24) 
	{ 
		for(int bx = 0; bx < 8; ++bx, pnewsrc += 3) 
		{ 
			pblock[bx] = *(u32*)pnewsrc; 
		} 
	} 
	
	for(int bx = 0; bx < 7; ++bx, pnewsrc += 3) 
	{ 
		/* might be 1 byte out of bounds of GS memory */ 
		pblock[bx] = *(u32*)pnewsrc; 
	} 
	
	/* do 3 bytes for the last copy */ 
	*((u8*)pblock+28) = pnewsrc[0]; 
	*((u8*)pblock+29) = pnewsrc[1]; 
	*((u8*)pblock+30) = pnewsrc[2]; 
	SwizzleBlock32((u8*)dst, (u8*)tempblock, 32, 0x00ffffff); 
} 

#define SwizzleBlock24u SwizzleBlock24

static __forceinline __fastcall void SwizzleBlock8H(u8 *dst, u8 *src, int pitch, u32 WriteMask = 0xffffffff)
{
	u8* pnewsrc = src; 
	u32* pblock = tempblock; 
	
	for(int by = 0; by < 8; ++by, pblock += 8, pnewsrc += pitch) 
	{ 
		u32 u = *(u32*)pnewsrc; 
		pblock[0] = u<<24; 
		pblock[1] = u<<16; 
		pblock[2] = u<<8; 
		pblock[3] = u; 
		u = *(u32*)(pnewsrc+4); 
		pblock[4] = u<<24; 
		pblock[5] = u<<16; 
		pblock[6] = u<<8; 
		pblock[7] = u; 
	} 
	SwizzleBlock32((u8*)dst, (u8*)tempblock, 32, 0xff000000); 
} 

#define SwizzleBlock8Hu SwizzleBlock8H

static __forceinline __fastcall void SwizzleBlock4HH(u8 *dst, u8 *src, int pitch, u32 WriteMask = 0xffffffff) 
{ 
	u8* pnewsrc = src; 
	u32* pblock = tempblock; 
	
	for(int by = 0; by < 8; ++by, pblock += 8, pnewsrc += pitch) 
	{ 
		u32 u = *(u32*)pnewsrc; 
		pblock[0] = u<<28; 
		pblock[1] = u<<24; 
		pblock[2] = u<<20; 
		pblock[3] = u<<16; 
		pblock[4] = u<<12; 
		pblock[5] = u<<8; 
		pblock[6] = u<<4; 
		pblock[7] = u; 
	} 
	SwizzleBlock32((u8*)dst, (u8*)tempblock, 32, 0xf0000000); 
} 

#define SwizzleBlock4HHu SwizzleBlock4HH

static __forceinline __fastcall void SwizzleBlock4HL(u8 *dst, u8 *src, int pitch, u32 WriteMask = 0xffffffff) 
{
	u8* pnewsrc = src; 
	u32* pblock = tempblock; 
	
	for(int by = 0; by < 8; ++by, pblock += 8, pnewsrc += pitch) 
	{ 
		u32 u = *(u32*)pnewsrc; 
		pblock[0] = u<<24; 
		pblock[1] = u<<20; 
		pblock[2] = u<<16; 
		pblock[3] = u<<12; 
		pblock[4] = u<<8; 
		pblock[5] = u<<4; 
		pblock[6] = u; 
		pblock[7] = u>>4; 
	} 
	SwizzleBlock32((u8*)dst, (u8*)tempblock, 32, 0x0f000000); 
} 

#define SwizzleBlock4HLu SwizzleBlock4HL


#endif // MEM_SWIZZLE_H_INCLUDED
