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
#include "GSWndWGL.h"

#ifdef _WINDOWS
GSWndWGL::GSWndWGL()
	: m_NativeWindow(NULL), m_NativeDisplay(NULL), m_context(NULL)
{
}

bool GSWndWGL::CreateContext(int major, int minor)
{
	if ( !m_NativeDisplay || !m_NativeWindow )
	{
		fprintf( stderr, "Wrong display/window\n" );
		exit(1);
	}

	// GL2 context are quite easy but we need GL3 which is another painful story...
	if (!(m_context = wglCreateContext(m_NativeDisplay))) {
		fprintf(stderr, "Failed to create a 2.0 context\n");
		return false;
	}

	// FIXME test it
	// Note: albeit every tutorial said that we need an opengl context to use the GL function wglCreateContextAttribsARB
	// On linux it works without the extra temporary context, not sure the limitation still applied
	if (major >= 3) {
		AttachContext();

		// Create a context
		int context_attribs[] =
		{
			WGL_CONTEXT_MAJOR_VERSION_ARB, major,
			WGL_CONTEXT_MINOR_VERSION_ARB, minor,
			// FIXME : Request a debug context to ease opengl development
			// Note: don't support deprecated feature (pre openg 3.1)
			//GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_DEBUG_BIT_ARB | GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
			WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
			0
		};

		PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
		if (!wglCreateContextAttribsARB) {
			fprintf(stderr, "Failed to init wglCreateContextAttribsARB function pointer\n");
			return false;
		}

		HGLRC context30 = wglCreateContextAttribsARB(m_NativeDisplay, NULL, context_attribs);
		if (!context30) {
			fprintf(stderr, "Failed to create a 3.x context\n");
			return false;
		}

		DetachContext();
		wglDeleteContext(m_context);

		m_context = context30;
		fprintf(stderr, "3.x GL context successfully created\n");
	}

	return true;
}

void GSWndWGL::AttachContext()
{
	if (!IsContextAttached()) {
		wglMakeCurrent(m_NativeDisplay, m_context);
		m_ctx_attached = true;
	}
}

void GSWndWGL::DetachContext()
{
	if (IsContextAttached()) {
		wglMakeCurrent(NULL, NULL);
		m_ctx_attached = false;
	}
}

//TODO: DROP ???
void GSWndWGL::CheckContext()
{
#if 0
	int glxMajorVersion, glxMinorVersion;
	glXQueryVersion(m_NativeDisplay, &glxMajorVersion, &glxMinorVersion);
	if (glXIsDirect(m_NativeDisplay, m_context))
		fprintf(stderr, "glX-Version %d.%d with Direct Rendering\n", glxMajorVersion, glxMinorVersion);
	else
		fprintf(stderr, "glX-Version %d.%d with Indirect Rendering !!! It won't support properly opengl\n", glxMajorVersion, glxMinorVersion);
#endif
}

bool GSWndWGL::Attach(void* handle, bool managed)
{
	m_NativeWindow = (HWND)handle;
	m_managed = managed;

	if (!OpenWGLDisplay()) return false;

	if (!CreateContext(3, 3)) return false;

	AttachContext();

	CheckContext();

	// TODO
	//m_swapinterval = (PFNGLXSWAPINTERVALMESAPROC)glXGetProcAddress((const GLubyte*) "glXSwapIntervalMESA");
	//PFNGLXSWAPINTERVALMESAPROC m_swapinterval = (PFNGLXSWAPINTERVALMESAPROC)glXGetProcAddress((const GLubyte*) "glXSwapInterval");

	PopulateGlFunction();

	UpdateWindow(m_NativeWindow);

	return true;
}

void GSWndWGL::Detach()
{
	// Actually the destructor is not called when there is only a GSclose/GSshutdown
	// The window still need to be closed
	DetachContext();

	if (m_context) wglDeleteContext(m_context);
	m_context = NULL;

	CloseWGLDisplay();
}

