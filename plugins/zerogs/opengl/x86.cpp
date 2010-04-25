/*  ZeroGS KOSMOS
 *  Copyright (C) 2005-2006 Gabest/zerofrog@gmail.com
 *  http://www.gabest.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "GS.h"
#include "Mem.h"
#include "x86.h"

#if defined(ZEROGS_SSE2) && defined(_WIN32)
#include <xmmintrin.h>
#include <emmintrin.h>
#endif

// swizzling

void __fastcall SwizzleBlock32_c(u8* dst, u8* src, int srcpitch, u32 WriteMask)
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

void __fastcall SwizzleBlock16_c(u8* dst, u8* src, int srcpitch)
{
	u32* d = &g_columnTable16[0][0];

	for(int j = 0; j < 8; j++, d += 16, src += srcpitch)
		for(int i = 0; i < 16; i++)
			((u16*)dst)[d[i]] = ((u16*)src)[i];
}

void __fastcall SwizzleBlock8_c(u8* dst, u8* src, int srcpitch)
{
	u32* d = &g_columnTable8[0][0];

	for(int j = 0; j < 16; j++, d += 16, src += srcpitch)
		for(int i = 0; i < 16; i++)
			dst[d[i]] = src[i];
}

void __fastcall SwizzleBlock4_c(u8* dst, u8* src, int srcpitch)
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

#define _FrameSwizzleBlock(type, transfer, transfer16, incsrc) \
/* FrameSwizzleBlock32 */ \
void __fastcall FrameSwizzleBlock32##type##c(u32* dst, u32* src, int srcpitch, u32 WriteMask) \
{ \
	u32* d = &g_columnTable32[0][0]; \
	\
	if( WriteMask == 0xffffffff ) { \
		for(int i = 0; i < 8; ++i, d += 8) { \
			for(int j = 0; j < 8; ++j) { \
				dst[d[j]] = (transfer); \
			} \
			src += srcpitch << incsrc; \
		} \
	} \
	else { \
		for(int i = 0; i < 8; ++i, d += 8) { \
			for(int j = 0; j < 8; ++j) { \
				dst[d[j]] = ((transfer)&WriteMask)|(dst[d[j]]&~WriteMask); \
			} \
			src += srcpitch << incsrc; \
		} \
	} \
} \
\
/* FrameSwizzleBlock16 */ \
void __fastcall FrameSwizzleBlock16##type##c(u16* dst, u32* src, int srcpitch, u32 WriteMask) \
{ \
	u32* d = &g_columnTable16[0][0]; \
	\
	if( WriteMask == 0xffff ) {  \
		for(int i = 0; i < 8; ++i, d += 16) { \
			for(int j = 0; j < 16; ++j) { \
				u32 temp = (transfer); \
				dst[d[j]] = RGBA32to16(temp); \
			} \
			src += srcpitch << incsrc; \
		} \
	} \
	else { \
		for(int i = 0; i < 8; ++i, d += 16) { \
			for(int j = 0; j < 16; ++j) { \
			u32 temp = (transfer); \
				u32 dsrc = RGBA32to16(temp); \
				dst[d[j]] = (dsrc&WriteMask)|(dst[d[j]]&~WriteMask); \
			} \
			src += srcpitch << incsrc; \
		} \
	} \
} \
\
/* Frame16SwizzleBlock32 */ \
void __fastcall Frame16SwizzleBlock32##type##c(u32* dst, Vector_16F* src, int srcpitch, u32 WriteMask) \
{ \
	u32* d = &g_columnTable32[0][0]; \
\
	if( WriteMask == 0xffffffff ) {  \
		for(int i = 0; i < 8; ++i, d += 8) { \
			for(int j = 0; j < 8; ++j) { \
				Vector_16F dsrc16 = (transfer16); \
				dst[d[j]] = Float16ToARGB(dsrc16); \
			} \
			src += srcpitch << incsrc; \
		} \
	} \
	else { \
		for(int i = 0; i < 8; ++i, d += 8) { \
			for(int j = 0; j < 8; ++j) { \
				Vector_16F dsrc16 = (transfer16); \
				u32 dsrc = Float16ToARGB(dsrc16); \
				dst[d[j]] = (dsrc&WriteMask)|(dst[d[j]]&~WriteMask); \
			} \
			src += srcpitch << incsrc; \
		} \
	} \
 } \
\
/* Frame16SwizzleBlock32Z */ \
void __fastcall Frame16SwizzleBlock32Z##type##c(u32* dst, Vector_16F* src, int srcpitch, u32 WriteMask) \
{ \
	u32* d = &g_columnTable32[0][0]; \
	if( WriteMask == 0xffffffff ) { /* breaks KH text if not checked */ \
		for(int i = 0; i < 8; ++i, d += 8) { \
			for(int j = 0; j < 8; ++j) { \
				Vector_16F dsrc16 = (transfer16); \
				dst[d[j]] = Float16ToARGB_Z(dsrc16); \
			} \
			src += srcpitch << incsrc; \
		} \
	} \
	else { \
		for(int i = 0; i < 8; ++i, d += 8) { \
			for(int j = 0; j < 8; ++j) { \
				Vector_16F dsrc16 = (transfer16); \
				u32 dsrc = Float16ToARGB_Z(dsrc16); \
				dst[d[j]] = (dsrc&WriteMask)|(dst[d[j]]&~WriteMask); \
			} \
			src += srcpitch << incsrc; \
		} \
	} \
 } \
 \
 /* Frame16SwizzleBlock16 */ \
