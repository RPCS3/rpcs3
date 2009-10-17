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
#include "GSDevice10.h"
#include "resource.h"

GSDevice10::GSDevice10()
{
	memset(&m_state, 0, sizeof(m_state));
	memset(&m_vs_cb_cache, 0, sizeof(m_vs_cb_cache));
	memset(&m_ps_cb_cache, 0, sizeof(m_ps_cb_cache));

	m_state.topology = D3D10_PRIMITIVE_TOPOLOGY_UNDEFINED;
	m_state.bf = -1;
}

GSDevice10::~GSDevice10()
{
}

bool GSDevice10::Create(GSWnd* wnd, bool vsync)
{
	if(!__super::Create(wnd, vsync))
	{
		return false;
	}

	HRESULT hr = E_FAIL;

	DXGI_SWAP_CHAIN_DESC scd;
	D3D10_BUFFER_DESC bd;
	D3D10_SAMPLER_DESC sd;
	D3D10_DEPTH_STENCIL_DESC dsd;
    D3D10_RASTERIZER_DESC rd;
	D3D10_BLEND_DESC bsd;

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

	//Crashes when 2 threads work on the swapchain, as in pcsx2/wx.
	//Todo : Figure out a way to have this flag anyway
	uint32 flags = 0; //D3D10_CREATE_DEVICE_SINGLETHREADED;

#ifdef DEBUG
	flags |= D3D10_CREATE_DEVICE_DEBUG;
#endif

	D3D10_FEATURE_LEVEL1 levels[] = 
	{
		D3D10_FEATURE_LEVEL_10_1, 
		D3D10_FEATURE_LEVEL_10_0
	};

	for(int i = 0; i < countof(levels); i++)
	{
		hr = D3D10CreateDeviceAndSwapChain1(NULL, D3D10_DRIVER_TYPE_HARDWARE, NULL, flags, levels[i], D3D10_1_SDK_VERSION, &scd, &m_swapchain, &m_dev);

		if(SUCCEEDED(hr))
		{
			if(!SetFeatureLevel((D3D_FEATURE_LEVEL)levels[i], true))
			{
				return false;
			}

			break;
		}
	}

	if(FAILED(hr)) return false;

	// msaa

	for(uint32 i = 2; i <= D3D10_MAX_MULTISAMPLE_SAMPLE_COUNT; i++)
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

	// convert

	D3D10_INPUT_ELEMENT_DESC il_convert[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D10_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 16, D3D10_INPUT_PER_VERTEX_DATA, 0},
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

	bsd.BlendEnable[0] = false;
	bsd.RenderTargetWriteMask[0] = D3D10_COLOR_WRITE_ENABLE_ALL;

	hr = m_dev->CreateBlendState(&bsd, &m_convert.bs);

	// merge

	memset(&bd, 0, sizeof(bd));

    bd.ByteWidth = sizeof(MergeConstantBuffer);
    bd.Usage = D3D10_USAGE_DEFAULT;
    bd.BindFlags = D3D10_BIND_CONSTANT_BUFFER;

    hr = m_dev->CreateBuffer(&bd, NULL, &m_merge.cb);

	for(int i = 0; i < countof(m_merge.ps); i++)
	{
		hr = CompileShader(IDR_MERGE_FX, format("ps_main%d", i), NULL, &m_merge.ps[i]);
	}

	memset(&bsd, 0, sizeof(bsd));

	bsd.BlendEnable[0] = true;
	bsd.BlendOp = D3D10_BLEND_OP_ADD;
	bsd.SrcBlend = D3D10_BLEND_SRC_ALPHA;
	bsd.DestBlend = D3D10_BLEND_INV_SRC_ALPHA;
	bsd.BlendOpAlpha = D3D10_BLEND_OP_ADD;
	bsd.SrcBlendAlpha = D3D10_BLEND_ONE;
	bsd.DestBlendAlpha = D3D10_BLEND_ZERO;
	bsd.RenderTargetWriteMask[0] = D3D10_COLOR_WRITE_ENABLE_ALL;

	hr = m_dev->CreateBlendState(&bsd, &m_merge.bs);

	// interlace

	memset(&bd, 0, sizeof(bd));

    bd.ByteWidth = sizeof(InterlaceConstantBuffer);
    bd.Usage = D3D10_USAGE_DEFAULT;
    bd.BindFlags = D3D10_BIND_CONSTANT_BUFFER;

    hr = m_dev->CreateBuffer(&bd, NULL, &m_interlace.cb);

	for(int i = 0; i < countof(m_interlace.ps); i++)
	{
		hr = CompileShader(IDR_INTERLACE_FX, format("ps_main%d", i), NULL, &m_interlace.ps[i]);
	}

	//

	memset(&rd, 0, sizeof(rd));

	rd.FillMode = D3D10_FILL_SOLID;
	rd.CullMode = D3D10_CULL_NONE;
	rd.FrontCounterClockwise = false;
	rd.DepthBias = false;
	rd.DepthBiasClamp = 0;
	rd.SlopeScaledDepthBias = 0;
	rd.DepthClipEnable = false; // ???
	rd.ScissorEnable = true;
	rd.MultisampleEnable = true;
	rd.AntialiasedLineEnable = false;

	hr = m_dev->CreateRasterizerState(&rd, &m_rs);

	m_dev->RSSetState(m_rs);

	//

	memset(&sd, 0, sizeof(sd));

	sd.Filter = D3D10_FILTER_MIN_MAG_MIP_LINEAR;
	sd.AddressU = D3D10_TEXTURE_ADDRESS_CLAMP;
	sd.AddressV = D3D10_TEXTURE_ADDRESS_CLAMP;
	sd.AddressW = D3D10_TEXTURE_ADDRESS_CLAMP;
	sd.MaxLOD = FLT_MAX;
	sd.MaxAnisotropy = 16; 
	sd.ComparisonFunc = D3D10_COMPARISON_NEVER;

	hr = m_dev->CreateSamplerState(&sd, &m_convert.ln);

	sd.Filter = D3D10_FILTER_MIN_MAG_MIP_POINT;

	hr = m_dev->CreateSamplerState(&sd, &m_convert.pt);

	//

	Reset(1, 1);

	//

	CreateTextureFX();

	return true;
}

