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

#ifndef GLWIN_H_INCLUDED
#define GLWIN_H_INCLUDED


#ifdef _WIN32
#define GL_WIN32_WINDOW
#else
#define GL_X11_WINDOW
#endif

#undef CreateWindow	// Undo Windows.h global namespace pollution

class GLWindow
{
	private:
#ifdef GL_X11_WINDOW
		Display *glDisplay;
		int glScreen;
		GLXContext context;
		XVisualInfo *vi;
		
		Window glWindow;
		XSetWindowAttributes attr;
		
		bool CreateVisual();
		void GetGLXVersion();
		void GetWindowSize();
        void UpdateGrabKey();
        void Force43Ratio();
#endif
		bool fullScreen, doubleBuffered;
		u32 width, height, depth;

	public:
		void SwapGLBuffers();
		bool ReleaseContext();

#ifdef GL_X11_WINDOW
        void ToggleFullscreen();
#endif
		
		bool CreateWindow(void *pDisplay);
		void CloseWindow();
		bool DisplayWindow(int _width, int _height);
		void SetTitle(char *strtitle);
		void ResizeCheck();
};


extern GLWindow GLWin;

#endif // GLWIN_H_INCLUDED
