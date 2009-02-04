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

// static FILE* m_log = NULL;

GSTextureCacheSW::GSTextureCacheSW(GSState* state)
	: m_state(state)
{
	// m_log = _tfopen(_T("c:\\log.txt"), _T("w"));
}

GSTextureCacheSW::~GSTextureCacheSW()
{
	// fclose(m_log);

	RemoveAll();
}

const GSTextureCacheSW::GSTexture* GSTextureCacheSW::Lookup(const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA, const CRect* r)
{
	GSLocalMemory& mem = m_state->m_mem;

	const GSLocalMemory::psm_t& psm = GSLocalMemory::m_psm[TEX0.PSM];

	const CAtlList<GSTexturePage*>& t2p = m_p2t[TEX0.TBP0 >> 5];

	// fprintf(m_log, "lu %05x %d %d (%d) ", TEX0.TBP0, TEX0.TBW, TEX0.PSM, t2p.GetCount());

	// if(r) fprintf(m_log, "(%d %d %d %d) ", r->left, r->top, r->right, r->bottom);

	GSTexture* t = NULL;

	POSITION pos = t2p.GetHeadPosition();

	while(pos)
	{
		GSTexture* t2 = t2p.GetNext(pos)->t;

		// if(t2->m_TEX0.TBP0 != TEX0.TBP0 || t2->m_TEX0.TBW != TEX0.TBW || t2->m_TEX0.PSM != TEX0.PSM || t2->m_TEX0.TW != TEX0.TW || t2->m_TEX0.TH != TEX0.TH)
		if(((t2->m_TEX0.ai32[0] ^ TEX0.ai32[0]) | ((t2->m_TEX0.ai32[1] ^ TEX0.ai32[1]) & 3)) != 0)
		{
			continue;
		}

		if((psm.trbpp == 16 || psm.trbpp == 24) && (t2->m_TEX0.TCC != TEX0.TCC || TEX0.TCC == 1 && !(t2->m_TEXA == (GSVector4i)TEXA).alltrue()))
		{
			continue;
		}

		// fprintf(m_log, "cache hit\n");

		t = t2;

		t->m_age = 0;

		break;
	}

	if(t == NULL)
	{
		// fprintf(m_log, "cache miss\n");

		t = new GSTexture(m_state);

		t->m_pos = m_textures.AddTail(t);

		int tw = 1 << TEX0.TW;
		int th = 1 << TEX0.TH;

		DWORD bp = TEX0.TBP0;
		DWORD bw = TEX0.TBW;

		for(int j = 0, y = 0; y < th; j++, y += psm.pgs.cy)
		{
			DWORD page = psm.pgn(0, y, bp, bw);

			for(int i = 0, x = 0; x < tw && page < MAX_PAGES; i++, x += psm.pgs.cx, page++)
			{
				GSTexturePage* p = new GSTexturePage();
				
				p->t = t;
				p->row = j;
				p->col = i;

				GSTexturePageEntry* p2te = new GSTexturePageEntry();

				p2te->p2t = &m_p2t[page];
				p2te->pos = m_p2t[page].AddHead(p);

				t->m_p2te.AddTail(p2te);

				t->m_maxpages++;
			}
		}
	}

	if(!t->Update(TEX0, TEXA, r))
	{
		m_textures.RemoveAt(t->m_pos);

		delete t;

		printf("!@#$%\n"); // memory allocation may fail if the game is too hungry

		return NULL;
	}

	return t;
}

void GSTextureCacheSW::RemoveAll()
{
	POSITION pos = m_textures.GetHeadPosition();

	while(pos)
	{
		delete m_textures.GetNext(pos);
	}

	m_textures.RemoveAll();

	for(int i = 0; i < MAX_PAGES; i++)
	{
		CAtlList<GSTexturePage*>& t2p = m_p2t[i];

		ASSERT(t2p.IsEmpty());

		POSITION pos = t2p.GetHeadPosition();

		while(pos)
		{
			delete t2p.GetNext(pos);
		}

		t2p.RemoveAll();
	}
}

void GSTextureCacheSW::IncAge()
{
	POSITION pos = m_textures.GetHeadPosition();

	while(pos)
	{
		POSITION cur = pos;

		GSTexture* t = m_textures.GetNext(pos);

		if(++t->m_age > 3)
		{
			m_textures.RemoveAt(cur);

			delete t;
		}
	}
}

