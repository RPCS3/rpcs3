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

#pragma once

#include "GSDevice.h"
#include "GSTexture9.h"

struct Direct3DSamplerState9
{
    D3DTEXTUREFILTERTYPE FilterMin[2];
    D3DTEXTUREFILTERTYPE FilterMag[2];
    D3DTEXTUREADDRESS AddressU;
    D3DTEXTUREADDRESS AddressV;
};

struct Direct3DDepthStencilState9
{
    BOOL DepthEnable;
    BOOL DepthWriteMask;
    D3DCMPFUNC DepthFunc;
    BOOL StencilEnable;
    UINT8 StencilReadMask;
    UINT8 StencilWriteMask;
    D3DSTENCILOP StencilFailOp;
    D3DSTENCILOP StencilDepthFailOp;
    D3DSTENCILOP StencilPassOp;
    D3DCMPFUNC StencilFunc;
	uint32 StencilRef;
};

struct Direct3DBlendState9
{
    BOOL BlendEnable;
    D3DBLEND SrcBlend;
    D3DBLEND DestBlend;
    D3DBLENDOP BlendOp;
    D3DBLEND SrcBlendAlpha;
    D3DBLEND DestBlendAlpha;
    D3DBLENDOP BlendOpAlpha;
    UINT8 RenderTargetWriteMask;
};

class GSDevice9 : public GSDevice
{
private:
	IDirect3DVertexBuffer9* m_vb;
	size_t m_vb_stride;
	IDirect3DVertexDeclaration9* m_layout;
	D3DPRIMITIVETYPE m_topology;
	IDirect3DVertexShader9* m_vs;
	float* m_vs_cb;
	int m_vs_cb_len;
	IDirect3DTexture9* m_ps_srvs[2];
	IDirect3DPixelShader9* m_ps;
	float* m_ps_cb;
	int m_ps_cb_len;
	Direct3DSamplerState9* m_ps_ss;
	GSVector4i m_scissor;
	Direct3DDepthStencilState9* m_dss;
	Direct3DBlendState9* m_bs;
	uint32 m_bf;
	IDirect3DSurface9* m_rtv;
	IDirect3DSurface9* m_dsv;

	//

	GSTexture* Create(int type, int w, int h, int format);

	void DoMerge(GSTexture* st[2], GSVector4* sr, GSVector4* dr, GSTexture* dt, bool slbg, bool mmod, const GSVector4& c);
	void DoInterlace(GSTexture* st, GSTexture* dt, int shader, bool linear, float yoffset = 0);

	//

	DDCAPS m_ddcaps;
	D3DCAPS9 m_d3dcaps;
	D3DPRESENT_PARAMETERS m_pp;
	CComPtr<IDirect3D9> m_d3d;
	CComPtr<IDirect3DDevice9> m_dev;
	CComPtr<IDirect3DSwapChain9> m_swapchain;
	bool m_lost;

	struct
	{
		CComPtr<IDirect3DVertexBuffer9> vb, vb_old;
		size_t stride, start, count, limit;
	} m_vertices;

public: // TODO

	struct
	{
		CComPtr<IDirect3DVertexDeclaration9> il;
		CComPtr<IDirect3DVertexShader9> vs;
		CComPtr<IDirect3DPixelShader9> ps[7];
		Direct3DSamplerState9 ln;
		Direct3DSamplerState9 pt;
		Direct3DDepthStencilState9 dss;
		Direct3DBlendState9 bs;
	} m_convert;

	struct
	{
		CComPtr<IDirect3DPixelShader9> ps[2];
		Direct3DBlendState9 bs;
	} m_merge;

	struct
	{
		CComPtr<IDirect3DPixelShader9> ps[4];
	} m_interlace;

public:
	GSDevice9();
	virtual ~GSDevice9();

	bool Create(GSWnd* wnd, bool vsync);
	bool Reset(int w, int h, int mode);
	bool IsLost(bool update);
	void Flip();

	void BeginScene();
	void DrawPrimitive();
	void EndScene();

	void ClearRenderTarget(GSTexture* t, const GSVector4& c);
	void ClearRenderTarget(GSTexture* t, uint32 c);
	void ClearDepth(GSTexture* t, float c);
	void ClearStencil(GSTexture* t, uint8 c);

	GSTexture* CreateRenderTarget(int w, int h, int format = 0);
	GSTexture* CreateDepthStencil(int w, int h, int format = 0);
	GSTexture* CreateTexture(int w, int h, int format = 0);
	GSTexture* CreateOffscreen(int w, int h, int format = 0);

	GSTexture* CopyOffscreen(GSTexture* src, const GSVector4& sr, int w, int h, int format = 0);

	virtual bool IsCurrentRGBA() {return false;}

	void StretchRect(GSTexture* st, const GSVector4& sr, GSTexture* dt, const GSVector4& dr, int shader = 0, bool linear = true);
	void StretchRect(GSTexture* st, const GSVector4& sr, GSTexture* dt, const GSVector4& dr, IDirect3DPixelShader9* ps, const float* ps_cb, int ps_cb_len, bool linear = true);
	void StretchRect(GSTexture* st, const GSVector4& sr, GSTexture* dt, const GSVector4& dr, IDirect3DPixelShader9* ps, const float* ps_cb, int ps_cb_len, Direct3DBlendState9* bs, bool linear = true);

	void IASetVertexBuffer(const void* vertices, size_t stride, size_t count);
	void IASetVertexBuffer(IDirect3DVertexBuffer9* vb, size_t stride);
	void IASetInputLayout(IDirect3DVertexDeclaration9* layout);
	void IASetPrimitiveTopology(D3DPRIMITIVETYPE topology);
	void VSSetShader(IDirect3DVertexShader9* vs, const float* vs_cb, int vs_cb_len);
	void PSSetShaderResources(GSTexture* sr0, GSTexture* sr1);
	void PSSetShader(IDirect3DPixelShader9* ps, const float* ps_cb, int ps_cb_len);
	void PSSetSamplerState(Direct3DSamplerState9* ss);
	void RSSet(int width, int height, const GSVector4i* scissor = NULL);
	void OMSetDepthStencilState(Direct3DDepthStencilState9* dss);
	void OMSetBlendState(Direct3DBlendState9* bs, uint32 bf);
	void OMSetRenderTargets(GSTexture* rt, GSTexture* ds);

	IDirect3DDevice9* operator->() {return m_dev;}
	operator IDirect3DDevice9*() {return m_dev;}

	HRESULT CompileShader(uint32 id, const string& entry, const D3DXMACRO* macro, IDirect3DVertexShader9** vs, const D3DVERTEXELEMENT9* layout, int count, IDirect3DVertexDeclaration9** il);
	HRESULT CompileShader(uint32 id, const string& entry, const D3DXMACRO* macro, IDirect3DPixelShader9** ps);
};
