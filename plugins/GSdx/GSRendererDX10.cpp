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
#include "GSRendererDX10.h"
#include "GSCrc.h"
#include "resource.h"

GSRendererDX10::GSRendererDX10(bool mt, void (*irq)())
	: GSRendererDX<GSVertexHW10>(mt, irq, new GSTextureCache10(this), GSVector2(-0.5f, -0.5f))
{
	InitVertexKick<GSRendererDX10>();
}

bool GSRendererDX10::CreateDevice(GSDevice* dev)
{
	if(!__super::CreateDevice(dev))
		return false;

	//

	D3D10_DEPTH_STENCIL_DESC dsd;

	memset(&dsd, 0, sizeof(dsd));

	dsd.DepthEnable = false;
	dsd.StencilEnable = true;
	dsd.StencilReadMask = 1;
	dsd.StencilWriteMask = 1;
	dsd.FrontFace.StencilFunc = D3D10_COMPARISON_ALWAYS;
	dsd.FrontFace.StencilPassOp = D3D10_STENCIL_OP_REPLACE;
	dsd.FrontFace.StencilFailOp = D3D10_STENCIL_OP_KEEP;
	dsd.FrontFace.StencilDepthFailOp = D3D10_STENCIL_OP_KEEP;
	dsd.BackFace.StencilFunc = D3D10_COMPARISON_ALWAYS;
	dsd.BackFace.StencilPassOp = D3D10_STENCIL_OP_REPLACE;
	dsd.BackFace.StencilFailOp = D3D10_STENCIL_OP_KEEP;
	dsd.BackFace.StencilDepthFailOp = D3D10_STENCIL_OP_KEEP;

	(*(GSDevice10*)m_dev)->CreateDepthStencilState(&dsd, &m_date.dss);

	D3D10_BLEND_DESC bd;

	memset(&bd, 0, sizeof(bd));

	(*(GSDevice10*)m_dev)->CreateBlendState(&bd, &m_date.bs);

	//

	return true;
}

template<uint32 prim, uint32 tme, uint32 fst> 
void GSRendererDX10::VertexKick(bool skip)
{
	GSVertexHW10& dst = m_vl.AddTail();

	dst.vi[0] = m_v.vi[0];
	dst.vi[1] = m_v.vi[1];

	if(tme && fst)
	{
		GSVector4::storel(&dst.ST, m_v.GetUV());
	}

	int count = 0;
	
	if(GSVertexHW10* v = DrawingKick<prim>(skip, count))
	{
		GSVector4i scissor = m_context->scissor.dx10;

		GSVector4i pmin, pmax;

		#if _M_SSE >= 0x401

		GSVector4i v0, v1, v2;

		switch(prim)
		{
		case GS_POINTLIST:
			v0 = GSVector4i::load((int)v[0].p.xy).upl16();
			pmin = v0;
			pmax = v0;
			break;
		case GS_LINELIST:
		case GS_LINESTRIP:
		case GS_SPRITE:
			v0 = GSVector4i::load((int)v[0].p.xy);
			v1 = GSVector4i::load((int)v[1].p.xy);
			pmin = v0.min_u16(v1).upl16();
			pmax = v0.max_u16(v1).upl16();
			break;
		case GS_TRIANGLELIST:
		case GS_TRIANGLESTRIP:
		case GS_TRIANGLEFAN:
			v0 = GSVector4i::load((int)v[0].p.xy);
			v1 = GSVector4i::load((int)v[1].p.xy);
			v2 = GSVector4i::load((int)v[2].p.xy);
			pmin = v0.min_u16(v1).min_u16(v2).upl16();
			pmax = v0.max_u16(v1).max_u16(v2).upl16();
			break;
		}

		#else

		switch(prim)
		{
		case GS_POINTLIST:
			pmin.x = v[0].p.x;
			pmin.y = v[0].p.y;
			pmax.x = v[0].p.x;
			pmax.y = v[0].p.y;
			break;
		case GS_LINELIST:
		case GS_LINESTRIP:
		case GS_SPRITE:
			pmin.x = std::min<uint16>(v[0].p.x, v[1].p.x);
			pmin.y = std::min<uint16>(v[0].p.y, v[1].p.y);
			pmax.x = std::max<uint16>(v[0].p.x, v[1].p.x);
			pmax.y = std::max<uint16>(v[0].p.y, v[1].p.y);
			break;
		case GS_TRIANGLELIST:
		case GS_TRIANGLESTRIP:
		case GS_TRIANGLEFAN:
			pmin.x = std::min<uint16>(std::min<uint16>(v[0].p.x, v[1].p.x), v[2].p.x);
			pmin.y = std::min<uint16>(std::min<uint16>(v[0].p.y, v[1].p.y), v[2].p.y);
			pmax.x = std::max<uint16>(std::max<uint16>(v[0].p.x, v[1].p.x), v[2].p.x);
			pmax.y = std::max<uint16>(std::max<uint16>(v[0].p.y, v[1].p.y), v[2].p.y);
			break;
		}

		#endif

		GSVector4i test = (pmax < scissor) | (pmin > scissor.zwxy());

		switch(prim)
		{
		case GS_TRIANGLELIST:
		case GS_TRIANGLESTRIP:
		case GS_TRIANGLEFAN:
		case GS_SPRITE:
			test |= pmin == pmax;
			break;
		}

		if(test.mask() & 0xff)
		{
			return;
		}

		m_count += count;
	}
}

