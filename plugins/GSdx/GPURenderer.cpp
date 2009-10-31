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

#include "StdAfx.h"
#include "GPURenderer.h"
#include "GSdx.h"

map<HWND, GPURenderer*> GPURenderer::m_wnd2gpu;

GPURenderer::GPURenderer(GSDevice* dev)
	: m_dev(dev)
	, m_hWnd(NULL)
	, m_wndproc(NULL)
{
	m_filter = theApp.GetConfig("filter", 0);
	m_dither = theApp.GetConfig("dithering", 1);
	m_aspectratio = theApp.GetConfig("AspectRatio", 1);
	m_vsync = !!theApp.GetConfig("vsync", 0);
	m_scale = m_mem.GetScale();
}

GPURenderer::~GPURenderer()
{
	if(m_wndproc)
	{
		SetWindowLongPtr(m_hWnd, GWLP_WNDPROC, (LONG_PTR)m_wndproc);

		m_wnd2gpu.erase(m_hWnd);
	}
}

bool GPURenderer::Create(HWND hWnd)
{
	// TODO: move subclassing inside GSWnd::Attach

	m_hWnd = hWnd;

	m_wndproc = (WNDPROC)GetWindowLongPtr(hWnd, GWLP_WNDPROC);
	SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)WndProc);

	if(!m_wnd.Attach(m_hWnd))
	{
		return false;
	}

	m_wnd2gpu[hWnd] = this;

	DWORD style = GetWindowLong(hWnd, GWL_STYLE);
	style |= WS_OVERLAPPEDWINDOW;
	SetWindowLong(hWnd, GWL_STYLE, style);

	m_wnd.Show();

	if(!m_dev->Create(&m_wnd))
	{
		return false;
	}

	m_dev->SetVsync(m_vsync);
	Reset();

	return true;
}

bool GPURenderer::Merge()
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

void GPURenderer::VSync()
{
	GSPerfMonAutoTimer pmat(m_perfmon);

	m_perfmon.Put(GSPerfMon::Frame);

	// m_env.STATUS.LCF = ~m_env.STATUS.LCF; // ?

	if(!IsWindow(m_hWnd)) return;

	Flush();

	if(!m_dev->IsLost(true))
	{
		if(!Merge())
		{
			return;
		}
	}
	else
	{
		ResetDevice();
	}

	// osd 

	if((m_perfmon.GetFrame() & 0x1f) == 0)
	{
		m_perfmon.Update();

		double fps = 1000.0f / m_perfmon.Get(GSPerfMon::Frame);

		GSVector4i r = m_env.GetDisplayRect();

		int w = r.width() << m_scale.x;
		int h = r.height() << m_scale.y;

		string s = format(
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
			s = format("%s | %.2f mpps", s.c_str(), fps * fillrate / (1024 * 1024));
		}

		SetWindowText(m_hWnd, s.c_str());
	}

	GSVector4i r;
	
	GetClientRect(m_hWnd, r);

	m_dev->Present(r.fit(m_aspectratio), 0);
}

bool GPURenderer::MakeSnapshot(const string& path)
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

LRESULT CALLBACK GPURenderer::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	map<HWND, GPURenderer*>::iterator i = m_wnd2gpu.find(hWnd);

	if(i != m_wnd2gpu.end())
	{
		return i->second->OnMessage(message, wParam, lParam);
	}

	ASSERT(0);

	return 0;
}

LRESULT GPURenderer::OnMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
	if(message == WM_KEYUP)
	{
		switch(wParam)
		{
		case VK_DELETE:
			m_filter = (m_filter + 1) % 3;
			return 0;
		case VK_END:
			m_dither = m_dither ? 0 : 1;
			return 0;
		case VK_NEXT:
			m_aspectratio = (m_aspectratio + 1) % 3;
			return 0;
		}
	}

	return CallWindowProc(m_wndproc, m_hWnd, message, wParam, lParam);
}

