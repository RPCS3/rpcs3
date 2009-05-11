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

#include "GPU.h"
#include "GSVector.h"

class GPULocalMemory
{
	static const GSVector4i m_xxxa;
	static const GSVector4i m_xxbx;
	static const GSVector4i m_xgxx;
	static const GSVector4i m_rxxx;

	WORD* m_vm; 

	struct 
	{
		WORD* buff;
		int tp, cx, cy;
		bool dirty;
	} m_clut;

	struct
	{
		BYTE* buff[3];
		void* page[3][2][16];
		WORD valid[3][2];
	} m_texture;

	CSize m_scale;

public:
	GPULocalMemory(const CSize& scale);
	virtual ~GPULocalMemory();

	CSize GetScale() {return m_scale;}
	int GetWidth() {return 1 << (10 + m_scale.cx);}
	int GetHeight() {return 1 << (9 + m_scale.cy);}

	WORD* GetPixelAddress(int x, int y) const {return &m_vm[(y << (10 + m_scale.cx)) + x];}
	WORD* GetPixelAddressScaled(int x, int y) const {return &m_vm[((y << m_scale.cy) << (10 + m_scale.cx)) + (x << m_scale.cx)];}

	const WORD* GetCLUT(int tp, int cx, int cy);
	const void* GetTexture(int tp, int tx, int ty);

	void Invalidate(const CRect& r);

	void FillRect(const CRect& r, WORD c);
	void WriteRect(const CRect& r, const WORD* RESTRICT src);
	void ReadRect(const CRect& r, WORD* RESTRICT dst);
	void MoveRect(const CPoint& src, const CPoint& dst, int w, int h);

	void ReadPage4(int tx, int ty, BYTE* RESTRICT dst);
	void ReadPage8(int tx, int ty, BYTE* RESTRICT dst);
	void ReadPage16(int tx, int ty, WORD* RESTRICT dst);

	void ReadFrame32(const CRect& r, DWORD* RESTRICT dst, bool rgb24);

	void Expand16(const WORD* RESTRICT src, DWORD* RESTRICT dst, int pixels);
	void Expand24(const WORD* RESTRICT src, DWORD* RESTRICT dst, int pixels);

	void SaveBMP(const string& path, CRect r, int tp, int cx, int cy);
};

#pragma warning(default: 4244)