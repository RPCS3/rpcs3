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

#include "GSLocalMemory.h"
#include "GSVector.h"

union GSScanlineSelector
{
	struct
	{
		DWORD fpsm:2; // 0
		DWORD zpsm:2; // 2
		DWORD ztst:2; // 4 (0: off, 1: write, 2: test (ge), 3: test (g))
		DWORD atst:3; // 6
		DWORD afail:2; // 9
		DWORD iip:1; // 11
		DWORD tfx:3; // 12
		DWORD tcc:1; // 15
		DWORD fst:1; // 16
		DWORD ltf:1; // 17
		DWORD tlu:1; // 18
		DWORD fge:1; // 19
		DWORD date:1; // 20
		DWORD abea:2; // 21
		DWORD abeb:2; // 23
		DWORD abec:2; // 25
		DWORD abed:2; // 27
		DWORD pabe:1; // 29
		DWORD rfb:1; // 30
		DWORD sprite:1; // 31

		DWORD fwrite:1; // 32
		DWORD ftest:1; // 33
		DWORD zwrite:1; // 34
		DWORD ztest:1; // 35
		DWORD wms:2; // 36
		DWORD wmt:2; // 38 
		DWORD datm:1; // 40
		DWORD colclamp:1; // 41
		DWORD fba:1; // 42
		DWORD dthe:1; // 43
		DWORD zoverflow:1; // 44 (z max >= 0x80000000)
		DWORD aa1:1; // 45
	};

	struct
	{
		DWORD _pad1:21;
		DWORD abe:8;
		DWORD _pad2:3;
		DWORD fb:2;
		DWORD zb:2;
	};

	struct
	{
		DWORD lo;
		DWORD hi;
	};

	UINT64 key;

	operator DWORD() {return lo;}
	operator UINT64() {return key;}

	bool IsSolidRect()
	{
		return sprite
			&& iip == 0
			&& tfx == TFX_NONE
			&& abe == 255 
			&& ztst <= 1 
			&& atst <= 1
			&& date == 0
			&& fge == 0;
	}
};

__declspec(align(16)) struct GSScanlineParam
{
	GSScanlineSelector sel;

	void* vm;
	const void* tex;
	const DWORD* clut;
	DWORD tw;

	GSLocalMemory::Offset* fbo;
	GSLocalMemory::Offset* zbo;
	GSLocalMemory::Offset4* fzbo;

	DWORD fm, zm;
};

__declspec(align(16)) struct GSScanlineEnvironment
{
	GSScanlineSelector sel;

	void* vm;
	const void* tex;
	const DWORD* clut;
	DWORD tw;

	GSVector4i* fbr;
	GSVector4i* zbr;
	int** fbc;
	int** zbc;
	GSVector2i* fzbr;
	GSVector2i* fzbc;

	GSVector4i* dimx;

	GSVector4i fm, zm;
	struct {GSVector4i min, max, mask, invmask;} t; // [u] x 4 [v] x 4
	GSVector4i aref;
	GSVector4i afix;
	GSVector4i frb, fga;

	struct {GSVector4 z, s, t, q; GSVector4i rb, ga, f, si, ti, _pad[7];} d[4];
	struct {GSVector4 z, stq; GSVector4i c, f, st;} d4;
	struct {GSVector4i rb, ga;} c;
	struct {GSVector4i z, f;} p;
	struct {GSVector4i z, f, s, t, q, rb, ga, zs, zd, uf, vf;} temp;
};
