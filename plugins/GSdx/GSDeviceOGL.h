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
	bool   m_r_msk;
	bool   m_b_msk;
	bool   m_g_msk;
	bool   m_a_msk;

	GSBlendStateOGL() : m_enable(false)
		, m_equation_RGB(0)
		, m_equation_ALPHA(GL_FUNC_ADD)
		, m_func_sRGB(0)
		, m_func_dRGB(0)
		, m_func_sALPHA(GL_ONE)
		, m_func_dALPHA(GL_ZERO)
		, m_r_msk(GL_TRUE)
		, m_b_msk(GL_TRUE)
		, m_g_msk(GL_TRUE)
		, m_a_msk(GL_TRUE)
	{}

};

struct GSDepthStencilOGL {
	bool m_depth_enable;
	GLenum m_depth_func;
	GLboolean m_depth_mask;
	// Note front face and back might be split but it seems they have same parameter configuration
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
		, m_stencil_mask(1)
		, m_stencil_func(0)
		, m_stencil_ref(0)
		, m_stencil_sfail_op(GL_KEEP)
		, m_stencil_spass_dfail_op(GL_KEEP)
		, m_stencil_spass_dpass_op(GL_KEEP)
	{}

};

class GSUniformBufferOGL {
	GLuint buffer;		// data object
	GLuint index;		// GLSL slot
	uint   size;	    // size of the data
	const GLenum target;

public:
	GSUniformBufferOGL(GLuint index, uint size) : index(index)
												  , size(size)
												  ,target(GL_UNIFORM_BUFFER)
	{
		glGenBuffers(1, &buffer);
		bind();
		allocate();
		attach();
	}

	void bind()
	{
		glBindBuffer(target, buffer);
	}

	void allocate()
	{
		glBufferData(target, size, NULL, GL_STREAM_DRAW);
	}

	void attach()
	{
		glBindBufferBase(target, index, buffer);
	}

	void upload(const void* src)
	{
		uint32 flags = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT;
		uint8* dst = (uint8*) glMapBufferRange(target, 0, size, flags);
		memcpy(dst, src, size);
		glUnmapBuffer(target);
	}

	~GSUniformBufferOGL() {
		glDeleteBuffers(1, &buffer);
	}
};

struct GSInputLayout {
	GLuint  index;
	GLint   size;
	GLenum  type;
	GLboolean normalize;
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
			switch (layout[i].type) {
				case GL_UNSIGNED_SHORT:
				case GL_UNSIGNED_INT:
					// Rule: when shader use integral (not normalized) you must use glVertexAttribIPointer (note the extra I)
					glVertexAttribIPointer(layout[i].index, layout[i].size, layout[i].type, layout[i].stride, layout[i].offset);
					break;
				default:
					glVertexAttribPointer(layout[i].index, layout[i].size, layout[i].type, layout[i].normalize,  layout[i].stride, layout[i].offset);
					break;
			}
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
	public:
	__aligned(struct, 32) VSConstantBuffer
	{
		GSVector4 VertexScale;
		GSVector4 VertexOffset;
		GSVector4 TextureScale;

		VSConstantBuffer()
		{
			VertexScale = GSVector4::zero();
			VertexOffset = GSVector4::zero();
			TextureScale = GSVector4::zero();
		}

		__forceinline bool Update(const VSConstantBuffer* cb)
		{
			GSVector4i* a = (GSVector4i*)this;
			GSVector4i* b = (GSVector4i*)cb;

			GSVector4i b0 = b[0];
			GSVector4i b1 = b[1];
			GSVector4i b2 = b[2];

			if(!((a[0] == b0) & (a[1] == b1) & (a[2] == b2)).alltrue())
			{
				a[0] = b0;
				a[1] = b1;
				a[2] = b2;

				return true;
			}

			return false;
		}
	};

	struct VSSelector
	{
		union
		{
			struct
			{
				uint32 bppz:2;
				uint32 tme:1;
				uint32 fst:1;
				uint32 logz:1;
				uint32 rtcopy:1;
			};

			uint32 key;
		};

		operator uint32() {return key & 0x3f;}

		VSSelector() : key(0) {}
	};

	__aligned(struct, 32) PSConstantBuffer
	{
		GSVector4 FogColor_AREF;
		GSVector4 HalfTexel;
		GSVector4 WH;
		GSVector4 MinMax;
		GSVector4 MinF_TA;
		GSVector4i MskFix;

		PSConstantBuffer()
		{
			FogColor_AREF = GSVector4::zero();
			HalfTexel = GSVector4::zero();
			WH = GSVector4::zero();
			MinMax = GSVector4::zero();
			MinF_TA = GSVector4::zero();
			MskFix = GSVector4i::zero();
		}

		__forceinline bool Update(const PSConstantBuffer* cb)
		{
			GSVector4i* a = (GSVector4i*)this;
			GSVector4i* b = (GSVector4i*)cb;

			GSVector4i b0 = b[0];
			GSVector4i b1 = b[1];
			GSVector4i b2 = b[2];
			GSVector4i b3 = b[3];
			GSVector4i b4 = b[4];
			GSVector4i b5 = b[5];

			if(!((a[0] == b0) /*& (a[1] == b1)*/ & (a[2] == b2) & (a[3] == b3) & (a[4] == b4) & (a[5] == b5)).alltrue()) // if WH matches HalfTexel does too
			{
				a[0] = b0;
				a[1] = b1;
				a[2] = b2;
				a[3] = b3;
				a[4] = b4;
				a[5] = b5;

				return true;
			}

			return false;
		}
	};

