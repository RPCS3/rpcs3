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
#include "GSDrawScanline.h"
#include "GSTextureCacheSW.h"

GSDrawScanline::GSDrawScanline(GSState* state, int id)
	: m_state(state)
	, m_id(id)
{
	memset(&m_env, 0, sizeof(m_env));
}

GSDrawScanline::~GSDrawScanline()
{
}

void GSDrawScanline::BeginDraw(const GSRasterizerData* data, Functions* f)
{
	GSDrawingEnvironment& env = m_state->m_env;
	GSDrawingContext* context = m_state->m_context;

	const GSScanlineParam* p = (const GSScanlineParam*)data->param;

	m_env.sel = p->sel;

	m_env.vm = p->vm;
	m_env.fbr = p->fbo->row;
	m_env.zbr = p->zbo->row;
	m_env.fbc = p->fbo->col;
	m_env.zbc = p->zbo->col;
	m_env.fzbr = p->fzbo->row;
	m_env.fzbc = p->fzbo->col;
	m_env.fm = GSVector4i(p->fm);
	m_env.zm = GSVector4i(p->zm);
	m_env.datm = context->TEST.DATM ? GSVector4i::x80000000() : GSVector4i::zero();
	m_env.colclamp = env.COLCLAMP.CLAMP ? GSVector4i::xffffffff() : GSVector4i::x00ff();
	m_env.fba = context->FBA.FBA ? GSVector4i::x80000000() : GSVector4i::zero();
	m_env.aref = GSVector4i((int)context->TEST.AREF);
	m_env.afix = GSVector4i((int)context->ALPHA.FIX << 16);
	m_env.afix2 = m_env.afix.yywwlh().sll16(7);
	m_env.frb = GSVector4i((int)env.FOGCOL.ai32[0] & 0x00ff00ff);
	m_env.fga = GSVector4i((int)(env.FOGCOL.ai32[0] >> 8) & 0x00ff00ff);

	if(m_env.sel.fpsm == 1)
	{
		m_env.fm |= GSVector4i::xff000000();
	}
	else if(m_env.sel.fpsm == 2)
	{
		GSVector4i rb = m_env.fm & 0x00f800f8;
		GSVector4i ga = m_env.fm & 0x8000f800;

		m_env.fm = (ga >> 16) | (rb >> 9) | (ga >> 6) | (rb >> 3) | GSVector4i::xffff0000();
	}

	if(m_env.sel.zpsm == 1)
	{
		m_env.zm |= GSVector4i::xff000000();
	}
	else if(m_env.sel.zpsm == 2)
	{
		m_env.zm |= GSVector4i::xffff0000();
	}

	if(m_env.sel.atst == ATST_LESS)
	{
		m_env.sel.atst = ATST_LEQUAL;

		m_env.aref -= GSVector4i::x00000001();
	}
	else if(m_env.sel.atst == ATST_GREATER)
	{
		m_env.sel.atst = ATST_GEQUAL;

		m_env.aref += GSVector4i::x00000001();
	}

	if(m_env.sel.tfx != TFX_NONE)
	{
		m_env.tex = p->tex;
		m_env.clut = p->clut;
		m_env.tw = p->tw;

		unsigned short tw = (unsigned short)(1 << context->TEX0.TW);
		unsigned short th = (unsigned short)(1 << context->TEX0.TH);

		switch(context->CLAMP.WMS)
		{
		case CLAMP_REPEAT: 
			m_env.t.min.u16[0] = tw - 1;
			m_env.t.max.u16[0] = 0;
			m_env.t.mask.u32[0] = 0xffffffff; 
			break;
		case CLAMP_CLAMP: 
			m_env.t.min.u16[0] = 0;
			m_env.t.max.u16[0] = tw - 1;
			m_env.t.mask.u32[0] = 0; 
			break;
		case CLAMP_REGION_CLAMP: 
			m_env.t.min.u16[0] = min(context->CLAMP.MINU, tw - 1);
			m_env.t.max.u16[0] = min(context->CLAMP.MAXU, tw - 1);
			m_env.t.mask.u32[0] = 0; 
			break;
		case CLAMP_REGION_REPEAT: 
			m_env.t.min.u16[0] = context->CLAMP.MINU;
			m_env.t.max.u16[0] = context->CLAMP.MAXU;
			m_env.t.mask.u32[0] = 0xffffffff; 
			break;
		default: 
			__assume(0);
		}

		switch(context->CLAMP.WMT)
		{
		case CLAMP_REPEAT: 
			m_env.t.min.u16[4] = th - 1;
			m_env.t.max.u16[4] = 0;
			m_env.t.mask.u32[2] = 0xffffffff; 
			break;
		case CLAMP_CLAMP: 
			m_env.t.min.u16[4] = 0;
			m_env.t.max.u16[4] = th - 1;
			m_env.t.mask.u32[2] = 0; 
			break;
		case CLAMP_REGION_CLAMP: 
			m_env.t.min.u16[4] = min(context->CLAMP.MINV, th - 1);
			m_env.t.max.u16[4] = min(context->CLAMP.MAXV, th - 1); // ffx anima summon scene, when the anchor appears (th = 256, maxv > 256)
			m_env.t.mask.u32[2] = 0; 
			break;
		case CLAMP_REGION_REPEAT: 
			m_env.t.min.u16[4] = context->CLAMP.MINV;
			m_env.t.max.u16[4] = context->CLAMP.MAXV;
			m_env.t.mask.u32[2] = 0xffffffff; 
			break;
		default: 
			__assume(0);
		}

		m_env.t.min = m_env.t.min.xxxxlh();
		m_env.t.max = m_env.t.max.xxxxlh();
		m_env.t.mask = m_env.t.mask.xxzz();
	}

	//

	f->sl = m_ds.Lookup(m_env.sel);

	//

	if(m_env.sel.IsSolidRect())
	{
		f->sr = (DrawSolidRectPtr)&GSDrawScanline::DrawSolidRect;
	}

	//

	DWORD sel = 0;

	if(data->primclass != GS_POINT_CLASS)
	{
		sel |= (m_env.sel.ztst ? 1 : 0) << 0;
		sel |= m_env.sel.fge << 1;
		sel |= (m_env.sel.tfx != TFX_NONE ? 1 : 0) << 2;
		sel |= m_env.sel.fst << 3;
		sel |= m_env.sel.iip << 4;
	}

	f->sp = m_sp.Lookup(sel);
}

void GSDrawScanline::EndDraw(const GSRasterizerStats& stats)
{
	m_ds.UpdateStats(stats, m_state->m_perfmon.GetFrame());
}

template<DWORD zbe, DWORD fge, DWORD tme, DWORD fst, DWORD iip>
void GSDrawScanline::SetupPrim(const GSVertexSW* vertices, const GSVertexSW& dscan)
{
	// p

	GSVector4 p = dscan.p;
	
	GSVector4 dz = p.zzzz();
	GSVector4 df = p.wwww();

	if(zbe)
	{
		m_env.d4.z = dz * 4.0f;
	}

	if(fge)
	{
		m_env.d4.f = GSVector4i(df * 4.0f).xxzzlh();
	}

	for(int i = 0; i < 4; i++)
	{
		GSVector4 v = m_shift[i];

		if(zbe)
		{
			m_env.d[i].z = dz * v;
		}

		if(fge)
		{
			m_env.d[i].f = GSVector4i(df * v).xxzzlh();
		}
	}

	if(iip == 0) // should be sprite == 1, but close enough
	{
		GSVector4 p = vertices[0].p;

		if(zbe)
		{
			GSVector4 z = p.zzzz();

			m_env.p.z = (GSVector4i(z * 0.5f) << 1) | (GSVector4i(z) & GSVector4i::x00000001());
		}

		if(fge)
		{
			m_env.p.f = GSVector4i(p).zzzzh().zzzz();
		}
	}

	// t

	if(tme)
	{
		GSVector4 t = dscan.t;

		if(fst)
		{
			m_env.d4.st = GSVector4i(t * 4.0f);

			GSVector4 ds = t.xxxx();
			GSVector4 dt = t.yyyy();

			for(int i = 0; i < 4; i++)
			{
				GSVector4 v = m_shift[i];

				m_env.d[i].si = GSVector4i(ds * v);
				m_env.d[i].ti = GSVector4i(dt * v);
			}
		}
		else
		{
			m_env.d4.stq = t * 4.0f;

			GSVector4 ds = t.xxxx();
			GSVector4 dt = t.yyyy();
			GSVector4 dq = t.zzzz();

			for(int i = 0; i < 4; i++)
			{
				GSVector4 v = m_shift[i];

				m_env.d[i].s = ds * v;
				m_env.d[i].t = dt * v;
				m_env.d[i].q = dq * v;
			}
		}
	}

	// c

	if(iip)
	{
		GSVector4 c = dscan.c;

		m_env.d4.c = GSVector4i(c * 4.0f).xzyw().ps32();

		GSVector4 dr = c.xxxx();
		GSVector4 dg = c.yyyy();
		GSVector4 db = c.zzzz();
		GSVector4 da = c.wwww();

		for(int i = 0; i < 4; i++)
		{
			GSVector4 v = m_shift[i];

			GSVector4i rg = GSVector4i(dr * v).ps32(GSVector4i(dg * v));
			GSVector4i ba = GSVector4i(db * v).ps32(GSVector4i(da * v));

			m_env.d[i].rb = rg.upl16(ba);
			m_env.d[i].ga = rg.uph16(ba);
		}
	}
	else
	{
		GSVector4i rgba = GSVector4i(vertices[0].c);
		
		GSVector4i rbga = rgba.upl16(rgba.zwxy());

		if(tme == 0)
		{
			rbga = rbga.srl16(7);

			DWORD abe = m_env.sel.abe & 0x3f; // a, b, c

			DWORD abea = m_env.sel.abea;
			DWORD abeb = m_env.sel.abeb;
			DWORD abec = m_env.sel.abec;
			DWORD abed = m_env.sel.abed;

			if(fge == 0 && abe != 0x3f && !(abe & 0x15) && abea != abeb) // 0x15 = 010101b => a, b, c != 1
			{
				GSVector4i c[4];

				c[0] = rbga;
				c[1] = rgba.zzzzh().zzzz();
				c[2] = GSVector4i::zero();
				c[3] = m_env.afix2;

				GSVector4i cc = GSVector4i::lerp16<1>(c[abea], c[abeb], c[abec + 1]);

				if(abed == 0)
				{
					cc = cc.add16(c[0]);
				}

				m_env.c2.rb = cc.xxxx();
				m_env.c2.ga = cc.zzzz().mix16(c[1].srl16(7));
			}
		}

		m_env.c.rb = rbga.xxxx();
		m_env.c.ga = rbga.zzzz();
	}
}

GSVector4i GSDrawScanline::Wrap(const GSVector4i& t)
{
	GSVector4i clamp = t.sat_i16(m_env.t.min, m_env.t.max);
	GSVector4i repeat = (t & m_env.t.min) | m_env.t.max;

	return clamp.blend8(repeat, m_env.t.mask);
}

void GSDrawScanline::SampleTexture(DWORD ltf, DWORD tlu, const GSVector4i& u, const GSVector4i& v, GSVector4i* c)
{
	const void* RESTRICT tex = m_env.tex;
	const DWORD* RESTRICT clut = m_env.clut;
	const DWORD tw = m_env.tw;

	GSVector4i uv = u.sra32(16).ps32(v.sra32(16));

	GSVector4i c00, c01, c10, c11;

	if(ltf)
	{
		GSVector4i uf = u.xxzzlh().srl16(1);
		GSVector4i vf = v.xxzzlh().srl16(1);

		GSVector4i uv0 = Wrap(uv);
		GSVector4i uv1 = Wrap(uv.add16(GSVector4i::x0001()));

		GSVector4i y0 = uv0.uph16() << tw;
		GSVector4i y1 = uv1.uph16() << tw;
		GSVector4i x0 = uv0.upl16();
		GSVector4i x1 = uv1.upl16();

		GSVector4i addr00 = y0 + x0;
		GSVector4i addr01 = y0 + x1;
		GSVector4i addr10 = y1 + x0;
		GSVector4i addr11 = y1 + x1;

		if(tlu)
		{
			c00 = addr00.gather32_32((const BYTE*)tex, clut);
			c01 = addr01.gather32_32((const BYTE*)tex, clut);
			c10 = addr10.gather32_32((const BYTE*)tex, clut);
			c11 = addr11.gather32_32((const BYTE*)tex, clut);
		}
		else
		{
			c00 = addr00.gather32_32((const DWORD*)tex);
			c01 = addr01.gather32_32((const DWORD*)tex);
			c10 = addr10.gather32_32((const DWORD*)tex);
			c11 = addr11.gather32_32((const DWORD*)tex);
		}

		GSVector4i mask = GSVector4i::x00ff();

		GSVector4i rb00 = c00 & mask;
		GSVector4i rb01 = c01 & mask;
		GSVector4i rb10 = c10 & mask;
		GSVector4i rb11 = c11 & mask;

		rb00 = rb00.lerp16<0>(rb01, uf);
		rb10 = rb10.lerp16<0>(rb11, uf);
		rb00 = rb00.lerp16<0>(rb10, vf);

		c[0] = rb00;

		GSVector4i ga00 = (c00 >> 8) & mask;
		GSVector4i ga01 = (c01 >> 8) & mask;
		GSVector4i ga10 = (c10 >> 8) & mask;
		GSVector4i ga11 = (c11 >> 8) & mask;

		ga00 = ga00.lerp16<0>(ga01, uf);
		ga10 = ga10.lerp16<0>(ga11, uf);
		ga00 = ga00.lerp16<0>(ga10, vf);

		c[1] = ga00;
	}
	else
	{
		GSVector4i uv0 = Wrap(uv);

		GSVector4i addr00 = (uv0.uph16() << tw) + uv0.upl16();

		if(tlu)
		{
			c00 = addr00.gather32_32((const BYTE*)tex, clut);
		}
		else
		{
			c00 = addr00.gather32_32((const DWORD*)tex);
		}

		GSVector4i mask = GSVector4i::x00ff();

		c[0] = c00 & mask;
		c[1] = (c00 >> 8) & mask;
	}
}

