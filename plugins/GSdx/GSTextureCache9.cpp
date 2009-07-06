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
#include "GSTextureCache9.h"

// GSTextureCache9

GSTextureCache9::GSTextureCache9(GSRenderer* r)
	: GSTextureCache(r)
{
}

// Source9

// Target9

void GSTextureCache9::Target9::Read(const GSVector4i& r)
{
	if(m_type != RenderTarget) 
	{
		// TODO

		return; 
	}

	if(m_TEX0.PSM != PSM_PSMCT32 
	&& m_TEX0.PSM != PSM_PSMCT24
	&& m_TEX0.PSM != PSM_PSMCT16
	&& m_TEX0.PSM != PSM_PSMCT16S)
	{
		//ASSERT(0);

		return;
	}

	if(!m_dirty.empty())
	{
		return;
	}

	// printf("GSRenderTarget::Read %d,%d - %d,%d (%08x)\n", r.left, r.top, r.right, r.bottom, m_TEX0.TBP0);

	// m_renderer->m_perfmon.Put(GSPerfMon::ReadRT, 1);

	int w = r.width();
	int h = r.height();

	GSVector4 src = GSVector4(r) * GSVector4(m_texture->m_scale).xyxy() / GSVector4(m_texture->GetSize()).xyxy();

	if(GSTexture* offscreen = m_renderer->m_dev->CopyOffscreen(m_texture, src, w, h))
	{
		GSTexture::GSMap m;

		if(offscreen->Map(m))
		{
			// TODO: block level write

			GSLocalMemory::PixelOffset* po = m_renderer->m_mem.GetPixelOffset(m_TEX0.TBP0, m_TEX0.TBW, m_TEX0.PSM);

			switch(m_TEX0.PSM)
			{
			case PSM_PSMCT32:
				m_renderer->m_mem.WritePixel32(m.bits, m.pitch, po, r);
				break;
			case PSM_PSMCT24:
				m_renderer->m_mem.WritePixel24(m.bits, m.pitch, po, r);
				break;
			case PSM_PSMCT16:
			case PSM_PSMCT16S:
				m_renderer->m_mem.WriteFrame16(m.bits, m.pitch, po, r);
				break;
			default:
				ASSERT(0);
			}

			offscreen->Unmap();
		}

		m_renderer->m_dev->Recycle(offscreen);
	}
}

