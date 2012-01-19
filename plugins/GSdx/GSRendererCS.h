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
	struct VSSelector
	{
		union
		{
			struct
			{
				uint32 tme:1;
				uint32 fst:1;
			};

			uint32 key;
		};

		operator uint32() {return key & 0x3;}

		VSSelector() : key(0) {}
	};

	__aligned(struct, 32) VSConstantBuffer
	{
		GSVector4 VertexScale;
		GSVector4 VertexOffset;
	};

	struct GSSelector
	{
		union
		{
			struct
			{
				uint32 iip:1;
				uint32 prim:2;
			};

			uint32 key;
		};

		operator uint32() {return key & 0x7;}

		GSSelector() : key(0) {}
	};

	struct PSSelector
	{
		union
		{
			struct
			{
				uint32 TODO:1;
			};

			uint32 key;
		};

		operator uint32() {return key & 0x1;}

		PSSelector() : key(0) {}
	};

	__aligned(struct, 32) PSConstantBuffer
	{
		GSVector4 TODO;
	};

	CComPtr<ID3D11DepthStencilState> m_dss;
	CComPtr<ID3D11BlendState> m_bs;
	CComPtr<ID3D11SamplerState> m_ss;
	CComPtr<ID3D11Buffer> m_vm;
	//CComPtr<ID3D11Texture2D> m_vm;
	CComPtr<ID3D11UnorderedAccessView> m_vm_uav;
	uint32 m_vm_valid[16];
	CComPtr<ID3D11Buffer> m_pb;
	//CComPtr<ID3D11Texture2D> m_pb;
	hash_map<uint32, GSVertexShader11 > m_vs;
	CComPtr<ID3D11Buffer> m_vs_cb;
	hash_map<uint32, CComPtr<ID3D11GeometryShader> > m_gs;
	hash_map<uint32, CComPtr<ID3D11PixelShader> > m_ps;
	CComPtr<ID3D11Buffer> m_ps_cb;
	CComPtr<ID3D11Buffer> m_sob;

	void Write(GSOffset* o, const GSVector4i& r);
	void Read(GSOffset* o, const GSVector4i& r, bool invalidate);

	struct OffsetBuffer
	{
		CComPtr<ID3D11Buffer> row, col;
		CComPtr<ID3D11ShaderResourceView> row_view, col_view;
	};

	hash_map<uint32, OffsetBuffer> m_offset;

	bool GetOffsetBuffer(OffsetBuffer** fzbo);

protected:
	bool CreateDevice(GSDevice* dev);
	void VSync(int field);
	GSTexture* GetOutput(int i);
	void Draw();
	void InvalidateVideoMem(const GIFRegBITBLTBUF& BITBLTBUF, const GSVector4i& r);
	void InvalidateLocalMem(const GIFRegBITBLTBUF& BITBLTBUF, const GSVector4i& r, bool clut);

public:
	GSRendererCS();
	virtual ~GSRendererCS();
};
