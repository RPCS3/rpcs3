/*  ZZ Open GL graphics plugin
 *  Copyright (c)2009-2010 zeydlitz@gmail.com, arcum42@gmail.com
 *  Based on Zerofrog's ZeroGS KOSMOS (c)2005-2008
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

#include "GS.h"
#include "Mem.h"
#include "x86.h"

#if defined(ZEROGS_SSE2)
#include <emmintrin.h>
#endif

// swizzling

//These were only used in the old version of RESOLVE_32_BITS. Keeping for reference.
#if 0

/* FrameSwizzleBlock32 */ 
void __fastcall FrameSwizzleBlock32_c(u32* dst, u32* src, int srcpitch, u32 WriteMask) 
{ 
	u32* d = &g_columnTable32[0][0]; 
	
	if (WriteMask == 0xffffffff) 
	{ 
		for(int i = 0; i < 8; ++i, d += 8) 
		{ 
			for(int j = 0; j < 8; ++j) 
			{ 
				dst[d[j]] = (src[j]); 
			} 
			src += srcpitch; 
		} 
	} 
	else 
	{ 
		for(int i = 0; i < 8; ++i, d += 8) 
		{ 
			for(int j = 0; j < 8; ++j) 
			{				
				dst[d[j]] = ((src[j])&WriteMask)|(dst[d[j]]&~WriteMask); 
			} 
			src += srcpitch; 
		} 
	} 
} 

void __fastcall FrameSwizzleBlock32A2_c(u32* dst, u32* src, int srcpitch, u32 WriteMask) 
{ 
	u32* d = &g_columnTable32[0][0]; 
	
	if( WriteMask == 0xffffffff ) { 
		for(int i = 0; i < 8; ++i, d += 8) { 
			for(int j = 0; j < 8; ++j) { 
				dst[d[j]] = ((src[2*j] + src[2*j+1]) >> 1); 
			} 
			src += srcpitch; 
		} 
	} 
	else { 
		for(int i = 0; i < 8; ++i, d += 8) { 
			for(int j = 0; j < 8; ++j) { 
				dst[d[j]] = (((src[2*j] + src[2*j+1]) >> 1)&WriteMask)|(dst[d[j]]&~WriteMask); 
			} 
			src += srcpitch; 
		} 
	} 
} 

void __fastcall FrameSwizzleBlock32A4_c(u32* dst, u32* src, int srcpitch, u32 WriteMask) 
{ 
	u32* d = &g_columnTable32[0][0]; 
	
	if( WriteMask == 0xffffffff ) { 
		for(int i = 0; i < 8; ++i, d += 8) { 
			for(int j = 0; j < 8; ++j) { 
				dst[d[j]] = ((src[2*j] + src[2*j+1] + src[2*j+srcpitch] + src[2*j+srcpitch+1]) >> 2); 
			} 
			src += srcpitch << 1; 
		} 
	} 
	else { 
		for(int i = 0; i < 8; ++i, d += 8) { 
			for(int j = 0; j < 8; ++j) { 
				dst[d[j]] = (((src[2*j] + src[2*j+1] + src[2*j+srcpitch] + src[2*j+srcpitch+1]) >> 2)&WriteMask)|(dst[d[j]]&~WriteMask); 
			} 
			src += srcpitch << 1; 
		} 
	} 
} 

#define FrameSwizzleBlock24_c FrameSwizzleBlock32_c
#define FrameSwizzleBlock24A2_c FrameSwizzleBlock32A2_c
#define FrameSwizzleBlock24A4_c FrameSwizzleBlock32A4_c

/* FrameSwizzleBlock16 */ 
void __fastcall FrameSwizzleBlock16_c(u16* dst, u32* src, int srcpitch, u32 WriteMask) 
{ 
	u32* d = &g_columnTable16[0][0]; 
	
	if (WriteMask == 0xffff) 
	{  
		for(int i = 0; i < 8; ++i, d += 16) 
		{ 
			for(int j = 0; j < 16; ++j) 
			{ 
				u32 temp = (src[j]); 
				dst[d[j]] = RGBA32to16(temp); 
			} 
			src += srcpitch; 
		} 
	} 
	else 
	{ 
		for(int i = 0; i < 8; ++i, d += 16) 
		{ 
			for(int j = 0; j < 16; ++j) 
			{ 
				u32 temp = (src[j]); 
				u32 dsrc = RGBA32to16(temp); 
				dst[d[j]] = (dsrc&WriteMask)|(dst[d[j]]&~WriteMask); 
			} 
			src += srcpitch; 
		} 
	} 
}

void __fastcall FrameSwizzleBlock16A2_c(u16* dst, u32* src, int srcpitch, u32 WriteMask) 
{ 
	u32* d = &g_columnTable16[0][0]; 
	
	if (WriteMask == 0xffff) 
	{  
		for(int i = 0; i < 8; ++i, d += 16) 
		{ 
			for(int j = 0; j < 16; ++j) 
			{ 
				u32 temp = ((src[2*j] + src[2*j+1]) >> 1); 
				dst[d[j]] = RGBA32to16(temp); 
			} 
			src += srcpitch; 
		} 
	} 
	else 
	{ 
		for(int i = 0; i < 8; ++i, d += 16) 
		{ 
			for(int j = 0; j < 16; ++j) 
			{ 
				u32 temp = ((src[2*j] + src[2*j+1]) >> 1); 
				u32 dsrc = RGBA32to16(temp); 
				dst[d[j]] = (dsrc&WriteMask)|(dst[d[j]]&~WriteMask); 
			} 
			src += srcpitch; 
		} 
	} 
}

void __fastcall FrameSwizzleBlock16A4_c(u16* dst, u32* src, int srcpitch, u32 WriteMask) 
{ 
	u32* d = &g_columnTable16[0][0]; 
	
	if (WriteMask == 0xffff) 
	{  
		for(int i = 0; i < 8; ++i, d += 16) 
		{ 
			for(int j = 0; j < 16; ++j) 
			{ 
				u32 temp = ((src[2*j] + src[2*j+1] + src[2*j+srcpitch] + src[2*j+srcpitch+1]) >> 2); 
				dst[d[j]] = RGBA32to16(temp); 
			} 
			src += srcpitch << 1; 
		} 
	} 
	else 
	{ 
		for(int i = 0; i < 8; ++i, d += 16) 
		{ 
			for(int j = 0; j < 16; ++j) 
			{ 
				u32 temp = ((src[2*j] + src[2*j+1] + src[2*j+srcpitch] + src[2*j+srcpitch+1]) >> 2); 
				u32 dsrc = RGBA32to16(temp); 
				dst[d[j]] = (dsrc&WriteMask)|(dst[d[j]]&~WriteMask); 
			} 
			src += srcpitch << 1; 
		} 
	} 
}


