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

#include "stdafx.h"
#include "GSdx.h"
#include "GSDevice11.h"
#include "GSUtil.h"
#include "resource.h"

GSDevice11::GSDevice11()
{
	memset(&m_state, 0, sizeof(m_state));
	memset(&m_vs_cb_cache, 0, sizeof(m_vs_cb_cache));
	memset(&m_ps_cb_cache, 0, sizeof(m_ps_cb_cache));

	m_state.topology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
	m_state.bf = -1;
}

GSDevice11::~GSDevice11()
{
	GSUtil::UnloadDynamicLibraries();
}

bool GSDevice11::Create(GSWnd* wnd)
{
	if(!__super::Create(wnd))
	{
		return false;
	}

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

	//return false;

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

	// convert

	D3D11_INPUT_ELEMENT_DESC il_convert[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};

	hr = CompileShader(IDR_CONVERT_FX, "vs_main", NULL, &m_convert.vs, il_convert, countof(il_convert), &m_convert.il);

	for(int i = 0; i < countof(m_convert.ps); i++)
	{
		hr = CompileShader(IDR_CONVERT_FX, format("ps_main%d", i).c_str(), NULL, &m_convert.ps[i]);
	}

	memset(&dsd, 0, sizeof(dsd));

	dsd.DepthEnable = false;
	dsd.StencilEnable = false;

	hr = m_dev->CreateDepthStencilState(&dsd, &m_convert.dss);

	memset(&bsd, 0, sizeof(bsd));

	bsd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	hr = m_dev->CreateBlendState(&bsd, &m_convert.bs);

	// merge

	memset(&bd, 0, sizeof(bd));

	bd.ByteWidth = sizeof(MergeConstantBuffer);
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	hr = m_dev->CreateBuffer(&bd, NULL, &m_merge.cb);

	for(int i = 0; i < countof(m_merge.ps); i++)
	{
		hr = CompileShader(IDR_MERGE_FX, format("ps_main%d", i).c_str(), NULL, &m_merge.ps[i]);
	}

	memset(&bsd, 0, sizeof(bsd));

	bsd.RenderTarget[0].BlendEnable = true;
	bsd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	bsd.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	bsd.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	bsd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	bsd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	bsd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	bsd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	hr = m_dev->CreateBlendState(&bsd, &m_merge.bs);

	// interlace

	memset(&bd, 0, sizeof(bd));

	bd.ByteWidth = sizeof(InterlaceConstantBuffer);
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	hr = m_dev->CreateBuffer(&bd, NULL, &m_interlace.cb);

	for(int i = 0; i < countof(m_interlace.ps); i++)
	{
		hr = CompileShader(IDR_INTERLACE_FX, format("ps_main%d", i).c_str(), NULL, &m_interlace.ps[i]);
	}

	// fxaa

	memset(&bd, 0, sizeof(bd));

	bd.ByteWidth = sizeof(FXAAConstantBuffer);
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	hr = m_dev->CreateBuffer(&bd, NULL, &m_fxaa.cb);

	hr = CompileShader(IDR_FXAA_FX, "ps_main", NULL, &m_fxaa.ps);

	//

	memset(&rd, 0, sizeof(rd));

	rd.FillMode = D3D11_FILL_SOLID;
	rd.CullMode = D3D11_CULL_NONE;
	rd.FrontCounterClockwise = false;
	rd.DepthBias = false;
	rd.DepthBiasClamp = 0;
	rd.SlopeScaledDepthBias = 0;
	rd.DepthClipEnable = false; // ???
	rd.ScissorEnable = true;
	rd.MultisampleEnable = true;
	rd.AntialiasedLineEnable = false;

	hr = m_dev->CreateRasterizerState(&rd, &m_rs);

	m_ctx->RSSetState(m_rs);

	//

	memset(&sd, 0, sizeof(sd));

	sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sd.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	sd.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	sd.MaxLOD = FLT_MAX;
	sd.MaxAnisotropy = 16;
	sd.ComparisonFunc = D3D11_COMPARISON_NEVER;

	hr = m_dev->CreateSamplerState(&sd, &m_convert.ln);

	sd.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;

	hr = m_dev->CreateSamplerState(&sd, &m_convert.pt);

	//

	Reset(1, 1);

	//

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

	// Exclusive/Fullscreen flip, issued for legacy (managed) windows only.  GSopen2 style
	// emulators will issue the flip themselves later on.

	if(m_wnd->IsManaged())
	{
		SetExclusive(!theApp.GetConfig("windowed", 1));
	}

	return true;
}

