/* 
 *	Copyright (C) 2003-2005 Gabest
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
#include "GSHash.h"
#include "GSRendererHW.h"

//

bool IsRenderTarget(IDirect3DTexture9* pTexture)
{
	D3DSURFACE_DESC desc;
	memset(&desc, 0, sizeof(desc));
	return pTexture && S_OK == pTexture->GetLevelDesc(0, &desc) && (desc.Usage&D3DUSAGE_RENDERTARGET);
}

bool HasSharedBits(DWORD sbp, DWORD spsm, DWORD dbp, DWORD dpsm)
{
	if(sbp != dbp) return false;

	switch(spsm)
	{
	case PSM_PSMCT32:
	case PSM_PSMCT16:
	case PSM_PSMCT16S:
	case PSM_PSMT8:
	case PSM_PSMT4:
		return true;
	case PSM_PSMCT24:
		return !(dpsm == PSM_PSMT8H || dpsm == PSM_PSMT4HL || dpsm == PSM_PSMT4HH);
	case PSM_PSMT8H:
		return !(dpsm == PSM_PSMCT24);
	case PSM_PSMT4HL:
		return !(dpsm == PSM_PSMCT24 || dpsm == PSM_PSMT4HH);
	case PSM_PSMT4HH:
		return !(dpsm == PSM_PSMCT24 || dpsm == PSM_PSMT4HL);
	}

	return true;
}

//

GSDirtyRect::GSDirtyRect(DWORD PSM, CRect r)
{
	m_PSM = PSM;
	m_rcDirty = r;
}

CRect GSDirtyRect::GetDirtyRect(const GIFRegTEX0& TEX0)
{
	CRect rcDirty = m_rcDirty;

	CSize src = GSLocalMemory::m_psmtbl[m_PSM].bs;
	rcDirty.left = (rcDirty.left) & ~(src.cx-1);
	rcDirty.right = (rcDirty.right + (src.cx-1) /* + 1 */) & ~(src.cx-1);
	rcDirty.top = (rcDirty.top) & ~(src.cy-1);
	rcDirty.bottom = (rcDirty.bottom + (src.cy-1) /* + 1 */) & ~(src.cy-1);

	if(m_PSM != TEX0.PSM)
	{
		CSize dst = GSLocalMemory::m_psmtbl[TEX0.PSM].bs;
		rcDirty.left = MulDiv(m_rcDirty.left, dst.cx, src.cx);
		rcDirty.right = MulDiv(m_rcDirty.right, dst.cx, src.cx);
		rcDirty.top = MulDiv(m_rcDirty.top, dst.cy, src.cy);
		rcDirty.bottom = MulDiv(m_rcDirty.bottom, dst.cy, src.cy);
	}

	rcDirty &= CRect(0, 0, 1<<TEX0.TW, 1<<TEX0.TH);

	return rcDirty;
}

void GSDirtyRectList::operator = (const GSDirtyRectList& l)
{
	RemoveAll();
	POSITION pos = l.GetHeadPosition();
	while(pos) AddTail(l.GetNext(pos));
}

CRect GSDirtyRectList::GetDirtyRect(const GIFRegTEX0& TEX0)
{
	if(IsEmpty()) return CRect(0, 0, 0, 0);
	CRect r(INT_MAX, INT_MAX, 0, 0);
	POSITION pos = GetHeadPosition();
	while(pos) r |= GetNext(pos).GetDirtyRect(TEX0);
	return r;
}

//

GSTextureBase::GSTextureBase()
{
	m_scale = scale_t(1, 1);
	m_fRT = false;
	memset(&m_desc, 0, sizeof(m_desc));
}

GSTexture::GSTexture()
{
	m_TEX0.TBP0 = ~0;
	m_rcValid = CRect(0, 0, 0, 0);
	m_dwHash = ~0;
	m_nHashDiff = m_nHashSame = 0;
	m_rcHash = CRect(0, 0, 0, 0);
	m_nBytes = 0;
	m_nAge = 0;
	m_nVsyncs = 0;
	m_fTemp = false;
}

//

GSTextureCache::GSTextureCache()
{
}

GSTextureCache::~GSTextureCache()
{
	RemoveAll();
}

HRESULT GSTextureCache::CreateTexture(GSState* s, GSTexture* pt, DWORD PSM, DWORD CPSM)
{
	if(!pt || pt->m_pTexture) {ASSERT(0); return E_FAIL;}

	int w = 1 << pt->m_TEX0.TW;
	int h = 1 << pt->m_TEX0.TH;

	int bpp = 0;
	D3DFORMAT fmt = D3DFMT_UNKNOWN;
	D3DFORMAT palfmt = D3DFMT_UNKNOWN;

	switch(PSM)
	{
	default:
	case PSM_PSMCT32:
		bpp = 32;
		fmt = D3DFMT_A8R8G8B8;
		break;
	case PSM_PSMCT24:
		bpp = 32;
		fmt = D3DFMT_X8R8G8B8;
		break;
	case PSM_PSMCT16:
	case PSM_PSMCT16S:
		bpp = 16;
		fmt = D3DFMT_A1R5G5B5;
		break;
	case PSM_PSMT8:
	case PSM_PSMT4:
	case PSM_PSMT8H:
	case PSM_PSMT4HL:
	case PSM_PSMT4HH:
		bpp = 8;
		fmt = D3DFMT_L8;
		palfmt = CPSM == PSM_PSMCT32 ? D3DFMT_A8R8G8B8 : D3DFMT_A1R5G5B5;
		break;
	}

	pt->m_nBytes = w*h*bpp>>3;

	POSITION pos = m_pTexturePool.GetHeadPosition();
	while(pos)
	{
		IDirect3DTexture9* pTexture = m_pTexturePool.GetNext(pos);

		D3DSURFACE_DESC desc;
		memset(&desc, 0, sizeof(desc));
		pTexture->GetLevelDesc(0, &desc);

		if(w == desc.Width && h == desc.Height && fmt == desc.Format && !IsTextureInCache(pTexture))
		{
			pt->m_pTexture = pTexture;
			pt->m_desc = desc;
			break;
		}
	}

	if(!pt->m_pTexture)
	{
		while(m_pTexturePool.GetCount() > 20)
			m_pTexturePool.RemoveTail();

		if(FAILED(s->m_pD3DDev->CreateTexture(w, h, 1, 0, fmt, D3DPOOL_MANAGED, &pt->m_pTexture, NULL)))
			return E_FAIL;

		pt->m_pTexture->GetLevelDesc(0, &pt->m_desc);

		m_pTexturePool.AddHead(pt->m_pTexture);
	}

	if(bpp == 8)
	{
		if(FAILED(s->m_pD3DDev->CreateTexture(256, 1, 1, 0, palfmt, D3DPOOL_MANAGED, &pt->m_pPalette, NULL)))
		{
			pt->m_pTexture = NULL;
			return E_FAIL;
		}
	}

	return S_OK;
}