/* Frame16SwizzleBlock32 */ 
void __fastcall Frame16SwizzleBlock32_c(u32* dst, Vector_16F* src, int srcpitch, u32 WriteMask) 
{ 
	u32* d = &g_columnTable32[0][0]; 

	if( WriteMask == 0xffffffff ) 
	{  
		for(int i = 0; i < 8; ++i, d += 8) 
		{ 
			for(int j = 0; j < 8; ++j) 
			{ 
				Vector_16F dsrc16 = (src[j]); 
				dst[d[j]] = Float16ToARGB(dsrc16); 
			} 
			src += srcpitch; 
		} 
	} 
	else 
	{ 
		for(int i = 0; i < 8; ++i, d += 8) 
		{ 
			for(int j = 0; j < 8; ++j) 
			{ 
				Vector_16F dsrc16 = (src[j]); 
				u32 dsrc = Float16ToARGB(dsrc16);
				dst[d[j]] = (dsrc&WriteMask)|(dst[d[j]]&~WriteMask); 
			} 
			src += srcpitch; 
		} 
	} 
 } 
 
void __fastcall Frame16SwizzleBlock32A2_c(u32* dst, Vector_16F* src, int srcpitch, u32 WriteMask) 
{ 
	u32* d = &g_columnTable32[0][0]; 

	if( WriteMask == 0xffffffff ) 
	{  
		for(int i = 0; i < 8; ++i, d += 8) 
		{ 
			for(int j = 0; j < 8; ++j) 
			{ 
				Vector_16F dsrc16 = (src[2*j]); 
				dst[d[j]] = Float16ToARGB(dsrc16); 
			} 
			src += srcpitch; 
		} 
	} 
	else 
	{ 
		for(int i = 0; i < 8; ++i, d += 8) 
		{ 
			for(int j = 0; j < 8; ++j) 
			{ 
				Vector_16F dsrc16 = (src[2*j]); 
				u32 dsrc = Float16ToARGB(dsrc16);
				dst[d[j]] = (dsrc&WriteMask)|(dst[d[j]]&~WriteMask); 
			} 
			src += srcpitch; 
		} 
	} 
 } 
 
void __fastcall Frame16SwizzleBlock32A4_c(u32* dst, Vector_16F* src, int srcpitch, u32 WriteMask) 
{ 
	u32* d = &g_columnTable32[0][0]; 

	if( WriteMask == 0xffffffff ) 
	{  
		for(int i = 0; i < 8; ++i, d += 8) 
		{ 
			for(int j = 0; j < 8; ++j) 
			{ 
				Vector_16F dsrc16 = (src[2*j]); 
				dst[d[j]] = Float16ToARGB(dsrc16); 
			} 
			src += srcpitch << 1; 
		} 
	} 
	else 
	{ 
		for(int i = 0; i < 8; ++i, d += 8) 
		{ 
			for(int j = 0; j < 8; ++j) 
			{ 
				Vector_16F dsrc16 = (src[2*j]); 
				u32 dsrc = Float16ToARGB(dsrc16);
				dst[d[j]] = (dsrc&WriteMask)|(dst[d[j]]&~WriteMask); 
			} 
			src += srcpitch << 1; 
		} 
	} 
 } 

/* Frame16SwizzleBlock32Z */ 
void __fastcall Frame16SwizzleBlock32Z_c(u32* dst, Vector_16F* src, int srcpitch, u32 WriteMask) 
{ 
	u32* d = &g_columnTable32[0][0]; 
	if( WriteMask == 0xffffffff )  /* breaks KH text if not checked */ 
	{
		for(int i = 0; i < 8; ++i, d += 8) 
		{ 
			for(int j = 0; j < 8; ++j) 
			{ 
				Vector_16F dsrc16 = (src[j]); 
				dst[d[j]] = Float16ToARGB_Z(dsrc16); 
			} 
			src += srcpitch; 
		} 
	} 
	else 
	{
		for(int i = 0; i < 8; ++i, d += 8) 
		{ 
			for(int j = 0; j < 8; ++j) 
			{ 
				Vector_16F dsrc16 = (src[j]); 
				u32 dsrc = Float16ToARGB_Z(dsrc16); 
				dst[d[j]] = (dsrc&WriteMask)|(dst[d[j]]&~WriteMask); 
			} 
			src += srcpitch; 
		} 
	} 
 }
 
void __fastcall Frame16SwizzleBlock32ZA2_c(u32* dst, Vector_16F* src, int srcpitch, u32 WriteMask) 
{ 
	u32* d = &g_columnTable32[0][0]; 
	if( WriteMask == 0xffffffff )  /* breaks KH text if not checked */ 
	{
		for(int i = 0; i < 8; ++i, d += 8) 
		{ 
			for(int j = 0; j < 8; ++j) 
			{ 
				Vector_16F dsrc16 = (src[2*j]); 
				dst[d[j]] = Float16ToARGB_Z(dsrc16); 
			} 
			src += srcpitch; 
		} 
	} 
	else 
	{
		for(int i = 0; i < 8; ++i, d += 8) 
		{ 
			for(int j = 0; j < 8; ++j) 
			{ 
				Vector_16F dsrc16 = (src[2*j]); 
				u32 dsrc = Float16ToARGB_Z(dsrc16); 
				dst[d[j]] = (dsrc&WriteMask)|(dst[d[j]]&~WriteMask); 
			} 
			src += srcpitch; 
		} 
	} 
 }
 
void __fastcall Frame16SwizzleBlock32ZA4_c(u32* dst, Vector_16F* src, int srcpitch, u32 WriteMask) 
{ 
	u32* d = &g_columnTable32[0][0]; 
	if( WriteMask == 0xffffffff )  /* breaks KH text if not checked */ 
	{
		for(int i = 0; i < 8; ++i, d += 8) 
		{ 
			for(int j = 0; j < 8; ++j) 
			{ 
				Vector_16F dsrc16 = (src[2*j]); 
				dst[d[j]] = Float16ToARGB_Z(dsrc16); 
			} 
			src += srcpitch << 1; 
		} 
	} 
	else 
	{
		for(int i = 0; i < 8; ++i, d += 8) 
		{ 
			for(int j = 0; j < 8; ++j) 
			{ 
				Vector_16F dsrc16 = (src[2*j]); 
				u32 dsrc = Float16ToARGB_Z(dsrc16); 
				dst[d[j]] = (dsrc&WriteMask)|(dst[d[j]]&~WriteMask); 
			} 
			src += srcpitch << 1; 
		} 
	} 
 }
 
 
 /* Frame16SwizzleBlock16 */ 
