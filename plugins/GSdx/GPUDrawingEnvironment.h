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

__aligned(class, 32) GPUDrawingEnvironment
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

	GSVector4i GetDisplayRect()
	{
		static int s_width[] = {256, 320, 512, 640, 368, 384, 512, 640};
		static int s_height[] = {240, 480};

		GSVector4i r;

		r.left = DAREA.X & ~7; // FIXME
		r.top = DAREA.Y;
		r.right = r.left + s_width[(STATUS.WIDTH1 << 2) | STATUS.WIDTH0];
		r.bottom = r.top + (DVRANGE.Y2 - DVRANGE.Y1) * s_height[STATUS.HEIGHT] / 240;

		return r.rintersect(GSVector4i(0, 0, 1024, 512));
	}

	float GetFPS()
	{
		return STATUS.ISPAL ? 50.0f : 59.94f;
	}
};