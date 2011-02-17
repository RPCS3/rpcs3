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

#pragma pack(push, 1)

__aligned32 struct GPUVertex
{
	union
	{
		struct
		{
			GPURegRGB RGB;
			GPURegXY XY;
			GPURegXY UV;
		};

		struct {__m128i m128i;};
		struct {__m128 m128;};
	};

	GPUVertex() {memset(this, 0, sizeof(*this));}
};

struct GPUVertexNull 
{
};

#pragma pack(pop)
