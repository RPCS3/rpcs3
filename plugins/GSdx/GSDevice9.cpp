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
#include "GSDevice9.h"
#include "resource.h"

GSDevice9::GSDevice9() 
	: m_lost(false)
{
	m_rbswapped = true;

	memset(&m_pp, 0, sizeof(m_pp));
	//memset(&m_ddcaps, 0, sizeof(m_ddcaps)); // Unreferenced
	memset(&m_d3dcaps, 0, sizeof(m_d3dcaps));

	memset(&m_state, 0, sizeof(m_state));

	m_state.bf = 0xffffffff;
}

GSDevice9::~GSDevice9()
{
	for_each(m_mskfix.begin(), m_mskfix.end(), delete_second());
	
	for_each(m_om_bs.begin(), m_om_bs.end(), delete_second());
	for_each(m_om_dss.begin(), m_om_dss.end(), delete_second());
	for_each(m_ps_ss.begin(), m_ps_ss.end(), delete_second());

	if(m_state.vs_cb) _aligned_free(m_state.vs_cb);
	if(m_state.ps_cb) _aligned_free(m_state.ps_cb);
}

bool GSDevice9::Create(GSWnd* wnd)
{
	if(!__super::Create(wnd))
	{
		return false;
	}

	HRESULT hr;

	// dd

	// Unreferenced
	/*CComPtr<IDirectDraw7> dd; 

	hr = DirectDrawCreateEx(0, (void**)&dd, IID_IDirectDraw7, 0);

	if(FAILED(hr)) return false;

	memset(&m_ddcaps, 0, sizeof(m_ddcaps));

	m_ddcaps.dwSize = sizeof(DDCAPS); 

	hr = dd->GetCaps(&m_ddcaps, NULL);

	if(FAILED(hr)) return false;

	dd = NULL;*/

	// d3d

	m_d3d.Attach(Direct3DCreate9(D3D_SDK_VERSION));

	if(!m_d3d) return false;

	hr = m_d3d->CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, D3DFMT_D24S8);

	if(FAILED(hr)) return false;

	hr = m_d3d->CheckDepthStencilMatch(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8, D3DFMT_X8R8G8B8, D3DFMT_D24S8);

	if(FAILED(hr)) return false;

	memset(&m_d3dcaps, 0, sizeof(m_d3dcaps));

	m_d3d->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &m_d3dcaps);

	//

	if(m_d3dcaps.VertexShaderVersion < (m_d3dcaps.PixelShaderVersion & ~0x10000))
	{
		if(m_d3dcaps.VertexShaderVersion > D3DVS_VERSION(0, 0))
		{
			ASSERT(0);

			return false;
		}

		// else vertex shader should be emulated in software (gma950)
	}

	m_d3dcaps.VertexShaderVersion = m_d3dcaps.PixelShaderVersion & ~0x10000;

	if(m_d3dcaps.PixelShaderVersion >= D3DPS_VERSION(3, 0))
	{
		SetFeatureLevel(D3D_FEATURE_LEVEL_9_3, false);
	}
	else if(m_d3dcaps.PixelShaderVersion >= D3DPS_VERSION(2, 0))
	{
		SetFeatureLevel(D3D_FEATURE_LEVEL_9_2, false);
	}
	else
	{
		string s = format(
			"Supported pixel shader version is too low!\n\nSupported: %d.%d\nNeeded: 2.0 or higher",
			D3DSHADER_VERSION_MAJOR(m_d3dcaps.PixelShaderVersion), D3DSHADER_VERSION_MINOR(m_d3dcaps.PixelShaderVersion));

		MessageBox(NULL, s.c_str(), "GSdx", MB_OK);

		return false;
	}

	// msaa

	for(uint32 i = 2; i <= 16; i++)
	{
		DWORD quality[2] = {0, 0};

		if(SUCCEEDED(m_d3d->CheckDeviceMultiSampleType(m_d3dcaps.AdapterOrdinal, m_d3dcaps.DeviceType, D3DFMT_A8R8G8B8, TRUE, (D3DMULTISAMPLE_TYPE)i, &quality[0])) && quality[0] > 0
		&& SUCCEEDED(m_d3d->CheckDeviceMultiSampleType(m_d3dcaps.AdapterOrdinal, m_d3dcaps.DeviceType, D3DFMT_D24S8, TRUE, (D3DMULTISAMPLE_TYPE)i, &quality[1])) && quality[1] > 0)
		{
			m_msaa_desc.Count = i;
			m_msaa_desc.Quality = std::min<DWORD>(quality[0] - 1, quality[1] - 1);

			if(i >= m_msaa) break;
		}
	}

	//

	if(!Reset(1, 1)) 
	{
		return false;
	}

	m_dev->Clear(0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0);

	// convert

	static const D3DVERTEXELEMENT9 il_convert[] =
	{
		{0, 0,  D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
		{0, 16, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
		D3DDECL_END()
	};

	CompileShader(IDR_CONVERT_FX, "vs_main", NULL, &m_convert.vs, il_convert, countof(il_convert), &m_convert.il);

	for(int i = 0; i < countof(m_convert.ps); i++)
	{
		CompileShader(IDR_CONVERT_FX, format("ps_main%d", i), NULL, &m_convert.ps[i]);
	}

	m_convert.dss.DepthEnable = false;
	m_convert.dss.StencilEnable = false;

	m_convert.bs.BlendEnable = false;
	m_convert.bs.RenderTargetWriteMask = D3DCOLORWRITEENABLE_RGBA;

	m_convert.ln.FilterMin[0] = D3DTEXF_LINEAR;
	m_convert.ln.FilterMag[0] = D3DTEXF_LINEAR;
	m_convert.ln.FilterMin[1] = D3DTEXF_LINEAR;
	m_convert.ln.FilterMag[1] = D3DTEXF_LINEAR;
	m_convert.ln.AddressU = D3DTADDRESS_CLAMP;
	m_convert.ln.AddressV = D3DTADDRESS_CLAMP;

	m_convert.pt.FilterMin[0] = D3DTEXF_POINT;
	m_convert.pt.FilterMag[0] = D3DTEXF_POINT;
	m_convert.pt.FilterMin[1] = D3DTEXF_POINT;
	m_convert.pt.FilterMag[1] = D3DTEXF_POINT;
	m_convert.pt.AddressU = D3DTADDRESS_CLAMP;
	m_convert.pt.AddressV = D3DTADDRESS_CLAMP;

	// merge

	for(int i = 0; i < countof(m_merge.ps); i++)
	{
		CompileShader(IDR_MERGE_FX, format("ps_main%d", i), NULL, &m_merge.ps[i]);
	}

	m_merge.bs.BlendEnable = true;
	m_merge.bs.BlendOp = D3DBLENDOP_ADD;
	m_merge.bs.SrcBlend = D3DBLEND_SRCALPHA;
	m_merge.bs.DestBlend = D3DBLEND_INVSRCALPHA;
	m_merge.bs.BlendOpAlpha = D3DBLENDOP_ADD;
	m_merge.bs.SrcBlendAlpha = D3DBLEND_ONE;
	m_merge.bs.DestBlendAlpha = D3DBLEND_ZERO;
	m_merge.bs.RenderTargetWriteMask = D3DCOLORWRITEENABLE_RGBA;

	// interlace

	for(int i = 0; i < countof(m_interlace.ps); i++)
	{
		CompileShader(IDR_INTERLACE_FX, format("ps_main%d", i), NULL, &m_interlace.ps[i]);
	}

	// create shader layout

	VSSelector sel;
	VSConstantBuffer cb;

	SetupVS(sel, &cb);

	//

	memset(&m_date.dss, 0, sizeof(m_date.dss));

	m_date.dss.StencilEnable = true;
	m_date.dss.StencilReadMask = 1;
	m_date.dss.StencilWriteMask = 1;
	m_date.dss.StencilFunc = D3DCMP_ALWAYS;
	m_date.dss.StencilPassOp = D3DSTENCILOP_REPLACE;
	m_date.dss.StencilRef = 1;

	memset(&m_date.bs, 0, sizeof(m_date.bs));

	//

	return true;
}

void GSDevice9::SetVsync(bool enable)
{
	if( m_vsync == enable ) return;
	__super::SetVsync(enable);

	// Clever trick:  Delete the backbuffer, so that the next Present will fail and
	// cause a DXDevice9::Reset call, which re-creates the backbuffer with current
	// vsync settings. :)

	delete m_backbuffer;
	m_backbuffer = NULL;
}

bool GSDevice9::Reset(int w, int h)
{
	if(!__super::Reset(w, h))
		return false;

	HRESULT hr;

	int mode = (!m_wnd->IsManaged() || theApp.GetConfig("windowed", 1)) ? Windowed : Fullscreen;
	if(mode == DontCare)
	{
		mode = m_pp.Windowed ? Windowed : Fullscreen;
	}

	if(!m_lost)
	{
		if(m_swapchain && mode != Fullscreen && m_pp.Windowed)
		{
			m_swapchain = NULL;

			m_pp.BackBufferWidth = w;
			m_pp.BackBufferHeight = h;
			m_pp.PresentationInterval = m_vsync ? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_IMMEDIATE;

			hr = m_dev->CreateAdditionalSwapChain(&m_pp, &m_swapchain);

			if(FAILED(hr)) return false;

			CComPtr<IDirect3DSurface9> backbuffer;
			hr = m_swapchain->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &backbuffer);
			m_backbuffer = new GSTexture9(backbuffer);

			return true;
		}
	}

	m_swapchain = NULL;

	m_vb = NULL;
	m_vb_old = NULL;

	m_vertices.start = 0;
	m_vertices.count = 0;

	if(m_state.vs_cb) _aligned_free(m_state.vs_cb);
	if(m_state.ps_cb) _aligned_free(m_state.ps_cb);

	memset(&m_state, 0, sizeof(m_state));

	m_state.bf = 0xffffffff;

	memset(&m_pp, 0, sizeof(m_pp));

	m_pp.Windowed = TRUE;
	m_pp.hDeviceWindow = (HWND)m_wnd->GetHandle();
	m_pp.SwapEffect = D3DSWAPEFFECT_FLIP;
	m_pp.BackBufferFormat = D3DFMT_X8R8G8B8;
	m_pp.BackBufferWidth = 1;
	m_pp.BackBufferHeight = 1;
	m_pp.PresentationInterval = m_vsync ? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_IMMEDIATE;

	// m_pp.Flags |= D3DPRESENTFLAG_VIDEO; // enables tv-out (but I don't think anyone would still use a regular tv...)

	int mw = theApp.GetConfig("ModeWidth", 0);
	int mh = theApp.GetConfig("ModeHeight", 0);
	int mrr = theApp.GetConfig("ModeRefreshRate", 0);

	if(m_wnd->IsManaged() && mode == Fullscreen && mw > 0 && mh > 0 && mrr >= 0)
	{
		m_pp.Windowed = FALSE;
		m_pp.BackBufferWidth = mw;
		m_pp.BackBufferHeight = mh;
		// m_pp.FullScreen_RefreshRateInHz = mrr;

		m_wnd->HideFrame();
	}

	if(!m_dev)
	{
		uint32 flags = m_d3dcaps.VertexProcessingCaps ? D3DCREATE_HARDWARE_VERTEXPROCESSING : D3DCREATE_SOFTWARE_VERTEXPROCESSING;

		if(flags & D3DCREATE_HARDWARE_VERTEXPROCESSING)
		{
			flags |= D3DCREATE_PUREDEVICE;
		}
 
		hr = m_d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, (HWND)m_wnd->GetHandle(), flags, &m_pp, &m_dev);

		if(FAILED(hr)) return false;
	}
	else
	{
		hr = m_dev->Reset(&m_pp);

		if(FAILED(hr))
		{
			if(D3DERR_DEVICELOST == hr)
			{
				Sleep(1000);

				hr = m_dev->Reset(&m_pp);
			}

			if(FAILED(hr)) return false;
		}
	}

	if(m_pp.Windowed)
	{
		m_pp.BackBufferWidth = 1;
		m_pp.BackBufferHeight = 1;

		hr = m_dev->CreateAdditionalSwapChain(&m_pp, &m_swapchain);

		if(FAILED(hr)) return false;
	}

	CComPtr<IDirect3DSurface9> backbuffer;

	if(m_swapchain)
	{
		hr = m_swapchain->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &backbuffer);
	}
	else
	{
		hr = m_dev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &backbuffer);
	}

	m_backbuffer = new GSTexture9(backbuffer);

	m_dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	m_dev->SetRenderState(D3DRS_LIGHTING, FALSE);
	m_dev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
	m_dev->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);

	return true;
}

