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

#include "stdafx.h"
#include "GSVector.h"

const GSVector4i GSVector4i::m_xff[17] = 
{
	GSVector4i(0x00000000, 0x00000000, 0x00000000, 0x00000000), 
	GSVector4i(0x000000ff, 0x00000000, 0x00000000, 0x00000000), 
	GSVector4i(0x0000ffff, 0x00000000, 0x00000000, 0x00000000), 
	GSVector4i(0x00ffffff, 0x00000000, 0x00000000, 0x00000000), 
	GSVector4i(0xffffffff, 0x00000000, 0x00000000, 0x00000000),
	GSVector4i(0xffffffff, 0x000000ff, 0x00000000, 0x00000000), 
	GSVector4i(0xffffffff, 0x0000ffff, 0x00000000, 0x00000000), 
	GSVector4i(0xffffffff, 0x00ffffff, 0x00000000, 0x00000000), 
	GSVector4i(0xffffffff, 0xffffffff, 0x00000000, 0x00000000),
	GSVector4i(0xffffffff, 0xffffffff, 0x000000ff, 0x00000000), 
	GSVector4i(0xffffffff, 0xffffffff, 0x0000ffff, 0x00000000), 
	GSVector4i(0xffffffff, 0xffffffff, 0x00ffffff, 0x00000000), 
	GSVector4i(0xffffffff, 0xffffffff, 0xffffffff, 0x00000000),
	GSVector4i(0xffffffff, 0xffffffff, 0xffffffff, 0x000000ff), 
	GSVector4i(0xffffffff, 0xffffffff, 0xffffffff, 0x0000ffff), 
	GSVector4i(0xffffffff, 0xffffffff, 0xffffffff, 0x00ffffff), 
	GSVector4i(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff),
};

const GSVector4i GSVector4i::m_x0f[17] =
{
	GSVector4i(0x00000000, 0x00000000, 0x00000000, 0x00000000), 
	GSVector4i(0x0000000f, 0x00000000, 0x00000000, 0x00000000), 
	GSVector4i(0x00000f0f, 0x00000000, 0x00000000, 0x00000000), 
	GSVector4i(0x000f0f0f, 0x00000000, 0x00000000, 0x00000000), 
	GSVector4i(0x0f0f0f0f, 0x00000000, 0x00000000, 0x00000000),
	GSVector4i(0x0f0f0f0f, 0x0000000f, 0x00000000, 0x00000000), 
	GSVector4i(0x0f0f0f0f, 0x00000f0f, 0x00000000, 0x00000000), 
	GSVector4i(0x0f0f0f0f, 0x000f0f0f, 0x00000000, 0x00000000), 
	GSVector4i(0x0f0f0f0f, 0x0f0f0f0f, 0x00000000, 0x00000000),
	GSVector4i(0x0f0f0f0f, 0x0f0f0f0f, 0x0000000f, 0x00000000), 
	GSVector4i(0x0f0f0f0f, 0x0f0f0f0f, 0x00000f0f, 0x00000000), 
	GSVector4i(0x0f0f0f0f, 0x0f0f0f0f, 0x000f0f0f, 0x00000000), 
	GSVector4i(0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x00000000),
	GSVector4i(0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x0000000f), 
	GSVector4i(0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x00000f0f), 
	GSVector4i(0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x000f0f0f), 
	GSVector4i(0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f),
};

const GSVector4 GSVector4::m_ps0123(0.0f, 1.0f, 2.0f, 3.0f);
const GSVector4 GSVector4::m_ps4567(4.0f, 5.0f, 6.0f, 7.0f);
const GSVector4 GSVector4::m_half(0.5f);
const GSVector4 GSVector4::m_one(1.0f);
const GSVector4 GSVector4::m_two(2.0f);
const GSVector4 GSVector4::m_four(4.0f);
const GSVector4 GSVector4::m_x4b000000(_mm_castsi128_ps(_mm_set1_epi32(0x4b000000)));
const GSVector4 GSVector4::m_x4f800000(_mm_castsi128_ps(_mm_set1_epi32(0x4f800000)));

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

static const int s_ar[][2] = {{0, 0}, {4, 3}, {16, 9}};

GSVector4i GSVector4i::fit(int preset) const
{
	GSVector4i r;

	if(preset > 0 && preset < countof(s_ar))
	{
		r = fit(s_ar[preset][0], s_ar[preset][1]);
	}
	else
	{
		r = *this;
	}

	return r;
}