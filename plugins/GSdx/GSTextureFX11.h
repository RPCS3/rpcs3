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

#include "GSTextureFX.h"
#include "GSDevice11.h"

class GSTextureFX11 : public GSTextureFX
{
	GSDevice11* m_dev;
	CComPtr<ID3D11InputLayout> m_il;
	hash_map<uint32, CComPtr<ID3D11VertexShader> > m_vs;
	CComPtr<ID3D11Buffer> m_vs_cb;
	hash_map<uint32, CComPtr<ID3D11GeometryShader> > m_gs;
	hash_map<uint32, CComPtr<ID3D11PixelShader> > m_ps;
	CComPtr<ID3D11Buffer> m_ps_cb;
	hash_map<uint32, CComPtr<ID3D11SamplerState> > m_ps_ss;
	CComPtr<ID3D11SamplerState> m_palette_ss;
	hash_map<uint32, CComPtr<ID3D11DepthStencilState> > m_om_dss;	
	hash_map<uint32, CComPtr<ID3D11BlendState> > m_om_bs;	

	VSConstantBuffer m_vs_cb_cache;
	PSConstantBuffer m_ps_cb_cache;
	
public:
	GSTextureFX11();

	bool Create(GSDevice11* dev);
	
	bool SetupIA(const GSVertexHW11* vertices, int count, D3D11_PRIMITIVE_TOPOLOGY prim);
	bool SetupVS(VSSelector sel, const VSConstantBuffer* cb);
	bool SetupGS(GSSelector sel);
	bool SetupPS(PSSelector sel, const PSConstantBuffer* cb, PSSamplerSelector ssel, GSTexture* tex, GSTexture* pal);
	void UpdatePS(PSSelector sel, const PSConstantBuffer* cb, PSSamplerSelector ssel);
	void SetupRS(int w, int h, const GSVector4i& scissor);
	void SetupOM(OMDepthStencilSelector dssel, OMBlendSelector bsel, float bf, GSTexture* rt, GSTexture* ds);
	void UpdateOM(OMDepthStencilSelector dssel, OMBlendSelector bsel, float bf);
};
