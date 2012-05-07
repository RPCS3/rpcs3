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

#ifdef GL_X11_WINDOW

#include <X11/Xlib.h>
#include <stdlib.h>

#ifdef USE_GSOPEN2
bool GLWindow::CreateWindow(void *pDisplay)
{
	glWindow = (Window)*((u32*)(pDisplay)+1);
	// Do not take the display which come from pcsx2 neither change it.
	// You need a new one to do the operation in the GS thread
	glDisplay = XOpenDisplay(NULL);

	return true;
}
#else
bool GLWindow::CreateWindow(void *pDisplay)
{
    // init support of multi thread
    if (!XInitThreads())
        ZZLog::Error_Log("Failed to init the xlib concurent threads");

	glDisplay = XOpenDisplay(NULL);

	if (pDisplay == NULL) 
	{
		ZZLog::Error_Log("Failed to create window. Exiting...");
		return false;
	}

	// Allow pad to use the display
	*(Display**)pDisplay = glDisplay;
	// Pad can use the window to grab the input. For the moment just set to 0 to avoid
	// to grab an unknow window... Anyway GSopen1 might be dropped in the future
	*((u32*)(pDisplay)+1) = 0;

	return true;
}
#endif

bool GLWindow::ReleaseContext()
{
    bool status = true;
    if (!glDisplay) return status;

    // free the context
	if (context)
	{
		if (!glXMakeCurrent(glDisplay, None, NULL)) {
			ZZLog::Error_Log("Could not release drawing context.");
            status = false;
        }
			
		glXDestroyContext(glDisplay, context);
		context = NULL;
	}
	
    // free the visual
    if (vi) {
        XFree(vi);
        vi = NULL;
    }

	return status;
}

void GLWindow::CloseWindow()
{
	SaveConfig();
	if (!glDisplay) return;

    XCloseDisplay(glDisplay);
    glDisplay = NULL;
}

bool GLWindow::CreateVisual()
{
	// attributes for a single buffered visual in RGBA format with at least
	// 8 bits per color and a 24 bit depth buffer
	int attrListSgl[] = {GLX_RGBA, GLX_RED_SIZE, 8,
						 GLX_GREEN_SIZE, 8,
						 GLX_BLUE_SIZE, 8,
						 GLX_DEPTH_SIZE, 24,
						 None
						};

	// attributes for a double buffered visual in RGBA format with at least
	// 8 bits per color and a 24 bit depth buffer
	int attrListDbl[] = { GLX_RGBA, GLX_DOUBLEBUFFER,
						  GLX_RED_SIZE, 8,
						  GLX_GREEN_SIZE, 8,
						  GLX_BLUE_SIZE, 8,
						  GLX_DEPTH_SIZE, 24,
						  None
						};

	/* get an appropriate visual */
	vi = glXChooseVisual(glDisplay, DefaultScreen(glDisplay), attrListDbl);

	if (vi == NULL)
	{
		vi = glXChooseVisual(glDisplay, DefaultScreen(glDisplay), attrListSgl);
		doubleBuffered = false;
		ZZLog::Error_Log("Only Singlebuffered Visual!");
	}
	else
	{
		doubleBuffered = true;
		ZZLog::Error_Log("Got Doublebuffered Visual!");
	}

	if (vi == NULL)
	{
		ZZLog::Error_Log("Failed to get buffered Visual!");
		return false;
	}
	return true;
}

void GLWindow::GetWindowSize()
{
    if (!glDisplay or !glWindow) return;

	unsigned int borderDummy;
	Window winDummy;
    s32 xDummy;
    s32 yDummy;
	
    XLockDisplay(glDisplay);
	XGetGeometry(glDisplay, glWindow, &winDummy, &xDummy, &yDummy, &width, &height, &borderDummy, &depth);
    XUnlockDisplay(glDisplay);

    // update the gl buffer size
    UpdateWindowSize(width, height);

#ifndef USE_GSOPEN2
	// too verbose!
    ZZLog::Dev_Log("Resolution %dx%d. Depth %d bpp. Position (%d,%d)", width, height, depth, conf.x, conf.y);
#endif
}

void GLWindow::GetGLXVersion()
{
	int glxMajorVersion, glxMinorVersion;
	
	glXQueryVersion(glDisplay, &glxMajorVersion, &glxMinorVersion);

	if (glXIsDirect(glDisplay, context))
		ZZLog::Error_Log("glX-Version %d.%d with Direct Rendering", glxMajorVersion, glxMinorVersion);
	else
		ZZLog::Error_Log("glX-Version %d.%d with Indirect Rendering !!! It will be slow", glxMajorVersion, glxMinorVersion);

}

