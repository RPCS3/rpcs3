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
	: GSRenderer(new GSVertexTraceCS(this), sizeof(GSVertex))
{
	m_nativeres = true;

	InitConvertVertex(GSRendererCS);

	memset(m_vm_valid, 0, sizeof(m_vm_valid));
}

GSRendererCS::~GSRendererCS()
{
}

bool GSRendererCS::CreateDevice(GSDevice* dev_unk)
{
	if(!__super::CreateDevice(dev_unk))
		return false;

	D3D_FEATURE_LEVEL level;

	((GSDeviceDX*)dev_unk)->GetFeatureLevel(level);

	if(level < D3D_FEATURE_LEVEL_10_0)
		return false;

	HRESULT hr; 

	GSDevice11* dev = (GSDevice11*)dev_unk;

	D3D11_BUFFER_DESC bd;
	D3D11_UNORDERED_ACCESS_VIEW_DESC uavd;
	D3D11_SHADER_RESOURCE_VIEW_DESC srvd;

	// video memory (4MB)

	memset(&bd, 0, sizeof(bd));

	bd.ByteWidth = 4 * 1024 * 1024;
	bd.StructureByteStride = 4;
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

	// vertex buffer

	memset(&bd, 0, sizeof(bd));

	bd.ByteWidth = sizeof(GSVertex) * 10000;
	bd.StructureByteStride = sizeof(GSVertex);
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bd.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	bd.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

	hr = (*dev)->CreateBuffer(&bd, NULL, &m_vb);

	if(FAILED(hr)) return false;

	// index buffer

	memset(&bd, 0, sizeof(bd));

	bd.ByteWidth = sizeof(uint32) * 10000 * 3;
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bd.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	hr = (*dev)->CreateBuffer(&bd, NULL, &m_ib);

	if(FAILED(hr)) return false;

	// one page, for copying between cpu<->gpu

	memset(&bd, 0, sizeof(bd));

	bd.ByteWidth = PAGE_SIZE;
	bd.Usage = D3D11_USAGE_STAGING;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;

	hr = (*dev)->CreateBuffer(&bd, NULL, &m_pb);

	if(FAILED(hr)) return false;

	return true;
}

GSTexture* GSRendererCS::GetOutput(int i)
{
	// TODO: create a compute shader which unswizzles the frame from m_vm to the output texture

	return NULL;
}

template<uint32 prim, uint32 tme, uint32 fst> 
void GSRendererCS::ConvertVertex(size_t dst_index, size_t src_index)
{
	// TODO: vertex format more fitting as the input for the compute shader

	if(src_index != dst_index)
	{
		GSVertex v = ((GSVertex*)m_vertex.buff)[src_index];

		((GSVertex*)m_vertex.buff)[dst_index] = v;
	}
}

