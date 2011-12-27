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
		GSRendererSW* m_parent;
		const list<uint32>* m_fb_pages;
		const list<uint32>* m_zb_pages;
		bool m_using_pages;

	public:
		GSRasterizerData2(GSRendererSW* parent, const list<uint32>* fb_pages, const list<uint32>* zb_pages)
			: m_parent(parent)
			, m_fb_pages(fb_pages)
			, m_zb_pages(zb_pages)
			, m_using_pages(false)
		{
			GSScanlineGlobalData* gd = (GSScanlineGlobalData*)_aligned_malloc(sizeof(GSScanlineGlobalData), 32);

			gd->sel.key = 0;

			gd->clut = NULL;
			gd->dimx = NULL;

			param = gd;
		}

		virtual ~GSRasterizerData2()
		{
			ReleaseTargetPages();

			GSScanlineGlobalData* gd = (GSScanlineGlobalData*)param;

			if(gd->clut) _aligned_free(gd->clut);
			if(gd->dimx) _aligned_free(gd->dimx);

			_aligned_free(gd);

			m_parent->m_perfmon.Put(GSPerfMon::Fillrate, pixels);
		}

		void UseTargetPages()
		{
			if(m_using_pages) {ASSERT(0); return;}

			GSScanlineGlobalData* gd = (GSScanlineGlobalData*)param;
				
			if(gd->sel.fwrite)
			{
				m_parent->UseTargetPages(m_fb_pages, 0);
			}
				
			if(gd->sel.zwrite)
			{
				m_parent->UseTargetPages(m_zb_pages, 1);
			}

			m_using_pages = true;
		}

		void ReleaseTargetPages()
		{
			if(!m_using_pages) {ASSERT(0); return;}

			GSScanlineGlobalData* gd = (GSScanlineGlobalData*)param;
				
			if(gd->sel.fwrite)
			{
				m_parent->ReleaseTargetPages(m_fb_pages, 0);
			}
				
			if(gd->sel.zwrite)
			{
				m_parent->ReleaseTargetPages(m_zb_pages, 1);
			}

			m_using_pages = false;
		}
	};

protected:
	IRasterizer* m_rl;
	GSTextureCacheSW* m_tc;
	GSTexture* m_texture[2];
	uint8* m_output;
	bool m_reset;
	GSPixelOffset4* m_fzb;
	uint32 m_fzb_pages[512]; // uint16 frame/zbuf pages interleaved
	uint32 m_tex_pages[16];

	void Reset();
	void VSync(int field);
	void ResetDevice();
	GSTexture* GetOutput(int i);

	void Draw();
	void Sync(int reason);
	void InvalidateVideoMem(const GIFRegBITBLTBUF& BITBLTBUF, const GSVector4i& r);
	void InvalidateLocalMem(const GIFRegBITBLTBUF& BITBLTBUF, const GSVector4i& r, bool clut = false);

	void UseTargetPages(const list<uint32>* pages, int offset);
	void ReleaseTargetPages(const list<uint32>* pages, int offset);
	void UseSourcePages(const GSTextureCacheSW::Texture* t);

	bool GetScanlineGlobalData(GSScanlineGlobalData& gd);

public:
	GSRendererSW(int threads);
	virtual ~GSRendererSW();

	template<uint32 prim, uint32 tme, uint32 fst>
	void VertexKick(bool skip);
};