bool GSDevice9::IsLost(bool update)
{
	if(!m_lost || update)
	{
		HRESULT hr = m_dev->TestCooperativeLevel();

		m_lost = hr == D3DERR_DEVICELOST || hr == D3DERR_DEVICENOTRESET;
	}

	return m_lost;
}

void GSDevice9::Flip()
{
	m_dev->EndScene();

	HRESULT hr;

	if(m_swapchain)
	{
		hr = m_swapchain->Present(NULL, NULL, NULL, NULL, 0);
	}
	else
	{
		hr = m_dev->Present(NULL, NULL, NULL, NULL);
	}

	m_dev->BeginScene();

	if(FAILED(hr))
	{
		m_lost = true;
	}
}

void GSDevice9::BeginScene()
{
	// m_dev->BeginScene();
}

void GSDevice9::DrawPrimitive()
{
	int prims = 0;

	switch(m_state.topology)
	{
    case D3DPT_TRIANGLELIST:
		prims = m_vertices.count / 3;
		break;
    case D3DPT_LINELIST:
		prims = m_vertices.count / 2;
		break;
    case D3DPT_POINTLIST:
		prims = m_vertices.count;
		break;
    case D3DPT_TRIANGLESTRIP:
    case D3DPT_TRIANGLEFAN:
		prims = m_vertices.count - 2;
		break;
    case D3DPT_LINESTRIP:
		prims = m_vertices.count - 1;
		break;
	}

	m_dev->DrawPrimitive(m_state.topology, m_vertices.start, prims);
}