bool GSTextureCache::IsTextureInCache(IDirect3DTexture9* pTexture)
{
	POSITION pos = GetHeadPosition();
	while(pos)
	{
		if(GetNext(pos)->m_pTexture == pTexture)
			return true;
	}

	return false;
}

void GSTextureCache::RemoveOldTextures(GSState* s)
{
	DWORD nBytes = 0;

	POSITION pos = GetHeadPosition();
	while(pos) nBytes += GetNext(pos)->m_nBytes;

	pos = GetTailPosition();
	while(pos && nBytes > 96*1024*1024/*s->m_ddcaps.dwVidMemTotal*/)
	{
#ifdef DEBUG_LOG
		s->LOG(_T("*TC2 too many textures in cache (%d, %.2f MB)\n"), GetCount(), 1.0f*nBytes/1024/1024);
#endif
		POSITION cur = pos;

		GSTexture* pt = GetPrev(pos);
		if(!pt->m_fRT)
		{
			nBytes -= pt->m_nBytes;
			RemoveAt(cur);
			delete pt;
		}
	}
}

static bool RectInRect(const RECT& inner, const RECT& outer)
{
	return outer.left <= inner.left && inner.right <= outer.right
		&& outer.top <= inner.top && inner.bottom <= outer.bottom;
}

static bool RectInRectH(const RECT& inner, const RECT& outer)
{
	return outer.top <= inner.top && inner.bottom <= outer.bottom;
}

static bool RectInRectV(const RECT& inner, const RECT& outer)
{
	return outer.left <= inner.left && inner.right <= outer.right;
}

bool GSTextureCache::GetDirtyRect(GSState* s, GSTexture* pt, CRect& r)
{
	int w = 1 << pt->m_TEX0.TW;
	int h = 1 << pt->m_TEX0.TH;

	r.SetRect(0, 0, w, h);

// FIXME: kyo's left hand after being selected for player one (PS2-SNK_Vs_Capcom_SVC_Chaos_PAL_CDFull.iso)
// return true;

	s->MinMaxUV(w, h, r);

	CRect rcDirty = pt->m_rcDirty.GetDirtyRect(pt->m_TEX0);
	CRect rcValid = pt->m_rcValid;

#ifdef DEBUG_LOG
	s->LOG(_T("*TC2 used %d,%d-%d,%d (%dx%d), valid %d,%d-%d,%d, dirty %d,%d-%d,%d\n"), r, w, h, rcValid, rcDirty);
#endif

	if(RectInRect(r, rcValid))
	{
		if(rcDirty.IsRectEmpty()) return false;
		else if(RectInRect(rcDirty, r)) r = rcDirty;
		else if(RectInRect(rcDirty, rcValid)) r |= rcDirty;
		else r = rcValid | rcDirty;
	}
	else
	{
		if(RectInRectH(r, rcValid) && (r.left >= rcValid.left || r.right <= rcValid.right))
		{
			r.top = rcValid.top;
			r.bottom = rcValid.bottom;
			if(r.left < rcValid.left) r.right = rcValid.left;
			else /*if(r.right > rcValid.right)*/ r.left = rcValid.right;
		}
		else if(RectInRectV(r, rcValid) && (r.top >= rcValid.top || r.bottom <= rcValid.bottom))
		{
			r.left = rcValid.left;
			r.right = rcValid.right;
			if(r.top < rcValid.top) r.bottom = rcValid.top;
			else /*if(r.bottom > rcValid.bottom)*/ r.top = rcValid.bottom;
		}
		else
		{
			r |= rcValid;
		}
	}

	return true;
}

DWORD GSTextureCache::HashTexture(const CRect& r, int pitch, void* bits)
{
	// TODO: make the hash more unique

	BYTE* p = (BYTE*)bits;
	DWORD hash = r.left + r.right + r.top + r.bottom + pitch + *(BYTE*)bits;

	if(r.Width() > 0)
	{
		int size = r.Width()*r.Height();
/*
		if(size <= 8*8) return rand(); // :P
		else 
*/
		if(size <= 16*16) hash += hash_crc(r, pitch, p);
		else if(size <= 32*32) hash += hash_adler(r, pitch, p);
		else hash += hash_checksum(r, pitch, p);
	}

	return hash;
}

