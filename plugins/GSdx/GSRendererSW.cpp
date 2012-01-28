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

#define LOG 0

static FILE* s_fp = LOG ? fopen("c:\\temp1\\_.txt", "w") : NULL;

const GSVector4 g_pos_scale(1.0f / 16, 1.0f / 16, 1.0f, 128.0f);

GSRendererSW::GSRendererSW(int threads)
	: m_fzb(NULL)
{
	m_nativeres = true; // ignore ini, sw is always native

	m_tc = new GSTextureCacheSW(this);

	memset(m_texture, 0, sizeof(m_texture));

	m_rl = GSRasterizerList::Create<GSDrawScanline>(threads, &m_perfmon);

	m_output = (uint8*)_aligned_malloc(1024 * 1024 * sizeof(uint32), 32);

	memset(m_fzb_pages, 0, sizeof(m_fzb_pages));
	memset(m_tex_pages, 0, sizeof(m_tex_pages));

	#define InitCVB(P) \
		m_cvb[P][0][0] = &GSRendererSW::ConvertVertexBuffer<P, 0, 0>; \
		m_cvb[P][0][1] = &GSRendererSW::ConvertVertexBuffer<P, 0, 1>; \
		m_cvb[P][1][0] = &GSRendererSW::ConvertVertexBuffer<P, 1, 0>; \
		m_cvb[P][1][1] = &GSRendererSW::ConvertVertexBuffer<P, 1, 1>; \

	InitCVB(GS_POINT_CLASS);
	InitCVB(GS_LINE_CLASS);
	InitCVB(GS_TRIANGLE_CLASS);
	InitCVB(GS_SPRITE_CLASS);
}

GSRendererSW::~GSRendererSW()
{
	delete m_tc;

	for(int i = 0; i < countof(m_texture); i++)
	{
		delete m_texture[i];
	}

	delete m_rl;

	_aligned_free(m_output);
}

void GSRendererSW::Reset()
{
	Sync(-1);

	m_tc->RemoveAll();

	GSRenderer::Reset();
}

