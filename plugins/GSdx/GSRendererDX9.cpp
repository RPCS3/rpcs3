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
	: GSRendererDX<GSVertexHW9>(new GSTextureCache9(this))
{
	InitVertexKick<GSRendererDX9>();
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
void GSRendererDX9::VertexKick(bool skip)
{
	GSVertexHW9 v;

	v.p = GSVector4(((GSVector4i)m_v.XYZ).upl16());

	if(tme && !fst)
	{
		v.p = v.p.xyxy(GSVector4((float)m_v.XYZ.Z, m_v.RGBAQ.Q));
	}
	else
	{
		v.p = v.p.xyxy(GSVector4::load((float)m_v.XYZ.Z));
	}

	if(tme)
	{
		if(fst)
		{
			v.t = m_v.GetUV();
		}
		else
		{
			v.t = GSVector4::loadl(&m_v.ST);
		}
	}

	GSVertexHW9& dst = m_vl.AddTail();

	dst = v;

	dst.c0 = m_v.RGBAQ.u32[0];
	dst.c1 = m_v.FOG.u32[1];

	int count = 0;
	
	if(GSVertexHW9* v = DrawingKick<prim>(skip, count))
	{
		GSVector4 scissor = m_context->scissor.dx9;

		GSVector4 pmin, pmax;

		switch(prim)
		{
		case GS_POINTLIST:
			pmin = v[0].p;
			pmax = v[0].p;
			break;
		case GS_LINELIST:
		case GS_LINESTRIP:
		case GS_SPRITE:
			pmin = v[0].p.min(v[1].p);
			pmax = v[0].p.max(v[1].p);
			break;
		case GS_TRIANGLELIST:
		case GS_TRIANGLESTRIP:
		case GS_TRIANGLEFAN:
			pmin = v[0].p.min(v[1].p).min(v[2].p);
			pmax = v[0].p.max(v[1].p).max(v[2].p);
			break;
		}

		GSVector4 test = (pmax < scissor) | (pmin > scissor.zwxy());

		switch(prim)
		{
		case GS_TRIANGLELIST:
		case GS_TRIANGLESTRIP:
		case GS_TRIANGLEFAN:
		case GS_SPRITE:
			test |= pmin == pmax;
			break;
		}

		if(test.mask() & 3)
		{
			return;
		}

		switch(prim)
		{
		case GS_POINTLIST:
			break;
		case GS_LINELIST:
		case GS_LINESTRIP:
			if(PRIM->IIP == 0) {v[0].c0 = v[1].c0;}
			break;
		case GS_TRIANGLELIST:
		case GS_TRIANGLESTRIP:
		case GS_TRIANGLEFAN:
			if(PRIM->IIP == 0) {v[0].c0 = v[1].c0 = v[2].c0;}
			break;
		case GS_SPRITE:
			if(PRIM->IIP == 0) {v[0].c0 = v[1].c0;}
			v[0].p.z = v[1].p.z;
			v[0].p.w = v[1].p.w;
			v[0].c1 = v[1].c1;
			v[2] = v[1];
			v[3] = v[1];
			v[1].p.y = v[0].p.y;
			v[1].t.y = v[0].t.y;
			v[2].p.x = v[0].p.x;
			v[2].t.x = v[0].t.x;
			v[4] = v[1];
			v[5] = v[2];
			count += 4;
			break;
		}

		m_count += count;
	}
}

void GSRendererDX9::Draw(GSTexture* rt, GSTexture* ds, GSTextureCache::Source* tex)
{
	switch(m_vt.m_primclass)
	{
	case GS_POINT_CLASS:
		m_topology = D3DPT_POINTLIST;
		m_perfmon.Put(GSPerfMon::Prim, m_count);
		break;
	case GS_LINE_CLASS: 
		m_topology = D3DPT_LINELIST;
		m_perfmon.Put(GSPerfMon::Prim, m_count / 2);
		break;
	case GS_TRIANGLE_CLASS: 
	case GS_SPRITE_CLASS:
		m_topology = D3DPT_TRIANGLELIST;
		m_perfmon.Put(GSPerfMon::Prim, m_count / 3);
		break;
	default:
		__assume(0);
	}

	(*(GSDevice9*)m_dev)->SetRenderState(D3DRS_SHADEMODE, PRIM->IIP ? D3DSHADE_GOURAUD : D3DSHADE_FLAT); // TODO

	__super::Draw(rt, ds, tex);
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

	GSVector4 src = ((m_vt.m_min.p.xyxy(m_vt.m_max.p) + o.xxyy()) * s.xyxy()).sat(o.zzyy());
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
