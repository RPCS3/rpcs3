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
#ifdef ENABLE_SDL_DEV
	if(m_window != NULL && m_managed)
	{
		SDL_DestroyWindow(m_window);
		m_window = NULL;
	}
#endif
	if (m_XDisplay) {
		XCloseDisplay(m_XDisplay);
		m_XDisplay = NULL;
	}
}

bool GSWnd::CreateContext(int major, int minor)
{
	if ( !m_XDisplay || !m_Xwindow )
	{
		fprintf( stderr, "Wrong X11 display/window\n" );
		exit(1);
	}

	// Get visual information
	static int attrListDbl[] =
	{
		GLX_X_RENDERABLE    , True,
		GLX_DRAWABLE_TYPE   , GLX_WINDOW_BIT,
		GLX_RENDER_TYPE     , GLX_RGBA_BIT,
		GLX_RED_SIZE        , 8,
		GLX_GREEN_SIZE      , 8,
		GLX_BLUE_SIZE       , 8,
		GLX_DEPTH_SIZE      , 24,
		GLX_DOUBLEBUFFER    , True,
		None
	};

	PFNGLXCHOOSEFBCONFIGPROC glXChooseFBConfig = (PFNGLXCHOOSEFBCONFIGPROC) glXGetProcAddress((GLubyte *) "glXChooseFBConfig");
	int fbcount = 0;
	GLXFBConfig *fbc = glXChooseFBConfig(m_XDisplay, DefaultScreen(m_XDisplay), attrListDbl, &fbcount);
	if (!fbc || fbcount < 1) return false;

	PFNGLXCREATECONTEXTATTRIBSARBPROC glXCreateContextAttribsARB = (PFNGLXCREATECONTEXTATTRIBSARBPROC)glXGetProcAddress((const GLubyte*) "glXCreateContextAttribsARB");
	if (!glXCreateContextAttribsARB) return false;

	// Create a context
	int context_attribs[] =
	{
		GLX_CONTEXT_MAJOR_VERSION_ARB, major,
		GLX_CONTEXT_MINOR_VERSION_ARB, minor,
		// FIXME : Request a debug context to ease opengl development
		// Note: don't support deprecated feature (pre openg 3.1)
		//GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_DEBUG_BIT_ARB | GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
		GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_DEBUG_BIT_ARB,
		None
	};

	m_context = glXCreateContextAttribsARB(m_XDisplay, fbc[0], 0, true, context_attribs);
	if (!m_context) return false;

	XSync( m_XDisplay, false);
}

void GSWnd::AttachContext()
{
	glXMakeCurrent(m_XDisplay, m_Xwindow, m_context);
}

void GSWnd::DetachContext()
{
	glXMakeCurrent(m_XDisplay, None, NULL);
}

void GSWnd::CheckContext()
{
	int glxMajorVersion, glxMinorVersion;
	glXQueryVersion(m_XDisplay, &glxMajorVersion, &glxMinorVersion);
	if (glXIsDirect(m_XDisplay, m_context))
		fprintf(stderr, "glX-Version %d.%d with Direct Rendering\n", glxMajorVersion, glxMinorVersion);
	else
		fprintf(stderr, "glX-Version %d.%d with Indirect Rendering !!! It will be slow\n", glxMajorVersion, glxMinorVersion);
}

bool GSWnd::Attach(void* handle, bool managed)
{
	m_Xwindow = *(Window*)handle;
	m_managed = managed;

	m_renderer = theApp.GetConfig("renderer", 0) / 3;
	if (m_renderer != 2) {
		m_XDisplay = XOpenDisplay(NULL);

		// Note: 4.2 crash on latest nvidia drivers!
		if (!CreateContext(3, 3)) return false;

		AttachContext();

		CheckContext();
	}

	return true;
}

void GSWnd::Detach()
{
	// Actually the destructor is not called when there is only a GSclose/GSshutdown
	// The window still need to be closed
	if (m_renderer == 2) {
#ifdef ENABLE_SDL_DEV
		if(m_window != NULL && m_managed)
		{
			SDL_DestroyWindow(m_window);
			m_window = NULL;
		}
#endif
	} else {
		DetachContext();
		if (m_context) glXDestroyContext(m_XDisplay, m_context);
	}
	if (m_XDisplay) {
		XCloseDisplay(m_XDisplay);
		m_XDisplay = NULL;
	}
}

