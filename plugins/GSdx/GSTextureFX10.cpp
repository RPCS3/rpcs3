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
#include "GSDevice10.h"
#include "resource.h"
#include "GSTables.h"

bool GSDevice10::CreateTextureFX()
{
	HRESULT hr;

	D3D10_BUFFER_DESC bd;

	memset(&bd, 0, sizeof(bd));

	bd.ByteWidth = sizeof(VSConstantBuffer);
	bd.Usage = D3D10_USAGE_DEFAULT;
	bd.BindFlags = D3D10_BIND_CONSTANT_BUFFER;

	hr = m_dev->CreateBuffer(&bd, NULL, &m_vs_cb);

	if(FAILED(hr)) return false;

	memset(&bd, 0, sizeof(bd));

	bd.ByteWidth = sizeof(PSConstantBuffer);
	bd.Usage = D3D10_USAGE_DEFAULT;
	bd.BindFlags = D3D10_BIND_CONSTANT_BUFFER;

	hr = m_dev->CreateBuffer(&bd, NULL, &m_ps_cb);

	if(FAILED(hr)) return false;

	D3D10_SAMPLER_DESC sd;

	memset(&sd, 0, sizeof(sd));

	sd.Filter = D3D10_FILTER_MIN_MAG_MIP_POINT;
	sd.AddressU = D3D10_TEXTURE_ADDRESS_CLAMP;
	sd.AddressV = D3D10_TEXTURE_ADDRESS_CLAMP;
	sd.AddressW = D3D10_TEXTURE_ADDRESS_CLAMP;
	sd.MaxLOD = FLT_MAX;
	sd.MaxAnisotropy = 16; 
	sd.ComparisonFunc = D3D10_COMPARISON_NEVER;

	hr = m_dev->CreateSamplerState(&sd, &m_palette_ss);

	if(FAILED(hr)) return false;

	// create layout

	VSSelector sel;
	VSConstantBuffer cb;

	SetupVS(sel, &cb);
	
	return true;
}

void GSDevice10::SetupIA(const void* vertices, int count, int prim)
{
	IASetVertexBuffer(vertices, sizeof(GSVertexHW10), count);
	IASetInputLayout(m_il);
	IASetPrimitiveTopology((D3D10_PRIMITIVE_TOPOLOGY)prim);
}

void GSDevice10::SetupVS(VSSelector sel, const VSConstantBuffer* cb)
{
	hash_map<uint32, CComPtr<ID3D10VertexShader> >::const_iterator i = m_vs.find(sel);

	if(i == m_vs.end())
	{
		string str[3];

		str[0] = format("%d", sel.bppz);
		str[1] = format("%d", sel.tme);
		str[2] = format("%d", sel.fst);

		D3D10_SHADER_MACRO macro[] =
		{
			{"VS_BPPZ", str[0].c_str()},
			{"VS_TME", str[1].c_str()},
			{"VS_FST", str[2].c_str()},
			{NULL, NULL},
		};

		D3D10_INPUT_ELEMENT_DESC layout[] =
		{
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D10_INPUT_PER_VERTEX_DATA, 0},
			{"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 8, D3D10_INPUT_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 1, DXGI_FORMAT_R32_FLOAT, 0, 12, D3D10_INPUT_PER_VERTEX_DATA, 0},
			{"POSITION", 0, DXGI_FORMAT_R16G16_UINT, 0, 16, D3D10_INPUT_PER_VERTEX_DATA, 0},
			{"POSITION", 1, DXGI_FORMAT_R32_UINT, 0, 20, D3D10_INPUT_PER_VERTEX_DATA, 0},
			{"COLOR", 1, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 28, D3D10_INPUT_PER_VERTEX_DATA, 0},
		};

		CComPtr<ID3D10InputLayout> il;
		CComPtr<ID3D10VertexShader> vs;

		CompileShader(IDR_TFX_FX, "vs_main", macro, &vs, layout, countof(layout), &il);

		if(m_il == NULL)
		{
			m_il = il;
		}

		m_vs[sel] = vs;

		i = m_vs.find(sel);
	}

	if(m_vs_cb_cache.Update(cb))
	{
		m_dev->UpdateSubresource(m_vs_cb, 0, NULL, cb, 0, 0);
	}

	VSSetShader(i->second, m_vs_cb);
}

