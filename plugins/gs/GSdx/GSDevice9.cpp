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
#include "GSDevice9.h"
#include "resource.h"

GSDevice9::GSDevice9() 
	: m_vb(NULL)
	, m_vb_count(0)
	, m_vb_vertices(NULL)
	, m_vb_stride(0)
	, m_layout(NULL)
	, m_topology((D3DPRIMITIVETYPE)0)
	, m_vs(NULL)
	, m_vs_cb(NULL)
	, m_vs_cb_len(0)
	, m_ps(NULL)
	, m_ps_cb(NULL)
	, m_ps_cb_len(0)
	, m_ps_ss(NULL)
	, m_scissor(0, 0, 0, 0)
	, m_dss(NULL)
	, m_sref(0)
	, m_bs(NULL)
	, m_bf(0xffffffff)
	, m_rtv(NULL)
	, m_dsv(NULL)
{
	memset(&m_pp, 0, sizeof(m_pp));
	memset(&m_ddcaps, 0, sizeof(m_ddcaps));
	memset(&m_d3dcaps, 0, sizeof(m_d3dcaps));
	memset(m_ps_srvs, 0, sizeof(m_ps_srvs));
}

GSDevice9::~GSDevice9()
{
	if(m_vs_cb) _aligned_free(m_vs_cb);
	if(m_ps_cb) _aligned_free(m_ps_cb);
}

