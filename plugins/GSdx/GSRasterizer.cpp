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
#include "GSRasterizer.h"

GSRasterizer::GSRasterizer(IDrawScanline* ds, int id, int threads)
	: m_ds(ds)
	, m_id(id)
	, m_threads(threads)
{
}

GSRasterizer::~GSRasterizer()
{
	delete m_ds;
}

void GSRasterizer::Draw(const GSRasterizerData* data)
{
	m_dsf.sl = NULL;
	m_dsf.sr = NULL;
	m_dsf.sp = NULL;
m_dsf.ssl = NULL;
m_dsf.ssp = NULL;

	m_ds->BeginDraw(data, &m_dsf);

	const GSVector4i scissor = data->scissor;
	const GSVertexSW* vertices = data->vertices;
	const int count = data->count;

	m_stats.Reset();

	__int64 start = __rdtsc();

	switch(data->primclass)
	{
	case GS_POINT_CLASS:
		m_stats.prims = count;
		for(int i = 0; i < count; i++) DrawPoint(&vertices[i], scissor);
		break;
	case GS_LINE_CLASS:
		ASSERT(!(count & 1));
		m_stats.prims = count / 2;
		for(int i = 0; i < count; i += 2) DrawLine(&vertices[i], scissor);
		break;
	case GS_TRIANGLE_CLASS:
		ASSERT(!(count % 3));
		m_stats.prims = count / 3;
		for(int i = 0; i < count; i += 3) DrawTriangle(&vertices[i], scissor);
		break;
	case GS_SPRITE_CLASS:
		ASSERT(!(count & 1));
		m_stats.prims = count / 2;
		for(int i = 0; i < count; i += 2) DrawSprite(&vertices[i], scissor);
		break;
	default:
		__assume(0);
	}

	m_stats.ticks = __rdtsc() - start;

	m_ds->EndDraw(m_stats);
}

void GSRasterizer::GetStats(GSRasterizerStats& stats)
{
	stats = m_stats;
}

void GSRasterizer::DrawPoint(const GSVertexSW* v, const GSVector4i& scissor)
{
	// TODO: round to closest for point, prestep for line

	GSVector4i p(v->p);

	if(scissor.x <= p.x && p.x < scissor.z && scissor.y <= p.y && p.y < scissor.w)
	{
		if((p.y % m_threads) == m_id) 
		{
			(m_ds->*m_dsf.sp)(v, *v);
			// TODO: (m_dsf.ssp)(v, *v);

			(m_ds->*m_dsf.sl)(p.y, p.x, p.x + 1, *v);
			// TODO: (m_dsf.ssl)(p.y, p.x, p.x + 1, *v);

			m_stats.pixels++;
		}
	}
}

void GSRasterizer::DrawLine(const GSVertexSW* v, const GSVector4i& scissor)
{
	GSVertexSW dv = v[1] - v[0];

	GSVector4 dp = dv.p.abs();
	GSVector4i dpi(dp);

	if(dpi.y == 0)
	{
		if(dpi.x > 0)
		{
			// shortcut for horizontal lines

			GSVector4 mask = (v[0].p > v[1].p).xxxx();

			GSVertexSW l, dl;

			l.p = v[0].p.blend8(v[1].p, mask);
			l.t = v[0].t.blend8(v[1].t, mask);
			l.c = v[0].c.blend8(v[1].c, mask);

			GSVector4 r;

			r = v[1].p.blend8(v[0].p, mask);

			GSVector4i p(l.p);

			if(scissor.y <= p.y && p.y < scissor.w)
			{
				GSVertexSW dscan = dv / dv.p.xxxx();

				(m_ds->*m_dsf.sp)(v, dscan);

				l.p = l.p.upl(r).xyzw(l.p); // r.x => l.y

				DrawTriangleSection(p.y, p.y + 1, l, dl, dscan, scissor);
			}
		}

		return;
	}

	int i = dpi.x > dpi.y ? 0 : 1;

	GSVertexSW edge = v[0];
	GSVertexSW dedge = dv / dp.v[i];

	// TODO: prestep + clip with the scissor

	int steps = dpi.v[i];

	while(steps-- > 0)
	{
		DrawPoint(&edge, scissor);

		edge += dedge;
	}
}

static const int s_abc[8][4] = 
{
	{0, 1, 2, 0},
	{1, 0, 2, 0},
	{0, 0, 0, 0},
	{1, 2, 0, 0},
	{0, 2, 1, 0},
	{0, 0, 0, 0},
	{2, 0, 1, 0},
	{2, 1, 0, 0},
};