bool GSDevice11::Reset(int w, int h)
{
	if(!__super::Reset(w, h))
		return false;

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

	return true;
}

void GSDevice11::SetExclusive(bool isExcl)
{
	if(!m_swapchain) return;

	// TODO : Support for alternative display modes, by finishing this code below:
	//  Video mode info should be pulled form config/ini.

	/*DXGI_MODE_DESC desc;
	memset(&desc, 0, sizeof(desc));
	desc.RefreshRate = 0;		// must be zero for best results.

	m_swapchain->ResizeTarget(&desc);
	*/

	HRESULT hr = m_swapchain->SetFullscreenState(isExcl, NULL);

	if(hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE)
	{
		fprintf(stderr, "(GSdx10) SetExclusive(%s) failed; request unavailable.", isExcl ? "true" : "false");
	}
}

void GSDevice11::Flip()
{
	m_swapchain->Present(m_vsync, 0);
}

void GSDevice11::DrawPrimitive()
{
	m_ctx->Draw(m_vertex.count, m_vertex.start);
}

void GSDevice11::DrawIndexedPrimitive()
{
	m_ctx->DrawIndexed(m_index.count, m_index.start, m_vertex.start);
}

void GSDevice11::Dispatch(uint32 x, uint32 y, uint32 z)
{
	m_ctx->Dispatch(x, y, z);
}

void GSDevice11::ClearRenderTarget(GSTexture* t, const GSVector4& c)
{
	m_ctx->ClearRenderTargetView(*(GSTexture11*)t, c.v);
}

void GSDevice11::ClearRenderTarget(GSTexture* t, uint32 c)
{
	GSVector4 color = GSVector4::rgba32(c) * (1.0f / 255);

	m_ctx->ClearRenderTargetView(*(GSTexture11*)t, color.v);
}

void GSDevice11::ClearDepth(GSTexture* t, float c)
{
	m_ctx->ClearDepthStencilView(*(GSTexture11*)t, D3D11_CLEAR_DEPTH, c, 0);
}

void GSDevice11::ClearStencil(GSTexture* t, uint8 c)
{
	m_ctx->ClearDepthStencilView(*(GSTexture11*)t, D3D11_CLEAR_STENCIL, 0, c);
}

GSTexture* GSDevice11::CreateSurface(int type, int w, int h, bool msaa, int format)
{
	HRESULT hr;

	D3D11_TEXTURE2D_DESC desc;

	memset(&desc, 0, sizeof(desc));

	desc.Width = w;
	desc.Height = h;
	desc.Format = (DXGI_FORMAT)format;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;

	if(msaa)
	{
		desc.SampleDesc = m_msaa_desc;
	}

	switch(type)
	{
	case GSTexture::RenderTarget:
		desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		break;
	case GSTexture::DepthStencil:
		desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		break;
	case GSTexture::Texture:
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		break;
	case GSTexture::Offscreen:
		desc.Usage = D3D11_USAGE_STAGING;
		desc.CPUAccessFlags |= D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
		break;
	}

	GSTexture11* t = NULL;

	CComPtr<ID3D11Texture2D> texture;

	hr = m_dev->CreateTexture2D(&desc, NULL, &texture);

	if(SUCCEEDED(hr))
	{
		t = new GSTexture11(texture);

		switch(type)
		{
		case GSTexture::RenderTarget:
			ClearRenderTarget(t, 0);
			break;
		case GSTexture::DepthStencil:
			ClearDepth(t, 0);
			break;
		}
	}

	return t;
}

GSTexture* GSDevice11::CreateRenderTarget(int w, int h, bool msaa, int format)
{
	return __super::CreateRenderTarget(w, h, msaa, format ? format : DXGI_FORMAT_R8G8B8A8_UNORM);
}

GSTexture* GSDevice11::CreateDepthStencil(int w, int h, bool msaa, int format)
{
	return __super::CreateDepthStencil(w, h, msaa, format ? format : DXGI_FORMAT_D32_FLOAT_S8X24_UINT); // DXGI_FORMAT_R32G8X24_TYPELESS
}

