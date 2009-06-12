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
#include "GPUDrawScanline.h"

GPUDrawScanline::GPUDrawScanline(GPUState* state, int id)
	: m_state(state)
	, m_id(id)
	, m_sp(m_env)
	, m_ds(m_env)
{
}

GPUDrawScanline::~GPUDrawScanline()
{
}

void GPUDrawScanline::BeginDraw(const GSRasterizerData* data, Functions* f)
{
	GPUDrawingEnvironment& env = m_state->m_env;

	const GPUScanlineParam* p = (const GPUScanlineParam*)data->param;

	m_env.sel = p->sel;

	m_env.vm = m_state->m_mem.GetPixelAddress(0, 0);

	if(m_env.sel.tme)
	{
		m_env.tex = p->tex;
		m_env.clut = p->clut;

		if(m_env.sel.twin)
		{
			uint32 u, v;

			u = ~(env.TWIN.TWW << 3) & 0xff;
			v = ~(env.TWIN.TWH << 3) & 0xff;

			m_env.twin[0].u = GSVector4i((u << 16) | u);
			m_env.twin[0].v = GSVector4i((v << 16) | v);

			u = env.TWIN.TWX << 3;
			v = env.TWIN.TWY << 3;
			
			m_env.twin[1].u = GSVector4i((u << 16) | u) & ~m_env.twin[0].u;
			m_env.twin[1].v = GSVector4i((v << 16) | v) & ~m_env.twin[0].v;
		}
	}

	//

	f->ssl = m_ds[m_env.sel];

	f->sr = NULL; // TODO

	// doesn't need all bits => less functions generated

	GPUScanlineSelector sel;

	sel.key = 0;

	sel.iip = m_env.sel.iip;
	sel.tfx = m_env.sel.tfx;
	sel.twin = m_env.sel.twin;
	sel.sprite = m_env.sel.sprite;

	f->ssp = m_sp[sel];
}

void GPUDrawScanline::EndDraw(const GSRasterizerStats& stats)
{
	m_ds.UpdateStats(stats, m_state->m_perfmon.GetFrame());
}

//

GPUDrawScanline::GPUSetupPrimMap::GPUSetupPrimMap(GPUScanlineEnvironment& env)
	: GSCodeGeneratorFunctionMap("GPUSetupPrim")
	, m_env(env)
{
}

GPUSetupPrimCodeGenerator* GPUDrawScanline::GPUSetupPrimMap::Create(uint32 key, void* ptr, size_t maxsize)
{
	return new GPUSetupPrimCodeGenerator(m_env, ptr, maxsize);
}

//

GPUDrawScanline::GPUDrawScanlineMap::GPUDrawScanlineMap(GPUScanlineEnvironment& env)
	: GSCodeGeneratorFunctionMap("GPUDrawScanline")
	, m_env(env)
{
}

GPUDrawScanlineCodeGenerator* GPUDrawScanline::GPUDrawScanlineMap::Create(uint32 key, void* ptr, size_t maxsize)
{
	return new GPUDrawScanlineCodeGenerator(m_env, ptr, maxsize);
}
