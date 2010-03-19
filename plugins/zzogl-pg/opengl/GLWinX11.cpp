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
	int i;
	XVisualInfo *vi;
	Colormap cmap;
	int dpyWidth, dpyHeight;
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

	GLWin.fullScreen = !!(conf.options & GSOPTION_FULLSCREEN);
	
	/* get an appropriate visual */
	vi = glXChooseVisual(GLWin.glDisplay, GLWin.glScreen, attrListDbl);
	if (vi == NULL) {
		vi = glXChooseVisual(GLWin.glDisplay, GLWin.glScreen, attrListSgl);
		GLWin.doubleBuffered = False;
		ERROR_LOG("Only Singlebuffered Visual!\n");
	}
	else {
		GLWin.doubleBuffered = True;
		ERROR_LOG("Got Doublebuffered Visual!\n");
	}

	glXQueryVersion(GLWin.glDisplay, &glxMajorVersion, &glxMinorVersion);
	ERROR_LOG("glX-Version %d.%d\n", glxMajorVersion, glxMinorVersion);
	/* create a GLX context */
	GLWin.context = glXCreateContext(GLWin.glDisplay, vi, 0, GL_TRUE);
	/* create a color map */
	cmap = XCreateColormap(GLWin.glDisplay, RootWindow(GLWin.glDisplay, vi->screen),
						   vi->visual, AllocNone);
	GLWin.attr.colormap = cmap;
	GLWin.attr.border_pixel = 0;

	// get a connection
	XF86VidModeQueryVersion(GLWin.glDisplay, &vidModeMajorVersion, &vidModeMinorVersion);

	if (GLWin.fullScreen) {
		
		XF86VidModeModeInfo **modes = NULL;
		int modeNum = 0;
		int bestMode = 0;

		// set best mode to current
		bestMode = 0;
		ERROR_LOG("XF86VidModeExtension-Version %d.%d\n", vidModeMajorVersion, vidModeMinorVersion);
		XF86VidModeGetAllModeLines(GLWin.glDisplay, GLWin.glScreen, &modeNum, &modes);
		
		if( modeNum > 0 && modes != NULL ) {
			/* save desktop-resolution before switching modes */
			GLWin.deskMode = *modes[0];
			/* look for mode with requested resolution */
			for (i = 0; i < modeNum; i++) {
				if ((modes[i]->hdisplay == _width) && (modes[i]->vdisplay == _height)) {
					bestMode = i;
				}
			}	

			XF86VidModeSwitchToMode(GLWin.glDisplay, GLWin.glScreen, modes[bestMode]);
			XF86VidModeSetViewPort(GLWin.glDisplay, GLWin.glScreen, 0, 0);
			dpyWidth = modes[bestMode]->hdisplay;
			dpyHeight = modes[bestMode]->vdisplay;
			ERROR_LOG("Resolution %dx%d\n", dpyWidth, dpyHeight);
			XFree(modes);
			
			/* create a fullscreen window */
			GLWin.attr.override_redirect = True;
			GLWin.attr.event_mask = ExposureMask | KeyPressMask | ButtonPressMask |
				StructureNotifyMask;
			GLWin.glWindow = XCreateWindow(GLWin.glDisplay, RootWindow(GLWin.glDisplay, vi->screen),
									  0, 0, dpyWidth, dpyHeight, 0, vi->depth, InputOutput, vi->visual,
									  CWBorderPixel | CWColormap | CWEventMask | CWOverrideRedirect,
									  &GLWin.attr);
			XWarpPointer(GLWin.glDisplay, None, GLWin.glWindow, 0, 0, 0, 0, 0, 0);
			XMapRaised(GLWin.glDisplay, GLWin.glWindow);
			XGrabKeyboard(GLWin.glDisplay, GLWin.glWindow, True, GrabModeAsync,
						  GrabModeAsync, CurrentTime);
			XGrabPointer(GLWin.glDisplay, GLWin.glWindow, True, ButtonPressMask,
						 GrabModeAsync, GrabModeAsync, GLWin.glWindow, None, CurrentTime);
		}
		else {
			ERROR_LOG("Failed to start fullscreen. If you received the \n"
					  "\"XFree86-VidModeExtension\" extension is missing, add\n"
					  "Load \"extmod\"\n"
					  "to your X configuration file (under the Module Section)\n");
			GLWin.fullScreen = false;
		}
	}
	
	if( !GLWin.fullScreen ) {

		//XRootWindow(dpy,screen)
		//int X = (rcdesktop.right-rcdesktop.left)/2 - (rc.right-rc.left)/2;
		//int Y = (rcdesktop.bottom-rcdesktop.top)/2 - (rc.bottom-rc.top)/2;

		// create a window in window mode
		GLWin.attr.event_mask = ExposureMask | KeyPressMask | ButtonPressMask |
			StructureNotifyMask;
		GLWin.glWindow = XCreateWindow(GLWin.glDisplay, RootWindow(GLWin.glDisplay, vi->screen),
								  0, 0, _width, _height, 0, vi->depth, InputOutput, vi->visual,
								  CWBorderPixel | CWColormap | CWEventMask, &GLWin.attr);
		// only set window title and handle wm_delete_events if in windowed mode
		wmDelete = XInternAtom(GLWin.glDisplay, "WM_DELETE_WINDOW", True);
		XSetWMProtocols(GLWin.glDisplay, GLWin.glWindow, &wmDelete, 1);
		XSetStandardProperties(GLWin.glDisplay, GLWin.glWindow, "ZZOgl-PG",
								   "ZZOgl-PG", None, NULL, 0, NULL);
		XMapRaised(GLWin.glDisplay, GLWin.glWindow);
	}	   

	// connect the glx-context to the window
	glXMakeCurrent(GLWin.glDisplay, GLWin.glWindow, GLWin.context);
	XGetGeometry(GLWin.glDisplay, GLWin.glWindow, &winDummy, &GLWin.x, &GLWin.y,
				 &GLWin.width, &GLWin.height, &borderDummy, &GLWin.depth);
	ERROR_LOG("Depth %d\n", GLWin.depth);
	if (glXIsDirect(GLWin.glDisplay, GLWin.context)) 
		ERROR_LOG("you have Direct Rendering!\n");
	else
		ERROR_LOG("no Direct Rendering possible!\n");

	// better for pad plugin key input (thc)
	XSelectInput(GLWin.glDisplay, GLWin.glWindow, ExposureMask | KeyPressMask | KeyReleaseMask | 
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
