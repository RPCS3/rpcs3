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
#include "GSRendererSW.h"

const GSVector4 g_pos_scale(1.0f / 16, 1.0f / 16, 1.0f, 128.0f);

GSRendererSW::GSRendererSW(uint8* base, bool mt, void (*irq)(), GSDevice* dev)
	: GSRendererT(base, mt, irq, dev)
{
	m_tc = new GSTextureCacheSW(this);

	memset(m_texture, 0, sizeof(m_texture));

	m_rl.Create<GSDrawScanline>(this, theApp.GetConfig("swthreads", 1));

	InitVertexKick<GSRendererSW>();
}

GSRendererSW::~GSRendererSW()
{
	delete m_tc;
}

void GSRendererSW::Reset() 
{
	// TODO: GSreset can come from the main thread too => crash
	// m_tc->RemoveAll();

	m_reset = true;

	__super::Reset();
}

void GSRendererSW::VSync(int field)
{
	__super::VSync(field);

	m_tc->IncAge();

	if(m_reset)
	{
		m_tc->RemoveAll();

		m_reset = false;
	}

	// if((m_perfmon.GetFrame() & 255) == 0) m_rl.PrintStats();
}

void GSRendererSW::ResetDevice() 
{
	for(int i = 0; i < countof(m_texture); i++)
	{
		delete m_texture[i];

		m_texture[i] = NULL;
	}
}

GSTexture* GSRendererSW::GetOutput(int i)
{
	const GSRegDISPFB& DISPFB = m_regs->DISP[i].DISPFB;

	GIFRegTEX0 TEX0;

	TEX0.TBP0 = DISPFB.Block();
	TEX0.TBW = DISPFB.FBW;
	TEX0.PSM = DISPFB.PSM;

	GSVector4i r(0, 0, TEX0.TBW * 64, GetFrameRect(i).bottom);

	// TODO: round up bottom

	int w = r.width();
	int h = r.height();

	if(m_texture[i])
	{
		if(m_texture[i]->GetWidth() != w || m_texture[i]->GetHeight() != h)
		{
			delete m_texture[i];

			m_texture[i] = NULL;
		}
	}

	if(!m_texture[i])
	{
		m_texture[i] = m_dev->CreateTexture(w, h);

		if(!m_texture[i])
		{
			return NULL;
		}
	}

	// TODO
	static uint8* buff = (uint8*)_aligned_malloc(1024 * 1024 * 4, 16);
	static int pitch = 1024 * 4;

	m_mem.ReadTexture(r, buff, pitch, TEX0, m_env.TEXA);

	m_texture[i]->Update(r, buff, pitch);

	if(s_dump)
	{
		if(s_save)
		{
			m_texture[i]->Save(format("c:\\temp1\\_%05d_f%I64d_fr%d_%05x_%d.bmp", s_n, m_perfmon.GetFrame(), i, (int)TEX0.TBP0, (int)TEX0.PSM));
		}

		s_n++;
	}

	return m_texture[i];
}

