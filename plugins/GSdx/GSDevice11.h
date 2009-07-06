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
#include "GSTexture11.h"

class GSDevice11 : public GSDevice
{
	ID3D11Buffer* m_vb;
	size_t m_vb_stride;
	ID3D11InputLayout* m_layout;
	D3D11_PRIMITIVE_TOPOLOGY m_topology;
	ID3D11VertexShader* m_vs;
	ID3D11Buffer* m_vs_cb;
	ID3D11GeometryShader* m_gs;
	ID3D11ShaderResourceView* m_ps_srv[2];
	ID3D11PixelShader* m_ps;
	ID3D11Buffer* m_ps_cb;
	ID3D11SamplerState* m_ps_ss[2];
	GSVector2i m_viewport;
	GSVector4i m_scissor;
	ID3D11DepthStencilState* m_dss;
	uint8 m_sref;
	ID3D11BlendState* m_bs;
	float m_bf;
	ID3D11RenderTargetView* m_rtv;
	ID3D11DepthStencilView* m_dsv;

	//

	GSTexture* Create(int type, int w, int h, int format);

	void DoMerge(GSTexture* st[2], GSVector4* sr, GSVector4* dr, GSTexture* dt, bool slbg, bool mmod, const GSVector4& c);
	void DoInterlace(GSTexture* st, GSTexture* dt, int shader, bool linear, float yoffset = 0);

	//

	D3D_FEATURE_LEVEL m_level;
	CComPtr<ID3D11Device> m_dev;
	CComPtr<ID3D11DeviceContext> m_ctx;
	CComPtr<IDXGISwapChain> m_swapchain;
	struct {string model, vs, gs, ps;} m_shader;

	struct
	{
		CComPtr<ID3D11Buffer> vb, vb_old;
		size_t stride, start, count, limit;
	} m_vertices;

public: // TODO
	CComPtr<ID3D11RasterizerState> m_rs;

	struct
	{
		CComPtr<ID3D11InputLayout> il;
		CComPtr<ID3D11VertexShader> vs;
		CComPtr<ID3D11PixelShader> ps[7];
		CComPtr<ID3D11SamplerState> ln;
		CComPtr<ID3D11SamplerState> pt;
		CComPtr<ID3D11DepthStencilState> dss;
		CComPtr<ID3D11BlendState> bs;
	} m_convert;

	struct
	{
		CComPtr<ID3D11PixelShader> ps[2];
		CComPtr<ID3D11Buffer> cb;
		CComPtr<ID3D11BlendState> bs;
	} m_merge;

	struct
	{
		CComPtr<ID3D11PixelShader> ps[4];
		CComPtr<ID3D11Buffer> cb;
	} m_interlace;

public:
	GSDevice11();
	virtual ~GSDevice11();

	bool Create(GSWnd* wnd, bool vsync);
	bool Reset(int w, int h, int mode);
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

	void CopyRect(GSTexture* st, GSTexture* dt, const GSVector4i& r);

	void StretchRect(GSTexture* st, const GSVector4& sr, GSTexture* dt, const GSVector4& dr, int shader = 0, bool linear = true);
	void StretchRect(GSTexture* st, const GSVector4& sr, GSTexture* dt, const GSVector4& dr, ID3D11PixelShader* ps, ID3D11Buffer* ps_cb, bool linear = true);
	void StretchRect(GSTexture* st, const GSVector4& sr, GSTexture* dt, const GSVector4& dr, ID3D11PixelShader* ps, ID3D11Buffer* ps_cb, ID3D11BlendState* bs, bool linear = true);

	void IASetVertexBuffer(const void* vertices, size_t stride, size_t count);
	void IASetVertexBuffer(ID3D11Buffer* vb, size_t stride);
	void IASetInputLayout(ID3D11InputLayout* layout);
	void IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY topology);
	void VSSetShader(ID3D11VertexShader* vs, ID3D11Buffer* vs_cb);
	void GSSetShader(ID3D11GeometryShader* gs);
	void PSSetShaderResources(GSTexture* sr0, GSTexture* sr1);
	void PSSetShader(ID3D11PixelShader* ps, ID3D11Buffer* ps_cb);
	void PSSetSamplerState(ID3D11SamplerState* ss0, ID3D11SamplerState* ss1);
	void RSSet(int width, int height, const GSVector4i* scissor = NULL);
	void OMSetDepthStencilState(ID3D11DepthStencilState* dss, uint8 sref);
	void OMSetBlendState(ID3D11BlendState* bs, float bf);
	void OMSetRenderTargets(GSTexture* rt, GSTexture* ds);

	ID3D11Device* operator->() {return m_dev;}
	operator ID3D11Device*() {return m_dev;}
	operator ID3D11DeviceContext*() {return m_ctx;}

	HRESULT CompileShader(uint32 id, const string& entry, D3D11_SHADER_MACRO* macro, ID3D11VertexShader** vs, D3D11_INPUT_ELEMENT_DESC* layout, int count, ID3D11InputLayout** il);
	HRESULT CompileShader(uint32 id, const string& entry, D3D11_SHADER_MACRO* macro, ID3D11GeometryShader** gs);
	HRESULT CompileShader(uint32 id, const string& entry, D3D11_SHADER_MACRO* macro, ID3D11PixelShader** ps);
};
