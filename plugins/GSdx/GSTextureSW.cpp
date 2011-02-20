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
#include "GSTextureSW.h"

GSTextureSW::GSTextureSW(int type, int width, int height)
	: m_mapped(0)
{
	m_size = GSVector2i(width, height);
	m_type = type;
	m_format = 0;
	m_pitch = ((width << 2) + 31) & ~31;
	m_data = _aligned_malloc(m_pitch * height, 32);
}

GSTextureSW::~GSTextureSW()
{
	_aligned_free(m_data);
}

bool GSTextureSW::Update(const GSVector4i& r, const void* data, int pitch)
{
	GSMap m;

	if(m_data != NULL && Map(m, &r))
	{
		uint8* RESTRICT src = (uint8*)data;
		uint8* RESTRICT dst = m.bits;

		int rowbytes = r.width() << 2;

		for(int h = r.height(); h > 0; h--, src += pitch, dst += m.pitch)
		{
			memcpy(dst, src, rowbytes);
		}

		Unmap();

		return true;
	}

	return false;
}

bool GSTextureSW::Map(GSMap& m, const GSVector4i* r)
{
	HRESULT hr;

	if(m_data != NULL && r->left >= 0 && r->right <= m_size.x && r->top >= 0 && r->bottom <= m_size.y)
	{
		if(!_interlockedbittestandset(&m_mapped, 0))
		{
			m.bits = (uint8*)m_data + ((m_pitch * r->top + r->left) << 2);
			m.pitch = m_pitch;

			return true;
		}
	}

	return false;
}

void GSTextureSW::Unmap()
{
	m_mapped = 0;
}

#ifndef _WINDOWS

#pragma pack(push, 1)

struct BITMAPFILEHEADER
{
	uint16 bfType;
	uint32 bfSize;
	uint16 bfReserved1;
	uint16 bfReserved2;
	uint32 bfOffBits;
};

struct BITMAPINFOHEADER
{
	uint32 biSize;
	int32 biWidth;
	int32 biHeight;
	uint16 biPlanes;
	uint16 biBitCount;
	uint32 biCompression;
	uint32 biSizeImage;
	int32 biXPelsPerMeter;
	int32 biYPelsPerMeter;
	uint32 biClrUsed;
	uint32 biClrImportant;
};

#pragma pack(pop)

#endif

bool GSTextureSW::Save(const string& fn, bool dds)
{
	if(dds) return false; // not implemented

	if(FILE* fp = fopen(fn.c_str(), "wb"))
	{
		BITMAPINFOHEADER bih;

		memset(&bih, 0, sizeof(bih));

        bih.biSize = sizeof(bih);
        bih.biWidth = m_size.x;
        bih.biHeight = m_size.y;
        bih.biPlanes = 1;
        bih.biBitCount = 32;
        bih.biCompression = BI_RGB;
        bih.biSizeImage = m_size.x * m_size.y << 2;

		BITMAPFILEHEADER bfh;

		memset(&bfh, 0, sizeof(bfh));
		
		bfh.bfType = 'MB';
		bfh.bfOffBits = sizeof(bfh) + sizeof(bih);
		bfh.bfSize = bfh.bfOffBits + bih.biSizeImage;
		bfh.bfReserved1 = bfh.bfReserved2 = 0;

		fwrite(&bfh, 1, sizeof(bfh), fp);
		fwrite(&bih, 1, sizeof(bih), fp);

		uint8* data = (uint8*)m_data + (m_size.y - 1) * m_pitch;

		for(int h = m_size.y; h > 0; h--, data -= m_pitch)
		{
			fwrite(data, 1, m_size.x << 2, fp); // TODO: swap red-blue?
		}

		fclose(fp);

		return true;
	}

	return false;
}