GSTexture* GSDevice11::CreateTexture(int w, int h, int format)
{
	return __super::CreateTexture(w, h, format ? format : DXGI_FORMAT_R8G8B8A8_UNORM);
}

GSTexture* GSDevice11::CreateOffscreen(int w, int h, int format)
{
	return __super::CreateOffscreen(w, h, format ? format : DXGI_FORMAT_R8G8B8A8_UNORM);
}

GSTexture* GSDevice11::Resolve(GSTexture* t)
{
	ASSERT(t != NULL && t->IsMSAA());

	if(GSTexture* dst = CreateRenderTarget(t->GetWidth(), t->GetHeight(), false, t->GetFormat()))
	{
		dst->SetScale(t->GetScale());

		m_ctx->ResolveSubresource(*(GSTexture11*)dst, 0, *(GSTexture11*)t, 0, (DXGI_FORMAT)t->GetFormat());

		return dst;
	}

	return NULL;
}

GSTexture* GSDevice11::CopyOffscreen(GSTexture* src, const GSVector4& sr, int w, int h, int format)
{
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
}

void GSDevice11::CopyRect(GSTexture* st, GSTexture* dt, const GSVector4i& r)
{
	if(!st || !dt)
	{
		ASSERT(0);
		return;
	}

	D3D11_BOX box = {r.left, r.top, 0, r.right, r.bottom, 1};

	m_ctx->CopySubresourceRegion(*(GSTexture11*)dt, 0, 0, 0, 0, *(GSTexture11*)st, 0, &box);
}

void GSDevice11::StretchRect(GSTexture* st, const GSVector4& sr, GSTexture* dt, const GSVector4& dr, int shader, bool linear)
{
	StretchRect(st, sr, dt, dr, m_convert.ps[shader], NULL, linear);
}

void GSDevice11::StretchRect(GSTexture* st, const GSVector4& sr, GSTexture* dt, const GSVector4& dr, ID3D11PixelShader* ps, ID3D11Buffer* ps_cb, bool linear)
{
	StretchRect(st, sr, dt, dr, ps, ps_cb, m_convert.bs, linear);
}

void GSDevice11::StretchRect(GSTexture* st, const GSVector4& sr, GSTexture* dt, const GSVector4& dr, ID3D11PixelShader* ps, ID3D11Buffer* ps_cb, ID3D11BlendState* bs, bool linear)
{
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
}

void GSDevice11::DoMerge(GSTexture* st[2], GSVector4* sr, GSTexture* dt, GSVector4* dr, bool slbg, bool mmod, const GSVector4& c)
{
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
}

void GSDevice11::DoInterlace(GSTexture* st, GSTexture* dt, int shader, bool linear, float yoffset)
{
	GSVector4 s = GSVector4(dt->GetSize());

	GSVector4 sr(0, 0, 1, 1);
	GSVector4 dr(0.0f, yoffset, s.x, s.y + yoffset);

	InterlaceConstantBuffer cb;

	cb.ZrH = GSVector2(0, 1.0f / s.y);
	cb.hH = s.y / 2;

	m_ctx->UpdateSubresource(m_interlace.cb, 0, NULL, &cb, 0, 0);

	StretchRect(st, sr, dt, dr, m_interlace.ps[shader], m_interlace.cb, linear);
}

void GSDevice11::DoFXAA(GSTexture* st, GSTexture* dt)
{
	GSVector2i s = dt->GetSize();

	GSVector4 sr(0, 0, 1, 1);
	GSVector4 dr(0, 0, s.x, s.y);

	FXAAConstantBuffer cb;

	cb.rcpFrame = GSVector4(1.0f / s.x, 1.0f / s.y, 0.0f, 0.0f);
	cb.rcpFrameOpt = GSVector4::zero();

	m_ctx->UpdateSubresource(m_fxaa.cb, 0, NULL, &cb, 0, 0);

	StretchRect(st, sr, dt, dr, m_fxaa.ps, m_fxaa.cb, true);

	//st->Save("c:\\temp1\\1.bmp");
	//dt->Save("c:\\temp1\\2.bmp");
}