void GSDevice9::EndScene()
{
	// m_dev->EndScene();

	__super::EndScene();
}

void GSDevice9::ClearRenderTarget(GSTexture* t, const GSVector4& c)
{
	ClearRenderTarget(t, (c * 255 + 0.5f).zyxw().rgba32());
}

void GSDevice9::ClearRenderTarget(GSTexture* rt, uint32 c)
{
	CComPtr<IDirect3DSurface9> surface;
	m_dev->GetRenderTarget(0, &surface);
	m_dev->SetRenderTarget(0, *(GSTexture9*)rt);
	m_dev->Clear(0, NULL, D3DCLEAR_TARGET, c, 0, 0);
	m_dev->SetRenderTarget(0, surface);
}

void GSDevice9::ClearDepth(GSTexture* t, float c)
{
	CComPtr<IDirect3DSurface9> dssurface;
	m_dev->GetDepthStencilSurface(&dssurface);
	m_dev->SetDepthStencilSurface(*(GSTexture9*)t);
	m_dev->Clear(0, NULL, D3DCLEAR_ZBUFFER, 0, c, 0);
	m_dev->SetDepthStencilSurface(dssurface);
}

void GSDevice9::ClearStencil(GSTexture* t, uint8 c)
{
	CComPtr<IDirect3DSurface9> dssurface;
	m_dev->GetDepthStencilSurface(&dssurface);
	m_dev->SetDepthStencilSurface(*(GSTexture9*)t);
	m_dev->Clear(0, NULL, D3DCLEAR_STENCIL, 0, 0, c);
	m_dev->SetDepthStencilSurface(dssurface);
}

