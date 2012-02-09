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

#define GS_BILINEAR_PRECISION 4 // max precision 15, but several games like okami, rogue galaxy, dq8 break above 4

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
		uint32 prim:2; // 46

		uint32 edge:1; // 48
		uint32 tw:3; // 49 (encodes values between 3 -> 10, texture cache makes sure it is at least 3)
		uint32 lcm:1; // 52
		uint32 mmin:2; // 53
		uint32 notest:1; // 54 (no ztest, no atest, no date, no scissor test, and horizontally aligned to 4 pixels)
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

	operator uint32() const {return lo;}
	operator uint64() const {return key;}

	bool IsSolidRect() const
	{
		return prim == GS_SPRITE_CLASS
			&& iip == 0
			&& tfx == TFX_NONE
			&& abe == 0
			&& ztst <= 1
			&& atst <= 1
			&& date == 0
			&& fge == 0;
	}
};

__aligned(struct, 32) GSScanlineGlobalData // per batch variables, this is like a pixel shader constant buffer
{
	GSScanlineSelector sel;

	// - the data of vm, tex may change, multi-threaded drawing must be finished before that happens, clut and dimx are copies
	// - tex is a cached texture, it may be recycled to free up memory, its absolute address cannot be compiled into code
	// - row and column pointers are allocated once and never change or freed, thier address can be used directly

	void* vm;
	const void* tex[7];
	uint32* clut;
	GSVector4i* dimx; 

	const int* fbr;
	const int* zbr;
	const int* fbc;
	const int* zbc;
	const GSVector2i* fzbr;
	const GSVector2i* fzbc;

	GSVector4i fm, zm;
	struct {GSVector4i min, max, minmax, mask, invmask;} t; // [u] x 4 [v] x 4
	GSVector4i aref;
	GSVector4i afix;
	GSVector4i frb, fga;
	GSVector4 mxl;
	GSVector4 k; // TEX1.K * 0x10000
	GSVector4 l; // TEX1.L * -0x10000
	struct {GSVector4i i, f;} lod; // lcm == 1
};

__aligned(struct, 32) GSScanlineLocalData // per prim variables, each thread has its own
{
	struct skip {GSVector4 z, s, t, q; GSVector4i rb, ga, f, _pad;} d[4];
	struct step {GSVector4 z, stq; GSVector4i c, f;} d4;
	struct {GSVector4i rb, ga;} c;
	struct {GSVector4i z, f;} p;

	// these should be stored on stack as normal local variables (no free regs to use, esp cannot be saved to anywhere, and we need an aligned stack)

	struct 
	{
		GSVector4 z, zo;
		GSVector4i f;
		GSVector4 s, t, q;
		GSVector4i rb, ga;
		GSVector4i zs, zd;
		GSVector4i uf, vf;
		GSVector4i cov;

		// mipmapping

		struct {GSVector4i i, f;} lod;
		GSVector4i uv[2];
		GSVector4i uv_minmax[2];
		GSVector4i trb, tga;
		GSVector4i test;
	} temp; 

	//

	const GSScanlineGlobalData* gd;
};
