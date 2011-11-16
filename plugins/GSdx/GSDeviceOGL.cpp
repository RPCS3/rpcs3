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

#include "GSDeviceOGL.h"

// Various Note: separate build of shader
// ************************** BUILD METHOD 1
// 1/ create a stand alone prog
// uint CreateShaderProgramv( enum type, sizei count, const char **strings );
// 2/ Create various prog pipeline (maybe 1 foreach combination or only 1)
// glGenProgramPipelines
// 3/ Attach a program to a pipeline
// void UseProgramStages( uint pipeline, bitfield stages, uint program );
// ...
// ...
// 5/ Before the use of the program, we can validate it. (mandatory or not ?) 
// glValidateProgramPipeline
//
// ************************** BUILD METHOD 2
// Humm, there is another solutions. You can setup function pointer in GLSL and configure them with OPENGL. shader_subroutine. glUseProgram must be replace with glUseProgramInstance in this case (and instance must be managed)
//GL EXT: Overview
//GL EXT: 
//GL EXT:     This extension adds support to shaders for "indirect subroutine calls", 
//GL EXT:     where a single shader can include many subroutines and dynamically select
//GL EXT:     through the API which subroutine is called from each call site. 
//GL EXT:     Switching subroutines dynamically in this fashion can avoid the cost of 
//GL EXT:     recompiling and managing multiple shaders, while still retaining most of
//GL EXT:     the performance of specialized shaders.
//
// ************************** Uniform buffer
// // Note don't know how to chose block_binding_point (probably managed like texture unit 
// maybe any number will be fine)
// index=glGetUniformBlockIndex(program, "BlockName");
// glBindBufferBase(GL_UNIFORM_BUFFER, block_binding_point, some_buffer_object);
// glUniformBlockBinding(program, block_index, block_binding_point);


GSDeviceOGL::GSDeviceOGL()
{
	/* TODO */
	m_msaa = theApp.GetConfig("msaa", 0);
}

GSDeviceOGL::~GSDeviceOGL() { /* TODO */ }

GSTexture* GSDeviceOGL::CreateSurface(int type, int w, int h, bool msaa, int format)
{
	// A wrapper to call GSTextureOGL, with the different kind of parameter
}

GSTexture* GSDeviceOGL::FetchSurface(int type, int w, int h, bool msaa, int format)
{
	// FIXME: keep DX code. Do not know how work msaa but not important for the moment
#if 0
	if(m_msaa < 2) {
		msaa = false;
	}
#endif
	msaa = false;

	return GSDevice::FetchSurface(type, w, h, msaa, format);
}

bool GSDeviceOGL::Create(GSWnd* wnd)
{
	/* TODO */
}

bool GSDeviceOGL::Reset(int w, int h)
{
	// Hum map, m_backbuffer to a GSTexture
}

// Swap buffer (back buffer, front buffer)
// Warning it is not OGL dependent but application dependant (glx and so not portable)
void GSDeviceOGL::Flip() { /* TODO */ }

// glDrawArrays
void GSDeviceOGL::DrawPrimitive() { /* TODO */ }

// Just a wrapper around glClear* functions
void GSDeviceOGL::ClearRenderTarget(GSTexture* t, const GSVector4& c) { /* TODO */ }
void GSDeviceOGL::ClearRenderTarget(GSTexture* t, uint32 c) { /* TODO */ }
void GSDeviceOGL::ClearDepth(GSTexture* t, float c) { /* TODO */ }
void GSDeviceOGL::ClearStencil(GSTexture* t, uint8 c) { /* TODO */ }

// the 4 next functions are only used to set some default value for the format.
// Need to find the default format...
// depth_stencil => GL_DEPTH32F_STENCIL8
// others => (need 4* 8bits unsigned normalized integer) (GL_RGBA8 )
GSTexture* GSDeviceOGL::CreateRenderTarget(int w, int h, bool msaa, int format) { /* TODO */ }
GSTexture* GSDeviceOGL::CreateDepthStencil(int w, int h, bool msaa, int format) { /* TODO */ }
GSTexture* GSDeviceOGL::CreateTexture(int w, int h, int format) { /* TODO */ }
GSTexture* GSDeviceOGL::CreateOffscreen(int w, int h, int format) { /* TODO */ }