bool GSDevice10::Reset(int w, int h)
{
	if(!__super::Reset(w, h))
		return false;

	if(m_swapchain)
	{
		DXGI_SWAP_CHAIN_DESC scd;
		memset(&scd, 0, sizeof(scd));
		m_swapchain->GetDesc(&scd);
		m_swapchain->ResizeBuffers(scd.BufferCount, w, h, scd.BufferDesc.Format, 0);
		
		CComPtr<ID3D10Texture2D> backbuffer;
		m_swapchain->GetBuffer(0, __uuidof(ID3D10Texture2D), (void**)&backbuffer);
		m_backbuffer = new GSTexture10(backbuffer);
	}

	return true;
}

void GSDevice10::Flip(bool limit)
{
	m_swapchain->Present(m_vsync && limit ? 1 : 0, 0);
}

void GSDevice10::DrawPrimitive()
{
	m_dev->Draw(m_vertices.count, m_vertices.start);
}

void GSDevice10::ClearRenderTarget(GSTexture* t, const GSVector4& c)
{
	m_dev->ClearRenderTargetView(*(GSTexture10*)t, c.v);
}

void GSDevice10::ClearRenderTarget(GSTexture* t, uint32 c)
{
	GSVector4 color = GSVector4(c) * (1.0f / 255);

	m_dev->ClearRenderTargetView(*(GSTexture10*)t, color.v);
}

void GSDevice10::ClearDepth(GSTexture* t, float c)
{
	m_dev->ClearDepthStencilView(*(GSTexture10*)t, D3D10_CLEAR_DEPTH, c, 0);
}

