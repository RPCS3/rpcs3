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
#include "resource.h"

GSDevice11::GSDevice11()
	: m_vb(NULL)
	, m_vb_stride(0)
	, m_layout(NULL)
	, m_topology(D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED)
	, m_vs(NULL)
	, m_vs_cb(NULL)
	, m_gs(NULL)
	, m_ps(NULL)
	, m_ps_cb(NULL)
	, m_scissor(0, 0, 0, 0)
	, m_viewport(0, 0)
	, m_dss(NULL)
	, m_sref(0)
	, m_bs(NULL)
	, m_bf(-1)
	, m_rtv(NULL)
	, m_dsv(NULL)
{
	memset(m_ps_srv, 0, sizeof(m_ps_srv));
	memset(m_ps_ss, 0, sizeof(m_ps_ss));

	m_vertices.stride = 0;
	m_vertices.start = 0;
	m_vertices.count = 0;
	m_vertices.limit = 0;
}

GSDevice11::~GSDevice11()
{
}

bool GSDevice11::Create(GSWnd* wnd, bool vsync)
{
	if(!__super::Create(wnd, vsync))
	{
		return false;
	}

	HRESULT hr;

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
	scd.Windowed = TRUE;

	uint32 flags = 0;

#ifdef DEBUG
	flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION, &scd, &m_swapchain, &m_dev, &m_level, &m_ctx);
	// hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_REFERENCE, NULL, 0, NULL, 0, D3D11_SDK_VERSION, &scd, &m_swapchain, &m_dev, &m_level, &m_ctx);

	if(FAILED(hr)) return false;

	D3D11_FEATURE_DATA_D3D10_X_HARDWARE_OPTIONS options;
	
	hr = m_dev->CheckFeatureSupport(D3D11_FEATURE_D3D10_X_HARDWARE_OPTIONS, &options, sizeof(D3D11_FEATURE_D3D10_X_HARDWARE_OPTIONS));

	switch(m_level)
	{
	case D3D_FEATURE_LEVEL_9_1:
	case D3D_FEATURE_LEVEL_9_2:
		m_shader.model = "0x200";
		m_shader.vs = "vs_4_0_level_9_1";
		m_shader.ps = "ps_4_0_level_9_1";
		break;
	case D3D_FEATURE_LEVEL_9_3:
		m_shader.model = "0x300";
		m_shader.vs = "vs_4_0_level_9_3";
		m_shader.ps = "ps_4_0_level_9_3";
		break;
	case D3D_FEATURE_LEVEL_10_0:
	case D3D_FEATURE_LEVEL_10_1:
		m_shader.model = "0x400";
		m_shader.vs = "vs_4_0";
		m_shader.gs = "gs_4_0";
		m_shader.ps = "ps_4_0";
		break;
	case D3D_FEATURE_LEVEL_11_0:
		m_shader.model = "0x500";
		m_shader.vs = "vs_5_0";
		m_shader.gs = "gs_5_0";
		m_shader.ps = "ps_5_0";
		break;
	}

	if(m_level < D3D_FEATURE_LEVEL_10_0)
	{
		return false;
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
		hr = CompileShader(IDR_CONVERT_FX, format("ps_main%d", i), NULL, &m_convert.ps[i]);
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
		hr = CompileShader(IDR_MERGE_FX, format("ps_main%d", i), NULL, &m_merge.ps[i]);
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
		hr = CompileShader(IDR_INTERLACE_FX, format("ps_main%d", i), NULL, &m_interlace.ps[i]);
	}

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
	rd.MultisampleEnable = false;
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

	Reset(1, 1, Windowed);

	//

	return true;
}

bool GSDevice11::Reset(int w, int h, int mode)
{
	if(!__super::Reset(w, h, mode))
		return false;

	DXGI_SWAP_CHAIN_DESC scd;
	memset(&scd, 0, sizeof(scd));
	m_swapchain->GetDesc(&scd);
	m_swapchain->ResizeBuffers(scd.BufferCount, w, h, scd.BufferDesc.Format, 0);
	
	CComPtr<ID3D11Texture2D> backbuffer;
	m_swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backbuffer);
	m_backbuffer = new GSTexture11(backbuffer);

	return true;
}

