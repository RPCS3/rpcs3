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

GSTextureCache9::GSTextureCache9(GSRenderer<GSDevice9>* renderer)
	: GSTextureCache<GSDevice9>(renderer)
{
}

// GSRenderTarget9

void GSTextureCache9::GSRenderTarget9::Update()
{
	__super::Update();

	// FIXME: the union of the rects may also update wrong parts of the render target (but a lot faster :)

	CRect r = m_dirty.GetDirtyRect(m_TEX0, m_texture.GetSize());

	m_dirty.RemoveAll();

	if(r.IsRectEmpty()) return;

	int w = r.Width();
	int h = r.Height();

	Texture texture;

	if(!m_renderer->m_dev.CreateTexture(texture, w, h)) 
		return;

	BYTE* bits;
	int pitch;

	if(texture.Map(&bits, pitch))
	{
		GIFRegTEXA TEXA;

		TEXA.AEM = 1;
		TEXA.TA0 = 0;
		TEXA.TA1 = 0x80;

		GIFRegCLAMP CLAMP;

		CLAMP.WMS = 0;
		CLAMP.WMT = 0;

		m_renderer->m_mem.ReadTexture(r, bits, pitch, m_TEX0, TEXA, CLAMP);

		texture.Unmap();
		
		// m_renderer->m_perfmon.Put(GSPerfMon::Unswizzle, r.Width() * r.Height() * 4);

		GSVector4 dr(
			m_texture.m_scale.x * r.left, 
			m_texture.m_scale.y * r.top, 
			m_texture.m_scale.x * r.right, 
			m_texture.m_scale.y * r.bottom);

		m_renderer->m_dev.StretchRect(texture, m_texture, dr);
	}

	m_renderer->m_dev.Recycle(texture);
}

void GSTextureCache9::GSRenderTarget9::Read(CRect r)
{
	if(m_TEX0.PSM != PSM_PSMCT32 
	&& m_TEX0.PSM != PSM_PSMCT24
	&& m_TEX0.PSM != PSM_PSMCT16
	&& m_TEX0.PSM != PSM_PSMCT16S)
	{
		//ASSERT(0);
		return;
	}

	if(!m_dirty.IsEmpty())
	{
		return;
	}

	TRACE(_T("GSRenderTarget::Read %d,%d - %d,%d (%08x)\n"), r.left, r.top, r.right, r.bottom, m_TEX0.TBP0);

	// m_renderer->m_perfmon.Put(GSPerfMon::ReadRT, 1);

	int w = r.Width();
	int h = r.Height();

	GSVector4 src;

	src.x = m_texture.m_scale.x * r.left / m_texture.GetWidth();
	src.y = m_texture.m_scale.y * r.top / m_texture.GetHeight();
	src.z = m_texture.m_scale.x * r.right / m_texture.GetWidth();
	src.w = m_texture.m_scale.y * r.bottom / m_texture.GetHeight();

	Texture offscreen;

	if(!m_renderer->m_dev.CopyOffscreen(m_texture, src, offscreen, w, h))
		return;

	BYTE* bits;
	int pitch;

	if(offscreen.Map(&bits, pitch))
	{
		// TODO: block level write

		DWORD bp = m_TEX0.TBP0;
		DWORD bw = m_TEX0.TBW;

		GSLocalMemory::pixelAddress pa = GSLocalMemory::m_psm[m_TEX0.PSM].pa;

		if(m_TEX0.PSM == PSM_PSMCT32)
		{
			for(int y = r.top; y < r.bottom; y++, bits += pitch)
			{
				DWORD addr = pa(0, y, bp, bw);
				int* offset = GSLocalMemory::m_psm[m_TEX0.PSM].rowOffset[y & 7];

				for(int x = r.left, i = 0; x < r.right; x++, i++)
				{
					m_renderer->m_mem.WritePixel32(addr + offset[x], ((DWORD*)bits)[i]);
				}
			}
		}
		else if(m_TEX0.PSM == PSM_PSMCT24)
		{
			for(int y = r.top; y < r.bottom; y++, bits += pitch)
			{
				DWORD addr = pa(0, y, bp, bw);
				int* offset = GSLocalMemory::m_psm[m_TEX0.PSM].rowOffset[y & 7];

				for(int x = r.left, i = 0; x < r.right; x++, i++)
				{
					m_renderer->m_mem.WritePixel24(addr + offset[x], ((DWORD*)bits)[i]);
				}
			}
		}
		else if(m_TEX0.PSM == PSM_PSMCT16 || m_TEX0.PSM == PSM_PSMCT16S)
		{
			for(int y = r.top; y < r.bottom; y++, bits += pitch)
			{
				DWORD addr = pa(0, y, bp, bw);
				int* offset = GSLocalMemory::m_psm[m_TEX0.PSM].rowOffset[y & 7];

				for(int x = r.left, i = 0; x < r.right; x++, i++)
				{
					m_renderer->m_mem.WriteFrame16(addr + offset[x], ((DWORD*)bits)[i]);
				}
			}
		}
		else
		{
			ASSERT(0);
		}

		offscreen.Unmap();
	}

	m_renderer->m_dev.Recycle(offscreen);
}

