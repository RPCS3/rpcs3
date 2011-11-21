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
	: m_free_window(false)
	  , m_window(NULL)
	  , m_vb(0)
	  , m_pipeline(0)
	  , m_srv_changed(false)
	  , m_ss_changed(false)
{
	m_msaa = theApp.GetConfig("msaa", 0);
	memset(&m_merge, 0, sizeof(m_merge));
	memset(&m_interlace, 0, sizeof(m_interlace));
	memset(&m_convert, 0, sizeof(m_convert));
	memset(&m_date, 0, sizeof(m_date));
	memset(&m_state, 0, sizeof(m_state));
}

GSDeviceOGL::~GSDeviceOGL()
{
	if(m_window != NULL && m_free_window)
	{
		SDL_DestroyWindow(m_window);
	}
}

GSTexture* GSDeviceOGL::CreateSurface(int type, int w, int h, bool msaa, int format)
{
	// A wrapper to call GSTextureOGL, with the different kind of parameter
	GSTextureOGL* t = NULL;
	t = new GSTextureOGL(type, w, h, msaa, format);

	switch(type)
	{
		case GSTexture::RenderTarget:
			ClearRenderTarget(t, 0);
			break;
		case GSTexture::DepthStencil:
			ClearDepth(t, 0);
			//FIXME might be need to clear the stencil too
			break;
	}
	return t;
}

