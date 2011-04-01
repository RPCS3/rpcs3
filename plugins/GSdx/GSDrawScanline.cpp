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
#include "GSDrawScanline.h"
#include "GSTextureCacheSW.h"

GSDrawScanline::GSDrawScanline()
	: m_sp_map("GSSetupPrim", &m_local)
	, m_ds_map("GSDrawScanline", &m_local)
{
	memset(&m_local, 0, sizeof(m_local));

	m_local.gd = &m_global;
}

GSDrawScanline::~GSDrawScanline()
{
}

void GSDrawScanline::BeginDraw(const void* param)
{
	memcpy(&m_global, param, sizeof(m_global));

	if(m_global.sel.mmin && m_global.sel.lcm)
	{
		GSVector4i v = m_global.t.minmax.srl16(m_global.lod.i.x);

		v = v.upl16(v);

		m_local.temp.uv_minmax[0] = v.upl32(v);
		m_local.temp.uv_minmax[1] = v.uph32(v);
	}

	m_ds = m_ds_map[m_global.sel];

	if(m_global.sel.aa1)
	{
		GSScanlineSelector sel;

		sel.key = m_global.sel.key;
		sel.zwrite = 0;
		sel.edge = 1;

		m_de = m_ds_map[sel];
	}
	else
	{
		m_de = NULL;
	}

	if(m_global.sel.IsSolidRect())
	{
		m_dr = (DrawRectPtr)&GSDrawScanline::DrawRect;
	}
	else
	{
		m_dr = NULL;
	}

	// doesn't need all bits => less functions generated

	GSScanlineSelector sel;

	sel.key = 0;

	sel.iip = m_global.sel.iip;
	sel.tfx = m_global.sel.tfx;
	sel.tcc = m_global.sel.tcc;
	sel.fst = m_global.sel.fst;
	sel.fge = m_global.sel.fge;
	sel.sprite = m_global.sel.sprite;
	sel.fb = m_global.sel.fb;
	sel.zb = m_global.sel.zb;
	sel.zoverflow = m_global.sel.zoverflow;

	m_sp = m_sp_map[sel];
}

void GSDrawScanline::EndDraw(const GSRasterizerStats& stats, uint64 frame)
{
	m_ds_map.UpdateStats(stats, frame);
}

void GSDrawScanline::DrawRect(const GSVector4i& r, const GSVertexSW& v)
{
	ASSERT(r.y >= 0);
	ASSERT(r.w >= 0);

	// FIXME: sometimes the frame and z buffer may overlap, the outcome is undefined

	uint32 m;

	m = m_global.zm.u32[0];

	if(m != 0xffffffff)
	{
		const int* zbr = m_global.zbr;
		const int* zbc = m_global.zbc;

		uint32 z = (uint32)v.p.z;

		if(m_global.sel.zpsm != 2)
		{
			if(m == 0)
			{
				DrawRectT<uint32, false>(zbr, zbc, r, z, m);
			}
			else
			{
				DrawRectT<uint32, true>(zbr, zbc, r, z, m);
			}
		}
		else
		{
			if(m == 0)
			{
				DrawRectT<uint16, false>(zbr, zbc, r, z, m);
			}
			else
			{
				DrawRectT<uint16, true>(zbr, zbc, r, z, m);
			}
		}
	}

	m = m_global.fm.u32[0];

	if(m != 0xffffffff)
	{
		const int* fbr = m_global.fbr;
		const int* fbc = m_global.fbc;

		uint32 c = (GSVector4i(v.c) >> 7).rgba32();

		if(m_global.sel.fba)
		{
			c |= 0x80000000;
		}

		if(m_global.sel.fpsm != 2)
		{
			if(m == 0)
			{
				DrawRectT<uint32, false>(fbr, fbc, r, c, m);
			}
			else
			{
				DrawRectT<uint32, true>(fbr, fbc, r, c, m);
			}
		}
		else
		{
			c = ((c & 0xf8) >> 3) | ((c & 0xf800) >> 6) | ((c & 0xf80000) >> 9) | ((c & 0x80000000) >> 16);

			if(m == 0)
			{
				DrawRectT<uint16, false>(fbr, fbc, r, c, m);
			}
			else
			{
				DrawRectT<uint16, true>(fbr, fbc, r, c, m);
			}
		}
	}
}

template<class T, bool masked>
void GSDrawScanline::DrawRectT(const int* RESTRICT row, const int* RESTRICT col, const GSVector4i& r, uint32 c, uint32 m)
{
	if(m == 0xffffffff) return;

	GSVector4i color((int)c);
	GSVector4i mask((int)m);

	if(sizeof(T) == sizeof(uint16))
	{
		color = color.xxzzlh();
		mask = mask.xxzzlh();
	}

	color = color.andnot(mask);

	GSVector4i br = r.ralign<Align_Inside>(GSVector2i(8 * 4 / sizeof(T), 8));

	if(!br.rempty())
	{
		FillRect<T, masked>(row, col, GSVector4i(r.x, r.y, r.z, br.y), c, m);
		FillRect<T, masked>(row, col, GSVector4i(r.x, br.w, r.z, r.w), c, m);

		if(r.x < br.x || br.z < r.z)
		{
			FillRect<T, masked>(row, col, GSVector4i(r.x, br.y, br.x, br.w), c, m);
			FillRect<T, masked>(row, col, GSVector4i(br.z, br.y, r.z, br.w), c, m);
		}

		FillBlock<T, masked>(row, col, br, color, mask);
	}
	else
	{
		FillRect<T, masked>(row, col, r, c, m);
	}
}

template<class T, bool masked>
void GSDrawScanline::FillRect(const int* RESTRICT row, const int* RESTRICT col, const GSVector4i& r, uint32 c, uint32 m)
{
	if(r.x >= r.z) return;

	T* vm = (T*)m_global.vm;

	for(int y = r.y; y < r.w; y++)
	{
		T* RESTRICT d = &vm[row[y]];

		for(int x = r.x; x < r.z; x++)
		{
			d[col[x]] = (T)(!masked ? c : (c | (d[col[x]] & m)));
		}
	}
}

template<class T, bool masked>
void GSDrawScanline::FillBlock(const int* RESTRICT row, const int* RESTRICT col, const GSVector4i& r, const GSVector4i& c, const GSVector4i& m)
{
	if(r.x >= r.z) return;

	T* vm = (T*)m_global.vm;

	for(int y = r.y; y < r.w; y += 8)
	{
		T* RESTRICT d = &vm[row[y]];

		for(int x = r.x; x < r.z; x += 8 * 4 / sizeof(T))
		{
			GSVector4i* RESTRICT p = (GSVector4i*)&d[col[x]];

			for(int i = 0; i < 16; i += 4)
			{
				p[i + 0] = !masked ? c : (c | (p[i + 0] & m));
				p[i + 1] = !masked ? c : (c | (p[i + 1] & m));
				p[i + 2] = !masked ? c : (c | (p[i + 2] & m));
				p[i + 3] = !masked ? c : (c | (p[i + 3] & m));
			}
		}
	}
}