// blit a texture into an offscreen buffer
GSTexture* GSDeviceOGL::CopyOffscreen(GSTexture* src, const GSVector4& sr, int w, int h, int format)
{
	// Need to find format equivalent. Then I think it will be straight forward

	// A four-component, 32-bit unsigned-normalized-integer format that supports 8 bits per channel including alpha.
	// DXGI_FORMAT_R8G8B8A8_UNORM <=> GL_RGBA8

	// A single-component, 16-bit unsigned-integer format that supports 16 bits for the red channel
	// DXGI_FORMAT_R16_UINT <=> GL_R16
#if 0
	GSTexture* dst = NULL;

	if(format == 0)
	{
		format = DXGI_FORMAT_R8G8B8A8_UNORM;
	}

	if(format != DXGI_FORMAT_R8G8B8A8_UNORM && format != DXGI_FORMAT_R16_UINT)
	{
		ASSERT(0);

		return false;
	}

	if(GSTexture* rt = CreateRenderTarget(w, h, false, format))
	{
		GSVector4 dr(0, 0, w, h);

		if(GSTexture* src2 = src->IsMSAA() ? Resolve(src) : src)
		{
			StretchRect(src2, sr, rt, dr, m_convert.ps[format == DXGI_FORMAT_R16_UINT ? 1 : 0], NULL);

			if(src2 != src) Recycle(src2);
		}

		dst = CreateOffscreen(w, h, format);

		if(dst)
		{
			m_ctx->CopyResource(*(GSTexture11*)dst, *(GSTexture11*)rt);
		}

		Recycle(rt);
	}

	return dst;
#endif
}

// Copy a sub part of a texture into another
// Several question to answer did texture have same size?
// From a sub-part to the same sub-part
// From a sub-part to a full texture
void GSDeviceOGL::CopyRect(GSTexture* st, GSTexture* dt, const GSVector4i& r)
{
#if 0
	if(!st || !dt)
	{
		ASSERT(0);
		return;
	}

	D3D11_BOX box = {r.left, r.top, 0, r.right, r.bottom, 1};

	m_ctx->CopySubresourceRegion(*(GSTexture11*)dt, 0, 0, 0, 0, *(GSTexture11*)st, 0, &box);
#endif
}


// Raster flow to resize a texture
// Note just call the StretchRect hardware accelerated
void GSDeviceOGL::StretchRect(GSTexture* st, const GSVector4& sr, GSTexture* dt, const GSVector4& dr, int shader, bool linear)
{
	/* TODO */
}
void GSDeviceOGL::StretchRect(GSTexture* st, const GSVector4& sr, GSTexture* dt, const GSVector4& dr, GLuint ps, GSUniformBufferOGL* ps_cb, bool linear)
{
	// TODO
}

// probably no difficult to convert when all helpers function will be done
void GSDeviceOGL::StretchRect(GSTexture* st, const GSVector4& sr, GSTexture* dt, const GSVector4& dr, GLuint ps, GSUniformBufferOGL ps_cb, GSBlendStateOGL* bs, bool linear)
{
#if 0
	if(!st || !dt)
	{
		ASSERT(0);
		return;
	}

	BeginScene();

	GSVector2i ds = dt->GetSize();

	// om

	OMSetDepthStencilState(m_convert.dss, 0);
	OMSetBlendState(bs, 0);
	OMSetRenderTargets(dt, NULL);

	// ia

	float left = dr.x * 2 / ds.x - 1.0f;
	float top = 1.0f - dr.y * 2 / ds.y;
	float right = dr.z * 2 / ds.x - 1.0f;
	float bottom = 1.0f - dr.w * 2 / ds.y;

	GSVertexPT1 vertices[] =
	{
		{GSVector4(left, top, 0.5f, 1.0f), GSVector2(sr.x, sr.y)},
		{GSVector4(right, top, 0.5f, 1.0f), GSVector2(sr.z, sr.y)},
		{GSVector4(left, bottom, 0.5f, 1.0f), GSVector2(sr.x, sr.w)},
		{GSVector4(right, bottom, 0.5f, 1.0f), GSVector2(sr.z, sr.w)},
	};

	IASetVertexBuffer(vertices, sizeof(vertices[0]), countof(vertices));
	IASetInputLayout(m_convert.il);
	IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	// vs

	VSSetShader(m_convert.vs, NULL);

	// gs

	GSSetShader(NULL);

	// ps

	PSSetShaderResources(st, NULL);
	PSSetSamplerState(linear ? m_convert.ln : m_convert.pt, NULL);
	PSSetShader(ps, ps_cb);

	//

	DrawPrimitive();

	//

	EndScene();

	PSSetShaderResources(NULL, NULL);
#endif
}

