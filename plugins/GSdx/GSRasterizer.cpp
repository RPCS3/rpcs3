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

#define THREAD_HEIGHT 5

GSRasterizer::GSRasterizer(IDrawScanline* ds)
	: m_ds(ds)
	, m_id(-1)
	, m_threads(-1)
{
	m_edge.buff = (GSVertexSW*)vmalloc(sizeof(GSVertexSW) * 2048, false);
	m_edge.count = 0;

	m_myscanline = (uint8*)_aligned_malloc((2048 >> THREAD_HEIGHT) + 16, 64);

	Init(0, 1);
}

GSRasterizer::~GSRasterizer()
{
	_aligned_free(m_myscanline);

	if(m_edge.buff != NULL) vmfree(m_edge.buff, sizeof(GSVertexSW) * 2048);

	delete m_ds;
}

bool GSRasterizer::IsOneOfMyScanlines(int scanline) const
{
	return m_myscanline[scanline >> THREAD_HEIGHT] != 0;
}

bool GSRasterizer::IsOneOfMyScanlines(int top, int bottom) const
{
	top >>= THREAD_HEIGHT;
	bottom >>= THREAD_HEIGHT;

	do
	{
		if(m_myscanline[top]) return true;
	}
	while(top++ < bottom);

	return false;
}

void GSRasterizer::Init(int id, int threads) 
{
	if(m_id != id || m_threads != threads)
	{
		m_id = id; 
		m_threads = threads;

		if(threads > 1)
		{
			int row = 0;

			while(row < (2048 >> THREAD_HEIGHT))
			{
				for(int i = 0; i < threads; i++, row++)
				{
					m_myscanline[row] = i == id ? 1 : 0;
				}
			}
		}
		else
		{
			memset(m_myscanline, 1, 2048 >> THREAD_HEIGHT);
		}
	}
}

