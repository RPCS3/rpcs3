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

#if _M_SSE < 0x500 && (defined(_M_AMD64) || defined(_WIN64))

using namespace Xbyak;

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

	enter(32, true);

	movdqa(ptr[rsp + 0], xmm6);
	movdqa(ptr[rsp + 16], xmm7);

	// min.p = FLT_MAX;
	// max.p = -FLT_MAX;

	mov(rax, (size_t)&s_minmax);

	movss(xmm4, ptr[rax + 0]);
	movss(xmm5, ptr[rax + 4]);

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

	align(16);

	L("loop");

	if(tme && !fst && primclass == GS_SPRITE_CLASS)
	{
		movaps(xmm1, ptr[rdx + 1 * sizeof(GSVertexSW) + 32]);
		shufps(xmm1, xmm1, _MM_SHUFFLE(2, 2, 2, 2));
	}

	for(int j = 0; j < n; j++)
	{
		if(color && (iip || j == n - 1))
		{
			// min.c = min.c.minv(v[i + j].c);
			// max.c = max.c.maxv(v[i + j].c);

			movaps(xmm0, ptr[rdx + j * sizeof(GSVertexSW)]);

			minps(xmm2, xmm0);
			maxps(xmm3, xmm0);
		}

		// min.p = min.p.minv(v[i + j].p);
		// max.p = max.p.maxv(v[i + j].p);

		movaps(xmm0, ptr[rdx + j * sizeof(GSVertexSW) + 16]);

		minps(xmm4, xmm0);
		maxps(xmm5, xmm0);

		if(tme)
		{
			// min.t = min.t.minv(v[i + j].t);
			// max.t = max.t.maxv(v[i + j].t);

			movaps(xmm0, ptr[rdx + j * sizeof(GSVertexSW) + 32]);

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

	add(rdx, n * sizeof(GSVertexSW));
	sub(rcx, n);

	jg("loop");

	// }

	if(color)
	{
		cvttps2dq(xmm2, xmm2);
		psrld(xmm2, 7);
		movaps(ptr[r8], xmm2);

		cvttps2dq(xmm3, xmm3);
		psrld(xmm3, 7);
		movaps(ptr[r9], xmm3);
	}

	movaps(ptr[r8 + 16], xmm4);
	movaps(ptr[r9 + 16], xmm5);

	if(tme)
	{
		movaps(ptr[r8 + 32], xmm6);
		movaps(ptr[r9 + 32], xmm7);
	}

	movdqa(xmm6, ptr[rsp + 0]);
	movdqa(xmm7, ptr[rsp + 16]);

	leave();

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

	enter(32, true);

	movdqa(ptr[rsp + 0], xmm6);
	movdqa(ptr[rsp + 16], xmm7);

	// min.p = FLT_MAX;
	// max.p = -FLT_MAX;

	mov(rax, (size_t)&s_minmax);

	movss(xmm4, ptr[rax + 0]);
	movss(xmm5, ptr[rax + 16]);

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

	align(16);

	L("loop");

	if(tme && !fst && primclass == GS_SPRITE_CLASS)
	{
		movaps(xmm1, ptr[rdx + 5 * sizeof(GSVertexHW9) + 16]);
		shufps(xmm1, xmm1, _MM_SHUFFLE(3, 3, 3, 3));
	}

	for(int j = 0; j < n; j++)
	{
		// min.p = min.p.minv(v[i + j].p);
		// max.p = max.p.maxv(v[i + j].p);

		movaps(xmm0, ptr[rdx + j * sizeof(GSVertexHW9) + 16]);

		minps(xmm4, xmm0);
		maxps(xmm5, xmm0);

		if(tme && !fst && primclass != GS_SPRITE_CLASS)
		{
			movaps(xmm1, xmm0);
			shufps(xmm1, xmm1, _MM_SHUFFLE(3, 3, 3, 3));
		}

		if(color && (iip || j == n - 1) || tme)
		{
			movaps(xmm0, ptr[rdx + j * sizeof(GSVertexHW9)]);
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

	add(rdx, n * sizeof(GSVertexHW9));
	sub(ecx, n);

	jg("loop");

	// }

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

		movaps(ptr[r8], xmm2);
		movaps(ptr[r9], xmm3);
	}

	// m_min.p = pmin;
	// m_max.p = pmax;

	movaps(ptr[r8 + 16], xmm4);
	movaps(ptr[r9 + 16], xmm5);

	if(tme)
	{
		// m_min.t = tmin.xyww(pmin);
		// m_max.t = tmax.xyww(pmax);

		shufps(xmm6, xmm4, _MM_SHUFFLE(3, 3, 1, 0));
		shufps(xmm7, xmm5, _MM_SHUFFLE(3, 3, 1, 0));

		movaps(ptr[r8 + 32], xmm6);
		movaps(ptr[r9 + 32], xmm7);
	}

	movdqa(xmm6, ptr[rsp + 0]);
	movdqa(xmm7, ptr[rsp + 16]);

	leave();

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

	enter(32, true);

	movdqa(ptr[rsp + 0], xmm6);
	movdqa(ptr[rsp + 16], xmm7);

	// min.p = FLT_MAX;
	// max.p = -FLT_MAX;

	mov(rax, (size_t)&s_minmax);

	movss(xmm4, ptr[rax + 0]);
	movss(xmm5, ptr[rax + 16]);

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

	align(16);

	L("loop");

	for(int j = 0; j < n; j++)
	{
		if(color && (iip || j == n - 1) || tme)
		{
			movaps(xmm0, ptr[rdx + j * sizeof(GSVertexHW11)]);
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

		movdqa(xmm0, ptr[rdx + j * sizeof(GSVertexHW11) + 16]);

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

	add(rdx, n * sizeof(GSVertexHW11));
	sub(ecx, n);

	jg("loop");

	// }

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

		movaps(ptr[r8], xmm2);
		movaps(ptr[r9], xmm3);
	}

	// m_min.p = pmin.xyww();
	// m_max.p = pmax.xyww();

	shufps(xmm4, xmm4, _MM_SHUFFLE(3, 3, 1, 0));
	shufps(xmm5, xmm5, _MM_SHUFFLE(3, 3, 1, 0));

	movaps(ptr[r8 + 16], xmm4);
	movaps(ptr[r9 + 16], xmm5);

	if(tme)
	{
		// m_min.t = tmin;
		// m_max.t = tmax;

		movaps(ptr[r8 + 32], xmm6);
		movaps(ptr[r9 + 32], xmm7);
	}

	movdqa(xmm6, ptr[rsp + 0]);
	movdqa(xmm7, ptr[rsp + 16]);

	leave();

	ret();
}

#endif
