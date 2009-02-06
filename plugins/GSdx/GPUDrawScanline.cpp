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
#include "GPUDrawScanline.h"

GPUDrawScanline::GPUDrawScanline(GPUState* state, int id)
	: m_state(state)
	, m_id(id)
{
}

GPUDrawScanline::~GPUDrawScanline()
{
}

void GPUDrawScanline::BeginDraw(const GSRasterizerData* data, Functions* f)
{
	GPUDrawingEnvironment& env = m_state->m_env;

	const GPUScanlineParam* p = (const GPUScanlineParam*)data->param;

	m_env.sel = p->sel;

	m_env.mem = &m_state->m_mem;

	if(m_env.sel.tme)
	{
		m_env.tex = p->tex;
		m_env.clut = p->clut;

		if(m_env.sel.twin)
		{
			DWORD u, v;

			u = ~(env.TWIN.TWW << 3) & 0xff;
			v = ~(env.TWIN.TWH << 3) & 0xff;

			m_env.u[0] = GSVector4i((u << 16) | u);
			m_env.v[0] = GSVector4i((v << 16) | v);

			u = env.TWIN.TWX << 3;
			v = env.TWIN.TWY << 3;
			
			m_env.u[1] = GSVector4i((u << 16) | u) & ~m_env.u[0];
			m_env.v[1] = GSVector4i((v << 16) | v) & ~m_env.v[0];
		}
	}

	m_env.a = GSVector4i(env.PRIM.ABE ? 0xffffffff : 0);
	m_env.md = GSVector4i(env.STATUS.MD ? 0x80008000 : 0);

	f->sl = m_ds.Lookup(m_env.sel);

	f->sr = NULL; // TODO
	
	DWORD sel = 0;

	sel |= (data->primclass == GS_SPRITE_CLASS ? 1 : 0) << 0;
	sel |= m_env.sel.tme << 1;
	sel |= m_env.sel.iip << 2;

	f->sp = m_sp.Lookup(sel);
}

