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
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "GSSetupPrimCodeGenerator.h"
#include "GSVertexSW.h"

#if _M_SSE >= 0x501 && !(defined(_M_AMD64) || defined(_WIN64))

using namespace Xbyak;

static const int _args = 0;
static const int _vertex = _args + 4;
static const int _index = _args + 8;
static const int _dscan = _args + 12;

void GSSetupPrimCodeGenerator::Generate()
{
	if((m_en.z || m_en.f) && m_sel.prim != GS_SPRITE_CLASS || m_en.t || m_en.c && m_sel.iip)
	{
		mov(edx, dword[esp + _dscan]);

		for(int i = 0; i < (m_sel.notest ? 2 : 5); i++)
		{
			vmovaps(Ymm(3 + i), ptr[&m_shift[i]]);
		}
	}

	Depth();

	Texture();

	Color();

	ret();
}

void GSSetupPrimCodeGenerator::Depth()
{
	if(!m_en.z && !m_en.f)
	{
		return;
	}

	if(m_sel.prim != GS_SPRITE_CLASS)
	{
		// GSVector4 dp8 = dscan.p * GSVector4::broadcast32(&shift[0]);

		vbroadcastf128(ymm0, ptr[edx + offsetof(GSVertexSW, p)]);

		vmulps(ymm1, ymm0, ymm3);

		if(m_en.z)
		{
			// m_local.d8.p.z = dp8.extract32<2>();

			vextractps(ptr[&m_local.d8.p.z], xmm1, 2);
		}
		
		if(m_en.f)
		{
			// m_local.d8.p.f = GSVector4i(dp8).extract32<3>();

			vcvtps2dq(ymm2, ymm1);
			vpextrd(ptr[&m_local.d8.p.f], xmm2, 3);
		}

		if(m_en.z)
		{
			// GSVector8 dz = GSVector8(dscan.p).zzzz();

			vshufps(ymm2, ymm0, ymm0, _MM_SHUFFLE(2, 2, 2, 2));
		}

		if(m_en.f)
		{
			// GSVector8 df = GSVector8(dscan.p).wwww();

			vshufps(ymm1, ymm0, ymm0, _MM_SHUFFLE(3, 3, 3, 3));
		}

		for(int i = 0; i < (m_sel.notest ? 1 : 8); i++)
		{
			if(m_en.z)
			{
				// m_local.d[i].z = dz * shift[1 + i];

				if(i < 4) vmulps(ymm0, ymm2, Ymm(4 + i));
				else vmulps(ymm0, ymm2, ptr[&m_shift[i + 1]]);
				vmovaps(ptr[&m_local.d[i].z], ymm0);
			}

			if(m_en.f)
			{
				// m_local.d[i].f = GSVector8i(df * m_shift[i]).xxzzlh();

				if(i < 4) vmulps(ymm0, ymm1, Ymm(4 + i));
				else vmulps(ymm0, ymm1, ptr[&m_shift[i + 1]]);
				vcvttps2dq(ymm0, ymm0);
				vpshuflw(ymm0, ymm0, _MM_SHUFFLE(2, 2, 0, 0));
				vpshufhw(ymm0, ymm0, _MM_SHUFFLE(2, 2, 0, 0));
				vmovdqa(ptr[&m_local.d[i].f], ymm0);
			}
		}
	}
	else
	{
		// GSVector4 p = vertex[index[1]].p;

		mov(ecx, ptr[esp + _index]);
		mov(ecx, ptr[ecx + sizeof(uint32) * 1]);
		shl(ecx, 6); // * sizeof(GSVertexSW)
		add(ecx, ptr[esp + _vertex]);

		if(m_en.f)
		{
			// m_local.p.f = GSVector4i(vertex[index[1]].p).extract32<3>();

			vmovaps(xmm0, ptr[ecx + offsetof(GSVertexSW, p)]);
			vcvttps2dq(xmm0, xmm0);
			vpextrd(ptr[&m_local.p.f], xmm0, 3);
		}

		if(m_en.z)
		{
			// m_local.p.z = vertex[index[1]].t.u32[3]; // uint32 z is bypassed in t.w

			mov(eax, ptr[ecx + offsetof(GSVertexSW, t.w)]);
			mov(ptr[&m_local.p.z], eax);
		}
	}
}