GSTexture* GSDevice9::Create(int type, int w, int h, bool msaa, int format)
{
	HRESULT hr;

	CComPtr<IDirect3DTexture9> texture;
	CComPtr<IDirect3DSurface9> surface;

	switch(type)
	{
	case GSTexture::RenderTarget:
		if(msaa) hr = m_dev->CreateRenderTarget(w, h, (D3DFORMAT)format, (D3DMULTISAMPLE_TYPE)m_msaa_desc.Count, m_msaa_desc.Quality, FALSE, &surface, NULL);
		else hr = m_dev->CreateTexture(w, h, 1, D3DUSAGE_RENDERTARGET, (D3DFORMAT)format, D3DPOOL_DEFAULT, &texture, NULL);
		break;
	case GSTexture::DepthStencil:
		if(msaa) hr = m_dev->CreateDepthStencilSurface(w, h, (D3DFORMAT)format, (D3DMULTISAMPLE_TYPE)m_msaa_desc.Count, m_msaa_desc.Quality, FALSE, &surface, NULL);
		else hr = m_dev->CreateDepthStencilSurface(w, h, (D3DFORMAT)format, D3DMULTISAMPLE_NONE, 0, FALSE, &surface, NULL);
		break;
	case GSTexture::Texture:
		hr = m_dev->CreateTexture(w, h, 1, 0, (D3DFORMAT)format, D3DPOOL_MANAGED, &texture, NULL);
		break;
	case GSTexture::Offscreen:
		hr = m_dev->CreateOffscreenPlainSurface(w, h, (D3DFORMAT)format, D3DPOOL_SYSTEMMEM, &surface, NULL);
		break;
	}

	GSTexture9* t = NULL;

	if(surface)
	{
		t = new GSTexture9(surface);
	}

	if(texture)
	{
		t = new GSTexture9(texture);
	}

	if(t)
	{
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

GSTexture* GSDevice9::CreateRenderTarget(int w, int h, bool msaa, int format)
{
	return __super::CreateRenderTarget(w, h, msaa, format ? format : D3DFMT_A8R8G8B8);
}

GSTexture* GSDevice9::CreateDepthStencil(int w, int h, bool msaa, int format)
{
	return __super::CreateDepthStencil(w, h, msaa, format ? format : D3DFMT_D24S8);
}

GSTexture* GSDevice9::CreateTexture(int w, int h, int format)
{
	return __super::CreateTexture(w, h, format ? format : D3DFMT_A8R8G8B8);
}

GSTexture* GSDevice9::CreateOffscreen(int w, int h, int format)
{
	return __super::CreateOffscreen(w, h, format ? format : D3DFMT_A8R8G8B8);
}

GSTexture* GSDevice9::Resolve(GSTexture* t)
{
	ASSERT(t != NULL && t->IsMSAA());

	if(GSTexture* dst = CreateRenderTarget(t->GetWidth(), t->GetHeight(), false, t->GetFormat()))
	{
		dst->SetScale(t->GetScale());

		m_dev->StretchRect(*(GSTexture9*)t, NULL, *(GSTexture9*)dst, NULL, D3DTEXF_POINT);

		return dst;
	}

	return NULL;
}

GSTexture* GSDevice9::CopyOffscreen(GSTexture* src, const GSVector4& sr, int w, int h, int format)
{
	GSTexture* dst = NULL;

	if(format == 0)
	{
		format = D3DFMT_A8R8G8B8;
	}

	if(format != D3DFMT_A8R8G8B8)
	{
		ASSERT(0);

		return false;
	}

	if(GSTexture* rt = CreateRenderTarget(w, h, false, format))
	{
		GSVector4 dr(0, 0, w, h);

		if(GSTexture* src2 = src->IsMSAA() ? Resolve(src) : src)
		{
			StretchRect(src2, sr, rt, dr, m_convert.ps[1], NULL, 0);

			if(src2 != src) Recycle(src2);
		}

		dst = CreateOffscreen(w, h, format);

		if(dst)
		{
			m_dev->GetRenderTargetData(*(GSTexture9*)rt, *(GSTexture9*)dst);
		}

		Recycle(rt);
	}

	return dst;
}

void GSDevice9::CopyRect(GSTexture* st, GSTexture* dt, const GSVector4i& r)
{
	m_dev->StretchRect(*(GSTexture9*)st, r, *(GSTexture9*)dt, r, D3DTEXF_NONE);
}

void GSDevice9::StretchRect(GSTexture* st, const GSVector4& sr, GSTexture* dt, const GSVector4& dr, int shader, bool linear)
{
	StretchRect(st, sr, dt, dr, m_convert.ps[shader], NULL, 0, linear);
}

void GSDevice9::StretchRect(GSTexture* st, const GSVector4& sr, GSTexture* dt, const GSVector4& dr, IDirect3DPixelShader9* ps, const float* ps_cb, int ps_cb_len, bool linear)
{
	StretchRect(st, sr, dt, dr, ps, ps_cb, ps_cb_len, &m_convert.bs, linear);
}

void GSDevice9::StretchRect(GSTexture* st, const GSVector4& sr, GSTexture* dt, const GSVector4& dr, IDirect3DPixelShader9* ps, const float* ps_cb, int ps_cb_len, Direct3DBlendState9* bs, bool linear)
{
	BeginScene();

	GSVector2i ds = dt->GetSize();

	// om

	OMSetDepthStencilState(&m_convert.dss);
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

	for(int i = 0; i < countof(vertices); i++)
	{
		vertices[i].p.x -= 1.0f / ds.x;
		vertices[i].p.y += 1.0f / ds.y;
	}

	IASetVertexBuffer(vertices, sizeof(vertices[0]), countof(vertices));
	IASetPrimitiveTopology(D3DPT_TRIANGLESTRIP);

	// vs

	VSSetShader(m_convert.vs, NULL, 0);
	IASetInputLayout(m_convert.il);

	// ps

	PSSetShader(ps, ps_cb, ps_cb_len);
	PSSetSamplerState(linear ? &m_convert.ln : &m_convert.pt);
	PSSetShaderResources(st, NULL);

	//

	DrawPrimitive();

	//

	EndScene();
}

void GSDevice9::DoMerge(GSTexture* st[2], GSVector4* sr, GSVector4* dr, GSTexture* dt, bool slbg, bool mmod, const GSVector4& c)
{
	ClearRenderTarget(dt, c);

	if(st[1] && !slbg)
	{
		StretchRect(st[1], sr[1], dt, dr[1], m_merge.ps[0], NULL, true);
	}

	if(st[0])
	{
		MergeConstantBuffer cb;

		cb.BGColor = c;

		StretchRect(st[0], sr[0], dt, dr[0], m_merge.ps[mmod ? 1 : 0], (const float*)&cb, 1, &m_merge.bs, true);
	}
}

void GSDevice9::DoInterlace(GSTexture* st, GSTexture* dt, int shader, bool linear, float yoffset)
{
	GSVector4 s = GSVector4(dt->GetSize());

	GSVector4 sr(0, 0, 1, 1);
	GSVector4 dr(0.0f, yoffset, s.x, s.y + yoffset);

	InterlaceConstantBuffer cb;

	cb.ZrH = GSVector2(0, 1.0f / s.y);
	cb.hH = (float)s.y / 2;

	StretchRect(st, sr, dt, dr, m_interlace.ps[shader], (const float*)&cb, 1, linear);
}

void GSDevice9::SetupDATE(GSTexture* rt, GSTexture* ds, const GSVertexPT1 (&iaVertices)[4], bool datm)
{
	const GSVector2i& size = rt->GetSize();

	if(GSTexture* t = CreateRenderTarget(size.x, size.y, rt->IsMSAA()))
	{
		// sfex3 (after the capcom logo), vf4 (first menu fading in), ffxii shadows, rumble roses shadows, persona4 shadows

		BeginScene();

		ClearStencil(ds, 0);

		// om

		OMSetDepthStencilState(&m_date.dss);
		OMSetBlendState(&m_date.bs, 0);
		OMSetRenderTargets(t, ds);

		// ia

		IASetVertexBuffer(iaVertices, sizeof(iaVertices[0]), countof(iaVertices));
		IASetPrimitiveTopology(D3DPT_TRIANGLESTRIP);

		// vs

		VSSetShader(m_convert.vs, NULL, 0);
		IASetInputLayout(m_convert.il);

		// ps

		GSTexture* rt2 = rt->IsMSAA() ? Resolve(rt) : rt;

		PSSetShaderResources(rt2, NULL);
		PSSetShader(m_convert.ps[datm ? 2 : 3], NULL, 0);
		PSSetSamplerState(&m_convert.pt);

		//

		DrawPrimitive();

		//

		EndScene();

		Recycle(t);

		if(rt2 != rt) Recycle(rt2);
	}
}

void GSDevice9::IASetVertexBuffer(const void* vertices, size_t stride, size_t count)
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
		HRESULT hr;
		
		hr = m_dev->CreateVertexBuffer(m_vertices.limit * stride, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, &m_vb, NULL);

		if(FAILED(hr)) return;
	}

	uint32 flags = D3DLOCK_NOOVERWRITE;

	if(m_vertices.start + count > m_vertices.limit || stride != m_vertices.stride)
	{
		m_vertices.start = 0;

		flags = D3DLOCK_DISCARD;
	}

	void* v = NULL;

	if(SUCCEEDED(m_vb->Lock(m_vertices.start * stride, count * stride, &v, flags)))
	{
		GSVector4i::storent(v, vertices, count * stride);

		m_vb->Unlock();
	}

	m_vertices.count = count;
	m_vertices.stride = stride;

	IASetVertexBuffer(m_vb, stride);
}