bool GSDevice9::Create(HWND hWnd, bool vsync)
{
	if(!__super::Create(hWnd, vsync))
	{
		return false;
	}

	HRESULT hr;

	// dd

	CComPtr<IDirectDraw7> dd; 

	hr = DirectDrawCreateEx(0, (void**)&dd, IID_IDirectDraw7, 0);

	if(FAILED(hr)) return false;

	memset(&m_ddcaps, 0, sizeof(m_ddcaps));

	m_ddcaps.dwSize = sizeof(DDCAPS); 

	hr = dd->GetCaps(&m_ddcaps, NULL);

	if(FAILED(hr)) return false;

	dd = NULL;

	// d3d

	m_d3d = Direct3DCreate9(D3D_SDK_VERSION);

	if(!m_d3d) return false;

	hr = m_d3d->CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, D3DFMT_D24S8);
		
	if(FAILED(hr)) return false;

	hr = m_d3d->CheckDepthStencilMatch(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8, D3DFMT_X8R8G8B8, D3DFMT_D24S8);

	if(FAILED(hr)) return false;

	memset(&m_d3dcaps, 0, sizeof(m_d3dcaps));

	m_d3d->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &m_d3dcaps);

	bool fs = AfxGetApp()->GetProfileInt(_T("Settings"), _T("ModeWidth"), 0) > 0;

	if(!Reset(1, 1, fs)) return false;

	m_dev->Clear(0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0);

	// shaders

	DWORD psver = AfxGetApp()->GetProfileInt(_T("Settings"), _T("PixelShaderVersion2"), D3DPS_VERSION(2, 0));

	if(psver > m_d3dcaps.PixelShaderVersion)
	{
		CString str;

		str.Format(_T("Supported pixel shader version is too low!\n\nSupported: %d.%d\nSelected: %d.%d"),
			D3DSHADER_VERSION_MAJOR(m_d3dcaps.PixelShaderVersion), D3DSHADER_VERSION_MINOR(m_d3dcaps.PixelShaderVersion),
			D3DSHADER_VERSION_MAJOR(psver), D3DSHADER_VERSION_MINOR(psver));

		AfxMessageBox(str);

		return false;
	}

	m_d3dcaps.PixelShaderVersion = min(psver, m_d3dcaps.PixelShaderVersion);
	m_d3dcaps.VertexShaderVersion = m_d3dcaps.PixelShaderVersion & ~0x10000;

	// convert

	static const D3DVERTEXELEMENT9 il_convert[] =
	{
		{0, 0,  D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
		{0, 16, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
		D3DDECL_END()
	};

	CompileShader(IDR_CONVERT9_FX, "vs_main", NULL, &m_convert.vs, il_convert, countof(il_convert), &m_convert.il);

	for(int i = 0; i < countof(m_convert.ps); i++)
	{
		CStringA main;
		main.Format("ps_main%d", i);
		CompileShader(IDR_CONVERT9_FX, main, NULL, &m_convert.ps[i]);
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
		CStringA main;
		main.Format("ps_main%d", i);
		CompileShader(IDR_MERGE9_FX, main, NULL, &m_merge.ps[i]);
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
		CStringA main;
		main.Format("ps_main%d", i);
		CompileShader(IDR_INTERLACE9_FX, main, NULL, &m_interlace.ps[i]);
	}

	//

	return true;
}

bool GSDevice9::Reset(int w, int h, bool fs)
{
	if(!__super::Reset(w, h, fs))
		return false;

	HRESULT hr;

	if(!m_d3d) return false;

	if(m_swapchain && !fs && m_pp.Windowed)
	{
		m_swapchain = NULL;

		m_pp.BackBufferWidth = w;
		m_pp.BackBufferHeight = h;

		hr = m_dev->CreateAdditionalSwapChain(&m_pp, &m_swapchain);

		if(FAILED(hr)) return false;

		CComPtr<IDirect3DSurface9> backbuffer;
		hr = m_swapchain->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &backbuffer);
		m_backbuffer = Texture(backbuffer);

		return true;
	}

	m_swapchain = NULL;
	m_backbuffer = Texture();
	if(m_font) m_font->OnLostDevice();
	m_font = NULL;

	if(m_vs_cb) _aligned_free(m_vs_cb);
	if(m_ps_cb) _aligned_free(m_ps_cb);

	m_vb = NULL;
	m_vb_stride = 0;
	m_layout = NULL;
	m_vs = NULL;
	m_vs_cb = NULL;
	m_vs_cb_len = 0;
	m_ps = NULL;
	m_ps_cb = NULL;
	m_ps_cb_len = 0;
	m_ps_ss = NULL;
	m_scissor = CRect(0, 0, 0, 0);
	m_dss = NULL;
	m_sref = 0;
	m_bs = NULL;
	m_bf = 0xffffffff;
	m_rtv = NULL;
	m_dsv = NULL;

	memset(&m_pp, 0, sizeof(m_pp));

	m_pp.Windowed = TRUE;
	m_pp.hDeviceWindow = m_hWnd;
	m_pp.SwapEffect = D3DSWAPEFFECT_FLIP;
	m_pp.BackBufferFormat = D3DFMT_X8R8G8B8;
	m_pp.BackBufferWidth = 1;
	m_pp.BackBufferHeight = 1;
	m_pp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

	if(m_vsync)
	{
		m_pp.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;
	}

	if(!!AfxGetApp()->GetProfileInt(_T("Settings"), _T("tvout"), FALSE))
	{
		m_pp.Flags |= D3DPRESENTFLAG_VIDEO;
	}

	int mw = AfxGetApp()->GetProfileInt(_T("Settings"), _T("ModeWidth"), 0);
	int mh = AfxGetApp()->GetProfileInt(_T("Settings"), _T("ModeHeight"), 0);
	int mrr = AfxGetApp()->GetProfileInt(_T("Settings"), _T("ModeRefreshRate"), 0);

	if(fs && mw > 0 && mh > 0 && mrr >= 0)
	{
		m_pp.Windowed = FALSE;
		m_pp.BackBufferWidth = mw;
		m_pp.BackBufferHeight = mh;
		// m_pp.FullScreen_RefreshRateInHz = mrr;

		::SetWindowLong(m_hWnd, GWL_STYLE, ::GetWindowLong(m_hWnd, GWL_STYLE) & ~(WS_CAPTION|WS_THICKFRAME));
		::SetWindowPos(m_hWnd, NULL, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
		::SetMenu(m_hWnd, NULL);
	}

	if(!m_dev)
	{
		UINT flags = D3DCREATE_MULTITHREADED | (m_d3dcaps.VertexProcessingCaps ? D3DCREATE_HARDWARE_VERTEXPROCESSING : D3DCREATE_SOFTWARE_VERTEXPROCESSING);

		hr = m_d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, m_hWnd, flags, &m_pp, &m_dev);

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

	m_backbuffer = Texture(backbuffer);

	D3DXFONT_DESC fd;
	memset(&fd, 0, sizeof(fd));
	_tcscpy(fd.FaceName, _T("Arial"));
	fd.Height = 20;
	D3DXCreateFontIndirect(m_dev, &fd, &m_font);

	m_dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	m_dev->SetRenderState(D3DRS_LIGHTING, FALSE);
	m_dev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
	m_dev->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);

	return true;
}