void GSRendererSW::Draw()
{
	GS_PRIM_CLASS primclass = GSUtil::GetPrimClass(PRIM->PRIM);

	m_vt.Update(m_vertices, m_count, primclass, PRIM, m_context);

	if(m_dump) 
	{
		m_dump.Object(m_vertices, m_count, primclass);
	}

	GSScanlineParam p;

	GetScanlineParam(p, primclass);

	if((p.fm & p.zm) == 0xffffffff)
	{
		return;
	}

	if(s_dump)
	{
		uint64 frame = m_perfmon.GetFrame();

		string s;

		if(s_save && PRIM->TME) 
		{
			s = format("c:\\temp1\\_%05d_f%I64d_tex_%05x_%d.bmp", s_n, frame, (int)m_context->TEX0.TBP0, (int)m_context->TEX0.PSM);

			m_mem.SaveBMP(s, m_context->TEX0.TBP0, m_context->TEX0.TBW, m_context->TEX0.PSM, 1 << m_context->TEX0.TW, 1 << m_context->TEX0.TH);
		}

		s_n++;

		if(s_save)
		{
			s = format("c:\\temp1\\_%05d_f%I64d_rt0_%05x_%d.bmp", s_n, frame, m_context->FRAME.Block(), m_context->FRAME.PSM);

			m_mem.SaveBMP(s, m_context->FRAME.Block(), m_context->FRAME.FBW, m_context->FRAME.PSM, GetFrameRect().width(), 512);//GetFrameSize(1).cy);
		}

		if(s_savez)
		{
			s = format("c:\\temp1\\_%05d_f%I64d_rz0_%05x_%d.bmp", s_n, frame, m_context->ZBUF.Block(), m_context->ZBUF.PSM);

			m_mem.SaveBMP(s, m_context->ZBUF.Block(), m_context->FRAME.FBW, m_context->ZBUF.PSM, GetFrameRect().width(), 512);
		}

		s_n++;
	}

	GSRasterizerData data;

	data.scissor = GSVector4i(m_context->scissor.in);
	data.scissor.z = min(data.scissor.z, (int)m_context->FRAME.FBW * 64); // TODO: find a game that overflows and check which one is the right behaviour
	data.primclass = primclass;
	data.vertices = m_vertices;
	data.count = m_count;
	data.param = &p;

	m_rl.Draw(&data);

	GSRasterizerStats stats;

	m_rl.GetStats(stats);

	m_perfmon.Put(GSPerfMon::Draw, 1);
	m_perfmon.Put(GSPerfMon::Prim, stats.prims);
	m_perfmon.Put(GSPerfMon::Fillrate, stats.pixels);

	GSVector4i r = GSVector4i(m_vt.m_min.p.xyxy(m_vt.m_max.p)).rintersect(data.scissor);

	GIFRegBITBLTBUF BITBLTBUF;

	BITBLTBUF.DBW = m_context->FRAME.FBW;

	if(p.fm != 0xffffffff)
	{
		BITBLTBUF.DBP = m_context->FRAME.Block();
		BITBLTBUF.DPSM = m_context->FRAME.PSM;

		m_tc->InvalidateVideoMem(BITBLTBUF, r);
	}

	if(p.zm != 0xffffffff)
	{
		BITBLTBUF.DBP = m_context->ZBUF.Block();
		BITBLTBUF.DPSM = m_context->ZBUF.PSM;

		m_tc->InvalidateVideoMem(BITBLTBUF, r);
	}

	if(s_dump)
	{
		uint64 frame = m_perfmon.GetFrame();

		string s;

		if(s_save)
		{
			s = format("c:\\temp1\\_%05d_f%I64d_rt1_%05x_%d.bmp", s_n, frame, m_context->FRAME.Block(), m_context->FRAME.PSM);

			m_mem.SaveBMP(s, m_context->FRAME.Block(), m_context->FRAME.FBW, m_context->FRAME.PSM, GetFrameRect().width(), 512);//GetFrameSize(1).cy);
		}

		if(s_savez)
		{
			s = format("c:\\temp1\\_%05d_f%I64d_rz1_%05x_%d.bmp", s_n, frame, m_context->ZBUF.Block(), m_context->ZBUF.PSM);

			m_mem.SaveBMP(s, m_context->ZBUF.Block(), m_context->FRAME.FBW, m_context->ZBUF.PSM, GetFrameRect().width(), 512);
		}

		s_n++;
	}

	if(0)//stats.ticks > 5000000)
	{
		printf("* [%I64d | %012I64x] ticks %I64d prims %d (%d) pixels %d (%d)\n", 
			m_perfmon.GetFrame(), p.sel.key, 
			stats.ticks, 
			stats.prims, stats.prims > 0 ? (int)(stats.ticks / stats.prims) : -1, 
			stats.pixels, stats.pixels > 0 ? (int)(stats.ticks / stats.pixels) : -1);
	}
}

void GSRendererSW::InvalidateVideoMem(const GIFRegBITBLTBUF& BITBLTBUF, const GSVector4i& r)
{
	m_tc->InvalidateVideoMem(BITBLTBUF, r);
}

