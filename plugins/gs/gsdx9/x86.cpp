/* 
 *	Copyright (C) 2003-2005 Gabest
 *	http://www.gabest.org
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

#include "stdafx.h"
#include "GSTables.h"
#include "x86.h"

// unswizzling

void __fastcall unSwizzleBlock32_c(BYTE* src, BYTE* dst, int dstpitch)
{
	DWORD* s = &columnTable32[0][0];

	for(int j = 0; j < 8; j++, s += 8, dst += dstpitch)
		for(int i = 0; i < 8; i++)
			((DWORD*)dst)[i] = ((DWORD*)src)[s[i]];
}

void __fastcall unSwizzleBlock16_c(BYTE* src, BYTE* dst, int dstpitch)
{
	DWORD* s = &columnTable16[0][0];

	for(int j = 0; j < 8; j++, s += 16, dst += dstpitch)
		for(int i = 0; i < 16; i++)
			((WORD*)dst)[i] = ((WORD*)src)[s[i]];
}

void __fastcall unSwizzleBlock8_c(BYTE* src, BYTE* dst, int dstpitch)
{
	DWORD* s = &columnTable8[0][0];

	for(int j = 0; j < 16; j++, s += 16, dst += dstpitch)
		for(int i = 0; i < 16; i++)
			dst[i] = src[s[i]];
}

void __fastcall unSwizzleBlock4_c(BYTE* src, BYTE* dst, int dstpitch)
{
	DWORD* s = &columnTable4[0][0];

	for(int j = 0; j < 16; j++, s += 32, dst += dstpitch)
	{
		for(int i = 0; i < 32; i++)
		{
			DWORD addr = s[i];
			BYTE c = (src[addr>>1] >> ((addr&1) << 2)) & 0x0f;
			int shift = (i&1) << 2;
			dst[i >> 1] = (dst[i >> 1] & (0xf0 >> shift)) | (c << shift);
		}
	}
}

void __fastcall unSwizzleBlock8HP_c(BYTE* src, BYTE* dst, int dstpitch)
{
	DWORD* s = &columnTable32[0][0];

	for(int j = 0; j < 8; j++, s += 8, dst += dstpitch)
		for(int i = 0; i < 8; i++)
			dst[i] = (BYTE)(((DWORD*)src)[s[i]]>>24);
}

void __fastcall unSwizzleBlock4HLP_c(BYTE* src, BYTE* dst, int dstpitch)
{
	DWORD* s = &columnTable32[0][0];

	for(int j = 0; j < 8; j++, s += 8, dst += dstpitch)
		for(int i = 0; i < 8; i++)
			dst[i] = (BYTE)(((DWORD*)src)[s[i]]>>24)&0xf;
}

void __fastcall unSwizzleBlock4HHP_c(BYTE* src, BYTE* dst, int dstpitch)
{
	DWORD* s = &columnTable32[0][0];

	for(int j = 0; j < 8; j++, s += 8, dst += dstpitch)
		for(int i = 0; i < 8; i++)
			dst[i] = (BYTE)(((DWORD*)src)[s[i]]>>28);
}

void __fastcall unSwizzleBlock4P_c(BYTE* src, BYTE* dst, int dstpitch)
{
	DWORD* s = &columnTable4[0][0];

	for(int j = 0; j < 16; j++, s += 32, dst += dstpitch)
	{
		for(int i = 0; i < 32; i++)
		{
			DWORD addr = s[i];
			dst[i] = (src[addr>>1] >> ((addr&1) << 2)) & 0x0f;
		}
	}
}

// swizzling

void __fastcall SwizzleBlock32_c(BYTE* dst, BYTE* src, int srcpitch, DWORD WriteMask)
{
	DWORD* d = &columnTable32[0][0];

	if(WriteMask == 0xffffffff)
	{
		for(int j = 0; j < 8; j++, d += 8, src += srcpitch)
			for(int i = 0; i < 8; i++)
				((DWORD*)dst)[d[i]] = ((DWORD*)src)[i];
	}
	else
	{
		for(int j = 0; j < 8; j++, d += 8, src += srcpitch)
			for(int i = 0; i < 8; i++)
				((DWORD*)dst)[d[i]] = (((DWORD*)dst)[d[i]] & ~WriteMask) | (((DWORD*)src)[i] & WriteMask);
	}
}

void __fastcall SwizzleBlock16_c(BYTE* dst, BYTE* src, int srcpitch)
{
	DWORD* d = &columnTable16[0][0];

	for(int j = 0; j < 8; j++, d += 16, src += srcpitch)
		for(int i = 0; i < 16; i++)
			((WORD*)dst)[d[i]] = ((WORD*)src)[i];
}

void __fastcall SwizzleBlock8_c(BYTE* dst, BYTE* src, int srcpitch)
{
	DWORD* d = &columnTable8[0][0];

	for(int j = 0; j < 16; j++, d += 16, src += srcpitch)
		for(int i = 0; i < 16; i++)
			dst[d[i]] = src[i];
}

void __fastcall SwizzleBlock4_c(BYTE* dst, BYTE* src, int srcpitch)
{
	DWORD* d = &columnTable4[0][0];

	for(int j = 0; j < 16; j++, d += 32, src += srcpitch)
	{
		for(int i = 0; i < 32; i++)
		{
			DWORD addr = d[i];
			BYTE c = (src[i>>1] >> ((i&1) << 2)) & 0x0f;
			DWORD shift = (addr&1) << 2;
			dst[addr >> 1] = (dst[addr >> 1] & (0xf0 >> shift)) | (c << shift);
		}
	}
}

// column swizzling (TODO: sse2)

void __fastcall SwizzleColumn32_c(int y, BYTE* dst, BYTE* src, int srcpitch, DWORD WriteMask)
{
	DWORD* d = &columnTable32[((y/2)&3)*2][0];

	if(WriteMask == 0xffffffff)
	{
		for(int j = 0; j < 2; j++, d += 8, src += srcpitch)
			for(int i = 0; i < 8; i++)
				((DWORD*)dst)[d[i]] = ((DWORD*)src)[i];
	}
	else
	{
		for(int j = 0; j < 2; j++, d += 8, src += srcpitch)
			for(int i = 0; i < 8; i++)
				((DWORD*)dst)[d[i]] = (((DWORD*)dst)[d[i]] & ~WriteMask) | (((DWORD*)src)[i] & WriteMask);
	}
}

void __fastcall SwizzleColumn16_c(int y, BYTE* dst, BYTE* src, int srcpitch)
{
	DWORD* d = &columnTable16[((y/2)&3)*2][0];

	for(int j = 0; j < 2; j++, d += 16, src += srcpitch)
		for(int i = 0; i < 16; i++)
			((WORD*)dst)[d[i]] = ((WORD*)src)[i];
}

void __fastcall SwizzleColumn8_c(int y, BYTE* dst, BYTE* src, int srcpitch)
{
	DWORD* d = &columnTable8[((y/4)&3)*4][0];

	for(int j = 0; j < 4; j++, d += 16, src += srcpitch)
		for(int i = 0; i < 16; i++)
			dst[d[i]] = src[i];
}

void __fastcall SwizzleColumn4_c(int y, BYTE* dst, BYTE* src, int srcpitch)
{
	DWORD* d = &columnTable4[y&(3<<2)][0]; // ((y/4)&3)*4

	for(int j = 0; j < 4; j++, d += 32, src += srcpitch)
	{
		for(int i = 0; i < 32; i++)
		{
			DWORD addr = d[i];
			BYTE c = (src[i>>1] >> ((i&1) << 2)) & 0x0f;
			DWORD shift = (addr&1) << 2;
			dst[addr >> 1] = (dst[addr >> 1] & (0xf0 >> shift)) | (c << shift);
		}
	}
}

//

#if defined(_M_AMD64) || _M_IX86_FP >= 2

static __m128i s_zero = _mm_setzero_si128();
static __m128i s_bgrm = _mm_set1_epi32(0x00ffffff);
static __m128i s_am = _mm_set1_epi32(0x00008000);
static __m128i s_bm = _mm_set1_epi32(0x00007c00);
static __m128i s_gm = _mm_set1_epi32(0x000003e0);
static __m128i s_rm = _mm_set1_epi32(0x0000001f);

void __fastcall ExpandBlock24_sse2(DWORD* src, DWORD* dst, int dstpitch, GIFRegTEXA* pTEXA)
{
	__m128i TA0 = _mm_set1_epi32((DWORD)pTEXA->TA0 << 24);

	if(!pTEXA->AEM)
	{
		for(int j = 0; j < 8; j++, src += 8, dst += dstpitch>>2)
		{
			for(int i = 0; i < 8; i += 4)
			{
				__m128i c = _mm_load_si128((__m128i*)&src[i]);
				c = _mm_and_si128(c, s_bgrm);
				c = _mm_or_si128(c, TA0);
				_mm_store_si128((__m128i*)&dst[i], c);
			}
		}
	}
	else
	{
		for(int j = 0; j < 8; j++, src += 8, dst += dstpitch>>2)
		{
			for(int i = 0; i < 8; i += 4)
			{
				__m128i c = _mm_load_si128((__m128i*)&src[i]);
				c = _mm_and_si128(c, s_bgrm);
				__m128i a = _mm_andnot_si128(_mm_cmpeq_epi32(c, s_zero), TA0);
				c = _mm_or_si128(c, a);
				_mm_store_si128((__m128i*)&dst[i], c);
			}
		}
	}
}

void __fastcall ExpandBlock16_sse2(WORD* src, DWORD* dst, int dstpitch, GIFRegTEXA* pTEXA)
{
	__m128i TA0 = _mm_set1_epi32((DWORD)pTEXA->TA0 << 24);
	__m128i TA1 = _mm_set1_epi32((DWORD)pTEXA->TA1 << 24);
	__m128i a, b, g, r;

	if(!pTEXA->AEM)
	{
		for(int j = 0; j < 8; j++, src += 16, dst += dstpitch>>2)
		{
			for(int i = 0; i < 16; i += 8)
			{
				__m128i c = _mm_load_si128((__m128i*)&src[i]);

				__m128i cl = _mm_unpacklo_epi16(c, s_zero);
				__m128i ch = _mm_unpackhi_epi16(c, s_zero);

				__m128i alm = _mm_cmplt_epi32(cl, s_am);
				__m128i ahm = _mm_cmplt_epi32(ch, s_am);

				// lo

				b = _mm_slli_epi32(_mm_and_si128(cl, s_bm), 9);
				g = _mm_slli_epi32(_mm_and_si128(cl, s_gm), 6);
				r = _mm_slli_epi32(_mm_and_si128(cl, s_rm), 3);
				a = _mm_or_si128(_mm_and_si128(alm, TA0), _mm_andnot_si128(alm, TA1));

				cl = _mm_or_si128(_mm_or_si128(a, b), _mm_or_si128(g, r));

				_mm_store_si128((__m128i*)&dst[i], cl);

				// hi

				b = _mm_slli_epi32(_mm_and_si128(ch, s_bm), 9);
				g = _mm_slli_epi32(_mm_and_si128(ch, s_gm), 6);
				r = _mm_slli_epi32(_mm_and_si128(ch, s_rm), 3);
				a = _mm_or_si128(_mm_and_si128(ahm, TA0), _mm_andnot_si128(ahm, TA1));

				ch = _mm_or_si128(_mm_or_si128(a, b), _mm_or_si128(g, r));

				_mm_store_si128((__m128i*)&dst[i+4], ch);
			}
		}
	}
	else
	{
		for(int j = 0; j < 8; j++, src += 16, dst += dstpitch>>2)
		{
			for(int i = 0; i < 16; i += 8)
			{
				__m128i c = _mm_load_si128((__m128i*)&src[i]);

				__m128i cl = _mm_unpacklo_epi16(c, s_zero);
				__m128i ch = _mm_unpackhi_epi16(c, s_zero);

				__m128i alm = _mm_cmplt_epi32(cl, s_am);
				__m128i ahm = _mm_cmplt_epi32(ch, s_am);

				__m128i trm = _mm_cmpeq_epi16(c, s_zero);
				__m128i trlm = _mm_unpacklo_epi16(trm, trm);
				__m128i trhm = _mm_unpackhi_epi16(trm, trm);

				// lo

				b = _mm_slli_epi32(_mm_and_si128(cl, s_bm), 9);
				g = _mm_slli_epi32(_mm_and_si128(cl, s_gm), 6);
				r = _mm_slli_epi32(_mm_and_si128(cl, s_rm), 3);
				a = _mm_or_si128(_mm_and_si128(alm, TA0), _mm_andnot_si128(alm, TA1));
				a = _mm_andnot_si128(trlm, a);

				cl = _mm_or_si128(_mm_or_si128(a, b), _mm_or_si128(g, r));

				_mm_store_si128((__m128i*)&dst[i], cl);

				// hi

				b = _mm_slli_epi32(_mm_and_si128(ch, s_bm), 9);
				g = _mm_slli_epi32(_mm_and_si128(ch, s_gm), 6);
				r = _mm_slli_epi32(_mm_and_si128(ch, s_rm), 3);
				a = _mm_or_si128(_mm_and_si128(ahm, TA0), _mm_andnot_si128(ahm, TA1));
				a = _mm_andnot_si128(trhm, a);

				ch = _mm_or_si128(_mm_or_si128(a, b), _mm_or_si128(g, r));

				_mm_store_si128((__m128i*)&dst[i+4], ch);
			}
		}
	}
}

void __fastcall Expand16_sse2(WORD* src, DWORD* dst, int w, GIFRegTEXA* pTEXA)
{
	ASSERT(!(w&7));

	__m128i TA0 = _mm_set1_epi32((DWORD)pTEXA->TA0 << 24);
	__m128i TA1 = _mm_set1_epi32((DWORD)pTEXA->TA1 << 24);
	__m128i a, b, g, r;

	if(!pTEXA->AEM)
	{
		for(int i = 0; i < w; i += 8)
		{
			__m128i c = _mm_load_si128((__m128i*)&src[i]);

			__m128i cl = _mm_unpacklo_epi16(c, s_zero);
			__m128i ch = _mm_unpackhi_epi16(c, s_zero);

			__m128i alm = _mm_cmplt_epi32(cl, s_am);
			__m128i ahm = _mm_cmplt_epi32(ch, s_am);

			// lo

			b = _mm_slli_epi32(_mm_and_si128(cl, s_bm), 9);
			g = _mm_slli_epi32(_mm_and_si128(cl, s_gm), 6);
			r = _mm_slli_epi32(_mm_and_si128(cl, s_rm), 3);
			a = _mm_or_si128(_mm_and_si128(alm, TA0), _mm_andnot_si128(alm, TA1));

			cl = _mm_or_si128(_mm_or_si128(a, b), _mm_or_si128(g, r));

			_mm_store_si128((__m128i*)&dst[i], cl);

			// hi

			b = _mm_slli_epi32(_mm_and_si128(ch, s_bm), 9);
			g = _mm_slli_epi32(_mm_and_si128(ch, s_gm), 6);
			r = _mm_slli_epi32(_mm_and_si128(ch, s_rm), 3);
			a = _mm_or_si128(_mm_and_si128(ahm, TA0), _mm_andnot_si128(ahm, TA1));

			ch = _mm_or_si128(_mm_or_si128(a, b), _mm_or_si128(g, r));

			_mm_store_si128((__m128i*)&dst[i+4], ch);
		}
	}
	else
	{
		for(int i = 0; i < w; i += 8)
		{
			__m128i c = _mm_load_si128((__m128i*)&src[i]);

			__m128i cl = _mm_unpacklo_epi16(c, s_zero);
			__m128i ch = _mm_unpackhi_epi16(c, s_zero);

			__m128i alm = _mm_cmplt_epi32(cl, s_am);
			__m128i ahm = _mm_cmplt_epi32(ch, s_am);

			__m128i trm = _mm_cmpeq_epi16(c, s_zero);
			__m128i trlm = _mm_unpacklo_epi16(trm, trm);
			__m128i trhm = _mm_unpackhi_epi16(trm, trm);

			// lo

			b = _mm_slli_epi32(_mm_and_si128(cl, s_bm), 9);
			g = _mm_slli_epi32(_mm_and_si128(cl, s_gm), 6);
			r = _mm_slli_epi32(_mm_and_si128(cl, s_rm), 3);
			a = _mm_or_si128(_mm_and_si128(alm, TA0), _mm_andnot_si128(alm, TA1));
			a = _mm_andnot_si128(trlm, a);

			cl = _mm_or_si128(_mm_or_si128(a, b), _mm_or_si128(g, r));

			_mm_store_si128((__m128i*)&dst[i], cl);

			// hi

			b = _mm_slli_epi32(_mm_and_si128(ch, s_bm), 9);
			g = _mm_slli_epi32(_mm_and_si128(ch, s_gm), 6);
			r = _mm_slli_epi32(_mm_and_si128(ch, s_rm), 3);
			a = _mm_or_si128(_mm_and_si128(ahm, TA0), _mm_andnot_si128(ahm, TA1));
			a = _mm_andnot_si128(trhm, a);

			ch = _mm_or_si128(_mm_or_si128(a, b), _mm_or_si128(g, r));

			_mm_store_si128((__m128i*)&dst[i+4], ch);
		}
	}
}

#endif

void __fastcall ExpandBlock24_c(DWORD* src, DWORD* dst, int dstpitch, GIFRegTEXA* pTEXA)
{
	DWORD TA0 = (DWORD)pTEXA->TA0 << 24;

	if(!pTEXA->AEM)
	{
		for(int j = 0; j < 8; j++, src += 8, dst += dstpitch>>2)
			for(int i = 0; i < 8; i++)
				dst[i] = TA0 | (src[i]&0xffffff);
	}
	else
	{
		for(int j = 0; j < 8; j++, src += 8, dst += dstpitch>>2)
			for(int i = 0; i < 8; i++)
				dst[i] = ((src[i]&0xffffff) ? TA0 : 0) | (src[i]&0xffffff);
	}
}

void __fastcall ExpandBlock16_c(WORD* src, DWORD* dst, int dstpitch, GIFRegTEXA* pTEXA)
{
	DWORD TA0 = (DWORD)pTEXA->TA0 << 24;
	DWORD TA1 = (DWORD)pTEXA->TA1 << 24;

	if(!pTEXA->AEM)
	{
		for(int j = 0; j < 8; j++, src += 16, dst += dstpitch>>2)
			for(int i = 0; i < 16; i++)
				dst[i] = ((src[i]&0x8000) ? TA1 : TA0)
					| ((src[i]&0x7c00) << 9) | ((src[i]&0x03e0) << 6) | ((src[i]&0x001f) << 3);
	}
	else
	{
		for(int j = 0; j < 8; j++, src += 16, dst += dstpitch>>2)
			for(int i = 0; i < 16; i++)
				dst[i] = ((src[i]&0x8000) ? TA1 : src[i] ? TA0 : 0)
					| ((src[i]&0x7c00) << 9) | ((src[i]&0x03e0) << 6) | ((src[i]&0x001f) << 3);
	}
}

void __fastcall Expand16_c(WORD* src, DWORD* dst, int w, GIFRegTEXA* pTEXA)
{
	DWORD TA0 = (DWORD)pTEXA->TA0 << 24;
	DWORD TA1 = (DWORD)pTEXA->TA1 << 24;

	if(!pTEXA->AEM)
	{
		for(int i = 0; i < w; i++)
			dst[i] = ((src[i]&0x8000) ? TA1 : TA0)
				| ((src[i]&0x7c00) << 9) | ((src[i]&0x03e0) << 6) | ((src[i]&0x001f) << 3);
	}
	else
	{
		for(int i = 0; i < w; i++)
			dst[i] = ((src[i]&0x8000) ? TA1 : src[i] ? TA0 : 0)
				| ((src[i]&0x7c00) << 9) | ((src[i]&0x03e0) << 6) | ((src[i]&0x001f) << 3);
	}
}

//

#if defined(_M_AMD64) || _M_IX86_FP >= 2

static __m128 s_uvmin = _mm_set1_ps(+1e10);
static __m128 s_uvmax = _mm_set1_ps(-1e10);

void __fastcall UVMinMax_sse2(int nVertices, vertex_t* pVertices, uvmm_t* uv)
{
	__m128 uvmin = s_uvmin;
	__m128 uvmax = s_uvmax;

	__m128* p = (__m128*)pVertices + 1;

	int i = 0;

	nVertices -= 5;

	for(; i < nVertices; i += 6) // 6 regs for loading, 2 regs for min/max
	{
		uvmin = _mm_min_ps(uvmin, p[(i+0)*2]);
		uvmax = _mm_max_ps(uvmax, p[(i+0)*2]);
		uvmin = _mm_min_ps(uvmin, p[(i+1)*2]);
		uvmax = _mm_max_ps(uvmax, p[(i+1)*2]);
		uvmin = _mm_min_ps(uvmin, p[(i+2)*2]);
		uvmax = _mm_max_ps(uvmax, p[(i+2)*2]);
		uvmin = _mm_min_ps(uvmin, p[(i+3)*2]);
		uvmax = _mm_max_ps(uvmax, p[(i+3)*2]);
		uvmin = _mm_min_ps(uvmin, p[(i+4)*2]);
		uvmax = _mm_max_ps(uvmax, p[(i+4)*2]);
		uvmin = _mm_min_ps(uvmin, p[(i+5)*2]);
		uvmax = _mm_max_ps(uvmax, p[(i+5)*2]);
	}

	nVertices += 5;

	for(; i < nVertices; i++)
	{
		uvmin = _mm_min_ps(uvmin, p[i*2]);
		uvmax = _mm_max_ps(uvmax, p[i*2]);
	}

	_mm_storeh_pi((__m64*)uv, uvmin);
	_mm_storeh_pi((__m64*)uv + 1, uvmax);
}

#endif

void __fastcall UVMinMax_c(int nVertices, vertex_t* pVertices, uvmm_t* uv)
{
	uv->umin = uv->vmin = +1e10;
	uv->umax = uv->vmax = -1e10;

	for(; nVertices-- > 0; pVertices++)
	{
		float u = pVertices->u;
		if(uv->umax < u) uv->umax = u;
		if(uv->umin > u) uv->umin = u;
		float v = pVertices->v;
		if(uv->vmax < v) uv->vmax = v;
		if(uv->vmin > v) uv->vmin = v;
	}
}

#if defined(_M_AMD64) || _M_IX86_FP >= 2

static __m128i s_clut[64];

void __fastcall WriteCLUT_T16_I8_CSM1_sse2(WORD* vm, WORD* clut)
{
	__m128i* src = (__m128i*)vm;
	__m128i* dst = (__m128i*)clut;

	for(int i = 0; i < 32; i += 4)
	{
		__m128i r0 = _mm_load_si128(&src[i+0]);
		__m128i r1 = _mm_load_si128(&src[i+1]);
		__m128i r2 = _mm_load_si128(&src[i+2]);
		__m128i r3 = _mm_load_si128(&src[i+3]);

		__m128i r4 = _mm_unpacklo_epi16(r0, r1);
		__m128i r5 = _mm_unpackhi_epi16(r0, r1);
		__m128i r6 = _mm_unpacklo_epi16(r2, r3);
		__m128i r7 = _mm_unpackhi_epi16(r2, r3);

		r0 = _mm_unpacklo_epi32(r4, r6);
		r1 = _mm_unpackhi_epi32(r4, r6);
		r2 = _mm_unpacklo_epi32(r5, r7);
		r3 = _mm_unpackhi_epi32(r5, r7);

		r4 = _mm_unpacklo_epi16(r0, r1);
		r5 = _mm_unpackhi_epi16(r0, r1);
		r6 = _mm_unpacklo_epi16(r2, r3);
		r7 = _mm_unpackhi_epi16(r2, r3);

		_mm_store_si128(&dst[i+0], r4);
		_mm_store_si128(&dst[i+1], r6);
		_mm_store_si128(&dst[i+2], r5);
		_mm_store_si128(&dst[i+3], r7);
	}
}

void __fastcall WriteCLUT_T32_I8_CSM1_sse2(DWORD* vm, WORD* clut)
{
	__m128i* src = (__m128i*)vm;
	__m128i* dst = s_clut;

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

	for(int i = 0; i < 32; i++)
	{
		__m128i r0 = s_clut[i*2];
		__m128i r1 = s_clut[i*2+1];
		__m128i r2 = _mm_unpacklo_epi16(r0, r1);
		__m128i r3 = _mm_unpackhi_epi16(r0, r1);
		r0 = _mm_unpacklo_epi16(r2, r3);
		r1 = _mm_unpackhi_epi16(r2, r3);
		r2 = _mm_unpacklo_epi16(r0, r1);
		r3 = _mm_unpackhi_epi16(r0, r1);
		_mm_store_si128(&((__m128i*)clut)[i], r2);
		_mm_store_si128(&((__m128i*)clut)[i+32], r3);
	}
}

void __fastcall WriteCLUT_T16_I4_CSM1_sse2(WORD* vm, WORD* clut)
{
	// TODO (probably not worth, _c is going to be just as fast)
	WriteCLUT_T16_I4_CSM1_c(vm, clut);
}

void __fastcall WriteCLUT_T32_I4_CSM1_sse2(DWORD* vm, WORD* clut)
{
	__m128i* src = (__m128i*)vm;
	__m128i* dst = s_clut;

	__m128i r0 = _mm_load_si128(&src[0]);
	__m128i r1 = _mm_load_si128(&src[1]);
	__m128i r2 = _mm_load_si128(&src[2]);
	__m128i r3 = _mm_load_si128(&src[3]);

	_mm_store_si128(&dst[0], _mm_unpacklo_epi64(r0, r1));
	_mm_store_si128(&dst[1], _mm_unpacklo_epi64(r2, r3));
	_mm_store_si128(&dst[2], _mm_unpackhi_epi64(r0, r1));
	_mm_store_si128(&dst[3], _mm_unpackhi_epi64(r2, r3));

	for(int i = 0; i < 2; i++)
	{
		__m128i r0 = s_clut[i*2];
		__m128i r1 = s_clut[i*2+1];
		__m128i r2 = _mm_unpacklo_epi16(r0, r1);
		__m128i r3 = _mm_unpackhi_epi16(r0, r1);
		r0 = _mm_unpacklo_epi16(r2, r3);
		r1 = _mm_unpackhi_epi16(r2, r3);
		r2 = _mm_unpacklo_epi16(r0, r1);
		r3 = _mm_unpackhi_epi16(r0, r1);
		_mm_store_si128(&((__m128i*)clut)[i], r2);
		_mm_store_si128(&((__m128i*)clut)[i+32], r3);
	}
}

#endif

void __fastcall WriteCLUT_T16_I8_CSM1_c(WORD* vm, WORD* clut)
{
	const static DWORD map[] = 
	{
		0, 2, 8, 10, 16, 18, 24, 26,
		4, 6, 12, 14, 20, 22, 28, 30,
		1, 3, 9, 11, 17, 19, 25, 27, 
		5, 7, 13, 15, 21, 23, 29, 31
	};

	for(int j = 0; j < 8; j++, vm += 32, clut += 32) 
	{
		for(int i = 0; i < 32; i++)
		{
			clut[i] = vm[map[i]];
		}
	}
}

void __fastcall WriteCLUT_T32_I8_CSM1_c(DWORD* vm, WORD* clut)
{
	const static DWORD map[] = 
	{
		0, 1, 4, 5, 8, 9, 12, 13, 2, 3, 6, 7, 10, 11, 14, 15, 
		64, 65, 68, 69, 72, 73, 76, 77, 66, 67, 70, 71, 74, 75, 78, 79, 
		16, 17, 20, 21, 24, 25, 28, 29, 18, 19, 22, 23, 26, 27, 30, 31, 
		80, 81, 84, 85, 88, 89, 92, 93, 82, 83, 86, 87, 90, 91, 94, 95, 
		32, 33, 36, 37, 40, 41, 44, 45, 34, 35, 38, 39, 42, 43, 46, 47, 
		96, 97, 100, 101, 104, 105, 108, 109, 98, 99, 102, 103, 106, 107, 110, 111, 
		48, 49, 52, 53, 56, 57, 60, 61, 50, 51, 54, 55, 58, 59, 62, 63, 
		112, 113, 116, 117, 120, 121, 124, 125, 114, 115, 118, 119, 122, 123, 126, 127
	};

	for(int j = 0; j < 2; j++, vm += 128, clut += 128)
	{
		for(int i = 0; i < 128; i++) 
		{
			DWORD dw = vm[map[i]];
			clut[i] = (WORD)(dw & 0xffff);
			clut[i+256] = (WORD)(dw >> 16);
		}
	}
}

void __fastcall WriteCLUT_T16_I4_CSM1_c(WORD* vm, WORD* clut)
{
	const static DWORD map[] = 
	{
		0, 2, 8, 10, 16, 18, 24, 26,
		4, 6, 12, 14, 20, 22, 28, 30
	};

	for(int i = 0; i < 16; i++) 
	{
		clut[i] = vm[map[i]];
	}
}

void __fastcall WriteCLUT_T32_I4_CSM1_c(DWORD* vm, WORD* clut)
{
	const static DWORD map[] = 
	{
		0, 1, 4, 5, 8, 9, 12, 13,
		2, 3, 6, 7, 10, 11, 14, 15
	};

	for(int i = 0; i < 16; i++) 
	{
		DWORD dw = vm[map[i]];
		clut[i] = (WORD)(dw & 0xffff);
		clut[i+256] = (WORD)(dw >> 16);
	}
}

//

#if defined(_M_AMD64) || _M_IX86_FP >= 2

extern "C" void __fastcall ReadCLUT32_T32_I8_sse2(WORD* src, DWORD* dst)
{
	for(int i = 0; i < 256; i += 16)
	{
		ReadCLUT32_T32_I4_sse2(&src[i], &dst[i]); // going to be inlined nicely
	}
}

extern "C" void __fastcall ReadCLUT32_T32_I4_sse2(WORD* src, DWORD* dst)
{
	__m128i r0 = ((__m128i*)src)[0];
	__m128i r1 = ((__m128i*)src)[1];
	__m128i r2 = ((__m128i*)src)[0+32];
	__m128i r3 = ((__m128i*)src)[1+32];
	_mm_store_si128(&((__m128i*)dst)[0], _mm_unpacklo_epi16(r0, r2));
	_mm_store_si128(&((__m128i*)dst)[1], _mm_unpackhi_epi16(r0, r2));
	_mm_store_si128(&((__m128i*)dst)[2], _mm_unpacklo_epi16(r1, r3));
	_mm_store_si128(&((__m128i*)dst)[3], _mm_unpackhi_epi16(r1, r3));
}

extern "C" void __fastcall ReadCLUT32_T16_I8_sse2(WORD* src, DWORD* dst)
{
	for(int i = 0; i < 256; i += 16)
	{
		ReadCLUT32_T16_I4_sse2(&src[i], &dst[i]);
	}
}

extern "C" void __fastcall ReadCLUT32_T16_I4_sse2(WORD* src, DWORD* dst)
{
	__m128i r0 = ((__m128i*)src)[0];
	__m128i r1 = ((__m128i*)src)[1];
	_mm_store_si128(&((__m128i*)dst)[0], _mm_unpacklo_epi16(r0, s_zero));
	_mm_store_si128(&((__m128i*)dst)[1], _mm_unpackhi_epi16(r0, s_zero));
	_mm_store_si128(&((__m128i*)dst)[2], _mm_unpacklo_epi16(r1, s_zero));
	_mm_store_si128(&((__m128i*)dst)[3], _mm_unpackhi_epi16(r1, s_zero));
}

#endif

void __fastcall ReadCLUT32_T32_I8_c(WORD* src, DWORD* dst)
{
	for(int i = 0; i < 256; i++)
	{
		dst[i] = ((DWORD)src[i+256] << 16) | src[i];
	}
}

void __fastcall ReadCLUT32_T32_I4_c(WORD* src, DWORD* dst)
{
	for(int i = 0; i < 16; i++)
	{
		dst[i] = ((DWORD)src[i+256] << 16) | src[i];
	}
}

void __fastcall ReadCLUT32_T16_I8_c(WORD* src, DWORD* dst)
{
	for(int i = 0; i < 256; i++)
	{
		dst[i] = (DWORD)src[i];
	}
}

void __fastcall ReadCLUT32_T16_I4_c(WORD* src, DWORD* dst)
{
	for(int i = 0; i < 16; i++)
	{
		dst[i] = (DWORD)src[i];
	}
}

//