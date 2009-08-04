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
#include "GSTexture10.h"

GSTexture10::GSTexture10(ID3D10Texture2D* texture)
	: m_texture(texture)
{
	ASSERT(m_texture);

	m_texture->GetDevice(&m_dev);
	m_texture->GetDesc(&m_desc);

	m_size.x = (int)m_desc.Width;
	m_size.y = (int)m_desc.Height;

	if(m_desc.BindFlags & D3D10_BIND_RENDER_TARGET) m_type = RenderTarget;
	else if(m_desc.BindFlags & D3D10_BIND_DEPTH_STENCIL) m_type = DepthStencil;
	else if(m_desc.BindFlags & D3D10_BIND_SHADER_RESOURCE) m_type = Texture;
	else if(m_desc.Usage == D3D10_USAGE_STAGING) m_type = Offscreen;

	m_format = (int)m_desc.Format;

	m_msaa = m_desc.SampleDesc.Count > 1;
}

bool GSTexture10::Update(const GSVector4i& r, const void* data, int pitch)
{
	if(m_dev && m_texture)
	{
		D3D10_BOX box = {r.left, r.top, 0, r.right, r.bottom, 1};
		
		m_dev->UpdateSubresource(m_texture, 0, &box, data, pitch, 0);

		return true;
	}

	return false;
}

bool GSTexture10::Map(GSMap& m, const GSVector4i* r)
{
	if(r != NULL)
	{
		// ASSERT(0); // not implemented

		return false;
	}

	if(m_texture && m_desc.Usage == D3D10_USAGE_STAGING)
	{
		D3D10_MAPPED_TEXTURE2D map;

		if(SUCCEEDED(m_texture->Map(0, D3D10_MAP_READ_WRITE, 0, &map)))
		{
			m.bits = (uint8*)map.pData;
			m.pitch = (int)map.RowPitch;

			return true;
		}
	}

	return false;
}

void GSTexture10::Unmap()
{
	if(m_texture)
	{
		m_texture->Unmap(0);
	}
}

bool GSTexture10::Save(const string& fn, bool dds)
{
	CComPtr<ID3D10Resource> res;

	if(m_desc.BindFlags & D3D10_BIND_DEPTH_STENCIL)
	{
		HRESULT hr;

		D3D10_TEXTURE2D_DESC desc;

		memset(&desc, 0, sizeof(desc));

		m_texture->GetDesc(&desc);

		desc.Usage = D3D10_USAGE_STAGING;
		desc.BindFlags = 0;
		desc.CPUAccessFlags = D3D10_CPU_ACCESS_READ;

		CComPtr<ID3D10Texture2D> src, dst;

		hr = m_dev->CreateTexture2D(&desc, NULL, &src);

		m_dev->CopyResource(src, m_texture);

		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.CPUAccessFlags = D3D10_CPU_ACCESS_WRITE;

		hr = m_dev->CreateTexture2D(&desc, NULL, &dst);

		D3D10_MAPPED_TEXTURE2D sm, dm;

		hr = src->Map(0, D3D10_MAP_READ, 0, &sm);
		hr = dst->Map(0, D3D10_MAP_WRITE, 0, &dm);

		uint8* s = (uint8*)sm.pData;
		uint8* d = (uint8*)dm.pData;

		for(uint32 y = 0; y < desc.Height; y++, s += sm.RowPitch, d += dm.RowPitch)
		{
			for(uint32 x = 0; x < desc.Width; x++)
			{
				((uint32*)d)[x] = (uint32)(((float*)s)[x*2] * UINT_MAX);
			}
		}

		src->Unmap(0);
		dst->Unmap(0);

		res = dst;
	}
	else
	{
		res = m_texture;
	}

	return SUCCEEDED(D3DX10SaveTextureToFile(res, dds ? D3DX10_IFF_DDS : D3DX10_IFF_BMP, fn.c_str()));
}

GSTexture10::operator ID3D10Texture2D*()
{
	return m_texture;
}

GSTexture10::operator ID3D10ShaderResourceView*()
{
	if(!m_srv && m_dev && m_texture)
	{
		ASSERT(!m_msaa);

		D3D10_SHADER_RESOURCE_VIEW_DESC* desc = NULL;

		if(m_desc.Format == DXGI_FORMAT_R32G8X24_TYPELESS)
		{
			desc = new D3D10_SHADER_RESOURCE_VIEW_DESC();
			memset(desc, 0, sizeof(*desc));
			desc->Format = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
			desc->ViewDimension = D3D10_SRV_DIMENSION_TEXTURE2D;
			desc->Texture2D.MostDetailedMip = 0;
			desc->Texture2D.MipLevels = 1;
		}

		m_dev->CreateShaderResourceView(m_texture, desc, &m_srv);

		delete desc;
	}

	return m_srv;
}

GSTexture10::operator ID3D10RenderTargetView*()
{
	ASSERT(m_dev);

	if(!m_rtv && m_dev && m_texture)
	{
		m_dev->CreateRenderTargetView(m_texture, NULL, &m_rtv);
	}

	return m_rtv;
}

GSTexture10::operator ID3D10DepthStencilView*()
{
	if(!m_dsv && m_dev && m_texture)
	{
		D3D10_DEPTH_STENCIL_VIEW_DESC* desc = NULL;

		if(m_desc.Format == DXGI_FORMAT_R32G8X24_TYPELESS)
		{
			desc = new D3D10_DEPTH_STENCIL_VIEW_DESC();
			memset(desc, 0, sizeof(*desc));
			desc->Format = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
			desc->ViewDimension = D3D10_DSV_DIMENSION_TEXTURE2D;
		}

		m_dev->CreateDepthStencilView(m_texture, desc, &m_dsv);

		delete desc;
	}

	return m_dsv;
}
