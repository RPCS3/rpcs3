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
 
#include "StdAfx.h"
#include "GPULocalMemory.h"

const GSVector4i GPULocalMemory::m_xxxa(0x00008000);
const GSVector4i GPULocalMemory::m_xxbx(0x00007c00);
const GSVector4i GPULocalMemory::m_xgxx(0x000003e0);
const GSVector4i GPULocalMemory::m_rxxx(0x0000001f);

GPULocalMemory::GPULocalMemory(const GSVector2i& scale)
{
	m_scale.x = min(max(scale.x, 0), 2);
	m_scale.y = min(max(scale.y, 0), 2);

	//

	int size = (1 << (12 + 11)) * sizeof(uint16);

	m_vm = (uint16*)VirtualAlloc(NULL, size * 2, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

	memset(m_vm, 0, size);

	//

	m_clut.buff = m_vm + size;
	m_clut.dirty = true;

	//

	size = 256 * 256 * (1 + 1 + 4) * 32;

	m_texture.buff[0] = (uint8*)VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	m_texture.buff[1] = m_texture.buff[0] + 256 * 256 * 32;
	m_texture.buff[2] = m_texture.buff[1] + 256 * 256 * 32;

	memset(m_texture.buff[0], 0, size);

	memset(m_texture.valid, 0, sizeof(m_texture.valid));

	for(int y = 0, offset = 0; y < 2; y++)
	{
		for(int x = 0; x < 16; x++, offset += 256 * 256)
		{
			m_texture.page[0][y][x] = &((uint8*)m_texture.buff[0])[offset];
			m_texture.page[1][y][x] = &((uint8*)m_texture.buff[1])[offset];
		}
	}

	for(int y = 0, offset = 0; y < 2; y++)
	{
		for(int x = 0; x < 16; x++, offset += 256 * 256)
		{
			m_texture.page[2][y][x] = &((uint32*)m_texture.buff[2])[offset];
		}
	}
}

GPULocalMemory::~GPULocalMemory()
{
	VirtualFree(m_vm, 0, MEM_RELEASE);

	VirtualFree(m_texture.buff[0], 0, MEM_RELEASE);
}

const uint16* GPULocalMemory::GetCLUT(int tp, int cx, int cy)
{
	if(m_clut.dirty || m_clut.tp != tp || m_clut.cx != cx || m_clut.cy != cy)
	{
		uint16* src = GetPixelAddressScaled(cx << 4, cy);
		uint16* dst = m_clut.buff;

		if(m_scale.x == 0)
		{
			memcpy(dst, src, (tp == 0 ? 16 : 256) * 2);
		}
		else if(m_scale.x == 1)
		{
			if(tp == 0)
			{
				for(int i = 0; i < 16; i++)
				{
					dst[i] = src[i * 2];
				}
			}
			else if(tp == 1)
			{
				for(int i = 0; i < 256; i++)
				{
					dst[i] = src[i * 2];
				}
			}
		}
		else if(m_scale.x == 2)
		{
			if(tp == 0)
			{
				for(int i = 0; i < 16; i++)
				{
					dst[i] = src[i * 4];
				}
			}
			else if(tp == 1)
			{
				for(int i = 0; i < 256; i++)
				{
					dst[i] = src[i * 4];
				}
			}
		}
		else 
		{
			ASSERT(0);
		}
		
		m_clut.tp = tp;
		m_clut.cx = cx;
		m_clut.cy = cy;
		m_clut.dirty = false;
	}

	return m_clut.buff;
}

const void* GPULocalMemory::GetTexture(int tp, int tx, int ty)
{
	if(tp == 3)
	{
		ASSERT(0);

		return NULL;
	}

	void* buff = m_texture.page[tp][ty][tx];

	uint32 flag = 1 << tx;

	if((m_texture.valid[tp][ty] & flag) == 0)
	{
		int bpp = 0;

		switch(tp)
		{
		case 0: 
			ReadPage4(tx, ty, (uint8*)buff); 
			bpp = 4;
			break;
		case 1: 
			ReadPage8(tx, ty, (uint8*)buff);
			bpp = 8;
			break;
		case 2: 
		case 3: 
			ReadPage16(tx, ty, (uint16*)buff);
			bpp = 16;
		default:
			// FIXME: __assume(0); // vc9 generates bogus code in release mode
			break;
		}

		// TODO: m_state->m_perfmon.Put(GSPerfMon::Unswizzle, 256 * 256 * bpp >> 3);

		m_texture.valid[tp][ty] |= flag;
	}

	return buff;
}

void GPULocalMemory::Invalidate(const GSVector4i& r)
{
	if(!m_clut.dirty)
	{
		if(r.top <= m_clut.cy && m_clut.cy < r.bottom)
		{
			int left = m_clut.cx << 4;
			int right = left + (m_clut.tp == 0 ? 16 : 256);

			if(r.left < right && r.right > left)
			{
				m_clut.dirty = true;
			}
		}
	}

	for(int y = 0, ye = min(r.bottom, 512), j = 0; y < ye; y += 256, j++)
	{
		if(r.top >= y + 256) continue;

		for(int x = 0, xe = min(r.right, 1024), i = 0; x < xe; x += 64, i++)
		{
			uint32 flag = 1 << i;

			if(r.left >= x + 256) continue;

			m_texture.valid[2][j] &= ~flag;

			if(r.left >= x + 128) continue;

			m_texture.valid[1][j] &= ~flag;

			if(r.left >= x + 64) continue;

			m_texture.valid[0][j] &= ~flag;
		}
	}
}

void GPULocalMemory::FillRect(const GSVector4i& r, uint16 c)
{
	Invalidate(r);

	uint16* RESTRICT dst = GetPixelAddressScaled(r.left, r.top);

	int w = r.width() << m_scale.x;
	int h = r.height() << m_scale.y;

	int pitch = GetWidth();

	for(int j = 0; j < h; j++, dst += pitch)
	{
		for(int i = 0; i < w; i++)
		{
			dst[i] = c;
		}
	}
}

void GPULocalMemory::WriteRect(const GSVector4i& r, const uint16* RESTRICT src)
{
	Invalidate(r);

	uint16* RESTRICT dst = GetPixelAddressScaled(r.left, r.top);

	int w = r.width();
	int h = r.height();

	int pitch = GetWidth();

	if(m_scale.x == 0)
	{
		for(int j = 0; j < h; j++, src += w)
		{
			for(int k = 1 << m_scale.y; k >= 1; k--, dst += pitch)
			{
				memcpy(dst, src, w * 2);
			}
		}
	}
	else if(m_scale.x == 1)
	{
		for(int j = 0; j < h; j++, src += w)
		{
			for(int k = 1 << m_scale.y; k >= 1; k--, dst += pitch)
			{
				for(int i = 0; i < w; i++)
				{
					dst[i * 2 + 0] = src[i];
					dst[i * 2 + 1] = src[i];
				}
			}
		}
	}
	else if(m_scale.x == 2)
	{
		for(int j = 0; j < h; j++, src += w)
		{
			for(int k = 1 << m_scale.y; k >= 1; k--, dst += pitch)
			{
				for(int i = 0; i < w; i++)
				{
					dst[i * 4 + 0] = src[i];
					dst[i * 4 + 1] = src[i];
					dst[i * 4 + 2] = src[i];
					dst[i * 4 + 3] = src[i];
				}
			}
		}
	}
	else
	{
		ASSERT(0);
	}
}

void GPULocalMemory::ReadRect(const GSVector4i& r, uint16* RESTRICT dst)
{
	uint16* RESTRICT src = GetPixelAddressScaled(r.left, r.top);

	int w = r.width();
	int h = r.height();

	int pitch = GetWidth() << m_scale.y;

	if(m_scale.x == 0)
	{
		for(int j = 0; j < h; j++, src += pitch, dst += w)
		{
			memcpy(dst, src, w * 2);
		}
	}
	else if(m_scale.x == 1)
	{
		for(int j = 0; j < h; j++, src += pitch, dst += w)
		{
			for(int i = 0; i < w; i++)
			{
				dst[i] = src[i * 2];
			}
		}
	}
	else if(m_scale.x == 2)
	{
		for(int j = 0; j < h; j++, src += pitch, dst += w)
		{
			for(int i = 0; i < w; i++)
			{
				dst[i] = src[i * 4];
			}
		}
	}
	else
	{
		ASSERT(0);
	}
}

void GPULocalMemory::MoveRect(int sx, int sy, int dx, int dy, int w, int h)
{
	Invalidate(GSVector4i(dx, dy, dx + w, dy + h));

	uint16* s = GetPixelAddressScaled(sx, sy);
	uint16* d = GetPixelAddressScaled(dx, dy);

	w <<= m_scale.x;
	h <<= m_scale.y;

	int pitch = GetWidth();

	for(int i = 0; i < h; i++, s += pitch, d += pitch)
	{
		memcpy(d, s, w * sizeof(uint16));
	}
}

void GPULocalMemory::ReadPage4(int tx, int ty, uint8* RESTRICT dst)
{
	uint16* src = GetPixelAddressScaled(tx << 6, ty << 8);

	int pitch = GetWidth() << m_scale.y;

	if(m_scale.x == 0)
	{
		for(int j = 0; j < 256; j++, src += pitch, dst += 256)
		{
			for(int i = 0; i < 64; i++)
			{
				dst[i * 4 + 0] = (src[i] >> 0) & 0xf;
				dst[i * 4 + 1] = (src[i] >> 4) & 0xf;
				dst[i * 4 + 2] = (src[i] >> 8) & 0xf;
				dst[i * 4 + 3] = (src[i] >> 12) & 0xf;
			}
		}
	}
	else if(m_scale.x == 1)
	{
		for(int j = 0; j < 256; j++, src += pitch, dst += 256)
		{
			for(int i = 0; i < 64; i++)
			{
				dst[i * 4 + 0] = (src[i * 2] >> 0) & 0xf;
				dst[i * 4 + 1] = (src[i * 2] >> 4) & 0xf;
				dst[i * 4 + 2] = (src[i * 2] >> 8) & 0xf;
				dst[i * 4 + 3] = (src[i * 2] >> 12) & 0xf;
			}
		}
	}
	else if(m_scale.x == 2)
	{
		for(int j = 0; j < 256; j++, src += pitch, dst += 256)
		{
			for(int i = 0; i < 64; i++)
			{
				dst[i * 4 + 0] = (src[i * 4] >> 0) & 0xf;
				dst[i * 4 + 1] = (src[i * 4] >> 4) & 0xf;
				dst[i * 4 + 2] = (src[i * 4] >> 8) & 0xf;
				dst[i * 4 + 3] = (src[i * 4] >> 12) & 0xf;
			}
		}
	}
	else
	{
		ASSERT(0);
	}
}

void GPULocalMemory::ReadPage8(int tx, int ty, uint8* RESTRICT dst)
{
	uint16* src = GetPixelAddressScaled(tx << 6, ty << 8);

	int pitch = GetWidth() << m_scale.y;

	if(m_scale.x == 0)
	{
		for(int j = 0; j < 256; j++, src += pitch, dst += 256)
		{
			memcpy(dst, src, 256);
		}
	}
	else if(m_scale.x == 1)
	{
		for(int j = 0; j < 256; j++, src += pitch, dst += 256)
		{
			for(int i = 0; i < 128; i++)
			{
				((uint16*)dst)[i] = src[i * 2];
			}
		}
	}
	else if(m_scale.x == 2)
	{
		for(int j = 0; j < 256; j++, src += pitch, dst += 256)
		{
			for(int i = 0; i < 128; i++)
			{
				((uint16*)dst)[i] = src[i * 4];
			}
		}
	}
	else
	{
		ASSERT(0);
	}
}

void GPULocalMemory::ReadPage16(int tx, int ty, uint16* RESTRICT dst)
{
	uint16* src = GetPixelAddressScaled(tx << 6, ty << 8);

	int pitch = GetWidth() << m_scale.y;

	if(m_scale.x == 0)
	{
		for(int j = 0; j < 256; j++, src += pitch, dst += 256)
		{
			memcpy(dst, src, 512);
		}
	}
	else if(m_scale.x == 1)
	{
		for(int j = 0; j < 256; j++, src += pitch, dst += 256)
		{
			for(int i = 0; i < 256; i++)
			{
				dst[i] = src[i * 2];
			}
		}
	}
	else if(m_scale.x == 2)
	{
		for(int j = 0; j < 256; j++, src += pitch, dst += 256)
		{
			for(int i = 0; i < 256; i++)
			{
				dst[i] = src[i * 4];
			}
		}
	}
	else
	{
		ASSERT(0);
	}
}

void GPULocalMemory::ReadFrame32(const GSVector4i& r, uint32* RESTRICT dst, bool rgb24)
{
	uint16* src = GetPixelAddress(r.left, r.top);

	int pitch = GetWidth();

	if(rgb24)
	{
		for(int i = r.top; i < r.bottom; i++, src += pitch, dst += pitch)
		{
			Expand24(src, dst, r.width());
		}
	}
	else
	{
		for(int i = r.top; i < r.bottom; i++, src += pitch, dst += pitch)
		{
			Expand16(src, dst, r.width());
		}
	}
}

void GPULocalMemory::Expand16(const uint16* RESTRICT src, uint32* RESTRICT dst, int pixels)
{
	GSVector4i rm = m_rxxx;
	GSVector4i gm = m_xgxx;
	GSVector4i bm = m_xxbx;
	GSVector4i am = m_xxxa;

	GSVector4i* s = (GSVector4i*)src;
	GSVector4i* d = (GSVector4i*)dst;

	for(int i = 0, j = pixels >> 3; i < j; i++)
	{
		GSVector4i c = s[i];

		GSVector4i l = c.upl16();
		GSVector4i h = c.uph16();

		d[i * 2 + 0] = ((l & rm) << 3) | ((l & gm) << 6) | ((l & bm) << 9) | ((l & am) << 16);
		d[i * 2 + 1] = ((h & rm) << 3) | ((h & gm) << 6) | ((h & bm) << 9) | ((h & am) << 16);
	}
}

void GPULocalMemory::Expand24(const uint16* RESTRICT src, uint32* RESTRICT dst, int pixels)
{
	uint8* s = (uint8*)src;

	if(m_scale.x == 0)
	{
		for(int i = 0; i < pixels; i += 2, s += 6)
		{
			dst[i + 0] = (s[2] << 16) | (s[1] << 8) | s[0];
			dst[i + 1] = (s[5] << 16) | (s[4] << 8) | s[3];
		}
	}
	else if(m_scale.x == 1)
	{
		for(int i = 0; i < pixels; i += 4, s += 12)
		{
			dst[i + 0] = dst[i + 1] = (s[4] << 16) | (s[1] << 8) | s[0];
			dst[i + 2] = dst[i + 3] = (s[9] << 16) | (s[8] << 8) | s[5];
		}
	}
	else if(m_scale.x == 2)
	{
		for(int i = 0; i < pixels; i += 8, s += 24)
		{
			dst[i + 0] = dst[i + 1] = dst[i + 2] = dst[i + 3] = (s[8] << 16) | (s[1] << 8) | s[0];
			dst[i + 4] = dst[i + 5] = dst[i + 6] = dst[i + 7] = (s[17] << 16) | (s[16] << 8) | s[9];
		}
	}
	else
	{
		ASSERT(0);
	}
}

void GPULocalMemory::SaveBMP(const string& path, const GSVector4i& r2, int tp, int cx, int cy)
{
	GSVector4i r;
	
	r.left = r2.left << m_scale.x;
	r.top = r2.top << m_scale.y;
	r.right = r2.right << m_scale.x;
	r.bottom = r2.bottom << m_scale.y;

	r.left &= ~1;
	r.right &= ~1;

	if(FILE* fp = fopen(path.c_str(), "wb"))
	{
		BITMAPINFOHEADER bih;
		memset(&bih, 0, sizeof(bih));
        bih.biSize = sizeof(bih);
		bih.biWidth = r.width();
		bih.biHeight = r.height();
        bih.biPlanes = 1;
        bih.biBitCount = 32;
        bih.biCompression = BI_RGB;
        bih.biSizeImage = bih.biWidth * bih.biHeight * 4;

		BITMAPFILEHEADER bfh;
		memset(&bfh, 0, sizeof(bfh));
		bfh.bfType = 'MB';
		bfh.bfOffBits = sizeof(bfh) + sizeof(bih);
		bfh.bfSize = bfh.bfOffBits + bih.biSizeImage;
		bfh.bfReserved1 = bfh.bfReserved2 = 0;

		fwrite(&bfh, 1, sizeof(bfh), fp);
		fwrite(&bih, 1, sizeof(bih), fp);

		int pitch = GetWidth();

		uint16* buff = (uint16*)_aligned_malloc(pitch * sizeof(WORD), 16);
		uint32* buff32 = (uint32*)_aligned_malloc(pitch * sizeof(uint32), 16);
		uint16* src = GetPixelAddress(r.left, r.bottom - 1);
		const uint16* clut = GetCLUT(tp, cx, cy);

		for(int j = r.bottom - 1; j >= r.top; j--, src -= pitch)
		{
			switch(tp)
			{
			case 0: // 4 bpp

				for(int i = 0, k = r.width() / 2; i < k; i++)
				{
					buff[i * 2 + 0] = clut[((uint8*)src)[i] & 0xf];
					buff[i * 2 + 1] = clut[((uint8*)src)[i] >> 4];
				}

				break;
				
			case 1: // 8 bpp

				for(int i = 0, k = r.width(); i < k; i++)
				{
					buff[i] = clut[((uint8*)src)[i]];
				}

				break;

			case 2: // 16 bpp;

				for(int i = 0, k = r.width(); i < k; i++)
				{
					buff[i] = src[i];
				}

				break;

			case 3: // 24 bpp

				// TODO

				break;
			}

			Expand16(buff, buff32, r.width());

			for(int i = 0, k = r.width(); i < k; i++)
			{
				buff32[i] = (buff32[i] & 0xff00ff00) | ((buff32[i] & 0x00ff0000) >> 16) | ((buff32[i] & 0x000000ff) << 16);
			}

			fwrite(buff32, 1, r.width() * 4, fp);
		}

		_aligned_free(buff);
		_aligned_free(buff32);

		fclose(fp);
	}
}