void GSDevice11::Flip()
{
	m_swapchain->Present(m_vsync ? 1 : 0, 0);
}

void GSDevice11::BeginScene()
{
}

void GSDevice11::DrawPrimitive()
{
	m_ctx->Draw(m_vertices.count, m_vertices.start);
}

void GSDevice11::EndScene()
{
	PSSetShaderResources(NULL, NULL);

	// not clearing the rt/ds gives a little fps boost in complex games (5-10%)

	// OMSetRenderTargets(NULL, NULL);

	m_vertices.start += m_vertices.count;
	m_vertices.count = 0;
}

void GSDevice11::ClearRenderTarget(GSTexture* t, const GSVector4& c)
{
	m_ctx->ClearRenderTargetView(*(GSTexture11*)t, c.v);
}

void GSDevice11::ClearRenderTarget(GSTexture* t, uint32 c)
{
	GSVector4 color = GSVector4(c) * (1.0f / 255);

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

GSTexture* GSDevice11::Create(int type, int w, int h, int format)
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

GSTexture* GSDevice11::CreateRenderTarget(int w, int h, int format)
{
	return __super::CreateRenderTarget(w, h, format ? format : DXGI_FORMAT_R8G8B8A8_UNORM);
}

GSTexture* GSDevice11::CreateDepthStencil(int w, int h, int format)
{
	return __super::CreateDepthStencil(w, h, format ? format : DXGI_FORMAT_D32_FLOAT_S8X24_UINT);
}

GSTexture* GSDevice11::CreateTexture(int w, int h, int format)
{
	return __super::CreateTexture(w, h, format ? format : DXGI_FORMAT_R8G8B8A8_UNORM);
}

GSTexture* GSDevice11::CreateOffscreen(int w, int h, int format)
{
	return __super::CreateOffscreen(w, h, format ? format : DXGI_FORMAT_R8G8B8A8_UNORM);
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

	if(GSTexture* rt = CreateRenderTarget(w, h, format))
	{
		GSVector4 dr(0, 0, w, h);

		StretchRect(src, sr, rt, dr, m_convert.ps[format == DXGI_FORMAT_R16_UINT ? 1 : 0], NULL);

		dst = CreateOffscreen(w, h, format);

		if(dst)
		{
			m_ctx->CopyResource(*(GSTexture11*)dst, *(GSTexture11*)rt);
		}

		Recycle(rt);
	}

	return dst;
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

	PSSetShader(ps, ps_cb);
	PSSetSamplerState(linear ? m_convert.ln : m_convert.pt, NULL);
	PSSetShaderResources(st, NULL);

	// rs

	RSSet(ds.x, ds.y);

	//

	DrawPrimitive();

	//

	EndScene();
}

void GSDevice11::DoMerge(GSTexture* st[2], GSVector4* sr, GSVector4* dr, GSTexture* dt, bool slbg, bool mmod, const GSVector4& c)
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

void GSDevice11::IASetVertexBuffer(const void* vertices, size_t stride, size_t count)
{
	ASSERT(m_vertices.count == 0);

	if(count * stride > m_vertices.limit * m_vertices.stride)
	{
		m_vertices.vb_old = m_vertices.vb;
		m_vertices.vb = NULL;
		m_vertices.start = 0;
		m_vertices.count = 0;
		m_vertices.limit = std::max<int>(count * 3 / 2, 10000);
	}

	if(m_vertices.vb == NULL)
	{
		D3D11_BUFFER_DESC bd;

		memset(&bd, 0, sizeof(bd));

		bd.Usage = D3D11_USAGE_DYNAMIC;
		bd.ByteWidth = m_vertices.limit * stride;
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		HRESULT hr;
		
		hr = m_dev->CreateBuffer(&bd, NULL, &m_vertices.vb);

		if(FAILED(hr)) return;
	}

	D3D11_MAP type = D3D11_MAP_WRITE_NO_OVERWRITE;

	if(m_vertices.start + count > m_vertices.limit || stride != m_vertices.stride)
	{
		m_vertices.start = 0;

		type = D3D11_MAP_WRITE_DISCARD;
	}

	D3D11_MAPPED_SUBRESOURCE m;

	if(SUCCEEDED(m_ctx->Map(m_vertices.vb, 0, type, 0, &m)))
	{
		GSVector4i::storent((uint8*)m.pData + m_vertices.start * stride, vertices, count * stride);

		m_ctx->Unmap(m_vertices.vb, 0);
	}

	m_vertices.count = count;
	m_vertices.stride = stride;

	IASetVertexBuffer(m_vertices.vb, stride);
}

void GSDevice11::IASetVertexBuffer(ID3D11Buffer* vb, size_t stride)
{
	if(m_vb != vb || m_vb_stride != stride)
	{
		uint32 offset = 0;

		m_ctx->IASetVertexBuffers(0, 1, &vb, &stride, &offset);

		m_vb = vb;
		m_vb_stride = stride;
	}
}

void GSDevice11::IASetInputLayout(ID3D11InputLayout* layout)
{
	if(m_layout != layout)
	{
		m_ctx->IASetInputLayout(layout);

		m_layout = layout;
	}
}

void GSDevice11::IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY topology)
{
	if(m_topology != topology)
	{
		m_ctx->IASetPrimitiveTopology(topology);

		m_topology = topology;
	}
}