void __fastcall Frame16SwizzleBlock16_c(u16* dst, Vector_16F* src, int srcpitch, u32 WriteMask) 
{ 
	u32* d = &g_columnTable16[0][0]; 
	
	if ((WriteMask&0xfff8f8f8) == 0xfff8f8f8) 
	{  
		for(int i = 0; i < 8; ++i, d += 16) 
		{ 
			for(int j = 0; j < 16; ++j) 
			{ 
				Vector_16F dsrc16 = (src[j]); 
				dst[d[j]] = Float16ToARGB16(dsrc16); 
			} 
			src += srcpitch; 
		} 
	} 
	else 
	{ 
		for(int i = 0; i < 8; ++i, d += 16) 
		{ 
			for(int j = 0; j < 16; ++j) 
			{ 
				Vector_16F dsrc16 = (src[j]); 
				u32 dsrc = Float16ToARGB16(dsrc16); 
				dst[d[j]] = (dsrc&WriteMask)|(dst[d[j]]&~WriteMask); 
			} 
			src += srcpitch; 
		} 
	} 
 } 
 
void __fastcall Frame16SwizzleBlock16A2_c(u16* dst, Vector_16F* src, int srcpitch, u32 WriteMask) 
{ 
	u32* d = &g_columnTable16[0][0]; 
	
	if ((WriteMask&0xfff8f8f8) == 0xfff8f8f8) 
	{  
		for(int i = 0; i < 8; ++i, d += 16) 
		{ 
			for(int j = 0; j < 16; ++j) 
			{ 
				Vector_16F dsrc16 = (src[2*j]); 
				dst[d[j]] = Float16ToARGB16(dsrc16); 
			} 
			src += srcpitch; 
		} 
	} 
	else 
	{ 
		for(int i = 0; i < 8; ++i, d += 16) 
		{ 
			for(int j = 0; j < 16; ++j) 
			{ 
				Vector_16F dsrc16 = (src[2*j]); 
				u32 dsrc = Float16ToARGB16(dsrc16); 
				dst[d[j]] = (dsrc&WriteMask)|(dst[d[j]]&~WriteMask); 
			} 
			src += srcpitch; 
		} 
	} 
 } 
 
void __fastcall Frame16SwizzleBlock16A4_c(u16* dst, Vector_16F* src, int srcpitch, u32 WriteMask) 
{ 
	u32* d = &g_columnTable16[0][0]; 
	
	if ((WriteMask&0xfff8f8f8) == 0xfff8f8f8) 
	{  
		for(int i = 0; i < 8; ++i, d += 16) 
		{ 
			for(int j = 0; j < 16; ++j) 
			{ 
				Vector_16F dsrc16 = (src[2*j]); 
				dst[d[j]] = Float16ToARGB16(dsrc16); 
			} 
			src += srcpitch << 1; 
		} 
	} 
	else 
	{ 
		for(int i = 0; i < 8; ++i, d += 16) 
		{ 
			for(int j = 0; j < 16; ++j) 
			{ 
				Vector_16F dsrc16 = (src[2*j]); 
				u32 dsrc = Float16ToARGB16(dsrc16); 
				dst[d[j]] = (dsrc&WriteMask)|(dst[d[j]]&~WriteMask); 
			} 
			src += srcpitch << 1; 
		} 
	} 
 } 
 
 /* Frame16SwizzleBlock16Z */ 
void __fastcall Frame16SwizzleBlock16Z_c(u16* dst, Vector_16F* src, int srcpitch, u32 WriteMask) 
{ 
	u32* d = &g_columnTable16[0][0]; 
	
	for(int i = 0; i < 8; ++i, d += 16) 
	{ 
		for(int j = 0; j < 16; ++j) 
		{ 
			Vector_16F dsrc16 = (src[j]); 
			dst[d[j]] = Float16ToARGB16_Z(dsrc16); 
		} 
		src += srcpitch; 
	} 
} 

void __fastcall Frame16SwizzleBlock16ZA2_c(u16* dst, Vector_16F* src, int srcpitch, u32 WriteMask) 
{ 
	u32* d = &g_columnTable16[0][0]; 
	
	for(int i = 0; i < 8; ++i, d += 16) 
	{ 
		for(int j = 0; j < 16; ++j) 
		{ 
			Vector_16F dsrc16 = (src[2*j]); 
			dst[d[j]] = Float16ToARGB16_Z(dsrc16); 
		} 
		src += srcpitch; 
	} 
} 

void __fastcall Frame16SwizzleBlock16ZA4_c(u16* dst, Vector_16F* src, int srcpitch, u32 WriteMask) 
{ 
	u32* d = &g_columnTable16[0][0]; 
	
	for(int i = 0; i < 8; ++i, d += 16) 
	{ 
		for(int j = 0; j < 16; ++j) 
		{ 
			Vector_16F dsrc16 = (src[2*j]); 
			dst[d[j]] = Float16ToARGB16_Z(dsrc16); 
		} 
		src += srcpitch << 1; 
	} 
} 
#endif

#ifdef ZEROGS_SSE2

//void __fastcall WriteCLUT_T16_I8_CSM1_sse2(u32* vm, u32* clut)
//{
//  __asm {
//	  mov eax, vm
//	  mov ecx, clut
//	  mov edx, 8
//  }
//
//Extract32x2:
//  __asm {
//	  movdqa xmm0, qword ptr [eax]
//	  movdqa xmm1, qword ptr [eax+16]
//	  movdqa xmm2, qword ptr [eax+32]
//	  movdqa xmm3, qword ptr [eax+48]
//
//	  // rearrange
//	  pshuflw xmm0, xmm0, 0xd8
//	  pshufhw xmm0, xmm0, 0xd8
//	  pshuflw xmm1, xmm1, 0xd8
//	  pshufhw xmm1, xmm1, 0xd8
//	  pshuflw xmm2, xmm2, 0xd8
//	  pshufhw xmm2, xmm2, 0xd8
//	  pshuflw xmm3, xmm3, 0xd8
//	  pshufhw xmm3, xmm3, 0xd8
//
//	  movdqa xmm4, xmm0
//	  movdqa xmm6, xmm2
//
//	  shufps xmm0, xmm1, 0x88
//	  shufps xmm2, xmm3, 0x88
//
//	  shufps xmm4, xmm1, 0xdd
//	  shufps xmm6, xmm3, 0xdd
//
//	  pshufd xmm0, xmm0, 0xd8
//	  pshufd xmm2, xmm2, 0xd8
//	  pshufd xmm4, xmm4, 0xd8
//	  pshufd xmm6, xmm6, 0xd8
//
//	  // left column
//	  movhlps xmm1, xmm0
//	  movlhps xmm0, xmm2
//	  //movdqa xmm7, [ecx]
//
//	  movdqa [ecx], xmm0
//	  shufps xmm1, xmm2, 0xe4
//	  movdqa [ecx+16], xmm1
//
//	  // right column
//	  movhlps xmm3, xmm4
//	  movlhps xmm4, xmm6
//	  movdqa [ecx+32], xmm4
//	  shufps xmm3, xmm6, 0xe4
//	  movdqa [ecx+48], xmm3
//
//	  add eax, 16*4
//	  add ecx, 16*8
//	  sub edx, 1
//	  cmp edx, 0
//	  jne Extract32x2
//  }
//}