void GSDeviceOGL::DoMerge(GSTexture* st[2], GSVector4* sr, GSTexture* dt, GSVector4* dr, bool slbg, bool mmod, const GSVector4& c)
{
#if 0
	ClearRenderTarget(dt, c);

	if(st[1] && !slbg)
	{
		StretchRect(st[1], sr[1], dt, dr[1], m_merge.ps[0], NULL, true);
	}

	if(st[0])
	{
		m_ctx->UpdateSubresource(m_merge.cb, 0, NULL, &c, 0, 0);

		StretchRect(st[0], sr[0], dt, dr[0], m_merge.ps[mmod ? 1 : 0], m_merge.cb, m_merge.bs, true);
	}
#endif
}

void GSDeviceOGL::DoInterlace(GSTexture* st, GSTexture* dt, int shader, bool linear, float yoffset)
{
#if 0
	GSVector4 s = GSVector4(dt->GetSize());

	GSVector4 sr(0, 0, 1, 1);
	GSVector4 dr(0.0f, yoffset, s.x, s.y + yoffset);

	InterlaceConstantBuffer cb;

	cb.ZrH = GSVector2(0, 1.0f / s.y);
	cb.hH = s.y / 2;

	m_ctx->UpdateSubresource(m_interlace.cb, 0, NULL, &cb, 0, 0);

	StretchRect(st, sr, dt, dr, m_interlace.ps[shader], m_interlace.cb, linear);
#endif
}

// copy a multisample texture to a non-texture multisample. On opengl you need 2 FBO with different level of
// sample and then do a blit. Headach expected to for the moment just drop MSAA...
GSTexture* GSDeviceOGL::Resolve(GSTexture* t)
{
#if 0
	ASSERT(t != NULL && t->IsMSAA());

	if(GSTexture* dst = CreateRenderTarget(t->GetWidth(), t->GetHeight(), false, t->GetFormat()))
	{
		dst->SetScale(t->GetScale());

		m_ctx->ResolveSubresource(*(GSTexture11*)dst, 0, *(GSTexture11*)t, 0, (DXGI_FORMAT)t->GetFormat());

		return dst;
	}

	return NULL;
#endif
	return NULL;
}

