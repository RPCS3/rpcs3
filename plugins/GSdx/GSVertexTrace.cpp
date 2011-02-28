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
	m_map_sw[Hash(primclass)](count, v, m_min, m_max);

	m_eq.value = (m_min.c == m_max.c).mask() | ((m_min.p == m_max.p).mask() << 16) | ((m_min.t == m_max.t).mask() << 20);

	m_alpha.valid = false;
}

void GSVertexTrace::Update(const GSVertexHW9* v, int count, GS_PRIM_CLASS primclass)
{
	m_map_hw9[Hash(primclass)](count, v, m_min, m_max);

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
	m_map_hw11[Hash(primclass)](count, v, m_min, m_max);

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
