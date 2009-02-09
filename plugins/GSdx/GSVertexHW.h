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
		union {struct {BYTE r, g, b, a;}; DWORD c0;};
		union {struct {BYTE ta0, ta1, res, f;}; DWORD c1;};
		GSVector4 p;
	};
	
	struct {__m128i m128i[2];};
	struct {__m128 m128[2];};

#if _M_SSE >= 0x200
	GSVertexHW9& operator = (GSVertexHW9& v) {m128i[0] = v.m128i[0]; m128i[1] = v.m128i[1]; return *this;}
#endif

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
			struct {union {struct {WORD x, y;}; DWORD xy;}; DWORD z;} p;
			GIFRegXYZ XYZ;
		};
		
		union 
		{
			union {struct {BYTE r, g, b, a; float q;}; DWORD c0;};
			GIFRegRGBAQ RGBAQ;
		};

		union 
		{
			struct {DWORD _pad[1]; union {struct {BYTE ta0, ta1, res, f;}; DWORD c1;};};
			GIFRegFOG FOG;
		};
	};
	
	struct {__m128i m128i[2];};
	struct {__m128 m128[2];};

#if _M_SSE >= 0x200
	GSVertexHW10& operator = (GSVertexHW10& v) {m128i[0] = v.m128i[0]; m128i[1] = v.m128i[1]; return *this;}
#endif

	float GetQ() {return q;}
};

#pragma pack(pop)