void GSRendererCS::Draw()
{
	HRESULT hr;

	GSDevice11* dev = (GSDevice11*)m_dev;
	
	ID3D11DeviceContext* ctx = *dev;

	D3D11_BUFFER_DESC bd;
	D3D11_UNORDERED_ACCESS_VIEW_DESC uavd;
	D3D11_SHADER_RESOURCE_VIEW_DESC srvd;
	D3D11_MAPPED_SUBRESOURCE map;

	CComPtr<ID3D11ShaderResourceView> vb_srv;
	CComPtr<ID3D11ShaderResourceView> ib_srv;

	// TODO: cache these in hash_maps

	CComPtr<ID3D11Buffer> fbr, fbc, zbr, zbc;
	CComPtr<ID3D11ShaderResourceView> fbr_srv, fbc_srv, zbr_srv, zbc_srv;

	// TODO: grow m_vb, m_ib if needed

	if(m_vertex.next > 10000) return;
	if(m_index.tail > 30000) return;

	// TODO: fill/advance/discardwhenfull, as in GSDevice11::IASetVertexBuffer/IASetIndexBuffer 

	hr = ctx->Map(m_vb, 0, D3D11_MAP_WRITE_DISCARD, 0, &map); // discarding, until properly advancing the start pointer around

	if(FAILED(hr)) return;

	memcpy(map.pData, m_vertex.buff, sizeof(GSVertex) * m_vertex.next);

	ctx->Unmap(m_vb, 0);

	//

	hr = ctx->Map(m_ib, 0, D3D11_MAP_WRITE_DISCARD, 0, &map); // discarding, until properly advancing the start pointer around

	if(FAILED(hr)) return;

	memcpy(map.pData, m_index.buff, sizeof(uint32) * m_index.tail);

	ctx->Unmap(m_ib, 0);

	// TODO: UpdateResource might be faster, based on my exprience with the real vertex buffer, write-no-overwrite/discarded dynamic buffer + map is better

	//

	memset(&srvd, 0, sizeof(srvd));

	srvd.Format = DXGI_FORMAT_UNKNOWN;
	srvd.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	srvd.Buffer.FirstElement = 0;
	srvd.Buffer.NumElements = m_vertex.next;

	hr = (*dev)->CreateShaderResourceView(m_vb, &srvd, &vb_srv); // TODO: have to create this dyncamically in Draw() or pass the start/count in a const reg

	memset(&srvd, 0, sizeof(srvd));

	srvd.Format = DXGI_FORMAT_R32_UINT;
	srvd.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	srvd.Buffer.FirstElement = 0;
	srvd.Buffer.NumElements = m_index.tail;

	hr = (*dev)->CreateShaderResourceView(m_ib, &srvd, &ib_srv); // TODO: have to create this dyncamically in Draw() or pass the start/count in a const reg

	// fzb offsets

	memset(&bd, 0, sizeof(bd));

	bd.ByteWidth = sizeof(int) * 4096;
	bd.StructureByteStride = sizeof(int);
	bd.Usage = D3D11_USAGE_IMMUTABLE;
	bd.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	D3D11_SUBRESOURCE_DATA data;

	memset(&data, 0, sizeof(data));

	data.pSysMem = m_context->offset.fb->pixel.row;

	hr = (*dev)->CreateBuffer(&bd, &data, &fbr);

	data.pSysMem = m_context->offset.fb->pixel.col[0]; // same column layout for every line in case of frame and zbuffer formats
	
	hr = (*dev)->CreateBuffer(&bd, &data, &fbc);

	data.pSysMem = m_context->offset.zb->pixel.row;
	
	hr = (*dev)->CreateBuffer(&bd, &data, &zbr);

	data.pSysMem = m_context->offset.zb->pixel.col[0]; // same column layout for every line in case of frame and zbuffer formats
	
	hr = (*dev)->CreateBuffer(&bd, &data, &zbc);

	// TODO: D3D10_SHADER_MACRO (primclass, less frequently changing drawing attribs, etc.)

	uint32 sel = 0; // TODO

	hash_map<uint32, CComPtr<ID3D11ComputeShader> >::iterator i = m_cs.find(sel);

	CComPtr<ID3D11ComputeShader> cs;

	if(i == m_cs.end())
	{
		// hr = dev->CompileShader(IDR_CS_FX, "cs_main", NULL, &cs);
		hr = dev->CompileShader("E:\\Progs\\pcsx2\\plugins\\GSdx\\res\\cs.fx", "cs_main", NULL, &cs);

		if(FAILED(hr)) return;

		m_cs[sel] = cs;
	}
	else
	{
		cs = i->second;
	}
	
	//

	dev->CSSetShaderUAV(0, m_vm_uav);
	
	dev->CSSetShaderSRV(0, vb_srv);
	dev->CSSetShaderSRV(1, ib_srv);
	dev->CSSetShaderSRV(2, fbr_srv);
	dev->CSSetShaderSRV(3, fbc_srv);
	dev->CSSetShaderSRV(4, zbr_srv);
	dev->CSSetShaderSRV(5, zbc_srv);
	
	dev->CSSetShader(cs);

	GSVector4i bbox = GSVector4i(0, 0, 640, 512); // TODO: vertex trace

	GSVector4i r = bbox.ralign<Align_Outside>(GSVector2i(16, 8));

	bool fb = true; // TODO: frame buffer used
	bool zb = true; // TODO: z-buffer used

	if(fb) Write(m_context->offset.fb, r);
	if(zb) Write(m_context->offset.zb, r);

	// TODO: constant buffer (frequently chaning drawing attribs)
	// TODO: texture (implement texture cache)
	// TODO: clut to a palette texture (should be texture1d, not simply buffer, it is random accessed)
	// TODO: CSSetShaderSRV(6 7 8 ..., texture level 0 1 2 ...) or use Texture3D?
	// TODO: invalidate texture cache

	/*
	CComPtr<ID3D11Query> q;

	D3D11_QUERY_DESC qd;
	memset(&qd, 0, sizeof(qd));
	qd.Query = D3D11_QUERY_EVENT;

	hr = (*dev)->CreateQuery(&qd, &q);

	ctx->Begin(q);
	*/
	
	printf("[%lld] dispatch %05x %d %05x %d %05x %d %dx%d | %d %d %d\n",
		__rdtsc(),
		m_context->FRAME.Block(), m_context->FRAME.PSM,
		m_context->ZBUF.Block(), m_context->ZBUF.PSM,
		PRIM->TME ? m_context->TEX0.TBP0 : 0xfffff, m_context->TEX0.PSM, (int)m_context->TEX0.TW, (int)m_context->TEX0.TH, 
		PRIM->PRIM, m_vertex.next, m_index.tail);

	GSVector4i rsize = r.rsize();

	dev->Dispatch(rsize.z >> 4, rsize.w >> 3, 1); // TODO: pass upper-left corner offset (r.xy) in a const buffer

	/*
	ctx->End(q);

	uint64 t0 = __rdtsc();

	BOOL b;

	while(S_OK != ctx->GetData(q, &b, sizeof(BOOL), 0)) {}

	printf("%lld\n", __rdtsc() - t0);
	*/
}