void GSDevice10::ClearStencil(GSTexture* t, uint8 c)
{
	m_dev->ClearDepthStencilView(*(GSTexture10*)t, D3D10_CLEAR_STENCIL, 0, c);
}

GSTexture* GSDevice10::Create(int type, int w, int h, bool msaa, int format)
{
	HRESULT hr;

	D3D10_TEXTURE2D_DESC desc;

	memset(&desc, 0, sizeof(desc));

	desc.Width = w;
	desc.Height = h;
	desc.Format = (DXGI_FORMAT)format;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D10_USAGE_DEFAULT;

	if(msaa) 
	{
		desc.SampleDesc = m_msaa_desc;
	}

	switch(type)
	{
	case GSTexture::RenderTarget:
		desc.BindFlags = D3D10_BIND_RENDER_TARGET | D3D10_BIND_SHADER_RESOURCE;
		break;
	case GSTexture::DepthStencil:
		desc.BindFlags = D3D10_BIND_DEPTH_STENCIL;// | D3D10_BIND_SHADER_RESOURCE;
		break;
	case GSTexture::Texture:
		desc.BindFlags = D3D10_BIND_SHADER_RESOURCE;
		break;
	case GSTexture::Offscreen:
		desc.Usage = D3D10_USAGE_STAGING;
		desc.CPUAccessFlags |= D3D10_CPU_ACCESS_READ | D3D10_CPU_ACCESS_WRITE;
		break;
	}

	GSTexture10* t = NULL;

	CComPtr<ID3D10Texture2D> texture;

	hr = m_dev->CreateTexture2D(&desc, NULL, &texture);

	if(SUCCEEDED(hr))
	{
		t = new GSTexture10(texture);

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

GSTexture* GSDevice10::CreateRenderTarget(int w, int h, bool msaa, int format)
{
	return __super::CreateRenderTarget(w, h, msaa, format ? format : DXGI_FORMAT_R8G8B8A8_UNORM);
}

GSTexture* GSDevice10::CreateDepthStencil(int w, int h, bool msaa, int format)
{
	return __super::CreateDepthStencil(w, h, msaa, format ? format : DXGI_FORMAT_D32_FLOAT_S8X24_UINT); // DXGI_FORMAT_R32G8X24_TYPELESS
}

GSTexture* GSDevice10::CreateTexture(int w, int h, int format)
{
	return __super::CreateTexture(w, h, format ? format : DXGI_FORMAT_R8G8B8A8_UNORM);
}

GSTexture* GSDevice10::CreateOffscreen(int w, int h, int format)
{
	return __super::CreateOffscreen(w, h, format ? format : DXGI_FORMAT_R8G8B8A8_UNORM);
}

GSTexture* GSDevice10::Resolve(GSTexture* t)
{
	ASSERT(t != NULL && t->IsMSAA());

	if(GSTexture* dst = CreateRenderTarget(t->GetWidth(), t->GetHeight(), false, t->GetFormat()))
	{
		dst->SetScale(t->GetScale());

		m_dev->ResolveSubresource(*(GSTexture10*)dst, 0, *(GSTexture10*)t, 0, (DXGI_FORMAT)t->GetFormat());

		return dst;
	}

	return NULL;
}

GSTexture* GSDevice10::CopyOffscreen(GSTexture* src, const GSVector4& sr, int w, int h, int format)
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
			m_dev->CopyResource(*(GSTexture10*)dst, *(GSTexture10*)rt);
		}

		Recycle(rt);
	}

	return dst;
}

void GSDevice10::CopyRect(GSTexture* st, GSTexture* dt, const GSVector4i& r)
{
	D3D10_BOX box = {r.left, r.top, 0, r.right, r.bottom, 1};

	m_dev->CopySubresourceRegion(*(GSTexture10*)dt, 0, r.left, r.top, 0, *(GSTexture10*)st, 0, &box);
}

void GSDevice10::StretchRect(GSTexture* st, const GSVector4& sr, GSTexture* dt, const GSVector4& dr, int shader, bool linear)
{
	StretchRect(st, sr, dt, dr, m_convert.ps[shader], NULL, linear);
}

