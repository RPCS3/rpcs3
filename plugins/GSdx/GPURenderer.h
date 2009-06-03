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

#include "GPUState.h"
#include "GSVertexList.h"
#include "GSDevice.h"

class GPURenderer : public GPUState
{
	bool Merge();

protected:
	GSDevice* m_dev;
	int m_filter;
	int m_dither;
	int m_aspectratio;
	bool m_vsync;
	GSVector2i m_scale;

	virtual void ResetDevice() {}
	virtual GSTexture* GetOutput() = 0;

	HWND m_hWnd;
	WNDPROC m_wndproc;
	static map<HWND, GPURenderer*> m_wnd2gpu;
	GSWnd m_wnd;

	static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	LRESULT OnMessage(UINT message, WPARAM wParam, LPARAM lParam);

public:
	GPURenderer(GSDevice* dev);
	virtual ~GPURenderer();

	virtual bool Create(HWND hWnd);
	virtual void VSync();
	virtual bool MakeSnapshot(const string& path);
};

template<class Vertex> 
class GPURendererT : public GPURenderer
{
protected:
	Vertex* m_vertices;
	int m_count;
	int m_maxcount;
	GSVertexList<Vertex> m_vl;

	void Reset()
	{
		m_count = 0;
		m_vl.RemoveAll();

		__super::Reset();
	}

	void ResetPrim()
	{
		m_vl.RemoveAll();
	}

	void FlushPrim() 
	{
		if(m_count > 0)
		{
			/*
			Dump("db");

			if(m_env.PRIM.TME)
			{
				GSVector4i r;

				r.left = m_env.STATUS.TX << 6;
				r.top = m_env.STATUS.TY << 8;
				r.right = r.left + 256;
				r.bottom = r.top + 256;

				Dump(format("da_%d_%d_%d_%d_%d", m_env.STATUS.TP, r).c_str(), m_env.STATUS.TP, r, false);
			}
			*/

			Draw();

			m_count = 0;

			//Dump("dc", false);
		}
	}

	void GrowVertexBuffer()
	{
		m_maxcount = max(10000, m_maxcount * 3/2);
		m_vertices = (Vertex*)_aligned_realloc(m_vertices, sizeof(Vertex) * m_maxcount, 16);
		m_maxcount -= 100;
	}

	__forceinline Vertex* DrawingKick(int& count)
	{
		count = (int)m_env.PRIM.VTX;

		if(m_vl.GetCount() < count)
		{
			return NULL;
		}

		if(m_count >= m_maxcount)
		{
			GrowVertexBuffer();
		}

		Vertex* v = &m_vertices[m_count];

		switch(m_env.PRIM.TYPE)
		{
		case GPU_POLYGON:
			m_vl.GetAt(0, v[0]);
			m_vl.GetAt(1, v[1]);
			m_vl.GetAt(2, v[2]);
			m_vl.RemoveAll();
			break;
		case GPU_LINE:
			m_vl.GetAt(0, v[0]);
			m_vl.GetAt(1, v[1]);
			m_vl.RemoveAll();
			break;
		case GPU_SPRITE:
			m_vl.GetAt(0, v[0]);
			m_vl.GetAt(1, v[1]);
			m_vl.RemoveAll();
			break;
		default:
			ASSERT(0);
			m_vl.RemoveAll();
			return NULL;
		}

		return v;
	}

	virtual void VertexKick() = 0;

	virtual void Draw() = 0;

public:
	GPURendererT(GSDevice* dev)
		: GPURenderer(dev)
		, m_count(0)
		, m_maxcount(0)
		, m_vertices(NULL)
	{
	}

	virtual ~GPURendererT()
	{
		if(m_vertices) _aligned_free(m_vertices);
	}
};