extern "C" void __fastcall WriteCLUT_T32_I8_CSM1_sse2(u32* vm, u32* clut)
{
	__m128i* src = (__m128i*)vm;
	__m128i* dst = (__m128i*)clut;

	for (int j = 0; j < 64; j += 32, src += 32, dst += 32)
	{
		for (int i = 0; i < 16; i += 4)
		{
			__m128i r0 = _mm_load_si128(&src[i+0]);
			__m128i r1 = _mm_load_si128(&src[i+1]);
			__m128i r2 = _mm_load_si128(&src[i+2]);
			__m128i r3 = _mm_load_si128(&src[i+3]);

			_mm_store_si128(&dst[i*2+0], _mm_unpacklo_epi64(r0, r1));
			_mm_store_si128(&dst[i*2+1], _mm_unpacklo_epi64(r2, r3));
			_mm_store_si128(&dst[i*2+2], _mm_unpackhi_epi64(r0, r1));
			_mm_store_si128(&dst[i*2+3], _mm_unpackhi_epi64(r2, r3));

			__m128i r4 = _mm_load_si128(&src[i+0+16]);
			__m128i r5 = _mm_load_si128(&src[i+1+16]);
			__m128i r6 = _mm_load_si128(&src[i+2+16]);
			__m128i r7 = _mm_load_si128(&src[i+3+16]);

			_mm_store_si128(&dst[i*2+4], _mm_unpacklo_epi64(r4, r5));
			_mm_store_si128(&dst[i*2+5], _mm_unpacklo_epi64(r6, r7));
			_mm_store_si128(&dst[i*2+6], _mm_unpackhi_epi64(r4, r5));
			_mm_store_si128(&dst[i*2+7], _mm_unpackhi_epi64(r6, r7));
		}
	}
}


extern "C" void __fastcall WriteCLUT_T32_I4_CSM1_sse2(u32* vm, u32* clut)
{
	__m128i* src = (__m128i*)vm;
	__m128i* dst = (__m128i*)clut;

	__m128i r0 = _mm_load_si128(&src[0]);
	__m128i r1 = _mm_load_si128(&src[1]);
	__m128i r2 = _mm_load_si128(&src[2]);
	__m128i r3 = _mm_load_si128(&src[3]);

	_mm_store_si128(&dst[0], _mm_unpacklo_epi64(r0, r1));
	_mm_store_si128(&dst[1], _mm_unpacklo_epi64(r2, r3));
	_mm_store_si128(&dst[2], _mm_unpackhi_epi64(r0, r1));
	_mm_store_si128(&dst[3], _mm_unpackhi_epi64(r2, r3));
}

static const __aligned16 int s_clut_16bits_mask[4] = { 0x0000ffff, 0x0000ffff, 0x0000ffff, 0x0000ffff };
static const __aligned16 int s_clut16mask2[4] = { 0x0000ffff, 0x0000ffff, 0x0000ffff, 0x0000ffff };
static const __aligned16 int s_clut16mask[8] = { 0xffff0000, 0xffff0000, 0xffff0000, 0xffff0000,
										   0x0000ffff, 0x0000ffff, 0x0000ffff, 0x0000ffff
										   };

template<bool CSA_0_15, bool HIGH_16BITS_VM>
void __fastcall WriteCLUT_T16_I4_CSM1_core_sse2(u32* vm, u32* clut)
{
    __m128i vm_0;
    __m128i vm_1;
    __m128i vm_2;
    __m128i vm_3;
    __m128i clut_0;
    __m128i clut_1;
    __m128i clut_2;
    __m128i clut_3;

    __m128i clut_mask = _mm_load_si128((__m128i*)s_clut_16bits_mask);

    // !HIGH_16BITS_VM
    // CSA in 0-15
    // Replace lower 16 bits of clut0 with lower 16 bits of vm
    // CSA in 16-31
    // Replace higher 16 bits of clut0 with lower 16 bits of vm

    // HIGH_16BITS_VM
    // CSA in 0-15
    // Replace lower 16 bits of clut0 with higher 16 bits of vm
    // CSA in 16-31
    // Replace higher 16 bits of clut0 with higher 16 bits of vm
    if(HIGH_16BITS_VM && CSA_0_15) {
        // move up to low
        vm_0 = _mm_load_si128((__m128i*)vm); // 9 8 1 0
        vm_1 = _mm_load_si128((__m128i*)vm+1); // 11 10 3 2
        vm_2 = _mm_load_si128((__m128i*)vm+2); // 13 12 5 4
        vm_3 = _mm_load_si128((__m128i*)vm+3); // 15 14 7 6
        vm_0 = _mm_srli_epi32(vm_0, 16);
        vm_1 = _mm_srli_epi32(vm_1, 16);
        vm_2 = _mm_srli_epi32(vm_2, 16);
        vm_3 = _mm_srli_epi32(vm_3, 16);
    } else if(HIGH_16BITS_VM && !CSA_0_15) {
        // Remove lower 16 bits
        vm_0 = _mm_andnot_si128(clut_mask, _mm_load_si128((__m128i*)vm)); // 9 8 1 0
        vm_1 = _mm_andnot_si128(clut_mask, _mm_load_si128((__m128i*)vm+1)); // 11 10 3 2
        vm_2 = _mm_andnot_si128(clut_mask, _mm_load_si128((__m128i*)vm+2)); // 13 12 5 4
        vm_3 = _mm_andnot_si128(clut_mask, _mm_load_si128((__m128i*)vm+3)); // 15 14 7 6
    } else if(!HIGH_16BITS_VM && CSA_0_15) {
        // Remove higher 16 bits
        vm_0 = _mm_and_si128(clut_mask, _mm_load_si128((__m128i*)vm)); // 9 8 1 0
        vm_1 = _mm_and_si128(clut_mask, _mm_load_si128((__m128i*)vm+1)); // 11 10 3 2
        vm_2 = _mm_and_si128(clut_mask, _mm_load_si128((__m128i*)vm+2)); // 13 12 5 4
        vm_3 = _mm_and_si128(clut_mask, _mm_load_si128((__m128i*)vm+3)); // 15 14 7 6
    } else if(!HIGH_16BITS_VM && !CSA_0_15) {
        // move low to high
        vm_0 = _mm_load_si128((__m128i*)vm); // 9 8 1 0
        vm_1 = _mm_load_si128((__m128i*)vm+1); // 11 10 3 2
        vm_2 = _mm_load_si128((__m128i*)vm+2); // 13 12 5 4
        vm_3 = _mm_load_si128((__m128i*)vm+3); // 15 14 7 6
        vm_0 = _mm_slli_epi32(vm_0, 16);
        vm_1 = _mm_slli_epi32(vm_1, 16);
        vm_2 = _mm_slli_epi32(vm_2, 16);
        vm_3 = _mm_slli_epi32(vm_3, 16);
    }

    // Unsizzle the data
    __m128i row_0 = _mm_unpacklo_epi32(vm_0, vm_1); // 3 2 1 0
    __m128i row_1 = _mm_unpacklo_epi32(vm_2, vm_3); // 7 6 5 4
    __m128i row_2 = _mm_unpackhi_epi32(vm_0, vm_1); // 11 10 9 8
    __m128i row_3 = _mm_unpackhi_epi32(vm_2, vm_3); // 15 14 13 12

    // load old data & remove useless part
    if(CSA_0_15) {
        // Remove lower 16 bits
        clut_0 = _mm_andnot_si128(clut_mask, _mm_load_si128((__m128i*)clut));
        clut_1 = _mm_andnot_si128(clut_mask, _mm_load_si128((__m128i*)clut+1));
        clut_2 = _mm_andnot_si128(clut_mask, _mm_load_si128((__m128i*)clut+2));
        clut_3 = _mm_andnot_si128(clut_mask, _mm_load_si128((__m128i*)clut+3));
    } else {
        // Remove higher 16 bits
        clut_0 = _mm_and_si128(clut_mask, _mm_load_si128((__m128i*)clut));
        clut_1 = _mm_and_si128(clut_mask, _mm_load_si128((__m128i*)clut+1));
        clut_2 = _mm_and_si128(clut_mask, _mm_load_si128((__m128i*)clut+2));
        clut_3 = _mm_and_si128(clut_mask, _mm_load_si128((__m128i*)clut+3));
    }

    // Merge old & new data
    clut_0 = _mm_or_si128(clut_0, row_0);
    clut_1 = _mm_or_si128(clut_1, row_1);
    clut_2 = _mm_or_si128(clut_2, row_2);
    clut_3 = _mm_or_si128(clut_3, row_3);

    _mm_store_si128((__m128i*)clut, clut_0);
    _mm_store_si128((__m128i*)clut+1, clut_1);
    _mm_store_si128((__m128i*)clut+2, clut_2);
    _mm_store_si128((__m128i*)clut+3, clut_3);
}

