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
#include "GSRendererSW.h"

const GSVector4 g_pos_scale(1.0f / 16, 1.0f / 16, 1.0f, 128.0f);

GSRendererSW::GSRendererSW(int threads)
{
	InitVertexKick(GSRendererSW);

	m_tc = new GSTextureCacheSW(this);

	memset(m_texture, 0, sizeof(m_texture));

	m_rl.Create<GSDrawScanline>(threads);

	m_output = (uint8*)_aligned_malloc(1024 * 1024 * sizeof(uint32), 32);
}

GSRendererSW::~GSRendererSW()
{
	delete m_tc;

	for(int i = 0; i < countof(m_texture); i++)
	{
		delete m_texture[i];
	}

	_aligned_free(m_output);
}

void GSRendererSW::Reset()
{
	// TODO: GSreset can come from the main thread too => crash
	// m_tc->RemoveAll();

	m_reset = true;

	GSRendererT<GSVertexSW>::Reset();
}

void GSRendererSW::VSync(int field)
{
	GSRendererT<GSVertexSW>::VSync(field);

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

	int w = DISPFB.FBW * 64;
	int h = GetFrameRect(i).bottom;

	// TODO: round up bottom

	if(m_dev->ResizeTexture(&m_texture[i], w, h))
	{
		static int pitch = 1024 * 4;

		GSVector4i r(0, 0, w, h);

		const GSLocalMemory::psm_t& psm = GSLocalMemory::m_psm[DISPFB.PSM];

		(m_mem.*psm.rtx)(m_mem.GetOffset(DISPFB.Block(), DISPFB.FBW, DISPFB.PSM), r.ralign<Align_Outside>(psm.bs), m_output, pitch, m_env.TEXA);

		m_texture[i]->Update(r, m_output, pitch);

		if(s_dump)
		{
			if(s_save && s_n >= s_saven)
			{
				m_texture[i]->Save(format("c:\\temp1\\_%05d_f%lld_fr%d_%05x_%d.bmp", s_n, m_perfmon.GetFrame(), i, (int)DISPFB.Block(), (int)DISPFB.PSM));
			}

			s_n++;
		}
	}

	return m_texture[i];
}

