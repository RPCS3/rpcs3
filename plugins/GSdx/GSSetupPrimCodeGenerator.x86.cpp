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

#if _M_SSE < 0x500 && !(defined(_M_AMD64) || defined(_WIN64))

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
			movaps(Xmm(3 + i), ptr[&m_shift[i]]);
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
		// GSVector4 p = dscan.p;

		movaps(xmm0, ptr[edx + offsetof(GSVertexSW, p)]);

		if(m_en.f)
		{
			// GSVector4 df = p.wwww();

			movaps(xmm1, xmm0);
			shufps(xmm1, xmm1, _MM_SHUFFLE(3, 3, 3, 3));

			// m_local.d4.f = GSVector4i(df * 4.0f).xxzzlh();

			movaps(xmm2, xmm1);
			mulps(xmm2, xmm3);
			cvttps2dq(xmm2, xmm2);
			pshuflw(xmm2, xmm2, _MM_SHUFFLE(2, 2, 0, 0));
			pshufhw(xmm2, xmm2, _MM_SHUFFLE(2, 2, 0, 0));
			movdqa(ptr[&m_local.d4.f], xmm2);

			for(int i = 0; i < (m_sel.notest ? 1 : 4); i++)
			{
				// m_local.d[i].f = GSVector4i(df * m_shift[i]).xxzzlh();

				movaps(xmm2, xmm1);
				mulps(xmm2, Xmm(4 + i));
				cvttps2dq(xmm2, xmm2);
				pshuflw(xmm2, xmm2, _MM_SHUFFLE(2, 2, 0, 0));
				pshufhw(xmm2, xmm2, _MM_SHUFFLE(2, 2, 0, 0));
				movdqa(ptr[&m_local.d[i].f], xmm2);
			}
		}

		if(m_en.z)
		{
			// GSVector4 dz = p.zzzz();

			shufps(xmm0, xmm0, _MM_SHUFFLE(2, 2, 2, 2));

			// m_local.d4.z = dz * 4.0f;

			movaps(xmm1, xmm0);
			mulps(xmm1, xmm3);
			movdqa(ptr[&m_local.d4.z], xmm1);

			for(int i = 0; i < (m_sel.notest ? 1 : 4); i++)
			{
				// m_local.d[i].z = dz * m_shift[i];

				movaps(xmm1, xmm0);
				mulps(xmm1, Xmm(4 + i));
				movdqa(ptr[&m_local.d[i].z], xmm1);
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

		movaps(xmm0, ptr[ecx + offsetof(GSVertexSW, p)]);

		if(m_en.f)
		{
			// m_local.p.f = GSVector4i(p).zzzzh().zzzz();

			cvttps2dq(xmm1, xmm0);
			pshufhw(xmm1, xmm1, _MM_SHUFFLE(2, 2, 2, 2));
			pshufd(xmm1, xmm1, _MM_SHUFFLE(2, 2, 2, 2));
			movdqa(ptr[&m_local.p.f], xmm1);
		}

		if(m_en.z)
		{
			// uint32 z is bypassed in t.w

			movdqa(xmm0, ptr[ecx + offsetof(GSVertexSW, t)]);
			pshufd(xmm0, xmm0, _MM_SHUFFLE(3, 3, 3, 3));
			movdqa(ptr[&m_local.p.z], xmm0);
		}
	}
}

