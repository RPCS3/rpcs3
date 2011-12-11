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
#include "GPUDrawScanline.h"

GPUDrawScanline::GPUDrawScanline()
	: m_sp_map("GPUSetupPrim", &m_local)
	, m_ds_map("GPUDrawScanline", &m_local)
{
	memset(&m_local, 0, sizeof(m_local));

	m_local.gd = &m_global;
}

GPUDrawScanline::~GPUDrawScanline()
{
}

void GPUDrawScanline::BeginDraw(const void* param)
{
	memcpy(&m_global, param, sizeof(m_global));

	if(m_global.sel.tme && m_global.sel.twin)
	{
		uint32 u, v;

		u = ~(m_global.twin.x << 3) & 0xff; // TWW
		v = ~(m_global.twin.y << 3) & 0xff; // TWH

		m_local.twin[0].u = GSVector4i((u << 16) | u);
		m_local.twin[0].v = GSVector4i((v << 16) | v);

		u = m_global.twin.z << 3; // TWX
		v = m_global.twin.w << 3; // TWY

		m_local.twin[1].u = GSVector4i((u << 16) | u) & ~m_local.twin[0].u;
		m_local.twin[1].v = GSVector4i((v << 16) | v) & ~m_local.twin[0].v;
	}

	m_ds = m_ds_map[m_global.sel];

	m_de = NULL;

	m_dr = NULL; // TODO

	// doesn't need all bits => less functions generated

	GPUScanlineSelector sel;

	sel.key = 0;

	sel.iip = m_global.sel.iip;
	sel.tfx = m_global.sel.tfx;
	sel.twin = m_global.sel.twin;
	sel.sprite = m_global.sel.sprite;

	m_sp = m_sp_map[sel];
}

void GPUDrawScanline::EndDraw(const GSRasterizerStats& stats, uint64 frame)
{
	m_ds_map.UpdateStats(stats, frame);
}

void GPUDrawScanline::PrintStats() 
{
	m_ds_map.PrintStats();
}

#ifndef JIT_DRAW

void GPUDrawScanline::SetupPrim(const GSVertexSW* vertices, const GSVertexSW& dscan)
{
	GPUScanlineSelector sel = m_global.sel;

	const GSVector4* shift = GPUSetupPrimCodeGenerator::m_shift;

	if(sel.tme && !sel.twin)
	{
		if(sel.sprite)
		{
			GSVector4i t = (GSVector4i(vertices[1].t) >> 8) - GSVector4i::x00000001();

			t = t.ps32(t);
			t = t.upl16(t);
			
			m_local.twin[2].u = t.xxxx();
			m_local.twin[2].v = t.yyyy();
		}
		else
		{
			// TODO: not really needed

			m_local.twin[2].u = GSVector4i::x00ff();
			m_local.twin[2].v = GSVector4i::x00ff();
		}
	}

	if(sel.tme || sel.iip && sel.tfx != 3)
	{
		GSVector4 dt = dscan.t;
		GSVector4 dc = dscan.c;

		GSVector4i dtc8 = GSVector4i(dt * shift[0]).ps32(GSVector4i(dc * shift[0]));

		if(sel.tme)
		{
			m_local.d8.st = dtc8.upl16(dtc8);
		}

		if(sel.iip && sel.tfx != 3)
		{
			m_local.d8.c = dtc8.uph16(dtc8);
		}

		if(sel.tme)
		{
			GSVector4 dtx = dt.xxxx();
			GSVector4 dty = dt.yyyy();

			m_local.d.s = GSVector4i(dtx * shift[1]).ps32(GSVector4i(dtx * shift[2]));
			m_local.d.t = GSVector4i(dty * shift[1]).ps32(GSVector4i(dty * shift[2]));
		}

		if(sel.iip && sel.tfx != 3)
		{
			GSVector4 dcx = dc.xxxx();
			GSVector4 dcy = dc.yyyy();
			GSVector4 dcz = dc.zzzz();

			m_local.d.r = GSVector4i(dcx * shift[1]).ps32(GSVector4i(dcx * shift[2]));
			m_local.d.g = GSVector4i(dcy * shift[1]).ps32(GSVector4i(dcy * shift[2]));
			m_local.d.b = GSVector4i(dcz * shift[1]).ps32(GSVector4i(dcz * shift[2]));
		}
	}
}

