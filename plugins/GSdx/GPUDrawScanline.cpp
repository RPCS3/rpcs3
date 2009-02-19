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
	m_env.fbw = 10 + m_state->m_mem.GetScale().cx;

	if(m_env.sel.tme)
	{
		m_env.tex = p->tex;
		m_env.clut = p->clut;

		if(m_env.sel.twin)
		{
			DWORD u, v;

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

	f->ssl = m_ds.Lookup(m_env.sel);

	f->sr = NULL; // TODO

	//
	
	DWORD sel = 0;

	sel |= (data->primclass == GS_SPRITE_CLASS ? 1 : 0) << 0;
	sel |= m_env.sel.tme << 1;
	sel |= m_env.sel.iip << 2;

	f->sp = m_sp.Lookup(sel);
}

template<DWORD sprite, DWORD tme, DWORD iip>
void GPUDrawScanline::SetupPrim(const GSVertexSW* vertices, const GSVertexSW& dscan)
{
	if(m_env.sel.tme && !m_env.sel.twin)
	{
		if(sprite)
		{
			GSVector4i t;

			t = (GSVector4i(vertices[1].t) >> 8) - GSVector4i::x00000001();
			t = t.ps32(t);
			t = t.upl16(t);

			m_env.twin[2].u = t.xxxx();
			m_env.twin[2].v = t.yyyy();
		}
		else
		{
			m_env.twin[2].u = GSVector4i::x00ff();
			m_env.twin[2].v = GSVector4i::x00ff();
		}
	}

	GSVector4 ps0123 = GSVector4::ps0123();
	GSVector4 ps4567 = GSVector4::ps4567();

	GSVector4 dt = dscan.t;
	GSVector4 dc = dscan.c;

	GSVector4i dtc8 = GSVector4i(dt * 8.0f).ps32(GSVector4i(dc * 8.0f));

	if(tme)
	{
		m_env.d8.st = dtc8.upl16(dtc8);

		m_env.d.s = GSVector4i(dt.xxxx() * ps0123).ps32(GSVector4i(dt.xxxx() * ps4567));
		m_env.d.t = GSVector4i(dt.yyyy() * ps0123).ps32(GSVector4i(dt.yyyy() * ps4567));
	}

	if(iip)
	{
		m_env.d8.c = dtc8.uph16(dtc8);

		m_env.d.r = GSVector4i(dc.xxxx() * ps0123).ps32(GSVector4i(dc.xxxx() * ps4567));
		m_env.d.g = GSVector4i(dc.yyyy() * ps0123).ps32(GSVector4i(dc.yyyy() * ps4567));
		m_env.d.b = GSVector4i(dc.zzzz() * ps0123).ps32(GSVector4i(dc.zzzz() * ps4567));
	}
	else
	{
		// TODO: m_env.c.r/g/b = ...
	}
}

//

GPUDrawScanline::GPUSetupPrimMap::GPUSetupPrimMap()
{
	#define InitSP_IIP(sprite, tme, iip) \
		m_default[sprite][tme][iip] = (SetupPrimPtr)&GPUDrawScanline::SetupPrim<sprite, tme, iip>; \

	#define InitSP_TME(sprite, tme) \
		InitSP_IIP(sprite, tme, 0) \
		InitSP_IIP(sprite, tme, 1) \

	#define InitSP_SPRITE(sprite) \
		InitSP_TME(sprite, 0) \
		InitSP_TME(sprite, 1) \

	InitSP_SPRITE(0);
	InitSP_SPRITE(1);
}

IDrawScanline::SetupPrimPtr GPUDrawScanline::GPUSetupPrimMap::GetDefaultFunction(DWORD key)
{
	DWORD sprite = (key >> 0) & 1;
	DWORD tme = (key >> 1) & 1;
	DWORD iip = (key >> 2) & 1;

	return m_default[sprite][tme][iip];
}

//

GPUDrawScanline::GPUDrawScanlineMap::GPUDrawScanlineMap(GPUScanlineEnvironment& env)
	: m_env(env)
{
}

GPUDrawScanlineCodeGenerator* GPUDrawScanline::GPUDrawScanlineMap::Create(DWORD key, void* ptr, size_t maxsize)
{
	return new GPUDrawScanlineCodeGenerator(m_env, ptr, maxsize);
}