void GSDevice9::IASetVertexBuffer(IDirect3DVertexBuffer9* vb, size_t stride)
{
	if(m_state.vb != vb || m_state.vb_stride != stride)
	{
		m_state.vb = vb;
		m_state.vb_stride = stride;

		m_dev->SetStreamSource(0, vb, 0, stride);
	}
}

void GSDevice9::IASetInputLayout(IDirect3DVertexDeclaration9* layout)
{
	if(m_state.layout != layout)
	{
		m_state.layout = layout;

		m_dev->SetVertexDeclaration(layout);
	}
}

void GSDevice9::IASetPrimitiveTopology(D3DPRIMITIVETYPE topology)
{
	m_state.topology = topology;
}

void GSDevice9::VSSetShader(IDirect3DVertexShader9* vs, const float* vs_cb, int vs_cb_len)
{
	if(m_state.vs != vs)
	{
		m_state.vs = vs;

		m_dev->SetVertexShader(vs);
	}

	if(vs_cb && vs_cb_len > 0)
	{
		int size = vs_cb_len * sizeof(float) * 4;
		
		if(m_state.vs_cb_len != vs_cb_len || m_state.vs_cb == NULL || memcmp(m_state.vs_cb, vs_cb, size))
		{
			if(m_state.vs_cb == NULL || m_state.vs_cb_len < vs_cb_len)
			{
				if(m_state.vs_cb) _aligned_free(m_state.vs_cb);

				m_state.vs_cb = (float*)_aligned_malloc(size, 16);
			}

			m_state.vs_cb_len = vs_cb_len;

			memcpy(m_state.vs_cb, vs_cb, size);

			m_dev->SetVertexShaderConstantF(0, vs_cb, vs_cb_len);
		}
	}
}

