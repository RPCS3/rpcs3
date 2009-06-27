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
	m_src.RemoveAll();

	for(int type = 0; type < 2; type++)
	{
		for(list<Target*>::iterator i = m_dst[type].begin(); i != m_dst[type].end(); i++)
		{
			delete *i;
		}

		m_dst[type].clear();
	}
}

GSTextureCache::Source* GSTextureCache::LookupSource(const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA, const GSVector4i& r)
{
	const GSLocalMemory::psm_t& psm = GSLocalMemory::m_psm[TEX0.PSM];
	const uint32* clut = m_renderer->m_mem.m_clut;
	
	Source* src = NULL;

	const hash_map<Source*, bool>& map = m_src.m_map[TEX0.TBP0 >> 5];

	for(hash_map<Source*, bool>::const_iterator i = map.begin(); i != map.end(); i++)
	{
		Source* s = i->first;

		if(((s->m_TEX0.u32[0] ^ TEX0.u32[0]) | ((s->m_TEX0.u32[1] ^ TEX0.u32[1]) & 3)) != 0) // TBP0 TBW PSM TW TH
		{
			continue;
		}

		if((psm.trbpp == 16 || psm.trbpp == 24) && TEX0.TCC && TEXA != s->m_TEXA)
		{
			continue;
		}

		if(psm.pal > 0 && !GSVector4i::compare(s->m_clut, clut, psm.pal * sizeof(clut[0])))
		{
			continue;
		}

		src = s;

		break;
	}

	Target* dst = NULL;

	if(src == NULL)
	{
		for(int type = 0; type < 2 && dst == NULL; type++)
		{
			for(list<Target*>::iterator i = m_dst[type].begin(); i != m_dst[type].end(); i++)
			{
				Target* t = *i;

				if(t->m_used && t->m_dirty.empty() && GSUtil::HasSharedBits(t->m_TEX0.TBP0, t->m_TEX0.PSM, TEX0.TBP0, TEX0.PSM))
				{
					dst = t;

					break;
				}
			}
		}
	}

	if(src == NULL)
	{
		src = CreateSource();

		if(!(dst ? src->Create(dst) : src->Create()))
		{
			delete src;

			return NULL;
		}

		if(psm.pal > 0)
		{
			memcpy(src->m_clut, clut, psm.pal * sizeof(clut[0]));
		}

		m_src.Add(src, TEX0);
	}

	if(psm.pal > 0)
	{
		int size = psm.pal * sizeof(clut[0]);

		if(src->m_palette)
		{
			if(src->m_initpalette || GSVector4i::update(src->m_clut, clut, size))
			{
				src->m_palette->Update(GSVector4i(0, 0, psm.pal, 1), src->m_clut, size);
				src->m_initpalette = false;
			}
		}
	}

	src->Update(TEX0, TEXA, r);

	m_src.m_used = true;

	return src;
}

GSTextureCache::Target* GSTextureCache::LookupTarget(const GIFRegTEX0& TEX0, int w, int h, int type, bool used, bool fb)
{
	Target* dst = NULL;

	for(list<Target*>::iterator i = m_dst[type].begin(); i != m_dst[type].end(); i++)
	{
		Target* t = *i;

		if(t->m_TEX0.TBP0 == TEX0.TBP0)
		{
			m_dst[type].splice(m_dst[type].begin(), m_dst[type], i);

			dst = t;

			if(!fb) dst->m_TEX0 = TEX0;

			break;
		}
	}

	if(dst == NULL && fb)
	{
		// HACK: try to find something close to the base pointer

		for(list<Target*>::iterator i = m_dst[type].begin(); i != m_dst[type].end(); i++)
		{
			Target* t = *i;

			if(t->m_TEX0.TBP0 <= TEX0.TBP0 && TEX0.TBP0 < t->m_TEX0.TBP0 + 0x700 && (!dst || t->m_TEX0.TBP0 >= dst->m_TEX0.TBP0))
			{
				dst = t;
			}
		}
	}

	if(dst == NULL)
	{
		dst = CreateTarget();

		dst->m_TEX0 = TEX0;

		if(!dst->Create(w, h, type))
		{
			delete dst;

			return NULL;
		}

		m_dst[type].push_front(dst);
	}
	else
	{
		dst->Update();
	}

	if(m_renderer->CanUpscale())
	{
		GSVector4i fr = m_renderer->GetFrameRect();

		int ww = (int)(fr.left + dst->m_TEX0.TBW * 64);
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
			dst->m_texture->m_scale.x = (float)w / ww;
			dst->m_texture->m_scale.y = (float)h / hh;
		}
	}

	if(used)
	{
		dst->m_used = true;
	}

	return dst;
}

