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
protected:
	int m_filter;
	int m_dither;
	int m_aspectratio;
	bool m_vsync;
	GSVector2i m_scale;

	HWND m_hWnd;
	WNDPROC m_wndproc;
	static map<HWND, GPURenderer*> m_wnd2gpu;

	static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	LRESULT OnMessage(UINT message, WPARAM wParam, LPARAM lParam);

public:
	GSDevice* m_dev;

public:
	GPURenderer(GSDevice* dev);
	virtual ~GPURenderer();

	virtual bool Create(HWND hWnd);
	virtual void VSync() = 0;
	virtual bool MakeSnapshot(const string& path) = 0;
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

	void VertexKick()
	{
		if(m_vl.GetCount() < (int)m_env.PRIM.VTX)
		{
			return;
		}

		if(m_count > m_maxcount)
		{
			m_maxcount = max(10000, m_maxcount * 3/2);
			m_vertices = (Vertex*)_aligned_realloc(m_vertices, sizeof(Vertex) * m_maxcount, 16);
			m_maxcount -= 100;
		}

		Vertex* v = &m_vertices[m_count];

		int count = 0;

		switch(m_env.PRIM.TYPE)
		{
		case GPU_POLYGON:
			m_vl.GetAt(0, v[0]);
			m_vl.GetAt(1, v[1]);
			m_vl.GetAt(2, v[2]);
			m_vl.RemoveAll();
			count = 3;
			break;
		case GPU_LINE:
			m_vl.GetAt(0, v[0]);
			m_vl.GetAt(1, v[1]);
			m_vl.RemoveAll();
			count = 2;
			break;
		case GPU_SPRITE:
			m_vl.GetAt(0, v[0]);
			m_vl.GetAt(1, v[1]);
			m_vl.RemoveAll();
			count = 2;
			break;
		default:
			ASSERT(0);
			m_vl.RemoveAll();
			count = 0;
			break;
		}

		(this->*m_fpDrawingKickHandlers[m_env.PRIM.TYPE])(v, count);

		m_count += count;
	}

	typedef void (GPURendererT<Vertex>::*DrawingKickHandler)(Vertex* v, int& count);

	DrawingKickHandler m_fpDrawingKickHandlers[4];

	void DrawingKickNull(Vertex* v, int& count)
	{
		ASSERT(0);
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

	virtual void ResetDevice() {}
	virtual void Draw() = 0;
	virtual GSTexture* GetOutput() = 0;

	bool Merge()
	{
		GSTexture* st[2] = {GetOutput(), NULL};

		if(!st[0])
		{
			return false;
		}

		GSVector2i s = st[0]->GetSize();

		GSVector4 sr[2];
		GSVector4 dr[2];

		sr[0] = GSVector4(0, 0, 1, 1);
		dr[0] = GSVector4(0, 0, s.x, s.y);

		m_dev->Merge(st, sr, dr, s, 1, 1, GSVector4(0, 0, 0, 1));

		return true;
	}

public:
	GPURendererT(GSDevice* dev)
		: GPURenderer(dev)
		, m_count(0)
		, m_maxcount(10000)
	{
		m_vertices = (Vertex*)_aligned_malloc(sizeof(Vertex) * m_maxcount, 16);
		m_maxcount -= 100;

		for(int i = 0; i < countof(m_fpDrawingKickHandlers); i++)
		{
			m_fpDrawingKickHandlers[i] = &GPURendererT<Vertex>::DrawingKickNull;
		}
	}

	virtual ~GPURendererT()
	{
		if(m_vertices) _aligned_free(m_vertices);
	}

	virtual bool Create(HWND hWnd)
	{
		if(!__super::Create(hWnd))
		{
			return false;
		}

		if(!m_dev->Create(hWnd, m_vsync))
		{
			return false;
		}

		Reset();

		return true;
	}

	virtual void VSync()
	{
		GSPerfMonAutoTimer pmat(m_perfmon);

		m_perfmon.Put(GSPerfMon::Frame);

		// m_env.STATUS.LCF = ~m_env.STATUS.LCF; // ?

		if(!IsWindow(m_hWnd)) return;

		Flush();

		if(!Merge()) return;

		// osd 

		static uint64 s_frame = 0;
		static string s_stats;

		if(m_perfmon.GetFrame() - s_frame >= 30)
		{
			m_perfmon.Update();

			s_frame = m_perfmon.GetFrame();

			double fps = 1000.0f / m_perfmon.Get(GSPerfMon::Frame);

			GSVector4i r = m_env.GetDisplayRect();

			int w = r.width() << m_scale.x;
			int h = r.height() << m_scale.y;

			s_stats = format(
				"%I64d | %d x %d | %.2f fps (%d%%) | %d/%d | %d%% CPU | %.2f | %.2f", 
				m_perfmon.GetFrame(), w, h, fps, (int)(100.0 * fps / m_env.GetFPS()),
				(int)m_perfmon.Get(GSPerfMon::Prim),
				(int)m_perfmon.Get(GSPerfMon::Draw),
				m_perfmon.CPU(),
				m_perfmon.Get(GSPerfMon::Swizzle) / 1024,
				m_perfmon.Get(GSPerfMon::Unswizzle) / 1024
			);

			double fillrate = m_perfmon.Get(GSPerfMon::Fillrate);

			if(fillrate > 0)
			{
				s_stats = format("%s | %.2f mpps", s_stats.c_str(), fps * fillrate / (1024 * 1024));
			}

			SetWindowText(m_hWnd, s_stats.c_str());
		}

		if(m_dev->IsLost())
		{
			ResetDevice();
		}

		GSVector4i r;
		
		GetClientRect(m_hWnd, r);

		m_dev->Present(r.fit(m_aspectratio), 0);
	}

	virtual bool MakeSnapshot(const string& path)
	{
		time_t t = time(NULL);

		char buff[16];

		if(!strftime(buff, sizeof(buff), "%Y%m%d%H%M%S", localtime(&t)))
		{
			return false;
		}

		if(GSTexture* t = m_dev->GetCurrent())
		{
			return t->Save(format("%s_%s.bmp", path.c_str(), buff));
		}

		return false;
	}
};
