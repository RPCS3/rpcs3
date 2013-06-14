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
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "GSDeviceOGL.h"

#include "res/convert.h"
#include "res/interlace.h"
#include "res/merge.h"
#include "res/shadeboost.h"

// TODO performance cost to investigate
// Texture attachment/glDrawBuffer. For the moment it set every draw and potentially multiple time (first time in clear, second time in rendering)
//  Attachment 1 is only used with the GL_16UI format

//#define LOUD_DEBUGGING
//#define PRINT_FRAME_NUMBER
//#define ONLY_LINES
#if 0
#ifdef _DEBUG
#define ENABLE_OGL_STENCIL_DEBUG
#endif
#endif

static uint32 g_draw_count = 0;
static uint32 g_frame_count = 1;		

static const uint32 g_merge_cb_index      = 10;
static const uint32 g_interlace_cb_index  = 11;
static const uint32 g_shadeboost_cb_index = 12;
static const uint32 g_fxaa_cb_index       = 13;

GSDeviceOGL::GSDeviceOGL()
	: m_free_window(false)
	  , m_window(NULL)
	  , m_pipeline(0)
	  , m_fbo(0)
	  , m_fbo_read(0)
	  , m_vb_sr(NULL)
{
	m_msaa = !!theApp.GetConfig("UserHacks", 0) ? theApp.GetConfig("UserHacks_MSAA", 0) : 0;

	memset(&m_merge_obj, 0, sizeof(m_merge_obj));
	memset(&m_interlace, 0, sizeof(m_interlace));
	memset(&m_convert, 0, sizeof(m_convert));
	memset(&m_date, 0, sizeof(m_date));
	memset(&m_state, 0, sizeof(m_state));

	// Reset the debug file
	#ifdef ENABLE_OGL_DEBUG
	FILE* f = fopen("Debug.txt","w");
	fclose(f);
	#endif
}