template<DWORD sprite, DWORD tme, DWORD iip>
void GPUDrawScanline::SetupPrim(const GSVertexSW* vertices, const GSVertexSW& dscan)
{
	if(m_env.sel.tme && !m_env.sel.twin)
	{
		if(sprite)
		{
			GSVector4i t;

			t = (GSVector4i(vertices[1].t) >> 8) - GSVector4i::x00000001();
			t = t.ps32(t);
			t = t.upl16(t);

			m_env.u[2] = t.xxxx();
			m_env.v[2] = t.yyyy();
		}
		else
		{
			m_env.u[2] = GSVector4i::x00ff();
			m_env.v[2] = GSVector4i::x00ff();
		}
	}

	GSVector4 ps0123 = GSVector4::ps0123();
	GSVector4 ps4567 = GSVector4::ps4567();

	GSVector4 dt = dscan.t;
	GSVector4 dc = dscan.c;

	GSVector4i dtc8 = GSVector4i(dt * 8.0f).ps32(GSVector4i(dc * 8.0f));

	if(tme)
	{
		m_env.dst8 = dtc8.upl16(dtc8);

		m_env.ds = GSVector4i(dt.xxxx() * ps0123).ps32(GSVector4i(dt.xxxx() * ps4567));
		m_env.dt = GSVector4i(dt.yyyy() * ps0123).ps32(GSVector4i(dt.yyyy() * ps4567));
	}

	if(iip)
	{
		m_env.dc8 = dtc8.uph16(dtc8);

		m_env.dr = GSVector4i(dc.xxxx() * ps0123).ps32(GSVector4i(dc.xxxx() * ps4567));
		m_env.dg = GSVector4i(dc.yyyy() * ps0123).ps32(GSVector4i(dc.yyyy() * ps4567));
		m_env.db = GSVector4i(dc.zzzz() * ps0123).ps32(GSVector4i(dc.zzzz() * ps4567));
	}
}
void GPUDrawScanline::SampleTexture(DWORD ltf, DWORD tlu, DWORD twin, GSVector4i& test, const GSVector4i& s, const GSVector4i& t, GSVector4i* c)
{
	const void* RESTRICT tex = m_env.tex;
	const WORD* RESTRICT clut = m_env.clut;

	if(ltf)
	{
		GSVector4i u = s.sub16(GSVector4i(0x00200020)); // - 0.125f
		GSVector4i v = t.sub16(GSVector4i(0x00200020)); // - 0.125f

		GSVector4i u0 = u.srl16(8);
		GSVector4i v0 = v.srl16(8);

		GSVector4i u1 = u0.add16(GSVector4i::x0001());
		GSVector4i v1 = v0.add16(GSVector4i::x0001());

		GSVector4i uf = (u & GSVector4i::x00ff()) << 7;
		GSVector4i vf = (v & GSVector4i::x00ff()) << 7;

		if(twin)
		{
			u0 = (u0 & m_env.u[0]).add16(m_env.u[1]);
			v0 = (v0 & m_env.v[0]).add16(m_env.v[1]);
			u1 = (u1 & m_env.u[0]).add16(m_env.u[1]);
			v1 = (v1 & m_env.v[0]).add16(m_env.v[1]);
		}
		else
		{
			u0 = u0.min_i16(m_env.u[2]);
			v0 = v0.min_i16(m_env.v[2]);
			u1 = u1.min_i16(m_env.u[2]);
			v1 = v1.min_i16(m_env.v[2]);
		}

		GSVector4i addr00 = v0.sll16(8) | u0;
		GSVector4i addr01 = v0.sll16(8) | u1;
		GSVector4i addr10 = v1.sll16(8) | u0;
		GSVector4i addr11 = v1.sll16(8) | u1;

		GSVector4i c00, c01, c10, c11;

		if(tlu)
		{
			c00 = addr00.gather16_16((const BYTE*)tex, clut);
			c01 = addr01.gather16_16((const BYTE*)tex, clut);
			c10 = addr10.gather16_16((const BYTE*)tex, clut);
			c11 = addr11.gather16_16((const BYTE*)tex, clut);
		}
		else
		{
			c00 = addr00.gather16_16((const WORD*)tex);
			c01 = addr01.gather16_16((const WORD*)tex);
			c10 = addr00.gather16_16((const WORD*)tex);
			c11 = addr01.gather16_16((const WORD*)tex);
		}

		GSVector4i r00 = (c00 & 0x001f001f) << 3;
		GSVector4i r01 = (c01 & 0x001f001f) << 3;
		GSVector4i r10 = (c10 & 0x001f001f) << 3;
		GSVector4i r11 = (c11 & 0x001f001f) << 3;

		r00 = r00.lerp16<0>(r01, uf);
		r10 = r10.lerp16<0>(r11, uf);
		c[0] = r00.lerp16<0>(r10, vf);

		GSVector4i g00 = (c00 & 0x03e003e0) >> 2;
		GSVector4i g01 = (c01 & 0x03e003e0) >> 2;
		GSVector4i g10 = (c10 & 0x03e003e0) >> 2;
		GSVector4i g11 = (c11 & 0x03e003e0) >> 2;

		g00 = g00.lerp16<0>(g01, uf);
		g10 = g10.lerp16<0>(g11, uf);
		c[1] = g00.lerp16<0>(g10, vf);

		GSVector4i b00 = (c00 & 0x7c007c00) >> 7;
		GSVector4i b01 = (c01 & 0x7c007c00) >> 7;
		GSVector4i b10 = (c10 & 0x7c007c00) >> 7;
		GSVector4i b11 = (c11 & 0x7c007c00) >> 7;

		b00 = b00.lerp16<0>(b01, uf);
		b10 = b10.lerp16<0>(b11, uf);
		c[2] = b00.lerp16<0>(b10, vf);

		GSVector4i a00 = (c00 & 0x80008000) >> 8;
		GSVector4i a01 = (c01 & 0x80008000) >> 8;
		GSVector4i a10 = (c10 & 0x80008000) >> 8;
		GSVector4i a11 = (c11 & 0x80008000) >> 8;

		a00 = a00.lerp16<0>(a01, uf);
		a10 = a10.lerp16<0>(a11, uf);
		c[3] = a00.lerp16<0>(a10, vf).gt16(GSVector4i::zero());

		// mask out blank pixels (not perfect)

		test |= 
			c[0].eq16(GSVector4i::zero()) & 
			c[1].eq16(GSVector4i::zero()) &
			c[2].eq16(GSVector4i::zero()) & 
			c[3].eq16(GSVector4i::zero());
	}
	else
	{
		GSVector4i u = s.srl16(8);
		GSVector4i v = t.srl16(8);

		if(twin)
		{
			u = (u & m_env.u[0]).add16(m_env.u[1]);
			v = (v & m_env.v[0]).add16(m_env.v[1]);
		}
		else
		{
			u = u.min_i16(m_env.u[2]);
			v = v.min_i16(m_env.v[2]);
		}

		GSVector4i addr = v.sll16(8) | u;

		GSVector4i c00;

		if(tlu)
		{
			c00 = addr.gather16_16((const BYTE*)tex, clut);
		}
		else
		{
			c00 = addr.gather16_16((const WORD*)tex);
		}

		test |= c00.eq16(GSVector4i::zero()); // mask out blank pixels

		c[0] = (c00 & 0x001f001f) << 3;
		c[1] = (c00 & 0x03e003e0) >> 2;
		c[2] = (c00 & 0x7c007c00) >> 7;
		c[3] = c00.sra16(15);
	}
}