HRESULT GSTextureCache::UpdateTexture(GSState* s, GSTexture* pt, GSLocalMemory::readTexture rt)
{
	CRect r;
	if(!GetDirtyRect(s, pt, r))
		return S_OK;

#ifdef DEBUG_LOG
	s->LOG(_T("*TC2 updating texture %d,%d-%d,%d (%dx%d)\n"), r.left, r.top, r.right, r.bottom, 1 << pt->m_TEX0.TW, 1 << pt->m_TEX0.TH);
#endif

	int bpp = 0;

	switch(pt->m_desc.Format)
	{
	case D3DFMT_A8R8G8B8: bpp = 32; break;
	case D3DFMT_X8R8G8B8: bpp = 32; break;
	case D3DFMT_A1R5G5B5: bpp = 16; break;
	case D3DFMT_L8: bpp = 8; break;
	default: ASSERT(0); return E_FAIL;
	}

	D3DLOCKED_RECT lr;
	if(FAILED(pt->m_pTexture->LockRect(0, &lr, &r, D3DLOCK_NO_DIRTY_UPDATE))) {ASSERT(0); return E_FAIL;}
	(s->m_lm.*rt)(r, (BYTE*)lr.pBits, lr.Pitch, s->m_ctxt->TEX0, s->m_de.TEXA, s->m_ctxt->CLAMP);
	s->m_perfmon.IncCounter(GSPerfMon::c_unswizzle, r.Width()*r.Height()*bpp>>3);
	pt->m_pTexture->UnlockRect(0);

	pt->m_rcValid |= r;
	pt->m_rcDirty.RemoveAll();

	const static DWORD limit = 7;

	if((pt->m_nHashDiff & limit) && pt->m_nHashDiff >= limit && pt->m_rcHash == pt->m_rcValid) // predicted to be dirty
	{
		pt->m_nHashDiff++;
	}
	else
	{
		if(FAILED(pt->m_pTexture->LockRect(0, &lr, &pt->m_rcValid, D3DLOCK_NO_DIRTY_UPDATE|D3DLOCK_READONLY))) {ASSERT(0); return E_FAIL;}
		DWORD dwHash = HashTexture(
			CRect((pt->m_rcValid.left>>2)*(bpp>>3), pt->m_rcValid.top, (pt->m_rcValid.right>>2)*(bpp>>3), pt->m_rcValid.bottom), 
			lr.Pitch, lr.pBits);
		pt->m_pTexture->UnlockRect(0);

		if(pt->m_rcHash != pt->m_rcValid)
		{
			pt->m_nHashDiff = 0;
			pt->m_nHashSame = 0;
			pt->m_rcHash = pt->m_rcValid;
			pt->m_dwHash = dwHash;
		}
		else
		{
			if(pt->m_dwHash != dwHash)
			{
				pt->m_nHashDiff++;
				pt->m_nHashSame = 0;
				pt->m_dwHash = dwHash;
			}
			else
			{
				if(pt->m_nHashDiff < limit) r.SetRect(0, 0, 1, 1);
				// else pt->m_dwHash is not reliable, must update
				pt->m_nHashDiff = 0;
				pt->m_nHashSame++;
			}
		}
	}

	pt->m_pTexture->AddDirtyRect(&r);
	pt->m_pTexture->PreLoad();
	s->m_perfmon.IncCounter(GSPerfMon::c_texture, r.Width()*r.Height()*bpp>>3);

#ifdef DEBUG_LOG
	s->LOG(_T("*TC2 texture was updated, valid %d,%dx%d,%d\n"), pt->m_rcValid);
#endif

#ifdef DEBUG_SAVETEXTURES   
if(s->m_ctxt->FRAME.Block() == 0x00000 && pt->m_TEX0.TBP0 == 0x02800)
{   
	CString fn;   
	fn.Format(_T("c:\\%08I64x_%I64d_%I64d_%I64d_%I64d_%I64d_%I64d_%I64d-%I64d_%I64d-%I64d.bmp"),   
			pt->m_TEX0.TBP0, pt->m_TEX0.PSM, pt->m_TEX0.TBW,   
			pt->m_TEX0.TW, pt->m_TEX0.TH,   
			pt->m_CLAMP.WMS, pt->m_CLAMP.WMT, pt->m_CLAMP.MINU, pt->m_CLAMP.MAXU, pt->m_CLAMP.MINV, pt->m_CLAMP.MAXV);   
	D3DXSaveTextureToFile(fn, D3DXIFF_BMP, pt->m_pTexture, NULL);   
}   
#endif 

	return S_OK;
}

