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

#define PS_BATCH_SIZE 512

GSRendererCS::GSRendererCS()
	: GSRenderer()
{
	m_nativeres = true;

	memset(m_vm_valid, 0, sizeof(m_vm_valid));

	memset(m_texture, 0, sizeof(m_texture));

	m_output = (uint8*)_aligned_malloc(1024 * 1024 * sizeof(uint32), 32);
}

GSRendererCS::~GSRendererCS()
{
	for(int i = 0; i < countof(m_texture); i++)
	{
		delete m_texture[i];
	}

	_aligned_free(m_output);
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
	D3D11_SHADER_RESOURCE_VIEW_DESC srvd;

	D3D_FEATURE_LEVEL level;

	((GSDeviceDX*)dev_unk)->GetFeatureLevel(level);

	if(level < D3D_FEATURE_LEVEL_11_0)
		return false;

	GSDevice11* dev = (GSDevice11*)dev_unk;

	ID3D11DeviceContext* ctx = *dev;

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

	// link buffer

	memset(&bd, 0, sizeof(bd));

	bd.ByteWidth = 256 << 20; // 256 MB w00t
	bd.StructureByteStride = sizeof(uint32) * 4; // c, z, id, next
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	bd.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

	hr = (*dev)->CreateBuffer(&bd, NULL, &m_lb);

	{
		uint32 data[] = {0, 0, 0xffffffff, 0};

		D3D11_BOX box;
		memset(&box, 0, sizeof(box));
		box.right = sizeof(data);
		box.bottom = 1;
		box.back = 1;

		ctx->UpdateSubresource(m_lb, 0, &box, data, 0, 0);
	}

	if(FAILED(hr)) return false;

	memset(&uavd, 0, sizeof(uavd));

	uavd.Format = DXGI_FORMAT_UNKNOWN;
	uavd.Buffer.NumElements = bd.ByteWidth / bd.StructureByteStride;
	uavd.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_COUNTER;
	uavd.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;

	hr = (*dev)->CreateUnorderedAccessView(m_lb, &uavd, &m_lb_uav);

	if(FAILED(hr)) return false;

	memset(&srvd, 0, sizeof(srvd));

	srvd.Format = DXGI_FORMAT_UNKNOWN;
	srvd.Buffer.NumElements = bd.ByteWidth / bd.StructureByteStride;
	srvd.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;

	hr = (*dev)->CreateShaderResourceView(m_lb, &srvd, &m_lb_srv);

	if(FAILED(hr)) return false;

	// start offset buffer

	memset(&bd, 0, sizeof(bd));

	bd.ByteWidth = sizeof(uint32) * 2048 * 2048; // index
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	bd.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;

	hr = (*dev)->CreateBuffer(&bd, NULL, &m_sob);

	if(FAILED(hr)) return false;

	memset(&uavd, 0, sizeof(uavd));

	uavd.Format = DXGI_FORMAT_R32_TYPELESS;
	uavd.Buffer.NumElements = bd.ByteWidth / sizeof(uint32);
	uavd.Buffer.Flags =  D3D11_BUFFER_UAV_FLAG_RAW;
	uavd.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;

	hr = (*dev)->CreateUnorderedAccessView(m_sob, &uavd, &m_sob_uav);

	if(FAILED(hr)) return false;

	memset(&srvd, 0, sizeof(srvd));

	srvd.Format = DXGI_FORMAT_R32_TYPELESS;
	srvd.BufferEx.NumElements = bd.ByteWidth / sizeof(uint32);
	srvd.BufferEx.Flags = D3D11_BUFFEREX_SRV_FLAG_RAW;
	srvd.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;

	hr = (*dev)->CreateShaderResourceView(m_sob, &srvd, &m_sob_srv);

	if(FAILED(hr)) return false;

	const uint32 tmp = 0;

	ctx->ClearUnorderedAccessViewUint(m_sob_uav, &tmp); // initial clear, next time Draw should restore it in Step 2

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

	// PS

	D3D11_SHADER_MACRO macro[] =
	{
		{NULL, NULL},
	};

	hr = dev->CompileShader(IDR_CS_FX, "ps_main0", macro, &m_ps0); 

	if(FAILED(hr)) return false;

	// PSConstantBuffer

	memset(&bd, 0, sizeof(bd));

	bd.ByteWidth = sizeof(PSConstantBuffer);
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	hr = (*dev)->CreateBuffer(&bd, NULL, &m_ps_cb);

	if(FAILED(hr)) return false;

	//

	return true;
}

void GSRendererCS::ResetDevice()
{
	for(int i = 0; i < countof(m_texture); i++)
	{
		delete m_texture[i];

		m_texture[i] = NULL;
	}
}

