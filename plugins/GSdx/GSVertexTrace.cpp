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

	GSVector4 tmin = s_minmax.xxxx();
	GSVector4 tmax = s_minmax.yyyy();
	GSVector4i cmin = GSVector4i::xffffffff();
	GSVector4i cmax = GSVector4i::zero();

	#if _M_SSE >= 0x401

	GSVector4i pmin = GSVector4i::xffffffff();
	GSVector4i pmax = GSVector4i::zero();

	#else

	GSVector4 pmin = s_minmax.xxxx();
	GSVector4 pmax = s_minmax.yyyy();
	
	#endif

	const GSVertex* RESTRICT v = (GSVertex*)vertex;

	for(int i = 0; i < count; i += n)
	{
		if(primclass == GS_POINT_CLASS)
		{
			GSVector4i c(v[index[i]].m[0]);

			if(color)
			{
				cmin = cmin.min_u8(c);
				cmax = cmax.max_u8(c);
			}

			if(tme)
			{
				if(!fst)
				{
					GSVector4 stq = GSVector4::cast(c);

					GSVector4 q = stq.wwww();

					stq = (stq.xyww() * q.rcpnr()).xyww(q);

					tmin = tmin.min(stq);
					tmax = tmax.max(stq);
				}
				else
				{
					GSVector4i uv(v[index[i]].m[1]);

					GSVector4 st = GSVector4(uv.uph16()).xyxy();

					tmin = tmin.min(st);
					tmax = tmax.max(st);
				}
			}

			GSVector4i xyzf(v[index[i]].m[1]);

			GSVector4i xy = xyzf.upl16();
			GSVector4i z = xyzf.yyyy();

			#if _M_SSE >= 0x401

			GSVector4i p = xy.blend16<0xf0>(z.uph32(xyzf));

			pmin = pmin.min_u32(p);
			pmax = pmax.max_u32(p);

			#else

			GSVector4 p = GSVector4(xy.upl64(z.srl32(1).upl32(xyzf.wwww())));

			pmin = pmin.min(p);
			pmax = pmax.max(p);

			#endif
		}
		else if(primclass == GS_LINE_CLASS)
		{
			GSVector4i c0(v[index[i + 0]].m[0]);
			GSVector4i c1(v[index[i + 1]].m[0]);

			if(color)
			{
				if(iip)
				{
					cmin = cmin.min_u8(c0.min_u8(c1));
					cmax = cmax.max_u8(c0.max_u8(c1));
				}
				else
				{
					cmin = cmin.min_u8(c1);
					cmax = cmax.max_u8(c1);
				}
			}

			if(tme)
			{
				if(!fst)
				{
					GSVector4 stq0 = GSVector4::cast(c0);
					GSVector4 stq1 = GSVector4::cast(c1);

					GSVector4 q = stq0.wwww(stq1).rcpnr();

					stq0 = (stq0.xyww() * q.xxxx()).xyww(stq0);
					stq1 = (stq1.xyww() * q.zzzz()).xyww(stq1);

					tmin = tmin.min(stq0.min(stq1));
					tmax = tmax.max(stq0.max(stq1));
				}
				else
				{
					GSVector4i uv0(v[index[i + 0]].m[1]);
					GSVector4i uv1(v[index[i + 1]].m[1]);

					GSVector4 st0 = GSVector4(uv0.uph16()).xyxy();
					GSVector4 st1 = GSVector4(uv1.uph16()).xyxy();

					tmin = tmin.min(st0.min(st1));
					tmax = tmax.max(st0.max(st1));
				}
			}

			GSVector4i xyzf0(v[index[i + 0]].m[1]);
			GSVector4i xyzf1(v[index[i + 1]].m[1]);

			GSVector4i xy0 = xyzf0.upl16();
			GSVector4i z0 = xyzf0.yyyy();
			GSVector4i xy1 = xyzf1.upl16();
			GSVector4i z1 = xyzf1.yyyy();

			#if _M_SSE >= 0x401

			GSVector4i p0 = xy0.blend16<0xf0>(z0.uph32(xyzf0));
			GSVector4i p1 = xy1.blend16<0xf0>(z1.uph32(xyzf1));

			pmin = pmin.min_u32(p0.min_u32(p1));
			pmax = pmax.max_u32(p0.max_u32(p1));

			#else

			GSVector4 p0 = GSVector4(xy0.upl64(z0.srl32(1).upl32(xyzf0.wwww())));
			GSVector4 p1 = GSVector4(xy1.upl64(z1.srl32(1).upl32(xyzf1.wwww())));

			pmin = pmin.min(p0.min(p1));
			pmax = pmax.max(p0.max(p1));

			#endif
		}
		else if(primclass == GS_TRIANGLE_CLASS)
		{
			GSVector4i c0(v[index[i + 0]].m[0]);
			GSVector4i c1(v[index[i + 1]].m[0]);
			GSVector4i c2(v[index[i + 2]].m[0]);

			if(color)
			{
				if(iip)
				{
					cmin = cmin.min_u8(c2).min_u8(c0.min_u8(c1));
					cmax = cmax.max_u8(c2).max_u8(c0.max_u8(c1));
				}
				else
				{
					cmin = cmin.min_u8(c2);
					cmax = cmax.max_u8(c2);
				}
			}

			if(tme)
			{
				if(!fst)
				{
					GSVector4 stq0 = GSVector4::cast(c0);
					GSVector4 stq1 = GSVector4::cast(c1);
					GSVector4 stq2 = GSVector4::cast(c2);

					GSVector4 q = stq0.wwww(stq1).xzww(stq2).rcpnr();

					stq0 = (stq0.xyww() * q.xxxx()).xyww(stq0);
					stq1 = (stq1.xyww() * q.yyyy()).xyww(stq1);
					stq2 = (stq2.xyww() * q.zzzz()).xyww(stq2);

					tmin = tmin.min(stq2).min(stq0.min(stq1));
					tmax = tmax.max(stq2).max(stq0.max(stq1));
				}
				else
				{
					GSVector4i uv0(v[index[i + 0]].m[1]);
					GSVector4i uv1(v[index[i + 1]].m[1]);
					GSVector4i uv2(v[index[i + 2]].m[1]);

					GSVector4 st0 = GSVector4(uv0.uph16()).xyxy();
					GSVector4 st1 = GSVector4(uv1.uph16()).xyxy();
					GSVector4 st2 = GSVector4(uv2.uph16()).xyxy();

					tmin = tmin.min(st2).min(st0.min(st1));
					tmax = tmax.max(st2).max(st0.max(st1));
				}
			}

			GSVector4i xyzf0(v[index[i + 0]].m[1]);
			GSVector4i xyzf1(v[index[i + 1]].m[1]);
			GSVector4i xyzf2(v[index[i + 2]].m[1]);

			GSVector4i xy0 = xyzf0.upl16();
			GSVector4i z0 = xyzf0.yyyy();
			GSVector4i xy1 = xyzf1.upl16();
			GSVector4i z1 = xyzf1.yyyy();
			GSVector4i xy2 = xyzf2.upl16();
			GSVector4i z2 = xyzf2.yyyy();

			#if _M_SSE >= 0x401

			GSVector4i p0 = xy0.blend16<0xf0>(z0.uph32(xyzf0));
			GSVector4i p1 = xy1.blend16<0xf0>(z1.uph32(xyzf1));
			GSVector4i p2 = xy2.blend16<0xf0>(z2.uph32(xyzf2));

			pmin = pmin.min_u32(p2).min_u32(p0.min_u32(p1));
			pmax = pmax.max_u32(p2).max_u32(p0.max_u32(p1));

			#else

			GSVector4 p0 = GSVector4(xy0.upl64(z0.srl32(1).upl32(xyzf0.wwww())));
			GSVector4 p1 = GSVector4(xy1.upl64(z1.srl32(1).upl32(xyzf1.wwww())));
			GSVector4 p2 = GSVector4(xy2.upl64(z2.srl32(1).upl32(xyzf2.wwww())));

			pmin = pmin.min(p2).min(p0.min(p1));
			pmax = pmax.max(p2).max(p0.max(p1));

			#endif
		}
		else if(primclass == GS_SPRITE_CLASS)
		{
			GSVector4i c0(v[index[i + 0]].m[0]);
			GSVector4i c1(v[index[i + 1]].m[0]);

			if(color)
			{
				if(iip)
				{
					cmin = cmin.min_u8(c0.min_u8(c1));
					cmax = cmax.max_u8(c0.max_u8(c1));
				}
				else
				{
					cmin = cmin.min_u8(c1);
					cmax = cmax.max_u8(c1);
				}
			}

			if(tme)
			{
				if(!fst)
				{
					GSVector4 stq0 = GSVector4::cast(c0);
					GSVector4 stq1 = GSVector4::cast(c1);

					GSVector4 q = stq1.wwww().rcpnr();

					stq0 = (stq0.xyww() * q).xyww(stq1);
					stq1 = (stq1.xyww() * q).xyww(stq1);

					tmin = tmin.min(stq0.min(stq1));
					tmax = tmax.max(stq0.max(stq1));
				}
				else
				{
					GSVector4i uv0(v[index[i + 0]].m[1]);
					GSVector4i uv1(v[index[i + 1]].m[1]);

					GSVector4 st0 = GSVector4(uv0.uph16()).xyxy();
					GSVector4 st1 = GSVector4(uv1.uph16()).xyxy();

					tmin = tmin.min(st0.min(st1));
					tmax = tmax.max(st0.max(st1));
				}
			}

			GSVector4i xyzf0(v[index[i + 0]].m[1]);
			GSVector4i xyzf1(v[index[i + 1]].m[1]);

			GSVector4i xy0 = xyzf0.upl16();
			GSVector4i z0 = xyzf0.yyyy();
			GSVector4i xy1 = xyzf1.upl16();
			GSVector4i z1 = xyzf1.yyyy();

			#if _M_SSE >= 0x401

			GSVector4i p0 = xy0.blend16<0xf0>(z0.uph32(xyzf1));
			GSVector4i p1 = xy1.blend16<0xf0>(z1.uph32(xyzf1));

			pmin = pmin.min_u32(p0.min_u32(p1));
			pmax = pmax.max_u32(p0.max_u32(p1));

			#else

			GSVector4 p0 = GSVector4(xy0.upl64(z0.srl32(1).upl32(xyzf1.wwww())));
			GSVector4 p1 = GSVector4(xy1.upl64(z1.srl32(1).upl32(xyzf1.wwww())));

			pmin = pmin.min(p0.min(p1));
			pmax = pmax.max(p0.max(p1));

			#endif
		}
	}

	#if _M_SSE >= 0x401

	pmin = pmin.blend16<0x30>(pmin.srl32(1));
	pmax = pmax.blend16<0x30>(pmax.srl32(1));

	#endif

	GSVector4 o(context->XYOFFSET);
	GSVector4 s(1.0f / 16, 1.0f / 16, 2.0f, 1.0f);

	m_min.p = (GSVector4(pmin) - o) * s;
	m_max.p = (GSVector4(pmax) - o) * s;

	if(tme)
	{
		if(fst)
		{
			s = GSVector4(1.0f / 16, 1.0f).xxyy();
		}
		else
		{
			s = GSVector4(1 << context->TEX0.TW, 1 << context->TEX0.TH, 1, 1);
		}

		m_min.t = tmin * s;
		m_max.t = tmax * s;
	}
	else
	{
		m_min.t = GSVector4::zero();
		m_max.t = GSVector4::zero();
	}

	if(color)
	{
		m_min.c = cmin.zzzz().u8to32();
		m_max.c = cmax.zzzz().u8to32();
	}
	else
	{
		m_min.c = GSVector4i::zero();
		m_max.c = GSVector4i::zero();
	}
}
