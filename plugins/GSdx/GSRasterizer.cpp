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

// - for more threads screen segments should be smaller to better distribute the pixels
// - but not too small to keep the threading overhead low
// - ideal value between 3 and 5, or log2(64 / number of threads)

#define THREAD_HEIGHT 4

int GSRasterizerData::s_counter = 0;

GSRasterizer::GSRasterizer(IDrawScanline* ds, int id, int threads, GSPerfMon* perfmon)
	: m_ds(ds)
	, m_id(id)
	, m_threads(threads)
	, m_perfmon(perfmon)
	, m_pixels(0)
{
	m_edge.buff = (GSVertexSW*)vmalloc(sizeof(GSVertexSW) * 2048, false);
	m_edge.count = 0;

	m_scanline = (uint8*)_aligned_malloc((2048 >> THREAD_HEIGHT) + 16, 64);

	int row = 0;

	while(row < (2048 >> THREAD_HEIGHT))
	{
		for(int i = 0; i < threads; i++, row++)
		{
			m_scanline[row] = i == id ? 1 : 0;
		}
	}
}

GSRasterizer::~GSRasterizer()
{
	_aligned_free(m_scanline);

	if(m_edge.buff != NULL) vmfree(m_edge.buff, sizeof(GSVertexSW) * 2048);

	delete m_ds;
}

bool GSRasterizer::IsOneOfMyScanlines(int top) const
{
	ASSERT(top >= 0 && top < 2048);

	return m_scanline[top >> THREAD_HEIGHT] != 0;
}

bool GSRasterizer::IsOneOfMyScanlines(int top, int bottom) const
{
	ASSERT(top >= 0 && top < 2048 && bottom >= 0 && bottom < 2048);

	top = top >> THREAD_HEIGHT;
	bottom = (bottom + (1 << THREAD_HEIGHT) - 1) >> THREAD_HEIGHT;

	while(top < bottom)
	{
		if(m_scanline[top++])
		{
			return true;
		}
	}

	return false;
}

int GSRasterizer::FindMyNextScanline(int top) const
{
	int i = top >> THREAD_HEIGHT;

	if(m_scanline[i] == 0)
	{
		while(m_scanline[++i] == 0);

		top = i << THREAD_HEIGHT;
	}

	return top;
}

void GSRasterizer::Queue(shared_ptr<GSRasterizerData> data)
{
	Draw(data.get());
}

int GSRasterizer::GetPixels(bool reset) 
{
	int pixels = m_pixels;
	
	if(reset)
	{
		m_pixels = 0;
	}

	return pixels;
}

void GSRasterizer::Draw(GSRasterizerData* data)
{
	GSPerfMonAutoTimer pmat(m_perfmon, GSPerfMon::WorkerDraw0 + m_id);

	if(data->vertex != NULL && data->vertex_count == 0 || data->index != NULL && data->index_count == 0) return;

	data->start = __rdtsc();

	m_ds->BeginDraw(data);

	const GSVertexSW* vertex = data->vertex;
	const GSVertexSW* vertex_end = data->vertex + data->vertex_count;
	
	const uint32* index = data->index;
	const uint32* index_end = data->index + data->index_count;

	uint32 tmp_index[] = {0, 1, 2};

	bool scissor_test = !data->bbox.eq(data->bbox.rintersect(data->scissor));

	m_scissor = data->scissor;
	m_fscissor_x = GSVector4(data->scissor).xzxz();
	m_fscissor_y = GSVector4(data->scissor).ywyw();

	switch(data->primclass)
	{
	case GS_POINT_CLASS:

		if(scissor_test)
		{
			DrawPoint<true>(vertex, data->vertex_count, index, data->index_count);
		}
		else 
		{
			DrawPoint<false>(vertex, data->vertex_count, index, data->index_count);
		}

		break;

	case GS_LINE_CLASS:
		
		if(index != NULL)
		{
			do {DrawLine(vertex, index); index += 2;}
			while(index < index_end);
		}
		else
		{
			do {DrawLine(vertex, tmp_index); vertex += 2;}
			while(vertex < vertex_end);
		}

		break;

	case GS_TRIANGLE_CLASS:
		
		if(index != NULL)
		{
			do {DrawTriangle(vertex, index); index += 3;}
			while(index < index_end);
		}
		else
		{
			do {DrawTriangle(vertex, tmp_index); vertex += 3;}
			while(vertex < vertex_end);
		}

		break;

	case GS_SPRITE_CLASS:
		
		if(index != NULL)
		{
			do {DrawSprite(vertex, index); index += 2;}
			while(index < index_end);
		}
		else
		{
			do {DrawSprite(vertex, tmp_index); vertex += 2;}
			while(vertex < vertex_end);
		}

		break;

	default:
		__assume(0);
	}

	data->pixels = m_pixels;

	uint64 ticks = __rdtsc() - data->start;

	m_ds->EndDraw(data->frame, ticks, m_pixels);
}

