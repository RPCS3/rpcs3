/*
 *	Copyright (C) 2011-2013 Gregory hainaut
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
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once

#include "GSDevice.h"
#include "GSTextureOGL.h"
#include "GSdx.h"
#include "GSVertexArrayOGL.h"
#include "GSUniformBufferOGL.h"
#include "GSShaderOGL.h"
#include "GLState.h"

#ifdef ENABLE_OGL_DEBUG_MEM_BW
extern uint32 g_texture_upload_byte;
extern uint32 g_vertex_upload_byte;
#endif

class GSBlendStateOGL {
	// Note: You can also select the index of the draw buffer for which to set the blend setting
	// We will keep basic the first try
	bool   m_enable;
	GLenum m_equation_RGB;
	GLenum m_equation_A;
	GLenum m_func_sRGB;
	GLenum m_func_dRGB;
	GLenum m_func_sA;
	GLenum m_func_dA;
	bool   m_r_msk;
	bool   m_b_msk;
	bool   m_g_msk;
	bool   m_a_msk;
	bool   constant_factor;

public:

	GSBlendStateOGL() : m_enable(false)
		, m_equation_RGB(0)
		, m_equation_A(GL_FUNC_ADD)
		, m_func_sRGB(0)
		, m_func_dRGB(0)
		, m_func_sA(GL_ONE)
		, m_func_dA(GL_ZERO)
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
		m_equation_A = op;
		m_func_sA = src;
		m_func_dA = dst;
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
		// FIXME align then SSE
		if (GLState::r_msk != m_r_msk || GLState::g_msk != m_g_msk || GLState::b_msk != m_b_msk || GLState::a_msk != m_a_msk) {
			GLState::r_msk = m_r_msk;
			GLState::g_msk = m_g_msk;
			GLState::b_msk = m_b_msk;
			GLState::a_msk = m_a_msk;

#ifdef ENABLE_GLES
			gl_ColorMask(m_r_msk, m_g_msk, m_b_msk, m_a_msk);
#else
			gl_ColorMaski(0, m_r_msk, m_g_msk, m_b_msk, m_a_msk);
#endif
		}
	}

	void SetupBlend(float factor)
	{
		SetupColorMask();

		if (GLState::blend != m_enable) {
			GLState::blend = m_enable;
			if (m_enable)
				glEnable(GL_BLEND);
			else
				glDisable(GL_BLEND);
		}

		if (m_enable) {
			if (HasConstantFactor()) {
				if (GLState::bf != factor) {
					GLState::bf = factor;
					gl_BlendColor(factor, factor, factor, 0);
				}
			}

			if (GLState::eq_RGB != m_equation_RGB || GLState::eq_A != m_equation_A) {
				GLState::eq_RGB = m_equation_RGB;
				GLState::eq_A   = m_equation_A;
#ifdef ENABLE_GLES
				gl_BlendEquationSeparate(m_equation_RGB, m_equation_A);
#else
				gl_BlendEquationSeparateiARB(0, m_equation_RGB, m_equation_A);
#endif
			}
			// FIXME align then SSE
			if (GLState::f_sRGB != m_func_sRGB || GLState::f_dRGB != m_func_dRGB || GLState::f_sA != m_func_sA || GLState::f_dA != m_func_dA) {
				GLState::f_sRGB = m_func_sRGB;
				GLState::f_dRGB = m_func_dRGB;
				GLState::f_sA = m_func_sA;
				GLState::f_dA = m_func_dA;
#ifdef ENABLE_GLES
				gl_BlendFuncSeparate(m_func_sRGB, m_func_dRGB, m_func_sA, m_func_dA);
#else
				gl_BlendFuncSeparateiARB(0, m_func_sRGB, m_func_dRGB, m_func_sA, m_func_dA);
#endif
			}
		}
	}
};

class GSDepthStencilOGL {
	bool m_depth_enable;
	GLenum m_depth_func;
	bool m_depth_mask;
	// Note front face and back might be split but it seems they have same parameter configuration
	bool m_stencil_enable;
	GLenum m_stencil_func;
	GLenum m_stencil_spass_dpass_op;

public:

	GSDepthStencilOGL() : m_depth_enable(false)
		, m_depth_func(0)
		, m_depth_mask(0)
		, m_stencil_enable(false)
		, m_stencil_func(0)
		, m_stencil_spass_dpass_op(GL_KEEP)
	{
		// Only needed once since m_stencil_mask is constant
		// Control which stencil bitplane are written
		glStencilMask(1);
	}

	void EnableDepth() { m_depth_enable = true; }
	void EnableStencil() { m_stencil_enable = true; }

	void SetDepth(GLenum func, bool mask) { m_depth_func = func; m_depth_mask = mask; }
	void SetStencil(GLenum func, GLenum pass) { m_stencil_func = func; m_stencil_spass_dpass_op = pass; }

	void SetupDepth()
	{
		if (GLState::depth != m_depth_enable) {
			GLState::depth = m_depth_enable;
			if (m_depth_enable)
				glEnable(GL_DEPTH_TEST);
			else
				glDisable(GL_DEPTH_TEST);
		}

		if (m_depth_enable) {
			if (GLState::depth_func != m_depth_func) {
				GLState::depth_func = m_depth_func;
				glDepthFunc(m_depth_func);
			}
			if (GLState::depth_mask != m_depth_mask) {
				GLState::depth_mask = m_depth_mask;
				glDepthMask((GLboolean)m_depth_mask);
			}
		}
	}

	void SetupStencil()
	{
		if (GLState::stencil != m_stencil_enable) {
			GLState::stencil = m_stencil_enable;
			if (m_stencil_enable)
				glEnable(GL_STENCIL_TEST);
			else
				glDisable(GL_STENCIL_TEST);
		}

		if (m_stencil_enable) {
			// Note: here the mask control which bitplane is considered by the operation
			if (GLState::stencil_func != m_stencil_func) {
				GLState::stencil_func = m_stencil_func;
				glStencilFunc(m_stencil_func, 1, 1);
			}
			if (GLState::stencil_pass != m_stencil_spass_dpass_op) {
				GLState::stencil_pass = m_stencil_spass_dpass_op;
				glStencilOp(GL_KEEP, GL_KEEP, m_stencil_spass_dpass_op);
			}
		}
	}

	bool IsMaskEnable() { return m_depth_mask != GL_FALSE; }
};

class GSDeviceOGL : public GSDevice
{
	public:
	__aligned(struct, 32) VSConstantBuffer
	{
		GSVector4 Vertex_Scale_Offset;
		GSVector4 TextureScale;

		VSConstantBuffer()
		{
			Vertex_Scale_Offset = GSVector4::zero();
			TextureScale = GSVector4::zero();
		}

		__forceinline bool Update(const VSConstantBuffer* cb)
		{
			GSVector4i* a = (GSVector4i*)this;
			GSVector4i* b = (GSVector4i*)cb;

			GSVector4i b0 = b[0];
			GSVector4i b1 = b[1];

			if(!((a[0] == b0) & (a[1] == b1)).alltrue())
			{
				a[0] = b0;
				a[1] = b1;

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
				uint32 logz:1;
				// Next param will be handle by subroutine
				uint32 tme:1;
				uint32 fst:1;
			};

			uint32 key;
		};

		operator uint32() {return key & 0x3f;}

		VSSelector() : key(0) {}
		VSSelector(uint32 k) : key(k) {}

		static uint32 size() { return 1 << 5; }
	};

	__aligned(struct, 32) PSConstantBuffer
	{
		GSVector4 FogColor_AREF;
		GSVector4 HalfTexel;
		GSVector4 WH;
		GSVector4 MinMax;
		GSVector4 MinF_TA;
		GSVector4i MskFix;

		GSVector4 TC_OffsetHack;

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

			// if WH matches both HalfTexel and TC_OffsetHack do too
			if(!((a[0] == b0) & (a[2] == b2) & (a[3] == b3) & (a[4] == b4) & (a[5] == b5)).alltrue())
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

	struct PSSelector
	{
		union
		{
			struct
			{
				uint32 fst:1;
				uint32 fmt:3;
				uint32 aem:1;
				uint32 fog:1;
				uint32 clr1:1;
				uint32 fba:1;
				uint32 aout:1;
				uint32 date:2;
				uint32 spritehack:1;
				uint32 tcoffsethack:1;
				uint32 point_sampler:1;
				uint32 iip:1;
				// Next param will be handle by subroutine
				uint32 colclip:2;
				uint32 atst:3;

				uint32 tfx:3;
				uint32 tcc:1;
				uint32 wms:2;
				uint32 wmt:2;
				uint32 ltf:1;
			};

			uint32 key;
		};

		operator uint32() {return key & 0x1fffffff;}

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
		PSSamplerSelector(uint32 k) : key(k) {}

		static uint32 size() { return 1 << 3; }
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
				uint32 alpha_stencil:1;
			};

			uint32 key;
		};

		operator uint32() {return key & 0x3f;}

		OMDepthStencilSelector() : key(0) {}
		OMDepthStencilSelector(uint32 k) : key(k) {}

		static uint32 size() { return 1 << 6; }
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

	GLuint m_fbo;				// frame buffer container
	GLuint m_fbo_read;			// frame buffer container only for reading

	GSVertexBufferStateOGL* m_vb;	  // vb_state for HW renderer
	GSVertexBufferStateOGL* m_vb_sr; // vb_state for StretchRect

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
		GLuint ps[10];	// program object
		GLuint ln;		// sampler object
		GLuint pt;		// sampler object
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
		GSTexture* t;
	} m_date;

	struct {
		GLuint ps;
		GSUniformBufferOGL *cb;
	} m_shadeboost;

	struct {
		GSVertexBufferStateOGL* vb;
		GSDepthStencilOGL* dss;
		GSBlendStateOGL* bs;
		float bf; // blend factor
	} m_state;

	GLuint m_vs[1<<5];
	GLuint m_gs;
	GLuint m_ps_ss[1<<3];
	GSDepthStencilOGL* m_om_dss[1<<6];
	hash_map<uint32, GLuint > m_ps;
	hash_map<uint32, GSBlendStateOGL* > m_om_bs;
	GLuint m_apitrace;

	GLuint m_palette_ss;
	GLuint m_rt_ss;

	GSUniformBufferOGL* m_vs_cb;
	GSUniformBufferOGL* m_ps_cb;

	VSConstantBuffer m_vs_cb_cache;
	PSConstantBuffer m_ps_cb_cache;

	GSTexture* CreateSurface(int type, int w, int h, bool msaa, int format);
	GSTexture* FetchSurface(int type, int w, int h, bool msaa, int format);

	void DoMerge(GSTexture* st[2], GSVector4* sr, GSTexture* dt, GSVector4* dr, bool slbg, bool mmod, const GSVector4& c);
	void DoInterlace(GSTexture* st, GSTexture* dt, int shader, bool linear, float yoffset = 0);
	void DoFXAA(GSTexture* st, GSTexture* dt);
	void DoShadeBoost(GSTexture* st, GSTexture* dt);

	void OMAttachRt(GLuint rt);
	void OMAttachDs(GLuint ds);
	void OMSetFBO(GLuint fbo);

	public:
	GSShaderOGL* m_shader;

	GSDeviceOGL();
	virtual ~GSDeviceOGL();

	static void CheckDebugLog();
	static void DebugOutputToFile(unsigned int source, unsigned int type, unsigned int id, unsigned int severity, const char* message);

	bool HasStencil() { return true; }
	bool HasDepth32() { return true; }

	bool Create(GSWnd* wnd);
	bool Reset(int w, int h);
	void Flip();
	void SetVSync(bool enable);
	// Used for opengl multithread hack
	void AttachContext();
	void DetachContext();

	void DrawPrimitive();
	void DrawIndexedPrimitive();
	void DrawIndexedPrimitive(int offset, int count);
	void BeforeDraw();
	void AfterDraw();

	void ClearRenderTarget(GSTexture* t, const GSVector4& c);
	void ClearRenderTarget(GSTexture* t, uint32 c);
	void ClearRenderTarget_ui(GSTexture* t, uint32 c);
	void ClearDepth(GSTexture* t, float c);
	void ClearStencil(GSTexture* t, uint8 c);

	GSTexture* CreateRenderTarget(int w, int h, bool msaa, int format = 0);
	GSTexture* CreateDepthStencil(int w, int h, bool msaa, int format = 0);
	GSTexture* CreateTexture(int w, int h, int format = 0);
	GSTexture* CreateOffscreen(int w, int h, int format = 0);
	void InitPrimDateTexture(int w, int h);
	void RecycleDateTexture();
	void BindDateTexture();

	GSTexture* CopyOffscreen(GSTexture* src, const GSVector4& sr, int w, int h, int format = 0);

	void CopyRect(GSTexture* st, GSTexture* dt, const GSVector4i& r);
	void StretchRect(GSTexture* st, const GSVector4& sr, GSTexture* dt, const GSVector4& dr, int shader = 0, bool linear = true);
	void StretchRect(GSTexture* st, const GSVector4& sr, GSTexture* dt, const GSVector4& dr, GLuint ps, bool linear = true);
	void StretchRect(GSTexture* st, const GSVector4& sr, GSTexture* dt, const GSVector4& dr, GLuint ps, GSBlendStateOGL* bs, bool linear = true);

	void SetupDATE(GSTexture* rt, GSTexture* ds, const GSVertexPxyT1* vertices, bool datm);

	void EndScene();

	void IASetPrimitiveTopology(GLenum topology);
	void IASetVertexBuffer(const void* vertices, size_t count);
	bool IAMapVertexBuffer(void** vertex, size_t stride, size_t count);
	void IAUnmapVertexBuffer();
	void IASetIndexBuffer(const void* index, size_t count);
	void IASetVertexState(GSVertexBufferStateOGL* vb = NULL);

	void PSSetShaderResource(GLuint sr);
	void PSSetShaderResources(GLuint tex[2]);
	void PSSetSamplerState(GLuint ss);

	void OMSetDepthStencilState(GSDepthStencilOGL* dss, uint8 sref);
	void OMSetBlendState(GSBlendStateOGL* bs, float bf);
	void OMSetRenderTargets(GSTexture* rt, GSTexture* ds, const GSVector4i* scissor = NULL);
	void OMSetWriteBuffer(GLenum buffer = GL_COLOR_ATTACHMENT0);

	void CreateTextureFX();
	GLuint CompileVS(VSSelector sel);
	GLuint CompileGS();
	GLuint CompilePS(PSSelector sel);
	GLuint CreateSampler(bool bilinear, bool tau, bool tav);
	GLuint CreateSampler(PSSamplerSelector sel);
	GSDepthStencilOGL* CreateDepthStencil(OMDepthStencilSelector dssel);
	GSBlendStateOGL* CreateBlend(OMBlendSelector bsel, uint8 afix);


	void SetupIA(const void* vertex, int vertex_count, const uint32* index, int index_count, int prim);
	void SetupVS(VSSelector sel);
	void SetupGS(bool enable);
	void SetupPS(PSSelector sel);
	void SetupCB(const VSConstantBuffer* vs_cb, const PSConstantBuffer* ps_cb);
	void SetupSampler(PSSamplerSelector ssel);
	void SetupOM(OMDepthStencilSelector dssel, OMBlendSelector bsel, uint8 afix);
	GLuint GetSamplerID(PSSamplerSelector ssel);
	GLuint GetPaletteSamplerID();

	void Barrier(GLbitfield b);
};
