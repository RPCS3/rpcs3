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
#include "GSDrawScanline.h"
#include "GSTextureCacheSW.h"

GSDrawScanline::GSDrawScanline(GSState* state, int id)
	: m_state(state)
	, m_id(id)
	, m_sp(m_env)
	, m_ds(m_env)
{
	memset(&m_env, 0, sizeof(m_env));
}

GSDrawScanline::~GSDrawScanline()
{
}

void GSDrawScanline::BeginDraw(const GSRasterizerData* data, Functions* f)
{
	GSDrawingEnvironment& env = m_state->m_env;
	GSDrawingContext* context = m_state->m_context;

	const GSScanlineParam* p = (const GSScanlineParam*)data->param;

	m_sel = p->sel;

	m_env.vm = p->vm;
	m_env.fbr = p->fbo->pixel.row;
	m_env.zbr = p->zbo->pixel.row;
	m_env.fbc = p->fbo->pixel.col[0];
	m_env.zbc = p->zbo->pixel.col[0];
	m_env.fzbr = p->fzbo->row;
	m_env.fzbc = p->fzbo->col;
	m_env.fm = GSVector4i(p->fm);
	m_env.zm = GSVector4i(p->zm);
	m_env.aref = GSVector4i((int)context->TEST.AREF);
	m_env.afix = GSVector4i((int)context->ALPHA.FIX << 7).xxzzlh();
	m_env.frb = GSVector4i((int)env.FOGCOL.u32[0] & 0x00ff00ff);
	m_env.fga = GSVector4i((int)(env.FOGCOL.u32[0] >> 8) & 0x00ff00ff);
	m_env.dimx = env.dimx;

	if(m_sel.fpsm == 1)
	{
		m_env.fm |= GSVector4i::xff000000();
	}
	else if(m_sel.fpsm == 2)
	{
		GSVector4i rb = m_env.fm & 0x00f800f8;
		GSVector4i ga = m_env.fm & 0x8000f800;

		m_env.fm = (ga >> 16) | (rb >> 9) | (ga >> 6) | (rb >> 3) | GSVector4i::xffff0000();
	}

	if(m_sel.zpsm == 1)
	{
		m_env.zm |= GSVector4i::xff000000();
	}
	else if(m_sel.zpsm == 2)
	{
		m_env.zm |= GSVector4i::xffff0000();
	}

	if(m_sel.atst == ATST_LESS)
	{
		m_sel.atst = ATST_LEQUAL;

		m_env.aref -= GSVector4i::x00000001();
	}
	else if(m_sel.atst == ATST_GREATER)
	{
		m_sel.atst = ATST_GEQUAL;

		m_env.aref += GSVector4i::x00000001();
	}

	if(m_sel.tfx != TFX_NONE)
	{
		m_env.tex = p->tex;
		m_env.clut = p->clut;
		m_env.tw = p->tw;

		unsigned short tw = (unsigned short)(1 << context->TEX0.TW);
		unsigned short th = (unsigned short)(1 << context->TEX0.TH);

		switch(context->CLAMP.WMS)
		{
		case CLAMP_REPEAT:
			m_env.t.min.u16[0] = tw - 1;
			m_env.t.max.u16[0] = 0;
			m_env.t.mask.u32[0] = 0xffffffff;
			break;
		case CLAMP_CLAMP:
			m_env.t.min.u16[0] = 0;
			m_env.t.max.u16[0] = tw - 1;
			m_env.t.mask.u32[0] = 0;
			break;
		case CLAMP_REGION_CLAMP:
			m_env.t.min.u16[0] = std::min<int>(context->CLAMP.MINU, tw - 1);
			m_env.t.max.u16[0] = std::min<int>(context->CLAMP.MAXU, tw - 1);
			m_env.t.mask.u32[0] = 0;
			break;
		case CLAMP_REGION_REPEAT:
			m_env.t.min.u16[0] = context->CLAMP.MINU;
			m_env.t.max.u16[0] = context->CLAMP.MAXU;
			m_env.t.mask.u32[0] = 0xffffffff;
			break;
		default:
			__assume(0);
		}

		switch(context->CLAMP.WMT)
		{
		case CLAMP_REPEAT:
			m_env.t.min.u16[4] = th - 1;
			m_env.t.max.u16[4] = 0;
			m_env.t.mask.u32[2] = 0xffffffff;
			break;
		case CLAMP_CLAMP:
			m_env.t.min.u16[4] = 0;
			m_env.t.max.u16[4] = th - 1;
			m_env.t.mask.u32[2] = 0;
			break;
		case CLAMP_REGION_CLAMP:
			m_env.t.min.u16[4] = std::min<int>(context->CLAMP.MINV, th - 1);
			m_env.t.max.u16[4] = std::min<int>(context->CLAMP.MAXV, th - 1); // ffx anima summon scene, when the anchor appears (th = 256, maxv > 256)
			m_env.t.mask.u32[2] = 0;
			break;
		case CLAMP_REGION_REPEAT:
			m_env.t.min.u16[4] = context->CLAMP.MINV;
			m_env.t.max.u16[4] = context->CLAMP.MAXV;
			m_env.t.mask.u32[2] = 0xffffffff;
			break;
		default:
			__assume(0);
		}

		m_env.t.min = m_env.t.min.xxxxlh();
		m_env.t.max = m_env.t.max.xxxxlh();
		m_env.t.mask = m_env.t.mask.xxzz();
		m_env.t.invmask = ~m_env.t.mask;
	}

	//

	f->ssl = m_ds[m_sel];

	if(m_sel.aa1)// && (m_state->m_perfmon.GetFrame() & 0x40))
	{
		GSScanlineSelector sel;

		sel.key = m_sel.key;
		sel.zwrite = 0;
		sel.edge = 1;

		f->ssle = m_ds[sel];
	}

	if(m_sel.IsSolidRect())
	{
		f->sr = (DrawSolidRectPtr)&GSDrawScanline::DrawSolidRect;
	}

	// doesn't need all bits => less functions generated

	GSScanlineSelector sel;

	sel.key = 0;

	sel.iip = m_sel.iip;
	sel.tfx = m_sel.tfx;
	sel.tcc = m_sel.tcc;
	sel.fst = m_sel.fst;
	sel.fge = m_sel.fge;
	sel.sprite = m_sel.sprite;
	sel.fb = m_sel.fb;
	sel.zb = m_sel.zb;
	sel.zoverflow = m_sel.zoverflow;

	f->ssp = m_sp[sel];
}