// GSDepthStencil9

void GSTextureCache9::GSDepthStencil9::Update()
{
	__super::Update();

	// TODO
}

// GSTexture9

bool GSTextureCache9::GSTexture9::Create()
{
	// m_renderer->m_perfmon.Put(GSPerfMon::WriteTexture, 1);

	m_TEX0 = m_renderer->m_context->TEX0;
	m_CLAMP = m_renderer->m_context->CLAMP;

	DWORD psm = m_TEX0.PSM;

	switch(psm)
	{
	case PSM_PSMT8:
	case PSM_PSMT8H:
	case PSM_PSMT4:
	case PSM_PSMT4HL:
	case PSM_PSMT4HH:
		psm = m_TEX0.CPSM;
		break;
	}

	D3DFORMAT format;

	switch(psm)
	{
	default:
		TRACE(_T("Invalid TEX0.PSM/CPSM (%I64d, %I64d)\n"), m_TEX0.PSM, m_TEX0.CPSM);
	case PSM_PSMCT32:
		m_bpp = 32;
		m_bpp2 = 0;
		format = D3DFMT_A8R8G8B8;
		break;
	case PSM_PSMCT24:
		m_bpp = 32;
		m_bpp2 = 1;
		format = D3DFMT_A8R8G8B8;
		break;
	case PSM_PSMCT16:
	case PSM_PSMCT16S:
		m_bpp = 16;
		m_bpp2 = 2;
		format = D3DFMT_A1R5G5B5;
		break;
	}

	int w = 1 << m_TEX0.TW;
	int h = 1 << m_TEX0.TH;

	return m_renderer->m_dev.CreateTexture(m_texture, w, h, format);
}

