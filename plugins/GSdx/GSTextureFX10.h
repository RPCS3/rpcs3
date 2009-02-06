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
			DWORD bpp:3;
			DWORD bppz:2;
			DWORD tme:1;
			DWORD fst:1;
			DWORD prim:3;
		};

		DWORD dw;

		operator DWORD() {return dw & 0x3ff;}
	};

	__declspec(align(16)) struct PSConstantBuffer
	{
		GSVector4 FogColor;
		float MINU;
		float MAXU;
		float MINV;
		float MAXV;
		DWORD UMSK;
		DWORD UFIX;
		DWORD VMSK;
		DWORD VFIX;
		float TA0;
		float TA1;
		float AREF;
		float _pad[1];
		GSVector2 WH;
		GSVector2 rWrH;

		struct PSConstantBuffer() {memset(this, 0, sizeof(*this));}

		__forceinline bool Update(const PSConstantBuffer* cb)
		{
			GSVector4i* a = (GSVector4i*)this;
			GSVector4i* b = (GSVector4i*)cb;

			GSVector4i b0 = b[0];
			GSVector4i b1 = b[1];
			GSVector4i b2 = b[2];
			GSVector4i b3 = b[3];
			GSVector4i b4 = b[4];

			if(!((a[0] == b0) & (a[1] == b1) & (a[2] == b2) & (a[3] == b3) & (a[4] == b4)).alltrue())
			{
				a[0] = b0;
				a[1] = b1;
				a[2] = b2;
				a[3] = b3;
				a[4] = b4;

				return true;
			}

			return false;
		}
	};

	union PSSelector
	{
		struct
		{
			DWORD fst:1;
			DWORD wms:2;
			DWORD wmt:2;
			DWORD bpp:3;
			DWORD aem:1;
			DWORD tfx:3;
			DWORD tcc:1;
			DWORD ate:1;
			DWORD atst:3;
			DWORD fog:1;
			DWORD clr1:1;
			DWORD fba:1;
			DWORD aout:1;
		};

		DWORD dw;

		operator DWORD() {return dw & 0x1fffff;}
	};

	union GSSelector
	{
		struct
		{
			DWORD iip:1;
			DWORD prim:2;
		};

		DWORD dw;

		operator DWORD() {return dw & 0x7;}
	};

	union PSSamplerSelector
	{
		struct
		{
			DWORD tau:1;
			DWORD tav:1;
			DWORD min:1;
			DWORD mag:1;
		};

		DWORD dw;

		operator DWORD() {return dw & 0xf;}
	};

	union OMDepthStencilSelector
	{
		struct
		{
			DWORD zte:1;
			DWORD ztst:2;
			DWORD zwe:1;
			DWORD date:1;
		};

		DWORD dw;

		operator DWORD() {return dw & 0x1f;}
	};

	union OMBlendSelector
	{
		struct
		{
			DWORD abe:1;
			DWORD a:2;
			DWORD b:2;
			DWORD c:2;
			DWORD d:2;
			DWORD wr:1;
			DWORD wg:1;
			DWORD wb:1;
			DWORD wa:1;
		};

		DWORD dw;

		operator DWORD() {return dw & 0x1fff;}
	};

	#pragma pack(pop)

private:
	GSDevice10* m_dev;
	CComPtr<ID3D10InputLayout> m_il;
	CRBMapC<DWORD, CComPtr<ID3D10VertexShader> > m_vs;
	CComPtr<ID3D10Buffer> m_vs_cb;
	CRBMapC<DWORD, CComPtr<ID3D10GeometryShader> > m_gs;
	CRBMapC<DWORD, CComPtr<ID3D10PixelShader> > m_ps;
	CComPtr<ID3D10Buffer> m_ps_cb;
	CRBMapC<DWORD, CComPtr<ID3D10SamplerState> > m_ps_ss;
	CComPtr<ID3D10SamplerState> m_palette_ss;
	CRBMapC<DWORD, CComPtr<ID3D10DepthStencilState> > m_om_dss;	
	CRBMapC<DWORD, CComPtr<ID3D10BlendState> > m_om_bs;	

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
	bool SetupPS(PSSelector sel, const PSConstantBuffer* cb, PSSamplerSelector ssel, ID3D10ShaderResourceView* tex, ID3D10ShaderResourceView* pal);
	void UpdatePS(PSSelector sel, const PSConstantBuffer* cb, PSSamplerSelector ssel);
	void SetupRS(UINT w, UINT h, const RECT& scissor);
	void SetupOM(OMDepthStencilSelector dssel, OMBlendSelector bsel, float bf, ID3D10RenderTargetView* rtv, ID3D10DepthStencilView* dsv);
	void UpdateOM(OMDepthStencilSelector dssel, OMBlendSelector bsel, float bf);
	void Draw();
};
