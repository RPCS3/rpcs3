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
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once

#include "GSRenderer.h"
#include "GSTextureCacheSW.h"
#include "GSDrawScanline.h"

class GSRendererSW : public GSRenderer
{
	class SharedData : public GSDrawScanline::SharedData
	{
		__aligned(struct, 16) TextureLevel 
		{
			GSVector4i r; 
			GSTextureCacheSW::Texture* t;
		};

	public:
		GSRendererSW* m_parent;
		const uint32* m_fb_pages;
		const uint32* m_zb_pages;
		int m_fpsm;
		int m_zpsm;
		bool m_using_pages;
		TextureLevel m_tex[7 + 1]; // NULL terminated
		enum {SyncNone, SyncSource, SyncTarget} m_syncpoint;

	public:
		SharedData(GSRendererSW* parent);
		virtual ~SharedData();

		void UsePages(const uint32* fb_pages, int fpsm, const uint32* zb_pages, int zpsm);
		void ReleasePages();

		void SetSource(GSTextureCacheSW::Texture* t, const GSVector4i& r, int level);
		void UpdateSource();
	};

	typedef void (GSRendererSW::*ConvertVertexBufferPtr)(GSVertexSW* RESTRICT dst, const GSVertex* RESTRICT src, size_t count);

	ConvertVertexBufferPtr m_cvb[4][2][2];

	template<uint32 primclass, uint32 tme, uint32 fst>
	void ConvertVertexBuffer(GSVertexSW* RESTRICT dst, const GSVertex* RESTRICT src, size_t count);

protected:
	IRasterizer* m_rl;
	GSTextureCacheSW* m_tc;
	GSTexture* m_texture[2];
	uint8* m_output;
	GSPixelOffset4* m_fzb;
	GSVector4i m_fzb_bbox;
	uint32 m_fzb_cur_pages[16];
	uint32 m_fzb_pages[512]; // uint16 frame/zbuf pages interleaved
	uint16 m_tex_pages[512];
	uint32 m_tmp_pages[512 + 1];

	void Reset();
	void VSync(int field);
	void ResetDevice();
	GSTexture* GetOutput(int i);

	void Draw();
	void Queue(shared_ptr<GSRasterizerData>& item);
	void Sync(int reason);
	void InvalidateVideoMem(const GIFRegBITBLTBUF& BITBLTBUF, const GSVector4i& r);
	void InvalidateLocalMem(const GIFRegBITBLTBUF& BITBLTBUF, const GSVector4i& r, bool clut = false);

	void UsePages(const uint32* pages, int type);
	void ReleasePages(const uint32* pages, int type);

	bool CheckTargetPages(const uint32* fb_pages, const uint32* zb_pages, const GSVector4i& r);
	bool CheckSourcePages(SharedData* sd);

	bool GetScanlineGlobalData(SharedData* data);

public:
	GSRendererSW(int threads);
	virtual ~GSRendererSW();
};
