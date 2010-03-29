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

#ifdef GL_WIN32_WINDOW

LRESULT WINAPI MsgProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	static int nWindowWidth = 0, nWindowHeight = 0;

	switch( msg ) {
		case WM_DESTROY:
			PostQuitMessage( 0 );
			return 0;

		case WM_KEYDOWN:
//			switch(wParam) {
//				case VK_ESCAPE:
//					SendMessage(hWnd, WM_DESTROY, 0L, 0L);
//					break;
//			}
			break;

		case WM_ACTIVATE:

			if( wParam != WA_INACTIVE ) {
				//DEBUG_LOG("restoring device\n");
				ZeroGS::Restore();
			}

			break;

		case WM_SIZE:
			nWindowWidth = lParam&0xffff;
			nWindowHeight = lParam>>16;
			ZeroGS::ChangeWindowSize(nWindowWidth, nWindowHeight);

			break;

		case WM_SIZING:
			// if button is 0, then just released so can resize
			if( GetSystemMetrics(SM_SWAPBUTTON) ? !GetAsyncKeyState(VK_RBUTTON) : !GetAsyncKeyState(VK_LBUTTON) ) {
				ZeroGS::SetChangeDeviceSize(nWindowWidth, nWindowHeight);
			}
			break;

		case WM_SETCURSOR:
			SetCursor(NULL);
			break;
	}

	return DefWindowProc( hWnd, msg, wParam, lParam );
}

bool GLWindow::CreateWindow(void *pDisplay)
{       
	RECT rc, rcdesktop;
	rc.left = 0; rc.top = 0;
	rc.right = conf.width; rc.bottom = conf.height;

	WNDCLASSEX wc;
	HINSTANCE hInstance = GetModuleHandle(NULL);
	DWORD dwExStyle, dwStyle;

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_CLASSDC;
	wc.lpfnWndProc = (WNDPROC) MsgProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = NULL;
	wc.hIconSm = NULL;
	wc.hCursor = NULL;
	wc.hbrBackground = NULL;
	wc.lpszMenuName = NULL;
	wc.lpszClassName = "PS2EMU_ZEROGS";

	RegisterClassEx( &wc );

	if( conf.options & GSOPTION_FULLSCREEN)
	{
		dwExStyle = WS_EX_APPWINDOW;
		dwStyle = WS_POPUP;
	}
	else
	{
		dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
		dwStyle = WS_OVERLAPPEDWINDOW;
	}

	AdjustWindowRectEx(&rc, dwStyle, FALSE, dwExStyle);
	GetWindowRect(GetDesktopWindow(), &rcdesktop);

	GShwnd = CreateWindowEx(
		dwExStyle,
		"PS2EMU_ZEROGS",
		"ZeroGS",
		dwStyle,
		(rcdesktop.right - (rc.right - rc.left)) / 2,
		(rcdesktop.bottom - (rc.bottom - rc.top)) / 2,
		rc.right - rc.left,
		rc.bottom - rc.top,
		NULL,
		NULL,
		hInstance,
		NULL);

	if (GShwnd == NULL) return false;

	if (pDisplay != NULL) *(HWND*)pDisplay = GShwnd;
	
	return (pDisplay != NULL);
}

