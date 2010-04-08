/*  ZeroGS KOSMOS
 *  Copyright (C) 2005-2006 zerofrog@gmail.com
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
 
#include "GS.h"
#include "zerogs.h"

#ifdef GL_X11_WINDOW

bool GLWindow::CreateWindow(void *pDisplay)
{	
    glDisplay = XOpenDisplay(0);
	glScreen = DefaultScreen(glDisplay);

	if ( pDisplay == NULL ) return false;
	
    *(Display**)pDisplay = glDisplay;
	
	return true;
}

bool GLWindow::DestroyWindow()
{	
    if (context)
	{
		if (!glXMakeCurrent(glDisplay, None, NULL))
		{
			ERROR_LOG("Could not release drawing context.\n");
		}
		
		glXDestroyContext(glDisplay, context);
		context = NULL;
	}
	
	/* switch back to original desktop resolution if we were in fullScreen */
	if ( glDisplay != NULL ) 
	{
		if (fullScreen) 
		{
				XF86VidModeSwitchToMode(glDisplay, glScreen, &deskMode);
				XF86VidModeSetViewPort(glDisplay, glScreen, 0, 0);
		}
	}
}

void GLWindow::CloseWindow()
{	
    if ( glDisplay != NULL ) 
    {
		XCloseDisplay(glDisplay);
		glDisplay = NULL;
	}
}

