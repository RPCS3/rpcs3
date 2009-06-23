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
#include "GSTextureCache.h"

GSTextureCache::GSTextureCache(GSRenderer* r)
	: m_renderer(r)
{
}

GSTextureCache::~GSTextureCache()
{
	RemoveAll();
}

void GSTextureCache::RemoveAll()
{
	for(list<GSRenderTarget*>::iterator i = m_rt.begin(); i != m_rt.end(); i++)
	{
		delete *i;
	}

	m_rt.clear();

	for(list<GSDepthStencil*>::iterator i = m_ds.begin(); i != m_ds.end(); i++)
	{
		delete *i;
	}

	m_ds.clear();

	for(list<GSCachedTexture*>::iterator i = m_tex.begin(); i != m_tex.end(); i++)
	{
		delete *i;
	}

	m_tex.clear();
}

GSTextureCache::GSRenderTarget* GSTextureCache::GetRenderTarget(const GIFRegTEX0& TEX0, int w, int h, bool fb)
{
	GSRenderTarget* rt = NULL;

	if(rt == NULL)
	{
		for(list<GSRenderTarget*>::iterator i = m_rt.begin(); i != m_rt.end(); i++)
		{
			GSRenderTarget* rt2 = *i;

			if(rt2->m_TEX0.TBP0 == TEX0.TBP0)
			{
				m_rt.splice(m_rt.begin(), m_rt, i);

				rt = rt2;

				if(!fb) rt->m_TEX0 = TEX0;

				rt->Update();

				break;
			}
		}
	}

	if(rt == NULL && fb)
	{
		// HACK: try to find something close to the base pointer

		for(list<GSRenderTarget*>::iterator i = m_rt.begin(); i != m_rt.end(); i++)
		{
			GSRenderTarget* rt2 = *i;

			if(rt2->m_TEX0.TBP0 <= TEX0.TBP0 && TEX0.TBP0 < rt2->m_TEX0.TBP0 + 0x700 && (!rt || rt2->m_TEX0.TBP0 >= rt->m_TEX0.TBP0))
			{
				rt = rt2;
			}
		}

		if(rt)
		{
			rt->Update();
		}
	}

	if(rt == NULL)
	{
		rt = CreateRenderTarget();

		rt->m_TEX0 = TEX0;

		if(!rt->Create(w, h))
		{
			delete rt;

			return NULL;
		}

		m_rt.push_front(rt);
	}

	if(m_renderer->CanUpscale())
	{
		GSVector4i fr = m_renderer->GetFrameRect();

		int ww = (int)(fr.left + rt->m_TEX0.TBW * 64);
		int hh = (int)(fr.top + m_renderer->GetDisplayRect().height());

		if(hh <= m_renderer->GetDeviceSize().y / 2)
		{
			hh *= 2;
		}
/*
		if(hh < 512)
		{
			hh = 512;
		}
*/
		if(ww > 0 && hh > 0)
		{
			rt->m_texture->m_scale.x = (float)w / ww;
			rt->m_texture->m_scale.y = (float)h / hh;
		}
	}

	if(!fb)
	{
		rt->m_used = true;
	}

	return rt;
}

GSTextureCache::GSDepthStencil* GSTextureCache::GetDepthStencil(const GIFRegTEX0& TEX0, int w, int h)
{
	GSDepthStencil* ds = NULL;

	if(ds == NULL)
	{
		for(list<GSDepthStencil*>::iterator i = m_ds.begin(); i != m_ds.end(); i++)
		{
			GSDepthStencil* ds2 = *i;

			if(ds2->m_TEX0.TBP0 == TEX0.TBP0)
			{
				m_ds.splice(m_ds.begin(), m_ds, i);

				ds = ds2;

				ds->m_TEX0 = TEX0;

				ds->Update();

				break;
			}
		}
	}

	if(ds == NULL)
	{
		ds = CreateDepthStencil();

		ds->m_TEX0 = TEX0;

		if(!ds->Create(w, h))
		{
			delete ds;

			return NULL;
		}

		m_ds.push_front(ds);
	}

	if(m_renderer->m_context->DepthWrite())
	{
		ds->m_used = true;
	}

	return ds;
}

