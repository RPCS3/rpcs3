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

__aligned(struct, 32) GSVertexSW
{
	GSVector4 c, p, t;

	GSVertexSW() {}
	GSVertexSW(const GSVertexSW& v) {*this = v;}

	void operator = (const GSVertexSW& v) {c = v.c; p = v.p; t = v.t;}
	void operator += (const GSVertexSW& v) {c += v.c; p += v.p; t += v.t;}

	friend GSVertexSW operator + (const GSVertexSW& v1, const GSVertexSW& v2);
	friend GSVertexSW operator - (const GSVertexSW& v1, const GSVertexSW& v2);
	friend GSVertexSW operator * (const GSVertexSW& v, const GSVector4& vv);
	friend GSVertexSW operator / (const GSVertexSW& v, const GSVector4& vv);
	friend GSVertexSW operator * (const GSVertexSW& v, float f);
	friend GSVertexSW operator / (const GSVertexSW& v, float f);

	static bool IsQuad(const GSVertexSW* v, int& tl, int& br)
	{
		GSVector4 v0 = v[0].p.xyxy(v[0].t);
		GSVector4 v1 = v[1].p.xyxy(v[1].t);
		GSVector4 v2 = v[2].p.xyxy(v[2].t);

		GSVector4 v01 = v0 == v1;
		GSVector4 v12 = v1 == v2;
		GSVector4 v02 = v0 == v2;

		GSVector4 vtl, vbr;

		GSVector4 test;

		int i;

		if(v12.allfalse())
		{
			test = (v01 ^ v02) & (v01 ^ v02.zwxy());
			vtl = v0;
			vbr = v1 + (v2 - v0);
			i = 0;
		}
		else if(v02.allfalse())
		{
			test = (v01 ^ v12) & (v01 ^ v12.zwxy());
			vtl = v1;
			vbr = v0 + (v2 - v1);
			i = 1;
		}
		else if(v01.allfalse())
		{
			test = (v02 ^ v12) & (v02 ^ v12.zwxy());
			vtl = v2;
			vbr = v0 + (v1 - v2);
			i = 2;
		}
		else
		{
			return false;
		}

		if(!test.alltrue())
		{
			return false;
		}

		tl = i;

		GSVector4 v3 = v[3].p.xyxy(v[3].t);
		GSVector4 v4 = v[4].p.xyxy(v[4].t);
		GSVector4 v5 = v[5].p.xyxy(v[5].t);

		GSVector4 v34 = v3 == v4;
		GSVector4 v45 = v4 == v5;
		GSVector4 v35 = v3 == v5;

		if(v34.allfalse())
		{
			test = (v35 ^ v45) & (v35 ^ v45.zwxy()) & (vtl + v5 == v3 + v4) & (vbr == v5);
			i = 5;
		}
		else if(v35.allfalse())
		{
			test = (v34 ^ v45) & (v34 ^ v45.zwxy()) & (vtl + v4 == v3 + v5) & (vbr == v4);
			i = 4;
		}
		else if(v45.allfalse())
		{
			test = (v34 ^ v35) & (v34 ^ v35.zwxy()) & (vtl + v3 == v5 + v4) & (vbr == v3);
			i = 3;
		}
		else
		{
			return false;
		}

		if(!test.alltrue())
		{
			return false;
		}

		br = i;

		v0 = v[0].p.zwzw(v[0].t);
		v1 = v[1].p.zwzw(v[1].t);
		v2 = v[2].p.zwzw(v[2].t);
		v3 = v[3].p.zwzw(v[3].t);
		v4 = v[4].p.zwzw(v[4].t);
		v5 = v[5].p.zwzw(v[5].t);

		test = ((v0 == v1) & (v0 == v2)) & ((v0 == v3) & (v0 == v4)) & (v0 == v5);

		if(!test.alltrue())
		{
			return false;
		}

		v0 = v[0].c;
		v1 = v[1].c;
		v2 = v[2].c;
		v3 = v[3].c;
		v4 = v[4].c;
		v5 = v[5].c;

		test = ((v0 == v1) & (v0 == v2)) & ((v0 == v3) & (v0 == v4)) & (v0 == v5);

		if(!test.alltrue())
		{
			return false;
		}

		return true;
	}
};

__forceinline GSVertexSW operator + (const GSVertexSW& v1, const GSVertexSW& v2)
{
	GSVertexSW v0;
	v0.c = v1.c + v2.c;
	v0.p = v1.p + v2.p;
	v0.t = v1.t + v2.t;
	return v0;
}

__forceinline GSVertexSW operator - (const GSVertexSW& v1, const GSVertexSW& v2)
{
	GSVertexSW v0;
	v0.c = v1.c - v2.c;
	v0.p = v1.p - v2.p;
	v0.t = v1.t - v2.t;
	return v0;
}

__forceinline GSVertexSW operator * (const GSVertexSW& v, const GSVector4& vv)
{
	GSVertexSW v0;
	v0.c = v.c * vv;
	v0.p = v.p * vv;
	v0.t = v.t * vv;
	return v0;
}

__forceinline GSVertexSW operator / (const GSVertexSW& v, const GSVector4& vv)
{
	GSVertexSW v0;
	v0.c = v.c / vv;
	v0.p = v.p / vv;
	v0.t = v.t / vv;
	return v0;
}

__forceinline GSVertexSW operator * (const GSVertexSW& v, float f)
{
	GSVertexSW v0;
	GSVector4 vf(f);
	v0.c = v.c * vf;
	v0.p = v.p * vf;
	v0.t = v.t * vf;
	return v0;
}

__forceinline GSVertexSW operator / (const GSVertexSW& v, float f)
{
	GSVertexSW v0;
	GSVector4 vf(f);
	v0.c = v.c / vf;
	v0.p = v.p / vf;
	v0.t = v.t / vf;
	return v0;
}
