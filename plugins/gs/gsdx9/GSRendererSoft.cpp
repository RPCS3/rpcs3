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
#include "GSRendererSoft.h"
#include "x86.h"

template <class Vertex>
GSRendererSoft<Vertex>::GSRendererSoft(HWND hWnd, HRESULT& hr)
	: GSRenderer<Vertex>(640, 512, hWnd, hr)
{
	Reset();

	int i = SHRT_MIN;
	BYTE j = 0;
	for(; i < 0; i++, j++) m_clip[j] = 0, m_mask[j] = j;
	for(; i < 256; i++, j++) m_clip[j] = (BYTE)i, m_mask[j] = j;
	for(; i < SHRT_MAX; i++, j++) m_clip[j] = 255, m_mask[j] = j;

	m_uv = (uv_wrap_t*)_aligned_malloc(sizeof(uv_wrap_t), 16);

	// w00t :P

	#define InitATST(iZTST, iATST) \
		m_dv[iZTST][iATST] = &GSRendererSoft<Vertex>::DrawVertex<iZTST, iATST>; \

	#define InitZTST(iZTST) \
		InitATST(iZTST, 0) \
		InitATST(iZTST, 1) \
		InitATST(iZTST, 2) \
		InitATST(iZTST, 3) \
		InitATST(iZTST, 4) \
		InitATST(iZTST, 5) \
		InitATST(iZTST, 6) \
		InitATST(iZTST, 7) \

	#define InitDV() \
		InitZTST(0) \
		InitZTST(1) \
		InitZTST(2) \
		InitZTST(3) \

	InitDV();

	#define InitTFX(iLOD, bLCM, bTCC, iTFX) \
		m_dvtfx[iLOD][bLCM][bTCC][iTFX] = &GSRendererSoft<Vertex>::DrawVertexTFX<iLOD, bLCM, bTCC, iTFX>; \

	#define InitTCC(iLOD, bLCM, bTCC) \
		InitTFX(iLOD, bLCM, bTCC, 0) \
		InitTFX(iLOD, bLCM, bTCC, 1) \
		InitTFX(iLOD, bLCM, bTCC, 2) \
		InitTFX(iLOD, bLCM, bTCC, 3) \

	#define InitLCM(iLOD, bLCM) \
		InitTCC(iLOD, bLCM, false) \
		InitTCC(iLOD, bLCM, true) \

	#define InitLOD(iLOD) \
		InitLCM(iLOD, false) \
		InitLCM(iLOD, true) \

	#define InitDVTFX() \
		InitLOD(0) \
		InitLOD(1) \
		InitLOD(2) \
		InitLOD(3) \

	InitDVTFX();
}

template <class Vertex>
GSRendererSoft<Vertex>::~GSRendererSoft()
{
	_aligned_free(m_uv);
}

template <class Vertex>
HRESULT GSRendererSoft<Vertex>::ResetDevice(bool fForceWindowed)
{
	m_pRT[0] = NULL;
	m_pRT[1] = NULL;

	return __super::ResetDevice(fForceWindowed);
}

template <class Vertex>
void GSRendererSoft<Vertex>::Reset()
{
	m_primtype = PRIM_NONE;
	m_pTexture = NULL;

	__super::Reset();
}

template <class Vertex>
int GSRendererSoft<Vertex>::DrawingKick(bool fSkip)
{
	Vertex* pVertices = &m_pVertices[m_nVertices];
	int nVertices = 0;

	switch(m_PRIM)
	{
	case 3: // triangle list
		m_primtype = PRIM_TRIANGLE;
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		LOGV((pVertices[0], _T("TriList")));
		LOGV((pVertices[1], _T("TriList")));
		LOGV((pVertices[2], _T("TriList")));
		break;
	case 4: // triangle strip
		m_primtype = PRIM_TRIANGLE;
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		m_vl.GetAt(0, pVertices[nVertices++]);
		m_vl.GetAt(1, pVertices[nVertices++]);
		LOGV((pVertices[0], _T("TriStrip")));
		LOGV((pVertices[1], _T("TriStrip")));
		LOGV((pVertices[2], _T("TriStrip")));
		break;
	case 5: // triangle fan
		m_primtype = PRIM_TRIANGLE;
		m_vl.GetAt(0, pVertices[nVertices++]);
		m_vl.RemoveAt(1, pVertices[nVertices++]);
		m_vl.GetAt(1, pVertices[nVertices++]);
		LOGV((pVertices[0], _T("TriFan")));
		LOGV((pVertices[1], _T("TriFan")));
		LOGV((pVertices[2], _T("TriFan")));
		break;
	case 6: // sprite
		m_primtype = PRIM_SPRITE;
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		nVertices += 2;
		pVertices[0].p.z = pVertices[1].p.z;
		pVertices[0].p.q = pVertices[1].p.q;
		pVertices[2] = pVertices[1];
		pVertices[3] = pVertices[1];
		pVertices[1].p.y = pVertices[0].p.y;
		pVertices[1].t.y = pVertices[0].t.y;
		pVertices[2].p.x = pVertices[0].p.x;
		pVertices[2].t.x = pVertices[0].t.x;
		LOGV((pVertices[0], _T("Sprite")));
		LOGV((pVertices[1], _T("Sprite")));
		LOGV((pVertices[2], _T("Sprite")));
		LOGV((pVertices[3], _T("Sprite")));
		/*
		m_primtype = PRIM_TRIANGLE;
		nVertices += 2;
		pVertices[5] = pVertices[3];
		pVertices[3] = pVertices[1];
		pVertices[4] = pVertices[2];
		*/
		break;
	case 1: // line
		m_primtype = PRIM_LINE;
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		LOGV((pVertices[0], _T("LineList")));
		LOGV((pVertices[1], _T("LineList")));
		break;
	case 2: // line strip
		m_primtype = PRIM_LINE;
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		m_vl.GetAt(0, pVertices[nVertices++]);
		LOGV((pVertices[0], _T("LineStrip")));
		LOGV((pVertices[1], _T("LineStrip")));
		break;
	case 0: // point
		m_primtype = PRIM_POINT;
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		LOGV((pVertices[0], _T("PointList")));
		break;
	default:
		ASSERT(0);
		return 0;
	}

	if(fSkip || !m_rs.IsEnabled(0) && !m_rs.IsEnabled(1))
		return 0;

	if(!m_pPRIM->IIP)
	{
		Vertex::Vector c = pVertices[nVertices-1].c;
		for(int i = 0; i < nVertices-1; i++) 
			pVertices[i].c = c;
	}

	return nVertices;
}