void GSDrawScanline::ColorTFX(DWORD iip, DWORD tfx, const GSVector4i& rbf, const GSVector4i& gaf, GSVector4i& rbt, GSVector4i& gat)
{
	GSVector4i rb = iip ? rbf : m_env.c.rb;
	GSVector4i ga = iip ? gaf : m_env.c.ga;

	GSVector4i af;

	switch(tfx)
	{
	case TFX_MODULATE:
		rbt = rbt.modulate16<1>(rb).clamp8();
		break;
	case TFX_DECAL:
		break;
	case TFX_HIGHLIGHT:
	case TFX_HIGHLIGHT2:
		af = ga.yywwlh().srl16(7);
		rbt = rbt.modulate16<1>(rb).add16(af).clamp8();
		gat = gat.modulate16<1>(ga).add16(af).clamp8().mix16(gat);
		break;
	case TFX_NONE:
		rbt = iip ? rb.srl16(7) : rb;
		break;
	default:
		__assume(0);
	}
}

void GSDrawScanline::AlphaTFX(DWORD iip, DWORD tfx, DWORD tcc, const GSVector4i& gaf, GSVector4i& gat)
{
	GSVector4i ga = iip ? gaf : m_env.c.ga;

	switch(tfx)
	{
	case TFX_MODULATE:
		gat = gat.modulate16<1>(ga).clamp8(); // mul16hrs rounds and breaks fogging in resident evil 4 (only modulate16<0> uses mul16hrs, but watch out)
		if(!tcc) gat = gat.mix16(ga.srl16(7));
		break;
	case TFX_DECAL: 
		break;
	case TFX_HIGHLIGHT: 
		gat = gat.mix16(!tcc ? ga.srl16(7) : gat.addus8(ga.srl16(7)));
		break;
	case TFX_HIGHLIGHT2:
		if(!tcc) gat = gat.mix16(ga.srl16(7));
		break;
	case TFX_NONE: 
		gat = iip ? ga.srl16(7) : ga;
		break; 
	default: 
		__assume(0);
	}
}

void GSDrawScanline::Fog(DWORD fge, const GSVector4i& f, GSVector4i& rb, GSVector4i& ga)
{
	if(fge)
	{
		rb = m_env.frb.lerp16<0>(rb, f);
		ga = m_env.fga.lerp16<0>(ga, f).mix16(ga);
	}
}

bool GSDrawScanline::TestZ(DWORD zpsm, DWORD ztst, const GSVector4i& zs, const GSVector4i& zd, GSVector4i& test)
{
	if(ztst > 1)
	{
		GSVector4i o = GSVector4i::x80000000(zs);

		GSVector4i zso = zs - o;
		GSVector4i zdo;

		switch(zpsm)
		{
		case 0: zdo = zd - o; break;
		case 1: zdo = (zd & GSVector4i::x00ffffff(zs)) - o; break;
		case 2: zdo = (zd & GSVector4i::x0000ffff(zs)) - o; break;
		}

		switch(ztst)
		{
		case ZTST_GEQUAL: test |= zso < zdo; break;
		case ZTST_GREATER: test |= zso <= zdo; break;
		default: __assume(0);
		}

		if(test.alltrue())
		{
			return false;
		}
	}

	return true;
}

bool GSDrawScanline::TestAlpha(DWORD atst, DWORD afail, const GSVector4i& ga, GSVector4i& fm, GSVector4i& zm, GSVector4i& test)
{
	if(atst != ATST_ALWAYS)
	{
		GSVector4i t;

		switch(atst)
		{
		case ATST_NEVER: t = GSVector4i::xffffffff(); break;
		case ATST_ALWAYS: t = GSVector4i::zero(); break;
		case ATST_LESS: 
		case ATST_LEQUAL: t = (ga >> 16) > m_env.aref; break;
		case ATST_EQUAL: t = (ga >> 16) != m_env.aref; break;
		case ATST_GEQUAL: 
		case ATST_GREATER: t = (ga >> 16) < m_env.aref; break;
		case ATST_NOTEQUAL: t = (ga >> 16) == m_env.aref; break;
		default: __assume(0);
		}

		switch(afail)
		{
		case AFAIL_KEEP:
			test |= t;
			if(test.alltrue()) return false;
			break;
		case AFAIL_FB_ONLY:
			zm |= t;
			break;
		case AFAIL_ZB_ONLY:
			fm |= t;
			break;
		case AFAIL_RGB_ONLY: 
			fm |= t & GSVector4i::xff000000();
			zm |= t;
			break;
		default: 
			__assume(0);
		}
	}

	return true;
}

bool GSDrawScanline::TestDestAlpha(DWORD fpsm, DWORD date, const GSVector4i& fd, GSVector4i& test)
{
	if(date)
	{
		switch(fpsm)
		{
		case 0:
			test |= (fd ^ m_env.datm).sra32(31);
			if(test.alltrue()) return false;
		case 1:
			break;
		case 2:
			test |= ((fd << 16) ^ m_env.datm).sra32(31);
			if(test.alltrue()) return false;
		case 3:
			break;
		default:
			__assume(0);
		}
	}

	return true;
}

void GSDrawScanline::ReadPixel(int psm, int addr, GSVector4i& c) const
{
	WORD* vm16 = (WORD*)m_env.vm;

	if(psm != 3)
	{
		c = GSVector4i::load(&vm16[addr], &vm16[addr + 8]);
	}
}

void GSDrawScanline::WritePixel(int psm, WORD* RESTRICT vm16, DWORD c) 
{
	DWORD* RESTRICT vm32 = (DWORD*)vm16;

	switch(psm)
	{
	case 0: *vm32 = c; break;
	case 1: *vm32 = (*vm32 & 0xff000000) | (c & 0x00ffffff); break;
	case 2: *vm16 = (WORD)c; break;
	}
}

void GSDrawScanline::WriteFrame(int fpsm, int rfb, GSVector4i* c, const GSVector4i& fd, const GSVector4i& fm, int addr, int fzm)
{
	WORD* RESTRICT vm16 = (WORD*)m_env.vm;

	c[0] &= m_env.colclamp;
	c[1] &= m_env.colclamp;

	GSVector4i fs = c[0].upl16(c[1]).pu16(c[0].uph16(c[1]));

	if(fpsm != 1)
	{
		fs |= m_env.fba;
	}

	if(fpsm == 2)
	{
		GSVector4i rb = fs & 0x00f800f8;
		GSVector4i ga = fs & 0x8000f800;

		fs = (ga >> 16) | (rb >> 9) | (ga >> 6) | (rb >> 3);
	}

	if(rfb)
	{
		fs = fs.blend(fd, fm);

		if(fpsm < 2)
		{
			if(fzm & 0x03) GSVector4i::storel(&vm16[addr + 0], fs); 
			if(fzm & 0x0c) GSVector4i::storeh(&vm16[addr + 8], fs); 

			return;
		}
	}

	if(fzm & 0x01) WritePixel(fpsm, &vm16[addr + 0], fs.extract32<0>()); 
	if(fzm & 0x02) WritePixel(fpsm, &vm16[addr + 2], fs.extract32<1>());
	if(fzm & 0x04) WritePixel(fpsm, &vm16[addr + 8], fs.extract32<2>()); 
	if(fzm & 0x08) WritePixel(fpsm, &vm16[addr + 10], fs.extract32<3>());
}

void GSDrawScanline::WriteZBuf(int zpsm, int ztst, const GSVector4i& z, const GSVector4i& zd, const GSVector4i& zm, int addr, int fzm)
{
	if(ztst == 0) return;

	WORD* RESTRICT vm16 = (WORD*)m_env.vm;

	GSVector4i zs = z;

	if(ztst > 1)
	{
		if(zpsm < 2)
		{
			zs = zs.blend8(zd, zm);

			if(fzm & 0x30) GSVector4i::storel(&vm16[addr + 0], zs); 
			if(fzm & 0xc0) GSVector4i::storeh(&vm16[addr + 8], zs); 

			return;
		}
	}

	if(fzm & 0x10) WritePixel(zpsm, &vm16[addr + 0], zs.extract32<0>()); 
	if(fzm & 0x20) WritePixel(zpsm, &vm16[addr + 2], zs.extract32<1>());
	if(fzm & 0x40) WritePixel(zpsm, &vm16[addr + 8], zs.extract32<2>()); 
	if(fzm & 0x80) WritePixel(zpsm, &vm16[addr + 10], zs.extract32<3>());
}

template<DWORD fpsm, DWORD zpsm, DWORD ztst, DWORD iip>
void GSDrawScanline::DrawScanline(int top, int left, int right, const GSVertexSW& v)
{
	int skip = left & 3;

	left -= skip;

	int steps = right - left - 4;

	GSVector4i test = m_test[skip] | m_test[7 + (steps & (steps >> 31))];

	//

	GSVector2i fza_base;
	GSVector2i* fza_offset;

	GSVector4 z, s, t, q;
	GSVector4i si, ti, f, rb, ga;

	// fza

	fza_base = m_env.fzbr[top];
	fza_offset = &m_env.fzbc[left >> 2];

	// v.p

	GSVector4 vp = v.p;

	z = vp.zzzz() + m_env.d[skip].z;
	f = GSVector4i(vp).zzzzh().zzzz().add16(m_env.d[skip].f);

	// v.t

	GSVector4 vt = v.t;

	if(m_env.sel.fst)
	{
		GSVector4i vti(vt);

		si = vti.xxxx() + m_env.d[skip].si;
		ti = vti.yyyy() + m_env.d[skip].ti;
	}
	else
	{
		s = vt.xxxx() + m_env.d[skip].s; 
		t = vt.yyyy() + m_env.d[skip].t;
		q = vt.zzzz() + m_env.d[skip].q;
	}

	// v.c

	if(iip)
	{
		GSVector4i vc = GSVector4i(v.c);

		vc = vc.upl16(vc.zwxy());

		rb = vc.xxxx().add16(m_env.d[skip].rb);
		ga = vc.zzzz().add16(m_env.d[skip].ga);
	}

	//

	while(1)
	{
		do
		{
			int fa = fza_base.x + fza_offset->x;
			int za = fza_base.y + fza_offset->y;
			
			GSVector4i zs = (GSVector4i(z * 0.5f) << 1) | (GSVector4i(z) & GSVector4i::x00000001());
			GSVector4i zd;

			if(ztst > 1)
			{
				ReadPixel(zpsm, za, zd);

				if(!TestZ(zpsm, ztst, zs, zd, test))
				{
					continue;
				}
			}

			GSVector4i c[6];

			if(m_env.sel.tfx != TFX_NONE)
			{
				GSVector4i u, v;

				if(m_env.sel.fst)
				{
					u = si;
					v = ti;
				}
				else
				{
					GSVector4 w = q.rcp();

					u = GSVector4i(s * w);
					v = GSVector4i(t * w);

					if(m_env.sel.ltf)
					{
						u -= 0x8000;
						v -= 0x8000;
					}
				}

				SampleTexture(m_env.sel.ltf, m_env.sel.tlu, u, v, c);
			}

			AlphaTFX(iip, m_env.sel.tfx, m_env.sel.tcc, ga, c[1]);

			GSVector4i fm = m_env.fm;
			GSVector4i zm = m_env.zm;

			if(!TestAlpha(m_env.sel.atst, m_env.sel.afail, c[1], fm, zm, test))
			{
				continue;
			}

			ColorTFX(iip, m_env.sel.tfx, rb, ga, c[0], c[1]);

			Fog(m_env.sel.fge, f, c[0], c[1]);

			GSVector4i fd;

			if(m_env.sel.rfb)
			{
				ReadPixel(fpsm, fa, fd);

				if(!TestDestAlpha(fpsm, m_env.sel.date, fd, test))
				{
					continue;
				}
			}

			fm |= test;
			zm |= test;

			int fzm = ~(fm == GSVector4i::xffffffff()).ps32(zm == GSVector4i::xffffffff()).ps32().mask();

			WriteZBuf(zpsm, ztst, zs, zd, zm, za, fzm);

			if(m_env.sel.abe != 255)
			{
				GSVector4i mask = GSVector4i::x00ff(fd);

				switch(fpsm)
				{
				case 0:
					c[2] = fd & mask;
					c[3] = (fd >> 8) & mask;
					break;
				case 1:
					c[2] = fd & mask;
					c[3] = (fd >> 8) & mask;
					c[3] = c[3].mix16(GSVector4i(0x00800000));
					break;
				case 2:
					c[2] = ((fd & 0x7c00) << 9) | ((fd & 0x001f) << 3);
					c[3] = ((fd & 0x8000) << 8) | ((fd & 0x03e0) >> 2);
					break;
				}

				c[4] = GSVector4::zero();
				c[5] = m_env.afix;

				DWORD abea = m_env.sel.abea;
				DWORD abeb = m_env.sel.abeb;
				DWORD abec = m_env.sel.abec;
				DWORD abed = m_env.sel.abed;

				GSVector4i a = c[abec * 2 + 1].yywwlh().sll16(7);

				GSVector4i rb = GSVector4i::lerp16<1>(c[abea * 2 + 0], c[abeb * 2 + 0], a, c[abed * 2 + 0]);
				GSVector4i ga = GSVector4i::lerp16<1>(c[abea * 2 + 1], c[abeb * 2 + 1], a, c[abed * 2 + 1]);

				if(m_env.sel.pabe)
				{
					mask = (c[1] << 8).sra32(31);

					rb = c[0].blend8(rb, mask);
					ga = c[1].blend8(ga, mask);
				}

				c[0] = rb;
				c[1] = ga.mix16(c[1]);
			}

			WriteFrame(fpsm, m_env.sel.rfb, c, fd, fm, fa, fzm);
		}
		while(0);

		if(steps <= 0) break;

		steps -= 4;

		test = m_test[7 + (steps & (steps >> 31))];

		fza_offset++;

		z += m_env.d4.z;
		f = f.add16(m_env.d4.f);

		if(m_env.sel.fst)
		{
			GSVector4i st = m_env.d4.st;

			si += st.xxxx();
			ti += st.yyyy();
		}
		else
		{
			GSVector4 stq = m_env.d4.stq;

			s += stq.xxxx();
			t += stq.yyyy();
			q += stq.zzzz();
		}

		if(iip)
		{
			GSVector4i c = m_env.d4.c;

			rb = rb.add16(c.xxxx());
			ga = ga.add16(c.yyyy());
		}
	}
}