void GSRasterizer::Draw(const GSRasterizerData* data)
{
	m_ds->BeginDraw(data->param);

	const GSVertexSW* vertices = data->vertices;
	const int count = data->count;

	m_scissor = data->scissor;
	m_fscissor = GSVector4(data->scissor);

	m_stats.Reset();

	uint64 start = __rdtsc();

	// NOTE: data->scissor_test with templated Draw* speeds up large point lists (ffxii videos), but do not seem to make any difference for others

	switch(data->primclass)
	{
	case GS_POINT_CLASS:
		m_stats.prims = count;
		if(data->scissor_test) DrawPoint<true>(vertices, count);
		else DrawPoint<false>(vertices, count);
		break;
	case GS_LINE_CLASS:
		ASSERT(!(count & 1));
		m_stats.prims = count / 2;
		for(int i = 0; i < count; i += 2) DrawLine(&vertices[i]);
		break;
	case GS_TRIANGLE_CLASS:
		ASSERT(!(count % 3));
		m_stats.prims = count / 3;
		for(int i = 0; i < count; i += 3) DrawTriangle(&vertices[i]);
		break;
	case GS_SPRITE_CLASS:
		ASSERT(!(count & 1));
		m_stats.prims = count / 2;
		for(int i = 0; i < count; i += 2) DrawSprite(&vertices[i]);
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

template<bool scissor_test> 
void GSRasterizer::DrawPoint(const GSVertexSW* v, int count)
{
	for(; count > 0; count--, v++)
	{
		GSVector4i p(v->p);

		if(!scissor_test || m_scissor.left <= p.x && p.x < m_scissor.right && m_scissor.top <= p.y && p.y < m_scissor.bottom)
		{
			if(IsOneOfMyScanlines(p.y))
			{
				m_stats.pixels++;

				m_ds->SetupPrim(v, *v);

				m_ds->DrawScanline(1, p.x, p.y, *v);
			}
		}
	}
}

void GSRasterizer::DrawLine(const GSVertexSW* v)
{
	GSVertexSW dv = v[1] - v[0];

	GSVector4 dp = dv.p.abs();

	int i = (dp < dp.yxwz()).mask() & 1; // |dx| <= |dy|

	if(m_ds->IsEdge())
	{
		DrawEdge(v[0], v[1], dv, i, 0);
		DrawEdge(v[0], v[1], dv, i, 1);

		Flush(v, GSVertexSW::zero(), true);

		return;
	}

	GSVector4i dpi(dp);

	if(dpi.y == 0)
	{
		if(dpi.x > 0)
		{
			// shortcut for horizontal lines

			GSVector4 mask = (v[0].p > v[1].p).xxxx();

			GSVertexSW scan;

			scan.p = v[0].p.blend32(v[1].p, mask);
			scan.t = v[0].t.blend32(v[1].t, mask);
			scan.c = v[0].c.blend32(v[1].c, mask);

			GSVector4i p(scan.p);

			if(m_scissor.top <= p.y && p.y < m_scissor.bottom && IsOneOfMyScanlines(p.y))
			{
				GSVector4 scissor = m_fscissor.xzxz();
			
				GSVector4 lrf = scan.p.upl(v[1].p.blend32(v[0].p, mask)).ceil();
				GSVector4 l = lrf.max(scissor);
				GSVector4 r = lrf.min(scissor);
				GSVector4i lr = GSVector4i(l.xxyy(r));

				int left = lr.extract32<0>();
				int right = lr.extract32<2>();

				int pixels = right - left;

				if(pixels > 0)
				{
					m_stats.pixels += pixels;

					GSVertexSW dscan = dv / dv.p.xxxx();

					scan += dscan * (l - scan.p).xxxx();

					m_ds->SetupPrim(v, dscan);

					m_ds->DrawScanline(pixels, left, p.y, scan);
				}
			}
		}

		return;
	}

	int steps = dpi.v[i];

	if(steps > 0)
	{
		GSVertexSW edge = v[0];
		GSVertexSW dedge = dv / GSVector4(dp.v[i]);

		GSVertexSW* RESTRICT e = m_edge.buff;

		while(1)
		{
			GSVector4i p(edge.p);

			if(m_scissor.left <= p.x && p.x < m_scissor.right && m_scissor.top <= p.y && p.y < m_scissor.bottom)
			{
				if(IsOneOfMyScanlines(p.y))
				{
					AddScanline(e, 1, p.x, p.y, edge);

					e++;
				}
			}

			if(--steps == 0) break;

			edge += dedge;
		}

		m_edge.count = e - m_edge.buff;

		Flush(v, GSVertexSW::zero());
	}
}

static const uint8 s_ysort[8][4] =
{
	{0, 1, 2, 0}, // y0 <= y1 <= y2
	{1, 0, 2, 0}, // y1 < y0 <= y2
	{0, 0, 0, 0},
	{1, 2, 0, 0}, // y1 <= y2 < y0
	{0, 2, 1, 0}, // y0 <= y2 < y1
	{0, 0, 0, 0},
	{2, 0, 1, 0}, // y2 < y0 <= y1
	{2, 1, 0, 0}, // y2 < y1 < y0
};

void GSRasterizer::DrawTriangle(const GSVertexSW* vertices)
{
	GSVertexSW v[3];
	GSVertexSW dv[3];
	GSVertexSW edge;
	GSVertexSW dedge;
	GSVertexSW dscan;

	GSVector4 y0011 = vertices[0].p.yyyy(vertices[1].p);
	GSVector4 y1221 = vertices[1].p.yyyy(vertices[2].p).xzzx();

	int mask = (y0011 > y1221).mask() & 7;

	v[0] = vertices[s_ysort[mask][0]];
	v[1] = vertices[s_ysort[mask][1]];
	v[2] = vertices[s_ysort[mask][2]];

	y0011 = v[0].p.yyyy(v[1].p);
	y1221 = v[1].p.yyyy(v[2].p).xzzx();

	int i = (y0011 == y1221).mask() & 7;

	// if(i == 0) => y0 < y1 < y2
	// if(i == 1) => y0 == y1 < y2
	// if(i == 4) => y0 < y1 == y2

	if(i == 7) return; // y0 == y1 == y2

	GSVector4 tbf = y0011.xzxz(y1221).ceil();
	GSVector4 tbmax = tbf.max(m_fscissor.ywyw());
	GSVector4 tbmin = tbf.min(m_fscissor.ywyw());
	GSVector4i tb = GSVector4i(tbmax.xzyw(tbmin));

	if(m_threads > 1 && !IsOneOfMyScanlines(tb.x, tb.w)) return;

	dv[0] = v[1] - v[0];
	dv[1] = v[2] - v[0];
	dv[2] = v[2] - v[1];

	GSVector4 cross = dv[0].p * dv[1].p.yxwz();

	cross = (cross - cross.yxwz()).yyyy(); // select the second component, the negated cross product
	
	// the longest horizontal span would be cross.x / dv[1].p.y, but we don't need its actual value

	int j = cross.upl(cross == GSVector4::zero()).mask();

	if(j & 2) return;

	j &= 1;

	cross = cross.rcpnr();

	GSVector4 dxy01 = dv[0].p.xyxy(dv[1].p);

	GSVector4 dx = dxy01.xzxy(dv[2].p);
	GSVector4 dy = dxy01.ywyx(dv[2].p);

	GSVector4 ddx[3]; 
	
	ddx[0] = dx / dy;
	ddx[1] = ddx[0].yxzw();
	ddx[2] = ddx[0].xzyw();

	GSVector4 dxy01c = dxy01 * cross;

	GSVector4 _z = dxy01c * dv[1].p.zzzz(dv[0].p); // dx0 * z1, dy0 * z1, dx1 * z0, dy1 * z0
	GSVector4 _f = dxy01c * dv[1].p.wwww(dv[0].p); // dx0 * f1, dy0 * f1, dx1 * f0, dy1 * f0

	GSVector4 _zf = _z.ywyw(_f).hsub(_z.zxzx(_f)); // dy0 * z1 - dy1 * z0, dy0 * f1 - dy1 * f0, dx1 * z0 - dx0 * z1, dx1 * f0 - dx0 * f1

	dscan.p = _zf.zwxy(); // dy0 * z1 - dy1 * z0, dy0 * f1 - dy1 * f0
	dedge.p = _zf; // dx1 * z0 - dx0 * z1, dx1 * f0 - dx0 * f1

	GSVector4 _s = dxy01c * dv[1].t.xxxx(dv[0].t); // dx0 * s1, dy0 * s1, dx1 * s0, dy1 * s0
	GSVector4 _t = dxy01c * dv[1].t.yyyy(dv[0].t); // dx0 * t1, dy0 * t1, dx1 * t0, dy1 * t0
	GSVector4 _q = dxy01c * dv[1].t.zzzz(dv[0].t); // dx0 * q1, dy0 * q1, dx1 * q0, dy1 * q0
	
	dscan.t = _s.ywyw(_t).hsub(_q.ywyw()); // dy0 * s1 - dy1 * s0, dy0 * t1 - dy1 * t0, dy0 * q1 - dy1 * q0
	dedge.t = _s.zxzx(_t).hsub(_q.zxzx()); // dx1 * s0 - dx0 * s1, dx1 * t0 - dx0 * t1, dx1 * q0 - dx0 * q1

	GSVector4 _r = dxy01c * dv[1].c.xxxx(dv[0].c); // dx0 * r1, dy0 * r1, dx1 * r0, dy1 * r0
	GSVector4 _g = dxy01c * dv[1].c.yyyy(dv[0].c); // dx0 * g1, dy0 * g1, dx1 * g0, dy1 * g0
	GSVector4 _b = dxy01c * dv[1].c.zzzz(dv[0].c); // dx0 * b1, dy0 * b1, dx1 * b0, dy1 * b0
	GSVector4 _a = dxy01c * dv[1].c.wwww(dv[0].c); // dx0 * a1, dy0 * a1, dx1 * a0, dy1 * a0

	dscan.c = _r.ywyw(_g).hsub(_b.ywyw(_a)); // dy0 * r1 - dy1 * r0, dy0 * g1 - dy1 * g0, dy0 * b1 - dy1 * b0, dy0 * a1 - dy1 * a0
	dedge.c = _r.zxzx(_g).hsub(_b.zxzx(_a)); // dx1 * r0 - dx0 * r1, dx1 * g0 - dx0 * g1, dx1 * b0 - dx0 * b1, dx1 * a0 - dx0 * a1

	if(i & 1)
	{
		if(tb.y < tb.w)
		{
			edge = v[1 - j];

			edge.p = edge.p.insert<0, 1>(v[j].p);
			dedge.p = ddx[2 - (j << 1)].yzzw(dedge.p);

			DrawTriangleSection(tb.x, tb.w, edge, dedge, dscan, v[1 - j].p);
		}
	}
	else
	{
		if(tb.x < tb.z)
		{
			edge = v[0];

			edge.p = edge.p.xxzw();
			dedge.p = ddx[j].xyzw(dedge.p);

			DrawTriangleSection(tb.x, tb.z, edge, dedge, dscan, v[0].p);
		}

		if(tb.y < tb.w)
		{
			edge = v[1];

			edge.p = (v[0].p.xxxx() + ddx[j] * dv[0].p.yyyy()).xyzw(edge.p);
			dedge.p = ddx[2 - (j << 1)].yzzw(dedge.p);

			DrawTriangleSection(tb.y, tb.w, edge, dedge, dscan, v[1].p);
		}
	}

	Flush(v, dscan);

	if(m_ds->IsEdge())
	{
		GSVector4 a = dx.abs() < dy.abs(); // |dx| <= |dy|
		GSVector4 b = dx < GSVector4::zero(); // dx < 0
		GSVector4 c = cross < GSVector4::zero(); // longest.p.x < 0

		int i = a.mask();
		int j = ((a | b) ^ c).mask() ^ 2; // evil

		DrawEdge(v[0], v[1], dv[0], i & 1, j & 1);
		DrawEdge(v[0], v[2], dv[1], i & 2, j & 2);
		DrawEdge(v[1], v[2], dv[2], i & 4, j & 4);

		Flush(v, GSVertexSW::zero(), true);
	}
}

void GSRasterizer::DrawTriangleSection(int top, int bottom, GSVertexSW& edge, const GSVertexSW& dedge, const GSVertexSW& dscan, const GSVector4& p0)
{
	ASSERT(top < bottom);
	ASSERT(edge.p.x <= edge.p.y);

	GSVertexSW* RESTRICT e = &m_edge.buff[m_edge.count];

	GSVector4 scissor = m_fscissor.xzxz();

	do
	{
		if(IsOneOfMyScanlines(top))
		{
			GSVector4 dy = GSVector4(top) - p0.yyyy();

			GSVertexSW scan;
			
			scan.p = edge.p + dedge.p * dy;

			GSVector4 lrf = scan.p.ceil();
			GSVector4 l = lrf.max(scissor);
			GSVector4 r = lrf.min(scissor);
			GSVector4i lr = GSVector4i(l.xxyy(r));

			int left = lr.extract32<0>();
			int right = lr.extract32<2>();

			int pixels = right - left;

			if(pixels > 0)
			{
				scan.t = edge.t + dedge.t * dy;
				scan.c = edge.c + dedge.c * dy;

				AddScanline(e++, pixels, left, top, scan + dscan * (l - p0).xxxx());
			}
		}
	}
	while(++top < bottom);
	
	m_edge.count += e - &m_edge.buff[m_edge.count];
}

void GSRasterizer::DrawSprite(const GSVertexSW* vertices)
{
	GSVertexSW v[2];

	GSVector4 mask = (vertices[0].p < vertices[1].p).xyzw(GSVector4::zero());

	v[0].p = vertices[1].p.blend32(vertices[0].p, mask);
	v[0].t = vertices[1].t.blend32(vertices[0].t, mask);
	v[0].c = vertices[1].c;

	v[1].p = vertices[0].p.blend32(vertices[1].p, mask);
	v[1].t = vertices[0].t.blend32(vertices[1].t, mask);

	GSVector4i r(v[0].p.xyxy(v[1].p).ceil());

	r = r.rintersect(m_scissor);

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

	GSVertexSW dv = v[1] - v[0];

	GSVector4 dt = dv.t / dv.p.xyxy();

	GSVertexSW dedge;
	GSVertexSW dscan;

	dedge.t = GSVector4::zero().insert<1, 1>(dt);
	dscan.t = GSVector4::zero().insert<0, 0>(dt);

	GSVector4 prestep = GSVector4(r.left, r.top) - scan.p;

	int m = (prestep == GSVector4::zero()).mask();

	if((m & 2) == 0) scan.t += dedge.t * prestep.yyyy();
	if((m & 1) == 0) scan.t += dscan.t * prestep.xxxx();

	m_ds->SetupPrim(v, dscan);

	while(1)
	{
		if(IsOneOfMyScanlines(r.top))
		{
			m_stats.pixels += r.width();

			m_ds->DrawScanline(r.width(), r.left, r.top, scan);
		}

		if(++r.top >= r.bottom) break;

		scan.t += dedge.t;
	}
}

void GSRasterizer::DrawEdge(const GSVertexSW& v0, const GSVertexSW& v1, const GSVertexSW& dv, int orientation, int side)
{
	// orientation:
	// - true: |dv.p.y| > |dv.p.x|
	// - false |dv.p.x| > |dv.p.y|
	// side:
	// - true: top/left edge
	// - false: bottom/right edge

	// TODO: bit slow and too much duplicated code
	// TODO: inner pre-step is still missing (hardly noticable)
	// TODO: it does not always line up with the edge of the surrounded triangle

	GSVertexSW* RESTRICT e = &m_edge.buff[m_edge.count];
			
	GSVector4 lrtb = v0.p.upl(v1.p).ceil();

	if(orientation)
	{
		GSVector4 tbmax = lrtb.max(m_fscissor.yyyy());
		GSVector4 tbmin = lrtb.min(m_fscissor.wwww());
		GSVector4i tb = GSVector4i(tbmax.zwzw(tbmin));

		int top, bottom;

		GSVertexSW edge, dedge;

		if((dv.p >= GSVector4::zero()).mask() & 2)
		{
			top = tb.extract32<0>();
			bottom = tb.extract32<3>();

			if(top >= bottom) return;

			edge = v0;
			dedge = dv / dv.p.yyyy();

			edge += dedge * (tbmax.zzzz() - edge.p.yyyy());
		}
		else
		{
			top = tb.extract32<1>();
			bottom = tb.extract32<2>();

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
				int xi = x >> 16;
				int xf = x & 0xffff;

				if(m_scissor.left <= xi && xi < m_scissor.right && IsOneOfMyScanlines(top))
				{
					AddScanline(e, 1, xi, top, edge);

					e->t.u32[3] = (0x10000 - xf) & 0xffff;

					e++;
				}

				if(++top >= bottom) break;

				edge += dedge;
				x += dx;
			}
		}
		else
		{
			while(1)
			{
				int xi = (x >> 16) + 1;
				int xf = x & 0xffff;

				if(m_scissor.left <= xi && xi < m_scissor.right && IsOneOfMyScanlines(top))
				{
					AddScanline(e, 1, xi, top, edge);

					e->t.u32[3] = xf;

					e++;
				}

				if(++top >= bottom) break;

				edge += dedge;
				x += dx;
			}
		}
	}
	else
	{
		GSVector4 lrmax = lrtb.max(m_fscissor.xxxx());
		GSVector4 lrmin = lrtb.min(m_fscissor.zzzz());
		GSVector4i lr = GSVector4i(lrmax.xyxy(lrmin));

		int left, right;

		GSVertexSW edge, dedge;

		if((dv.p >= GSVector4::zero()).mask() & 1)
		{
			left = lr.extract32<0>();
			right = lr.extract32<3>();

			if(left >= right) return;

			edge = v0;
			dedge = dv / dv.p.xxxx();

			edge += dedge * (lrmax.xxxx() - edge.p.xxxx());
		}
		else
		{
			left = lr.extract32<1>();
			right = lr.extract32<2>();

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
				int yi = y >> 16;
				int yf = y & 0xffff;

				if(m_scissor.top <= yi && yi < m_scissor.bottom && IsOneOfMyScanlines(yi))
				{
					AddScanline(e, 1, left, yi, edge);

					e->t.u32[3] = (0x10000 - yf) & 0xffff;

					e++;
				}

				if(++left >= right) break;

				edge += dedge;
				y += dy;
			}
		}
		else
		{
			while(1)
			{
				int yi = (y >> 16) + 1;
				int yf = y & 0xffff;

				if(m_scissor.top <= yi && yi < m_scissor.bottom && IsOneOfMyScanlines(yi))
				{
					AddScanline(e, 1, left, yi, edge);

					e->t.u32[3] = yf;

					e++;
				}

				if(++left >= right) break;

				edge += dedge;
				y += dy;
			}
		}
	}

	m_edge.count += e - &m_edge.buff[m_edge.count];
}