void GSDeviceOGL::IASetVertexBuffer(const void* vertices, size_t stride, size_t count)
{
	ASSERT(m_vertices.count == 0);

	if(count * stride > m_vertices.limit * m_vertices.stride)
	{
		// Current GPU buffer is too small need to realocate a new one
#if 0
		m_vb_old = m_vb;
		m_vb = NULL;

		m_vertices.start = 0;
		m_vertices.count = 0;
		m_vertices.limit = std::max<int>(count * 3 / 2, 11000);
#endif
	}

	if(m_vb == NULL)
	{
		// Allocate a GPU buffer: GL_DYNAMIC_DRAW vs GL_STREAM_DRAW !!!
		// glBufferData(GL_ARRAY_BUFFER, m_vertices.limit, NULL, GL_DYNAMIC_DRAW)
#if 0
		D3D11_BUFFER_DESC bd;

		memset(&bd, 0, sizeof(bd));

		bd.Usage = D3D11_USAGE_DYNAMIC;
		bd.ByteWidth = m_vertices.limit * stride;
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		HRESULT hr;

		hr = m_dev->CreateBuffer(&bd, NULL, &m_vb);

		if(FAILED(hr)) return;
#endif
	}

	// append data or go back to the beginning
	// Hum why we don't always go back to the beginning !!!
#if 0
	D3D11_MAP type = D3D11_MAP_WRITE_NO_OVERWRITE;

	if(m_vertices.start + count > m_vertices.limit || stride != m_vertices.stride)
	{
		m_vertices.start = 0;

		type = D3D11_MAP_WRITE_DISCARD;
	}
#endif

	// Allocate the buffer
	// glBufferSubData
#if 0
	D3D11_MAPPED_SUBRESOURCE m;

	if(SUCCEEDED(m_ctx->Map(m_vb, 0, type, 0, &m)))
	{
		GSVector4i::storent((uint8*)m.pData + m_vertices.start * stride, vertices, count * stride);

		m_ctx->Unmap(m_vb, 0);
	}
#endif

	m_vertices.count = count;
	m_vertices.stride = stride;

// Useless ?
// binding of the buffer must be done anyway before glBufferSubData or glBufferData calls.
#if 0
	IASetVertexBuffer(m_vb, stride);
#endif
}

// Useless ?
#if 0
void GSDeviceOGL::IASetVertexBuffer(GLuint vb, size_t stride)
{
	if(m_state.vb != vb || m_state.vb_stride != stride)
	{
		m_state.vb = vb;
		m_state.vb_stride = stride;

		uint32 stride2 = stride;
		uint32 offset = 0;

		m_ctx->IASetVertexBuffers(0, 1, &vb, &stride2, &offset);
	}
}
#endif

// Useless ?
#if 0
void GSDeviceOGL::IASetInputLayout(ID3D11InputLayout* layout)
{
	if(m_state.layout != layout)
	{
		m_state.layout = layout;

		m_ctx->IASetInputLayout(layout);
	}
}
#endif

// Useless ?
#if 0
void GSDeviceOGL::IASetPrimitiveTopology(GLenum topology)
{
	if(m_state.topology != topology)
	{
		m_state.topology = topology;

		m_ctx->IASetPrimitiveTopology(topology);
	}
}
#endif

void GSDeviceOGL::VSSetShader(GLuint vs, GSUniformBufferOGL* vs_cb)
{
	// glUseProgramStage
#if 0
	if(m_state.vs != vs)
	{
		m_state.vs = vs;

		m_ctx->VSSetShader(vs, NULL, 0);
	}
#endif

	// glBindBufferBase
#if 0
	if(m_state.vs_cb != vs_cb)
	{
		m_state.vs_cb = vs_cb;

		m_ctx->VSSetConstantBuffers(0, 1, &vs_cb);
	}
#endif
}

void GSDeviceOGL::GSSetShader(GLuint gs)
{
	// glUseProgramStage
#if 0
	if(m_state.gs != gs)
	{
		m_state.gs = gs;

		m_ctx->GSSetShader(gs, NULL, 0);
	}
#endif
}

void GSDeviceOGL::PSSetShaderResources(GSTexture* sr0, GSTexture* sr1)
{
	PSSetShaderResource(0, sr0);
	PSSetShaderResource(1, sr1);
	PSSetShaderResource(2, NULL);
}

void GSDeviceOGL::PSSetShaderResource(int i, GSTexture* sr)
{
#if 0
	ID3D11ShaderResourceView* srv = NULL;

	if(sr) srv = *(GSTexture11*)sr;

	if(m_state.ps_srv[i] != srv)
	{
		m_state.ps_srv[i] = srv;

		m_srv_changed = true;
	}
#endif
}

void GSDeviceOGL::PSSetSamplerState(GLuint ss0, GLuint ss1, GLuint ss2)
{
#if 0
	if(m_state.ps_ss[0] != ss0 || m_state.ps_ss[1] != ss1 || m_state.ps_ss[2] != ss2)
	{
		m_state.ps_ss[0] = ss0;
		m_state.ps_ss[1] = ss1;
		m_state.ps_ss[2] = ss2;

		m_ss_changed = true;
	}
#endif
}

