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
#include "GPUDrawScanline.h"

GPUDrawScanline::GPUDrawScanline()
	: m_sp_map("GPUSetupPrim", &m_local)
	, m_ds_map("GPUDrawScanline", &m_local)
{
	memset(&m_local, 0, sizeof(m_local));

	m_local.gd = &m_global;
}

GPUDrawScanline::~GPUDrawScanline()
{
}

void GPUDrawScanline::BeginDraw(const void* param)
{
	memcpy(&m_global, param, sizeof(m_global));

	if(m_global.sel.tme && m_global.sel.twin)
	{
		uint32 u, v;

		u = ~(m_global.twin.x << 3) & 0xff; // TWW
		v = ~(m_global.twin.y << 3) & 0xff; // TWH

		m_local.twin[0].u = GSVector4i((u << 16) | u);
		m_local.twin[0].v = GSVector4i((v << 16) | v);

		u = m_global.twin.z << 3; // TWX
		v = m_global.twin.w << 3; // TWY

		m_local.twin[1].u = GSVector4i((u << 16) | u) & ~m_local.twin[0].u;
		m_local.twin[1].v = GSVector4i((v << 16) | v) & ~m_local.twin[0].v;
	}

	m_ds = m_ds_map[m_global.sel];

	m_de = NULL;

	m_dr = NULL; // TODO

	// doesn't need all bits => less functions generated

	GPUScanlineSelector sel;

	sel.key = 0;

	sel.iip = m_global.sel.iip;
	sel.tfx = m_global.sel.tfx;
	sel.twin = m_global.sel.twin;
	sel.sprite = m_global.sel.sprite;

	m_sp = m_sp_map[sel];
}

void GPUDrawScanline::EndDraw(const GSRasterizerStats& stats, uint64 frame)
{
	m_ds_map.UpdateStats(stats, frame);
}
