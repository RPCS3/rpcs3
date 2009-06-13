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
	, m_vbo(0)
	, m_fbo(0)
	, m_topology(-1)
{
	m_vertices.stride = 0;
	m_vertices.start = 0;
	m_vertices.count = 0;
	m_vertices.limit = 0;
}

GSDeviceOGL::~GSDeviceOGL()
{
	if(m_vbo) glDeleteBuffers(1, &m_vbo);
	if(m_fbo) glDeleteFramebuffers(1, &m_fbo);
	
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
	{
		return false;
	}

	m_hGLRC = wglCreateContext(m_hDC);

	if(!m_hGLRC) return false;

	if(!wglMakeCurrent(m_hDC, m_hGLRC))
	{
		return false;
	}

	if(WGLEW_EXT_swap_control)
	{
		wglSwapIntervalEXT(vsync ? 1 : 0);
	}

	#endif

	if(glewInit() != GLEW_OK)
	{
		return false;
	}

	// const char* exts = (const char*)glGetString(GL_EXTENSIONS);

	glGenBuffers(1, &m_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	// TODO: setup layout?

	GSVector4i r = wnd->GetClientRect();

	Reset(r.width(), r.height(), false);

	return true;
}

bool GSDeviceOGL::Reset(int w, int h, bool fs)
{
	if(!__super::Reset(w, h, fs))
		return false;

	m_backbuffer = new GSTextureOGL(0, GSTexture::RenderTarget, w, h);

	glCullFace(GL_FRONT_AND_BACK);
	glDisable(GL_LIGHTING);
	glDisable(GL_ALPHA_TEST);
	glEnable(GL_SCISSOR_TEST);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

	// glViewport(0, 0, w, h);

	if(m_fbo) {glDeleteFramebuffersEXT(1, &m_fbo); m_fbo = 0;}

	glGenFramebuffers(1, &m_fbo);

	if(m_fbo == 0) return false;

	glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

	return true;
}

void GSDeviceOGL::Flip()
{
	#ifdef _WINDOWS

	SwapBuffers(m_hDC);

	#endif
}

void GSDeviceOGL::BeginScene()
{
}

void GSDeviceOGL::DrawPrimitive()
{
	glDrawArrays(m_topology, m_vertices.count, m_vertices.start);
}

void GSDeviceOGL::EndScene()
{
	m_vertices.start += m_vertices.count;
	m_vertices.count = 0;
}

void GSDeviceOGL::ClearRenderTarget(GSTexture* t, const GSVector4& c)
{
	// TODO: disable scissor, color mask

    glClearColor(c.r, c.g, c.b, c.a);
	glClear(GL_COLOR_BUFFER_BIT);
}

void GSDeviceOGL::ClearRenderTarget(GSTexture* t, uint32 c)
{
	ClearRenderTarget(t, GSVector4(c) * (1.0f / 255));
}

void GSDeviceOGL::ClearDepth(GSTexture* t, float c)
{
	// TODO: disable scissor, depth mask

    glClearDepth(c);
	glClear(GL_DEPTH_BUFFER_BIT);
}

void GSDeviceOGL::ClearStencil(GSTexture* t, uint8 c)
{
	// TODO: disable scissor, depth (?) mask

	glClearStencil((GLint)c);
	glClear(GL_STENCIL_BUFFER_BIT);
}

GSTexture* GSDeviceOGL::Create(int type, int w, int h, int format)
{
	GLuint texture = 0;

	switch(type)
	{
	case GSTexture::RenderTarget:
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		break;
	case GSTexture::DepthStencil:
	    glGenRenderbuffersEXT(1, &texture);
        glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, texture);
        glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH32F_STENCIL8, w, h);
		// TODO: depth textures?
		break;
	case GSTexture::Texture:
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		break;
	case GSTexture::Offscreen:
		// TODO: ???
		break;
	}

	if(!texture) return false;

	GSTextureOGL* t = new GSTextureOGL(texture, type, w, h, format);

	switch(type)
	{
	case GSTexture::RenderTarget:
		ClearRenderTarget(t, 0);
		break;
	case GSTexture::DepthStencil:
		ClearDepth(t, 0);
		break;
	}

	return t;
}

GSTexture* GSDeviceOGL::CreateRenderTarget(int w, int h, int format)
{
	return __super::CreateRenderTarget(w, h, format ? format : GL_RGBA8);
}

GSTexture* GSDeviceOGL::CreateDepthStencil(int w, int h, int format)
{
	return __super::CreateDepthStencil(w, h, format ? format : GL_DEPTH32F_STENCIL8); // TODO: GL_DEPTH24_STENCIL8_EXT, GL_DEPTH24_STENCIL8
}

GSTexture* GSDeviceOGL::CreateTexture(int w, int h, int format)
{
	return __super::CreateTexture(w, h, format ? format : GL_RGBA8);
}

GSTexture* GSDeviceOGL::CreateOffscreen(int w, int h, int format)
{
	return __super::CreateOffscreen(w, h, format ? format : GL_RGBA8);
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

void GSDeviceOGL::IASetVertexBuffer(const void* vertices, size_t stride, size_t count)
{
	ASSERT(m_vertices.count == 0);

	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

	if(count * stride > m_vertices.limit * m_vertices.stride)
	{
		m_vertices.start = 0;
		m_vertices.count = 0;
		m_vertices.limit = 0;
	}

	if(m_vertices.limit == 0)
	{
		m_vertices.limit = max(count * 3 / 2, 10000);

		glBufferData(GL_ARRAY_BUFFER, m_vertices.limit * stride, NULL, GL_DYNAMIC_DRAW); // GL_STREAM_DRAW?
	}

	if(m_vertices.start + count > m_vertices.limit || stride != m_vertices.stride)
	{
		m_vertices.start = 0; // TODO: how to discard and not overwrite previous data?
	}

	if(GLvoid* v = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY))
	{
		GSVector4i::storent((uint8*)v + m_vertices.start * stride, vertices, count * stride);

		glUnmapBuffer(GL_ARRAY_BUFFER);
	}

	m_vertices.count = count;
	m_vertices.stride = stride;
}

void GSDeviceOGL::IASetInputLayout()
{
	// TODO
}

void GSDeviceOGL::IASetPrimitiveTopology(int topology)
{
	m_topology = topology;
}

