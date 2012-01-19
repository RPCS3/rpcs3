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
#include "GSUtil.h"
#include "GSState.h"

const GSVector4 GSVertexTrace::s_minmax(FLT_MAX, -FLT_MAX);

GSVertexTrace::GSVertexTrace(const GSState* state)
	: m_state(state)
{
	#define InitUpdate3(P, IIP, TME, FST, COLOR) \
		m_fmm[COLOR][FST][TME][IIP][P] = &GSVertexTrace::FindMinMax<P, IIP, TME, FST, COLOR>;

	#define InitUpdate2(P, IIP, TME) \
		InitUpdate3(P, IIP, TME, 0, 0) \
		InitUpdate3(P, IIP, TME, 0, 1) \
		InitUpdate3(P, IIP, TME, 1, 0) \
		InitUpdate3(P, IIP, TME, 1, 1) \

	#define InitUpdate(P) \
		InitUpdate2(P, 0, 0) \
		InitUpdate2(P, 0, 1) \
		InitUpdate2(P, 1, 0) \
		InitUpdate2(P, 1, 1) \

	InitUpdate(GS_POINT_CLASS);
	InitUpdate(GS_LINE_CLASS);
	InitUpdate(GS_TRIANGLE_CLASS);
	InitUpdate(GS_SPRITE_CLASS);
}

void GSVertexTrace::Update(const void* vertex, const uint32* index, int count, GS_PRIM_CLASS primclass)
{
	m_primclass = primclass;

	uint32 iip = m_state->PRIM->IIP;
	uint32 tme = m_state->PRIM->TME;
	uint32 fst = m_state->PRIM->FST;
	uint32 color = !(m_state->PRIM->TME && m_state->m_context->TEX0.TFX == TFX_DECAL && m_state->m_context->TEX0.TCC);

	(this->*m_fmm[color][fst][tme][iip][primclass])(vertex, index, count);

	m_eq.value = (m_min.c == m_max.c).mask() | ((m_min.p == m_max.p).mask() << 16) | ((m_min.t == m_max.t).mask() << 20);

	m_alpha.valid = false;

	if(m_state->PRIM->TME)
	{
		const GIFRegTEX1& TEX1 = m_state->m_context->TEX1;

		m_filter.mmag = TEX1.IsMagLinear();
		m_filter.mmin = TEX1.IsMinLinear();

		if(TEX1.MXL == 0) // MXL == 0 => MMIN ignored, tested it on ps2
		{
			m_filter.linear = m_filter.mmag;

			return;
		}

		float K = (float)TEX1.K / 16;

		if(TEX1.LCM == 0 && m_state->PRIM->FST == 0) // FST == 1 => Q is not interpolated
		{
			// LOD = log2(1/|Q|) * (1 << L) + K

			GSVector4::storel(&m_lod, m_max.t.uph(m_min.t).log2(3).neg() * (float)(1 << TEX1.L) + K);

			if(m_lod.x > m_lod.y) {float tmp = m_lod.x; m_lod.x = m_lod.y; m_lod.y = tmp;}
		}
		else
		{
			m_lod.x = K;
			m_lod.y = K;
		}

		if(m_lod.y <= 0)
		{
			m_filter.linear = m_filter.mmag;
		}
		else if(m_lod.x > 0)
		{
			m_filter.linear = m_filter.mmin;
		}
		else
		{
			m_filter.linear = m_filter.mmag | m_filter.mmin;
		}
	}
}

template<GS_PRIM_CLASS primclass, uint32 iip, uint32 tme, uint32 fst, uint32 color>
void GSVertexTrace::FindMinMax(const void* vertex, const uint32* index, int count)
{
	const GSDrawingContext* context = m_state->m_context;

	bool sprite = primclass == GS_SPRITE_CLASS;

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

	GSVector4 pmin = s_minmax.xxxx();
	GSVector4 pmax = s_minmax.yyyy();
	GSVector4 tmin = s_minmax.xxxx();
	GSVector4 tmax = s_minmax.yyyy();
	GSVector4i cmin = GSVector4i::xffffffff();
	GSVector4i cmax = GSVector4i::zero();

	const GSVertex* RESTRICT v = (GSVertex*)vertex;

	for(int i = 0; i < count; i += n)
	{
		GSVector4 q;
		GSVector4i f;

		if(sprite)
		{
			if(tme && !fst)
			{
				q = GSVector4::load<true>(&v[index[i + 1]]).wwww();
			}

			f = GSVector4i(v[index[i + 1]].m[1]).wwww();
		}

		for(int j = 0; j < n; j++)
		{
			GSVector4i c(v[index[i + j]].m[0]);

			if(color && (iip || j == n - 1)) // TODO: unroll, to avoid j == n - 1
			{
				cmin = cmin.min_u8(c);
				cmax = cmax.max_u8(c);
			}

			if(tme)
			{
				if(!fst)
				{
					GSVector4 stq = GSVector4::cast(c);

					GSVector4 q2 = !sprite ? stq.wwww() : q;

					stq = (stq.xyww() * q2.rcpnr()).xyww(q2);

					tmin = tmin.min(stq);
					tmax = tmax.max(stq);
				}
				else
				{
					GSVector4i uv(v[index[i + j]].m[1]);

					GSVector4 st = GSVector4(uv.uph16()).xyxy();

					tmin = tmin.min(st);
					tmax = tmax.max(st);
				}
			}

			GSVector4i xyzf(v[index[i + j]].m[1]);

			GSVector4i xy = xyzf.upl16();
			GSVector4i z = xyzf.yyyy().srl32(1);

			GSVector4 p = GSVector4(xy.upl64(z.upl32(!sprite ? xyzf.wwww() : f)));

			pmin = pmin.min(p);
			pmax = pmax.max(p);
		}
	}

	GSVector4 o(context->XYOFFSET);
	GSVector4 s(1.0f / 16, 1.0f / 16, 2.0f, 1.0f);

	m_min.p = (pmin - o) * s;
	m_max.p = (pmax - o) * s;

	if(tme)
	{
		if(fst)
		{
			s = GSVector4(1 << (16 - 4), 1).xxyy();
		}
		else
		{
			s = GSVector4(0x10000 << context->TEX0.TW, 0x10000 << context->TEX0.TH, 1, 1);
		}

		m_min.t = tmin * s;
		m_max.t = tmax * s;
	}

	if(color)
	{
		m_min.c = cmin.zzzz().u8to32();
		m_max.c = cmax.zzzz().u8to32();
	}
}
