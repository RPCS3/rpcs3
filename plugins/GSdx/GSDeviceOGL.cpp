/* 
 *	Copyright (C) 2007-2009 Gabest
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
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "GSdx.h"
#include "GSDeviceOGL.h"
#include "resource.h"

GSDeviceOGL::GSDeviceOGL()
	: m_hDC(NULL)
	, m_hGLRC(NULL)
{
}

GSDeviceOGL::~GSDeviceOGL()
{
	#ifdef _WINDOWS

	if(m_hGLRC) {wglMakeCurrent(NULL, NULL); wglDeleteContext(m_hGLRC);}
	if(m_hDC) ReleaseDC((HWND)m_wnd->GetHandle(), m_hDC);

	#endif
}

void GSDeviceOGL::OnCgError(CGcontext ctx, CGerror err)
{
	printf("%s\n", cgGetErrorString(err));
	printf("%s\n", cgGetLastListing(ctx)); // ?
}

bool GSDeviceOGL::Create(GSWnd* wnd, bool vsync)
{
	if(!__super::Create(wnd, vsync))
	{
		return false;
	}

	cgSetErrorHandler(OnStaticCgError, this);

	#ifdef _WINDOWS

	PIXELFORMATDESCRIPTOR pfd;
	
	memset(&pfd, 0, sizeof(pfd));

	pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 32; // 24?
    pfd.cStencilBits = 8;
    pfd.iLayerType = PFD_MAIN_PLANE;

	m_hDC = GetDC((HWND)m_wnd->GetHandle());

	if(!m_hDC) return false;

	if(!SetPixelFormat(m_hDC, ChoosePixelFormat(m_hDC, &pfd), &pfd)) 
		return false;

	m_hGLRC = wglCreateContext(m_hDC);

	if(!m_hGLRC) return false;

	if(!wglMakeCurrent(m_hDC, m_hGLRC))
		return false;

	#endif

	// const char* exts = (const char*)glGetString(GL_EXTENSIONS);

	// TODO

	Reset(1, 1, true);

	//

	return true;
}

bool GSDeviceOGL::Reset(int w, int h, bool fs)
{
	if(!__super::Reset(w, h, fs))
		return false;

	m_backbuffer = new GSTextureOGL(0); // ???

	glCullFace(GL_FRONT_AND_BACK);
	glDisable(GL_LIGHTING);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_SCISSOR_TEST);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

	// glViewport(0, 0, ?, ?);

	return true;
}

void GSDeviceOGL::Flip()
{
	// TODO
}

void GSDeviceOGL::BeginScene()
{
	// TODO
}

void GSDeviceOGL::DrawPrimitive()
{
	// TODO
}

void GSDeviceOGL::EndScene()
{
	// TODO
}

void GSDeviceOGL::ClearRenderTarget(GSTexture* t, const GSVector4& c)
{
	// TODO
}

void GSDeviceOGL::ClearRenderTarget(GSTexture* t, uint32 c)
{
	// TODO
}

void GSDeviceOGL::ClearDepth(GSTexture* t, float c)
{
	// TODO
}

void GSDeviceOGL::ClearStencil(GSTexture* t, uint8 c)
{
	// TODO
}

GSTexture* GSDeviceOGL::Create(int type, int w, int h, int format)
{
	// TODO

	return NULL;
}

GSTexture* GSDeviceOGL::CreateRenderTarget(int w, int h, int format)
{
	return __super::CreateRenderTarget(w, h, format ? format : 0); // TODO
}

GSTexture* GSDeviceOGL::CreateDepthStencil(int w, int h, int format)
{
	return __super::CreateDepthStencil(w, h, format ? format : 0); // TODO
}

GSTexture* GSDeviceOGL::CreateTexture(int w, int h, int format)
{
	return __super::CreateTexture(w, h, format ? format : 0); // TODO
}

GSTexture* GSDeviceOGL::CreateOffscreen(int w, int h, int format)
{
	return __super::CreateOffscreen(w, h, format ? format : 0); // TODO
}

GSTexture* GSDeviceOGL::CopyOffscreen(GSTexture* src, const GSVector4& sr, int w, int h, int format)
{
	// TODO

	return NULL;
}

void GSDeviceOGL::StretchRect(GSTexture* st, const GSVector4& sr, GSTexture* dt, const GSVector4& dr, int shader, bool linear)
{
	// TODO
}

void GSDeviceOGL::DoMerge(GSTexture* st[2], GSVector4* sr, GSVector4* dr, GSTexture* dt, bool slbg, bool mmod, const GSVector4& c)
{
	// TODO
}

void GSDeviceOGL::DoInterlace(GSTexture* st, GSTexture* dt, int shader, bool linear, float yoffset)
{
	// TODO
}
