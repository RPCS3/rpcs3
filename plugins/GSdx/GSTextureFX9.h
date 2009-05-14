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

#include "GSDevice9.h"

class GSTextureFX9
{
public:
	#pragma pack(push, 1)

	struct VSConstantBuffer
	{
		GSVector4 VertexScale;
		GSVector4 VertexOffset;
		GSVector2 TextureScale;
		float _pad[2];
	};

	union VSSelector
	{
		struct
		{
			uint32 bppz:2;
			uint32 tme:1;
			uint32 fst:1;
			uint32 logz:1;
		};

		uint32 key;

		operator uint32() {return key & 0x1f;}
	};

	struct PSConstantBuffer
	{
		GSVector4 FogColor;
		float MINU;
		float MAXU;
		float MINV;
		float MAXV;
		uint32 UMSK;
		uint32 UFIX;
		uint32 VMSK;
		uint32 VFIX;
		float TA0;
		float TA1;
		float AREF;
		float _pad[1];
		GSVector2 WH;
		GSVector2 rWrH;
	};

	union PSSelector
	{
		struct
		{
			uint32 fst:1;
			uint32 wms:2;
			uint32 wmt:2;
			uint32 bpp:3;
			uint32 aem:1;
			uint32 tfx:3;
			uint32 tcc:1;
			uint32 ate:1;
			uint32 atst:3;
			uint32 fog:1;
			uint32 clr1:1;
			uint32 rt:1;
		};

		uint32 key;

		operator uint32() {return key & 0xfffff;}
	};

	union PSSamplerSelector
	{
		struct
		{
			uint32 tau:1;
			uint32 tav:1;
			uint32 min:1;
			uint32 mag:1;
		};

		uint32 key;

		operator uint32() {return key & 0xf;}
	};

	union OMDepthStencilSelector
	{
		struct
		{
			uint32 zte:1;
			uint32 ztst:2;
			uint32 zwe:1;
			uint32 date:1;
			uint32 fba:1;
		};

		uint32 key;

		operator uint32() {return key & 0x3f;}
	};

	union OMBlendSelector
	{
		struct
		{
			uint32 abe:1;
			uint32 a:2;
			uint32 b:2;
			uint32 c:2;
			uint32 d:2;
			uint32 wr:1;
			uint32 wg:1;
			uint32 wb:1;
			uint32 wa:1;
		};

		uint32 key;

		operator uint32() {return key & 0x1fff;}
	};

	#pragma pack(pop)

private:
	GSDevice9* m_dev;
	CComPtr<IDirect3DVertexDeclaration9> m_il;
	hash_map<uint32, CComPtr<IDirect3DVertexShader9> > m_vs;
	D3DXHANDLE m_vs_params;
	hash_map<uint32, CComPtr<IDirect3DPixelShader9> > m_ps;
	hash_map<uint32, Direct3DSamplerState9* > m_ps_ss;
	hash_map<uint32, Direct3DDepthStencilState9* > m_om_dss;	
	hash_map<uint32, Direct3DBlendState9* > m_om_bs;	
	hash_map<uint32, GSTexture9> m_mskfix;

public:
	GSTextureFX9();

	bool Create(GSDevice9* dev);
	bool CreateMskFix(GSTexture9& t, uint32 size, uint32 msk, uint32 fix);
	
	bool SetupIA(const GSVertexHW9* vertices, int count, D3DPRIMITIVETYPE prim);
	bool SetupVS(VSSelector sel, const VSConstantBuffer* cb);
	bool SetupPS(PSSelector sel, const PSConstantBuffer* cb, PSSamplerSelector ssel, IDirect3DTexture9* tex, IDirect3DTexture9* pal, bool psrr);
	void UpdatePS(PSSelector sel, const PSConstantBuffer* cb, PSSamplerSelector ssel, bool psrr);
	void SetupRS(int w, int h, const GSVector4i& scissor);
	void SetupOM(OMDepthStencilSelector dssel, OMBlendSelector bsel, uint8 bf, IDirect3DSurface9* rt, IDirect3DSurface9* ds);
	void UpdateOM(OMDepthStencilSelector dssel, OMBlendSelector bsel, uint8 bf);
};