template<bool scissor_test>
void GSRasterizer::DrawPoint(const GSVertexSW* vertex, int vertex_count, const uint32* index, int index_count)
{
	if(index != NULL)
	{
		for(int i = 0; i < index_count; i++, index++)
		{
			const GSVertexSW& v = vertex[*index];

			GSVector4i p(v.p);

			if(!scissor_test || m_scissor.left <= p.x && p.x < m_scissor.right && m_scissor.top <= p.y && p.y < m_scissor.bottom)
			{
				if(IsOneOfMyScanlines(p.y))
				{
					m_pixels++;

					m_ds->SetupPrim(vertex, index, GSVertexSW::zero());

					m_ds->DrawScanline(1, p.x, p.y, v);
				}
			}
		}
	}
	else
	{
		uint32 tmp_index[1] = {0};

		for(int i = 0; i < vertex_count; i++, vertex++)
		{
			const GSVertexSW& v = vertex[0];

			GSVector4i p(v.p);

			if(!scissor_test || m_scissor.left <= p.x && p.x < m_scissor.right && m_scissor.top <= p.y && p.y < m_scissor.bottom)
			{
				if(IsOneOfMyScanlines(p.y))
				{
					m_pixels++;

					m_ds->SetupPrim(vertex, tmp_index, GSVertexSW::zero());

					m_ds->DrawScanline(1, p.x, p.y, v);
				}
			}
		}
	}
}

