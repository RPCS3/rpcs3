/*
 *	Copyright (C) 2007-2012 Gabest
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

#include "stdafx.h"
#include "GSWndDX.h"

#ifdef _WINDOWS
GSWndDX::GSWndDX()
	: m_hWnd(NULL)
	, m_frame(true)
{
}

GSWndDX::~GSWndDX()
{
}

LRESULT CALLBACK GSWndDX::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	GSWndDX* wnd = NULL;

	if(message == WM_NCCREATE)
	{
		wnd = (GSWndDX*)((LPCREATESTRUCT)lParam)->lpCreateParams;

		SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)wnd);

		wnd->m_hWnd = hWnd;
	}
	else
	{
		wnd = (GSWndDX*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	}

	if(wnd == NULL)
	{
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return wnd->OnMessage(message, wParam, lParam);
}

LRESULT GSWndDX::OnMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_CLOSE:
		Hide();
		// DestroyWindow(m_hWnd);
		return 0;
	case WM_DESTROY:
		// This kills the emulator when GS is closed, which *really* isn't desired behavior,
		// especially in STGS mode (worked in MTGS mode since it only quit the thread, but even
		// that wasn't needed).
		//PostQuitMessage(0);
		return 0;
	default:
		break;
	}

	return DefWindowProc((HWND)m_hWnd, message, wParam, lParam);
}

bool GSWndDX::Create(const string& title, int w, int h)
{
	if(m_hWnd) return false;

	m_managed = true;

	WNDCLASS wc;

	memset(&wc, 0, sizeof(wc));

	wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wc.lpfnWndProc = WndProc;
	wc.hInstance = theApp.GetModuleHandle();
	// TODO: wc.hIcon = ;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszClassName = "GSWndDX";

	if(!GetClassInfo(wc.hInstance, wc.lpszClassName, &wc))
	{
		if(!RegisterClass(&wc))
		{
			return false;
		}
	}

	DWORD style = WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_OVERLAPPEDWINDOW | WS_BORDER;

	GSVector4i r;

	GetWindowRect(GetDesktopWindow(), r);

	bool remote = !!GetSystemMetrics(SM_REMOTESESSION);

	if(w <= 0 || h <= 0 || remote)
	{
		w = r.width() / 3;
		h = r.width() / 4;

		if(!remote)
		{
			w *= 2;
			h *= 2;
		}
	}

	r.left = (r.left + r.right - w) / 2;
	r.top = (r.top + r.bottom - h) / 2;
	r.right = r.left + w;
	r.bottom = r.top + h;

	AdjustWindowRect(r, style, FALSE);

	m_hWnd = CreateWindow(wc.lpszClassName, title.c_str(), style, r.left, r.top, r.width(), r.height(), NULL, NULL, wc.hInstance, (LPVOID)this);

	return m_hWnd != NULL;
}

bool GSWndDX::Attach(void* handle, bool managed)
{
	// TODO: subclass

	m_hWnd = (HWND)handle;
	m_managed = managed;

	return true;
}

void GSWndDX::Detach()
{
	if(m_hWnd && m_managed)
	{
		// close the window, since it's under GSdx care.  It's not taking messages anyway, and
		// that means its big, ugly, and in the way.

		DestroyWindow(m_hWnd);
	}

	m_hWnd = NULL;
	m_managed = true;
}

GSVector4i GSWndDX::GetClientRect()
{
	GSVector4i r;

	::GetClientRect(m_hWnd, r);

	return r;
}

// Returns FALSE if the window has no title, or if th window title is under the strict
// management of the emulator.

bool GSWndDX::SetWindowText(const char* title)
{
	if(!m_managed) return false;

	::SetWindowText(m_hWnd, title);

	return m_frame;
}

void GSWndDX::Show()
{
	if(!m_managed) return;

	SetForegroundWindow(m_hWnd);
	ShowWindow(m_hWnd, SW_SHOWNORMAL);
	UpdateWindow(m_hWnd);
}

void GSWndDX::Hide()
{
	if(!m_managed) return;

	ShowWindow(m_hWnd, SW_HIDE);
}

void GSWndDX::HideFrame()
{
	if(!m_managed) return;

	SetWindowLong(m_hWnd, GWL_STYLE, GetWindowLong(m_hWnd, GWL_STYLE) & ~(WS_CAPTION|WS_THICKFRAME));
	SetWindowPos(m_hWnd, NULL, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
	SetMenu(m_hWnd, NULL);

	m_frame = false;
}
#endif