GSTextureCache::GSCachedTexture* GSTextureCache::GetTexture(const GSVector4i& r)
{
	const GIFRegTEX0& TEX0 = m_renderer->m_context->TEX0;
	const GIFRegCLAMP& CLAMP = m_renderer->m_context->CLAMP;
	const GIFRegTEXA& TEXA = m_renderer->m_env.TEXA;

	const GSLocalMemory::psm_t& psm = GSLocalMemory::m_psm[TEX0.PSM];
	const uint32* clut = m_renderer->m_mem.m_clut;
	
	GSCachedTexture* t = NULL;

	for(list<GSCachedTexture*>::iterator i = m_tex.begin(); i != m_tex.end(); i++)
	{
		GSCachedTexture* t2 = *i;

		if(((t2->m_TEX0.u32[0] ^ TEX0.u32[0]) | ((t2->m_TEX0.u32[1] ^ TEX0.u32[1]) & 3)) != 0) // TBP0 TBW PSM TW TH
		{
			continue;
		}

		if((psm.trbpp == 16 || psm.trbpp == 24) && TEX0.TCC && TEXA != t2->m_TEXA)
		{
			continue;
		}

		if(psm.pal > 0 && !(t2->m_TEX0.CPSM == TEX0.CPSM && GSVector4i::compare(t2->m_clut, clut, psm.pal * sizeof(clut[0]))))
		{
			continue;
		}

		t = t2;

		m_tex.splice(m_tex.begin(), m_tex, i);

		break;
	}

	if(t == NULL)
	{
		for(list<GSRenderTarget*>::iterator i = m_rt.begin(); i != m_rt.end(); i++)
		{
			GSRenderTarget* rt = *i;

			if(rt->m_dirty.empty() && GSUtil::HasSharedBits(rt->m_TEX0.TBP0, rt->m_TEX0.PSM, TEX0.TBP0, TEX0.PSM))
			{
				t = CreateTexture();

				if(!t->Create(rt))
				{
					delete t;

					return NULL;
				}

				m_tex.push_front(t);

				break;
			}
		}
	}

	if(t == NULL)
	{
		for(list<GSDepthStencil*>::iterator i = m_ds.begin(); i != m_ds.end(); i++)
		{
			GSDepthStencil* ds = *i;

			if(ds->m_dirty.empty() && ds->m_used && GSUtil::HasSharedBits(ds->m_TEX0.TBP0, ds->m_TEX0.PSM, TEX0.TBP0, TEX0.PSM))
			{
				t = CreateTexture();

				if(!t->Create(ds))
				{
					delete t;

					return NULL;
				}

				m_tex.push_front(t);

				break;
			}
		}
	}

	if(t == NULL)
	{
		t = CreateTexture();

		if(!t->Create())
		{
			delete t;

			return NULL;
		}

		m_tex.push_front(t);
	}

	if(psm.pal > 0)
	{
		int size = psm.pal * sizeof(clut[0]);

		if(t->m_palette)
		{
			if(t->m_initpalette)
			{
				memcpy(t->m_clut, clut, size);
				t->m_palette->Update(GSVector4i(0, 0, psm.pal, 1), t->m_clut, size);
				t->m_initpalette = false;
			}
			else
			{
				if(GSVector4i::update(t->m_clut, clut, size))
				{
					t->m_palette->Update(GSVector4i(0, 0, psm.pal, 1), t->m_clut, size);
				}
			}
		}
		else
		{
			memcpy(t->m_clut, clut, size);
		}
	}

	t->Update(r);

	m_tex_used = true;

	return t;
}

void GSTextureCache::InvalidateTextures(const GIFRegFRAME& FRAME, const GIFRegZBUF& ZBUF)
{
	for(list<GSCachedTexture*>::iterator i = m_tex.begin(); i != m_tex.end(); )
	{
		list<GSCachedTexture*>::iterator j = i++;

		GSCachedTexture* t = *j;

		if(GSUtil::HasSharedBits(FRAME.Block(), FRAME.PSM, t->m_TEX0.TBP0, t->m_TEX0.PSM)
		|| GSUtil::HasSharedBits(ZBUF.Block(), ZBUF.PSM, t->m_TEX0.TBP0, t->m_TEX0.PSM))
		{
			m_tex.erase(j);

			delete t;
		}
	}
}