void GSDevice10::SetupGS(GSSelector sel)
{
	ID3D10GeometryShader* gs = NULL;

	if(sel.prim > 0 && (sel.iip == 0 || sel.prim == 3)) // geometry shader works in every case, but not needed
	{
		hash_map<uint32, CComPtr<ID3D10GeometryShader> >::const_iterator i = m_gs.find(sel);

		if(i != m_gs.end())
		{
			gs = i->second;
		}
		else
		{
			string str[2];

			str[0] = format("%d", sel.iip);
			str[1] = format("%d", sel.prim);

			D3D10_SHADER_MACRO macro[] =
			{
				{"GS_IIP", str[0].c_str()},
				{"GS_PRIM", str[1].c_str()},
				{NULL, NULL},
			};

			CompileShader(IDR_TFX_FX, "gs_main", macro, &gs);

			m_gs[sel] = gs;
		}
	}

	GSSetShader(gs);
}

void GSDevice10::SetupPS(PSSelector sel, const PSConstantBuffer* cb, PSSamplerSelector ssel)
{
	hash_map<uint32, CComPtr<ID3D10PixelShader> >::const_iterator i = m_ps.find(sel);

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
		str[10] = format("%d", sel.fba);
		str[11] = format("%d", sel.aout);
		str[12] = format("%d", sel.ltf);

		D3D10_SHADER_MACRO macro[] =
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
			{"PS_FBA", str[10].c_str()},
			{"PS_AOUT", str[11].c_str()},
			{"PS_LTF", str[12].c_str()},
			{NULL, NULL},
		};

		CComPtr<ID3D10PixelShader> ps;
		
		CompileShader(IDR_TFX_FX, "ps_main", macro, &ps);

		m_ps[sel] = ps;

		i = m_ps.find(sel);
	}

	if(m_ps_cb_cache.Update(cb))
	{
		m_dev->UpdateSubresource(m_ps_cb, 0, NULL, cb, 0, 0);
	}

	PSSetShader(i->second, m_ps_cb);

	ID3D10SamplerState* ss0 = NULL;
	ID3D10SamplerState* ss1 = NULL;

	if(sel.tfx != 4)
	{
		if(!(sel.fmt < 3 && sel.wms < 3 && sel.wmt < 3))
		{
			ssel.ltf = 0;
		}

		hash_map<uint32, CComPtr<ID3D10SamplerState> >::const_iterator i = m_ps_ss.find(ssel);

		if(i != m_ps_ss.end())
		{
			ss0 = i->second;
		}
		else
		{
			D3D10_SAMPLER_DESC sd;

			memset(&sd, 0, sizeof(sd));

			sd.Filter = ssel.ltf ? D3D10_FILTER_MIN_MAG_LINEAR_MIP_POINT : D3D10_FILTER_MIN_MAG_MIP_POINT;

			sd.AddressU = ssel.tau ? D3D10_TEXTURE_ADDRESS_WRAP : D3D10_TEXTURE_ADDRESS_CLAMP;
			sd.AddressV = ssel.tav ? D3D10_TEXTURE_ADDRESS_WRAP : D3D10_TEXTURE_ADDRESS_CLAMP;
			sd.AddressW = D3D10_TEXTURE_ADDRESS_CLAMP;

			sd.MaxLOD = FLT_MAX;
			sd.MaxAnisotropy = 16; 
			sd.ComparisonFunc = D3D10_COMPARISON_NEVER;

			m_dev->CreateSamplerState(&sd, &ss0);

			m_ps_ss[ssel] = ss0;
		}

		if(sel.fmt >= 3)
		{
			ss1 = m_palette_ss;
		}
	}

	PSSetSamplerState(ss0, ss1);
}

