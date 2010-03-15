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

#include "stdafx.h"
#include "GSDevice9.h"
#include "resource.h"
#include "GSTables.h"

GSTexture* GSDevice9::CreateMskFix(uint32 size, uint32 msk, uint32 fix)
{
	GSTexture* t = NULL;

	uint32 hash = (size << 20) | (msk << 10) | fix;

	hash_map<uint32, GSTexture*>::iterator i = m_mskfix.find(hash);

	if(i != m_mskfix.end())
	{
		t = i->second;
	}
	else
	{
		t = CreateTexture(size, 1, D3DFMT_R32F);

		if(t)
		{
			GSTexture::GSMap m;

			if(t->Map(m))
			{
				for(uint32 i = 0; i < size; i++)
				{
					((float*)m.bits)[i] = (float)((i & msk) | fix) / size;
				}

				t->Unmap();
			}

			m_mskfix[hash] = t;
		}
	}

	return t;
}

void GSDevice9::SetupIA(const void* vertices, int count, int prim)
{
	IASetVertexBuffer(vertices, sizeof(GSVertexHW9), count);
	IASetInputLayout(m_il);
	IASetPrimitiveTopology((D3DPRIMITIVETYPE)prim);
}

void GSDevice9::SetupVS(VSSelector sel, const VSConstantBuffer* cb)
{
	hash_map< uint32, CComPtr<IDirect3DVertexShader9> >::const_iterator i = m_vs.find(sel);

	if(i == m_vs.end())
	{
		string str[4];

		str[0] = format("%d", sel.bppz);
		str[1] = format("%d", sel.tme);
		str[2] = format("%d", sel.fst);
		str[3] = format("%d", sel.logz);

		D3DXMACRO macro[] =
		{
			{"VS_BPPZ", str[0].c_str()},
			{"VS_TME", str[1].c_str()},
			{"VS_FST", str[2].c_str()},
			{"VS_LOGZ", str[3].c_str()},
			{NULL, NULL},
		};

		static const D3DVERTEXELEMENT9 layout[] =
		{
			{0, 0, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
			{0, 8, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0},
			{0, 12, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 1},
			{0, 16,  D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
			D3DDECL_END()
		};

		CComPtr<IDirect3DVertexDeclaration9> il;
		CComPtr<IDirect3DVertexShader9> vs;

		CompileShader(IDR_TFX_FX, "vs_main", macro, &vs, layout, countof(layout), &il);

		if(m_il == NULL)
		{
			m_il = il;
		}

		m_vs[sel] = vs;

		i = m_vs.find(sel);
	}

	VSSetShader(i->second, (const float*)cb, sizeof(*cb) / sizeof(GSVector4));
}

void GSDevice9::SetupPS(PSSelector sel, const PSConstantBuffer* cb, PSSamplerSelector ssel)
{
	if(cb->WH.z > 0 && cb->WH.w > 0 && (sel.wms == 3 || sel.wmt == 3))
	{
		GSVector4i size(cb->WH);

		if(sel.wms == 3)
		{
			if(GSTexture* t = CreateMskFix(size.z, cb->MskFix.x, cb->MskFix.z))
			{
				m_dev->SetTexture(2, *(GSTexture9*)t);
			}
		}

		if(sel.wmt == 3)
		{
			if(GSTexture* t = CreateMskFix(size.w, cb->MskFix.y, cb->MskFix.w))
			{
				m_dev->SetTexture(3, *(GSTexture9*)t);
			}
		}
	}

	hash_map<uint32, CComPtr<IDirect3DPixelShader9> >::const_iterator i = m_ps.find(sel);

	if(i == m_ps.end())
	{
		string str[13];

		str[0] = format("%d", sel.fst);
		str[1] = format("%d", sel.wms);
		str[2] = format("%d", sel.wmt);
		str[3] = format("%d", sel.fmt);
		str[4] = format("%d", sel.aem);
		str[5] = format("%d", sel.tfx);
		str[6] = format("%d", sel.tcc);
		str[7] = format("%d", sel.atst);
		str[8] = format("%d", sel.fog);
		str[9] = format("%d", sel.clr1);
		str[10] = format("%d", sel.rt);
		str[11] = format("%d", sel.ltf);
		str[12] = format("%d", sel.colclip);

		D3DXMACRO macro[] =
		{
			{"PS_FST", str[0].c_str()},
			{"PS_WMS", str[1].c_str()},
			{"PS_WMT", str[2].c_str()},
			{"PS_FMT", str[3].c_str()},
			{"PS_AEM", str[4].c_str()},
			{"PS_TFX", str[5].c_str()},
			{"PS_TCC", str[6].c_str()},
			{"PS_ATST", str[7].c_str()},
			{"PS_FOG", str[8].c_str()},
			{"PS_CLR1", str[9].c_str()},
			{"PS_RT", str[10].c_str()},
			{"PS_LTF", str[11].c_str()},
			{"PS_COLCLIP", str[12].c_str()},
			{NULL, NULL},
		};

		CComPtr<IDirect3DPixelShader9> ps;

		CompileShader(IDR_TFX_FX, "ps_main", macro, &ps);

		m_ps[sel] = ps;

		i = m_ps.find(sel);
	}

	PSSetShader(i->second, (const float*)cb, sizeof(*cb) / sizeof(GSVector4));

	Direct3DSamplerState9* ss = NULL;

	if(sel.tfx != 4)
	{
		if(!(sel.fmt < 3 && sel.wms < 3 && sel.wmt < 3))
		{
			ssel.ltf = 0;
		}

		hash_map<uint32, Direct3DSamplerState9* >::const_iterator i = m_ps_ss.find(ssel);

		if(i != m_ps_ss.end())
		{
			ss = i->second;
		}
		else
		{
			ss = new Direct3DSamplerState9();

			memset(ss, 0, sizeof(*ss));

			ss->FilterMin[0] = ssel.ltf ? D3DTEXF_LINEAR : D3DTEXF_POINT;
			ss->FilterMag[0] = ssel.ltf ? D3DTEXF_LINEAR : D3DTEXF_POINT;
			ss->FilterMin[1] = D3DTEXF_POINT;
			ss->FilterMag[1] = D3DTEXF_POINT;

			ss->AddressU = ssel.tau ? D3DTADDRESS_WRAP : D3DTADDRESS_CLAMP;
			ss->AddressV = ssel.tav ? D3DTADDRESS_WRAP : D3DTADDRESS_CLAMP;

			m_ps_ss[ssel] = ss;
		}
	}

	PSSetSamplerState(ss);
}

void GSDevice9::SetupOM(OMDepthStencilSelector dssel, OMBlendSelector bsel, uint8 afix)
{
	Direct3DDepthStencilState9* dss = NULL;

	hash_map<uint32, Direct3DDepthStencilState9*>::const_iterator i = m_om_dss.find(dssel);

	if(i == m_om_dss.end())
	{
		dss = new Direct3DDepthStencilState9();

		memset(dss, 0, sizeof(*dss));

		if(dssel.date || dssel.fba)
		{
			dss->StencilEnable = true;
			dss->StencilReadMask = 1;
			dss->StencilWriteMask = 2;
			dss->StencilFunc = dssel.date ? D3DCMP_EQUAL : D3DCMP_ALWAYS;
			dss->StencilPassOp = dssel.fba ? D3DSTENCILOP_REPLACE : D3DSTENCILOP_KEEP;
			dss->StencilFailOp = dssel.fba ? D3DSTENCILOP_ZERO : D3DSTENCILOP_KEEP;
			dss->StencilDepthFailOp = dssel.fba ? D3DSTENCILOP_ZERO : D3DSTENCILOP_KEEP;
			dss->StencilRef = 3;
		}

		if(dssel.ztst != ZTST_ALWAYS || dssel.zwe)
		{
			static const D3DCMPFUNC ztst[] = 
			{
				D3DCMP_NEVER, 
				D3DCMP_ALWAYS, 
				D3DCMP_GREATEREQUAL, 
				D3DCMP_GREATER
			};

			dss->DepthEnable = true;
			dss->DepthWriteMask = dssel.zwe;
			dss->DepthFunc = ztst[dssel.ztst];
		}

		m_om_dss[dssel] = dss;

		i = m_om_dss.find(dssel);
	}

	OMSetDepthStencilState(i->second);

	hash_map<uint32, Direct3DBlendState9*>::const_iterator j = m_om_bs.find(bsel);

	if(j == m_om_bs.end())
	{
		Direct3DBlendState9* bs = new Direct3DBlendState9();

		memset(bs, 0, sizeof(*bs));

		bs->BlendEnable = bsel.abe;

		if(bsel.abe)
		{
			int i = ((bsel.a * 3 + bsel.b) * 3 + bsel.c) * 3 + bsel.d;

			bs->BlendOp = blendMapD3D9[i].op;
			bs->SrcBlend = blendMapD3D9[i].src;
			bs->DestBlend = blendMapD3D9[i].dst;
			bs->BlendOpAlpha = D3DBLENDOP_ADD;
			bs->SrcBlendAlpha = D3DBLEND_ONE;
			bs->DestBlendAlpha = D3DBLEND_ZERO;

			// Not very good but I don't wanna write another 81 row table
			if (bsel.negative) {
				if (bs->BlendOp == D3DBLENDOP_ADD)
					bs->BlendOp = D3DBLENDOP_REVSUBTRACT;
				else if (bs->BlendOp == D3DBLENDOP_REVSUBTRACT)
					bs->BlendOp = D3DBLENDOP_ADD;
				else
					; // god knows, best just not to mess with it for now
			}

			if(blendMapD3D9[i].bogus == 1)
			{
				(bsel.a == 0 ? bs->SrcBlend : bs->DestBlend) = D3DBLEND_ONE;

				const string afixstr = format("%d >> 7", afix);
				const char *col[3] = {"Cs", "Cd", "0"};
				const char *alpha[3] = {"As", "Ad", afixstr.c_str()};
				printf("Impossible blend for D3D: (%s - %s) * %s + %s\n",
					col[bsel.a], col[bsel.b], alpha[bsel.c], col[bsel.d]);
			}
		}

		// this is not a typo; dx9 uses BGRA rather than the gs native RGBA, unlike dx10
		if(bsel.wr) bs->RenderTargetWriteMask |= D3DCOLORWRITEENABLE_BLUE;
		if(bsel.wg) bs->RenderTargetWriteMask |= D3DCOLORWRITEENABLE_GREEN;
		if(bsel.wb) bs->RenderTargetWriteMask |= D3DCOLORWRITEENABLE_RED;
		if(bsel.wa) bs->RenderTargetWriteMask |= D3DCOLORWRITEENABLE_ALPHA;

		m_om_bs[bsel] = bs;

		j = m_om_bs.find(bsel);
	}

	OMSetBlendState(j->second, afix >= 0x80 ? 0xffffff : 0x020202 * afix);
}
