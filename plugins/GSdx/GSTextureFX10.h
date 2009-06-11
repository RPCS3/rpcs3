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

#include "GSDevice10.h"

class GSTextureFX10
{
public:
	#pragma pack(push, 1)

	__declspec(align(16)) struct VSConstantBuffer
	{
		GSVector4 VertexScale;
		GSVector4 VertexOffset;
		GSVector2 TextureScale;
		float _pad[2];

		struct VSConstantBuffer() {memset(this, 0, sizeof(*this));}

		__forceinline bool Update(const VSConstantBuffer* cb)
		{
			GSVector4i* a = (GSVector4i*)this;
			GSVector4i* b = (GSVector4i*)cb;

			GSVector4i b0 = b[0];
			GSVector4i b1 = b[1];
			GSVector4i b2 = b[2];

			if(!((a[0] == b0) & (a[1] == b1) & (a[2] == b2)).alltrue())
			{
				a[0] = b0;
				a[1] = b1;
				a[2] = b2;

				return true;
			}

			return false;
		}
	};

	union VSSelector
	{
		struct
		{
			uint32 bpp:3;
			uint32 bppz:2;
			uint32 tme:1;
			uint32 fst:1;
			uint32 prim:3;
		};

		uint32 key;

		operator uint32() {return key & 0x3ff;}
	};

	__declspec(align(16)) struct PSConstantBuffer
	{
		GSVector4 FogColorAREF;
		GSVector4 TA;
		GSVector4i MinMax;
		GSVector4 MinMaxF;

		struct PSConstantBuffer() {memset(this, 0, sizeof(*this));}

		__forceinline bool Update(const PSConstantBuffer* cb)
		{
			GSVector4i* a = (GSVector4i*)this;
			GSVector4i* b = (GSVector4i*)cb;

			GSVector4i b0 = b[0];
			GSVector4i b1 = b[1];
			GSVector4i b2 = b[2];
			GSVector4i b3 = b[3];

			if(!((a[0] == b0) & (a[1] == b1) & (a[2] == b2) & (a[3] == b3)).alltrue())
			{
				a[0] = b0;
				a[1] = b1;
				a[2] = b2;
				a[3] = b3;

				return true;
			}

			return false;
		}
	};

	union GSSelector
	{
		struct
		{
			uint32 iip:1;
			uint32 prim:2;
		};

		uint32 key;

		operator uint32() {return key & 0x7;}
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
			uint32 fba:1;
			uint32 aout:1;
			uint32 ltf:1;
		};

		uint32 key;

		operator uint32() {return key & 0x3fffff;}
	};

	union PSSamplerSelector
	{
		struct
		{
			uint32 tau:1;
			uint32 tav:1;
			uint32 ltf:1;
		};

		uint32 key;

		operator uint32() {return key & 0x7;}
	};

	union OMDepthStencilSelector
	{
		struct
		{
			uint32 zte:1;
			uint32 ztst:2;
			uint32 zwe:1;
			uint32 date:1;
		};

		uint32 key;

		operator uint32() {return key & 0x1f;}
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
	GSDevice10* m_dev;
	CComPtr<ID3D10InputLayout> m_il;
	hash_map<uint32, CComPtr<ID3D10VertexShader> > m_vs;
	CComPtr<ID3D10Buffer> m_vs_cb;
	hash_map<uint32, CComPtr<ID3D10GeometryShader> > m_gs;
	hash_map<uint32, CComPtr<ID3D10PixelShader> > m_ps;
	CComPtr<ID3D10Buffer> m_ps_cb;
	hash_map<uint32, CComPtr<ID3D10SamplerState> > m_ps_ss;
	CComPtr<ID3D10SamplerState> m_palette_ss;
	hash_map<uint32, CComPtr<ID3D10DepthStencilState> > m_om_dss;	
	hash_map<uint32, CComPtr<ID3D10BlendState> > m_om_bs;	

	CComPtr<ID3D10Buffer> m_vb, m_vb_old;
	int m_vb_max;
	int m_vb_start;
	int m_vb_count;

	VSConstantBuffer m_vs_cb_cache;
	PSConstantBuffer m_ps_cb_cache;
	
public:
	GSTextureFX10();

	bool Create(GSDevice10* dev);
	
	bool SetupIA(const GSVertexHW10* vertices, int count, D3D10_PRIMITIVE_TOPOLOGY prim);
	bool SetupVS(VSSelector sel, const VSConstantBuffer* cb);
	bool SetupGS(GSSelector sel);
	bool SetupPS(PSSelector sel, const PSConstantBuffer* cb, PSSamplerSelector ssel, GSTexture* tex, GSTexture* pal);
	void UpdatePS(PSSelector sel, const PSConstantBuffer* cb, PSSamplerSelector ssel);
	void SetupRS(int w, int h, const GSVector4i& scissor);
	void SetupOM(OMDepthStencilSelector dssel, OMBlendSelector bsel, float bf, GSTexture* rt, GSTexture* ds);
	void UpdateOM(OMDepthStencilSelector dssel, OMBlendSelector bsel, float bf);
	void Draw();
};