GSTexture* GSDeviceOGL::FetchSurface(int type, int w, int h, bool msaa, int format)
{
	// FIXME: keep DX code. Do not know how work msaa but not important for the moment
	// Current config give only 0 or 1
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
	if (m_window == NULL) {
#if SDL_VERSION_ATLEAST(1,3,0)
		m_window = SDL_CreateWindowFrom(wnd->GetHandle());
#else
		assert(0);
#endif
	 	m_free_window = true;
	}

#ifdef __LINUX__
	// In GSopen2, sdl failed to received any resize event. GSWnd::GetClientRect need to manually
	// set the window size... So we send the m_window to the wnd object to allow some manipulation on it.
	wnd->SetWindow(m_window);
#endif

	// ****************************************************************
	// Various object
	// ****************************************************************
	glGenProgramPipelines(1, &m_pipeline);
	glBindProgramPipeline(m_pipeline);

	glGenFramebuffers(1, &m_fbo);
	// ****************************************************************
	// convert
	// ****************************************************************

	GSInputLayout il_convert[2] = 
	{
		{0, 4, GL_FLOAT, sizeof(GSVertexPT1), (const GLvoid*)offsetof(struct GSVertexPT1, p) },
		{1, 2, GL_FLOAT, sizeof(GSVertexPT1), (const GLvoid*)offsetof(struct GSVertexPT1, t) },
	};

	m_convert.il[0] = il_convert[0];
	m_convert.il[1] = il_convert[1];

	CompileShaderFromSource("convert.glsl", "vs_main", GL_VERTEX_SHADER, &m_convert.vs);
	for(int i = 0; i < countof(m_convert.ps); i++)
		CompileShaderFromSource("convert.glsl", format("ps_main%d", i), GL_FRAGMENT_SHADER, &m_convert.ps[i]);

	// Note the following object are initialized to 0 so disabled.
	// Note: maybe enable blend with a factor of 1
	// m_convert.dss, m_convert.bs

#if 0
	memset(&dsd, 0, sizeof(dsd));

	dsd.DepthEnable = false;
	dsd.StencilEnable = false;

	hr = m_dev->CreateDepthStencilState(&dsd, &m_convert.dss);

	memset(&bsd, 0, sizeof(bsd));

	bsd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	hr = m_dev->CreateBlendState(&bsd, &m_convert.bs);
#endif
	glGenSamplers(1, &m_convert.ln);
	glSamplerParameteri(m_convert.ln, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glSamplerParameteri(m_convert.ln, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glSamplerParameteri(m_convert.ln, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glSamplerParameteri(m_convert.ln, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glSamplerParameteri(m_convert.ln, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	// FIXME which value for GL_TEXTURE_MIN_LOD
	glSamplerParameteri(m_convert.ln, GL_TEXTURE_MAX_LOD, FLT_MAX);
	// FIXME: seems there is 2 possibility in opengl
	// DX: sd.ComparisonFunc = D3D11_COMPARISON_NEVER;
	// glSamplerParameteri(m_convert.ln, GL_TEXTURE_COMPARE_MODE, GL_NONE);
	glSamplerParameteri(m_convert.ln, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glSamplerParameteri(m_convert.ln, GL_TEXTURE_COMPARE_FUNC, GL_NEVER);


	glGenSamplers(1, &m_convert.pt);
	glGenSamplers(1, &m_convert.pt);
	glSamplerParameteri(m_convert.pt, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glSamplerParameteri(m_convert.pt, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glSamplerParameteri(m_convert.pt, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glSamplerParameteri(m_convert.pt, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glSamplerParameteri(m_convert.pt, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	// FIXME which value for GL_TEXTURE_MIN_LOD
	glSamplerParameteri(m_convert.pt, GL_TEXTURE_MAX_LOD, FLT_MAX);
	// FIXME: seems there is 2 possibility in opengl
	// DX: sd.ComparisonFunc = D3D11_COMPARISON_NEVER;
	// glSamplerParameteri(m_convert.pt, GL_TEXTURE_COMPARE_MODE, GL_NONE);
	glSamplerParameteri(m_convert.pt, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glSamplerParameteri(m_convert.pt, GL_TEXTURE_COMPARE_FUNC, GL_NEVER);

	// ****************************************************************
	// merge
	// ****************************************************************

	m_merge.cb->index = 1;
	m_merge.cb->byte_size = sizeof(MergeConstantBuffer);
	glGenBuffers(1, &m_merge.cb->buffer);
	glBindBuffer(GL_UNIFORM_BUFFER, m_merge.cb->buffer);
	glBufferData(GL_UNIFORM_BUFFER, m_merge.cb->byte_size, NULL, GL_DYNAMIC_DRAW);

	for(int i = 0; i < countof(m_merge.ps); i++)
		CompileShaderFromSource("merge.glsl", format("ps_main%d", i), GL_FRAGMENT_SHADER, &m_merge.ps[i]);

	m_merge.bs->m_enable         = true;
	m_merge.bs->m_equation_RGB   = GL_FUNC_ADD;
	m_merge.bs->m_equation_ALPHA = GL_FUNC_ADD;
	m_merge.bs->m_func_sRGB      = GL_SRC_ALPHA;
	m_merge.bs->m_func_dRGB      = GL_ONE_MINUS_SRC_ALPHA;
	m_merge.bs->m_func_sALPHA    = GL_ONE;
	m_merge.bs->m_func_dALPHA    = GL_ZERO;

	// ****************************************************************
	// interlace
	// ****************************************************************
	m_interlace.cb->index = 2;
	m_interlace.cb->byte_size = sizeof(InterlaceConstantBuffer);
	glGenBuffers(1, &m_interlace.cb->buffer);
	glBindBuffer(GL_UNIFORM_BUFFER, m_interlace.cb->buffer);
	glBufferData(GL_UNIFORM_BUFFER, m_interlace.cb->byte_size, NULL, GL_DYNAMIC_DRAW);

	for(int i = 0; i < countof(m_interlace.ps); i++)
		CompileShaderFromSource("interlace.glsl", format("ps_main%d", i), GL_FRAGMENT_SHADER, &m_interlace.ps[i]);

	// ****************************************************************
	// rasterization configuration
	// ****************************************************************
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDisable(GL_CULL_FACE);
	glEnable(GL_SCISSOR_TEST);
	// FIXME enable it when multisample code will be here
	// DX: rd.MultisampleEnable = true;
	glDisable(GL_MULTISAMPLE);
	// Hum I don't know for those options but let's hope there are not activated
#if 0
	rd.FrontCounterClockwise = false;
	rd.DepthBias = false;
	rd.DepthBiasClamp = 0;
	rd.SlopeScaledDepthBias = 0;
	rd.DepthClipEnable = false; // ???
	rd.AntialiasedLineEnable = false;
#endif

	GSVector4i rect = wnd->GetClientRect();
	Reset(rect.z, rect.w);

#if 0
	HRESULT hr = E_FAIL;

	DXGI_SWAP_CHAIN_DESC scd;
	D3D11_BUFFER_DESC bd;
	D3D11_SAMPLER_DESC sd;
	D3D11_DEPTH_STENCIL_DESC dsd;
	D3D11_RASTERIZER_DESC rd;
	D3D11_BLEND_DESC bsd;

	memset(&scd, 0, sizeof(scd));

	scd.BufferCount = 2;
	scd.BufferDesc.Width = 1;
	scd.BufferDesc.Height = 1;
	scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	//scd.BufferDesc.RefreshRate.Numerator = 60;
	//scd.BufferDesc.RefreshRate.Denominator = 1;
	scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scd.OutputWindow = (HWND)m_wnd->GetHandle();
	scd.SampleDesc.Count = 1;
	scd.SampleDesc.Quality = 0;

	// Always start in Windowed mode.  According to MS, DXGI just "prefers" this, and it's more or less
	// required if we want to add support for dual displays later on.  The fullscreen/exclusive flip
	// will be issued after all other initializations are complete.

	scd.Windowed = TRUE;

	// NOTE : D3D11_CREATE_DEVICE_SINGLETHREADED
	//   This flag is safe as long as the DXGI's internal message pump is disabled or is on the
	//   same thread as the GS window (which the emulator makes sure of, if it utilizes a
	//   multithreaded GS).  Setting the flag is a nice and easy 5% speedup on GS-intensive scenes.

	uint32 flags = D3D11_CREATE_DEVICE_SINGLETHREADED;

#ifdef DEBUG
	flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_FEATURE_LEVEL level;

	const D3D_FEATURE_LEVEL levels[] =
	{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
	};

	hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, flags, levels, countof(levels), D3D11_SDK_VERSION, &scd, &m_swapchain, &m_dev, &level, &m_ctx);
	// hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_REFERENCE, NULL, flags, NULL, 0, D3D11_SDK_VERSION, &scd, &m_swapchain, &m_dev, &level, &m_ctx);
#endif

	// ****************************************************************
	// The check of capability is done when context is created on openGL
	// For the moment don't bother with extension, I just ask the most recent openGL version
	// ****************************************************************
#if 0
	if(FAILED(hr)) return false;

	if(!SetFeatureLevel(level, true))
	{
		return false;
	}

	D3D11_FEATURE_DATA_D3D10_X_HARDWARE_OPTIONS options;

	hr = m_dev->CheckFeatureSupport(D3D11_FEATURE_D3D10_X_HARDWARE_OPTIONS, &options, sizeof(D3D11_FEATURE_D3D10_X_HARDWARE_OPTIONS));

	// msaa

	for(uint32 i = 2; i <= D3D11_MAX_MULTISAMPLE_SAMPLE_COUNT; i++)
	{
		uint32 quality[2] = {0, 0};

		if(SUCCEEDED(m_dev->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM, i, &quality[0])) && quality[0] > 0
		&& SUCCEEDED(m_dev->CheckMultisampleQualityLevels(DXGI_FORMAT_D32_FLOAT_S8X24_UINT, i, &quality[1])) && quality[1] > 0)
		{
			m_msaa_desc.Count = i;
			m_msaa_desc.Quality = std::min<uint32>(quality[0] - 1, quality[1] - 1);

			if(i >= m_msaa) break;
		}
	}

	if(m_msaa_desc.Count == 1)
	{
		m_msaa = 0;
	}
#endif

	// TODO Later
	// ****************************************************************
	// fxaa
	// ****************************************************************
#if 0

	memset(&bd, 0, sizeof(bd));

	bd.ByteWidth = sizeof(FXAAConstantBuffer);
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	hr = m_dev->CreateBuffer(&bd, NULL, &m_fxaa.cb);

	hr = CompileShader(IDR_FXAA_FX, "ps_main", NULL, &m_fxaa.ps);
#endif


	// TODO later
#if 0
	CreateTextureFX();

	//

	memset(&dsd, 0, sizeof(dsd));

	dsd.DepthEnable = false;
	dsd.StencilEnable = true;
	dsd.StencilReadMask = 1;
	dsd.StencilWriteMask = 1;
	dsd.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	dsd.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
	dsd.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsd.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	dsd.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	dsd.BackFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
	dsd.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsd.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;

	m_dev->CreateDepthStencilState(&dsd, &m_date.dss);

	D3D11_BLEND_DESC blend;

	memset(&blend, 0, sizeof(blend));

	m_dev->CreateBlendState(&blend, &m_date.bs);
#endif
}

bool GSDeviceOGL::Reset(int w, int h)
{
	if(!GSDevice::Reset(w, h))
		return false;

	// TODO
	// Opengl allocate the backbuffer with the window. The render is done in the backbuffer when
	// there isn't any FBO. Only a dummy texture is created to easily detect when the rendering is done 
	// in the backbuffer
	m_backbuffer = new GSTextureOGL(0, w, h, false, 0);

#if 0
	if(m_swapchain)
	{
		DXGI_SWAP_CHAIN_DESC scd;

		memset(&scd, 0, sizeof(scd));

		m_swapchain->GetDesc(&scd);
		m_swapchain->ResizeBuffers(scd.BufferCount, w, h, scd.BufferDesc.Format, 0);

		CComPtr<ID3D11Texture2D> backbuffer;

		if(FAILED(m_swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backbuffer)))
		{
			return false;
		}

		m_backbuffer = new GSTexture11(backbuffer);
	}
#endif

	return true;
}

void GSDeviceOGL::Flip()
{
	// Warning it is not OGL dependent but application dependant (glx and so not portable)
#if SDL_VERSION_ATLEAST(1,3,0)
	SDL_GL_SwapWindow(m_window);
#else
	SDL_GL_SwapBuffers();
#endif

}

void GSDeviceOGL::DrawPrimitive()
{
	glDrawArrays(m_state.topology, m_vertices.start, m_vertices.count);
}

void GSDeviceOGL::ClearRenderTarget(GSTexture* t, const GSVector4& c)
{
	GSTextureOGL* t_ogl = (GSTextureOGL*)t;
	// FIXME I need to clarify this FBO attachment stuff
	// I would like to avoid FBO for a basic clean operation
	//glClearBufferfv(GL_COLOR, t_ogl->attachment(), c.v);
}

void GSDeviceOGL::ClearRenderTarget(GSTexture* t, uint32 c)
{
	GSTextureOGL* t_ogl = (GSTextureOGL*)t;
	GSVector4 color = GSVector4::rgba32(c) * (1.0f / 255);
	// FIXME I need to clarify this FBO attachment stuff
	// I would like to avoid FBO for a basic clean operation
	//glClearBufferfv(GL_COLOR, t_ogl->attachment(), color.v);
}

void GSDeviceOGL::ClearDepth(GSTexture* t, float c)
{
	glClearBufferfv(GL_DEPTH, 0, &c);
}

void GSDeviceOGL::ClearStencil(GSTexture* t, uint8 c)
{
	GLint color = c;
	glClearBufferiv(GL_STENCIL, 0, &color);
}

GSTexture* GSDeviceOGL::CreateRenderTarget(int w, int h, bool msaa, int format)
{
	return GSDevice::CreateRenderTarget(w, h, msaa, format ? format : GL_RGBA8);
}

GSTexture* GSDeviceOGL::CreateDepthStencil(int w, int h, bool msaa, int format)
{
	return GSDevice::CreateDepthStencil(w, h, msaa, format ? format : GL_DEPTH32F_STENCIL8);
}

GSTexture* GSDeviceOGL::CreateTexture(int w, int h, int format)
{
	return GSDevice::CreateTexture(w, h, format ? format : GL_RGBA8);
}

GSTexture* GSDeviceOGL::CreateOffscreen(int w, int h, int format)
{
	return GSDevice::CreateOffscreen(w, h, format ? format : GL_RGBA8);
}

// blit a texture into an offscreen buffer
GSTexture* GSDeviceOGL::CopyOffscreen(GSTexture* src, const GSVector4& sr, int w, int h, int format)
{
	// I'm not sure about the object type for offscreen buffer
	// TODO later;
	assert(0);

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
	if(!st || !dt)
	{
		ASSERT(0);
		return;
	}

	assert(0);
	// FIXME attach the texture to the FBO
	GSTextureOGL* st_ogl = (GSTextureOGL*) st;
	GSTextureOGL* dt_ogl = (GSTextureOGL*) dt;
	dt_ogl->Attach(GL_COLOR_ATTACHMENT0);
	st_ogl->Attach(GL_COLOR_ATTACHMENT1);

	glReadBuffer(GL_COLOR_ATTACHMENT1);
	// FIXME I'not sure how to select the destination
	//	const GLenum draw_buffer[1] = { GL_COLOR_ATTACHMENT0 };
	//	glDrawBuffers(draw_buffer);
	dt_ogl->EnableUnit(0);
	// FIXME need acess of target and it probably need to be same for both
	//glCopyTexSubImage2D(dt_ogl.m_texture_target, 0, 0, 0, r.left, r.bottom, r.right-r.left, r.top-r.bottom);
	// FIXME I'm not sure GL_TEXTURE_RECTANGLE is supported!!!
	//glCopyTexSubImage2D(GL_TEXTURE_RECTANGLE, 0, 0, 0, r.left, r.bottom, r.right-r.left, r.top-r.bottom);
#if 0
	D3D11_BOX box = {r.left, r.top, 0, r.right, r.bottom, 1};
	m_ctx->CopySubresourceRegion(*(GSTexture11*)dt, 0, 0, 0, 0, *(GSTexture11*)st, 0, &box);
#endif
}

void GSDeviceOGL::StretchRect(GSTexture* st, const GSVector4& sr, GSTexture* dt, const GSVector4& dr, int shader, bool linear)
{
	StretchRect(st, sr, dt, dr, m_convert.ps[shader], NULL, linear);
}

void GSDeviceOGL::StretchRect(GSTexture* st, const GSVector4& sr, GSTexture* dt, const GSVector4& dr, GLuint ps, GSUniformBufferOGL* ps_cb, bool linear)
{
	StretchRect(st, sr, dt, dr, ps, ps_cb, m_convert.bs, linear);
}

// probably no difficult to convert when all helpers function will be done
void GSDeviceOGL::StretchRect(GSTexture* st, const GSVector4& sr, GSTexture* dt, const GSVector4& dr, GLuint ps, GSUniformBufferOGL* ps_cb, GSBlendStateOGL* bs, bool linear)
{
	//TODO later
	assert(0);
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
	ClearRenderTarget(dt, c);

	if(st[1] && !slbg)
	{
		StretchRect(st[1], sr[1], dt, dr[1], m_merge.ps[0], NULL, true);
	}

	if(st[0])
	{
		if (m_state.cb != m_merge.cb->buffer) {
			m_state.cb = m_merge.cb->buffer;
			glBindBuffer(GL_UNIFORM_BUFFER, m_merge.cb->buffer);
		}
		glBufferSubData(GL_UNIFORM_BUFFER, 0, m_merge.cb->byte_size, &c.v);
#if 0
		m_ctx->UpdateSubresource(m_merge.cb, 0, NULL, &c, 0, 0);
#endif
		StretchRect(st[0], sr[0], dt, dr[0], m_merge.ps[mmod ? 1 : 0], m_merge.cb, m_merge.bs, true);
	}
}

void GSDeviceOGL::DoInterlace(GSTexture* st, GSTexture* dt, int shader, bool linear, float yoffset)
{
	GSVector4 s = GSVector4(dt->GetSize());

	GSVector4 sr(0, 0, 1, 1);
	GSVector4 dr(0.0f, yoffset, s.x, s.y + yoffset);

	InterlaceConstantBuffer cb;

	cb.ZrH = GSVector2(0, 1.0f / s.y);
	cb.hH = s.y / 2;

	if (m_state.cb != m_interlace.cb->buffer) {
		m_state.cb = m_interlace.cb->buffer;
		glBindBuffer(GL_UNIFORM_BUFFER, m_interlace.cb->buffer);
	}
	// FIXME I'm not sure it will be happy with InterlaceConstantBuffer type
	glBufferSubData(GL_UNIFORM_BUFFER, 0, m_interlace.cb->byte_size, &cb);
#if 0
	m_ctx->UpdateSubresource(m_interlace.cb, 0, NULL, &cb, 0, 0);
#endif

	StretchRect(st, sr, dt, dr, m_interlace.ps[shader], m_interlace.cb, linear);
}

// copy a multisample texture to a non-texture multisample. On opengl you need 2 FBO with different level of
// sample and then do a blit. Headach expected to for the moment just drop MSAA...
GSTexture* GSDeviceOGL::Resolve(GSTexture* t)
{
	ASSERT(t != NULL && t->IsMSAA());
#if 0

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
		if (m_vb) {
			glDeleteBuffers(1, &m_vb);
			m_vb = 0;
		}

		m_vertices.start = 0;
		m_vertices.count = 0;
		m_vertices.limit = std::max<int>(count * 3 / 2, 11000);
	}

	if(!m_vb)
	{
		glGenBuffers(1, &m_vb);
		// FIXME: GL_DYNAMIC_DRAW vs GL_STREAM_DRAW !!!
		glBindBuffer(GL_ARRAY_BUFFER, m_vb);
		glBufferData(GL_ARRAY_BUFFER, m_vertices.limit, NULL, GL_DYNAMIC_DRAW);
	}

	// append data or go back to the beginning
	// Hum why we don't always go back to the beginning !!!
	if(m_vertices.start + count > m_vertices.limit || stride != m_vertices.stride)
		m_vertices.start = 0;

	// Allocate the buffer
	// glBufferSubData
	glBufferSubData(GL_ARRAY_BUFFER, m_vertices.start * stride, count * stride, vertices);

	m_vertices.count = count;
	m_vertices.stride = stride;
}

void GSDeviceOGL::IASetInputLayout(GSInputLayout* layout, int layout_nbr)
{
	if(m_state.layout != layout)
	{
		m_state.layout = layout;

		for (int i = 0; i < layout_nbr; i++)
			glVertexAttribPointer(layout->index, layout->size, layout->type, GL_FALSE,  layout->stride, layout->offset);
	}
}

void GSDeviceOGL::IASetPrimitiveTopology(GLenum topology)
{
	if(m_state.topology != topology)
	{
		m_state.topology = topology;
	}
}

void GSDeviceOGL::VSSetShader(GLuint vs, GSUniformBufferOGL* vs_cb)
{
	if(m_state.vs != vs)
	{
		m_state.vs = vs;
		glUseProgramStages(m_pipeline, GL_VERTEX_SHADER_BIT, vs);
	}

	if(m_state.vs_cb != vs_cb)
	{
		m_state.vs_cb = vs_cb;
		glBindBufferBase(GL_UNIFORM_BUFFER, vs_cb->index, vs_cb->buffer);
	}
}

void GSDeviceOGL::GSSetShader(GLuint gs)
{
	if(m_state.gs != gs)
	{
		m_state.gs = gs;
		glUseProgramStages(m_pipeline, GL_GEOMETRY_SHADER_BIT, gs);
	}
}

void GSDeviceOGL::PSSetShaderResources(GSTexture* sr0, GSTexture* sr1)
{
	PSSetShaderResource(0, sr0);
	PSSetShaderResource(1, sr1);
	PSSetShaderResource(2, NULL);
}

void GSDeviceOGL::PSSetShaderResource(int i, GSTexture* sr)
{
	GSTextureOGL* srv = NULL;

	if(sr) srv = (GSTextureOGL*)sr;

	if(m_state.ps_srv[i] != srv)
	{
		m_state.ps_srv[i] = srv;

		m_srv_changed = true;
	}
}

void GSDeviceOGL::PSSetSamplerState(GLuint ss0, GLuint ss1, GLuint ss2)
{
	if(m_state.ps_ss[0] != ss0 || m_state.ps_ss[1] != ss1 || m_state.ps_ss[2] != ss2)
	{
		m_state.ps_ss[0] = ss0;
		m_state.ps_ss[1] = ss1;
		m_state.ps_ss[2] = ss2;

		m_ss_changed = true;
	}
}

void GSDeviceOGL::PSSetShader(GLuint ps, GSUniformBufferOGL* ps_cb)
{

	if(m_state.ps != ps)
	{
		m_state.ps = ps;
		glUseProgramStages(m_pipeline, GL_FRAGMENT_SHADER_BIT, ps);
	}

// Sampler and texture must be set at the same time
// 1/ select the texture unit
// glActiveTexture(GL_TEXTURE0 + 1);
// 2/ bind the texture
// glBindTexture(GL_TEXTURE_2D , brickTexture);
// 3/ sets the texture sampler in GLSL (could be useless with layout stuff)
// glUniform1i(brickSamplerId , 1);
// 4/ set the sampler state
// glBindSampler(1 , sampler);
	if (m_srv_changed || m_ss_changed) {
		for (uint i=0 ; i < 3; i++) {
			m_state.ps_srv[i]->EnableUnit(i);
			glBindSampler(i, m_state.ps_ss[i]);
		}
	}
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

	if(m_state.ps_cb != ps_cb)
	{
		m_state.ps_cb = ps_cb;
		glBindBufferBase(GL_UNIFORM_BUFFER, ps_cb->index, ps_cb->buffer);
	}
}

void GSDeviceOGL::OMSetDepthStencilState(GSDepthStencilOGL* dss, uint8 sref)
{
	uint ref = sref;

	if(m_state.dss != dss || m_state.sref != sref)
	{
		m_state.dss = dss;
		m_state.sref = sref;

		if (dss->m_depth_enable) {
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(dss->m_depth_func);
			glDepthMask(dss->m_depth_mask);
		} else
			glDisable(GL_DEPTH_TEST);

		if (dss->m_stencil_enable) {
			glEnable(GL_STENCIL_TEST);
			glStencilFunc(dss->m_stencil_func, ref, dss->m_stencil_mask);
			glStencilOp(dss->m_stencil_sfail_op, dss->m_stencil_spass_dfail_op, dss->m_stencil_spass_dpass_op);
		} else
			glDisable(GL_STENCIL_TEST);
	}
}

void GSDeviceOGL::OMSetBlendState(GSBlendStateOGL* bs, float bf)
{
	// DX:Blend factor D3D11_BLEND_BLEND_FACTOR | D3D11_BLEND_INV_BLEND_FACTOR
	// OPENGL: GL_CONSTANT_COLOR | GL_ONE_MINUS_CONSTANT_COLOR
	// Note factor must be set before by glBlendColor
	if(m_state.bs != bs || m_state.bf != bf)
	{
		m_state.bs = bs;
		m_state.bf = bf;

		// FIXME: double check when blend stuff is complete
		if (bs->m_func_sRGB == GL_CONSTANT_COLOR || bs->m_func_sRGB == GL_ONE_MINUS_CONSTANT_COLOR
				|| bs->m_func_dRGB == GL_CONSTANT_COLOR || bs->m_func_dRGB == GL_ONE_MINUS_CONSTANT_COLOR)
			glBlendColor(bf, bf, bf, 0);

		glBlendEquationSeparate(bs->m_equation_RGB, bs->m_equation_ALPHA);
		glBlendFuncSeparate(bs->m_func_sRGB, bs->m_func_dRGB, bs->m_func_sALPHA, bs->m_func_dALPHA);
	}
}

void GSDeviceOGL::OMSetRenderTargets(GSTexture* rt, GSTexture* ds, const GSVector4i* scissor)
{
	// attach render&depth  to the FBO
	// Hum, need to separate 2 case, Render target fbo and render target backbuffer
	// Or maybe blit final result to the backbuffer
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
	GSTextureOGL* rt_ogl = (GSTextureOGL*)rt;
	GSTextureOGL* ds_ogl = (GSTextureOGL*)ds;

	if (m_backbuffer == rt_ogl) {
		if (m_state.fbo) {
			m_state.fbo = 0;
			glBindFramebuffer(GL_FRAMEBUFFER, 0); // render in the backbuffer
		}

		assert(ds_ogl == NULL); // no depth-stencil without FBO
	} else {
		if (m_state.fbo != m_fbo) {
			m_state.fbo = m_fbo;
			glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
		}

		assert(rt_ogl != NULL); // a render target must exists
		rt_ogl->Attach(GL_COLOR_ATTACHMENT0);
		if (ds_ogl != NULL)
			ds_ogl->Attach(GL_DEPTH_STENCIL_ATTACHMENT);
	}

	// Viewport -> glViewport
	if(m_state.viewport != rt->GetSize())
	{
		m_state.viewport = rt->GetSize();
		glViewport(0, 0, rt->GetWidth(), rt->GetHeight());
	}

	// Scissor -> glScissor (note must be enabled)
	GSVector4i r = scissor ? *scissor : GSVector4i(rt->GetSize()).zwxy();

	if(!m_state.scissor.eq(r))
	{
		m_state.scissor = r;
		// FIXME check position
		glScissor(r.x, r.w, r.z-r.x, r.y-r.w);
#if 0
		m_ctx->RSSetScissorRects(1, r);
#endif
	}
}

void GSDeviceOGL::CompileShaderFromSource(const std::string& glsl_file, const std::string& entry, GLenum type, GLuint* program)
{
	// Could be useful to filter ubber shader
	std::string shader_type;
	switch (type) {
		case GL_VERTEX_SHADER:
			shader_type = "#define VERTEX_SHADER 1\n";
		case GL_GEOMETRY_SHADER:
			shader_type = "#define GEOMETRY_SHADER 1\n";
		case GL_FRAGMENT_SHADER:
			shader_type = "#define FRAGMENT_SHADER 1\n";
	}

	// Select the entry point
	std::string entry_main = format("#define %s main\n", entry.c_str());

	// Read the source file
	std::string source;
	std::string line;
	// Each linux distributions have his rules for path so we give them the possibility to
	// change it with compilation flags. -- Gregory
#ifdef PLUGIN_DIR_COMPILATION
#define xPLUGIN_DIR_str(s) PLUGIN_DIR_str(s)
#define PLUGIN_DIR_str(s) #s
	const std::string shader_file = string(xPLUGIN_DIR_str(PLUGIN_DIR_COMPILATION)) + '/' + glsl_file;
#else
	const std::string shader_file = string("plugins/") + glsl_file;
#endif
	std::ifstream myfile(shader_file.c_str());
	if (myfile.is_open()) {
		while ( myfile.good() )
		{
			getline (myfile,line);
			source += line;
			source += '\n';
		}
		myfile.close();
	} else {
		fprintf(stderr, "Error opening %s: ", shader_file.c_str());
	}

	// Create a giant string with everythings
	std::string full_source = shader_type + entry_main + source;

	char* sources = (char*)malloc(full_source.size());
	strncpy(sources, full_source.c_str(), full_source.size());
	sources[full_source.size()] = '\0';

	*program = glCreateShaderProgramv(type, 1, (const char**)&sources);

	free(sources);
}
