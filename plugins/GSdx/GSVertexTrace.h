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

#include "GSDrawingContext.h"
#include "GSVertex.h"
#include "GSVertexSW.h"
#include "GSVertexHW.h"
#include "GSFunctionMap.h"

class GSState;

__aligned(class, 32) GSVertexTrace : public GSAlignedClass<32>
{
public:
	struct Vertex {GSVector4i c; GSVector4 p, t;}; // t.xy * 0x10000
	struct VertexAlpha {int min, max; bool valid;};

protected:
	const GSState* m_state;

	static const GSVector4 s_minmax;

	typedef void (GSVertexTrace::*FindMinMaxPtr)(const void* vertex, const uint32* index, int count);

	FindMinMaxPtr m_fmm[2][2][2][2][4];

	template<GS_PRIM_CLASS primclass, uint32 iip, uint32 tme, uint32 fst, uint32 color>
	void FindMinMax(const void* vertex, const uint32* index, int count);

public:
	GS_PRIM_CLASS m_primclass;

	Vertex m_min;
	Vertex m_max;
	VertexAlpha m_alpha; // source alpha range after tfx, GSRenderer::GetAlphaMinMax() updates it

	union
	{
		uint32 value;
		struct {uint32 r:4, g:4, b:4, a:4, x:1, y:1, z:1, f:1, s:1, t:1, q:1, _pad:1;};
		struct {uint32 rgba:16, xyzf:4, stq:4;};
	} m_eq;

	union 
	{
		struct {uint32 mmag:1, mmin:1, linear:1;};
	} m_filter;

	GSVector2 m_lod; // x = min, y = max

public:
	GSVertexTrace(const GSState* state);
	virtual ~GSVertexTrace() {}

	void Update(const void* vertex, const uint32* index, int count, GS_PRIM_CLASS primclass);

	bool IsLinear() const {return m_filter.linear;}
};