void GSRendererSW::VSync(int field)
{
	Sync(0); // IncAge might delete a cached texture in use

	if(0) if(LOG)
	{
		fprintf(s_fp, "%lld\n", m_perfmon.GetFrame());

		GSVector4i dr = GetDisplayRect();
		GSVector4i fr = GetFrameRect();
		GSVector2i ds = GetDeviceSize();

		fprintf(s_fp, "dr %d %d %d %d, fr %d %d %d %d, ds %d %d\n",
			dr.x, dr.y, dr.z, dr.w,
			fr.x, fr.y, fr.z, fr.w,
			ds.x, ds.y);

		for(int i = 0; i < 2; i++)
		{
			if(i == 0 && !m_regs->PMODE.EN1) continue;
			if(i == 1 && !m_regs->PMODE.EN2) continue;

			fprintf(s_fp, "DISPFB[%d] BP=%05x BW=%d PSM=%d DBX=%d DBY=%d\n", 
				i,
				m_regs->DISP[i].DISPFB.Block(),
				m_regs->DISP[i].DISPFB.FBW,
				m_regs->DISP[i].DISPFB.PSM,
				m_regs->DISP[i].DISPFB.DBX,
				m_regs->DISP[i].DISPFB.DBY
				);

			fprintf(s_fp, "DISPLAY[%d] DX=%d DY=%d DW=%d DH=%d MAGH=%d MAGV=%d\n", 
				i,
				m_regs->DISP[i].DISPLAY.DX,
				m_regs->DISP[i].DISPLAY.DY,
				m_regs->DISP[i].DISPLAY.DW,
				m_regs->DISP[i].DISPLAY.DH,
				m_regs->DISP[i].DISPLAY.MAGH,
				m_regs->DISP[i].DISPLAY.MAGV
				);
		}

		fprintf(s_fp, "PMODE EN1=%d EN2=%d CRTMD=%d MMOD=%d AMOD=%d SLBG=%d ALP=%d\n", 
			m_regs->PMODE.EN1,
			m_regs->PMODE.EN2,
			m_regs->PMODE.CRTMD,
			m_regs->PMODE.MMOD,
			m_regs->PMODE.AMOD,
			m_regs->PMODE.SLBG,
			m_regs->PMODE.ALP
			);

		fprintf(s_fp, "SMODE1 CLKSEL=%d CMOD=%d EX=%d GCONT=%d LC=%d NVCK=%d PCK2=%d PEHS=%d PEVS=%d PHS=%d PRST=%d PVS=%d RC=%d SINT=%d SLCK=%d SLCK2=%d SPML=%d T1248=%d VCKSEL=%d VHP=%d XPCK=%d\n",
			m_regs->SMODE1.CLKSEL,
			m_regs->SMODE1.CMOD,
			m_regs->SMODE1.EX,
			m_regs->SMODE1.GCONT,
			m_regs->SMODE1.LC,
			m_regs->SMODE1.NVCK,
			m_regs->SMODE1.PCK2,
			m_regs->SMODE1.PEHS,
			m_regs->SMODE1.PEVS,
			m_regs->SMODE1.PHS,
			m_regs->SMODE1.PRST,
			m_regs->SMODE1.PVS,
			m_regs->SMODE1.RC,
			m_regs->SMODE1.SINT,
			m_regs->SMODE1.SLCK,
			m_regs->SMODE1.SLCK2,
			m_regs->SMODE1.SPML,
			m_regs->SMODE1.T1248,
			m_regs->SMODE1.VCKSEL,
			m_regs->SMODE1.VHP,
			m_regs->SMODE1.XPCK
			);

		fprintf(s_fp, "SMODE2 INT=%d FFMD=%d DPMS=%d\n", 
			m_regs->SMODE2.INT,
			m_regs->SMODE2.FFMD,
			m_regs->SMODE2.DPMS
			);

		fprintf(s_fp, "SRFSH %08x_%08x\n", 
			m_regs->SRFSH.u32[0],
			m_regs->SRFSH.u32[1]
			);

		fprintf(s_fp, "SYNCH1 %08x_%08x\n", 
			m_regs->SYNCH1.u32[0],
			m_regs->SYNCH1.u32[1]
			);

		fprintf(s_fp, "SYNCH2 %08x_%08x\n", 
			m_regs->SYNCH2.u32[0],
			m_regs->SYNCH2.u32[1]
			);

		fprintf(s_fp, "SYNCV %08x_%08x\n", 
			m_regs->SYNCV.u32[0],
			m_regs->SYNCV.u32[1]
			);

		fprintf(s_fp, "CSR %08x_%08x\n", 
			m_regs->CSR.u32[0],
			m_regs->CSR.u32[1]
			);

		fflush(s_fp);
	}

	/*
	int draw[8], sum = 0;

	for(int i = 0; i < countof(draw); i++)
	{
		draw[i] = m_perfmon.CPU(GSPerfMon::WorkerDraw0 + i);
		sum += draw[i];
	}

	printf("CPU %d Sync %d W %d %d %d %d %d %d %d %d (%d)\n",
		m_perfmon.CPU(GSPerfMon::Main),
		m_perfmon.CPU(GSPerfMon::Sync),
		draw[0], draw[1], draw[2], draw[3], draw[4], draw[5], draw[6], draw[7], sum);

	//
	*/

	GSRenderer::VSync(field);

	m_tc->IncAge();

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
	Sync(1);

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

template<uint32 primclass, uint32 tme, uint32 fst>
void GSRendererSW::ConvertVertexBuffer(GSVertexSW* RESTRICT dst, const GSVertex* RESTRICT src, size_t count)
{
	size_t i = m_vertex.next;

	GSVector4i o = (GSVector4i)m_context->XYOFFSET;
	GSVector4 tsize = GSVector4(0x10000 << m_context->TEX0.TW, 0x10000 << m_context->TEX0.TH, 1, 0);

	#if _M_SSE >= 0x501

	// TODO: process vertices in pairs, when AVX2 becomes available

	#endif
	
	for(; i > 0; i--, src++, dst++)
	{
		GSVector4 stcq = GSVector4::load<true>(&src->m[0]); // s t rgba q

		#if _M_SSE >= 0x401

		GSVector4i xyzuvf(src->m[1]);

		GSVector4i xy = xyzuvf.upl16() - o;
		GSVector4i zf = xyzuvf.ywww().min_u32(GSVector4i::xffffff00());

		#else

		uint32 z = src->XYZ.Z;

		GSVector4i xy = GSVector4i::load((int)src->XYZ.u32[0]).upl16() - o;
		GSVector4i zf = GSVector4i((int)std::min<uint32>(z, 0xffffff00), src->FOG); // NOTE: larger values of z may roll over to 0 when converting back to uint32 later

		#endif

		dst->p = GSVector4(xy).xyxy(GSVector4(zf) + (GSVector4::m_x4f800000 & GSVector4::cast(zf.sra32(31)))) * g_pos_scale;
		dst->c = GSVector4(GSVector4i::cast(stcq).zzzz().u8to32() << 7);

		GSVector4 t;

		if(tme)
		{
			if(fst)
			{
				#if _M_SSE >= 0x401

				t = GSVector4(xyzuvf.uph16() << (16 - 4));
					
				#else

				t = GSVector4(GSVector4i::load(src->UV).upl16() << (16 - 4));

				#endif
			}
			else
			{
				t = stcq.xyww() * tsize;
			}
		}

		if(primclass == GS_SPRITE_CLASS)
		{
			#if _M_SSE >= 0x401

			t = t.insert<1, 3>(GSVector4::cast(xyzuvf));
				
			#else
				
			t = t.insert<0, 3>(GSVector4::cast(GSVector4i::load(z)));

			#endif
		}

		dst->t = t;
	}
}

void GSRendererSW::Draw()
{
	const GSDrawingContext* context = m_context;

	SharedData* sd = new SharedData(this);

	shared_ptr<GSRasterizerData> data(sd);

	sd->primclass = m_vt.m_primclass;
	sd->buff = (uint8*)_aligned_malloc(sizeof(GSVertexSW) * m_vertex.next + sizeof(uint32) * m_index.tail, 32);
	sd->vertex = (GSVertexSW*)sd->buff;
	sd->vertex_count = m_vertex.next;
	sd->index = (uint32*)(sd->buff + sizeof(GSVertexSW) * m_vertex.next);
	sd->index_count = m_index.tail;

	(this->*m_cvb[m_vt.m_primclass][PRIM->TME][PRIM->FST])(sd->vertex, m_vertex.buff, m_vertex.next);

	memcpy(sd->index, m_index.buff, sizeof(uint32) * m_index.tail);

	if(!GetScanlineGlobalData(sd)) return;

	GSVector4i scissor = GSVector4i(context->scissor.in);
	GSVector4i bbox = GSVector4i(m_vt.m_min.p.floor().xyxy(m_vt.m_max.p.ceil()));
	GSVector4i r = bbox.rintersect(scissor);

	scissor.z = std::min<int>(scissor.z, (int)context->FRAME.FBW * 64); // TODO: find a game that overflows and check which one is the right behaviour
	
	sd->scissor = scissor;
	sd->bbox = bbox;
	sd->frame = m_perfmon.GetFrame();

	//

	GSScanlineGlobalData& gd = sd->global;

	uint32* fb_pages = NULL;
	uint32* zb_pages = NULL;

	if(sd->global.sel.fb)
	{
		fb_pages = m_context->offset.fb->GetPages(r);
	}

	if(sd->global.sel.zb)
	{
		zb_pages = m_context->offset.zb->GetPages(r);
	}

	// check if there is an overlap between this and previous targets

	if(CheckTargetPages(fb_pages, zb_pages, r))
	{
		sd->m_syncpoint = SharedData::SyncTarget;
	}

	// check if the texture is not part of a target currently in use

	if(CheckSourcePages(sd))
	{
		sd->m_syncpoint = SharedData::SyncSource;
	}

	// addref source and target pages

	sd->UsePages(fb_pages, m_context->offset.fb->psm, zb_pages, m_context->offset.zb->psm);

	//

	if(LOG) {fprintf(s_fp, "queue %05x %d (%d) %05x %d (%d) %05x %d %dx%d | %d %d %d\n",
		m_context->FRAME.Block(), m_context->FRAME.PSM, gd.sel.fwrite, 
		m_context->ZBUF.Block(), m_context->ZBUF.PSM, gd.sel.zwrite,
		PRIM->TME ? m_context->TEX0.TBP0 : 0xfffff, m_context->TEX0.PSM, (int)m_context->TEX0.TW, (int)m_context->TEX0.TH, 
		PRIM->PRIM, sd->vertex_count, sd->index_count); fflush(s_fp);}

	if(s_dump)
	{
		Sync(3);

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

			m_mem.SaveBMP(s, m_context->FRAME.Block(), m_context->FRAME.FBW, m_context->FRAME.PSM, GetFrameRect().width(), 512);
		}

		if(s_savez && s_n >= s_saven)
		{
			s = format("c:\\temp1\\_%05d_f%lld_rz0_%05x_%d.bmp", s_n, frame, m_context->ZBUF.Block(), m_context->ZBUF.PSM);

			m_mem.SaveBMP(s, m_context->ZBUF.Block(), m_context->FRAME.FBW, m_context->ZBUF.PSM, GetFrameRect().width(), 512);
		}

		s_n++;

		Queue(data);

		Sync(4);

		if(s_save && s_n >= s_saven)
		{
			s = format("c:\\temp1\\_%05d_f%lld_rt1_%05x_%d.bmp", s_n, frame, m_context->FRAME.Block(), m_context->FRAME.PSM);

			m_mem.SaveBMP(s, m_context->FRAME.Block(), m_context->FRAME.FBW, m_context->FRAME.PSM, GetFrameRect().width(), 512);
		}

		if(s_savez && s_n >= s_saven)
		{
			s = format("c:\\temp1\\_%05d_f%lld_rz1_%05x_%d.bmp", s_n, frame, m_context->ZBUF.Block(), m_context->ZBUF.PSM);

			m_mem.SaveBMP(s, m_context->ZBUF.Block(), m_context->FRAME.FBW, m_context->ZBUF.PSM, GetFrameRect().width(), 512);
		}

		s_n++;
	}
	else
	{
		Queue(data);
	}

	/*
	if(0)//stats.ticks > 5000000)
	{
		printf("* [%lld | %012llx] ticks %lld prims %d (%d) pixels %d (%d)\n",
			m_perfmon.GetFrame(), gd->sel.key,
			stats.ticks,
			stats.prims, stats.prims > 0 ? (int)(stats.ticks / stats.prims) : -1,
			stats.pixels, stats.pixels > 0 ? (int)(stats.ticks / stats.pixels) : -1);
	}
	*/
}