void GSDevice11::SetupDATE(GSTexture* rt, GSTexture* ds, const GSVertexPT1* vertices, bool datm)
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

		IASetVertexBuffer(vertices, sizeof(vertices[0]), 4);
		IASetInputLayout(m_convert.il);
		IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		// vs

		VSSetShader(m_convert.vs, NULL);

		// gs

		GSSetShader(NULL);

		// ps

		GSTexture* rt2 = rt->IsMSAA() ? Resolve(rt) : rt;

		PSSetShaderResources(rt2, NULL);
		PSSetSamplerState(m_convert.pt, NULL);
		PSSetShader(m_convert.ps[datm ? 2 : 3], NULL);

		//

		DrawPrimitive();

		//

		EndScene();

		Recycle(t);

		if(rt2 != rt) Recycle(rt2);
	}
}

void GSDevice11::IASetVertexBuffer(const void* vertex, size_t stride, size_t count)
{
	void* ptr = NULL;

	if(IAMapVertexBuffer(&ptr, stride, count))
	{
		GSVector4i::storent(ptr, vertex, count * stride);

		IAUnmapVertexBuffer();
	}
}

bool GSDevice11::IAMapVertexBuffer(void** vertex, size_t stride, size_t count)
{
	ASSERT(m_vertex.count == 0);

	if(count * stride > m_vertex.limit * m_vertex.stride)
	{
		m_vb_old = m_vb;
		m_vb = NULL;

		m_vertex.start = 0;
		m_vertex.limit = std::max<int>(count * 3 / 2, 11000);
	}

	if(m_vb == NULL)
	{
		D3D11_BUFFER_DESC bd;

		memset(&bd, 0, sizeof(bd));

		bd.Usage = D3D11_USAGE_DYNAMIC;
		bd.ByteWidth = m_vertex.limit * stride;
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		HRESULT hr;

		hr = m_dev->CreateBuffer(&bd, NULL, &m_vb);

		if(FAILED(hr)) return false;
	}

	D3D11_MAP type = D3D11_MAP_WRITE_NO_OVERWRITE;

	if(m_vertex.start + count > m_vertex.limit || stride != m_vertex.stride)
	{
		m_vertex.start = 0;

		type = D3D11_MAP_WRITE_DISCARD;
	}

	D3D11_MAPPED_SUBRESOURCE m;

	if(FAILED(m_ctx->Map(m_vb, 0, type, 0, &m)))
	{
		return false;
	}

	*vertex = (uint8*)m.pData + m_vertex.start * stride;

	m_vertex.count = count;
	m_vertex.stride = stride;

	return true;
}

void GSDevice11::IAUnmapVertexBuffer()
{
	m_ctx->Unmap(m_vb, 0);

	IASetVertexBuffer(m_vb, m_vertex.stride);
}

void GSDevice11::IASetVertexBuffer(ID3D11Buffer* vb, size_t stride)
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

void GSDevice11::IASetIndexBuffer(const void* index, size_t count)
{
	ASSERT(m_index.count == 0);

	if(count > m_index.limit)
	{
		m_ib_old = m_ib;
		m_ib = NULL;

		m_index.start = 0;
		m_index.limit = std::max<int>(count * 3 / 2, 11000);
	}

	if(m_ib == NULL)
	{
		D3D11_BUFFER_DESC bd;

		memset(&bd, 0, sizeof(bd));

		bd.Usage = D3D11_USAGE_DYNAMIC;
		bd.ByteWidth = m_index.limit * sizeof(uint32);
		bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		HRESULT hr;

		hr = m_dev->CreateBuffer(&bd, NULL, &m_ib);

		if(FAILED(hr)) return;
	}

	D3D11_MAP type = D3D11_MAP_WRITE_NO_OVERWRITE;

	if(m_index.start + count > m_index.limit)
	{
		m_index.start = 0;

		type = D3D11_MAP_WRITE_DISCARD;
	}

	D3D11_MAPPED_SUBRESOURCE m;

	if(SUCCEEDED(m_ctx->Map(m_ib, 0, type, 0, &m)))
	{
		memcpy((uint8*)m.pData + m_index.start * sizeof(uint32), index, count * sizeof(uint32));

		m_ctx->Unmap(m_ib, 0);
	}

	m_index.count = count;

	IASetIndexBuffer(m_ib);
}

void GSDevice11::IASetIndexBuffer(ID3D11Buffer* ib)
{
	if(m_state.ib != ib)
	{
		m_state.ib = ib;

		m_ctx->IASetIndexBuffer(ib, DXGI_FORMAT_R32_UINT, 0);
	}
}

