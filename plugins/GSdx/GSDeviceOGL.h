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
#include "GSVertexArrayOGL.h"
#include "GSUniformBufferOGL.h"

class GSBlendStateOGL {
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
	bool   constant_factor;
	float  debug_factor;

public:

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
		, constant_factor(false)
	{}

	void SetRGB(GLenum op, GLenum src, GLenum dst)
	{
		m_equation_RGB = op;
		m_func_sRGB = src;
		m_func_dRGB = dst;
		if (IsConstant(src) || IsConstant(dst)) constant_factor = true;
	}

	void SetALPHA(GLenum op, GLenum src, GLenum dst)
	{
		m_equation_ALPHA = op;
		m_func_sALPHA = src;
		m_func_dALPHA = dst;
	}

	void SetMask(bool r, bool g, bool b, bool a) { m_r_msk = r; m_g_msk = g; m_b_msk = b; m_a_msk = a; }

	void RevertOp()
	{
		if(m_equation_RGB == GL_FUNC_ADD)
			m_equation_RGB = GL_FUNC_REVERSE_SUBTRACT;
		else if(m_equation_RGB == GL_FUNC_REVERSE_SUBTRACT)
			m_equation_RGB = GL_FUNC_ADD;
	}

	void EnableBlend() { m_enable = true;}

	bool IsConstant(GLenum factor) { return ((factor == GL_CONSTANT_COLOR) || (factor == GL_ONE_MINUS_CONSTANT_COLOR)); }

	bool HasConstantFactor() { return constant_factor; }

	void SetupColorMask()
	{
		glColorMask(m_r_msk, m_g_msk, m_b_msk, m_a_msk);
	}

	void SetupBlend(float factor)
	{
		SetupColorMask();

		if (m_enable) {
			glEnable(GL_BLEND);
			if (HasConstantFactor()) {
				debug_factor = factor;
				glBlendColor(factor, factor, factor, 0);
			}

			glBlendEquationSeparate(m_equation_RGB, m_equation_ALPHA);
			glBlendFuncSeparate(m_func_sRGB, m_func_dRGB, m_func_sALPHA, m_func_dALPHA);
		} else {
			glDisable(GL_BLEND);
		}
	}

	char* NameOfParam(GLenum p)
	{
		switch (p) {
			case GL_FUNC_ADD: return "ADD";
			case GL_FUNC_SUBTRACT: return "SUB";
			case GL_FUNC_REVERSE_SUBTRACT: return "REV SUB";
			case GL_ONE: return "ONE";
			case GL_ZERO: return "ZERO";
			case GL_SRC1_ALPHA: return "SRC1 ALPHA";
			case GL_SRC_ALPHA: return "SRC ALPHA";
			case GL_ONE_MINUS_DST_ALPHA: return "1 - DST ALPHA";
			case GL_DST_ALPHA: return "DST ALPHA";
			case GL_DST_COLOR:	return "DST COLOR";
			case GL_ONE_MINUS_SRC1_ALPHA: return "1 - SRC1 ALPHA";
			case GL_ONE_MINUS_SRC_ALPHA: return "1 - SRC ALPHA";
			case GL_CONSTANT_COLOR: return "CST";
			case GL_ONE_MINUS_CONSTANT_COLOR: return "1 - CST";
			default: return "UKN";
		}
		return "UKN";
	}

	void debug()
	{
		if (!m_enable) return;
		fprintf(stderr,"Blend op: %s; src:%s; dst:%s\n", NameOfParam(m_equation_RGB), NameOfParam(m_func_sRGB), NameOfParam(m_func_dRGB));
		if (HasConstantFactor()) fprintf(stderr, "Blend constant: %f\n", debug_factor);
		fprintf(stderr,"Mask. R:%d B:%d G:%d A:%d\n", m_r_msk, m_b_msk, m_g_msk, m_a_msk);
	}
};

class GSDepthStencilOGL {
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

	char* NameOfParam(GLenum p)
	{
		switch(p) {
			case GL_NEVER: return "NEVER";
			case GL_ALWAYS: return "ALWAYS";
			case GL_GEQUAL: return "GEQUAL";
			case GL_GREATER: return "GREATER";
			case GL_KEEP: return "KEEP";
			case GL_EQUAL: return "EQUAL";
			case GL_REPLACE: return "REPLACE";
			default: return "UKN";
		}
		return "UKN";
	}

public:

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

	void EnableDepth() { m_depth_enable = true; }
	void EnableStencil() { m_stencil_enable = true; }

	void SetDepth(GLenum func, GLboolean mask) { m_depth_func = func; m_depth_mask = mask; }
	void SetStencil(GLuint func, GLuint pass) { m_stencil_func = func; m_stencil_spass_dpass_op = pass; }

	void SetupDepth()
	{
		if (m_depth_enable) {
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(m_depth_func);
			glDepthMask(m_depth_mask);
		} else
			glDisable(GL_DEPTH_TEST);
	}

	void SetupStencil(uint8 sref)
	{
		uint ref = sref;
		if (m_stencil_enable) {
			glEnable(GL_STENCIL_TEST);
			glStencilFunc(m_stencil_func, ref, m_stencil_mask);
			glStencilOp(m_stencil_sfail_op, m_stencil_spass_dfail_op, m_stencil_spass_dpass_op);
		} else
			glDisable(GL_STENCIL_TEST);
	}

	void debug() { debug_depth(); debug_stencil(); }

