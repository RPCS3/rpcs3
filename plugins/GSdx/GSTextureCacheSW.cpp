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
}

GSTextureCacheSW::~GSTextureCacheSW()
{
	RemoveAll();
}

const GSTextureCacheSW::Texture* GSTextureCacheSW::Lookup(const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA, const GSVector4i& r, uint32 tw0)
{
	const GSLocalMemory::psm_t& psm = GSLocalMemory::m_psm[TEX0.PSM];

	Texture* t = NULL;

	list<Texture*>& m = m_map[TEX0.TBP0 >> 5];

	for(list<Texture*>::iterator i = m.begin(); i != m.end(); i++)
	{
		Texture* t2 = *i;

		if(((TEX0.u32[0] ^ t2->m_TEX0.u32[0]) | ((TEX0.u32[1] ^ t2->m_TEX0.u32[1]) & 3)) != 0) // TBP0 TBW PSM TW TH
		{
			continue;
		}

		if((psm.trbpp == 16 || psm.trbpp == 24) && TEX0.TCC && TEXA != t2->m_TEXA)
		{
			continue;
		}

		if(tw0 != 0 && t2->m_tw != tw0)
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
		const GSOffset* o = m_state->m_mem.GetOffset(TEX0.TBP0, TEX0.TBW, TEX0.PSM);

		t = new Texture(m_state, o, tw0, TEX0, TEXA);

		m_textures.insert(t);

		__aligned(uint32, 16) pages[16];

		((GSVector4i*)pages)[0] = GSVector4i::zero();
		((GSVector4i*)pages)[1] = GSVector4i::zero();
		((GSVector4i*)pages)[2] = GSVector4i::zero();
		((GSVector4i*)pages)[3] = GSVector4i::zero();

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
					pages[page >> 5] |= 1 << (page & 31);
				}
			}
		}

		for(int i = 0; i < countof(pages); i++)
		{
			uint32 p = pages[i];

			if(p != 0)
			{
				list<Texture*>* m = &m_map[i << 5];

				unsigned long j;

				while(_BitScanForward(&j, p))
				{
					p ^= 1 << j;

					m[j].push_front(t);
				}
			}
		}
	}

	if(!t->Update(r))
	{
		printf("!@#$\n"); // memory allocation may fail if the game is too hungry (tales of legendia fight transition/scene)

		RemoveAt(t);

		t = NULL;
	}

	return t;
}