void GSRasterizer::AddScanline(GSVertexSW* e, int pixels, int left, int top, const GSVertexSW& scan)
{
	*e = scan;

	e->p.i16[0] = (int16)pixels;
	e->p.i16[1] = (int16)left;
	e->p.i16[2] = (int16)top;
}

void GSRasterizer::Flush(const GSVertexSW* vertices, const GSVertexSW& dscan, bool edge)
{
	// TODO: on win64 this could be the place where xmm6-15 are preserved (not by each DrawScanline)

	int count = m_edge.count;

	if(count > 0)
	{
		m_ds->SetupPrim(vertices, dscan);

		const GSVertexSW* RESTRICT e = m_edge.buff;
		const GSVertexSW* RESTRICT ee = e + count;

		if(!edge)
		{
			do 
			{
				int pixels = e->p.i16[0];
				int left = e->p.i16[1];
				int top = e->p.i16[2];

				m_stats.pixels += pixels;

				m_ds->DrawScanline(pixels, left, top, *e++);
			}
			while(e < ee);
		}
		else
		{
			do 
			{
				int pixels = e->p.i16[0];
				int left = e->p.i16[1];
				int top = e->p.i16[2];

				m_stats.pixels += pixels;

				m_ds->DrawEdge(pixels, left, top, *e++);
			}
			while(e < ee);
		}

		m_edge.count = 0;
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
	Init(0, 1);

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
	for(size_t i = 0; i < size(); i++)
	{
		delete (*this)[i];
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
		(*this)[i]->Init(i, m_threads);
		(*this)[i]->Draw(data);
	}

	(*this)[0]->Init(0, m_threads);
	(*this)[0]->Draw(data);
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

void GSRasterizerList::GetStats(GSRasterizerStats& stats)
{
	stats = m_stats;
}

void GSRasterizerList::PrintStats()
{
	if(!empty())
	{
		front()->PrintStats();

		/*
		int index = 0;

		for(std::vector<IRasterizer*>::iterator i = begin(); i != end(); i++)
		{
			printf("[Thread %d]\n", index++);

			(*i)->PrintStats();
		}
		*/
	}
}
