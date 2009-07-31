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

#pragma once

#include "GSDevice.h"
#include "GSTextureOGL.h"

struct SamplerStateOGL
{
	struct {GLenum s, t;} wrap;
	GLenum filter;
};

struct DepthStencilStateOGL
{
	struct
	{
		bool enable;
		bool write;
		GLenum func;
	} depth;

	struct
	{
		bool enable;
		GLenum func;
		GLint ref;
		GLuint mask;
		GLenum sfail;
		GLenum dpfail;
		GLenum dppass;
		GLuint wmask;
	} stencil;
};

struct BlendStateOGL
{
	bool enable;
	GLenum srcRGB;
	GLenum dstRGB;
	GLenum srcAlpha;
	GLenum dstAlpha;
	GLenum modeRGB;
	GLenum modeAlpha;
	union {uint8 r:1, g:1, b:1, a:1;} mask;
};

class GSDeviceOGL : public GSDevice
{
	#ifdef _WINDOWS

	HDC m_hDC;
	HGLRC m_hGLRC;
	
	#endif

	GLuint m_vbo;
	GLuint m_fbo;

	struct
	{
		size_t stride, start, count, limit;
	} m_vertices;

	int m_topology;
	SamplerStateOGL* m_ps_ss;
	GSVector4i m_scissor;
	GSVector2i m_viewport;
	DepthStencilStateOGL* m_dss;
	BlendStateOGL* m_bs;
	float m_bf;
	GLuint m_rt;
	GLuint m_ds;

	//

	CGcontext m_context;

	static void OnStaticCgError(CGcontext ctx, CGerror err, void* p) {((GSDeviceOGL*)p)->OnCgError(ctx, err);}
	void OnCgError(CGcontext ctx, CGerror err);

	//

	GSTexture* Create(int type, int w, int h, int format);

	void DoMerge(GSTexture* st[2], GSVector4* sr, GSVector4* dr, GSTexture* dt, bool slbg, bool mmod, const GSVector4& c);
	void DoInterlace(GSTexture* st, GSTexture* dt, int shader, bool linear, float yoffset = 0);

public:
	GSDeviceOGL();
	virtual ~GSDeviceOGL();

	bool Create(GSWnd* wnd, bool vsync);
	bool Reset(int w, int h, int mode);
	void Present(const GSVector4i& r, int shader, bool limit);
	void Flip(bool limit);

	void DrawPrimitive();

	void ClearRenderTarget(GSTexture* t, const GSVector4& c);
	void ClearRenderTarget(GSTexture* t, uint32 c);
	void ClearDepth(GSTexture* t, float c);
	void ClearStencil(GSTexture* t, uint8 c);

	GSTexture* CreateRenderTarget(int w, int h, int format = 0);
	GSTexture* CreateDepthStencil(int w, int h, int format = 0);
	GSTexture* CreateTexture(int w, int h, int format = 0);
	GSTexture* CreateOffscreen(int w, int h, int format = 0);

	GSTexture* CopyOffscreen(GSTexture* src, const GSVector4& sr, int w, int h, int format = 0);

	void CopyRect(GSTexture* st, GSTexture* dt, const GSVector4i& r);

	void StretchRect(GSTexture* st, const GSVector4& sr, GSTexture* dt, const GSVector4& dr, int shader = 0, bool linear = true);

	void IASetVertexBuffer(const void* vertices, size_t stride, size_t count);
	void IASetInputLayout(); // TODO
	void IASetPrimitiveTopology(int topology);

	void PSSetShaderResources(GSTexture* sr0, GSTexture* sr1);
	void PSSetSamplerState(SamplerStateOGL* ss);
	void OMSetDepthStencilState(DepthStencilStateOGL* dss);
	void OMSetBlendState(BlendStateOGL* bs, float bf);
	void OMSetRenderTargets(GSTexture* rt, GSTexture* ds, const GSVector4i* scissor = NULL);

	static void CheckError() 
	{
		#ifdef _DEBUG

		GLenum error = glGetError(); 
		
		if(error != GL_NO_ERROR)
		{
			printf("%s\n", gluErrorString(error));
		}

		#endif
	}

	static void CheckFrameBuffer()
	{
		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		
		if(status != GL_FRAMEBUFFER_COMPLETE)
		{
			printf("%d\n", status);
		}
	}

	void CheckCgError()
	{
		CGerror error;

		const char* s = cgGetLastErrorString(&error);

		if(error != CG_NO_ERROR)
		{
			printf("%s\n", s);

			if(error == CG_COMPILER_ERROR)
			{
				printf("%s\n", cgGetLastListing(m_context));
			}
		}
	}
};