void GPUDrawScanline::ColorTFX(DWORD tfx, const GSVector4i& r, const GSVector4i& g, const GSVector4i& b, GSVector4i* c)
{
	switch(tfx)
	{
	case 0: // none (tfx = 0)
	case 1: // none (tfx = tge)
		c[0] = r.srl16(7);
		c[1] = g.srl16(7);
		c[2] = b.srl16(7);
		break;
	case 2: // modulate (tfx = tme | tge)
		c[0] = c[0].modulate16<1>(r).clamp8();
		c[1] = c[1].modulate16<1>(g).clamp8();
		c[2] = c[2].modulate16<1>(b).clamp8();
		break;
	case 3: // decal (tfx = tme)
		break;
	default:
		__assume(0);
	}
}

void GPUDrawScanline::AlphaBlend(UINT32 abr, UINT32 tme, const GSVector4i& d, GSVector4i* c)
{
	GSVector4i r = (d & 0x001f001f) << 3;
	GSVector4i g = (d & 0x03e003e0) >> 2;
	GSVector4i b = (d & 0x7c007c00) >> 7;

	switch(abr)
	{
	case 0:
		r = r.avg8(c[0]);
		g = g.avg8(c[0]);
		b = b.avg8(c[0]);
		break;
	case 1:
		r = r.addus8(c[0]);
		g = g.addus8(c[1]);
		b = b.addus8(c[2]);
		break;
	case 2:
		r = r.subus8(c[0]);
		g = g.subus8(c[1]);
		b = b.subus8(c[2]);
		break;
	case 3:
		r = r.addus8(c[0].srl16(2));
		g = g.addus8(c[1].srl16(2));
		b = b.addus8(c[2].srl16(2));
		break;
	default:
		__assume(0);
	}

	if(tme) // per pixel
	{
		c[0] = c[0].blend8(r, c[3]);
		c[1] = c[1].blend8(g, c[3]);
		c[2] = c[2].blend8(b, c[3]);
	}
	else
	{
		c[0] = r;
		c[1] = g;
		c[2] = b;
		c[3] = GSVector4i::zero();
	}
}

void GPUDrawScanline::WriteFrame(WORD* RESTRICT fb, const GSVector4i& test, const GSVector4i* c, int pixels)
{
	GSVector4i r = (c[0] & 0x00f800f8) >> 3;
	GSVector4i g = (c[1] & 0x00f800f8) << 2;
	GSVector4i b = (c[2] & 0x00f800f8) << 7;
	GSVector4i a = (c[3] & 0x00800080) << 8;

	GSVector4i s = r | g | b | a | m_env.md;

	int i = 0;

	do
	{
		if(test.u16[i] == 0)
		{
			fb[i] = s.u16[i];
		}
	}
	while(++i < pixels);
}

//

__declspec(align(16)) static WORD s_dither[4][16] = 
{
	{7, 0, 6, 1, 7, 0, 6, 1, 7, 0, 6, 1, 7, 0, 6, 1},
	{2, 5, 3, 4, 2, 5, 3, 4, 2, 5, 3, 4, 2, 5, 3, 4}, 
	{1, 6, 0, 7, 1, 6, 0, 7, 1, 6, 0, 7, 1, 6, 0, 7}, 
	{4, 3, 5, 2, 4, 3, 5, 2, 4, 3, 5, 2, 4, 3, 5, 2}, 
};

