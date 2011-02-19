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

#include "stdafx.h"
#include "GSTextureCacheSW.h"

GSTextureCacheSW::GSTextureCacheSW(GSState* state)
	: m_state(state)
{
	memset(m_pages, 0, sizeof(m_pages));
}

GSTextureCacheSW::~GSTextureCacheSW()
{
	RemoveAll();
}

const GSTextureCacheSW::GSTexture* GSTextureCacheSW::Lookup(const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA, const GSVector4i& r)
{
	const GSLocalMemory::psm_t& psm = GSLocalMemory::m_psm[TEX0.PSM];

	GSTexture* t = NULL;

	list<GSTexture*>& m = m_map[TEX0.TBP0 >> 5];

	for(list<GSTexture*>::iterator i = m.begin(); i != m.end(); i++)
	{
		GSTexture* t2 = *i;

		if(((TEX0.u32[0] ^ t2->m_TEX0.u32[0]) | ((TEX0.u32[1] ^ t2->m_TEX0.u32[1]) & 3)) != 0) // TBP0 TBW PSM TW TH
		{
			continue;
		}

		if((psm.trbpp == 16 || psm.trbpp == 24) && TEX0.TCC && TEXA != t2->m_TEXA)
		{
			continue;
		}

		m.splice(m.begin(), m, i);

		t = t2;

		t->m_age = 0;

		break;
	}

	if(t == NULL)
	{
		t = new GSTexture(m_state);

		m_textures.insert(t);

		const GSOffset* o = m_state->m_context->offset.tex;

		GSVector2i bs = (TEX0.TBP0 & 31) == 0 ? psm.pgs : psm.bs;

		int tw = 1 << TEX0.TW;
		int th = 1 << TEX0.TH;

		for(int y = 0; y < th; y += bs.y)
		{
			uint32 base = o->block.row[y >> 3];

			for(int x = 0; x < tw; x += bs.x)
			{
				uint32 page = (base + o->block.col[x >> 3]) >> 5;

				if(page < MAX_PAGES)
				{
					m_pages[page >> 5] |= 1 << (page & 31);
				}
			}
		}

		for(int i = 0; i < countof(m_pages); i++)
		{
			if(uint32 p = m_pages[i])
			{
				m_pages[i] = 0;

				list<GSTexture*>* m = &m_map[i << 5];

				unsigned long j;

				while(_BitScanForward(&j, p))
				{
					p ^= 1 << j;

					m[j].push_front(t);
				}
			}
		}
	}

	if(!t->Update(TEX0, TEXA, r))
	{
		printf("!@#$\n"); // memory allocation may fail if the game is too hungry

		RemoveAt(t);

		t = NULL;
	}

	return t;
}

void GSTextureCacheSW::InvalidateVideoMem(const GSOffset* o, const GSVector4i& rect)
{
	uint32 bp = o->bp;
	uint32 bw = o->bw;
	uint32 psm = o->psm;

	GSVector2i bs = (bp & 31) == 0 ? GSLocalMemory::m_psm[psm].pgs : GSLocalMemory::m_psm[psm].bs;

	GSVector4i r = rect.ralign<GSVector4i::Outside>(bs);

	for(int y = r.top; y < r.bottom; y += bs.y)
	{
		uint32 base = o->block.row[y >> 3];

		for(int x = r.left; x < r.right; x += bs.x)
		{
			uint32 page = (base + o->block.col[x >> 3]) >> 5;

			if(page < MAX_PAGES)
			{
				const list<GSTexture*>& map = m_map[page];

				for(list<GSTexture*>::const_iterator i = map.begin(); i != map.end(); i++)
				{
					GSTexture* t = *i;

					if(GSUtil::HasSharedBits(psm, t->m_TEX0.PSM))
					{
						t->m_valid[page] = 0;
						t->m_complete = false;
					}
				}
			}
		}
	}
}

void GSTextureCacheSW::RemoveAll()
{
	for_each(m_textures.begin(), m_textures.end(), delete_object());

	m_textures.clear();

	for(int i = 0; i < MAX_PAGES; i++)
	{
		m_map[i].clear();
	}
}

