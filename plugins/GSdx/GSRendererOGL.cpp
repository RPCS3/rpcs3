/*
 *	Copyright (C) 2011-2011 Gregory hainaut
 *	Copyright (C) 2007-2009 Gabest
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

#include "GSRendererOGL.h"
#include "GSRenderer.h"


GSRendererOGL::GSRendererOGL()
	// FIXME
	//: GSRendererHW<GSVertexHWOGL>(new GSTextureCacheOGL(this))
	: GSRendererHW<GSVertexHW11>(new GSTextureCacheOGL(this))
	  , m_topology(0)
{
	m_logz = !!theApp.GetConfig("logz", 0);
	m_fba = !!theApp.GetConfig("fba", 1);
	UserHacks_AlphaHack = !!theApp.GetConfig("UserHacks_AlphaHack", 0);
	m_pixelcenter = GSVector2(-0.5f, -0.5f);

	// TODO must be implementer with macro InitVertexKick(GSRendererOGL)
	// template<uint32 prim, uint32 tme, uint32 fst> void VertexKick(bool skip);
	InitVertexKick(GSRendererOGL);
}

bool GSRendererOGL::CreateDevice(GSDevice* dev)
{
	if(!GSRenderer::CreateDevice(dev))
		return false;

	return true;
}

template<uint32 prim, uint32 tme, uint32 fst>
void GSRendererOGL::VertexKick(bool skip)
{
	GSVertexHW11& dst = m_vl.AddTail();

	dst = *(GSVertexHW11*)&m_v;

#ifdef USE_UPSCALE_HACKS

	if(tme && fst)
	{
		//GSVector4::storel(&dst.ST, m_v.GetUV());

		int Udiff = 0;
		int Vdiff = 0;
		int Uadjust = 0;
		int Vadjust = 0;

		int multiplier = GetUpscaleMultiplier();

		if(multiplier > 1)
		{
			Udiff = m_v.UV.U & 4095;
			Vdiff = m_v.UV.V & 4095;

			if(Udiff != 0)
			{
				if		(Udiff >= 4080)	{/*printf("U+ %d %d\n", Udiff, m_v.UV.U);*/  Uadjust = -1; }
				else if (Udiff <= 16)	{/*printf("U- %d %d\n", Udiff, m_v.UV.U);*/  Uadjust = 1; }
			}
			
			if(Vdiff != 0)
			{
				if		(Vdiff >= 4080)	{/*printf("V+ %d %d\n", Vdiff, m_v.UV.V);*/  Vadjust = -1; }
				else if	(Vdiff <= 16)	{/*printf("V- %d %d\n", Vdiff, m_v.UV.V);*/  Vadjust = 1; }
			}

			Udiff = m_v.UV.U & 255;
			Vdiff = m_v.UV.V & 255;

			if(Udiff != 0)
			{
				if		(Udiff >= 248)	{ Uadjust = -1;	}
				else if (Udiff <= 8)	{ Uadjust = 1; }
			}

			if(Vdiff != 0)
			{
				if		(Vdiff >= 248)	{ Vadjust = -1;	}
				else if	(Vdiff <= 8)	{ Vadjust = 1; }
			}

			Udiff = m_v.UV.U & 15;
			Vdiff = m_v.UV.V & 15;

			if(Udiff != 0)
			{
				if		(Udiff >= 15)	{ Uadjust = -1; }
				else if (Udiff <= 1)	{ Uadjust = 1; }
			}

			if(Vdiff != 0)
			{
				if		(Vdiff >= 15)	{ Vadjust = -1; }
				else if	(Vdiff <= 1)	{ Vadjust = 1; }
			}
		}

		dst.ST.S = (float)m_v.UV.U - Uadjust;
		dst.ST.T = (float)m_v.UV.V - Vadjust;
	}
	else if(tme)
	{
		// Wip :p
		//dst.XYZ.X += 5;
		//dst.XYZ.Y += 5;
	}

#else

	if(tme && fst)
	{
		GSVector4::storel(&dst.ST, m_v.GetUV());
	}

#endif

	int count = 0;
	
	if(GSVertexHW11* v = DrawingKick<prim>(skip, count))
	{
		GSVector4i scissor = m_context->scissor.dx10;

		GSVector4i pmin, pmax;

		#if _M_SSE >= 0x401

		GSVector4i v0, v1, v2;

		switch(prim)
		{
		case GS_POINTLIST:
			v0 = GSVector4i::load((int)v[0].p.xy).upl16();
			pmin = v0;
			pmax = v0;
			break;
		case GS_LINELIST:
		case GS_LINESTRIP:
		case GS_SPRITE:
			v0 = GSVector4i::load((int)v[0].p.xy);
			v1 = GSVector4i::load((int)v[1].p.xy);
			pmin = v0.min_u16(v1).upl16();
			pmax = v0.max_u16(v1).upl16();
			break;
		case GS_TRIANGLELIST:
		case GS_TRIANGLESTRIP:
		case GS_TRIANGLEFAN:
			v0 = GSVector4i::load((int)v[0].p.xy);
			v1 = GSVector4i::load((int)v[1].p.xy);
			v2 = GSVector4i::load((int)v[2].p.xy);
			pmin = v0.min_u16(v1).min_u16(v2).upl16();
			pmax = v0.max_u16(v1).max_u16(v2).upl16();
			break;
		}

		#else

		switch(prim)
		{
		case GS_POINTLIST:
			pmin.x = v[0].p.x;
			pmin.y = v[0].p.y;
			pmax.x = v[0].p.x;
			pmax.y = v[0].p.y;
			break;
		case GS_LINELIST:
		case GS_LINESTRIP:
		case GS_SPRITE:
			pmin.x = std::min<uint16>(v[0].p.x, v[1].p.x);
			pmin.y = std::min<uint16>(v[0].p.y, v[1].p.y);
			pmax.x = std::max<uint16>(v[0].p.x, v[1].p.x);
			pmax.y = std::max<uint16>(v[0].p.y, v[1].p.y);
			break;
		case GS_TRIANGLELIST:
		case GS_TRIANGLESTRIP:
		case GS_TRIANGLEFAN:
			pmin.x = std::min<uint16>(std::min<uint16>(v[0].p.x, v[1].p.x), v[2].p.x);
			pmin.y = std::min<uint16>(std::min<uint16>(v[0].p.y, v[1].p.y), v[2].p.y);
			pmax.x = std::max<uint16>(std::max<uint16>(v[0].p.x, v[1].p.x), v[2].p.x);
			pmax.y = std::max<uint16>(std::max<uint16>(v[0].p.y, v[1].p.y), v[2].p.y);
			break;
		}

		#endif

		GSVector4i test = (pmax < scissor) | (pmin > scissor.zwxy());

		switch(prim)
		{
		case GS_TRIANGLELIST:
		case GS_TRIANGLESTRIP:
		case GS_TRIANGLEFAN:
		case GS_SPRITE:
			test |= pmin == pmax;
			break;
		}

		if(test.mask() & 0xff)
		{
			return;
		}

		m_count += count;
	}
}
