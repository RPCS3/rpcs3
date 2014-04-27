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

#include "GS.h"
#include "GLWin.h"

#ifdef GL_WIN32_WINDOW

LRESULT WINAPI MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static int nWindowWidth = 0, nWindowHeight = 0;

	switch (msg)
	{
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;

		case WM_KEYDOWN:
//			switch(wParam) {
//				case VK_ESCAPE:
//					SendMessage(hWnd, WM_DESTROY, 0L, 0L);
//					break;
//			}
			break;

		case WM_SIZE:
			nWindowWidth = lParam & 0xffff;
			nWindowHeight = lParam >> 16;
			GLWin.UpdateWindowSize(nWindowWidth, nWindowHeight);
			break;

		case WM_SIZING:
			// if button is 0, then just released so can resize
			if (GetSystemMetrics(SM_SWAPBUTTON) ? !GetAsyncKeyState(VK_RBUTTON) : !GetAsyncKeyState(VK_LBUTTON))
			{
				SetDeviceSize(nWindowWidth, nWindowHeight);
			}
			break;

		case WM_SETCURSOR:
			SetCursor(NULL);
			break;
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

bool GLWindow::CreateWindow(void *pDisplay)
{
	RECT rc, rcdesktop;
	rc.left = 0;
	rc.top = 0;
	rc.right = conf.width;
	rc.bottom = conf.height;

	WNDCLASSEX wc;
	HINSTANCE hInstance = GetModuleHandle(NULL); // Grab An Instance For Our Window
	DWORD dwExStyle, dwStyle;

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style		= CS_HREDRAW | CS_VREDRAW | CS_OWNDC;		// Redraw On Move, And Own DC For Window
	wc.lpfnWndProc		= (WNDPROC) MsgProc;					// MsgProc Handles Messages
	wc.cbClsExtra		= 0;									// No Extra Window Data
	wc.cbWndExtra		= 0;									// No Extra Window Data
	wc.hInstance		= hInstance;							// Set The Instance
	wc.hIcon		= NULL;			
	wc.hIconSm		= NULL;										// Load The Default Icon
	wc.hCursor		= LoadCursor(NULL, IDC_ARROW);				// Load The Arrow Pointer
	wc.hbrBackground	= (HBRUSH)GetStockObject(BLACK_BRUSH);	// No Background Required For GL
	wc.lpszMenuName		= NULL;									// We Don't Want A Menu
	wc.lpszClassName	= L"PS2EMU_ZEROGS";						// Set The Class Name

	RegisterClassEx(&wc);

	if (conf.fullscreen())
	{
		dwExStyle = WS_EX_APPWINDOW;
		dwStyle = WS_POPUP;
	}
	else
	{
		dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
		dwStyle = WS_OVERLAPPEDWINDOW | WS_BORDER;
	}

	dwStyle |= WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
	AdjustWindowRectEx(&rc, dwStyle, false, dwExStyle);

	GetWindowRect(GetDesktopWindow(), &rcdesktop);

	NativeWindow = CreateWindowEx(	dwExStyle,				// Extended Style For The Window
					L"PS2EMU_ZEROGS",				// Class Name
					L"ZZOgl",					// Window Title
					dwStyle,				// Selected Window Style
					(rcdesktop.right - (rc.right - rc.left)) / 2,  // Window Position
					(rcdesktop.bottom - (rc.bottom - rc.top)) / 2, // Window Position
					rc.right - rc.left,	// Calculate Adjusted Window Width
					rc.bottom - rc.top,	// Calculate Adjusted Window Height
					NULL,					// No Parent Window
					NULL,					// No Menu
					hInstance,				// Instance
					NULL);					// Don't Pass Anything To WM_CREATE

	if (NativeWindow == NULL) 
	{
		ZZLog::Error_Log("Failed to create window. Exiting...");
		return false;
	}

	if (pDisplay != NULL) *(HWND*)pDisplay = NativeWindow;

	// set just in case
	SetWindowLongPtr(NativeWindow, GWLP_WNDPROC, (LPARAM)(WNDPROC)MsgProc);

	ShowWindow(NativeWindow, SW_SHOWDEFAULT);

	UpdateWindow(NativeWindow);

	SetFocus(NativeWindow);

	if (pDisplay == NULL) ZZLog::Error_Log("Failed to create window. Exiting...");
	return (pDisplay != NULL);
}

bool GLWindow::ReleaseContext()
{
	if (wglContext)											// Do We Have A Rendering Context?
	{
		if (!wglMakeCurrent(NULL, NULL))				 // Are We Able To Release The DC And RC Contexts?
		{
			MessageBox(NULL, L"Release Of DC And RC Failed.", L"SHUTDOWN ERROR", MB_OK | MB_ICONINFORMATION);
		}

		if (!wglDeleteContext(wglContext))					 // Are We Able To Delete The RC?
		{
			MessageBox(NULL, L"Release Rendering Context Failed.", L"SHUTDOWN ERROR", MB_OK | MB_ICONINFORMATION);
		}

		wglContext = NULL;									 // Set RC To NULL
	}

	CloseWGLDisplay();

	return true;
}

void GLWindow::CloseWindow()
{
	if (NativeWindow != NULL)
	{
		DestroyWindow(NativeWindow);
		NativeWindow = NULL;
	}
}

bool GLWindow::DisplayWindow(int _width, int _height)
{
	DWORD	   dwExStyle;			  // Window Extended Style
	DWORD	   dwStyle;				// Window Style

	RECT rcdesktop;
	GetWindowRect(GetDesktopWindow(), &rcdesktop);

	if (conf.fullscreen())
	{
		backbuffer.w = rcdesktop.right - rcdesktop.left;
		backbuffer.h = rcdesktop.bottom - rcdesktop.top;

		dwExStyle = WS_EX_APPWINDOW;
		dwStyle = WS_POPUP;
		ShowCursor(false);
	}
	else
	{
		dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
		dwStyle = WS_OVERLAPPEDWINDOW;
		backbuffer.w = _width;
		backbuffer.h = _height;
	}
	dwStyle |= WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

	RECT rc;

	rc.left = 0;
	rc.top = 0;
	rc.right = backbuffer.w;
	rc.bottom = backbuffer.h;
	AdjustWindowRectEx(&rc, dwStyle, false, dwExStyle);
	int X = (rcdesktop.right - rcdesktop.left) / 2 - (rc.right - rc.left) / 2;
	int Y = (rcdesktop.bottom - rcdesktop.top) / 2 - (rc.bottom - rc.top) / 2;

	SetWindowLong(NativeWindow, GWL_STYLE, dwStyle);
	SetWindowLong(NativeWindow, GWL_EXSTYLE, dwExStyle);

	SetWindowPos(NativeWindow, HWND_TOP, X, Y, rc.right - rc.left, rc.bottom - rc.top, SWP_SHOWWINDOW);

	if (conf.fullscreen())
	{
		DEVMODE dmScreenSettings;
		memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));
		dmScreenSettings.dmSize = sizeof(dmScreenSettings);
		dmScreenSettings.dmPelsWidth	= backbuffer.w;
		dmScreenSettings.dmPelsHeight   = backbuffer.h;
		dmScreenSettings.dmBitsPerPel   = 32;
		dmScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

		// Try To Set Selected Mode And Get Results.  NOTE: CDS_FULLSCREEN Gets Rid Of Start Bar.

		if (ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
		{
			if (MessageBox(NULL,
					L"The Requested Fullscreen Mode Is Not Supported By\nYour Video Card. Use Windowed Mode Instead?",
					L"NeHe GL",
					MB_YESNO | MB_ICONEXCLAMATION) == IDYES)
				conf.setFullscreen(false);
			else
				return false;
		}
	}
	else
	{
		// change to default resolution
		ChangeDisplaySettings(NULL, 0);
	}

	if (!OpenWGLDisplay()) return false;

	if (!CreateContextGL()) return false;

	UpdateWindow(NativeWindow);

	return true;
}