void GSRendererCS::VSync(int field)
{
	__super::VSync(field);

	//printf("%lld\n", m_perfmon.GetFrame());
}

GSTexture* GSRendererCS::GetOutput(int i)
{
	// TODO: create a compute shader which unswizzles the frame from m_vm to the output texture

	const GSRegDISPFB& DISPFB = m_regs->DISP[i].DISPFB;

	int w = DISPFB.FBW * 64;
	int h = GetFrameRect(i).bottom;

	// TODO: round up bottom

	if(m_dev->ResizeTexture(&m_texture[i], w, h))
	{
		const GSLocalMemory::psm_t& psm = GSLocalMemory::m_psm[DISPFB.PSM];

		GSVector4i r(0, 0, w, h);
		GSVector4i r2 = r.ralign<Align_Outside>(psm.bs);

		GSOffset* o = m_mem.GetOffset(DISPFB.Block(), DISPFB.FBW, DISPFB.PSM);

		Read(o, r2, false);

		(m_mem.*psm.rtx)(o, r2, m_output, 1024 * 4, m_env.TEXA);

		m_texture[i]->Update(r, m_output, 1024 * 4);

		if(s_dump)
		{
			if(s_save && s_n >= s_saven)
			{
				m_texture[i]->Save(format("c:\\temp1\\_%05d_f%lld_fr%d_%05x_%d.bmp", s_n, m_perfmon.GetFrame(), i, (int)DISPFB.Block(), (int)DISPFB.PSM));
			}

			s_n++;
		}
	}

	return m_texture[i];
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

	// TODO: if(24-bit) fm/zm |= 0xff000000;

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

	//

	dev->BeginScene();

	// SetupOM

	dev->OMSetDepthStencilState(m_dss, 0);
	dev->OMSetBlendState(m_bs, 0);
	
	ID3D11UnorderedAccessView* uavs[] = {m_vm_uav, m_lb_uav, m_sob_uav};
	uint32 counters[] = {1, 0, 0};

	dev->OMSetRenderTargets(rtsize, countof(uavs), uavs, counters, &scissor);

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

	GSVector4i r2 = bbox.add32(GSVector4i(-1, -1, 1, 1)).rintersect(scissor);

	m_vertex.buff[m_vertex.next + 0].XYZ.X = context->XYOFFSET.OFX + (r2.left << 4);
	m_vertex.buff[m_vertex.next + 0].XYZ.Y = context->XYOFFSET.OFY + (r2.top << 4);
	m_vertex.buff[m_vertex.next + 1].XYZ.X = context->XYOFFSET.OFX + (r2.right << 4);
	m_vertex.buff[m_vertex.next + 1].XYZ.Y = context->XYOFFSET.OFY + (r2.bottom << 4);
	
	m_index.buff[m_index.tail + 0] = m_vertex.next + 0;
	m_index.buff[m_index.tail + 1] = m_vertex.next + 1;

	dev->IASetVertexBuffer(m_vertex.buff, sizeof(GSVertex), m_vertex.next + 2);
	dev->IASetIndexBuffer(m_index.buff, m_index.tail + 2);

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
		GSVertexShader11 vs;

		hash_map<uint32, GSVertexShader11>::const_iterator i = m_vs.find(vs_sel);

		if(i != m_vs.end())
		{
			vs = i->second;
		}
		else
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

			dev->CompileShader(IDR_CS_FX, "vs_main", macro, &vs.vs, layout, countof(layout), &vs.il);

			m_vs[vs_sel] = vs;
		}

		ctx->UpdateSubresource(m_vs_cb, 0, NULL, &vs_cb, 0, 0); // TODO: only update if changed

		dev->VSSetShader(vs.vs, m_vs_cb);

		dev->IASetInputLayout(vs.il);
	}

	// SetupGS

	GSSelector gs_sel;

	gs_sel.iip = PRIM->IIP;

	CComPtr<ID3D11GeometryShader> gs[2];

	for(int j = 0; j < 2; j++)
	{
		gs_sel.prim = j == 0 ? m_vt.m_primclass : GS_SPRITE_CLASS;

		hash_map<uint32, CComPtr<ID3D11GeometryShader> >::const_iterator i = m_gs.find(gs_sel);

		if(i != m_gs.end())
		{
			gs[j] = i->second;
		}
		else
		{
			string str[2];

			str[0] = format("%d", gs_sel.iip);
			str[1] = format("%d", j == 0 ? gs_sel.prim : GS_SPRITE_CLASS);

			D3D11_SHADER_MACRO macro[] =
			{
				{"GS_IIP", str[0].c_str()},
				{"GS_PRIM", str[1].c_str()},
				{NULL, NULL},
			};

			dev->CompileShader(IDR_CS_FX, "gs_main", macro, &gs[j]);

			m_gs[gs_sel] = gs[j];
		}
	}

	// SetupPS

	dev->PSSetSamplerState(m_ss, NULL, NULL);

	PSSelector ps_sel;

	ps_sel.fpsm = context->FRAME.PSM;
	ps_sel.zpsm = context->ZBUF.PSM;

	CComPtr<ID3D11PixelShader> ps[2] = {m_ps0, NULL};

	hash_map<uint32, CComPtr<ID3D11PixelShader> >::const_iterator i = m_ps1.find(ps_sel);

	if(i != m_ps1.end())
	{
		ps[1] = i->second;
	}
	else
	{
		string str[15];

		str[0] = format("%d", PS_BATCH_SIZE);
		str[1] = format("%d", context->FRAME.PSM);
		str[2] = format("%d", context->ZBUF.PSM);

		D3D11_SHADER_MACRO macro[] =
		{
			{"PS_BATCH_SIZE", str[0].c_str()},
			{"PS_FPSM", str[1].c_str()},
			{"PS_ZPSM", str[2].c_str()},
			{NULL, NULL},
		};

		dev->CompileShader(IDR_CS_FX, "ps_main1", macro, &ps[1]);

		m_ps1[ps_sel] = ps[1];
	}

	PSConstantBuffer ps_cb;

	ps_cb.fm = fm;
	ps_cb.zm = zm;

	ctx->UpdateSubresource(m_ps_cb, 0, NULL, &ps_cb, 0, 0); // TODO: only update if changed

	OffsetBuffer* fzbo = NULL;
	
	GetOffsetBuffer(&fzbo);

	dev->PSSetShaderResourceView(0, fzbo->row_srv);
	dev->PSSetShaderResourceView(1, fzbo->col_srv);
	// TODO: palette, texture

	int step = PS_BATCH_SIZE * GSUtil::GetVertexCount(PRIM->PRIM);

	for(int i = 0; i < m_index.tail; i += step)
	{
		dev->IASetPrimitiveTopology(topology);
		dev->GSSetShader(gs[0]);
		dev->PSSetShader(ps[0], m_ps_cb);
		dev->DrawIndexedPrimitive(i, std::min<int>(m_index.tail - i, step));

		dev->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
		dev->GSSetShader(gs[1]);
		dev->PSSetShader(ps[1], m_ps_cb);
		dev->DrawIndexedPrimitive(m_index.tail, 2);

		//printf("%d/%d, %d %d %d %d\n", i, m_index.tail, r2.x, r2.y, r2.z, r2.w);
	}

	dev->EndScene();

	if(0)
	{
		std::string s;
		/*
		s = format("c:\\temp1\\_%05d_f%lld_fb0_%05x_%d.bmp", s_n, m_perfmon.GetFrame(), 0, 0);
		m_mem.SaveBMP(s, 0, 16, PSM_PSMCT32, 1024, 1024);
		Read(m_mem.GetOffset(0, 16, PSM_PSMCT32), GSVector4i(0, 0, 1024, 1024), false);
		*/
		//
		if(fm != 0xffffffff) Read(context->offset.fb, r, false);
		//
		if(zm != 0xffffffff) Read(context->offset.zb, r, false);

		s = format("c:\\temp1\\_%05d_f%lld_rt1_%05x_%d.bmp", s_n, m_perfmon.GetFrame(), m_context->FRAME.Block(), m_context->FRAME.PSM);
		m_mem.SaveBMP(s, m_context->FRAME.Block(), m_context->FRAME.FBW, m_context->FRAME.PSM, GetFrameRect().width(), 512);

		s = format("c:\\temp1\\_%05d_f%lld_zt1_%05x_%d.bmp", s_n, m_perfmon.GetFrame(), m_context->ZBUF.Block(), m_context->ZBUF.PSM);
		m_mem.SaveBMP(s, m_context->ZBUF.Block(), m_context->FRAME.FBW, m_context->ZBUF.PSM, GetFrameRect().width(), 512);

		/*
		s = format("c:\\temp1\\_%05d_f%lld_fb1_%05x_%d.bmp", s_n, m_perfmon.GetFrame(), 0, 0);
		m_mem.SaveBMP(s, 0, 16, PSM_PSMCT32, 1024, 1024);
		*/

		s_n++;
	}
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

		hr = (*dev)->CreateShaderResourceView(ob.row, &srvd, &ob.row_srv);

		if(FAILED(hr)) return false;

		hr = (*dev)->CreateShaderResourceView(ob.col, &srvd, &ob.col_srv);

		if(FAILED(hr)) return false;

		m_offset[m_context->offset.fzb->hash] = ob;

		i = m_offset.find(m_context->offset.fzb->hash);
	}

	*fzbo = &i->second;

	return true;
}