void GSRasterizer::DrawTriangle(const GSVertexSW* vertices, const GSVector4i& scissor)
{
	GSVertexSW v[3];

	GSVector4 aabb = vertices[0].p.yyyy(vertices[1].p);
	GSVector4 bccb = vertices[1].p.yyyy(vertices[2].p).xzzx();

	int i = (aabb > bccb).mask() & 7;

	v[0] = vertices[s_abc[i][0]];
	v[1] = vertices[s_abc[i][1]];
	v[2] = vertices[s_abc[i][2]];

	aabb = v[0].p.yyyy(v[1].p);
	bccb = v[1].p.yyyy(v[2].p).xzzx();

	i = (aabb == bccb).mask() & 7;

	switch(i)
	{
	case 0: // a < b < c
		DrawTriangleTopBottom(v, scissor);
		break;
	case 1: // a == b < c
		DrawTriangleBottom(v, scissor);
		break;
	case 4: // a < b == c
		DrawTriangleTop(v, scissor);
		break;
	case 7: // a == b == c
		break;
	default:
		__assume(0);
	}
}

void GSRasterizer::DrawTriangleTop(GSVertexSW* v, const GSVector4i& scissor)
{
	GSVertexSW longest;

	longest.p = v[2].p - v[1].p;

	int i = (longest.p > GSVector4::zero()).upl(longest.p == GSVector4::zero()).mask();

	if(i & 2) return;

	i &= 1;

	GSVertexSW& l = v[0];
	GSVector4& r = v[0].p;

	GSVector4i tb(l.p.xyxy(v[2].p).ceil());

	int top = tb.extract32<1>();
	int bottom = tb.extract32<3>();

	if(top < scissor.y) top = scissor.y;
	if(bottom > scissor.w) bottom = scissor.w;
	if(top >= bottom) return;

	longest.t = v[2].t - v[1].t;
	longest.c = v[2].c - v[1].c;

	GSVertexSW dscan = longest * longest.p.xxxx().rcp();

	GSVertexSW vl = v[2 - i] - l;
	GSVector4 vr = v[1 + i].p - r;

	GSVertexSW dl = vl / vl.p.yyyy();
	GSVector4 dr = vr / vr.yyyy();

	float py = (float)top - l.p.y;

	l.p = l.p.upl(r).xyzw(l.p); // r.x => l.y
	dl.p = dl.p.upl(dr).xyzw(dl.p); // dr.x => dl.y

	if(py > 0) l += dl * py;

	(m_ds->*m_dsf.sp)(v, dscan);
	// TODO: (m_dsf.ssp)(v, dscan);

	DrawTriangleSection(top, bottom, l, dl, dscan, scissor);
}

void GSRasterizer::DrawTriangleBottom(GSVertexSW* v, const GSVector4i& scissor)
{
	GSVertexSW longest;
	
	longest.p = v[1].p - v[0].p;

	int i = (longest.p > GSVector4::zero()).upl(longest.p == GSVector4::zero()).mask();

	if(i & 2) return;

	i &= 1;

	GSVertexSW& l = v[1 - i];
	GSVector4& r = v[i].p;

	GSVector4i tb(l.p.xyxy(v[2].p).ceil());

	int top = tb.extract32<1>();
	int bottom = tb.extract32<3>();

	if(top < scissor.y) top = scissor.y;
	if(bottom > scissor.w) bottom = scissor.w;
	if(top >= bottom) return;

	longest.t = v[1].t - v[0].t;
	longest.c = v[1].c - v[0].c;

	GSVertexSW dscan = longest * longest.p.xxxx().rcp();

	GSVertexSW vl = v[2] - l;
	GSVector4 vr = v[2].p - r;

	GSVertexSW dl = vl / vl.p.yyyy();
	GSVector4 dr = vr / vr.yyyy();

	float py = (float)top - l.p.y;

	l.p = l.p.upl(r).xyzw(l.p); // r.x => l.y
	dl.p = dl.p.upl(dr).xyzw(dl.p); // dr.x => dl.y

	if(py > 0) l += dl * py;

	(m_ds->*m_dsf.sp)(v, dscan);
	// TODO: (m_dsf.ssp)(v, dscan);

	DrawTriangleSection(top, bottom, l, dl, dscan, scissor);
}