void GSDevice10::StretchRect(GSTexture* st, const GSVector4& sr, GSTexture* dt, const GSVector4& dr, ID3D10PixelShader* ps, ID3D10Buffer* ps_cb, bool linear)
{
	StretchRect(st, sr, dt, dr, ps, ps_cb, m_convert.bs, linear);
}

void GSDevice10::StretchRect(GSTexture* st, const GSVector4& sr, GSTexture* dt, const GSVector4& dr, ID3D10PixelShader* ps, ID3D10Buffer* ps_cb, ID3D10BlendState* bs, bool linear)
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
	IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	// vs

	VSSetShader(m_convert.vs, NULL);

	// gs

	GSSetShader(NULL);

	// ps

	PSSetShader(ps, ps_cb);
	PSSetSamplerState(linear ? m_convert.ln : m_convert.pt, NULL);
	PSSetShaderResources(st, NULL);

	//

	DrawPrimitive();

	//

	EndScene();

	PSSetShaderResources(NULL, NULL);
}

void GSDevice10::DoMerge(GSTexture* st[2], GSVector4* sr, GSVector4* dr, GSTexture* dt, bool slbg, bool mmod, const GSVector4& c)
{
	ClearRenderTarget(dt, c);

	if(st[1] && !slbg)
	{
		StretchRect(st[1], sr[1], dt, dr[1], m_merge.ps[0], NULL, true);
	}

	if(st[0])
	{
		m_dev->UpdateSubresource(m_merge.cb, 0, NULL, &c, 0, 0);

		StretchRect(st[0], sr[0], dt, dr[0], m_merge.ps[mmod ? 1 : 0], m_merge.cb, m_merge.bs, true);
	}
}

void GSDevice10::DoInterlace(GSTexture* st, GSTexture* dt, int shader, bool linear, float yoffset)
{
	GSVector4 s = GSVector4(dt->GetSize());

	GSVector4 sr(0, 0, 1, 1);
	GSVector4 dr(0.0f, yoffset, s.x, s.y + yoffset);

	InterlaceConstantBuffer cb;

	cb.ZrH = GSVector2(0, 1.0f / s.y);
	cb.hH = s.y / 2;

	m_dev->UpdateSubresource(m_interlace.cb, 0, NULL, &cb, 0, 0);

	StretchRect(st, sr, dt, dr, m_interlace.ps[shader], m_interlace.cb, linear);
}

void GSDevice10::IASetVertexBuffer(const void* vertices, size_t stride, size_t count)
{
	ASSERT(m_vertices.count == 0);

	if(count * stride > m_vertices.limit * m_vertices.stride)
	{
		m_vb_old = m_vb;
		m_vb = NULL;

		m_vertices.start = 0;
		m_vertices.count = 0;
		m_vertices.limit = std::max<int>(count * 3 / 2, 10000);
	}

	if(m_vb == NULL)
	{
		D3D10_BUFFER_DESC bd;

		memset(&bd, 0, sizeof(bd));

		bd.Usage = D3D10_USAGE_DYNAMIC;
		bd.ByteWidth = m_vertices.limit * stride;
		bd.BindFlags = D3D10_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = D3D10_CPU_ACCESS_WRITE;

		HRESULT hr;
		
		hr = m_dev->CreateBuffer(&bd, NULL, &m_vb);

		if(FAILED(hr)) return;
	}

	D3D10_MAP type = D3D10_MAP_WRITE_NO_OVERWRITE;

	if(m_vertices.start + count > m_vertices.limit || stride != m_vertices.stride)
	{
		m_vertices.start = 0;

		type = D3D10_MAP_WRITE_DISCARD;
	}

	void* v = NULL;

	if(SUCCEEDED(m_vb->Map(type, 0, &v)))
	{
		GSVector4i::storent((uint8*)v + m_vertices.start * stride, vertices, count * stride);

		m_vb->Unmap();
	}

	m_vertices.count = count;
	m_vertices.stride = stride;

	IASetVertexBuffer(m_vb, stride);
}