void GSDrawScanline::EndDraw(const GSRasterizerStats& stats)
{
	m_ds.UpdateStats(stats, m_state->m_perfmon.GetFrame());
}

void GSDrawScanline::DrawSolidRect(const GSVector4i& r, const GSVertexSW& v)
{
	ASSERT(r.y >= 0);
	ASSERT(r.w >= 0);

	// FIXME: sometimes the frame and z buffer may overlap, the outcome is undefined

	uint32 m;

	m = m_env.zm.u32[0];

	if(m != 0xffffffff)
	{
		uint32 z = (uint32)v.p.z;

		if(m_sel.zpsm != 2)
		{
			if(m == 0)
			{
				DrawSolidRectT<uint32, false>(m_env.zbr, m_env.zbc, r, z, m);
			}
			else
			{
				DrawSolidRectT<uint32, true>(m_env.zbr, m_env.zbc, r, z, m);
			}
		}
		else
		{
			if(m == 0)
			{
				DrawSolidRectT<uint16, false>(m_env.zbr, m_env.zbc, r, z, m);
			}
			else
			{
				DrawSolidRectT<uint16, true>(m_env.zbr, m_env.zbc, r, z, m);
			}
		}
	}

	m = m_env.fm.u32[0];

	if(m != 0xffffffff)
	{
		uint32 c = (GSVector4i(v.c) >> 7).rgba32();

		if(m_state->m_context->FBA.FBA)
		{
			c |= 0x80000000;
		}

		if(m_sel.fpsm != 2)
		{
			if(m == 0)
			{
				DrawSolidRectT<uint32, false>(m_env.fbr, m_env.fbc, r, c, m);
			}
			else
			{
				DrawSolidRectT<uint32, true>(m_env.fbr, m_env.fbc, r, c, m);
			}
		}
		else
		{
			c = ((c & 0xf8) >> 3) | ((c & 0xf800) >> 6) | ((c & 0xf80000) >> 9) | ((c & 0x80000000) >> 16);

			if(m == 0)
			{
				DrawSolidRectT<uint16, false>(m_env.fbr, m_env.fbc, r, c, m);
			}
			else
			{
				DrawSolidRectT<uint16, true>(m_env.fbr, m_env.fbc, r, c, m);
			}
		}
	}
}

template<class T, bool masked>
void GSDrawScanline::DrawSolidRectT(const int* RESTRICT row, const int* RESTRICT col, const GSVector4i& r, uint32 c, uint32 m)
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

	GSVector4i br = r.ralign<GSVector4i::Inside>(GSVector2i(8 * 4 / sizeof(T), 8));

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

	for(int y = r.y; y < r.w; y++)
	{
		T* RESTRICT d = &((T*)m_env.vm)[row[y]];

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

	for(int y = r.y; y < r.w; y += 8)
	{
		T* RESTRICT d = &((T*)m_env.vm)[row[y]];

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

//

GSDrawScanline::GSSetupPrimMap::GSSetupPrimMap(GSScanlineEnvironment& env)
	: GSCodeGeneratorFunctionMap("GSSetupPrim")
	, m_env(env)
{
}

GSSetupPrimCodeGenerator* GSDrawScanline::GSSetupPrimMap::Create(uint64 key, void* ptr, size_t maxsize)
{
	return new GSSetupPrimCodeGenerator(m_env, key, ptr, maxsize);
}

//

GSDrawScanline::GSDrawScanlineMap::GSDrawScanlineMap(GSScanlineEnvironment& env)
	: GSCodeGeneratorFunctionMap("GSDrawScanline")
	, m_env(env)
{
}

GSDrawScanlineCodeGenerator* GSDrawScanline::GSDrawScanlineMap::Create(uint64 key, void* ptr, size_t maxsize)
{
	return new GSDrawScanlineCodeGenerator(m_env, key, ptr, maxsize);
}