bool GSDevice9::IsLost()
{
	HRESULT hr = m_dev->TestCooperativeLevel();
	
	return hr == D3DERR_DEVICELOST || hr == D3DERR_DEVICENOTRESET;
}

void GSDevice9::Present(const CRect& r)
{
	CRect cr;

	GetClientRect(m_hWnd, &cr);

	if(m_backbuffer.GetWidth() != cr.Width() || m_backbuffer.GetHeight() != cr.Height())
	{
		Reset(cr.Width(), cr.Height(), false);
	}

	OMSetRenderTargets(m_backbuffer, NULL);

	m_dev->Clear(0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0);

	if(m_current)
	{
		StretchRect(m_current, m_backbuffer, GSVector4(r));
	}

	if(m_swapchain)
	{
		m_swapchain->Present(NULL, NULL, NULL, NULL, 0);
	}
	else
	{
		m_dev->Present(NULL, NULL, NULL, NULL);
	}
}

void GSDevice9::BeginScene()
{
	m_dev->BeginScene();
}

void GSDevice9::EndScene()
{
	m_dev->EndScene();
}

void GSDevice9::Draw(LPCTSTR str)
{
	/*
	if(!m_pp.Windowed)
	{
		BeginScene();

		OMSetRenderTargets(m_backbuffer, NULL);

		CRect r(0, 0, m_backbuffer.GetWidth(), m_backbuffer.GetHeight());

		D3DCOLOR c = D3DCOLOR_ARGB(255, 0, 255, 0);

		if(m_font->DrawText(NULL, str, -1, &r, DT_CALCRECT|DT_LEFT|DT_WORDBREAK, c))
		{
			m_font->DrawText(NULL, str, -1, &r, DT_LEFT|DT_WORDBREAK, c);
		}

		EndScene();
	}
	*/
}

bool GSDevice9::CopyOffscreen(Texture& src, const GSVector4& sr, Texture& dst, int w, int h, int format)
{
	dst = Texture();

	if(format == 0)
	{
		format = D3DFMT_A8R8G8B8;
	}

	if(format != D3DFMT_A8R8G8B8)
	{
		ASSERT(0);

		return false;
	}

	Texture rt;

	if(CreateRenderTarget(rt, w, h, format))
	{
		GSVector4 dr(0, 0, w, h);

		StretchRect(src, sr, rt, dr, m_convert.ps[1], NULL, 0);

		if(CreateOffscreen(dst, w, h, format))
		{
			m_dev->GetRenderTargetData(rt, dst);
		}
	}

	Recycle(rt);

	return !!dst;
}

void GSDevice9::ClearRenderTarget(Texture& t, const GSVector4& c)
{
	ClearRenderTarget(t, D3DCOLOR_RGBA((BYTE)(c.r * 255 + 0.5f), (BYTE)(c.g * 255 + 0.5f), (BYTE)(c.b * 255 + 0.5f), (BYTE)(c.a * 255 + 0.5f)));
}

void GSDevice9::ClearRenderTarget(Texture& t, DWORD c)
{
	CComPtr<IDirect3DSurface9> surface;
	m_dev->GetRenderTarget(0, &surface);
	m_dev->SetRenderTarget(0, t);
	m_dev->Clear(0, NULL, D3DCLEAR_TARGET, c, 0, 0);
	m_dev->SetRenderTarget(0, surface);
}

void GSDevice9::ClearDepth(Texture& t, float c)
{
	CComPtr<IDirect3DSurface9> surface;
	m_dev->GetDepthStencilSurface(&surface);
	m_dev->SetDepthStencilSurface(t);
	m_dev->Clear(0, NULL, D3DCLEAR_ZBUFFER, 0, c, 0);
	m_dev->SetDepthStencilSurface(surface);
}

void GSDevice9::ClearStencil(Texture& t, BYTE c)
{
	CComPtr<IDirect3DSurface9> surface;
	m_dev->GetDepthStencilSurface(&surface);
	m_dev->SetDepthStencilSurface(t);
	m_dev->Clear(0, NULL, D3DCLEAR_STENCIL, 0, 0, c);
	m_dev->SetDepthStencilSurface(surface);
}

