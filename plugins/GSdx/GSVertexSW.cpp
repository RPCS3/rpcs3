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
#include "GSVertexSW.h"

using namespace Xbyak;

GSVertexTrace::GSVertexTraceCodeGenerator::GSVertexTraceCodeGenerator(uint32 key, void* ptr, size_t maxsize)
	: CodeGenerator(maxsize, ptr)
{
	#if _M_AMD64
	#error TODO
	#endif

	const int params = 0;

	uint32 primclass = (key >> 0) & 3;
	uint32 iip = (key >> 2) & 1;
	uint32 tme = (key >> 3) & 1;
	uint32 color = (key >> 4) & 1;

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

	static const float fmin = -FLT_MAX;
	static const float fmax = FLT_MAX;

	movss(xmm0, xmmword[&fmax]);
	shufps(xmm0, xmm0, _MM_SHUFFLE(0, 0, 0, 0));

	movss(xmm1, xmmword[&fmin]);
	shufps(xmm1, xmm1, _MM_SHUFFLE(0, 0, 0, 0));

	if(color)
	{
		// min.c = FLT_MAX;
		// max.c = -FLT_MAX;

		movaps(xmm2, xmm0); 
		movaps(xmm3, xmm1); 
	}

	// min.p = FLT_MAX;
	// max.p = -FLT_MAX;

	movaps(xmm4, xmm0);
	movaps(xmm5, xmm1);
	
	if(tme)
	{
		// min.t = FLT_MAX;
		// max.t = -FLT_MAX;

		movaps(xmm6, xmm0);
		movaps(xmm7, xmm1);
	}

	// for(int i = 0; i < count; i += step) {

	mov(edx, dword[esp + _v]);
	mov(ecx, dword[esp + _count]);

	align(16);

L("loop");

	for(int j = 0; j < n; j++)
	{
		if(color && (iip || j == n - 1)) 
		{
			// min.c = min.c.minv(v[i + j].c); 
			// max.c = max.c.maxv(v[i + j].c);

			movaps(xmm0, xmmword[edx + j * sizeof(GSVertexSW)]);

			minps(xmm2, xmm0);
			maxps(xmm3, xmm0);
		}

		// min.p = min.p.minv(v[i + j].p); 
		// max.p = max.p.maxv(v[i + j].p);

		movaps(xmm0, xmmword[edx + j * sizeof(GSVertexSW) + 16]);

		minps(xmm4, xmm0);
		maxps(xmm5, xmm0);

		if(tme) 
		{
			// min.t = min.t.minv(v[i + j].t); 
			// max.t = max.t.maxv(v[i + j].t);

			movaps(xmm0, xmmword[edx + j * sizeof(GSVertexSW) + 32]);

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
		movaps(xmmword[eax], xmm2);
		movaps(xmmword[edx], xmm3);
	}

	movaps(xmmword[eax + 16], xmm4);
	movaps(xmmword[edx + 16], xmm5);

	if(tme)
	{
		movaps(xmmword[eax + 32], xmm6);
		movaps(xmmword[edx + 32], xmm7);
	}

	ret();
}
