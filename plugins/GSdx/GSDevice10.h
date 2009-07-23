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
#include "GSTexture10.h"

class GSDevice10 : public GSDevice
{
	ID3D10Buffer* m_vb;
	size_t m_vb_stride;
	ID3D10InputLayout* m_layout;
	D3D10_PRIMITIVE_TOPOLOGY m_topology;
	ID3D10VertexShader* m_vs;
	ID3D10Buffer* m_vs_cb;
	ID3D10GeometryShader* m_gs;
	ID3D10ShaderResourceView* m_ps_srv[2];
	ID3D10PixelShader* m_ps;
	ID3D10Buffer* m_ps_cb;
	ID3D10SamplerState* m_ps_ss[2];
	GSVector2i m_viewport;
	GSVector4i m_scissor;
	ID3D10DepthStencilState* m_dss;
	uint8 m_sref;
	ID3D10BlendState* m_bs;
	float m_bf;
	ID3D10RenderTargetView* m_rtv;
	ID3D10DepthStencilView* m_dsv;

	//

	GSTexture* Create(int type, int w, int h, int format);

	void DoMerge(GSTexture* st[2], GSVector4* sr, GSVector4* dr, GSTexture* dt, bool slbg, bool mmod, const GSVector4& c);
	void DoInterlace(GSTexture* st, GSTexture* dt, int shader, bool linear, float yoffset = 0);

	//

	CComPtr<ID3D10Device1> m_dev;
	CComPtr<IDXGISwapChain> m_swapchain;

	struct
	{
		CComPtr<ID3D10Buffer> vb, vb_old;
		size_t stride, start, count, limit;
	} m_vertices;

public: // TODO
	CComPtr<ID3D10RasterizerState> m_rs;

	struct
	{
		CComPtr<ID3D10InputLayout> il;
		CComPtr<ID3D10VertexShader> vs;
		CComPtr<ID3D10PixelShader> ps[7];
		CComPtr<ID3D10SamplerState> ln;
		CComPtr<ID3D10SamplerState> pt;
		CComPtr<ID3D10DepthStencilState> dss;
		CComPtr<ID3D10BlendState> bs;
	} m_convert;

	struct
	{
		CComPtr<ID3D10PixelShader> ps[2];
		CComPtr<ID3D10Buffer> cb;
		CComPtr<ID3D10BlendState> bs;
	} m_merge;

	struct
	{
		CComPtr<ID3D10PixelShader> ps[4];
		CComPtr<ID3D10Buffer> cb;
	} m_interlace;

public:
	GSDevice10();
	virtual ~GSDevice10();

	bool Create(GSWnd* wnd, bool vsync);
	bool Reset(int w, int h, int mode);
	void Flip(bool limit);

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

	void CopyRect(GSTexture* st, GSTexture* dt, const GSVector4i& r);

	void StretchRect(GSTexture* st, const GSVector4& sr, GSTexture* dt, const GSVector4& dr, int shader = 0, bool linear = true);
	void StretchRect(GSTexture* st, const GSVector4& sr, GSTexture* dt, const GSVector4& dr, ID3D10PixelShader* ps, ID3D10Buffer* ps_cb, bool linear = true);
	void StretchRect(GSTexture* st, const GSVector4& sr, GSTexture* dt, const GSVector4& dr, ID3D10PixelShader* ps, ID3D10Buffer* ps_cb, ID3D10BlendState* bs, bool linear = true);

	void IASetVertexBuffer(const void* vertices, size_t stride, size_t count);
	void IASetVertexBuffer(ID3D10Buffer* vb, size_t stride);
	void IASetInputLayout(ID3D10InputLayout* layout);
	void IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY topology);
	void VSSetShader(ID3D10VertexShader* vs, ID3D10Buffer* vs_cb);
	void GSSetShader(ID3D10GeometryShader* gs);
	void PSSetShaderResources(GSTexture* sr0, GSTexture* sr1);
	void PSSetShader(ID3D10PixelShader* ps, ID3D10Buffer* ps_cb);
	void PSSetSamplerState(ID3D10SamplerState* ss0, ID3D10SamplerState* ss1);
	void OMSetDepthStencilState(ID3D10DepthStencilState* dss, uint8 sref);
	void OMSetBlendState(ID3D10BlendState* bs, float bf);
	void OMSetRenderTargets(GSTexture* rt, GSTexture* ds, const GSVector4i* scissor = NULL);

	ID3D10Device* operator->() {return m_dev;}
	operator ID3D10Device*() {return m_dev;}

	HRESULT CompileShader(uint32 id, const string& entry, D3D10_SHADER_MACRO* macro, ID3D10VertexShader** vs, D3D10_INPUT_ELEMENT_DESC* layout, int count, ID3D10InputLayout** il);
	HRESULT CompileShader(uint32 id, const string& entry, D3D10_SHADER_MACRO* macro, ID3D10GeometryShader** gs);
	HRESULT CompileShader(uint32 id, const string& entry, D3D10_SHADER_MACRO* macro, ID3D10PixelShader** ps);
};
