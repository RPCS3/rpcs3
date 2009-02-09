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

#pragma pack(push, 1)

__declspec(align(16)) class GPUDrawingEnvironment
{
public:
	GPURegSTATUS STATUS;
	GPURegPRIM PRIM;
	GPURegDAREA DAREA;
	GPURegDHRANGE DHRANGE;
	GPURegDVRANGE DVRANGE;
	GPURegDRAREA DRAREATL;
	GPURegDRAREA DRAREABR;
	GPURegDROFF DROFF;
	GPURegTWIN TWIN;
	GPURegCLUT CLUT;

	GPUDrawingEnvironment()
	{
		Reset();
	}

	void Reset()
	{
		memset(this, 0, sizeof(*this));

		STATUS.IDLE = 1;
		STATUS.COM = 1;
		STATUS.WIDTH0 = 1;
		DVRANGE.Y1 = 16;
		DVRANGE.Y2 = 256;
	}

	CRect GetDisplayRect()
	{
		static int s_width[] = {256, 320, 512, 640, 368, 384, 512, 640};
		static int s_height[] = {240, 480};

		CRect r;

		r.left = DAREA.X & ~7; // FIXME
		r.top = DAREA.Y;
		r.right = r.left + s_width[(STATUS.WIDTH1 << 2) | STATUS.WIDTH0];
		r.bottom = r.top + (DVRANGE.Y2 - DVRANGE.Y1) * s_height[STATUS.HEIGHT] / 240;

		r &= CRect(0, 0, 1024, 512);

		return r;
	}

	int GetFPS()
	{
		return STATUS.ISPAL ? 50 : 60;
	}
};

#pragma pack(pop)
