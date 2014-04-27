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
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once

#include "GSDeviceDX.h"
#include "GSTexture11.h"

struct GSVertexShader11
{
	CComPtr<ID3D11VertexShader> vs;
	CComPtr<ID3D11InputLayout> il;
};

class GSDevice11 : public GSDeviceDX
{
	GSTexture* CreateSurface(int type, int w, int h, bool msaa, int format);

	void DoMerge(GSTexture* st[2], GSVector4* sr, GSTexture* dt, GSVector4* dr, bool slbg, bool mmod, const GSVector4& c);
	void DoInterlace(GSTexture* st, GSTexture* dt, int shader, bool linear, float yoffset = 0);
	void DoFXAA(GSTexture* st, GSTexture* dt);
	void DoShadeBoost(GSTexture* st, GSTexture* dt);
	void DoExternalFX(GSTexture* st, GSTexture* dt);

	void InitExternalFX();
	void InitFXAA(); // Bug workaround! Stack corruption? Heap corruption? No idea
	
	//

	CComPtr<ID3D11Device> m_dev;
	CComPtr<ID3D11DeviceContext> m_ctx;
	CComPtr<IDXGISwapChain> m_swapchain;
	CComPtr<ID3D11Buffer> m_vb;
	CComPtr<ID3D11Buffer> m_vb_old;
	CComPtr<ID3D11Buffer> m_ib;
	CComPtr<ID3D11Buffer> m_ib_old;

	bool m_srv_changed, m_ss_changed;

	struct
	{
		ID3D11Buffer* vb;
		size_t vb_stride;
		ID3D11Buffer* ib;
		ID3D11InputLayout* layout;
		D3D11_PRIMITIVE_TOPOLOGY topology;
		ID3D11VertexShader* vs;
		ID3D11Buffer* vs_cb;
		ID3D11GeometryShader* gs;
		ID3D11ShaderResourceView* ps_srv[16];
		ID3D11PixelShader* ps;
		ID3D11Buffer* ps_cb;
		ID3D11SamplerState* ps_ss[3];
		ID3D11ShaderResourceView* cs_srv[16];
		ID3D11ComputeShader* cs;
		ID3D11Buffer* cs_cb;
		GSVector2i viewport;
		GSVector4i scissor;
		ID3D11DepthStencilState* dss;
		uint8 sref;
		ID3D11BlendState* bs;
		float bf;
		ID3D11RenderTargetView* rtv;
		ID3D11DepthStencilView* dsv;
	} m_state;

public: // TODO
	CComPtr<ID3D11RasterizerState> m_rs;

	bool FXAA_Compiled;
	bool ExShader_Compiled;
	bool UserHacks_NVIDIAHack;

	struct
	{
		CComPtr<ID3D11InputLayout> il;
		CComPtr<ID3D11VertexShader> vs;
		CComPtr<ID3D11PixelShader> ps[10];
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

	struct
	{
		CComPtr<ID3D11PixelShader> ps;
		CComPtr<ID3D11Buffer> cb;
	} m_shaderfx;

	struct 
	{
		CComPtr<ID3D11PixelShader> ps;
		CComPtr<ID3D11Buffer> cb;
	} m_fxaa;

	struct 
	{
		CComPtr<ID3D11PixelShader> ps;
		CComPtr<ID3D11Buffer> cb;
	} m_shadeboost;

	struct
	{
		CComPtr<ID3D11DepthStencilState> dss;
		CComPtr<ID3D11BlendState> bs;
	} m_date;

	void SetupDATE(GSTexture* rt, GSTexture* ds, const GSVertexPT1* vertices, bool datm);

	// Shaders...

	hash_map<uint32, GSVertexShader11 > m_vs;
	CComPtr<ID3D11Buffer> m_vs_cb;
	hash_map<uint32, CComPtr<ID3D11GeometryShader> > m_gs;
	hash_map<uint32, CComPtr<ID3D11PixelShader> > m_ps;
	CComPtr<ID3D11Buffer> m_ps_cb;
	hash_map<uint32, CComPtr<ID3D11SamplerState> > m_ps_ss;
	CComPtr<ID3D11SamplerState> m_palette_ss;
	CComPtr<ID3D11SamplerState> m_rt_ss;
	hash_map<uint32, CComPtr<ID3D11DepthStencilState> > m_om_dss;
	hash_map<uint32, CComPtr<ID3D11BlendState> > m_om_bs;

	VSConstantBuffer m_vs_cb_cache;
	PSConstantBuffer m_ps_cb_cache;

	bool CreateTextureFX();

public:
	GSDevice11();
	virtual ~GSDevice11();

	bool Create(GSWnd* wnd);
	bool Reset(int w, int h);
	void Flip();

	void SetExclusive(bool isExcl);

	void DrawPrimitive();
	void DrawIndexedPrimitive();
	void DrawIndexedPrimitive(int offset, int count);
	void Dispatch(uint32 x, uint32 y, uint32 z);

	void ClearRenderTarget(GSTexture* t, const GSVector4& c);
	void ClearRenderTarget(GSTexture* t, uint32 c);
	void ClearDepth(GSTexture* t, float c);
	void ClearStencil(GSTexture* t, uint8 c);