void GSRasterizer::DrawLine(const GSVertexSW* vertex, const uint32* index)
{
	const GSVertexSW& v0 = vertex[index[0]];
	const GSVertexSW& v1 = vertex[index[1]];
	
	GSVertexSW dv = v1 - v0;

	GSVector4 dp = dv.p.abs();

	int i = (dp < dp.yxwz()).mask() & 1; // |dx| <= |dy|

	if(m_ds->HasEdge())
	{
		DrawEdge(v0, v1, dv, i, 0);
		DrawEdge(v0, v1, dv, i, 1);

		Flush(vertex, index, GSVertexSW::zero(), true);

		return;
	}

	GSVector4i dpi(dp);

	if(dpi.y == 0)
	{
		if(dpi.x > 0)
		{
			// shortcut for horizontal lines

			GSVector4 mask = (v0.p > v1.p).xxxx();

			GSVertexSW scan;

			scan.p = v0.p.blend32(v1.p, mask);
			scan.t = v0.t.blend32(v1.t, mask);
			scan.c = v0.c.blend32(v1.c, mask);

			GSVector4i p(scan.p);

			if(m_scissor.top <= p.y && p.y < m_scissor.bottom && IsOneOfMyScanlines(p.y))
			{
				GSVector4 lrf = scan.p.upl(v1.p.blend32(v0.p, mask)).ceil();
				GSVector4 l = lrf.max(m_fscissor_x);
				GSVector4 r = lrf.min(m_fscissor_x);
				GSVector4i lr = GSVector4i(l.xxyy(r));

				int left = lr.extract32<0>();
				int right = lr.extract32<2>();

				int pixels = right - left;

				if(pixels > 0)
				{
					m_pixels += pixels;

					GSVertexSW dscan = dv / dv.p.xxxx();

					scan += dscan * (l - scan.p).xxxx();

					m_ds->SetupPrim(vertex, index, dscan);

					m_ds->DrawScanline(pixels, left, p.y, scan);
				}
			}
		}

		return;
	}

	int steps = dpi.v[i];

	if(steps > 0)
	{
		GSVertexSW edge = v0;
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

		Flush(vertex, index, GSVertexSW::zero());
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

void GSRasterizer::DrawTriangle(const GSVertexSW* vertex, const uint32* index)
{
	GSVertexSW dv[3];
	GSVertexSW edge;
	GSVertexSW dedge;
	GSVertexSW dscan;

	GSVector4 y0011 = vertex[index[0]].p.yyyy(vertex[index[1]].p);
	GSVector4 y1221 = vertex[index[1]].p.yyyy(vertex[index[2]].p).xzzx();

	int m1 = (y0011 > y1221).mask() & 7;

	int i[3];

	i[0] = index[s_ysort[m1][0]];
	i[1] = index[s_ysort[m1][1]];
	i[2] = index[s_ysort[m1][2]];

	const GSVertexSW& v0 = vertex[i[0]];
	const GSVertexSW& v1 = vertex[i[1]];
	const GSVertexSW& v2 = vertex[i[2]];

	y0011 = v0.p.yyyy(v1.p);
	y1221 = v1.p.yyyy(v2.p).xzzx();

	m1 = (y0011 == y1221).mask() & 7;

	// if(i == 0) => y0 < y1 < y2
	// if(i == 1) => y0 == y1 < y2
	// if(i == 4) => y0 < y1 == y2

	if(m1 == 7) return; // y0 == y1 == y2

	GSVector4 tbf = y0011.xzxz(y1221).ceil();
	GSVector4 tbmax = tbf.max(m_fscissor_y);
	GSVector4 tbmin = tbf.min(m_fscissor_y);
	GSVector4i tb = GSVector4i(tbmax.xzyw(tbmin)); // max(y0, t) max(y1, t) min(y1, b) min(y2, b)

	dv[0] = v1 - v0;
	dv[1] = v2 - v0;
	dv[2] = v2 - v1;

	GSVector4 cross = dv[0].p * dv[1].p.yxwz();

	cross = (cross - cross.yxwz()).yyyy(); // select the second component, the negated cross product

	// the longest horizontal span would be cross.x / dv[1].p.y, but we don't need its actual value

	int m2 = cross.upl(cross == GSVector4::zero()).mask();

	if(m2 & 2) return;

	m2 &= 1;

	cross = cross.rcpnr();

	GSVector4 dxy01 = dv[0].p.xyxy(dv[1].p);

	GSVector4 dx = dxy01.xzxy(dv[2].p);
	GSVector4 dy = dxy01.ywyx(dv[2].p);

	GSVector4 ddx[3];

	ddx[0] = dx / dy;
	ddx[1] = ddx[0].yxzw();
	ddx[2] = ddx[0].xzyw();

	GSVector4 dxy01c = dxy01 * cross;

	/*
	dscan = dv[1] * dxy01c.yyyy() - dv[0] * dxy01c.wwww();
	dedge = dv[0] * dxy01c.zzzz() - dv[1] * dxy01c.xxxx();
	*/

	dscan.p = dv[1].p * dxy01c.yyyy() - dv[0].p * dxy01c.wwww();
	dscan.t = dv[1].t * dxy01c.yyyy() - dv[0].t * dxy01c.wwww();
	dscan.c = dv[1].c * dxy01c.yyyy() - dv[0].c * dxy01c.wwww();

	dedge.p = dv[0].p * dxy01c.zzzz() - dv[1].p * dxy01c.xxxx();
	dedge.t = dv[0].t * dxy01c.zzzz() - dv[1].t * dxy01c.xxxx();
	dedge.c = dv[0].c * dxy01c.zzzz() - dv[1].c * dxy01c.xxxx();

	if(m1 & 1)
	{
		if(tb.y < tb.w)
		{
			edge = vertex[i[1 - m2]];

			edge.p = edge.p.insert<0, 1>(vertex[i[m2]].p);
			dedge.p = ddx[2 - (m2 << 1)].yzzw(dedge.p);

			DrawTriangleSection(tb.x, tb.w, edge, dedge, dscan, vertex[i[1 - m2]].p);
		}
	}
	else
	{
		if(tb.x < tb.z)
		{
			edge = v0;

			edge.p = edge.p.xxzw();
			dedge.p = ddx[m2].xyzw(dedge.p);

			DrawTriangleSection(tb.x, tb.z, edge, dedge, dscan, v0.p);
		}

		if(tb.y < tb.w)
		{
			edge = v1;

			edge.p = (v0.p.xxxx() + ddx[m2] * dv[0].p.yyyy()).xyzw(edge.p);
			dedge.p = ddx[2 - (m2 << 1)].yzzw(dedge.p);

			DrawTriangleSection(tb.y, tb.w, edge, dedge, dscan, v1.p);
		}
	}

	Flush(vertex, index, dscan);

	if(m_ds->HasEdge())
	{
		GSVector4 a = dx.abs() < dy.abs(); // |dx| <= |dy|
		GSVector4 b = dx < GSVector4::zero(); // dx < 0
		GSVector4 c = cross < GSVector4::zero(); // longest.p.x < 0

		int orientation = a.mask();
		int side = ((a | b) ^ c).mask() ^ 2; // evil

		DrawEdge(v0, v1, dv[0], orientation & 1, side & 1);
		DrawEdge(v0, v2, dv[1], orientation & 2, side & 2);
		DrawEdge(v1, v2, dv[2], orientation & 4, side & 4);

		Flush(vertex, index, GSVertexSW::zero(), true);
	}
}

void GSRasterizer::DrawTriangleSection(int top, int bottom, GSVertexSW& edge, const GSVertexSW& dedge, const GSVertexSW& dscan, const GSVector4& p0)
{
	ASSERT(top < bottom);
	ASSERT(edge.p.x <= edge.p.y);

	GSVertexSW* RESTRICT e = &m_edge.buff[m_edge.count];

	GSVector4 scissor = m_fscissor_x;

	top = FindMyNextScanline(top);
	
	while(top < bottom)
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

			GSVector4 prestep = (l - p0).xxxx();

			scan.p += dscan.p * prestep;
			scan.t += dscan.t * prestep;
			scan.c += dscan.c * prestep;

			AddScanline(e++, pixels, left, top, scan);
		}

		top++;

		if(!IsOneOfMyScanlines(top))
		{
			top += (m_threads - 1) << THREAD_HEIGHT;
		}
	}

	m_edge.count += e - &m_edge.buff[m_edge.count];
}

