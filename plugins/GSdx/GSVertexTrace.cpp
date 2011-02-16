/*
 *	Copyright (C) 2007-2009 Gabest
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

#pragma once

#include "stdafx.h"
#include "GSVertexTrace.h"
#include "GSUtil.h"
#include "GSState.h"

static const float s_fmin = -FLT_MAX;
static const float s_fmax = FLT_MAX;

GSVertexTrace::GSVertexTrace(const GSState* state)
	: m_state(state)
	, m_map_sw("VertexTraceSW", NULL)
	, m_map_hw9("VertexTraceHW9", NULL)
	, m_map_hw11("VertexTraceHW11", NULL)
{
}

uint32 GSVertexTrace::Hash(GS_PRIM_CLASS primclass)
{
	m_primclass = primclass;

	uint32 hash = m_primclass | (m_state->PRIM->IIP << 2) | (m_state->PRIM->TME << 3) | (m_state->PRIM->FST << 4);

	if(!(m_state->PRIM->TME && m_state->m_context->TEX0.TFX == TFX_DECAL && m_state->m_context->TEX0.TCC))
	{
		hash |= 1 << 5;
	}

	return hash;
}

void GSVertexTrace::Update(const GSVertexSW* v, int count, GS_PRIM_CLASS primclass)
{
	m_map_sw[Hash(primclass)](v, count, m_min, m_max);

	m_eq.value = (m_min.c == m_max.c).mask() | ((m_min.p == m_max.p).mask() << 16) | ((m_min.t == m_max.t).mask() << 20);

	m_alpha.valid = false;
}

void GSVertexTrace::Update(const GSVertexHW9* v, int count, GS_PRIM_CLASS primclass)
{
	m_map_hw9[Hash(primclass)](v, count, m_min, m_max);

	const GSDrawingContext* context = m_state->m_context;

	GSVector4 o(context->XYOFFSET);
	GSVector4 s(1.0f / 16, 1.0f / 16, 1.0f, 1.0f);

	m_min.p = (m_min.p - o) * s;
	m_max.p = (m_max.p - o) * s;

	if(m_state->PRIM->TME)
	{
		if(m_state->PRIM->FST)
		{
			s = GSVector4(1 << (16 - 4), 1).xxyy();
		}
		else
		{
			s = GSVector4(0x10000 << context->TEX0.TW, 0x10000 << context->TEX0.TH, 1, 1);
		}

		m_min.t *= s;
		m_max.t *= s;
	}

	m_eq.value = (m_min.c == m_max.c).mask() | ((m_min.p == m_max.p).mask() << 16) | ((m_min.t == m_max.t).mask() << 20);

	m_alpha.valid = false;
}

void GSVertexTrace::Update(const GSVertexHW11* v, int count, GS_PRIM_CLASS primclass)
{
	m_map_hw11[Hash(primclass)](v, count, m_min, m_max);

	const GSDrawingContext* context = m_state->m_context;

	GSVector4 o(context->XYOFFSET);
	GSVector4 s(1.0f / 16, 1.0f / 16, 2.0f, 1.0f);

	m_min.p = (m_min.p - o) * s;
	m_max.p = (m_max.p - o) * s;

	if(m_state->PRIM->TME)
	{
		if(m_state->PRIM->FST)
		{
			s = GSVector4(1 << (16 - 4), 1).xxyy();
		}
		else
		{
			s = GSVector4(0x10000 << context->TEX0.TW, 0x10000 << context->TEX0.TH, 1, 1);
		}

		m_min.t *= s;
		m_max.t *= s;
	}

	m_eq.value = (m_min.c == m_max.c).mask() | ((m_min.p == m_max.p).mask() << 16) | ((m_min.t == m_max.t).mask() << 20);

	m_alpha.valid = false;
}

using namespace Xbyak;

GSVertexTrace::CGSW::CGSW(const void* param, uint32 key, void* code, size_t maxsize)
	: GSCodeGenerator(code, maxsize)
{
	#if _M_AMD64
	#error TODO
	#endif

	const int params = 0;

	uint32 primclass = (key >> 0) & 3;
	uint32 iip = (key >> 2) & 1;
	uint32 tme = (key >> 3) & 1;
	uint32 fst = (key >> 4) & 1;
	uint32 color = (key >> 5) & 1;

	int n = 1;

	switch(primclass)
	{
	case GS_POINT_CLASS:
		n = 1;
		break;
	case GS_LINE_CLASS:
	case GS_SPRITE_CLASS:
		n = 2;
		break;
	case GS_TRIANGLE_CLASS:
		n = 3;
		break;
	}

	const int _v = params + 4;
	const int _count = params + 8;
	const int _min = params + 12;
	const int _max = params + 16;

	//

	if(m_cpu.has(util::Cpu::tAVX))
	{
		// min.p = FLT_MAX;
		// max.p = -FLT_MAX;

		vbroadcastss(xmm4, ptr[&s_fmax]);
		vbroadcastss(xmm5, ptr[&s_fmin]);

		if(color)
		{
			// min.c = FLT_MAX;
			// max.c = -FLT_MAX;

			vmovaps(xmm2, xmm4);
			vmovaps(xmm3, xmm5);
		}

		if(tme)
		{
			// min.t = FLT_MAX;
			// max.t = -FLT_MAX;

			vmovaps(xmm6, xmm4);
			vmovaps(xmm7, xmm5);
		}

		// for(int i = 0; i < count; i += step) {

		mov(edx, dword[esp + _v]);
		mov(ecx, dword[esp + _count]);

		align(16);

		L("loop");

		if(tme && !fst && primclass == GS_SPRITE_CLASS)
		{
			vmovaps(xmm1, ptr[edx + 1 * sizeof(GSVertexSW) + 32]);
			vshufps(xmm1, xmm1, _MM_SHUFFLE(2, 2, 2, 2));
		}

		for(int j = 0; j < n; j++)
		{
			if(color && (iip || j == n - 1))
			{
				// min.c = min.c.minv(v[i + j].c);
				// max.c = max.c.maxv(v[i + j].c);

				vmovaps(xmm0, ptr[edx + j * sizeof(GSVertexSW)]);

				vminps(xmm2, xmm0);
				vmaxps(xmm3, xmm0);
			}

			// min.p = min.p.minv(v[i + j].p);
			// max.p = max.p.maxv(v[i + j].p);

			vmovaps(xmm0, ptr[edx + j * sizeof(GSVertexSW) + 16]);

			vminps(xmm4, xmm0);
			vmaxps(xmm5, xmm0);

			if(tme)
			{
				// min.t = min.t.minv(v[i + j].t);
				// max.t = max.t.maxv(v[i + j].t);

				vmovaps(xmm0, ptr[edx + j * sizeof(GSVertexSW) + 32]);

				if(!fst)
				{
					if(primclass != GS_SPRITE_CLASS)
					{
						vmovaps(xmm1, xmm0);
						vshufps(xmm1, xmm1, _MM_SHUFFLE(2, 2, 2, 2));
					}

					vdivps(xmm0, xmm1);
					vshufps(xmm0, xmm1, _MM_SHUFFLE(3, 2, 1, 0));
				}

				vminps(xmm6, xmm0);
				vmaxps(xmm7, xmm0);
			}
		}

		add(edx, n * sizeof(GSVertexSW));
		sub(ecx, n);

		jg("loop");

		// }

		mov(eax, dword[esp + _min]);
		mov(edx, dword[esp + _max]);

		if(color)
		{
			vcvttps2dq(xmm2, xmm2);
			vpsrld(xmm2, 7);
			vmovaps(ptr[eax], xmm2);

			vcvttps2dq(xmm3, xmm3);
			vpsrld(xmm3, 7);
			vmovaps(ptr[edx], xmm3);
		}

		vmovaps(ptr[eax + 16], xmm4);
		vmovaps(ptr[edx + 16], xmm5);

		if(tme)
		{
			vmovaps(ptr[eax + 32], xmm6);
			vmovaps(ptr[edx + 32], xmm7);
		}
	}
	else
	{
		// min.p = FLT_MAX;
		// max.p = -FLT_MAX;

		movss(xmm4, ptr[&s_fmax]);
		movss(xmm5, ptr[&s_fmin]);

		shufps(xmm4, xmm4, _MM_SHUFFLE(0, 0, 0, 0));
		shufps(xmm5, xmm5, _MM_SHUFFLE(0, 0, 0, 0));

		if(color)
		{
			// min.c = FLT_MAX;
			// max.c = -FLT_MAX;

			movaps(xmm2, xmm4);
			movaps(xmm3, xmm5);
		}

		if(tme)
		{
			// min.t = FLT_MAX;
			// max.t = -FLT_MAX;

			movaps(xmm6, xmm4);
			movaps(xmm7, xmm5);
		}

		// for(int i = 0; i < count; i += step) {

		mov(edx, dword[esp + _v]);
		mov(ecx, dword[esp + _count]);

		align(16);

		L("loop");

		if(tme && !fst && primclass == GS_SPRITE_CLASS)
		{
			movaps(xmm1, ptr[edx + 1 * sizeof(GSVertexSW) + 32]);
			shufps(xmm1, xmm1, _MM_SHUFFLE(2, 2, 2, 2));
		}

		for(int j = 0; j < n; j++)
		{
			if(color && (iip || j == n - 1))
			{
				// min.c = min.c.minv(v[i + j].c);
				// max.c = max.c.maxv(v[i + j].c);

				movaps(xmm0, ptr[edx + j * sizeof(GSVertexSW)]);

				minps(xmm2, xmm0);
				maxps(xmm3, xmm0);
			}

			// min.p = min.p.minv(v[i + j].p);
			// max.p = max.p.maxv(v[i + j].p);

			movaps(xmm0, ptr[edx + j * sizeof(GSVertexSW) + 16]);

			minps(xmm4, xmm0);
			maxps(xmm5, xmm0);

			if(tme)
			{
				// min.t = min.t.minv(v[i + j].t);
				// max.t = max.t.maxv(v[i + j].t);

				movaps(xmm0, ptr[edx + j * sizeof(GSVertexSW) + 32]);

				if(!fst)
				{
					if(primclass != GS_SPRITE_CLASS)
					{
						movaps(xmm1, xmm0);
						shufps(xmm1, xmm1, _MM_SHUFFLE(2, 2, 2, 2));
					}

					divps(xmm0, xmm1);
					shufps(xmm0, xmm1, _MM_SHUFFLE(3, 2, 1, 0));
				}

				minps(xmm6, xmm0);
				maxps(xmm7, xmm0);
			}
		}

		add(edx, n * sizeof(GSVertexSW));
		sub(ecx, n);

		jg("loop");

		// }

		mov(eax, dword[esp + _min]);
		mov(edx, dword[esp + _max]);

		if(color)
		{
			cvttps2dq(xmm2, xmm2);
			psrld(xmm2, 7);
			movaps(ptr[eax], xmm2);

			cvttps2dq(xmm3, xmm3);
			psrld(xmm3, 7);
			movaps(ptr[edx], xmm3);
		}

		movaps(ptr[eax + 16], xmm4);
		movaps(ptr[edx + 16], xmm5);

		if(tme)
		{
			movaps(ptr[eax + 32], xmm6);
			movaps(ptr[edx + 32], xmm7);
		}
	}

	ret();
}

GSVertexTrace::CGHW9::CGHW9(const void* param, uint32 key, void* code, size_t maxsize)
	: GSCodeGenerator(code, maxsize)
{
	#if _M_AMD64
	#error TODO
	#endif

	const int params = 0;

	uint32 primclass = (key >> 0) & 3;
	uint32 iip = (key >> 2) & 1;
	uint32 tme = (key >> 3) & 1;
	uint32 fst = (key >> 4) & 1;
	uint32 color = (key >> 5) & 1;

	int n = 1;

	switch(primclass)
	{
	case GS_POINT_CLASS:
		n = 1;
		break;
	case GS_LINE_CLASS:
		n = 2;
		break;
	case GS_TRIANGLE_CLASS:
		n = 3;
		break;
	case GS_SPRITE_CLASS:
		n = 6;
		break;
	}

	const int _v = params + 4;
	const int _count = params + 8;
	const int _min = params + 12;
	const int _max = params + 16;

	//

	if(m_cpu.has(util::Cpu::tAVX))
	{
		// min.p = FLT_MAX;
		// max.p = -FLT_MAX;

		vbroadcastss(xmm4, ptr[&s_fmax]);
		vbroadcastss(xmm5, ptr[&s_fmin]);

		if(color)
		{
			// min.c = 0xffffffff;
			// max.c = 0;

			vpcmpeqd(xmm2, xmm2);
			vpxor(xmm3, xmm3);
		}

		if(tme)
		{
			// min.t = FLT_MAX;
			// max.t = -FLT_MAX;

			vmovaps(xmm6, xmm4);
			vmovaps(xmm7, xmm5);
		}

		// for(int i = 0; i < count; i += step) {

		mov(edx, dword[esp + _v]);
		mov(ecx, dword[esp + _count]);

		align(16);

		L("loop");

		if(tme && !fst && primclass == GS_SPRITE_CLASS)
		{
			vmovaps(xmm1, ptr[edx + 5 * sizeof(GSVertexHW9) + 16]);
			vshufps(xmm1, xmm1, _MM_SHUFFLE(3, 3, 3, 3));
		}

		for(int j = 0; j < n; j++)
		{
			// min.p = min.p.minv(v[i + j].p);
			// max.p = max.p.maxv(v[i + j].p);

			vmovaps(xmm0, ptr[edx + j * sizeof(GSVertexHW9) + 16]);

			vminps(xmm4, xmm0);
			vmaxps(xmm5, xmm0);

			if(tme && !fst && primclass != GS_SPRITE_CLASS)
			{
				vshufps(xmm1, xmm0, xmm0, _MM_SHUFFLE(3, 3, 3, 3));
			}

			if(color && (iip || j == n - 1) || tme)
			{
				vmovaps(xmm0, ptr[edx + j * sizeof(GSVertexHW9)]);
			}

			if(color && (iip || j == n - 1))
			{
				// min.c = min.c.min_u8(v[i + j].c);
				// max.c = max.c.min_u8(v[i + j].c);

				vpminub(xmm2, xmm0);
				vpmaxub(xmm3, xmm0);
			}

			if(tme)
			{
				vshufps(xmm0, xmm0, _MM_SHUFFLE(1, 0, 1, 0)); // avoid FP assist, high part is integral

				if(!fst)
				{
					// t /= p.wwww();

					vdivps(xmm0, xmm1);
				}

				// min.t = min.t.minv(v[i + j].t);
				// max.t = max.t.maxv(v[i + j].t);

				vminps(xmm6, xmm0);
				vmaxps(xmm7, xmm0);
			}
		}

		add(edx, n * sizeof(GSVertexHW9));
		sub(ecx, n);

		jg("loop");

		// }

		mov(eax, dword[esp + _min]);
		mov(edx, dword[esp + _max]);

		if(color)
		{
			// m_min.c = cmin.zzzz().u8to32();
			// m_max.c = cmax.zzzz().u8to32();

			if(m_cpu.has(util::Cpu::tSSE41))
			{
				vpshufd(xmm2, xmm2, _MM_SHUFFLE(2, 2, 2, 2));
				vpmovzxbd(xmm2, xmm2);

				vpshufd(xmm3, xmm3, _MM_SHUFFLE(2, 2, 2, 2));
				vpmovzxbd(xmm3, xmm3);
			}
			else
			{
				vpxor(xmm0, xmm0);

				vpunpckhbw(xmm2, xmm0);
				vpunpcklwd(xmm2, xmm0);

				vpunpckhbw(xmm3, xmm0);
				vpunpcklwd(xmm3, xmm0);
			}

			vmovaps(ptr[eax], xmm2);
			vmovaps(ptr[edx], xmm3);
		}

		// m_min.p = pmin;
		// m_max.p = pmax;

		vmovaps(ptr[eax + 16], xmm4);
		vmovaps(ptr[edx + 16], xmm5);

		if(tme)
		{
			// m_min.t = tmin.xyww(pmin);
			// m_max.t = tmax.xyww(pmax);

			vshufps(xmm6, xmm4, _MM_SHUFFLE(3, 3, 1, 0));
			vshufps(xmm7, xmm5, _MM_SHUFFLE(3, 3, 1, 0));

			vmovaps(ptr[eax + 32], xmm6);
			vmovaps(ptr[edx + 32], xmm7);
		}
	}
	else
	{
		// min.p = FLT_MAX;
		// max.p = -FLT_MAX;

		movss(xmm4, ptr[&s_fmax]);
		movss(xmm5, ptr[&s_fmin]);

		shufps(xmm4, xmm4, _MM_SHUFFLE(0, 0, 0, 0));
		shufps(xmm5, xmm5, _MM_SHUFFLE(0, 0, 0, 0));

		if(color)
		{
			// min.c = 0xffffffff;
			// max.c = 0;

			pcmpeqd(xmm2, xmm2);
			pxor(xmm3, xmm3);
		}

		if(tme)
		{
			// min.t = FLT_MAX;
			// max.t = -FLT_MAX;

			movaps(xmm6, xmm4);
			movaps(xmm7, xmm5);
		}

		// for(int i = 0; i < count; i += step) {

		mov(edx, dword[esp + _v]);
		mov(ecx, dword[esp + _count]);

		align(16);

		L("loop");

		if(tme && !fst && primclass == GS_SPRITE_CLASS)
		{
			movaps(xmm1, ptr[edx + 5 * sizeof(GSVertexHW9) + 16]);
			shufps(xmm1, xmm1, _MM_SHUFFLE(3, 3, 3, 3));
		}

		for(int j = 0; j < n; j++)
		{
			// min.p = min.p.minv(v[i + j].p);
			// max.p = max.p.maxv(v[i + j].p);

			movaps(xmm0, ptr[edx + j * sizeof(GSVertexHW9) + 16]);

			minps(xmm4, xmm0);
			maxps(xmm5, xmm0);

			if(tme && !fst && primclass != GS_SPRITE_CLASS)
			{
				movaps(xmm1, xmm0);
				shufps(xmm1, xmm1, _MM_SHUFFLE(3, 3, 3, 3));
			}

			if(color && (iip || j == n - 1) || tme)
			{
				movaps(xmm0, ptr[edx + j * sizeof(GSVertexHW9)]);
			}

			if(color && (iip || j == n - 1))
			{
				// min.c = min.c.min_u8(v[i + j].c);
				// max.c = max.c.min_u8(v[i + j].c);

				pminub(xmm2, xmm0);
				pmaxub(xmm3, xmm0);
			}

			if(tme)
			{
				shufps(xmm0, xmm0, _MM_SHUFFLE(1, 0, 1, 0)); // avoid FP assist, high part is integral

				if(!fst)
				{
					// t /= p.wwww();

					divps(xmm0, xmm1);
				}

				// min.t = min.t.minv(v[i + j].t);
				// max.t = max.t.maxv(v[i + j].t);

				minps(xmm6, xmm0);
				maxps(xmm7, xmm0);
			}
		}

		add(edx, n * sizeof(GSVertexHW9));
		sub(ecx, n);

		jg("loop");

		// }

		mov(eax, dword[esp + _min]);
		mov(edx, dword[esp + _max]);

		if(color)
		{
			// m_min.c = cmin.zzzz().u8to32();
			// m_max.c = cmax.zzzz().u8to32();

			if(m_cpu.has(util::Cpu::tSSE41))
			{
				pshufd(xmm2, xmm2, _MM_SHUFFLE(2, 2, 2, 2));
				pmovzxbd(xmm2, xmm2);

				pshufd(xmm3, xmm3, _MM_SHUFFLE(2, 2, 2, 2));
				pmovzxbd(xmm3, xmm3);
			}
			else
			{
				pxor(xmm0, xmm0);

				punpckhbw(xmm2, xmm0);
				punpcklwd(xmm2, xmm0);

				punpckhbw(xmm3, xmm0);
				punpcklwd(xmm3, xmm0);
			}

			movaps(ptr[eax], xmm2);
			movaps(ptr[edx], xmm3);
		}

		// m_min.p = pmin;
		// m_max.p = pmax;

		movaps(ptr[eax + 16], xmm4);
		movaps(ptr[edx + 16], xmm5);

		if(tme)
		{
			// m_min.t = tmin.xyww(pmin);
			// m_max.t = tmax.xyww(pmax);

			shufps(xmm6, xmm4, _MM_SHUFFLE(3, 3, 1, 0));
			shufps(xmm7, xmm5, _MM_SHUFFLE(3, 3, 1, 0));

			movaps(ptr[eax + 32], xmm6);
			movaps(ptr[edx + 32], xmm7);
		}
	}

	ret();
}

GSVertexTrace::CGHW11::CGHW11(const void* param, uint32 key, void* code, size_t maxsize)
	: GSCodeGenerator(code, maxsize)
{
	#if _M_AMD64
	#error TODO
	#endif

	const int params = 0;

	uint32 primclass = (key >> 0) & 3;
	uint32 iip = (key >> 2) & 1;
	uint32 tme = (key >> 3) & 1;
	uint32 fst = (key >> 4) & 1;
	uint32 color = (key >> 5) & 1;

	int n = 1;

	switch(primclass)
	{
	case GS_POINT_CLASS:
		n = 1;
		break;
	case GS_LINE_CLASS:
	case GS_SPRITE_CLASS:
		n = 2;
		break;
	case GS_TRIANGLE_CLASS:
		n = 3;
		break;
	}

	const int _v = params + 4;
	const int _count = params + 8;
	const int _min = params + 12;
	const int _max = params + 16;

	//

	if(m_cpu.has(util::Cpu::tAVX))
	{
		// min.p = FLT_MAX;
		// max.p = -FLT_MAX;

		vbroadcastss(xmm4, ptr[&s_fmax]);
		vbroadcastss(xmm5, ptr[&s_fmin]);

		if(color)
		{
			// min.c = 0xffffffff;
			// max.c = 0;

			vpcmpeqd(xmm2, xmm2);
			vpxor(xmm3, xmm3);
		}

		if(tme)
		{
			// min.t = FLT_MAX;
			// max.t = -FLT_MAX;

			vmovaps(xmm6, xmm4);
			vmovaps(xmm7, xmm5);
		}

		// for(int i = 0; i < count; i += step) {

		mov(edx, dword[esp + _v]);
		mov(ecx, dword[esp + _count]);

		align(16);

		L("loop");

		for(int j = 0; j < n; j++)
		{
			if(color && (iip || j == n - 1) || tme)
			{
				vmovaps(xmm0, ptr[edx + j * sizeof(GSVertexHW11)]);
			}

			if(color && (iip || j == n - 1))
			{
				vpminub(xmm2, xmm0);
				vpmaxub(xmm3, xmm0);
			}

			if(tme)
			{
				if(!fst)
				{
					vmovaps(xmm1, xmm0);
				}

				vshufps(xmm0, xmm0, _MM_SHUFFLE(3, 3, 1, 0)); // avoid FP assist, third dword is integral

				if(!fst)
				{
					vshufps(xmm1, xmm1, _MM_SHUFFLE(3, 3, 3, 3));
					vdivps(xmm0, xmm1);
					vshufps(xmm0, xmm1, _MM_SHUFFLE(3, 3, 1, 0)); // restore q
				}

				vminps(xmm6, xmm0);
				vmaxps(xmm7, xmm0);
			}

			vmovdqa(xmm0, ptr[edx + j * sizeof(GSVertexHW11) + 16]);

			if(m_cpu.has(util::Cpu::tSSE41))
			{
				vpmovzxwd(xmm1, xmm0);
			}
			else
			{
				vpunpcklwd(xmm1, xmm0, xmm0);
				vpsrld(xmm1, 16);
			}

			vpsrld(xmm0, 1);
			vpunpcklqdq(xmm1, xmm0);
			vcvtdq2ps(xmm1, xmm1);

			vminps(xmm4, xmm1);
			vmaxps(xmm5, xmm1);
		}

		add(edx, n * sizeof(GSVertexHW11));
		sub(ecx, n);

		jg("loop");

		// }

		mov(eax, dword[esp + _min]);
		mov(edx, dword[esp + _max]);

		if(color)
		{
			// m_min.c = cmin.zzzz().u8to32();
			// m_max.c = cmax.zzzz().u8to32();

			if(m_cpu.has(util::Cpu::tSSE41))
			{
				vpshufd(xmm2, xmm2, _MM_SHUFFLE(2, 2, 2, 2));
				vpmovzxbd(xmm2, xmm2);

				vpshufd(xmm3, xmm3, _MM_SHUFFLE(2, 2, 2, 2));
				vpmovzxbd(xmm3, xmm3);
			}
			else
			{
				vpxor(xmm0, xmm0);

				vpunpckhbw(xmm2, xmm0);
				vpunpcklwd(xmm2, xmm0);

				vpunpckhbw(xmm3, xmm0);
				vpunpcklwd(xmm3, xmm0);
			}

			vmovaps(ptr[eax], xmm2);
			vmovaps(ptr[edx], xmm3);
		}

		// m_min.p = pmin.xyww();
		// m_max.p = pmax.xyww();

		vshufps(xmm4, xmm4, _MM_SHUFFLE(3, 3, 1, 0));
		vshufps(xmm5, xmm5, _MM_SHUFFLE(3, 3, 1, 0));

		vmovaps(ptr[eax + 16], xmm4);
		vmovaps(ptr[edx + 16], xmm5);

		if(tme)
		{
			// m_min.t = tmin;
			// m_max.t = tmax;

			vmovaps(ptr[eax + 32], xmm6);
			vmovaps(ptr[edx + 32], xmm7);
		}
	}
	else
	{
		// min.p = FLT_MAX;
		// max.p = -FLT_MAX;

		movss(xmm4, ptr[&s_fmax]);
		movss(xmm5, ptr[&s_fmin]);

		shufps(xmm4, xmm4, _MM_SHUFFLE(0, 0, 0, 0));
		shufps(xmm5, xmm5, _MM_SHUFFLE(0, 0, 0, 0));

		if(color)
		{
			// min.c = 0xffffffff;
			// max.c = 0;

			pcmpeqd(xmm2, xmm2);
			pxor(xmm3, xmm3);
		}

		if(tme)
		{
			// min.t = FLT_MAX;
			// max.t = -FLT_MAX;

			movaps(xmm6, xmm4);
			movaps(xmm7, xmm5);
		}

		// for(int i = 0; i < count; i += step) {

		mov(edx, dword[esp + _v]);
		mov(ecx, dword[esp + _count]);

		align(16);

		L("loop");

		for(int j = 0; j < n; j++)
		{
			if(color && (iip || j == n - 1) || tme)
			{
				movaps(xmm0, ptr[edx + j * sizeof(GSVertexHW11)]);
			}

			if(color && (iip || j == n - 1))
			{
				pminub(xmm2, xmm0);
				pmaxub(xmm3, xmm0);
			}

			if(tme)
			{
				if(!fst)
				{
					movaps(xmm1, xmm0);
				}

				shufps(xmm0, xmm0, _MM_SHUFFLE(3, 3, 1, 0)); // avoid FP assist, third dword is integral

				if(!fst)
				{
					shufps(xmm1, xmm1, _MM_SHUFFLE(3, 3, 3, 3));
					divps(xmm0, xmm1);
					shufps(xmm0, xmm1, _MM_SHUFFLE(3, 3, 1, 0)); // restore q
				}

				minps(xmm6, xmm0);
				maxps(xmm7, xmm0);
			}

			movdqa(xmm0, ptr[edx + j * sizeof(GSVertexHW11) + 16]);

			if(m_cpu.has(util::Cpu::tSSE41))
			{
				pmovzxwd(xmm1, xmm0);
			}
			else
			{
				movdqa(xmm1, xmm0);
				punpcklwd(xmm1, xmm1);
				psrld(xmm1, 16);
			}

			psrld(xmm0, 1);
			punpcklqdq(xmm1, xmm0);
			cvtdq2ps(xmm1, xmm1);

			minps(xmm4, xmm1);
			maxps(xmm5, xmm1);
		}

		add(edx, n * sizeof(GSVertexHW11));
		sub(ecx, n);

		jg("loop");

		// }

		mov(eax, dword[esp + _min]);
		mov(edx, dword[esp + _max]);

		if(color)
		{
			// m_min.c = cmin.zzzz().u8to32();
			// m_max.c = cmax.zzzz().u8to32();

			if(m_cpu.has(util::Cpu::tSSE41))
			{
				pshufd(xmm2, xmm2, _MM_SHUFFLE(2, 2, 2, 2));
				pmovzxbd(xmm2, xmm2);

				pshufd(xmm3, xmm3, _MM_SHUFFLE(2, 2, 2, 2));
				pmovzxbd(xmm3, xmm3);
			}
			else
			{
				pxor(xmm0, xmm0);

				punpckhbw(xmm2, xmm0);
				punpcklwd(xmm2, xmm0);

				punpckhbw(xmm3, xmm0);
				punpcklwd(xmm3, xmm0);
			}

			movaps(ptr[eax], xmm2);
			movaps(ptr[edx], xmm3);
		}

		// m_min.p = pmin.xyww();
		// m_max.p = pmax.xyww();

		shufps(xmm4, xmm4, _MM_SHUFFLE(3, 3, 1, 0));
		shufps(xmm5, xmm5, _MM_SHUFFLE(3, 3, 1, 0));

		movaps(ptr[eax + 16], xmm4);
		movaps(ptr[edx + 16], xmm5);

		if(tme)
		{
			// m_min.t = tmin;
			// m_max.t = tmax;

			movaps(ptr[eax + 32], xmm6);
			movaps(ptr[edx + 32], xmm7);
		}
	}

	ret();
}
