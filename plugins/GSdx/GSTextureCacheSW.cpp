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
#include "GSTextureCacheSW.h"

GSTextureCacheSW::GSTextureCacheSW(GSState* state)
	: m_state(state)
{
}

GSTextureCacheSW::~GSTextureCacheSW()
{
	RemoveAll();
}

const GSTextureCacheSW::GSTexture* GSTextureCacheSW::Lookup(const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA, const CRect* r)
{
	GSLocalMemory& mem = m_state->m_mem;

	const GSLocalMemory::psm_t& psm = GSLocalMemory::m_psm[TEX0.PSM];

	const hash_map<GSTexture*, bool>& map = m_map[TEX0.TBP0 >> 5];

	GSTexture* t = NULL;

	for(hash_map<GSTexture*, bool>::const_iterator i = map.begin(); i != map.end(); i++)
	{
		GSTexture* t2 = (*i).first;

		// if(t2->m_TEX0.TBP0 != TEX0.TBP0 || t2->m_TEX0.TBW != TEX0.TBW || t2->m_TEX0.PSM != TEX0.PSM || t2->m_TEX0.TW != TEX0.TW || t2->m_TEX0.TH != TEX0.TH)
		if(((t2->m_TEX0.ai32[0] ^ TEX0.ai32[0]) | ((t2->m_TEX0.ai32[1] ^ TEX0.ai32[1]) & 3)) != 0)
		{
			continue;
		}

		if((psm.trbpp == 16 || psm.trbpp == 24) && (t2->m_TEX0.TCC != TEX0.TCC || TEX0.TCC && !(t2->m_TEXA == (GSVector4i)TEXA).alltrue()))
		{
			continue;
		}

		t = t2;

		t->m_age = 0;

		break;
	}

	if(t == NULL)
	{
		t = new GSTexture(m_state);

		m_textures[t] = true;

		int tw = 1 << TEX0.TW;
		int th = 1 << TEX0.TH;

		DWORD bp = TEX0.TBP0;
		DWORD bw = TEX0.TBW;

		CSize s = (bp & 31) == 0 ? psm.pgs : psm.bs;

		for(int y = 0; y < th; y += s.cy)
		{
			DWORD base = psm.bn(0, y, bp, bw);

			for(int x = 0; x < tw; x += s.cx)
			{
				DWORD page = (base + psm.blockOffset[x >> 3]) >> 5;

				if(page >= MAX_PAGES)
				{
					continue;
				}

				m_map[page][t] = true;
			}
		}
	}

	if(!t->Update(TEX0, TEXA, r))
	{
		printf("!@#$%\n"); // memory allocation may fail if the game is too hungry

		m_textures.erase(t);

		for(int i = 0; i < MAX_PAGES; i++)
		{
			m_map[i].erase(t);
		}

		delete t;

		return NULL;
	}

	return t;
}

void GSTextureCacheSW::RemoveAll()
{
	for(hash_map<GSTexture*, bool>::iterator i = m_textures.begin(); i != m_textures.end(); i++)
	{
		delete i->first;
	}

	m_textures.clear();

	for(int i = 0; i < MAX_PAGES; i++)
	{
		m_map[i].clear();
	}
}

void GSTextureCacheSW::IncAge()
{
	for(hash_map<GSTexture*, bool>::iterator i = m_textures.begin(); i != m_textures.end(); )
	{
		hash_map<GSTexture*, bool>::iterator j = i++;

		GSTexture* t = j->first;

		if(++t->m_age > 30)
		{
			m_textures.erase(j);

			for(int i = 0; i < MAX_PAGES; i++)
			{
				m_map[i].erase(t);
			}

			delete t;
		}
	}
}