void GSDevice11::IASetInputLayout(ID3D11InputLayout* layout)
{
	if(m_state.layout != layout)
	{
		m_state.layout = layout;

		m_ctx->IASetInputLayout(layout);
	}
}

void GSDevice11::IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY topology)
{
	if(m_state.topology != topology)
	{
		m_state.topology = topology;

		m_ctx->IASetPrimitiveTopology(topology);
	}
}

void GSDevice11::VSSetShader(ID3D11VertexShader* vs, ID3D11Buffer* vs_cb)
{
	if(m_state.vs != vs)
	{
		m_state.vs = vs;

		m_ctx->VSSetShader(vs, NULL, 0);
	}

	if(m_state.vs_cb != vs_cb)
	{
		m_state.vs_cb = vs_cb;

		m_ctx->VSSetConstantBuffers(0, 1, &vs_cb);
	}
}

void GSDevice11::GSSetShader(ID3D11GeometryShader* gs)
{
	if(m_state.gs != gs)
	{
		m_state.gs = gs;

		m_ctx->GSSetShader(gs, NULL, 0);
	}
}

void GSDevice11::PSSetShaderResources(GSTexture* sr0, GSTexture* sr1)
{
	PSSetShaderResource(0, sr0);
	PSSetShaderResource(1, sr1);

	for(int i = 2; i < countof(m_state.ps_srv); i++)
	{
		PSSetShaderResource(i, NULL);
	}
}

void GSDevice11::PSSetShaderResource(int i, GSTexture* sr)
{
	ID3D11ShaderResourceView* srv = NULL;

	if(sr) srv = *(GSTexture11*)sr;

	PSSetShaderResourceView(i, srv);
}

void GSDevice11::PSSetShaderResourceView(int i, ID3D11ShaderResourceView* srv)
{
	ASSERT(i < countof(m_state.ps_srv));

	if(m_state.ps_srv[i] != srv)
	{
		m_state.ps_srv[i] = srv;

		m_srv_changed = true;
	}
}

void GSDevice11::PSSetSamplerState(ID3D11SamplerState* ss0, ID3D11SamplerState* ss1, ID3D11SamplerState* ss2)
{
	if(m_state.ps_ss[0] != ss0 || m_state.ps_ss[1] != ss1 || m_state.ps_ss[2] != ss2)
	{
		m_state.ps_ss[0] = ss0;
		m_state.ps_ss[1] = ss1;
		m_state.ps_ss[2] = ss2;

		m_ss_changed = true;
	}
}

void GSDevice11::PSSetShader(ID3D11PixelShader* ps, ID3D11Buffer* ps_cb)
{
	if(m_state.ps != ps)
	{
		m_state.ps = ps;

		m_ctx->PSSetShader(ps, NULL, 0);
	}

	if(m_srv_changed)
	{
		m_ctx->PSSetShaderResources(0, countof(m_state.ps_srv), m_state.ps_srv);

		m_srv_changed = false;
	}

	if(m_ss_changed)
	{
		m_ctx->PSSetSamplers(0, countof(m_state.ps_ss), m_state.ps_ss);

		m_ss_changed = false;
	}

	if(m_state.ps_cb != ps_cb)
	{
		m_state.ps_cb = ps_cb;

		m_ctx->PSSetConstantBuffers(0, 1, &ps_cb);
	}
}

void GSDevice11::CSSetShaderSRV(int i, ID3D11ShaderResourceView* srv)
{
	// TODO: if(m_state.cs_srv[i] != srv)
	{
		// TODO: m_state.cs_srv[i] = srv;

		m_ctx->CSSetShaderResources(i, 1, &srv);
	}
}

void GSDevice11::CSSetShaderUAV(int i, ID3D11UnorderedAccessView* uav)
{
	// TODO: if(m_state.cs_uav[i] != uav)
	{
		// TODO: m_state.cs_uav[i] = uav;

		// uint32 count[] = {-1};

		m_ctx->CSSetUnorderedAccessViews(i, 1, &uav, NULL);
	}
}

void GSDevice11::CSSetShader(ID3D11ComputeShader* cs)
{
	if(m_state.cs != cs)
	{
		m_state.cs = cs;

		m_ctx->CSSetShader(cs, NULL, 0);
	}
}

