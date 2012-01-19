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
#include "GSRendererCS.h"

GSRendererCS::GSRendererCS()
	: GSRenderer()
{
	m_nativeres = true;

	memset(m_vm_valid, 0, sizeof(m_vm_valid));
}

GSRendererCS::~GSRendererCS()
{
}

bool GSRendererCS::CreateDevice(GSDevice* dev_unk)
{
	if(!__super::CreateDevice(dev_unk))
		return false;

	HRESULT hr; 

	D3D11_DEPTH_STENCIL_DESC dsd;
	D3D11_BLEND_DESC bsd;
	D3D11_SAMPLER_DESC sd;
	D3D11_BUFFER_DESC bd;
	D3D11_TEXTURE2D_DESC td;
	D3D11_UNORDERED_ACCESS_VIEW_DESC uavd;

	D3D_FEATURE_LEVEL level;

	((GSDeviceDX*)dev_unk)->GetFeatureLevel(level);

	if(level < D3D_FEATURE_LEVEL_11_0)
		return false;

	GSDevice11* dev = (GSDevice11*)dev_unk;

	ID3D11DeviceContext* ctx = *dev;

	delete dev->CreateRenderTarget(1024, 1024, false);

	// empty depth stencil state

	memset(&dsd, 0, sizeof(dsd));

	dsd.StencilEnable = false;
	dsd.DepthEnable = false;

	hr = (*dev)->CreateDepthStencilState(&dsd, &m_dss);

	if(FAILED(hr)) return false;
	
	// empty blend state

	memset(&bsd, 0, sizeof(bsd));

	bsd.RenderTarget[0].BlendEnable = false;

	hr = (*dev)->CreateBlendState(&bsd, &m_bs);

	if(FAILED(hr)) return false;

	// point sampler

	memset(&sd, 0, sizeof(sd));

	sd.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;

	sd.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	sd.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;

	sd.MaxLOD = FLT_MAX;
	sd.MaxAnisotropy = 16;
	sd.ComparisonFunc = D3D11_COMPARISON_NEVER;

	hr = (*dev)->CreateSamplerState(&sd, &m_ss);

	if(FAILED(hr)) return false;

	// video memory (4MB)

	memset(&bd, 0, sizeof(bd));

	bd.ByteWidth = 4 * 1024 * 1024;
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
	bd.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;

	hr = (*dev)->CreateBuffer(&bd, NULL, &m_vm);

	if(FAILED(hr)) return false;

	memset(&uavd, 0, sizeof(uavd));

	uavd.Format = DXGI_FORMAT_R32_TYPELESS;
	uavd.Buffer.FirstElement = 0;
	uavd.Buffer.NumElements = 1024 * 1024;
	uavd.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_RAW;
	uavd.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;

	hr = (*dev)->CreateUnorderedAccessView(m_vm, &uavd, &m_vm_uav);

	if(FAILED(hr)) return false;
/*
	memset(&td, 0, sizeof(td));

	td.Width = PAGE_SIZE;
	td.Height = MAX_PAGES;
	td.Format = DXGI_FORMAT_R8_UINT;
	td.MipLevels = 1;
	td.ArraySize = 1;
	td.SampleDesc.Count = 1;
	td.SampleDesc.Quality = 0;
	td.Usage = D3D11_USAGE_DEFAULT;
	td.BindFlags = D3D11_BIND_UNORDERED_ACCESS;

	hr = (*dev)->CreateTexture2D(&td, NULL, &m_vm);

	if(FAILED(hr)) return false;

	memset(&uavd, 0, sizeof(uavd));

	uavd.Format = DXGI_FORMAT_R8_UINT;
	uavd.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;

	hr = (*dev)->CreateUnorderedAccessView(m_vm, &uavd, &m_vm_uav);

	if(FAILED(hr)) return false;
*/
	// one page, for copying between cpu<->gpu

	memset(&bd, 0, sizeof(bd));

	bd.ByteWidth = PAGE_SIZE;
	bd.Usage = D3D11_USAGE_STAGING;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;

	hr = (*dev)->CreateBuffer(&bd, NULL, &m_pb);

	if(FAILED(hr)) return false;
/*
	memset(&td, 0, sizeof(td));

	td.Width = PAGE_SIZE;
	td.Height = 1;
	td.Format = DXGI_FORMAT_R8_UINT;
	td.MipLevels = 1;
	td.ArraySize = 1;
	td.SampleDesc.Count = 1;
	td.SampleDesc.Quality = 0;
	td.Usage = D3D11_USAGE_STAGING;
	td.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;

	hr = (*dev)->CreateTexture2D(&td, NULL, &m_pb);

	if(FAILED(hr)) return false;
*/
	// VSConstantBuffer

	memset(&bd, 0, sizeof(bd));

	bd.ByteWidth = sizeof(VSConstantBuffer);
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	hr = (*dev)->CreateBuffer(&bd, NULL, &m_vs_cb);

	if(FAILED(hr)) return false;

	// PSConstantBuffer

	memset(&bd, 0, sizeof(bd));

	bd.ByteWidth = sizeof(PSConstantBuffer);
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	hr = (*dev)->CreateBuffer(&bd, NULL, &m_ps_cb);

	if(FAILED(hr)) return false;

	//

	memset(&bd, 0, sizeof(bd));

	bd.ByteWidth = 14 * sizeof(float) * 200000;
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.BindFlags = D3D11_BIND_STREAM_OUTPUT | D3D11_BIND_SHADER_RESOURCE;

	hr = (*dev)->CreateBuffer(&bd, NULL, &m_sob);

	//

	return true;
}

