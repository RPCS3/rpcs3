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

// TODO performance cost to investigate
// Texture attachment/glDrawBuffer. For the moment it set every draw and potentially multiple time (first time in clear, second time in rendering)
//  Attachment 1 is only used with the GL_16UI format

//#define LOUD_DEBUGGING
//#define PRINT_FRAME_NUMBER
//#define ONLY_LINES

// It seems dual blending does not work on AMD !!!
//#define DISABLE_DUAL_BLEND

static uint32 g_draw_count = 0;
static uint32 g_frame_count = 1;		


GSDeviceOGL::GSDeviceOGL()
	: m_free_window(false)
	  , m_window(NULL)
	  , m_pipeline(0)
	  , m_fbo(0)
	  , m_fbo_read(0)
	  , m_vb_sr(NULL)
	  , m_srv_changed(false)
	  , m_ss_changed(false)
{
	m_msaa = theApp.GetConfig("msaa", 0);

	memset(&m_merge_obj, 0, sizeof(m_merge_obj));
	memset(&m_interlace, 0, sizeof(m_interlace));
	memset(&m_convert, 0, sizeof(m_convert));
	memset(&m_date, 0, sizeof(m_date));
	memset(&m_state, 0, sizeof(m_state));

	// Reset the debug file
	FILE* f = fopen("Debug.txt","w");
	fclose(f);
}

GSDeviceOGL::~GSDeviceOGL()
{
	// Clean vertex buffer state
	delete (m_vb_sr);

	// Clean m_merge_obj
	for (uint i = 0; i < 2; i++)
		glDeleteProgram(m_merge_obj.ps[i]);
	delete (m_merge_obj.cb);
	delete (m_merge_obj.bs);
	
	// Clean m_interlace
	for (uint i = 0; i < 2; i++)
		glDeleteProgram(m_interlace.ps[i]);
	delete (m_interlace.cb);

	// Clean m_convert
	glDeleteProgram(m_convert.vs);
	for (uint i = 0; i < 2; i++)
		glDeleteProgram(m_convert.ps[i]);
	glDeleteSamplers(1, &m_convert.ln);
	glDeleteSamplers(1, &m_convert.pt);
	delete m_convert.dss;
	delete m_convert.bs;

	// Clean m_date
	delete m_date.dss;
	delete m_date.bs;

	// Clean various opengl allocation
	glDeleteProgramPipelines(1, &m_pipeline);
	glDeleteFramebuffers(1, &m_fbo);
	glDeleteFramebuffers(1, &m_fbo_read);

	// Delete HW FX
	delete m_vs_cb;
	delete m_ps_cb;
	glDeleteSamplers(1, &m_rt_ss);
	delete m_vb;

	for (auto it = m_vs.begin(); it != m_vs.end() ; it++) glDeleteProgram(it->second);
	for (auto it = m_gs.begin(); it != m_gs.end() ; it++) glDeleteProgram(it->second);
	for (auto it = m_ps.begin(); it != m_ps.end() ; it++) glDeleteProgram(it->second);
	for (auto it = m_ps_ss.begin(); it != m_ps_ss.end() ; it++) glDeleteSamplers(1, &it->second);
	m_vs.clear();
	m_gs.clear();
	m_ps.clear();
	m_ps_ss.clear();
	m_om_dss.clear();
	m_om_bs.clear();
}

