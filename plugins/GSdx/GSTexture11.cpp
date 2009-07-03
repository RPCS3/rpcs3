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
#include "GSTexture11.h"

GSTexture11::GSTexture11(ID3D11Texture2D* texture)
	: m_texture(texture)
{
	ASSERT(m_texture);

	m_texture->GetDevice(&m_dev);
	m_texture->GetDesc(&m_desc);

	m_dev->GetImmediateContext(&m_ctx);
}

int GSTexture11::GetType() const
{
	if(m_desc.BindFlags & D3D11_BIND_RENDER_TARGET) return GSTexture::RenderTarget;
	if(m_desc.BindFlags & D3D11_BIND_DEPTH_STENCIL) return GSTexture::DepthStencil;
	if(m_desc.BindFlags & D3D11_BIND_SHADER_RESOURCE) return GSTexture::Texture;
	if(m_desc.Usage == D3D11_USAGE_STAGING) return GSTexture::Offscreen;
	return GSTexture::None;
}

int GSTexture11::GetWidth() const 
{
	return m_desc.Width;
}

int GSTexture11::GetHeight() const 
{
	return m_desc.Height;
}

int GSTexture11::GetFormat() const 
{
	return m_desc.Format;
}

bool GSTexture11::Update(const GSVector4i& r, const void* data, int pitch)
{
	if(m_dev && m_texture)
	{
		D3D11_BOX box = {r.left, r.top, 0, r.right, r.bottom, 1};
		
		m_ctx->UpdateSubresource(m_texture, 0, &box, data, pitch, 0);

		return true;
	}

	return false;
}

bool GSTexture11::Map(GSMap& m, const GSVector4i* r)
{
	if(r != NULL)
	{
		// ASSERT(0); // not implemented

		return false;
	}

	if(m_texture && m_desc.Usage == D3D11_USAGE_STAGING)
	{
		D3D11_MAPPED_SUBRESOURCE map;

		if(SUCCEEDED(m_ctx->Map(m_texture, 0, D3D11_MAP_READ_WRITE, 0, &map)))
		{
			m.bits = (uint8*)map.pData;
			m.pitch = (int)map.RowPitch;

			return true;
		}
	}

	return false;
}

void GSTexture11::Unmap()
{
	if(m_texture)
	{
		m_ctx->Unmap(m_texture, 0);
	}
}

bool GSTexture11::Save(const string& fn, bool dds)
{
	CComPtr<ID3D11Resource> res;

	if(m_desc.BindFlags & D3D11_BIND_DEPTH_STENCIL)
	{
		HRESULT hr;

		D3D11_TEXTURE2D_DESC desc;

		memset(&desc, 0, sizeof(desc));

		m_texture->GetDesc(&desc);

		desc.Usage = D3D11_USAGE_STAGING;
		desc.BindFlags = 0;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

		CComPtr<ID3D11Texture2D> src, dst;

		hr = m_dev->CreateTexture2D(&desc, NULL, &src);

		m_ctx->CopyResource(src, m_texture);

		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		hr = m_dev->CreateTexture2D(&desc, NULL, &dst);

		D3D11_MAPPED_SUBRESOURCE sm, dm;

		hr = m_ctx->Map(src, 0, D3D11_MAP_READ, 0, &sm);
		hr = m_ctx->Map(dst, 0, D3D11_MAP_WRITE, 0, &dm);

		uint8* s = (uint8*)sm.pData;
		uint8* d = (uint8*)dm.pData;

		for(uint32 y = 0; y < desc.Height; y++, s += sm.RowPitch, d += dm.RowPitch)
		{
			for(uint32 x = 0; x < desc.Width; x++)
			{
				((uint32*)d)[x] = (uint32)(((float*)s)[x*2] * UINT_MAX);
			}
		}

		m_ctx->Unmap(src, 0);
		m_ctx->Unmap(dst, 0);

		res = dst;
	}
	else
	{
		res = m_texture;
	}

	return SUCCEEDED(D3DX11SaveTextureToFile(m_ctx, res, dds ? D3DX11_IFF_DDS : D3DX11_IFF_BMP, fn.c_str()));
}

GSTexture11::operator ID3D11Texture2D*()
{
	return m_texture;
}

GSTexture11::operator ID3D11ShaderResourceView*()
{
	if(!m_srv && m_dev && m_texture)
	{
		m_dev->CreateShaderResourceView(m_texture, NULL, &m_srv);
	}

	return m_srv;
}

GSTexture11::operator ID3D11RenderTargetView*()
{
	ASSERT(m_dev);

	if(!m_rtv && m_dev && m_texture)
	{
		m_dev->CreateRenderTargetView(m_texture, NULL, &m_rtv);
	}

	return m_rtv;
}

GSTexture11::operator ID3D11DepthStencilView*()
{
	if(!m_dsv && m_dev && m_texture)
	{
		m_dev->CreateDepthStencilView(m_texture, NULL, &m_dsv);
	}

	return m_dsv;
}
