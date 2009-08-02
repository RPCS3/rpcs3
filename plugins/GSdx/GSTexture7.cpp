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
#include "GSTexture7.h"

GSTexture7::GSTexture7(int type, IDirectDrawSurface7* system)
	: m_system(system)
{
	memset(&m_desc, 0, sizeof(m_desc));

	m_desc.dwSize = sizeof(m_desc);

	system->GetSurfaceDesc(&m_desc);

	m_size.x = (int)m_desc.dwWidth;
	m_size.y = (int)m_desc.dwHeight;

	m_type = type;

	m_format = (int)m_desc.ddpfPixelFormat.dwFourCC;
}

GSTexture7::GSTexture7(int type, IDirectDrawSurface7* system, IDirectDrawSurface7* video)
	: m_system(system)
	, m_video(video)
{
	memset(&m_desc, 0, sizeof(m_desc));

	m_desc.dwSize = sizeof(m_desc);

	video->GetSurfaceDesc(&m_desc);

	m_size.x = (int)m_desc.dwWidth;
	m_size.y = (int)m_desc.dwHeight;

	m_type = type;

	m_format = (int)m_desc.ddpfPixelFormat.dwFourCC;
}

bool GSTexture7::Update(const GSVector4i& r, const void* data, int pitch)
{
	HRESULT hr;

	GSVector4i r2 = r;

	DDSURFACEDESC2 desc;

	memset(&desc, 0, sizeof(desc));

	desc.dwSize = sizeof(desc);

	if(SUCCEEDED(hr = m_system->Lock(r2, &desc, DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR | DDLOCK_WRITEONLY, NULL)))
	{
		uint8* src = (uint8*)data;
		uint8* dst = (uint8*)desc.lpSurface;

		int bytes = std::min<int>(pitch, desc.lPitch);

		for(int i = 0, j = r.height(); i < j; i++, src += pitch, dst += desc.lPitch)
		{
			// memcpy(dst, src, bytes);

			// HACK!!!

			GSVector4i* s = (GSVector4i*)src;
			GSVector4i* d = (GSVector4i*)dst;

			int w = bytes >> 4;

			for(int x = 0; x < w; x++)
			{
				GSVector4i c = s[x];

				c = (c & 0xff00ff00) | ((c & 0x00ff0000) >> 16) | ((c & 0x000000ff) << 16);

				d[x] = c;
			}
		}

		hr = m_system->Unlock(r2);

		if(m_video)
		{
			hr = m_video->Blt(r2, m_system, r2, DDBLT_WAIT, NULL);
		}

		return true;
	}

	return false;
}

bool GSTexture7::Map(GSMap& m, const GSVector4i* r)
{
	HRESULT hr;

	if(r != NULL)
	{
		// ASSERT(0); // not implemented

		return false;
	}

	DDSURFACEDESC2 desc;

	if(SUCCEEDED(hr = m_system->Lock(NULL, &desc, DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR, NULL)))
	{
		m.bits = (uint8*)desc.lpSurface;
		m.pitch = (int)desc.lPitch;

		return true;
	}

	return false;
}

void GSTexture7::Unmap()
{
	HRESULT hr;

	hr = m_system->Unlock(NULL);

	if(m_video)
	{
		hr = m_video->Blt(NULL, m_system, NULL, DDBLT_WAIT, NULL);
	}
}

bool GSTexture7::Save(const string& fn, bool dds)
{
	// TODO

	return false;
}

IDirectDrawSurface7* GSTexture7::operator->()
{
	return m_video ? m_video : m_system;
}

GSTexture7::operator IDirectDrawSurface7*()
{
	return m_video ? m_video : m_system;
}
