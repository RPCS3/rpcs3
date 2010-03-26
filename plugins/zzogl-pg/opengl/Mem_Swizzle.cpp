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

#include "GS.h"
#include "Mem.h"
#include "Mem_Swizzle.h"

// special swizzle macros - which I converted to functions.
#ifdef ZEROGS_SSE2

__forceinline void SwizzleBlock32(u8 *dst, u8 *src, int pitch, u32 WriteMask) 
{
	SwizzleBlock32_sse2(dst, src, pitch, WriteMask);
}

__forceinline void SwizzleBlock16(u8 *dst, u8 *src, int pitch, u32 WriteMask) 
{
	SwizzleBlock16_sse2(dst, src, pitch/*, WriteMask*/);
}
__forceinline void SwizzleBlock8(u8 *dst, u8 *src, int pitch, u32 WriteMask) 
{
	SwizzleBlock8_sse2(dst, src, pitch/*, WriteMask*/);
}
__forceinline void SwizzleBlock4(u8 *dst, u8 *src, int pitch, u32 WriteMask) 
{
	SwizzleBlock4_sse2(dst, src, pitch/*, WriteMask*/);
}
__forceinline void SwizzleBlock32u(u8 *dst, u8 *src, int pitch, u32 WriteMask) 
{
	SwizzleBlock32u_sse2(dst, src, pitch, WriteMask);
}
__forceinline void SwizzleBlock16u(u8 *dst, u8 *src, int pitch, u32 WriteMask) 
{
	SwizzleBlock16u_sse2(dst, src, pitch/*, WriteMask*/);
}
__forceinline void SwizzleBlock8u(u8 *dst, u8 *src, int pitch, u32 WriteMask) 
{
	SwizzleBlock8u_sse2(dst, src, pitch/*, WriteMask*/);
}
__forceinline void SwizzleBlock4u(u8 *dst, u8 *src, int pitch, u32 WriteMask) 
{
	SwizzleBlock4u_sse2(dst, src, pitch/*, WriteMask*/);
}
#else

__forceinline void SwizzleBlock32(u8 *dst, u8 *src, int pitch, u32 WriteMask) 
{
	SwizzleBlock32_c(dst, src, pitch, WriteMask);
}

__forceinline void SwizzleBlock16(u8 *dst, u8 *src, int pitch, u32 WriteMask) 
{
	SwizzleBlock16_c(dst, src, pitch/*, WriteMask*/);
}
__forceinline void SwizzleBlock8(u8 *dst, u8 *src, int pitch, u32 WriteMask) 
{
	SwizzleBlock8_c(dst, src, pitch/*, WriteMask*/);
}
__forceinline void SwizzleBlock4(u8 *dst, u8 *src, int pitch, u32 WriteMask) 
{
	SwizzleBlock4_c(dst, src, pitch/*, WriteMask*/);
}
__forceinline void SwizzleBlock32u(u8 *dst, u8 *src, int pitch, u32 WriteMask) 
{
	SwizzleBlock32_c(dst, src, pitch, WriteMask);
}
__forceinline void SwizzleBlock16u(u8 *dst, u8 *src, int pitch, u32 WriteMask) 
{
	SwizzleBlock16_c(dst, src, pitch/*, WriteMask*/);
}
__forceinline void SwizzleBlock8u(u8 *dst, u8 *src, int pitch, u32 WriteMask) 
{
	SwizzleBlock8_c(dst, src, pitch/*, WriteMask*/);
}
__forceinline void SwizzleBlock4u(u8 *dst, u8 *src, int pitch, u32 WriteMask) 
{
	SwizzleBlock4_c(dst, src, pitch/*, WriteMask*/);
}

__forceinline void __fastcall SwizzleBlock32_c(u8* dst, u8* src, int srcpitch, u32 WriteMask)
{
	u32* d = &g_columnTable32[0][0];

	if(WriteMask == 0xffffffff)
	{
		for(int j = 0; j < 8; j++, d += 8, src += srcpitch)
			for(int i = 0; i < 8; i++)
				((u32*)dst)[d[i]] = ((u32*)src)[i];
	}
	else
	{
		for(int j = 0; j < 8; j++, d += 8, src += srcpitch)
			for(int i = 0; i < 8; i++)
				((u32*)dst)[d[i]] = (((u32*)dst)[d[i]] & ~WriteMask) | (((u32*)src)[i] & WriteMask);
	}
}


__forceinline void __fastcall SwizzleBlock24_c(u8* dst, u8* src, int srcpitch, u32 WriteMask)
{
	u32* d = &g_columnTable32[0][0];

	if(WriteMask == 0x00ffffff)
	{
		for(int j = 0; j < 8; j++, d += 8, src += srcpitch)
			for(int i = 0; i < 8; i++)
				((u32*)dst)[d[i]] = ((u32*)src)[i];
	}
	else
	{
		for(int j = 0; j < 8; j++, d += 8, src += srcpitch)
			for(int i = 0; i < 8; i++)
				((u32*)dst)[d[i]] = (((u32*)dst)[d[i]] & ~WriteMask) | (((u32*)src)[i] & WriteMask);
	}
}

__forceinline void __fastcall SwizzleBlock16_c(u8* dst, u8* src, int srcpitch, u32 WriteMask)
{
	u32* d = &g_columnTable16[0][0];

	for(int j = 0; j < 8; j++, d += 16, src += srcpitch)
		for(int i = 0; i < 16; i++)
			((u16*)dst)[d[i]] = ((u16*)src)[i];
}

__forceinline void __fastcall SwizzleBlock8_c(u8* dst, u8* src, int srcpitch, u32 WriteMask)
{
	u32* d = &g_columnTable8[0][0];

	for(int j = 0; j < 16; j++, d += 16, src += srcpitch)
		for(int i = 0; i < 16; i++)
			dst[d[i]] = src[i];
}

__forceinline void __fastcall SwizzleBlock4_c(u8* dst, u8* src, int srcpitch, u32 WriteMask)
{
	u32* d = &g_columnTable4[0][0];

	for(int j = 0; j < 16; j++, d += 32, src += srcpitch)
	{
		for(int i = 0; i < 32; i++)
		{
			u32 addr = d[i];
			u8 c = (src[i>>1] >> ((i&1) << 2)) & 0x0f;
			u32 shift = (addr&1) << 2;
			dst[addr >> 1] = (dst[addr >> 1] & (0xf0 >> shift)) | (c << shift);
		}
	}
}
#endif
__forceinline void SwizzleBlock24(u8 *dst, u8 *src, int pitch, u32 WriteMask) 
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

__forceinline void SwizzleBlock8H(u8 *dst, u8 *src, int pitch, u32 WriteMask)
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

__forceinline void SwizzleBlock4HH(u8 *dst, u8 *src, int pitch, u32 WriteMask) 
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

__forceinline void SwizzleBlock4HL(u8 *dst, u8 *src, int pitch, u32 WriteMask) 
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
