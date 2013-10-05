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
#include "GSWndEGL.h"

#if defined(_LINUX)

GSWndEGL::GSWndEGL()
	: m_NativeWindow(0), m_NativeDisplay(NULL)
{
}

void GSWndEGL::CreateContext(int major, int minor)
{
	EGLConfig eglConfig;
	EGLint numConfigs;
	EGLint contextAttribs[] =
#ifdef ENABLE_GLES
	{
		 EGL_CONTEXT_CLIENT_VERSION, 2,
		 EGL_NONE
	};
#else
	{
		EGL_CONTEXT_MAJOR_VERSION_KHR, major,
		EGL_CONTEXT_MINOR_VERSION_KHR, minor,
#ifdef ENABLE_OGL_DEBUG
		EGL_CONTEXT_FLAGS_KHR, EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR,
#endif
		EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR,
		EGL_NONE
	};
#endif
	EGLint NullContextAttribs[] = { EGL_NONE };
	EGLint attrList[] = {
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_DEPTH_SIZE, 24,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
		EGL_NONE
	};

#ifndef ENABLE_GLES
	eglBindAPI(EGL_OPENGL_API);
#endif

	if ( !eglChooseConfig(m_eglDisplay, attrList, &eglConfig, 1, &numConfigs) )
	{
		fprintf(stderr,"EGL: Failed to get a frame buffer config!\n");
		throw GSDXRecoverableError();
	}

	m_eglSurface = eglCreateWindowSurface(m_eglDisplay, eglConfig, m_NativeWindow, NULL);
	if ( m_eglSurface == EGL_NO_SURFACE )
	{
		fprintf(stderr,"EGL: Failed to get a window surface\n");
		throw GSDXRecoverableError();
	}

	m_eglContext = eglCreateContext(m_eglDisplay, eglConfig, EGL_NO_CONTEXT, contextAttribs);
	EGLint status = eglGetError();
	if (status == EGL_BAD_ATTRIBUTE || status == EGL_BAD_MATCH) {
		// Radeon/Gallium don't support advance attribute. Fallback to random value
		// Note: Intel gives an EGL_BAD_MATCH. I don't know why but let's by stubborn and retry.
		fprintf(stderr, "EGL: warning your driver doesn't suport advance openGL context attributes\n");
		m_eglContext = eglCreateContext(m_eglDisplay, eglConfig, EGL_NO_CONTEXT, NullContextAttribs);
		status = eglGetError();
	}
	if ( m_eglContext == EGL_NO_CONTEXT )
	{
		fprintf(stderr,"EGL: Failed to create the context\n");
		fprintf(stderr,"EGL STATUS: %x\n", status);
		throw GSDXRecoverableError();
	}

	if ( !eglMakeCurrent(m_eglDisplay, m_eglSurface, m_eglSurface, m_eglContext) )
	{
		throw GSDXRecoverableError();
	}
}

void GSWndEGL::AttachContext()
{
	if (!IsContextAttached()) {
		// The setting of the API is local to a thread. This function 
		// can be called from 2 threads.
#ifndef ENABLE_GLES
		eglBindAPI(EGL_OPENGL_API);
#endif

		//fprintf(stderr, "Attach the context\n");
		eglMakeCurrent(m_eglDisplay, m_eglSurface, m_eglSurface, m_eglContext);
		m_ctx_attached = true;
	}
}

void GSWndEGL::DetachContext()
{
	if (IsContextAttached()) {
		//fprintf(stderr, "Detach the context\n");
		eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		m_ctx_attached = false;
	}
}

void GSWndEGL::CheckContext()
{
	fprintf(stderr,"EGL: %s : %s\n", eglQueryString(m_eglDisplay, EGL_VENDOR) , eglQueryString(m_eglDisplay, EGL_VERSION) );
	fprintf(stderr,"EGL: extensions supported: %s\n", eglQueryString(m_eglDisplay, EGL_EXTENSIONS));
}

bool GSWndEGL::Attach(void* handle, bool managed)
{
	m_NativeWindow = *(Window*)handle;
	m_managed = managed;

	m_NativeDisplay = XOpenDisplay(NULL);
	OpenEGLDisplay();

	CreateContext(3, 3);

	AttachContext();

	CheckContext();

	PopulateGlFunction();

	return true;
}

