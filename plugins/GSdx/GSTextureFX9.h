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
#include "GSDevice9.h"

class GSTextureFX9 : public GSTextureFX
{
	GSDevice9* m_dev;
	CComPtr<IDirect3DVertexDeclaration9> m_il;
	hash_map<uint32, CComPtr<IDirect3DVertexShader9> > m_vs;
	D3DXHANDLE m_vs_params;
	hash_map<uint32, CComPtr<IDirect3DPixelShader9> > m_ps;
	hash_map<uint32, Direct3DSamplerState9* > m_ps_ss;
	hash_map<uint32, Direct3DDepthStencilState9* > m_om_dss;	
	hash_map<uint32, Direct3DBlendState9* > m_om_bs;	
	hash_map<uint32, GSTexture*> m_mskfix;

	GSTexture* CreateMskFix(uint32 size, uint32 msk, uint32 fix);

public:
	GSTextureFX9();

	bool Create(GSDevice9* dev);
	
	bool SetupIA(const GSVertexHW9* vertices, int count, D3DPRIMITIVETYPE prim);
	bool SetupVS(VSSelector sel, const VSConstantBuffer* cb);
	bool SetupPS(PSSelector sel, const PSConstantBuffer* cb, PSSamplerSelector ssel, GSTexture* tex, GSTexture* pal);
	void UpdatePS(PSSelector sel, const PSConstantBuffer* cb, PSSamplerSelector ssel);
	void SetupRS(int w, int h, const GSVector4i& scissor);
	void SetupOM(OMDepthStencilSelector dssel, OMBlendSelector bsel, uint8 bf, GSTexture* rt, GSTexture* ds);
	void UpdateOM(OMDepthStencilSelector dssel, OMBlendSelector bsel, uint8 bf);
};
