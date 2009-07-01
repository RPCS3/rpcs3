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
	enum {RenderTarget, DepthStencil};

	class Surface : public GSAlignedClass<16>
	{
	protected:
		GSRenderer* m_renderer;

	public:
		GSTexture* m_texture;
		GIFRegTEX0 m_TEX0;
		GIFRegTEXA m_TEXA;
		int m_age;

	public:
		explicit Surface(GSRenderer* r);
		virtual ~Surface();

		virtual void Update();
	};

	class Target;

	class Source : public Surface
	{
		struct {GSVector4i* rect; uint32 count;} m_write;

		void Write(const GSVector4i& r);
		void Flush(uint32 count);

	public:
		GSTexture* m_palette;
		bool m_initpalette;
		uint32 m_valid[MAX_PAGES]; // each uint32 bits map to the 32 blocks of that page
		uint32* m_clut;
		int m_bpp;
		bool m_target;
		bool m_complete;

	public:
		explicit Source(GSRenderer* renderer);
		virtual ~Source();

		virtual bool Create() = 0;
		virtual bool Create(Target* dst) = 0;
		virtual void Update(const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA, const GSVector4i& rect);
	};

	class Target : public Surface
	{
	public:
		int m_type;
		bool m_used;
		GSDirtyRectList m_dirty;

	public:
		explicit Target(GSRenderer* r);

		virtual bool Create(int w, int h, int type);
		virtual void Update();
		virtual void Read(const GSVector4i& r) = 0;
	};

protected:
	GSRenderer* m_renderer;

	struct SourceMap
	{
		hash_map<Source*, bool> m_surfaces;
		hash_map<Source*, bool> m_map[MAX_PAGES];
		uint32 m_pages[16];
		bool m_used;

		SourceMap() : m_used(false) {memset(m_pages, 0, sizeof(m_pages));}

		void Add(Source* s, const GIFRegTEX0& TEX0);
		void RemoveAll();
		void RemoveAt(Source* s);

	} m_src;

	list<Target*> m_dst[2];

	virtual Source* CreateSource() = 0;
	virtual Target* CreateTarget() = 0;

public:
	GSTextureCache(GSRenderer* r);
	virtual ~GSTextureCache();

	void RemoveAll();

	Source* LookupSource(const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA, const GSVector4i& r);
	Target* LookupTarget(const GIFRegTEX0& TEX0, int w, int h, int type, bool used, bool fb = false);

	void InvalidateVideoMem(const GIFRegBITBLTBUF& BITBLTBUF, const GSVector4i& r, bool target = true);
	void InvalidateLocalMem(const GIFRegBITBLTBUF& BITBLTBUF, const GSVector4i& r);

	void IncAge();
};
