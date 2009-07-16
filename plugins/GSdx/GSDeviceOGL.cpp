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
	, m_context(0)
	, m_topology(-1)
	, m_ps_ss(NULL)
	, m_scissor(0, 0, 0, 0)
	, m_viewport(0, 0)
	, m_dss(NULL)
	, m_bs(NULL)
	, m_bf(-1)
	, m_rt((GLuint)-1)
	, m_ds((GLuint)-1)
{
	m_vertices.stride = 0;
	m_vertices.start = 0;
	m_vertices.count = 0;
	m_vertices.limit = 0;
}

GSDeviceOGL::~GSDeviceOGL()
{
	if(m_context) cgDestroyContext(m_context);
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

	#endif

	if(glewInit() != GLEW_OK)
	{
		return false;
	}

	#ifdef _WINDOWS

	if(WGLEW_EXT_swap_control)
	{
		wglSwapIntervalEXT(vsync ? 1 : 0);
	}

	#endif

	const char* vendor = (const char*)glGetString(GL_VENDOR);
	const char* renderer = (const char*)glGetString(GL_RENDERER);
	const char* version = (const char*)glGetString(GL_VERSION);
	const char* exts = (const char*)glGetString(GL_EXTENSIONS);

	printf("%s, %s, OpenGL %s\n", vendor, renderer, version);

	const char* str = strstr(exts, "ARB_texture_non_power_of_two");

	glGenBuffers(1, &m_vbo); CheckError();
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo); CheckError();
	// TODO: setup layout?

	m_context = cgCreateContext();

	// cgGLSetDebugMode(CG_FALSE); CheckCgError();

	cgSetParameterSettingMode(m_context, CG_DEFERRED_PARAMETER_SETTING); CheckCgError();
/*
	struct {CGprofile vs, gs, ps;} m_profile;
	m_profile.vs = cgGLGetLatestProfile(CG_GL_VERTEX); CheckCgError();
	m_profile.gs = cgGLGetLatestProfile(CG_GL_GEOMETRY); CheckCgError();
	m_profile.ps = cgGLGetLatestProfile(CG_GL_FRAGMENT); CheckCgError();
*/
	GSVector4i r = wnd->GetClientRect();

	Reset(r.width(), r.height(), Windowed);

	return true;
}

bool GSDeviceOGL::Reset(int w, int h, int mode)
{
	if(!__super::Reset(w, h, mode))
		return false;

	glCullFace(GL_FRONT_AND_BACK); CheckError();
	glDisable(GL_LIGHTING); CheckError();
	glDisable(GL_ALPHA_TEST); CheckError();
	glEnable(GL_SCISSOR_TEST); CheckError();

	glMatrixMode(GL_PROJECTION); CheckError();
	glLoadIdentity(); CheckError();
	glMatrixMode(GL_MODELVIEW); CheckError();
	glLoadIdentity(); CheckError();

	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST); CheckError();

	// glViewport(0, 0, w, h);

	if(m_fbo) {glDeleteFramebuffersEXT(1, &m_fbo); m_fbo = 0;}

	glGenFramebuffers(1, &m_fbo); CheckError();

	if(m_fbo == 0) return false;

	glBindFramebuffer(GL_FRAMEBUFFER, m_fbo); CheckError();

	return true;
}

void GSDeviceOGL::Present(const GSVector4i& r, int shader, bool limit)
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0); CheckError();

	// TODO: m_current => backbuffer

	Flip(limit);
}

void GSDeviceOGL::Flip(bool limit)
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
	glDrawArrays(m_topology, m_vertices.count, m_vertices.start); CheckError();
}

void GSDeviceOGL::EndScene()
{
	m_vertices.start += m_vertices.count;
	m_vertices.count = 0;
}

void GSDeviceOGL::ClearRenderTarget(GSTexture* t, const GSVector4& c)
{
	GLuint texture = *(GSTextureOGL*)t;

	if(texture == 0)
	{
		// TODO: backbuffer
	}
	else
	{
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, texture); CheckError();
	}

	// TODO: disable scissor, color mask

    glClearColor(c.r, c.g, c.b, c.a); CheckError();
	glClear(GL_COLOR_BUFFER_BIT); CheckError();
}

void GSDeviceOGL::ClearRenderTarget(GSTexture* t, uint32 c)
{
	ClearRenderTarget(t, GSVector4(c) * (1.0f / 255));
}

void GSDeviceOGL::ClearDepth(GSTexture* t, float c)
{
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_DEPTH_COMPONENT, *(GSTextureOGL*)t); CheckError();

	// TODO: disable scissor, depth mask

    glClearDepth(c); CheckError();
	glClear(GL_DEPTH_BUFFER_BIT); CheckError();
}

void GSDeviceOGL::ClearStencil(GSTexture* t, uint8 c)
{
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_STENCIL_INDEX, *(GSTextureOGL*)t); CheckError();

	// TODO: disable scissor, depth (?) mask

	glClearStencil((GLint)c); CheckError();
	glClear(GL_STENCIL_BUFFER_BIT); CheckError();
}