void GSDevice11::OMSetDepthStencilState(ID3D11DepthStencilState* dss, uint8 sref)
{
	if(m_state.dss != dss || m_state.sref != sref)
	{
		m_state.dss = dss;
		m_state.sref = sref;

		m_ctx->OMSetDepthStencilState(dss, sref);
	}
}

void GSDevice11::OMSetBlendState(ID3D11BlendState* bs, float bf)
{
	if(m_state.bs != bs || m_state.bf != bf)
	{
		m_state.bs = bs;
		m_state.bf = bf;

		float BlendFactor[] = {bf, bf, bf, 0};

		m_ctx->OMSetBlendState(bs, BlendFactor, 0xffffffff);
	}
}

void GSDevice11::OMSetRenderTargets(GSTexture* rt, GSTexture* ds, const GSVector4i* scissor)
{
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

	memset(m_state.uav, 0, sizeof(m_state.uav));

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

	GSVector4i r = scissor ? *scissor : GSVector4i(rt->GetSize()).zwxy();

	if(!m_state.scissor.eq(r))
	{
		m_state.scissor = r;

		m_ctx->RSSetScissorRects(1, r);
	}
}

void GSDevice11::OMSetRenderTargets(const GSVector2i& rtsize, ID3D11UnorderedAccessView** uav, int count, const GSVector4i* scissor)
{
	for(int i = 0; i < count; i++)
	{
		if(m_state.uav[i] != uav[i])
		{
			memcpy(m_state.uav, uav, sizeof(uav[0]) * count);
			memset(m_state.uav + count, 0, sizeof(m_state.uav) - sizeof(uav[0]) * count);

			m_ctx->OMSetRenderTargetsAndUnorderedAccessViews(0, NULL, NULL, 0, count, uav, NULL);

			break;
		}
	}

	m_state.rtv = NULL;
	m_state.dsv = NULL;

	if(m_state.viewport != rtsize)
	{
		m_state.viewport = rtsize;

		D3D11_VIEWPORT vp;

		memset(&vp, 0, sizeof(vp));

		vp.TopLeftX = 0;
		vp.TopLeftY = 0;
		vp.Width = (float)rtsize.x;
		vp.Height = (float)rtsize.y;
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;

		m_ctx->RSSetViewports(1, &vp);
	}

	GSVector4i r = scissor ? *scissor : GSVector4i(rtsize).zwxy();

	if(!m_state.scissor.eq(r))
	{
		m_state.scissor = r;

		m_ctx->RSSetScissorRects(1, r);
	}
}

HRESULT GSDevice11::CompileShader(uint32 id, const char* entry, D3D11_SHADER_MACRO* macro, ID3D11VertexShader** vs, D3D11_INPUT_ELEMENT_DESC* layout, int count, ID3D11InputLayout** il)
{
	HRESULT hr;

	vector<D3D11_SHADER_MACRO> m;

	PrepareShaderMacro(m, macro);

	CComPtr<ID3D11Blob> shader, error;

    hr = D3DX11CompileFromResource(theApp.GetModuleHandle(), MAKEINTRESOURCE(id), NULL, &m[0], NULL, entry, m_shader.vs.c_str(), 0, 0, NULL, &shader, &error, NULL);

	if(error)
	{
		printf("%s\n", (const char*)error->GetBufferPointer());
	}

	if(FAILED(hr))
	{
		return hr;
	}

	hr = m_dev->CreateVertexShader((void*)shader->GetBufferPointer(), shader->GetBufferSize(), NULL, vs);

	if(FAILED(hr))
	{
		return hr;
	}

	hr = m_dev->CreateInputLayout(layout, count, shader->GetBufferPointer(), shader->GetBufferSize(), il);

	if(FAILED(hr))
	{
		return hr;
	}

	return hr;
}

HRESULT GSDevice11::CompileShader(uint32 id, const char* entry, D3D11_SHADER_MACRO* macro, ID3D11GeometryShader** gs)
{
	HRESULT hr;

	vector<D3D11_SHADER_MACRO> m;

	PrepareShaderMacro(m, macro);

	CComPtr<ID3D11Blob> shader, error;

    hr = D3DX11CompileFromResource(theApp.GetModuleHandle(), MAKEINTRESOURCE(id), NULL, &m[0], NULL, entry, m_shader.gs.c_str(), 0, 0, NULL, &shader, &error, NULL);

	if(error)
	{
		printf("%s\n", (const char*)error->GetBufferPointer());
	}

	if(FAILED(hr))
	{
		return hr;
	}

	hr = m_dev->CreateGeometryShader((void*)shader->GetBufferPointer(), shader->GetBufferSize(), NULL, gs);

	if(FAILED(hr))
	{
		return hr;
	}

	return hr;
}