GSTexture* GSTextureCache::ConvertRTPitch(GSState* s, GSTexture* pt)
{
	if(pt->m_TEX0.TBW == s->m_ctxt->TEX0.TBW)
		return pt;

	// sfex3 uses this trick (bw: 10 -> 5, wraps the right side below the left)
	ASSERT(pt->m_TEX0.TBW > s->m_ctxt->TEX0.TBW); // otherwise scale.x need to be reduced to make the larger texture fit (TODO)

	int bw = 64;
	int bh = s->m_ctxt->TEX0.PSM == PSM_PSMCT32 || s->m_ctxt->TEX0.PSM == PSM_PSMCT24 ? 32 : 64;

	int sw = pt->m_TEX0.TBW << 6;

	int dw = s->m_ctxt->TEX0.TBW << 6;
	int dh = 1 << s->m_ctxt->TEX0.TH;

	// TRACE(_T("ConvertRT: %05x %x %d -> %d\n"), (DWORD)s->m_ctxt->TEX0.TBP0, (DWORD)s->m_ctxt->TEX0.PSM, (DWORD)pt->m_TEX0.TBW, (DWORD)s->m_ctxt->TEX0.TBW);

	HRESULT hr;
/*
if(s->m_perfmon.GetFrame() > 400)
hr = D3DXSaveTextureToFile(_T("g:/1.bmp"), D3DXIFF_BMP, pt->m_pTexture, NULL);
*/
	D3DSURFACE_DESC desc;
	hr = pt->m_pTexture->GetLevelDesc(0, &desc);
	if(FAILED(hr)) return NULL;

	CComPtr<IDirect3DTexture9> pRT;
	if(FAILED(hr = CreateRT(s, desc.Width, desc.Height, &pRT)))
		return NULL;

	CComPtr<IDirect3DSurface9> pSrc, pDst;
	hr = pRT->GetSurfaceLevel(0, &pSrc);
	if(FAILED(hr)) return NULL;
	hr = pt->m_pTexture->GetSurfaceLevel(0, &pDst);
	if(FAILED(hr)) return NULL;

	hr = s->m_pD3DDev->StretchRect(pDst, NULL, pSrc, NULL, D3DTEXF_POINT);
	if(FAILED(hr)) return NULL;

	scale_t scale(pt->m_pTexture);

	for(int dy = 0; dy < dh; dy += bh)
	{
		for(int dx = 0; dx < dw; dx += bw)
		{
			int o = dy * dw / bh + dx;

			int sx = o % sw;
			int sy = o / sw;

			// TRACE(_T("%d,%d - %d,%d  <=  %d,%d - %d,%d\n"), dx, dy, dx + bw, dy + bh, sx, sy, sx + bw, sy + bh);

			CRect src, dst;

			src.left = (LONG)(scale.x * sx + 0.5f);
			src.top = (LONG)(scale.y * sy + 0.5f);
			src.right = (LONG)(scale.x * (sx + bw) + 0.5f);
			src.bottom = (LONG)(scale.y * (sy + bh) + 0.5f);

			dst.left = (LONG)(scale.x * dx + 0.5f);
			dst.top = (LONG)(scale.y * dy + 0.5f);
			dst.right = (LONG)(scale.x * (dx + bw) + 0.5f);
			dst.bottom = (LONG)(scale.y * (dy + bh) + 0.5f);

			hr = s->m_pD3DDev->StretchRect(pSrc, src, pDst, dst, D3DTEXF_POINT);

			// TODO: this is quite a lot of StretchRect call, do it with one DrawPrimUP
		}
	}

	pt->m_TEX0.TW = s->m_ctxt->TEX0.TW;
	pt->m_TEX0.TBW = s->m_ctxt->TEX0.TBW;
/*		
if(s->m_perfmon.GetFrame() > 400)
hr = D3DXSaveTextureToFile(_T("g:/2.bmp"), D3DXIFF_BMP, pt->m_pTexture, NULL);
*/

	return pt;
}

GSTexture* GSTextureCache::ConvertRTWidthHeight(GSState* s, GSTexture* pt)
{
	int tw = pt->m_scale.x * (1 << s->m_ctxt->TEX0.TW);
	int th = pt->m_scale.y * (1 << s->m_ctxt->TEX0.TH);

	int rw = pt->m_desc.Width;
	int rh = pt->m_desc.Height;

	if(tw != rw || th != rh)
	//if(tw < rw && th <= rh || tw <= rw && th < rh)
	{
		GSTexture* pt2 = new GSTexture();

		pt2->m_pPalette = pt->m_pPalette;
		pt2->m_fRT = pt->m_pPalette == NULL;
		pt2->m_scale = pt->m_scale;
		pt2->m_fTemp = true;

		POSITION pos = pt->m_pSubTextures.GetHeadPosition();
		while(pos)
		{
			IDirect3DTexture9* pTexture = pt->m_pSubTextures.GetNext(pos);
			pTexture->GetLevelDesc(0, &pt2->m_desc);
			scale_t scale(pTexture);
			if(pt2->m_desc.Width == tw && pt2->m_desc.Height == th
			&& pt2->m_scale.x == scale.x && pt2->m_scale.y == scale.y)
			{
				pt2->m_pTexture = pTexture;
				break;
			}
		}

		if(!pt2->m_pTexture)
		{
			CRect dst(0, 0, tw, th);
			
			if(tw > rw)
			{
				pt2->m_scale.x = pt2->m_scale.x * rw / tw;
				dst.right = rw * rw / tw;
				tw = rw;
			}
			
			if(th > rh)
			{
				pt2->m_scale.y = pt2->m_scale.y * rh / th;
				dst.bottom = rh * rh / th;
				th = rh;
			}

			CRect src(0, 0, tw, th);

			HRESULT hr;

			if(FAILED(hr = CreateRT(s, tw, th, &pt2->m_pTexture)) || FAILED(hr = pt2->m_pTexture->GetLevelDesc(0, &pt2->m_desc)))
			{
				delete pt2; 
				return false;
			}

			CComPtr<IDirect3DSurface9> pSrc, pDst;
			hr = pt->m_pTexture->GetSurfaceLevel(0, &pSrc);
			hr = pt2->m_pTexture->GetSurfaceLevel(0, &pDst);

			ASSERT(pSrc);
			ASSERT(pDst);

			hr = s->m_pD3DDev->StretchRect(pSrc, src, pDst, dst, src == dst ? D3DTEXF_POINT : D3DTEXF_LINEAR);

			pt2->m_scale.Set(pt2->m_pTexture);
			pt->m_pSubTextures.AddTail(pt2->m_pTexture);
		}
		
		pt = pt2;
	}

	return pt;
}