void GSWndEGL::Detach()
{
	// Actually the destructor is not called when there is only a GSclose/GSshutdown
	// The window still need to be closed
	DetachContext();
	eglDestroyContext(m_eglDisplay, m_eglContext);
	m_eglContext = NULL;
	eglDestroySurface(m_eglDisplay, m_eglSurface);
	m_eglSurface = NULL;
	CloseEGLDisplay();

	if (m_NativeDisplay) {
		XCloseDisplay(m_NativeDisplay);
		m_NativeDisplay = NULL;
	}
}

bool GSWndEGL::Create(const string& title, int w, int h)
{
	if(m_NativeWindow)
		throw GSDXRecoverableError();

	if(w <= 0 || h <= 0) {
		w = theApp.GetConfig("ModeWidth", 640);
		h = theApp.GetConfig("ModeHeight", 480);
	}

	m_managed = true;

	// note this part must be only executed when replaying .gs debug file
	m_NativeDisplay = XOpenDisplay(NULL);
	OpenEGLDisplay();

	m_NativeWindow = XCreateSimpleWindow(m_NativeDisplay, DefaultRootWindow(m_NativeDisplay), 0, 0, w, h, 0, 0, 0);
	XMapWindow (m_NativeDisplay, m_NativeWindow);

	CreateContext(3, 3);

	AttachContext();

	CheckContext();

	PopulateGlFunction();

	if (m_NativeWindow == 0)
		throw GSDXRecoverableError();

	return true;
}

void* GSWndEGL::GetProcAddress(const char* name, bool opt)
{
	void* ptr = (void*)eglGetProcAddress(name);
	if (ptr == NULL) {
		fprintf(stderr, "Failed to find %s\n", name);
		if (!opt)
			throw GSDXRecoverableError();
	}
	return ptr;
}

void* GSWndEGL::GetDisplay()
{
	// note this part must be only executed when replaying .gs debug file
	return (void*)m_NativeDisplay;
}

GSVector4i GSWndEGL::GetClientRect()
{
	unsigned int h = 480;
	unsigned int w = 640;

	unsigned int borderDummy;
	unsigned int depthDummy;
	Window winDummy;
    int xDummy;
    int yDummy;

	if (!m_NativeDisplay) m_NativeDisplay = XOpenDisplay(NULL);
	XGetGeometry(m_NativeDisplay, m_NativeWindow, &winDummy, &xDummy, &yDummy, &w, &h, &borderDummy, &depthDummy);

	return GSVector4i(0, 0, (int)w, (int)h);
}

// Returns FALSE if the window has no title, or if th window title is under the strict
// management of the emulator.

bool GSWndEGL::SetWindowText(const char* title)
{
	if (!m_managed) return true;

	XTextProperty prop;

	memset(&prop, 0, sizeof(prop));

	char* ptitle = (char*)title;
	if (XStringListToTextProperty(&ptitle, 1, &prop)) {
		XSetWMName(m_NativeDisplay, m_NativeWindow, &prop);
	}

	XFree(prop.value);
	XFlush(m_NativeDisplay);

	return true;
}

void GSWndEGL::SetVSync(bool enable)
{
	// 0 -> disable vsync
	// n -> wait n frame
	eglSwapInterval(m_eglDisplay, enable);
}

void GSWndEGL::Flip()
{
	eglSwapBuffers(m_eglDisplay, m_eglSurface);
}

void GSWndEGL::Show()
{
	XMapRaised(m_NativeDisplay, m_NativeWindow);
	XFlush(m_NativeDisplay);
}

void GSWndEGL::Hide()
{
	XUnmapWindow(m_NativeDisplay, m_NativeWindow);
	XFlush(m_NativeDisplay);
}

void GSWndEGL::HideFrame()
{
	// TODO
}

void GSWndEGL::CloseEGLDisplay()
{
	eglReleaseThread();
	eglTerminate(m_eglDisplay);
}

void GSWndEGL::OpenEGLDisplay()
{
	// Create an EGL display from the native display
	m_eglDisplay = eglGetDisplay((EGLNativeDisplayType)m_NativeDisplay);
	if ( m_eglDisplay == EGL_NO_DISPLAY )
		throw GSDXRecoverableError();

	if ( !eglInitialize(m_eglDisplay, NULL, NULL) )
		throw GSDXRecoverableError();
}

#endif