void GSTextureCache::InvalidateVideoMem(const GIFRegBITBLTBUF& BITBLTBUF, const GSVector4i& r)
{
	bool found = false;

	for(list<GSCachedTexture*>::iterator i = m_tex.begin(); i != m_tex.end(); )
	{
		list<GSCachedTexture*>::iterator j = i++;

		GSCachedTexture* t = *j;

		if(GSUtil::HasSharedBits(BITBLTBUF.DBP, BITBLTBUF.DPSM, t->m_TEX0.TBP0, t->m_TEX0.PSM))
		{
			if(BITBLTBUF.DBW == t->m_TEX0.TBW && !t->m_rendered)
			{
				t->m_dirty.push_back(GSDirtyRect(r, BITBLTBUF.DPSM));

				found = true;
			}
			else
			{
				m_tex.erase(j);
				delete t;
			}
		}
		else if(GSUtil::HasCompatibleBits(BITBLTBUF.DPSM, t->m_TEX0.PSM))
		{
			if(BITBLTBUF.DBW == t->m_TEX0.TBW && !t->m_rendered)
			{
				int rowsize = (int)BITBLTBUF.DBW * 8192;
				int offset = ((int)BITBLTBUF.DBP - (int)t->m_TEX0.TBP0) * 256;

				if(rowsize > 0 && offset % rowsize == 0)
				{
					int y = m_renderer->m_mem.m_psm[BITBLTBUF.DPSM].pgs.y * offset / rowsize;

					GSVector4i r2(r.left, r.top + y, r.right, r.bottom + y);

					int w = 1 << t->m_TEX0.TW;
					int h = 1 << t->m_TEX0.TH;

					if(r2.bottom > 0 && r2.top < h && r2.right > 0 && r2.left < w)
					{
						t->m_dirty.push_back(GSDirtyRect(r2, BITBLTBUF.DPSM));
					}
				}
			}
		}
	}

	for(list<GSRenderTarget*>::iterator i = m_rt.begin(); i != m_rt.end(); )
	{
		list<GSRenderTarget*>::iterator j = i++;

		GSRenderTarget* rt = *j;

		if(GSUtil::HasSharedBits(BITBLTBUF.DBP, BITBLTBUF.DPSM, rt->m_TEX0.TBP0, rt->m_TEX0.PSM))
		{
			if(!found && GSUtil::HasCompatibleBits(BITBLTBUF.DPSM, rt->m_TEX0.PSM))
			{
				rt->m_dirty.push_back(GSDirtyRect(r, BITBLTBUF.DPSM));
				rt->m_TEX0.TBW = BITBLTBUF.DBW;
			}
			else
			{
				m_rt.erase(j);
				delete rt;
				continue;
			}
		}

		if(GSUtil::HasSharedBits(BITBLTBUF.DPSM, rt->m_TEX0.PSM) && BITBLTBUF.DBP < rt->m_TEX0.TBP0)
		{
			uint32 rowsize = BITBLTBUF.DBW * 8192;
			uint32 offset = (uint32)((rt->m_TEX0.TBP0 - BITBLTBUF.DBP) * 256);

			if(rowsize > 0 && offset % rowsize == 0)
			{
				int y = m_renderer->m_mem.m_psm[BITBLTBUF.DPSM].pgs.y * offset / rowsize;

				if(r.bottom > y)
				{
					// TODO: do not add this rect above too
					rt->m_dirty.push_back(GSDirtyRect(GSVector4i(r.left, r.top - y, r.right, r.bottom - y), BITBLTBUF.DPSM));
					rt->m_TEX0.TBW = BITBLTBUF.DBW;
					continue;
				}
			}
		}
	}

	// copypaste for ds

	for(list<GSDepthStencil*>::iterator i = m_ds.begin(); i != m_ds.end(); )
	{
		list<GSDepthStencil*>::iterator j = i++;

		GSDepthStencil* ds = *j;

		if(GSUtil::HasSharedBits(BITBLTBUF.DBP, BITBLTBUF.DPSM, ds->m_TEX0.TBP0, ds->m_TEX0.PSM))
		{
			if(!found && GSUtil::HasCompatibleBits(BITBLTBUF.DPSM, ds->m_TEX0.PSM))
			{
				ds->m_dirty.push_back(GSDirtyRect(r, BITBLTBUF.DPSM));
				ds->m_TEX0.TBW = BITBLTBUF.DBW;
			}
			else
			{
				m_ds.erase(j);
				delete ds;
				continue;
			}
		}

		if(GSUtil::HasSharedBits(BITBLTBUF.DPSM, ds->m_TEX0.PSM) && BITBLTBUF.DBP < ds->m_TEX0.TBP0)
		{
			uint32 rowsize = BITBLTBUF.DBW * 8192;
			uint32 offset = (uint32)((ds->m_TEX0.TBP0 - BITBLTBUF.DBP) * 256);

			if(rowsize > 0 && offset % rowsize == 0)
			{
				int y = m_renderer->m_mem.m_psm[BITBLTBUF.DPSM].pgs.y * offset / rowsize;

				if(r.bottom > y)
				{
					// TODO: do not add this rect above too
					ds->m_dirty.push_back(GSDirtyRect(GSVector4i(r.left, r.top - y, r.right, r.bottom - y), BITBLTBUF.DPSM));
					ds->m_TEX0.TBW = BITBLTBUF.DBW;
					continue;
				}
			}
		}
	}
}

