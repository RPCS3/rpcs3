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
	CAtlList<Texture> m_pool;

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

		for(POSITION pos = m_pool.GetHeadPosition(); pos; m_pool.GetNext(pos))
		{
			const Texture& t2 = m_pool.GetAt(pos);

			if(t2.GetType() == type && t2.GetWidth() == w && t2.GetHeight() == h && t2.GetFormat() == format)
			{
				t = t2;

				m_pool.RemoveAt(pos);

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
		m_pool.RemoveAll();
		m_backbuffer = Texture();
		m_merge = Texture();
		m_weavebob = Texture();
		m_blend = Texture();
		m_1x1 = Texture();
		m_current = Texture();

		return true;
	}

	virtual bool IsLost() = 0;

	virtual void Present(const CRect& r) = 0;

	virtual void BeginScene() = 0;

	virtual void EndScene() = 0;

	virtual void Draw(LPCTSTR str) = 0;

	virtual bool CopyOffscreen(Texture& src, const GSVector4& sr, Texture& dst, int w, int h, int format = 0) = 0;

	virtual void ClearRenderTarget(Texture& t, const GSVector4& c) = 0;

	virtual void ClearRenderTarget(Texture& t, DWORD c) = 0;

	virtual void ClearDepth(Texture& t, float c) = 0;

	virtual void ClearStencil(Texture& t, BYTE c) = 0;

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
			m_pool.AddHead(t);

			while(m_pool.GetCount() > 200)
			{
				m_pool.RemoveTail();
			}

			t = Texture();
		}
	}

	bool SaveCurrent(LPCTSTR fn)
	{
		return m_current.Save(fn);
	}

	void GetCurrent(Texture& t)
	{
		t = m_current;
	}

	void Merge(Texture* st, GSVector4* sr, GSVector4* dr, CSize fs, bool slbg, bool mmod, GSVector4& c)
	{
		if(!m_merge || m_merge.GetWidth() != fs.cx || m_merge.GetHeight() != fs.cy)
		{
			CreateRenderTarget(m_merge, fs.cx, fs.cy);
		}

		// TODO: m_1x1

		DoMerge(st, sr, dr, m_merge, slbg, mmod, c);

		m_current = m_merge;
	}

	bool Interlace(CSize ds, int field, int mode, float yoffset)
	{
		if(!m_weavebob || m_weavebob.GetWidth() != ds.cx || m_weavebob.GetHeight() != ds.cy)
		{
			CreateRenderTarget(m_weavebob, ds.cx, ds.cy);
		}

		if(mode == 0 || mode == 2) // weave or blend
		{
			// weave first

			DoInterlace(m_merge, m_weavebob, field, false, 0);

			if(mode == 2)
			{
				// blend

				if(!m_blend || m_blend.GetWidth() != ds.cx || m_blend.GetHeight() != ds.cy)
				{
					CreateRenderTarget(m_blend, ds.cx, ds.cy);
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
