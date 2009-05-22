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

map<HWND, GPURenderer*> GPURenderer::m_wnd2gpu;

GPURenderer::GPURenderer(const GPURendererSettings& rs)
	: GPUState(rs.m_scale)
	, m_hWnd(NULL)
	, m_wndproc(NULL)
{
	m_filter = rs.m_filter;
	m_dither = rs.m_dither;
	m_aspectratio = rs.m_aspectratio;
	m_vsync = rs.m_vsync;
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
	m_hWnd = hWnd;

	m_wndproc = (WNDPROC)GetWindowLongPtr(hWnd, GWLP_WNDPROC);
	SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)WndProc);

	m_wnd2gpu[hWnd] = this;

	DWORD style = GetWindowLong(hWnd, GWL_STYLE);
	style |= WS_OVERLAPPEDWINDOW;
	SetWindowLong(hWnd, GWL_STYLE, style);
	UpdateWindow(hWnd);

	ShowWindow(hWnd, SW_SHOWNORMAL);

	return true;
}

LRESULT CALLBACK GPURenderer::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	map<HWND, GPURenderer*>::iterator i = m_wnd2gpu.find(hWnd);

	if(i != m_wnd2gpu.end())
	{
		return (*i).second->OnMessage(message, wParam, lParam);
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

	return m_wndproc(m_hWnd, message, wParam, lParam);
}