void GSRendererCS::InvalidateVideoMem(const GIFRegBITBLTBUF& BITBLTBUF, const GSVector4i& r)
{
	GSOffset* o = m_mem.GetOffset(BITBLTBUF.DBP, BITBLTBUF.DBW, BITBLTBUF.DPSM);

	Read(o, r, true); // TODO: fully overwritten pages are not needed to be read, only invalidated

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
			box.right = box.left + PAGE_SIZE;

			ctx->UpdateSubresource(m_vm, 0, &box, m_mem.m_vm8 + box.left, 0, 0);

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

	uint32* pages = o->GetPages(r);

	for(size_t i = 0; pages[i] != GSOffset::EOP; i++)
	{
		uint32 page = pages[i];

		uint32 row = page >> 5;
		uint32 col = 1 << (page & 31);

		if(m_vm_valid[row] & col)
		{
			if(invalidate) m_vm_valid[row] ^= col;

			box.left = page * PAGE_SIZE;
			box.right = box.left + PAGE_SIZE;

			ctx->CopySubresourceRegion(m_pb, 0, 0, 0, 0, m_vm, 0, &box);

			D3D11_MAPPED_SUBRESOURCE map;

			if(SUCCEEDED(ctx->Map(m_pb, 0, D3D11_MAP_READ_WRITE, 0, &map)))
			{
				memcpy(m_mem.m_vm8 + box.left, map.pData, PAGE_SIZE);

				ctx->Unmap(m_pb, 0);

				printf("[%lld] read %05x %d %d (%d)\n", __rdtsc(), o->bp, o->bw, o->psm, page);
			}
		}
	}

	delete [] pages;
}
