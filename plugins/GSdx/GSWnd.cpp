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
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA USA.
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

GSWnd::GSWnd()
	: m_window(NULL), m_Xwindow(0), m_XDisplay(NULL), m_ctx_attached(false), m_swapinterval(NULL)
{
}

GSWnd::~GSWnd()
{
	if (m_XDisplay) {
		XCloseDisplay(m_XDisplay);
		m_XDisplay = NULL;
	}
}

static bool ctxError = false;
static int  ctxErrorHandler(Display *dpy, XErrorEvent *ev)
{
	ctxError = true;
	return 0;
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
		// GLX_X_RENDERABLE: If True is specified, then only frame buffer configurations that have associated X
		// visuals (and can be used to render to Windows and/or GLX pixmaps) will be considered. The default value is GLX_DONT_CARE.
		GLX_X_RENDERABLE    , True,
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

	// Install a dummy handler to handle gracefully (aka not segfault) the support of GL version
	int (*oldHandler)(Display*, XErrorEvent*) = XSetErrorHandler(&ctxErrorHandler);
	// Be sure the handler is installed
	XSync( m_XDisplay, false);

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

	// Don't forget to reinstall the older Handler
	XSetErrorHandler(oldHandler);

	// Get latest error
	XSync( m_XDisplay, false);

	if (!m_context || ctxError) {
		fprintf(stderr, "Failed to create the opengl context. Check your drivers support openGL %d.%d. Hint: opensource drivers don't\n", major, minor );
		return false;
	}

	return true;
}

void GSWnd::AttachContext()
{
	if (!IsContextAttached()) {
		//fprintf(stderr, "Attach the context\n");
		glXMakeCurrent(m_XDisplay, m_Xwindow, m_context);
		m_ctx_attached = true;
	}
}

void GSWnd::DetachContext()
{
	if (IsContextAttached()) {
		//fprintf(stderr, "Detach the context\n");
		glXMakeCurrent(m_XDisplay, None, NULL);
		m_ctx_attached = false;
	}
}

void GSWnd::CheckContext()
{
	int glxMajorVersion, glxMinorVersion;
	glXQueryVersion(m_XDisplay, &glxMajorVersion, &glxMinorVersion);
	if (glXIsDirect(m_XDisplay, m_context))
		fprintf(stderr, "glX-Version %d.%d with Direct Rendering\n", glxMajorVersion, glxMinorVersion);
	else
		fprintf(stderr, "glX-Version %d.%d with Indirect Rendering !!! It won't support properly opengl\n", glxMajorVersion, glxMinorVersion);
}

bool GSWnd::Attach(void* handle, bool managed)
{
	m_Xwindow = *(Window*)handle;
	m_managed = managed;

	m_renderer = theApp.GetConfig("renderer", 0) / 3;
	assert(m_renderer != 2);

	m_XDisplay = XOpenDisplay(NULL);

	// Note: 4.2 crash on latest nvidia drivers!
	if (!CreateContext(3, 3)) return false;

	AttachContext();

	CheckContext();

	PFNGLXSWAPINTERVALMESAPROC m_swapinterval = (PFNGLXSWAPINTERVALMESAPROC)glXGetProcAddress((const GLubyte*) "glXSwapIntervalMESA");
	//PFNGLXSWAPINTERVALMESAPROC m_swapinterval = (PFNGLXSWAPINTERVALMESAPROC)glXGetProcAddress((const GLubyte*) "glXSwapInterval");


	return true;
}

void GSWnd::Detach()
{
	// Actually the destructor is not called when there is only a GSclose/GSshutdown
	// The window still need to be closed
	DetachContext();
	if (m_context) glXDestroyContext(m_XDisplay, m_context);

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

	m_managed = true;

	// note this part must be only executed when replaying .gs debug file
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
}

Display* GSWnd::GetDisplay()
{
	// note this part must be only executed when replaying .gs debug file
	return m_XDisplay;
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

	if (!m_XDisplay) m_XDisplay = XOpenDisplay(NULL);
	XGetGeometry(m_XDisplay, m_Xwindow, &winDummy, &xDummy, &yDummy, &w, &h, &borderDummy, &depthDummy);

	return GSVector4i(0, 0, (int)w, (int)h);
}

// Returns FALSE if the window has no title, or if th window title is under the strict
// management of the emulator.

bool GSWnd::SetWindowText(const char* title)
{
	if (!m_managed) return true;

	XTextProperty prop;

	memset(&prop, 0, sizeof(prop));

	char* ptitle = (char*)title;
	if (XStringListToTextProperty(&ptitle, 1, &prop)) {
		XSetWMName(m_XDisplay, m_Xwindow, &prop);
	}

	XFree(prop.value);
	XFlush(m_XDisplay);

	return true;
}

void GSWnd::SetVSync(bool enable)
{
	// m_swapinterval uses an integer as parameter
	// 0 -> disable vsync
	// n -> wait n frame
	if (m_swapinterval) m_swapinterval((int)enable);
}

void GSWnd::Flip()
{
	glXSwapBuffers(m_XDisplay, m_Xwindow);
}

void GSWnd::Show()
{
	XMapRaised(m_XDisplay, m_Xwindow);
	XFlush(m_XDisplay);
}

void GSWnd::Hide()
{
	XUnmapWindow(m_XDisplay, m_Xwindow);
	XFlush(m_XDisplay);
}

void GSWnd::HideFrame()
{
	// TODO
}

#endif