bool GLWindow::CreateContextGL(int major, int minor)
{
	if (!glDisplay) return false;

	if (major <= 2) {
		context = glXCreateContext(glDisplay, vi, NULL, GL_TRUE);
		return true;
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
	GLXFBConfig *fbc = glXChooseFBConfig(glDisplay, DefaultScreen(glDisplay), attrListDbl, &fbcount);
	if (!fbc || fbcount < 1) return false;

	PFNGLXCREATECONTEXTATTRIBSARBPROC glXCreateContextAttribsARB = (PFNGLXCREATECONTEXTATTRIBSARBPROC)glXGetProcAddress((const GLubyte*) "glXCreateContextAttribsARB");
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
		GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_DEBUG_BIT_ARB,
		None
	};

	context = glXCreateContextAttribsARB(glDisplay, fbc[0], 0, true, context_attribs);
	if (!context) return false;

	XSync( glDisplay, false);

	return true;
}

void GLWindow::CreateContextGL()
{
#if defined(OGL4_LOG) || defined(GLSL4_API)
	// We need to define a debug context. So we need at a 3.0 context (if not 3.2 actually)
	CreateContextGL(4, 2);
#else
	// FIXME there was some issue with previous context creation on Geforce7. Code was rewritten
	// for GSdx unfortunately it was not tested on Geforce7 so keep the 2.0 context for now.
#if 0
	if (! CreateContextGL(3, 0) )
		CreateContextGL(2, 0);
#else
	CreateContextGL(2, 0);
#endif

#endif
}

#ifdef USE_GSOPEN2
bool GLWindow::DisplayWindow(int _width, int _height)
{
	GetWindowSize();

	if (!CreateVisual()) return false;


	CreateContextGL();
	// connect the glx-context to the window
	glXMakeCurrent(glDisplay, glWindow, context);

	GetGLXVersion();

	return true;
}
#else
bool GLWindow::DisplayWindow(int _width, int _height)
{
	backbuffer.w = _width;
	backbuffer.h = _height;

	if (!CreateVisual()) return false;

	/* create a color map */
	attr.colormap = XCreateColormap(glDisplay, RootWindow(glDisplay, vi->screen),
						   vi->visual, AllocNone);
	attr.border_pixel = 0;
    attr.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask | ButtonPressMask |
        StructureNotifyMask | SubstructureRedirectMask | SubstructureNotifyMask |
        EnterWindowMask | LeaveWindowMask | FocusChangeMask ;

    // Create a window at the last position/size
    glWindow = XCreateWindow(glDisplay, RootWindow(glDisplay, vi->screen),
            conf.x , conf.y , _width, _height, 0, vi->depth, InputOutput, vi->visual,
            CWBorderPixel | CWColormap | CWEventMask,
            &attr);

    /* Allow to kill properly the window */
    Atom wmDelete = XInternAtom(glDisplay, "WM_DELETE_WINDOW", True);
    XSetWMProtocols(glDisplay, glWindow, &wmDelete, 1);

    // Set icon name
    XSetIconName(glDisplay, glWindow, "ZZogl-pg");

    // Draw the window
    XMapRaised(glDisplay, glWindow);
    XSync(glDisplay, false);

	// connect the glx-context to the window
	CreateContextGL();
	glXMakeCurrent(glDisplay, glWindow, context);
	
	GetGLXVersion();

    // Always start in window mode
	fullScreen = 0;
    GetWindowSize();

	return true;
}
#endif

void GLWindow::SwapGLBuffers()
{
	if (glGetError() != GL_NO_ERROR) ZZLog::Debug_Log("glError before swap!");
	ZZLog::Check_GL_Error();

	// FIXME I think we need to flush when there is only 1 visual buffer
	glXSwapBuffers(glDisplay, glWindow);
	// glClear(GL_COLOR_BUFFER_BIT);
}

u32 THR_KeyEvent = 0; // Value for key event processing between threads
bool THR_bShift = false;
bool THR_bCtrl = false;

