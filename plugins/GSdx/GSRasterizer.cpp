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

// TODO: JIT Draw* (flags: depth, texture, color (+iip), scissor)

#include "stdafx.h"
#include "GSRasterizer.h"

// Using a spinning finish on the main (MTGS) thread is apparently a big win still, over trying
// to wait out all the pending m_finished semaphores.  It leaves one spinwait in the rasterizer,
// but that's still worlds better than 2-6 spinning threads like before.

// NOTE: spinning: 100-500 ticks, waiting: 1000-5000 ticks

//
#define UseSpinningFinish

// Set this to 1 to remove a lot of non-const div/modulus ops from the rasterization process.
// Might likely be a measurable speedup but limits threading to 1, 2, 4, and 8 threads.
// note by rama: Speedup is around 5% on average.

// #define UseConstThreadCount

#ifdef UseConstThreadCount
	// ThreadsConst - const number of threads.  User-configured threads (in GSdx panel) must match
	// this value if UseConstThreadCount is enabled. [yeah, it's hacky for now]
	static const int ThreadsConst = 2;
	static const int ThreadMaskConst = ThreadsConst - 1;
#endif

// align threads to page height (1 << 5)

#define THREAD_HEIGHT 5

GSRasterizer::GSRasterizer(IDrawScanline* ds)
	: m_ds(ds)
	, m_id(0)
	, m_threads(1)
{
}

GSRasterizer::~GSRasterizer()
{
	delete m_ds;
}

__forceinline bool GSRasterizer::IsOneOfMyScanlines(int scanline) const
{
	#ifdef UseConstThreadCount

	return ThreadMaskConst == 0 || (scanline & ThreadMaskConst) == m_id;

	#else

	return m_threads == 1 || ((scanline >> THREAD_HEIGHT) % m_threads) == m_id;

	#endif
}

void GSRasterizer::Draw(const GSRasterizerData* data)
{
	m_ds->BeginDraw(data->param);

	const GSVector4i scissor = data->scissor;
	const GSVertexSW* vertices = data->vertices;
	const int count = data->count;

	m_stats.Reset();

	int64 start = __rdtsc();

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

	m_ds->EndDraw(m_stats, data->frame);
}

void GSRasterizer::GetStats(GSRasterizerStats& stats)
{
	stats = m_stats;
}

void GSRasterizer::DrawPoint(const GSVertexSW* v, const GSVector4i& scissor)
{
	// TODO: round to closest for point, prestep for line

	GSVector4i p(v->p);

	if(scissor.left <= p.x && p.x < scissor.right && scissor.top <= p.y && p.y < scissor.bottom)
	{
		if(IsOneOfMyScanlines(p.y))
		{
			m_ds->SetupPrim(v, *v);

			m_ds->DrawScanline(p.x + 1, p.x, p.y, *v);

			m_stats.pixels++;
		}
	}
}

