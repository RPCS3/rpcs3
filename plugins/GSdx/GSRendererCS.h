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

#include "GSRenderer.h"
#include "GSDevice11.h"

class GSRendererCS : public GSRenderer
{
	class GSVertexTraceCS : public GSVertexTrace
	{
	public:
		GSVertexTraceCS(const GSState* state) : GSVertexTrace(state) {}
	};

	CComPtr<ID3D11Buffer> m_vm;
	CComPtr<ID3D11UnorderedAccessView> m_vm_uav;
	CComPtr<ID3D11Buffer> m_vb;
	CComPtr<ID3D11Buffer> m_ib;
	CComPtr<ID3D11Buffer> m_pb;
	hash_map<uint32, CComPtr<ID3D11ComputeShader> > m_cs;
	uint32 m_vm_valid[16];

	void Write(GSOffset* o, const GSVector4i& r);
	void Read(GSOffset* o, const GSVector4i& r, bool invalidate);
	
protected:
	template<uint32 prim, uint32 tme, uint32 fst> 
	void ConvertVertex(size_t dst_index, size_t src_index);

	bool CreateDevice(GSDevice* dev);
	GSTexture* GetOutput(int i);
	void Draw();
	void InvalidateVideoMem(const GIFRegBITBLTBUF& BITBLTBUF, const GSVector4i& r);
	void InvalidateLocalMem(const GIFRegBITBLTBUF& BITBLTBUF, const GSVector4i& r, bool clut);

public:
	GSRendererCS();
	virtual ~GSRendererCS();
};