void GSSetupPrimCodeGenerator::Texture()
{
	if(!m_en.t)
	{
		return;
	}

	// GSVector8 dt(dscan.t);

	vbroadcastf128(ymm0, ptr[edx + offsetof(GSVertexSW, t)]);

	// GSVector8 dt8 = dt * shift[0];

	vmulps(ymm1, ymm0, ymm3);

	if(m_sel.fst)
	{
		// m_local.d8.stq = GSVector8::cast(GSVector8i(dt8));

		vcvttps2dq(ymm1, ymm1);

		vmovdqa(ptr[&m_local.d8.stq], xmm1);
	}
	else
	{
		// m_local.d8.stq = dt8;

		vmovaps(ptr[&m_local.d8.stq], xmm1);
	}

	for(int j = 0, k = m_sel.fst ? 2 : 3; j < k; j++)
	{
		// GSVector8 dstq = dt.xxxx/yyyy/zzzz();

		vshufps(ymm1, ymm0, ymm0, (uint8)_MM_SHUFFLE(j, j, j, j));

		for(int i = 0; i < (m_sel.notest ? 1 : 8); i++)
		{
			// GSVector8 v = dstq * shift[1 + i];

			if(i < 4) vmulps(ymm2, ymm1, Ymm(4 + i));
			else vmulps(ymm2, ymm1, ptr[&m_shift[i + 1]]);

			if(m_sel.fst)
			{
				// m_local.d[i].s/t = GSVector8::cast(GSVector8i(v));

				vcvttps2dq(ymm2, ymm2);

				switch(j)
				{
				case 0: vmovdqa(ptr[&m_local.d[i].s], ymm2); break;
				case 1: vmovdqa(ptr[&m_local.d[i].t], ymm2); break;
				}
			}
			else
			{
				// m_local.d[i].s/t/q = v;

				switch(j)
				{
				case 0: vmovaps(ptr[&m_local.d[i].s], ymm2); break;
				case 1: vmovaps(ptr[&m_local.d[i].t], ymm2); break;
				case 2: vmovaps(ptr[&m_local.d[i].q], ymm2); break;
				}
			}
		}
	}
}