void GPUDrawScanline::DrawScanline(int top, int left, int right, const GSVertexSW& v)	
{
	GSVector4i s, t;
	GSVector4i r, g, b;

	if(m_env.sel.tme)
	{
		GSVector4i vt = GSVector4i(v.t).xxzzl();

		s = vt.xxxx().add16(m_env.ds);
		t = vt.yyyy().add16(m_env.dt);
	}

	GSVector4i vc = GSVector4i(v.c).xxzzlh();

	r = vc.xxxx();
	g = vc.yyyy();
	b = vc.zzzz();

	if(m_env.sel.iip)
	{
		r = r.add16(m_env.dr);
		g = g.add16(m_env.dg);
		b = b.add16(m_env.db);
	}

	GSVector4i dither;

	if(m_env.sel.dtd)
	{
		dither = GSVector4i::load<false>(&s_dither[top & 3][left & 3]);
	}

	int steps = right - left;

	WORD* fb = m_env.mem->GetPixelAddress(left, top);

	while(1)
	{
		do
		{
			int pixels = GSVector4i::min_i16(steps, 8);

			GSVector4i test = GSVector4i::zero();

			GSVector4i d = GSVector4i::zero();

			if(m_env.sel.rfb) // me | abe
			{
				d = GSVector4i::load<false>(fb);

				if(m_env.sel.me)
				{
					test = d.sra16(15);

					if(test.alltrue())
					{
						continue;
					}
				}
			}

			GSVector4i c[4];

			if(m_env.sel.tme)
			{
				SampleTexture(m_env.sel.ltf, m_env.sel.tlu, m_env.sel.twin, test, s, t, c);
			}

			ColorTFX(m_env.sel.tfx, r, g, b, c);

			if(m_env.sel.abe)
			{
				AlphaBlend(m_env.sel.abr, m_env.sel.tme, d, c);
			}

			if(m_env.sel.dtd)
			{
				c[0] = c[0].addus8(dither);
				c[1] = c[1].addus8(dither);
				c[2] = c[2].addus8(dither);
			}

			WriteFrame(fb, test, c, pixels);
		}
		while(0);

		if(steps <= 8) break;

		steps -= 8;

		fb += 8;

		if(m_env.sel.tme)
		{
			GSVector4i dst8 = m_env.dst8;

			s = s.add16(dst8.xxxx());
			t = t.add16(dst8.yyyy());
		}

		if(m_env.sel.iip)
		{
			GSVector4i dc8 = m_env.dc8;

			r = r.add16(dc8.xxxx());
			g = g.add16(dc8.yyyy());
			b = b.add16(dc8.zzzz());
		}
	}
}

template<DWORD sel>
void GPUDrawScanline::DrawScanlineEx(int top, int left, int right, const GSVertexSW& v)
{
	DWORD iip = (sel >> 0) & 1;
	DWORD me = (sel >> 1) & 1;
	DWORD abe = (sel >> 2) & 1;
	DWORD abr = (sel >> 3) & 3;
	// DWORD tge = (sel >> 5) & 1;
	DWORD tme = (sel >> 6) & 1;
	DWORD twin = (sel >> 7) & 1;
	DWORD rfb = (sel >> 1) & 3;
	DWORD tfx = (sel >> 5) & 3;

	GSVector4i s, t;
	GSVector4i r, g, b;

	if(tme)
	{
		GSVector4i vt = GSVector4i(v.t).xxzzl();

		s = vt.xxxx().add16(m_env.ds);
		t = vt.yyyy().add16(m_env.dt);
	}

	GSVector4i vc = GSVector4i(v.c).xxzzlh();

	r = vc.xxxx();
	g = vc.yyyy();
	b = vc.zzzz();

	if(iip)
	{
		r = r.add16(m_env.dr);
		g = g.add16(m_env.dg);
		b = b.add16(m_env.db);
	}

	GSVector4i dither;

	if(m_env.sel.dtd)
	{
		dither = GSVector4i::load<false>(&s_dither[top & 3][left & 3]);
	}

	int steps = right - left;

	WORD* fb = m_env.mem->GetPixelAddress(left, top);

	while(1)
	{
		do
		{
			int pixels = GSVector4i::min_i16(steps, 8);

			GSVector4i test = GSVector4i::zero();

			GSVector4i d = GSVector4i::zero();

			if(rfb) // me | abe
			{
				d = GSVector4i::load<false>(fb);

				if(me)
				{
					test = d.sra16(15);

					if(test.alltrue())
					{
						continue;
					}
				}
			}

			GSVector4i c[4];

			if(tme)
			{
				SampleTexture(m_env.sel.ltf, m_env.sel.tlu, twin, test, s, t, c);
			}

			ColorTFX(tfx, r, g, b, c);

			if(abe)
			{
				AlphaBlend(abr, tme, d, c);
			}

			if(m_env.sel.dtd)
			{
				c[0] = c[0].addus8(dither);
				c[1] = c[1].addus8(dither);
				c[2] = c[2].addus8(dither);
			}

			WriteFrame(fb, test, c, pixels);
		}
		while(0);

		if(steps <= 8) break;

		steps -= 8;

		fb += 8;

		if(tme)
		{
			GSVector4i dst8 = m_env.dst8;

			s = s.add16(dst8.xxxx());
			t = t.add16(dst8.yyyy());
		}

		if(iip)
		{
			GSVector4i dc8 = m_env.dc8;

			r = r.add16(dc8.xxxx());
			g = g.add16(dc8.yyyy());
			b = b.add16(dc8.zzzz());
		}
	}
}