	GSTexture* CreateRenderTarget(int w, int h, bool msaa, int format = 0);
	GSTexture* CreateDepthStencil(int w, int h, bool msaa, int format = 0);
	GSTexture* CreateTexture(int w, int h, int format = 0);
	GSTexture* CreateOffscreen(int w, int h, int format = 0);

	GSTexture* Resolve(GSTexture* t);

	GSTexture* CopyOffscreen(GSTexture* src, const GSVector4& sr, int w, int h, int format = 0);

	void CopyRect(GSTexture* st, GSTexture* dt, const GSVector4i& r);

	void StretchRect(GSTexture* st, const GSVector4& sr, GSTexture* dt, const GSVector4& dr, int shader = 0, bool linear = true);
	void StretchRect(GSTexture* st, const GSVector4& sr, GSTexture* dt, const GSVector4& dr, ID3D11PixelShader* ps, ID3D11Buffer* ps_cb, bool linear = true);
	void StretchRect(GSTexture* st, const GSVector4& sr, GSTexture* dt, const GSVector4& dr, ID3D11PixelShader* ps, ID3D11Buffer* ps_cb, ID3D11BlendState* bs, bool linear = true);

	void IASetVertexBuffer(const void* vertex, size_t stride, size_t count);
	bool IAMapVertexBuffer(void** vertex, size_t stride, size_t count);
	void IAUnmapVertexBuffer();
	void IASetVertexBuffer(ID3D11Buffer* vb, size_t stride);
	void IASetIndexBuffer(const void* index, size_t count);
	void IASetIndexBuffer(ID3D11Buffer* ib);
	void IASetInputLayout(ID3D11InputLayout* layout);
	void IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY topology);
	void VSSetShader(ID3D11VertexShader* vs, ID3D11Buffer* vs_cb);
	void GSSetShader(ID3D11GeometryShader* gs);
	void PSSetShaderResources(GSTexture* sr0, GSTexture* sr1);
	void PSSetShaderResource(int i, GSTexture* sr);
	void PSSetShaderResourceView(int i, ID3D11ShaderResourceView* srv);
	void PSSetShader(ID3D11PixelShader* ps, ID3D11Buffer* ps_cb);
	void PSSetSamplerState(ID3D11SamplerState* ss0, ID3D11SamplerState* ss1, ID3D11SamplerState* ss2 = NULL);
	void CSSetShaderSRV(int i, ID3D11ShaderResourceView* srv);
	void CSSetShaderUAV(int i, ID3D11UnorderedAccessView* uav);
	void CSSetShader(ID3D11ComputeShader* cs, ID3D11Buffer* cs_cb);
	void OMSetDepthStencilState(ID3D11DepthStencilState* dss, uint8 sref);
	void OMSetBlendState(ID3D11BlendState* bs, float bf);
	void OMSetRenderTargets(GSTexture* rt, GSTexture* ds, const GSVector4i* scissor = NULL);
	void OMSetRenderTargets(const GSVector2i& rtsize, int count, ID3D11UnorderedAccessView** uav, uint32* counters, const GSVector4i* scissor = NULL);

	void SetupVS(VSSelector sel, const VSConstantBuffer* cb);
	void SetupGS(GSSelector sel);
	void SetupPS(PSSelector sel, const PSConstantBuffer* cb, PSSamplerSelector ssel);
	void SetupOM(OMDepthStencilSelector dssel, OMBlendSelector bsel, uint8 afix);

	bool HasStencil() { return true; }
	bool HasDepth32() { return true; }

	ID3D11Device* operator->() {return m_dev;}
	operator ID3D11Device*() {return m_dev;}
	operator ID3D11DeviceContext*() {return m_ctx;}

	void CompileShader(uint32 id, const char* entry, D3D11_SHADER_MACRO* macro, ID3D11VertexShader** vs, D3D11_INPUT_ELEMENT_DESC* layout, int count, ID3D11InputLayout** il);
	void CompileShader(uint32 id, const char* entry, D3D11_SHADER_MACRO* macro, ID3D11GeometryShader** gs);
	void CompileShader(uint32 id, const char* entry, D3D11_SHADER_MACRO* macro, ID3D11GeometryShader** gs, D3D11_SO_DECLARATION_ENTRY* layout, int count);	
	void CompileShader(uint32 id, const char* entry, D3D11_SHADER_MACRO* macro, ID3D11PixelShader** ps);
	void CompileShader(uint32 id, const char* entry, D3D11_SHADER_MACRO* macro, ID3D11ComputeShader** cs);
	void CompileShader(const char* fn, const char* entry, D3D11_SHADER_MACRO* macro, ID3D11ComputeShader** cs);
	void CompileShader(const char* fn, const char* entry, D3D11_SHADER_MACRO* macro, ID3D11VertexShader** vs, D3D11_INPUT_ELEMENT_DESC* layout, int count, ID3D11InputLayout** il);
	void CompileShader(const char* fn, const char* entry, D3D11_SHADER_MACRO* macro, ID3D11PixelShader** ps);
};