void GSRendererSW::Draw()
{
	if(m_dump)
	{
		m_dump.Object(m_vertices, m_count, m_vt.m_primclass);
	}

	GSScanlineGlobalData gd;

	GetScanlineGlobalData(gd);

	if(!gd.sel.fwrite && !gd.sel.zwrite)
	{
		return;
	}

	if(s_dump)// && m_context->TEX1.MXL > 0 && m_context->TEX1.MMIN >= 2 && m_context->TEX1.MMIN <= 5 && m_vt.m_lod.x > 0)
	{
		uint64 frame = m_perfmon.GetFrame();

		string s;

		if(s_save && s_n >= s_saven && PRIM->TME)
		{
			s = format("c:\\temp1\\_%05d_f%lld_tex_%05x_%d.bmp", s_n, frame, (int)m_context->TEX0.TBP0, (int)m_context->TEX0.PSM);

			m_mem.SaveBMP(s, m_context->TEX0.TBP0, m_context->TEX0.TBW, m_context->TEX0.PSM, 1 << m_context->TEX0.TW, 1 << m_context->TEX0.TH);
		}

		s_n++;

		if(s_save && s_n >= s_saven)
		{
			s = format("c:\\temp1\\_%05d_f%lld_rt0_%05x_%d.bmp", s_n, frame, m_context->FRAME.Block(), m_context->FRAME.PSM);

			m_mem.SaveBMP(s, m_context->FRAME.Block(), m_context->FRAME.FBW, m_context->FRAME.PSM, GetFrameRect().width(), 512);//GetFrameSize(1).cy);
		}

		if(s_savez && s_n >= s_saven)
		{
			s = format("c:\\temp1\\_%05d_f%lld_rz0_%05x_%d.bmp", s_n, frame, m_context->ZBUF.Block(), m_context->ZBUF.PSM);

			m_mem.SaveBMP(s, m_context->ZBUF.Block(), m_context->FRAME.FBW, m_context->ZBUF.PSM, GetFrameRect().width(), 512);
		}

		s_n++;
	}

	GSRasterizerData data;

	data.scissor = GSVector4i(m_context->scissor.in);
	data.scissor.z = min(data.scissor.z, (int)m_context->FRAME.FBW * 64); // TODO: find a game that overflows and check which one is the right behaviour
	data.primclass = m_vt.m_primclass;
	data.vertices = m_vertices;
	data.count = m_count;
	data.frame = m_perfmon.GetFrame();
	data.param = &gd;

	GSVector4i r = GSVector4i(m_vt.m_min.p.xyxy(m_vt.m_max.p)).rintersect(data.scissor);

	m_rl.Draw(&data, r.width(), r.height());

	if(gd.sel.fwrite)
	{
		m_tc->InvalidateVideoMem(m_context->offset.fb, r);
	}

	if(gd.sel.zwrite)
	{
		m_tc->InvalidateVideoMem(m_context->offset.zb, r);
	}

	// By only syncing here we can do the two InvalidateVideoMem calls free if the other threads finish
	// their drawings later than this one (they usually do because they start on an event).

	m_rl.Sync();

	GSRasterizerStats stats;

	m_rl.GetStats(stats);

	m_perfmon.Put(GSPerfMon::Prim, stats.prims);
	m_perfmon.Put(GSPerfMon::Fillrate, stats.pixels);

	if(s_dump)// && m_context->TEX1.MXL > 0 && m_context->TEX1.MMIN >= 2 && m_context->TEX1.MMIN <= 5 && m_vt.m_lod.x > 0)
	{
		uint64 frame = m_perfmon.GetFrame();

		string s;

		if(s_save && s_n >= s_saven)
		{
			s = format("c:\\temp1\\_%05d_f%lld_rt1_%05x_%d.bmp", s_n, frame, m_context->FRAME.Block(), m_context->FRAME.PSM);

			m_mem.SaveBMP(s, m_context->FRAME.Block(), m_context->FRAME.FBW, m_context->FRAME.PSM, GetFrameRect().width(), 512);//GetFrameSize(1).cy);
		}

		if(s_savez && s_n >= s_saven)
		{
			s = format("c:\\temp1\\_%05d_f%lld_rz1_%05x_%d.bmp", s_n, frame, m_context->ZBUF.Block(), m_context->ZBUF.PSM);

			m_mem.SaveBMP(s, m_context->ZBUF.Block(), m_context->FRAME.FBW, m_context->ZBUF.PSM, GetFrameRect().width(), 512);
		}

		s_n++;
	}

	if(0)//stats.ticks > 5000000)
	{
		printf("* [%lld | %012llx] ticks %lld prims %d (%d) pixels %d (%d)\n",
			m_perfmon.GetFrame(), gd.sel.key,
			stats.ticks,
			stats.prims, stats.prims > 0 ? (int)(stats.ticks / stats.prims) : -1,
			stats.pixels, stats.pixels > 0 ? (int)(stats.ticks / stats.pixels) : -1);
	}
}

void GSRendererSW::InvalidateVideoMem(const GIFRegBITBLTBUF& BITBLTBUF, const GSVector4i& r)
{
	m_tc->InvalidateVideoMem(m_mem.GetOffset(BITBLTBUF.DBP, BITBLTBUF.DBW, BITBLTBUF.DPSM), r);
}