bool GSDevice9::Create(int type, Texture& t, int w, int h, int format)
{
	HRESULT hr;

	CComPtr<IDirect3DTexture9> texture;
	CComPtr<IDirect3DSurface9> surface;

	switch(type)
	{
	case GSTexture::RenderTarget:
		hr = m_dev->CreateTexture(w, h, 1, D3DUSAGE_RENDERTARGET, (D3DFORMAT)format, D3DPOOL_DEFAULT, &texture, NULL);
		break;
	case GSTexture::DepthStencil:
		hr = m_dev->CreateDepthStencilSurface(w, h, (D3DFORMAT)format, D3DMULTISAMPLE_NONE, 0, FALSE, &surface, NULL);
		break;
	case GSTexture::Texture:
		hr = m_dev->CreateTexture(w, h, 1, 0, (D3DFORMAT)format, D3DPOOL_MANAGED, &texture, NULL);
		break;
	case GSTexture::Offscreen:
		hr = m_dev->CreateOffscreenPlainSurface(w, h, (D3DFORMAT)format, D3DPOOL_SYSTEMMEM, &surface, NULL);
		break;
	}

	if(surface)
	{
		t = Texture(surface);
	}

	if(texture)
	{
		t = Texture(texture);
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

		return t;
	}

	return false;
}

bool GSDevice9::CreateRenderTarget(Texture& t, int w, int h, int format)
{
	return __super::CreateRenderTarget(t, w, h, format ? format : D3DFMT_A8R8G8B8);
}

bool GSDevice9::CreateDepthStencil(Texture& t, int w, int h, int format)
{
	return __super::CreateDepthStencil(t, w, h, format ? format : D3DFMT_D24S8);
}

bool GSDevice9::CreateTexture(Texture& t, int w, int h, int format)
{
	return __super::CreateTexture(t, w, h, format ? format : D3DFMT_A8R8G8B8);
}

bool GSDevice9::CreateOffscreen(Texture& t, int w, int h, int format)
{
	return __super::CreateOffscreen(t, w, h, format ? format : D3DFMT_A8R8G8B8);
}

void GSDevice9::DoMerge(Texture* st, GSVector4* sr, GSVector4* dr, Texture& dt, bool slbg, bool mmod, GSVector4& c)
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

void GSDevice9::DoInterlace(Texture& st, Texture& dt, int shader, bool linear, float yoffset)
{
	GSVector4 sr(0, 0, 1, 1);
	GSVector4 dr(0.0f, yoffset, (float)dt.GetWidth(), (float)dt.GetHeight() + yoffset);

	InterlaceConstantBuffer cb;

	cb.ZrH = GSVector2(0, 1.0f / dt.GetHeight());
	cb.hH = (float)dt.GetHeight() / 2;

	StretchRect(st, sr, dt, dr, m_interlace.ps[shader], (const float*)&cb, 1, linear);
}
/*
void GSDevice9::IASetVertexBuffer(IDirect3DVertexBuffer9* vb, UINT count, const void* vertices, UINT stride)
{
	void* data = NULL;

	if(SUCCEEDED(vb->Lock(0, count * stride, &data, D3DLOCK_DISCARD)))
	{
		memcpy(data, vertices, count * stride);

		vb->Unlock();
	}

	if(m_vb != vb || m_vb_stride != stride)
	{
		m_dev->SetStreamSource(0, vb, 0, stride);

		m_vb = vb;
		m_vb_stride = stride;
	}
}
*/
void GSDevice9::IASetVertexBuffer(UINT count, const void* vertices, UINT stride)
{
	m_vb_count = count;
	m_vb_vertices = vertices;
	m_vb_stride = stride;
}

void GSDevice9::IASetInputLayout(IDirect3DVertexDeclaration9* layout)
{
	// TODO: get rid of all SetFVF before enabling this

	// if(m_layout != layout)
	{
		m_dev->SetVertexDeclaration(layout);

		// m_layout = layout;
	}
}

void GSDevice9::IASetPrimitiveTopology(D3DPRIMITIVETYPE topology)
{
	m_topology = topology;
}

