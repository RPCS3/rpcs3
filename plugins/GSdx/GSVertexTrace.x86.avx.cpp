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

#include "stdafx.h"
#include "GSVertexTrace.h"

#if _M_SSE >= 0x500 && !(defined(_M_AMD64) || defined(_WIN64))

using namespace Xbyak;

static const int _args = 0;
static const int _count = _args + 4; // rcx
static const int _v = _args + 8; // rdx
static const int _min = _args + 12; // r8
static const int _max = _args + 16; // r9

GSVertexTrace::CGSW::CGSW(const void* param, uint32 key, void* code, size_t maxsize)
	: GSCodeGenerator(code, maxsize)
{
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

	// min.p = FLT_MAX;
	// max.p = -FLT_MAX;

	vbroadcastss(xmm4, ptr[&s_minmax.x]);
	vbroadcastss(xmm5, ptr[&s_minmax.y]);

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
		vmovaps(xmm1, ptr[edx + 1 * sizeof(GSVertexSW) + offsetof(GSVertexSW, t)]);
		vshufps(xmm1, xmm1, _MM_SHUFFLE(2, 2, 2, 2));
	}

	for(int j = 0; j < n; j++)
	{
		if(color && (iip || j == n - 1))
		{
			// min.c = min.c.minv(v[i + j].c);
			// max.c = max.c.maxv(v[i + j].c);

			vmovaps(xmm0, ptr[edx + j * sizeof(GSVertexSW) + offsetof(GSVertexSW, c)]);

			vminps(xmm2, xmm0);
			vmaxps(xmm3, xmm0);
		}

		// min.p = min.p.minv(v[i + j].p);
		// max.p = max.p.maxv(v[i + j].p);

		vmovaps(xmm0, ptr[edx + j * sizeof(GSVertexSW) + offsetof(GSVertexSW, p)]);

		vminps(xmm4, xmm0);
		vmaxps(xmm5, xmm0);

		if(tme)
		{
			// min.t = min.t.minv(v[i + j].t);
			// max.t = max.t.maxv(v[i + j].t);

			vmovaps(xmm0, ptr[edx + j * sizeof(GSVertexSW) + offsetof(GSVertexSW, t)]);

			if(!fst)
			{
				if(primclass != GS_SPRITE_CLASS)
				{
					vshufps(xmm1, xmm0, xmm0, _MM_SHUFFLE(2, 2, 2, 2));
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
		vmovaps(ptr[eax + offsetof(GSVertexTrace::Vertex, c)], xmm2);

		vcvttps2dq(xmm3, xmm3);
		vpsrld(xmm3, 7);
		vmovaps(ptr[edx + offsetof(GSVertexTrace::Vertex, c)], xmm3);
	}

	vmovaps(ptr[eax + offsetof(GSVertexTrace::Vertex, p)], xmm4);
	vmovaps(ptr[edx + offsetof(GSVertexTrace::Vertex, p)], xmm5);

	if(tme)
	{
		vmovaps(ptr[eax + offsetof(GSVertexTrace::Vertex, t)], xmm6);
		vmovaps(ptr[edx + offsetof(GSVertexTrace::Vertex, t)], xmm7);
	}

	ret();
}

GSVertexTrace::CGHW9::CGHW9(const void* param, uint32 key, void* code, size_t maxsize)
	: GSCodeGenerator(code, maxsize)
{
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

	// min.p = FLT_MAX;
	// max.p = -FLT_MAX;

	vbroadcastss(xmm4, ptr[&s_minmax.x]);
	vbroadcastss(xmm5, ptr[&s_minmax.y]);

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
		vmovaps(xmm1, ptr[edx + 5 * sizeof(GSVertexHW9) + offsetof(GSVertexHW9, p)]);
		vshufps(xmm1, xmm1, _MM_SHUFFLE(3, 3, 3, 3));
	}

	for(int j = 0; j < n; j++)
	{
		// min.p = min.p.minv(v[i + j].p);
		// max.p = max.p.maxv(v[i + j].p);

		vmovaps(xmm0, ptr[edx + j * sizeof(GSVertexHW9) + offsetof(GSVertexHW9, p)]);

		vminps(xmm4, xmm0);
		vmaxps(xmm5, xmm0);

		if(tme && !fst && primclass != GS_SPRITE_CLASS)
		{
			vshufps(xmm1, xmm0, xmm0, _MM_SHUFFLE(3, 3, 3, 3));
		}

		if(color && (iip || j == n - 1) || tme)
		{
			vmovaps(xmm0, ptr[edx + j * sizeof(GSVertexHW9) + offsetof(GSVertexHW9, t)]);
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

		vpshufd(xmm2, xmm2, _MM_SHUFFLE(2, 2, 2, 2));
		vpmovzxbd(xmm2, xmm2);

		vpshufd(xmm3, xmm3, _MM_SHUFFLE(2, 2, 2, 2));
		vpmovzxbd(xmm3, xmm3);

		vmovaps(ptr[eax + offsetof(GSVertexTrace::Vertex, c)], xmm2);
		vmovaps(ptr[edx + offsetof(GSVertexTrace::Vertex, c)], xmm3);
	}

	// m_min.p = pmin;
	// m_max.p = pmax;

	vmovaps(ptr[eax + offsetof(GSVertexTrace::Vertex, p)], xmm4);
	vmovaps(ptr[edx + offsetof(GSVertexTrace::Vertex, p)], xmm5);

	if(tme)
	{
		// m_min.t = tmin.xyww(pmin);
		// m_max.t = tmax.xyww(pmax);

		vshufps(xmm6, xmm4, _MM_SHUFFLE(3, 3, 1, 0));
		vshufps(xmm7, xmm5, _MM_SHUFFLE(3, 3, 1, 0));

		vmovaps(ptr[eax + offsetof(GSVertexTrace::Vertex, t)], xmm6);
		vmovaps(ptr[edx + offsetof(GSVertexTrace::Vertex, t)], xmm7);
	}

	ret();
}

GSVertexTrace::CGHW11::CGHW11(const void* param, uint32 key, void* code, size_t maxsize)
	: GSCodeGenerator(code, maxsize)
{
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

	// min.p = FLT_MAX;
	// max.p = -FLT_MAX;

	vbroadcastss(xmm4, ptr[&s_minmax.x]);
	vbroadcastss(xmm5, ptr[&s_minmax.y]);

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
		vpmovzxwd(xmm1, xmm0);

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

		vpshufd(xmm2, xmm2, _MM_SHUFFLE(2, 2, 2, 2));
		vpmovzxbd(xmm2, xmm2);

		vpshufd(xmm3, xmm3, _MM_SHUFFLE(2, 2, 2, 2));
		vpmovzxbd(xmm3, xmm3);

		vmovaps(ptr[eax + offsetof(GSVertexTrace::Vertex, c)], xmm2);
		vmovaps(ptr[edx + offsetof(GSVertexTrace::Vertex, c)], xmm3);
	}

	// m_min.p = pmin.xyww();
	// m_max.p = pmax.xyww();

	vshufps(xmm4, xmm4, _MM_SHUFFLE(3, 3, 1, 0));
	vshufps(xmm5, xmm5, _MM_SHUFFLE(3, 3, 1, 0));

	vmovaps(ptr[eax + offsetof(GSVertexTrace::Vertex, p)], xmm4);
	vmovaps(ptr[edx + offsetof(GSVertexTrace::Vertex, p)], xmm5);

	if(tme)
	{
		// m_min.t = tmin;
		// m_max.t = tmax;

		vmovaps(ptr[eax + offsetof(GSVertexTrace::Vertex, t)], xmm6);
		vmovaps(ptr[edx + offsetof(GSVertexTrace::Vertex, t)], xmm7);
	}

	ret();
}

#endif