bool GSWndWGL::OpenWGLDisplay()
{
	GLuint	  PixelFormat;			// Holds The Results After Searching For A Match
	PIXELFORMATDESCRIPTOR pfd =			 // pfd Tells Windows How We Want Things To Be

	{
		sizeof(PIXELFORMATDESCRIPTOR),			  // Size Of This Pixel Format Descriptor
		1,										  // Version Number
		PFD_DRAW_TO_WINDOW |						// Format Must Support Window
		PFD_SUPPORT_OPENGL |						// Format Must Support OpenGL
		PFD_DOUBLEBUFFER,						   // Must Support Double Buffering
		PFD_TYPE_RGBA,							  // Request An RGBA Format
		32,										 // Select Our Color Depth
		0, 0, 0, 0, 0, 0,						   // Color Bits Ignored
		0,										  // 8bit Alpha Buffer
		0,										  // Shift Bit Ignored
		0,										  // No Accumulation Buffer
		0, 0, 0, 0,								 // Accumulation Bits Ignored
		24,										 // 24Bit Z-Buffer (Depth Buffer)
		8,										  // 8bit Stencil Buffer
		0,										  // No Auxiliary Buffer
		PFD_MAIN_PLANE,							 // Main Drawing Layer
		0,										  // Reserved
		0, 0, 0									 // Layer Masks Ignored
	};

	if (!(m_NativeDisplay = GetDC(m_NativeWindow)))
	{
		MessageBox(NULL, "(1) Can't Create A GL Device Context.", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}

	if (!(PixelFormat = ChoosePixelFormat(m_NativeDisplay, &pfd)))
	{
		MessageBox(NULL, "(2) Can't Find A Suitable PixelFormat.", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}

	if (!SetPixelFormat(m_NativeDisplay, PixelFormat, &pfd))
	{
		MessageBox(NULL, "(3) Can't Set The PixelFormat.", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}

	return true;
}

void GSWndWGL::CloseWGLDisplay()
{
	if (m_NativeDisplay && !ReleaseDC(m_NativeWindow, m_NativeDisplay))				 // Are We Able To Release The DC
	{
		MessageBox(NULL, "Release Device Context Failed.", "SHUTDOWN ERROR", MB_OK | MB_ICONINFORMATION);
	}
	m_NativeDisplay = NULL;									 // Set DC To NULL
}

//TODO: GSopen 1 => Drop?
bool GSWndWGL::Create(const string& title, int w, int h)
{
#if 0
	if(m_NativeWindow) return false;

	if(w <= 0 || h <= 0) {
		w = theApp.GetConfig("ModeWidth", 640);
		h = theApp.GetConfig("ModeHeight", 480);
	}

	m_managed = true;

	// note this part must be only executed when replaying .gs debug file
	m_NativeDisplay = XOpenDisplay(NULL);

	int attrListDbl[] = { GLX_RGBA, GLX_DOUBLEBUFFER,
		GLX_RED_SIZE, 8,
		GLX_GREEN_SIZE, 8,
		GLX_BLUE_SIZE, 8,
		GLX_DEPTH_SIZE, 24,
		None
	};
	XVisualInfo* vi = glXChooseVisual(m_NativeDisplay, DefaultScreen(m_NativeDisplay), attrListDbl);

	/* create a color map */
	XSetWindowAttributes attr;
	attr.colormap = XCreateColormap(m_NativeDisplay, RootWindow(m_NativeDisplay, vi->screen),
			vi->visual, AllocNone);
	attr.border_pixel = 0;
	attr.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask | ButtonPressMask |
		StructureNotifyMask | SubstructureRedirectMask | SubstructureNotifyMask |
		EnterWindowMask | LeaveWindowMask | FocusChangeMask ;

	// Create a window at the last position/size
	m_NativeWindow = XCreateWindow(m_NativeDisplay, RootWindow(m_NativeDisplay, vi->screen),
			0 , 0 , w, h, 0, vi->depth, InputOutput, vi->visual,
			CWBorderPixel | CWColormap | CWEventMask, &attr);

	XMapWindow (m_NativeDisplay, m_NativeWindow);
	XFree(vi);

	if (!CreateContext(3, 3)) return false;

	AttachContext();

	return (m_NativeWindow != 0);
#else
	return false;
#endif
}

//Same as DX
GSVector4i GSWndWGL::GetClientRect()
{
	GSVector4i r;

	::GetClientRect(m_NativeWindow, r);

	return r;
}

void* GSWndWGL::GetProcAddress(const char* name, bool opt)
{
	void* ptr = (void*)wglGetProcAddress(name);
	if (ptr == NULL) {
		fprintf(stderr, "Failed to find %s\n", name);
	}
	return ptr;
}

//TODO: check extensions supported or not
//FIXME : extension allocation
void GSWndWGL::SetVSync(bool enable)
{
	// m_swapinterval uses an integer as parameter
	// 0 -> disable vsync
	// n -> wait n frame
	//if (m_swapinterval) m_swapinterval((int)enable);
	// wglSwapIntervalEXT(!enable);

}

void GSWndWGL::Flip()
{
	SwapBuffers(m_NativeDisplay);
}

void GSWndWGL::Show()
{
}

void GSWndWGL::Hide()
{
}

void GSWndWGL::HideFrame()
{
}

// Returns FALSE if the window has no title, or if th window title is under the strict
// management of the emulator.

bool GSWndWGL::SetWindowText(const char* title)
{
	return false;
}


#endif