HRESULT GSTextureCache::CreateRT(GSState* s, int w, int h, IDirect3DTexture9** ppRT)
{
	ASSERT(ppRT && *ppRT == NULL);

	HRESULT hr;

	POSITION pos = m_pRTPool.GetHeadPosition();
	while(pos)
	{
		IDirect3DTexture9* pRT = m_pRTPool.GetNext(pos);
		D3DSURFACE_DESC desc;
		pRT->GetLevelDesc(0, &desc);
		if(desc.Width == w && desc.Height == h)
		{
			(*ppRT = pRT)->AddRef();
			return S_OK;
		}
	}

	hr = s->m_pD3DDev->CreateTexture(w, h, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, ppRT, NULL);
	if(FAILED(hr)) return hr;
/**/
	m_pRTPool.AddHead(*ppRT);
	while(m_pRTPool.GetCount() > 3) m_pRTPool.RemoveTail();

	return S_OK;
}

GSTexture* GSTextureCache::ConvertRT(GSState* s, GSTexture* pt)
{
	ASSERT(pt->m_fRT);

	// FIXME: RT + 8h,4hl,4hh

	if(s->m_ctxt->TEX0.PSM == PSM_PSMT8H)
	{
		if(!pt->m_pPalette)
		{
			if(FAILED(s->m_pD3DDev->CreateTexture(256, 1, 1, 0, s->m_ctxt->TEX0.CPSM == PSM_PSMCT32 ? D3DFMT_A8R8G8B8 : D3DFMT_A1R5G5B5, D3DPOOL_MANAGED, &pt->m_pPalette, NULL)))
				return NULL;
		}
	}
	else if(GSLocalMemory::m_psmtbl[s->m_ctxt->TEX0.PSM].pal)
	{
		return NULL;
	}

	pt = ConvertRTPitch(s, pt);

	pt = ConvertRTWidthHeight(s, pt);

	return pt;
}

bool GSTextureCache::Fetch(GSState* s, GSTextureBase& t)
{
	GSTexture* pt = NULL;

	int nPaletteEntries = GSLocalMemory::m_psmtbl[s->m_ctxt->TEX0.PSM].pal;

	DWORD clut[256];

	if(nPaletteEntries)
	{
		s->m_lm.SetupCLUT32(s->m_ctxt->TEX0, s->m_de.TEXA);
		s->m_lm.CopyCLUT32(clut, nPaletteEntries);
	}

#ifdef DEBUG_LOG
	s->LOG(_T("*TC2 Fetch %dx%d %05I64x %I64d (%d)\n"), 
		1 << s->m_ctxt->TEX0.TW, 1 << s->m_ctxt->TEX0.TH, 
		s->m_ctxt->TEX0.TBP0, s->m_ctxt->TEX0.PSM, nPaletteEntries);
#endif

	enum lookupresult {notfound, needsupdate, found} lr = notfound;

	POSITION pos = GetHeadPosition();
	while(pos && !pt)
	{
		POSITION cur = pos;
		pt = GetNext(pos);

		if(HasSharedBits(pt->m_TEX0.TBP0, pt->m_TEX0.PSM, s->m_ctxt->TEX0.TBP0, s->m_ctxt->TEX0.PSM))
		{
			if(pt->m_fRT)
			{
				lr = found;

				if(!(pt = ConvertRT(s, pt)))
					return false;
			}
			else if(s->m_ctxt->TEX0.PSM == pt->m_TEX0.PSM && pt->m_TEX0.TBW == s->m_ctxt->TEX0.TBW
			&& s->m_ctxt->TEX0.TW == pt->m_TEX0.TW && s->m_ctxt->TEX0.TH == pt->m_TEX0.TH
			&& (!(s->m_ctxt->CLAMP.WMS&2) && !(pt->m_CLAMP.WMS&2) && !(s->m_ctxt->CLAMP.WMT&2) && !(pt->m_CLAMP.WMT&2) || s->m_ctxt->CLAMP.i64 == pt->m_CLAMP.i64)
			&& s->m_de.TEXA.TA0 == pt->m_TEXA.TA0 && s->m_de.TEXA.TA1 == pt->m_TEXA.TA1 && s->m_de.TEXA.AEM == pt->m_TEXA.AEM
			&& (!nPaletteEntries || s->m_ctxt->TEX0.CPSM == pt->m_TEX0.CPSM && !memcmp(pt->m_clut, clut, nPaletteEntries*sizeof(clut[0]))))
			{
				lr = needsupdate;
			}
		}

		if(lr != notfound) {MoveToHead(cur); break;}

		pt = NULL;
	}

#ifdef DEBUG_LOG
	s->LOG(_T("*TC2 lr = %s\n"), lr == found ? _T("found") : lr == needsupdate ? _T("needsupdate") : _T("notfound"));
#endif

	if(lr == notfound)
	{
		pt = new GSTexture();

		pt->m_TEX0 = s->m_ctxt->TEX0;
		pt->m_CLAMP = s->m_ctxt->CLAMP;
		pt->m_TEXA = s->m_de.TEXA;

		if(!SUCCEEDED(CreateTexture(s, pt, PSM_PSMCT32)))
		{
			delete pt;
			return false;
		}

		RemoveOldTextures(s);

		AddHead(pt);

		lr = needsupdate;
	}

	ASSERT(pt);

	if(pt && nPaletteEntries)
	{
		memcpy(pt->m_clut, clut, nPaletteEntries*sizeof(clut[0]));
	}

	if(lr == needsupdate)
	{
		UpdateTexture(s, pt, &GSLocalMemory::ReadTexture);

		lr = found;
	}

	if(lr == found)
	{
#ifdef DEBUG_LOG
		s->LOG(_T("*TC2 texture was found, age %d -> 0\n"), pt->m_nAge);
#endif
		pt->m_nAge = 0;
		t = *pt;
		if(pt->m_fTemp) delete pt;
		return true;
	}

	return false;
}