void GSRasterizer::DrawSprite(const GSVertexSW* vertex, const uint32* index)
{
	const GSVertexSW& v0 = vertex[index[0]];
	const GSVertexSW& v1 = vertex[index[1]];

	GSVector4 mask = (v0.p < v1.p).xyzw(GSVector4::zero());

	GSVertexSW v[2];

	v[0].p = v1.p.blend32(v0.p, mask);
	v[0].t = v1.t.blend32(v0.t, mask);
	v[0].c = v1.c;

	v[1].p = v0.p.blend32(v1.p, mask);
	v[1].t = v0.t.blend32(v1.t, mask);

	GSVector4i r(v[0].p.xyxy(v[1].p).ceil());

	r = r.rintersect(m_scissor);

	if(r.rempty()) return;

	GSVertexSW scan = v[0];

	if(m_ds->IsSolidRect())
	{
		if(m_threads == 1)
		{
			m_ds->DrawRect(r, scan);

			m_pixels += r.width() * r.height();
		}
		else
		{
			int top = FindMyNextScanline(r.top);
			int bottom = r.bottom;

			while(top < bottom)
			{
				r.top = top;
				r.bottom = std::min<int>((top + (1 << THREAD_HEIGHT)) & ~((1 << THREAD_HEIGHT) - 1), bottom);

				m_ds->DrawRect(r, scan);
			
				m_pixels += r.width() * r.height();

				top = r.bottom + ((m_threads - 1) << THREAD_HEIGHT);
			}
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

	m_ds->SetupPrim(vertex, index, dscan);

	while(1)
	{
		if(IsOneOfMyScanlines(r.top))
		{
			m_pixels += r.width();

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

	if(orientation)
	{
		GSVector4 tbf = v0.p.yyyy(v1.p).ceil(); // t t b b
		GSVector4 tbmax = tbf.max(m_fscissor_y); // max(t, st) max(t, sb) max(b, st) max(b, sb)
		GSVector4 tbmin = tbf.min(m_fscissor_y); // min(t, st) min(t, sb) min(b, st) min(b, sb)
		GSVector4i tb = GSVector4i(tbmax.xzyw(tbmin)); // max(t, st) max(b, sb) min(t, st) min(b, sb)

		int top, bottom;

		GSVertexSW edge, dedge;

		if((dv.p >= GSVector4::zero()).mask() & 2)
		{
			top = tb.extract32<0>(); // max(t, st)
			bottom = tb.extract32<3>(); // min(b, sb)

			if(top >= bottom) return;

			edge = v0;
			dedge = dv / dv.p.yyyy();

			edge += dedge * (tbmax.xxxx() - edge.p.yyyy());
		}
		else
		{
			top = tb.extract32<1>(); // max(b, st)
			bottom = tb.extract32<2>(); // min(t, sb)

			if(top >= bottom) return;

			edge = v1;
			dedge = dv / dv.p.yyyy();

			edge += dedge * (tbmax.zzzz() - edge.p.yyyy());
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
		GSVector4 lrf = v0.p.xxxx(v1.p).ceil(); // l l r r
		GSVector4 lrmax = lrf.max(m_fscissor_x); // max(l, sl) max(l, sr) max(r, sl) max(r, sr)
		GSVector4 lrmin = lrf.min(m_fscissor_x); // min(l, sl) min(l, sr) min(r, sl) min(r, sr)
		GSVector4i lr = GSVector4i(lrmax.xzyw(lrmin)); // max(l, sl) max(r, sl) min(l, sr) min(r, sr)

		int left, right;

		GSVertexSW edge, dedge;

		if((dv.p >= GSVector4::zero()).mask() & 1)
		{
			left = lr.extract32<0>(); // max(l, sl)
			right = lr.extract32<3>(); // min(r, sr)

			if(left >= right) return;

			edge = v0;
			dedge = dv / dv.p.xxxx();

			edge += dedge * (lrmax.xxxx() - edge.p.xxxx());
		}
		else
		{
			left = lr.extract32<1>(); // max(r, sl)
			right = lr.extract32<2>(); // min(l, sr)

			if(left >= right) return;

			edge = v1;
			dedge = dv / dv.p.xxxx();

			edge += dedge * (lrmax.zzzz() - edge.p.xxxx());
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

void GSRasterizer::Flush(const GSVertexSW* vertex, const uint32* index, const GSVertexSW& dscan, bool edge)
{
	// TODO: on win64 this could be the place where xmm6-15 are preserved (not by each DrawScanline)

	int count = m_edge.count;

	if(count > 0)
	{
		m_ds->SetupPrim(vertex, index, dscan);

		const GSVertexSW* RESTRICT e = m_edge.buff;
		const GSVertexSW* RESTRICT ee = e + count;

		if(!edge)
		{
			do
			{
				int pixels = e->p.i16[0];
				int left = e->p.i16[1];
				int top = e->p.i16[2];

				m_pixels += pixels;

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

				m_pixels += pixels;

				m_ds->DrawEdge(pixels, left, top, *e++);
			}
			while(e < ee);
		}

		m_edge.count = 0;
	}
}

//

GSRasterizerList::GSRasterizerList(int threads, GSPerfMon* perfmon)
	: m_perfmon(perfmon)
{
	m_scanline = (uint8*)_aligned_malloc((2048 >> THREAD_HEIGHT) + 16, 64);

	int row = 0;

	while(row < (2048 >> THREAD_HEIGHT))
	{
		for(int i = 0; i < threads; i++, row++)
		{
			m_scanline[row] = i;
		}
	}
}

GSRasterizerList::~GSRasterizerList()
{
	for(vector<GSWorker*>::iterator i = m_workers.begin(); i != m_workers.end(); i++)
	{
		delete *i;
	}

	_aligned_free(m_scanline);
}

void GSRasterizerList::Queue(shared_ptr<GSRasterizerData> data)
{
	GSVector4i r = data->bbox.rintersect(data->scissor);

	ASSERT(r.top >= 0 && r.top < 2048 && r.bottom >= 0 && r.bottom < 2048);

	int top = r.top >> THREAD_HEIGHT;
	int bottom = std::min<int>((r.bottom + (1 << THREAD_HEIGHT) - 1) >> THREAD_HEIGHT, top + m_workers.size());

	while(top < bottom)
	{
		m_workers[m_scanline[top++]]->Push(data);
	}
}

void GSRasterizerList::Sync()
{
	if(!IsSynced())
	{
		for(size_t i = 0; i < m_workers.size(); i++)
		{
			m_workers[i]->Wait();
		}

		m_perfmon->Put(GSPerfMon::SyncPoint, 1);
	}
}

bool GSRasterizerList::IsSynced() const
{
	for(size_t i = 0; i < m_workers.size(); i++)
	{
		if(!m_workers[i]->IsEmpty())
		{
			return false;
		}
	}

	return true;
}

int GSRasterizerList::GetPixels(bool reset) 
{
	int pixels = 0;
	
	for(size_t i = 0; i < m_workers.size(); i++)
	{
		pixels += m_workers[i]->GetPixels(reset);
	}

	return pixels;
}

// GSRasterizerList::GSWorker

GSRasterizerList::GSWorker::GSWorker(GSRasterizer* r) 
	: GSJobQueue<shared_ptr<GSRasterizerData> >()
	, m_r(r)
{
}

GSRasterizerList::GSWorker::~GSWorker() 
{
	Wait();

	delete m_r;
}

int GSRasterizerList::GSWorker::GetPixels(bool reset)
{
	return m_r->GetPixels(reset);
}

void GSRasterizerList::GSWorker::Process(shared_ptr<GSRasterizerData>& item) 
{
	m_r->Draw(item.get());
}
