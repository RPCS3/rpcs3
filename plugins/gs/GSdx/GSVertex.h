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
#include "GSVertexHW.h"
#include "GSVertexSW.h"

#pragma pack(push, 1)

__declspec(align(16)) struct GSVertex
{
	union
	{
		struct
		{
			GIFRegST ST;
			GIFRegXYZ XYZ;
			GIFRegRGBAQ RGBAQ;
			GIFRegFOG FOG;
		};

		struct {__m128i m128i[2];};
		struct {__m128 m128[2];};
	};

	GIFRegUV UV;

	GSVertex() {memset(this, 0, sizeof(*this));}

	GSVector4 GetUV() const {return GSVector4(GSVector4i::load(UV.ai32[0]).upl16());}
};

struct GSVertexOld
{
	GIFRegRGBAQ		RGBAQ;
	GIFRegST		ST;
	GIFRegUV		UV;
	GIFRegXYZ		XYZ;
	GIFRegFOG		FOG;

	GSVertexOld() {memset(this, 0, sizeof(*this));}
};

struct GSVertexP
{
	GSVector4 p;
};

struct GSVertexPT1
{
	GSVector4 p;
	GSVector2 t;
};

struct GSVertexPT2
{
	GSVector4 p;
	GSVector2 t[2];
};

struct GSVertexNull 
{
	GSVector4 p;
};

#pragma pack(pop)