void GSTextureCache::InvalidateVideoMem(const GIFRegBITBLTBUF& BITBLTBUF, const GSVector4i& rect, bool target)
{
	bool found = false;

	const GSLocalMemory::psm_t& psm = GSLocalMemory::m_psm[BITBLTBUF.DPSM];

	uint32 bp = BITBLTBUF.DBP;
	uint32 bw = BITBLTBUF.DBW;

	GSVector2i bs = (bp & 31) == 0 ? psm.pgs : psm.bs;

	GSVector4i r = rect.ralign<GSVector4i::Outside>(bs);

	if(!target)
	{
		const hash_map<Source*, bool>& map = m_src.m_map[bp >> 5];

		for(hash_map<Source*, bool>::const_iterator i = map.begin(); i != map.end(); )
		{
			hash_map<Source*, bool>::const_iterator j = i++;

			Source* s = j->first;

			if(GSUtil::HasSharedBits(bp, BITBLTBUF.DPSM, s->m_TEX0.TBP0, s->m_TEX0.PSM))
			{
				m_src.RemoveAt(s);
			}
		}
	}

	for(int y = r.top; y < r.bottom; y += bs.y)
	{
		uint32 base = psm.bn(0, y, bp, bw);

		for(int x = r.left; x < r.right; x += bs.x)
		{
			uint32 page = (base + psm.blockOffset[x >> 3]) >> 5;

			if(page < MAX_PAGES)
			{
				const hash_map<Source*, bool>& map = m_src.m_map[page];

				for(hash_map<Source*, bool>::const_iterator i = map.begin(); i != map.end(); )
				{
					hash_map<Source*, bool>::const_iterator j = i++;

					Source* s = j->first;

					if(GSUtil::HasSharedBits(BITBLTBUF.DPSM, s->m_TEX0.PSM))
					{
						if(!s->m_target)
						{
							s->m_valid[page] = 0;

							found = true;
						}
						else
						{
							if(s->m_TEX0.TBP0 == bp)
							{
								m_src.RemoveAt(s);
							}
						}
					}
				}
			}
		}
	}

	if(!target) return;

	for(int type = 0; type < 2; type++)
	{
		for(list<Target*>::iterator i = m_dst[type].begin(); i != m_dst[type].end(); )
		{
			list<Target*>::iterator j = i++;

			Target* t = *j;

			if(GSUtil::HasSharedBits(BITBLTBUF.DBP, BITBLTBUF.DPSM, t->m_TEX0.TBP0, t->m_TEX0.PSM))
			{
				if(!found && GSUtil::HasCompatibleBits(BITBLTBUF.DPSM, t->m_TEX0.PSM))
				{
					t->m_dirty.push_back(GSDirtyRect(r, BITBLTBUF.DPSM));
					t->m_TEX0.TBW = BITBLTBUF.DBW;
				}
				else
				{
					m_dst[type].erase(j);
					delete t;
					continue;
				}
			}

			if(GSUtil::HasSharedBits(BITBLTBUF.DPSM, t->m_TEX0.PSM) && BITBLTBUF.DBP < t->m_TEX0.TBP0)
			{
				uint32 rowsize = BITBLTBUF.DBW * 8192;
				uint32 offset = (uint32)((t->m_TEX0.TBP0 - BITBLTBUF.DBP) * 256);

				if(rowsize > 0 && offset % rowsize == 0)
				{
					int y = GSLocalMemory::m_psm[BITBLTBUF.DPSM].pgs.y * offset / rowsize;

					if(r.bottom > y)
					{
						// TODO: do not add this rect above too
						t->m_dirty.push_back(GSDirtyRect(GSVector4i(r.left, r.top - y, r.right, r.bottom - y), BITBLTBUF.DPSM));
						t->m_TEX0.TBW = BITBLTBUF.DBW;
						continue;
					}
				}
			}
		}
	}
}