void GSRendererSW::Queue(shared_ptr<GSRasterizerData>& item)
{
	SharedData* sd = (SharedData*)item.get();

	if(sd->m_syncpoint == SharedData::SyncSource) 
	{
		m_rl->Sync();
	}

	// update previously invalidated parts

	sd->UpdateSource();

	// invalidate new parts rendered onto

	if(sd->global.sel.fwrite)
	{
		m_tc->InvalidatePages(sd->m_fb_pages, sd->m_fpsm);
	}

	if(sd->global.sel.zwrite)
	{
		m_tc->InvalidatePages(sd->m_zb_pages, sd->m_zpsm);
	}

	if(sd->m_syncpoint == SharedData::SyncTarget)
	{
		m_rl->Sync();
	}

	m_rl->Queue(item);
}

void GSRendererSW::Sync(int reason)
{
	//printf("sync %d\n", reason);

	GSPerfMonAutoTimer pmat(&m_perfmon, GSPerfMon::Sync);

	uint64 t = __rdtsc();

	m_rl->Sync();

	if(0) if(LOG)
	{
		s_n++;

		std::string s;
		
		if(s_save)
		{
			s = format("c:\\temp1\\_%05d_f%lld_rt1_%05x_%d.bmp", s_n, m_perfmon.GetFrame(), m_context->FRAME.Block(), m_context->FRAME.PSM);

			m_mem.SaveBMP(s, m_context->FRAME.Block(), m_context->FRAME.FBW, m_context->FRAME.PSM, GetFrameRect().width(), 512);
		}

		if(s_savez)
		{
			s = format("c:\\temp1\\_%05d_f%lld_zb1_%05x_%d.bmp", s_n, m_perfmon.GetFrame(), m_context->ZBUF.Block(), m_context->ZBUF.PSM);

			m_mem.SaveBMP(s, m_context->ZBUF.Block(), m_context->FRAME.FBW, m_context->ZBUF.PSM, GetFrameRect().width(), 512);
		}
	}

	t = __rdtsc() - t;

	int pixels = m_rl->GetPixels();

	if(LOG) {fprintf(s_fp, "sync n=%d r=%d t=%lld p=%d %c\n", s_n, reason, t, pixels, t > 10000000 ? '*' : ' '); fflush(s_fp);}

	m_perfmon.Put(GSPerfMon::Fillrate, pixels);
}