GSTexture* GSDeviceOGL::CreateSurface(int type, int w, int h, bool msaa, int format)
{
	// A wrapper to call GSTextureOGL, with the different kind of parameter
	GSTextureOGL* t = NULL;
	t = new GSTextureOGL(type, w, h, msaa, format, m_fbo_read);

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
		// FIXME......
		// GLEW's problem is that it calls glGetString(GL_EXTENSIONS) which causes GL_INVALID_ENUM on GL 3.2 forward compatible context as soon as glewInit() is called. It also doesn't fetch the function pointers. The solution is for GLEW to use glGetStringi instead.
		// The current version of GLEW is 1.7.0 but they still haven't corrected it. The only fix is to use glewExperimental for now :
		//NOTE: I'm not sure experimental work on 1.6 ...
		glewExperimental=true;
		const int glew_ok = glewInit();
		if (glew_ok != GLEW_OK)
		{
			// FIXME:proper logging
			fprintf(stderr, "Error: Failed to init glew :%s\n", glewGetErrorString(glew_ok));
			return false;
		}
		// Note: don't rely on glew to avoid to pull glew1.7
		// Instead we just copy/adapt the 10 lines of code
		// if (!GLEW_VERSION_4_2) {
		// 	fprintf(stderr, "4.2 is not supported!\n");
		// 	return false;
		// }
		const GLubyte* s;
		s = glGetString(GL_VERSION);
		if (s == NULL) return false;

		GLuint dot = 0;
		while (s[dot] != '\0' && s[dot] != '.') dot++;
		if (dot == 0) return false;

		GLuint major = s[dot-1]-'0';
		GLuint minor = s[dot+1]-'0';
		// Note: 4.2 crash on latest nvidia drivers!
		// So only check 4.1
		// if ( (major < 4) || ( major == 4 && minor < 2 ) ) return false;
		// if ( (major < 4) || ( major == 4 && minor < 1 ) ) return false;
		if ( (major < 3) || ( major == 3 && minor < 3 ) ) {
			fprintf(stderr, "OPENGL 3.3 is not supported\n");
			return false;
		}

		if ( !glewIsSupported("GL_ARB_separate_shader_objects")) {
			fprintf(stderr, "GL_ARB_separate_shader_objects is not supported\n");
			return false;
		}
		if ( !glewIsSupported("GL_ARB_shading_language_420pack")) {
			fprintf(stderr, "GL_ARB_shading_language_420pack is not supported\n");
			//return false;
		}


	}

	// FIXME disable it when code is ready
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);

	m_window = wnd;

	// ****************************************************************
	// Various object
	// ****************************************************************
	glGenProgramPipelines(1, &m_pipeline);
	glBindProgramPipeline(m_pipeline);

	glGenFramebuffers(1, &m_fbo);
	glGenFramebuffers(1, &m_fbo_read);

	// ****************************************************************
	// Vertex buffer state
	// ****************************************************************
	GSInputLayoutOGL il_convert[2] =
	{
		{0, 4, GL_FLOAT, GL_FALSE, sizeof(GSVertexPT1), (const GLvoid*)offsetof(struct GSVertexPT1, p) },
		{1, 2, GL_FLOAT, GL_FALSE, sizeof(GSVertexPT1), (const GLvoid*)offsetof(struct GSVertexPT1, t) },
	};
	m_vb_sr = new GSVertexBufferStateOGL(sizeof(GSVertexPT1), il_convert, countof(il_convert));

	// ****************************************************************
	// convert
	// ****************************************************************
	CompileShaderFromSource("convert.glsl", "vs_main", GL_VERTEX_SHADER, &m_convert.vs);
	CompileShaderFromSource("convert.glsl", "gs_main", GL_GEOMETRY_SHADER, &m_convert.gs);
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
	// FIXME: need ogl extension sd.MaxAnisotropy = 16;


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
	// FIXME: need ogl extension sd.MaxAnisotropy = 16;

	m_convert.dss = new GSDepthStencilOGL();
	m_convert.bs  = new GSBlendStateOGL();

	// ****************************************************************
	// merge
	// ****************************************************************
	m_merge_obj.cb = new GSUniformBufferOGL(1, sizeof(MergeConstantBuffer));

	for(int i = 0; i < countof(m_merge_obj.ps); i++)
		CompileShaderFromSource("merge.glsl", format("ps_main%d", i), GL_FRAGMENT_SHADER, &m_merge_obj.ps[i]);

	m_merge_obj.bs = new GSBlendStateOGL();
	m_merge_obj.bs->EnableBlend();
	m_merge_obj.bs->SetRGB(GL_FUNC_ADD, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// ****************************************************************
	// interlace
	// ****************************************************************
	m_interlace.cb = new GSUniformBufferOGL(2, sizeof(InterlaceConstantBuffer));

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
#ifdef ONLY_LINES
	glLineWidth(5.0);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
#endif
	// Hum I don't know for those options but let's hope there are not activated
#if 0
	rd.FrontCounterClockwise = false;
	rd.DepthBias = false;
	rd.DepthBiasClamp = 0;
	rd.SlopeScaledDepthBias = 0;
	rd.DepthClipEnable = false; // ???
	rd.AntialiasedLineEnable = false;
#endif

	// TODO Later
	// ****************************************************************
	// fxaa (bonus)
	// ****************************************************************
	// FIXME need to define FXAA_GLSL_130 for the shader
	// FIXME need to manually set the index...
	// FIXME need dofxaa interface too
	// m_fxaa.cb = new GSUniformBufferOGL(3, sizeof(FXAAConstantBuffer));
	//CompileShaderFromSource("fxaa.fx", format("ps_main", i), GL_FRAGMENT_SHADER, &m_fxaa.ps);

	// ****************************************************************
	// date
	// ****************************************************************

	m_date.dss = new GSDepthStencilOGL();
	m_date.dss->EnableStencil();
	m_date.dss->SetStencil(GL_ALWAYS, GL_REPLACE);
	//memset(&dsd, 0, sizeof(dsd));

	//dsd.DepthEnable = false;
	//dsd.StencilEnable = true;
	//dsd.StencilReadMask = 1;
	//dsd.StencilWriteMask = 1;

	//dsd.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	//dsd.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
	//dsd.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	//dsd.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;

	//m_dev->CreateDepthStencilState(&dsd, &m_date.dss);

	// FIXME are the blend state really empty
	m_date.bs = new GSBlendStateOGL();
	//D3D11_BLEND_DESC blend;

	//memset(&blend, 0, sizeof(blend));

	//m_dev->CreateBlendState(&blend, &m_date.bs);

	// ****************************************************************
	// HW renderer shader
	// ****************************************************************
	CreateTextureFX();

	// ****************************************************************
	// Finish window setup and backbuffer
	// ****************************************************************
	if(!GSDevice::Create(wnd))
		return false;

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

	return true;
}

bool GSDeviceOGL::Reset(int w, int h)
{
	if(!GSDevice::Reset(w, h))
		return false;

	// TODO
	// Opengl allocate the backbuffer with the window. The render is done in the backbuffer when
	// there isn't any FBO. Only a dummy texture is created to easily detect when the rendering is done
	// in the backbuffer
	m_backbuffer = new GSTextureOGL(GSTextureOGL::Backbuffer, w, h, false, 0, m_fbo_read);

	return true;
}

void GSDeviceOGL::Flip()
{
	// FIXME: disable it when code is working
	CheckDebugLog();

	m_wnd->Flip();

#ifdef PRINT_FRAME_NUMBER
	fprintf(stderr, "Draw %d (Frame %d)\n", g_draw_count, g_frame_count);
#endif
#ifdef OGL_DEBUG
	if (theApp.GetConfig("debug_ogl_dump", 0) != 0)
		g_frame_count++;
#endif
}

void GSDeviceOGL::DebugBB()
{
	bool dump_me = false;
	uint32 start = theApp.GetConfig("debug_ogl_dump", 0);
	uint32 length = theApp.GetConfig("debug_ogl_dump_length", 5);
	if ( (start != 0 && g_frame_count >= start && g_frame_count < (start + length)) ) dump_me = true;

	if (!dump_me) return;

	GLuint fbo_old = m_state.fbo;
	OMSetFBO(m_fbo);

	GSVector2i size = m_backbuffer->GetSize();
	GSTexture* rt = CreateRenderTarget(size.x, size.y, false);

	static_cast<GSTextureOGL*>(rt)->Attach(GL_COLOR_ATTACHMENT0);

	glBlitFramebuffer(0, 0, size.x, size.y,
			0, 0, size.x, size.y,
			GL_COLOR_BUFFER_BIT, GL_NEAREST);

	rt->Save(format("/tmp/out_f%d__d%d__bb.bmp", g_frame_count, g_draw_count));

	delete rt;
	OMSetFBO(fbo_old);
}

void GSDeviceOGL::DebugInput()
{
	bool dump_me = false;
	uint32 start = theApp.GetConfig("debug_ogl_dump", 0);
	uint32 length = theApp.GetConfig("debug_ogl_dump_length", 5);
	if ( (start != 0 && g_frame_count >= start && g_frame_count < (start + length)) ) dump_me = true;

	if (!dump_me) return;

	for (auto i = 0 ; i < 3 ; i++) {
		if (m_state.ps_srv[i] != NULL) {
			m_state.ps_srv[i]->Save(format("/tmp/in_f%d__d%d__%d.bmp", g_frame_count, g_draw_count, i));
		}
	}
	//if (m_state.rtv != NULL) m_state.rtv->Save(format("/tmp/target_f%d__d%d__tex.bmp", g_frame_count, g_draw_count));
	//if (m_state.dsv != NULL) m_state.dsv->Save(format("/tmp/ds_in_%d.bmp", g_draw_count));

	fprintf(stderr, "Draw %d (Frame %d)\n", g_draw_count, g_frame_count);
	fprintf(stderr, "vs: %d ; gs: %d ; ps: %d\n", m_state.vs, m_state.gs, m_state.ps);
	m_state.vb->debug();
	m_state.bs->debug();
	m_state.dss->debug();
}