extern "C" void __fastcall WriteCLUT_T16_I4_CSM1_sse2(u32* vm, u32 csa)
{
    u32* clut = (u32*)(g_pbyGSClut + 64*(csa & 15));

    if (csa > 15) {
        WriteCLUT_T16_I4_CSM1_core_sse2<false, false>(vm, clut);
    } else {
        WriteCLUT_T16_I4_CSM1_core_sse2<true, false>(vm, clut);
    }
}

extern "C" void __fastcall WriteCLUT_T16_I4_CSM1_sse2_old(u32* vm, u32* clut)
{
#define YET_ANOTHER_INTRINSIC
#ifdef YET_ANOTHER_INTRINSIC
    __m128i vm0 = _mm_load_si128((__m128i*)vm);
    __m128i vm1 = _mm_load_si128((__m128i*)vm+1);
    __m128i vm2 = _mm_load_si128((__m128i*)vm+2);
    __m128i vm3 = _mm_load_si128((__m128i*)vm+3);

    // rearrange 16bits words
    vm0 = _mm_shufflehi_epi16(vm0, 0x88);
    vm0 = _mm_shufflelo_epi16(vm0, 0x88); // 6 4 6 4  2 0 2 0
    vm1 = _mm_shufflehi_epi16(vm1, 0x88);
    vm1 = _mm_shufflelo_epi16(vm1, 0x88); // 14 12 14 12  10 8 10 8

    // Note: MSVC complains about direct c-cast...
    // vm0 = (__m128i)_mm_shuffle_ps((__m128)vm0, (__m128)vm1, 0x88); // 14 12 10 8  6 4 2 0
    __m128 vm0_f = (_mm_shuffle_ps((__m128&)vm0, (__m128&)vm1, 0x88)); // 14 12 10 8  6 4 2 0
    vm0 = (__m128i&)vm0_f;
    vm0 = _mm_shuffle_epi32(vm0, 0xD8); // 14 12 6 4  10 8 2 0

    // *** Same jobs for vm2 and vm3
    vm2 = _mm_shufflehi_epi16(vm2, 0x88);
    vm2 = _mm_shufflelo_epi16(vm2, 0x88);
    vm3 = _mm_shufflehi_epi16(vm3, 0x88);
    vm3 = _mm_shufflelo_epi16(vm3, 0x88);

    // Note: MSVC complains about direct c-cast...
    // vm2 = (__m128i)_mm_shuffle_ps((__m128)vm2, (__m128)vm3, 0x88);
    __m128 vm2_f = (_mm_shuffle_ps((__m128&)vm2, (__m128&)vm3, 0x88));
    vm2 = (__m128i&)vm2_f;
    vm2 = _mm_shuffle_epi32(vm2, 0xD8);

    // Create a zero register.
    __m128i zero_128 = _mm_setzero_si128();

    if ((u32)clut & 0x0F) {
        // Unaligned write.

        u16* clut_word_ptr = (u16*)clut;
        __m128i clut_mask = _mm_load_si128((__m128i*)s_clut16mask2);

        // Load previous data and clear high 16 bits of double words
        __m128i clut_0 = _mm_load_si128((__m128i*)(clut_word_ptr-1)); // 6 5 4 3  2 1 0 x
        __m128i clut_2 = _mm_load_si128((__m128i*)(clut_word_ptr-1)+2); // 22 21 20 19  18 17 16 15
        clut_0 = _mm_and_si128(clut_0, clut_mask); // - 5 - 3  - 1 - x
        clut_2 = _mm_and_si128(clut_2, clut_mask); // - 21 - 19  - 17 - 15

        // Convert 16bits to 32 bits vm0 (zero entended)
        __m128i vm0_low = _mm_unpacklo_epi16(vm0, zero_128); // - 10 - 8  - 2 - 0
        __m128i vm0_high = _mm_unpackhi_epi16(vm0, zero_128); // - 14 - 12  - 6  - 4

        // shift the value to aligned it with clut
        vm0_low = _mm_slli_epi32(vm0_low, 16); // 10 - 8 -  2 - 0 -
        vm0_high = _mm_slli_epi32(vm0_high, 16); // 14 - 12 -  6 - 4 -

        // Interlace old and new data
        clut_0 = _mm_or_si128(clut_0, vm0_low); // 10 5 8 3  2 1 0 x
        clut_2 = _mm_or_si128(clut_2, vm0_high); // 14 21 12 19  6 17 4 15

        // Save the result
        _mm_store_si128((__m128i*)(clut_word_ptr-1), clut_0);
        _mm_store_si128((__m128i*)(clut_word_ptr-1)+2, clut_2);

        // *** Same jobs for clut_1 and clut_3
        __m128i clut_1 = _mm_load_si128((__m128i*)(clut_word_ptr-1)+1);
        __m128i clut_3 = _mm_load_si128((__m128i*)(clut_word_ptr-1)+3);
        clut_1 = _mm_and_si128(clut_1, clut_mask);
        clut_3 = _mm_and_si128(clut_3, clut_mask);

        __m128i vm2_low = _mm_unpacklo_epi16(vm2, zero_128);
        __m128i vm2_high = _mm_unpackhi_epi16(vm2, zero_128);
        vm2_low = _mm_slli_epi32(vm2_low, 16);
        vm2_high = _mm_slli_epi32(vm2_high, 16);

        clut_1 = _mm_or_si128(clut_1, vm2_low);
        clut_3 = _mm_or_si128(clut_3, vm2_high);

        _mm_store_si128((__m128i*)(clut_word_ptr-1)+1, clut_1);
        _mm_store_si128((__m128i*)(clut_word_ptr-1)+3, clut_3);
    } else {
        // Standard write

        __m128i clut_mask = _mm_load_si128((__m128i*)s_clut16mask);

        // Load previous data and clear low 16 bits of double words
        __m128i clut_0 = _mm_and_si128(_mm_load_si128((__m128i*)clut), clut_mask); // 7 - 5 -  3 - 1 -
        __m128i clut_2 = _mm_and_si128(_mm_load_si128((__m128i*)clut+2), clut_mask); // 23 - 21 -  19 - 17 -

        //  Convert 16bits to 32 bits vm0 (zero entended)
        __m128i vm0_low = _mm_unpacklo_epi16(vm0, zero_128); // - 10 - 8  - 2 - 0
        __m128i vm0_high = _mm_unpackhi_epi16(vm0, zero_128); // - 14 - 12  - 6  - 4

        // Interlace old and new data
        clut_0 = _mm_or_si128(clut_0, vm0_low); // 7 10 5 8  3 2 1 0
        clut_2 = _mm_or_si128(clut_2, vm0_high); // 23 14 21 12  19 6 17 4

        // Save the result
        _mm_store_si128((__m128i*)clut, clut_0);
        _mm_store_si128((__m128i*)clut+2, clut_2);

        // *** Same jobs for clut_1 and clut_3
        __m128i clut_1 = _mm_and_si128(_mm_load_si128((__m128i*)clut+1), clut_mask);
        __m128i clut_3 = _mm_and_si128(_mm_load_si128((__m128i*)clut+3), clut_mask);

        __m128i vm2_low = _mm_unpacklo_epi16(vm2, zero_128);
        __m128i vm2_high = _mm_unpackhi_epi16(vm2, zero_128);

        clut_1 = _mm_or_si128(clut_1, vm2_low);
        clut_3 = _mm_or_si128(clut_3, vm2_high);

        _mm_store_si128((__m128i*)clut+1, clut_1);
        _mm_store_si128((__m128i*)clut+3, clut_3);
    }

#else
#if defined(_MSC_VER)
	__asm
	{
		mov eax, vm
		mov ecx, clut
		movdqa xmm0, qword ptr [eax]
		movdqa xmm1, qword ptr [eax+16]
		movdqa xmm2, qword ptr [eax+32]
		movdqa xmm3, qword ptr [eax+48]

		// rearrange
		pshuflw xmm0, xmm0, 0x88
		pshufhw xmm0, xmm0, 0x88
		pshuflw xmm1, xmm1, 0x88
		pshufhw xmm1, xmm1, 0x88
		pshuflw xmm2, xmm2, 0x88
		pshufhw xmm2, xmm2, 0x88
		pshuflw xmm3, xmm3, 0x88
		pshufhw xmm3, xmm3, 0x88

		shufps xmm0, xmm1, 0x88
		shufps xmm2, xmm3, 0x88

		pshufd xmm0, xmm0, 0xd8
		pshufd xmm2, xmm2, 0xd8

		pxor xmm6, xmm6

		test ecx, 15
		jnz WriteUnaligned

		movdqa xmm7, s_clut16mask // saves upper 16 bytes

		// have to save interlaced with the old data
		movdqa xmm4, [ecx]
		movdqa xmm5, [ecx+32]
		movhlps xmm1, xmm0
		movlhps xmm0, xmm2 // lower 8 colors

		pand xmm4, xmm7
		pand xmm5, xmm7

		shufps xmm1, xmm2, 0xe4 // upper 8 colors
		movdqa xmm2, xmm0
		movdqa xmm3, xmm1

		punpcklwd xmm0, xmm6
		punpcklwd xmm1, xmm6
		por xmm0, xmm4
		por xmm1, xmm5

		punpckhwd xmm2, xmm6
		punpckhwd xmm3, xmm6

		movdqa [ecx], xmm0
		movdqa [ecx+32], xmm1

		movdqa xmm5, xmm7
		pand xmm7, [ecx+16]
		pand xmm5, [ecx+48]

		por xmm2, xmm7
		por xmm3, xmm5

		movdqa [ecx+16], xmm2
		movdqa [ecx+48], xmm3
		jmp End

WriteUnaligned:
		// ecx is offset by 2
		sub ecx, 2

		movdqa xmm7, s_clut16mask2 // saves lower 16 bytes

		// have to save interlaced with the old data
		movdqa xmm4, [ecx]
		movdqa xmm5, [ecx+32]
		movhlps xmm1, xmm0
		movlhps xmm0, xmm2 // lower 8 colors

		pand xmm4, xmm7
		pand xmm5, xmm7

		shufps xmm1, xmm2, 0xe4 // upper 8 colors
		movdqa xmm2, xmm0
		movdqa xmm3, xmm1

		punpcklwd xmm0, xmm6
		punpcklwd xmm1, xmm6
		pslld xmm0, 16
		pslld xmm1, 16
		por xmm0, xmm4
		por xmm1, xmm5

		punpckhwd xmm2, xmm6
		punpckhwd xmm3, xmm6
		pslld xmm2, 16
		pslld xmm3, 16

		movdqa [ecx], xmm0
		movdqa [ecx+32], xmm1

		movdqa xmm5, xmm7
		pand xmm7, [ecx+16]
		pand xmm5, [ecx+48]

		por xmm2, xmm7
		por xmm3, xmm5

		movdqa [ecx+16], xmm2
		movdqa [ecx+48], xmm3

End:
	}
#else
	__asm__ __volatile__(".intel_syntax noprefix\n"
	"movdqa xmm0, xmmword ptr [%[vm]]\n"
	"movdqa xmm1, xmmword ptr [%[vm]+16]\n"
	"movdqa xmm2, xmmword ptr [%[vm]+32]\n"
	"movdqa xmm3, xmmword ptr [%[vm]+48]\n"

	// rearrange
	"pshuflw xmm0, xmm0, 0x88\n"
	"pshufhw xmm0, xmm0, 0x88\n"
	"pshuflw xmm1, xmm1, 0x88\n"
	"pshufhw xmm1, xmm1, 0x88\n"
	"pshuflw xmm2, xmm2, 0x88\n"
	"pshufhw xmm2, xmm2, 0x88\n"
	"pshuflw xmm3, xmm3, 0x88\n"
	"pshufhw xmm3, xmm3, 0x88\n"

	"shufps xmm0, xmm1, 0x88\n"
	"shufps xmm2, xmm3, 0x88\n"

	"pshufd xmm0, xmm0, 0xd8\n"
	"pshufd xmm2, xmm2, 0xd8\n"

	"pxor xmm6, xmm6\n"

	"test %[clut], 15\n"
	"jnz WriteUnaligned\n"

	"movdqa xmm7, %[s_clut16mask]\n" // saves upper 16 bits

	// have to save interlaced with the old data
	"movdqa xmm4, [%[clut]]\n"
	"movdqa xmm5, [%[clut]+32]\n"
	"movhlps xmm1, xmm0\n"
	"movlhps xmm0, xmm2\n"// lower 8 colors

	"pand xmm4, xmm7\n"
	"pand xmm5, xmm7\n"

	"shufps xmm1, xmm2, 0xe4\n" // upper 8 colors
	"movdqa xmm2, xmm0\n"
	"movdqa xmm3, xmm1\n"

	"punpcklwd xmm0, xmm6\n"
	"punpcklwd xmm1, xmm6\n"
	"por xmm0, xmm4\n"
	"por xmm1, xmm5\n"

	"punpckhwd xmm2, xmm6\n"
	"punpckhwd xmm3, xmm6\n"

	"movdqa [%[clut]], xmm0\n"
	"movdqa [%[clut]+32], xmm1\n"

	"movdqa xmm5, xmm7\n"
	"pand xmm7, [%[clut]+16]\n"
	"pand xmm5, [%[clut]+48]\n"

	"por xmm2, xmm7\n"
	"por xmm3, xmm5\n"

	"movdqa [%[clut]+16], xmm2\n"
	"movdqa [%[clut]+48], xmm3\n"
	"jmp WriteCLUT_T16_I4_CSM1_End\n"

	"WriteUnaligned:\n"
	// %[clut] is offset by 2
	"sub %[clut], 2\n"

	"movdqa xmm7, %[s_clut16mask2]\n" // saves lower 16 bits

	// have to save interlaced with the old data
	"movdqa xmm4, [%[clut]]\n"
	"movdqa xmm5, [%[clut]+32]\n"
	"movhlps xmm1, xmm0\n"
	"movlhps xmm0, xmm2\n" // lower 8 colors

	"pand xmm4, xmm7\n"
	"pand xmm5, xmm7\n"

	"shufps xmm1, xmm2, 0xe4\n" // upper 8 colors
	"movdqa xmm2, xmm0\n"
	"movdqa xmm3, xmm1\n"

	"punpcklwd xmm0, xmm6\n"
	"punpcklwd xmm1, xmm6\n"
	"pslld xmm0, 16\n"
	"pslld xmm1, 16\n"
	"por xmm0, xmm4\n"
	"por xmm1, xmm5\n"

	"punpckhwd xmm2, xmm6\n"
	"punpckhwd xmm3, xmm6\n"
	"pslld xmm2, 16\n"
	"pslld xmm3, 16\n"

	"movdqa [%[clut]], xmm0\n"
	"movdqa [%[clut]+32], xmm1\n"

	"movdqa xmm5, xmm7\n"
	"pand xmm7, [%[clut]+16]\n"
	"pand xmm5, [%[clut]+48]\n"

	"por xmm2, xmm7\n"
	"por xmm3, xmm5\n"

	"movdqa [%[clut]+16], xmm2\n"
	"movdqa [%[clut]+48], xmm3\n"
	"WriteCLUT_T16_I4_CSM1_End:\n"
	"\n"
	".att_syntax\n" 
    :
    : [vm] "r" (vm), [clut] "r" (clut), [s_clut16mask] "m" (*s_clut16mask), [s_clut16mask2] "m" (*s_clut16mask2)
    : "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7", "memory"
       );
#endif // _MSC_VER
#endif
}

