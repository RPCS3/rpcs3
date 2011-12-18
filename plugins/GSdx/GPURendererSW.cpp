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
#include "GPURendererSW.h"
//#include "GSdx.h"

GPURendererSW::GPURendererSW(GSDevice* dev, int threads)
	: GPURendererT<GSVertexSW>(dev)
	, m_texture(NULL)
{
	m_output = (uint32*)_aligned_malloc(m_mem.GetWidth() * m_mem.GetHeight() * sizeof(uint32), 16);

	m_rl = GSRasterizerList::Create<GPUDrawScanline>(threads);
}

GPURendererSW::~GPURendererSW()
{
	delete m_texture;

	delete m_rl;

	_aligned_free(m_output);
}

void GPURendererSW::ResetDevice()
{
	delete m_texture;

	m_texture = NULL;
}

GSTexture* GPURendererSW::GetOutput()
{
	GSVector4i r = m_env.GetDisplayRect();

	r.left <<= m_scale.x;
	r.top <<= m_scale.y;
	r.right <<= m_scale.x;
	r.bottom <<= m_scale.y;

	if(m_dev->ResizeTexture(&m_texture, r.width(), r.height()))
	{
		m_mem.ReadFrame32(r, m_output, !!m_env.STATUS.ISRGB24);

		m_texture->Update(r.rsize(), m_output, m_mem.GetWidth() * sizeof(uint32));
	}

	return m_texture;
}

void GPURendererSW::Draw()
{
	class GPURasterizerData : public GSRasterizerData
	{
	public:
		GPURasterizerData()
		{
			GPUScanlineGlobalData* gd = (GPUScanlineGlobalData*)_aligned_malloc(sizeof(GPUScanlineGlobalData), 32);

			gd->clut = NULL;

			param = gd;
		}

		virtual ~GPURasterizerData()
		{
			GPUScanlineGlobalData* gd = (GPUScanlineGlobalData*)param;

			if(gd->clut) _aligned_free(gd->clut);

			_aligned_free(gd);
		}
	};	

	shared_ptr<GSRasterizerData> data(new GPURasterizerData());

	GPUScanlineGlobalData& gd = *(GPUScanlineGlobalData*)data->param;

	const GPUDrawingEnvironment& env = m_env;

	gd.sel.key = 0;
	gd.sel.iip = env.PRIM.IIP;
	gd.sel.me = env.STATUS.ME;

	if(env.PRIM.ABE)
	{
		gd.sel.abe = env.PRIM.ABE;
		gd.sel.abr = env.STATUS.ABR;
	}

	gd.sel.tge = env.PRIM.TGE;

	if(env.PRIM.TME)
	{
		gd.sel.tme = env.PRIM.TME;
		gd.sel.tlu = env.STATUS.TP < 2;
		gd.sel.twin = (env.TWIN.u32 & 0xfffff) != 0;
		gd.sel.ltf = m_filter == 1 && env.PRIM.TYPE == GPU_POLYGON || m_filter == 2 ? 1 : 0;

		const void* t = m_mem.GetTexture(env.STATUS.TP, env.STATUS.TX, env.STATUS.TY);

		if(!t) {ASSERT(0); return;}

		gd.tex = t;

		gd.clut = (uint16*)_aligned_malloc(sizeof(uint16) * 256, 32);

		memcpy(gd.clut, m_mem.GetCLUT(env.STATUS.TP, env.CLUT.X, env.CLUT.Y), sizeof(uint16) * (env.STATUS.TP == 0 ? 16 : 256));

		gd.twin = GSVector4i(env.TWIN.TWW, env.TWIN.TWH, env.TWIN.TWX, env.TWIN.TWY);
	}

	gd.sel.dtd = m_dither ? env.STATUS.DTD : 0;
	gd.sel.md = env.STATUS.MD;
	gd.sel.sprite = env.PRIM.TYPE == GPU_SPRITE;
	gd.sel.scalex = m_mem.GetScale().x;

	gd.vm = m_mem.GetPixelAddress(0, 0);

	data->vertices = (GSVertexSW*)_aligned_malloc(sizeof(GSVertexSW) * m_count, 16);
	memcpy(data->vertices, m_vertices, sizeof(GSVertexSW) * m_count);
	data->count = m_count;

	data->frame = m_perfmon.GetFrame();

	data->scissor.left = (int)m_env.DRAREATL.X << m_scale.x;
	data->scissor.top = (int)m_env.DRAREATL.Y << m_scale.y;
	data->scissor.right = min((int)(m_env.DRAREABR.X + 1) << m_scale.x, m_mem.GetWidth());
	data->scissor.bottom = min((int)(m_env.DRAREABR.Y + 1) << m_scale.y, m_mem.GetHeight());

	switch(env.PRIM.TYPE)
	{
	case GPU_POLYGON: data->primclass = GS_TRIANGLE_CLASS; break;
	case GPU_LINE: data->primclass = GS_LINE_CLASS; break;
	case GPU_SPRITE: data->primclass = GS_SPRITE_CLASS; break;
	default: __assume(0);
	}

	// TODO: VertexTrace

	GSVector4 tl(+1e10f);
	GSVector4 br(-1e10f);

	GSVertexSW* v = data->vertices;

	for(int i = 0, j = m_count; i < j; i++)
	{
		GSVector4 p = v[i].p;

		tl = tl.min(p);
		br = br.max(p);
	}

	data->bbox = GSVector4i(tl.xyxy(br));

	GSVector4i r = data->bbox.rintersect(data->scissor);

	r.left >>= m_scale.x;
	r.top >>= m_scale.y;
	r.right >>= m_scale.x;
	r.bottom >>= m_scale.y;

	Invalidate(r);

	m_rl->Queue(data);

	m_rl->Sync();

	// TODO: m_perfmon.Put(GSPerfMon::Draw, 1);
	// TODO: m_perfmon.Put(GSPerfMon::Prim, stats.prims);
	// TODO: m_perfmon.Put(GSPerfMon::Fillrate, stats.pixels);
}

void GPURendererSW::VertexKick()
{
	GSVertexSW& dst = m_vl.AddTail();

	// TODO: x/y + off.x/y should wrap around at +/-1024

	int x = (int)(m_v.XY.X + m_env.DROFF.X) << m_scale.x;
	int y = (int)(m_v.XY.Y + m_env.DROFF.Y) << m_scale.y;

	int u = m_v.UV.X;
	int v = m_v.UV.Y;

	GSVector4 pt(x, y, u, v);

	dst.p = pt.xyxy(GSVector4::zero());
	dst.t = (pt.zwzw(GSVector4::zero()) + GSVector4(0.125f)) * 256.0f;
	// dst.c = GSVector4(m_v.RGB.u32) * 128.0f;
	dst.c = GSVector4(GSVector4i::load((int)m_v.RGB.u32).u8to32() << 7);

	int count = 0;

	if(GSVertexSW* v = DrawingKick(count))
	{
		// TODO

		m_count += count;
	}
}

