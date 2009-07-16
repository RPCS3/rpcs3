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
#include "GSDevice10.h"

class GSTextureFX10 : public GSTextureFX
{
	CComPtr<ID3D10InputLayout> m_il;
	hash_map<uint32, CComPtr<ID3D10VertexShader> > m_vs;
	CComPtr<ID3D10Buffer> m_vs_cb;
	hash_map<uint32, CComPtr<ID3D10GeometryShader> > m_gs;
	hash_map<uint32, CComPtr<ID3D10PixelShader> > m_ps;
	CComPtr<ID3D10Buffer> m_ps_cb;
	hash_map<uint32, CComPtr<ID3D10SamplerState> > m_ps_ss;
	CComPtr<ID3D10SamplerState> m_palette_ss;
	// hash_map<uint32, CComPtr<ID3D10DepthStencilState> > m_om_dss;
	CComPtr<ID3D10DepthStencilState> m_om_dss[32];
	hash_map<uint32, CComPtr<ID3D10BlendState> > m_om_bs;	

	VSConstantBuffer m_vs_cb_cache;
	PSConstantBuffer m_ps_cb_cache;
	
public:
	GSTextureFX10();

	bool Create(GSDevice* dev);
	
	void SetupIA(const void* vertices, int count, int prim);
	void SetupVS(VSSelector sel, const VSConstantBuffer* cb);
	void SetupGS(GSSelector sel);
	void SetupPS(PSSelector sel, const PSConstantBuffer* cb, PSSamplerSelector ssel, GSTexture* tex, GSTexture* pal);
	void UpdatePS(PSSelector sel, const PSConstantBuffer* cb, PSSamplerSelector ssel);
	void SetupRS(const GSVector2i& size, const GSVector4i& scissor);
	void SetupOM(OMDepthStencilSelector dssel, OMBlendSelector bsel, uint8 afix, GSTexture* rt, GSTexture* ds);
	void UpdateOM(OMDepthStencilSelector dssel, OMBlendSelector bsel, uint8 afix);
};