void GSTextureCache::InvalidateLocalMem(const GIFRegBITBLTBUF& BITBLTBUF, const GSVector4i& r)
{
	for(list<GSRenderTarget*>::iterator i = m_rt.begin(); i != m_rt.end(); )
	{
		list<GSRenderTarget*>::iterator j = i++;

		GSRenderTarget* rt = *j;

		if(GSUtil::HasSharedBits(BITBLTBUF.SBP, BITBLTBUF.SPSM, rt->m_TEX0.TBP0, rt->m_TEX0.PSM))
		{
			if(GSUtil::HasCompatibleBits(BITBLTBUF.SPSM, rt->m_TEX0.PSM))
			{
				rt->Read(r);
				return;
			}
			else if(BITBLTBUF.SPSM == PSM_PSMCT32 && (rt->m_TEX0.PSM == PSM_PSMCT16 || rt->m_TEX0.PSM == PSM_PSMCT16S)) 
			{
				// ffx-2 riku changing to her default (shoots some reflecting glass at the end), 16-bit rt read as 32-bit

				rt->Read(GSVector4i(r.left, r.top, r.right, r.top + (r.bottom - r.top) * 2));
				return;
			}
			else
			{
				m_rt.erase(j);
				delete rt;
				continue;
			}
		}
	}
/*
	// no good, ffx does a lot of readback after exiting menu, at 0x02f00 this wrongly finds rt 0x02100 (0,448 - 512,480)

	GSRenderTarget* rt2 = NULL;
	int ymin = INT_MAX;

	pos = m_rt.GetHeadPosition();

	while(pos)
	{
		GSRenderTarget* rt = m_rt.GetNext(pos);

		if(HasSharedBits(BITBLTBUF.SPSM, rt->m_TEX0.PSM) && BITBLTBUF.SBP > rt->m_TEX0.TBP0)
		{
			// ffx2 pause screen background

			uint32 rowsize = BITBLTBUF.SBW * 8192;
			uint32 offset = (uint32)((BITBLTBUF.SBP - rt->m_TEX0.TBP0) * 256);

			if(rowsize > 0 && offset % rowsize == 0)
			{
				int y = m_renderer->m_mem.m_psm[BITBLTBUF.SPSM].pgs.y * offset / rowsize;

				if(y < ymin && y < 512)
				{
					rt2 = rt;
					ymin = y;
				}
			}
		}
	}

	if(rt2)
	{
		rt2->Read(GSVector4i(r.left, r.top + ymin, r.right, r.bottom + ymin));
	}

	// TODO: ds
*/
}