bool GLWindow::OpenWGLDisplay()
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

	if (!(NativeDisplay = GetDC(NativeWindow)))
	{
		MessageBox(NULL, L"(1) Can't Create A GL Device Context.", L"ERROR", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}

	if (!(PixelFormat = ChoosePixelFormat(NativeDisplay, &pfd)))
	{
		MessageBox(NULL, L"(2) Can't Find A Suitable PixelFormat.", L"ERROR", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}

	if (!SetPixelFormat(NativeDisplay, PixelFormat, &pfd))
	{
		MessageBox(NULL, L"(3) Can't Set The PixelFormat.", L"ERROR", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}

	return true;
}

void GLWindow::CloseWGLDisplay()
{
	if (NativeDisplay && !ReleaseDC(NativeWindow, NativeDisplay))				 // Are We Able To Release The DC
	{
		MessageBox(NULL, L"Release Device Context Failed.", L"SHUTDOWN ERROR", MB_OK | MB_ICONINFORMATION);
		NativeDisplay = NULL;									 // Set DC To NULL
	}
}

bool GLWindow::CreateContextGL()
{
	return CreateContextGL(2, 0);
}

bool GLWindow::CreateContextGL(int major, int minor)
{
	if (major <= 2) {
		if (!(wglContext = wglCreateContext(NativeDisplay)))
		{
			MessageBox(NULL, L"(4) Can't Create A GL Rendering Context.", L"ERROR", MB_OK | MB_ICONEXCLAMATION);
			return false;
		}

		if (!wglMakeCurrent(NativeDisplay, wglContext))
		{
			MessageBox(NULL, L"(5) Can't Activate The GL Rendering Context.", L"ERROR", MB_OK | MB_ICONEXCLAMATION);
			return false;
		}
	} else {
		// todo
		return false;
	}

	return true;
}

void GLWindow::SwapGLBuffers()
{
	static u32 lastswaptime = 0;

	if (glGetError() != GL_NO_ERROR) ZZLog::Debug_Log("glError before swap!");

	SwapBuffers(NativeDisplay);
	lastswaptime = timeGetTime();
}

void GLWindow::SetTitle(char *strtitle)
{
	if (!conf.fullscreen()) SetWindowText(NativeWindow, wxString::FromUTF8(strtitle));
}

extern void ChangeDeviceSize(int nNewWidth, int nNewHeight);

void GLWindow::ProcessEvents()
{
	MSG msg;

	ZeroMemory(&msg, sizeof(msg));

	while (1)
	{
		if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
		{
			switch (msg.message)
			{
				case WM_KEYDOWN :
					int my_KeyEvent = msg.wParam;
					bool my_bShift = !!(GetKeyState(VK_SHIFT) & 0x8000);

					switch (msg.wParam)
					{
						case VK_F5:
						case VK_F6:
						case VK_F7:
						case VK_F9:
							OnFKey(msg.wParam - VK_F1 + 1, my_bShift);
							break;

						case VK_ESCAPE:

							if (conf.fullscreen())
							{
								// destroy that msg
								conf.setFullscreen(false);
								ChangeDeviceSize(conf.width, conf.height);
								UpdateWindow(NativeWindow);
								continue; // so that msg doesn't get sent
							}
							else
							{
								SendMessage(NativeWindow, WM_DESTROY, 0, 0);
								return;
							}

							break;
					}

					break;
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			break;
		}
	}

	if ((GetKeyState(VK_MENU) & 0x8000) && (GetKeyState(VK_RETURN) & 0x8000))
	{
		conf.zz_options.fullscreen = !conf.zz_options.fullscreen;

		SetDeviceSize(
			(conf.fullscreen()) ? 1280 : conf.width,
			(conf.fullscreen()) ? 960 : conf.height);
	}
}

void* GLWindow::GetProcAddress(const char* function)
{
	return (void*)wglGetProcAddress(function);
}

void GLWindow::InitVsync(bool extension)
{
	vsync_supported = extension;
}

void GLWindow::SetVsync(bool enable)
{
	if (vsync_supported) {
		wglSwapIntervalEXT(0);
	}
}

#endif