template <class Vertex>
void GSRendererSoft<Vertex>::FlushPrim()
{
	if(m_nVertices > 0)
	{
CString fn;
static int s_savenum = 0;
s_savenum++;

if(0)
//if(m_ctxt->FRAME.Block() == 0x008c0 && (DWORD)m_ctxt->TEX0.TBP0 == 0x03a98)
//if(m_ctxt->TEX0.PSM == 0x1b)
if(m_perfmon.GetFrame() >= 200)
{
fn.Format(_T("g:/tmp/%04I64d_%06d_1f_%05x_%x.bmp"), m_perfmon.GetFrame(), s_savenum, m_ctxt->FRAME.Block(), m_ctxt->FRAME.PSM);
m_lm.SaveBMP(m_pD3DDev, fn, m_ctxt->FRAME.Block(), m_ctxt->FRAME.FBW, m_ctxt->FRAME.PSM, m_ctxt->FRAME.FBW*64, 224);

if(m_pPRIM->TME)
{
fn.Format(_T("g:/tmp/%04I64d_%06d_2t_%05x_%x.bmp"), m_perfmon.GetFrame(), s_savenum, (DWORD)m_ctxt->TEX0.TBP0, (DWORD)m_ctxt->TEX0.PSM);
m_lm.SaveBMP(m_pD3DDev, fn, m_ctxt->TEX0.TBP0, m_ctxt->TEX0.TBW, m_ctxt->TEX0.PSM, 1 << m_ctxt->TEX0.TW, 1 << m_ctxt->TEX0.TH);
}
}

		int iZTST = !m_ctxt->TEST.ZTE ? 1 : m_ctxt->TEST.ZTST;
		int iATST = !m_ctxt->TEST.ATE ? 1 : m_ctxt->TEST.ATST;

		m_pDrawVertex = m_dv[iZTST][iATST];

		if(m_pPRIM->TME)
		{
			int iLOD = (m_ctxt->TEX1.MMAG & 1) + (m_ctxt->TEX1.MMIN & 1);
			int bLCM = m_ctxt->TEX1.LCM ? 1 : 0;
			int bTCC = m_ctxt->TEX0.TCC ? 1 : 0;
			int iTFX = m_ctxt->TEX0.TFX;

			if(m_pPRIM->FST)
			{
				iLOD = 3;
				bLCM = m_ctxt->TEX1.K <= 0 && (m_ctxt->TEX1.MMAG & 1) || m_ctxt->TEX1.K > 0 && (m_ctxt->TEX1.MMIN & 1);
			}

			m_pDrawVertexTFX = m_dvtfx[iLOD][bLCM][bTCC][iTFX];
		}

		SetupTexture();
		
		m_scissor.SetRect(
			max(m_ctxt->SCISSOR.SCAX0, 0),
			max(m_ctxt->SCISSOR.SCAY0, 0),
			min(m_ctxt->SCISSOR.SCAX1+1, m_ctxt->FRAME.FBW * 64),
			min(m_ctxt->SCISSOR.SCAY1+1, 4096));

		m_clamp = (m_de.COLCLAMP.CLAMP ? m_clip : m_mask) + 32768;

		int nPrims = 0;
		Vertex* pVertices = m_pVertices;

		switch(m_primtype)
		{
		case PRIM_SPRITE:
			ASSERT(!(m_nVertices&3));
			nPrims = m_nVertices / 4;
			LOG(_T("FlushPrim(pt=%d, nVertices=%d, nPrims=%d)\n"), m_primtype, m_nVertices, nPrims);
			for(int i = 0; i < nPrims; i++, pVertices += 4) DrawSprite(pVertices);
			break;
		case PRIM_TRIANGLE:
			ASSERT(!(m_nVertices%3));
			nPrims = m_nVertices / 3;
			LOG(_T("FlushPrim(pt=%d, nVertices=%d, nPrims=%d)\n"), m_primtype, m_nVertices, nPrims);
			for(int i = 0; i < nPrims; i++, pVertices += 3) DrawTriangle(pVertices);
			break;
		case PRIM_LINE: 
			ASSERT(!(m_nVertices&1));
			nPrims = m_nVertices / 2;
			LOG(_T("FlushPrim(pt=%d, nVertices=%d, nPrims=%d)\n"), m_primtype, m_nVertices, nPrims);
			for(int i = 0; i < nPrims; i++, pVertices += 2) DrawLine(pVertices);
			break;
		case PRIM_POINT:
			nPrims = m_nVertices;
			LOG(_T("FlushPrim(pt=%d, nVertices=%d, nPrims=%d)\n"), m_primtype, m_nVertices, nPrims);
			for(int i = 0; i < nPrims; i++, pVertices++) DrawPoint(pVertices);
			break;
		default:
			ASSERT(m_nVertices == 0);
			return;
		}

		m_perfmon.IncCounter(GSPerfMon::c_prim, nPrims);

if(0)
//if(m_ctxt->FRAME.Block() == 0x008c0 && (DWORD)m_ctxt->TEX0.TBP0 == 0x03a98)
//if(m_ctxt->TEX0.PSM == 0x1b)
if(m_perfmon.GetFrame() >= 200)
{
fn.Format(_T("g:/tmp/%04I64d_%06d_3f_%05x_%x.bmp"), m_perfmon.GetFrame(), s_savenum, m_ctxt->FRAME.Block(), m_ctxt->FRAME.PSM);
m_lm.SaveBMP(m_pD3DDev, fn, m_ctxt->FRAME.Block(), m_ctxt->FRAME.FBW, m_ctxt->FRAME.PSM, m_ctxt->FRAME.FBW*64, 224);
}
	}

	m_primtype = PRIM_NONE;

	__super::FlushPrim();
}