void GSRendererSW::GetScanlineParam(GSScanlineParam& p, GS_PRIM_CLASS primclass)
{
	const GSDrawingEnvironment& env = m_env;
	const GSDrawingContext* context = m_context;

	p.vm = m_mem.m_vm8;

	p.fbo = m_mem.GetOffset(context->FRAME.Block(), context->FRAME.FBW, context->FRAME.PSM);
	p.zbo = m_mem.GetOffset(context->ZBUF.Block(), context->FRAME.FBW, context->ZBUF.PSM);
	p.fzbo = m_mem.GetOffset4(context->FRAME, context->ZBUF);

	p.sel.key = 0;

	p.sel.fpsm = 3;
	p.sel.zpsm = 3;
	p.sel.atst = ATST_ALWAYS;
	p.sel.tfx = TFX_NONE;
	p.sel.ababcd = 255;
	p.sel.sprite = primclass == GS_SPRITE_CLASS ? 1 : 0;

	p.fm = context->FRAME.FBMSK;
	p.zm = context->ZBUF.ZMSK || context->TEST.ZTE == 0 ? 0xffffffff : 0;

	if(context->TEST.ZTE && context->TEST.ZTST == ZTST_NEVER)
	{
		p.fm = 0xffffffff;
		p.zm = 0xffffffff;
	}

	if(PRIM->TME)
	{
		m_mem.m_clut.Read32(context->TEX0, env.TEXA);
	}

	if(context->TEST.ATE)
	{
		if(!TryAlphaTest(p.fm, p.zm))
		{
			p.sel.atst = context->TEST.ATST;
			p.sel.afail = context->TEST.AFAIL;
		}
	}

	bool fwrite = p.fm != 0xffffffff;
	bool ftest = p.sel.atst != ATST_ALWAYS || context->TEST.DATE && context->FRAME.PSM != PSM_PSMCT24;

	p.sel.fwrite = fwrite;
	p.sel.ftest = ftest;

	if(fwrite || ftest)
	{
		p.sel.fpsm = GSUtil::EncodePSM(context->FRAME.PSM);

		if((primclass == GS_LINE_CLASS || primclass == GS_TRIANGLE_CLASS) && m_vt.m_eq.rgba != 0xffff)
		{
			p.sel.iip = PRIM->IIP;
		}

		if(PRIM->TME)
		{
			p.sel.tfx = context->TEX0.TFX;
			p.sel.tcc = context->TEX0.TCC;
			p.sel.fst = PRIM->FST;
			p.sel.ltf = context->TEX1.IsLinear();
			p.sel.tlu = GSLocalMemory::m_psm[context->TEX0.PSM].pal > 0;
			p.sel.wms = context->CLAMP.WMS;
			p.sel.wmt = context->CLAMP.WMT;

			if(/*p.sel.iip == 0 &&*/ p.sel.tfx == TFX_MODULATE && p.sel.tcc && m_vt.m_eq.rgba == 0xffff && m_vt.m_min.c.eq(GSVector4i(128)))
			{
				// modulate does not do anything when vertex color is 0x80

				p.sel.tfx = TFX_DECAL;
			}

			if(p.sel.fst == 0)
			{
				// skip per pixel division if q is constant

				GSVertexSW* v = m_vertices;

				if(m_vt.m_eq.q)
				{
					p.sel.fst = 1;

					if(v[0].t.z != 1.0f)
					{
						GSVector4 w = v[0].t.zzzz().rcpnr();

						for(int i = 0, j = m_count; i < j; i++)
						{
							v[i].t *= w;
						}
					}
				}
				else if(primclass == GS_SPRITE_CLASS)
				{
					p.sel.fst = 1;

					for(int i = 0, j = m_count; i < j; i += 2)
					{
						GSVector4 w = v[i + 1].t.zzzz().rcpnr();

						v[i + 0].t *= w;
						v[i + 1].t *= w;
					}
				}
			}

			if(p.sel.ltf)
			{
				GSVector4 half(0x8000, 0x8000);

				if(p.sel.fst)
				{
					// if q is constant we can do the half pel shift for bilinear sampling on the vertices

					GSVertexSW* v = m_vertices;

					for(int i = 0, j = m_count; i < j; i++)
					{
						v[i].t -= half;
					}
				}
			}

			GSVector4i r;

			GetTextureMinMax(r);

			const GSTextureCacheSW::GSTexture* t = m_tc->Lookup(context->TEX0, env.TEXA, r);

			if(!t) {ASSERT(0); return;}

			p.tex = t->m_buff;
			p.clut = m_mem.m_clut;
			p.tw = t->m_tw;
		}

		p.sel.fge = PRIM->FGE;

		if(context->FRAME.PSM != PSM_PSMCT24)
		{
			p.sel.date = context->TEST.DATE;
			p.sel.datm = context->TEST.DATM;
		}

		if(!IsOpaque())
		{
			p.sel.abe = PRIM->ABE;
			p.sel.ababcd = context->ALPHA.u32[0];

			if(env.PABE.PABE)
			{
				p.sel.pabe = 1;
			}

			if(PRIM->AA1 && (primclass == GS_LINE_CLASS || primclass == GS_TRIANGLE_CLASS))
			{
				p.sel.aa1 = m_aa1 ? 1 : 0;
			}
		}

		if(p.sel.date 
		|| p.sel.aba == 1 || p.sel.abb == 1 || p.sel.abc == 1 || p.sel.abd == 1
		|| p.sel.atst != ATST_ALWAYS && p.sel.afail == AFAIL_RGB_ONLY 
		|| p.sel.fpsm == 0 && p.fm != 0 && p.fm != 0xffffffff
		|| p.sel.fpsm == 1 && (p.fm & 0x00ffffff) != 0 && (p.fm & 0x00ffffff) != 0x00ffffff
		|| p.sel.fpsm == 2 && (p.fm & 0x80f8f8f8) != 0 && (p.fm & 0x80f8f8f8) != 0x80f8f8f8)
		{
			p.sel.rfb = 1;
		}

		p.sel.colclamp = env.COLCLAMP.CLAMP;
		p.sel.fba = context->FBA.FBA;
		p.sel.dthe = env.DTHE.DTHE;
	}

	bool zwrite = p.zm != 0xffffffff;
	bool ztest = context->TEST.ZTE && context->TEST.ZTST > 1;

	p.sel.zwrite = zwrite;
	p.sel.ztest = ztest;

	if(zwrite || ztest)
	{
		p.sel.zpsm = GSUtil::EncodePSM(context->ZBUF.PSM);
		p.sel.ztst = ztest ? context->TEST.ZTST : 1;
		p.sel.zoverflow = GSVector4i(m_vt.m_max.p).z == 0x80000000;
	}
}