void GSDevice9::PSSetShaderResources(GSTexture* sr0, GSTexture* sr1)
{
	IDirect3DTexture9* srv0 = NULL;
	IDirect3DTexture9* srv1 = NULL;
	
	if(sr0) srv0 = *(GSTexture9*)sr0;
	if(sr1) srv1 = *(GSTexture9*)sr1;

	if(m_state.ps_srvs[0] != srv0)
	{
		m_state.ps_srvs[0] = srv0;

		m_dev->SetTexture(0, srv0);
	}

	if(m_state.ps_srvs[1] != srv1)
	{
		m_state.ps_srvs[1] = srv1;

		m_dev->SetTexture(1, srv1);
	}
}

void GSDevice9::PSSetShader(IDirect3DPixelShader9* ps, const float* ps_cb, int ps_cb_len)
{
	if(m_state.ps != ps)
	{
		m_state.ps = ps;

		m_dev->SetPixelShader(ps);
	}
	
	if(ps_cb && ps_cb_len > 0)
	{
		int size = ps_cb_len * sizeof(float) * 4;
		
		if(m_state.ps_cb_len != ps_cb_len || m_state.ps_cb == NULL || memcmp(m_state.ps_cb, ps_cb, size))
		{
			if(m_state.ps_cb == NULL || m_state.ps_cb_len < ps_cb_len)
			{
				if(m_state.ps_cb) _aligned_free(m_state.ps_cb);

				m_state.ps_cb = (float*)_aligned_malloc(size, 16);
			}

			m_state.ps_cb_len = ps_cb_len;

			memcpy(m_state.ps_cb, ps_cb, size);

			m_dev->SetPixelShaderConstantF(0, ps_cb, ps_cb_len);
		}
	}
}