template <class Vertex>
void GSRendererSoft<Vertex>::Flip()
{
	HRESULT hr;

	FlipInfo rt[2];

	for(int i = 0; i < countof(rt); i++)
	{
		if(m_rs.IsEnabled(i))
		{
			CRect rect = CRect(CPoint(0, 0), m_rs.GetDispRect(i).BottomRight());

			//GSLocalMemory::RoundUp(, GSLocalMemory::GetBlockSize(m_rs.DISPFB[i].PSM));

			ZeroMemory(&rt[i].rd, sizeof(rt[i].rd));
			if(m_pRT[i]) m_pRT[i]->GetLevelDesc(0, &rt[i].rd);

			if(rt[i].rd.Width != (UINT)rect.right || rt[i].rd.Height != (UINT)rect.bottom)
				m_pRT[i] = NULL;

			if(!m_pRT[i])
			{
				CComPtr<IDirect3DTexture9> pRT;
				D3DLOCKED_RECT lr;
				int nTries = 0, nMaxTries = 10;
				do
				{
					pRT = NULL;
					hr = m_pD3DDev->CreateTexture(rect.right, rect.bottom, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &pRT, NULL);
					if(FAILED(hr)) break;
					if(SUCCEEDED(pRT->LockRect(0, &lr, NULL, 0)))
						pRT->UnlockRect(0);
					m_pRT[i] = pRT;
				}
				while((((DWORD_PTR)lr.pBits & 0xf) || (lr.Pitch & 0xf)) && ++nTries < nMaxTries);

				if(nTries == nMaxTries) continue;

				ZeroMemory(&rt[i].rd, sizeof(rt[i].rd));
				hr = m_pRT[i]->GetLevelDesc(0, &rt[i].rd);
			}

			rt[i].pRT = m_pRT[i];

			rt[i].scale = scale_t(1, 1);

			D3DLOCKED_RECT lr;
			if(FAILED(hr = rt[i].pRT->LockRect(0, &lr, NULL, 0)))
				continue;

			GIFRegTEX0 TEX0;
			TEX0.TBP0 = m_rs.pDISPFB[i]->FBP<<5;
			TEX0.TBW = m_rs.pDISPFB[i]->FBW;
			TEX0.PSM = m_rs.pDISPFB[i]->PSM;

			GIFRegCLAMP CLAMP;
			CLAMP.WMS = CLAMP.WMT = 1;

#ifdef DEBUG_RENDERTARGETS
			if(::GetAsyncKeyState(VK_SPACE)&0x80000000)
			{
				TEX0.TBP0 = m_ctxt->FRAME.Block();
				TEX0.TBW = m_ctxt->FRAME.FBW;
				TEX0.PSM = m_ctxt->FRAME.PSM;
			}

			MSG msg;
			ZeroMemory(&msg, sizeof(msg));
			while(msg.message != WM_QUIT)
			{
				if(PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
				{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
				else if(!(::GetAsyncKeyState(VK_RCONTROL)&0x80000000))
				{
					break;
				}
			}

			if(::GetAsyncKeyState(VK_LCONTROL)&0x80000000)
				Sleep(500);

#endif

			m_lm.ReadTexture(rect, (BYTE*)lr.pBits, lr.Pitch, TEX0, m_de.TEXA, CLAMP);

			rt[i].pRT->UnlockRect(0);
		}
	}

	FinishFlip(rt);
}

template <class Vertex>
void GSRendererSoft<Vertex>::EndFrame()
{
}

template <class Vertex>
void GSRendererSoft<Vertex>::RowInit(int x, int y)
{
	m_faddr_x0 = (m_ctxt->ftbl->pa)(0, y, m_ctxt->FRAME.FBP<<5, m_ctxt->FRAME.FBW);
	m_zaddr_x0 = (m_ctxt->ztbl->pa)(0, y, m_ctxt->ZBUF.ZBP<<5, m_ctxt->FRAME.FBW);

	m_faddr_ro = &m_ctxt->ftbl->rowOffset[y&7][x];
	m_zaddr_ro = &m_ctxt->ztbl->rowOffset[y&7][x];

	m_fx = x-1; // -1 because RowStep() will do +1, yea lame...
	m_fy = y;

	RowStep();
}

template <class Vertex>
void GSRendererSoft<Vertex>::RowStep()
{
	m_fx++;

	m_faddr = m_faddr_x0 + *m_faddr_ro++;
	m_zaddr = m_zaddr_x0 + *m_zaddr_ro++;
}

template <class Vertex>
void GSRendererSoft<Vertex>::DrawPoint(Vertex* v)
{
	CPoint p = *v;
	if(!m_scissor.PtInRect(p))
	{
		RowInit(p.x, p.y);
		(this->*m_pDrawVertex)(*v);
	}
}

template <class Vertex>
void GSRendererSoft<Vertex>::DrawLine(Vertex* v)
{
	Vertex dv = v[1] - v[0];

	Vertex::Vector dp = dv.p;
	dp.x.abs();
	dp.y.abs();

	int dx = (int)dp.x;
	int dy = (int)dp.y;

	if(dx == 0 && dy == 0) return;

	int i = dx > dy ? 0 : 1;

	Vertex edge = v[0];
	Vertex dedge = dv / dp.v[i];

	// TODO: clip with the scissor

	int steps = (int)dp.v[i];

	while(steps-- > 0)
	{
		CPoint p = edge;

		if(m_scissor.PtInRect(p))
		{
			RowInit(p.x, p.y);
			(this->*m_pDrawVertex)(edge);
		}

		edge += dedge;
	}
}

template <class Vertex>
void GSRendererSoft<Vertex>::DrawTriangle(Vertex* v)
{
	if(v[1].p.y < v[0].p.y) {Exchange(&v[0], &v[1]);}
	if(v[2].p.y < v[0].p.y) {Exchange(&v[0], &v[2]);}
	if(v[2].p.y < v[1].p.y) {Exchange(&v[1], &v[2]);}

	if(!(v[0].p.y < v[2].p.y)) return;

	Vertex v01 = v[1] - v[0];
	Vertex v02 = v[2] - v[0];

	Vertex::Scalar temp = v01.p.y / v02.p.y;
	Vertex::Scalar longest = temp * v02.p.x - v01.p.x;

	int ledge, redge;
	if(Vertex::Scalar(0) < longest) {ledge = 0; redge = 1; if(longest < Vertex::Scalar(1)) longest = Vertex::Scalar(1);}
	else if(longest < Vertex::Scalar(0)) {ledge = 1; redge = 0; if(Vertex::Scalar(-1) < longest) longest = Vertex::Scalar(-1);}
	else return;

	Vertex edge[2] = {v[0], v[0]};

	Vertex dedge[2];
	dedge[0].p.y = dedge[1].p.y = Vertex::Scalar(1);
	if(Vertex::Scalar(0) < v01.p.y) dedge[ledge] = v01 / v01.p.y;
	if(Vertex::Scalar(0) < v02.p.y) dedge[redge] = v02 / v02.p.y;

	Vertex scan;

	Vertex dscan = (v02 * temp - v01) / longest;
	dscan.p.y = 0;

	for(int i = 0; i < 2; i++, v++)
	{ 
		int top = edge[0].p.y.ceil_i(), bottom = v[1].p.y.ceil_i();
		if(top < m_scissor.top) top = min(m_scissor.top, bottom);
		if(bottom > m_scissor.bottom) bottom = m_scissor.bottom;
		if(edge[0].p.y < Vertex::Scalar(top)) // for(int j = 0; j < 2; j++) edge[j] += dedge[j] * ((float)top - edge[0].p.y);
		{
			Vertex::Scalar dy = Vertex::Scalar(top) - edge[0].p.y;
			edge[0] += dedge[0] * dy;
			edge[1].p.x += dedge[1].p.x * dy;
			edge[0].p.y = edge[1].p.y = Vertex::Scalar(top);
		}

		ASSERT(top >= bottom || (int)((edge[1].p.y - edge[0].p.y) * 10) == 0);
/*
static struct eb_t {Vertex scan; int top; int left; int steps;} eb[1024];
int _top = top;
int _bottom = bottom;
*/
		for(; top < bottom; top++)
		{
			scan = edge[0];

			int left = edge[0].p.x.ceil_i(), right = edge[1].p.x.ceil_i();
			if(left < m_scissor.left) left = m_scissor.left;
			if(right > m_scissor.right) right = m_scissor.right;
			if(edge[0].p.x < Vertex::Scalar(left))
			{
				scan += dscan * (Vertex::Scalar(left) - edge[0].p.x);
				scan.p.x = Vertex::Scalar(left);
			}
/*
eb[top].scan = scan;
eb[top].top = top;
eb[top].left = left;
eb[top].steps = right - left;
*/
///*
			RowInit(left, top);

			for(int steps = right - left; steps > 0; steps--)
			{
				(this->*m_pDrawVertex)(scan);
				scan += dscan;
				RowStep();
			}
//*/
			// for(int j = 0; j < 2; j++) edge[j] += dedge[j];
			edge[0] += dedge[0];
			edge[1].p += dedge[1].p;
		}
/*
top = _top;
bottom = _bottom;
for(; top < bottom; top++)
{
	eb_t& sl = eb[top];

	RowInit(sl.left, top);

	for(; sl.steps > 0; sl.steps--)
	{
		(this->*m_pDrawVertex)(sl.scan);
		sl.scan += dscan;
		RowStep();
	}
}
*/
		if(v[1].p.y < v[2].p.y)
		{
			edge[ledge] = v[1];
			dedge[ledge] = (v[2] - v[1]) / (v[2].p.y - v[1].p.y);
			edge[ledge] += dedge[ledge] * (edge[ledge].p.y.ceil_s() - edge[ledge].p.y);
		}
	}
}

template <class Vertex>
void GSRendererSoft<Vertex>::DrawSprite(Vertex* v)
{
	if(v[2].p.y < v[0].p.y) {Exchange(&v[0], &v[2]); Exchange(&v[1], &v[3]);}
	if(v[1].p.x < v[0].p.x) {Exchange(&v[0], &v[1]); Exchange(&v[2], &v[3]);}

	if(v[0].p.x == v[1].p.x || v[0].p.y == v[2].p.y) return;

	Vertex v01 = v[1] - v[0];
	Vertex v02 = v[2] - v[0];

	Vertex edge = v[0];
	Vertex dedge = v02 / v02.p.y;
	Vertex scan;
	Vertex dscan = v01 / v01.p.x;

	int top = v[0].p.y.ceil_i(), bottom = v[2].p.y.ceil_i();
	if(top < m_scissor.top) top = min(m_scissor.top, bottom);
	if(bottom > m_scissor.bottom) bottom = m_scissor.bottom;
	if(v[0].p.y < Vertex::Scalar(top)) edge += dedge * (Vertex::Scalar(top) - v[0].p.y);

	int left = v[0].p.x.ceil_i(), right = v[1].p.x.ceil_i();
	if(left < m_scissor.left) left = m_scissor.left;
	if(right > m_scissor.right) right = m_scissor.right;
	if(v[0].p.x < Vertex::Scalar(left)) edge += dscan * (Vertex::Scalar(left) - v[0].p.x);

	if(DrawFilledRect(left, top, right, bottom, edge))
		return;

	for(; top < bottom; top++)
	{
		scan = edge;

		RowInit(left, top);

		for(int steps = right - left; steps > 0; steps--)
		{
			(this->*m_pDrawVertex)(scan);
			scan += dscan;
			RowStep();
		}

		edge += dedge;
	}
}

template <class Vertex>
bool GSRendererSoft<Vertex>::DrawFilledRect(int left, int top, int right, int bottom, const Vertex& v)
{
	if(left >= right || top >= bottom)
		return false;

	ASSERT(top >= 0);
	ASSERT(bottom >= 0);

	if(m_pPRIM->IIP
	|| m_ctxt->TEST.ZTE && m_ctxt->TEST.ZTST != 1
	|| m_ctxt->TEST.ATE && m_ctxt->TEST.ATST != 1
	|| m_ctxt->TEST.DATE
	|| m_pPRIM->TME
	|| m_pPRIM->ABE
	|| m_pPRIM->FGE
	|| m_de.DTHE.DTHE
	|| m_ctxt->FRAME.FBMSK)
		return false;

	DWORD FBP = m_ctxt->FRAME.FBP<<5, FBW = m_ctxt->FRAME.FBW;
	DWORD ZBP = m_ctxt->ZBUF.ZBP<<5;

	if(!m_ctxt->ZBUF.ZMSK)
	{
		m_lm.FillRect(CRect(left, top, right, bottom), v.GetZ(), m_ctxt->ZBUF.PSM, ZBP, FBW);
	}

	__declspec(align(16)) union {struct {short Rf, Gf, Bf, Af;}; UINT64 Cui64;};
	Cui64 = v.c;

	Rf = m_clamp[Rf];
	Gf = m_clamp[Gf];
	Bf = m_clamp[Bf];
	Af |= m_ctxt->FBA.FBA << 7;

	DWORD Cdw;
	
	if(m_ctxt->FRAME.PSM == PSM_PSMCT16 || m_ctxt->FRAME.PSM == PSM_PSMCT16S)
	{
		Cdw = ((DWORD)(Rf&0xf8) >> 3)
			| ((DWORD)(Gf&0xf8) << 2) 
			| ((DWORD)(Bf&0xf8) << 7) 
			| ((DWORD)(Af&0x80) << 8);
	}
	else
	{
#if _M_IX86_FP >= 2 || defined(_M_AMD64)
		__m128i r0 = _mm_load_si128((__m128i*)&Cui64);
		Cdw = (DWORD)_mm_cvtsi128_si32(_mm_packus_epi16(r0, r0));
#else
		Cdw = ((DWORD)(Rf&0xff) << 0)
			| ((DWORD)(Gf&0xff) << 8) 
			| ((DWORD)(Bf&0xff) << 16) 
			| ((DWORD)(Af&0xff) << 24);
#endif
	}

	m_lm.FillRect(CRect(left, top, right, bottom), Cdw, m_ctxt->FRAME.PSM, FBP, FBW);

	return true;
}

template <class Vertex>
template <int iZTST, int iATST>
void GSRendererSoft<Vertex>::DrawVertex(const Vertex& v)
{
	DWORD vz;

	switch(iZTST)
	{
	case 0: return;
	case 1: break;
	case 2: vz = v.GetZ(); if(vz < (m_lm.*m_ctxt->ztbl->rpa)(m_zaddr)) return; break;
	case 3: vz = v.GetZ(); if(vz <= (m_lm.*m_ctxt->ztbl->rpa)(m_zaddr)) return; break;
	default: __assume(0);
	}

	union
	{
		struct {Vertex::Vector Cf, Cd, Ca;};
		struct {Vertex::Vector Cfda[3];};
	};

	Cf = v.c;

	if(m_pPRIM->TME)
	{
		(this->*m_pDrawVertexTFX)(Cf, v);
	}

	if(m_pPRIM->FGE)
	{
		Vertex::Scalar a = Cf.a;
		Vertex::Vector Cfog((DWORD)m_de.FOGCOL.ai32[0]);
		Cf = Cfog + (Cf - Cfog) * v.t.z;
		Cf.a = a;
	}

	BOOL ZMSK = m_ctxt->ZBUF.ZMSK;
	DWORD FBMSK = m_ctxt->FRAME.FBMSK;

	bool fAlphaPass = true;

	BYTE Af = (BYTE)(int)Cf.a;

	switch(iATST)
	{
	case 0: fAlphaPass = false; break;
	case 1: fAlphaPass = true; break;
	case 2: fAlphaPass = Af < m_ctxt->TEST.AREF; break;
	case 3: fAlphaPass = Af <= m_ctxt->TEST.AREF; break;
	case 4: fAlphaPass = Af == m_ctxt->TEST.AREF; break;
	case 5: fAlphaPass = Af >= m_ctxt->TEST.AREF; break;
	case 6: fAlphaPass = Af > m_ctxt->TEST.AREF; break;
	case 7: fAlphaPass = Af != m_ctxt->TEST.AREF; break;
	default: __assume(0);
	}

	if(!fAlphaPass)
	{
		switch(m_ctxt->TEST.AFAIL)
		{
		case 0: return;
		case 1: ZMSK = 1; break; // RGBA
		case 2: FBMSK = 0xffffffff; break; // Z
		case 3: FBMSK = 0xff000000; ZMSK = 1; break; // RGB
		default: __assume(0);
		}
	}

	if(!ZMSK)
	{
		if(iZTST != 2 && iZTST != 3) vz = v.GetZ(); 
		(m_lm.*m_ctxt->ztbl->wpa)(m_zaddr, vz);
	}

	if(FBMSK != ~0)
	{
		if(m_ctxt->TEST.DATE && m_ctxt->FRAME.PSM <= PSM_PSMCT16S && m_ctxt->FRAME.PSM != PSM_PSMCT24)
		{
			BYTE A = (BYTE)((m_lm.*m_ctxt->ftbl->rpa)(m_faddr) >> (m_ctxt->FRAME.PSM == PSM_PSMCT32 ? 31 : 15));
			if(A ^ m_ctxt->TEST.DATM) return;
		}

		// FIXME: for AA1 the value of Af should be calculated from the pixel coverage...

		bool fABE = (m_pPRIM->ABE || m_pPRIM->AA1 && (m_pPRIM->PRIM == 1 || m_pPRIM->PRIM == 2)) && (!m_de.PABE.PABE || (int)Cf.a >= 0x80);

		if(FBMSK || fABE)
		{
			GIFRegTEXA TEXA;
			/*
			TEXA.AEM = 0;
			TEXA.TA0 = 0;
			TEXA.TA1 = 0x80;
			*/
			TEXA.ai32[0] = 0;
			TEXA.ai32[1] = 0x80;
			Cd = (m_lm.*m_ctxt->ftbl->rta)(m_faddr, m_ctxt->TEX0, TEXA);
		}

		if(fABE)
		{
			Ca = Vertex::Vector(Vertex::Scalar(0));
			Ca.a = Vertex::Scalar((int)m_ctxt->ALPHA.FIX);

			Vertex::Scalar a = Cf.a;
			Cf = ((Cfda[m_ctxt->ALPHA.A] - Cfda[m_ctxt->ALPHA.B]) * Cfda[m_ctxt->ALPHA.C].a >> 7) + Cfda[m_ctxt->ALPHA.D];
			Cf.a = a;
		}

		DWORD Cdw; 

		if(m_de.COLCLAMP.CLAMP && !m_de.DTHE.DTHE)
		{
			Cdw = Cf;
		}
		else
		{
			__declspec(align(16)) union {struct {short Rf, Gf, Bf, Af;}; UINT64 Cui64;};
			Cui64 = Cf;

			if(m_de.DTHE.DTHE)
			{
				short DMxy = (signed char)((*((WORD*)&m_de.DIMX.i64 + (m_fy&3)) >> ((m_fx&3)<<2)) << 5) >> 5;
				Rf = (short)(Rf + DMxy);
				Gf = (short)(Gf + DMxy);
				Bf = (short)(Bf + DMxy);
			}

			Rf = m_clamp[Rf];
			Gf = m_clamp[Gf];
			Bf = m_clamp[Bf];
			Af |= m_ctxt->FBA.FBA << 7;

#if _M_IX86_FP >= 2 || defined(_M_AMD64)
			__m128i r0 = _mm_load_si128((__m128i*)&Cui64);
			Cdw = (DWORD)_mm_cvtsi128_si32(_mm_packus_epi16(r0, r0));
#else
			Cdw = ((DWORD)(Rf&0xff) << 0)
				| ((DWORD)(Gf&0xff) << 8) 
				| ((DWORD)(Bf&0xff) << 16) 
				| ((DWORD)(Af&0xff) << 24);
#endif
		}

		if(FBMSK != 0)
		{
			Cdw = (Cdw & ~FBMSK) | ((DWORD)Cd & FBMSK);
		}

		(m_lm.*m_ctxt->ftbl->wfa)(m_faddr, Cdw);
	}
}

static const float one_over_log2 = 1.0f / log(2.0f);

template <class Vertex>
template <int iLOD, bool bLCM, bool bTCC, int iTFX>
void GSRendererSoft<Vertex>::DrawVertexTFX(typename Vertex::Vector& Cf, const Vertex& v)
{
	ASSERT(m_pPRIM->TME);
	
	Vertex::Vector t = v.t;

	bool fBiLinear = iLOD == 2; 

	if(iLOD == 3)
	{
		fBiLinear = bLCM;
	}
	else
	{
		t.q.rcp();
		t *= t.q;

		if(iLOD == 1)
		{
			float lod = (float)(int)m_ctxt->TEX1.K;
			if(!bLCM) lod += log(fabs((float)t.q)) * one_over_log2 * (1 << m_ctxt->TEX1.L);
			fBiLinear = lod <= 0 && (m_ctxt->TEX1.MMAG & 1) || lod > 0 && (m_ctxt->TEX1.MMIN & 1);
		}
	}

	if(fBiLinear) t -= Vertex::Scalar(0.5f);

	__declspec(align(16)) short ituv[8] = 
	{
		(short)(int)t.x, 
		(short)(int)t.x+1, 
		(short)(int)t.y, 
		(short)(int)t.y+1
	};

#if _M_IX86_FP >= 2 || defined(_M_AMD64)

	__m128i uv = _mm_load_si128((__m128i*)ituv);
	__m128i mask = _mm_load_si128((__m128i*)m_uv->mask);
	__m128i region = _mm_or_si128(_mm_and_si128(uv, *(__m128i*)m_uv->and), *(__m128i*)m_uv->or);
	__m128i clamp = _mm_min_epi16(_mm_max_epi16(uv, *(__m128i*)m_uv->min), *(__m128i*)m_uv->max);
	_mm_store_si128((__m128i*)ituv, _mm_or_si128(_mm_and_si128(region, mask), _mm_andnot_si128(mask, clamp)));

#else

	for(int i = 0; i < 4; i++)
	{
		short region = (ituv[i] & m_uv->and[i]) | m_uv->or[i];
		short clamp = ituv[i] < m_uv->min[i] ? m_uv->min[i] : ituv[i] > m_uv->max[i] ? m_uv->max[i] : ituv[i];
		ituv[i] = (region & m_uv->mask[i]) | (clamp & ~m_uv->mask[i]);
	}

#endif

	Vertex::Vector Ct[4];

	if(fBiLinear)
	{
		if(0 && m_pTexture)
		{
			Ct[0] = m_pTexture[(ituv[2] << m_ctxt->TEX0.TW) + ituv[0]];
			Ct[1] = m_pTexture[(ituv[2] << m_ctxt->TEX0.TW) + ituv[1]];
			Ct[2] = m_pTexture[(ituv[3] << m_ctxt->TEX0.TW) + ituv[0]];
			Ct[3] = m_pTexture[(ituv[3] << m_ctxt->TEX0.TW) + ituv[1]];
		}
		else
		{
			Ct[0] = (m_lm.*m_ctxt->ttbl->rt)(ituv[0], ituv[2], m_ctxt->TEX0, m_de.TEXA);
			Ct[1] = (m_lm.*m_ctxt->ttbl->rt)(ituv[1], ituv[2], m_ctxt->TEX0, m_de.TEXA);
			Ct[2] = (m_lm.*m_ctxt->ttbl->rt)(ituv[0], ituv[3], m_ctxt->TEX0, m_de.TEXA);
			Ct[3] = (m_lm.*m_ctxt->ttbl->rt)(ituv[1], ituv[3], m_ctxt->TEX0, m_de.TEXA);
		}

		Vertex::Vector ft = t - t.floor();

		Ct[0] = Ct[0] + (Ct[1] - Ct[0]) * ft.x;
		Ct[2] = Ct[2] + (Ct[3] - Ct[2]) * ft.x;
		Ct[0] = Ct[0] + (Ct[2] - Ct[0]) * ft.y;
	}
	else 
	{
		if(0 && m_pTexture)
		{
			Ct[0] = m_pTexture[(ituv[2] << m_ctxt->TEX0.TW) + ituv[0]];
		}
		else
		{
			Ct[0] = (m_lm.*m_ctxt->ttbl->rt)(ituv[0], ituv[2], m_ctxt->TEX0, m_de.TEXA);
		}
	}

	Vertex::Scalar a = Cf.a;

	switch(iTFX)
	{
	case 0:
		Cf = (Cf * Ct[0] >> 7);
		if(!bTCC) Cf.a = a;
		break;
	case 1:
		Cf = Ct[0];
		break;
	case 2:
		Cf = (Cf * Ct[0] >> 7) + Cf.a;
		Cf.a = !bTCC ? a : (Ct[0].a + a);
		break;
	case 3:
		Cf = (Cf * Ct[0] >> 7) + Cf.a;
		Cf.a = !bTCC ? a : Ct[0].a;
		break;
	default: 
		__assume(0);
	}

	Cf.sat();
}

template <class Vertex>
void GSRendererSoft<Vertex>::SetupTexture()
{
	if(!m_pPRIM->TME) return;
	
	m_lm.SetupCLUT32(m_ctxt->TEX0, m_de.TEXA);

	//

	int tw = 1 << m_ctxt->TEX0.TW;
	int th = 1 << m_ctxt->TEX0.TH;

	switch(m_ctxt->CLAMP.WMS)
	{
	case 0: m_uv->and[0] = (short)(tw-1); m_uv->or[0] = 0; m_uv->mask[0] = 0xffff; break;
	case 1: m_uv->min[0] = 0; m_uv->max[0] = (short)(tw-1); m_uv->mask[0] = 0; break;
	case 2: m_uv->min[0] = (short)m_ctxt->CLAMP.MINU; m_uv->max[0] = (short)m_ctxt->CLAMP.MAXU; m_uv->mask[0] = 0; break;
	case 3: m_uv->and[0] = (short)m_ctxt->CLAMP.MINU; m_uv->or[0] = (short)m_ctxt->CLAMP.MAXU; m_uv->mask[0] = 0xffff; break;
	default: __assume(0);
	}

	m_uv->and[1] = m_uv->and[0];
	m_uv->or[1] = m_uv->or[0];
	m_uv->min[1] = m_uv->min[0];
	m_uv->max[1] = m_uv->max[0];
	m_uv->mask[1] = m_uv->mask[0];

	switch(m_ctxt->CLAMP.WMT)
	{
	case 0: m_uv->and[2] = (short)(th-1); m_uv->or[2] = 0; m_uv->mask[2] = 0xffff; break;
	case 1: m_uv->min[2] = 0; m_uv->max[2] = (short)(th-1); m_uv->mask[2] = 0; break;
	case 2: m_uv->min[2] = (short)m_ctxt->CLAMP.MINV; m_uv->max[2] = (short)m_ctxt->CLAMP.MAXV; m_uv->mask[2] = 0; break;
	case 3: m_uv->and[2] = (short)m_ctxt->CLAMP.MINV; m_uv->or[2] = (short)m_ctxt->CLAMP.MAXV; m_uv->mask[2] = 0xffff; break;
	default: __assume(0);
	}

	m_uv->and[3] = m_uv->and[2];
	m_uv->or[3] = m_uv->or[2];
	m_uv->min[3] = m_uv->min[2];
	m_uv->max[3] = m_uv->max[2];
	m_uv->mask[3] = m_uv->mask[2];
}

//
// GSRendererSoftFP
//

GSRendererSoftFP::GSRendererSoftFP(HWND hWnd, HRESULT& hr)
	: GSRendererSoft<GSSoftVertexFP>(hWnd, hr)
{
}

void GSRendererSoftFP::VertexKick(bool fSkip)
{
	GSSoftVertexFP& v = m_vl.AddTail();

	v.c = (DWORD)m_v.RGBAQ.ai32[0];

	v.p.x = (int)m_v.XYZ.X - (int)m_ctxt->XYOFFSET.OFX;
	v.p.y = (int)m_v.XYZ.Y - (int)m_ctxt->XYOFFSET.OFY;
	v.p *= GSSoftVertexFP::Scalar(1.0f/16);
	v.p.z = (float)(m_v.XYZ.Z >> 16);
	v.p.q = (float)(m_v.XYZ.Z & 0xffff);

	if(m_pPRIM->TME)
	{
		if(m_pPRIM->FST)
		{
			v.t.x = (float)(int)m_v.UV.U;
			v.t.y = (float)(int)m_v.UV.V;
			v.t *= GSSoftVertexFP::Scalar(1.0f/16);
			v.t.q = 1.0f;
		}
		else
		{
			v.t.x = m_v.ST.S * (1 << m_ctxt->TEX0.TW);
			v.t.y = m_v.ST.T * (1 << m_ctxt->TEX0.TH);
			v.t.q = m_v.RGBAQ.Q;
		}
	}

	if(m_pPRIM->FGE)
	{
		v.t.z = (float)m_v.FOG.F * (1.0f/255);
	}

	__super::VertexKick(fSkip);
}
/*
//
// GSRendererSoftFX
//

GSRendererSoftFX::GSRendererSoftFX(HWND hWnd, HRESULT& hr)
	: GSRendererSoft<GSSoftVertexFX>(hWnd, hr)
{
}

void GSRendererSoftFX::VertexKick(bool fSkip)
{
	GSSoftVertexFX& v = m_vl.AddTail();

	v.c = (DWORD)m_v.RGBAQ.ai32[0];

	v.p.x = ((int)m_v.XYZ.X - (int)m_ctxt->XYOFFSET.OFX) << 12;
	v.p.y = ((int)m_v.XYZ.Y - (int)m_ctxt->XYOFFSET.OFY) << 12;
	v.p.z = (int)((m_v.XYZ.Z & 0xffff0000) >> 1);
	v.p.q = (int)((m_v.XYZ.Z & 0x0000ffff) << 15);

	if(m_pPRIM->TME)
	{
		if(m_pPRIM->FST)
		{
			v.t.x = ((int)m_v.UV.U << (12 >> m_ctxt->TEX0.TW));
			v.t.y = ((int)m_v.UV.V << (12 >> m_ctxt->TEX0.TH));
			v.t.q = 1<<16;
		}
		else
		{
			// TODO
			v.t.x = m_v.ST.S;
			v.t.y = m_v.ST.T;
			v.t.q = m_v.RGBAQ.Q;
		}
	}

	if(m_pPRIM->FGE)
	{
		v.t.z = (int)m_v.FOG.F << 8;
	}

	__super::VertexKick(fSkip);
}
*/