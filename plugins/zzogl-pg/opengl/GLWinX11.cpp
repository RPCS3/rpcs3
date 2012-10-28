/*  ZZ Open GL graphics plugin
 *  Copyright (c)2009-2010 zeydlitz@gmail.com, arcum42@gmail.com
 *  Based on Zerofrog's ZeroGS KOSMOS (c)2005-2008
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "Util.h"
#include "GLWin.h"

#if defined(GL_X11_WINDOW)

#ifdef EGL_API
// Need at least MESA 9.0 (plan for october/november 2012)
// So force the destiny to at least check the compilation
#ifndef EGL_KHR_create_context
#define EGL_KHR_create_context 1
#define EGL_CONTEXT_MAJOR_VERSION_KHR			    EGL_CONTEXT_CLIENT_VERSION
#define EGL_CONTEXT_MINOR_VERSION_KHR			    0x30FB
#define EGL_CONTEXT_FLAGS_KHR				    0x30FC
#define EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR		    0x30FD
#define EGL_CONTEXT_OPENGL_RESET_NOTIFICATION_STRATEGY_KHR  0x31BD
#define EGL_NO_RESET_NOTIFICATION_KHR			    0x31BE
#define EGL_LOSE_CONTEXT_ON_RESET_KHR			    0x31BF
#define EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR		    0x00000001
#define EGL_CONTEXT_OPENGL_FORWARD_COMPATIBLE_BIT_KHR	    0x00000002
#define EGL_CONTEXT_OPENGL_ROBUST_ACCESS_BIT_KHR	    0x00000004
#define EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR		    0x00000001
#define EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT_KHR    0x00000002
#endif
#endif

#ifdef USE_GSOPEN2
bool GLWindow::CreateWindow(void *pDisplay)
{
	bool ret = true;

	NativeWindow = (Window)*((u32*)(pDisplay)+1);
	// Do not take the display which come from pcsx2 neither change it.
	// You need a new one to do the operation in the GS thread
	NativeDisplay = XOpenDisplay(NULL);
	if (!NativeDisplay) ret = false;

#ifdef EGL_API
	if (!OpenEGLDisplay()) ret = false;
#endif

	return ret;
}
#else
bool GLWindow::CreateWindow(void *pDisplay)
{
	bool ret = true;

    // init support of multi thread
    if (!XInitThreads())
        ZZLog::Error_Log("Failed to init the xlib concurent threads");

	NativeDisplay = XOpenDisplay(NULL);
	if (!NativeDisplay) ret = false;

	if (pDisplay == NULL)
	{
		ZZLog::Error_Log("Failed to create window. Exiting...");
		return false;
	}

	// Allow pad to use the display
	*(Display**)pDisplay = NativeDisplay;
	// Pad can use the window to grab the input. For the moment just set to 0 to avoid
	// to grab an unknow window... Anyway GSopen1 might be dropped in the future
	*((u32*)(pDisplay)+1) = 0;

#ifdef EGL_API
	if (!OpenEGLDisplay()) ret = false;
#endif

	return ret;
}
#endif

#ifdef EGL_API
EGLBoolean GLWindow::OpenEGLDisplay()
{
	// Create an EGL display from the native display
	eglDisplay = eglGetDisplay((EGLNativeDisplayType)NativeDisplay);
	if ( eglDisplay == EGL_NO_DISPLAY ) return EGL_FALSE;

	if ( !eglInitialize(eglDisplay, NULL, NULL) ) return EGL_FALSE;

	return EGL_TRUE;
}
#endif

#ifdef EGL_API
void GLWindow::CloseEGLDisplay()
{
	eglTerminate(eglDisplay);
}
#endif

bool GLWindow::ReleaseContext()
{
    bool status = true;
#ifdef GLX_API
    if (!NativeDisplay) return status;

    // free the context
	if (glxContext)
	{
		if (!glXMakeCurrent(NativeDisplay, None, NULL)) {
			ZZLog::Error_Log("GLX: Could not release drawing context.");
            status = false;
        }

		glXDestroyContext(NativeDisplay, glxContext);
		glxContext = NULL;
	}
#endif
#ifdef EGL_API
	eglReleaseThread();
#endif

	return status;
}

void GLWindow::CloseWindow()
{
	SaveConfig();
	if (!NativeDisplay) return;

#ifdef EGL_API
	CloseEGLDisplay();
#endif

    XCloseDisplay(NativeDisplay);
    NativeDisplay = NULL;
}

void GLWindow::GetWindowSize()
{
    if (!NativeDisplay or !NativeWindow) return;

	u32 depth = 0;
#ifdef GLX_API
	unsigned int borderDummy;
	Window winDummy;
    s32 xDummy;
    s32 yDummy;
	u32 width;
	u32 height;

    XLockDisplay(NativeDisplay);
	XGetGeometry(NativeDisplay, NativeWindow, &winDummy, &xDummy, &yDummy, &width, &height, &borderDummy, &depth);
    XUnlockDisplay(NativeDisplay);
#endif

	// FIXME: Not sure it works but that could remove latest X11 bits.
#ifdef EGL_API
	int width;
	int height;
	eglQuerySurface(eglDisplay, eglSurface, EGL_WIDTH, &width);
	eglQuerySurface(eglDisplay, eglSurface, EGL_HEIGHT, &height);
#endif

    // update the gl buffer size
    UpdateWindowSize(width, height);

#ifndef USE_GSOPEN2
	// too verbose!
    ZZLog::Dev_Log("Resolution %dx%d. Depth %d bpp. Position (%d,%d)", width, height, depth, conf.x, conf.y);
#endif
}

void GLWindow::PrintProtocolVersion()
{
#ifdef GLX_API
	int glxMajorVersion, glxMinorVersion;

	glXQueryVersion(NativeDisplay, &glxMajorVersion, &glxMinorVersion);

	if (glXIsDirect(NativeDisplay, glxContext))
		ZZLog::Error_Log("glX-Version %d.%d with Direct Rendering", glxMajorVersion, glxMinorVersion);
	else
		ZZLog::Error_Log("glX-Version %d.%d with Indirect Rendering !!! It will be slow", glxMajorVersion, glxMinorVersion);
#endif
#ifdef EGL_API
	ZZLog::Error_Log("EGL: %s : %s", eglQueryString(eglDisplay, EGL_VENDOR) , eglQueryString(eglDisplay, EGL_VERSION) );
	ZZLog::Error_Log("EGL: extensions supported: %s", eglQueryString(eglDisplay, EGL_EXTENSIONS));
#endif
}

#ifdef GLX_API
bool GLWindow::CreateContextGL(int major, int minor)
{
	if (!NativeDisplay) return false;

	// Get visual information
	int attrListDbl[] =
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

	// Attribute are very sensible to the various implementation (intel, nvidia, amd)
	// Nvidia and Intel doesn't support previous attributes for opengl2.0
	int attrListDbl_2_0[] =
	{
		GLX_RGBA,
		GLX_DOUBLEBUFFER,
		GLX_RED_SIZE, 8,
		GLX_GREEN_SIZE, 8,
		GLX_BLUE_SIZE, 8,
		GLX_DEPTH_SIZE, 24,
		None
	};

	// Only keep for older card but NVIDIA and AMD both drop the support of those cards
	if (major <= 2) {
		XVisualInfo *vi = glXChooseVisual(NativeDisplay, DefaultScreen(NativeDisplay), attrListDbl_2_0);
		if (vi == NULL) return NULL;

		glxContext = glXCreateContext(NativeDisplay, vi, NULL, GL_TRUE);
        XFree(vi);

		if (!glxContext) return false;

		glXMakeCurrent(NativeDisplay, NativeWindow, glxContext);
		return true;
	}

	PFNGLXCHOOSEFBCONFIGPROC glXChooseFBConfig = (PFNGLXCHOOSEFBCONFIGPROC)GetProcAddress("glXChooseFBConfig");
	int fbcount = 0;
	GLXFBConfig *fbc = glXChooseFBConfig(NativeDisplay, DefaultScreen(NativeDisplay), attrListDbl, &fbcount);
	if (!fbc || fbcount < 1) {
		ZZLog::Error_Log("GLX: failed to find a framebuffer");
		return false;
	}

	PFNGLXCREATECONTEXTATTRIBSARBPROC glXCreateContextAttribsARB = (PFNGLXCREATECONTEXTATTRIBSARBPROC)GetProcAddress("glXCreateContextAttribsARB");
	if (!glXCreateContextAttribsARB) return false;

	// Create a context
	int context_attribs[] =
	{
		GLX_CONTEXT_MAJOR_VERSION_ARB, major,
		GLX_CONTEXT_MINOR_VERSION_ARB, minor,
		// Keep compatibility for old cruft
		GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
		//GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_DEBUG_BIT_ARB | GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
		// FIXME : Request a debug context to ease opengl development
#if defined(ZEROGS_DEVBUILD) || defined(_DEBUG)
		GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_DEBUG_BIT_ARB,
#endif
		None
	};

	glxContext = glXCreateContextAttribsARB(NativeDisplay, fbc[0], 0, true, context_attribs);
	if (!glxContext) {
		ZZLog::Error_Log("GLX: failed to create an opengl context");
		return false;
	}

	XSync( NativeDisplay, false);

	glXMakeCurrent(NativeDisplay, NativeWindow, glxContext);

	return true;
}
#endif

bool GLWindow::CreateContextGL()
{
	bool ret;
#if defined(OGL4_LOG) || defined(GLSL4_API)
	// We need to define a debug context. So we need at a 3.0 context (if not 3.2 actually)
	ret = CreateContextGL(3, 3);
#else
	// FIXME there was some issue with previous context creation on Geforce7. Code was rewritten
	// for GSdx unfortunately it was not tested on Geforce7 so keep the 2.0 context for now.
	// Note: Geforce 6&7 was dropped from nvidia driver (2012)
#if 0
	ret = CreateContextGL(3, 0)
	if (! ret )
		ret = CreateContextGL(2, 0);
#else
	ret = CreateContextGL(2, 0);
#endif

#endif
	return ret;
}

#ifdef EGL_API
bool GLWindow::CreateContextGL(int major, int minor)
{
	EGLConfig eglConfig;
	EGLint numConfigs;
	EGLint contextAttribs[] =
	{
		EGL_CONTEXT_MAJOR_VERSION_KHR, major,
		EGL_CONTEXT_MINOR_VERSION_KHR, minor,
		// Keep compatibility for old cruft
		EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR, EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT_KHR,
		//EGL_CONTEXT_FLAGS_KHR, EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR | EGL_CONTEXT_OPENGL_FORWARD_COMPATIBLE_BIT_KHR,
		// FIXME : Request a debug context to ease opengl development
#if defined(ZEROGS_DEVBUILD) || defined(_DEBUG)
		EGL_CONTEXT_FLAGS_KHR, EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR,
#endif
		EGL_NONE
	};
	EGLint attrList[] = {
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_DEPTH_SIZE, 24,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
		EGL_NONE
	};

	eglBindAPI(EGL_OPENGL_API);

	if ( !eglChooseConfig(eglDisplay, attrList, &eglConfig, 1, &numConfigs) )
	{
		ZZLog::Error_Log("EGL: Failed to get a frame buffer config!");
		return EGL_FALSE;
	}

	eglSurface = eglCreateWindowSurface(eglDisplay, eglConfig, NativeWindow, NULL);
	if ( eglSurface == EGL_NO_SURFACE )
	{
		ZZLog::Error_Log("EGL: Failed to get a window surface");
		return EGL_FALSE;
	}

	eglContext = eglCreateContext(eglDisplay, eglConfig, EGL_NO_CONTEXT, contextAttribs );
	if ( eglContext == EGL_NO_CONTEXT )
	{
		ZZLog::Error_Log("EGL: Failed to create the context");
		ZZLog::Error_Log("EGL STATUS: %x", eglGetError());
		return EGL_FALSE;
	}

	if ( !eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext) )
	{
		return EGL_FALSE;
	}

	return EGL_TRUE;
}
#endif


#ifdef USE_GSOPEN2
bool GLWindow::DisplayWindow(int _width, int _height)
{
	GetWindowSize();

	if ( !CreateContextGL() ) return false;

	PrintProtocolVersion();

	return true;
}
#else
bool GLWindow::DisplayWindow(int _width, int _height)
{
	backbuffer.w = _width;
	backbuffer.h = _height;

	NativeWindow = XCreateSimpleWindow(NativeDisplay, DefaultRootWindow(NativeDisplay), conf.x, conf.y, backbuffer.w, backbuffer.h, 0, 0, 0);

    // Draw the window
    XMapRaised(NativeDisplay, NativeWindow);
    XSync(NativeDisplay, false);

	if ( !CreateContextGL() ) return false;

	PrintProtocolVersion();

    GetWindowSize();

	return true;
}
#endif

void* GLWindow::GetProcAddress(const char *function)
{
#ifdef EGL_API
	return (void*)eglGetProcAddress(function);
#endif
#ifdef GLX_API
	return (void*)glXGetProcAddress((const GLubyte*)function);
#endif
}

void GLWindow::SwapGLBuffers()
{
	if (glGetError() != GL_NO_ERROR) ZZLog::Debug_Log("glError before swap!");
	ZZLog::Check_GL_Error();

#ifdef GLX_API
	glXSwapBuffers(NativeDisplay, NativeWindow);
#endif
#ifdef EGL_API
	eglSwapBuffers(eglDisplay, eglSurface);
#endif
	// glClear(GL_COLOR_BUFFER_BIT);
}

void GLWindow::InitVsync(bool extension)
{
#ifdef GLX_API
	vsync_supported = false;
	if (extension) {
		swapinterval = (_PFNSWAPINTERVAL)GetProcAddress("glXSwapInterval");

		if (!swapinterval)
			swapinterval = (_PFNSWAPINTERVAL)GetProcAddress("glXSwapIntervalSGI");

		if (!swapinterval)
			swapinterval = (_PFNSWAPINTERVAL)GetProcAddress("glXSwapIntervalEXT");

		if (swapinterval) {
			swapinterval(0);
			vsync_supported = true;
		} else {
			ZZLog::Error_Log("GLX: No support for SwapInterval (framerate clamped to monitor refresh rate),");
		}
	}

#endif
}

void GLWindow::SetVsync(bool enable)
{
	fprintf(stderr, "change vsync %d\n", enable);
#ifdef EGL_API
	eglSwapInterval(eglDisplay, enable);
#endif
#ifdef GLX_API
	if (vsync_supported && swapinterval) {
		swapinterval(enable);
	}
#endif
}

u32 THR_KeyEvent = 0; // Value for key event processing between threads
bool THR_bShift = false;
bool THR_bCtrl = false;

void GLWindow::ProcessEvents()
{
	FUNCLOG

#ifdef USE_GSOPEN2
	GetWindowSize();
#endif

	if (THR_KeyEvent)     // This value was passed from GSKeyEvents which could be in another thread
	{
		int my_KeyEvent = THR_KeyEvent;
		bool my_bShift = THR_bShift;
		bool my_bCtrl = THR_bCtrl;
		THR_KeyEvent = 0;

		switch (my_KeyEvent)
		{
			case XK_F5:
			case XK_F6:
			case XK_F7:
			case XK_F9:
				// Note: to avoid some clash with PCSX2 shortcut in GSOpen2.
				// GS shortcut will only be activated when ctrl is press
				if (my_bCtrl)
					OnFKey(my_KeyEvent - XK_F1 + 1, my_bShift);
				break;
		}
	}
}


// ************************** Function that are useless in GSopen2 (GSopen 1 is only used with the debug replayer)
void GLWindow::SetTitle(char *strtitle) { }

#endif
