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

// dumb device implementation, only good for simple image output

#include "stdafx.h"
#include "GSDevice7.h"

GSDevice7::GSDevice7()
{
}

GSDevice7::~GSDevice7()
{
}

bool GSDevice7::Create(HWND hWnd, bool vsync)
{
	if(!__super::Create(hWnd, vsync))
	{
		return false;
	}

	if(FAILED(DirectDrawCreateEx(NULL, (VOID**)&m_dd, IID_IDirectDraw7, NULL)))
	{
		return false;
	}

	// TODO: fullscreen

	if(FAILED(m_dd->SetCooperativeLevel(hWnd, DDSCL_NORMAL)))
	{
		return false;
	}

    DDSURFACEDESC2 desc;

	memset(&desc, 0, sizeof(desc));

	desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS;
    desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
    
	if(FAILED(m_dd->CreateSurface(&desc, &m_primary, NULL)))
	{
		return false;
	}

	CComPtr<IDirectDrawClipper> clipper;

    if(FAILED(m_dd->CreateClipper(0, &clipper, NULL))
	|| FAILED(clipper->SetHWnd(0, hWnd))
	|| FAILED(m_primary->SetClipper(clipper)))
	{
		return false;
	}

	Reset(1, 1, true);

	return true;
}

bool GSDevice7::Reset(int w, int h, bool fs)
{
	if(!__super::Reset(w, h, fs))
		return false;

	m_backbuffer = NULL;

    DDSURFACEDESC2 desc;

	memset(&desc, 0, sizeof(desc));

	desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    desc.ddsCaps.dwCaps = DDSCAPS_VIDEOMEMORY | DDSCAPS_3DDEVICE;
	desc.dwWidth = w;
	desc.dwHeight = h;

	if(FAILED(m_dd->CreateSurface(&desc, &m_backbuffer, NULL)))
	{
		return false;
	}

	CComPtr<IDirectDrawClipper> clipper;

    if(FAILED(m_dd->CreateClipper(0, &clipper, NULL)))
	{
		return false;
	}

	{
		// ???

		HRGN hrgn = CreateRectRgn(0, 0, w, h);

		uint8 buff[1024];

		GetRegionData(hrgn, sizeof(buff), (RGNDATA*)buff);
		
		DeleteObject(hrgn);

		clipper->SetClipList((RGNDATA*)buff, 0);

		if(FAILED(m_backbuffer->SetClipper(clipper)))
		{
			return false;
		}
	}

	return true;
}

void GSDevice7::Present(const GSVector4i& r)
{
	HRESULT hr;

	GSVector4i cr;

	GetClientRect(m_hWnd, cr);

    DDSURFACEDESC2 desc;
	memset(&desc, 0, sizeof(desc));
	desc.dwSize = sizeof(desc);

	hr = m_backbuffer->GetSurfaceDesc(&desc);

	if(desc.dwWidth != cr.width() || desc.dwHeight != cr.height())
	{
		Reset(cr.width(), cr.height(), false);
	}

	DDBLTFX fx;
	
	memset(&fx, 0, sizeof(fx));
	
	fx.dwSize = sizeof(fx);
	fx.dwFillColor = 0;

	hr = m_backbuffer->Blt(NULL, NULL, NULL, DDBLT_WAIT | DDBLT_COLORFILL, &fx);

	GSVector4i r2 = r;

	hr = m_backbuffer->Blt(r2, m_merge, NULL, DDBLT_WAIT, NULL);

	r2 = cr;

	MapWindowPoints(m_hWnd, HWND_DESKTOP, (POINT*)&r2, 2);
	
	if(m_vsync)
	{
		hr = m_dd->WaitForVerticalBlank(DDWAITVB_BLOCKBEGIN, NULL);
	}

	hr = m_primary->Blt(r2, m_backbuffer, cr, DDBLT_WAIT, NULL);

	if(hr == DDERR_SURFACELOST)
	{
		// TODO

		HRESULT hr = m_dd->TestCooperativeLevel();

		if(hr == DDERR_WRONGMODE) 
		{
		}
	}
}

bool GSDevice7::Create(int type, Texture& t, int w, int h, int format)
{
	HRESULT hr;

	t = Texture();

	DDSURFACEDESC2 desc;
	
	memset(&desc, 0, sizeof(desc));

	desc.dwSize = sizeof(desc);
	desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
	desc.ddsCaps.dwCaps = DDSCAPS_VIDEOMEMORY;
	desc.dwWidth = w;
	desc.dwHeight = h;
	desc.ddpfPixelFormat.dwSize = sizeof(desc.ddpfPixelFormat);
	desc.ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
	desc.ddpfPixelFormat.dwRGBBitCount = 32;
	desc.ddpfPixelFormat.dwRGBAlphaBitMask	= 0xff000000;
	desc.ddpfPixelFormat.dwRBitMask = 0x00ff0000;
	desc.ddpfPixelFormat.dwGBitMask = 0x0000ff00;
	desc.ddpfPixelFormat.dwBBitMask = 0x000000ff;

	CComPtr<IDirectDrawSurface7> system, video;

	switch(type)
	{
	case GSTexture::RenderTarget:
	case GSTexture::DepthStencil:
	case GSTexture::Texture:
		desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY;
		if(FAILED(hr = m_dd->CreateSurface(&desc, &system, NULL))) return false;
		desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_VIDEOMEMORY;
		if(FAILED(hr = m_dd->CreateSurface(&desc, &video, NULL))) return false;
		t = Texture(type, system, video);
		break;
	case GSTexture::Offscreen:
		desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY;
		if(FAILED(hr = m_dd->CreateSurface(&desc, &system, NULL))) return false;
		t = Texture(type, system);
		break;
	}

	return !!t;
}

void GSDevice7::DoMerge(Texture* st, GSVector4* sr, GSVector4* dr, Texture& dt, bool slbg, bool mmod, GSVector4& c)
{
	HRESULT hr;

	hr = dt->Blt(NULL, st[0], NULL, DDBLT_WAIT, NULL);
}

void GSDevice7::DoInterlace(Texture& st, Texture& dt, int shader, bool linear, float yoffset)
{
}