bool GSWnd::Create(const string& title, int w, int h)
{
#ifdef ENABLE_SDL_DEV
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
#else

	// note this part must be only executed when replaying .gs debug file
	m_managed = true;

	m_XDisplay = XOpenDisplay(NULL);

	int attrListDbl[] = { GLX_RGBA, GLX_DOUBLEBUFFER,
		GLX_RED_SIZE, 8,
		GLX_GREEN_SIZE, 8,
		GLX_BLUE_SIZE, 8,
		GLX_DEPTH_SIZE, 24,
		None
	};
	XVisualInfo* vi = glXChooseVisual(m_XDisplay, DefaultScreen(m_XDisplay), attrListDbl);

	/* create a color map */
	XSetWindowAttributes attr;
	attr.colormap = XCreateColormap(m_XDisplay, RootWindow(m_XDisplay, vi->screen),
						   vi->visual, AllocNone);
	attr.border_pixel = 0;
    attr.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask | ButtonPressMask |
        StructureNotifyMask | SubstructureRedirectMask | SubstructureNotifyMask |
        EnterWindowMask | LeaveWindowMask | FocusChangeMask ;

    // Create a window at the last position/size
    m_Xwindow = XCreateWindow(m_XDisplay, RootWindow(m_XDisplay, vi->screen),
            0 , 0 , w, h, 0, vi->depth, InputOutput, vi->visual,
            CWBorderPixel | CWColormap | CWEventMask, &attr);

	XMapWindow (m_XDisplay, m_Xwindow);
	XFree(vi);

	if (!CreateContext(3, 3)) return false;

	AttachContext();

	return (m_Xwindow != 0);
#endif
}

Display* GSWnd::GetDisplay()
{
#ifdef ENABLE_SDL_DEV
	SDL_SysWMinfo wminfo;

	memset(&wminfo, 0, sizeof(wminfo));

	wminfo.version.major = SDL_MAJOR_VERSION;
	wminfo.version.minor = SDL_MINOR_VERSION;

	SDL_GetWindowWMInfo(m_window, &wminfo);

	return wminfo.subsystem == SDL_SYSWM_X11 ? wminfo.info.x11.display : NULL;
#else
	// note this part must be only executed when replaying .gs debug file
	return m_XDisplay;
#endif
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
#ifdef ENABLE_SDL_DEV
	if (m_renderer == 2)
		SDL_SetWindowSize(m_window, w, h);
#endif

	return GSVector4i(0, 0, (int)w, (int)h);
}

// Returns FALSE if the window has no title, or if th window title is under the strict
// management of the emulator.

bool GSWnd::SetWindowText(const char* title)
{
#if ENABLE_SDL_DEV
	if (!m_managed) return true;

	// Do not find anyway to check the current fullscreen status
	// Better than nothing heuristic, check the window position. Fullscreen = (0,0)
	// if(!(m_window->flags & SDL_WINDOW_FULLSCREEN) ) // Do not compile
	//
	// We call SDL_PumpEvents to refresh x and y value.
	// but we not use this function anyway.
	// FIXME: it does not feel a good solution -- Gregory
	// NOte: it might be more thread safe to use a call to XGetGeometry
	int x,y = 0;
	if (m_renderer == 2) {
		SDL_PumpEvents();
		SDL_GetWindowPosition(m_window, &x, &y);
		if ( x && y )
			SDL_SetWindowTitle(m_window, title);
	}

#endif
	return true;
}

void GSWnd::Flip()
{
	if (m_renderer == 2) {
#ifdef ENABLE_SDL_DEV
#if SDL_VERSION_ATLEAST(1,3,0)
	SDL_GL_SwapWindow(m_window);
#else
	SDL_GL_SwapBuffers();
#endif
#endif
	} else {
		glXSwapBuffers(m_XDisplay, m_Xwindow);
	}
}

void GSWnd::Show()
{
	if (m_renderer == 2) {
#ifdef ENABLE_SDL_DEV
		SDL_ShowWindow(m_window);
#endif
	} else {
		XMapRaised(m_XDisplay, m_Xwindow);
		XFlush(m_XDisplay);
	}
}

void GSWnd::Hide()
{
	if (m_renderer == 2) {
#ifdef ENABLE_SDL_DEV
		SDL_HideWindow(m_window);
#endif
	} else {
		XUnmapWindow(m_XDisplay, m_Xwindow);
		XFlush(m_XDisplay);
	}
}

void GSWnd::HideFrame()
{
	// TODO
}

#endif