void GLWindow::ProcessEvents()
{
	FUNCLOG

#ifdef USE_GSOPEN2
	GetWindowSize();
#else
	ResizeCheck();
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


// ************************** Function that are either stub or useless in GSOPEN2
#define _NET_WM_STATE_REMOVE 0
#define _NET_WM_STATE_ADD 1
#define _NET_WM_STATE_TOGGLE 2

void GLWindow::Force43Ratio()
{
#ifndef USE_GSOPEN2
    // avoid black border in fullscreen
    if (fullScreen && conf.isWideScreen) {
        conf.width = width;
        conf.height = height;
    }

    if(!fullScreen && !conf.isWideScreen) {
        // Compute the width based on height
        s32 new_width = (4*height)/3;
        // do not bother to resize for 5 pixels. Avoid a loop
        // due to round value
        if ( ABS(new_width - width) > 5) {
            width = new_width;
            conf.width = new_width;
            // resize the window
            XLockDisplay(glDisplay);
            XResizeWindow(glDisplay, glWindow, new_width, height);
            XSync(glDisplay, False);
            XUnlockDisplay(glDisplay);
        }
    }
#endif
}

void GLWindow::UpdateGrabKey()
{
    // Do not stole the key in debug mode. It is not breakpoint friendly...
#ifndef _DEBUG
    XLockDisplay(glDisplay);
    if (fullScreen) {
        XGrabPointer(glDisplay, glWindow, True, ButtonPressMask, GrabModeAsync, GrabModeAsync, glWindow, None, CurrentTime);
        XGrabKeyboard(glDisplay, glWindow, True, GrabModeAsync, GrabModeAsync, CurrentTime);
    } else {
        XUngrabPointer(glDisplay, CurrentTime);
        XUngrabKeyboard(glDisplay, CurrentTime);
    }
    XUnlockDisplay(glDisplay);
#endif
}

void GLWindow::ToggleFullscreen()
{
#ifndef USE_GSOPEN2
    if (!glDisplay or !glWindow) return;

    Force43Ratio();

    u32 mask = SubstructureRedirectMask | SubstructureNotifyMask;
    // Setup a new event structure
    XClientMessageEvent cme;
    cme.type = ClientMessage;
    cme.send_event = True;
    cme.display = glDisplay;
    cme.window  = glWindow;
    cme.message_type = XInternAtom(glDisplay, "_NET_WM_STATE", False);
    cme.format = 32;
    // Note: can not use _NET_WM_STATE_TOGGLE because the WM can change the fullscreen state
    // and screw up the fullscreen variable... The test on fulscreen restore a sane configuration
    cme.data.l[0] = fullScreen  ? _NET_WM_STATE_REMOVE : _NET_WM_STATE_ADD;
    cme.data.l[1] = (u32)XInternAtom(glDisplay, "_NET_WM_STATE_FULLSCREEN", False);
    cme.data.l[2] = 0;
    cme.data.l[3] = 0;

    // send the event
    XLockDisplay(glDisplay);
    if (!XSendEvent(glDisplay, RootWindow(glDisplay, vi->screen), False, mask, (XEvent*)(&cme)))
        ZZLog::Error_Log("Failed to send event: toggle fullscreen");
    else {
        fullScreen = (!fullScreen);
        conf.setFullscreen(fullScreen);
    }
    XUnlockDisplay(glDisplay);

    // Apply the change
    XSync(glDisplay, false);

    // Wait a little that the VM does his joes. Actually the best is to check some WM event
    // but it not sure it will appear so a time out is necessary.
    usleep(100*1000); // 100 us should be far enough for old computer and unnoticeable for users

    // update info structure
    GetWindowSize();

	UpdateGrabKey();

    // avoid black border in widescreen fullscreen
    if (fullScreen && conf.isWideScreen) {
        conf.width = width;
        conf.height = height;
    }

    // Hide the cursor in the right bottom corner
    if(fullScreen)
        XWarpPointer(glDisplay, None, glWindow, 0, 0, 0, 0, 2*width, 2*height);

#endif
}

void GLWindow::ResizeCheck()
{
	XEvent event;
    if (!glDisplay or !glWindow) return;

    XLockDisplay(glDisplay);
	while (XCheckTypedWindowEvent(glDisplay, glWindow, ConfigureNotify, &event))
	{
		if ((event.xconfigure.width != width) || (event.xconfigure.height != height))
		{
			width = event.xconfigure.width;
			height = event.xconfigure.height;
            Force43Ratio();
			UpdateWindowSize(width, height);
		}

        if (!fullScreen) {
            if ((event.xconfigure.x != conf.x) || (event.xconfigure.y != conf.y))
            {
                // Fixme; x&y occassionally gives values near the top left corner rather then the real values,
                // causing the window to change positions when adjusting ZZOgl's settings.
                conf.x = event.xconfigure.x;
                conf.y = event.xconfigure.y;
            }
        }
	}
    XUnlockDisplay(glDisplay);
}

void GLWindow::SetTitle(char *strtitle)
{
#ifndef USE_GSOPEN2
    if (!glDisplay or !glWindow) return;
	if (fullScreen) return;

    XTextProperty prop;
    memset(&prop, 0, sizeof(prop));

    char* ptitle = strtitle;
    if (XStringListToTextProperty(&ptitle, 1, &prop)) {
        XLockDisplay(glDisplay);
        XSetWMName(glDisplay, glWindow, &prop);
        XUnlockDisplay(glDisplay);
    }

    XFree(prop.value);
#endif
}

#endif