void GSRendererSW::InvalidateVideoMem(const GIFRegBITBLTBUF& BITBLTBUF, const GSVector4i& r)
{
	if(LOG) {fprintf(s_fp, "w %05x %d %d, %d %d %d %d\n", BITBLTBUF.DBP, BITBLTBUF.DBW, BITBLTBUF.DPSM, r.x, r.y, r.z, r.w); fflush(s_fp);}
	
	GSOffset* o = m_mem.GetOffset(BITBLTBUF.DBP, BITBLTBUF.DBW, BITBLTBUF.DPSM);

	o->GetPages(r, m_tmp_pages);

	// check if the changing pages either used as a texture or a target

	if(!m_rl->IsSynced())
	{
		for(uint32* RESTRICT p = m_tmp_pages; *p != GSOffset::EOP; p++)
		{
			if(m_fzb_pages[*p] | m_tex_pages[*p])
			{
				Sync(5);

				break;
			}
		}
	}

	m_tc->InvalidatePages(m_tmp_pages, o->psm); // if texture update runs on a thread and Sync(5) happens then this must come later
}

void GSRendererSW::InvalidateLocalMem(const GIFRegBITBLTBUF& BITBLTBUF, const GSVector4i& r, bool clut)
{
	if(LOG) {fprintf(s_fp, "r %05x %d %d, %d %d %d %d\n", BITBLTBUF.SBP, BITBLTBUF.SBW, BITBLTBUF.SPSM, r.x, r.y, r.z, r.w); fflush(s_fp);}

	if(!m_rl->IsSynced())
	{
		GSOffset* o = m_mem.GetOffset(BITBLTBUF.SBP, BITBLTBUF.SBW, BITBLTBUF.SPSM);

		o->GetPages(r, m_tmp_pages);

		for(uint32* RESTRICT p = m_tmp_pages; *p != GSOffset::EOP; p++)
		{
			if(m_fzb_pages[*p])
			{
				Sync(7);

				break;
			}
		}
	}
}

void GSRendererSW::UsePages(const uint32* pages, int type)
{
	if(type < 2)
	{
		for(const uint32* p = pages; *p != GSOffset::EOP; p++)
		{
			ASSERT(((short*)&m_fzb_pages[*p])[type] < SHRT_MAX);

			_InterlockedIncrement16((short*)&m_fzb_pages[*p] + type);
		}
	}
	else
	{
		for(const uint32* p = pages; *p != GSOffset::EOP; p++)
		{
			ASSERT(m_tex_pages[*p] < SHRT_MAX);

			_InterlockedIncrement16((short*)&m_tex_pages[*p]); // remember which texture pages are used
		}
	}
}

void GSRendererSW::ReleasePages(const uint32* pages, int type)
{
	if(type < 2)
	{
		for(const uint32* p = pages; *p != GSOffset::EOP; p++)
		{
			ASSERT(((short*)&m_fzb_pages[*p])[type] > 0);

			_InterlockedDecrement16((short*)&m_fzb_pages[*p] + type);
		}
	}
	else
	{
		for(const uint32* p = pages; *p != GSOffset::EOP; p++)
		{
			ASSERT(m_tex_pages[*p] > 0);

			_InterlockedDecrement16((short*)&m_tex_pages[*p]);
		}
	}
}

