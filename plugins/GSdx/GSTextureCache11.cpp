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
#include "GSTextureCache11.h"

// GSTextureCache11

GSTextureCache11::GSTextureCache11(GSRenderer* r)
	: GSTextureCache(r)
{
}

void GSTextureCache11::Read(Target* t, const GSVector4i& r)
{
	if(t->m_type != RenderTarget) 
	{
		// TODO

		return; 
	}

	const GIFRegTEX0& TEX0 = t->m_TEX0;

	if(TEX0.PSM != PSM_PSMCT32 
	&& TEX0.PSM != PSM_PSMCT24
	&& TEX0.PSM != PSM_PSMCT16
	&& TEX0.PSM != PSM_PSMCT16S)
	{
		//ASSERT(0);

		return;
	}

	if(!t->m_dirty.empty())
	{
		return;
	}

	// printf("GSRenderTarget::Read %d,%d - %d,%d (%08x)\n", r.left, r.top, r.right, r.bottom, TEX0.TBP0);

	int w = r.width();
	int h = r.height();

	GSVector4 src = GSVector4(r) * GSVector4(t->m_texture->GetScale()).xyxy() / GSVector4(t->m_texture->GetSize()).xyxy();

	DXGI_FORMAT format = TEX0.PSM == PSM_PSMCT16 || TEX0.PSM == PSM_PSMCT16S ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R8G8B8A8_UNORM;

	if(GSTexture* offscreen = m_renderer->m_dev->CopyOffscreen(t->m_texture, src, w, h, format))
	{
		GSTexture::GSMap m;

		if(offscreen->Map(m))
		{
			// TODO: block level write

			GSOffset* o = m_renderer->m_mem.GetOffset(TEX0.TBP0, TEX0.TBW, TEX0.PSM);

			switch(TEX0.PSM)
			{
			case PSM_PSMCT32:
				m_renderer->m_mem.WritePixel32(m.bits, m.pitch, o, r);
				break;
			case PSM_PSMCT24:
				m_renderer->m_mem.WritePixel24(m.bits, m.pitch, o, r);
				break;
			case PSM_PSMCT16:
			case PSM_PSMCT16S:
				m_renderer->m_mem.WritePixel16(m.bits, m.pitch, o, r);
				break;
			default:
				ASSERT(0);
			}

			offscreen->Unmap();
		}

		m_renderer->m_dev->Recycle(offscreen);
	}
}