void GSDeviceOGL::DebugOutput()
{
	CheckDebugLog();

	bool dump_me = false;
	uint32 start = theApp.GetConfig("debug_ogl_dump", 0);
	uint32 length = theApp.GetConfig("debug_ogl_dump_length", 5);
	if ( (start != 0 && g_frame_count >= start && g_frame_count < (start + length)) ) dump_me = true;

	if (!dump_me) return;

	if (m_state.rtv == m_backbuffer) {
		m_state.rtv->Save(format("/tmp/out_f%d__d%d__back.bmp", g_frame_count, g_draw_count));
	} else {
		if (m_state.rtv != NULL) m_state.rtv->Save(format("/tmp/out_f%d__d%d__tex.bmp", g_frame_count, g_draw_count));
	}
	//if (m_state.dsv != NULL) m_state.dsv->Save(format("/tmp/ds_out_%d.bmp", g_draw_count));

	fprintf(stderr, "\n");
	//DebugBB();
}

void GSDeviceOGL::DrawPrimitive()
{
#ifdef OGL_DEBUG
	DebugInput();
#endif

	m_state.vb->DrawPrimitive();

#ifdef OGL_DEBUG
	DebugOutput();
	g_draw_count++;
#endif
}

void GSDeviceOGL::DrawIndexedPrimitive()
{
#ifdef OGL_DEBUG
	DebugInput();
#endif

	m_state.vb->DrawIndexedPrimitive();

#ifdef OGL_DEBUG
	DebugOutput();
	g_draw_count++;
#endif
}

void GSDeviceOGL::DrawIndexedPrimitive(int offset, int count)
{
	ASSERT(offset + count <= m_index.count);
#ifdef OGL_DEBUG
	DebugInput();
#endif

	m_state.vb->DrawIndexedPrimitive(offset, count);

#ifdef OGL_DEBUG
	DebugOutput();
	g_draw_count++;
#endif
}

void GSDeviceOGL::ClearRenderTarget(GSTexture* t, const GSVector4& c)
{
	GLuint fbo_old = m_state.fbo;
	if (static_cast<GSTextureOGL*>(t)->IsBackbuffer()) {
		// FIXME I really not sure
		OMSetFBO(0);
		//glClearBufferfv(GL_COLOR, GL_LEFT, c.v);
		glClearBufferfv(GL_COLOR, 0, c.v);
		// code for the old interface
		// glClearColor(c.x, c.y, c.z, c.w);
		// glClear(GL_COLOR_BUFFER_BIT);
	} else {
		// FIXME1 I need to clarify this FBO attachment stuff
		// I would like to avoid FBO for a basic clean operation
		OMSetFBO(m_fbo);
		static_cast<GSTextureOGL*>(t)->Attach(GL_COLOR_ATTACHMENT0);
		glClearBufferfv(GL_COLOR, 0, c.v);
	}
	OMSetFBO(fbo_old);
}

void GSDeviceOGL::ClearRenderTarget(GSTexture* t, uint32 c)
{
	GSVector4 color = GSVector4::rgba32(c) * (1.0f / 255);
	ClearRenderTarget(t, color);
}

void GSDeviceOGL::ClearDepth(GSTexture* t, float c)
{
	GLuint fbo_old = m_state.fbo;
	// FIXME I need to clarify this FBO attachment stuff
	// I would like to avoid FBO for a basic clean operation
	OMSetFBO(m_fbo);
	static_cast<GSTextureOGL*>(t)->Attach(GL_DEPTH_STENCIL_ATTACHMENT);
	// FIXME can you clean depth and stencil separately
	glClearBufferfv(GL_DEPTH, 0, &c);
	OMSetFBO(fbo_old);
}

void GSDeviceOGL::ClearStencil(GSTexture* t, uint8 c)
{
	GLuint fbo_old = m_state.fbo;
	// FIXME I need to clarify this FBO attachment stuff
	// I would like to avoid FBO for a basic clean operation
	OMSetFBO(m_fbo);
	static_cast<GSTextureOGL*>(t)->Attach(GL_DEPTH_STENCIL_ATTACHMENT);
	GLint color = c;
	// FIXME can you clean depth and stencil separately
	glClearBufferiv(GL_STENCIL, 0, &color);
	OMSetFBO(fbo_old);
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
	GSTexture* dst = NULL;

	if(format == 0) format = GL_RGBA8;

	if(format != GL_RGBA8 && format != GL_R16UI)
	{
		ASSERT(0);

		return false;
	}

	// FIXME: It is possible to bypass completely offscreen-buffer on opengl but it needs some re-thinking of the code.
	// For the moment mimic dx11 
	GSTexture* rt = CreateRenderTarget(w, h, false, format);
	if(rt)
	{
		GSVector4 dr(0, 0, w, h);

		if(GSTexture* src2 = src->IsMSAA() ? Resolve(src) : src)
		{
			StretchRect(src2, sr, rt, dr, m_convert.ps[format == GL_R16UI ? 1 : 0]);

			if(src2 != src) Recycle(src2);
		}


		GSVector4i dor(0, 0, w, h);
		dst = CreateOffscreen(w, h, format);

		if (dst) CopyRect(rt, dst, dor);
#if 0
		if(dst)
		{
			m_ctx->CopyResource(*(GSTexture11*)dst, *(GSTexture11*)rt);
		}
#endif

		Recycle(rt);
	}

	return dst;
	//return rt;
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

	// GL_NV_copy_image seem like the good extension but not supported on AMD...
	// Maybe opengl 4.3 !
	// FIXME check those function work as expected
	// FIXME: it is an NVIDIA extension. Hopefully lastest AMD driver support it too.
	// An EXT extensions might be release later.
	// void CopyImageSubDataNV(
    //         uint srcName, enum srcTarget, int srcLevel, int srcX, int srcY, int srcZ,
	//     uint dstName, enum dstTarget, int dstLevel, int dstX, int dstY, int dstZ,
	//     sizei width, sizei height, sizei depth);
	glCopyImageSubDataNV( static_cast<GSTextureOGL*>(st)->GetID(), static_cast<GSTextureOGL*>(st)->GetTarget(), 
			0, r.x, r.y, 0,
			static_cast<GSTextureOGL*>(dt)->GetID(), static_cast<GSTextureOGL*>(dt)->GetTarget(),
			0, r.x, r.y, 0,
			r.width(), r.height(), 1);

#if 0
	D3D11_BOX box = {r.left, r.top, 0, r.right, r.bottom, 1};
	m_ctx->CopySubresourceRegion(*(GSTexture11*)dt, 0, 0, 0, 0, *(GSTexture11*)st, 0, &box);
#endif
}