void GSTextureCache::IncAge()
{
	RecycleByAge(m_tex, m_tex_used ? 2 : 30);
	RecycleByAge(m_rt);
	RecycleByAge(m_ds);

	m_tex_used = false;
}

// GSTextureCache::GSSurface

GSTextureCache::GSSurface::GSSurface(GSRenderer* r)
	: m_renderer(r)
	, m_texture(NULL)
	, m_palette(NULL)
	, m_initpalette(false)
	, m_age(0)
{
	m_TEX0.TBP0 = (uint32)~0;
}

GSTextureCache::GSSurface::~GSSurface()
{
	m_renderer->m_dev->Recycle(m_texture);
	m_renderer->m_dev->Recycle(m_palette);

	m_texture = NULL;
	m_palette = NULL;
}

void GSTextureCache::GSSurface::Update()
{
	m_age = 0;
}

// GSTextureCache::GSRenderTarget

GSTextureCache::GSRenderTarget::GSRenderTarget(GSRenderer* r)
	: GSSurface(r)
	, m_used(true)
{
}

bool GSTextureCache::GSRenderTarget::Create(int w, int h)
{
	// FIXME: initial data should be unswizzled from local mem in Update() if dirty

	m_texture = m_renderer->m_dev->CreateRenderTarget(w, h);

	return m_texture != NULL;
}

void GSTextureCache::GSRenderTarget::Update()
{
	__super::Update();

	// FIXME: the union of the rects may also update wrong parts of the render target (but a lot faster :)

	GSVector4i r = m_dirty.GetDirtyRectAndClear(m_TEX0, m_texture->GetSize());

	if(r.rempty()) return;

	int w = r.width();
	int h = r.height();

	if(GSTexture* t = m_renderer->m_dev->CreateTexture(w, h))
	{
		GIFRegTEXA TEXA;

		TEXA.AEM = 1;
		TEXA.TA0 = 0;
		TEXA.TA1 = 0x80;

		GSTexture::GSMap m;

		if(t->Map(m))
		{
			m_renderer->m_mem.ReadTexture(r, m.bits,  m.pitch, m_TEX0, TEXA);

			t->Unmap();
		}
		else
		{
			static uint8* buff = (uint8*)::_aligned_malloc(1024 * 1024 * 4, 16);
			
			int pitch = ((w + 3) & ~3) * 4;

			m_renderer->m_mem.ReadTexture(r, buff, pitch, m_TEX0, TEXA);
			
			t->Update(r.rsize(), buff, pitch);
		}

		// m_renderer->m_perfmon.Put(GSPerfMon::Unswizzle, w * h * 4);

		m_renderer->m_dev->StretchRect(t, m_texture, GSVector4(r) * GSVector4(m_texture->m_scale).xyxy());

		m_renderer->m_dev->Recycle(t);
	}
}

// GSTextureCache::GSDepthStencil

GSTextureCache::GSDepthStencil::GSDepthStencil(GSRenderer* r)
	: GSSurface(r)
	, m_used(false)
{
}

bool GSTextureCache::GSDepthStencil::Create(int w, int h)
{
	// FIXME: initial data should be unswizzled from local mem in Update() if dirty

	m_texture = m_renderer->m_dev->CreateDepthStencil(w, h);

	return m_texture != NULL;
}

void GSTextureCache::GSDepthStencil::Update()
{
	__super::Update();

	GSVector4i r = m_dirty.GetDirtyRectAndClear(m_TEX0, m_texture->GetSize());

	if(r.rempty()) return;

	// TODO

	m_renderer->m_dev->ClearDepth(m_texture, 0);
}

// GSTextureCache::GSCachedTexture

GSTextureCache::GSCachedTexture::GSCachedTexture(GSRenderer* r)
	: GSSurface(r)
	, m_bpp(0)
	, m_rendered(false)
{
	m_valid = GSVector4i::zero();

	m_clut = (uint32*)_aligned_malloc(256 * sizeof(uint32), 16);

	memset(m_clut, 0, sizeof(m_clut));
}