template<DWORD sel>
void GSDrawScanline::DrawScanlineEx(int top, int left, int right, const GSVertexSW& v)
{
	const DWORD fpsm = (sel >> 0) & 3;
	const DWORD zpsm = (sel >> 2) & 3;
	const DWORD ztst = (sel >> 4) & 3;
	const DWORD atst = (sel >> 6) & 7;
	const DWORD afail = (sel >> 9) & 3;
	const DWORD iip = (sel >> 11) & 1;
	const DWORD tfx = (sel >> 12) & 7;
	const DWORD tcc = (sel >> 15) & 1;
	const DWORD fst = (sel >> 16) & 1;
	const DWORD ltf = (sel >> 17) & 1;
	const DWORD tlu = (sel >> 18) & 1;
	const DWORD fge = (sel >> 19) & 1;
	const DWORD date = (sel >> 20) & 1;
	const DWORD abe = (sel >> 21) & 255;
	const DWORD abea = (sel >> 21) & 3;
	const DWORD abeb = (sel >> 23) & 3;
	const DWORD abec = (sel >> 25) & 3;
	const DWORD abed = (sel >> 27) & 3;
	const DWORD pabe = (sel >> 29) & 1;
	const DWORD rfb = (sel >> 30) & 1;
	const DWORD sprite = (sel >> 31) & 1;

	//

	int skip = left & 3;

	left -= skip;

	int steps = right - left - 4;

	GSVector4i test = m_test[skip] | m_test[7 + (steps & (steps >> 31))];

	//

	GSVector2i fza_base;
	GSVector2i* fza_offset;

	GSVector4 z, s, t, q;
	GSVector4i zi, si, ti, f, rb, ga;

	// fza

	fza_base = m_env.fzbr[top];
	fza_offset = &m_env.fzbc[left >> 2];

	// v.p

	GSVector4 vp = v.p;

	if(sprite)
	{
		zi = m_env.p.z;
		f = m_env.p.f;
	}
	else
	{
		z = vp.zzzz() + m_env.d[skip].z;
		f = GSVector4i(vp).zzzzh().zzzz().add16(m_env.d[skip].f);
	}

	// v.t

	GSVector4 vt = v.t;

	if(fst)
	{
		GSVector4i vti(vt);

		si = vti.xxxx();
		ti = vti.yyyy();

		si += m_env.d[skip].si;
		if(!sprite) ti += m_env.d[skip].ti;
	}
	else
	{
		s = vt.xxxx() + m_env.d[skip].s; 
		t = vt.yyyy() + m_env.d[skip].t;
		q = vt.zzzz() + m_env.d[skip].q;
	}

	// v.c

	if(iip)
	{
		GSVector4i vc = GSVector4i(v.c);

		vc = vc.upl16(vc.zwxy());

		rb = vc.xxxx().add16(m_env.d[skip].rb);
		ga = vc.zzzz().add16(m_env.d[skip].ga);
	}

	//

	while(1)
	{
		do
		{
			int fa = fza_base.x + fza_offset->x;
			int za = fza_base.y + fza_offset->y;

			GSVector4i zs = sprite ? zi : (GSVector4i(z * 0.5f) << 1) | (GSVector4i(z) & GSVector4i::x00000001());
			GSVector4i zd;

			if(ztst > 1)
			{
				ReadPixel(zpsm, za, zd);

				if(!TestZ(zpsm, ztst, zs, zd, test))
				{
					continue;
				}
			}

			GSVector4i c[6];

			if(tfx != TFX_NONE)
			{
				GSVector4i u, v;

				if(fst)
				{
					u = si;
					v = ti;
				}
				else
				{
					GSVector4 w = q.rcp();

					u = GSVector4i(s * w);
					v = GSVector4i(t * w);

					if(ltf)
					{
						u -= 0x8000;
						v -= 0x8000;
					}
				}

				SampleTexture(ltf, tlu, u, v, c);
			}

			AlphaTFX(iip, tfx, tcc, ga, c[1]);

			GSVector4i fm = m_env.fm;
			GSVector4i zm = m_env.zm;

			if(!TestAlpha(atst, afail, c[1], fm, zm, test))
			{
				continue;
			}

			ColorTFX(iip, tfx, rb, ga, c[0], c[1]);

			Fog(fge, f, c[0], c[1]);

			GSVector4i fd;

			if(rfb)
			{
				ReadPixel(fpsm, fa, fd);

				if(!TestDestAlpha(fpsm, date, fd, test))
				{
					continue;
				}
			}

			fm |= test;
			zm |= test;

			int fzm = ~(fm == GSVector4i::xffffffff()).ps32(zm == GSVector4i::xffffffff()).ps32().mask();

			WriteZBuf(zpsm, ztst, zs, zd, zm, za, fzm);

			if(abe != 255)
			{
				GSVector4i mask = GSVector4i::x00ff(fd);

				switch(fpsm)
				{
				case 0:
				case 1:
					c[2] = fd & mask;
					c[3] = (fd >> 8) & mask;
					break;
				case 2:
					c[2] = ((fd & 0x7c00) << 9) | ((fd & 0x001f) << 3);
					c[3] = ((fd & 0x8000) << 8) | ((fd & 0x03e0) >> 2);
					break;
				}

				c[4] = GSVector4::zero();
				c[5] = GSVector4::zero();

				GSVector4i rb, ga;

				if(tfx == TFX_NONE && fge == 0 && abea != 1 && abeb != 1 && abec != 1 && abea != abeb)
				{
					c[0] = m_env.c2.rb;
					c[1] = m_env.c2.ga;

					rb = c[0];
					ga = c[1];

					if(abed == 1)
					{
						rb = rb.add16(c[2]);
						ga = ga.add16(c[3]);
					}
				}
				else
				{
					if(abea != abeb)
					{
						rb = c[abea * 2 + 0];
						ga = c[abea * 2 + 1];

						if(abeb < 2)
						{
							rb = rb.sub16(c[abeb * 2 + 0]);
							ga = ga.sub16(c[abeb * 2 + 1]);
						}

						if(!(fpsm == 1 && abec == 1))
						{
							GSVector4i a = abec < 2 ? c[abec * 2 + 1].yywwlh().sll16(7) : m_env.afix2;

							rb = rb.modulate16<1>(a);
							ga = ga.modulate16<1>(a);
						}

						if(abed < 2)
						{
							rb = rb.add16(c[abed * 2 + 0]);
							ga = ga.add16(c[abed * 2 + 1]);
						}
					}
					else
					{
						rb = c[abed * 2 + 0];
						ga = c[abed * 2 + 1];
					}
				}

				if(pabe)
				{
					mask = (c[1] << 8).sra32(31);

					rb = c[0].blend8(rb, mask);
					ga = c[1].blend8(ga, mask);
				}

				c[0] = rb;
				c[1] = ga.mix16(c[1]);
			}

			WriteFrame(fpsm, rfb, c, fd, fm, fa, fzm);
		}
		while(0);

		if(steps <= 0) break;

		steps -= 4;

		test = m_test[7 + (steps & (steps >> 31))];

		fza_offset++;

		if(!sprite)
		{
			z += m_env.d4.z;
			f = f.add16(m_env.d4.f);
		}

		if(fst)
		{
			GSVector4i st = m_env.d4.st;

			si += st.xxxx();
			if(!sprite) ti += st.yyyy();
		}
		else
		{
			GSVector4 stq = m_env.d4.stq;

			s += stq.xxxx();
			t += stq.yyyy();
			q += stq.zzzz();
		}

		if(iip)
		{
			GSVector4i c = m_env.d4.c;

			rb = rb.add16(c.xxxx());
			ga = ga.add16(c.yyyy());
		}
	}
}

void GSDrawScanline::DrawSolidRect(const GSVector4i& r, const GSVertexSW& v)
{
/*
static FILE* s_fp = NULL;
if(!s_fp) s_fp = fopen("c:\\log2.txt", "w");
__int64 start = __rdtsc();
int size = (r.z - r.x) * (r.w - r.y);
*/
	ASSERT(r.y >= 0);
	ASSERT(r.w >= 0);

	// FIXME: sometimes the frame and z buffer may overlap, the outcome is undefined

	DWORD m;

	m = m_env.zm.u32[0];

	if(m != 0xffffffff)
	{
		DWORD z = (DWORD)(float)v.p.z;

		if(m_env.sel.zpsm != 2)
		{
			if(m == 0)
			{
				DrawSolidRectT<DWORD, false>(m_env.zbr, m_env.zbc[0], r, z, m);
			}
			else
			{
				DrawSolidRectT<DWORD, true>(m_env.zbr, m_env.zbc[0], r, z, m);
			}
		}
		else
		{
			if(m == 0)
			{
				DrawSolidRectT<WORD, false>(m_env.zbr, m_env.zbc[0], r, z, m);
			}
			else
			{
				DrawSolidRectT<WORD, true>(m_env.zbr, m_env.zbc[0], r, z, m);
			}
		}
	}

	m = m_env.fm.u32[0];

	if(m != 0xffffffff)
	{
		DWORD c = (GSVector4i(v.c) >> 7).rgba32();

		if(m_state->m_context->FBA.FBA)
		{
			c |= 0x80000000;
		}
		
		if(m_env.sel.fpsm != 2)
		{
			if(m == 0)
			{
				DrawSolidRectT<DWORD, false>(m_env.fbr, m_env.fbc[0], r, c, m);
			}
			else
			{
				DrawSolidRectT<DWORD, true>(m_env.fbr, m_env.fbc[0], r, c, m);
			}
		}
		else
		{
			c = ((c & 0xf8) >> 3) | ((c & 0xf800) >> 6) | ((c & 0xf80000) >> 9) | ((c & 0x80000000) >> 16);

			if(m == 0)
			{
				DrawSolidRectT<WORD, false>(m_env.fbr, m_env.fbc[0], r, c, m);
			}
			else
			{
				DrawSolidRectT<WORD, true>(m_env.fbr, m_env.fbc[0], r, c, m);
			}
		}
	}
/*
__int64 stop = __rdtsc();
fprintf(s_fp, "%I64d => %I64d = %I64d (%d,%d - %d,%d) %d\n", start, stop, stop - start, r.x, r.y, r.z, r.w, size);
*/
}

template<class T, bool masked> 
void GSDrawScanline::DrawSolidRectT(const GSVector4i* row, int* col, const GSVector4i& r, DWORD c, DWORD m)
{
	if(m == 0xffffffff) return;

	GSVector4i color((int)c);
	GSVector4i mask((int)m);

	if(sizeof(T) == sizeof(WORD))
	{
		color = color.xxzzlh();
		mask = mask.xxzzlh();
	}

	color = color.andnot(mask);

	GSVector4i bm(8 * 4 / sizeof(T) - 1, 8 - 1);
	GSVector4i br = (r + bm).andnot(bm.xyxy());

	FillRect<T, masked>(row, col, GSVector4i(r.x, r.y, r.z, br.y), c, m);
	FillRect<T, masked>(row, col, GSVector4i(r.x, br.w, r.z, r.w), c, m);

	if(r.x < br.x || br.z < r.z)
	{
		FillRect<T, masked>(row, col, GSVector4i(r.x, br.y, br.x, br.w), c, m);
		FillRect<T, masked>(row, col, GSVector4i(br.z, br.y, r.z, br.w), c, m);
	}

	FillBlock<T, masked>(row, col, br, color, mask);
}

template<class T, bool masked> 
void GSDrawScanline::FillRect(const GSVector4i* row, int* col, const GSVector4i& r, DWORD c, DWORD m)
{
	if(r.x >= r.z) return;

	for(int y = r.y; y < r.w; y++)
	{
		DWORD base = row[y].x;

		for(int x = r.x; x < r.z; x++)
		{
			T* p = &((T*)m_env.vm)[base + col[x]];

			*p = (T)(!masked ? c : (c | (*p & m)));
		}
	}
}

template<class T, bool masked> 
void GSDrawScanline::FillBlock(const GSVector4i* row, int* col, const GSVector4i& r, const GSVector4i& c, const GSVector4i& m)
{
	if(r.x >= r.z) return;

	for(int y = r.y; y < r.w; y += 8)
	{
		DWORD base = row[y].x;

		for(int x = r.x; x < r.z; x += 8 * 4 / sizeof(T))
		{
			GSVector4i* p = (GSVector4i*)&((T*)m_env.vm)[base + col[x]];

			for(int i = 0; i < 16; i += 4)
			{
				p[i + 0] = !masked ? c : (c | (p[i + 0] & m));
				p[i + 1] = !masked ? c : (c | (p[i + 1] & m));
				p[i + 2] = !masked ? c : (c | (p[i + 2] & m));
				p[i + 3] = !masked ? c : (c | (p[i + 3] & m));
			}
		}
	}
}

//

