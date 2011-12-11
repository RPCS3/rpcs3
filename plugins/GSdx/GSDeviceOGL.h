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

struct GSUniformBufferOGL {
	GLuint buffer;		// data object
	GLuint index;		// GLSL slot
	uint   byte_size;	// size of the data

	GSUniformBufferOGL(GLuint index, uint byte_size) : buffer(0)
														, index(index)
														, byte_size(byte_size)
	{}

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

class GSDeviceOGL : public GSDevice
{
	uint32 m_msaa;				// Level of Msaa

	bool m_free_window;			
	GSWnd* m_window;

	GLuint m_vb;				// vertex buffer object
	GLuint m_pipeline;			// pipeline to attach program shader
	GLuint m_fbo;				// frame buffer container
	uint32 m_sr_vb_offset;

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
		// Hum I think this one is useless. As far as I understand
		// it only get the index name of GLSL-equivalent input attribut 
		// ??? CComPtr<ID3D11InputLayout> il;
		//GSInputLayout il[2]; // description of the vertex array
		GLuint va;		// vertex array object
		GLuint vb;		// vertex buffer
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

	// struct
	// {
	// 	ID3D11Buffer* vb;
	// 	size_t vb_stride;
	// 	ID3D11InputLayout* layout;
	// 	D3D11_PRIMITIVE_TOPOLOGY topology;
	// 	ID3D11VertexShader* vs;
	// 	ID3D11Buffer* vs_cb;
	// 	ID3D11GeometryShader* gs;
	// 	ID3D11ShaderResourceView* ps_srv[3];
	// 	ID3D11PixelShader* ps;
	// 	ID3D11Buffer* ps_cb;
	// 	ID3D11SamplerState* ps_ss[3];
	// 	GSVector2i viewport;
	// 	GSVector4i scissor;
	// 	ID3D11DepthStencilState* dss;
	// 	uint8 sref;
	// 	ID3D11BlendState* bs;
	// 	float bf;
	// 	ID3D11RenderTargetView* rtv;
	// 	ID3D11DepthStencilView* dsv;
	// } m_state;
	struct
	{
		GLuint vb;  // vertex buffer
		// Hum I think those things can be dropped on OGL. It probably need an others architecture (see glVertexAttribPointer)
		// size_t vb_stride;
		// ID3D11InputLayout* layout;
		//GSInputLayout* layout;
		//uint32 layout_nbr;
		GLuint va;  // vertex array
		GLenum topology; // (ie GL_TRIANGLES...)
		GLuint vs; // program
		GLuint cb; // uniform current buffer
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
	//bool m_vb_changed;

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

		void IASetPrimitiveTopology(GLenum topology);
		//void IASetInputLayout(GSInputLayout* layout, int layout_nbr);
		void IASetVertexBuffer(const void* vertices, size_t stride, size_t count);
		void IASetVertexBufferBind(GLuint vb);
		void IASetVertexArrray(GLuint va);

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
