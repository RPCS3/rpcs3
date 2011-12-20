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
	public:
		GSRasterizerData2()
		{
			GSScanlineGlobalData* gd = (GSScanlineGlobalData*)_aligned_malloc(sizeof(GSScanlineGlobalData), 32);

			gd->clut = NULL;
			gd->dimx = NULL;

			param = gd;
		}

		virtual ~GSRasterizerData2()
		{
			GSScanlineGlobalData* gd = (GSScanlineGlobalData*)param;

			if(gd->clut) _aligned_free(gd->clut);
			if(gd->dimx) _aligned_free(gd->dimx);

			_aligned_free(gd);
		}
	};

protected:
	IRasterizer* m_rl;
	GSTextureCacheSW* m_tc;
	GSTexture* m_texture[2];
	uint8* m_output;
	bool m_reset;
	GSPixelOffset4* m_fzb;
	uint32 m_fzb_pages[16];
	uint32 m_tex_pages[16];

	void Reset();
	void VSync(int field);
	void ResetDevice();
	GSTexture* GetOutput(int i);

	void Draw();
	void Sync();
	void InvalidateVideoMem(const GIFRegBITBLTBUF& BITBLTBUF, const GSVector4i& r);
	void InvalidateLocalMem(const GIFRegBITBLTBUF& BITBLTBUF, const GSVector4i& r, bool clut = false);

	void InvalidatePages(const GSOffset* o, const GSVector4i& rect);
	void InvalidatePages(const GSTextureCacheSW::Texture* t);
	bool CheckPages(const GSOffset* o, const GSVector4i& rect);

	bool GetScanlineGlobalData(GSScanlineGlobalData& gd);

public:
	GSRendererSW(int threads);
	virtual ~GSRendererSW();

	template<uint32 prim, uint32 tme, uint32 fst>
	void VertexKick(bool skip);
};