void GSSetupPrimCodeGenerator::Color()
{
	if(!m_en.c)
	{
		return;
	}

	if(m_sel.iip)
	{
		// GSVector8 dc(dscan.c);

		vbroadcastf128(ymm0, ptr[edx + offsetof(GSVertexSW, c)]);

		// m_local.d8.c = GSVector8i(dc * shift[0]).xzyw().ps32();

		vmulps(ymm1, ymm0, ymm3);
		vcvttps2dq(ymm1, ymm1);
		vpshufd(ymm1, ymm1, _MM_SHUFFLE(3, 1, 2, 0));
		vpackssdw(ymm1, ymm1);
		vmovq(ptr[&m_local.d8.c], xmm1);

		// ymm3 is not needed anymore

		// GSVector8 dr = dc.xxxx();
		// GSVector8 db = dc.zzzz();

		vshufps(ymm2, ymm0, ymm0, _MM_SHUFFLE(0, 0, 0, 0));
		vshufps(ymm3, ymm0, ymm0, _MM_SHUFFLE(2, 2, 2, 2));

		for(int i = 0; i < (m_sel.notest ? 1 : 8); i++)
		{
			// GSVector8i r = GSVector8i(dr * shift[1 + i]).ps32();

			if(i < 4) vmulps(ymm0, ymm2, Ymm(4 + i));
			else vmulps(ymm0, ymm2, ptr[&m_shift[i + 1]]);
			vcvttps2dq(ymm0, ymm0);
			vpackssdw(ymm0, ymm0);

			// GSVector4i b = GSVector8i(db * shift[1 + i]).ps32();

			if(i < 4) vmulps(ymm1, ymm3, Ymm(4 + i));
			else vmulps(ymm1, ymm3, ptr[&m_shift[i + 1]]);
			vcvttps2dq(ymm1, ymm1);
			vpackssdw(ymm1, ymm1);

			// m_local.d[i].rb = r.upl16(b);

			vpunpcklwd(ymm0, ymm1);
			vmovdqa(ptr[&m_local.d[i].rb], ymm0);
		}

		// GSVector8 dc(dscan.c);

		vbroadcastf128(ymm0, ptr[edx + offsetof(GSVertexSW, c)]); // not enough regs, have to reload it

		// GSVector8 dg = dc.yyyy();
		// GSVector8 da = dc.wwww();

		vshufps(ymm2, ymm0, ymm0, _MM_SHUFFLE(1, 1, 1, 1));
		vshufps(ymm3, ymm0, ymm0, _MM_SHUFFLE(3, 3, 3, 3));

		for(int i = 0; i < (m_sel.notest ? 1 : 8); i++)
		{
			// GSVector8i g = GSVector8i(dg * shift[1 + i]).ps32();

			if(i < 4) vmulps(ymm0, ymm2, Ymm(4 + i));
			else vmulps(ymm0, ymm2, ptr[&m_shift[i + 1]]);
			vcvttps2dq(ymm0, ymm0);
			vpackssdw(ymm0, ymm0);

			// GSVector8i a = GSVector8i(da * shift[1 + i]).ps32();

			if(i < 4) vmulps(ymm1, ymm3, Ymm(4 + i));
			else vmulps(ymm1, ymm3, ptr[&m_shift[i + 1]]);
			vcvttps2dq(ymm1, ymm1);
			vpackssdw(ymm1, ymm1);

			// m_local.d[i].ga = g.upl16(a);

			vpunpcklwd(ymm0, ymm1);
			vmovdqa(ptr[&m_local.d[i].ga], ymm0);
		}
	}
	else
	{
		// GSVector8i c = GSVector8i(GSVector8(vertex[index[last]].c));

		int last = 0;

		switch(m_sel.prim)
		{
		case GS_POINT_CLASS: last = 0; break;
		case GS_LINE_CLASS: last = 1; break;
		case GS_TRIANGLE_CLASS: last = 2; break;
		case GS_SPRITE_CLASS: last = 1; break;
		}

		if(!(m_sel.prim == GS_SPRITE_CLASS && (m_en.z || m_en.f))) // if this is a sprite, the last vertex was already loaded in Depth()
		{
			mov(ecx, ptr[esp + _index]);
			mov(ecx, ptr[ecx + sizeof(uint32) * last]);
			shl(ecx, 6); // * sizeof(GSVertexSW)
			add(ecx, ptr[esp + _vertex]);
		}

		vbroadcasti128(ymm0, ptr[ecx + offsetof(GSVertexSW, c)]);
		vcvttps2dq(ymm0, ymm0);

		// c = c.upl16(c.zwxy());

		vpshufd(ymm1, ymm0, _MM_SHUFFLE(1, 0, 3, 2));
		vpunpcklwd(ymm0, ymm1);

		// if(!tme) c = c.srl16(7);

		if(m_sel.tfx == TFX_NONE)
		{
			vpsrlw(ymm0, 7);
		}

		// m_local.c.rb = c.xxxx();
		// m_local.c.ga = c.zzzz();

		vpshufd(ymm1, ymm0, _MM_SHUFFLE(0, 0, 0, 0));
		vpshufd(ymm2, ymm0, _MM_SHUFFLE(2, 2, 2, 2));

		vmovdqa(ptr[&m_local.c.rb], ymm1);
		vmovdqa(ptr[&m_local.c.ga], ymm2);
	}
}

#endif