void GSDevice9::VSSetShader(IDirect3DVertexShader9* vs, const float* vs_cb, int vs_cb_len)
{
	if(m_vs != vs)
	{
		m_dev->SetVertexShader(vs);

		m_vs = vs;
	}

	if(vs_cb && vs_cb_len > 0)
	{
		int size = vs_cb_len * sizeof(float) * 4;
		
		if(m_vs_cb_len != vs_cb_len || m_vs_cb == NULL || memcmp(m_vs_cb, vs_cb, size))
		{
			if(m_vs_cb == NULL || m_vs_cb_len < vs_cb_len)
			{
				if(m_vs_cb) _aligned_free(m_vs_cb);

				m_vs_cb = (float*)_aligned_malloc(size, 16);
			}

			memcpy(m_vs_cb, vs_cb, size);

			m_dev->SetVertexShaderConstantF(0, vs_cb, vs_cb_len);

			m_vs_cb_len = vs_cb_len;
		}
	}
}

void GSDevice9::PSSetShaderResources(IDirect3DTexture9* srv0, IDirect3DTexture9* srv1)
{
	if(m_ps_srvs[0] != srv0)
	{
		m_dev->SetTexture(0, srv0);

		m_ps_srvs[0] = srv0;
	}

	if(m_ps_srvs[1] != srv1)
	{
		m_dev->SetTexture(1, srv1);

		m_ps_srvs[1] = srv1;
	}
}

void GSDevice9::PSSetShader(IDirect3DPixelShader9* ps, const float* ps_cb, int ps_cb_len)
{
	if(m_ps != ps)
	{
		m_dev->SetPixelShader(ps);

		m_ps = ps;
	}
	
	if(ps_cb && ps_cb_len > 0)
	{
		int size = ps_cb_len * sizeof(float) * 4;
		
		if(m_ps_cb_len != ps_cb_len || m_ps_cb == NULL || memcmp(m_ps_cb, ps_cb, size))
		{
			if(m_ps_cb == NULL || m_ps_cb_len < ps_cb_len)
			{
				if(m_ps_cb) _aligned_free(m_ps_cb);

				m_ps_cb = (float*)_aligned_malloc(size, 16);
			}

			memcpy(m_ps_cb, ps_cb, size);

			m_dev->SetPixelShaderConstantF(0, ps_cb, ps_cb_len);

			m_ps_cb_len = ps_cb_len;
		}
	}
}

