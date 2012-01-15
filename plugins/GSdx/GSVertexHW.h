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

#include "GS.h"
#include "GSVector.h"

#pragma pack(push, 1)

__aligned(struct, 32) GSVertexHW9
{
	GSVector4 t; 
	GSVector4 p;

	// t.z = union {struct {uint8 r, g, b, a;}; uint32 c0;};
	// t.w = union {struct {uint8 ta0, ta1, res, f;}; uint32 c1;}

	GSVertexHW9& operator = (GSVertexHW9& v) {t = v.t; p = v.p; return *this;}
};

__aligned(union, 32) GSVertexHW11
{
	struct
	{
		union
		{
			struct {float x, y;} t;
			GIFRegST ST;
		};

		union
		{
			union {struct {uint8 r, g, b, a; float q;}; uint32 c0;};
			GIFRegRGBAQ RGBAQ;
		};

		union
		{
			struct {union {struct {uint16 x, y;}; uint32 xy;}; uint32 z;} p;
			GIFRegXYZ XYZ;
		};

		union
		{
			struct {uint32 _pad; union {struct {uint8 ta0, ta1, res, f;}; uint32 c1;};};
			GIFRegFOG FOG;
		};
	};

	GSVertexHW11& operator = (GSVertexHW11& v) 
	{
		GSVector4i* RESTRICT src = (GSVector4i*)&v;
		GSVector4i* RESTRICT dst = (GSVector4i*)this;

		dst[0] = src[0];
		dst[1] = src[1];
		
		return *this;
	}
};

#pragma pack(pop)