template<uint32 prim, uint32 tme, uint32 fst> 
void GSRendererSW::VertexKick(bool skip)
{
	const GSDrawingContext* context = m_context;

	GSVector4i xy = GSVector4i::load((int)m_v.XYZ.u32[0]);
	
	xy = xy.insert16<3>(m_v.FOG.F);
	xy = xy.upl16();
	xy -= context->XYOFFSET;

	GSVertexSW v;

	v.p = GSVector4(xy) * g_pos_scale;

	v.c = GSVector4(GSVector4i::load((int)m_v.RGBAQ.u32[0]).u8to32() << 7);

	if(tme)
	{
		float q;

		if(fst)
		{
			v.t = GSVector4(((GSVector4i)m_v.UV).upl16() << (16 - 4));
			q = 1.0f;
		}
		else
		{
			v.t = GSVector4(m_v.ST.S, m_v.ST.T);
			v.t *= GSVector4(0x10000 << context->TEX0.TW, 0x10000 << context->TEX0.TH);
			q = m_v.RGBAQ.Q;
		}

		v.t = v.t.xyxy(GSVector4::load(q));
	}

	GSVertexSW& dst = m_vl.AddTail();

	dst = v;

	dst.p.z = (float)min(m_v.XYZ.Z, 0xffffff00); // max value which can survive the uint32 => float => uint32 conversion

	int count = 0;
	
	if(GSVertexSW* v = DrawingKick<prim>(skip, count))
	{
if(!m_dump)
{
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
			pmin = v[0].p.minv(v[1].p);
			pmax = v[0].p.maxv(v[1].p);
			break;
		case GS_TRIANGLELIST:
		case GS_TRIANGLESTRIP:
		case GS_TRIANGLEFAN:
			pmin = v[0].p.minv(v[1].p).minv(v[2].p);
			pmax = v[0].p.maxv(v[1].p).maxv(v[2].p);
			break;
		}

		GSVector4 scissor = context->scissor.ex;

		GSVector4 test = (pmax < scissor) | (pmin > scissor.zwxy());

		switch(prim)
		{
		case GS_TRIANGLELIST:
		case GS_TRIANGLESTRIP:
		case GS_TRIANGLEFAN:
		case GS_SPRITE:
			test |= pmin.ceil() == pmax.ceil();
			break;
		}

		switch(prim)
		{
		case GS_TRIANGLELIST:
		case GS_TRIANGLESTRIP:
		case GS_TRIANGLEFAN:
			// are in line or just two of them are the same (cross product == 0)
			GSVector4 tmp = (v[1].p - v[0].p) * (v[2].p - v[0].p).yxwz();
			test |= tmp == tmp.yxwz();
			break;
		}
		
		if(test.mask() & 3)
		{
			return;
		}
}
		switch(prim)
		{
		case GS_POINTLIST:
			break;
		case GS_LINELIST:
		case GS_LINESTRIP:
			if(PRIM->IIP == 0) {v[0].c = v[1].c;}
			break;
		case GS_TRIANGLELIST:
		case GS_TRIANGLESTRIP:
		case GS_TRIANGLEFAN:
			if(PRIM->IIP == 0) {v[0].c = v[2].c; v[1].c = v[2].c;}
			break;
		case GS_SPRITE:
			break;
		}

		if(m_count < 30 && m_count >= 3)
		{
			GSVertexSW* v = &m_vertices[m_count - 3];

			int tl = 0;
			int br = 0;

			bool isquad = false;

			switch(prim)
			{
			case GS_TRIANGLESTRIP:
			case GS_TRIANGLEFAN:
			case GS_TRIANGLELIST:
				isquad = GSVertexSW::IsQuad(v, tl, br);
				break;
			}

			if(isquad)
			{
				m_count -= 3;

				if(m_count > 0)
				{
					tl += m_count;
					br += m_count;

					Flush();
				}

				if(tl != 0) m_vertices[0] = m_vertices[tl];
				if(br != 1) m_vertices[1] = m_vertices[br];

				m_count = 2;

				uint32 tmp = PRIM->PRIM;
				PRIM->PRIM = GS_SPRITE;

				Flush();

				PRIM->PRIM = tmp;

				m_perfmon.Put(GSPerfMon::Quad, 1);

				return;
			}
		}

		m_count += count;
	}
}