bool GSTextureCache::FetchP(GSState* s, GSTextureBase& t)
{
	GSTexture* pt = NULL;

	int nPaletteEntries = GSLocalMemory::m_psmtbl[s->m_ctxt->TEX0.PSM].pal;

#ifdef DEBUG_LOG
	s->LOG(_T("*TC2 Fetch %dx%d %05I64x %I64d (%d)\n"), 
		1 << s->m_ctxt->TEX0.TW, 1 << s->m_ctxt->TEX0.TH, 
		s->m_ctxt->TEX0.TBP0, s->m_ctxt->TEX0.PSM, nPaletteEntries);
#endif

	enum lookupresult {notfound, needsupdate, found} lr = notfound;

	POSITION pos = GetHeadPosition();
	while(pos && !pt)
	{
		POSITION cur = pos;
		pt = GetNext(pos);

		if(HasSharedBits(pt->m_TEX0.TBP0, pt->m_TEX0.PSM, s->m_ctxt->TEX0.TBP0, s->m_ctxt->TEX0.PSM))
		{
			if(pt->m_fRT)
			{
				lr = found;

				if(!(pt = ConvertRT(s, pt)))
					return false;
			}
			else if(s->m_ctxt->TEX0.PSM == pt->m_TEX0.PSM && pt->m_TEX0.TBW == s->m_ctxt->TEX0.TBW
			&& s->m_ctxt->TEX0.TW == pt->m_TEX0.TW && s->m_ctxt->TEX0.TH == pt->m_TEX0.TH
			&& (!(s->m_ctxt->CLAMP.WMS&2) && !(pt->m_CLAMP.WMS&2) && !(s->m_ctxt->CLAMP.WMT&2) && !(pt->m_CLAMP.WMT&2) || s->m_ctxt->CLAMP.i64 == pt->m_CLAMP.i64))
			{
				lr = needsupdate;
			}
		}

		if(lr != notfound) {MoveToHead(cur); break;}
		
		pt = NULL;
	}

#ifdef DEBUG_LOG
	s->LOG(_T("*TC2 lr = %s\n"), lr == found ? _T("found") : lr == needsupdate ? _T("needsupdate") : _T("notfound"));
#endif

	if(lr == notfound)
	{
		pt = new GSTexture();

		pt->m_TEX0 = s->m_ctxt->TEX0;
		pt->m_CLAMP = s->m_ctxt->CLAMP;
		// pt->m_TEXA = s->m_de.TEXA;

		if(!SUCCEEDED(CreateTexture(s, pt, s->m_ctxt->TEX0.PSM, PSM_PSMCT32)))
		{
			delete pt;
			return false;
		}

		RemoveOldTextures(s);

		AddHead(pt);

		lr = needsupdate;
	}

	ASSERT(pt);

	if(pt && pt->m_pPalette) 
	{
		D3DLOCKED_RECT r;
		if(FAILED(pt->m_pPalette->LockRect(0, &r, NULL, 0)))
			return false;
		s->m_lm.ReadCLUT32(s->m_ctxt->TEX0, s->m_de.TEXA, (DWORD*)r.pBits);
		pt->m_pPalette->UnlockRect(0);
		s->m_perfmon.IncCounter(GSPerfMon::c_texture, 256*4);
	}

	if(lr == needsupdate)
	{
		UpdateTexture(s, pt, &GSLocalMemory::ReadTextureP);

		lr = found;
	}

	if(lr == found)
	{
#ifdef DEBUG_LOG
		s->LOG(_T("*TC2 texture was found, age %d -> 0\n"), pt->m_nAge);
#endif
		pt->m_nAge = 0;
		t = *pt;
		if(pt->m_fTemp) delete pt;
		return true;
	}

	return false;
}