bool GSTextureCache9::GSTexture9::Create(GSRenderTarget* rt)
{
	rt->Update();

	// m_renderer->m_perfmon.Put(GSPerfMon::ConvertRT2T, 1);

	m_TEX0 = m_renderer->m_context->TEX0;
	m_CLAMP = m_renderer->m_context->CLAMP;
	m_rendered = true;

	int tw = 1 << m_TEX0.TW;
	int th = 1 << m_TEX0.TH;
	int tp = (int)m_TEX0.TW << 6;

	// do not round here!!! if edge becomes a black pixel and addressing mode is clamp => everything outside the clamped area turns into black (kh2 shadows)

	int w = (int)(rt->m_texture.m_scale.x * tw);
	int h = (int)(rt->m_texture.m_scale.y * th);

	// pitch conversion

	if(rt->m_TEX0.TBW != m_TEX0.TBW) // && rt->m_TEX0.PSM == m_TEX0.PSM
	{
		// sfex3 uses this trick (bw: 10 -> 5, wraps the right side below the left)

		// ASSERT(rt->m_TEX0.TBW > m_TEX0.TBW); // otherwise scale.x need to be reduced to make the larger texture fit (TODO)

		m_renderer->m_dev.CreateRenderTarget(m_texture, rt->m_texture.GetWidth(), rt->m_texture.GetHeight());

		int bw = 64;
		int bh = m_TEX0.PSM == PSM_PSMCT32 || m_TEX0.PSM == PSM_PSMCT24 ? 32 : 64;

		int sw = (int)rt->m_TEX0.TBW << 6;

		int dw = (int)m_TEX0.TBW << 6;
		int dh = 1 << m_TEX0.TH;

		for(int dy = 0; dy < dh; dy += bh)
		{
			for(int dx = 0; dx < dw; dx += bw)
			{
				int o = dy * dw / bh + dx;

				int sx = o % sw;
				int sy = o / sw;

				GSVector4 src, dst;

				src.x = rt->m_texture.m_scale.x * sx / rt->m_texture.GetWidth();
				src.y = rt->m_texture.m_scale.y * sy / rt->m_texture.GetHeight();
				src.z = rt->m_texture.m_scale.x * (sx + bw) / rt->m_texture.GetWidth();
				src.w = rt->m_texture.m_scale.y * (sy + bh) / rt->m_texture.GetHeight();

				dst.x = rt->m_texture.m_scale.x * dx;
				dst.y = rt->m_texture.m_scale.y * dy;
				dst.z = rt->m_texture.m_scale.x * (dx + bw);
				dst.w = rt->m_texture.m_scale.y * (dy + bh);

				m_renderer->m_dev.StretchRect(rt->m_texture, src, m_texture, dst);

				// TODO: this is quite a lot of StretchRect, do it with one Draw
			}
		}
	}
	else if(tw < tp)
	{
		// FIXME: timesplitters blurs the render target by blending itself over a couple of times

		if(tw == 256 && th == 128 && tp == 512 && (m_TEX0.TBP0 == 0 || m_TEX0.TBP0 == 0x00e00))
		{
			return false;
		}
	}

	// width/height conversion

	GSVector2 scale = rt->m_texture.m_scale;

	GSVector4 dst(0, 0, w, h);
	
	if(w > rt->m_texture.GetWidth()) 
	{
		scale.x = (float)rt->m_texture.GetWidth() / tw;
		dst.z = (float)rt->m_texture.GetWidth() * scale.x / rt->m_texture.m_scale.x;
		w = rt->m_texture.GetWidth();
	}
	
	if(h > rt->m_texture.GetHeight()) 
	{
		scale.y = (float)rt->m_texture.GetHeight() / th;
		dst.w = (float)rt->m_texture.GetHeight() * scale.y / rt->m_texture.m_scale.y;
		h = rt->m_texture.GetHeight();
	}

	GSVector4 src(0, 0, w, h);

	Texture* st;
	Texture* dt;
	Texture tmp;

	if(!m_texture)
	{
		st = &rt->m_texture;
		dt = &m_texture;
	}
	else
	{
		st = &m_texture;
		dt = &tmp;
	}

	m_renderer->m_dev.CreateRenderTarget(*dt, w, h);

	if(src.x == dst.x && src.y == dst.y && src.z == dst.z && src.w == dst.w)
	{
		CRect r(0, 0, w, h);

		m_renderer->m_dev->StretchRect(*st, r, *dt, r, D3DTEXF_POINT);
	}
	else
	{
		src.z /= st->GetWidth();
		src.w /= st->GetHeight();

		m_renderer->m_dev.StretchRect(*st, src, *dt, dst);
	}

	if(tmp)
	{
		m_renderer->m_dev.Recycle(m_texture);

		m_texture = tmp;
	}

	m_texture.m_scale = scale;

	switch(m_TEX0.PSM)
	{
	case PSM_PSMCT32:
		m_bpp2 = 0;
		break;
	case PSM_PSMCT24:
		m_bpp2 = 1;
		break;
	case PSM_PSMCT16:
	case PSM_PSMCT16S:
		m_bpp2 = 2;
		break;
	case PSM_PSMT8H:
		m_bpp2 = 3;
		m_renderer->m_dev.CreateTexture(m_palette, 256, 1, m_TEX0.CPSM == PSM_PSMCT32 ? D3DFMT_A8R8G8B8 : D3DFMT_A1R5G5B5);
		m_initpalette = true;
		break;
	case PSM_PSMT4HL:
	case PSM_PSMT4HH:
		ASSERT(0); // TODO
		break;
	}

	return true;
}

bool GSTextureCache9::GSTexture9::Create(GSDepthStencil* ds)
{
	m_rendered = true;

	// TODO

	return false;
}

void GSTextureCache9::GSTexture9::Update()
{
	__super::Update();

	if(m_rendered)
	{
		return;
	}

	CRect r;

	if(!GetDirtyRect(r))
	{
		return;
	}

	m_valid |= r;

	BYTE* bits;
	int pitch;

	if(m_texture.Map(&bits, pitch, &r)) 
	{
		if(m_renderer->m_psrr)
		{
			m_renderer->m_mem.ReadTextureNPNC(r, bits, pitch, m_renderer->m_context->TEX0, m_renderer->m_env.TEXA, m_renderer->m_context->CLAMP);
		}
		else
		{
			m_renderer->m_mem.ReadTextureNP(r, bits, pitch, m_renderer->m_context->TEX0, m_renderer->m_env.TEXA, m_renderer->m_context->CLAMP);
		}

		m_texture.Unmap();
	}

	m_renderer->m_perfmon.Put(GSPerfMon::Unswizzle, r.Width() * r.Height() * m_bpp >> 3);
}