GSDeviceOGL::~GSDeviceOGL()
{
	// Clean vertex buffer state
	delete (m_vb_sr);

	// Clean m_merge_obj
	for (uint32 i = 0; i < 2; i++)
		if (GLLoader::found_GL_ARB_separate_shader_objects)
			gl_DeleteProgram(m_merge_obj.ps[i]);
		else
			gl_DeleteShader(m_merge_obj.ps[i]);
	delete (m_merge_obj.cb);
	delete (m_merge_obj.bs);
	
	// Clean m_interlace
	for (uint32 i = 0; i < 2; i++)
		if (GLLoader::found_GL_ARB_separate_shader_objects)
			gl_DeleteProgram(m_interlace.ps[i]);
		else
			gl_DeleteShader(m_interlace.ps[i]);
	delete (m_interlace.cb);

	// Clean m_convert
	if (GLLoader::found_GL_ARB_separate_shader_objects) {
		gl_DeleteProgram(m_convert.vs);
		for (uint32 i = 0; i < 2; i++)
			gl_DeleteProgram(m_convert.ps[i]);
	} else {
		gl_DeleteShader(m_convert.vs);
		for (uint32 i = 0; i < 2; i++)
			gl_DeleteShader(m_convert.ps[i]);
	}
	gl_DeleteSamplers(1, &m_convert.ln);
	gl_DeleteSamplers(1, &m_convert.pt);
	delete m_convert.dss;
	delete m_convert.bs;

	// Clean m_date
	delete m_date.dss;
	delete m_date.bs;

	// Clean various opengl allocation
	if (GLLoader::found_GL_ARB_separate_shader_objects)
		gl_DeleteProgramPipelines(1, &m_pipeline);
	gl_DeleteFramebuffers(1, &m_fbo);
	gl_DeleteFramebuffers(1, &m_fbo_read);

	// Delete HW FX
	delete m_vs_cb;
	delete m_ps_cb;
	gl_DeleteSamplers(1, &m_palette_ss);
	delete m_vb;

	if (GLLoader::found_GL_ARB_separate_shader_objects) {
		for (auto it = m_vs.begin(); it != m_vs.end() ; it++) gl_DeleteProgram(it->second);
		for (auto it = m_gs.begin(); it != m_gs.end() ; it++) gl_DeleteProgram(it->second);
		for (auto it = m_ps.begin(); it != m_ps.end() ; it++) gl_DeleteProgram(it->second);
	} else {
		for (auto it = m_vs.begin(); it != m_vs.end() ; it++) gl_DeleteShader(it->second);
		for (auto it = m_gs.begin(); it != m_gs.end() ; it++) gl_DeleteShader(it->second);
		for (auto it = m_ps.begin(); it != m_ps.end() ; it++) gl_DeleteShader(it->second);

		for (auto it = m_single_prog.begin(); it != m_single_prog.end() ; it++) gl_DeleteProgram(it->second);
		m_single_prog.clear();
	}

	for (auto it = m_ps_ss.begin(); it != m_ps_ss.end() ; it++) gl_DeleteSamplers(1, &it->second);
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
		GLLoader::init_gl_function();

		if (!GLLoader::check_gl_version(3, 0)) return false;

		if (!GLLoader::check_gl_supported_extension()) return false;
	}

	// FIXME disable it when code is ready
	// glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);

	m_window = wnd;

	// ****************************************************************
	// Various object
	// ****************************************************************
	if (GLLoader::found_GL_ARB_separate_shader_objects) {
		gl_GenProgramPipelines(1, &m_pipeline);
		gl_BindProgramPipeline(m_pipeline);
	}

	gl_GenFramebuffers(1, &m_fbo);
	gl_GenFramebuffers(1, &m_fbo_read);

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
	CompileShaderFromSource("convert.glsl", "vs_main", GL_VERTEX_SHADER, &m_convert.vs, convert_glsl);
	for(uint32 i = 0; i < countof(m_convert.ps); i++)
		CompileShaderFromSource("convert.glsl", format("ps_main%d", i), GL_FRAGMENT_SHADER, &m_convert.ps[i], convert_glsl);

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

	CreateSampler(m_convert.ln, true, false, false);
	CreateSampler(m_convert.pt, false, false, false);

	m_convert.dss = new GSDepthStencilOGL();
	m_convert.bs  = new GSBlendStateOGL();

	// ****************************************************************
	// merge
	// ****************************************************************
	m_merge_obj.cb = new GSUniformBufferOGL(g_merge_cb_index, sizeof(MergeConstantBuffer));

	for(uint32 i = 0; i < countof(m_merge_obj.ps); i++)
		CompileShaderFromSource("merge.glsl", format("ps_main%d", i), GL_FRAGMENT_SHADER, &m_merge_obj.ps[i], merge_glsl);

	m_merge_obj.bs = new GSBlendStateOGL();
	m_merge_obj.bs->EnableBlend();
	m_merge_obj.bs->SetRGB(GL_FUNC_ADD, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// ****************************************************************
	// interlace
	// ****************************************************************
	m_interlace.cb = new GSUniformBufferOGL(g_interlace_cb_index, sizeof(InterlaceConstantBuffer));

	for(uint32 i = 0; i < countof(m_interlace.ps); i++)
		CompileShaderFromSource("interlace.glsl", format("ps_main%d", i), GL_FRAGMENT_SHADER, &m_interlace.ps[i], interlace_glsl);
	// ****************************************************************
	// Shade boost
	// ****************************************************************
	m_shadeboost.cb = new GSUniformBufferOGL(g_shadeboost_cb_index, sizeof(ShadeBoostConstantBuffer));

	int ShadeBoost_Contrast = theApp.GetConfig("ShadeBoost_Contrast", 50);
	int ShadeBoost_Brightness = theApp.GetConfig("ShadeBoost_Brightness", 50);
	int ShadeBoost_Saturation = theApp.GetConfig("ShadeBoost_Saturation", 50);
	std::string macro = format("#define SB_SATURATION %d\n", ShadeBoost_Saturation)
		+ format("#define SB_BRIGHTNESS %d\n", ShadeBoost_Brightness)
		+ format("#define SB_CONTRAST %d\n", ShadeBoost_Contrast);

	CompileShaderFromSource("shadeboost.glsl", "ps_main", GL_FRAGMENT_SHADER, &m_shadeboost.ps, shadeboost_glsl, macro);

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
	// m_fxaa.cb = new GSUniformBufferOGL(g_fxaa_cb_index, sizeof(FXAAConstantBuffer));
	//CompileShaderFromSource("fxaa.fx", format("ps_main", i), GL_FRAGMENT_SHADER, &m_fxaa.ps, fxaa_glsl);

	// ****************************************************************
	// DATE
	// ****************************************************************

	m_date.dss = new GSDepthStencilOGL();
	m_date.dss->EnableStencil();
	m_date.dss->SetStencil(GL_ALWAYS, GL_REPLACE);

	m_date.bs = new GSBlendStateOGL();
#ifndef ENABLE_OGL_STENCIL_DEBUG
	// Only keep stencil data
	m_date.bs->SetMask(false, false, false, false);
#endif

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

void GSDeviceOGL::SetVSync(bool enable)
{
	m_wnd->SetVSync(enable);
}

void GSDeviceOGL::Flip()
{
	// FIXME: disable it when code is working
	#ifdef ENABLE_OGL_DEBUG
	CheckDebugLog();
	#endif

	m_wnd->Flip();

#ifdef PRINT_FRAME_NUMBER
	fprintf(stderr, "Draw %d (Frame %d)\n", g_draw_count, g_frame_count);
#endif
#if defined(ENABLE_OGL_DEBUG) || defined(PRINT_FRAME_NUMBER)
	g_frame_count++;
#endif
}

static void set_uniform_buffer_binding(GLuint prog, GLchar* name, GLuint binding) {
	GLuint index;
	index = gl_GetUniformBlockIndex(prog, name);
	if (index != GL_INVALID_INDEX) {
		gl_UniformBlockBinding(prog, index, binding);
	}
}

static void set_sampler_uniform_binding(GLuint prog, GLchar* name, GLuint binding) {
	GLint loc = gl_GetUniformLocation(prog, name);
	if (loc != -1) {
		if (GLLoader::found_GL_ARB_separate_shader_objects) {
			gl_ProgramUniform1i(prog, loc, binding);
		} else {
			gl_Uniform1i(loc, binding);
		}
	}
}

GLuint GSDeviceOGL::link_prog()
{
	GLuint single_prog = gl_CreateProgram();
	if (m_state.vs) gl_AttachShader(single_prog, m_state.vs);
	if (m_state.ps) gl_AttachShader(single_prog, m_state.ps);
	if (m_state.gs) gl_AttachShader(single_prog, m_state.gs);

	gl_LinkProgram(single_prog);

	GLint status;
	gl_GetProgramiv(single_prog, GL_LINK_STATUS, &status);
	if (!status) {
		GLint log_length = 0;
		gl_GetProgramiv(single_prog, GL_INFO_LOG_LENGTH, &log_length);
		if (log_length > 0) {
			char* log = new char[log_length];
			gl_GetProgramInfoLog(single_prog, log_length, NULL, log);
			fprintf(stderr, "%s", log);
			delete[] log;
		}
		fprintf(stderr, "\n");
	}

#if 0
	if (m_state.vs) gl_DetachShader(single_prog, m_state.vs);
	if (m_state.ps) gl_DetachShader(single_prog, m_state.ps);
	if (m_state.gs) gl_DetachShader(single_prog, m_state.gs);
#endif

	return single_prog;
}

void GSDeviceOGL::BeforeDraw()
{
	hash_map<uint64, GLuint >::iterator single_prog;


	if (!GLLoader::found_GL_ARB_separate_shader_objects) {
		// Note: shader are integer lookup pointer. They start from 1 and incr
		// every time you create a new shader OR a new program.
		uint64 sel = (uint64)m_state.vs << 40 | (uint64)m_state.gs << 20 | m_state.ps;
		single_prog = m_single_prog.find(sel);
		if (single_prog == m_single_prog.end()) {
			m_single_prog[sel] = link_prog();
			single_prog = m_single_prog.find(sel);
		}

		gl_UseProgram(single_prog->second);
	}

	if (!GLLoader::found_GL_ARB_shading_language_420pack) {
		if (GLLoader::found_GL_ARB_separate_shader_objects) {
			set_uniform_buffer_binding(m_state.vs, "cb20", 20);
			set_uniform_buffer_binding(m_state.ps, "cb21", 21);

			set_uniform_buffer_binding(m_state.ps, "cb10", 10);
			set_uniform_buffer_binding(m_state.ps, "cb11", 11);
			set_uniform_buffer_binding(m_state.ps, "cb12", 12);
			set_uniform_buffer_binding(m_state.ps, "cb13", 13);

			set_sampler_uniform_binding(m_state.ps, "TextureSampler", 0);
			set_sampler_uniform_binding(m_state.ps, "PaletteSampler", 1);
			set_sampler_uniform_binding(m_state.ps, "RTCopySampler", 2);
		} else {
			set_uniform_buffer_binding(single_prog->second, "cb20", 20);
			set_uniform_buffer_binding(single_prog->second, "cb21", 21);

			set_uniform_buffer_binding(single_prog->second, "cb10", 10);
			set_uniform_buffer_binding(single_prog->second, "cb11", 11);
			set_uniform_buffer_binding(single_prog->second, "cb12", 12);
			set_uniform_buffer_binding(single_prog->second, "cb13", 13);

			set_sampler_uniform_binding(single_prog->second, "TextureSampler", 0);
			set_sampler_uniform_binding(single_prog->second, "PaletteSampler", 1);
			set_sampler_uniform_binding(single_prog->second, "RTCopySampler", 2);
		}
	}
}

void GSDeviceOGL::AfterDraw()
{
#if defined(ENABLE_OGL_DEBUG) || defined(PRINT_FRAME_NUMBER)
	g_draw_count++;
#endif
}

void GSDeviceOGL::DrawPrimitive()
{
	BeforeDraw();
	m_state.vb->DrawPrimitive();
	AfterDraw();
}

void GSDeviceOGL::DrawIndexedPrimitive()
{
	BeforeDraw();
	m_state.vb->DrawIndexedPrimitive();
	AfterDraw();
}

void GSDeviceOGL::DrawIndexedPrimitive(int offset, int count)
{
	ASSERT(offset + count <= m_index.count);

	BeforeDraw();
	m_state.vb->DrawIndexedPrimitive(offset, count);
	AfterDraw();
}

void GSDeviceOGL::ClearRenderTarget(GSTexture* t, const GSVector4& c)
{
	glDisable(GL_SCISSOR_TEST);
	if (static_cast<GSTextureOGL*>(t)->IsBackbuffer()) {
		OMSetFBO(0);

		// glDrawBuffer(GL_BACK); // this is the default when there is no FB
		// 0 will select the first drawbuffer ie GL_BACK
		gl_ClearBufferfv(GL_COLOR, 0, c.v);
	} else {
		OMSetFBO(m_fbo);
		static_cast<GSTextureOGL*>(t)->Attach(GL_COLOR_ATTACHMENT0);

		gl_ClearBufferfv(GL_COLOR, 0, c.v);
	}
	glEnable(GL_SCISSOR_TEST);
}

void GSDeviceOGL::ClearRenderTarget(GSTexture* t, uint32 c)
{
	GSVector4 color = GSVector4::rgba32(c) * (1.0f / 255);
	ClearRenderTarget(t, color);
}

void GSDeviceOGL::ClearDepth(GSTexture* t, float c)
{
	OMSetFBO(m_fbo);
	static_cast<GSTextureOGL*>(t)->Attach(GL_DEPTH_STENCIL_ATTACHMENT);

	glDisable(GL_SCISSOR_TEST);
	if (m_state.dss != NULL && m_state.dss->IsMaskEnable()) {
		gl_ClearBufferfv(GL_DEPTH, 0, &c);
	} else {
		glDepthMask(true);
		gl_ClearBufferfv(GL_DEPTH, 0, &c);
		glDepthMask(false);
	}
	glEnable(GL_SCISSOR_TEST);
}

void GSDeviceOGL::ClearStencil(GSTexture* t, uint8 c)
{
	OMSetFBO(m_fbo);
	static_cast<GSTextureOGL*>(t)->Attach(GL_DEPTH_STENCIL_ATTACHMENT);
	GLint color = c;

	glDisable(GL_SCISSOR_TEST);
	gl_ClearBufferiv(GL_STENCIL, 0, &color);
	glEnable(GL_SCISSOR_TEST);
}

void GSDeviceOGL::CreateSampler(GLuint& sampler, bool bilinear, bool tau, bool tav)
{
	gl_GenSamplers(1, &sampler);
	if (bilinear) {
		gl_SamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		gl_SamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	} else {
		gl_SamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		gl_SamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}

	// FIXME ensure U -> S,  V -> T and W->R
	if (tau)
		gl_SamplerParameteri(sampler, GL_TEXTURE_WRAP_S, GL_REPEAT);
	else
		gl_SamplerParameteri(sampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	if (tav)
		gl_SamplerParameteri(sampler, GL_TEXTURE_WRAP_T, GL_REPEAT);
	else
		gl_SamplerParameteri(sampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	gl_SamplerParameteri(sampler, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	// FIXME which value for GL_TEXTURE_MIN_LOD
	gl_SamplerParameterf(sampler, GL_TEXTURE_MAX_LOD, FLT_MAX);

	// FIXME: seems there is 2 possibility in opengl
	// DX: sd.ComparisonFunc = D3D11_COMPARISON_NEVER;
	// gl_SamplerParameteri(sampler, GL_TEXTURE_COMPARE_MODE, GL_NONE);
	gl_SamplerParameteri(sampler, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	gl_SamplerParameteri(sampler, GL_TEXTURE_COMPARE_FUNC, GL_NEVER);
	// FIXME: need ogl extension sd.MaxAnisotropy = 16;
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

		return NULL;
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

	// FIXME: the extension was integrated in opengl 4.3 (now we need driver that support OGL4.3)
	// FIXME check those function work as expected
	// void CopyImageSubDataNV(
    //         uint32 srcName, enum srcTarget, int srcLevel, int srcX, int srcY, int srcZ,
	//     uint32 dstName, enum dstTarget, int dstLevel, int dstX, int dstY, int dstZ,
	//     sizei width, sizei height, sizei depth);
	if (GLLoader::found_GL_NV_copy_image) {
		gl_CopyImageSubDataNV( static_cast<GSTextureOGL*>(st)->GetID(), static_cast<GSTextureOGL*>(st)->GetTarget(),
				0, r.x, r.y, 0,
				static_cast<GSTextureOGL*>(dt)->GetID(), static_cast<GSTextureOGL*>(dt)->GetTarget(),
				0, r.x, r.y, 0,
				r.width(), r.height(), 1);
	} else if (GLLoader::found_GL_ARB_copy_image) {
		// Would need an update of GL definition. For the moment it isn't supported by driver anyway.
#if 0
		gl_CopyImageSubData( static_cast<GSTextureOGL*>(st)->GetID(), static_cast<GSTextureOGL*>(st)->GetTarget(),
				0, r.x, r.y, 0,
				static_cast<GSTextureOGL*>(dt)->GetID(), static_cast<GSTextureOGL*>(dt)->GetTarget(),
				0, r.x, r.y, 0,
				r.width(), r.height(), 1);
#endif
	} else {

		GSTextureOGL* st_ogl = (GSTextureOGL*) st;
		GSTextureOGL* dt_ogl = (GSTextureOGL*) dt;

		gl_BindFramebuffer(GL_READ_FRAMEBUFFER, m_fbo_read);

		st_ogl->AttachRead(GL_COLOR_ATTACHMENT0);
		dt_ogl->EnableUnit(6);
		glCopyTexSubImage2D(dt_ogl->GetTarget(), 0, r.x, r.y, r.x, r.y, r.width(), r.height());

		gl_BindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	}

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


	// Original code from DX
	float left = dr.x * 2 / ds.x - 1.0f;
	float right = dr.z * 2 / ds.x - 1.0f;
#if 0
	float top = 1.0f - dr.y * 2 / ds.y;
	float bottom = 1.0f - dr.w * 2 / ds.y;
#else
	// Opengl get some issues with the coordinate
	// I flip top/bottom to fix scaling of the internal resolution
	float top = -1.0f + dr.y * 2 / ds.y;
	float bottom = -1.0f + dr.w * 2 / ds.y;
#endif

	// Flip y axis only when we render in the backbuffer
	// By default everything is render in the wrong order (ie dx).
	// 1/ consistency between several pass rendering (interlace)
	// 2/ in case some GSdx code expect thing in dx order.
	// Only flipping the backbuffer is transparent (I hope)...
	GSVector4 flip_sr = sr;
	if (static_cast<GSTextureOGL*>(dt)->IsBackbuffer()) {
		flip_sr.y = sr.w;
		flip_sr.w = sr.y;
	}

	GSVertexPT1 vertices[] =
	{
		{GSVector4(left, top, 0.5f, 1.0f), GSVector2(flip_sr.x, flip_sr.y)},
		{GSVector4(right, top, 0.5f, 1.0f), GSVector2(flip_sr.z, flip_sr.y)},
		{GSVector4(left, bottom, 0.5f, 1.0f), GSVector2(flip_sr.x, flip_sr.w)},
		{GSVector4(right, bottom, 0.5f, 1.0f), GSVector2(flip_sr.z, flip_sr.w)},
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

	GSSetShader(0);

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

void GSDeviceOGL::DoShadeBoost(GSTexture* st, GSTexture* dt)
{
	GSVector2i s = dt->GetSize();

	GSVector4 sr(0, 0, 1, 1);
	GSVector4 dr(0, 0, s.x, s.y);

	ShadeBoostConstantBuffer cb;

	cb.rcpFrame = GSVector4(1.0f / s.x, 1.0f / s.y, 0.0f, 0.0f);
	cb.rcpFrameOpt = GSVector4::zero();

	SetUniformBuffer(m_shadeboost.cb);
	m_shadeboost.cb->upload(&cb);

	StretchRect(st, sr, dt, dr, m_shadeboost.ps, m_shadeboost.cb);
}

void GSDeviceOGL::SetupDATE(GSTexture* rt, GSTexture* ds, const GSVertexPT1* vertices, bool datm)
{
#ifdef ENABLE_OGL_STENCIL_DEBUG
	const GSVector2i& size = rt->GetSize();
	GSTexture* t = CreateRenderTarget(size.x, size.y, rt->IsMSAA());
#else
	GSTexture* t = NULL;
#endif
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

	GSSetShader(0);

	// ps

	GSTexture* rt2 = rt->IsMSAA() ? Resolve(rt) : rt;

	PSSetShaderResources(rt2, NULL);
	PSSetSamplerState(m_convert.pt, 0);
	PSSetShader(m_convert.ps[datm ? 2 : 3]);

	//

	DrawPrimitive();

	//

	EndScene();

#ifdef ENABLE_OGL_STENCIL_DEBUG
	Recycle(t);
#endif

	if(rt2 != rt) Recycle(rt2);
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
	if (m_state.vs != vs)
	{
		m_state.vs = vs;
		if (GLLoader::found_GL_ARB_separate_shader_objects)
			gl_UseProgramStages(m_pipeline, GL_VERTEX_SHADER_BIT, vs);
	}
}

void GSDeviceOGL::GSSetShader(GLuint gs)
{
	if (m_state.gs != gs)
	{
		m_state.gs = gs;
		if (GLLoader::found_GL_ARB_separate_shader_objects)
			gl_UseProgramStages(m_pipeline, GL_GEOMETRY_SHADER_BIT, gs);
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

	if (m_state.ps_srv[i] != srv)
	{
		m_state.ps_srv[i] = srv;
		if (srv != NULL)
			m_state.ps_srv[i]->EnableUnit(i);
	}
}

void GSDeviceOGL::PSSetSamplerState(GLuint ss0, GLuint ss1, GLuint ss2)
{
	if (m_state.ps_ss[0] != ss0) {
		m_state.ps_ss[0] = ss0;
		gl_BindSampler(0, ss0);
	}
	if (m_state.ps_ss[1] != ss1) {
		m_state.ps_ss[1] = ss1;
		gl_BindSampler(1, ss1);
	}
}

void GSDeviceOGL::PSSetShader(GLuint ps)
{
	if (m_state.ps != ps)
	{
		m_state.ps = ps;
		if (GLLoader::found_GL_ARB_separate_shader_objects)
			gl_UseProgramStages(m_pipeline, GL_FRAGMENT_SHADER_BIT, ps);
	}
}

void GSDeviceOGL::OMSetFBO(GLuint fbo, GLenum buffer)
{
	if (m_state.fbo != fbo) {
		m_state.fbo = fbo;
		gl_BindFramebuffer(GL_FRAMEBUFFER, fbo);
		// FIXME DEBUG
		//if (fbo) fprintf(stderr, "FB status %x\n", gl_CheckFramebufferStatus(GL_FRAMEBUFFER));
	}

	if (m_state.draw != buffer) {
		m_state.draw = buffer;
		glDrawBuffer(buffer);
	}

}

void GSDeviceOGL::OMSetDepthStencilState(GSDepthStencilOGL* dss, uint8 sref)
{
	if (m_state.dss != dss) {
		m_state.dss = dss;

		dss->SetupDepth();
		dss->SetupStencil();
	}
}

void GSDeviceOGL::OMSetBlendState(GSBlendStateOGL* bs, float bf)
{
	if ( m_state.bs != bs || (m_state.bf != bf && bs->HasConstantFactor()) )
	{
		m_state.bs = bs;
		m_state.bf = bf;

		bs->SetupBlend(bf);
	}
}

void GSDeviceOGL::OMSetRenderTargets(GSTexture* rt, GSTexture* ds, const GSVector4i* scissor)
{
	if (rt == NULL || !static_cast<GSTextureOGL*>(rt)->IsBackbuffer()) {
		if (rt) {
			// FIXME DEBUG special case for GL_R16UI
			if (rt->GetFormat() == GL_R16UI) {
				OMSetFBO(m_fbo, GL_COLOR_ATTACHMENT1);
				static_cast<GSTextureOGL*>(rt)->Attach(GL_COLOR_ATTACHMENT1);
			} else {
				OMSetFBO(m_fbo, GL_COLOR_ATTACHMENT0);
				static_cast<GSTextureOGL*>(rt)->Attach(GL_COLOR_ATTACHMENT0);
			}
		} else {
			// Note: NULL rt is only used in DATE so far. Color writing is disabled
			// on the blend setup
			OMSetFBO(m_fbo, GL_NONE);
		}

		// Note: it must be done after OMSetFBO
		if (ds)
			static_cast<GSTextureOGL*>(ds)->Attach(GL_DEPTH_STENCIL_ATTACHMENT);

	} else {
		// Render in the backbuffer
		OMSetFBO(0);
	}



	GSVector2i size = rt ? rt->GetSize() : ds->GetSize();
	if(m_state.viewport != size)
	{
		m_state.viewport = size;
		glViewport(0, 0, size.x, size.y);
	}

	GSVector4i r = scissor ? *scissor : GSVector4i(size).zwxy();

	if(!m_state.scissor.eq(r))
	{
		m_state.scissor = r;
		glScissor( r.x, r.y, r.width(), r.height() );
	}
}

void GSDeviceOGL::CompileShaderFromSource(const std::string& glsl_file, const std::string& entry, GLenum type, GLuint* program, const char* glsl_h_code, const std::string& macro_sel)
{
	// Not supported
	if (type == GL_GEOMETRY_SHADER && !GLLoader::found_geometry_shader) {
		*program = 0;
		return;
	}

	// *****************************************************
	// Build a header string
	// *****************************************************
	// First select the version (must be the first line so we need to generate it
	std::string version;
	if (GLLoader::found_only_gl30) {
		version = "#version 130\n";
	} else {
		version = "#version 330\n";
	}
	if (GLLoader::found_GL_ARB_shading_language_420pack) {
		version += "#extension GL_ARB_shading_language_420pack: require\n";
	} else {
		version += "#define DISABLE_GL42\n";
	}
	if (GLLoader::found_GL_ARB_separate_shader_objects) {
		version += "#extension GL_ARB_separate_shader_objects : require\n";
		// REMOVE ME: Emulate open source driver
		//if (!GLLoader::found_GL_ARB_shading_language_420pack) {
		//	version += "#define NO_STRUCT 1\n";
		//}
	} else {
		if (GLLoader::found_only_gl30)
			version += "#define DISABLE_SSO\n";
	}
	if (GLLoader::found_only_gl30) {
		version += "#extension GL_ARB_explicit_attrib_location : require\n";
		version += "#extension GL_ARB_uniform_buffer_object : require\n";
	}
#ifdef ENABLE_OGL_STENCIL_DEBUG
	version += "#define ENABLE_OGL_STENCIL_DEBUG 1\n";
#endif

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
		default: ASSERT(0);
	}

	// Select the entry point ie the main function
	std::string entry_main = format("#define %s main\n", entry.c_str());

	std::string header = version + shader_type + entry_main + macro_sel;

	// *****************************************************
	// Read the source file
	// *****************************************************
	std::string source;
	std::string line;
	// Each linux distributions have his rules for path so we give them the possibility to
	// change it with compilation flags. -- Gregory
#ifdef GLSL_SHADER_DIR_COMPILATION
#define xGLSL_SHADER_DIR_str(s) GLSL_SHADER_DIR_str(s)
#define GLSL_SHADER_DIR_str(s) #s
	const std::string shader_file = string(xGLSL_SHADER_DIR_str(GLSL_SHADER_DIR_COMPILATION)) + '/' + glsl_file;
#else
	const std::string shader_file = string("plugins/") + glsl_file;
#endif
	std::ifstream myfile(shader_file.c_str());
	bool failed_to_open_glsl = true;
	if (myfile.is_open()) {
		while ( myfile.good() )
		{
			getline (myfile,line);
			source += line;
			source += '\n';
		}
		myfile.close();
		failed_to_open_glsl = false;
	}


	// Note it is better to separate header and source file to have the good line number
	// in the glsl compiler report
	const char** sources_array = (const char**)malloc(2*sizeof(char*));

	char* header_str = (char*)malloc(header.size() + 1);
	sources_array[0] = header_str;
	header.copy(header_str, header.size(), 0);
	header_str[header.size()] = '\0';

	char* source_str = (char*)malloc(source.size() + 1);
	if (failed_to_open_glsl) {
		sources_array[1] = glsl_h_code;
	} else {
		sources_array[1] = source_str;
		source.copy(source_str, source.size(), 0);
		source_str[source.size()] = '\0';
	}


	if (GLLoader::found_GL_ARB_separate_shader_objects) {
	#if 0
		// Could be useful one day
		const GLchar* ShaderSource[1];
		ShaderSource[0] = header.append(source).c_str();
		*program = gl_CreateShaderProgramv(type, 1, &ShaderSource[0]);
	#else
		*program = gl_CreateShaderProgramv(type, 2, sources_array);
	#endif
	} else {
		*program = gl_CreateShader(type);
		gl_ShaderSource(*program, 2, sources_array, NULL);
		gl_CompileShader(*program);
	}

	free(source_str);
	free(header_str);
	free(sources_array);

	if (theApp.GetConfig("debug_ogl_shader", 1) == 1) {
		// Print a nice debug log
		fprintf(stderr, "%s (entry %s, prog %d) :", glsl_file.c_str(), entry.c_str(), *program);
		fprintf(stderr, "\n%s", macro_sel.c_str());

		GLint log_length = 0;
		if (GLLoader::found_GL_ARB_separate_shader_objects)
			gl_GetProgramiv(*program, GL_INFO_LOG_LENGTH, &log_length);
		else
			gl_GetShaderiv(*program, GL_INFO_LOG_LENGTH, &log_length);

		if (log_length > 0) {
			char* log = new char[log_length];
			if (GLLoader::found_GL_ARB_separate_shader_objects)
				gl_GetProgramInfoLog(*program, log_length, NULL, log);
			else
				gl_GetShaderInfoLog(*program, log_length, NULL, log);

			fprintf(stderr, "%s", log);
			delete[] log;
		}
		fprintf(stderr, "\n");
	}
}

void GSDeviceOGL::CheckDebugLog()
{
       unsigned int count = 16; // max. num. of messages that will be read from the log
       int bufsize = 2048;
	   unsigned int sources[16] = {};
	   unsigned int types[16] = {};
	   unsigned int ids[16]   = {};
	   unsigned int severities[16] = {};
	   int lengths[16] = {};
       char* messageLog = new char[bufsize];

       unsigned int retVal = gl_GetDebugMessageLogARB(count, bufsize, sources, types, ids, severities, lengths, messageLog);

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

#define D3DBLEND_SRCALPHA		GL_SRC1_ALPHA
#define D3DBLEND_INVSRCALPHA	GL_ONE_MINUS_SRC1_ALPHA
                        
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