void GSRendererDX10::Draw(GSTexture* rt, GSTexture* ds, GSTextureCache::Source* tex)
{
	switch(m_vt.m_primclass)
	{
	case GS_POINT_CLASS:
		m_topology = D3D10_PRIMITIVE_TOPOLOGY_POINTLIST;
		m_perfmon.Put(GSPerfMon::Prim, m_count);
		break;
	case GS_LINE_CLASS: 
	case GS_SPRITE_CLASS:
		m_topology = D3D10_PRIMITIVE_TOPOLOGY_LINELIST;
		m_perfmon.Put(GSPerfMon::Prim, m_count / 2);
		break;
	case GS_TRIANGLE_CLASS:
		m_topology = D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		m_perfmon.Put(GSPerfMon::Prim, m_count / 3);
		break;
	default:
		__assume(0);
	}

	__super::Draw(rt, ds, tex);
}

void GSRendererDX10::SetupDATE(GSTexture* rt, GSTexture* ds)
{
	if(!m_context->TEST.DATE) return; // || (::GetAsyncKeyState(VK_CONTROL) & 0x8000)

	GSDevice10* dev = (GSDevice10*)m_dev;

	const GSVector2i& size = rt->GetSize();

	if(GSTexture* t = dev->CreateRenderTarget(size.x, size.y, rt->IsMSAA()))
	{
		// sfex3 (after the capcom logo), vf4 (first menu fading in), ffxii shadows, rumble roses shadows, persona4 shadows

		dev->BeginScene();

		dev->ClearStencil(ds, 0);

		// om

		dev->OMSetDepthStencilState(m_date.dss, 1);
		dev->OMSetBlendState(m_date.bs, 0);
		dev->OMSetRenderTargets(t, ds);

		// ia

		GSVector4 s = GSVector4(rt->GetScale().x / size.x, rt->GetScale().y / size.y);
		GSVector4 o = GSVector4(-1.0f, 1.0f);

		GSVector4 src = ((m_vt.m_min.p.xyxy(m_vt.m_max.p) + o.xxyy()) * s.xyxy()).sat(o.zzyy());
		GSVector4 dst = src * 2.0f + o.xxxx();

		GSVertexPT1 vertices[] =
		{
			{GSVector4(dst.x, -dst.y, 0.5f, 1.0f), GSVector2(src.x, src.y)},
			{GSVector4(dst.z, -dst.y, 0.5f, 1.0f), GSVector2(src.z, src.y)},
			{GSVector4(dst.x, -dst.w, 0.5f, 1.0f), GSVector2(src.x, src.w)},
			{GSVector4(dst.z, -dst.w, 0.5f, 1.0f), GSVector2(src.z, src.w)},
		};

		dev->IASetVertexBuffer(vertices, sizeof(vertices[0]), countof(vertices));
		dev->IASetInputLayout(dev->m_convert.il);
		dev->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		// vs

		dev->VSSetShader(dev->m_convert.vs, NULL);

		// gs

		dev->GSSetShader(NULL);

		// ps

		GSTexture* rt2 = rt->IsMSAA() ? dev->Resolve(rt) : rt;

		dev->PSSetShaderResources(rt2, NULL);
		dev->PSSetShader(dev->m_convert.ps[m_context->TEST.DATM ? 2 : 3], NULL);
		dev->PSSetSamplerState(dev->m_convert.pt, NULL);

		// 

		dev->DrawPrimitive();

		//

		dev->EndScene();

		dev->Recycle(t);

		if(rt2 != rt) dev->Recycle(rt2);
	}
}
