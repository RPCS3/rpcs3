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
#include "GSRendererDX11.h"
#include "GSCrc.h"
#include "resource.h"

GSRendererDX11::GSRendererDX11()
	: GSRendererDX(new GSVertexTraceDX11(this), sizeof(GSVertexHW11), new GSTextureCache11(this), GSVector2(-0.5f, -0.5f))
{
	InitConvertVertex(GSRendererDX11);
}

bool GSRendererDX11::CreateDevice(GSDevice* dev)
{
	if(!__super::CreateDevice(dev))
		return false;

	return true;
}

template<uint32 prim, uint32 tme, uint32 fst>
void GSRendererDX11::ConvertVertex(size_t dst_index, size_t src_index)
{
	GSVertex* s = (GSVertex*)((GSVertexHW11*)m_vertex.buff + src_index);
	GSVertexHW11* d = (GSVertexHW11*)m_vertex.buff + dst_index;

	GSVector4i v0 = ((GSVector4i*)s)[0];
	GSVector4i v1 = ((GSVector4i*)s)[1];

	if(tme && fst)
	{
		// TODO: modify VertexTrace and the shaders to read uv from v1.u16[0], v1.u16[1], then this step is not needed

		v0 = GSVector4i::cast(GSVector4(v1.uph16()).xyzw(GSVector4::cast(v0))); // uv => st
	}

	((GSVector4i*)d)[0] = v0;
	((GSVector4i*)d)[1] = v1;
}

void GSRendererDX11::DrawPrims(GSTexture* rt, GSTexture* ds, GSTextureCache::Source* tex)
{
	switch(m_vt->m_primclass)
	{
	case GS_POINT_CLASS:
		m_topology = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
		break;
	case GS_LINE_CLASS:
	case GS_SPRITE_CLASS:
		m_topology = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
		break;
	case GS_TRIANGLE_CLASS:
		m_topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		break;
	default:
		__assume(0);
	}

	__super::DrawPrims(rt, ds, tex);
}