void GSSetupPrimCodeGenerator::Texture()
{
	if(!m_en.t)
	{
		return;
	}

	// GSVector4 t = dscan.t;

	movaps(xmm0, ptr[edx + offsetof(GSVertexSW, t)]);

	movaps(xmm1, xmm0);
	mulps(xmm1, xmm3);

	if(m_sel.fst)
	{
		// m_local.d4.stq = GSVector4i(t * 4.0f);

		cvttps2dq(xmm1, xmm1);

		movdqa(ptr[&m_local.d4.stq], xmm1);
	}
	else
	{
		// m_local.d4.stq = t * 4.0f;

		movaps(ptr[&m_local.d4.stq], xmm1);
	}

	for(int j = 0, k = m_sel.fst ? 2 : 3; j < k; j++)
	{
		// GSVector4 ds = t.xxxx();
		// GSVector4 dt = t.yyyy();
		// GSVector4 dq = t.zzzz();

		movaps(xmm1, xmm0);
		shufps(xmm1, xmm1, (uint8)_MM_SHUFFLE(j, j, j, j));

		for(int i = 0; i < (m_sel.notest ? 1 : 4); i++)
		{
			// GSVector4 v = ds/dt * m_shift[i];

			movaps(xmm2, xmm1);
			mulps(xmm2, Xmm(4 + i));

			if(m_sel.fst)
			{
				// m_local.d[i].s/t = GSVector4i(v);

				cvttps2dq(xmm2, xmm2);

				switch(j)
				{
				case 0: movdqa(ptr[&m_local.d[i].s], xmm2); break;
				case 1: movdqa(ptr[&m_local.d[i].t], xmm2); break;
				}
			}
			else
			{
				// m_local.d[i].s/t/q = v;

				switch(j)
				{
				case 0: movaps(ptr[&m_local.d[i].s], xmm2); break;
				case 1: movaps(ptr[&m_local.d[i].t], xmm2); break;
				case 2: movaps(ptr[&m_local.d[i].q], xmm2); break;
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
		// GSVector4 c = dscan.c;

		movaps(xmm0, ptr[edx + offsetof(GSVertexSW, c)]);
		movaps(xmm1, xmm0);

		// m_local.d4.c = GSVector4i(c * 4.0f).xzyw().ps32();

		movaps(xmm2, xmm0);
		mulps(xmm2, xmm3);
		cvttps2dq(xmm2, xmm2);
		pshufd(xmm2, xmm2, _MM_SHUFFLE(3, 1, 2, 0));
		packssdw(xmm2, xmm2);
		movdqa(ptr[&m_local.d4.c], xmm2);

		// xmm3 is not needed anymore

		// GSVector4 dr = c.xxxx();
		// GSVector4 db = c.zzzz();

		shufps(xmm0, xmm0, _MM_SHUFFLE(0, 0, 0, 0));
		shufps(xmm1, xmm1, _MM_SHUFFLE(2, 2, 2, 2));

		for(int i = 0; i < (m_sel.notest ? 1 : 4); i++)
		{
			// GSVector4i r = GSVector4i(dr * m_shift[i]).ps32();

			movaps(xmm2, xmm0);
			mulps(xmm2, Xmm(4 + i));
			cvttps2dq(xmm2, xmm2);
			packssdw(xmm2, xmm2);

			// GSVector4i b = GSVector4i(db * m_shift[i]).ps32();

			movaps(xmm3, xmm1);
			mulps(xmm3, Xmm(4 + i));
			cvttps2dq(xmm3, xmm3);
			packssdw(xmm3, xmm3);

			// m_local.d[i].rb = r.upl16(b);

			punpcklwd(xmm2, xmm3);
			movdqa(ptr[&m_local.d[i].rb], xmm2);
		}

		// GSVector4 c = dscan.c;

		movaps(xmm0, ptr[edx + offsetof(GSVertexSW, c)]); // not enough regs, have to reload it
		movaps(xmm1, xmm0);

		// GSVector4 dg = c.yyyy();
		// GSVector4 da = c.wwww();

		shufps(xmm0, xmm0, _MM_SHUFFLE(1, 1, 1, 1));
		shufps(xmm1, xmm1, _MM_SHUFFLE(3, 3, 3, 3));

		for(int i = 0; i < (m_sel.notest ? 1 : 4); i++)
		{
			// GSVector4i g = GSVector4i(dg * m_shift[i]).ps32();

			movaps(xmm2, xmm0);
			mulps(xmm2, Xmm(4 + i));
			cvttps2dq(xmm2, xmm2);
			packssdw(xmm2, xmm2);

			// GSVector4i a = GSVector4i(da * m_shift[i]).ps32();

			movaps(xmm3, xmm1);
			mulps(xmm3, Xmm(4 + i));
			cvttps2dq(xmm3, xmm3);
			packssdw(xmm3, xmm3);

			// m_local.d[i].ga = g.upl16(a);

			punpcklwd(xmm2, xmm3);
			movdqa(ptr[&m_local.d[i].ga], xmm2);
		}
	}
	else
	{
		// GSVector4i c = GSVector4i(vertex[index[last].c);

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

		cvttps2dq(xmm0, ptr[ecx + offsetof(GSVertexSW, c)]);

		// c = c.upl16(c.zwxy());

		pshufd(xmm1, xmm0, _MM_SHUFFLE(1, 0, 3, 2));
		punpcklwd(xmm0, xmm1);

		// if(!tme) c = c.srl16(7);

		if(m_sel.tfx == TFX_NONE)
		{
			psrlw(xmm0, 7);
		}

		// m_local.c.rb = c.xxxx();
		// m_local.c.ga = c.zzzz();

		pshufd(xmm1, xmm0, _MM_SHUFFLE(0, 0, 0, 0));
		pshufd(xmm2, xmm0, _MM_SHUFFLE(2, 2, 2, 2));

		movdqa(ptr[&m_local.c.rb], xmm1);
		movdqa(ptr[&m_local.c.ga], xmm2);
	}
}

#endif