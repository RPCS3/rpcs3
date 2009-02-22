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

#include "GPURenderer.h"
#include "GPUDrawScanline.h"

template <class Device>
class GPURendererSW : public GPURenderer<Device, GSVertexSW>
{
protected:
	GSRasterizerList m_rl;
	Texture m_texture;

	void ResetDevice() 
	{
		m_texture = Texture();
	}

	bool GetOutput(Texture& t)
	{
		CRect r = m_env.GetDisplayRect();

		r.left <<= m_scale.cx;
		r.top <<= m_scale.cy;
		r.right <<= m_scale.cx;
		r.bottom <<= m_scale.cy;

		// TODO
		static DWORD* buff = (DWORD*)_aligned_malloc(m_mem.GetWidth() * m_mem.GetHeight() * sizeof(DWORD), 16);

		m_mem.ReadFrame32(r, buff, !!m_env.STATUS.ISRGB24);

		r.OffsetRect(-r.TopLeft());

		if(m_texture.GetWidth() != r.Width() || m_texture.GetHeight() != r.Height())
		{
			m_texture = Texture();
		}

		if(!m_texture && !m_dev.CreateTexture(m_texture, r.Width(), r.Height())) 
		{
			return false;
		}

		m_texture.Update(r, buff, m_mem.GetWidth() * sizeof(DWORD));

		t = m_texture;

		return true;
	}

	void VertexKick()
	{
		GSVertexSW& v = m_vl.AddTail();

		// TODO: x/y + off.x/y should wrap around at +/-1024

		int x = (int)(m_v.XY.X + m_env.DROFF.X) << m_scale.cx;
		int y = (int)(m_v.XY.Y + m_env.DROFF.Y) << m_scale.cy;

		int s = m_v.UV.X;
		int t = m_v.UV.Y;

		GSVector4 pt(x, y, s, t);

		v.p = pt.xyxy(GSVector4::zero());
		v.t = (pt.zwzw(GSVector4::zero()) + GSVector4(0.125f)) * 256.0f;
		v.c = GSVector4((DWORD)m_v.RGB.ai32) * 128.0f;

		__super::VertexKick();
	}

	void DrawingKickTriangle(GSVertexSW* v, int& count)
	{
		// TODO
	}

	void DrawingKickLine(GSVertexSW* v, int& count)
	{
		// TODO
	}

	void DrawingKickSprite(GSVertexSW* v, int& count)
	{
		// TODO
	}

	GSVector4i GetScissor()
	{
		GSVector4i v;

		v.x = (int)m_env.DRAREATL.X << m_scale.cx;
		v.y = (int)m_env.DRAREATL.Y << m_scale.cy;
		v.z = min((int)(m_env.DRAREABR.X + 1) << m_scale.cx, m_mem.GetWidth());
		v.w = min((int)(m_env.DRAREABR.Y + 1) << m_scale.cy, m_mem.GetHeight());

		return v;
	}

	void Draw()
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
			p.sel.twin = (env.TWIN.ai32 & 0xfffff) != 0;
			p.sel.ltf = m_filter == 1 && env.PRIM.TYPE == GPU_POLYGON || m_filter == 2 ? 1 : 0;

			const void* t = m_mem.GetTexture(env.STATUS.TP, env.STATUS.TX, env.STATUS.TY);

			if(!t) {ASSERT(0); return;}

			p.tex = t;
			p.clut = m_mem.GetCLUT(env.STATUS.TP, env.CLUT.X, env.CLUT.Y);
		}

		p.sel.dtd = m_dither ? env.STATUS.DTD : 0;
		p.sel.md = env.STATUS.MD;
		p.sel.sprite = env.PRIM.TYPE == GPU_SPRITE;
		p.sel.scalex = m_mem.GetScale().cx;

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

			GSVector4i scissor = data.scissor;

			CRect r;

			r.left = max(scissor.x, min(scissor.z, (int)tl.x)) >> m_scale.cx;
			r.top = max(scissor.y, min(scissor.w, (int)tl.y)) >> m_scale.cy;
			r.right = max(scissor.x, min(scissor.z, (int)br.x)) >> m_scale.cx;
			r.bottom = max(scissor.y, min(scissor.w, (int)br.y)) >> m_scale.cy;

			Invalidate(r);
		}
	}

public:
	GPURendererSW(const GPURendererSettings& rs, int threads)
		: GPURenderer(rs)
	{
		m_rl.Create<GPUDrawScanline>(this, threads);

		m_fpDrawingKickHandlers[GPU_POLYGON] = (DrawingKickHandler)&GPURendererSW::DrawingKickTriangle;
		m_fpDrawingKickHandlers[GPU_LINE] = (DrawingKickHandler)&GPURendererSW::DrawingKickLine;
		m_fpDrawingKickHandlers[GPU_SPRITE] = (DrawingKickHandler)&GPURendererSW::DrawingKickSprite;
	}

	virtual ~GPURendererSW()
	{
	}
};
