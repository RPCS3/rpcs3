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
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once

#include "GPU.h"
#include "GSVector.h"

class GPULocalMemory
{
	static const GSVector4i m_xxxa;
	static const GSVector4i m_xxbx;
	static const GSVector4i m_xgxx;
	static const GSVector4i m_rxxx;

	uint16* m_vm; 

	struct 
	{
		uint16* buff;
		int tp, cx, cy;
		bool dirty;
	} m_clut;

	struct
	{
		uint8* buff[3];
		void* page[3][2][16];
		uint16 valid[3][2];
	} m_texture;

	GSVector2i m_scale;

public:
	GPULocalMemory();
	virtual ~GPULocalMemory();

	GSVector2i GetScale() {return m_scale;}

	int GetWidth() {return 1 << (10 + m_scale.x);}
	int GetHeight() {return 1 << (9 + m_scale.y);}

	uint16* GetPixelAddress(int x, int y) const {return &m_vm[(y << (10 + m_scale.x)) + x];}
	uint16* GetPixelAddressScaled(int x, int y) const {return &m_vm[((y << m_scale.y) << (10 + m_scale.x)) + (x << m_scale.x)];}

	const uint16* GetCLUT(int tp, int cx, int cy);
	const void* GetTexture(int tp, int tx, int ty);

	void Invalidate(const GSVector4i& r);

	void FillRect(const GSVector4i& r, uint16 c);
	void WriteRect(const GSVector4i& r, const uint16* RESTRICT src);
	void ReadRect(const GSVector4i& r, uint16* RESTRICT dst);
	void MoveRect(int sx, int sy, int dx, int dy, int w, int h);

	void ReadPage4(int tx, int ty, uint8* RESTRICT dst);
	void ReadPage8(int tx, int ty, uint8* RESTRICT dst);
	void ReadPage16(int tx, int ty, uint16* RESTRICT dst);

	void ReadFrame32(const GSVector4i& r, uint32* RESTRICT dst, bool rgb24);

	void Expand16(const uint16* RESTRICT src, uint32* RESTRICT dst, int pixels);
	void Expand24(const uint16* RESTRICT src, uint32* RESTRICT dst, int pixels);

	void SaveBMP(const string& fn, const GSVector4i& r, int tp, int cx, int cy);
};
