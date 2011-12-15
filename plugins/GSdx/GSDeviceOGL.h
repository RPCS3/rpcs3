/*
 *	Copyright (C) 2011-2011 Gregory hainaut
 *	Copyright (C) 2007-2009 Gabest
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

#include <fstream>
#include "GSDevice.h"
#include "GSTextureOGL.h"
#include "GSdx.h"

struct GSBlendStateOGL {
	// Note: You can also select the index of the draw buffer for which to set the blend setting
	// We will keep basic the first try
	bool   m_enable;
	GLenum m_equation_RGB;
	GLenum m_equation_ALPHA;
	GLenum m_func_sRGB;
	GLenum m_func_dRGB;
	GLenum m_func_sALPHA;
	GLenum m_func_dALPHA;

	GSBlendStateOGL() : m_enable(false)
		, m_equation_RGB(0)
		, m_equation_ALPHA(0)
		, m_func_sRGB(0)
		, m_func_dRGB(0)
		, m_func_sALPHA(0)
		, m_func_dALPHA(0)
	{}

};

struct GSDepthStencilOGL {
	bool m_depth_enable;
	GLenum m_depth_func;
	GLboolean m_depth_mask;
	// Note front face and back can be split might. But it seems they have same parameter configuration
	bool m_stencil_enable;
	GLuint m_stencil_mask;
	GLuint m_stencil_func;
	GLuint m_stencil_ref;
	GLuint m_stencil_sfail_op;
	GLuint m_stencil_spass_dfail_op;
	GLuint m_stencil_spass_dpass_op;

	GSDepthStencilOGL() : m_depth_enable(false)
		, m_depth_func(0)
		, m_depth_mask(0)
		, m_stencil_enable(false)
		, m_stencil_mask(0)
		, m_stencil_func(0)
		, m_stencil_ref(0)
		, m_stencil_sfail_op(0)
		, m_stencil_spass_dfail_op(0)
		, m_stencil_spass_dpass_op(0)
	{}
};

class GSUniformBufferOGL {
	GLuint buffer;		// data object
	GLuint index;		// GLSL slot
	uint   size;	    // size of the data

public:
	GSUniformBufferOGL(GLuint index, uint size) : index(index)
														, size(size)
	{
		glGenBuffers(1, &buffer);
		bind();
		allocate();
		attach();
	}

	void bind()
	{
		glBindBuffer(GL_UNIFORM_BUFFER, buffer);
	}

	void allocate()
	{
		glBufferData(GL_UNIFORM_BUFFER, size, NULL, GL_STREAM_DRAW);
	}

	void attach()
	{
		glBindBufferBase(GL_UNIFORM_BUFFER, index, buffer);
	}

	void upload(const void* src)
	{
		uint32 flags = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT;
		uint8* dst = (uint8*) glMapBufferRange(GL_UNIFORM_BUFFER, 0, size, flags);
		memcpy(dst, src, size);
		glUnmapBuffer(GL_UNIFORM_BUFFER);
	}

	~GSUniformBufferOGL() {
		glDeleteBuffers(1, &buffer);
	}
};

struct GSInputLayout {
	GLuint  index;
	GLint   size;
	GLenum  type;
	GLsizei stride;
	const GLvoid* offset;
};

struct GSVertexBufferState {
	size_t stride;
	size_t start;
	size_t count;
	size_t limit;
	GLuint vb;
	GLuint va;

	GSVertexBufferState(size_t stride, GSInputLayout* layout, uint32 layout_nbr) : stride(stride)
								  , count(0)
	{
		glGenBuffers(1, &vb);
		glGenVertexArrays(1, &va);
		bind();
		allocate(60000); // Opengl works best with 1-4MB buffer. 60k element seems a good value.
		set_internal_format(layout, layout_nbr);
	}

	void allocate(size_t new_limit)
	{
		start = 0;
		limit = new_limit;
		glBufferData(GL_ARRAY_BUFFER,  limit * stride, NULL, GL_STREAM_DRAW);
	}

	void bind()
	{
		glBindVertexArray(va);
		glBindBuffer(GL_ARRAY_BUFFER, vb);
	}

	void upload(const void* src, uint32 flags)
	{
		uint8* dst = (uint8*) glMapBufferRange(GL_ARRAY_BUFFER, stride*start, stride*count, flags);
		memcpy(dst, src, stride*count);
		glUnmapBuffer(GL_ARRAY_BUFFER);
	}

	void set_internal_format(GSInputLayout* layout, uint32 layout_nbr)
	{
		for (int i = 0; i < layout_nbr; i++) {
			// Note this function need both a vertex array object and a GL_ARRAY_BUFFER buffer
			glEnableVertexAttribArray(layout[i].index);
			glVertexAttribPointer(layout[i].index, layout[i].size, layout[i].type, GL_FALSE,  layout[i].stride, layout[i].offset);
		}
	}

	~GSVertexBufferState()
	{
		glDeleteBuffers(1, &vb);
		glDeleteVertexArrays(1, &va);
	}
};

class GSDeviceOGL : public GSDevice
{
	uint32 m_msaa;				// Level of Msaa

	bool m_free_window;			
	GSWnd* m_window;

	GLuint m_pipeline;			// pipeline to attach program shader
	GLuint m_fbo;				// frame buffer container

	GSVertexBufferState* m_vb_sr; // vb_state for StretchRect

	struct {
		GLuint ps[2];				 // program object
		GSUniformBufferOGL* cb;		 // uniform buffer object
		GSBlendStateOGL* bs;
	} m_merge;

	struct {
		GLuint ps[4];				// program object
		GSUniformBufferOGL* cb;		// uniform buffer object
	} m_interlace;

	struct {
		GLuint vs;		// program object
		GLuint ps[8];	// program object
		GLuint ln;		// sampler object
		GLuint pt;		// sampler object
		GSDepthStencilOGL* dss;
		GSBlendStateOGL* bs;
	} m_convert;

	struct
	{
		GSDepthStencilOGL* dss;
		GSBlendStateOGL* bs;
	} m_date;

	struct
	{
		GSVertexBufferState* vb_state;
		GLenum topology; // (ie GL_TRIANGLES...)
		GLuint vs; // program
		GSUniformBufferOGL* cb; // uniform current buffer
		GLuint gs; // program
		// FIXME texture binding. Maybe not equivalent for the state but the best I could find.
		GSTextureOGL* ps_srv[3];
		// ID3D11ShaderResourceView* ps_srv[3];
		GLuint ps; // program
		GLuint ps_ss[3]; // sampler
		GSVector2i viewport;
		GSVector4i scissor;
		GSDepthStencilOGL* dss;
		uint8 sref;
		GSBlendStateOGL* bs;
		float bf;
		// FIXME texture attachment in the FBO
		// ID3D11RenderTargetView* rtv;
		// ID3D11DepthStencilView* dsv;
		GSTextureOGL* rtv;
		GSTextureOGL* dsv;
		GLuint	   fbo;
	} m_state;

	bool m_srv_changed;
	bool m_ss_changed;

#if 0
	CComPtr<ID3D11Device> m_dev;
	CComPtr<ID3D11DeviceContext> m_ctx;
	CComPtr<IDXGISwapChain> m_swapchain;

	CComPtr<ID3D11RasterizerState> m_rs;

	struct 
	{
		CComPtr<ID3D11PixelShader> ps;
		CComPtr<ID3D11Buffer> cb;
	} m_fxaa;


	// Shaders...

	hash_map<uint32, GSVertexShader11 > m_vs;
	CComPtr<ID3D11Buffer> m_vs_cb;
	hash_map<uint32, CComPtr<ID3D11GeometryShader> > m_gs;
	hash_map<uint32, CComPtr<ID3D11PixelShader> > m_ps;
	CComPtr<ID3D11Buffer> m_ps_cb;
	hash_map<uint32, CComPtr<ID3D11SamplerState> > m_ps_ss;
	CComPtr<ID3D11SamplerState> m_palette_ss;
	CComPtr<ID3D11SamplerState> m_rt_ss;
	hash_map<uint32, CComPtr<ID3D11DepthStencilState> > m_om_dss;
	hash_map<uint32, CComPtr<ID3D11BlendState> > m_om_bs;

	VSConstantBuffer m_vs_cb_cache;
	PSConstantBuffer m_ps_cb_cache;
#endif

	protected:
		GSTexture* CreateSurface(int type, int w, int h, bool msaa, int format);
		GSTexture* FetchSurface(int type, int w, int h, bool msaa, int format);
		void DoMerge(GSTexture* st[2], GSVector4* sr, GSTexture* dt, GSVector4* dr, bool slbg, bool mmod, const GSVector4& c);
		void DoInterlace(GSTexture* st, GSTexture* dt, int shader, bool linear, float yoffset = 0);

	public:
		GSDeviceOGL();
		virtual ~GSDeviceOGL();

		void CheckDebugLog();
		static void DebugOutputToFile(unsigned int source, unsigned int type, unsigned int id, unsigned int severity, const char* message);


		bool Create(GSWnd* wnd);
		bool Reset(int w, int h);
		void Flip();

		void DrawPrimitive();

		void ClearRenderTarget(GSTexture* t, const GSVector4& c);
		void ClearRenderTarget(GSTexture* t, uint32 c);
		void ClearDepth(GSTexture* t, float c);
		void ClearStencil(GSTexture* t, uint8 c);

		GSTexture* CreateRenderTarget(int w, int h, bool msaa, int format = 0);
		GSTexture* CreateDepthStencil(int w, int h, bool msaa, int format = 0);
		GSTexture* CreateTexture(int w, int h, int format = 0);
		GSTexture* CreateOffscreen(int w, int h, int format = 0);

		GSTexture* CopyOffscreen(GSTexture* src, const GSVector4& sr, int w, int h, int format = 0);

		void CopyRect(GSTexture* st, GSTexture* dt, const GSVector4i& r);
		void StretchRect(GSTexture* st, const GSVector4& sr, GSTexture* dt, const GSVector4& dr, int shader = 0, bool linear = true);
		void StretchRect(GSTexture* st, const GSVector4& sr, GSTexture* dt, const GSVector4& dr, GLuint ps, bool linear = true);
		void StretchRect(GSTexture* st, const GSVector4& sr, GSTexture* dt, const GSVector4& dr, GLuint ps, GSBlendStateOGL* bs, bool linear = true);

		GSTexture* Resolve(GSTexture* t);

		void CompileShaderFromSource(const std::string& glsl_file, const std::string& entry, GLenum type, GLuint* program);

		void EndScene();

		void IASetPrimitiveTopology(GLenum topology);
		void IASetVertexBuffer(const void* vertices, size_t count);
		void IASetVertexState(GSVertexBufferState* vb_state);

		void VSSetShader(GLuint vs);
		void GSSetShader(GLuint gs);

		void PSSetShaderResources(GSTexture* sr0, GSTexture* sr1);
		void PSSetShaderResource(int i, GSTexture* sr);
		void PSSetSamplerState(GLuint ss0, GLuint ss1, GLuint ss2 = 0);
		void PSSetShader(GLuint ps);

		void OMSetFBO(GLuint fbo);
		void OMSetDepthStencilState(GSDepthStencilOGL* dss, uint8 sref);
		void OMSetBlendState(GSBlendStateOGL* bs, float bf);
		void OMSetRenderTargets(GSTexture* rt, GSTexture* ds, const GSVector4i* scissor = NULL);


};