void GSTextureCacheSW::InvalidateVideoMem(const GIFRegBITBLTBUF& BITBLTBUF, const CRect& r)
{
	const GSLocalMemory::psm_t& psm = GSLocalMemory::m_psm[BITBLTBUF.DPSM];

	CRect r2;

	r2.left = r.left & ~(psm.pgs.cx - 1);
	r2.top = r.top & ~(psm.pgs.cy - 1);
	r2.right = (r.right + (psm.pgs.cx - 1)) & ~(psm.pgs.cx - 1);
	r2.bottom = (r.bottom + (psm.pgs.cy - 1)) & ~(psm.pgs.cy - 1);

	DWORD bp = BITBLTBUF.DBP;
	DWORD bw = BITBLTBUF.DBW;

	// fprintf(m_log, "ivm %05x %d %d (%d %d %d %d)\n", bp, bw, BITBLTBUF.DPSM, r2.left, r2.top, r2.right, r2.bottom);

	for(int y = r2.top; y < r2.bottom; y += psm.pgs.cy)
	{
		DWORD page = psm.pgn(r2.left, y, bp, bw);

		for(int x = r2.left; x < r2.right && page < MAX_PAGES; x += psm.pgs.cx, page++)
		{
			const CAtlList<GSTexturePage*>& t2p = m_p2t[page];

			POSITION pos = t2p.GetHeadPosition();

			while(pos)
			{
				GSTexturePage* p = t2p.GetNext(pos);

				DWORD flag = 1 << p->col;

				if((p->t->m_valid[p->row] & flag) == 0)
				{
					continue;
				}

				if(GSUtil::HasSharedBits(BITBLTBUF.DPSM, p->t->m_TEX0.PSM))
				{
					p->t->m_valid[p->row] &= ~flag;
					p->t->m_pages--;

					// fprintf(m_log, "ivm hit %05x %d %d (%d %d) (%d)", p->t->m_TEX0.TBP0, p->t->m_TEX0.TBW, p->t->m_TEX0.PSM, p->row, p->col, p->t->m_pages);
					// if(p->t->m_pages == 0) fprintf(m_log, " *");
					// fprintf(m_log, "\n");
				}
			}
		}
	}
}

//

GSTextureCacheSW::GSTexture::GSTexture(GSState* state)
	: m_state(state)
	, m_buff(NULL)
	, m_tw(0)
	, m_maxpages(0)
	, m_pages(0)
	, m_pos(NULL)
	, m_age(0)
{
	memset(m_valid, 0, sizeof(m_valid));
}

GSTextureCacheSW::GSTexture::~GSTexture()
{
	if(m_buff)
	{
		_aligned_free(m_buff);
	}

	POSITION pos = m_p2te.GetHeadPosition();

	while(pos)
	{
		GSTexturePageEntry* p2te = m_p2te.GetNext(pos);

		GSTexturePage* p = p2te->p2t->GetAt(p2te->pos);

		ASSERT(p->t == this);

		delete p;

		p2te->p2t->RemoveAt(p2te->pos);

		delete p2te;
	}

	m_p2te.RemoveAll();
}

bool GSTextureCacheSW::GSTexture::Update(const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA, const CRect* r)
{
	if(m_pages == m_maxpages)
	{
		return true;
	}

	m_TEX0 = TEX0;
	m_TEXA = TEXA;

	GSLocalMemory& mem = m_state->m_mem;

	const GSLocalMemory::psm_t& psm = GSLocalMemory::m_psm[TEX0.PSM];

	int tw = 1 << TEX0.TW;
	int th = 1 << TEX0.TH;

	if(tw < psm.bs.cx) tw = psm.bs.cx;
	if(th < psm.bs.cy) th = psm.bs.cy;

	if(m_buff == NULL)
	{
		// fprintf(m_log, "up new (%d %d)\n", tw, th);

		m_buff = _aligned_malloc(tw * th * sizeof(DWORD), 16);

		if(m_buff == NULL)
		{
			return false;
		}

		m_tw = max(psm.pal > 0 ? 5 : 3, TEX0.TW); // makes one row 32 bytes at least, matches the smallest block size that is allocated above for m_buff
	}

	CRect r2;

	if(r)
	{
		r2.left = r->left & ~(psm.pgs.cx - 1);
		r2.top = r->top & ~(psm.pgs.cy - 1);
		r2.right = (r->right + (psm.pgs.cx - 1)) & ~(psm.pgs.cx - 1);
		r2.bottom = (r->bottom + (psm.pgs.cy - 1)) & ~(psm.pgs.cy - 1);
	}

	// TODO

	GSLocalMemory::readTexture rt = psm.pal > 0 ? psm.rtxP : psm.rtx;
	int bytes = psm.pal > 0 ? 1 : 4;

	BYTE* dst = (BYTE*)m_buff;

	DWORD pitch = (1 << m_tw) * bytes;
	DWORD mask = pitch - 1;

	for(int j = 0, y = 0; y < th; j++, y += psm.pgs.cy, dst += pitch * psm.pgs.cy)
	{
		if(m_valid[j] == mask)
		{
			continue;
		}

		if(r)
		{
			if(y < r2.top) continue;
			if(y >= r2.bottom) break;
		}

		DWORD page = psm.pgn(0, y, TEX0.TBP0, TEX0.TBW);

		for(int i = 0, x = 0; x < tw && page < MAX_PAGES; i++, x += psm.pgs.cx, page++)
		{
			if(r)
			{
				if(x < r2.left) continue;
				if(x >= r2.right) break;
			}

			DWORD flag = 1 << i;

			if(m_valid[j] & flag)
			{
				continue;
			}

			m_valid[j] |= flag;
			m_pages++;

			ASSERT(m_pages <= m_maxpages);

			CRect r;
			
			r.left = x;
			r.top = y;
			r.right = min(x + psm.pgs.cx, tw);
			r.bottom = min(y + psm.pgs.cy, th);

			// fprintf(m_log, "up fetch (%d %d) (%d %d %d %d)\n", j, i, r.left, r.top, r.right, r.bottom);

			(mem.*rt)(r, &dst[x * bytes], pitch, TEX0, TEXA);

			m_state->m_perfmon.Put(GSPerfMon::Unswizzle, r.Width() * r.Height() * bytes);
		}
	}

	return true;
}