HRESULT GSDevice11::CompileShader(uint32 id, const char* entry, D3D11_SHADER_MACRO* macro, ID3D11GeometryShader** gs, D3D11_SO_DECLARATION_ENTRY* layout, int count)
{
	HRESULT hr;

	vector<D3D11_SHADER_MACRO> m;

	PrepareShaderMacro(m, macro);

	CComPtr<ID3D11Blob> shader, error;

    hr = D3DX11CompileFromResource(theApp.GetModuleHandle(), MAKEINTRESOURCE(id), NULL, &m[0], NULL, entry, m_shader.gs.c_str(), 0, 0, NULL, &shader, &error, NULL);

	if(error)
	{
		printf("%s\n", (const char*)error->GetBufferPointer());
	}

	if(FAILED(hr))
	{
		return hr;
	}

	hr = m_dev->CreateGeometryShaderWithStreamOutput((void*)shader->GetBufferPointer(), shader->GetBufferSize(), layout, count, NULL, 0, D3D11_SO_NO_RASTERIZED_STREAM, NULL, gs);

	if(FAILED(hr))
	{
		return hr;
	}

	return hr;
}

HRESULT GSDevice11::CompileShader(uint32 id, const char* entry, D3D11_SHADER_MACRO* macro, ID3D11PixelShader** ps)
{
	HRESULT hr;

	vector<D3D11_SHADER_MACRO> m;

	PrepareShaderMacro(m, macro);

	CComPtr<ID3D11Blob> shader, error;

    hr = D3DX11CompileFromResource(theApp.GetModuleHandle(), MAKEINTRESOURCE(id), NULL, &m[0], NULL, entry, m_shader.ps.c_str(), 0, 0, NULL, &shader, &error, NULL);

	if(error)
	{
		printf("%s\n", (const char*)error->GetBufferPointer());
	}

	if(FAILED(hr))
	{
		return hr;
	}

	hr = m_dev->CreatePixelShader((void*)shader->GetBufferPointer(), shader->GetBufferSize(),NULL, ps);

	if(FAILED(hr))
	{
		return hr;
	}

	return hr;
}

HRESULT GSDevice11::CompileShader(uint32 id, const char* entry, D3D11_SHADER_MACRO* macro, ID3D11ComputeShader** cs)
{
	HRESULT hr;

	vector<D3D11_SHADER_MACRO> m;

	PrepareShaderMacro(m, macro);

	CComPtr<ID3D11Blob> shader, error;

    hr = D3DX11CompileFromResource(theApp.GetModuleHandle(), MAKEINTRESOURCE(id), NULL, &m[0], NULL, entry, m_shader.ps.c_str(), 0, 0, NULL, &shader, &error, NULL);

	if(error)
	{
		printf("%s\n", (const char*)error->GetBufferPointer());
	}

	if(FAILED(hr))
	{
		return hr;
	}

	hr = m_dev->CreateComputeShader((void*)shader->GetBufferPointer(), shader->GetBufferSize(),NULL, cs);

	if(FAILED(hr))
	{
		return hr;
	}

	return hr;
}

HRESULT GSDevice11::CompileShader(const char* fn, const char* entry, D3D11_SHADER_MACRO* macro, ID3D11ComputeShader** cs)
{
	HRESULT hr;

	vector<D3D11_SHADER_MACRO> m;

	PrepareShaderMacro(m, macro);

	CComPtr<ID3D11Blob> shader, error;

    hr = D3DX11CompileFromFile(fn, &m[0], NULL, entry, m_shader.cs.c_str(), 0, 0, NULL, &shader, &error, NULL);

	if(error)
	{
		printf("%s\n", (const char*)error->GetBufferPointer());
	}

	if(FAILED(hr))
	{
		return hr;
	}

	hr = m_dev->CreateComputeShader((void*)shader->GetBufferPointer(), shader->GetBufferSize(),NULL, cs);

	if(FAILED(hr))
	{
		return hr;
	}

	return hr;
}

