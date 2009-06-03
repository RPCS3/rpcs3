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
#include "GSdx.h"
#include "GSWnd.h"

GSWnd::GSWnd()
	: m_hWnd(NULL)
{
}

GSWnd::~GSWnd()
{
}

LRESULT CALLBACK GSWnd::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	GSWnd* wnd = NULL;

	if(message == WM_NCCREATE)
	{
		wnd = (GSWnd*)((LPCREATESTRUCT)lParam)->lpCreateParams;

		SetWindowLongPtr(hWnd, GWL_USERDATA, (LONG_PTR)wnd);

		wnd->m_hWnd = hWnd;
	}
	else
	{
		wnd = (GSWnd*)GetWindowLongPtr(hWnd, GWL_USERDATA);
	}

	if(wnd == NULL)
	{
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return wnd->OnMessage(message, wParam, lParam);
}

LRESULT GSWnd::OnMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_CLOSE:
		Hide();
		// DestroyWindow(m_hWnd);
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	default:
		break;
	}

	return DefWindowProc(m_hWnd, message, wParam, lParam);
}

bool GSWnd::Create(const string& title)
{
	GSVector4i r;

	GetWindowRect(GetDesktopWindow(), r);

	int w = r.width() / 3;
	int h = r.width() / 4;

	if(!GetSystemMetrics(SM_REMOTESESSION))
	{
		w *= 2;
		h *= 2;
	}

	int x = (r.left + r.right - w) / 2;
	int y = (r.top + r.bottom - h) / 2;

	WNDCLASS wc;

	memset(&wc, 0, sizeof(wc));

	wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wc.lpfnWndProc = WndProc;
	wc.hInstance = theApp.GetModuleHandle();
	// TODO: wc.hIcon = ;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszClassName = "GSWnd";

	if(!RegisterClass(&wc))
	{
		return false;
	}

	DWORD style = WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_OVERLAPPEDWINDOW | WS_BORDER;

	m_hWnd = CreateWindow(wc.lpszClassName, title.c_str(), style, x, y, w, h, NULL, NULL, wc.hInstance, (LPVOID)this);

	if(!m_hWnd)
	{
		return false;
	}

	return true;
}

bool GSWnd::Attach(HWND hWnd)
{
	// TODO: subclass

	m_hWnd = hWnd;

	return true;
}

GSVector4i GSWnd::GetClientRect()
{
	GSVector4i r;

	::GetClientRect(m_hWnd, r);
	
	return r;
}

void GSWnd::SetWindowText(const char* title)
{
	::SetWindowText(m_hWnd, title);
}

void GSWnd::Show()
{
	//SetWindowPos(&wndTop, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
	
	SetForegroundWindow(m_hWnd);
	
	ShowWindow(m_hWnd, SW_SHOWNORMAL);
	
	UpdateWindow(m_hWnd);
}

void GSWnd::Hide()
{
	ShowWindow(m_hWnd, SW_HIDE);
}

void GSWnd::HideFrame()
{
	SetWindowLong(m_hWnd, GWL_STYLE, GetWindowLong(m_hWnd, GWL_STYLE) & ~(WS_CAPTION|WS_THICKFRAME));
	
	SetWindowPos(m_hWnd, NULL, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
	
	SetMenu(m_hWnd, NULL);
}