GPUDrawScanline::GPUDrawScanlineMap::GPUDrawScanlineMap()
{
	for(int i = 0; i < countof(m_default); i++)
	{
		m_default[i] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanline;
	}

	#ifdef FAST_DRAWSCANLINE

	m_default[0x00] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x00>;
	m_default[0x01] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x01>;
	m_default[0x02] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x02>;
	m_default[0x03] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x03>;
	m_default[0x04] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x04>;
	m_default[0x05] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x05>;
	m_default[0x06] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x06>;
	m_default[0x07] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x07>;
	m_default[0x08] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x08>;
	m_default[0x09] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x09>;
	m_default[0x0a] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x0a>;
	m_default[0x0b] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x0b>;
	m_default[0x0c] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x0c>;
	m_default[0x0d] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x0d>;
	m_default[0x0e] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x0e>;
	m_default[0x0f] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x0f>;
	m_default[0x10] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x10>;
	m_default[0x11] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x11>;
	m_default[0x12] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x12>;
	m_default[0x13] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x13>;
	m_default[0x14] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x14>;
	m_default[0x15] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x15>;
	m_default[0x16] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x16>;
	m_default[0x17] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x17>;
	m_default[0x18] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x18>;
	m_default[0x19] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x19>;
	m_default[0x1a] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x1a>;
	m_default[0x1b] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x1b>;
	m_default[0x1c] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x1c>;
	m_default[0x1d] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x1d>;
	m_default[0x1e] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x1e>;
	m_default[0x1f] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x1f>;
	m_default[0x20] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x20>;
	m_default[0x21] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x21>;
	m_default[0x22] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x22>;
	m_default[0x23] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x23>;
	m_default[0x24] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x24>;
	m_default[0x25] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x25>;
	m_default[0x26] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x26>;
	m_default[0x27] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x27>;
	m_default[0x28] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x28>;
	m_default[0x29] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x29>;
	m_default[0x2a] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x2a>;
	m_default[0x2b] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x2b>;
	m_default[0x2c] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x2c>;
	m_default[0x2d] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x2d>;
	m_default[0x2e] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x2e>;
	m_default[0x2f] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x2f>;
	m_default[0x30] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x30>;
	m_default[0x31] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x31>;
	m_default[0x32] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x32>;
	m_default[0x33] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x33>;
	m_default[0x34] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x34>;
	m_default[0x35] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x35>;
	m_default[0x36] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x36>;
	m_default[0x37] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x37>;
	m_default[0x38] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x38>;
	m_default[0x39] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x39>;
	m_default[0x3a] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x3a>;
	m_default[0x3b] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x3b>;
	m_default[0x3c] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x3c>;
	m_default[0x3d] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x3d>;
	m_default[0x3e] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x3e>;
	m_default[0x3f] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x3f>;
	m_default[0x40] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x40>;
	m_default[0x41] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x41>;
	m_default[0x42] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x42>;
	m_default[0x43] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x43>;
	m_default[0x44] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x44>;
	m_default[0x45] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x45>;
	m_default[0x46] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x46>;
	m_default[0x47] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x47>;
	m_default[0x48] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x48>;
	m_default[0x49] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x49>;
	m_default[0x4a] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x4a>;
	m_default[0x4b] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x4b>;
	m_default[0x4c] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x4c>;
	m_default[0x4d] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x4d>;
	m_default[0x4e] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x4e>;
	m_default[0x4f] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x4f>;
	m_default[0x50] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x50>;
	m_default[0x51] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x51>;
	m_default[0x52] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x52>;
	m_default[0x53] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x53>;
	m_default[0x54] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x54>;
	m_default[0x55] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x55>;
	m_default[0x56] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x56>;
	m_default[0x57] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x57>;
	m_default[0x58] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x58>;
	m_default[0x59] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x59>;
	m_default[0x5a] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x5a>;
	m_default[0x5b] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x5b>;
	m_default[0x5c] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x5c>;
	m_default[0x5d] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x5d>;
	m_default[0x5e] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x5e>;
	m_default[0x5f] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x5f>;
	m_default[0x60] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x60>;
	m_default[0x61] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x61>;
	m_default[0x62] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x62>;
	m_default[0x63] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x63>;
	m_default[0x64] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x64>;
	m_default[0x65] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x65>;
	m_default[0x66] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x66>;
	m_default[0x67] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x67>;
	m_default[0x68] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x68>;
	m_default[0x69] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x69>;
	m_default[0x6a] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x6a>;
	m_default[0x6b] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x6b>;
	m_default[0x6c] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x6c>;
	m_default[0x6d] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x6d>;
	m_default[0x6e] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x6e>;
	m_default[0x6f] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x6f>;
	m_default[0x70] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x70>;
	m_default[0x71] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x71>;
	m_default[0x72] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x72>;
	m_default[0x73] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x73>;
	m_default[0x74] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x74>;
	m_default[0x75] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x75>;
	m_default[0x76] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x76>;
	m_default[0x77] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x77>;
	m_default[0x78] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x78>;
	m_default[0x79] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x79>;
	m_default[0x7a] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x7a>;
	m_default[0x7b] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x7b>;
	m_default[0x7c] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x7c>;
	m_default[0x7d] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x7d>;
	m_default[0x7e] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x7e>;
	m_default[0x7f] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x7f>;
	m_default[0x80] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x80>;
	m_default[0x81] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x81>;
	m_default[0x82] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x82>;
	m_default[0x83] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x83>;
	m_default[0x84] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x84>;
	m_default[0x85] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x85>;
	m_default[0x86] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x86>;
	m_default[0x87] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x87>;
	m_default[0x88] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x88>;
	m_default[0x89] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x89>;
	m_default[0x8a] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x8a>;
	m_default[0x8b] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x8b>;
	m_default[0x8c] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x8c>;
	m_default[0x8d] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x8d>;
	m_default[0x8e] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x8e>;
	m_default[0x8f] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x8f>;
	m_default[0x90] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x90>;
	m_default[0x91] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x91>;
	m_default[0x92] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x92>;
	m_default[0x93] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x93>;
	m_default[0x94] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x94>;
	m_default[0x95] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x95>;
	m_default[0x96] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x96>;
	m_default[0x97] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x97>;
	m_default[0x98] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x98>;
	m_default[0x99] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x99>;
	m_default[0x9a] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x9a>;
	m_default[0x9b] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x9b>;
	m_default[0x9c] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x9c>;
	m_default[0x9d] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x9d>;
	m_default[0x9e] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x9e>;
	m_default[0x9f] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0x9f>;
	m_default[0xa0] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xa0>;
	m_default[0xa1] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xa1>;
	m_default[0xa2] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xa2>;
	m_default[0xa3] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xa3>;
	m_default[0xa4] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xa4>;
	m_default[0xa5] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xa5>;
	m_default[0xa6] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xa6>;
	m_default[0xa7] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xa7>;
	m_default[0xa8] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xa8>;
	m_default[0xa9] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xa9>;
	m_default[0xaa] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xaa>;
	m_default[0xab] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xab>;
	m_default[0xac] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xac>;
	m_default[0xad] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xad>;
	m_default[0xae] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xae>;
	m_default[0xaf] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xaf>;
	m_default[0xb0] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xb0>;
	m_default[0xb1] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xb1>;
	m_default[0xb2] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xb2>;
	m_default[0xb3] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xb3>;
	m_default[0xb4] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xb4>;
	m_default[0xb5] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xb5>;
	m_default[0xb6] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xb6>;
	m_default[0xb7] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xb7>;
	m_default[0xb8] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xb8>;
	m_default[0xb9] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xb9>;
	m_default[0xba] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xba>;
	m_default[0xbb] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xbb>;
	m_default[0xbc] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xbc>;
	m_default[0xbd] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xbd>;
	m_default[0xbe] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xbe>;
	m_default[0xbf] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xbf>;
	m_default[0xc0] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xc0>;
	m_default[0xc1] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xc1>;
	m_default[0xc2] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xc2>;
	m_default[0xc3] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xc3>;
	m_default[0xc4] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xc4>;
	m_default[0xc5] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xc5>;
	m_default[0xc6] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xc6>;
	m_default[0xc7] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xc7>;
	m_default[0xc8] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xc8>;
	m_default[0xc9] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xc9>;
	m_default[0xca] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xca>;
	m_default[0xcb] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xcb>;
	m_default[0xcc] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xcc>;
	m_default[0xcd] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xcd>;
	m_default[0xce] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xce>;
	m_default[0xcf] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xcf>;
	m_default[0xd0] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xd0>;
	m_default[0xd1] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xd1>;
	m_default[0xd2] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xd2>;
	m_default[0xd3] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xd3>;
	m_default[0xd4] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xd4>;
	m_default[0xd5] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xd5>;
	m_default[0xd6] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xd6>;
	m_default[0xd7] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xd7>;
	m_default[0xd8] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xd8>;
	m_default[0xd9] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xd9>;
	m_default[0xda] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xda>;
	m_default[0xdb] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xdb>;
	m_default[0xdc] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xdc>;
	m_default[0xdd] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xdd>;
	m_default[0xde] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xde>;
	m_default[0xdf] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xdf>;
	m_default[0xe0] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xe0>;
	m_default[0xe1] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xe1>;
	m_default[0xe2] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xe2>;
	m_default[0xe3] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xe3>;
	m_default[0xe4] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xe4>;
	m_default[0xe5] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xe5>;
	m_default[0xe6] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xe6>;
	m_default[0xe7] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xe7>;
	m_default[0xe8] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xe8>;
	m_default[0xe9] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xe9>;
	m_default[0xea] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xea>;
	m_default[0xeb] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xeb>;
	m_default[0xec] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xec>;
	m_default[0xed] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xed>;
	m_default[0xee] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xee>;
	m_default[0xef] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xef>;
	m_default[0xf0] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xf0>;
	m_default[0xf1] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xf1>;
	m_default[0xf2] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xf2>;
	m_default[0xf3] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xf3>;
	m_default[0xf4] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xf4>;
	m_default[0xf5] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xf5>;
	m_default[0xf6] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xf6>;
	m_default[0xf7] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xf7>;
	m_default[0xf8] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xf8>;
	m_default[0xf9] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xf9>;
	m_default[0xfa] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xfa>;
	m_default[0xfb] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xfb>;
	m_default[0xfc] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xfc>;
	m_default[0xfd] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xfd>;
	m_default[0xfe] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xfe>;
	m_default[0xff] = (DrawScanlinePtr)&GPUDrawScanline::DrawScanlineEx<0xff>;

	#endif
}

