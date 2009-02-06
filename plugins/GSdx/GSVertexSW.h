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

__declspec(align(16)) union GSVertexSW
{
	struct {GSVector4 c, p, t;};
	struct {GSVector4 v[3];};
	struct {float f[12];};

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
			test = (v35 ^ v45) & (v35 ^ v45.zwxy()) & (vtl == v3 + (v4 - v5)) & (vbr == v5);
			i = 5; 
		}
		else if(v35.allfalse()) 
		{
			test = (v34 ^ v45) & (v34 ^ v45.zwxy()) & (vtl == v3 + (v5 - v4)) & (vbr == v4);
			i = 4; 
		}
		else if(v45.allfalse())
		{
			test = (v34 ^ v35) & (v34 ^ v35.zwxy()) & (vtl == v5 + (v4 - v3)) & (vbr == v3);
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

__declspec(align(16)) struct GSVertexTrace
{
	GSVertexSW m_min, m_max;

	union
	{
		DWORD value; 
		struct {DWORD x:1, y:1, z:1, f:1, s:1, t:1, q:1, _pad:1, r:1, g:1, b:1, a:1;};
		struct {DWORD xyzf:4, stq:4, rgba:4;};
	} m_eq;

	void Update(const GSVertexSW* v, int count)
	{
		GSVertexSW min, max;

		min.p = v[0].p;
		max.p = v[0].p;
		min.t = v[0].t;
		max.t = v[0].t;
		min.c = v[0].c;
		max.c = v[0].c;

		for(int i = 1; i < count; i++)
		{
			min.c = min.c.minv(v[i].c);
			max.c = max.c.maxv(v[i].c);
			min.p = min.p.minv(v[i].p);
			max.p = max.p.maxv(v[i].p);
			min.t = min.t.minv(v[i].t);
			max.t = max.t.maxv(v[i].t);
		}

		m_min = min;
		m_max = max;

		m_eq.value = (min.p == max.p).mask() | ((min.t == max.t).mask() << 4) | ((min.c == max.c).mask() << 8);
	}
};