void GSRasterizer::DrawLine(const GSVertexSW* v, const GSVector4i& scissor)
{
	GSVertexSW dv = v[1] - v[0];

	GSVector4 dp = dv.p.abs();

	if(m_ds->IsEdge())
	{
		int i = (dp < dp.yxwz()).mask() & 1; // |x| <= |y|

		GSVertexSW dscan;

		dscan.p = GSVector4::zero();
		dscan.t = GSVector4::zero();
		dscan.c = GSVector4::zero();

		m_ds->SetupPrim(v, dscan);

		DrawEdge(v[0], v[1], dv, scissor, i, 0);
		DrawEdge(v[0], v[1], dv, scissor, i, 1);

		return;
	}

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

			if(scissor.top <= p.y && p.y < scissor.bottom)
			{
				GSVertexSW dscan = dv / dv.p.xxxx();

				m_ds->SetupPrim(v, dscan);

				l.p = l.p.upl(r).xyzw(l.p); // r.x => l.y

				GSVector4 fscissor(scissor);

				DrawTriangleSection(p.y, p.y + 1, l, dl, dscan, fscissor);
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
	{0, 1, 2, 0}, // c >= b >= a
	{1, 0, 2, 0}, // c >= a > b
	{0, 0, 0, 0},
	{1, 2, 0, 0}, // a > c >= b
	{0, 2, 1, 0}, // b > c >= a
	{0, 0, 0, 0},
	{2, 0, 1, 0}, // b >= a > c
	{2, 1, 0, 0}, // a > b > c
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

	if(m_ds->IsEdge())
	{
		DrawEdge(v, scissor);
	}

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

void GSRasterizer::DrawEdge(const GSVertexSW* v, const GSVector4i& scissor)
{
	GSVertexSW dv[3];

	dv[0] = v[1] - v[0];
	dv[1] = v[2] - v[0];
	dv[2] = v[2] - v[1];

	GSVector4 dx = dv[0].p.upl(dv[1].p).xyxy(dv[2].p);
	GSVector4 dy = dv[0].p.upl(dv[1].p).zwyx(dv[2].p);

	GSVector4 a = dx.abs() < dy.abs(); // |x| <= |y|
	GSVector4 b = dx < GSVector4::zero(); // x < 0
	GSVector4 c = dv[1].p * (dv[0].p / dv[1].p).yyyy() < dv[0].p; // longest.p.x < 0

	int i = a.mask();
	int j = ((a | b) ^ c.xxxx()).mask() ^ 2; // evil

	GSVertexSW dscan;

	dscan.p = GSVector4::zero();
	dscan.t = GSVector4::zero();
	dscan.c = GSVector4::zero();

	m_ds->SetupPrim(v, dscan); // TODO: don't call it twice (can't be sure about the second call if the triangle is too small)

	DrawEdge(v[0], v[1], dv[0], scissor, i & 1, j & 1);
	DrawEdge(v[0], v[2], dv[1], scissor, i & 2, j & 2);
	DrawEdge(v[1], v[2], dv[2], scissor, i & 4, j & 4);
}

void GSRasterizer::DrawTriangleTop(GSVertexSW* v, const GSVector4i& scissor)
{
	GSVertexSW longest;

	longest.p = v[2].p - v[1].p;

	int i = longest.p.upl(longest.p == GSVector4::zero()).mask();

	if(i & 2) return;

	i &= 1;

	GSVertexSW& l = v[0];
	GSVector4& r = v[0].p;

	GSVector4 fscissor(scissor);

	GSVector4 tb = l.p.upl(v[2].p).ceil();

	GSVector4 tbmax = tb.max(fscissor.yyyy());
	GSVector4 tbmin = tb.min(fscissor.wwww());

	GSVector4i tbi = GSVector4i(tbmax.zzww(tbmin));

	int top = tbi.extract32<0>();
	int bottom = tbi.extract32<2>();

	if(top >= bottom) return;

	longest.t = v[2].t - v[1].t;
	longest.c = v[2].c - v[1].c;

	GSVertexSW dscan = longest * longest.p.xxxx().rcp();

	GSVertexSW vl = v[1 + i] - l;
	GSVector4 vr = v[2 - i].p - r;

	GSVertexSW dl = vl / vl.p.yyyy();
	GSVector4 dr = vr / vr.yyyy();

	GSVector4 dy = tbmax.zzzz() - l.p.yyyy();

	l.p = l.p.upl(r).xyzw(l.p); // r.x => l.y
	dl.p = dl.p.upl(dr).xyzw(dl.p); // dr.x => dl.y

	l += dl * dy;

	m_ds->SetupPrim(v, dscan);

	DrawTriangleSection(top, bottom, l, dl, dscan, fscissor);
}

void GSRasterizer::DrawTriangleBottom(GSVertexSW* v, const GSVector4i& scissor)
{
	GSVertexSW longest;

	longest.p = v[1].p - v[0].p;

	int i = longest.p.upl(longest.p == GSVector4::zero()).mask();

	if(i & 2) return;

	i &= 1;

	GSVertexSW& l = v[i];
	GSVector4& r = v[1 - i].p;

	GSVector4 fscissor(scissor);

	GSVector4 tb = l.p.upl(v[2].p).ceil();

	GSVector4 tbmax = tb.max(fscissor.yyyy());
	GSVector4 tbmin = tb.min(fscissor.wwww());

	GSVector4i tbi = GSVector4i(tbmax.zzww(tbmin));

	int top = tbi.extract32<0>();
	int bottom = tbi.extract32<2>();

	if(top >= bottom) return;

	longest.t = v[1].t - v[0].t;
	longest.c = v[1].c - v[0].c;

	GSVertexSW dscan = longest * longest.p.xxxx().rcp();

	GSVertexSW vl = v[2] - l;
	GSVector4 vr = v[2].p - r;

	GSVertexSW dl = vl / vl.p.yyyy();
	GSVector4 dr = vr / vr.yyyy();

	GSVector4 dy = tbmax.zzzz() - l.p.yyyy();

	l.p = l.p.upl(r).xyzw(l.p); // r.x => l.y
	dl.p = dl.p.upl(dr).xyzw(dl.p); // dr.x => dl.y

	l += dl * dy;

	m_ds->SetupPrim(v, dscan);

	DrawTriangleSection(top, bottom, l, dl, dscan, fscissor);
}

void GSRasterizer::DrawTriangleTopBottom(GSVertexSW* v, const GSVector4i& scissor)
{
	GSVertexSW dv[3];

	dv[0] = v[1] - v[0];
	dv[1] = v[2] - v[0];

	GSVertexSW longest = dv[1] * (dv[0].p / dv[1].p).yyyy() - dv[0];

	int i = longest.p.upl(longest.p == GSVector4::zero()).mask();

	if(i & 2) return;

	i &= 1;

	GSVertexSW dscan = longest * longest.p.xxxx().rcp();

	m_ds->SetupPrim(v, dscan);

	GSVector4 fscissor(scissor);

	GSVector4 tb = v[0].p.upl(v[1].p).zwzw(v[1].p.upl(v[2].p)).ceil();

	GSVector4 tbmax = tb.max(fscissor.yyyy());
	GSVector4 tbmin = tb.min(fscissor.wwww());

	GSVector4i tbi = GSVector4i(tbmax.xzyw(tbmin));

	int top = tbi.extract32<0>();
	int bottom = tbi.extract32<2>();

	GSVertexSW& l = v[0];
	GSVector4 r = v[0].p;

	GSVertexSW dl = dv[i] / dv[i].p.yyyy();
	GSVector4 dr = dv[1 - i].p / dv[1 - i].p.yyyy();

	GSVector4 dy = tbmax.xxxx() - l.p.yyyy();

	l += dl * dy;
	r += dr * dy;

	if(top < bottom)
	{
		DrawTriangleSection(top, bottom, l, dl, r, dr, dscan, fscissor);
	}

	top = tbi.y;
	bottom = tbi.w;

	if(top < bottom)
	{
		if(i == 0)
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

		l += dl * (tbmax.zzzz() - l.p.yyyy());
		r += dr * (tbmax.zzzz() - r.yyyy());

		l.p = l.p.upl(r).xyzw(l.p); // r.x => l.y
		dl.p = dl.p.upl(dr).xyzw(dl.p); // dr.x => dl.y

		DrawTriangleSection(top, bottom, l, dl, dscan, fscissor);
	}
}

void GSRasterizer::DrawTriangleSection(int top, int bottom, GSVertexSW& l, const GSVertexSW& dl, GSVector4& r, const GSVector4& dr, const GSVertexSW& dscan, const GSVector4& fscissor)
{
	ASSERT(top < bottom);

	while(1)
	{
		do
		{
			if(IsOneOfMyScanlines(top))
			{
				GSVector4 lr = l.p.xyxy(r).ceil();

				GSVector4 lrmax = lr.max(fscissor.xxxx());
				GSVector4 lrmin = lr.min(fscissor.zzzz());

				GSVector4i lri = GSVector4i(lrmax.xxzz(lrmin));

				int left = lri.extract32<0>();
				int right = lri.extract32<2>();

				int pixels = right - left;

				if(pixels > 0)
				{
					m_stats.pixels += pixels;

					GSVertexSW scan = l + dscan * (lrmax - l.p).xxxx();

					m_ds->DrawScanline(right, left, top, scan);
				}
			}
		}
		while(0);

		if(++top >= bottom) break;

		l += dl;
		r += dr;
	}
}

void GSRasterizer::DrawTriangleSection(int top, int bottom, GSVertexSW& l, const GSVertexSW& dl, const GSVertexSW& dscan, const GSVector4& fscissor)
{
	ASSERT(top < bottom);

	while(1)
	{
		do
		{
			if(IsOneOfMyScanlines(top))
			{
				GSVector4 lr = l.p.ceil();

				GSVector4 lrmax = lr.max(fscissor.xxxx());
				GSVector4 lrmin = lr.min(fscissor.zzzz());

				GSVector4i lri = GSVector4i(lrmax.xxyy(lrmin));

				int left = lri.extract32<0>();
				int right = lri.extract32<2>();

				int pixels = right - left;

				if(pixels > 0)
				{
					m_stats.pixels += pixels;

					GSVertexSW scan = l + dscan * (lrmax - l.p).xxxx();

					m_ds->DrawScanline(right, left, top, scan);
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

	r = r.rintersect(scissor);

	if(r.rempty()) return;

	GSVertexSW scan = v[0];

	if(m_ds->IsRect())
	{
		if(m_id == 0)
		{
			m_ds->DrawRect(r, scan);

			m_stats.pixels += r.width() * r.height();
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

	if(scan.p.y < (float)r.top) scan.t += dedge.t * ((float)r.top - scan.p.y);
	if(scan.p.x < (float)r.left) scan.t += dscan.t * ((float)r.left - scan.p.x);

	m_ds->SetupPrim(v, dscan);

	for(; r.top < r.bottom; r.top++, scan.t += dedge.t)
	{
		if(IsOneOfMyScanlines(r.top))
		{
			m_ds->DrawScanline(r.right, r.left, r.top, scan);

			m_stats.pixels += r.width();
		}
	}
}

void GSRasterizer::DrawEdge(const GSVertexSW& v0, const GSVertexSW& v1, const GSVertexSW& dv, const GSVector4i& scissor, int orientation, int side)
{
	// orientation:
	// - true: |dv.p.y| > |dv.p.x|
	// - false |dv.p.x| > |dv.p.y|
	// side:
	// - true: top/left edge
	// - false: bottom/right edge

	// TODO: bit slow and too much duplicated code
	// TODO: inner pre-step is still missing (hardly noticable)

	GSVector4 fscissor(scissor);

	GSVector4 lrtb = v0.p.upl(v1.p).ceil();

	if(orientation)
	{
		GSVector4 tbmax = lrtb.max(fscissor.yyyy());
		GSVector4 tbmin = lrtb.min(fscissor.wwww());

		GSVector4i tbi = GSVector4i(tbmax.zwzw(tbmin));

		int top, bottom;

		GSVertexSW edge, dedge;

		if((dv.p >= GSVector4::zero()).mask() & 2)
		{
			top = tbi.extract32<0>();
			bottom = tbi.extract32<3>();

			if(top >= bottom) return;

			edge = v0;
			dedge = dv / dv.p.yyyy();

			edge += dedge * (tbmax.zzzz() - edge.p.yyyy());
		}
		else
		{
			top = tbi.extract32<1>();
			bottom = tbi.extract32<2>();

			if(top >= bottom) return;

			edge = v1;
			dedge = dv / dv.p.yyyy();

			edge += dedge * (tbmax.wwww() - edge.p.yyyy());
		}

		GSVector4i p = GSVector4i(edge.p.upl(dedge.p) * 0x10000);

		int x = p.extract32<0>();
		int dx = p.extract32<1>();

		if(side)
		{
			while(1)
			{
				do
				{
					int xi = x >> 16;
					int xf = x & 0xffff;

					if(scissor.left <= xi && xi < scissor.right && IsOneOfMyScanlines(xi))
					{
						m_stats.pixels++;

						edge.t.u32[3] = (0x10000 - xf) & 0xffff;

						m_ds->DrawEdge(xi + 1, xi, top, edge);

						edge.t.u32[3] = 0;
					}
				}
				while(0);

				if(++top >= bottom) break;

				edge += dedge;
				x += dx;
			}
		}
		else
		{
			while(1)
			{
				do
				{
					int xi = (x >> 16) + 1;
					int xf = x & 0xffff;

					if(scissor.left <= xi && xi < scissor.right && IsOneOfMyScanlines(xi))
					{
						m_stats.pixels++;

						edge.t.u32[3] = xf;

						m_ds->DrawEdge(xi + 1, xi, top, edge);

						edge.t.u32[3] = 0;
					}
				}
				while(0);

				if(++top >= bottom) break;

				edge += dedge;
				x += dx;
			}
		}
	}
	else
	{
		GSVector4 lrmax = lrtb.max(fscissor.xxxx());
		GSVector4 lrmin = lrtb.min(fscissor.zzzz());

		GSVector4i lri = GSVector4i(lrmax.xyxy(lrmin));

		int left, right;

		GSVertexSW edge, dedge;

		if((dv.p >= GSVector4::zero()).mask() & 1)
		{
			left = lri.extract32<0>();
			right = lri.extract32<3>();

			if(left >= right) return;

			edge = v0;
			dedge = dv / dv.p.xxxx();

			edge += dedge * (lrmax.xxxx() - edge.p.xxxx());
		}
		else
		{
			left = lri.extract32<1>();
			right = lri.extract32<2>();

			if(left >= right) return;

			edge = v1;
			dedge = dv / dv.p.xxxx();

			edge += dedge * (lrmax.yyyy() - edge.p.xxxx());
		}

		GSVector4i p = GSVector4i(edge.p.upl(dedge.p) * 0x10000);

		int y = p.extract32<2>();
		int dy = p.extract32<3>();

		if(side)
		{
			while(1)
			{
				do
				{
					int yi = y >> 16;
					int yf = y & 0xffff;

					if(scissor.top <= yi && yi < scissor.bottom && IsOneOfMyScanlines(yi))
					{
						m_stats.pixels++;

						edge.t.u32[3] = (0x10000 - yf) & 0xffff;

						m_ds->DrawEdge(left + 1, left, yi, edge);

						edge.t.u32[3] = 0;
					}
				}
				while(0);

				if(++left >= right) break;

				edge += dedge;
				y += dy;
			}
		}
		else
		{
			while(1)
			{
				do
				{
					int yi = (y >> 16) + 1;
					int yf = y & 0xffff;

					if(scissor.top <= yi && yi < scissor.bottom && IsOneOfMyScanlines(yi))
					{
						m_stats.pixels++;

						edge.t.u32[3] = yf;

						m_ds->DrawEdge(left + 1, left, yi, edge);

						edge.t.u32[3] = 0;
					}
				}
				while(0);

				if(++left >= right) break;

				edge += dedge;
				y += dy;
			}
		}
	}
}

//

GSRasterizerMT::GSRasterizerMT(IDrawScanline* ds, volatile long& sync)
	: GSRasterizer(ds)
	, m_sync(sync)
	, m_data(NULL)
{
	CreateThread();
}

GSRasterizerMT::~GSRasterizerMT()
{
    Draw(NULL);

	CloseThread();
}

void GSRasterizerMT::Draw(const GSRasterizerData* data)
{
	m_data = data;

	m_draw.Set();
}

void GSRasterizerMT::ThreadProc()
{
	while(m_draw.Wait() && m_data != NULL)
	{
		GSRasterizer::Draw(m_data);

        _interlockedbittestandreset(&m_sync, m_id);
	}
}

//

GSRasterizerList::GSRasterizerList()
	: m_sync(0)
{
}

GSRasterizerList::~GSRasterizerList()
{
	for(size_t i = 0; i < size(); i++) delete (*this)[i];
}

void GSRasterizerList::Sync()
{
	while(m_sync) _mm_pause();

	m_stats.ticks = __rdtsc() - m_start;

	for(int i = 0; i < m_threads; i++)
	{
		GSRasterizerStats s;

		(*this)[i]->GetStats(s);

		m_stats.pixels += s.pixels;
		m_stats.prims = std::max<int>(m_stats.prims, s.prims);
	}
}

void GSRasterizerList::Draw(const GSRasterizerData* data, int width, int height)
{
	m_stats.Reset();

	m_start = __rdtsc();

	m_threads = std::min<int>(1 + (height >> THREAD_HEIGHT), size());

	m_sync = 0;

	for(int i = 1; i < m_threads; i++)
	{
		m_sync |= 1 << i;
	}

	for(int i = 1; i < m_threads; i++)
	{
		(*this)[i]->SetThreadId(i, m_threads);
		(*this)[i]->Draw(data);
	}

	(*this)[0]->SetThreadId(0, m_threads);
	(*this)[0]->Draw(data);
}

void GSRasterizerList::GetStats(GSRasterizerStats& stats)
{
	stats = m_stats;
}

void GSRasterizerList::PrintStats()
{
	if(!empty())
	{
		front()->PrintStats();
	}
}
