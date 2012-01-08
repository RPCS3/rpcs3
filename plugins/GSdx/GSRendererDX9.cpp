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
#include "GSRendererDX9.h"
#include "GSCrc.h"
#include "resource.h"

GSRendererDX9::GSRendererDX9()
	: GSRendererDX(new GSVertexTraceDX9(this), sizeof(GSVertexHW9), new GSTextureCache9(this))
{
	InitConvertVertex(GSRendererDX9);
}

bool GSRendererDX9::CreateDevice(GSDevice* dev)
{
	if(!__super::CreateDevice(dev))
		return false;

	//

	memset(&m_fba.dss, 0, sizeof(m_fba.dss));

	m_fba.dss.StencilEnable = true;
	m_fba.dss.StencilReadMask = 2;
	m_fba.dss.StencilWriteMask = 2;
	m_fba.dss.StencilFunc = D3DCMP_EQUAL;
	m_fba.dss.StencilPassOp = D3DSTENCILOP_ZERO;
	m_fba.dss.StencilFailOp = D3DSTENCILOP_ZERO;
	m_fba.dss.StencilDepthFailOp = D3DSTENCILOP_ZERO;
	m_fba.dss.StencilRef = 2;

	memset(&m_fba.bs, 0, sizeof(m_fba.bs));

	m_fba.bs.RenderTargetWriteMask = D3DCOLORWRITEENABLE_ALPHA;

	//

	return true;
}

template<uint32 prim, uint32 tme, uint32 fst>
void GSRendererDX9::ConvertVertex(size_t dst_index, size_t src_index)
{
	GSVertex* s = (GSVertex*)((GSVertexHW9*)m_vertex.buff + src_index);
	GSVertexHW9* d = (GSVertexHW9*)m_vertex.buff + dst_index;

	GSVector4 p = GSVector4(GSVector4i::load(s->XYZ.u32[0]).upl16());

	if(tme && !fst)
	{
		p = p.xyxy(GSVector4((float)s->XYZ.Z, s->RGBAQ.Q));
	}
	else
	{
		p = p.xyxy(GSVector4::load((float)s->XYZ.Z));
	}

	GSVector4 t = GSVector4::zero();

	if(tme)
	{
		if(fst)
		{
			t = GSVector4(GSVector4i::load(s->UV).upl16());
		}
		else
		{
			t = GSVector4::loadl(&s->ST);
		}
	}

	t = t.xyxy(GSVector4::cast(GSVector4i(s->RGBAQ.u32[0], s->FOG)));

	d->p = p;
	d->t = t;
}

void GSRendererDX9::DrawPrims(GSTexture* rt, GSTexture* ds, GSTextureCache::Source* tex)
{
	switch(m_vt->m_primclass)
	{
	case GS_POINT_CLASS:

		m_topology = D3DPT_POINTLIST;

		break;

	case GS_LINE_CLASS:

		m_topology = D3DPT_LINELIST;

		if(PRIM->IIP == 0)
		{
			for(size_t i = 0, j = m_index.tail; i < j; i += 2) 
			{
				uint32 tmp = m_index.buff[i + 0]; 
				m_index.buff[i + 0] = m_index.buff[i + 1];
				m_index.buff[i + 1] = tmp;
			}
		}

		break;

	case GS_TRIANGLE_CLASS:

		m_topology = D3DPT_TRIANGLELIST;

		if(PRIM->IIP == 0)
		{
			for(size_t i = 0, j = m_index.tail; i < j; i += 3) 
			{
				uint32 tmp = m_index.buff[i + 0]; 
				m_index.buff[i + 0] = m_index.buff[i + 2];
				m_index.buff[i + 2] = tmp;
			}
		}

		break;

	case GS_SPRITE_CLASS:

		m_topology = D3DPT_TRIANGLELIST;

		// each sprite converted to quad needs twice the space

		while(m_vertex.tail * 2 > m_vertex.maxcount)
		{
			GrowVertexBuffer();
		}

		// assume vertices are tightly packed and sequentially indexed (it should be the case)

		if(m_vertex.next >= 2)
		{
			size_t count = m_vertex.next;

			int i = (int)count * 2 - 4;
			GSVertexHW9* s = (GSVertexHW9*)&m_vertex.buff[sizeof(GSVertexHW9) * count] - 2;
			GSVertexHW9* q = (GSVertexHW9*)&m_vertex.buff[sizeof(GSVertexHW9) * (count * 2)] - 4;
			uint32* RESTRICT index = &m_index.buff[count * 3] - 6;
		
			for(; i >= 0; i -= 4, s -= 2, q -= 4, index -= 6) 
			{
				GSVertexHW9 v0 = s[0];
				GSVertexHW9 v1 = s[1];

				v0.p = v0.p.xyzw(v1.p); // z, q
				v0.t = v0.t.xyzw(v1.t); // c, f

				q[0] = v0;
				q[3] = v1;

				// swap x, s

				GSVector4 p = v0.p.insert<0, 0>(v1.p);
				GSVector4 t = v0.t.insert<0, 0>(v1.t);
				v1.p = v1.p.insert<0, 0>(v0.p);
				v1.t = v1.t.insert<0, 0>(v0.t);
				v0.p = p;
				v0.t = t;

				q[1] = v0;
				q[2] = v1;

				index[0] = i + 0;
				index[1] = i + 1;
				index[2] = i + 2;
				index[3] = i + 1;
				index[4] = i + 2;
				index[5] = i + 3;
			}

			m_vertex.head = m_vertex.tail = m_vertex.next = count * 2;
			m_index.tail = count * 3;
		}

		break;

	default:
		__assume(0);
	}

	(*(GSDevice9*)m_dev)->SetRenderState(D3DRS_SHADEMODE, PRIM->IIP ? D3DSHADE_GOURAUD : D3DSHADE_FLAT); // TODO

	__super::DrawPrims(rt, ds, tex);
}

void GSRendererDX9::UpdateFBA(GSTexture* rt)
{
	GSDevice9* dev = (GSDevice9*)m_dev;

	dev->BeginScene();

	// om

	dev->OMSetDepthStencilState(&m_fba.dss);
	dev->OMSetBlendState(&m_fba.bs, 0);

	// ia

	GSVector4 s = GSVector4(rt->GetScale().x / rt->GetWidth(), rt->GetScale().y / rt->GetHeight());
	GSVector4 o = GSVector4(-1.0f, 1.0f);

	GSVector4 src = ((m_vt->m_min.p.xyxy(m_vt->m_max.p) + o.xxyy()) * s.xyxy()).sat(o.zzyy());
	GSVector4 dst = src * 2.0f + o.xxxx();

	GSVertexPT1 vertices[] =
	{
		{GSVector4(dst.x, -dst.y, 0.5f, 1.0f), GSVector2(0, 0)},
		{GSVector4(dst.z, -dst.y, 0.5f, 1.0f), GSVector2(0, 0)},
		{GSVector4(dst.x, -dst.w, 0.5f, 1.0f), GSVector2(0, 0)},
		{GSVector4(dst.z, -dst.w, 0.5f, 1.0f), GSVector2(0, 0)},
	};

	dev->IASetVertexBuffer(vertices, sizeof(vertices[0]), countof(vertices));
	dev->IASetInputLayout(dev->m_convert.il);
	dev->IASetPrimitiveTopology(D3DPT_TRIANGLESTRIP);

	// vs

	dev->VSSetShader(dev->m_convert.vs, NULL, 0);

	// ps

	dev->PSSetShader(dev->m_convert.ps[4], NULL, 0);

	//

	dev->DrawPrimitive();

	//

	dev->EndScene();
}