void GSDeviceOGL::PSSetShader(GLuint ps, GSUniformBufferOGL* ps_cb)
{

	// glUseProgramStage
#if 0
	if(m_state.ps != ps)
	{
		m_state.ps = ps;

		m_ctx->PSSetShader(ps, NULL, 0);
	}
#endif

// Sampler and texture must be set at the same time
// 1/ select the texture unit
// glActiveTexture(GL_TEXTURE0 + 1);
// 2/ bind the texture
// glBindTexture(GL_TEXTURE_2D , brickTexture);
// 3/ sets the texture sampler in GLSL (could be useless with layout stuff)
// glUniform1i(brickSamplerId , 1);
// 4/ set the sampler state
// glBindSampler(1 , sampler);
#if 0
	if (m_srv_changed)
	{
		m_ctx->PSSetShaderResources(0, 3, m_state.ps_srv);

		m_srv_changed = false;
	}

	if(m_ss_changed)
	{
		m_ctx->PSSetSamplers(0, 3, m_state.ps_ss);

		m_ss_changed = false;
	}
#endif

	// glBindBufferBase
#if 0
	if(m_state.ps_cb != ps_cb)
	{
		m_state.ps_cb = ps_cb;

		m_ctx->PSSetConstantBuffers(0, 1, &ps_cb);
	}
#endif
}

void GSDeviceOGL::OMSetDepthStencilState(GSDepthStencilOGL* dss, uint8 sref)
{
	// Setup the stencil object. sref can be set by glStencilFunc (note remove it from the structure)
#if 0
	if(m_state.dss != dss || m_state.sref != sref)
	{
		m_state.dss = dss;
		m_state.sref = sref;

		m_ctx->OMSetDepthStencilState(dss, sref);
	}
#endif
}

void GSDeviceOGL::OMSetBlendState(GSBlendStateOGL* bs, float bf)
{
	// DX:Blend factor D3D11_BLEND_BLEND_FACTOR | D3D11_BLEND_INV_BLEND_FACTOR
	// OPENGL: GL_CONSTANT_COLOR | GL_ONE_MINUS_CONSTANT_COLOR
	// Note factor must be set before by glBlendColor
#if 0
	if(m_state.bs != bs || m_state.bf != bf)
	{
		m_state.bs = bs;
		m_state.bf = bf;

		float BlendFactor[] = {bf, bf, bf, 0};

		m_ctx->OMSetBlendState(bs, BlendFactor, 0xffffffff);
	}
#endif
}

void GSDeviceOGL::OMSetRenderTargets(GSTexture* rt, GSTexture* ds, const GSVector4i* scissor)
{
	// set the attachment inside the FBO

#if 0
	ID3D11RenderTargetView* rtv = NULL;
	ID3D11DepthStencilView* dsv = NULL;

	if(rt) rtv = *(GSTexture11*)rt;
	if(ds) dsv = *(GSTexture11*)ds;

	if(m_state.rtv != rtv || m_state.dsv != dsv)
	{
		m_state.rtv = rtv;
		m_state.dsv = dsv;

		m_ctx->OMSetRenderTargets(1, &rtv, dsv);
	}
#endif

	// Viewport -> glViewport
#if 0
	if(m_state.viewport != rt->GetSize())
	{
		m_state.viewport = rt->GetSize();

		D3D11_VIEWPORT vp;

		memset(&vp, 0, sizeof(vp));

		vp.TopLeftX = 0;
		vp.TopLeftY = 0;
		vp.Width = (float)rt->GetWidth();
		vp.Height = (float)rt->GetHeight();
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;

		m_ctx->RSSetViewports(1, &vp);
	}
#endif

	// Scissor -> glScissor (note must be enabled)
#if 0
	GSVector4i r = scissor ? *scissor : GSVector4i(rt->GetSize()).zwxy();

	if(!m_state.scissor.eq(r))
	{
		m_state.scissor = r;

		m_ctx->RSSetScissorRects(1, r);
	}
#endif
}