void GSTextureCache::InvalidateLocalMem(const GIFRegBITBLTBUF& BITBLTBUF, const GSVector4i& r)
{
	for(list<Target*>::iterator i = m_dst[RenderTarget].begin(); i != m_dst[RenderTarget].end(); )
	{
		list<Target*>::iterator j = i++;

		Target* t = *j;

		if(GSUtil::HasSharedBits(BITBLTBUF.SBP, BITBLTBUF.SPSM, t->m_TEX0.TBP0, t->m_TEX0.PSM))
		{
			if(GSUtil::HasCompatibleBits(BITBLTBUF.SPSM, t->m_TEX0.PSM))
			{
				t->Read(r);

				return;
			}
			else if(BITBLTBUF.SPSM == PSM_PSMCT32 && (t->m_TEX0.PSM == PSM_PSMCT16 || t->m_TEX0.PSM == PSM_PSMCT16S)) 
			{
				// ffx-2 riku changing to her default (shoots some reflecting glass at the end), 16-bit rt read as 32-bit

				t->Read(GSVector4i(r.left, r.top, r.right, r.top + (r.bottom - r.top) * 2));

				return;
			}
			else
			{
				m_dst[RenderTarget].erase(j);

				delete t;
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
				int y = GSLocalMemory::m_psm[BITBLTBUF.SPSM].pgs.y * offset / rowsize;

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
	int maxage = m_src.m_used ? 3 : 30;

	for(hash_map<Source*, bool>::iterator i = m_src.m_surfaces.begin(); i != m_src.m_surfaces.end(); )
	{
		hash_map<Source*, bool>::iterator j = i++;

		Source* s = j->first;

		if(++s->m_age > maxage)
		{
			m_src.RemoveAt(s);
		}
	}

	m_src.m_used = false;

	maxage = 3;

	for(int type = 0; type < 2; type++)
	{
		for(list<Target*>::iterator i = m_dst[type].begin(); i != m_dst[type].end(); )
		{
			list<Target*>::iterator j = i++;

			Target* t = *j;

			if(++t->m_age > maxage)
			{
				m_dst[type].erase(j);

				delete t;
			}
		}
	}
}

// GSTextureCache::Surface

GSTextureCache::Surface::Surface(GSRenderer* r)
	: m_renderer(r)
	, m_texture(NULL)
	, m_age(0)
{
	m_TEX0.TBP0 = (uint32)~0;
}

GSTextureCache::Surface::~Surface()
{
	m_renderer->m_dev->Recycle(m_texture);
}

void GSTextureCache::Surface::Update()
{
	m_age = 0;
}

// GSTextureCache::Source

GSTextureCache::Source::Source(GSRenderer* r)
	: Surface(r)
	, m_palette(NULL)
	, m_initpalette(false)
	, m_bpp(0)
	, m_target(false)
{
	memset(m_valid, 0, sizeof(m_valid));

	m_clut = (uint32*)_aligned_malloc(256 * sizeof(uint32), 16);

	memset(m_clut, 0, sizeof(m_clut));
}

GSTextureCache::Source::~Source()
{
	m_renderer->m_dev->Recycle(m_palette);

	_aligned_free(m_clut);
}

void GSTextureCache::Source::Update(const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA, const GSVector4i& rect)
{
	__super::Update();

	if(m_target)
	{
		return;
	}

	m_TEX0 = TEX0;
	m_TEXA = TEXA;

	const GSLocalMemory::psm_t& psm = GSLocalMemory::m_psm[m_TEX0.PSM];

	GSVector2i s = psm.bs;

	int tw = 1 << m_TEX0.TW;
	int th = 1 << m_TEX0.TH;

	GSVector4i tr(0, 0, tw, th);

	GSVector4i r = rect.ralign<GSVector4i::Outside>(s);

	uint32 bp = m_TEX0.TBP0;
	uint32 bw = m_TEX0.TBW;

	uint32 blocks = 0;

	// TODO
	static uint8* buff = (uint8*)_aligned_malloc(1024 * 16 * sizeof(uint32), 16); // max decompressed size for a row of blocks (1024 x 16, 4bpp)
	
	int pitch = max(tw, s.x) * sizeof(uint32);

	GSLocalMemory::readTexture rtx = psm.rtx;

	const GSLocalMemory& mem = m_renderer->m_mem;

	// TODO: bw == 0 (sfex)

	if(tw <= (bw << 6))
	{
		// r.right = min(r.right, bw << 6);

		for(int y = r.top; y < r.bottom; y += s.y)
		{
			uint32 base = psm.bn(0, y, bp, bw);

			int left = r.left;
			int right = r.left;

			for(int x = r.left; x < r.right; x += s.x)
			{
				uint32 block = base + psm.blockOffset[x >> 3];

				if(block < MAX_BLOCKS)
				{
					uint32 row = block >> 5;
					uint32 col = 1 << (block & 31);

					if((m_valid[row] & col) == 0)
					{
						m_valid[row] |= col;

						if(right < x)
						{
							Write(GSVector4i(left, y, right, y + s.y), tr, buff, pitch);

							left = right = x;
						}

						right += s.x;

						blocks++;
					}
				}
			}

			if(left < right)
			{
				Write(GSVector4i(left, y, right, y + s.y), tr, buff, pitch);
			}
		}
	}
	else
	{
		// unfortunatelly a block may be part of the same texture multiple times at different places (tw 1024 > tbw 640, between 640 -> 1024 it is repeated from the next row), 
		// so just can't set the block's bit to valid in one pass, even if 99.9% of the games don't address the repeated part at the right side
		
		// TODO: still bogus if those repeated parts aren't fetched together

		for(int y = r.top; y < r.bottom; y += s.y)
		{
			uint32 base = psm.bn(0, y, bp, bw);

			int left = r.left;
			int right = r.left;

			for(int x = r.left; x < r.right; x += s.x)
			{
				uint32 block = base + psm.blockOffset[x >> 3];

				if(block < MAX_BLOCKS)
				{
					uint32 row = block >> 5;
					uint32 col = 1 << (block & 31);

					if((m_valid[row] & col) == 0)
					{
						if(right < x)
						{
							Write(GSVector4i(left, y, right, y + s.y), tr, buff, pitch);

							left = right = x;
						}

						right += s.x;

						blocks++;
					}
				}
			}

			if(left < right)
			{
				Write(GSVector4i(left, y, right, y + s.y), tr, buff, pitch);
			}
		}

		for(int y = r.top; y < r.bottom; y += s.y)
		{
			uint32 base = psm.bn(0, y, bp, bw);

			for(int x = r.left; x < r.right; x += s.x)
			{
				uint32 block = base + psm.blockOffset[x >> 3];

				if(block < MAX_BLOCKS)
				{
					uint32 row = block >> 5;
					uint32 col = 1 << (block & 31);

					m_valid[row] |= col;
				}
			}
		}
	}

	//_aligned_free(buff);

	m_renderer->m_perfmon.Put(GSPerfMon::Unswizzle, s.x * s.y * sizeof(uint32) * blocks);
}

void GSTextureCache::Source::Write(const GSVector4i& r, const GSVector4i& tr, uint8* buff, int pitch)
{
	if(r.rempty()) return;

	GSLocalMemory::readTexture rtx = GSLocalMemory::m_psm[m_TEX0.PSM].rtx;

	if((r > tr).mask() & 0xff00)
	{
		(m_renderer->m_mem.*rtx)(r, buff, pitch, m_TEX0, m_TEXA);

		m_texture->Update(r.rintersect(tr), buff, pitch);
	}
	else
	{
		GSTexture::GSMap m;

		if(m_texture->Map(m, &r))
		{
			(m_renderer->m_mem.*rtx)(r, m.bits, m.pitch, m_TEX0, m_TEXA);

			m_texture->Unmap();
		}
		else
		{
			(m_renderer->m_mem.*rtx)(r, buff, pitch, m_TEX0, m_TEXA);

			m_texture->Update(r, buff, pitch);
		}
	}
}

// GSTextureCache::Target

GSTextureCache::Target::Target(GSRenderer* r)
	: Surface(r)
	, m_type(-1)
	, m_used(false)
{
}

bool GSTextureCache::Target::Create(int w, int h, int type)
{
	ASSERT(m_texture == NULL);

	// FIXME: initial data should be unswizzled from local mem in Update() if dirty

	m_type = type;

	if(type == RenderTarget)
	{
		m_texture = m_renderer->m_dev->CreateRenderTarget(w, h);

		m_used = true;
	}
	else if(type == DepthStencil)
	{
		m_texture = m_renderer->m_dev->CreateDepthStencil(w, h);
	}

	return m_texture != NULL;
}

void GSTextureCache::Target::Update()
{
	__super::Update();

	// FIXME: the union of the rects may also update wrong parts of the render target (but a lot faster :)

	GSVector4i r = m_dirty.GetDirtyRectAndClear(m_TEX0, m_texture->GetSize());

	if(r.rempty()) return;

	if(m_type == RenderTarget)
	{
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
	else if(m_type == DepthStencil)
	{
		// do the most likely thing a direct write would do, clear it

		m_renderer->m_dev->ClearDepth(m_texture, 0);
	}
}

// GSTextureCache::SourceMap

void GSTextureCache::SourceMap::Add(Source* s, const GIFRegTEX0& TEX0)
{
	m_surfaces[s] = true;

	int tw = 1 << TEX0.TW;
	int th = 1 << TEX0.TH;

	uint32 bp = TEX0.TBP0;
	uint32 bw = TEX0.TBW;

	const GSLocalMemory::psm_t& psm = GSLocalMemory::m_psm[TEX0.PSM];

	GSVector2i bs = (bp & 31) == 0 ? psm.pgs : psm.bs;

	int blocks = 0;

	for(int y = 0; y < th; y += bs.y)
	{
		uint32 base = psm.bn(0, y, bp, bw);

		for(int x = 0; x < tw; x += bs.x)
		{
			uint32 page = (base + psm.blockOffset[x >> 3]) >> 5;

			if(page < MAX_PAGES)
			{
				m_map[page][s] = true;

				s->m_pages.push_back(page);
			}
		}
	}
}

void GSTextureCache::SourceMap::RemoveAll()
{
	for(hash_map<Source*, bool>::iterator i = m_surfaces.begin(); i != m_surfaces.end(); i++)
	{
		delete i->first;
	}

	m_surfaces.clear();

	for(int i = 0; i < MAX_PAGES; i++)
	{
		m_map[i].clear();
	}
}

void GSTextureCache::SourceMap::RemoveAt(Source* s)
{
	m_surfaces.erase(s);

	for(list<int>::iterator i = s->m_pages.begin(); i != s->m_pages.end(); i++)
	{
		m_map[*i].erase(s);
	}

	delete s;
}