bool GSTextureCache::FetchNP(GSState* s, GSTextureBase& t)
{
	GSTexture* pt = NULL;

	int nPaletteEntries = GSLocalMemory::m_psmtbl[s->m_ctxt->TEX0.PSM].pal;

	DWORD clut[256];

	if(nPaletteEntries)
	{
		s->m_lm.SetupCLUT(s->m_ctxt->TEX0, s->m_de.TEXA);
		s->m_lm.CopyCLUT32(clut, nPaletteEntries);
	}

#ifdef DEBUG_LOG
	s->LOG(_T("*TC2 Fetch %dx%d %05I64x %I64d (%d)\n"), 
		1 << s->m_ctxt->TEX0.TW, 1 << s->m_ctxt->TEX0.TH, 
		s->m_ctxt->TEX0.TBP0, s->m_ctxt->TEX0.PSM, nPaletteEntries);
#endif

	enum lookupresult {notfound, needsupdate, found} lr = notfound;

	POSITION pos = GetHeadPosition();
	while(pos && !pt)
	{
		POSITION cur = pos;
		pt = GetNext(pos);

		if(HasSharedBits(pt->m_TEX0.TBP0, pt->m_TEX0.PSM, s->m_ctxt->TEX0.TBP0, s->m_ctxt->TEX0.PSM))
		{
			if(pt->m_fRT)
			{
				lr = found;

				if(!(pt = ConvertRT(s, pt)))
					return false;
			}
			else if(s->m_ctxt->TEX0.PSM == pt->m_TEX0.PSM && pt->m_TEX0.TBW == s->m_ctxt->TEX0.TBW
			&& s->m_ctxt->TEX0.TW == pt->m_TEX0.TW && s->m_ctxt->TEX0.TH == pt->m_TEX0.TH
			&& (!(s->m_ctxt->CLAMP.WMS&2) && !(pt->m_CLAMP.WMS&2) && !(s->m_ctxt->CLAMP.WMT&2) && !(pt->m_CLAMP.WMT&2) || s->m_ctxt->CLAMP.i64 == pt->m_CLAMP.i64)
			// && s->m_de.TEXA.TA0 == pt->m_TEXA.TA0 && s->m_de.TEXA.TA1 == pt->m_TEXA.TA1 && s->m_de.TEXA.AEM == pt->m_TEXA.AEM
			&& (!nPaletteEntries || s->m_ctxt->TEX0.CPSM == pt->m_TEX0.CPSM && !memcmp(pt->m_clut, clut, nPaletteEntries*sizeof(clut[0]))))
			{
				lr = needsupdate;
			}
		}

		if(lr != notfound) {MoveToHead(cur); break;}

		pt = NULL;
	}

#ifdef DEBUG_LOG
	s->LOG(_T("*TC2 lr = %s\n"), lr == found ? _T("found") : lr == needsupdate ? _T("needsupdate") : _T("notfound"));
#endif

	if(lr == notfound)
	{
		pt = new GSTexture();

		pt->m_TEX0 = s->m_ctxt->TEX0;
		pt->m_CLAMP = s->m_ctxt->CLAMP;
		// pt->m_TEXA = s->m_de.TEXA;

		DWORD psm = s->m_ctxt->TEX0.PSM;

		switch(psm)
		{
		case PSM_PSMT8:
		case PSM_PSMT8H:
		case PSM_PSMT4:
		case PSM_PSMT4HL:
		case PSM_PSMT4HH:
			psm = s->m_ctxt->TEX0.CPSM;
			break;
		}

		if(!SUCCEEDED(CreateTexture(s, pt, psm)))
		{
			delete pt;
			return false;
		}

		RemoveOldTextures(s);

		AddHead(pt);

		lr = needsupdate;
	}

	ASSERT(pt);

	if(pt && nPaletteEntries)
	{
		memcpy(pt->m_clut, clut, nPaletteEntries*sizeof(clut[0]));
	}

	if(lr == needsupdate)
	{
		UpdateTexture(s, pt, &GSLocalMemory::ReadTextureNP);

		lr = found;
	}

	if(lr == found)
	{
#ifdef DEBUG_LOG
		s->LOG(_T("*TC2 texture was found, age %d -> 0\n"), pt->m_nAge);
#endif
		pt->m_nAge = 0;
		t = *pt;
		if(pt->m_fTemp) delete pt;
		return true;
	}

	return false;
}

void GSTextureCache::IncAge(CSurfMap<IDirect3DTexture9>& pRTs)
{
	POSITION pos = GetHeadPosition();
	while(pos)
	{
		POSITION cur = pos;
		GSTexture* pt = GetNext(pos);
		pt->m_nAge++;
		pt->m_nVsyncs++;
		if(pt->m_nAge > 10 && (!pt->m_fRT || pRTs.GetCount() > 3))
		{
			pRTs.RemoveKey(pt->m_TEX0.TBP0);
			RemoveAt(cur);
			delete pt;
		}
	}
}

void GSTextureCache::ResetAge(DWORD TBP0)
{
	POSITION pos = GetHeadPosition();
	while(pos)
	{
		GSTexture* pt = GetNext(pos);
		if(pt->m_TEX0.TBP0 == TBP0) pt->m_nAge = 0;
	}
}

void GSTextureCache::RemoveAll()
{
	while(GetCount()) delete RemoveHead();
	m_pTexturePool.RemoveAll();
	m_pRTPool.RemoveAll();
}