IDrawScanline::DrawScanlinePtr GPUDrawScanline::GPUDrawScanlineMap::GetDefaultFunction(DWORD dw)
{
	GPUScanlineSelector sel;

	sel.dw = dw;

	return m_default[sel];
}

//

GPUDrawScanline::GPUSetupPrimMap::GPUSetupPrimMap()
{
	#define InitSP_IIP(sprite, tme, iip) \
		m_default[sprite][tme][iip] = (SetupPrimPtr)&GPUDrawScanline::SetupPrim<sprite, tme, iip>; \

	#define InitSP_TME(sprite, tme) \
		InitSP_IIP(sprite, tme, 0) \
		InitSP_IIP(sprite, tme, 1) \

	#define InitSP_SPRITE(sprite) \
		InitSP_TME(sprite, 0) \
		InitSP_TME(sprite, 1) \

	InitSP_SPRITE(0);
	InitSP_SPRITE(1);
}

IDrawScanline::SetupPrimPtr GPUDrawScanline::GPUSetupPrimMap::GetDefaultFunction(DWORD dw)
{
	DWORD sprite = (dw >> 0) & 1;
	DWORD tme = (dw >> 1) & 1;
	DWORD iip = (dw >> 2) & 1;

	return m_default[sprite][tme][iip];
}