bool GSRendererSW::CheckTargetPages(const uint32* fb_pages, const uint32* zb_pages, const GSVector4i& r)
{
	bool synced = m_rl->IsSynced();
	
	bool fb = fb_pages != NULL;
	bool zb = zb_pages != NULL;

	if(m_fzb != m_context->offset.fzb4)
	{
		// targets changed, check everything

		m_fzb = m_context->offset.fzb4;
		m_fzb_bbox = r;

		if(fb_pages == NULL) fb_pages = m_context->offset.fb->GetPages(r);
		if(zb_pages == NULL) zb_pages = m_context->offset.zb->GetPages(r);

		memset(m_fzb_cur_pages, 0, sizeof(m_fzb_cur_pages));

		uint32 used = 0;

		for(const uint32* p = fb_pages; *p != GSOffset::EOP; p++)
		{
			uint32 i = *p;

			uint32 row = i >> 5;
			uint32 col = 1 << (i & 31);
			
			m_fzb_cur_pages[row] |= col;

			used |= m_fzb_pages[i];
		}

		for(const uint32* p = zb_pages; *p != GSOffset::EOP; p++)
		{
			uint32 i = *p;
			
			uint32 row = i >> 5;
			uint32 col = 1 << (i & 31);
			
			m_fzb_cur_pages[row] |= col;

			used |= m_fzb_pages[i];
		}

		if(!synced)
		{
			if(used)
			{
				if(LOG) {fprintf(s_fp, "syncpoint 0\n"); fflush(s_fp);}

				return true;
			}

			//if(LOG) {fprintf(s_fp, "no syncpoint *\n"); fflush(s_fp);}
		}
	}
	else
	{
		// same target, only check new areas and cross-rendering between frame and z-buffer

		GSVector4i bbox = m_fzb_bbox.runion(r);

		bool check = !m_fzb_bbox.eq(bbox);

		m_fzb_bbox = bbox;

		if(check)
		{
			// drawing area is larger than previous time, check new parts only to avoid false positives (m_fzb_cur_pages guards)

			if(fb_pages == NULL) fb_pages = m_context->offset.fb->GetPages(r);
			if(zb_pages == NULL) zb_pages = m_context->offset.zb->GetPages(r);

			uint32 used = 0;

			for(const uint32* p = fb_pages; *p != GSOffset::EOP; p++)
			{
				uint32 i = *p;

				uint32 row = i >> 5;
				uint32 col = 1 << (i & 31);
			
				if((m_fzb_cur_pages[row] & col) == 0)
				{
					m_fzb_cur_pages[row] |= col;

					used |= m_fzb_pages[i];
				}
			}

			for(const uint32* p = zb_pages; *p != GSOffset::EOP; p++)
			{
				uint32 i = *p;

				uint32 row = i >> 5;
				uint32 col = 1 << (i & 31);
			
				if((m_fzb_cur_pages[row] & col) == 0)
				{
					m_fzb_cur_pages[row] |= col;

					used |= m_fzb_pages[i];
				}
			}

			if(!synced)
			{
				if(used)
				{
					if(LOG) {fprintf(s_fp, "syncpoint 1\n"); fflush(s_fp);}

					return true;
				}
			}
		}

		if(!synced)
		{
			// chross-check frame and z-buffer pages, they cannot overlap with eachother and with previous batches in queue,
			// have to be careful when the two buffers are mutually enabled/disabled and alternating (Bully FBP/ZBP = 0x2300)

			if(fb)
			{
				for(const uint32* p = fb_pages; *p != GSOffset::EOP; p++)
				{
					if(m_fzb_pages[*p] & 0xffff0000)
					{
						if(LOG) {fprintf(s_fp, "syncpoint 2\n"); fflush(s_fp);}

						return true;
					}
				}
			}

			if(zb)
			{
				for(const uint32* p = zb_pages; *p != GSOffset::EOP; p++)
				{
					if(m_fzb_pages[*p] & 0x0000ffff)
					{
						if(LOG) {fprintf(s_fp, "syncpoint 3\n"); fflush(s_fp);}

						return true;
					}
				}
			}
		}
	}

	return false;
}

bool GSRendererSW::CheckSourcePages(SharedData* sd)
{
	if(!m_rl->IsSynced())
	{
		for(size_t i = 0; sd->m_tex[i].t != NULL; i++)
		{
			sd->m_tex[i].t->m_offset->GetPages(sd->m_tex[i].r, m_tmp_pages); 

			uint32* pages = m_tmp_pages; // sd->m_tex[i].t->m_pages.n;

			for(const uint32* p = pages; *p != GSOffset::EOP; p++)
			{
				// TODO: 8H 4HL 4HH texture at the same place as the render target (24 bit, or 32-bit where the alpha channel is masked, Valkyrie Profile 2)

				if(m_fzb_pages[*p]) // currently being drawn to? => sync
				{
					if(LOG) fprintf(s_fp, "r=8 %05x\n", *p << 5);

					return true;
				}
			}
		}
	}

	return false;
}