__forceinline void  WriteCLUT_T16_I8_CSM1_sse2(u32* vm, u32 csa)
{
    // update the right clut column (csa < 16)
    u32* clut = (u32*)(g_pbyGSClut + 64*(csa & 15));
    u32 csa_right = (csa < 16) ? 16 - csa : 0;

    for(int i = (csa_right/2); i > 0 ; --i) {
        WriteCLUT_T16_I4_CSM1_core_sse2<true,false>(vm, clut);
        clut += 16;
        WriteCLUT_T16_I4_CSM1_core_sse2<true,true>(vm, clut);
        clut += 16;
        vm += 16; // go down one column
    }

    // update the left clut column
    u32 csa_left = (csa >= 16) ? 16 : csa;

    // In case csa_right is odd (so csa_left is also odd), we cross the clut column
    if(csa_right & 0x1) {
        WriteCLUT_T16_I4_CSM1_core_sse2<true,false>(vm, clut);
        // go back to the base before processing left clut column
        clut = (u32*)(g_pbyGSClut);
        WriteCLUT_T16_I4_CSM1_core_sse2<false,true>(vm, clut);
    } else if(csa_right != 0) {
        // go back to the base before processing left clut column
        clut = (u32*)(g_pbyGSClut);
    }

    for(int i = (csa_left/2); i > 0 ; --i) {
        WriteCLUT_T16_I4_CSM1_core_sse2<false,false>(vm, clut);
        clut += 16;
        WriteCLUT_T16_I4_CSM1_core_sse2<false,true>(vm, clut);
        clut += 16;
        vm += 16; // go down one column
    }
}