void GSDevice10::IASetVertexBuffer(ID3D10Buffer* vb, size_t stride)
{
	if(m_state.vb != vb || m_state.vb_stride != stride)
	{
		m_state.vb = vb;
		m_state.vb_stride = stride;

		uint32 offset = 0;

		m_dev->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
	}
}

void GSDevice10::IASetInputLayout(ID3D10InputLayout* layout)
{
	if(m_state.layout != layout)
	{
		m_state.layout = layout;

		m_dev->IASetInputLayout(layout);
	}
}

void GSDevice10::IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY topology)
{
	if(m_state.topology != topology)
	{
		m_state.topology = topology;

		m_dev->IASetPrimitiveTopology(topology);
	}
}

void GSDevice10::VSSetShader(ID3D10VertexShader* vs, ID3D10Buffer* vs_cb)
{
	if(m_state.vs != vs)
	{
		m_state.vs = vs;

		m_dev->VSSetShader(vs);
	}
	
	if(m_state.vs_cb != vs_cb)
	{
		m_state.vs_cb = vs_cb;

		m_dev->VSSetConstantBuffers(0, 1, &vs_cb);
	}
}

void GSDevice10::GSSetShader(ID3D10GeometryShader* gs)
{
	if(m_state.gs != gs)
	{
		m_state.gs = gs;

		m_dev->GSSetShader(gs);
	}
}

void GSDevice10::PSSetShaderResources(GSTexture* sr0, GSTexture* sr1)
{
	ID3D10ShaderResourceView* srv0 = NULL;
	ID3D10ShaderResourceView* srv1 = NULL;

	if(sr0) srv0 = *(GSTexture10*)sr0;
	if(sr1) srv1 = *(GSTexture10*)sr1;

	if(m_state.ps_srv[0] != srv0 || m_state.ps_srv[1] != srv1)
	{
		m_state.ps_srv[0] = srv0;
		m_state.ps_srv[1] = srv1;

		ID3D10ShaderResourceView* srvs[] = {srv0, srv1};
	
		m_dev->PSSetShaderResources(0, 2, srvs);
	}
}

void GSDevice10::PSSetShader(ID3D10PixelShader* ps, ID3D10Buffer* ps_cb)
{
	if(m_state.ps != ps)
	{
		m_state.ps = ps;

		m_dev->PSSetShader(ps);
	}
	
	if(m_state.ps_cb != ps_cb)
	{
		m_state.ps_cb = ps_cb;

		m_dev->PSSetConstantBuffers(0, 1, &ps_cb);
	}
}

void GSDevice10::PSSetSamplerState(ID3D10SamplerState* ss0, ID3D10SamplerState* ss1)
{
	if(m_state.ps_ss[0] != ss0 || m_state.ps_ss[1] != ss1)
	{
		m_state.ps_ss[0] = ss0;
		m_state.ps_ss[1] = ss1;

		ID3D10SamplerState* sss[] = {ss0, ss1};

		m_dev->PSSetSamplers(0, 2, sss);
	}
}

void GSDevice10::OMSetDepthStencilState(ID3D10DepthStencilState* dss, uint8 sref)
{
	if(m_state.dss != dss || m_state.sref != sref)
	{
		m_state.dss = dss;
		m_state.sref = sref;

		m_dev->OMSetDepthStencilState(dss, sref);
	}
}

void GSDevice10::OMSetBlendState(ID3D10BlendState* bs, float bf)
{
	if(m_state.bs != bs || m_state.bf != bf)
	{
		m_state.bs = bs;
		m_state.bf = bf;

		float BlendFactor[] = {bf, bf, bf, 0};

		m_dev->OMSetBlendState(bs, BlendFactor, 0xffffffff);
	}
}

