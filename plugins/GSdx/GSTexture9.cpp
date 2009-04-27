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
#include "GSTexture9.h"

GSTexture9::GSTexture9()
{
	memset(&m_desc, 0, sizeof(m_desc));
}

GSTexture9::GSTexture9(IDirect3DSurface9* surface)
{
	m_surface = surface;

	surface->GetDevice(&m_dev);
	surface->GetDesc(&m_desc);
	
	if(m_desc.Type != D3DRTYPE_SURFACE)
	{
		HRESULT hr = surface->GetContainer(__uuidof(IDirect3DTexture9), (void**)&m_texture);
		ASSERT(SUCCEEDED(hr));
	}
}

GSTexture9::GSTexture9(IDirect3DTexture9* texture)
{
	m_texture = texture;

	texture->GetDevice(&m_dev);
	texture->GetLevelDesc(0, &m_desc);
	texture->GetSurfaceLevel(0, &m_surface);
}

GSTexture9::~GSTexture9()
{
}

GSTexture9::operator bool()
{
	return !!m_surface;
}

int GSTexture9::GetType() const
{
	if(m_desc.Usage & D3DUSAGE_RENDERTARGET) return GSTexture::RenderTarget;
	if(m_desc.Usage & D3DUSAGE_DEPTHSTENCIL) return GSTexture::DepthStencil;
	if(m_desc.Pool == D3DPOOL_MANAGED) return GSTexture::Texture;
	if(m_desc.Pool == D3DPOOL_SYSTEMMEM) return GSTexture::Offscreen;
	return GSTexture::None;
}

int GSTexture9::GetWidth() const 
{
	return m_desc.Width;
}

int GSTexture9::GetHeight() const 
{
	return m_desc.Height;
}

int GSTexture9::GetFormat() const 
{
	return m_desc.Format;
}

bool GSTexture9::Update(const CRect& r, const void* data, int pitch)
{
	if(CComPtr<IDirect3DSurface9> surface = *this)
	{
		D3DLOCKED_RECT lr;

		if(SUCCEEDED(surface->LockRect(&lr, r, 0)))
		{
			BYTE* src = (BYTE*)data;
			BYTE* dst = (BYTE*)lr.pBits;

			int bytes = min(pitch, lr.Pitch);

			for(int i = 0, j = r.Height(); i < j; i++, src += pitch, dst += lr.Pitch)
			{
				memcpy(dst, src, bytes);
			}

			surface->UnlockRect();

			return true;
		}
	}

	return false;
}

bool GSTexture9::Map(BYTE** bits, int& pitch, const RECT* r)
{
	HRESULT hr;

	if(CComPtr<IDirect3DSurface9> surface = *this)
	{
		D3DLOCKED_RECT lr;

		if(SUCCEEDED(hr = surface->LockRect(&lr, r, 0)))
		{
			*bits = (BYTE*)lr.pBits;
			pitch = (int)lr.Pitch;

			return true;
		}
	}

	return false;
}

void GSTexture9::Unmap()
{
	if(CComPtr<IDirect3DSurface9> surface = *this)
	{
		surface->UnlockRect();
	}
}

bool GSTexture9::Save(CString fn, bool dds)
{
	CComPtr<IDirect3DSurface9> surface;
	
	if(m_desc.Usage & D3DUSAGE_DEPTHSTENCIL)
	{
		HRESULT hr;

		D3DSURFACE_DESC desc;

		m_surface->GetDesc(&desc);

		if(desc.Format != D3DFMT_D32F_LOCKABLE)
			return false;

		hr = m_dev->CreateOffscreenPlainSurface(desc.Width, desc.Height, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &surface, NULL);

		D3DLOCKED_RECT slr, dlr;

		hr = m_surface->LockRect(&slr, NULL, 0);
		hr = surface->LockRect(&dlr, NULL, 0);

		BYTE* s = (BYTE*)slr.pBits;
		BYTE* d = (BYTE*)dlr.pBits;

		for(UINT y = 0; y < desc.Height; y++, s += slr.Pitch, d += dlr.Pitch)
		{
			for(UINT x = 0; x < desc.Width; x++)
			{
				((float*)d)[x] = ((float*)s)[x];
			}
		}

		m_surface->UnlockRect();
		surface->UnlockRect();
	}
	else
	{
		surface = m_surface;
	}

	if(surface != NULL)
	{
		return SUCCEEDED(D3DXSaveSurfaceToFile(fn, dds ? D3DXIFF_DDS : D3DXIFF_BMP, surface, NULL, NULL));
	}
/*
	if(CComQIPtr<IDirect3DTexture9> texture = surface)
	{
		return SUCCEEDED(D3DXSaveTextureToFile(fn, dds ? D3DXIFF_DDS : D3DXIFF_BMP, texture, NULL));
	}
*/
	return false;
}

IDirect3DTexture9* GSTexture9::operator->()
{
	return m_texture;
}

GSTexture9::operator IDirect3DSurface9*()
{
	if(m_texture && !m_surface)
	{
		m_texture->GetSurfaceLevel(0, &m_surface);
	}

	return m_surface;
}

GSTexture9::operator IDirect3DTexture9*()
{
	if(m_surface && !m_texture)
	{
		m_surface->GetContainer(__uuidof(IDirect3DTexture9), (void**)&m_texture);

		ASSERT(m_texture);
	}

	return m_texture;
}