#endif // ZEROGS_SSE2

void __fastcall WriteCLUT_T16_I8_CSM1_c(u32* _vm, u32* _clut)
{
	const static u32 map[] =
	{
		0, 2, 8, 10, 16, 18, 24, 26,
		4, 6, 12, 14, 20, 22, 28, 30,
		1, 3, 9, 11, 17, 19, 25, 27,
		5, 7, 13, 15, 21, 23, 29, 31
	};

	u16* vm = (u16*)_vm;
	u16* clut = (u16*)_clut;

	int left = ((u32)(uptr)clut & 2) ? 512 : 512 - (((u32)(uptr)clut) & 0x3ff) / 2;

	for (int j = 0; j < 8; j++, vm += 32, clut += 64, left -= 32)
	{
		if (left == 32)
		{
			assert(left == 32);

			for (int i = 0; i < 16; i++)
				clut[2*i] = vm[map[i]];

			clut = (u16*)((uptr)clut & ~0x3ff) + 1;

			for (int i = 16; i < 32; i++)
				clut[2*i] = vm[map[i]];
		}
		else
		{
			if (left == 0)
			{
				clut = (u16*)((uptr)clut & ~0x3ff) + 1;
				left = -1;
			}

			for (int i = 0; i < 32; i++)
				clut[2*i] = vm[map[i]];
		}
	}
}