void GSDevice11::VSSetShader(ID3D11VertexShader* vs, ID3D11Buffer* vs_cb)
{
	if(m_vs != vs)
	{
		m_ctx->VSSetShader(vs, NULL, 0);

		m_vs = vs;
	}
	
	if(m_vs_cb != vs_cb)
	{
		m_ctx->VSSetConstantBuffers(0, 1, &vs_cb);

		m_vs_cb = vs_cb;
	}
}

void GSDevice11::GSSetShader(ID3D11GeometryShader* gs)
{
	if(m_gs != gs)
	{
		m_ctx->GSSetShader(gs, NULL, 0);

		m_gs = gs;
	}
}

void GSDevice11::PSSetShaderResources(GSTexture* sr0, GSTexture* sr1)
{
	ID3D11ShaderResourceView* srv0 = NULL;
	ID3D11ShaderResourceView* srv1 = NULL;
	
	if(sr0) srv0 = *(GSTexture11*)sr0;
	if(sr1) srv1 = *(GSTexture11*)sr1;

	if(m_ps_srv[0] != srv0 || m_ps_srv[1] != srv1)
	{
		ID3D11ShaderResourceView* srvs[] = {srv0, srv1};
	
		m_ctx->PSSetShaderResources(0, 2, srvs);

		m_ps_srv[0] = srv0;
		m_ps_srv[1] = srv1;
	}
}

void GSDevice11::PSSetShader(ID3D11PixelShader* ps, ID3D11Buffer* ps_cb)
{
	if(m_ps != ps)
	{
		m_ctx->PSSetShader(ps, NULL, 0);

		m_ps = ps;
	}
	
	if(m_ps_cb != ps_cb)
	{
		m_ctx->PSSetConstantBuffers(0, 1, &ps_cb);

		m_ps_cb = ps_cb;
	}
}

void GSDevice11::PSSetSamplerState(ID3D11SamplerState* ss0, ID3D11SamplerState* ss1)
{
	if(m_ps_ss[0] != ss0 || m_ps_ss[1] != ss1)
	{
		ID3D11SamplerState* sss[] = {ss0, ss1};

		m_ctx->PSSetSamplers(0, 2, sss);

		m_ps_ss[0] = ss0;
		m_ps_ss[1] = ss1;
	}
}