void GSDeviceOGL::StretchRect(GSTexture* st, const GSVector4& sr, GSTexture* dt, const GSVector4& dr, int shader, bool linear)
{
	StretchRect(st, sr, dt, dr, m_convert.ps[shader], linear);
}

void GSDeviceOGL::StretchRect(GSTexture* st, const GSVector4& sr, GSTexture* dt, const GSVector4& dr, GLuint ps, bool linear)
{
	StretchRect(st, sr, dt, dr, ps, m_convert.bs, linear);
}

void GSDeviceOGL::StretchRect(GSTexture* st, const GSVector4& sr, GSTexture* dt, const GSVector4& dr, GLuint ps, GSBlendStateOGL* bs, bool linear)
{

	if(!st || !dt)
	{
		ASSERT(0);
		return;
	}

	// ************************************
	// Init
	// ************************************

	BeginScene();

	GSVector2i ds = dt->GetSize();

	// ************************************
	// om
	// ************************************

	OMSetDepthStencilState(m_convert.dss, 0);
	OMSetBlendState(bs, 0);
	OMSetRenderTargets(dt, NULL);

	// ************************************
	// ia
	// ************************************


	float left = dr.x * 2 / ds.x - 1.0f;
	float top = 1.0f - dr.y * 2 / ds.y;
	float right = dr.z * 2 / ds.x - 1.0f;
	float bottom = 1.0f - dr.w * 2 / ds.y;

	// Flip y axis only when we render in the backbuffer
	// By default everything is render in the wrong order (ie dx).
	// 1/ consistency between several pass rendering (interlace)
	// 2/ in case some GSdx code expect thing in dx order.
	// Only flipping the backbuffer is transparent (I hope)...
	GSVector4 flip_sr = sr;
	if (static_cast<GSTextureOGL*>(dt)->IsBackbuffer()) {
		flip_sr.y = 1.0f - sr.y;
		flip_sr.w = 1.0f - sr.w;
	}

	GSVertexPT1 vertices[] =
	{
		{GSVector4(left, bottom, 0.5f, 1.0f), GSVector2(flip_sr.x, flip_sr.y)},
		{GSVector4(right, bottom, 0.5f, 1.0f), GSVector2(flip_sr.z, flip_sr.y)},
		{GSVector4(left, top, 0.5f, 1.0f), GSVector2(flip_sr.x, flip_sr.w)},
		{GSVector4(right, top, 0.5f, 1.0f), GSVector2(flip_sr.z, flip_sr.w)},
	};
	//fprintf(stderr, "A:%fx%f B:%fx%f\n", left, top, bottom, right);
	//fprintf(stderr, "SR: %f %f %f %f\n", sr.x, sr.y, sr.z, sr.w);

	IASetVertexState(m_vb_sr);
	IASetVertexBuffer(vertices, 4);
	IASetPrimitiveTopology(GL_TRIANGLE_STRIP);

	// ************************************
	// vs
	// ************************************

	VSSetShader(m_convert.vs);

	// ************************************
	// gs
	// ************************************

#ifdef AMD_DRIVER_WORKAROUND
	GSSetShader(m_convert.gs);
#else
	GSSetShader(0);
#endif

	// ************************************
	// ps
	// ************************************

	PSSetShaderResources(st, NULL);
	PSSetSamplerState(linear ? m_convert.ln : m_convert.pt, 0);
	PSSetShader(ps);

	// ************************************
	// Draw
	// ************************************
	DrawPrimitive();

	// ************************************
	// End
	// ************************************

	EndScene();

	PSSetShaderResources(NULL, NULL);
}