void __fastcall WriteCLUT_T32_I8_CSM1_c(u32* vm, u32* clut)
{
	u64* src = (u64*)vm;
	u64* dst = (u64*)clut;

	for (int j = 0; j < 2; j++, src += 32)
	{
		for (int i = 0; i < 4; i++, dst += 16, src += 8)
		{
			dst[0] = src[0];
			dst[1] = src[2];
			dst[2] = src[4];
			dst[3] = src[6];
			dst[4] = src[1];
			dst[5] = src[3];
			dst[6] = src[5];
			dst[7] = src[7];

			dst[8] = src[32];
			dst[9] = src[32+2];
			dst[10] = src[32+4];
			dst[11] = src[32+6];
			dst[12] = src[32+1];
			dst[13] = src[32+3];
			dst[14] = src[32+5];
			dst[15] = src[32+7];
		}
	}
}

void __fastcall WriteCLUT_T16_I4_CSM1_c(u32* _vm, u32* _clut)
{
	u16* dst = (u16*)_clut;
	u16* src = (u16*)_vm;

	dst[0] = src[0];
	dst[2] = src[2];
	dst[4] = src[8];
	dst[6] = src[10];
	dst[8] = src[16];
	dst[10] = src[18];
	dst[12] = src[24];
	dst[14] = src[26];
	dst[16] = src[4];
	dst[18] = src[6];
	dst[20] = src[12];
	dst[22] = src[14];
	dst[24] = src[20];
	dst[26] = src[22];
	dst[28] = src[28];
	dst[30] = src[30];
}

void __fastcall WriteCLUT_T32_I4_CSM1_c(u32* vm, u32* clut)
{
	u64* src = (u64*)vm;
	u64* dst = (u64*)clut;

	dst[0] = src[0];
	dst[1] = src[2];
	dst[2] = src[4];
	dst[3] = src[6];
	dst[4] = src[1];
	dst[5] = src[3];
	dst[6] = src[5];
	dst[7] = src[7];
}

void SSE2_UnswizzleZ16Target(u16* dst, u16* src, int iters)
{

#if defined(_MSC_VER)
	__asm
	{
		mov edx, iters
		pxor xmm7, xmm7
		mov eax, dst
		mov ecx, src

Z16Loop:
		// unpack 64 bytes at a time
		movdqa xmm0, [ecx]
		movdqa xmm2, [ecx+16]
		movdqa xmm4, [ecx+32]
		movdqa xmm6, [ecx+48]

		movdqa xmm1, xmm0
		movdqa xmm3, xmm2
		movdqa xmm5, xmm4

		punpcklwd xmm0, xmm7
		punpckhwd xmm1, xmm7
		punpcklwd xmm2, xmm7
		punpckhwd xmm3, xmm7

		// start saving
		movdqa [eax], xmm0
		movdqa [eax+16], xmm1

		punpcklwd xmm4, xmm7
		punpckhwd xmm5, xmm7

		movdqa [eax+32], xmm2
		movdqa [eax+48], xmm3

		movdqa xmm0, xmm6
		punpcklwd xmm6, xmm7

		movdqa [eax+64], xmm4
		movdqa [eax+80], xmm5

		punpckhwd xmm0, xmm7

		movdqa [eax+96], xmm6
		movdqa [eax+112], xmm0

		add ecx, 64
		add eax, 128
		sub edx, 1
		jne Z16Loop
	}
#else // _MSC_VER

	__asm__ __volatile__(".intel_syntax\n"
	"pxor %%xmm7, %%xmm7\n"

	"Z16Loop:\n"
	// unpack 64 bytes at a time
	"movdqa %%xmm0, [%[src]]\n"
	"movdqa %%xmm2, [%[src]+16]\n"
	"movdqa %%xmm4, [%[src]+32]\n"
	"movdqa %%xmm6, [%[src]+48]\n"

	"movdqa %%xmm1, %%xmm0\n"
	"movdqa %%xmm3, %%xmm2\n"
	"movdqa %%xmm5, %%xmm4\n"

	"punpcklwd %%xmm0, %%xmm7\n"
	"punpckhwd %%xmm1, %%xmm7\n"
	"punpcklwd %%xmm2, %%xmm7\n"
	"punpckhwd %%xmm3, %%xmm7\n"

	// start saving
	"movdqa [%[dst]], %%xmm0\n"
	"movdqa [%[dst]+16], %%xmm1\n"

	"punpcklwd %%xmm4, %%xmm7\n"
	"punpckhwd %%xmm5, %%xmm7\n"

	"movdqa [%[dst]+32], %%xmm2\n"
	"movdqa [%[dst]+48], %%xmm3\n"

	"movdqa %%xmm0, %%xmm6\n"
	"punpcklwd %%xmm6, %%xmm7\n"

	"movdqa [%[dst]+64], %%xmm4\n"
	"movdqa [%[dst]+80], %%xmm5\n"

	"punpckhwd %%xmm0, %%xmm7\n"

	"movdqa [%[dst]+96], %%xmm6\n"
	"movdqa [%[dst]+112], %%xmm0\n"

	"add %[src], 64\n"
	"add %[dst], 128\n"
	"sub %[iters], 1\n"
	"jne Z16Loop\n"

".att_syntax\n" 
    : "=&r"(src), "=&r"(dst), "=&r"(iters)
    : [src] "0"(src), [dst] "1"(dst), [iters] "2"(iters)
    : "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7", "memory"
       );
#endif // _MSC_VER
}