void GSRasterizer::DrawTriangleTopBottom(GSVertexSW* v, const GSVector4i& scissor)
{
	GSVertexSW dv[3];

	dv[0] = v[1] - v[0];
	dv[1] = v[2] - v[0];

	GSVertexSW longest = v[0] + dv[1] * (dv[0].p / dv[1].p).yyyy() - v[1];

	int i = (longest.p > GSVector4::zero()).upl(longest.p == GSVector4::zero()).mask();

	if(i & 2) return;

	i &= 1;

	GSVertexSW dscan = longest * longest.p.xxxx().rcp();

	(m_ds->*m_dsf.sp)(v, dscan);
	// TODO: (m_dsf.ssp)(v, dscan);

	GSVertexSW& l = v[0];
	GSVector4 r = v[0].p;

	GSVertexSW dl;
	GSVector4 dr;

	dl = dv[1 - i] / dv[1 - i].p.yyyy();
	dr = dv[i].p / dv[i].p.yyyy();

	GSVector4i tb(v[0].p.yyyy(v[1].p).xzyy(v[2].p).ceil());

	int top = tb.x;
	int bottom = tb.y;

	if(top < scissor.y) top = scissor.y;
	if(bottom > scissor.w) bottom = scissor.w;

	float py = (float)top - l.p.y;

	if(py > 0)
	{
		GSVector4 dy(py);

		l += dl * dy;
		r += dr * dy;
	}

	if(top < bottom)
	{
		DrawTriangleSection(top, bottom, l, dl, r, dr, dscan, scissor);
	}

	if(i)
	{
		l = v[1];

		dv[2] = v[2] - v[1];

		dl = dv[2] / dv[2].p.yyyy();
	}
	else
	{
		r = v[1].p;

		dv[2].p = v[2].p - v[1].p;

		dr = dv[2].p / dv[2].p.yyyy();
	}

	top = tb.y;
	bottom = tb.z;

	if(top < scissor.y) top = scissor.y;
	if(bottom > scissor.w) bottom = scissor.w;

	if(top < bottom)
	{
		py = (float)top - l.p.y;

		if(py > 0) l += dl * py;

		py = (float)top - r.y;

		if(py > 0) r += dr * py;

		l.p = l.p.upl(r).xyzw(l.p); // r.x => l.y
		dl.p = dl.p.upl(dr).xyzw(dl.p); // dr.x => dl.y

		DrawTriangleSection(top, bottom, l, dl, dscan, scissor);
	}
}

void GSRasterizer::DrawTriangleSection(int top, int bottom, GSVertexSW& l, const GSVertexSW& dl, GSVector4& r, const GSVector4& dr, const GSVertexSW& dscan, const GSVector4i& scissor)
{
	ASSERT(top < bottom);

	while(1)
	{
		do
		{
			if((top % m_threads) == m_id) 
			{
				GSVector4i lr(l.p.xyxy(r).ceil());

				int left = lr.extract32<0>();
				int right = lr.extract32<2>();

				if(left < scissor.x) left = scissor.x;
				if(right > scissor.z) right = scissor.z;

				int pixels = right - left;

				if(pixels > 0)
				{
					m_stats.pixels += pixels;

					GSVertexSW scan;

					float px = (float)left - l.p.x;

					if(px > 0)
					{
						scan = l + dscan * px;
					}
					else
					{
						scan = l;
					}

					(m_ds->*m_dsf.sl)(top, left, right, scan);
					// TODO: (m_dsf.ssl)(top, left, right, scan);
				}
			}
		}
		while(0);

		if(++top >= bottom) break;

		l += dl;
		r += dr;
	}
}


void GSRasterizer::DrawTriangleSection(int top, int bottom, GSVertexSW& l, const GSVertexSW& dl, const GSVertexSW& dscan, const GSVector4i& scissor)
{
	ASSERT(top < bottom);

	while(1)
	{
		do
		{
			if((top % m_threads) == m_id) 
			{
				GSVector4i lr(l.p.ceil());

				int left = lr.extract32<0>();
				int right = lr.extract32<1>();

				if(left < scissor.x) left = scissor.x;
				if(right > scissor.z) right = scissor.z;

				int pixels = right - left;

				if(pixels > 0)
				{
					m_stats.pixels += pixels;

					GSVertexSW scan;

					float px = (float)left - l.p.x;

					if(px > 0)
					{
						scan = l + dscan * px;
					}
					else
					{
						scan = l;
					}

					(m_ds->*m_dsf.sl)(top, left, right, scan);
					// TODO: (m_dsf.ssl)(top, left, right, scan);
				}
			}
		}
		while(0);

		if(++top >= bottom) break;

		l += dl;
	}
}

