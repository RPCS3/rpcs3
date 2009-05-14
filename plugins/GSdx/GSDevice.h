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

#include "GSTexture.h"
#include "GSVertex.h"

#pragma pack(push, 1)

struct MergeConstantBuffer
{
	GSVector4 BGColor;

	struct MergeConstantBuffer() {memset(this, 0, sizeof(*this));}
};

struct InterlaceConstantBuffer
{
	GSVector2 ZrH;
	float hH;
	float _pad[1];

	struct InterlaceConstantBuffer() {memset(this, 0, sizeof(*this));}
};

#pragma pack(pop)

template<class Texture> class GSDevice
{
	list<Texture> m_pool;

protected:
	HWND m_hWnd;
	bool m_vsync;
	Texture m_backbuffer;
	Texture m_merge;
	Texture m_weavebob;
	Texture m_blend;
	Texture m_1x1;
	Texture m_current;

	bool Fetch(int type, Texture& t, int w, int h, int format)
	{
		Recycle(t);

		for(list<Texture>::iterator i = m_pool.begin(); i != m_pool.end(); i++)
		{
			const Texture& t2 = *i;

			if(t2.GetType() == type && t2.GetWidth() == w && t2.GetHeight() == h && t2.GetFormat() == format)
			{
				t = t2;

				m_pool.erase(i);

				return true;
			}
		}

		return Create(type, t, w, h, format);
	}

	virtual bool Create(int type, Texture& t, int w, int h, int format) = 0;
	virtual void DoMerge(Texture* st, GSVector4* sr, GSVector4* dr, Texture& dt, bool slbg, bool mmod, GSVector4& c) = 0;
	virtual void DoInterlace(Texture& st, Texture& dt, int shader, bool linear, float yoffset) = 0;

public:
	GSDevice() : m_hWnd(NULL)
	{
	}

	virtual ~GSDevice() 
	{
	}

	virtual bool Create(HWND hWnd, bool vsync)
	{
		m_hWnd = hWnd;
		m_vsync = vsync;

		return true;
	}
	
	virtual bool Reset(int w, int h, bool fs)
	{
		m_pool.clear();
		m_backbuffer = Texture();
		m_merge = Texture();
		m_weavebob = Texture();
		m_blend = Texture();
		m_1x1 = Texture();
		m_current = Texture();

		return true;
	}

	virtual bool IsLost() = 0;

	virtual void Present(const GSVector4i& r) = 0;

	virtual void BeginScene() = 0;

	virtual void EndScene() = 0;

	virtual void Draw(const string& s) = 0;

	virtual bool CopyOffscreen(Texture& src, const GSVector4& sr, Texture& dst, int w, int h, int format = 0) = 0;

	virtual void ClearRenderTarget(Texture& t, const GSVector4& c) = 0;

	virtual void ClearRenderTarget(Texture& t, uint32 c) = 0;

	virtual void ClearDepth(Texture& t, float c) = 0;

	virtual void ClearStencil(Texture& t, uint8 c) = 0;

	virtual bool CreateRenderTarget(Texture& t, int w, int h, int format = 0)
	{
		return Fetch(GSTexture::RenderTarget, t, w, h, format);
	}

	virtual bool CreateDepthStencil(Texture& t, int w, int h, int format = 0)
	{
		return Fetch(GSTexture::DepthStencil, t, w, h, format);
	}

	virtual bool CreateTexture(Texture& t, int w, int h, int format = 0)
	{
		return Fetch(GSTexture::Texture, t, w, h, format);
	}

	virtual bool CreateOffscreen(Texture& t, int w, int h, int format = 0)
	{
		return Fetch(GSTexture::Offscreen, t, w, h, format);
	}

	void Recycle(Texture& t)
	{
		if(t)
		{
			m_pool.push_front(t);

			while(m_pool.size() > 200)
			{
				m_pool.pop_back();
			}

			t = Texture();
		}
	}

	bool SaveCurrent(const string& fn)
	{
		return m_current.Save(fn);
	}

	void GetCurrent(Texture& t)
	{
		t = m_current;
	}

	void Merge(Texture* st, GSVector4* sr, GSVector4* dr, const GSVector2i& fs, bool slbg, bool mmod, GSVector4& c)
	{
		if(!m_merge || m_merge.GetWidth() != fs.x || m_merge.GetHeight() != fs.y)
		{
			CreateRenderTarget(m_merge, fs.x, fs.y);
		}

		// TODO: m_1x1

		DoMerge(st, sr, dr, m_merge, slbg, mmod, c);

		m_current = m_merge;
	}

	bool Interlace(const GSVector2i& ds, int field, int mode, float yoffset)
	{
		if(!m_weavebob || m_weavebob.GetWidth() != ds.x || m_weavebob.GetHeight() != ds.y)
		{
			CreateRenderTarget(m_weavebob, ds.x, ds.y);
		}

		if(mode == 0 || mode == 2) // weave or blend
		{
			// weave first

			DoInterlace(m_merge, m_weavebob, field, false, 0);

			if(mode == 2)
			{
				// blend

				if(!m_blend || m_blend.GetWidth() != ds.x || m_blend.GetHeight() != ds.y)
				{
					CreateRenderTarget(m_blend, ds.x, ds.y);
				}

				DoInterlace(m_weavebob, m_blend, 2, false, 0);

				m_current = m_blend;
			}
			else
			{
				m_current = m_weavebob;
			}
		}
		else if(mode == 1) // bob
		{
			DoInterlace(m_merge, m_weavebob, 3, true, yoffset * field);

			m_current = m_weavebob;
		}
		else
		{
			m_current = m_merge;
		}

		return true;
	}

	virtual bool IsCurrentRGBA()
	{
		return true;
	}
};
