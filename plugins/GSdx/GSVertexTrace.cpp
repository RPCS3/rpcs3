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
}

void GSVertexTrace::Update(const void* vertex, const uint32* index, int count, GS_PRIM_CLASS primclass)
{
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

GSVertexTraceSW::GSVertexTraceSW(const GSState* state)
	: GSVertexTrace(state)
	, m_map("VertexTraceSW", NULL)
{
}

void GSVertexTraceSW::Update(const void* vertex, const uint32* index, int count, GS_PRIM_CLASS primclass)
{
	m_map[Hash(primclass)](count, vertex, index, m_min, m_max);

	GSVertexTrace::Update(vertex, index, count, primclass);
}

GSVertexTraceDX9::GSVertexTraceDX9(const GSState* state)
	: GSVertexTrace(state)
	, m_map("VertexTraceHW9", NULL)
{
}

void GSVertexTraceDX9::Update(const void* vertex, const uint32* index, int count, GS_PRIM_CLASS primclass)
{
	m_map[Hash(primclass)](count, vertex, index, m_min, m_max);

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

	GSVertexTrace::Update(vertex, index, count, primclass);
}

GSVertexTraceDX11::GSVertexTraceDX11(const GSState* state)
	: GSVertexTrace(state)
	, m_map("VertexTraceHW11", NULL)
{
}

void GSVertexTraceDX11::Update(const void* vertex, const uint32* index, int count, GS_PRIM_CLASS primclass)
{
	m_map[Hash(primclass)](count, vertex, index, m_min, m_max);

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

	GSVertexTrace::Update(vertex, index, count, primclass);
}

