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

#include "stdafx.h"
#include "GSdx.h"
#include "GSWnd.h"

#ifdef _WINDOWS

GSWnd::GSWnd()
	: m_hWnd(NULL)
	, m_managed(true)
	, m_frame(true)
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

		SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)wnd);

		wnd->m_hWnd = hWnd;
	}
	else
	{
		wnd = (GSWnd*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
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

bool GSWnd::Create(const string& title, int w, int h)
{
	if(m_hWnd) return false;

	WNDCLASS wc;

	memset(&wc, 0, sizeof(wc));

	wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wc.lpfnWndProc = WndProc;
	wc.hInstance = theApp.GetModuleHandle();
	// TODO: wc.hIcon = ;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszClassName = "GSWnd";

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

bool GSWnd::Attach(void* handle, bool managed)
{
	// TODO: subclass

	m_hWnd = (HWND)handle;
	m_managed = managed;

	return true;
}

void GSWnd::Detach()
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

GSVector4i GSWnd::GetClientRect()
{
	GSVector4i r;

	::GetClientRect(m_hWnd, r);

	return r;
}

// Returns FALSE if the window has no title, or if th window title is under the strict
// management of the emulator.

bool GSWnd::SetWindowText(const char* title)
{
	if(!m_managed) return false;

	::SetWindowText(m_hWnd, title);

	return m_frame;
}

void GSWnd::Show()
{
	if(!m_managed) return;

	SetForegroundWindow(m_hWnd);
	ShowWindow(m_hWnd, SW_SHOWNORMAL);
	UpdateWindow(m_hWnd);
}

void GSWnd::Hide()
{
	if(!m_managed) return;

	ShowWindow(m_hWnd, SW_HIDE);
}

void GSWnd::HideFrame()
{
	if(!m_managed) return;

	SetWindowLong(m_hWnd, GWL_STYLE, GetWindowLong(m_hWnd, GWL_STYLE) & ~(WS_CAPTION|WS_THICKFRAME));
	SetWindowPos(m_hWnd, NULL, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
	SetMenu(m_hWnd, NULL);

	m_frame = false;
}

#else

/*
GSWnd::GSWnd()
	: m_display(NULL)
	, m_window(0)
	, m_managed(true)
	, m_frame(true)
{
}

GSWnd::~GSWnd()
{
	if(m_display != NULL)
	{
		if(m_window != 0)
		{
			XDestroyWindow(m_display, m_window);
		}

		XCloseDisplay(m_display);
	}
}

bool GSWnd::Create(const string& title, int w, int h)
{
	if(m_display != NULL) return false;

	if(!XInitThreads()) return false;

	m_display = XOpenDisplay(0);

	if(m_display == NULL) return false;

	m_window = XCreateSimpleWindow(m_display, RootWindow(m_display, 0), 0, 0, 640, 480, 0, BlackPixel(m_display, 0), BlackPixel(m_display, 0));

	XFlush(m_display);

	return true;
}

GSVector4i GSWnd::GetClientRect()
{
	int x, y;
	unsigned int w, h;
	unsigned int border, depth;
	Window root;

	XLockDisplay(m_display);
	XGetGeometry(m_display, m_window, &root, &x, &y, &w, &h, &border, &depth);
	XUnlockDisplay(m_display);

	return GSVector4i(0, 0, w, h);
}

// Returns FALSE if the window has no title, or if th window title is under the strict
// management of the emulator.

bool GSWnd::SetWindowText(const char* title)
{
	if(!m_managed) return false;

        XTextProperty p;

        p.value = (unsigned char*)title;
        p.encoding = XA_STRING;
        p.format = 8;
        p.nitems = strlen(title);

        XSetWMName(m_display, m_window, &p);
        XFlush(m_display);

	return m_frame;
}

void GSWnd::Show()
{
	if(!m_managed) return;

	XMapWindow(m_display, m_window);
	XFlush(m_display);
}

void GSWnd::Hide()
{
	if(!m_managed) return;

	XUnmapWindow(m_display, m_window);
	XFlush(m_display);
}

void GSWnd::HideFrame()
{
	if(!m_managed) return;

	// TODO

	m_frame = false;
}
*/

GSWnd::GSWnd()
	: m_window(NULL), m_Xwindow(0), m_XDisplay(NULL)
{
}

GSWnd::~GSWnd()
{
	if(m_window != NULL && m_managed)
	{
		SDL_DestroyWindow(m_window);
		m_window = NULL;
	}
	if (m_XDisplay) {
		XCloseDisplay(m_XDisplay);
		m_XDisplay = NULL;
	}
}

bool GSWnd::Attach(void* handle, bool managed)
{
	m_Xwindow = *(Window*)handle;
	m_managed = managed;

	return true;
}

void GSWnd::Detach()
{
	// Actually the destructor is not called when there is only a GSclose/GSshutdown
	// The window still need to be closed
	if(m_window != NULL && m_managed)
	{
		SDL_DestroyWindow(m_window);
		m_window = NULL;
	}
	if (m_XDisplay) {
		XCloseDisplay(m_XDisplay);
		m_XDisplay = NULL;
	}
}

bool GSWnd::Create(const string& title, int w, int h)
{
	if(m_window != NULL) return false;

	if(w <= 0 || h <= 0) {
		w = theApp.GetConfig("ModeWidth", 640);
		h = theApp.GetConfig("ModeHeight", 480);
	}

#ifdef _LINUX
	// When you reconfigure the plugins during the play, SDL is shutdown so SDL_GetNumVideoDisplays return 0
	// and the plugins is badly closed. NOTE: SDL is initialized in SDL_CreateWindow.
	//
	// I'm not sure this sanity check is still useful, normally (I hope) SDL_CreateWindow will return a null
	// hence a false for this current function.
	// For the moment do an init -- Gregory
	if(!SDL_WasInit(SDL_INIT_VIDEO))
		if(SDL_Init(SDL_INIT_VIDEO) < 0) return false;

    // Sanity check; if there aren't any video displays available, we can't create a window.
    if (SDL_GetNumVideoDisplays() <= 0) return false;
#endif

	m_managed = true;

	m_window = SDL_CreateWindow(title.c_str(), 100, 100, w, h, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

	// Get the X window from the newly created window
	// It would be needed to get the current size
	SDL_SysWMinfo wminfo;
	memset(&wminfo, 0, sizeof(wminfo));

	wminfo.version.major = SDL_MAJOR_VERSION;
	wminfo.version.minor = SDL_MINOR_VERSION;

	SDL_GetWindowWMInfo(m_window, &wminfo);
	m_Xwindow = wminfo.info.x11.window;

	return (m_window != NULL);
}

Display* GSWnd::GetDisplay()
{
	SDL_SysWMinfo wminfo;

	memset(&wminfo, 0, sizeof(wminfo));

	wminfo.version.major = SDL_MAJOR_VERSION;
	wminfo.version.minor = SDL_MINOR_VERSION;

	SDL_GetWindowWMInfo(m_window, &wminfo);

	return wminfo.subsystem == SDL_SYSWM_X11 ? wminfo.info.x11.display : NULL;
}

GSVector4i GSWnd::GetClientRect()
{
	unsigned int h = 480;
	unsigned int w = 640;

	unsigned int borderDummy;
	unsigned int depthDummy;
	Window winDummy;
    int xDummy;
    int yDummy;

	// In gsopen2, pcsx2 stoles all event (including resize event). SDL is not able to update its structure
	// so you must do it yourself
	// In perfect world:
	// if (m_window) SDL_GetWindowSize(m_window, &w, &h);
	// In real world...:
	if (!m_XDisplay) m_XDisplay = XOpenDisplay(NULL);
	XGetGeometry(m_XDisplay, m_Xwindow, &winDummy, &xDummy, &yDummy, &w, &h, &borderDummy, &depthDummy);
	SDL_SetWindowSize(m_window, w, h);

	return GSVector4i(0, 0, (int)w, (int)h);
}

// Returns FALSE if the window has no title, or if th window title is under the strict
// management of the emulator.

bool GSWnd::SetWindowText(const char* title)
{
	if (!m_managed) return true;

	// Do not find anyway to check the current fullscreen status
	// Better than nothing heuristic, check the window position. Fullscreen = (0,0)
	// if(!(m_window->flags & SDL_WINDOW_FULLSCREEN) ) // Do not compile
	//
	// We call SDL_PumpEvents to refresh x and y value.
	// but we not use this function anyway.
	// FIXME: it does not feel a good solution -- Gregory
	// NOte: it might be more thread safe to use a call to XGetGeometry
	SDL_PumpEvents();
	int x,y = 0;
	SDL_GetWindowPosition(m_window, &x, &y);
	if ( x && y )
		SDL_SetWindowTitle(m_window, title);

	return true;
}

void GSWnd::Show()
{
	SDL_ShowWindow(m_window);
}

void GSWnd::Hide()
{
	SDL_HideWindow(m_window);
}

void GSWnd::HideFrame()
{
	// TODO
}

#endif