GSTexture* GSDeviceOGL::Create(int type, int w, int h, int format)
{
	GLuint texture = 0;

	switch(type)
	{
	case GSTexture::RenderTarget:
		glGenTextures(1, &texture); CheckError();
		glBindTexture(GL_TEXTURE_2D, texture); CheckError();
		glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL); CheckError();
		break;
	case GSTexture::DepthStencil:
	    glGenRenderbuffers(1, &texture); CheckError();
        glBindRenderbuffer(GL_RENDERBUFFER, texture); CheckError();
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH32F_STENCIL8, w, h); CheckError();
		// TODO: depth textures?
		break;
	case GSTexture::Texture:
		glGenTextures(1, &texture); CheckError();
		glBindTexture(GL_TEXTURE_2D, texture); CheckError();
		glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL); CheckError();
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

void GSDeviceOGL::CopyRect(GSTexture* st, GSTexture* dt, const GSVector4i& r)
{
	// TODO
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

	glBindBuffer(GL_ARRAY_BUFFER, m_vbo); CheckError();

	bool growbuffer = false;
	bool discard = false; // in opengl 3.x this should be a flag to glMapBuffer, as I read somewhere

	if(count * stride > m_vertices.limit * m_vertices.stride)
	{
		m_vertices.start = 0;
		m_vertices.count = 0;
		m_vertices.limit = std::max<int>(count * 3 / 2, 10000);

		growbuffer = true;
	}

	if(m_vertices.start + count > m_vertices.limit || stride != m_vertices.stride)
	{
		m_vertices.start = 0;

		discard = true;
	}

	if(growbuffer || discard)
	{
		glBufferData(GL_ARRAY_BUFFER, m_vertices.limit * stride, NULL, GL_DYNAMIC_DRAW); CheckError(); // GL_STREAM_DRAW?
	}

	glBufferSubData(GL_ARRAY_BUFFER, m_vertices.start * stride, count * stride, vertices); CheckError();
/*
	if(GLvoid* v = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY))
	{
		GSVector4i::storent((uint8*)v + m_vertices.start * stride, vertices, count * stride);

		glUnmapBuffer(GL_ARRAY_BUFFER); CheckError();
	}
*/
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

void GSDeviceOGL::PSSetSamplerState(SamplerStateOGL* ss)
{
	if(ss && m_ps_ss != ss)
	{
		glActiveTexture(GL_TEXTURE0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, ss->wrap.s);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, ss->wrap.t);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, ss->filter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, ss->filter);
		
		glActiveTexture(GL_TEXTURE1);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, ss->wrap.s);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, ss->wrap.t);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, ss->filter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, ss->filter);
		
		glActiveTexture(GL_TEXTURE2);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_POINT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_POINT);
		
		glActiveTexture(GL_TEXTURE3);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_POINT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_POINT);
		
		m_ps_ss = ss;
	}
}

void GSDeviceOGL::RSSet(const GSVector2i& size, const GSVector4i* scissor)
{
	if(m_viewport != size)
	{
		glViewport(0, 0, size.x, size.y); CheckError();

		m_viewport = size;
	}

	GSVector4i r = scissor ? *scissor : GSVector4i(size).zwxy();

	if(!m_scissor.eq(r))
	{
		glScissor(r.left, r.top, r.width(), r.height()); CheckError();

		m_scissor = r;
	}
}

void GSDeviceOGL::OMSetDepthStencilState(DepthStencilStateOGL* dss)
{
	if(m_dss != dss)
	{
		if(dss->depth.enable)
		{
			glEnable(GL_DEPTH_TEST); CheckError();
			glDepthFunc(dss->depth.func); CheckError();
			glDepthMask(dss->depth.write); CheckError();
		}
		else
		{
			glDisable(GL_DEPTH_TEST); CheckError();
		}

		if(dss->stencil.enable)
		{
			glEnable(GL_STENCIL_TEST); CheckError();
			glStencilFunc(dss->stencil.func, dss->stencil.ref, dss->stencil.mask); CheckError();
			glStencilOp(dss->stencil.sfail, dss->stencil.dpfail, dss->stencil.dppass); CheckError();
			glStencilMask(dss->stencil.wmask); CheckError();
		}
		else
		{
			glDisable(GL_STENCIL_TEST); CheckError();
		}

		m_dss = dss;
	}
}

void GSDeviceOGL::OMSetBlendState(BlendStateOGL* bs, float bf)
{
	if(m_bs != bs || m_bf != bf)
	{
		if(bs->enable)
		{
			glEnable(GL_BLEND); CheckError();
			glBlendEquationSeparate(bs->modeRGB, bs->modeAlpha); CheckError();
			glBlendFuncSeparate(bs->srcRGB, bs->dstRGB, bs->srcAlpha, bs->dstAlpha); CheckError();
			glBlendColor(bf, bf, bf, 0); CheckError();
		}
		else
		{
			glDisable(GL_BLEND); CheckError();
		}
		
		glColorMask(bs->mask.r, bs->mask.g, bs->mask.b, bs->mask.a); CheckError();

		m_bs = bs;
		m_bf = bf;
	}
}

void GSDeviceOGL::OMSetRenderTargets(GSTexture* rt, GSTexture* ds)
{
	GLuint rti = 0;
	GLuint dsi = 0;

	if(rt) rti = *(GSTextureOGL*)rt;
	if(ds) dsi = *(GSTextureOGL*)ds;

	if(m_rt != rti)
	{
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rti); CheckError();

		m_rt = rti;
	}

	if(m_ds != dsi)
	{
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_DEPTH_COMPONENT, dsi); CheckError();

		m_ds = dsi;
	}
}