	void debug_depth()
	{
		if (!m_depth_enable) return;
		fprintf(stderr, "Depth %s. Mask %x\n", NameOfParam(m_depth_func), m_depth_mask);
	}

	void debug_stencil()
	{
		if (!m_stencil_enable) return;
		fprintf(stderr, "Stencil %s. Both pass op %s\n", NameOfParam(m_stencil_func), NameOfParam(m_stencil_spass_dpass_op));
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
				uint32 wildhack:2;
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
				uint32 spritehack:1;
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
	GLuint m_fbo_read;			// frame buffer container only for reading

	GSVertexBufferStateOGL* m_vb;	  // vb_state for HW renderer
	GSVertexBufferStateOGL* m_vb_sr; // vb_state for StretchRect

	bool m_enable_shader_AMD_hack;

	struct {
		GLuint ps[2];				 // program object
		GSUniformBufferOGL* cb;		 // uniform buffer object
		GSBlendStateOGL* bs;
	} m_merge_obj;

	struct {
		GLuint ps[4];				// program object
		GSUniformBufferOGL* cb;		// uniform buffer object
	} m_interlace;

	struct {
		GLuint vs;		// program object
		GLuint ps[8];	// program object
		GLuint ln;		// sampler object
		GLuint pt;		// sampler object
		GLuint gs;
		GSDepthStencilOGL* dss;
		GSBlendStateOGL* bs;
	} m_convert;

	struct {
		GLuint ps;
		GSUniformBufferOGL *cb;
	} m_fxaa;

	struct {
		GSDepthStencilOGL* dss;
		GSBlendStateOGL* bs;
	} m_date;

	struct 
	{
		GLuint ps;
		GSUniformBufferOGL *cb;
	} m_shadeboost;



	struct {
		GSVertexBufferStateOGL* vb;
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
 		GLenum	   draw;
	} m_state;

	bool m_srv_changed;
	bool m_ss_changed;

	hash_map<uint32, GLuint > m_vs;
	hash_map<uint32, GLuint > m_gs;
	hash_map<uint32, GLuint > m_ps;
	hash_map<uint32, GLuint > m_ps_ss;
	hash_map<uint32, GSDepthStencilOGL* > m_om_dss;
	hash_map<uint32, GSBlendStateOGL* > m_om_bs;

	GLuint m_palette_ss;
	GLuint m_rt_ss;

	GSUniformBufferOGL* m_vs_cb;
	GSUniformBufferOGL* m_ps_cb;

	VSConstantBuffer m_vs_cb_cache;
	PSConstantBuffer m_ps_cb_cache;

	protected:
	GSTexture* CreateSurface(int type, int w, int h, bool msaa, int format);
	GSTexture* FetchSurface(int type, int w, int h, bool msaa, int format);
	void DoMerge(GSTexture* st[2], GSVector4* sr, GSTexture* dt, GSVector4* dr, bool slbg, bool mmod, const GSVector4& c);
	void DoInterlace(GSTexture* st, GSTexture* dt, int shader, bool linear, float yoffset = 0);
	void DoShadeBoost(GSTexture* st, GSTexture* dt);

	public:
	GSDeviceOGL();
	virtual ~GSDeviceOGL();

	void CheckDebugLog();
	static void DebugOutputToFile(unsigned int source, unsigned int type, unsigned int id, unsigned int severity, const char* message);
	void DebugOutput();
	void DebugInput();
	void DebugBB();

	bool HasStencil() { return true; }
	bool HasDepth32() { return true; }

	bool Create(GSWnd* wnd);
	bool Reset(int w, int h);
	void Flip();

	void DrawPrimitive();
	void DrawIndexedPrimitive();
	void DrawIndexedPrimitive(int offset, int count);

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
	bool IAMapVertexBuffer(void** vertex, size_t stride, size_t count);
	void IAUnmapVertexBuffer();
	void IASetIndexBuffer(const void* index, size_t count);
	void IASetVertexState(GSVertexBufferStateOGL* vb = NULL);

	void SetUniformBuffer(GSUniformBufferOGL* cb);

	void VSSetShader(GLuint vs);
	void GSSetShader(GLuint gs);

	void PSSetShaderResources(GSTexture* sr0, GSTexture* sr1);
	void PSSetShaderResource(int i, GSTexture* sr);
	void PSSetSamplerState(GLuint ss0, GLuint ss1, GLuint ss2 = 0);
	void PSSetShader(GLuint ps);

	void OMSetFBO(GLuint fbo, GLenum buffer = GL_COLOR_ATTACHMENT0);
	void OMSetDepthStencilState(GSDepthStencilOGL* dss, uint8 sref);
	void OMSetBlendState(GSBlendStateOGL* bs, float bf);
	void OMSetRenderTargets(GSTexture* rt, GSTexture* ds, const GSVector4i* scissor = NULL);


	void CreateTextureFX();
	void SetupIA(const void* vertex, int vertex_count, const uint32* index, int index_count, int prim);
	void SetupVS(VSSelector sel, const VSConstantBuffer* cb);
	void SetupGS(GSSelector sel);
	void SetupPS(PSSelector sel, const PSConstantBuffer* cb, PSSamplerSelector ssel);
	void SetupOM(OMDepthStencilSelector dssel, OMBlendSelector bsel, uint8 afix);

	GLuint glCreateShaderProgramv_AMD_BUG_WORKAROUND(GLenum  type,  GLsizei  count,  const char ** strings);
};
