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

// TODO: x64

#include "StdAfx.h"
#include "GSVertexSW.h"
#include "GPUSetupPrimCodeGenerator.h"

GPUSetupPrimCodeGenerator::GPUSetupPrimCodeGenerator(GPUScanlineEnvironment& env, void* ptr, size_t maxsize)
	: CodeGenerator(maxsize, ptr)
	, m_env(env)
{
	#if _M_AMD64
	#error TODO
	#endif

	Generate();
}

void GPUSetupPrimCodeGenerator::Generate()
{
	const int params = 0;

	const int _vertices = params + 4;
	const int _dscan = params + 8;

	mov(ecx, dword[esp + _vertices]);
	mov(edx, dword[esp + _dscan]);

	if(m_env.sel.tme && !m_env.sel.twin)
	{
		pcmpeqd(xmm0, xmm0);

		if(m_env.sel.sprite)
		{
			// t = (GSVector4i(vertices[1].t) >> 8) - GSVector4i::x00000001();

			cvttps2dq(xmm1, xmmword[ecx + sizeof(GSVertexSW) * 1 + 32]);
			psrld(xmm1, 8);
			psrld(xmm0, 31);
			psubd(xmm1, xmm0);

			// t = t.ps32(t);
			// t = t.upl16(t);

			packssdw(xmm1, xmm1);
			punpcklwd(xmm1, xmm1);

			// m_env.twin[2].u = t.xxxx();
			// m_env.twin[2].v = t.yyyy();

			pshufd(xmm2, xmm1, _MM_SHUFFLE(0, 0, 0, 0));
			pshufd(xmm3, xmm1, _MM_SHUFFLE(1, 1, 1, 1));

			movdqa(xmmword[&m_env.twin[2].u], xmm2);
			movdqa(xmmword[&m_env.twin[2].v], xmm3);
		}
		else
		{
			// TODO: not really needed

			// m_env.twin[2].u = GSVector4i::x00ff();
			// m_env.twin[2].v = GSVector4i::x00ff();

			psrlw(xmm0, 8);

			movdqa(xmmword[&m_env.twin[2].u], xmm0);
			movdqa(xmmword[&m_env.twin[2].v], xmm0);
		}
	}

	if(m_env.sel.tme || m_env.sel.iip && m_env.sel.tfx != 3)
	{
		for(int i = 0; i < 3; i++)
		{
			movaps(Xmm(5 + i), xmmword[&m_shift[i]]);
		}

		// GSVector4 dt = dscan.t;
		// GSVector4 dc = dscan.c;

		movaps(xmm4, xmmword[edx]);
		movaps(xmm3, xmmword[edx + 32]);

		// GSVector4i dtc8 = GSVector4i(dt * 8.0f).ps32(GSVector4i(dc * 8.0f));

		movaps(xmm1, xmm3);
		mulps(xmm1, xmm5);
		cvttps2dq(xmm1, xmm1);
		movaps(xmm2, xmm4);
		mulps(xmm2, xmm5);
		cvttps2dq(xmm2, xmm2);
		packssdw(xmm1, xmm2);

		if(m_env.sel.tme)
		{
			// m_env.d8.st = dtc8.upl16(dtc8);

			movdqa(xmm0, xmm1);
			punpcklwd(xmm0, xmm0);
			movdqa(xmmword[&m_env.d8.st], xmm0);
		}

		if(m_env.sel.iip && m_env.sel.tfx != 3)
		{
			// m_env.d8.c = dtc8.uph16(dtc8);

			punpckhwd(xmm1, xmm1);
			movdqa(xmmword[&m_env.d8.c], xmm1);
		}

		// xmm3 = dt
		// xmm4 = dc
		// xmm6 = ps0123
		// xmm7 = ps4567
		// xmm0, xmm1, xmm2, xmm5 = free

		if(m_env.sel.tme)
		{
			// GSVector4 dtx = dt.xxxx();
			// GSVector4 dty = dt.yyyy();

			movaps(xmm0, xmm3);
			shufps(xmm3, xmm3, _MM_SHUFFLE(0, 0, 0, 0));
			shufps(xmm0, xmm0, _MM_SHUFFLE(1, 1, 1, 1));

			// m_env.d.s = GSVector4i(dtx * ps0123).ps32(GSVector4i(dtx * ps4567));

			movaps(xmm1, xmm3);
			mulps(xmm3, xmm6);
			mulps(xmm1, xmm7);
			cvttps2dq(xmm3, xmm3);
			cvttps2dq(xmm1, xmm1);
			packssdw(xmm3, xmm1);
			movdqa(xmmword[&m_env.d.s], xmm3);

			// m_env.d.t = GSVector4i(dty * ps0123).ps32(GSVector4i(dty * ps4567));

			movaps(xmm1, xmm0);
			mulps(xmm0, xmm6);
			mulps(xmm1, xmm7);
			cvttps2dq(xmm0, xmm0);
			cvttps2dq(xmm1, xmm1);
			packssdw(xmm0, xmm1);
			movdqa(xmmword[&m_env.d.t], xmm0);
		}

		// xmm4 = dc
		// xmm6 = ps0123
		// xmm7 = ps4567
		// xmm0, xmm1, zmm2, xmm3, xmm5 = free

		if(m_env.sel.iip && m_env.sel.tfx != 3)
		{
			// GSVector4 dcx = dc.xxxx();
			// GSVector4 dcy = dc.yyyy();
			// GSVector4 dcz = dc.zzzz();

			movaps(xmm0, xmm4);
			movaps(xmm1, xmm4);
			shufps(xmm4, xmm4, _MM_SHUFFLE(0, 0, 0, 0));
			shufps(xmm0, xmm0, _MM_SHUFFLE(1, 1, 1, 1));
			shufps(xmm1, xmm1, _MM_SHUFFLE(2, 2, 2, 2));

			// m_env.d.r = GSVector4i(dcx * ps0123).ps32(GSVector4i(dcx * ps4567));

			movaps(xmm2, xmm4);
			mulps(xmm4, xmm6);
			mulps(xmm2, xmm7);
			cvttps2dq(xmm4, xmm4);
			cvttps2dq(xmm2, xmm2);
			packssdw(xmm4, xmm2);
			movdqa(xmmword[&m_env.d.r], xmm4);

			// m_env.d.g = GSVector4i(dcy * ps0123).ps32(GSVector4i(dcy * ps4567));

			movaps(xmm2, xmm0);
			mulps(xmm0, xmm6);
			mulps(xmm2, xmm7);
			cvttps2dq(xmm0, xmm0);
			cvttps2dq(xmm2, xmm2);
			packssdw(xmm0, xmm2);
			movdqa(xmmword[&m_env.d.g], xmm0);

			// m_env.d.b = GSVector4i(dcz * ps0123).ps32(GSVector4i(dcz * ps4567));

			movaps(xmm2, xmm1);
			mulps(xmm1, xmm6);
			mulps(xmm2, xmm7);
			cvttps2dq(xmm1, xmm1);
			cvttps2dq(xmm2, xmm2);
			packssdw(xmm1, xmm2);
			movdqa(xmmword[&m_env.d.b], xmm1);
		}
	}

	ret();
}

const GSVector4 GPUSetupPrimCodeGenerator::m_shift[3] = 
{
	GSVector4(8.0f, 8.0f, 8.0f, 8.0f),
	GSVector4(0.0f, 1.0f, 2.0f, 3.0f),
	GSVector4(4.0f, 5.0f, 6.0f, 7.0f),
};