void __fastcall Frame16SwizzleBlock16##type##c(u16* dst, Vector_16F* src, int srcpitch, u32 WriteMask) \
{ \
	u32* d = &g_columnTable16[0][0]; \
	\
	if( (WriteMask&0xfff8f8f8) == 0xfff8f8f8) {  \
		for(int i = 0; i < 8; ++i, d += 16) { \
			for(int j = 0; j < 16; ++j) { \
				Vector_16F dsrc16 = (transfer16); \
				dst[d[j]] = Float16ToARGB16(dsrc16); \
			} \
			src += srcpitch << incsrc; \
		} \
	} \
	else { \
		for(int i = 0; i < 8; ++i, d += 16) { \
			for(int j = 0; j < 16; ++j) { \
				Vector_16F dsrc16 = (transfer16); \
				u32 dsrc = Float16ToARGB16(dsrc16); \
				dst[d[j]] = (dsrc&WriteMask)|(dst[d[j]]&~WriteMask); \
			} \
			src += srcpitch << incsrc; \
		} \
	} \
 } \
 \
 /* Frame16SwizzleBlock16Z */ \
void __fastcall Frame16SwizzleBlock16Z##type##c(u16* dst, Vector_16F* src, int srcpitch, u32 WriteMask) \
{ \
	u32* d = &g_columnTable16[0][0]; \
	\
	for(int i = 0; i < 8; ++i, d += 16) { \
		for(int j = 0; j < 16; ++j) { \
			Vector_16F dsrc16 = (transfer16); \
			dst[d[j]] = Float16ToARGB16_Z(dsrc16); \
		} \
		src += srcpitch << incsrc; \
	} \
} \

_FrameSwizzleBlock(_, src[j], src[j], 0);
_FrameSwizzleBlock(A2_, (src[2*j]+src[2*j+1])>>1, src[2*j], 0);
_FrameSwizzleBlock(A4_, (src[2*j]+src[2*j+1]+src[2*j+srcpitch]+src[2*j+srcpitch+1])>>2, src[2*j], 1);

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

#if defined(_WIN32)

extern "C" void __fastcall WriteCLUT_T32_I8_CSM1_sse2(u32* vm, u32* clut)
{
	__m128i* src = (__m128i*)vm;
	__m128i* dst = (__m128i*)clut;

	for(int j = 0; j < 64; j += 32, src += 32, dst += 32)
	{
		for(int i = 0; i < 16; i += 4)
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
#endif

#if defined(_MSC_VER)

extern "C" {
PCSX2_ALIGNED16(int s_clut16mask2[4]) = { 0x0000ffff, 0x0000ffff, 0x0000ffff, 0x0000ffff };
PCSX2_ALIGNED16(int s_clut16mask[8]) = { 0xffff0000, 0xffff0000, 0xffff0000, 0xffff0000,
										0x0000ffff, 0x0000ffff, 0x0000ffff, 0x0000ffff};
}

extern "C" void __fastcall WriteCLUT_T16_I4_CSM1_sse2(u32* vm, u32* clut)
{
	__asm {
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

		movdqa xmm7, s_clut16mask // saves upper 16 bits

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

		movdqa xmm7, s_clut16mask2 // saves lower 16 bits

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
}
#endif // _MSC_VER

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

	int left = ((u32)(uptr)clut&2) ? 512 : 512-(((u32)(uptr)clut)&0x3ff)/2;

	for(int j = 0; j < 8; j++, vm += 32, clut += 64, left -= 32)
	{
		if(left == 32) {
			assert( left == 32 );
			for(int i = 0; i < 16; i++)
				clut[2*i] = vm[map[i]];

			clut = (u16*)((uptr)clut & ~0x3ff) + 1;

			for(int i = 16; i < 32; i++)
				clut[2*i] = vm[map[i]];
		}
		else {
			if( left == 0 ) {
				clut = (u16*)((uptr)clut & ~0x3ff) + 1;
				left = -1;
			}

			for(int i = 0; i < 32; i++)
				clut[2*i] = vm[map[i]];
		}
	}
}

void __fastcall WriteCLUT_T32_I8_CSM1_c(u32* vm, u32* clut)
{
	u64* src = (u64*)vm;
	u64* dst = (u64*)clut;

	for(int j = 0; j < 2; j++, src += 32) {
		for(int i = 0; i < 4; i++, dst+=16, src+=8)
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

	dst[0] = src[0]; dst[2] = src[2];
	dst[4] = src[8]; dst[6] = src[10];
	dst[8] = src[16]; dst[10] = src[18];
	dst[12] = src[24]; dst[14] = src[26];
	dst[16] = src[4]; dst[18] = src[6];
	dst[20] = src[12]; dst[22] = src[14];
	dst[24] = src[20]; dst[26] = src[22];
	dst[28] = src[28]; dst[30] = src[30];
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