void GSDevice9::PSSetSamplerState(Direct3DSamplerState9* ss)
{
	if(ss && m_ps_ss != ss)
	{
		m_dev->SetSamplerState(0, D3DSAMP_ADDRESSU, ss->AddressU);
		m_dev->SetSamplerState(0, D3DSAMP_ADDRESSV, ss->AddressV);
		m_dev->SetSamplerState(1, D3DSAMP_ADDRESSU, ss->AddressU);
		m_dev->SetSamplerState(1, D3DSAMP_ADDRESSV, ss->AddressV);
		m_dev->SetSamplerState(2, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
		m_dev->SetSamplerState(2, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
		m_dev->SetSamplerState(3, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
		m_dev->SetSamplerState(3, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
		m_dev->SetSamplerState(0, D3DSAMP_MINFILTER, ss->FilterMin[0]);
		m_dev->SetSamplerState(0, D3DSAMP_MAGFILTER, ss->FilterMag[0]);
		m_dev->SetSamplerState(1, D3DSAMP_MINFILTER, ss->FilterMin[1]);
		m_dev->SetSamplerState(1, D3DSAMP_MAGFILTER, ss->FilterMag[1]);
		m_dev->SetSamplerState(2, D3DSAMP_MINFILTER, D3DTEXF_POINT);
		m_dev->SetSamplerState(2, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
		m_dev->SetSamplerState(3, D3DSAMP_MINFILTER, D3DTEXF_POINT);
		m_dev->SetSamplerState(3, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
		
		m_ps_ss = ss;
	}
}

void GSDevice9::RSSet(int width, int height, const RECT* scissor)
{
	CRect r = scissor ? *scissor : CRect(0, 0, width, height);

	if(m_scissor != r)
	{
		m_dev->SetScissorRect(&r);

		m_scissor = r;
	}
}

void GSDevice9::OMSetDepthStencilState(Direct3DDepthStencilState9* dss, UINT sref)
{
	if(m_dss != dss || m_sref != sref)
	{
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
			m_dev->SetRenderState(D3DRS_STENCILREF, sref);
		}

		m_dss = dss;
		m_sref = sref;
	}
}

void GSDevice9::OMSetBlendState(Direct3DBlendState9* bs, DWORD bf)
{
	if(m_bs != bs || m_bf != bf)
	{
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

		m_bs = bs;
		m_bf = bf;
	}
}

void GSDevice9::OMSetRenderTargets(IDirect3DSurface9* rtv, IDirect3DSurface9* dsv)
{
	if(m_rtv != rtv)
	{
		m_dev->SetRenderTarget(0, rtv);

		m_rtv = rtv;
	}

	if(m_dsv != dsv)
	{
		m_dev->SetDepthStencilSurface(dsv);

		m_dsv = dsv;
	}
}

void GSDevice9::DrawPrimitive()
{
	int prims = 0;

	switch(m_topology)
	{
    case D3DPT_TRIANGLELIST:
		prims = m_vb_count / 3;
		break;
    case D3DPT_LINELIST:
		prims = m_vb_count / 2;
		break;
    case D3DPT_POINTLIST:
		prims = m_vb_count;
		break;
    case D3DPT_TRIANGLESTRIP:
    case D3DPT_TRIANGLEFAN:
		prims = m_vb_count - 2;
		break;
    case D3DPT_LINESTRIP:
		prims = m_vb_count - 1;
		break;
	}

	m_dev->DrawPrimitiveUP(m_topology, prims, m_vb_vertices, m_vb_stride);
}

void GSDevice9::StretchRect(Texture& st, Texture& dt, const GSVector4& dr, bool linear)
{
	StretchRect(st, GSVector4(0, 0, 1, 1), dt, dr, linear);
}

void GSDevice9::StretchRect(Texture& st, const GSVector4& sr, Texture& dt, const GSVector4& dr, bool linear)
{
	StretchRect(st, sr, dt, dr, m_convert.ps[0], NULL, 0, linear);
}

void GSDevice9::StretchRect(Texture& st, const GSVector4& sr, Texture& dt, const GSVector4& dr, IDirect3DPixelShader9* ps, const float* ps_cb, int ps_cb_len, bool linear)
{
	StretchRect(st, sr, dt, dr, ps, ps_cb, ps_cb_len, &m_convert.bs, linear);
}

void GSDevice9::StretchRect(Texture& st, const GSVector4& sr, Texture& dt, const GSVector4& dr, IDirect3DPixelShader9* ps, const float* ps_cb, int ps_cb_len, Direct3DBlendState9* bs, bool linear)
{
	BeginScene();

	// om

	OMSetDepthStencilState(&m_convert.dss, 0);
	OMSetBlendState(bs, 0);
	OMSetRenderTargets(dt, NULL);

	// ia

	float left = dr.x * 2 / dt.GetWidth() - 1.0f;
	float top = 1.0f - dr.y * 2 / dt.GetHeight();
	float right = dr.z * 2 / dt.GetWidth() - 1.0f;
	float bottom = 1.0f - dr.w * 2 / dt.GetHeight();

	GSVertexPT1 vertices[] =
	{
		{GSVector4(left, top, 0.5f, 1.0f), GSVector2(sr.x, sr.y)},
		{GSVector4(right, top, 0.5f, 1.0f), GSVector2(sr.z, sr.y)},
		{GSVector4(left, bottom, 0.5f, 1.0f), GSVector2(sr.x, sr.w)},
		{GSVector4(right, bottom, 0.5f, 1.0f), GSVector2(sr.z, sr.w)},
	};

	for(int i = 0; i < countof(vertices); i++)
	{
		vertices[i].p.x -= 1.0f / dt.GetWidth();
		vertices[i].p.y += 1.0f / dt.GetHeight();
	}

	IASetVertexBuffer(4, vertices);
	IASetInputLayout(m_convert.il);
	IASetPrimitiveTopology(D3DPT_TRIANGLESTRIP);

	// vs

	VSSetShader(m_convert.vs, NULL, 0);

	// ps

	PSSetShader(ps, ps_cb, ps_cb_len);
	PSSetSamplerState(linear ? &m_convert.ln : &m_convert.pt);
	PSSetShaderResources(st, NULL);

	// rs

	RSSet(dt.GetWidth(), dt.GetHeight());

	//

	DrawPrimitive();

	//

	EndScene();
}

// FIXME: D3DXCompileShaderFromResource of d3dx9 v37 (march 2008) calls GetFullPathName on id for some reason and then crashes

static HRESULT LoadShader(UINT id, LPCSTR& data, DWORD& size)
{
	CComPtr<ID3DXBuffer> shader, error;

	HRSRC hRes = FindResource(AfxGetResourceHandle(), MAKEINTRESOURCE(id), RT_RCDATA);

	if(!hRes) return E_FAIL;

	size = SizeofResource(AfxGetResourceHandle(), hRes);

	if(size == 0) return E_FAIL;

	HGLOBAL hResData  = LoadResource(AfxGetResourceHandle(), hRes);

	if(!hResData) return E_FAIL;

	data = (LPCSTR)LockResource(hResData);

	return S_OK;
}

HRESULT GSDevice9::CompileShader(UINT id, LPCSTR entry, const D3DXMACRO* macro, IDirect3DVertexShader9** vs, const D3DVERTEXELEMENT9* layout, int count, IDirect3DVertexDeclaration9** il)
{
	LPCSTR target;

	if(m_d3dcaps.VertexShaderVersion >= D3DVS_VERSION(3, 0))
	{
		target = "vs_3_0";
	}
	else if(m_d3dcaps.VertexShaderVersion >= D3DVS_VERSION(2, 0))
	{
		target = "vs_2_0";
	}
	else
	{
		return E_FAIL;
	}

	HRESULT hr;

	CComPtr<ID3DXBuffer> shader, error;

	// FIXME: hr = D3DXCompileShaderFromResource(AfxGetResourceHandle(), MAKEINTRESOURCE(id), macro, NULL, entry, target, 0, &shader, &error, NULL);

	LPCSTR data;
	DWORD size;

	hr = LoadShader(id, data, size);

	if(FAILED(hr)) return E_FAIL;

	hr = D3DXCompileShader(data, size, macro, NULL, entry, target, 0, &shader, &error, NULL);

	if(SUCCEEDED(hr))
	{
		hr = m_dev->CreateVertexShader((DWORD*)shader->GetBufferPointer(), vs);
	}
	else if(error)
	{
		LPCSTR msg = (LPCSTR)error->GetBufferPointer();

		TRACE(_T("%s\n"), CString(msg));
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

HRESULT GSDevice9::CompileShader(UINT id, LPCSTR entry, const D3DXMACRO* macro, IDirect3DPixelShader9** ps)
{
	LPCSTR target = NULL;
	UINT flags = 0;

	if(m_d3dcaps.PixelShaderVersion >= D3DPS_VERSION(3, 0))
	{
		target = "ps_3_0";
		flags |= D3DXSHADER_AVOID_FLOW_CONTROL;
	}
	else if(m_d3dcaps.PixelShaderVersion >= D3DPS_VERSION(2, 0))
	{
		target = "ps_2_0";
	}
	else 
	{
		return false;
	}

	HRESULT hr;

	CComPtr<ID3DXBuffer> shader, error;

	// FIXME: hr = D3DXCompileShaderFromResource(AfxGetResourceHandle(), MAKEINTRESOURCE(id), macro, NULL, entry, target, flags, &shader, &error, NULL);

	LPCSTR data;
	DWORD size;

	hr = LoadShader(id, data, size);

	if(FAILED(hr)) return E_FAIL;

	hr = D3DXCompileShader(data, size, macro, NULL, entry, target, 0, &shader, &error, NULL);

	if(SUCCEEDED(hr))
	{
		hr = m_dev->CreatePixelShader((DWORD*)shader->GetBufferPointer(), ps);

		ASSERT(SUCCEEDED(hr));
	}
	else if(error)
	{
		LPCSTR msg = (LPCSTR)error->GetBufferPointer();

		TRACE(_T("%s\n"), CString(msg));
	}

	ASSERT(SUCCEEDED(hr));

	if(FAILED(hr))
	{
		return hr;
	}

	return S_OK;
}