	struct GSSelector
	{
		union
		{
			struct
			{
				uint32 iip:1;
				uint32 prim:2;
			};

			uint32 key;
		};

		operator uint32() {return key & 0x7;}

		GSSelector() : key(0) {}
	};

	struct PSSelector
	{
		union
		{
			struct
			{
				uint32 fst:1;
				uint32 wms:2;
				uint32 wmt:2;
				uint32 fmt:3;
				uint32 aem:1;
				uint32 tfx:3;
				uint32 tcc:1;
				uint32 atst:3;
				uint32 fog:1;
				uint32 clr1:1;
				uint32 fba:1;
				uint32 aout:1;
				uint32 rt:1;
				uint32 ltf:1;
				uint32 colclip:2;
				uint32 date:2;
			};

			uint32 key;
		};

		operator uint32() {return key & 0x3ffffff;}

		PSSelector() : key(0) {}
	};

	struct PSSamplerSelector
	{
		union
		{
			struct
			{
				uint32 tau:1;
				uint32 tav:1;
				uint32 ltf:1;
			};

			uint32 key;
		};

		operator uint32() {return key & 0x7;}

		PSSamplerSelector() : key(0) {}
	};

	struct OMDepthStencilSelector
	{
		union
		{
			struct
			{
				uint32 ztst:2;
				uint32 zwe:1;
				uint32 date:1;
				uint32 fba:1;
			};

			uint32 key;
		};

		operator uint32() {return key & 0x1f;}

		OMDepthStencilSelector() : key(0) {}
	};

	struct OMBlendSelector
	{
		union
		{
			struct
			{
				uint32 abe:1;
				uint32 a:2;
				uint32 b:2;
				uint32 c:2;
				uint32 d:2;
				uint32 wr:1;
				uint32 wg:1;
				uint32 wb:1;
				uint32 wa:1;
				uint32 negative:1;
			};

			struct
			{
				uint32 _pad:1;
				uint32 abcd:8;
				uint32 wrgba:4;
			};

			uint32 key;
		};

		operator uint32() {return key & 0x3fff;}

		OMBlendSelector() : key(0) {}

		bool IsCLR1() const
		{
			return (key & 0x19f) == 0x93; // abe == 1 && a == 1 && b == 2 && d == 1
		}
	};

	struct D3D9Blend {int bogus, op, src, dst;};
	static const D3D9Blend m_blendMapD3D9[3*3*3*3];

	private:
	uint32 m_msaa;				// Level of Msaa

	bool m_free_window;			
	GSWnd* m_window;

	GLuint m_pipeline;			// pipeline to attach program shader
	GLuint m_fbo;				// frame buffer container

	GSVertexBufferState* m_vb;	  // vb_state for HW renderer
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
		GLuint ps;
		GSUniformBufferOGL *cb;
	} m_fxaa;

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


	// Shaders...

	CComPtr<ID3D11SamplerState> m_palette_ss;
	CComPtr<ID3D11SamplerState> m_rt_ss;

#endif
	// hash_map<uint32, GSVertexShader11 > m_vs;
	// hash_map<uint32, CComPtr<ID3D11GeometryShader> > m_gs;
	// hash_map<uint32, CComPtr<ID3D11PixelShader> > m_ps;
	// hash_map<uint32, CComPtr<ID3D11SamplerState> > m_ps_ss;
	// hash_map<uint32, CComPtr<ID3D11DepthStencilState> > m_om_dss;
	// hash_map<uint32, CComPtr<ID3D11BlendState> > m_om_bs;
	hash_map<uint32, GLuint > m_vs;
	hash_map<uint32, GLuint > m_gs;
	hash_map<uint32, GLuint > m_ps;
	hash_map<uint32, GLuint > m_ps_ss;
	hash_map<uint32, GSDepthStencilOGL* > m_om_dss;
	hash_map<uint32, GSBlendStateOGL* > m_om_bs;

	//CComPtr<ID3D11SamplerState> m_palette_ss;
	//CComPtr<ID3D11SamplerState> m_rt_ss;
	GLuint m_palette_ss;
	GLuint m_rt_ss;

	//CComPtr<ID3D11Buffer> m_vs_cb;
	//CComPtr<ID3D11Buffer> m_ps_cb;
	GSUniformBufferOGL* m_vs_cb;
	GSUniformBufferOGL* m_ps_cb;

	VSConstantBuffer m_vs_cb_cache;
	PSConstantBuffer m_ps_cb_cache;

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

	bool HasStencil() { return true; }
	bool HasDepth32() { return true; }

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

	void SetupDATE(GSTexture* rt, GSTexture* ds, const GSVertexPT1* vertices, bool datm);

	GSTexture* Resolve(GSTexture* t);

	void CompileShaderFromSource(const std::string& glsl_file, const std::string& entry, GLenum type, GLuint* program, const std::string& macro_sel = "");

	void EndScene();

	void IASetPrimitiveTopology(GLenum topology);
	void IASetVertexBuffer(const void* vertices, size_t count);
	void IASetVertexState(GSVertexBufferState* vb_state);

	void SetUniformBuffer(GSUniformBufferOGL* cb);

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


	void CreateTextureFX();
	void SetupIA(const void* vertices, int count, GLenum prim);
	void SetupVS(VSSelector sel, const VSConstantBuffer* cb);
	void SetupGS(GSSelector sel);
	void SetupPS(PSSelector sel, const PSConstantBuffer* cb, PSSamplerSelector ssel);
	void SetupOM(OMDepthStencilSelector dssel, OMBlendSelector bsel, uint8 afix);
};