void GSRasterizer::DrawSprite(const GSVertexSW* vertices, const GSVector4i& scissor)
{
	GSVertexSW v[2];

	GSVector4 mask = (vertices[0].p < vertices[1].p).xyzw(GSVector4::zero());

	v[0].p = vertices[1].p.blend8(vertices[0].p, mask);
	v[0].t = vertices[1].t.blend8(vertices[0].t, mask);
	v[0].c = vertices[1].c;

	v[1].p = vertices[0].p.blend8(vertices[1].p, mask);
	v[1].t = vertices[0].t.blend8(vertices[1].t, mask);

	GSVector4i r(v[0].p.xyxy(v[1].p).ceil());

	int& top = r.y;
	int& bottom = r.w;

	int& left = r.x;
	int& right = r.z;

	#if _M_SSE >= 0x401

	r = r.sat_i32(scissor);
	
	if((r < r.zwzw()).mask() != 0x00ff) return;

	#else

	if(top < scissor.y) top = scissor.y;
	if(bottom > scissor.w) bottom = scissor.w;
	if(top >= bottom) return;

	if(left < scissor.x) left = scissor.x;
	if(right > scissor.z) right = scissor.z;
	if(left >= right) return;

	#endif

	GSVertexSW scan = v[0];

	if(m_dsf.sr)
	{
		if(m_id == 0)
		{
			(m_ds->*m_dsf.sr)(r, scan);

			m_stats.pixels += (r.z - r.x) * (r.w - r.y);
		}

		return;
	}

	GSVector4 zero = GSVector4::zero();

	GSVertexSW dedge, dscan;

	dedge.p = zero;
	dscan.p = zero;

	dedge.c = zero;
	dscan.c = zero;

	GSVertexSW dv = v[1] - v[0];

	dedge.t = (dv.t / dv.p.yyyy()).xyxy(zero).wyww();
	dscan.t = (dv.t / dv.p.xxxx()).xyxy(zero).xwww();

	if(scan.p.y < (float)top) scan.t += dedge.t * ((float)top - scan.p.y);
	if(scan.p.x < (float)left) scan.t += dscan.t * ((float)left - scan.p.x);

	(m_ds->*m_dsf.sp)(v, dscan);
	// TODO: (m_dsf.ssp)(v, dscan);

	for(; top < bottom; top++, scan.t += dedge.t)
	{
		if((top % m_threads) == m_id) 
		{
			(m_ds->*m_dsf.sl)(top, left, right, scan);
			// TODO: (m_dsf.ssl)(top, left, right, scan);

			m_stats.pixels += right - left;
		}
	}
}

//

GSRasterizerMT::GSRasterizerMT(IDrawScanline* ds, int id, int threads, long* sync)
	: GSRasterizer(ds, id, threads)
	, m_sync(sync)
	, m_exit(false)
	, m_ThreadId(0)
	, m_hThread(NULL)
	, m_data(NULL)
{
	if(id > 0)
	{
		m_hThread = CreateThread(NULL, 0, StaticThreadProc, (LPVOID)this, 0, &m_ThreadId);
	}
}

GSRasterizerMT::~GSRasterizerMT()
{
	if(m_hThread != NULL)
	{
		m_exit = true;

		if(WaitForSingleObject(m_hThread, 5000) != WAIT_OBJECT_0)
		{
			TerminateThread(m_hThread, 1);
		}

		CloseHandle(m_hThread);
	}
}

void GSRasterizerMT::Draw(const GSRasterizerData* data)
{
	if(m_id == 0)
	{
		__super::Draw(data);
	}
	else
	{
		m_data = data;

		InterlockedBitTestAndSet(m_sync, m_id);
	}
}

DWORD WINAPI GSRasterizerMT::StaticThreadProc(LPVOID lpParam)
{
	return ((GSRasterizerMT*)lpParam)->ThreadProc();
}

DWORD GSRasterizerMT::ThreadProc()
{
	// _mm_setcsr(MXCSR);

	while(!m_exit)
	{
		if(*m_sync & (1 << m_id))
		{
			__super::Draw(m_data);

			InterlockedBitTestAndReset(m_sync, m_id);
		}
		else
		{
			_mm_pause();
		}
	}

	return 0;
}

//

GSRasterizerList::GSRasterizerList()
{
	// get a whole cache line (twice the size for future cpus ;)

	m_sync = (long*)_aligned_malloc(sizeof(*m_sync), 128);

	*m_sync = 0;
}

GSRasterizerList::~GSRasterizerList()
{
	_aligned_free(m_sync);

	FreeRasterizers();
}

void GSRasterizerList::FreeRasterizers()
{
	while(!IsEmpty()) 
	{
		delete RemoveHead();
	}
}

void GSRasterizerList::Draw(const GSRasterizerData* data)
{
	*m_sync = 0;

	m_stats.Reset();

	__int64 start = __rdtsc();

	POSITION pos = GetTailPosition();

	while(pos)
	{
		GetPrev(pos)->Draw(data);
	}

	while(*m_sync)
	{
		_mm_pause();
	}

	m_stats.ticks = __rdtsc() - start;

	pos = GetHeadPosition();

	while(pos)
	{
		GSRasterizerStats s;

		GetNext(pos)->GetStats(s);

		m_stats.pixels += s.pixels;
		m_stats.prims = max(m_stats.prims, s.prims);
	}
}

void GSRasterizerList::GetStats(GSRasterizerStats& stats)
{
	stats = m_stats;
}

void GSRasterizerList::PrintStats()
{
	if(!IsEmpty())
	{
		GetHead()->PrintStats();
	}
}