#include "GSTextureSW.h"

bool GSRendererSW::GetScanlineGlobalData(SharedData* data)
{
	GSScanlineGlobalData& gd = data->global;

	const GSDrawingEnvironment& env = m_env;
	const GSDrawingContext* context = m_context;
	const GS_PRIM_CLASS primclass = m_vt.m_primclass;

	gd.vm = m_mem.m_vm8;

	gd.fbr = context->offset.fb->pixel.row;
	gd.zbr = context->offset.zb->pixel.row;
	gd.fbc = context->offset.fb->pixel.col[0];
	gd.zbc = context->offset.zb->pixel.col[0];
	gd.fzbr = context->offset.fzb4->row;
	gd.fzbc = context->offset.fzb4->col;

	gd.sel.key = 0;

	gd.sel.fpsm = 3;
	gd.sel.zpsm = 3;
	gd.sel.atst = ATST_ALWAYS;
	gd.sel.tfx = TFX_NONE;
	gd.sel.ababcd = 255;
	gd.sel.prim = primclass;

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

	bool zwrite = zm != 0xffffffff;
	bool ztest = context->TEST.ZTE && context->TEST.ZTST > ZTST_ALWAYS;
	/*
	printf("%05x %d %05x %d %05x %d %dx%d\n", 
		fwrite || ftest ? m_context->FRAME.Block() : 0xfffff, m_context->FRAME.PSM,
		zwrite || ztest ? m_context->ZBUF.Block() : 0xfffff, m_context->ZBUF.PSM,
		PRIM->TME ? m_context->TEX0.TBP0 : 0xfffff, m_context->TEX0.PSM, (int)m_context->TEX0.TW, (int)m_context->TEX0.TH);
	*/
	if(!fwrite && !zwrite) return false;

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

			if(GSLocalMemory::m_psm[context->TEX0.PSM].pal > 0)
			{
				gd.sel.tlu = 1;

				gd.clut = (uint32*)_aligned_malloc(sizeof(uint32) * 256, 32); // FIXME: might address uninitialized data of the texture (0xCD) that is not in 0-15 range for 4-bpp formats

				memcpy(gd.clut, (const uint32*)m_mem.m_clut, sizeof(uint32) * GSLocalMemory::m_psm[context->TEX0.PSM].pal);
			}

			gd.sel.wms = context->CLAMP.WMS;
			gd.sel.wmt = context->CLAMP.WMT;

			if(gd.sel.tfx == TFX_MODULATE && gd.sel.tcc && m_vt.m_eq.rgba == 0xffff && m_vt.m_min.c.eq(GSVector4i(128)))
			{
				// modulate does not do anything when vertex color is 0x80

				gd.sel.tfx = TFX_DECAL;
			}

			GSTextureCacheSW::Texture* t = m_tc->Lookup(context->TEX0, env.TEXA);

			if(t == NULL) {ASSERT(0); return false;}

			GSVector4i r;

			GetTextureMinMax(r, context->TEX0, context->CLAMP, gd.sel.ltf);

			data->SetSource(t, r, 0);

			gd.sel.tw = t->m_tw - 3;

			if(m_mipmap && context->TEX1.MXL > 0 && context->TEX1.MMIN >= 2 && context->TEX1.MMIN <= 5 && m_vt.m_lod.y > 0)
			{
				// TEX1.MMIN
				// 000 p
				// 001 l
				// 010 p round
				// 011 p tri
				// 100 l round
				// 101 l tri

				if(m_vt.m_lod.x > 0)
				{
					gd.sel.ltf = context->TEX1.MMIN >> 2;
				}
				else
				{
					// TODO: isbilinear(mmag) != isbilinear(mmin) && m_vt.m_lod.x <= 0 && m_vt.m_lod.y > 0
				}

				gd.sel.mmin = (context->TEX1.MMIN & 1) + 1; // 1: round, 2: tri
				gd.sel.lcm = context->TEX1.LCM;

				int mxl = (std::min<int>((int)context->TEX1.MXL, 6) << 16);
				int k = context->TEX1.K << 12;

				if((int)m_vt.m_lod.x >= (int)context->TEX1.MXL)
				{
					k = (int)m_vt.m_lod.x << 16; // set lod to max level

					gd.sel.lcm = 1; // lod is constant
					gd.sel.mmin = 1; // tri-linear is meaningless
				}

				if(gd.sel.mmin == 2)
				{
					mxl--; // don't sample beyond the last level (TODO: add a dummy level instead?)
				}

				if(gd.sel.fst)
				{
					ASSERT(gd.sel.lcm == 1);
					ASSERT(((m_vt.m_min.t.uph(m_vt.m_max.t) == GSVector4::zero()).mask() & 3) == 3); // ratchet and clank (menu)

					gd.sel.lcm = 1;
				}

				if(gd.sel.lcm)
				{
					int lod = std::max<int>(std::min<int>(k, mxl), 0);

					if(gd.sel.mmin == 1)
					{
						lod = (lod + 0x8000) & 0xffff0000; // rounding
					}

					gd.lod.i = GSVector4i(lod >> 16);
					gd.lod.f = GSVector4i(lod & 0xffff).xxxxl().xxzz();

					// TODO: lot to optimize when lod is constant
				}
				else
				{
					gd.mxl = GSVector4((float)mxl);
					gd.l = GSVector4((float)(-0x10000 << context->TEX1.L));
					gd.k = GSVector4((float)k);
				}

				GIFRegTEX0 MIP_TEX0 = context->TEX0;
				GIFRegCLAMP MIP_CLAMP = context->CLAMP;

				GSVector4 tmin = m_vt.m_min.t;
				GSVector4 tmax = m_vt.m_max.t;

				static int s_counter = 0;

				if(0)
				//if(context->TEX0.TH > context->TEX0.TW)
				//if(s_n >= s_saven && s_n < s_saven + 3)
				//if(context->TEX0.TBP0 >= 0x2b80 && context->TEX0.TBW == 2 && context->TEX0.PSM == PSM_PSMT4)
				t->Save(format("c:/temp1/%08d_%05x_0.bmp", s_counter, context->TEX0.TBP0));

				for(int i = 1, j = std::min<int>((int)context->TEX1.MXL, 6); i <= j; i++)
				{
					switch(i)
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

					if(MIP_TEX0.TW > 0) MIP_TEX0.TW--;
					if(MIP_TEX0.TH > 0) MIP_TEX0.TH--;

					MIP_CLAMP.MINU >>= 1;
					MIP_CLAMP.MINV >>= 1;
					MIP_CLAMP.MAXU >>= 1;
					MIP_CLAMP.MAXV >>= 1;

					m_vt.m_min.t *= 0.5f;
					m_vt.m_max.t *= 0.5f;

					GSTextureCacheSW::Texture* t = m_tc->Lookup(MIP_TEX0, env.TEXA, gd.sel.tw + 3);

					if(t == NULL) {ASSERT(0); return false;}

					GSVector4i r;

					GetTextureMinMax(r, MIP_TEX0, MIP_CLAMP, gd.sel.ltf);

					data->SetSource(t, r, i);
				}

				s_counter++;

				m_vt.m_min.t = tmin;
				m_vt.m_max.t = tmax;
			}
			else
			{
				if(gd.sel.fst == 0)
				{
					// skip per pixel division if q is constant

					GSVertexSW* RESTRICT v = data->vertex;

					if(m_vt.m_eq.q)
					{
						gd.sel.fst = 1;

						const GSVector4& t = v[data->index[0]].t;

						if(t.z != 1.0f)
						{
							GSVector4 w = t.zzzz().rcpnr();

							for(int i = 0, j = data->vertex_count; i < j; i++)
							{
								GSVector4 t = v[i].t;

								v[i].t = (t * w).xyzw(t);
							}
						}
					}
					else if(primclass == GS_SPRITE_CLASS)
					{
						gd.sel.fst = 1;

						for(int i = 0, j = data->vertex_count; i < j; i += 2)
						{
							GSVector4 t0 = v[i + 0].t;
							GSVector4 t1 = v[i + 1].t;

							GSVector4 w = t1.zzzz().rcpnr();

							v[i + 0].t = (t0 * w).xyzw(t0);
							v[i + 1].t = (t1 * w).xyzw(t1);
						}
					}
				}

				if(gd.sel.ltf && gd.sel.fst)
				{
					// if q is constant we can do the half pel shift for bilinear sampling on the vertices

					// TODO: but not when mipmapping is used!!!

					GSVector4 half(0x8000, 0x8000);

					GSVertexSW* RESTRICT v = data->vertex;

					for(int i = 0, j = data->vertex_count; i < j; i++)
					{
						GSVector4 t = v[i].t;

						v[i].t = (t - half).xyzw(t);
					}
				}
			}

			uint16 tw = 1u << context->TEX0.TW;
			uint16 th = 1u << context->TEX0.TH;

			switch(context->CLAMP.WMS)
			{
			case CLAMP_REPEAT:
				gd.t.min.u16[0] = gd.t.minmax.u16[0] = tw - 1;
				gd.t.max.u16[0] = gd.t.minmax.u16[2] = 0;
				gd.t.mask.u32[0] = 0xffffffff;
				break;
			case CLAMP_CLAMP:
				gd.t.min.u16[0] = gd.t.minmax.u16[0] = 0;
				gd.t.max.u16[0] = gd.t.minmax.u16[2] = tw - 1;
				gd.t.mask.u32[0] = 0;
				break;
			case CLAMP_REGION_CLAMP:
				gd.t.min.u16[0] = gd.t.minmax.u16[0] = std::min<uint16>(context->CLAMP.MINU, tw - 1);
				gd.t.max.u16[0] = gd.t.minmax.u16[2] = std::min<uint16>(context->CLAMP.MAXU, tw - 1);
				gd.t.mask.u32[0] = 0;
				break;
			case CLAMP_REGION_REPEAT:
				gd.t.min.u16[0] = gd.t.minmax.u16[0] = context->CLAMP.MINU;
				gd.t.max.u16[0] = gd.t.minmax.u16[2] = context->CLAMP.MAXU;
				gd.t.mask.u32[0] = 0xffffffff;
				break;
			default:
				__assume(0);
			}

			switch(context->CLAMP.WMT)
			{
			case CLAMP_REPEAT:
				gd.t.min.u16[4] = gd.t.minmax.u16[1] = th - 1;
				gd.t.max.u16[4] = gd.t.minmax.u16[3] = 0;
				gd.t.mask.u32[2] = 0xffffffff;
				break;
			case CLAMP_CLAMP:
				gd.t.min.u16[4] = gd.t.minmax.u16[1] = 0;
				gd.t.max.u16[4] = gd.t.minmax.u16[3] = th - 1;
				gd.t.mask.u32[2] = 0;
				break;
			case CLAMP_REGION_CLAMP:
				gd.t.min.u16[4] = gd.t.minmax.u16[1] = std::min<uint16>(context->CLAMP.MINV, th - 1);
				gd.t.max.u16[4] = gd.t.minmax.u16[3] = std::min<uint16>(context->CLAMP.MAXV, th - 1); // ffx anima summon scene, when the anchor appears (th = 256, maxv > 256)
				gd.t.mask.u32[2] = 0;
				break;
			case CLAMP_REGION_REPEAT:
				gd.t.min.u16[4] = gd.t.minmax.u16[1] = context->CLAMP.MINV;
				gd.t.max.u16[4] = gd.t.minmax.u16[3] = context->CLAMP.MAXV;
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

		if(env.DTHE.DTHE)
		{
			gd.sel.dthe = 1;

			gd.dimx = (GSVector4i*)_aligned_malloc(sizeof(env.dimx), 32);

			memcpy(gd.dimx, env.dimx, sizeof(env.dimx));
		}
	}

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

	return true;
}