void GLWindow::DisplayWindow(int _width, int _height)
{
    XVisualInfo *vi;
	Colormap cmap;
	int GLDisplayWidth, GLDisplayHeight;
	int glxMajorVersion, glxMinorVersion;
	int vidModeMajorVersion, vidModeMinorVersion;
	Atom wmDelete;
	Window winDummy;
	unsigned int borderDummy;
	
	// attributes for a single buffered visual in RGBA format with at least
	// 8 bits per color and a 24 bit depth buffer
	int attrListSgl[] = {GLX_RGBA, GLX_RED_SIZE, 8, 
						 GLX_GREEN_SIZE, 8, 
						 GLX_BLUE_SIZE, 8, 
						 GLX_DEPTH_SIZE, 24,
						 None};

	// attributes for a double buffered visual in RGBA format with at least 
	// 8 bits per color and a 24 bit depth buffer
	int attrListDbl[] = { GLX_RGBA, GLX_DOUBLEBUFFER, 
						  GLX_RED_SIZE, 8, 
						  GLX_GREEN_SIZE, 8, 
						  GLX_BLUE_SIZE, 8, 
						  GLX_DEPTH_SIZE, 24,
						  None };

	fullScreen = !!(conf.options & GSOPTION_FULLSCREEN);
	
	/* get an appropriate visual */
	vi = glXChooseVisual(glDisplay, glScreen, attrListDbl);
	if (vi == NULL) {
		vi = glXChooseVisual(glDisplay, glScreen, attrListSgl);
		doubleBuffered = false;
		ERROR_LOG("Only Singlebuffered Visual!\n");
	}
	else {
		doubleBuffered = true;
		ERROR_LOG("Got Doublebuffered Visual!\n");
	}
	if (vi == NULL)                                           
	{                                                         
		ERROR_LOG("Failed to get buffered Visual!\n");    
		return false;                                     
	}

	glXQueryVersion(glDisplay, &glxMajorVersion, &glxMinorVersion);
	ERROR_LOG("glX-Version %d.%d\n", glxMajorVersion, glxMinorVersion);
	/* create a GLX context */
	context = glXCreateContext(glDisplay, vi, 0, GL_TRUE);
	/* create a color map */
	cmap = XCreateColormap(glDisplay, RootWindow(glDisplay, vi->screen),
						   vi->visual, AllocNone);
	attr.colormap = cmap;
	attr.border_pixel = 0;

	// get a connection
	XF86VidModeQueryVersion(glDisplay, &vidModeMajorVersion, &vidModeMinorVersion);

	if (fullScreen) {
		
		XF86VidModeModeInfo **modes = NULL;
		int modeNum = 0;
		int bestMode = 0;

		// set best mode to current
		bestMode = 0;
		ERROR_LOG("XF86VidModeExtension-Version %d.%d\n", vidModeMajorVersion, vidModeMinorVersion);
		XF86VidModeGetAllModeLines(glDisplay, glScreen, &modeNum, &modes);
		
		if( modeNum > 0 && modes != NULL ) {
			/* save desktop-resolution before switching modes */
			GLWin.deskMode = *modes[0];
			/* look for mode with requested resolution */
			for (int i = 0; i < modeNum; i++) {
				if ((modes[i]->hdisplay == _width) && (modes[i]->vdisplay == _height)) {
					bestMode = i;
				}
			}	

			XF86VidModeSwitchToMode(glDisplay, glScreen, modes[bestMode]);
			XF86VidModeSetViewPort(glDisplay, glScreen, 0, 0);
			GLDisplayWidth = modes[bestMode]->hdisplay;
			GLDisplayHeight = modes[bestMode]->vdisplay;
			ERROR_LOG("Resolution %dx%d\n", GLDisplayWidth, GLDisplayHeight);
			XFree(modes);
			
			/* create a fullscreen window */
			attr.override_redirect = True;
			attr.event_mask = ExposureMask | KeyPressMask | ButtonPressMask |
				StructureNotifyMask;
			glWindow = XCreateWindow(glDisplay, RootWindow(glDisplay, vi->screen),
									  0, 0, GLDisplayWidth, GLDisplayHeight, 0, vi->depth, InputOutput, vi->visual,
									  CWBorderPixel | CWColormap | CWEventMask | CWOverrideRedirect,
									  &attr);
			XWarpPointer(glDisplay, None, glWindow, 0, 0, 0, 0, 0, 0);
			XMapRaised(glDisplay, glWindow);
			XGrabKeyboard(glDisplay, glWindow, True, GrabModeAsync,
						  GrabModeAsync, CurrentTime);
			XGrabPointer(glDisplay, glWindow, True, ButtonPressMask,
						 GrabModeAsync, GrabModeAsync, glWindow, None, CurrentTime);
		}
		else {
			ERROR_LOG("Failed to start fullscreen. If you received the \n"
					  "\"XFree86-VidModeExtension\" extension is missing, add\n"
					  "Load \"extmod\"\n"
					  "to your X configuration file (under the Module Section)\n");
			fullScreen = 0;
		}
	}
	
	
	if( !fullScreen ) {

		//XRootWindow(glDisplay,glScreen)
		//int X = (rcdesktop.right-rcdesktop.left)/2 - (rc.right-rc.left)/2;
		//int Y = (rcdesktop.bottom-rcdesktop.top)/2 - (rc.bottom-rc.top)/2;

		// create a window in window mode
		attr.event_mask = ExposureMask | KeyPressMask | ButtonPressMask |
			StructureNotifyMask;
		glWindow = XCreateWindow(glDisplay, RootWindow(glDisplay, vi->screen),
								  0, 0, _width, _height, 0, vi->depth, InputOutput, vi->visual,
								  CWBorderPixel | CWColormap | CWEventMask, &attr);
		// only set window title and handle wm_delete_events if in windowed mode
		wmDelete = XInternAtom(glDisplay, "WM_DELETE_WINDOW", True);
		XSetWMProtocols(glDisplay, glWindow, &wmDelete, 1);
		XSetStandardProperties(glDisplay, glWindow, "ZeroGS",
								   "ZeroGS", None, NULL, 0, NULL);
		XMapRaised(glDisplay, glWindow);
	}	   

	// connect the glx-context to the window
	glXMakeCurrent(glDisplay, glWindow, context);
	XGetGeometry(glDisplay, glWindow, &winDummy, &x, &y,
				 &width, &height, &borderDummy, &depth);
	ERROR_LOG("Depth %d\n", depth);
	if (glXIsDirect(glDisplay, context)) 
		ERROR_LOG("you have Direct Rendering!\n");
	else
		ERROR_LOG("no Direct Rendering possible!\n");

	// better for pad plugin key input (thc)
	XSelectInput(glDisplay, glWindow, ExposureMask | KeyPressMask | KeyReleaseMask | 
				 ButtonPressMask | StructureNotifyMask | EnterWindowMask | LeaveWindowMask |
				 FocusChangeMask );
}

void GLWindow::SwapBuffers()
{
    glXSwapBuffers(glDisplay, glWindow);
}
		
void GLWindow::SetTitle(char *strtitle)
{
		XTextProperty prop;
		memset(&prop, 0, sizeof(prop));
		char* ptitle = strtitle;
		if( XStringListToTextProperty(&ptitle, 1, &prop) )
			XSetWMName(glDisplay, glWindow, &prop);
		XFree(prop.value);
}

void GLWindow::ResizeCheck()
{
	XEvent event;
	
	while(XCheckTypedEvent(glDisplay, ConfigureNotify, &event)) 
	{
		if ((event.xconfigure.width != width) || (event.xconfigure.height != height)) {
			ZeroGS::ChangeWindowSize(event.xconfigure.width, event.xconfigure.height);
			width = event.xconfigure.width;
			height = event.xconfigure.height;
		}
	}
}

#endif
