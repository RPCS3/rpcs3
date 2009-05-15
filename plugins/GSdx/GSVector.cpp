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

#include "StdAfx.h"
#include "GSVector.h"

const GSVector4 GSVector4::m_ps0123(0.0f, 1.0f, 2.0f, 3.0f);
const GSVector4 GSVector4::m_ps4567(4.0f, 5.0f, 6.0f, 7.0f);
const GSVector4 GSVector4::m_x3f800000(_mm_castsi128_ps(_mm_set1_epi32(0x3f800000)));
const GSVector4 GSVector4::m_x4b000000(_mm_castsi128_ps(_mm_set1_epi32(0x4b000000)));

void GSVector4::operator = (const GSVector4i& v) 
{
	m = _mm_cvtepi32_ps(v);
}

void GSVector4i::operator = (const GSVector4& v)
{
	m = _mm_cvttps_epi32(v);
}

GSVector4i GSVector4i::cast(const GSVector4& v)
{
	return GSVector4i(_mm_castps_si128(v.m));
}

GSVector4 GSVector4::cast(const GSVector4i& v)
{
	return GSVector4(_mm_castsi128_ps(v.m));
}

GSVector4i GSVector4i::fit(int arx, int ary) const
{
	GSVector4i r = *this;

	if(arx > 0 && ary > 0)
	{
		int w = width();
		int h = height();

		if(w * ary > h * arx)
		{
			w = h * arx / ary;
			r.left = (r.left + r.right - w) >> 1;
			if(r.left & 1) r.left++;
			r.right = r.left + w;
		}
		else
		{
			h = w * ary / arx;
			r.top = (r.top + r.bottom - h) >> 1;
			if(r.top & 1) r.top++;
			r.bottom = r.top + h;
		}

		r = r.rintersect(*this);
	}
	else
	{
		r = *this;
	}

	return r;
}

GSVector4i GSVector4i::fit(int preset) const
{
	GSVector4i r;

	static const int ar[][2] = {{0, 0}, {4, 3}, {16, 9}};

	if(preset > 0 && preset < countof(ar))
	{
		r = fit(ar[preset][0], ar[preset][1]);
	}
	else
	{
		r = *this;
	}

	return r;
}