void GSTextureCache::InvalidateTexture(GSState* s, const GIFRegBITBLTBUF& BITBLTBUF, const CRect& r)
{
	GIFRegTEX0 TEX0;
	TEX0.TBP0 = BITBLTBUF.DBP;
	TEX0.TBW = BITBLTBUF.DBW;
	TEX0.PSM = BITBLTBUF.DPSM;
	TEX0.TCC = 0;

#ifdef DEBUG_LOG
	s->LOG(_T("*TC2 invalidate %05x %x (%d,%d-%d,%d)\n"), TEX0.TBP0, TEX0.PSM, r.left, r.top, r.right, r.bottom);
#endif

	POSITION pos = GetHeadPosition();
	while(pos)
	{
		POSITION cur = pos;
		GSTexture* pt = GetNext(pos);
		if(HasSharedBits(TEX0.TBP0, TEX0.PSM, pt->m_TEX0.TBP0, pt->m_TEX0.PSM)) 
		{
			if(TEX0.TBW != pt->m_TEX0.TBW)
			{
				// if TEX0.TBW != pt->m_TEX0.TBW then this render target is more likely to 
				// be discarded by the game (means it doesn't want to transfer an image over 
				// another pre-rendered image) and can be refetched from local mem safely.

				RemoveAt(cur);
				delete pt;
			}
			else if(pt->m_fRT) 
			{
				// TEX0.TBW = pt->m_TEX0.TBW;
				TEX0.PSM = pt->m_TEX0.PSM;

				if(TEX0.PSM == PSM_PSMCT32 || TEX0.PSM == PSM_PSMCT24 
				|| TEX0.PSM == PSM_PSMCT16 || TEX0.PSM == PSM_PSMCT16S) 
				{
//					pt->m_rcDirty.AddHead(GSDirtyRect(PSM, r));

					HRESULT hr;

					int tw = (r.Width() + 3) & ~3;
					int th = r.Height();

					CComPtr<IDirect3DTexture9> pSrc;
					hr = s->m_pD3DDev->CreateTexture(tw, th, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &pSrc, NULL);

					D3DLOCKED_RECT lr;
					if(pSrc && SUCCEEDED(pSrc->LockRect(0, &lr, NULL, 0)))
					{
						GIFRegTEXA TEXA;
						TEXA.AEM = 1;
						TEXA.TA0 = 0;
						TEXA.TA1 = 0x80;

						GIFRegCLAMP CLAMP;
						CLAMP.WMS = 0;
						CLAMP.WMT = 0;

						s->m_lm.ReadTexture(r, (BYTE*)lr.pBits, lr.Pitch, TEX0, TEXA, CLAMP);
						s->m_perfmon.IncCounter(GSPerfMon::c_unswizzle, r.Width()*r.Height()*4);

						pSrc->UnlockRect(0);

						scale_t scale(pt->m_pTexture);

						CRect dst;
						dst.left = (long)(scale.x * r.left + 0.5);
						dst.top = (long)(scale.y * r.top + 0.5);
						dst.right = (long)(scale.x * r.right + 0.5);
						dst.bottom = (long)(scale.y * r.bottom + 0.5);

						//

						CComPtr<IDirect3DSurface9> pRTSurf;
						hr = pt->m_pTexture->GetSurfaceLevel(0, &pRTSurf);
						hr = s->m_pD3DDev->SetRenderTarget(0, pRTSurf);
						hr = s->m_pD3DDev->SetDepthStencilSurface(NULL);

						hr = s->m_pD3DDev->SetTexture(0, pSrc);
						hr = s->m_pD3DDev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
						hr = s->m_pD3DDev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
						hr = s->m_pD3DDev->SetRenderState(D3DRS_ZENABLE, FALSE);
						hr = s->m_pD3DDev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
						hr = s->m_pD3DDev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
						hr = s->m_pD3DDev->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
						hr = s->m_pD3DDev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
						hr = s->m_pD3DDev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
						hr = s->m_pD3DDev->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
						hr = s->m_pD3DDev->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RGBA);

						hr = s->m_pD3DDev->SetPixelShader(NULL);

						struct
						{
							float x, y, z, rhw;
							float tu, tv;
						}
						pVertices[] =
						{
							{(float)dst.left, (float)dst.top, 0.5f, 2.0f, 0, 0},
							{(float)dst.right, (float)dst.top, 0.5f, 2.0f, 1.0f * r.Width() / tw, 0},
							{(float)dst.left, (float)dst.bottom, 0.5f, 2.0f, 0, 1},
							{(float)dst.right, (float)dst.bottom, 0.5f, 2.0f, 1.0f * r.Width() / tw, 1},
						};

						hr = s->m_pD3DDev->BeginScene();
						hr = s->m_pD3DDev->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);
						hr = s->m_pD3DDev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, pVertices, sizeof(pVertices[0]));
						hr = s->m_pD3DDev->EndScene();

					}
				}
				else
				{
					RemoveAt(cur);
					delete pt;
				}
			}
			else
			{
				pt->m_rcDirty.AddHead(GSDirtyRect(TEX0.PSM, r));
			}
		}
	}
}

void GSTextureCache::InvalidateLocalMem(GSState* s, DWORD TBP0, DWORD BW, DWORD PSM, const CRect& r)
{
	CComPtr<IDirect3DTexture9> pRT;

	POSITION pos = GetHeadPosition();
	while(pos)
	{
		POSITION cur = pos;
		GSTexture* pt = GetNext(pos);
		if(pt->m_TEX0.TBP0 == TBP0 && pt->m_fRT) 
		{
			pRT = pt->m_pTexture;
			break;
		}
	}

	if(!pRT) return;

	// TODO: add resizing
/*
	HRESULT hr;

	D3DSURFACE_DESC desc;
	hr = pRT->GetLevelDesc(0, &desc);
	if(FAILED(hr)) return;

	CComPtr<IDirect3DSurface9> pVidMem;
	hr = pRT->GetSurfaceLevel(0, &pVidMem);
	if(FAILED(hr)) return;

	CComPtr<IDirect3DSurface9> pSysMem;
	hr = s->m_pD3DDev->CreateOffscreenPlainSurface(desc.Width, desc.Height, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &pSysMem, NULL);
	if(FAILED(hr)) return;

	hr = s->m_pD3DDev->GetRenderTargetData(pVidMem, pSysMem);
	if(FAILED(hr)) return;

	D3DLOCKED_RECT lr;
	hr = pSysMem->LockRect(&lr, &r, D3DLOCK_READONLY|D3DLOCK_NO_DIRTY_UPDATE);
	if(SUCCEEDED(hr))
	{
		BYTE* p = (BYTE*)lr.pBits;

		if(0 && r.left == 0 && r.top == 0 && PSM == PSM_PSMCT32)
		{
		}
		else
		{
			GSLocalMemory::writeFrame wf = s->m_lm.GetWriteFrame(PSM);

			for(int y = r.top; y < r.bottom; y++, p += lr.Pitch)
			{
				for(int x = r.left; x < r.right; x++)
				{
					(s->m_lm.*wf)(x, y, ((DWORD*)p)[x], TBP0, BW);
				}
			}
		}

		pSysMem->UnlockRect();
	}
	*/
}

void GSTextureCache::AddRT(GIFRegTEX0& TEX0, IDirect3DTexture9* pRT, scale_t scale)
{
	POSITION pos = GetHeadPosition();
	while(pos)
	{
		POSITION cur = pos;
		GSTexture* pt = GetNext(pos);
		if(HasSharedBits(TEX0.TBP0, TEX0.PSM, pt->m_TEX0.TBP0, pt->m_TEX0.PSM))
		{
			RemoveAt(cur);
			delete pt;
		}
	}

	GSTexture* pt = new GSTexture();
	pt->m_TEX0 = TEX0;
	pt->m_pTexture = pRT;
	pt->m_pTexture->GetLevelDesc(0, &pt->m_desc);
	pt->m_scale = scale;
	pt->m_fRT = true;

	AddHead(pt);
}
