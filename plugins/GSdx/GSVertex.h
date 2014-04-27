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

#include "GS.h"
#include "GSVector.h"
#include "GSVertexHW.h"
#include "GSVertexSW.h"

#pragma pack(push, 1)

__aligned(struct, 32) GSVertex
{
	union
	{
		struct
		{
			GIFRegST ST;
			GIFRegRGBAQ RGBAQ;
			GIFRegXYZ XYZ;
			union {uint32 UV; struct {uint16 U, V;};};
			uint32 FOG;
		};

		__m128i m[2];
	};

	void operator = (const GSVertex& v) {m[0] = v.m[0]; m[1] = v.m[1];}
};

struct GSVertexP
{
	GSVector4 p;
};

// Align 16 because the structure only contains 16B. Otherwise
// sizeof(GSVertexPxyT1) == 32 which defeat the purpose to save bandwidth vs
// GSVertexPT1
__aligned(struct, 16) GSVertexPxyT1
{
	GSVector2 p;
	GSVector2 t;
};

__aligned(struct, 32) GSVertexPT1
{
	GSVector4 p;
	GSVector2 t;
};

struct GSVertexPT2
{
	GSVector4 p;
	GSVector2 t[2];
};

#pragma pack(pop)