void GSTextureCacheSW::InvalidateVideoMem(const GIFRegBITBLTBUF& BITBLTBUF, const CRect& rect)
{
	const GSLocalMemory::psm_t& psm = GSLocalMemory::m_psm[BITBLTBUF.DPSM];

	DWORD bp = BITBLTBUF.DBP;
	DWORD bw = BITBLTBUF.DBW;

	CSize s = (bp & 31) == 0 ? psm.pgs : psm.bs;

	CRect r;

	r.left = rect.left & ~(s.cx - 1);
	r.top = rect.top & ~(s.cy - 1);
	r.right = (rect.right + (s.cx - 1)) & ~(s.cx - 1);
	r.bottom = (rect.bottom + (s.cy - 1)) & ~(s.cy - 1);

	for(int y = r.top; y < r.bottom; y += s.cy)
	{
		DWORD base = psm.bn(0, y, bp, bw);

		for(int x = r.left; x < r.right; x += s.cx)
		{
			DWORD page = (base + psm.blockOffset[x >> 3]) >> 5;

			if(page >= MAX_PAGES)
			{
				continue;
			}

			const hash_map<GSTexture*, bool>& map = m_map[page];

			for(hash_map<GSTexture*, bool>::const_iterator i = map.begin(); i != map.end(); i++)
			{
				GSTexture* t = (*i).first;

				t->m_valid[page] = 0;

				t->m_complete = false;
			}
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

bool GSTextureCacheSW::GSTexture::Update(const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA, const CRect* rect)
{
	if(m_complete)
	{
		return true;
	}

	m_TEX0 = TEX0;
	m_TEXA = TEXA;

	GSLocalMemory& mem = m_state->m_mem;

	const GSLocalMemory::psm_t& psm = GSLocalMemory::m_psm[TEX0.PSM];

	DWORD bp = TEX0.TBP0;
	DWORD bw = TEX0.TBW;

	CSize s = psm.bs;

	int tw = max(1 << TEX0.TW, s.cx);
	int th = max(1 << TEX0.TH, s.cy);

	if(m_buff == NULL)
	{
		m_buff = _aligned_malloc(tw * th * sizeof(DWORD), 16);

		if(m_buff == NULL)
		{
			return false;
		}

		m_tw = max(psm.pal > 0 ? 5 : 3, TEX0.TW); // makes one row 32 bytes at least, matches the smallest block size that is allocated above for m_buff
	}

	CRect r(0, 0, tw, th);

	if(rect)
	{
		r.left = rect->left & ~(s.cx - 1);
		r.top = rect->top & ~(s.cy - 1);
		r.right = (rect->right + (s.cx - 1)) & ~(s.cx - 1);
		r.bottom = (rect->bottom + (s.cy - 1)) & ~(s.cy - 1);
	}

	if(r.left == 0 && r.top == 0 && r.right == tw && r.bottom == th)
	{
		m_complete = true; // lame, but better than nothing
	}

	GSLocalMemory::readTextureBlock rtxb = psm.rtxbP;
	
	int bytes = psm.pal > 0 ? 1 : 4;

	DWORD pitch = (1 << m_tw) * bytes;

	BYTE* dst = (BYTE*)m_buff + pitch * r.top;

	DWORD blocks = 0;

	if(tw <= (bw << 6))
	{
		for(int y = r.top, o = pitch * s.cy; y < r.bottom; y += s.cy, dst += o)
		{
			DWORD base = psm.bn(0, y, bp, bw);

			for(int x = r.left; x < r.right; x += s.cx)
			{
				DWORD block = base + psm.blockOffset[x >> 3];

				if(block < MAX_BLOCKS)
				{
					DWORD row = block >> 5;
					DWORD col = 1 << (block & 31);

					if((m_valid[row] & col) == 0)
					{
						m_valid[row] |= col;

						(mem.*rtxb)(block, &dst[x * bytes], pitch, TEXA);

						blocks++;
					}
				}
			}
		}
	}
	else
	{
		// unfortunatelly a block may be part of the same texture multiple times at different places (tw 1024 > tbw 640, between 640 -> 1024 it is repeated from the next row), 
		// so just can't set the block's bit to valid in one pass, even if 99.9% of the games don't address the repeated part at the right side
		
		// TODO: still bogus if those repeated parts aren't fetched together

		for(int y = r.top, o = pitch * s.cy; y < r.bottom; y += s.cy, dst += o)
		{
			DWORD base = psm.bn(0, y, bp, bw);

			for(int x = r.left; x < r.right; x += s.cx)
			{
				DWORD block = base + psm.blockOffset[x >> 3];

				if(block < MAX_BLOCKS)
				{
					DWORD row = block >> 5;
					DWORD col = 1 << (block & 31);

					if((m_valid[row] & col) == 0)
					{
						(mem.*rtxb)(block, &dst[x * bytes], pitch, TEXA);

						blocks++;
					}
				}
			}
		}

		for(int y = r.top; y < r.bottom; y += s.cy)
		{
			DWORD base = psm.bn(0, y, bp, bw);

			for(int x = r.left; x < r.right; x += s.cx)
			{
				DWORD block = base + psm.blockOffset[x >> 3];

				if(block < MAX_BLOCKS)
				{
					DWORD row = block >> 5;
					DWORD col = 1 << (block & 31);

					m_valid[row] |= col;
				}
			}
		}
	}

	m_state->m_perfmon.Put(GSPerfMon::Unswizzle, s.cx * s.cy * bytes * blocks);

	return true;
}