bool GLWindow::ReleaseWindow()
{       
	if (hRC)											// Do We Have A Rendering Context?
	{
		if (!wglMakeCurrent(NULL,NULL))				 // Are We Able To Release The DC And RC Contexts?
		{
			MessageBox(NULL,"Release Of DC And RC Failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		}

		if (!wglDeleteContext(hRC))					 // Are We Able To Delete The RC?
		{
			MessageBox(NULL,"Release Rendering Context Failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		}
		hRC=NULL;									   // Set RC To NULL
	}

	if (hDC && !ReleaseDC(GShwnd,hDC))				  // Are We Able To Release The DC
	{
		MessageBox(NULL,"Release Device Context Failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		hDC=NULL;									   // Set DC To NULL
	}

	return true;
}

void GLWindow::CloseWindow()
{       
	if( GShwnd != NULL ) 
	{
		DestroyWindow(GShwnd);
		GShwnd = NULL;
	}
}

bool GLWindow::DisplayWindow(int _width, int _height)
{
	GLuint	  PixelFormat;			// Holds The Results After Searching For A Match
	DWORD	   dwExStyle;			  // Window Extended Style
	DWORD	   dwStyle;				// Window Style

	RECT rcdesktop;
	GetWindowRect(GetDesktopWindow(), &rcdesktop);

	if( conf.options & GSOPTION_FULLSCREEN) {
		nBackbufferWidth = rcdesktop.right - rcdesktop.left;
		nBackbufferHeight = rcdesktop.bottom - rcdesktop.top;

		dwExStyle=WS_EX_APPWINDOW;
		dwStyle=WS_POPUP;
		ShowCursor(FALSE);
	}
	else {
		dwExStyle=WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
		dwStyle=WS_OVERLAPPEDWINDOW;
	}

	RECT rc;
	rc.left = 0; rc.top = 0;
	rc.right = nBackbufferWidth; rc.bottom = nBackbufferHeight;
	AdjustWindowRectEx(&rc, dwStyle, FALSE, dwExStyle);
	int X = (rcdesktop.right-rcdesktop.left)/2 - (rc.right-rc.left)/2;
	int Y = (rcdesktop.bottom-rcdesktop.top)/2 - (rc.bottom-rc.top)/2;

	SetWindowLong( GShwnd, GWL_STYLE, dwStyle );
	SetWindowLong( GShwnd, GWL_EXSTYLE, dwExStyle );

	SetWindowPos(GShwnd, HWND_TOP, X, Y, rc.right-rc.left, rc.bottom-rc.top, SWP_SHOWWINDOW);
		
	if (conf.options & GSOPTION_FULLSCREEN) {
		DEVMODE dmScreenSettings;
		memset(&dmScreenSettings,0,sizeof(dmScreenSettings));
		dmScreenSettings.dmSize=sizeof(dmScreenSettings);
		dmScreenSettings.dmPelsWidth	= nBackbufferWidth;
		dmScreenSettings.dmPelsHeight   = nBackbufferHeight;
		dmScreenSettings.dmBitsPerPel   = 32;
		dmScreenSettings.dmFields=DM_BITSPERPEL|DM_PELSWIDTH|DM_PELSHEIGHT;

		// Try To Set Selected Mode And Get Results.  NOTE: CDS_FULLSCREEN Gets Rid Of Start Bar.
		if (ChangeDisplaySettings(&dmScreenSettings,CDS_FULLSCREEN)!=DISP_CHANGE_SUCCESSFUL)
		{
			if (MessageBox(NULL,"The Requested Fullscreen Mode Is Not Supported By\nYour Video Card. Use Windowed Mode Instead?","NeHe GL",MB_YESNO|MB_ICONEXCLAMATION)==IDYES)
				conf.options &= ~GSOPTION_FULLSCREEN;
			else
				return false;
		}
	}
	else {
		// change to default resolution
		ChangeDisplaySettings(NULL, 0);
	}

	PIXELFORMATDESCRIPTOR pfd=			  // pfd Tells Windows How We Want Things To Be
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
	
	if (!(hDC=GetDC(GShwnd))) {
		MessageBox(NULL,"(1) Can't Create A GL Device Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return false;
	}
	
	if (!(PixelFormat=ChoosePixelFormat(hDC,&pfd))) {
		MessageBox(NULL,"(2) Can't Find A Suitable PixelFormat.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return false;
	}

	if(!SetPixelFormat(hDC,PixelFormat,&pfd)) {
		MessageBox(NULL,"(3) Can't Set The PixelFormat.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return false;
	}

	if (!(hRC=wglCreateContext(hDC))) {
		MessageBox(NULL,"(4) Can't Create A GL Rendering Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return false;
	}

	if(!wglMakeCurrent(hDC,hRC)) {
		MessageBox(NULL,"(5) Can't Activate The GL Rendering Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return false;
	}
	
	UpdateWindow(GShwnd);
	return true;
}

void GLWindow::SwapGLBuffers()
{
	static u32 lastswaptime = 0;
	SwapBuffers(hDC);
	lastswaptime = timeGetTime();
}
                
void GLWindow::SetTitle(char *strtitle)
{
	SetWindowText(GShwnd, strtitle);
}

void GLWindow::ResizeCheck()
{

}

#endif