GSTextureCache::GSCachedTexture::~GSCachedTexture()
{
	_aligned_free(m_clut);
}

void GSTextureCache::GSCachedTexture::Update(const GSVector4i& rect)
{
	__super::Update();

	if(m_rendered)
	{
		return;
	}

	GSVector4i r = rect;

	if(!GetDirtyRect(r))
	{
		return;
	}

	m_valid = m_valid.runion(r);

	GSTexture::GSMap m;

	if(m_texture->Map(m, &r))
	{
		// in dx9 managed textures can be written directly, less copying is faster, but still not as fast as dx10's UpdateResource

		m_renderer->m_mem.ReadTexture(r, m.bits, m.pitch, m_renderer->m_context->TEX0, m_renderer->m_env.TEXA);

		m_texture->Unmap();
	}
	else
	{
		static uint8* buff = (uint8*)::_aligned_malloc(1024 * 1024 * 4, 16);
		
		int pitch = ((r.width() + 3) & ~3) * 4;

		m_renderer->m_mem.ReadTexture(r, buff, pitch, m_renderer->m_context->TEX0, m_renderer->m_env.TEXA);

		m_texture->Update(r, buff, pitch);
	}

	m_renderer->m_perfmon.Put(GSPerfMon::Unswizzle, r.width() * r.height() * 4);
}

bool GSTextureCache::GSCachedTexture::GetDirtyRect(GSVector4i& r)
{
	int w = 1 << m_TEX0.TW;
	int h = 1 << m_TEX0.TH;

	GSVector4i tr(0, 0, w, h);

	for(list<GSDirtyRect>::iterator i = m_dirty.begin(); i != m_dirty.end(); i++)
	{
		const GSVector4i& dirty = i->GetDirtyRect(m_TEX0).rintersect(tr);

		if(!m_valid.rintersect(dirty).rempty())
		{
			 // find the rect having the largest area, outside dirty, inside m_valid

			GSVector4i left(m_valid.left, m_valid.top, min(m_valid.right, dirty.left), m_valid.bottom);
			GSVector4i top(m_valid.left, m_valid.top, m_valid.right, min(m_valid.bottom, dirty.top));
			GSVector4i right(max(m_valid.left, dirty.right), m_valid.top, m_valid.right, m_valid.bottom);
			GSVector4i bottom(m_valid.left, max(m_valid.top, dirty.bottom), m_valid.right, m_valid.bottom);

			int leftsize = !left.rempty() ? left.width() * left.height() : 0;
			int topsize = !top.rempty() ? top.width() * top.height() : 0;
			int rightsize = !right.rempty() ? right.width() * right.height() : 0;
			int bottomsize = !bottom.rempty() ? bottom.width() * bottom.height() : 0;

			// TODO: sort

			m_valid = 
				leftsize > 0 ? left : 
				topsize > 0 ? top : 
				rightsize > 0 ? right : 
				bottomsize > 0 ? bottom : 
				GSVector4i::zero();
		}
	}

	m_dirty.clear();

	if(GSUtil::IsRectInRect(r, m_valid))
	{
		return false;
	}
	else if(GSUtil::IsRectInRectH(r, m_valid) && (r.left >= m_valid.left || r.right <= m_valid.right))
	{
		r.top = m_valid.top;
		r.bottom = m_valid.bottom;
		if(r.left < m_valid.left) r.right = m_valid.left;
		else r.left = m_valid.right; // if(r.right > m_valid.right)
	}
	else if(GSUtil::IsRectInRectV(r, m_valid) && (r.top >= m_valid.top || r.bottom <= m_valid.bottom))
	{
		r.left = m_valid.left;
		r.right = m_valid.right;
		if(r.top < m_valid.top) r.bottom = m_valid.top;
		else r.top = m_valid.bottom; // if(r.bottom > m_valid.bottom)
	}
	else
	{
		r = r.runion(m_valid);
	}

	if(r.rempty())
	{
		return false;
	}

	return true;
}