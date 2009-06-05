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

#include "GSVector.h"

#pragma pack(push, 1)

__declspec(align(16)) union GSVertexHW9
{
	struct
	{
		GSVector2 t;
		union {struct {uint8 r, g, b, a;}; uint32 c0;};
		union {struct {uint8 ta0, ta1, res, f;}; uint32 c1;};
		GSVector4 p;
	};

	struct {GSVector4i vi[2];};
	struct {GSVector4 vf[2];};

	GSVertexHW9& operator = (GSVertexHW9& v) {vi[0] = v.vi[0]; vi[1] = v.vi[1]; return *this;}
	
	float GetQ() {return p.w;}
};

__declspec(align(16)) union GSVertexHW10
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
			struct {union {struct {uint16 x, y;}; uint32 xy;}; uint32 z;} p;
			GIFRegXYZ XYZ;
		};
		
		union 
		{
			union {struct {uint8 r, g, b, a; float q;}; uint32 c0;};
			GIFRegRGBAQ RGBAQ;
		};

		union 
		{
			struct {uint32 _pad; union {struct {uint8 ta0, ta1, res, f;}; uint32 c1;};};
			GIFRegFOG FOG;
		};
	};

	struct {GSVector4i vi[2];};
	struct {GSVector4 vf[2];};

	GSVertexHW10& operator = (GSVertexHW10& v) {vi[0] = v.vi[0]; vi[1] = v.vi[1]; return *this;}

	float GetQ() {return q;}
};

#pragma pack(pop)
