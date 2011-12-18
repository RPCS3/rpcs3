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

#include "GSRenderer.h"
#include "GSTextureCacheSW.h"
#include "GSDrawScanline.h"

class GSRendererSW : public GSRendererT<GSVertexSW>
{
	class GSRasterizerData2 : public GSRasterizerData
	{
		GSRenderer* renderer;
		GIFRegFRAME FRAME;
		GIFRegZBUF ZBUF;
		GIFRegTEX0 TEX0;
		uint32 TME;
		GSVector2i framesize;

	public:
		GSRasterizerData2(GSRenderer* r)
		{
			GSScanlineGlobalData* gd = (GSScanlineGlobalData*)_aligned_malloc(sizeof(GSScanlineGlobalData), 32);

			gd->clut = NULL;
			gd->dimx = NULL;

			param = gd;

			renderer = r;
			FRAME = r->m_context->FRAME;
			ZBUF = r->m_context->ZBUF;
			TEX0 = r->m_context->TEX0;
			TME = r->PRIM->TME;
			framesize = GSVector2i(r->GetFrameRect().width(), 512);
		}

		virtual ~GSRasterizerData2()
		{
			GSScanlineGlobalData* gd = (GSScanlineGlobalData*)param;

			if(gd->clut) _aligned_free(gd->clut);
			if(gd->dimx) _aligned_free(gd->dimx);

			_aligned_free(gd);

			DumpOutput();
		}

		// FIXME: not really possible to save whole input/output anymore, strips of the picture may lag in multi-threaded mode

		void DumpInput()
		{
			if(!renderer->s_dump) return; // || !(m_context->TEX1.MXL > 0 && m_context->TEX1.MMIN >= 2 && m_context->TEX1.MMIN <= 5 && m_vt.m_lod.x > 0))

			GSAutoLock l(&renderer->s_lock);

			uint64 frame = renderer->m_perfmon.GetFrame();

			string s;

			if(renderer->s_save && renderer->s_n >= renderer->s_saven && TME)
			{
				s = format("c:\\temp1\\_%05d_f%lld_tex_%05x_%d.bmp", renderer->s_n, frame, (int)TEX0.TBP0, (int)TEX0.PSM);

				renderer->m_mem.SaveBMP(s, TEX0.TBP0, TEX0.TBW, TEX0.PSM, 1 << TEX0.TW, 1 << TEX0.TH);
			}

			renderer->s_n++;

			if(renderer->s_save && renderer->s_n >= renderer->s_saven)
			{
				s = format("c:\\temp1\\_%05d_f%lld_rt0_%05x_%d.bmp", renderer->s_n, frame, FRAME.Block(), FRAME.PSM);

				renderer->m_mem.SaveBMP(s, FRAME.Block(), FRAME.FBW, FRAME.PSM, framesize.x, framesize.y);
			}

			if(renderer->s_savez && renderer->s_n >= renderer->s_saven)
			{
				s = format("c:\\temp1\\_%05d_f%lld_rz0_%05x_%d.bmp", renderer->s_n, frame, ZBUF.Block(), ZBUF.PSM);

				renderer->m_mem.SaveBMP(s, ZBUF.Block(), FRAME.FBW, ZBUF.PSM, framesize.x, framesize.y);
			}

			renderer->s_n++;
		}

		void DumpOutput()
		{
			if(!renderer->s_dump) return; // || !(m_context->TEX1.MXL > 0 && m_context->TEX1.MMIN >= 2 && m_context->TEX1.MMIN <= 5 && m_vt.m_lod.x > 0)

			GSAutoLock l(&renderer->s_lock);

			uint64 frame = renderer->m_perfmon.GetFrame();

			string s;

			if(renderer->s_save && renderer->s_n >= renderer->s_saven)
			{
				s = format("c:\\temp1\\_%05d_f%lld_rt1_%05x_%d.bmp", renderer->s_n, frame, FRAME.Block(), FRAME.PSM);

				renderer->m_mem.SaveBMP(s, FRAME.Block(), FRAME.FBW, FRAME.PSM, framesize.x, framesize.y);
			}

			if(renderer->s_savez && renderer->s_n >= renderer->s_saven)
			{
				s = format("c:\\temp1\\_%05d_f%lld_rz1_%05x_%d.bmp", renderer->s_n, frame, ZBUF.Block(), ZBUF.PSM);

				renderer->m_mem.SaveBMP(s, ZBUF.Block(), FRAME.FBW, ZBUF.PSM, framesize.x, framesize.y);
			}

			renderer->s_n++;
		}
	};	

protected:
	GSRasterizerList* m_rl;
	GSTextureCacheSW* m_tc;
	GSTexture* m_texture[2];
	uint8* m_output;
	bool m_reset;
	GSPixelOffset4* m_fzb;

	void Reset();
	void VSync(int field);
	void ResetDevice();
	GSTexture* GetOutput(int i);

	void Draw();
	void Sync();
	void InvalidateVideoMem(const GIFRegBITBLTBUF& BITBLTBUF, const GSVector4i& r);
	void InvalidateLocalMem(const GIFRegBITBLTBUF& BITBLTBUF, const GSVector4i& r);

	bool GetScanlineGlobalData(GSScanlineGlobalData& gd);

public:
	GSRendererSW(int threads);
	virtual ~GSRendererSW();

	template<uint32 prim, uint32 tme, uint32 fst> 
	void VertexKick(bool skip);
};