GSRendererSW::SharedData::SharedData(GSRendererSW* parent)
	: m_parent(parent)
	, m_fb_pages(NULL)
	, m_zb_pages(NULL)
	, m_using_pages(false)
	, m_syncpoint(SyncNone)
{
	m_tex[0].t = NULL;

	global.sel.key = 0;

	global.clut = NULL;
	global.dimx = NULL;
}

GSRendererSW::SharedData::~SharedData()
{
	ReleasePages();

	if(global.clut) _aligned_free(global.clut);
	if(global.dimx) _aligned_free(global.dimx);
}

void GSRendererSW::SharedData::UsePages(const uint32* fb_pages, int fpsm, const uint32* zb_pages, int zpsm)
{
	if(m_using_pages) return;

	if(global.sel.fb)
	{
		m_parent->UsePages(fb_pages, 0);
	}

	if(global.sel.zb)
	{
		m_parent->UsePages(zb_pages, 1);
	}

	for(size_t i = 0; m_tex[i].t != NULL; i++)
	{
		m_parent->UsePages(m_tex[i].t->m_pages.n, 2);
	}

	m_fb_pages = fb_pages;
	m_zb_pages = zb_pages;
	m_fpsm = fpsm;
	m_zpsm = zpsm;

	m_using_pages = true;
}

