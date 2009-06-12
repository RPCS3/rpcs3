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

class GSTextureCache
{
public:
	class GSSurface : public GSAlignedClass<16>
	{
	protected:
		GSRenderer* m_renderer;

	public:
		GSTexture* m_texture;
		GSTexture* m_palette;
		bool m_initpalette;
		int m_age;
		GSDirtyRectList m_dirty;
		GIFRegTEX0 m_TEX0;

		explicit GSSurface(GSRenderer* r);
		virtual ~GSSurface();

		virtual void Update();
	};

	class GSRenderTarget : public GSSurface
	{
	public:
		bool m_used;

		explicit GSRenderTarget(GSRenderer* r);

		void Update();

		virtual bool Create(int w, int h);
		virtual void Read(const GSVector4i& r) = 0;
	};

	class GSDepthStencil : public GSSurface
	{
	public:
		bool m_used;

		explicit GSDepthStencil(GSRenderer* renderer);

		void Update();

		virtual bool Create(int w, int h);
	};

	class GSCachedTexture : public GSSurface
	{
	protected:
		bool GetDirtyRect(GSVector4i& r);

	public:
		uint32* m_clut; // *
		GSVector4i m_valid;
		int m_bpp;
		int m_bpp2;
		bool m_rendered;

		explicit GSCachedTexture(GSRenderer* renderer);
		virtual ~GSCachedTexture();

		void Update();

		virtual bool Create() = 0;
		virtual bool Create(GSRenderTarget* rt) = 0;
		virtual bool Create(GSDepthStencil* ds) = 0;
	};

protected:
	GSRenderer* m_renderer;

	list<GSRenderTarget*> m_rt;
	list<GSDepthStencil*> m_ds;
	list<GSCachedTexture*> m_tex;

	bool m_tex_used;

	template<class T> void RecycleByAge(list<T*>& l, int maxage = 60)
	{
		for(list<T*>::iterator i = l.begin(); i != l.end(); )
		{
			list<T*>::iterator j = i++;

			T* t = *j;

			if(++t->m_age > maxage)
			{
				l.erase(j);

				delete t;
			}
		}
	}

	virtual GSRenderTarget* CreateRenderTarget() = 0;
	virtual GSDepthStencil* CreateDepthStencil() = 0;
	virtual GSCachedTexture* CreateTexture() = 0;

public:
	GSTextureCache(GSRenderer* r);
	virtual ~GSTextureCache();

	void RemoveAll();

	GSRenderTarget* GetRenderTarget(const GIFRegTEX0& TEX0, int w, int h, bool fb = false);
	GSDepthStencil* GetDepthStencil(const GIFRegTEX0& TEX0, int w, int h);
	GSCachedTexture* GetTexture();

	void InvalidateTextures(const GIFRegFRAME& FRAME, const GIFRegZBUF& ZBUF);
	void InvalidateVideoMem(const GIFRegBITBLTBUF& BITBLTBUF, const GSVector4i& r);
	void InvalidateLocalMem(const GIFRegBITBLTBUF& BITBLTBUF, const GSVector4i& r);

	void IncAge();
};
