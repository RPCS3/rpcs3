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

#include "GSRenderer.h"
#include "GSTextureCacheSW.h"
#include "GSDrawScanline.h"

extern const GSVector4 g_pos_scale;

class GSRendererSW : public GSRendererT<GSVertexSW>
{
protected:
	GSRasterizerList m_rl;
	GSTextureCacheSW* m_tc;
	GSVertexTrace m_vtrace;
	GSTexture* m_texture[2];
	bool m_reset;

	void Reset();
	void VSync(int field);
	void ResetDevice();
	GSTexture* GetOutput(int i);

	void Draw();
	void InvalidateVideoMem(const GIFRegBITBLTBUF& BITBLTBUF, const GSVector4i& r);

	void GetTextureMinMax(int w, int h, GSVector4i& r, uint32 fst);
	void GetAlphaMinMax();
	bool TryAlphaTest(uint32& fm, uint32& zm);
	void GetScanlineParam(GSScanlineParam& p, GS_PRIM_CLASS primclass);

public:
	GSRendererSW(uint8* base, bool mt, void (*irq)(), GSDevice* dev);
	virtual ~GSRendererSW();

	template<uint32 prim, uint32 tme, uint32 fst> 
	void VertexKick(bool skip)
	{
		const GSDrawingContext* context = m_context;

		GSVector4i xy = GSVector4i::load((int)m_v.XYZ.u32[0]);
		
		xy = xy.insert16<3>(m_v.FOG.F);
		xy = xy.upl16();
		xy -= context->XYOFFSET;

		GSVertexSW v;

		v.p = GSVector4(xy) * g_pos_scale;

		v.c = GSVector4(GSVector4i::load((int)m_v.RGBAQ.u32[0]).u8to32() << 7);

		if(tme)
		{
			float q;

			if(fst)
			{
				v.t = GSVector4(((GSVector4i)m_v.UV).upl16() << (16 - 4));
				q = 1.0f;
			}
			else
			{
				v.t = GSVector4(m_v.ST.S, m_v.ST.T);
				v.t *= GSVector4(0x10000 << context->TEX0.TW, 0x10000 << context->TEX0.TH);
				q = m_v.RGBAQ.Q;
			}

			v.t = v.t.xyxy(GSVector4::load(q));
		}

		GSVertexSW& dst = m_vl.AddTail();

		dst = v;

		dst.p.z = (float)min(m_v.XYZ.Z, 0xffffff00); // max value which can survive the uint32 => float => uint32 conversion

		int count = 0;
		
		if(GSVertexSW* v = DrawingKick<prim>(skip, count))
		{
if(!m_dump)
{
			GSVector4 pmin, pmax;

			switch(prim)
			{
			case GS_POINTLIST:
				pmin = v[0].p;
				pmax = v[0].p;
				break;
			case GS_LINELIST:
			case GS_LINESTRIP:
			case GS_SPRITE:
				pmin = v[0].p.minv(v[1].p);
				pmax = v[0].p.maxv(v[1].p);
				break;
			case GS_TRIANGLELIST:
			case GS_TRIANGLESTRIP:
			case GS_TRIANGLEFAN:
				pmin = v[0].p.minv(v[1].p).minv(v[2].p);
				pmax = v[0].p.maxv(v[1].p).maxv(v[2].p);
				break;
			}

			GSVector4 scissor = context->scissor.ex;

			GSVector4 test = (pmax < scissor) | (pmin > scissor.zwxy());

			switch(prim)
			{
			case GS_TRIANGLELIST:
			case GS_TRIANGLESTRIP:
			case GS_TRIANGLEFAN:
			case GS_SPRITE:
				test |= pmin.ceil() == pmax.ceil();
				break;
			}

			switch(prim)
			{
			case GS_TRIANGLELIST:
			case GS_TRIANGLESTRIP:
			case GS_TRIANGLEFAN:
				// are in line or just two of them are the same (cross product == 0)
				GSVector4 tmp = (v[1].p - v[0].p) * (v[2].p - v[0].p).yxwz();
				test |= tmp == tmp.yxwz();
				break;
			}
			
			if(test.mask() & 3)
			{
				return;
			}
}
			switch(prim)
			{
			case GS_POINTLIST:
				break;
			case GS_LINELIST:
			case GS_LINESTRIP:
				if(PRIM->IIP == 0) {v[0].c = v[1].c;}
				break;
			case GS_TRIANGLELIST:
			case GS_TRIANGLESTRIP:
			case GS_TRIANGLEFAN:
				if(PRIM->IIP == 0) {v[0].c = v[2].c; v[1].c = v[2].c;}
				break;
			case GS_SPRITE:
				break;
			}

			if(m_count < 30 && m_count >= 3)
			{
				GSVertexSW* v = &m_vertices[m_count - 3];

				int tl = 0;
				int br = 0;

				bool isquad = false;

				switch(prim)
				{
				case GS_TRIANGLESTRIP:
				case GS_TRIANGLEFAN:
				case GS_TRIANGLELIST:
					isquad = GSVertexSW::IsQuad(v, tl, br);
					break;
				}

				if(isquad)
				{
					m_count -= 3;

					if(m_count > 0)
					{
						tl += m_count;
						br += m_count;

						Flush();
					}

					if(tl != 0) m_vertices[0] = m_vertices[tl];
					if(br != 1) m_vertices[1] = m_vertices[br];

					m_count = 2;

					uint32 tmp = PRIM->PRIM;
					PRIM->PRIM = GS_SPRITE;

					Flush();

					PRIM->PRIM = tmp;

					m_perfmon.Put(GSPerfMon::Quad, 1);

					return;
				}
			}

			m_count += count;
		}
	}
};