void GSDevice10::OMSetRenderTargets(GSTexture* rt, GSTexture* ds, const GSVector4i* scissor)
{
	ID3D10RenderTargetView* rtv = NULL;
	ID3D10DepthStencilView* dsv = NULL;

	if(rt) rtv = *(GSTexture10*)rt;
	if(ds) dsv = *(GSTexture10*)ds;

	if(m_state.rtv != rtv || m_state.dsv != dsv)
	{
		m_state.rtv = rtv;
		m_state.dsv = dsv;

		m_dev->OMSetRenderTargets(1, &rtv, dsv);
	}

	if(m_state.viewport != rt->GetSize())
	{
		m_state.viewport = rt->GetSize();

		D3D10_VIEWPORT vp;

		memset(&vp, 0, sizeof(vp));
		
		vp.TopLeftX = 0;
		vp.TopLeftY = 0;
		vp.Width = rt->GetWidth();
		vp.Height = rt->GetHeight();
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;

		m_dev->RSSetViewports(1, &vp);
	}

	GSVector4i r = scissor ? *scissor : GSVector4i(rt->GetSize()).zwxy();

	if(!m_state.scissor.eq(r))
	{
		m_state.scissor = r;

		m_dev->RSSetScissorRects(1, r);
	}
}

HRESULT GSDevice10::CompileShader(uint32 id, const string& entry, D3D10_SHADER_MACRO* macro, ID3D10VertexShader** vs, D3D10_INPUT_ELEMENT_DESC* layout, int count, ID3D10InputLayout** il)
{
	HRESULT hr;

	vector<D3D10_SHADER_MACRO> m;

	PrepareShaderMacro(m, macro);

	CComPtr<ID3D10Blob> shader, error;

    hr = D3DX10CompileFromResource(theApp.GetModuleHandle(), MAKEINTRESOURCE(id), NULL, &m[0], NULL, entry.c_str(), m_shader.vs.c_str(), 0, 0, NULL, &shader, &error, NULL);
	
	if(error)
	{
		printf("%s\n", (const char*)error->GetBufferPointer());
	}

	if(FAILED(hr))
	{
		return hr;
	}

	hr = m_dev->CreateVertexShader((void*)shader->GetBufferPointer(), shader->GetBufferSize(), vs);

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

HRESULT GSDevice10::CompileShader(uint32 id, const string& entry, D3D10_SHADER_MACRO* macro, ID3D10GeometryShader** gs)
{
	HRESULT hr;

	vector<D3D10_SHADER_MACRO> m;

	PrepareShaderMacro(m, macro);

	CComPtr<ID3D10Blob> shader, error;

    hr = D3DX10CompileFromResource(theApp.GetModuleHandle(), MAKEINTRESOURCE(id), NULL, &m[0], NULL, entry.c_str(), m_shader.gs.c_str(), 0, 0, NULL, &shader, &error, NULL);
	
	if(error)
	{
		printf("%s\n", (const char*)error->GetBufferPointer());
	}

	if(FAILED(hr))
	{
		return hr;
	}

	hr = m_dev->CreateGeometryShader((void*)shader->GetBufferPointer(), shader->GetBufferSize(), gs);

	if(FAILED(hr))
	{
		return hr;
	}

	return hr;
}

HRESULT GSDevice10::CompileShader(uint32 id, const string& entry, D3D10_SHADER_MACRO* macro, ID3D10PixelShader** ps)
{
	HRESULT hr;

	vector<D3D10_SHADER_MACRO> m;

	PrepareShaderMacro(m, macro);

	CComPtr<ID3D10Blob> shader, error;

    hr = D3DX10CompileFromResource(theApp.GetModuleHandle(), MAKEINTRESOURCE(id), NULL, &m[0], NULL, entry.c_str(), m_shader.ps.c_str(), 0, 0, NULL, &shader, &error, NULL);
	
	if(error)
	{
		printf("%s\n", (const char*)error->GetBufferPointer());
	}

	if(FAILED(hr))
	{
		return hr;
	}

	hr = m_dev->CreatePixelShader((void*)shader->GetBufferPointer(), shader->GetBufferSize(), ps);

	if(FAILED(hr))
	{
		return hr;
	}

	return hr;
}
