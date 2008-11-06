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

#pragma once

#include "GS.h"
#include "GSLocalMemory.h"

template <class T> class CSurfMap : public CMap<DWORD, DWORD, CComPtr<T>, CComPtr<T>& > {};

extern bool IsRenderTarget(IDirect3DTexture9* pTexture);
extern bool HasSharedBits(DWORD sbp, DWORD spsm, DWORD dbp, DWORD dpsm);

// TODO: get rid of this *PrivateData

#ifdef __INTEL_COMPILER
struct __declspec(uuid("5D5EFE0E-5407-4BCF-855D-C46CBCD075FA"))
#else
[uuid("5D5EFE0E-5407-4BCF-855D-C46CBCD075FA")] struct 
#endif
scale_t
{
	float x, y;
	struct scale_t() {x = y = 1;}
	struct scale_t(float x, float y) {this->x = x; this->y = y;}
	struct scale_t(IDirect3DResource9* p) {Get(p);}
	bool operator == (const struct scale_t& s) {return fabs(x - s.x) < 0.001 && fabs(y - s.y) < 0.001;}
	void Set(IDirect3DResource9* p) {p->SetPrivateData(__uuidof(*this), this, sizeof(*this), 0);}
	void Get(IDirect3DResource9* p) {DWORD size = sizeof(*this); p->GetPrivateData(__uuidof(*this), this, &size);}
};

class GSDirtyRect
{
	DWORD m_PSM;
	CRect m_rcDirty;

public:
	GSDirtyRect() : m_PSM(PSM_PSMCT32), m_rcDirty(0, 0, 0, 0) {}
	GSDirtyRect(DWORD PSM, CRect r);
	CRect GetDirtyRect(const GIFRegTEX0& TEX0);
};

class GSDirtyRectList : public CAtlList<GSDirtyRect>
{
public:
	GSDirtyRectList() {}
	GSDirtyRectList(const GSDirtyRectList& l) {*this = l;}
	void operator = (const GSDirtyRectList& l);
	CRect GetDirtyRect(const GIFRegTEX0& TEX0);
};

struct GSTextureBase
{
	CComPtr<IDirect3DTexture9> m_pTexture, m_pPalette;
	scale_t m_scale;
	bool m_fRT;
	D3DSURFACE_DESC m_desc;

	GSTextureBase();
};

struct GSTexture : public GSTextureBase
{
	GIFRegCLAMP m_CLAMP;
	GIFRegTEX0 m_TEX0;
	GIFRegTEXA m_TEXA; // *
	DWORD m_clut[256]; // *
	GSDirtyRectList m_rcDirty;
	CRect m_rcValid;
	CRect m_rcHash;
	DWORD m_dwHash, m_nHashDiff, m_nHashSame;
	DWORD m_nBytes;
	int m_nAge, m_nVsyncs;
	CInterfaceList<IDirect3DTexture9> m_pSubTextures;
	bool m_fTemp;

	GSTexture();
};

class GSState;

class GSTextureCache : private CAtlList<GSTexture*>
{
protected:
	CInterfaceList<IDirect3DTexture9> m_pTexturePool;
	HRESULT CreateTexture(GSState* s, GSTexture* pt, DWORD PSM, DWORD CPSM = PSM_PSMCT32);
	bool IsTextureInCache(IDirect3DTexture9* pTexture);
	void RemoveOldTextures(GSState* s);
	bool GetDirtyRect(GSState* s, GSTexture* pt, CRect& r);

	DWORD HashTexture(const CRect& r, int pitch, void* bits);
	HRESULT UpdateTexture(GSState* s, GSTexture* pt, GSLocalMemory::readTexture rt);

	GSTexture* ConvertRT(GSState* s, GSTexture* pt);
	GSTexture* ConvertRTPitch(GSState* s, GSTexture* pt);
	GSTexture* ConvertRTWidthHeight(GSState* s, GSTexture* pt);

	CInterfaceList<IDirect3DTexture9> m_pRTPool;
	HRESULT CreateRT(GSState* s, int w, int h, IDirect3DTexture9** ppRT);

public:
	GSTextureCache();
	~GSTextureCache();

	bool Fetch(GSState* s, GSTextureBase& t);
	bool FetchP(GSState* s, GSTextureBase& t);
	bool FetchNP(GSState* s, GSTextureBase& t);

	void IncAge(CSurfMap<IDirect3DTexture9>& pRTs);
	void ResetAge(DWORD TBP0);
	void RemoveAll();
	void InvalidateTexture(GSState* s, const GIFRegBITBLTBUF& BITBLTBUF, const CRect& r);
	void InvalidateLocalMem(GSState* s, DWORD TBP0, DWORD BW, DWORD PSM, const CRect& r);
	void AddRT(GIFRegTEX0& TEX0, IDirect3DTexture9* pRT, scale_t scale);
};
