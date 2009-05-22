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

#include "StdAfx.h"
#include "GPURendererSW.h"

GPURendererSW::GPURendererSW(const GPURendererSettings& rs, GSDevice* dev, int threads)
	: GPURendererT(rs)
	, m_texture(NULL)
{
	m_dev = dev;

	m_rl.Create<GPUDrawScanline>(this, threads);

	m_fpDrawingKickHandlers[GPU_POLYGON] = (DrawingKickHandler)&GPURendererSW::DrawingKickTriangle;
	m_fpDrawingKickHandlers[GPU_LINE] = (DrawingKickHandler)&GPURendererSW::DrawingKickLine;
	m_fpDrawingKickHandlers[GPU_SPRITE] = (DrawingKickHandler)&GPURendererSW::DrawingKickSprite;
}

GPURendererSW::~GPURendererSW()
{
}

void GPURendererSW::ResetDevice() 
{
	delete m_texture;

	m_texture = NULL;
}

GSVector4i GPURendererSW::GetScissor()
{
	GSVector4i r;

	r.left = (int)m_env.DRAREATL.X << m_scale.x;
	r.top = (int)m_env.DRAREATL.Y << m_scale.y;
	r.right = min((int)(m_env.DRAREABR.X + 1) << m_scale.x, m_mem.GetWidth());
	r.bottom = min((int)(m_env.DRAREABR.Y + 1) << m_scale.y, m_mem.GetHeight());

	return r;
}

void GPURendererSW::VertexKick()
{
	GSVertexSW& v = m_vl.AddTail();

	// TODO: x/y + off.x/y should wrap around at +/-1024

	int x = (int)(m_v.XY.X + m_env.DROFF.X) << m_scale.x;
	int y = (int)(m_v.XY.Y + m_env.DROFF.Y) << m_scale.y;

	int s = m_v.UV.X;
	int t = m_v.UV.Y;

	GSVector4 pt(x, y, s, t);

	v.p = pt.xyxy(GSVector4::zero());
	v.t = (pt.zwzw(GSVector4::zero()) + GSVector4(0.125f)) * 256.0f;
	v.c = GSVector4(m_v.RGB.u32) * 128.0f;

	__super::VertexKick();
}

void GPURendererSW::DrawingKickTriangle(GSVertexSW* v, int& count)
{
	// TODO
}

void GPURendererSW::DrawingKickLine(GSVertexSW* v, int& count)
{
	// TODO
}

void GPURendererSW::DrawingKickSprite(GSVertexSW* v, int& count)
{
	// TODO
}

void GPURendererSW::Draw()
{
	const GPUDrawingEnvironment& env = m_env;

	//

	GPUScanlineParam p;

	p.sel.key = 0;
	p.sel.iip = env.PRIM.IIP;
	p.sel.me = env.STATUS.ME;

	if(env.PRIM.ABE)
	{
		p.sel.abe = env.PRIM.ABE;
		p.sel.abr = env.STATUS.ABR;
	}

	p.sel.tge = env.PRIM.TGE;

	if(env.PRIM.TME)
	{
		p.sel.tme = env.PRIM.TME;
		p.sel.tlu = env.STATUS.TP < 2;
		p.sel.twin = (env.TWIN.u32 & 0xfffff) != 0;
		p.sel.ltf = m_filter == 1 && env.PRIM.TYPE == GPU_POLYGON || m_filter == 2 ? 1 : 0;

		const void* t = m_mem.GetTexture(env.STATUS.TP, env.STATUS.TX, env.STATUS.TY);

		if(!t) {ASSERT(0); return;}

		p.tex = t;
		p.clut = m_mem.GetCLUT(env.STATUS.TP, env.CLUT.X, env.CLUT.Y);
	}

	p.sel.dtd = m_dither ? env.STATUS.DTD : 0;
	p.sel.md = env.STATUS.MD;
	p.sel.sprite = env.PRIM.TYPE == GPU_SPRITE;
	p.sel.scalex = m_mem.GetScale().x;

	//

	GSRasterizerData data;

	data.scissor = GetScissor();
	data.vertices = m_vertices;
	data.count = m_count;
	data.param = &p;

	switch(env.PRIM.TYPE)
	{
	case GPU_POLYGON: data.primclass = GS_TRIANGLE_CLASS; break;
	case GPU_LINE: data.primclass = GS_LINE_CLASS; break;
	case GPU_SPRITE: data.primclass = GS_SPRITE_CLASS; break;
	default: __assume(0);
	}

	m_rl.Draw(&data);

	GSRasterizerStats stats;

	m_rl.GetStats(stats);

	m_perfmon.Put(GSPerfMon::Draw, 1);
	m_perfmon.Put(GSPerfMon::Prim, stats.prims);
	m_perfmon.Put(GSPerfMon::Fillrate, stats.pixels); 

	// TODO

	{
		GSVector4 tl(+1e10f);
		GSVector4 br(-1e10f);

		for(int i = 0, j = m_count; i < j; i++)
		{
			GSVector4 p = m_vertices[i].p;

			tl = tl.minv(p);
			br = br.maxv(p);
		}

		GSVector4i r = GSVector4i(tl.xyxy(br)).rintersect(data.scissor);

		r.left >>= m_scale.x;
		r.top >>= m_scale.y;
		r.right >>= m_scale.x;
		r.bottom >>= m_scale.y;

		Invalidate(r);
	}
}

GSTexture* GPURendererSW::GetOutput()
{
	GSVector4i r = m_env.GetDisplayRect();

	r.left <<= m_scale.x;
	r.top <<= m_scale.y;
	r.right <<= m_scale.x;
	r.bottom <<= m_scale.y;

	// TODO
	static uint32* buff = (uint32*)_aligned_malloc(m_mem.GetWidth() * m_mem.GetHeight() * sizeof(uint32), 16);

	m_mem.ReadFrame32(r, buff, !!m_env.STATUS.ISRGB24);

	int w = r.width();
	int h = r.height();

	if(m_texture)
	{
		if(m_texture->GetWidth() != w || m_texture->GetHeight() != h)
		{
			delete m_texture;

			m_texture = NULL;
		}
	}

	if(!m_texture)
	{
		m_texture = m_dev->CreateTexture(w, h);

		if(!m_texture)
		{
			return NULL;
		}
	}

	m_texture->Update(GSVector4i(0, 0, w, h), buff, m_mem.GetWidth() * sizeof(uint32));

	return m_texture;
}