void GSDeviceOGL::DoMerge(GSTexture* st[2], GSVector4* sr, GSTexture* dt, GSVector4* dr, bool slbg, bool mmod, const GSVector4& c)
{
	ClearRenderTarget(dt, c);

	if(st[1] && !slbg)
	{
		StretchRect(st[1], sr[1], dt, dr[1], m_merge_obj.ps[0]);
	}

	if(st[0])
	{
		SetUniformBuffer(m_merge_obj.cb);
		m_merge_obj.cb->upload(&c.v);

		StretchRect(st[0], sr[0], dt, dr[0], m_merge_obj.ps[mmod ? 1 : 0], m_merge_obj.bs);
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

	SetUniformBuffer(m_interlace.cb);
	m_interlace.cb->upload(&cb);

	StretchRect(st, sr, dt, dr, m_interlace.ps[shader], linear);
}

void GSDeviceOGL::SetupDATE(GSTexture* rt, GSTexture* ds, const GSVertexPT1* vertices, bool datm)
{
	const GSVector2i& size = rt->GetSize();

	if(GSTexture* t = CreateRenderTarget(size.x, size.y, rt->IsMSAA()))
	{
		// sfex3 (after the capcom logo), vf4 (first menu fading in), ffxii shadows, rumble roses shadows, persona4 shadows

		BeginScene();

		ClearStencil(ds, 0);

		// om

		OMSetDepthStencilState(m_date.dss, 1);
		OMSetBlendState(m_date.bs, 0);
		OMSetRenderTargets(t, ds);

		// ia

		IASetVertexState(m_vb_sr);
		IASetVertexBuffer(vertices, 4);
		IASetPrimitiveTopology(GL_TRIANGLE_STRIP);

		// vs

		VSSetShader(m_convert.vs);

		// gs

#ifdef AMD_DRIVER_WORKAROUND
		GSSetShader(m_convert.gs);
#else
		GSSetShader(0);
#endif


		// ps

		GSTexture* rt2 = rt->IsMSAA() ? Resolve(rt) : rt;

		PSSetShaderResources(rt2, NULL);
		PSSetSamplerState(m_convert.pt, 0);
		PSSetShader(m_convert.ps[datm ? 2 : 3]);

		//

		DrawPrimitive();

		//

		EndScene();

		Recycle(t);

		if(rt2 != rt) Recycle(rt2);
	}
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

void GSDeviceOGL::EndScene()
{
	m_state.vb->EndScene();
}

void GSDeviceOGL::SetUniformBuffer(GSUniformBufferOGL* cb)
{
	if (m_state.cb != cb) {
		 m_state.cb = cb;
		 cb->bind();
	}
}

void GSDeviceOGL::IASetVertexState(GSVertexBufferStateOGL* vb)
{
	if (vb == NULL) vb = m_vb;

	if (m_state.vb != vb) {
		m_state.vb = vb;
		vb->bind();
	}
}

void GSDeviceOGL::IASetVertexBuffer(const void* vertices, size_t count)
{
	m_state.vb->UploadVB(vertices, count);
}

bool GSDeviceOGL::IAMapVertexBuffer(void** vertex, size_t stride, size_t count)
{
	return m_state.vb->MapVB(vertex, count);
}

void GSDeviceOGL::IAUnmapVertexBuffer()
{
	m_state.vb->UnmapVB();
}

void GSDeviceOGL::IASetIndexBuffer(const void* index, size_t count)
{
	m_state.vb->UploadIB(index, count);
}

void GSDeviceOGL::IASetPrimitiveTopology(GLenum topology)
{
	m_state.vb->SetTopology(topology);
}

void GSDeviceOGL::VSSetShader(GLuint vs)
{
	if(m_state.vs != vs)
	{
		m_state.vs = vs;
		glUseProgramStages(m_pipeline, GL_VERTEX_SHADER_BIT, vs);
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
	//PSSetShaderResource(2, NULL);
}

void GSDeviceOGL::PSSetShaderResource(int i, GSTexture* sr)
{
	GSTextureOGL* srv = static_cast<GSTextureOGL*>(sr);

	if(m_state.ps_srv[i] != srv)
	{
		m_state.ps_srv[i] = srv;

		m_srv_changed = true;
	}
}

void GSDeviceOGL::PSSetSamplerState(GLuint ss0, GLuint ss1, GLuint ss2)
{
	if(m_state.ps_ss[0] != ss0 || m_state.ps_ss[1] != ss1)
	//if(m_state.ps_ss[0] != ss0 || m_state.ps_ss[1] != ss1 || m_state.ps_ss[2] != ss2)
	{
		m_state.ps_ss[0] = ss0;
		m_state.ps_ss[1] = ss1;
		//m_state.ps_ss[2] = ss2;

		m_ss_changed = true;
	}
}

void GSDeviceOGL::PSSetShader(GLuint ps)
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
		for (uint i=0 ; i < 1; i++) {
			if (m_state.ps_srv[i] != NULL) {
				m_state.ps_srv[i]->EnableUnit(i);
				glBindSampler(i, m_state.ps_ss[i]);
			}
		}
	}
}

void GSDeviceOGL::OMSetFBO(GLuint fbo, GLenum buffer)
{
	if (m_state.fbo != fbo) {
		m_state.fbo = fbo;
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		// FIXME DEBUG
		//if (fbo) fprintf(stderr, "FB status %x\n", glCheckFramebufferStatus(GL_FRAMEBUFFER));
	}

	if (m_state.draw != buffer) {
		m_state.draw = buffer;
		glDrawBuffer(buffer);
	}

}

void GSDeviceOGL::OMSetDepthStencilState(GSDepthStencilOGL* dss, uint8 sref)
{
	uint ref = sref;

	if(m_state.dss != dss) {
		m_state.dss = dss;
		m_state.sref = sref;

		dss->SetupDepth();
		dss->SetupStencil(sref);

	} else if (m_state.sref != sref) {
		m_state.sref = sref;

		dss->SetupStencil(sref);
	}
}

void GSDeviceOGL::OMSetBlendState(GSBlendStateOGL* bs, float bf)
{
	if( m_state.bs != bs || (m_state.bf != bf && bs->HasConstantFactor()) )
	{
		m_state.bs = bs;
		m_state.bf = bf;

		bs->SetupBlend(bf);
	}
}

void GSDeviceOGL::OMSetRenderTargets(GSTexture* rt, GSTexture* ds, const GSVector4i* scissor)
{
	// Hum, need to separate 2 case, Render target fbo and render target backbuffer
	// Or maybe blit final result to the backbuffer
	m_state.rtv = static_cast<GSTextureOGL*>(rt);
	m_state.dsv = static_cast<GSTextureOGL*>(ds);

	if (static_cast<GSTextureOGL*>(rt)->IsBackbuffer()) {
		assert(ds == NULL); // no depth-stencil without FBO

		OMSetFBO(0);

	} else {
		assert(rt != NULL); // a render target must exists

		// FIXME DEBUG special case for GL_R16UI
		if (rt->GetFormat() == GL_R16UI) {
			OMSetFBO(m_fbo, GL_COLOR_ATTACHMENT1);
			static_cast<GSTextureOGL*>(rt)->Attach(GL_COLOR_ATTACHMENT1);
		} else {
			OMSetFBO(m_fbo, GL_COLOR_ATTACHMENT0);
			static_cast<GSTextureOGL*>(rt)->Attach(GL_COLOR_ATTACHMENT0);
		}

		if (ds != NULL)
			static_cast<GSTextureOGL*>(ds)->Attach(GL_DEPTH_STENCIL_ATTACHMENT);
	}

	if(m_state.viewport != rt->GetSize())
	{
		m_state.viewport = rt->GetSize();
		glViewport(0, 0, rt->GetWidth(), rt->GetHeight());
	}

	GSVector4i r = scissor ? *scissor : GSVector4i(rt->GetSize()).zwxy();

	if(!m_state.scissor.eq(r))
	{
		m_state.scissor = r;
		glScissor( r.x, r.y, r.width(), r.height() );
	}
}

void GSDeviceOGL::CompileShaderFromSource(const std::string& glsl_file, const std::string& entry, GLenum type, GLuint* program, const std::string& macro_sel)
{
	// *****************************************************
	// Build a header string
	// *****************************************************
	// First select the version (must be the first line so we need to generate it
	std::string version = "#version 330\n#extension GL_ARB_shading_language_420pack: require\n#extension GL_ARB_separate_shader_objects : require\n";
	//std::string version = "#version 420\n";

	// Allow to puts several shader in 1 files
	std::string shader_type;
	switch (type) {
		case GL_VERTEX_SHADER:
			shader_type = "#define VERTEX_SHADER 1\n";
			break;
		case GL_GEOMETRY_SHADER:
			shader_type = "#define GEOMETRY_SHADER 1\n";
			break;
		case GL_FRAGMENT_SHADER:
			shader_type = "#define FRAGMENT_SHADER 1\n";
			break;
		default: assert(0);
	}

	// Select the entry point ie the main function
	std::string entry_main = format("#define %s main\n", entry.c_str());

	std::string header = version + shader_type + entry_main + macro_sel;
#ifdef DISABLE_DUAL_BLEND
	header += "#define DISABLE_DUAL_BLEND 1\n";
#endif

	// *****************************************************
	// Read the source file
	// *****************************************************
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


	// Note it is better to separate header and source file to have the good line number
	// in the glsl compiler report
	const char** sources_array = (const char**)malloc(2*sizeof(char*));
	char* source_str = (char*)malloc(source.size() + 1);
	char* header_str = (char*)malloc(header.size() + 1);
	sources_array[0] = header_str;
	sources_array[1] = source_str;

	source.copy(source_str, source.size(), 0);
	source_str[source.size()] = '\0';
	header.copy(header_str, header.size(), 0);
	header_str[header.size()] = '\0';

	*program = glCreateShaderProgramv(type, 2, sources_array);
	free(source_str);
	free(header_str);
	free(sources_array);

	if (theApp.GetConfig("debug_ogl_shader", 1) == 1) {
		// Print a nice debug log
		GLint log_length = 0;
		glGetProgramiv(*program, GL_INFO_LOG_LENGTH, &log_length);

		char* log = (char*)malloc(log_length);
		glGetProgramInfoLog(*program, log_length, NULL, log);

		fprintf(stderr, "%s (entry %s, prog %d) :", glsl_file.c_str(), entry.c_str(), *program);
		fprintf(stderr, "\n%s", macro_sel.c_str());
		fprintf(stderr, "%s\n", log);
		free(log);
	}
}

void GSDeviceOGL::CheckDebugLog()
{
       unsigned int count = 64; // max. num. of messages that will be read from the log
       int bufsize = 2048;
       unsigned int* sources      = new unsigned int[count];
       unsigned int* types        = new unsigned int[count];
       unsigned int* ids   = new unsigned int[count];
       unsigned int* severities = new unsigned int[count];
       int* lengths = new int[count];
       char* messageLog = new char[bufsize];

       unsigned int retVal = glGetDebugMessageLogARB(count, bufsize, sources, types, ids, severities, lengths, messageLog);

       if(retVal > 0)
       {
             unsigned int pos = 0;
             for(unsigned int i=0; i<retVal; i++)
             {
                    DebugOutputToFile(sources[i], types[i], ids[i], severities[i],
 &messageLog[pos]);
                    pos += lengths[i];
              }
       }

       delete [] sources;
       delete [] types;
       delete [] ids;
       delete [] severities;
       delete [] lengths;
       delete [] messageLog;
}

void GSDeviceOGL::DebugOutputToFile(unsigned int source, unsigned int type, unsigned int id, unsigned int severity, const char* message)
{
	char debType[20], debSev[5];
	static int sev_counter = 0;

	if(type == GL_DEBUG_TYPE_ERROR_ARB)
		strcpy(debType, "Error");
	else if(type == GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB)
		strcpy(debType, "Deprecated behavior");
	else if(type == GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB)
		strcpy(debType, "Undefined behavior");
	else if(type == GL_DEBUG_TYPE_PORTABILITY_ARB)
		strcpy(debType, "Portability");
	else if(type == GL_DEBUG_TYPE_PERFORMANCE_ARB)
		strcpy(debType, "Performance");
	else if(type == GL_DEBUG_TYPE_OTHER_ARB)
		strcpy(debType, "Other");
	else
		strcpy(debType, "UNKNOWN");

	if(severity == GL_DEBUG_SEVERITY_HIGH_ARB) {
		strcpy(debSev, "High");
		sev_counter++;
	}
	else if(severity == GL_DEBUG_SEVERITY_MEDIUM_ARB)
		strcpy(debSev, "Med");
	else if(severity == GL_DEBUG_SEVERITY_LOW_ARB)
		strcpy(debSev, "Low");

	#ifdef LOUD_DEBUGGING
	fprintf(stderr,"Type:%s\tID:%d\tSeverity:%s\tMessage:%s\n", debType, g_draw_count, debSev,message);
	#endif

	FILE* f = fopen("Debug.txt","a");
	if(f)
	{
		fprintf(f,"Type:%s\tID:%d\tSeverity:%s\tMessage:%s\n", debType, g_draw_count, debSev,message);
		fclose(f);
	}
	//if (sev_counter > 2) assert(0);
}

// (A - B) * C + D
// A: Cs/Cd/0
// B: Cs/Cd/0
// C: As/Ad/FIX
// D: Cs/Cd/0

// bogus: 0100, 0110, 0120, 0200, 0210, 0220, 1001, 1011, 1021
// tricky: 1201, 1211, 1221

// Source.rgb = float3(1, 1, 1);
// 1201 Cd*(1 + As) => Source * Dest color + Dest * Source alpha
// 1211 Cd*(1 + Ad) => Source * Dest color + Dest * Dest alpha
// 1221 Cd*(1 + F) => Source * Dest color + Dest * Factor

// Copy Dx blend table and convert it to ogl
#define D3DBLENDOP_ADD			GL_FUNC_ADD
#define D3DBLENDOP_SUBTRACT		GL_FUNC_SUBTRACT
#define D3DBLENDOP_REVSUBTRACT	GL_FUNC_REVERSE_SUBTRACT

#define D3DBLEND_ONE			GL_ONE
#define D3DBLEND_ZERO			GL_ZERO
#define D3DBLEND_INVDESTALPHA	GL_ONE_MINUS_DST_ALPHA
#define D3DBLEND_DESTALPHA		GL_DST_ALPHA
#define D3DBLEND_DESTCOLOR		GL_DST_COLOR
#define D3DBLEND_BLENDFACTOR	GL_CONSTANT_COLOR
#define D3DBLEND_INVBLENDFACTOR GL_ONE_MINUS_CONSTANT_COLOR

#ifdef DISABLE_DUAL_BLEND
	#define D3DBLEND_SRCALPHA		GL_SRC_ALPHA
	#define D3DBLEND_INVSRCALPHA	GL_ONE_MINUS_SRC_ALPHA
#else
	#define D3DBLEND_SRCALPHA		GL_SRC1_ALPHA
	#define D3DBLEND_INVSRCALPHA	GL_ONE_MINUS_SRC1_ALPHA
#endif
                        
const GSDeviceOGL::D3D9Blend GSDeviceOGL::m_blendMapD3D9[3*3*3*3] =
{
	{0, D3DBLENDOP_ADD, D3DBLEND_ONE, D3DBLEND_ZERO},						// 0000: (Cs - Cs)*As + Cs ==> Cs
	{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ONE},						// 0001: (Cs - Cs)*As + Cd ==> Cd
	{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ZERO},						// 0002: (Cs - Cs)*As +  0 ==> 0
	{0, D3DBLENDOP_ADD, D3DBLEND_ONE, D3DBLEND_ZERO},						// 0010: (Cs - Cs)*Ad + Cs ==> Cs
	{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ONE},						// 0011: (Cs - Cs)*Ad + Cd ==> Cd
	{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ZERO},						// 0012: (Cs - Cs)*Ad +  0 ==> 0
	{0, D3DBLENDOP_ADD, D3DBLEND_ONE, D3DBLEND_ZERO},						// 0020: (Cs - Cs)*F  + Cs ==> Cs
	{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ONE},						// 0021: (Cs - Cs)*F  + Cd ==> Cd
	{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ZERO},						// 0022: (Cs - Cs)*F  +  0 ==> 0
	{1, D3DBLENDOP_SUBTRACT, D3DBLEND_SRCALPHA, D3DBLEND_SRCALPHA},			//*0100: (Cs - Cd)*As + Cs ==> Cs*(As + 1) - Cd*As
	{0, D3DBLENDOP_ADD, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA},			// 0101: (Cs - Cd)*As + Cd ==> Cs*As + Cd*(1 - As)
	{0, D3DBLENDOP_SUBTRACT, D3DBLEND_SRCALPHA, D3DBLEND_SRCALPHA},			// 0102: (Cs - Cd)*As +  0 ==> Cs*As - Cd*As
	{1, D3DBLENDOP_SUBTRACT, D3DBLEND_DESTALPHA, D3DBLEND_DESTALPHA},		//*0110: (Cs - Cd)*Ad + Cs ==> Cs*(Ad + 1) - Cd*Ad
	{0, D3DBLENDOP_ADD, D3DBLEND_DESTALPHA, D3DBLEND_INVDESTALPHA},			// 0111: (Cs - Cd)*Ad + Cd ==> Cs*Ad + Cd*(1 - Ad)
	{0, D3DBLENDOP_SUBTRACT, D3DBLEND_DESTALPHA, D3DBLEND_DESTALPHA},		// 0112: (Cs - Cd)*Ad +  0 ==> Cs*Ad - Cd*Ad
	{1, D3DBLENDOP_SUBTRACT, D3DBLEND_BLENDFACTOR, D3DBLEND_BLENDFACTOR},	//*0120: (Cs - Cd)*F  + Cs ==> Cs*(F + 1) - Cd*F
	{0, D3DBLENDOP_ADD, D3DBLEND_BLENDFACTOR, D3DBLEND_INVBLENDFACTOR},		// 0121: (Cs - Cd)*F  + Cd ==> Cs*F + Cd*(1 - F)
	{0, D3DBLENDOP_SUBTRACT, D3DBLEND_BLENDFACTOR, D3DBLEND_BLENDFACTOR},	// 0122: (Cs - Cd)*F  +  0 ==> Cs*F - Cd*F
	{1, D3DBLENDOP_ADD, D3DBLEND_SRCALPHA, D3DBLEND_ZERO},					//*0200: (Cs -  0)*As + Cs ==> Cs*(As + 1)
	{0, D3DBLENDOP_ADD, D3DBLEND_SRCALPHA, D3DBLEND_ONE},					// 0201: (Cs -  0)*As + Cd ==> Cs*As + Cd
	{0, D3DBLENDOP_ADD, D3DBLEND_SRCALPHA, D3DBLEND_ZERO},					// 0202: (Cs -  0)*As +  0 ==> Cs*As
	{1, D3DBLENDOP_ADD, D3DBLEND_DESTALPHA, D3DBLEND_ZERO},					//*0210: (Cs -  0)*Ad + Cs ==> Cs*(Ad + 1)
	{0, D3DBLENDOP_ADD, D3DBLEND_DESTALPHA, D3DBLEND_ONE},					// 0211: (Cs -  0)*Ad + Cd ==> Cs*Ad + Cd
	{0, D3DBLENDOP_ADD, D3DBLEND_DESTALPHA, D3DBLEND_ZERO},					// 0212: (Cs -  0)*Ad +  0 ==> Cs*Ad
	{1, D3DBLENDOP_ADD, D3DBLEND_BLENDFACTOR, D3DBLEND_ZERO},				//*0220: (Cs -  0)*F  + Cs ==> Cs*(F + 1)
	{0, D3DBLENDOP_ADD, D3DBLEND_BLENDFACTOR, D3DBLEND_ONE},				// 0221: (Cs -  0)*F  + Cd ==> Cs*F + Cd
	{0, D3DBLENDOP_ADD, D3DBLEND_BLENDFACTOR, D3DBLEND_ZERO},				// 0222: (Cs -  0)*F  +  0 ==> Cs*F
	{0, D3DBLENDOP_ADD, D3DBLEND_INVSRCALPHA, D3DBLEND_SRCALPHA},			// 1000: (Cd - Cs)*As + Cs ==> Cd*As + Cs*(1 - As)
	{1, D3DBLENDOP_REVSUBTRACT, D3DBLEND_SRCALPHA, D3DBLEND_SRCALPHA},		//*1001: (Cd - Cs)*As + Cd ==> Cd*(As + 1) - Cs*As
	{0, D3DBLENDOP_REVSUBTRACT, D3DBLEND_SRCALPHA, D3DBLEND_SRCALPHA},		// 1002: (Cd - Cs)*As +  0 ==> Cd*As - Cs*As
	{0, D3DBLENDOP_ADD, D3DBLEND_INVDESTALPHA, D3DBLEND_DESTALPHA},			// 1010: (Cd - Cs)*Ad + Cs ==> Cd*Ad + Cs*(1 - Ad)
	{1, D3DBLENDOP_REVSUBTRACT, D3DBLEND_DESTALPHA, D3DBLEND_DESTALPHA},	//*1011: (Cd - Cs)*Ad + Cd ==> Cd*(Ad + 1) - Cs*Ad
	{0, D3DBLENDOP_REVSUBTRACT, D3DBLEND_DESTALPHA, D3DBLEND_DESTALPHA},	// 1012: (Cd - Cs)*Ad +  0 ==> Cd*Ad - Cs*Ad
	{0, D3DBLENDOP_ADD, D3DBLEND_INVBLENDFACTOR, D3DBLEND_BLENDFACTOR},		// 1020: (Cd - Cs)*F  + Cs ==> Cd*F + Cs*(1 - F)
	{1, D3DBLENDOP_REVSUBTRACT, D3DBLEND_BLENDFACTOR, D3DBLEND_BLENDFACTOR},//*1021: (Cd - Cs)*F  + Cd ==> Cd*(F + 1) - Cs*F
	{0, D3DBLENDOP_REVSUBTRACT, D3DBLEND_BLENDFACTOR, D3DBLEND_BLENDFACTOR},// 1022: (Cd - Cs)*F  +  0 ==> Cd*F - Cs*F
	{0, D3DBLENDOP_ADD, D3DBLEND_ONE, D3DBLEND_ZERO},						// 1100: (Cd - Cd)*As + Cs ==> Cs
	{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ONE},						// 1101: (Cd - Cd)*As + Cd ==> Cd
	{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ZERO},						// 1102: (Cd - Cd)*As +  0 ==> 0
	{0, D3DBLENDOP_ADD, D3DBLEND_ONE, D3DBLEND_ZERO},						// 1110: (Cd - Cd)*Ad + Cs ==> Cs
	{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ONE},						// 1111: (Cd - Cd)*Ad + Cd ==> Cd
	{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ZERO},						// 1112: (Cd - Cd)*Ad +  0 ==> 0
	{0, D3DBLENDOP_ADD, D3DBLEND_ONE, D3DBLEND_ZERO},						// 1120: (Cd - Cd)*F  + Cs ==> Cs
	{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ONE},						// 1121: (Cd - Cd)*F  + Cd ==> Cd
	{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ZERO},						// 1122: (Cd - Cd)*F  +  0 ==> 0
	{0, D3DBLENDOP_ADD, D3DBLEND_ONE, D3DBLEND_SRCALPHA},					// 1200: (Cd -  0)*As + Cs ==> Cs + Cd*As
	{2, D3DBLENDOP_ADD, D3DBLEND_DESTCOLOR, D3DBLEND_SRCALPHA},				//#1201: (Cd -  0)*As + Cd ==> Cd*(1 + As)  // ffxii main menu background glow effect
	{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_SRCALPHA},					// 1202: (Cd -  0)*As +  0 ==> Cd*As
	{0, D3DBLENDOP_ADD, D3DBLEND_ONE, D3DBLEND_DESTALPHA},					// 1210: (Cd -  0)*Ad + Cs ==> Cs + Cd*Ad
	{2, D3DBLENDOP_ADD, D3DBLEND_DESTCOLOR, D3DBLEND_DESTALPHA},			//#1211: (Cd -  0)*Ad + Cd ==> Cd*(1 + Ad)
	{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_DESTALPHA},					// 1212: (Cd -  0)*Ad +  0 ==> Cd*Ad
	{0, D3DBLENDOP_ADD, D3DBLEND_ONE, D3DBLEND_BLENDFACTOR},				// 1220: (Cd -  0)*F  + Cs ==> Cs + Cd*F
	{2, D3DBLENDOP_ADD, D3DBLEND_DESTCOLOR, D3DBLEND_BLENDFACTOR},			//#1221: (Cd -  0)*F  + Cd ==> Cd*(1 + F)
	{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_BLENDFACTOR},				// 1222: (Cd -  0)*F  +  0 ==> Cd*F
	{0, D3DBLENDOP_ADD, D3DBLEND_INVSRCALPHA, D3DBLEND_ZERO},				// 2000: (0  - Cs)*As + Cs ==> Cs*(1 - As)
	{0, D3DBLENDOP_REVSUBTRACT, D3DBLEND_SRCALPHA, D3DBLEND_ONE},			// 2001: (0  - Cs)*As + Cd ==> Cd - Cs*As
	{0, D3DBLENDOP_REVSUBTRACT, D3DBLEND_SRCALPHA, D3DBLEND_ZERO},			// 2002: (0  - Cs)*As +  0 ==> 0 - Cs*As
	{0, D3DBLENDOP_ADD, D3DBLEND_INVDESTALPHA, D3DBLEND_ZERO},				// 2010: (0  - Cs)*Ad + Cs ==> Cs*(1 - Ad)
	{0, D3DBLENDOP_REVSUBTRACT, D3DBLEND_DESTALPHA, D3DBLEND_ONE},			// 2011: (0  - Cs)*Ad + Cd ==> Cd - Cs*Ad
	{0, D3DBLENDOP_REVSUBTRACT, D3DBLEND_DESTALPHA, D3DBLEND_ZERO},			// 2012: (0  - Cs)*Ad +  0 ==> 0 - Cs*Ad
	{0, D3DBLENDOP_ADD, D3DBLEND_INVBLENDFACTOR, D3DBLEND_ZERO},			// 2020: (0  - Cs)*F  + Cs ==> Cs*(1 - F)
	{0, D3DBLENDOP_REVSUBTRACT, D3DBLEND_BLENDFACTOR, D3DBLEND_ONE},		// 2021: (0  - Cs)*F  + Cd ==> Cd - Cs*F
	{0, D3DBLENDOP_REVSUBTRACT, D3DBLEND_BLENDFACTOR, D3DBLEND_ZERO},		// 2022: (0  - Cs)*F  +  0 ==> 0 - Cs*F
	{0, D3DBLENDOP_SUBTRACT, D3DBLEND_ONE, D3DBLEND_SRCALPHA},				// 2100: (0  - Cd)*As + Cs ==> Cs - Cd*As
	{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_INVSRCALPHA},				// 2101: (0  - Cd)*As + Cd ==> Cd*(1 - As)
	{0, D3DBLENDOP_SUBTRACT, D3DBLEND_ZERO, D3DBLEND_SRCALPHA},				// 2102: (0  - Cd)*As +  0 ==> 0 - Cd*As
	{0, D3DBLENDOP_SUBTRACT, D3DBLEND_ONE, D3DBLEND_DESTALPHA},				// 2110: (0  - Cd)*Ad + Cs ==> Cs - Cd*Ad
	{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_INVDESTALPHA},				// 2111: (0  - Cd)*Ad + Cd ==> Cd*(1 - Ad)
	{0, D3DBLENDOP_SUBTRACT, D3DBLEND_ONE, D3DBLEND_DESTALPHA},				// 2112: (0  - Cd)*Ad +  0 ==> 0 - Cd*Ad
	{0, D3DBLENDOP_SUBTRACT, D3DBLEND_ONE, D3DBLEND_BLENDFACTOR},			// 2120: (0  - Cd)*F  + Cs ==> Cs - Cd*F
	{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_INVBLENDFACTOR},			// 2121: (0  - Cd)*F  + Cd ==> Cd*(1 - F)
	{0, D3DBLENDOP_SUBTRACT, D3DBLEND_ONE, D3DBLEND_BLENDFACTOR},			// 2122: (0  - Cd)*F  +  0 ==> 0 - Cd*F
	{0, D3DBLENDOP_ADD, D3DBLEND_ONE, D3DBLEND_ZERO},						// 2200: (0  -  0)*As + Cs ==> Cs
	{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ONE},						// 2201: (0  -  0)*As + Cd ==> Cd
	{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ZERO},						// 2202: (0  -  0)*As +  0 ==> 0
	{0, D3DBLENDOP_ADD, D3DBLEND_ONE, D3DBLEND_ZERO},						// 2210: (0  -  0)*Ad + Cs ==> Cs
	{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ONE},						// 2211: (0  -  0)*Ad + Cd ==> Cd
	{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ZERO},						// 2212: (0  -  0)*Ad +  0 ==> 0
	{0, D3DBLENDOP_ADD, D3DBLEND_ONE, D3DBLEND_ZERO},						// 2220: (0  -  0)*F  + Cs ==> Cs
	{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ONE},						// 2221: (0  -  0)*F  + Cd ==> Cd
	{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ZERO},						// 2222: (0  -  0)*F  +  0 ==> 0
};