void GSDevice9::PSSetSamplerState(Direct3DSamplerState9* ss)
{
	if(ss && m_state.ps_ss != ss)
	{
		m_state.ps_ss = ss;

		m_dev->SetSamplerState(0, D3DSAMP_ADDRESSU, ss->AddressU);
		m_dev->SetSamplerState(0, D3DSAMP_ADDRESSV, ss->AddressV);
		m_dev->SetSamplerState(1, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
		m_dev->SetSamplerState(1, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
		m_dev->SetSamplerState(2, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
		m_dev->SetSamplerState(2, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
		m_dev->SetSamplerState(3, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
		m_dev->SetSamplerState(3, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
		m_dev->SetSamplerState(0, D3DSAMP_MINFILTER, ss->FilterMin[0]);
		m_dev->SetSamplerState(0, D3DSAMP_MAGFILTER, ss->FilterMag[0]);
		m_dev->SetSamplerState(1, D3DSAMP_MINFILTER, D3DTEXF_POINT);
		m_dev->SetSamplerState(1, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
		m_dev->SetSamplerState(2, D3DSAMP_MINFILTER, D3DTEXF_POINT);
		m_dev->SetSamplerState(2, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
		m_dev->SetSamplerState(3, D3DSAMP_MINFILTER, D3DTEXF_POINT);
		m_dev->SetSamplerState(3, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
	}
}

void GSDevice9::OMSetDepthStencilState(Direct3DDepthStencilState9* dss)
{
	if(m_state.dss != dss)
	{
		m_state.dss = dss;

		m_dev->SetRenderState(D3DRS_ZENABLE, dss->DepthEnable);
		m_dev->SetRenderState(D3DRS_ZWRITEENABLE, dss->DepthWriteMask);
		
		if(dss->DepthEnable)
		{
			m_dev->SetRenderState(D3DRS_ZFUNC, dss->DepthFunc);
		}

		m_dev->SetRenderState(D3DRS_STENCILENABLE, dss->StencilEnable);

		if(dss->StencilEnable)
		{
			m_dev->SetRenderState(D3DRS_STENCILMASK, dss->StencilReadMask);
			m_dev->SetRenderState(D3DRS_STENCILWRITEMASK, dss->StencilWriteMask);	
			m_dev->SetRenderState(D3DRS_STENCILFUNC, dss->StencilFunc);
			m_dev->SetRenderState(D3DRS_STENCILPASS, dss->StencilPassOp);
			m_dev->SetRenderState(D3DRS_STENCILFAIL, dss->StencilFailOp);
			m_dev->SetRenderState(D3DRS_STENCILZFAIL, dss->StencilDepthFailOp);
			m_dev->SetRenderState(D3DRS_STENCILREF, dss->StencilRef);
		}
	}
}

void GSDevice9::OMSetBlendState(Direct3DBlendState9* bs, uint32 bf)
{
	if(m_state.bs != bs || m_state.bf != bf)
	{
		m_state.bs = bs;
		m_state.bf = bf;

		m_dev->SetRenderState(D3DRS_ALPHABLENDENABLE, bs->BlendEnable);

		if(bs->BlendEnable)
		{
			m_dev->SetRenderState(D3DRS_BLENDOP, bs->BlendOp);
			m_dev->SetRenderState(D3DRS_SRCBLEND, bs->SrcBlend);
			m_dev->SetRenderState(D3DRS_DESTBLEND, bs->DestBlend);
			m_dev->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, TRUE);
			m_dev->SetRenderState(D3DRS_BLENDOPALPHA, bs->BlendOpAlpha);
			m_dev->SetRenderState(D3DRS_SRCBLENDALPHA, bs->SrcBlendAlpha);
			m_dev->SetRenderState(D3DRS_DESTBLENDALPHA, bs->DestBlendAlpha);
			m_dev->SetRenderState(D3DRS_BLENDFACTOR, bf);
		}

		m_dev->SetRenderState(D3DRS_COLORWRITEENABLE, bs->RenderTargetWriteMask);
	}
}

void GSDevice9::OMSetRenderTargets(GSTexture* rt, GSTexture* ds, const GSVector4i* scissor)
{
	IDirect3DSurface9* rtv = NULL;
	IDirect3DSurface9* dsv = NULL;

	if(rt) rtv = *(GSTexture9*)rt;
	if(ds) dsv = *(GSTexture9*)ds;

	if(m_state.rtv != rtv)
	{
		m_state.rtv = rtv;

		m_dev->SetRenderTarget(0, rtv);
	}

	if(m_state.dsv != dsv)
	{
		m_state.dsv = dsv;

		m_dev->SetDepthStencilSurface(dsv);
	}

	GSVector4i r = scissor ? *scissor : GSVector4i(rt->GetSize()).zwxy();

	if(!m_state.scissor.eq(r))
	{
		m_state.scissor = r;

		m_dev->SetScissorRect(r);
	}
}

HRESULT GSDevice9::CompileShader(uint32 id, const string& entry, const D3DXMACRO* macro, IDirect3DVertexShader9** vs, const D3DVERTEXELEMENT9* layout, int count, IDirect3DVertexDeclaration9** il)
{
	vector<D3DXMACRO> m;

	PrepareShaderMacro(m, macro);

	HRESULT hr;

	CComPtr<ID3DXBuffer> shader, error;

	hr = D3DXCompileShaderFromResource(theApp.GetModuleHandle(), MAKEINTRESOURCE(id), &m[0], NULL, entry.c_str(), m_shader.vs.c_str(), 0, &shader, &error, NULL);

	if(SUCCEEDED(hr))
	{
		hr = m_dev->CreateVertexShader((DWORD*)shader->GetBufferPointer(), vs);
	}
	else if(error)
	{
		printf("%s\n", (const char*)error->GetBufferPointer());
	}

	ASSERT(SUCCEEDED(hr));

	if(FAILED(hr))
	{
		return hr;
	}

	hr = m_dev->CreateVertexDeclaration(layout, il);

	if(FAILED(hr))
	{
		return hr;
	}

	return S_OK;
}

HRESULT GSDevice9::CompileShader(uint32 id, const string& entry, const D3DXMACRO* macro, IDirect3DPixelShader9** ps)
{
	uint32 flags = 0;

	if(m_shader.level >= D3D_FEATURE_LEVEL_9_3)
	{
		flags |= D3DXSHADER_AVOID_FLOW_CONTROL;
	}
	else
	{
		flags |= D3DXSHADER_SKIPVALIDATION;
	}

	vector<D3DXMACRO> m;

	PrepareShaderMacro(m, macro);

	HRESULT hr;

	CComPtr<ID3DXBuffer> shader, error;

	hr = D3DXCompileShaderFromResource(theApp.GetModuleHandle(), MAKEINTRESOURCE(id), &m[0], NULL, entry.c_str(), m_shader.ps.c_str(), flags, &shader, &error, NULL);

	if(SUCCEEDED(hr))
	{
		hr = m_dev->CreatePixelShader((DWORD*)shader->GetBufferPointer(), ps);
	}
	else if(error)
	{
		printf("%s\n", (const char*)error->GetBufferPointer());
	}

	ASSERT(SUCCEEDED(hr));

	if(FAILED(hr))
	{
		return hr;
	}

	return S_OK;
}