void GPUDrawScanline::DrawScanline(int pixels, int left, int top, const GSVertexSW& scan)
{
	// TODO: not tested yet, probably bogus

	GPUScanlineSelector sel = m_global.sel;

	GSVector4i s, t;
	GSVector4i uf, vf;
	GSVector4i rf, gf, bf;
	GSVector4i dither;

	// Init

	uint16* fb = (uint16*)m_global.vm + (top << (10 + sel.scalex)) + left;

	int steps = pixels - 8;

	if(sel.dtd)
	{
		dither = GSVector4i::load<false>(&GPUDrawScanlineCodeGenerator::m_dither[top & 3][left & 3]);
	}

	if(sel.tme)
	{
		GSVector4i vt = GSVector4i(scan.t).xxzzl();

		s = vt.xxxx().add16(m_local.d.s);
		t = vt.yyyy();

		if(!sel.sprite)
		{
			t = t.add16(m_local.d.t);
		}
		else
		{
			if(sel.ltf)
			{
				vf = t.sll16(1).srl16(1);
			}
		}
	}

	if(sel.tfx != 3)
	{
		GSVector4i vc = GSVector4i(scan.c).xxzzlh();

		rf = vc.xxxx();
		gf = vc.yyyy();
		bf = vc.zzzz();

		if(sel.iip)
		{
			rf = rf.add16(m_local.d.r);
			gf = gf.add16(m_local.d.g);
			bf = bf.add16(m_local.d.b);
		}
	}

	while(1)
	{
		do
		{
			GSVector4i test = GPUDrawScanlineCodeGenerator::m_test[7 + (steps & (steps >> 31))];

			GSVector4i fd = GSVector4i::load(fb, fb + 8);

			GSVector4i r, g, b, a;

			// TestMask

			if(sel.me)
			{
				test |= fd.sra16(15);

				if(test.alltrue()) continue;
			}

			// SampleTexture

			if(sel.tme)
			{
				GSVector4i u0, v0, u1, v1;
				GSVector4i addr00, addr01, addr10, addr11;
				GSVector4i c00, c01, c10, c11;

				if(sel.ltf)
				{
					u0 = s.sub16(GSVector4i(0x00200020)); // - 0.125f
					v0 = t.sub16(GSVector4i(0x00200020)); // - 0.125f

					uf = u0.sll16(8).srl16(1);
					vf = v0.sll16(8).srl16(1);;
				}
				else
				{
					u0 = s;
					v0 = t;
				}
				
				u0 = u0.srl16(8);
				v0 = v0.srl16(8);

				if(sel.ltf)
				{
					u1 = u0.add16(GSVector4i::x0001());
					v1 = v0.add16(GSVector4i::x0001());

					if(sel.twin)
					{
						u0 = (u0 & m_local.twin[0].u).add16(m_local.twin[1].u);
						v0 = (v0 & m_local.twin[0].v).add16(m_local.twin[1].v);
						u1 = (u1 & m_local.twin[0].u).add16(m_local.twin[1].u);
						v1 = (v1 & m_local.twin[0].v).add16(m_local.twin[1].v);
					}
					else
					{
						u0 = u0.min_i16(m_local.twin[2].u);
						v0 = v0.min_i16(m_local.twin[2].v);
						u1 = u1.min_i16(m_local.twin[2].u);
						v1 = v1.min_i16(m_local.twin[2].v);
					}

					addr00 = v0.sll16(8) | u0;
					addr01 = v0.sll16(8) | u1;
					addr10 = v1.sll16(8) | u0;
					addr11 = v1.sll16(8) | u1;

					// TODO

					if(sel.tlu)
					{
						c00 = addr00.gather16_16((const uint16*)m_global.vm, m_global.clut);
						c01 = addr01.gather16_16((const uint16*)m_global.vm, m_global.clut);
						c10 = addr10.gather16_16((const uint16*)m_global.vm, m_global.clut);
						c11 = addr11.gather16_16((const uint16*)m_global.vm, m_global.clut);
					}
					else
					{
						c00 = addr00.gather16_16((const uint16*)m_global.vm);
						c01 = addr01.gather16_16((const uint16*)m_global.vm);
						c10 = addr10.gather16_16((const uint16*)m_global.vm);
						c11 = addr11.gather16_16((const uint16*)m_global.vm);
					}

					GSVector4i r00 = c00.sll16(11).srl16(8);
					GSVector4i r01 = c01.sll16(11).srl16(8);
					GSVector4i r10 = c10.sll16(11).srl16(8);
					GSVector4i r11 = c11.sll16(11).srl16(8);

					r00 = r00.lerp16<0>(r01, uf);
					r10 = r10.lerp16<0>(r11, uf);

					GSVector4i g00 = c00.sll16(6).srl16(11).sll16(3);
					GSVector4i g01 = c01.sll16(6).srl16(11).sll16(3);
					GSVector4i g10 = c10.sll16(6).srl16(11).sll16(3);
					GSVector4i g11 = c11.sll16(6).srl16(11).sll16(3);

					g00 = g00.lerp16<0>(g01, uf);
					g10 = g10.lerp16<0>(g11, uf);

					GSVector4i b00 = c00.sll16(1).srl16(11).sll16(3);
					GSVector4i b01 = c01.sll16(1).srl16(11).sll16(3);
					GSVector4i b10 = c10.sll16(1).srl16(11).sll16(3);
					GSVector4i b11 = c11.sll16(1).srl16(11).sll16(3);

					b00 = b00.lerp16<0>(b01, uf);
					b10 = b10.lerp16<0>(b11, uf);

					GSVector4i a00 = c00.sra16(15).sll16(8);
					GSVector4i a01 = c01.sra16(15).sll16(8);
					GSVector4i a10 = c10.sra16(15).sll16(8);
					GSVector4i a11 = c11.sra16(15).sll16(8);

					a00 = a00.lerp16<0>(a01, uf);
					a10 = a10.lerp16<0>(a11, uf);

					r = r00.lerp16<0>(r10, vf);
					g = g00.lerp16<0>(g10, vf);
					b = b00.lerp16<0>(b10, vf);
					a = a00.lerp16<0>(a10, vf);

					test |= (r | g | b | a).eq16(GSVector4i::zero()); // mask out blank pixels (not perfect)

					a = a.gt16(GSVector4i::zero());
				}
				else
				{
					if(sel.twin)
					{
						u0 = (u0 & m_local.twin[0].u).add16(m_local.twin[1].u);
						v0 = (v0 & m_local.twin[0].v).add16(m_local.twin[1].v);
					}
					else
					{
						u0 = u0.min_i16(m_local.twin[2].u);
						v0 = v0.min_i16(m_local.twin[2].v);
					}

					addr00 = v0.sll16(8) | u0;

					// TODO

					if(sel.tlu)
					{
						c00 = addr00.gather16_16((const uint16*)m_global.vm, m_global.clut);
					}
					else
					{
						c00 = addr00.gather16_16((const uint16*)m_global.vm);
					}

					r = (c00 << 3) & 0x00f800f8;
					g = (c00 >> 2) & 0x00f800f8;
					b = (c00 >> 7) & 0x00f800f8;
					a = c00.sra16(15);

					test |= c00.eq16(GSVector4i::zero()); // mask out blank pixels
				}
			}

			// ColorTFX

			switch(sel.tfx)
			{
			case 0: // none (tfx = 0)
			case 1: // none (tfx = tge)
				r = rf.srl16(7);
				g = gf.srl16(7);
				b = bf.srl16(7);
				break;
			case 2: // modulate (tfx = tme | tge)
				r = r.modulate16<1>(rf).clamp8();
				g = g.modulate16<1>(gf).clamp8();
				b = b.modulate16<1>(bf).clamp8();
				break;
			case 3: // decal (tfx = tme)
				break;
			default:
				__assume(0);
			}

			// AlphaBlend

			if(sel.abe)
			{
				GSVector4i rs = r;
				GSVector4i gs = g;
				GSVector4i bs = b;
				GSVector4i rd = (fd & 0x001f001f) << 3;
				GSVector4i gd = (fd & 0x03e003e0) >> 2;
				GSVector4i bd = (fd & 0x7c007c00) >> 7;

				switch(sel.abr)
				{
				case 0:
					r = rd.avg8(rs);
					g = gd.avg8(gs);
					b = bd.avg8(bs);
					break;
				case 1:
					r = rd.addus8(rs);
					g = gd.addus8(gs);
					b = bd.addus8(bs);
					break;
				case 2:
					r = rd.subus8(rs);
					g = gd.subus8(gs);
					b = bd.subus8(bs);
					break;
				case 3:
					r = rd.addus8(rs.srl16(2));
					g = gd.addus8(gs.srl16(2));
					b = bd.addus8(bs.srl16(2));
					break;
				default:
					__assume(0);
				}

				if(sel.tme)
				{
					r = rs.blend8(rd, a);
					g = gs.blend8(gd, a);
					b = bs.blend8(bd, a);
				}
			}

			// Dither

			if(sel.dtd)
			{
				r = r.addus8(dither);
				g = g.addus8(dither);
				b = b.addus8(dither);
			}
			
			// WriteFrame

			GSVector4i fs = r | g | b | (sel.md ? GSVector4i(0x80008000) : sel.tme ? a : GSVector4i::zero());

			fs = fs.blend8(fd, test);

			GSVector4i::store(fb, fb + 8, fs);
		}
		while(0);

		if(steps <= 0) break;

		steps -= 8;

		fb += 8;

		if(sel.tme)
		{
			GSVector4i st = m_local.d8.st;

			s = s.add16(st.xxxx());
			t = t.add16(st.yyyy());
		}

		if(sel.tfx != 3) // != decal
		{
			if(sel.iip)
			{
				GSVector4i c = m_local.d8.c;

				rf = rf.add16(c.xxxx());
				gf = gf.add16(c.yyyy());
				bf = bf.add16(c.zzzz());
			}
		}
	}
}

void GPUDrawScanline::DrawEdge(int pixels, int left, int top, const GSVertexSW& scan)
{
	ASSERT(0);
}

void GPUDrawScanline::DrawRect(const GSVector4i& r, const GSVertexSW& v)
{
	// TODO
}

#endif