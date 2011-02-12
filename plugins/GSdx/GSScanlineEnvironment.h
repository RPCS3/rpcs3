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
		uint32 fpsm:2; // 0
		uint32 zpsm:2; // 2
		uint32 ztst:2; // 4 (0: off, 1: write, 2: test (ge), 3: test (g))
		uint32 atst:3; // 6
		uint32 afail:2; // 9
		uint32 iip:1; // 11
		uint32 tfx:3; // 12
		uint32 tcc:1; // 15
		uint32 fst:1; // 16
		uint32 ltf:1; // 17
		uint32 tlu:1; // 18
		uint32 fge:1; // 19
		uint32 date:1; // 20
		uint32 abe:1; // 21
		uint32 aba:2; // 22
		uint32 abb:2; // 24
		uint32 abc:2; // 26
		uint32 abd:2; // 28
		uint32 pabe:1; // 30
		uint32 aa1:1; // 31

		uint32 fwrite:1; // 32
		uint32 ftest:1; // 33
		uint32 rfb:1; // 34
		uint32 zwrite:1; // 35
		uint32 ztest:1; // 36
		uint32 zoverflow:1; // 37 (z max >= 0x80000000)
		uint32 wms:2; // 38
		uint32 wmt:2; // 40
		uint32 datm:1; // 42
		uint32 colclamp:1; // 43
		uint32 fba:1; // 44
		uint32 dthe:1; // 45
		uint32 sprite:1; // 46
		uint32 edge:1; // 47

		uint32 tw:3; // 48 (encodes values between 3 -> 10, texture cache makes sure it is at least 3)
	};

	struct
	{
		uint32 _pad1:22;
		uint32 ababcd:8;
		uint32 _pad2:2;
		uint32 fb:2;
		uint32 _pad3:1;
		uint32 zb:2;
	};

	struct
	{
		uint32 lo;
		uint32 hi;
	};

	uint64 key;

	operator uint32() {return lo;}
	operator uint64() {return key;}

	bool IsSolidRect()
	{
		return sprite
			&& iip == 0
			&& tfx == TFX_NONE
			&& abe == 0
			&& ztst <= 1
			&& atst <= 1
			&& date == 0
			&& fge == 0;
	}
};

__aligned32 struct GSScanlineParam
{
	GSScanlineSelector sel;

	void* vm;
	const void* tex;
	const uint32* clut;
	//uint32 tw;

	GSOffset* fbo;
	GSOffset* zbo;
	GSPixelOffset4* fzbo;

	uint32 fm, zm;
};

__aligned32 struct GSScanlineEnvironment
{
	void* vm;
	const void* tex;
	const uint32* clut;
	//uint32 tw;

	int* fbr;
	int* zbr;
	int* fbc;
	int* zbc;
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
	struct {GSVector4i z, f, s, t, q, rb, ga, zs, zd, uf, vf, cov;} temp;
};