void GSRendererCS::VSync(int field)
{
	__super::VSync(field);

	//printf("%lld\n", m_perfmon.GetFrame());
}

GSTexture* GSRendererCS::GetOutput(int i)
{
	// TODO: create a compute shader which unswizzles the frame from m_vm to the output texture

	return NULL;
}

void GSRendererCS::Draw()
{
	GSDrawingEnvironment& env = m_env;
	GSDrawingContext* context = m_context;

	GSVector2i rtsize(2048, 2048);
	GSVector4i scissor = GSVector4i(context->scissor.in).rintersect(GSVector4i(rtsize).zwxy());
	GSVector4i bbox = GSVector4i(m_vt.m_min.p.floor().xyxy(m_vt.m_max.p.ceil()));
	GSVector4i r = bbox.rintersect(scissor);

	uint32 fm = context->FRAME.FBMSK;
	uint32 zm = context->ZBUF.ZMSK || context->TEST.ZTE == 0 ? 0xffffffff : 0;

	if(fm != 0xffffffff)
	{
		Write(context->offset.fb, r);

		// TODO: m_tc->InvalidateVideoMem(context->offset.fb, r, false);
	}

	if(zm != 0xffffffff)
	{
		Write(context->offset.zb, r);

		// TODO: m_tc->InvalidateVideoMem(context->offset.zb, r, false);
	}	

	if(PRIM->TME)
	{
		m_mem.m_clut.Read32(context->TEX0, env.TEXA);

		GSVector4i r;

		GetTextureMinMax(r, context->TEX0, context->CLAMP, m_vt.IsLinear());

		// TODO: unswizzle pages of r to a texture, check m_vm_valid, bit not set cpu->gpu, set gpu->gpu

		// TODO: Write transfer should directly write to m_vm, then Read/Write syncing won't be necessary, clut must be updated with the gpu also

		// TODO: tex = m_tc->LookupSource(context->TEX0, env.TEXA, r);

		// if(!tex) return;
	}

	//

	GSDevice11* dev = (GSDevice11*)m_dev;

	ID3D11DeviceContext* ctx = *dev;

	dev->BeginScene();

	// SetupOM

	ID3D11UnorderedAccessView* uavs[] = {m_vm_uav};

	dev->OMSetDepthStencilState(m_dss, 0);
	dev->OMSetBlendState(m_bs, 0);
	dev->OMSetRenderTargets(rtsize, uavs, countof(uavs), &scissor);
	
	// SetupIA

	D3D11_PRIMITIVE_TOPOLOGY topology;

	switch(m_vt.m_primclass)
	{
	case GS_POINT_CLASS:
		topology = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
		break;
	case GS_LINE_CLASS:
	case GS_SPRITE_CLASS:
		topology = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
		break;
	case GS_TRIANGLE_CLASS:
		topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		break;
	default:
		__assume(0);
	}

	dev->IASetVertexBuffer(m_vertex.buff, sizeof(GSVertex), m_vertex.next);
	dev->IASetIndexBuffer(m_index.buff, m_index.tail);
	dev->IASetPrimitiveTopology(topology);

	// SetupVS

	VSSelector vs_sel;

	vs_sel.tme = PRIM->TME;
	vs_sel.fst = PRIM->FST;

	VSConstantBuffer vs_cb;

	float sx = 2.0f / (rtsize.x << 4);
	float sy = 2.0f / (rtsize.y << 4);
	//float sx = 1.0f / 16;
	//float sy = 1.0f / 16;
	float ox = (float)(int)context->XYOFFSET.OFX;
	float oy = (float)(int)context->XYOFFSET.OFY;

	vs_cb.VertexScale  = GSVector4(sx, -sy, 0.0f, 0.0f);
	vs_cb.VertexOffset = GSVector4(ox * sx + 1, -(oy * sy + 1), 0.0f, -1.0f);
	//vs_cb.VertexScale  = GSVector4(sx, sy, 0.0f, 0.0f);
	//vs_cb.VertexOffset = GSVector4(ox * sx, oy * sy, 0.0f, -1.0f);

	{
		hash_map<uint32, GSVertexShader11 >::const_iterator i = m_vs.find(vs_sel);

		if(i == m_vs.end())
		{
			string str[2];

			str[0] = format("%d", vs_sel.tme);
			str[1] = format("%d", vs_sel.fst);

			D3D11_SHADER_MACRO macro[] =
			{
				{"VS_TME", str[0].c_str()},
				{"VS_FST", str[1].c_str()},
				{NULL, NULL},
			};
			
			D3D11_INPUT_ELEMENT_DESC layout[] =
			{
				{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
				{"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0},
				{"TEXCOORD", 1, DXGI_FORMAT_R32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
				{"POSITION", 0, DXGI_FORMAT_R16G16_UINT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0},
				{"POSITION", 1, DXGI_FORMAT_R32_UINT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0},
				{"TEXCOORD", 2, DXGI_FORMAT_R16G16_UINT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
				{"COLOR", 1, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0},
			};

			GSVertexShader11 vs;

			dev->CompileShader(IDR_CS_FX, "vs_main", macro, &vs.vs, layout, countof(layout), &vs.il);

			m_vs[vs_sel] = vs;

			i = m_vs.find(vs_sel);
		}

		ctx->UpdateSubresource(m_vs_cb, 0, NULL, &vs_cb, 0, 0); // TODO: only update if changed

		dev->VSSetShader(i->second.vs, m_vs_cb);

		dev->IASetInputLayout(i->second.il);
	}

	// SetupGS

	GSSelector gs_sel;

	gs_sel.iip = PRIM->IIP;
	gs_sel.prim = m_vt.m_primclass;

	CComPtr<ID3D11GeometryShader> gs;

	{
		hash_map<uint32, CComPtr<ID3D11GeometryShader> >::const_iterator i = m_gs.find(gs_sel);

		if(i != m_gs.end())
		{
			gs = i->second;
		}
		else
		{
			string str[2];

			str[0] = format("%d", gs_sel.iip);
			str[1] = format("%d", gs_sel.prim);

			D3D11_SHADER_MACRO macro[] =
			{
				{"GS_IIP", str[0].c_str()},
				{"GS_PRIM", str[1].c_str()},
				{NULL, NULL},
			};
			/*
			D3D11_SO_DECLARATION_ENTRY layout[] =
			{
				{0, "SV_Position", 0, 0, 4, 0},
				{0, "TEXCOORD", 0, 0, 2, 0},
				{0, "TEXCOORD", 1, 0, 4, 0},
				{0, "COLOR", 0, 0, 4, 0},
			};
			*/
			dev->CompileShader(IDR_CS_FX, "gs_main", macro, &gs);//, layout, countof(layout));

			m_gs[gs_sel] = gs;
		}
	}

	dev->GSSetShader(gs);

	// SetupPS

	PSSelector ps_sel;
	PSConstantBuffer ps_cb;

	hash_map<uint32, CComPtr<ID3D11PixelShader> >::const_iterator i = m_ps.find(ps_sel);

	if(i == m_ps.end())
	{
		string str[15];

		str[0] = format("%d", 0);

		D3D11_SHADER_MACRO macro[] =
		{
			{"PS_TODO", str[0].c_str()},
			{NULL, NULL},
		};

		CComPtr<ID3D11PixelShader> ps;

		dev->CompileShader(IDR_CS_FX, "ps_main", macro, &ps);

		m_ps[ps_sel] = ps;

		i = m_ps.find(ps_sel);
	}

	ctx->UpdateSubresource(m_ps_cb, 0, NULL, &ps_cb, 0, 0); // TODO: only update if changed

	dev->PSSetSamplerState(m_ss, NULL, NULL);

	dev->PSSetShader(i->second, m_ps_cb);

	// Offset

	OffsetBuffer* fzbo = NULL;
	
	GetOffsetBuffer(&fzbo);

	dev->PSSetShaderResourceView(0, fzbo->row_view);
	dev->PSSetShaderResourceView(1, fzbo->col_view);

	// TODO: 2 palette
	// TODO: 3, 4, ... texture levels

	//ID3D11Buffer* tmp[] = {m_sob};

	//ctx->SOSetTargets(countof(tmp), tmp, NULL);

	dev->DrawIndexedPrimitive();

	//ctx->SOSetTargets(0, NULL, NULL);

	if(0)
	{
		HRESULT hr;

		D3D11_BUFFER_DESC bd;

		memset(&bd, 0, sizeof(bd));

		bd.ByteWidth = 14 * sizeof(float) * 200000;
		bd.Usage = D3D11_USAGE_STAGING;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

		CComPtr<ID3D11Buffer> sob;

		hr = (*dev)->CreateBuffer(&bd, NULL, &sob);

		ctx->CopyResource(sob, m_sob);

		D3D11_MAPPED_SUBRESOURCE map;

		if(SUCCEEDED(ctx->Map(sob, 0, D3D11_MAP_READ, 0, &map)))
		{
			float* f = (float*)map.pData;

			for(int i = 0; i < 12; i++, f += 14)
			printf("%f %f %f %f\n%f %f\n%f %f %f %f\n%f %f %f %f\n", 
				f[0], f[1], f[2], f[3],
				f[4], f[5],
				f[6], f[7], f[8], f[9],
				f[10], f[11], f[12], f[13]);

			ctx->Unmap(sob, 0);
		}

	}

	if(1)
	{
		//Read(m_mem.GetOffset(0, 16, PSM_PSMCT32), GSVector4i(0, 0, 1024, 1024), false);

		//
		if(fm != 0xffffffff) Read(context->offset.fb, r, false);
		//
		if(zm != 0xffffffff) Read(context->offset.zb, r, false);

		std::string s;

		s = format("c:\\temp1\\_%05d_f%lld_rt1_%05x_%d.bmp", s_n, m_perfmon.GetFrame(), m_context->FRAME.Block(), m_context->FRAME.PSM);

		//
		m_mem.SaveBMP(s, m_context->FRAME.Block(), m_context->FRAME.FBW, m_context->FRAME.PSM, GetFrameRect().width(), 512);

		s = format("c:\\temp1\\_%05d_f%lld_zt1_%05x_%d.bmp", s_n, m_perfmon.GetFrame(), m_context->ZBUF.Block(), m_context->ZBUF.PSM);

		//
		m_mem.SaveBMP(s, m_context->ZBUF.Block(), m_context->FRAME.FBW, m_context->ZBUF.PSM, GetFrameRect().width(), 512);

		//m_mem.SaveBMP(s, 0, 16, PSM_PSMCT32, 1024, 1024);

		s_n++;
	}

	dev->EndScene();
}

void GSRendererCS::InvalidateVideoMem(const GIFRegBITBLTBUF& BITBLTBUF, const GSVector4i& r)
{
	GSOffset* o = m_mem.GetOffset(BITBLTBUF.DBP, BITBLTBUF.DBW, BITBLTBUF.DPSM);

	Read(o, r, true); // TODO: fully overwritten pages are not needed to be read, only invalidated (important)

	// TODO: false deps, 8H/4HL/4HH texture sharing pages with 24-bit target
	// TODO: invalidate texture cache
}

void GSRendererCS::InvalidateLocalMem(const GIFRegBITBLTBUF& BITBLTBUF, const GSVector4i& r, bool clut)
{
	GSOffset* o = m_mem.GetOffset(BITBLTBUF.SBP, BITBLTBUF.SBW, BITBLTBUF.SPSM);

	Read(o, r, false);
}

void GSRendererCS::Write(GSOffset* o, const GSVector4i& r)
{
	GSDevice11* dev = (GSDevice11*)m_dev;
	
	ID3D11DeviceContext* ctx = *dev;

	D3D11_BOX box;
	
	memset(&box, 0, sizeof(box));

	box.right = 1;
	box.bottom = 1;
	box.back = 1;

	uint32* pages = o->GetPages(r);

	for(size_t i = 0; pages[i] != GSOffset::EOP; i++)
	{
		uint32 page = pages[i];

		uint32 row = page >> 5;
		uint32 col = 1 << (page & 31);

		if((m_vm_valid[row] & col) == 0)
		{
			m_vm_valid[row] |= col;

			box.left = page * PAGE_SIZE;
			box.right = (page + 1) * PAGE_SIZE;

			ctx->UpdateSubresource(m_vm, 0, &box, m_mem.m_vm8 + page * PAGE_SIZE, 0, 0);
/*
			// m_vm texture row is 2k in bytes, one page is 8k => starting row: addr / 4k, number of rows: 8k / 2k = 4

			box.left = 0;
			box.right = PAGE_SIZE;
			box.top = page;
			box.bottom = box.top + 1;

			ctx->UpdateSubresource(m_vm, 0, &box, m_mem.m_vm8 + page * PAGE_SIZE, 0, 0);
*/
			if(0)
			printf("[%lld] write %05x %d %d (%d)\n", __rdtsc(), o->bp, o->bw, o->psm, page);
		}
	}

	delete [] pages;
}

void GSRendererCS::Read(GSOffset* o, const GSVector4i& r, bool invalidate)
{
	GSDevice11* dev = (GSDevice11*)m_dev;
	
	ID3D11DeviceContext* ctx = *dev;

	D3D11_BOX box;
	
	memset(&box, 0, sizeof(box));

	box.right = 1;
	box.bottom = 1;
	box.back = 1;

	uint32* pages = o->GetPages(r);

	for(size_t i = 0; pages[i] != GSOffset::EOP; i++)
	{
		uint32 page = pages[i];

		uint32 row = page >> 5;
		uint32 col = 1 << (page & 31);

		if(m_vm_valid[row] & col)
		{
			if(invalidate)
			{
				m_vm_valid[row] ^= col;
			}

			box.left = page * PAGE_SIZE;
			box.right = (page + 1) * PAGE_SIZE;

			ctx->CopySubresourceRegion(m_pb, 0, 0, 0, 0, m_vm, 0, &box);
/*
			// m_vm texture row is 2k in bytes, one page is 8k => starting row: addr / 4k, number of rows: 8k / 2k = 4

			box.left = 0;
			box.right = PAGE_SIZE;
			box.top = page;
			box.bottom = box.top + 1;

			ctx->CopySubresourceRegion(m_pb, 0, 0, 0, 0, m_vm, 0, &box);
*/
			D3D11_MAPPED_SUBRESOURCE map;

			if(SUCCEEDED(ctx->Map(m_pb, 0, D3D11_MAP_READ, 0, &map)))
			{
				memcpy(m_mem.m_vm8 + page * PAGE_SIZE, map.pData, PAGE_SIZE);

				ctx->Unmap(m_pb, 0);
				
				if(0)
				printf("[%lld] read %05x %d %d (%d)\n", __rdtsc(), o->bp, o->bw, o->psm, page);
			}
		}
	}

	delete [] pages;
}

bool GSRendererCS::GetOffsetBuffer(OffsetBuffer** fzbo)
{
	HRESULT hr;

	GSDevice11* dev = (GSDevice11*)m_dev;

	D3D11_BUFFER_DESC bd;
	D3D11_SHADER_RESOURCE_VIEW_DESC srvd;
	D3D11_SUBRESOURCE_DATA data;

	hash_map<uint32, OffsetBuffer>::iterator i = m_offset.find(m_context->offset.fzb->hash);

	if(i == m_offset.end())
	{
		OffsetBuffer ob;

		memset(&bd, 0, sizeof(bd));

		bd.ByteWidth = sizeof(GSVector2i) * 2048;
		bd.Usage = D3D11_USAGE_IMMUTABLE;
		bd.BindFlags = D3D11_BIND_SHADER_RESOURCE;

		memset(&data, 0, sizeof(data));

		data.pSysMem = m_context->offset.fzb->row;

		hr = (*dev)->CreateBuffer(&bd, &data, &ob.row);

		if(FAILED(hr)) return false;

		data.pSysMem = m_context->offset.fzb->col;

		hr = (*dev)->CreateBuffer(&bd, &data, &ob.col);

		if(FAILED(hr)) return false;

		memset(&srvd, 0, sizeof(srvd));

		srvd.Format = DXGI_FORMAT_R32G32_SINT;
		srvd.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
		srvd.Buffer.FirstElement = 0;
		srvd.Buffer.NumElements = 2048;

		hr = (*dev)->CreateShaderResourceView(ob.row, &srvd, &ob.row_view);

		if(FAILED(hr)) return false;

		hr = (*dev)->CreateShaderResourceView(ob.col, &srvd, &ob.col_view);

		if(FAILED(hr)) return false;

		m_offset[m_context->offset.fzb->hash] = ob;

		i = m_offset.find(m_context->offset.fzb->hash);
	}

	*fzbo = &i->second;

	return true;
}