GSDrawScanline::GSDrawScanlineMap::GSDrawScanlineMap()
{
	// w00t :P

	#define InitDS_IIP(fpsm, zpsm, ztst, iip) \
		m_default[fpsm][zpsm][ztst][iip] = (DrawScanlinePtr)&GSDrawScanline::DrawScanline<fpsm, zpsm, ztst, iip>; \

	#define InitDS_ZTST(fpsm, zpsm, ztst) \
		InitDS_IIP(fpsm, zpsm, ztst, 0) \
		InitDS_IIP(fpsm, zpsm, ztst, 1) \

	#define InitDS(fpsm, zpsm) \
		InitDS_ZTST(fpsm, zpsm, 0) \
		InitDS_ZTST(fpsm, zpsm, 1) \
		InitDS_ZTST(fpsm, zpsm, 2) \
		InitDS_ZTST(fpsm, zpsm, 3) \

	InitDS(0, 0);
	InitDS(0, 1);
	InitDS(0, 2);
	InitDS(0, 3);
	InitDS(1, 0);
	InitDS(1, 1);
	InitDS(1, 2);
	InitDS(1, 3);
	InitDS(2, 0);
	InitDS(2, 1);
	InitDS(2, 2);
	InitDS(2, 3);
	InitDS(3, 0);
	InitDS(3, 1);
	InitDS(3, 2);

	#define InitDS_Sel(sel) \
		SetAt(sel, (DrawScanlinePtr)&GSDrawScanline::DrawScanlineEx<##sel##>);

	#ifdef FAST_DRAWSCANLINE

	// bios

	InitDS_Sel(0x1fe04850); //   8.99%
	InitDS_Sel(0x1fe28870); //  26.46%
	InitDS_Sel(0x1fe38050); //  12.95%
	InitDS_Sel(0x1fe38060); //   8.59%
	InitDS_Sel(0x48428050); //   8.86%
	InitDS_Sel(0x48428060); //   6.30%
	InitDS_Sel(0x48804860); //  28.07%
	InitDS_Sel(0x49028060); //   6.31%
	InitDS_Sel(0x4902904c); //   5.46%
	InitDS_Sel(0x4b02804c); //   5.11%
	InitDS_Sel(0x4c804050); //   9.08%
	InitDS_Sel(0x4d038864); // 114.89%
	InitDS_Sel(0x9fe39064); //  14.72%
	InitDS_Sel(0xc8804050); //   8.58%
	InitDS_Sel(0xc9004050); //   8.69%
	InitDS_Sel(0xc9039050); //  17.17%
	InitDS_Sel(0xcc804050); //   8.16%
	InitDS_Sel(0xcd019050); //  85.40%

	// ffx

	InitDS_Sel(0x11020865); //   9.81%
	InitDS_Sel(0x1fe68875); //   9.90%
	InitDS_Sel(0x1fe69075); //  15.13%
	InitDS_Sel(0x1fe84075); //   5.16%
	InitDS_Sel(0x1fee8075); //   5.98%
	InitDS_Sel(0x1fee8875); //  43.94%
	InitDS_Sel(0x1fee8876); //  22.75%
	InitDS_Sel(0x1fee8975); //   5.70%
	InitDS_Sel(0x48404865); //  20.31%
	InitDS_Sel(0x48468865); //   8.18%
	InitDS_Sel(0x48478065); //  17.47%
	InitDS_Sel(0x48820965); //   7.89%
	InitDS_Sel(0x48830875); //   5.84%
	InitDS_Sel(0x48868865); //   5.44%
	InitDS_Sel(0x48868965); //   9.13%
	InitDS_Sel(0x48878165); //  28.27%
	InitDS_Sel(0x488f89f5); //  20.19%
	InitDS_Sel(0x488f89f6); //  27.92%
	InitDS_Sel(0x49068065); //   7.68%
	InitDS_Sel(0x49068865); //  20.15%
	InitDS_Sel(0x49068965); //   8.65%
	InitDS_Sel(0x49078065); //  14.00%
	InitDS_Sel(0x49078165); //   5.63%
	InitDS_Sel(0xc883004d); //  22.42%
	InitDS_Sel(0xc887814d); //   9.20%
	InitDS_Sel(0xc8878165); //  71.18%
	InitDS_Sel(0xc8879065); //  26.05%
	InitDS_Sel(0xc887914d); //   8.40%
	InitDS_Sel(0xc88791e5); //  82.67%
	InitDS_Sel(0xc9078065); //   8.00%
	InitDS_Sel(0xcc819055); //  14.54%
	InitDS_Sel(0xcc839065); //   7.41%
	InitDS_Sel(0xd5204055); //  34.58%
	InitDS_Sel(0x48804855); //  11.77%
	InitDS_Sel(0x48804865); //  12.94%
	InitDS_Sel(0x488e8965); //   5.04%
	InitDS_Sel(0x49004875); //  11.36%
	InitDS_Sel(0x9100404d); //   8.54%
	InitDS_Sel(0x9fe78075); //  18.54%
	InitDS_Sel(0x9fe78155); //  13.08%
	InitDS_Sel(0xcd078075); //   9.47%

	// ffx-2

	InitDS_Sel(0x110a8965); //  17.40%
	InitDS_Sel(0x1fe30069); //  17.38%
	InitDS_Sel(0x1fe5884d); //  11.56%
	InitDS_Sel(0x48468965); //  79.89%
	InitDS_Sel(0x4881884d); //   5.61%
	InitDS_Sel(0x488781f5); //   5.84%
	InitDS_Sel(0x4890404c); // 108.44%
	InitDS_Sel(0x4893084c); //  24.74%
	InitDS_Sel(0x49004859); //  48.72%
	InitDS_Sel(0x49004865); //  13.08%
	InitDS_Sel(0x49004869); //  22.94%
	InitDS_Sel(0x4900494d); // 470.30%
	InitDS_Sel(0x4907814d); //  14.15%
	InitDS_Sel(0x49078865); //  21.56%
	InitDS_Sel(0x49078965); //  11.37%
	InitDS_Sel(0x490e8165); //  15.52%
	InitDS_Sel(0xc8478165); //  10.44%
	InitDS_Sel(0xc8804055); //  36.58%
	InitDS_Sel(0xc881004d); //  13.53%
	InitDS_Sel(0xc8830055); //  23.97%
	InitDS_Sel(0xc885004d); //  14.32%
	InitDS_Sel(0xc893004c); // 136.83%
	InitDS_Sel(0xc895004c); //  16.48%
	InitDS_Sel(0xc9004055); //  13.92%
	InitDS_Sel(0xc9004059); //  15.87%
	InitDS_Sel(0xc9004065); //  18.61%
	InitDS_Sel(0xc9059155); //  13.56%
	InitDS_Sel(0xc907814d); //  15.06%
	InitDS_Sel(0xc9078165); //  12.21%
	InitDS_Sel(0xcc804055); //  16.51%
	InitDS_Sel(0xcc850055); //  17.01%
	InitDS_Sel(0xc88581cd); //   7.32%
	InitDS_Sel(0xc88581e5); //   5.41%

	// ffxii

	InitDS_Sel(0x1fe6804c); //   9.05%
	InitDS_Sel(0x1fe68064); //   5.11%
	InitDS_Sel(0x1fe6884c); //  14.58%
	InitDS_Sel(0x1fee8864); //  88.41%
	InitDS_Sel(0x1fee8964); //  33.72%
	InitDS_Sel(0x48404064); //  30.72%
	InitDS_Sel(0x4847004c); //  17.41%
	InitDS_Sel(0x48828864); //   6.06%
	InitDS_Sel(0x4883004c); //  20.10%
	InitDS_Sel(0x4883084c); //  12.37%
	InitDS_Sel(0x4886804c); //   5.10%
	InitDS_Sel(0x4887084c); // 226.61%
	InitDS_Sel(0x48878064); //   7.39%
	InitDS_Sel(0x488e8b64); //  16.26%
	InitDS_Sel(0x48904064); //  29.47%
	InitDS_Sel(0x49004064); //   9.31%
	InitDS_Sel(0x49078064); //  28.77%
	InitDS_Sel(0x5fe0404c); //  70.30%
	InitDS_Sel(0x9fe3904c); //  10.16%
	InitDS_Sel(0xc887004c); //  18.63%
	InitDS_Sel(0xc8878064); //  19.71%
	InitDS_Sel(0xc887904c); //  13.03%
	InitDS_Sel(0xc9278064); //  39.54%

	// kingdom hearts

	InitDS_Sel(0x4840404c); //  15.00%
	InitDS_Sel(0x48830874); //  14.41%
	InitDS_Sel(0x48868154); //   7.22%
	InitDS_Sel(0x4886884c); //  28.13%
	InitDS_Sel(0x4886904c); //  16.66%
	InitDS_Sel(0x490e8974); //  59.46%
	InitDS_Sel(0xc8818054); //  13.73%
	InitDS_Sel(0xc8858054); //  11.63%
	InitDS_Sel(0xc9004054); //  14.66%

	// kingdom hearts 2

	InitDS_Sel(0x48804060); //  19.75%
	InitDS_Sel(0x488a8964); //  20.35%
	InitDS_Sel(0x9fe39054); //  16.04%
	InitDS_Sel(0xc8810054); //  28.27%
	InitDS_Sel(0xcc83004d); //  53.63%
	InitDS_Sel(0xcd03004d); //  20.55%

	// persona 3

	InitDS_Sel(0x484e8068); //  29.91%
	InitDS_Sel(0x4881804c); //  18.16%
	InitDS_Sel(0x4881904c); //  24.90%
	InitDS_Sel(0x490e8068); //   5.82%
	InitDS_Sel(0x4b07904c); //  59.21%
	InitDS_Sel(0x4d47834c); //  29.42%
	InitDS_Sel(0x4d47934c); //  27.37%
	InitDS_Sel(0xca43004c); //  17.88%
	InitDS_Sel(0xcb07934c); //  11.38%
	InitDS_Sel(0xcd47804c); // 106.54%
	InitDS_Sel(0xcd47834c); // 104.37%

	// persona 4

	InitDS_Sel(0x1fe04058); //  23.41%
	InitDS_Sel(0x4840484c); //  24.66%
	InitDS_Sel(0x4881834c); //  21.87%
	InitDS_Sel(0x4881934c); //  21.68%
	InitDS_Sel(0x48828368); //   9.84%
	InitDS_Sel(0x48868f68); //  29.84%
	InitDS_Sel(0x48879168); //   5.74%
	InitDS_Sel(0x49068868); //  18.43%
	InitDS_Sel(0x49078068); //  63.62%
	InitDS_Sel(0x49079068); //  57.82%
	InitDS_Sel(0x490e8868); //  22.88%
	InitDS_Sel(0x4a47804c); //  68.03%
	InitDS_Sel(0x4a47904c); //  20.99%
	InitDS_Sel(0x4a80404c); // 190.06%
	InitDS_Sel(0x4a83004c); //  25.28%
	InitDS_Sel(0x4a87804c); //  26.31%
	InitDS_Sel(0x4a878068); //  29.36%
	InitDS_Sel(0x4a878868); //  23.13%
	InitDS_Sel(0x4a879068); //  11.16%
	InitDS_Sel(0x4b00404c); // 111.77%
	InitDS_Sel(0x4b07804c); //  23.00%
	InitDS_Sel(0x4b07884c); //  21.15%
	InitDS_Sel(0x5fe04058); //  37.01%
	InitDS_Sel(0x5fe04858); //  87.23%

	// sfex3

	InitDS_Sel(0x1fe04868); //   8.37%
	InitDS_Sel(0x1fe6b068); //  16.21%
	InitDS_Sel(0x1fe6b868); //   6.50%
	InitDS_Sel(0x41268068); //   8.16%
	InitDS_Sel(0x41269068); //   9.51%
	InitDS_Sel(0x4886b068); //  20.50%
	InitDS_Sel(0x4886b868); //  35.05%
	InitDS_Sel(0x49079078); //   6.90%
	InitDS_Sel(0x4c868068); //   5.48%
	InitDS_Sel(0x4c868868); //   6.05%
	InitDS_Sel(0x9fe1004e); //   6.98%
	InitDS_Sel(0x9fe3004e); //  13.33%
	InitDS_Sel(0xc8859058); //  10.74%
	InitDS_Sel(0xcc804058); //   8.15%
	InitDS_Sel(0xcd404058); //   5.04%

	// gt4

	InitDS_Sel(0x1fee8164); //   7.38%
	InitDS_Sel(0x488e8f64); //   7.09%
	InitDS_Sel(0x488e9764); //  31.70%
	InitDS_Sel(0x4b1a8864); //   5.45%
	InitDS_Sel(0x5fe3904e); //   8.54%

	// katamary damacy

	InitDS_Sel(0x488e89e4); //  10.50%
	InitDS_Sel(0x488e91d4); //  18.22%
	InitDS_Sel(0xc88181cc); //   7.69%
	InitDS_Sel(0xc8904054); //  12.39%

	// grandia 3

	InitDS_Sel(0x1fe0404e); //   7.99%
	InitDS_Sel(0x48868360); //   5.62%
	InitDS_Sel(0x48868860); //   8.40%
	InitDS_Sel(0x48869360); //   7.19%
	InitDS_Sel(0x4887884c); //   5.93%
	InitDS_Sel(0x488a8060); //  12.82%
	InitDS_Sel(0x488e8360); //  26.69%
	InitDS_Sel(0x488e8b60); //  32.33%
	InitDS_Sel(0x488e8f60); //  15.38%
	InitDS_Sel(0x488e9060); //   7.52%
	InitDS_Sel(0x50368060); //   9.24%
	InitDS_Sel(0xc8878070); //  37.81%
	InitDS_Sel(0xcc81804c); //  46.79%
	InitDS_Sel(0xcc839060); //  55.67%

	// rumble roses

	InitDS_Sel(0x1fe78064); //  26.70%
	InitDS_Sel(0x1fe79064); //   9.93%
	InitDS_Sel(0x4880484c); //   6.38%
	InitDS_Sel(0x48838164); //  11.01%
	InitDS_Sel(0x4887b864); //  35.30%
	InitDS_Sel(0x488e8164); //   5.23%
	InitDS_Sel(0x4900484c); //   8.86%
	InitDS_Sel(0x49078864); //   7.98%
	InitDS_Sel(0x490e8964); //   8.26%
	InitDS_Sel(0x9fe3004d); //   8.92%
	InitDS_Sel(0xc8838164); //  14.00%
	InitDS_Sel(0xc8878164); //  15.96%
	InitDS_Sel(0xcc830064); //  35.39%

	// dmc

	InitDS_Sel(0x4423904c); //   7.89%
	InitDS_Sel(0x4427904c); //  17.05%
	InitDS_Sel(0x45204078); //   9.08%
	InitDS_Sel(0x4c87914c); //  40.48%
	InitDS_Sel(0x54204078); //   9.09%
	InitDS_Sel(0x9fe39058); //   7.17%
	InitDS_Sel(0x9fe78068); //   9.28%
	InitDS_Sel(0xc427904c); //   8.30%
	InitDS_Sel(0xc520404c); //   8.11%
	InitDS_Sel(0xc8804078); //   6.11%
	InitDS_Sel(0xc8810068); //   7.80%
	InitDS_Sel(0xc8830068); //  10.05%
	InitDS_Sel(0xcc43804c); //  17.32%
	InitDS_Sel(0xd420404c); //   8.03%

	// xenosaga 2

	InitDS_Sel(0x1fee804c); //  15.39%
	InitDS_Sel(0x49079064); //  31.08%
	InitDS_Sel(0x51229064); //   8.86%
	InitDS_Sel(0xc8804074); //  16.71%
	InitDS_Sel(0xc9079054); //  17.35%
	InitDS_Sel(0xcc804054); //  14.57%
	InitDS_Sel(0xcc839054); //  24.04%
	InitDS_Sel(0xcd004054); //  14.54%

	// nfs mw

	InitDS_Sel(0x1fe68068); //  18.10%
	InitDS_Sel(0x1fe6806a); //   6.75%
	InitDS_Sel(0x1fe68164); //   5.78%
	InitDS_Sel(0x1fe68868); // 117.99%
	InitDS_Sel(0x1fe68964); //  48.41%
	InitDS_Sel(0x4883804e); //   7.16%
	InitDS_Sel(0x48868868); //  46.01%
	InitDS_Sel(0x4b004064); //  22.82%
	InitDS_Sel(0x4b004068); //  33.12%
	InitDS_Sel(0x4b004864); //  21.73%
	InitDS_Sel(0x4b004868); //  37.86%
	InitDS_Sel(0x4b028064); //  20.96%
	InitDS_Sel(0x4b028068); //  30.53%
	InitDS_Sel(0x4b028864); //  24.64%
	InitDS_Sel(0x4b028868); //  31.04%
	InitDS_Sel(0x4b038064); //  13.05%
	InitDS_Sel(0xc805904c); //  17.04%
	InitDS_Sel(0xc9078064); //  13.78%
	InitDS_Sel(0xc927904c); //  18.78%
	InitDS_Sel(0xcc83004c); //   5.32%
	InitDS_Sel(0xcc83804c); //  19.33%
	InitDS_Sel(0xcc83804e); //   5.85%
	InitDS_Sel(0xcd03914c); //  12.00%
	InitDS_Sel(0xd127904c); //  18.81%
	InitDS_Sel(0xdfe19064); //  27.63%

	// berserk

	InitDS_Sel(0x48804064); //  41.22%
	InitDS_Sel(0x48878864); //  41.61%
	InitDS_Sel(0x488e8064); //  32.85%
	InitDS_Sel(0x488e8964); //  33.35%
	InitDS_Sel(0x49004874); //  10.91%
	InitDS_Sel(0x4c8e8864); //  11.27%
	InitDS_Sel(0x4c8fb864); //   5.61%
	InitDS_Sel(0xc8804064); //  14.54%
	InitDS_Sel(0xc8830064); //  24.60%
	InitDS_Sel(0xcd03004c); //  31.16%
	InitDS_Sel(0xdfe3004c); //   7.40%
	InitDS_Sel(0xdfe3904c); //  18.00%

	// castlevania

	InitDS_Sel(0x1fe78868); //  19.07%
	InitDS_Sel(0x48878868); //  85.08%
	InitDS_Sel(0x4d00407a); //  17.23%
	InitDS_Sel(0x9fe1004c); //   9.10%
	InitDS_Sel(0x9fe3904e); //  16.38%
	InitDS_Sel(0x9fe5904c); //  11.74%
	InitDS_Sel(0xc8804068); //  13.17%
	InitDS_Sel(0xc881004e); //  17.78%
	InitDS_Sel(0xca80404c); //   8.43%
	InitDS_Sel(0xdfe3804c); //  17.00%

	// okami

	InitDS_Sel(0x48804058); //  10.29%
	InitDS_Sel(0x48878058); //  11.71%
	InitDS_Sel(0x48878168); // 250.79%
	InitDS_Sel(0x488e8068); //  20.22%
	InitDS_Sel(0x488e8868); //  32.58%
	InitDS_Sel(0x488e8968); //  29.40%
	InitDS_Sel(0x49078168); //  13.87%
	InitDS_Sel(0x9fe18058); //  10.94%
	InitDS_Sel(0xc5218058); //  26.94%
	InitDS_Sel(0xc881804c); //  11.98%
	InitDS_Sel(0xc8839158); //  22.61%
	InitDS_Sel(0xc8878158); //   8.98%
	InitDS_Sel(0xc8878168); //   9.05%
	InitDS_Sel(0xc8879168); //   6.53%
	InitDS_Sel(0xc9078168); //  15.66%
	InitDS_Sel(0xca83804c); //  16.89%
	InitDS_Sel(0xcc43904c); //  77.10%
	InitDS_Sel(0xdfe59068); //  64.26%

	// bully

	InitDS_Sel(0x110e8864); //  65.16%
	InitDS_Sel(0x110e8964); //  50.07%
	InitDS_Sel(0x4d068064); //  23.14%
	InitDS_Sel(0x4d068364); //  21.42%
	InitDS_Sel(0x4d068864); //  22.79%
	InitDS_Sel(0x9fe04077); //   8.61%
	InitDS_Sel(0xc901004c); //  22.07%
	InitDS_Sel(0xca83904c); //  51.08%
	InitDS_Sel(0xd480404d); //  15.61%
	InitDS_Sel(0xd501904e); //  37.30%

	// culdcept

	InitDS_Sel(0x1fe04866); //  15.93%
	InitDS_Sel(0x1fe2a9e6); //  55.93%
	InitDS_Sel(0x9fe391e6); //  31.07%
	InitDS_Sel(0x9fe3a1e6); //   9.26%
	InitDS_Sel(0x9fe591e6); //   9.73%
	InitDS_Sel(0xc88181e6); //   9.89%
	InitDS_Sel(0x1fe2a1e6); //  15.71%
	InitDS_Sel(0x49004866); //   5.05%
	InitDS_Sel(0x4d02a1e6); //  15.31%
	InitDS_Sel(0x9fe191e6); //   5.64%
	InitDS_Sel(0x9fe59066); //  20.56%
	InitDS_Sel(0x9fe991e6); //  19.59%
	InitDS_Sel(0xcd0381e6); //   5.84%

	// suikoden 5

	InitDS_Sel(0x00428868); //  14.32%
	InitDS_Sel(0x40428868); //  20.87%
	InitDS_Sel(0x4846804c); //  27.56%
	InitDS_Sel(0x48819368); //  26.24%
	InitDS_Sel(0x48828b68); //  29.80%
	InitDS_Sel(0x48829368); //  22.30%
	InitDS_Sel(0x48858368); //   8.44%
	InitDS_Sel(0x48858b68); //   6.10%
	InitDS_Sel(0x48859068); //  22.77%
	InitDS_Sel(0x48859368); //   7.35%
	InitDS_Sel(0x48869068); //  30.96%
	InitDS_Sel(0x48878b68); //   5.18%
	InitDS_Sel(0x48879368); //  10.31%
	InitDS_Sel(0x488a8b68); //  11.73%
	InitDS_Sel(0x49028868); //  14.35%
	InitDS_Sel(0x4906804c); //  30.53%
	InitDS_Sel(0x4d068868); //  33.72%
	InitDS_Sel(0x4d0e8868); //  34.68%

	// dq8

	InitDS_Sel(0x48830064); //   8.11%
	InitDS_Sel(0x48869164); //  12.03%
	InitDS_Sel(0x490a8164); //   5.03%
	InitDS_Sel(0x490e904c); //  19.05%
	InitDS_Sel(0x490f904c); //  15.81%
	InitDS_Sel(0x9103b04c); //   5.05%
	InitDS_Sel(0xc840404c); //  15.86%
	InitDS_Sel(0xc883914c); //   5.84%
	InitDS_Sel(0xc885804c); //  22.07%
	InitDS_Sel(0xc8859054); //   9.61%
	InitDS_Sel(0xc8c3804c); //  36.23%
	InitDS_Sel(0xdfe3904e); //   5.49%

	// resident evil 4

	InitDS_Sel(0x1fe04057); //   6.33%
	InitDS_Sel(0x48868064); //   7.84%
	InitDS_Sel(0x4886814c); //   5.42%
	InitDS_Sel(0x48868164); //  12.98%
	InitDS_Sel(0x48868864); //  39.04%
	InitDS_Sel(0x48868964); //  10.15%
	InitDS_Sel(0x48878164); //  64.35%
	InitDS_Sel(0x4b068064); //   6.74%
	InitDS_Sel(0x9fe18064); //  11.81%
	InitDS_Sel(0xc880404c); //   8.39%
	InitDS_Sel(0xc883814c); //  13.71%
	InitDS_Sel(0xc885904c); //  11.74%
	InitDS_Sel(0xc887804c); //  25.43%
	InitDS_Sel(0xc887814c); //  17.68%
	InitDS_Sel(0xc903904c); //  15.89%
	InitDS_Sel(0xc907814c); //  10.74%
	InitDS_Sel(0xcc879064); //  25.97%
	InitDS_Sel(0xcd004064); //  70.25%
	InitDS_Sel(0xcd03904c); //  23.53%
	InitDS_Sel(0xcd07814c); //   8.41%
	InitDS_Sel(0xd483904c); //   6.04%
	InitDS_Sel(0xdfe1904e); //  19.81%
	InitDS_Sel(0xdfe5904c); //  12.52%

	// tomoyo after 

	InitDS_Sel(0x48404868); //  30.60%
	InitDS_Sel(0x9fe38059); //  21.23%
	InitDS_Sel(0x9fe39059); //  20.70%
	InitDS_Sel(0xc8478068); //   8.06%
	InitDS_Sel(0xc8818068); //  26.07%
	InitDS_Sel(0xc9058068); //  15.90%
	InitDS_Sel(0xca858068); //  14.66%

	// .hack redemption

	InitDS_Sel(0x1fe04864); //   5.01%
	InitDS_Sel(0x48404074); //   6.97%
	InitDS_Sel(0x48469064); //  20.80%
	InitDS_Sel(0x48478064); //   8.87%
	InitDS_Sel(0x48804864); //   5.94%
	InitDS_Sel(0x48869364); //  22.39%
	InitDS_Sel(0x488e9064); //  23.49%
	InitDS_Sel(0x49004074); //   6.95%
	InitDS_Sel(0x49004864); //   7.26%
	InitDS_Sel(0xc123004c); //  17.86%
	InitDS_Sel(0xc8478064); //   8.89%
	InitDS_Sel(0xc8804054); //  13.68%
	InitDS_Sel(0xc903004c); //  16.45%
	InitDS_Sel(0xcc41804c); //  11.98%
	InitDS_Sel(0xdfe1004c); //  11.81%

	// wild arms 4

	InitDS_Sel(0x9fe19050); //   6.28%
	InitDS_Sel(0x9fe58064); //  11.45%
	InitDS_Sel(0x9fe59064); //  10.01%
	InitDS_Sel(0xc8404064); //  22.23%
	InitDS_Sel(0xccc0404c); //  10.08%
	InitDS_Sel(0xccc04064); //   5.64%
	InitDS_Sel(0xcd07804c); //  39.49%
	InitDS_Sel(0xcd078164); //  19.54%
	InitDS_Sel(0xcd45804c); //   5.23%
	InitDS_Sel(0xdfe19054); //  17.70%

	// wild arms 5

	InitDS_Sel(0x4885884c); //   6.64%
	InitDS_Sel(0x4887904c); //  25.66%
	InitDS_Sel(0x488e8764); //  25.45%
	InitDS_Sel(0x48c68864); //   6.46%
	InitDS_Sel(0x9fe3804c); //   7.00%
	InitDS_Sel(0xc845804c); //  14.03%
	InitDS_Sel(0xdfe39054); //  19.69%

	// shadow of the colossus

	InitDS_Sel(0x4883804c); // 184.84%
	InitDS_Sel(0x48868b64); //  12.88%
	InitDS_Sel(0x48869064); //  27.18%
	InitDS_Sel(0x48878364); //  23.80%
	InitDS_Sel(0x48879064); //  27.06%
	InitDS_Sel(0x48879364); //  16.89%
	InitDS_Sel(0x488e8864); // 148.99%
	InitDS_Sel(0x488e9364); //   5.28%
	InitDS_Sel(0x490e8064); //  66.50%
	InitDS_Sel(0x490e8864); //   8.82%
	InitDS_Sel(0x490f8064); //   8.23%
	InitDS_Sel(0x4d004064); //  88.17%
	InitDS_Sel(0x9fe04064); //   9.69%
	InitDS_Sel(0x9fe1004d); //  10.39%
	InitDS_Sel(0x9fe3904d); //  15.56%
	InitDS_Sel(0xc883804c); //  45.16%
	InitDS_Sel(0xc883904c); //   5.41%
	InitDS_Sel(0xc8938064); //  45.66%
	InitDS_Sel(0xc8939064); //  15.27%
	InitDS_Sel(0xc900404c); //   8.73%
	InitDS_Sel(0xc9004064); //  51.37%
	InitDS_Sel(0xc903804c); //  35.17%
	InitDS_Sel(0xca83004c); //  17.37%
	InitDS_Sel(0xcc030064); //  21.25%
	InitDS_Sel(0xcc80404c); //   9.19%

	// tales of rebirth

	InitDS_Sel(0x48404054); //  10.33%
	InitDS_Sel(0x48878054); //  75.33%
	InitDS_Sel(0x48878b64); //  10.17%
	InitDS_Sel(0x4c838854); //  15.15%
	InitDS_Sel(0xc8478054); //  13.29%
	InitDS_Sel(0xc88b9054); //   7.08%
	InitDS_Sel(0xcc838064); //  16.57%

	// digital devil saga

	InitDS_Sel(0x48804050); //   6.18%
	InitDS_Sel(0x48868870); //   5.80%
	InitDS_Sel(0x488e8860); //  22.08%
	InitDS_Sel(0x4907884c); //  28.15%
	InitDS_Sel(0x9fe39070); //   5.89%

	// dbzbt2

	InitDS_Sel(0x4906884c); //  17.04%
	InitDS_Sel(0x4c904064); //  28.94%
	InitDS_Sel(0xc8878074); //  26.80%
	InitDS_Sel(0xc9078054); //  22.93%

	// dbzbt3

	InitDS_Sel(0x489081e4); //  11.25%
	InitDS_Sel(0x48968864); //  16.90%
	InitDS_Sel(0x4c469064); //  14.83%
	InitDS_Sel(0x4c80404c); //   6.99%
	InitDS_Sel(0x4c869064); //  15.61%
	InitDS_Sel(0xc88391cc); //  12.45%
	InitDS_Sel(0xc885904e); //  16.45%
	InitDS_Sel(0xc8859074); //  15.19%
	InitDS_Sel(0xc905904c); //  27.52%
	InitDS_Sel(0xc917804c); //   5.66%
	InitDS_Sel(0xca40404c); //  10.86%
	InitDS_Sel(0xcc45904e); //  33.93%
	InitDS_Sel(0xcc80404e); //  12.83%
	InitDS_Sel(0xcc8391cc); //  25.49%

	// dbz iw

	InitDS_Sel(0x1fe48864); //  11.22%
	InitDS_Sel(0x1fe49064); //   5.09%
	InitDS_Sel(0x1fec8864); //  26.53%
	InitDS_Sel(0x1fec8964); //   6.54%
	InitDS_Sel(0x48808064); //   7.90%
	InitDS_Sel(0x48848064); //   5.32%
	InitDS_Sel(0x48848864); //  18.43%
	InitDS_Sel(0x48858064); //  20.14%
	InitDS_Sel(0x48859064); //  13.97%
	InitDS_Sel(0x49058064); //   9.97%
	InitDS_Sel(0x49084064); //   9.60%
	InitDS_Sel(0x9fe19064); //  17.67%
	InitDS_Sel(0xc881004c); //  16.40%
	InitDS_Sel(0xc8858064); //  38.16%
	InitDS_Sel(0xc8859064); //  26.38%
	InitDS_Sel(0xc88d8064); //   7.05%
	InitDS_Sel(0xc88d9064); //  15.38%
	InitDS_Sel(0xc9058064); //  12.95%
	InitDS_Sel(0xc9084064); //  14.20%
	InitDS_Sel(0xc90d8064); //  21.35%
	InitDS_Sel(0xcd404054); //  18.85%

	// disgaea 2

	InitDS_Sel(0x1fe04064); //   5.51%
	InitDS_Sel(0x1fe69074); //   8.82%
	InitDS_Sel(0x48878964); //  65.93%
	InitDS_Sel(0xc8879164); //   5.09%

	// gradius 5

	InitDS_Sel(0x48868968); //  40.64%
	InitDS_Sel(0x48878968); //   7.97%
	InitDS_Sel(0x49078968); //   7.80%
	InitDS_Sel(0x490e884c); //  37.26%
	InitDS_Sel(0x5fe68068); //  32.99%
	InitDS_Sel(0x5fe68968); //  15.59%
	InitDS_Sel(0x5fee8168); //   5.25%
	InitDS_Sel(0x5fee8868); //  36.64%
	InitDS_Sel(0x5fee8968); //  16.28%
	InitDS_Sel(0x5ffe8868); //   6.67%
	InitDS_Sel(0xdfe3814c); //  24.29%

	// tales of abyss

	InitDS_Sel(0x1fe39368); //   7.47%
	InitDS_Sel(0x4885804c); //  14.27%
	InitDS_Sel(0x48868068); //  11.14%
	InitDS_Sel(0x4886934c); //   6.41%
	InitDS_Sel(0x4887834c); //   8.90%
	InitDS_Sel(0x488e8368); //  13.35%
	InitDS_Sel(0x48cf89e8); //  39.25%
	InitDS_Sel(0x4903834c); //  21.04%
	InitDS_Sel(0x490c8b68); //  15.04%
	InitDS_Sel(0x490e8b68); //   8.05%
	InitDS_Sel(0x490f89e8); //  39.57%
	InitDS_Sel(0x4d03914c); //  18.97%
	InitDS_Sel(0xc121004c); //  26.59%
	InitDS_Sel(0xc887934c); //   5.73%
	InitDS_Sel(0xdfe59078); //  21.43%

	// Gundam Seed Destiny OMNI VS ZAFT II PLUS 

	InitDS_Sel(0x1fe68075); //  27.61%
	InitDS_Sel(0x4880484d); //  13.50%
	InitDS_Sel(0x48878075); //   8.16%
	InitDS_Sel(0x48878375); //   8.66%
	InitDS_Sel(0x4887884d); //  17.82%
	InitDS_Sel(0x48878b75); //  53.12%
	InitDS_Sel(0x488e8075); //  42.24%
	InitDS_Sel(0x488e8375); //  35.32%
	InitDS_Sel(0x488e8875); //  25.59%
	InitDS_Sel(0x488e8b75); //  51.44%
	InitDS_Sel(0x488e9075); //  16.57%
	InitDS_Sel(0x49068075); //  35.78%
	InitDS_Sel(0x4906884d); //   6.37%
	InitDS_Sel(0x490e8375); //  31.56%
	InitDS_Sel(0x490e8875); //  35.20%
	InitDS_Sel(0x490e8b75); //  38.85%
	InitDS_Sel(0x9fe19075); //  20.98%
	InitDS_Sel(0xc8878075); //  14.30%

	// nba 2k8

	InitDS_Sel(0x1fe04056); //  15.57%
	InitDS_Sel(0x1fe38966); //  28.88%
	InitDS_Sel(0x1fe39156); //  25.28%
	InitDS_Sel(0x1fe60866); //   5.67%
	InitDS_Sel(0x1fe68866); //   5.75%
	InitDS_Sel(0x48838166); //  10.93%
	InitDS_Sel(0x48868066); //   7.59%
	InitDS_Sel(0x48868166); //  10.44%
	InitDS_Sel(0x48868866); //  42.03%
	InitDS_Sel(0x48868966); //  30.06%
	InitDS_Sel(0x48869166); //   6.52%
	InitDS_Sel(0x48879066); //  10.60%
	InitDS_Sel(0x49028966); //   7.28%
	InitDS_Sel(0x49068066); //  31.37%
	InitDS_Sel(0x49068966); //  11.65%
	InitDS_Sel(0x49068976); //  45.50%
	InitDS_Sel(0x5fe68866); //  22.26%
	InitDS_Sel(0x9fe79066); //  28.38%
	InitDS_Sel(0xc8879166); //   6.42%
	InitDS_Sel(0xdfe79066); //  30.98%

	// onimusha 3

	InitDS_Sel(0x1fee004c); //   5.11%
	InitDS_Sel(0x1fee0868); //  42.66%
	InitDS_Sel(0x1fee8968); //   7.76%
	InitDS_Sel(0x48878068); //  24.27%
	InitDS_Sel(0x48c28368); //   5.97%
	InitDS_Sel(0x4903884c); //  28.26%
	InitDS_Sel(0x49068068); //   9.53%
	InitDS_Sel(0x4d05884c); //   6.59%
	InitDS_Sel(0x5fe04078); //  29.53%
	InitDS_Sel(0x9fe18068); //   5.38%
	InitDS_Sel(0xc8839168); //   6.59%
	InitDS_Sel(0xcc81904c); //   5.21%
	InitDS_Sel(0xcc878168); //   7.18%
	InitDS_Sel(0xcd004068); //  28.32%
	InitDS_Sel(0xcd03804c); //   7.20%
	InitDS_Sel(0xd425904c); //   5.69%
	InitDS_Sel(0xdfe78368); //   6.71%
	InitDS_Sel(0xdfe7904c); //   8.43%
	InitDS_Sel(0xdfe79368); //   9.36%

	// resident evil code veronica

	InitDS_Sel(0x1fee8168); //   9.31%
	InitDS_Sel(0x1fee8868); //   6.75%
	InitDS_Sel(0x48804068); //  12.40%
	InitDS_Sel(0x48804868); //  41.21%
	InitDS_Sel(0x48804b68); //   7.16%
	InitDS_Sel(0x9fe39068); //  17.58%
	InitDS_Sel(0x9fe79068); //  25.03%
	InitDS_Sel(0x9fe79168); //  25.89%
	InitDS_Sel(0xc8878068); //  29.01%
	InitDS_Sel(0xc8878368); //  11.44%
	InitDS_Sel(0xc8879368); //   6.59%
	InitDS_Sel(0xcc819058); //  23.03%

	// armored core 3

	InitDS_Sel(0x1fe84074); //   6.66%
	InitDS_Sel(0x1fee0874); //  59.28%
	InitDS_Sel(0x48404854); //   7.27%
	InitDS_Sel(0x48878074); //  10.32%
	InitDS_Sel(0x48878874); //  16.96%
	InitDS_Sel(0x488e8074); //  25.40%
	InitDS_Sel(0x490e8074); //  79.82%
	InitDS_Sel(0x4c4e8074); //  23.05%
	InitDS_Sel(0x4d0b0864); //   5.94%
	InitDS_Sel(0x4d0e8074); //   8.44%
	InitDS_Sel(0xc8404054); //   9.47%
	InitDS_Sel(0xc8850054); //  11.54%
	InitDS_Sel(0xc88581d4); //   6.72%
	InitDS_Sel(0xc88791d4); //   6.83%
	InitDS_Sel(0xc9059054); //  13.98%
	InitDS_Sel(0xc9078074); //   9.60%

	// aerial planet

	InitDS_Sel(0x4886894c); //  18.07%
	InitDS_Sel(0x488e814c); //  16.96%
	InitDS_Sel(0x4c868074); //  46.13%
	InitDS_Sel(0x4c868874); //   7.71%
	InitDS_Sel(0x4c868934); //  19.26%
	InitDS_Sel(0x4c8e8074); //  12.50%
	InitDS_Sel(0x4c8e8874); //  27.69%
	InitDS_Sel(0x4cc0404c); //  15.74%
	InitDS_Sel(0xc820404c); //  16.32%
	InitDS_Sel(0xc8478164); //  11.12%
	InitDS_Sel(0xc847914c); //   7.70%
	InitDS_Sel(0xc887914c); //   8.85%

	// one piece grand battle 3

	InitDS_Sel(0x48804054); //  12.28%
	InitDS_Sel(0x48878854); //  12.62%
	InitDS_Sel(0x49068174); //   5.54%
	InitDS_Sel(0x49068874); //  28.97%
	InitDS_Sel(0x49068964); //  16.11%
	InitDS_Sel(0x49078174); //  11.17%
	InitDS_Sel(0x9fe1904e); //  10.23%
	InitDS_Sel(0xc8839054); //  21.50%
	InitDS_Sel(0xc8878054); //   6.54%
	InitDS_Sel(0xc8879054); //  10.18%
	InitDS_Sel(0xc9078174); //   8.49%
	InitDS_Sel(0xcac0404c); //   9.30%
	InitDS_Sel(0xcc41904c); //  12.33%
	InitDS_Sel(0xcc4190cc); //   7.21%
	InitDS_Sel(0xd321914c); //   7.10%

	// one piece grand adventure

	InitDS_Sel(0x1fe0404c); //   5.18%
	InitDS_Sel(0x48829164); //  12.79%
	InitDS_Sel(0x48849164); //  11.67%
	InitDS_Sel(0x48869154); //   5.38%
	InitDS_Sel(0x48878154); //   5.01%
	InitDS_Sel(0x48879154); //   5.57%
	InitDS_Sel(0x49078964); // 151.46%
	InitDS_Sel(0xc421814c); //   7.73%
	InitDS_Sel(0xc843b04c); //  21.62%

	// shadow hearts

	InitDS_Sel(0x1fe6904c); //   5.97%
	InitDS_Sel(0x48868078); //   9.90%
	InitDS_Sel(0x48868778); //   9.59%
	InitDS_Sel(0x49004058); //   7.20%
	InitDS_Sel(0x49030058); //  40.18%
	InitDS_Sel(0x4c870878); //   6.19%
	InitDS_Sel(0x9fe3004c); //  20.51%
	InitDS_Sel(0xc8804058); //   9.57%
	InitDS_Sel(0xc881904e); //   5.29%
	InitDS_Sel(0xc8819168); //  13.94%
	InitDS_Sel(0xc8839058); //  12.53%
	InitDS_Sel(0xc8878058); //  13.80%
	InitDS_Sel(0xc8879058); //   9.68%
	InitDS_Sel(0xc9039058); //  12.08%
	InitDS_Sel(0xc9078068); //  29.18%

	// the punisher

	InitDS_Sel(0x48420864); //   5.88%
	InitDS_Sel(0x48468864); //   8.39%
	InitDS_Sel(0x48868764); //  16.69%
	InitDS_Sel(0x4886bf64); //  14.77%
	InitDS_Sel(0x49068064); //   7.90%
	InitDS_Sel(0x4906904c); //  14.02%
	InitDS_Sel(0x4d068f64); //  12.33%
	InitDS_Sel(0x5fe68f64); //  62.24%
	InitDS_Sel(0xc880474c); //  20.85%
	InitDS_Sel(0xc883974c); //   6.07%
	InitDS_Sel(0xc887874c); //   8.47%
	InitDS_Sel(0xc887974c); //  10.36%
	InitDS_Sel(0xdfe3974c); //  21.42%

	// guitar hero

	InitDS_Sel(0x1fe0407b); //   6.67%
	InitDS_Sel(0x1fe6887a); //   9.08%
	InitDS_Sel(0x4880484e); //  34.74%
	InitDS_Sel(0x4886807a); //   6.73%
	InitDS_Sel(0x4886887a); //  47.23%
	InitDS_Sel(0x4886907a); //   5.53%
	InitDS_Sel(0x4886956a); //   8.71%
	InitDS_Sel(0x4887854e); //   6.67%
	InitDS_Sel(0x4887887a); //  31.33%
	InitDS_Sel(0x4887954e); //  20.26%
	InitDS_Sel(0x488a917a); //  33.17%
	InitDS_Sel(0x488e887a); //  80.36%
	InitDS_Sel(0x488e8d7a); //   5.16%
	InitDS_Sel(0x4906806a); //  23.53%
	InitDS_Sel(0x4906886a); //   5.02%
	InitDS_Sel(0x4d06a06a); //  22.00%
	InitDS_Sel(0x4d06a86a); //  15.44%
	InitDS_Sel(0x4d0ea06a); //   5.08%
	InitDS_Sel(0x9503204e); //  22.47%
	InitDS_Sel(0x9fe3906a); //  12.46%
	InitDS_Sel(0xc887855a); //   5.09%
	InitDS_Sel(0xc887857a); //  27.54%
	InitDS_Sel(0xcd03204e); //  70.62%
	InitDS_Sel(0xcd07806a); //   8.94%

	// ico

	InitDS_Sel(0x1fe28060); //  16.14%
	InitDS_Sel(0x1fe68860); //  47.91%
	InitDS_Sel(0x48868060); //  10.32%
	InitDS_Sel(0x48868b60); //  94.89%
	InitDS_Sel(0x49068b60); //  10.26%
	InitDS_Sel(0x4c468b60); //  41.67%
	InitDS_Sel(0x4c478860); //  11.90%
	InitDS_Sel(0x4d004060); // 102.46%
	InitDS_Sel(0x4d028060); //  17.75%
	InitDS_Sel(0x4d068360); //  16.19%
	InitDS_Sel(0x4d068860); //  18.08%
	InitDS_Sel(0x4d068b60); // 223.13%
	InitDS_Sel(0x4d078360); //   5.55%
	InitDS_Sel(0x9fe04060); //   7.09%
	InitDS_Sel(0x9fe380cc); //  16.22%
	InitDS_Sel(0x9fe3a04c); //  47.14%
	InitDS_Sel(0xc8859060); //  13.85%
	InitDS_Sel(0xc893814c); //  40.77%
	InitDS_Sel(0xc9004060); //   6.15%
	InitDS_Sel(0xcd078060); //   5.49%
	InitDS_Sel(0xcd078360); //   9.27%

	// kuon

	InitDS_Sel(0x1fee0865); //  19.11%
	InitDS_Sel(0x48860065); //  26.44%
	InitDS_Sel(0x48860865); //  24.38%
	InitDS_Sel(0x48868365); //  18.01%
	InitDS_Sel(0x48868b65); //  31.33%
	InitDS_Sel(0x488e0865); //  39.97%
	InitDS_Sel(0x488e0b65); //  20.78%
	InitDS_Sel(0x488e8b65); //  12.26%
	InitDS_Sel(0x4c429065); //  25.89%
	InitDS_Sel(0x4d068365); //   9.17%
	InitDS_Sel(0xc847004d); //  13.35%
	InitDS_Sel(0xc887004d); //  17.53%
	InitDS_Sel(0xc8870065); //  28.03%
	InitDS_Sel(0xc907004d); //  18.64%
	InitDS_Sel(0xc9070065); //   5.21%

	// hxh

	InitDS_Sel(0x1fe04876); //   7.16%
	InitDS_Sel(0x1fee8076); //   9.68%
	InitDS_Sel(0x1fee8976); //  17.11%
	InitDS_Sel(0x9fe04076); //   5.37%
	InitDS_Sel(0x9fe79176); //   6.77%
	InitDS_Sel(0xc8804076); //   6.64%
	InitDS_Sel(0xc8838176); //   6.95%
	InitDS_Sel(0xc8839176); //   6.85%
	InitDS_Sel(0xc8878076); //   5.29%

	// grandia extreme

	InitDS_Sel(0x1fe3884c); //  27.03%
	InitDS_Sel(0x45269070); //   5.74%
	InitDS_Sel(0x452e9070); //   7.25%
	InitDS_Sel(0x48868070); //  13.25%
	InitDS_Sel(0x48869070); //  24.24%
	InitDS_Sel(0x48878370); //  22.45%
	InitDS_Sel(0x48879070); //  21.66%
	InitDS_Sel(0x48879370); //  13.17%
	InitDS_Sel(0x4888404c); //  12.23%
	InitDS_Sel(0x48884050); //  15.68%
	InitDS_Sel(0x488e8870); //  44.66%
	InitDS_Sel(0x488e8b70); //  45.62%
	InitDS_Sel(0x488e9370); //  14.64%
	InitDS_Sel(0x9fe3934c); //  16.61%
	InitDS_Sel(0xcc81934c); //  62.20%

	// enthusia

	InitDS_Sel(0x1fe04854); //  23.33%
	InitDS_Sel(0x1fe60064); //   5.72%
	InitDS_Sel(0x1fe60068); //   5.03%
	InitDS_Sel(0x1fee0064); //   7.51%
	InitDS_Sel(0x1fee0864); //  15.47%
	InitDS_Sel(0x48860f64); //   9.63%
	InitDS_Sel(0x48868f64); //  29.41%
	InitDS_Sel(0x488e0f64); //  11.80%
	InitDS_Sel(0x49068864); //  23.63%
	InitDS_Sel(0x4b020864); //  13.40%
	InitDS_Sel(0x4b060064); //  11.39%
	InitDS_Sel(0x4b060864); //   8.38%
	InitDS_Sel(0x4b068864); //  14.03%
	InitDS_Sel(0x9fe79064); //  13.49%
	InitDS_Sel(0xcd40404c); //  11.10%

	// ys 1/2 eternal story

	// bloody roar

	InitDS_Sel(0x48810068); //  25.54%
	InitDS_Sel(0x48848068); //  68.60%
	InitDS_Sel(0x488789e8); //  10.83%
	InitDS_Sel(0x488791e8); //  11.18%
	InitDS_Sel(0x49004068); //   9.51%
	InitDS_Sel(0x49004868); //   6.95%
	InitDS_Sel(0x49018368); //  15.01%
	InitDS_Sel(0x49019368); //  13.90%
	InitDS_Sel(0x4b068068); //  13.98%
	InitDS_Sel(0x4b068868); //  14.29%
	InitDS_Sel(0x4c469068); //   9.71%

	// ferrari f355 challenge

	InitDS_Sel(0x489e8064); //   9.07%
	InitDS_Sel(0x489e8b64); //   8.01%
	InitDS_Sel(0x5fe04064); //  10.07%
	InitDS_Sel(0x5fe04068); //  13.01%
	InitDS_Sel(0x5fe04868); //   6.33%
	InitDS_Sel(0x5fe60064); //  10.58%
	InitDS_Sel(0x5fee0064); //  14.14%
	InitDS_Sel(0x5fee0864); //  21.09%
	InitDS_Sel(0x5feeb864); //   7.78%
	InitDS_Sel(0x5ff60064); //  16.67%
	InitDS_Sel(0x5ffe0064); //  17.43%
	InitDS_Sel(0x5ffe0864); //  23.98%
	InitDS_Sel(0xc8858168); //  54.96%
	InitDS_Sel(0xc890404c); //   6.30%
	InitDS_Sel(0xdfef0064); //   8.68%

	// king of fighters xi

	InitDS_Sel(0x488589e8); // 110.21%
	InitDS_Sel(0x488591e8); //  55.38%
	InitDS_Sel(0xf4819050); //  12.12%

	// mana khemia

	InitDS_Sel(0x488e8369); //  96.11%
	InitDS_Sel(0xc880404d); //  12.30%
	InitDS_Sel(0xc881804d); //  16.48%
	InitDS_Sel(0xc8819059); //  22.73%
	InitDS_Sel(0xc885904d); //  12.36%
	InitDS_Sel(0xc907804d); //  59.32%
	InitDS_Sel(0xc90f8369); //   7.80%

	// ar tonelico 2

	InitDS_Sel(0x484f8369); //   7.55%
	InitDS_Sel(0x488e8069); //  22.13%
	InitDS_Sel(0x488e9069); //  33.42%
	InitDS_Sel(0x488e9369); // 103.56%
	InitDS_Sel(0x488f8369); //  23.74%
	InitDS_Sel(0x490f8369); //  29.15%
	InitDS_Sel(0xc885804d); //   5.80%
	InitDS_Sel(0xc887804d); //   7.94%
	InitDS_Sel(0xc88f9369); //  13.74%
	InitDS_Sel(0xc905904d); //   6.23%

	// rouge galaxy

	InitDS_Sel(0x1fe0484c); //   6.12%
	InitDS_Sel(0x484e8164); //  53.09%
	InitDS_Sel(0x48879054); //  47.81%
	InitDS_Sel(0x488b0964); //  95.23%
	InitDS_Sel(0x490e8164); //  24.34%
	InitDS_Sel(0x490e9164); //   5.24%
	InitDS_Sel(0xc8858154); //  18.49%
	InitDS_Sel(0xdff0404c); //   7.47%

	// mobile suit gundam seed battle assault 3

	InitDS_Sel(0x49004854); //   7.22%
	InitDS_Sel(0x4c804054); //  12.92%
	InitDS_Sel(0x5fee8074); //  14.87%
	InitDS_Sel(0x5fee8874); //  58.09%
	InitDS_Sel(0x5fee9174); //   5.15%
	InitDS_Sel(0xc88390cc); //  21.42%
	InitDS_Sel(0xc88791cc); //   6.20%
	InitDS_Sel(0xc90781cc); //  12.73%
	InitDS_Sel(0xcc81004d); //  11.15%

	// hajime no ippo all stars

	InitDS_Sel(0x48848868); //   7.31%
	InitDS_Sel(0x48848b68); //   6.25%
	InitDS_Sel(0x48858868); //   6.71%
	InitDS_Sel(0x488c8b68); //  16.70%

	// hajime no ippo 2

	InitDS_Sel(0x1fe04068); //  11.95%
	InitDS_Sel(0x4881004c); //  20.37%
	InitDS_Sel(0x48839368); //  31.79%
	InitDS_Sel(0x48858068); //  55.12%
	InitDS_Sel(0x48868b68); //  23.49%
	InitDS_Sel(0x48878368); //   6.54%
	InitDS_Sel(0x48879068); //  29.98%
	InitDS_Sel(0x488c8068); //  10.13%
	InitDS_Sel(0x488c8868); //  16.20%
	InitDS_Sel(0x488e8b68); //  38.97%
	InitDS_Sel(0x488e9068); //   7.20%
	InitDS_Sel(0x488e9368); //  13.42%
	InitDS_Sel(0x49028b68); //  48.16%
	InitDS_Sel(0x4b0e8068); //   5.51%
	InitDS_Sel(0x4b0e8868); //  14.85%
	InitDS_Sel(0x4d0e8068); //   7.65%
	InitDS_Sel(0xc8859068); //  21.31%

	// virtual tennis 2

	InitDS_Sel(0x488049e5); //   5.52%
	InitDS_Sel(0x48868075); //  28.20%
	InitDS_Sel(0x48868875); //  20.93%
	InitDS_Sel(0x4c8781f5); //  11.54%
	InitDS_Sel(0x9540404d); //   6.25%
	InitDS_Sel(0xc8859065); //  11.57%

	// crash wrath of cortex

	InitDS_Sel(0x1fe20864); //  15.87%
	InitDS_Sel(0x48828764); //   9.14%
	InitDS_Sel(0x48828f64); //  18.80%
	InitDS_Sel(0x48838364); //  12.07%
	InitDS_Sel(0x48838f64); //   5.84%
	InitDS_Sel(0x49028f64); //  16.48%
	InitDS_Sel(0x49038f64); //  34.44%
	InitDS_Sel(0x9fe78064); //  11.75%
	InitDS_Sel(0xc8818364); //   9.12%
	InitDS_Sel(0xc9030064); // 100.90%
	InitDS_Sel(0xca838364); //   8.86%
	InitDS_Sel(0xcd05834c); //   8.90%

	// sbam 2

	// remember 11

	// prince of tennis

	InitDS_Sel(0x4888484c); //   5.28%
	InitDS_Sel(0x488d8164); //  30.73%
	InitDS_Sel(0x488d9164); //  23.46%
	InitDS_Sel(0xc885914c); //   5.01%
	InitDS_Sel(0xc8859164); //   7.79%
	InitDS_Sel(0xc88d8164); //   5.33%
	InitDS_Sel(0xc88d9164); //  26.18%
	InitDS_Sel(0xc8958164); //  10.14%
	InitDS_Sel(0xc89d814c); //   7.09%
	InitDS_Sel(0xcd458064); //  13.67%

	// ar tonelico

	InitDS_Sel(0x48868369); //  17.29%
	InitDS_Sel(0x48869369); //  16.05%
	InitDS_Sel(0x488f9369); //   6.07%
	InitDS_Sel(0x49078b69); //  54.61%
	InitDS_Sel(0x490f8069); //  54.79%
	InitDS_Sel(0x490f9369); //   5.20%
	InitDS_Sel(0xc8804069); //  16.59%
	InitDS_Sel(0xc8878069); //  32.09%
	InitDS_Sel(0xc8878369); //  14.18%
	InitDS_Sel(0xc8879069); //  31.83%
	InitDS_Sel(0xc8879369); //  12.02%
	InitDS_Sel(0xc9038069); // 245.53%

	// dbz sagas

	InitDS_Sel(0x48828964); //  38.72%
	InitDS_Sel(0x488e9164); //  10.98%
	InitDS_Sel(0x54229164); //   6.16%

	// tourist trophy

	InitDS_Sel(0x1fe84064); //  14.21%
	InitDS_Sel(0x1fee8064); //  23.50%
	InitDS_Sel(0x1fee9064); //  13.72%
	InitDS_Sel(0x488a8064); //  10.03%
	InitDS_Sel(0x488e8065); //  16.63%
	InitDS_Sel(0x488e9065); //  16.00%
	InitDS_Sel(0x5fe3804c); //   8.34%
	InitDS_Sel(0x5fe3904c); //   7.65%
	InitDS_Sel(0x5fee8164); //   7.66%
	InitDS_Sel(0x9fe1904c); //   9.19%
	InitDS_Sel(0x9fe1904d); //   6.10%
	InitDS_Sel(0x9fe3804d); //   9.23%
	InitDS_Sel(0x9fe5904d); //   7.20%
	InitDS_Sel(0x9fe7804d); //   5.30%
	InitDS_Sel(0x9fe7904d); //   5.67%
	InitDS_Sel(0xc88181d4); //  11.57%
	InitDS_Sel(0xc881904d); //   7.00%
	InitDS_Sel(0xc88191d4); //  12.53%
	InitDS_Sel(0xc887904d); //   5.13%
	InitDS_Sel(0xcb03804c); //  21.06%
	InitDS_Sel(0xcc81904d); //  13.48%
	InitDS_Sel(0xcc83804d); //  10.28%
	InitDS_Sel(0xcc83904c); //  34.42%
	InitDS_Sel(0xcc83904d); //  10.24%
	InitDS_Sel(0xcc87904d); //  33.01%
	InitDS_Sel(0xcd05804c); //  17.24%
	InitDS_Sel(0xd520404c); //   9.81%
	InitDS_Sel(0xdfe1904c); //   7.52%
	InitDS_Sel(0xdfe5804c); //   7.14%

	// svr2k8

	InitDS_Sel(0x1fe79066); //  13.17%
	InitDS_Sel(0x4880404c); //   7.49%
	InitDS_Sel(0x4887804c); //   5.40%
	InitDS_Sel(0x4887814c); //   9.08%
	InitDS_Sel(0x488a0064); //   5.41%
	InitDS_Sel(0x4900404c); //  12.53%
	InitDS_Sel(0x490e8364); //  30.80%
	InitDS_Sel(0x4c97b874); //  15.16%
	InitDS_Sel(0xc887834c); //   7.67%

	// tokyo bus guide

	InitDS_Sel(0x1fe04070); //  13.22%
	InitDS_Sel(0x1fe04870); //  13.56%
	InitDS_Sel(0x1fee8170); //  10.70%
	InitDS_Sel(0x1fee8970); //  60.13%
	InitDS_Sel(0x1fee89f0); //  51.01%
	InitDS_Sel(0x48804870); //  12.85%
	InitDS_Sel(0x488a8850); //  21.74%
	InitDS_Sel(0xc8804070); //  13.67%
	InitDS_Sel(0xc8879070); //  35.52%
	InitDS_Sel(0xc88791f0); //  18.61%

	// 12riven

	// xenosaga

	InitDS_Sel(0x1fe38054); //  67.22%
	InitDS_Sel(0x1fe38074); //  25.86%
	InitDS_Sel(0x48478164); //  32.61%
	InitDS_Sel(0x48804964); //   7.12%
	InitDS_Sel(0x49078164); //  25.05%
	InitDS_Sel(0x4c818134); //  10.06%
	InitDS_Sel(0x4d069064); //  98.01%
	InitDS_Sel(0x4d084864); //  13.35%
	InitDS_Sel(0x5fe04074); //  61.58%
	InitDS_Sel(0x5fe68864); //  39.80%
	InitDS_Sel(0x5fee8864); //  45.95%
	InitDS_Sel(0x9fe04074); //   9.60%
	InitDS_Sel(0x9fe1804c); //   9.55%
	InitDS_Sel(0x9fe19074); //  16.03%
	InitDS_Sel(0xc8819054); //  35.54%
	InitDS_Sel(0xc8830074); //  47.20%
	InitDS_Sel(0xc8858164); //   7.84%
	InitDS_Sel(0xc9078164); // 154.58%
	InitDS_Sel(0xcc819074); //  19.87%
	InitDS_Sel(0xcd00404c); //  31.81%
	InitDS_Sel(0xd5204064); //  14.68%
	InitDS_Sel(0xdfe04074); //  14.26%

	// mgs3s1

	InitDS_Sel(0x482e8864); //  18.14%
	InitDS_Sel(0x484e8864); //  10.00%
	InitDS_Sel(0x48868364); //  94.90%
	InitDS_Sel(0x488e8364); //  15.82%
	InitDS_Sel(0x488f8064); //   5.31%
	InitDS_Sel(0x49268864); //   6.73%
	InitDS_Sel(0x4b0e8864); //   8.35%
	InitDS_Sel(0x4b0e8b64); //   6.23%
	InitDS_Sel(0x9fe1804d); //   9.13%
	InitDS_Sel(0x9fe7904c); //  11.17%
	InitDS_Sel(0xc8804364); //   5.24%
	InitDS_Sel(0xc8819364); //  19.10%
	InitDS_Sel(0xc883004c); //  17.25%
	InitDS_Sel(0xc8878364); //  12.09%
	InitDS_Sel(0xc8879064); //  22.53%
	InitDS_Sel(0xcc830074); //  54.93%
	InitDS_Sel(0xcd079364); //   5.24%
	InitDS_Sel(0xcd404074); //   9.03%

	// god of war

	// gta sa

	InitDS_Sel(0x1fe84864); //   6.23%
	InitDS_Sel(0x48810064); //  14.67%
	InitDS_Sel(0x49004054); //  16.40%
	InitDS_Sel(0x4d07804c); //  15.09%
	InitDS_Sel(0x4d078864); //   9.54%
	InitDS_Sel(0x4d0c8864); //   8.71%

	// haunting ground

	InitDS_Sel(0x1fe68864); //  38.74%
	InitDS_Sel(0x1fe689e4); //  44.79%
	InitDS_Sel(0x488689e4); //   5.03%
	InitDS_Sel(0xc843904c); //  16.66%
	InitDS_Sel(0xc88791e4); //  22.86%
	InitDS_Sel(0xcc23804c); //  16.80%
	InitDS_Sel(0xcd01904c); //  82.27%
	InitDS_Sel(0xdfe04064); //  14.73%
	InitDS_Sel(0xdff1904e); //  14.38%

	// odin sphere

	InitDS_Sel(0x4880404d); //   7.35%
	InitDS_Sel(0x4885804d); //   6.02%
	InitDS_Sel(0x4885904c); //  11.56%
	InitDS_Sel(0x488f904c); //   6.59%
	InitDS_Sel(0x488f904d); //   6.69%
	InitDS_Sel(0x4907804d); //   5.71%
	InitDS_Sel(0x4907884d); //   9.45%
	InitDS_Sel(0xc881904c); //   5.36%

	// kingdom hearts re:com

	InitDS_Sel(0x1fe04078); //   9.88%
	InitDS_Sel(0x1fee884c); //  17.61%
	InitDS_Sel(0x4880494c); //   6.46%
	InitDS_Sel(0x48868974); //  11.43%
	InitDS_Sel(0x488a8164); //   6.28%
	InitDS_Sel(0x488e804c); //   6.41%
	InitDS_Sel(0x488e874c); //   6.02%
	InitDS_Sel(0x4906814c); //  33.35%
	InitDS_Sel(0x4906894c); //  44.97%
	InitDS_Sel(0x49068974); //  15.85%
	InitDS_Sel(0x4907894c); //  16.25%
	InitDS_Sel(0x49078974); //   6.45%
	InitDS_Sel(0x9fe10054); //  14.10%
	InitDS_Sel(0x9fe3814d); //  14.12%
	InitDS_Sel(0x9fe3914d); //  17.86%
	InitDS_Sel(0xc885814c); //   7.69%
	InitDS_Sel(0xc907804c); //   8.56%
	InitDS_Sel(0xcd404064); //   9.28%
	InitDS_Sel(0xd120404c); //   9.47%

	// hyper street fighter 2 anniversary edition

	// star ocean 3

	InitDS_Sel(0x49068164); //  57.66%
	InitDS_Sel(0x9fe30064); //  24.77%
	InitDS_Sel(0x9fe38065); //  28.18%
	InitDS_Sel(0xc8839164); //  37.72%
	InitDS_Sel(0xc920404c); //  11.35%

	// aura for laura

	InitDS_Sel(0x1fe38070); //  31.10%
	InitDS_Sel(0x1fe3904c); //  19.62%
	InitDS_Sel(0x1fe39050); //   5.19%
	InitDS_Sel(0x1fe68070); //  19.79%
	InitDS_Sel(0x1fe78070); // 112.75%
	InitDS_Sel(0x1fe79070); //  65.97%
	InitDS_Sel(0x1fefb04c); //  17.85%
	InitDS_Sel(0x9fe38050); //  11.45%
	InitDS_Sel(0x9fe39050); //  11.01%
	InitDS_Sel(0x9fe78050); //  12.31%
	InitDS_Sel(0xc523804c); // 106.80%
	InitDS_Sel(0xc8839050); //  14.31%
	InitDS_Sel(0xc9038050); //   6.72%
	InitDS_Sel(0xc9058050); // 291.44%
	InitDS_Sel(0xcc818050); //  49.07%
	InitDS_Sel(0xd5204050); //   7.22%

	#endif
}

IDrawScanline::DrawScanlinePtr GSDrawScanline::GSDrawScanlineMap::GetDefaultFunction(DWORD dw)
{
	GSScanlineSelector sel;

	sel.dw = dw;

	return m_default[sel.fpsm][sel.zpsm][sel.ztst][sel.iip];
}

void GSDrawScanline::GSDrawScanlineMap::PrintStats()
{
	__super::PrintStats();

	if(FILE* fp = fopen("c:\\1.txt", "w"))
	{
		POSITION pos = m_map_active.GetHeadPosition();

		while(pos)
		{
			DWORD dw;
			ActivePtr* p;
			
			m_map_active.GetNextAssoc(pos, dw, p);

			if(m_map.Lookup(dw))
			{
				continue;
			}

			GSScanlineSelector sel;

			sel.dw = dw;

			if(p->frames > 30 && !sel.IsSolidRect())
			{
				int tpf = (int)((p->ticks / p->frames) * 10000 / (3000000000 / 60)); // 3 GHz, 60 fps

				if(tpf >= 500)
				{
					_ftprintf(fp, _T("InitDS_Sel(0x%08x); // %6.2f%%\n"), sel.dw, (float)tpf / 100);
				}
			}
		}

		fclose(fp);
	}
}

//

GSDrawScanline::GSSetupPrimMap::GSSetupPrimMap()
{
	#define InitSP_IIP(zbe, fge, tme, fst, iip) \
		m_default[zbe][fge][tme][fst][iip] = (SetupPrimPtr)&GSDrawScanline::SetupPrim<zbe, fge, tme, fst, iip>; \

	#define InitSP_FST(zbe, fge, tme, fst) \
		InitSP_IIP(zbe, fge, tme, fst, 0) \
		InitSP_IIP(zbe, fge, tme, fst, 1) \

	#define InitSP_TME(zbe, fge, tme) \
		InitSP_FST(zbe, fge, tme, 0) \
		InitSP_FST(zbe, fge, tme, 1) \

	#define InitSP_FGE(zbe, fge) \
		InitSP_TME(zbe, fge, 0) \
		InitSP_TME(zbe, fge, 1) \

	#define InitSP_ZBE(zbe) \
		InitSP_FGE(zbe, 0) \
		InitSP_FGE(zbe, 1) \

	InitSP_ZBE(0);
	InitSP_ZBE(1);
}

IDrawScanline::SetupPrimPtr GSDrawScanline::GSSetupPrimMap::GetDefaultFunction(DWORD dw)
{
	DWORD zbe = (dw >> 0) & 1;
	DWORD fge = (dw >> 1) & 1;
	DWORD tme = (dw >> 2) & 1;
	DWORD fst = (dw >> 3) & 1;
	DWORD iip = (dw >> 4) & 1;

	return m_default[zbe][fge][tme][fst][iip];
}

//

const GSVector4 GSDrawScanline::m_shift[4] = 
{
	GSVector4(0.0f, 1.0f, 2.0f, 3.0f),
	GSVector4(-1.0f, 0.0f, 1.0f, 2.0f),
	GSVector4(-2.0f, -1.0f, 0.0f, 1.0f),
	GSVector4(-3.0f, -2.0f, -1.0f, 0.0f),
};

const GSVector4i GSDrawScanline::m_test[8] = 
{
	GSVector4i::zero(),
	GSVector4i(0xffffffff, 0x00000000, 0x00000000, 0x00000000),
	GSVector4i(0xffffffff, 0xffffffff, 0x00000000, 0x00000000),
	GSVector4i(0xffffffff, 0xffffffff, 0xffffffff, 0x00000000),
	GSVector4i(0x00000000, 0xffffffff, 0xffffffff, 0xffffffff),
	GSVector4i(0x00000000, 0x00000000, 0xffffffff, 0xffffffff),
	GSVector4i(0x00000000, 0x00000000, 0x00000000, 0xffffffff),
	GSVector4i::zero(),
};