void GSRendererSW::GetScanlineGlobalData(GSScanlineGlobalData& gd)
{
	const GSDrawingEnvironment& env = m_env;
	const GSDrawingContext* context = m_context;
	const GS_PRIM_CLASS primclass = m_vt.m_primclass;

	gd.vm = m_mem.m_vm8;
	gd.dimx = env.dimx;

	gd.fbr = context->offset.fb->pixel.row;
	gd.zbr = context->offset.zb->pixel.row;
	gd.fbc = context->offset.fb->pixel.col[0];
	gd.zbc = context->offset.zb->pixel.col[0];
	gd.fzbr = context->offset.fzb->row;
	gd.fzbc = context->offset.fzb->col;

	gd.sel.key = 0;

	gd.sel.fpsm = 3;
	gd.sel.zpsm = 3;
	gd.sel.atst = ATST_ALWAYS;
	gd.sel.tfx = TFX_NONE;
	gd.sel.ababcd = 255;
	gd.sel.sprite = primclass == GS_SPRITE_CLASS ? 1 : 0;

	uint32 fm = context->FRAME.FBMSK;
	uint32 zm = context->ZBUF.ZMSK || context->TEST.ZTE == 0 ? 0xffffffff : 0;

	if(context->TEST.ZTE && context->TEST.ZTST == ZTST_NEVER)
	{
		fm = 0xffffffff;
		zm = 0xffffffff;
	}

	if(PRIM->TME)
	{
		m_mem.m_clut.Read32(context->TEX0, env.TEXA);
	}

	if(context->TEST.ATE)
	{
		if(!TryAlphaTest(fm, zm))
		{
			gd.sel.atst = context->TEST.ATST;
			gd.sel.afail = context->TEST.AFAIL;

			gd.aref = GSVector4i((int)context->TEST.AREF);

			switch(gd.sel.atst)
			{
			case ATST_LESS:
				gd.sel.atst = ATST_LEQUAL;
				gd.aref -= GSVector4i::x00000001();
				break;
			case ATST_GREATER:
				gd.sel.atst = ATST_GEQUAL;
				gd.aref += GSVector4i::x00000001();
				break;
			}
		}
	}

	bool fwrite = fm != 0xffffffff;
	bool ftest = gd.sel.atst != ATST_ALWAYS || context->TEST.DATE && context->FRAME.PSM != PSM_PSMCT24;

	gd.sel.fwrite = fwrite;
	gd.sel.ftest = ftest;

	if(fwrite || ftest)
	{
		gd.sel.fpsm = GSLocalMemory::m_psm[context->FRAME.PSM].fmt;

		if((primclass == GS_LINE_CLASS || primclass == GS_TRIANGLE_CLASS) && m_vt.m_eq.rgba != 0xffff)
		{
			gd.sel.iip = PRIM->IIP;
		}

		if(PRIM->TME)
		{
			gd.sel.tfx = context->TEX0.TFX;
			gd.sel.tcc = context->TEX0.TCC;
			gd.sel.fst = PRIM->FST;
			gd.sel.ltf = m_vt.IsLinear();
			gd.sel.tlu = GSLocalMemory::m_psm[context->TEX0.PSM].pal > 0;
			gd.sel.wms = context->CLAMP.WMS;
			gd.sel.wmt = context->CLAMP.WMT;

			if(gd.sel.tfx == TFX_MODULATE && gd.sel.tcc && m_vt.m_eq.rgba == 0xffff && m_vt.m_min.c.eq(GSVector4i(128)))
			{
				// modulate does not do anything when vertex color is 0x80

				gd.sel.tfx = TFX_DECAL;
			}

			if(gd.sel.fst == 0)
			{
				// skip per pixel division if q is constant

				GSVertexSW* v = m_vertices;

				if(m_vt.m_eq.q)
				{
					gd.sel.fst = 1;

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
					gd.sel.fst = 1;

					for(int i = 0, j = m_count; i < j; i += 2)
					{
						GSVector4 w = v[i + 1].t.zzzz().rcpnr();

						v[i + 0].t *= w;
						v[i + 1].t *= w;
					}
				}
			}

			GIFRegTEX0 MIP_TEX0 = context->TEX0;
			GIFRegCLAMP MIP_CLAMP = context->CLAMP;

			if(m_mipmap && context->TEX1.MXL > 0 && context->TEX1.MMIN >= 2 && context->TEX1.MMIN <= 5 && m_vt.m_lod.x > 0)
			{
				gd.sel.mipmap = 1; // TODO: pass mmin here and store mxl to m_global for clamping the lod
				gd.sel.lcm = context->TEX1.LCM;

				gd.l = GSVector4((float)(-0x10000 << context->TEX1.L));
				gd.k = GSVector4((float)(0x1000 * context->TEX1.K));

				// the rest is fake, should be removed later

				int level = (int)(m_vt.m_lod.x + 0.5f);

				level = std::min<int>(level, context->TEX1.MXL); 
				level = std::min<int>(level, 6);

				if(level > 0)
				{
					// printf("lvl %d\n", level);

					switch(level)
					{
					case 1: 
						MIP_TEX0.TBP0 = context->MIPTBP1.TBP1; 
						MIP_TEX0.TBW = context->MIPTBP1.TBW1; 
						break;
					case 2: 
						MIP_TEX0.TBP0 = context->MIPTBP1.TBP2; 
						MIP_TEX0.TBW = context->MIPTBP1.TBW2; 
						break;
					case 3: 
						MIP_TEX0.TBP0 = context->MIPTBP1.TBP3; 
						MIP_TEX0.TBW = context->MIPTBP1.TBW3; 
						break;
					case 4: 
						MIP_TEX0.TBP0 = context->MIPTBP2.TBP4; 
						MIP_TEX0.TBW = context->MIPTBP2.TBW4; 
						break;
					case 5: 
						MIP_TEX0.TBP0 = context->MIPTBP2.TBP5; 
						MIP_TEX0.TBW = context->MIPTBP2.TBW5; 
						break;
					case 6: 
						MIP_TEX0.TBP0 = context->MIPTBP2.TBP6; 
						MIP_TEX0.TBW = context->MIPTBP2.TBW6; 
						break;
					default:
						__assume(0);
					}

					ASSERT(MIP_TEX0.TBP0 != 0 && MIP_TEX0.TBW != 0);
				
					int tw = (int)MIP_TEX0.TW - level;
					int th = (int)MIP_TEX0.TH - level;

					switch(context->TEX1.MMIN)
					{
					case 2: case 3: // point (min size 1)
						tw = std::max<int>(tw, 0);
						th = std::max<int>(th, 0);
						break; 
					case 4: case 5: // linear (min size 8)
						tw = std::max<int>(tw, 3);
						th = std::max<int>(th, 3);
						break; 
					default:
						__assume(0);
					}

					// scale down the texture coordinates, including vertex trace

					GSVector4 scale = GSVector4(1.0f) / GSVector4(1 << ((int)MIP_TEX0.TW - tw), 1 << ((int)MIP_TEX0.TH - th), 1, 1);

					GSVertexSW* v = m_vertices;

					for(int i = 0, j = m_count; i < j; i++)
					{
						v[i].t *= scale;
					}

					m_vt.m_min.t *= scale;
					m_vt.m_max.t *= scale;

					MIP_TEX0.TW = (uint32)tw;
					MIP_TEX0.TH = (uint32)th;

					// this shift is done even for repeat modes

					MIP_CLAMP.MINU >>= level;
					MIP_CLAMP.MAXU >>= level;
					MIP_CLAMP.MINV >>= level;
					MIP_CLAMP.MAXV >>= level;
/*
					printf("%d%d%d%d%d L %d K %03x %.2f lod %.2f %.2f q %f %f\n", 
						m_context->TEX1.MXL, 
						m_context->TEX1.MMAG,
						m_context->TEX1.MMIN,
						PRIM->FST,
						m_context->TEX1.LCM,
						m_context->TEX1.L,
						m_context->TEX1.K,
						(float)m_context->TEX1.K / 16,
						m_context->TEX1.MXL > 0 ? m_vt.m_lod.x : 0, 
						m_context->TEX1.MXL > 0 ? m_vt.m_lod.y : 0,
						1.0f / m_vt.m_min.t.z,
						1.0f / m_vt.m_max.t.z);
*/
					if(s_dump)
					{
						uint64 frame = m_perfmon.GetFrame();

						string s;

						if(s_save && s_n >= s_saven)
						{
							s = format("c:\\temp1\\_%05d_f%lld_tex_%05x_%d_(%d%d%d%d%d %.2f %.2f).bmp", 
								s_n, frame, (int)MIP_TEX0.TBP0, (int)MIP_TEX0.PSM,
								m_context->TEX1.MXL, 
								m_context->TEX1.MMAG,
								m_vt.m_filter.mmag, 
								m_context->TEX1.MMIN,
								m_vt.m_filter.mmin, 
								m_context->TEX1.MXL > 0 ? m_vt.m_lod.x : 0, 
								m_context->TEX1.MXL > 0 ? m_vt.m_lod.y : 0
								);

							m_mem.SaveBMP(s, MIP_TEX0.TBP0, MIP_TEX0.TBW, MIP_TEX0.PSM, 1 << MIP_TEX0.TW, 1 << MIP_TEX0.TH);
						}
					}
				}
			}

			if(gd.sel.ltf)
			{
				if(gd.sel.fst)
				{
					// if q is constant we can do the half pel shift for bilinear sampling on the vertices

					GSVector4 half(0x8000, 0x8000);

					GSVertexSW* v = m_vertices;

					for(int i = 0, j = m_count; i < j; i++)
					{
						v[i].t -= half;
					}
				}
			}

			GSVector4i r;

			GetTextureMinMax(r, MIP_TEX0, MIP_CLAMP, gd.sel.ltf);

			const GSTextureCacheSW::GSTexture* t = m_tc->Lookup(MIP_TEX0, env.TEXA, r);

			if(!t) {ASSERT(0); return;}

			gd.tex = t->m_buff;
			gd.clut = m_mem.m_clut;

			gd.sel.tw = t->m_tw - 3;

			uint16 tw = (uint16)(1 << MIP_TEX0.TW);
			uint16 th = (uint16)(1 << MIP_TEX0.TH);

			switch(MIP_CLAMP.WMS)
			{
			case CLAMP_REPEAT:
				gd.t.min.u16[0] = tw - 1;
				gd.t.max.u16[0] = 0;
				gd.t.mask.u32[0] = 0xffffffff;
				break;
			case CLAMP_CLAMP:
				gd.t.min.u16[0] = 0;
				gd.t.max.u16[0] = tw - 1;
				gd.t.mask.u32[0] = 0;
				break;
			case CLAMP_REGION_CLAMP:
				gd.t.min.u16[0] = std::min<int>(MIP_CLAMP.MINU, tw - 1);
				gd.t.max.u16[0] = std::min<int>(MIP_CLAMP.MAXU, tw - 1);
				gd.t.mask.u32[0] = 0;
				break;
			case CLAMP_REGION_REPEAT:
				gd.t.min.u16[0] = MIP_CLAMP.MINU;
				gd.t.max.u16[0] = MIP_CLAMP.MAXU;
				gd.t.mask.u32[0] = 0xffffffff;
				break;
			default:
				__assume(0);
			}

			switch(MIP_CLAMP.WMT)
			{
			case CLAMP_REPEAT:
				gd.t.min.u16[4] = th - 1;
				gd.t.max.u16[4] = 0;
				gd.t.mask.u32[2] = 0xffffffff;
				break;
			case CLAMP_CLAMP:
				gd.t.min.u16[4] = 0;
				gd.t.max.u16[4] = th - 1;
				gd.t.mask.u32[2] = 0;
				break;
			case CLAMP_REGION_CLAMP:
				gd.t.min.u16[4] = std::min<int>(MIP_CLAMP.MINV, th - 1);
				gd.t.max.u16[4] = std::min<int>(MIP_CLAMP.MAXV, th - 1); // ffx anima summon scene, when the anchor appears (th = 256, maxv > 256)
				gd.t.mask.u32[2] = 0;
				break;
			case CLAMP_REGION_REPEAT:
				gd.t.min.u16[4] = MIP_CLAMP.MINV;
				gd.t.max.u16[4] = MIP_CLAMP.MAXV;
				gd.t.mask.u32[2] = 0xffffffff;
				break;
			default:
				__assume(0);
			}

			gd.t.min = gd.t.min.xxxxlh();
			gd.t.max = gd.t.max.xxxxlh();
			gd.t.mask = gd.t.mask.xxzz();
			gd.t.invmask = ~gd.t.mask;
		}

		if(PRIM->FGE)
		{
			gd.sel.fge = 1;

			gd.frb = GSVector4i((int)env.FOGCOL.u32[0] & 0x00ff00ff);
			gd.fga = GSVector4i((int)(env.FOGCOL.u32[0] >> 8) & 0x00ff00ff);
		}

		if(context->FRAME.PSM != PSM_PSMCT24)
		{
			gd.sel.date = context->TEST.DATE;
			gd.sel.datm = context->TEST.DATM;
		}

		if(!IsOpaque())
		{
			gd.sel.abe = PRIM->ABE;
			gd.sel.ababcd = context->ALPHA.u32[0];

			if(env.PABE.PABE)
			{
				gd.sel.pabe = 1;
			}

			if(m_aa1 && PRIM->AA1 && (primclass == GS_LINE_CLASS || primclass == GS_TRIANGLE_CLASS))
			{
				gd.sel.aa1 = 1;
			}

			gd.afix = GSVector4i((int)context->ALPHA.FIX << 7).xxzzlh();
		}

		if(gd.sel.date
		|| gd.sel.aba == 1 || gd.sel.abb == 1 || gd.sel.abc == 1 || gd.sel.abd == 1
		|| gd.sel.atst != ATST_ALWAYS && gd.sel.afail == AFAIL_RGB_ONLY
		|| gd.sel.fpsm == 0 && fm != 0 && fm != 0xffffffff
		|| gd.sel.fpsm == 1 && (fm & 0x00ffffff) != 0 && (fm & 0x00ffffff) != 0x00ffffff
		|| gd.sel.fpsm == 2 && (fm & 0x80f8f8f8) != 0 && (fm & 0x80f8f8f8) != 0x80f8f8f8)
		{
			gd.sel.rfb = 1;
		}

		gd.sel.colclamp = env.COLCLAMP.CLAMP;
		gd.sel.fba = context->FBA.FBA;
		gd.sel.dthe = env.DTHE.DTHE;
	}

	bool zwrite = zm != 0xffffffff;
	bool ztest = context->TEST.ZTE && context->TEST.ZTST > ZTST_ALWAYS;

	gd.sel.zwrite = zwrite;
	gd.sel.ztest = ztest;

	if(zwrite || ztest)
	{
		gd.sel.zpsm = GSLocalMemory::m_psm[context->ZBUF.PSM].fmt;
		gd.sel.ztst = ztest ? context->TEST.ZTST : ZTST_ALWAYS;
		gd.sel.zoverflow = GSVector4i(m_vt.m_max.p).z == 0x80000000;
	}

	gd.fm = GSVector4i(fm);
	gd.zm = GSVector4i(zm);

	if(gd.sel.fpsm == 1)
	{
		gd.fm |= GSVector4i::xff000000();
	}
	else if(gd.sel.fpsm == 2)
	{
		GSVector4i rb = gd.fm & 0x00f800f8;
		GSVector4i ga = gd.fm & 0x8000f800;

		gd.fm = (ga >> 16) | (rb >> 9) | (ga >> 6) | (rb >> 3) | GSVector4i::xffff0000();
	}

	if(gd.sel.zpsm == 1)
	{
		gd.zm |= GSVector4i::xff000000();
	}
	else if(gd.sel.zpsm == 2)
	{
		gd.zm |= GSVector4i::xffff0000();
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
		if(fst)
		{
			v.t = GSVector4(((GSVector4i)m_v.UV).upl16() << (16 - 4));
		}
		else
		{
			v.t = GSVector4(m_v.ST.S, m_v.ST.T);
			v.t *= GSVector4(0x10000 << context->TEX0.TW, 0x10000 << context->TEX0.TH);
		}

		v.t = v.t.xyxy(GSVector4::load(m_v.RGBAQ.Q));
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