void GSRendererSW::SharedData::ReleasePages()
{
	if(!m_using_pages) return;

	if(global.sel.fb)
	{
		m_parent->ReleasePages(m_fb_pages, 0);
	}

	if(global.sel.zb)
	{
		m_parent->ReleasePages(m_zb_pages, 1);
	}

	for(size_t i = 0; m_tex[i].t != NULL; i++)
	{
		m_parent->ReleasePages(m_tex[i].t->m_pages.n, 2);
	}

	delete [] m_fb_pages;
	delete [] m_zb_pages;

	m_fb_pages = NULL;
	m_zb_pages = NULL;

	m_using_pages = false;
}

void GSRendererSW::SharedData::SetSource(GSTextureCacheSW::Texture* t, const GSVector4i& r, int level)
{
	ASSERT(m_tex[level].t == NULL);

	m_tex[level].t = t;
	m_tex[level].r = r;

	m_tex[level + 1].t = NULL;
}

void GSRendererSW::SharedData::UpdateSource()
{
	for(size_t i = 0; m_tex[i].t != NULL; i++)
	{
		if(m_tex[i].t->Update(m_tex[i].r))
		{
			global.tex[i] = m_tex[i].t->m_buff;
		}
		else
		{
			printf("GSdx: out-of-memory, texturing temporarily disabled\n");

			global.sel.tfx = TFX_NONE;
		}

		// TODO
		
		if(m_parent->s_dump)
		{
			uint64 frame = m_parent->m_perfmon.GetFrame();

			string s;

			if(m_parent->s_save && m_parent->s_n >= m_parent->s_saven)
			{
				s = format("c:\\temp1\\_%05d_f%lld_tex%d_%05x_%d.bmp", m_parent->s_n, frame, i, (int)m_parent->m_context->TEX0.TBP0, (int)m_parent->m_context->TEX0.PSM);

				m_tex[i].t->Save(s);
			}
		}
	}
}
