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

GSRendererSW::GSRendererSW(uint8* base, bool mt, void (*irq)(), const GSRendererSettings& rs, GSDevice* dev, int threads)
	: GSRendererT(base, mt, irq, rs)
{
	m_dev = dev;

	m_tc = new GSTextureCacheSW(this);

	memset(m_texture, 0, sizeof(m_texture));

	m_rl.Create<GSDrawScanline>(this, threads);

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

	GIFRegCLAMP CLAMP;

	CLAMP.WMS = CLAMP.WMT = 1;

	// TODO
	static uint8* buff = (uint8*)_aligned_malloc(1024 * 1024 * 4, 16);
	static int pitch = 1024 * 4;

	m_mem.ReadTexture(r, buff, pitch, TEX0, m_env.TEXA, CLAMP);

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

	m_vtrace.Update(m_vertices, m_count, primclass, PRIM->IIP, PRIM->TME, m_context->TEX0.TFX, m_context->TEX0.TCC);

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

	GSVector4i r = GSVector4i(m_vtrace.m_min.p.xyxy(m_vtrace.m_max.p)).rintersect(data.scissor);

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

void GSRendererSW::GetTextureMinMax(int w, int h, GSVector4i& r, uint32 fst)
{
	const GSDrawingContext* context = m_context;

	int wms = context->CLAMP.WMS;
	int wmt = context->CLAMP.WMT;

	int minu = (int)context->CLAMP.MINU;
	int minv = (int)context->CLAMP.MINV;
	int maxu = (int)context->CLAMP.MAXU;
	int maxv = (int)context->CLAMP.MAXV;

	GSVector4i vr(0, 0, w, h);

	switch(wms)
	{
	case CLAMP_REPEAT:
		break;
	case CLAMP_CLAMP:
		break;
	case CLAMP_REGION_CLAMP:
		if(vr.x < minu) vr.x = minu;
		if(vr.z > maxu + 1) vr.z = maxu + 1;
		break;
	case CLAMP_REGION_REPEAT:
		vr.x = maxu; 
		vr.z = vr.x + (minu + 1);
		break;
	default: 
		__assume(0);
	}

	switch(wmt)
	{
	case CLAMP_REPEAT:
		break;
	case CLAMP_CLAMP:
		break;
	case CLAMP_REGION_CLAMP:
		if(vr.y < minv) vr.y = minv;
		if(vr.w > maxv + 1) vr.w = maxv + 1;
		break;
	case CLAMP_REGION_REPEAT:
		vr.y = maxv; 
		vr.w = vr.y + (minv + 1);
		break;
	default:
		__assume(0);
	}

	if(fst)
	{
		GSVector4i uv = GSVector4i(m_vtrace.m_min.t.xyxy(m_vtrace.m_max.t)).sra32(16);

		GSVector4i u, v;

		int mask = 0;

		if(wms == CLAMP_REPEAT || wmt == CLAMP_REPEAT)
		{
			int tw = context->TEX0.TW;
			int th = context->TEX0.TH;

			u = uv & GSVector4i::xffffffff().srl32(32 - tw);
			v = uv & GSVector4i::xffffffff().srl32(32 - th);

			GSVector4i uu = uv.sra32(tw);
			GSVector4i vv = uv.sra32(th);

			mask = (uu.upl32(vv) == uu.uph32(vv)).mask();
		}

		switch(wms)
		{
		case CLAMP_REPEAT:
			if(mask & 0x000f) {if(vr.x < u.x) vr.x = u.x; if(vr.z > u.z + 1) vr.z = u.z + 1;}
			break;
		case CLAMP_CLAMP:
		case CLAMP_REGION_CLAMP:
			if(vr.x < uv.x) vr.x = uv.x;
			if(vr.z > uv.z + 1) vr.z = uv.z + 1;
			break;
		case CLAMP_REGION_REPEAT: // TODO
			break;
		default:
			__assume(0);
		}

		switch(wmt)
		{
		case CLAMP_REPEAT:
			if(mask & 0xf000) {if(vr.y < v.y) vr.y = v.y; if(vr.w > v.w + 1) vr.w = v.w + 1;}
			break;
		case CLAMP_CLAMP:
		case CLAMP_REGION_CLAMP:
			if(vr.y < uv.y) vr.y = uv.y;
			if(vr.w > uv.w + 1) vr.w = uv.w + 1;
			break;
		case CLAMP_REGION_REPEAT: // TODO
			break;
		default:
			__assume(0);
		}
	}

	r = vr.rintersect(GSVector4i(0, 0, w, h));
}

void GSRendererSW::GetAlphaMinMax()
{
	if(m_vtrace.m_alpha.valid)
	{
		return;
	}

	const GSDrawingEnvironment& env = m_env;
	const GSDrawingContext* context = m_context;

	GSVector4i a = GSVector4i(m_vtrace.m_min.c.wwww(m_vtrace.m_max.c)) >> 7;

	if(PRIM->TME && context->TEX0.TCC)
	{
		uint32 bpp = GSLocalMemory::m_psm[context->TEX0.PSM].trbpp;
		uint32 cbpp = GSLocalMemory::m_psm[context->TEX0.CPSM].trbpp;
		uint32 pal = GSLocalMemory::m_psm[context->TEX0.PSM].pal;

		if(bpp == 32)
		{
			a.y = 0;
			a.w = 0xff;
		}
		else if(bpp == 24)
		{
			a.y = env.TEXA.AEM ? 0 : env.TEXA.TA0;
			a.w = env.TEXA.TA0;
		}
		else if(bpp == 16)
		{
			a.y = env.TEXA.AEM ? 0 : min(env.TEXA.TA0, env.TEXA.TA1);
			a.w = max(env.TEXA.TA0, env.TEXA.TA1);
		}
		else
		{
			m_mem.m_clut.GetAlphaMinMax32(a.y, a.w);
		}

		switch(context->TEX0.TFX)
		{
		case TFX_MODULATE:
			a.x = (a.x * a.y) >> 7;
			a.z = (a.z * a.w) >> 7;
			if(a.x > 0xff) a.x = 0xff;
			if(a.z > 0xff) a.z = 0xff;
			break;
		case TFX_DECAL:
			a.x = a.y;
			a.z = a.w;
			break;
		case TFX_HIGHLIGHT:
			a.x = a.x + a.y;
			a.z = a.z + a.w;
			if(a.x > 0xff) a.x = 0xff;
			if(a.z > 0xff) a.z = 0xff;
			break;
		case TFX_HIGHLIGHT2:
			a.x = a.y;
			a.z = a.w;
			break;
		default:
			__assume(0);
		}
	}

	m_vtrace.m_alpha.min = a.x;
	m_vtrace.m_alpha.max = a.z;
	m_vtrace.m_alpha.valid = true;
}

bool GSRendererSW::TryAlphaTest(uint32& fm, uint32& zm)
{
	const GSDrawingContext* context = m_context;

	bool pass = true;

	if(context->TEST.ATST == ATST_NEVER)
	{
		pass = false;
	}
	else if(context->TEST.ATST != ATST_ALWAYS)
	{
		GetAlphaMinMax();

		int amin = m_vtrace.m_alpha.min;
		int amax = m_vtrace.m_alpha.max;

		int aref = context->TEST.AREF;

		switch(context->TEST.ATST)
		{
		case ATST_NEVER: 
			pass = false; 
			break;
		case ATST_ALWAYS: 
			pass = true; 
			break;
		case ATST_LESS: 
			if(amax < aref) pass = true;
			else if(amin >= aref) pass = false;
			else return false;
			break;
		case ATST_LEQUAL: 
			if(amax <= aref) pass = true;
			else if(amin > aref) pass = false;
			else return false;
			break;
		case ATST_EQUAL: 
			if(amin == aref && amax == aref) pass = true;
			else if(amin > aref || amax < aref) pass = false;
			else return false;
			break;
		case ATST_GEQUAL: 
			if(amin >= aref) pass = true;
			else if(amax < aref) pass = false;
			else return false;
			break;
		case ATST_GREATER: 
			if(amin > aref) pass = true;
			else if(amax <= aref) pass = false;
			else return false;
			break;
		case ATST_NOTEQUAL: 
			if(amin == aref && amax == aref) pass = false;
			else if(amin > aref || amax < aref) pass = true;
			else return false;
			break;
		default: 
			__assume(0);
		}
	}

	if(!pass)
	{
		switch(context->TEST.AFAIL)
		{
		case AFAIL_KEEP: fm = zm = 0xffffffff; break;
		case AFAIL_FB_ONLY: zm = 0xffffffff; break;
		case AFAIL_ZB_ONLY: fm = 0xffffffff; break;
		case AFAIL_RGB_ONLY: fm |= 0xff000000; zm = 0xffffffff; break;
		default: __assume(0);
		}
	}

	return true;
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

		if((primclass == GS_LINE_CLASS || primclass == GS_TRIANGLE_CLASS) && m_vtrace.m_eq.rgba != 15)
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

			if(p.sel.iip == 0 && p.sel.tfx == TFX_MODULATE && p.sel.tcc)
			{
				if(m_vtrace.m_eq.rgba == 15 && (m_vtrace.m_min.c == GSVector4(128.0f * 128.0f)).alltrue())
				{
					// modulate does not do anything when vertex color is 0x80

					p.sel.tfx = TFX_DECAL;
				}
			}

			if(p.sel.fst == 0)
			{
				// skip per pixel division if q is constant

				GSVertexSW* v = m_vertices;

				if(m_vtrace.m_eq.q)
				{
					p.sel.fst = 1;

					if(v[0].t.z != 1.0f)
					{
						GSVector4 w = v[0].t.zzzz().rcpnr();

						for(int i = 0, j = m_count; i < j; i++)
						{
							v[i].t *= w;
						}

						m_vtrace.m_min.t *= w;
						m_vtrace.m_max.t *= w;
					}
				}
				else if(primclass == GS_SPRITE_CLASS)
				{
					p.sel.fst = 1;

					GSVector4 tmin = GSVector4(FLT_MAX);
					GSVector4 tmax = GSVector4(-FLT_MAX);

					for(int i = 0, j = m_count; i < j; i += 2)
					{
						GSVector4 w = v[i + 1].t.zzzz().rcpnr();

						GSVector4 v0 = v[i + 0].t * w;
						GSVector4 v1 = v[i + 1].t * w;

						v[i + 0].t = v0;
						v[i + 1].t = v1;

						tmin = tmin.minv(v0).minv(v1);
						tmax = tmax.maxv(v0).maxv(v1);
					}

					m_vtrace.m_max.t = tmax;
					m_vtrace.m_min.t = tmin;
				}
			}

			if(p.sel.fst)
			{
				// if q is constant we can do the half pel shift for bilinear sampling on the vertices

				if(p.sel.ltf)
				{
					GSVector4 half(0x8000, 0x8000);

					GSVertexSW* v = m_vertices;

					for(int i = 0, j = m_count; i < j; i++)
					{
						v[i].t -= half;
					}

					m_vtrace.m_min.t -= half;
					m_vtrace.m_max.t += half;
				}
			}
			/*
			else
			{
				GSVector4 tmin = GSVector4(FLT_MAX);
				GSVector4 tmax = GSVector4(-FLT_MAX);

				GSVertexSW* v = m_vertices;

				for(int i = 0, j = m_count; i < j; i++)
				{
					GSVector4 v0 = v[i].t * v[i].t.zzzz().rcpnr();

					tmin = tmin.minv(v0);
					tmax = tmax.maxv(v0);
				}

				if(p.sel.ltf)
				{
					GSVector4 half(0x8000, 0x8000);

					tmin -= half;
					tmax += half;
				}

				m_vtrace.min.t = tmin;
				m_vtrace.max.t = tmax;
			}
			*/

			int w = 1 << context->TEX0.TW;
			int h = 1 << context->TEX0.TH;

			GSVector4i r;

			GetTextureMinMax(w, h, r, p.sel.fst);

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

		int amin = 0, amax = 0xff;

		if(PRIM->ABE && context->ALPHA.A != context->ALPHA.B && !PRIM->AA1)
		{
			if(context->ALPHA.C == 0)
			{
				GetAlphaMinMax();

				amin = m_vtrace.m_alpha.min;
				amax = m_vtrace.m_alpha.max;
			}
			else if(context->ALPHA.C == 1)
			{
				if(p.sel.fpsm == 1)
				{
					amin = amax = 0x80;
				}
			}
			else if(context->ALPHA.C == 1)
			{
				amin = amax = context->ALPHA.FIX;
			}
		}

		if(PRIM->ABE && !context->ALPHA.IsOpaque(amin, amax) || PRIM->AA1)
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
		p.sel.zoverflow = GSVector4i(m_vtrace.m_max.p).z == 0x80000000;
	}
}

