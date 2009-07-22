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
	virtual ~GSTextureFX9();

	bool Create(GSDevice* dev);
	
	void SetupIA(const void* vertices, int count, int prim);
	void SetupVS(VSSelector sel, const VSConstantBuffer* cb);
	void SetupGS(GSSelector sel) {}
	void SetupPS(PSSelector sel, const PSConstantBuffer* cb, PSSamplerSelector ssel);
	void SetupOM(OMDepthStencilSelector dssel, OMBlendSelector bsel, uint8 afix);
};