void GSTextureCacheSW::RemoveAt(GSTexture* t)
{
	m_textures.erase(t);

	for(uint32 start = t->m_TEX0.TBP0 >> 5, end = countof(m_map) - 1; start <= end; start++)
	{
		list<GSTexture*>& m = m_map[start];

		for(list<GSTexture*>::iterator i = m.begin(); i != m.end(); )
		{
			list<GSTexture*>::iterator j = i++;

			if(*j == t) {m.erase(j); break;}
		}
	}

	delete t;
}

void GSTextureCacheSW::IncAge()
{
	for(hash_set<GSTexture*>::iterator i = m_textures.begin(); i != m_textures.end(); )
	{
		hash_set<GSTexture*>::iterator j = i++;

		GSTexture* t = *j;

		if(++t->m_age > 30)
		{
			RemoveAt(t);
		}
	}
}

//

GSTextureCacheSW::GSTexture::GSTexture(GSState* state)
	: m_state(state)
	, m_buff(NULL)
	, m_tw(0)
	, m_age(0)
	, m_complete(false)
{
	memset(m_valid, 0, sizeof(m_valid));
}

GSTextureCacheSW::GSTexture::~GSTexture()
{
	if(m_buff)
	{
		_aligned_free(m_buff);
	}
}

bool GSTextureCacheSW::GSTexture::Update(const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA, const GSVector4i& rect)
{
	if(m_complete)
	{
		return true;
	}

	m_TEX0 = TEX0;
	m_TEXA = TEXA;

	const GSLocalMemory::psm_t& psm = GSLocalMemory::m_psm[TEX0.PSM];

	GSVector2i bs = psm.bs;

	int tw = std::max<int>(1 << TEX0.TW, bs.x);
	int th = std::max<int>(1 << TEX0.TH, bs.y);

	GSVector4i r = rect.ralign<GSVector4i::Outside>(bs);

	if(r.eq(GSVector4i(0, 0, tw, th)))
	{
		m_complete = true; // lame, but better than nothing
	}

	if(m_buff == NULL)
	{
		m_buff = _aligned_malloc(tw * th * sizeof(uint32), 32);

		if(m_buff == NULL)
		{
			return false;
		}

		m_tw = std::max<int>(TEX0.TW, psm.pal > 0 ? 5 : 3); // makes one row 32 bytes at least, matches the smallest block size that is allocated above for m_buff
	}

	GSLocalMemory& mem = m_state->m_mem;

	const GSOffset* o = m_state->m_context->offset.tex;

	bool repeating = m_TEX0.IsRepeating();

	uint32 blocks = 0;

	GSLocalMemory::readTextureBlock rtxb = psm.rtxbP;

	int shift = psm.pal == 0 ? 2 : 0;

	uint32 pitch = (1 << m_tw) << shift;

	uint8* dst = (uint8*)m_buff + pitch * r.top;

	for(int y = r.top, block_pitch = pitch * bs.y; y < r.bottom; y += bs.y, dst += block_pitch)
	{
		uint32 base = o->block.row[y >> 3];

		for(int x = r.left; x < r.right; x += bs.x)
		{
			uint32 block = base + o->block.col[x >> 3];

			if(block < MAX_BLOCKS)
			{
				uint32 row = block >> 5;
				uint32 col = 1 << (block & 31);

				if((m_valid[row] & col) == 0)
				{
					if(!repeating)
					{
						m_valid[row] |= col;
					}

					(mem.*rtxb)(block, &dst[x << shift], pitch, TEXA);

					blocks++;
				}
			}
		}
	}

	if(blocks > 0)
	{
		if(repeating)
		{
			for(int y = r.top; y < r.bottom; y += bs.y)
			{
				uint32 base = o->block.row[y >> 3];

				for(int x = r.left; x < r.right; x += bs.x)
				{
					uint32 block = base + o->block.col[x >> 3];

					if(block < MAX_BLOCKS)
					{
						uint32 row = block >> 5;
						uint32 col = 1 << (block & 31);

						m_valid[row] |= col;
					}
				}
			}
		}

		m_state->m_perfmon.Put(GSPerfMon::Unswizzle, bs.x * bs.y * blocks << shift);
	}

	return true;
}
