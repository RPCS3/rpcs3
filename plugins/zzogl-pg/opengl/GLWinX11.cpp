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
#include "zerogs.h"

#ifdef GL_X11_WINDOW

#include <X11/Xlib.h>

bool GLWindow::CreateWindow(void *pDisplay)
{
    // init support of multi thread
    if (!XInitThreads())
        ZZLog::Error_Log("Failed to init the xlib concurent threads");

	glDisplay = XOpenDisplay(0);
	glScreen = DefaultScreen(glDisplay);

	if (pDisplay == NULL) return false;

	*(Display**)pDisplay = glDisplay;

	return true;
}

bool GLWindow::ReleaseContext()
{
	if (context && (glDisplay != NULL))
	{
		if (!glXMakeCurrent(glDisplay, None, NULL))
		{
			ZZLog::Error_Log("Could not release drawing context.");
		}
			
		glXDestroyContext(glDisplay, context);

		context = NULL;
	}
	
	return true;
}

void GLWindow::CloseWindow()
{
	conf.x = x;
	conf.y = y;
	SaveConfig();
	if (!glDisplay) return;

	/* switch back to original desktop resolution if we were in fullscreen */
    if (fullScreen)
        ToggleFullscreen();

    XSync(glDisplay, False);
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
	vi = glXChooseVisual(glDisplay, glScreen, attrListDbl);

	if (vi == NULL)
	{
		vi = glXChooseVisual(glDisplay, glScreen, attrListSgl);
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
	
	XGetGeometry(glDisplay, glWindow, &winDummy, &xDummy, &yDummy, &width, &height, &borderDummy, &depth);
    ZZLog::Error_Log("Resolution %dx%d. Depth %d bpp. Position (%d,%d)", width, height, depth, x, y);
}

void GLWindow::GetGLXVersion()
{
	int glxMajorVersion, glxMinorVersion;
	
	glXQueryVersion(glDisplay, &glxMajorVersion, &glxMinorVersion);

	ZZLog::Error_Log("glX-Version %d.%d", glxMajorVersion, glxMinorVersion);
}

void GLWindow::UpdateGrabKey() {
    XLockDisplay(glDisplay);
    if (fullScreen) {
        XGrabPointer(glDisplay, glWindow, True, ButtonPressMask, GrabModeAsync, GrabModeAsync, glWindow, None, CurrentTime);
        XGrabKeyboard(glDisplay, glWindow, True, GrabModeAsync, GrabModeAsync, CurrentTime);
    } else {
        XUngrabPointer(glDisplay, CurrentTime);
        XUngrabKeyboard(glDisplay, CurrentTime);
    }
    XUnlockDisplay(glDisplay);
}

#define _NET_WM_STATE_REMOVE 0
#define _NET_WM_STATE_ADD 1
#define _NET_WM_STATE_TOGGLE 2

void GLWindow::ToggleFullscreen()
{
    if (!glDisplay or !glWindow) return;

    u32 mask = SubstructureRedirectMask | SubstructureNotifyMask;
    // Setup a new event structure
    XClientMessageEvent cme;

    cme.type = ClientMessage;
    cme.send_event = True;
    cme.display = glDisplay;
    cme.window  = glWindow;
    cme.message_type = XInternAtom(glDisplay, "_NET_WM_STATE", False);
    cme.format = 32;
    cme.data.l[0] = _NET_WM_STATE_TOGGLE;
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
    XSync(glDisplay, False);

    // update info structure
    GetWindowSize();

    // Grab key if needed
    UpdateGrabKey();

    // Hide the cursor in the right bottom corner
    if(fullScreen)
        XWarpPointer(glDisplay, None, glWindow, 0, 0, 0, 0, width, height);

}

bool GLWindow::DisplayWindow(int _width, int _height)
{
	Colormap cmap;
	
	x = conf.x;
	y = conf.y;

	if (!CreateVisual()) return false;
	
	/* create a GLX context */
	context = glXCreateContext(glDisplay, vi, NULL, GL_TRUE);
	
	/* create a color map */
	cmap = XCreateColormap(glDisplay, RootWindow(glDisplay, vi->screen),
						   vi->visual, AllocNone);
	
	attr.colormap = cmap;
	attr.border_pixel = 0;
    attr.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask | ButtonPressMask |
        StructureNotifyMask | SubstructureRedirectMask | SubstructureNotifyMask |
        EnterWindowMask | LeaveWindowMask | FocusChangeMask ;

	GetGLXVersion();

    glWindow = XCreateWindow(glDisplay, RootWindow(glDisplay, vi->screen),
            0, 0, _width, _height, 0, vi->depth, InputOutput, vi->visual,
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
	glXMakeCurrent(glDisplay, glWindow, context);
	
	if (glXIsDirect(glDisplay, context))
		ZZLog::Error_Log("You have Direct Rendering!");
	else
		ZZLog::Error_Log("No Direct Rendering possible!");

    // init fullscreen to 0. ToggleFullscreen will update it
	fullScreen = 0;
	if (conf.fullscreen()) {
        ToggleFullscreen();
    } else {
        // Restore the window position
        XMoveWindow(glDisplay, glWindow, x, y);
        GetWindowSize();
    }

	return true;
}

void GLWindow::SwapGLBuffers()
{
	glXSwapBuffers(glDisplay, glWindow);
}

void GLWindow::SetTitle(char *strtitle)
{
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
}

void GLWindow::ResizeCheck()
{
	XEvent event;

	while (XCheckTypedEvent(glDisplay, ConfigureNotify, &event))
	{
		if ((event.xconfigure.width != width) || (event.xconfigure.height != height))
		{
			ZeroGS::ChangeWindowSize(event.xconfigure.width, event.xconfigure.height);
			width = event.xconfigure.width;
			height = event.xconfigure.height;
		}

        if (!fullScreen) {
            if ((event.xconfigure.x != x) || (event.xconfigure.y != y))
            {
                // Fixme; x&y occassionally gives values near the top left corner rather then the real values,
                // causing the window to change positions when adjusting ZZOgl's settings.
                x = event.xconfigure.x;
                y = event.xconfigure.y;
            }
        }
	}

}

#endif
