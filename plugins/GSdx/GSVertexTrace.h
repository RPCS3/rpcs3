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

__aligned32 class GSVertexTrace
{
	struct Vertex {GSVector4i c; GSVector4 p, t;};
	struct VertexAlpha {int min, max; bool valid;};

	typedef void (*VertexTracePtr)(const void* v, int count, Vertex& min, Vertex& max);

	class CGSW : public GSCodeGenerator
	{
	public:
		CGSW(const void* param, uint32 key, void* code, size_t maxsize);
	};

	class CGHW9 : public GSCodeGenerator
	{
	public:
		CGHW9(const void* param, uint32 key, void* code, size_t maxsize);
	};

	class CGHW11 : public GSCodeGenerator
	{
	public:
		CGHW11(const void* param, uint32 key, void* code, size_t maxsize);
	};

	GSCodeGeneratorFunctionMap<CGSW, uint32, VertexTracePtr> m_map_sw;
	GSCodeGeneratorFunctionMap<CGHW9, uint32, VertexTracePtr> m_map_hw9;
	GSCodeGeneratorFunctionMap<CGHW11, uint32, VertexTracePtr> m_map_hw11;

	uint32 Hash(GS_PRIM_CLASS primclass);

	const GSState* m_state;

public:
	GS_PRIM_CLASS m_primclass;
	Vertex m_min, m_max; // t.xy * 0x10000
	VertexAlpha m_alpha; // source alpha range after tfx, GSRenderer::GetAlphaMinMax() updates it

	union
	{
		uint32 value;
		struct {uint32 r:4, g:4, b:4, a:4, x:1, y:1, z:1, f:1, s:1, t:1, q:1, _pad:1;};
		struct {uint32 rgba:16, xyzf:4, stq:4;};
	} m_eq;

	GSVertexTrace(const GSState* state);

	void Update(const GSVertexSW* v, int count, GS_PRIM_CLASS primclass);
	void Update(const GSVertexHW9* v, int count, GS_PRIM_CLASS primclass);
	void Update(const GSVertexHW11* v, int count, GS_PRIM_CLASS primclass);
	void Update(const GSVertexNull* v, int count, GS_PRIM_CLASS primclass) {}
};