void GSDevice10::SetupOM(OMDepthStencilSelector dssel, OMBlendSelector bsel, uint8 afix)
{
/*
	hash_map<uint32, CComPtr<ID3D10DepthStencilState> >::const_iterator i = m_om_dss.find(dssel);

	if(i == m_om_dss.end())
	{
*/
	CComPtr<ID3D10DepthStencilState>& om_dss = m_om_dss[dssel];

	if(om_dss == NULL)
	{
		D3D10_DEPTH_STENCIL_DESC dsd;

		memset(&dsd, 0, sizeof(dsd));

		if(dssel.date)
		{
			dsd.StencilEnable = true;
			dsd.StencilReadMask = 1;
			dsd.StencilWriteMask = 1;
			dsd.FrontFace.StencilFunc = D3D10_COMPARISON_EQUAL;
			dsd.FrontFace.StencilPassOp = D3D10_STENCIL_OP_KEEP;
			dsd.FrontFace.StencilFailOp = D3D10_STENCIL_OP_KEEP;
			dsd.FrontFace.StencilDepthFailOp = D3D10_STENCIL_OP_KEEP;
			dsd.BackFace.StencilFunc = D3D10_COMPARISON_EQUAL;
			dsd.BackFace.StencilPassOp = D3D10_STENCIL_OP_KEEP;
			dsd.BackFace.StencilFailOp = D3D10_STENCIL_OP_KEEP;
			dsd.BackFace.StencilDepthFailOp = D3D10_STENCIL_OP_KEEP;
		}

		if(dssel.ztst != ZTST_ALWAYS || dssel.zwe)
		{
			static const D3D10_COMPARISON_FUNC ztst[] = 
			{
				D3D10_COMPARISON_NEVER, 
				D3D10_COMPARISON_ALWAYS, 
				D3D10_COMPARISON_GREATER_EQUAL, 
				D3D10_COMPARISON_GREATER
			};

			dsd.DepthEnable = true;
			dsd.DepthWriteMask = dssel.zwe ? D3D10_DEPTH_WRITE_MASK_ALL : D3D10_DEPTH_WRITE_MASK_ZERO;
			dsd.DepthFunc = ztst[dssel.ztst];
		}
/*
		CComPtr<ID3D10DepthStencilState> dss;

		m_dev->CreateDepthStencilState(&dsd, &dss);

		m_om_dss[dssel] = dss;

		i = m_om_dss.find(dssel);
*/
		m_dev->CreateDepthStencilState(&dsd, &om_dss);
	}

//	OMSetDepthStencilState(i->second, 1);

	OMSetDepthStencilState(om_dss, 1);

	hash_map<uint32, CComPtr<ID3D10BlendState> >::const_iterator j = m_om_bs.find(bsel);

	if(j == m_om_bs.end())
	{
		D3D10_BLEND_DESC bd;

		memset(&bd, 0, sizeof(bd));

		bd.BlendEnable[0] = bsel.abe;

		if(bsel.abe)
		{
			int i = ((bsel.a * 3 + bsel.b) * 3 + bsel.c) * 3 + bsel.d;

			bd.BlendOp = (D3D10_BLEND_OP)blendMapD3D9[i].op;
			bd.SrcBlend = (D3D10_BLEND)blendMapD3D9[i].src;
			bd.DestBlend = (D3D10_BLEND)blendMapD3D9[i].dst;
			bd.BlendOpAlpha = D3D10_BLEND_OP_ADD;
			bd.SrcBlendAlpha = D3D10_BLEND_ONE;
			bd.DestBlendAlpha = D3D10_BLEND_ZERO;

			// SRC* -> SRC1*
			// Yes, this casting mess really is needed.  I want to go back to C
			if (bd.SrcBlend >= 3 && bd.SrcBlend <= 6)
				bd.SrcBlend = (D3D10_BLEND)((int)bd.SrcBlend + 13);
			if (bd.DestBlend >= 3 && bd.DestBlend <= 6)
				bd.DestBlend = (D3D10_BLEND)((int)bd.DestBlend + 13);

			if(blendMapD3D9[i].bogus == 1)
			{
				ASSERT(0);

				(bsel.a == 0 ? bd.SrcBlend : bd.DestBlend) = D3D10_BLEND_ONE;
			}
		}

		if(bsel.wr) bd.RenderTargetWriteMask[0] |= D3D10_COLOR_WRITE_ENABLE_RED;
		if(bsel.wg) bd.RenderTargetWriteMask[0] |= D3D10_COLOR_WRITE_ENABLE_GREEN;
		if(bsel.wb) bd.RenderTargetWriteMask[0] |= D3D10_COLOR_WRITE_ENABLE_BLUE;
		if(bsel.wa) bd.RenderTargetWriteMask[0] |= D3D10_COLOR_WRITE_ENABLE_ALPHA;

		CComPtr<ID3D10BlendState> bs;

		m_dev->CreateBlendState(&bd, &bs);

		m_om_bs[bsel] = bs;

		j = m_om_bs.find(bsel);
	}

	OMSetBlendState(j->second, (float)(int)afix / 0x80);
}