void GSDevice11::RSSet(int width, int height, const GSVector4i* scissor)
{
	if(m_viewport.x != width || m_viewport.y != height)
	{
		D3D11_VIEWPORT vp;

		memset(&vp, 0, sizeof(vp));
		
		vp.TopLeftX = 0;
		vp.TopLeftY = 0;
		vp.Width = width;
		vp.Height = height;
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;

		m_ctx->RSSetViewports(1, &vp);

		m_viewport = GSVector2i(width, height);
	}

	GSVector4i r = scissor ? *scissor : GSVector4i(0, 0, width, height);

	if(!m_scissor.eq(r))
	{
		m_ctx->RSSetScissorRects(1, r);

		m_scissor = r;
	}
}

void GSDevice11::OMSetDepthStencilState(ID3D11DepthStencilState* dss, uint8 sref)
{
	if(m_dss != dss || m_sref != sref)
	{
		m_ctx->OMSetDepthStencilState(dss, sref);

		m_dss = dss;
		m_sref = sref;
	}
}

void GSDevice11::OMSetBlendState(ID3D11BlendState* bs, float bf)
{
	if(m_bs != bs || m_bf != bf)
	{
		float BlendFactor[] = {bf, bf, bf, 0};

		m_ctx->OMSetBlendState(bs, BlendFactor, 0xffffffff);

		m_bs = bs;
		m_bf = bf;
	}
}

void GSDevice11::OMSetRenderTargets(GSTexture* rt, GSTexture* ds)
{
	ID3D11RenderTargetView* rtv = NULL;
	ID3D11DepthStencilView* dsv = NULL;

	if(rt) rtv = *(GSTexture11*)rt;
	if(ds) dsv = *(GSTexture11*)ds;

	if(m_rtv != rtv || m_dsv != dsv)
	{
		m_ctx->OMSetRenderTargets(1, &rtv, dsv);

		m_rtv = rtv;
		m_dsv = dsv;
	}
}

HRESULT GSDevice11::CompileShader(uint32 id, const string& entry, D3D11_SHADER_MACRO* macro, ID3D11VertexShader** vs, D3D11_INPUT_ELEMENT_DESC* layout, int count, ID3D11InputLayout** il)
{
	HRESULT hr;

	vector<D3D11_SHADER_MACRO> m;

	PrepareShaderMacro(m, macro, m_shader.model.c_str());

	CComPtr<ID3D11Blob> shader, error;

    hr = D3DX11CompileFromResource(theApp.GetModuleHandle(), MAKEINTRESOURCE(id), NULL, &m[0], NULL, entry.c_str(), m_shader.vs.c_str(), 0, 0, NULL, &shader, &error, NULL);
	
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

HRESULT GSDevice11::CompileShader(uint32 id, const string& entry, D3D11_SHADER_MACRO* macro, ID3D11GeometryShader** gs)
{
	HRESULT hr;

	vector<D3D11_SHADER_MACRO> m;

	PrepareShaderMacro(m, macro, m_shader.model.c_str());

	CComPtr<ID3D11Blob> shader, error;

    hr = D3DX11CompileFromResource(theApp.GetModuleHandle(), MAKEINTRESOURCE(id), NULL, &m[0], NULL, entry.c_str(), m_shader.gs.c_str(), 0, 0, NULL, &shader, &error, NULL);
	
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

HRESULT GSDevice11::CompileShader(uint32 id, const string& entry, D3D11_SHADER_MACRO* macro, ID3D11PixelShader** ps)
{
	HRESULT hr;

	vector<D3D11_SHADER_MACRO> m;

	PrepareShaderMacro(m, macro, m_shader.model.c_str());

	CComPtr<ID3D10Blob> shader, error;

    hr = D3DX11CompileFromResource(theApp.GetModuleHandle(), MAKEINTRESOURCE(id), NULL, &m[0], NULL, entry.c_str(), m_shader.ps.c_str(), 0, 0, NULL, &shader, &error, NULL);
	
	if(error)
	{
		printf("%s\n", (const char*)error->GetBufferPointer());
	}

	if(FAILED(hr))
	{
		return hr;
	}

	hr = m_dev->CreatePixelShader((void*)shader->GetBufferPointer(), shader->GetBufferSize(),NULL,  ps);

	if(FAILED(hr))
	{
		return hr;
	}

	return hr;
}