bool GSTextureCacheSW::InvalidateVideoMem(const GSOffset* o, const GSVector4i& rect)
{
	bool changed = false;

	uint32 bp = o->bp;
	uint32 bw = o->bw;
	uint32 psm = o->psm;

	GSVector2i bs = (bp & 31) == 0 ? GSLocalMemory::m_psm[psm].pgs : GSLocalMemory::m_psm[psm].bs;

	GSVector4i r = rect.ralign<Align_Outside>(bs);

	for(int y = r.top; y < r.bottom; y += bs.y)
	{
		uint32 base = o->block.row[y >> 3];

		for(int x = r.left; x < r.right; x += bs.x)
		{
			uint32 page = (base + o->block.col[x >> 3]) >> 5;

			if(page < MAX_PAGES)
			{
				const list<Texture*>& map = m_map[page];

				for(list<Texture*>::const_iterator i = map.begin(); i != map.end(); i++)
				{
					Texture* t = *i;

					if(GSUtil::HasSharedBits(psm, t->m_TEX0.PSM))
					{
						changed = true;

						if(t->m_repeating)
						{
							list<GSVector2i>& l = t->m_p2t[page];
						
							for(list<GSVector2i>::iterator j = l.begin(); j != l.end(); j++)
							{
								t->m_valid[j->x] &= ~j->y;
							}
						}
						else
						{
							t->m_valid[page] = 0;
						}

						t->m_complete = false;
					}
				}
			}
		}
	}

	return changed;
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

void GSTextureCacheSW::RemoveAt(Texture* t)
{
	m_textures.erase(t);

	for(uint32 start = t->m_TEX0.TBP0 >> 5, end = countof(m_map) - 1; start <= end; start++)
	{
		list<Texture*>& m = m_map[start];

		for(list<Texture*>::iterator i = m.begin(); i != m.end(); )
		{
			list<Texture*>::iterator j = i++;

			if(*j == t) {m.erase(j); break;}
		}
	}

	delete t;
}

void GSTextureCacheSW::IncAge()
{
	for(hash_set<Texture*>::iterator i = m_textures.begin(); i != m_textures.end(); )
	{
		hash_set<Texture*>::iterator j = i++;

		Texture* t = *j;

		if(++t->m_age > 30)
		{
			RemoveAt(t);
		}
	}
}

//

GSTextureCacheSW::Texture::Texture(GSState* state, const GSOffset* offset, uint32 tw0, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA)
	: m_state(state)
	, m_offset(offset)
	, m_buff(NULL)
	, m_tw(tw0)
	, m_age(0)
	, m_complete(false)
	, m_p2t(NULL)
{
	m_TEX0 = TEX0;
	m_TEXA = TEXA;

	memset(m_valid, 0, sizeof(m_valid));
	
	m_repeating = m_TEX0.IsRepeating(); // repeating mode always works, it is just slightly slower

	if(m_repeating)
	{
		m_p2t = m_state->m_mem.GetPage2TileMap(m_TEX0);
	}
}

GSTextureCacheSW::Texture::~Texture()
{
	if(m_buff)
	{
		_aligned_free(m_buff);
	}
}

bool GSTextureCacheSW::Texture::Update(const GSVector4i& rect)
{
	if(m_complete)
	{
		return true;
	}

	const GSLocalMemory::psm_t& psm = GSLocalMemory::m_psm[m_TEX0.PSM];

	GSVector2i bs = psm.bs;

	int shift = psm.pal == 0 ? 2 : 0;

	int tw = std::max<int>(1 << m_TEX0.TW, bs.x);
	int th = std::max<int>(1 << m_TEX0.TH, bs.y);

	GSVector4i r = rect;

	r = r.ralign<Align_Outside>(bs);

	if(r.eq(GSVector4i(0, 0, tw, th)))
	{
		m_complete = true; // lame, but better than nothing
	}

	if(m_buff == NULL)
	{
		uint32 tw0 = std::max<int>(m_TEX0.TW, 5 - shift); // makes one row 32 bytes at least, matches the smallest block size that is allocated for m_buff

		if(m_tw == 0)
		{
			m_tw = tw0;
		}
		else
		{
			ASSERT(m_tw >= tw0);
		}

		uint32 pitch = (1 << m_tw) << shift;
		
		m_buff = _aligned_malloc(pitch * th * 4, 32);

		if(m_buff == NULL)
		{
			return false;
		}
	}

	GSLocalMemory& mem = m_state->m_mem;

	const GSOffset* RESTRICT o = m_offset;

	uint32 blocks = 0;

	GSLocalMemory::readTextureBlock rtxbP = psm.rtxbP;

	uint32 pitch = (1 << m_tw) << shift;

	uint8* dst = (uint8*)m_buff + pitch * r.top;

	if(m_repeating)
	{
		for(int y = r.top, block_pitch = pitch * bs.y; y < r.bottom; y += bs.y, dst += block_pitch)
		{
			uint32 base = o->block.row[y >> 3];

			for(int x = r.left, i = (y << 7) + x; x < r.right; x += bs.x, i += bs.x)
			{
				uint32 block = base + o->block.col[x >> 3];

				if(block < MAX_BLOCKS)
				{
					uint32 addr = i >> 3;

					uint32 row = addr >> 5;
					uint32 col = 1 << (addr & 31);

					if((m_valid[row] & col) == 0)
					{
						m_valid[row] |= col;

						(mem.*rtxbP)(block, &dst[x << shift], pitch, m_TEXA);

						blocks++;
					}
				}
			}
		}
	}
	else
	{
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
						m_valid[row] |= col;

						(mem.*rtxbP)(block, &dst[x << shift], pitch, m_TEXA);

						blocks++;
					}
				}
			}
		}
	}

	if(blocks > 0)
	{
		m_state->m_perfmon.Put(GSPerfMon::Unswizzle, bs.x * bs.y * blocks << shift);
	}

	return true;
}

#include "GSTextureSW.h"

bool GSTextureCacheSW::Texture::Save(const string& fn, bool dds) const
{
	const uint32* RESTRICT clut = m_state->m_mem.m_clut;

	int w = 1 << m_TEX0.TW;
	int h = 1 << m_TEX0.TH;

	GSTextureSW t(0, w, h);

	GSTexture::GSMap m;

	if(t.Map(m, NULL))
	{
		const GSLocalMemory::psm_t& psm = GSLocalMemory::m_psm[m_TEX0.PSM];

		const uint8* RESTRICT src = (uint8*)m_buff;
		int pitch = 1 << (m_tw + (psm.pal == 0 ? 2 : 0));

		for(int j = 0; j < h; j++, src += pitch, m.bits += m.pitch)
		{
			if(psm.pal == 0)
			{
				memcpy(m.bits, src, sizeof(uint32) * w);
			}
			else
			{
				for(int i = 0; i < w; i++)
				{
					((uint32*)m.bits)[i] = clut[((uint8*)src)[i]];
				}
			}
		}

		t.Unmap();

		return t.Save(fn.c_str());
	}

	return false;
}