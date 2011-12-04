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
#include "GSDrawScanline.h"
#include "GSTextureCacheSW.h"

GSDrawScanline::GSDrawScanline()
	: m_sp_map("GSSetupPrim", &m_local)
	, m_ds_map("GSDrawScanline", &m_local)
{
	memset(&m_local, 0, sizeof(m_local));

	m_local.gd = &m_global;
}

GSDrawScanline::~GSDrawScanline()
{
}

void GSDrawScanline::BeginDraw(const void* param)
{
	memcpy(&m_global, param, sizeof(m_global));

	if(m_global.sel.mmin && m_global.sel.lcm)
	{
		GSVector4i v = m_global.t.minmax.srl16(m_global.lod.i.x);

		v = v.upl16(v);

		m_local.temp.uv_minmax[0] = v.upl32(v);
		m_local.temp.uv_minmax[1] = v.uph32(v);
	}

	m_ds = m_ds_map[m_global.sel];

	if(m_global.sel.aa1)
	{
		GSScanlineSelector sel;

		sel.key = m_global.sel.key;
		sel.zwrite = 0;
		sel.edge = 1;

		m_de = m_ds_map[sel];
	}
	else
	{
		m_de = NULL;
	}

	if(m_global.sel.IsSolidRect())
	{
		m_dr = (DrawRectPtr)&GSDrawScanline::DrawRect;
	}
	else
	{
		m_dr = NULL;
	}

	// doesn't need all bits => less functions generated

	GSScanlineSelector sel;

	sel.key = 0;

	sel.iip = m_global.sel.iip;
	sel.tfx = m_global.sel.tfx;
	sel.tcc = m_global.sel.tcc;
	sel.fst = m_global.sel.fst;
	sel.fge = m_global.sel.fge;
	sel.sprite = m_global.sel.sprite;
	sel.fb = m_global.sel.fb;
	sel.zb = m_global.sel.zb;
	sel.zoverflow = m_global.sel.zoverflow;

	m_sp = m_sp_map[sel];
}

void GSDrawScanline::EndDraw(const GSRasterizerStats& stats, uint64 frame)
{
	m_ds_map.UpdateStats(stats, frame);
}

void GSDrawScanline::PrintStats() 
{
	m_ds_map.PrintStats();
}

#ifndef JIT_DRAW

void GSDrawScanline::SetupPrim(const GSVertexSW* vertices, const GSVertexSW& dscan)
{
	GSScanlineSelector sel = m_global.sel;

	const GSVector4* shift = GSSetupPrimCodeGenerator::m_shift;

	bool has_z = sel.zb != 0;
	bool has_f = sel.fb && sel.fge;
	bool has_t = sel.fb && sel.tfx != TFX_NONE;
	bool has_c = sel.fb && !(sel.tfx == TFX_DECAL && sel.tcc);

	if(has_z || has_f)
	{
		if(!sel.sprite)
		{
			if(has_f)
			{
				GSVector4 df = dscan.p.wwww();

				m_local.d4.f = GSVector4i(df * shift[0]).xxzzlh();

				for(int i = 0; i < 4; i++)
				{
					m_local.d[i].f = GSVector4i(df * shift[1 + i]).xxzzlh();
				}
			}

			if(has_z)
			{
				GSVector4 dz = dscan.p.zzzz();

				m_local.d4.z = dz * shift[0];

				for(int i = 0; i < 4; i++)
				{
					m_local.d[i].z = dz * shift[1 + i];
				}
			}
		}
		else
		{
			if(has_f)
			{
				m_local.p.f = GSVector4i(vertices[0].p).zzzzh().zzzz();
			}

			if(has_z)
			{
				m_local.p.z = vertices[0].t.u32[3]; // uint32 z is bypassed in t.w
			}
		}
	}

	if(has_t)
	{
		GSVector4 t = dscan.t;

		if(sel.fst)
		{
			m_local.d4.stq = GSVector4::cast(GSVector4i(t * shift[0]));
		}
		else
		{
			m_local.d4.stq = t * shift[0];
		}

		for(int j = 0, k = sel.fst ? 2 : 3; j < k; j++)
		{
			GSVector4 dstq;

			switch(j)
			{
			case 0: dstq = t.xxxx(); break;
			case 1: dstq = t.yyyy(); break;
			case 2: dstq = t.zzzz(); break;
			}

			for(int i = 0; i < 4; i++)
			{
				GSVector4 v = dstq * shift[1 + i];

				if(sel.fst)
				{
					switch(j)
					{
					case 0: m_local.d[i].s = GSVector4::cast(GSVector4i(v)); break;
					case 1: m_local.d[i].t = GSVector4::cast(GSVector4i(v)); break;
					}
				}
				else
				{
					switch(j)
					{
					case 0: m_local.d[i].s = v; break;
					case 1: m_local.d[i].t = v; break;
					case 2: m_local.d[i].q = v; break;
					}
				}
			}
		}
	}

	if(has_c)
	{
		if(sel.iip)
		{
			m_local.d4.c = GSVector4i(dscan.c * shift[0]).xzyw().ps32();

			GSVector4 dr = dscan.c.xxxx();
			GSVector4 db = dscan.c.zzzz();

			for(int i = 0; i < 4; i++)
			{
				GSVector4i r = GSVector4i(dr * shift[1 + i]).ps32();
				GSVector4i b = GSVector4i(db * shift[1 + i]).ps32();

				m_local.d[i].rb = r.upl16(b);
			}

			GSVector4 dg = dscan.c.yyyy();
			GSVector4 da = dscan.c.wwww();

			for(int i = 0; i < 4; i++)
			{
				GSVector4i g = GSVector4i(dg * shift[1 + i]).ps32();
				GSVector4i a = GSVector4i(da * shift[1 + i]).ps32();

				m_local.d[i].ga = g.upl16(a);
			}
		}
		else
		{
			GSVector4i c = GSVector4i(vertices[0].c);

			c = c.upl16(c.zwxy());

			if(sel.tfx == TFX_NONE) c = c.srl16(7);

			m_local.c.rb = c.xxxx();
			m_local.c.ga = c.zzzz();
		}
	}
}

void GSDrawScanline::DrawScanline(int pixels, int left, int top, const GSVertexSW& scan)
{
	GSScanlineSelector sel = m_global.sel;

	GSVector4i test;
	GSVector4 zo;
	GSVector4i f;
	GSVector4 s, t, q;
	GSVector4i uf, vf;
	GSVector4i rbf, gaf;
	GSVector4i cov;

	// Init

	int skip = left & 3;

	left -= skip;

	int steps = pixels + skip - 4;

	const GSVector2i* fza_base = &m_global.fzbr[top];
	const GSVector2i* fza_offset = &m_global.fzbc[left >> 2];

	test = GSDrawScanlineCodeGenerator::m_test[skip] | GSDrawScanlineCodeGenerator::m_test[7 + (steps & (steps >> 31))];

	if(!sel.sprite)
	{
		if(sel.fwrite && sel.fge)
		{
			f = GSVector4i(scan.p).zzzzh().zzzz().add16(m_local.d[skip].f);
		}

		if(sel.zb)
		{
			zo = m_local.d[skip].z;
		}
	}

	if(sel.fb)
	{
		if(sel.edge)
		{
			cov = GSVector4i::cast(scan.t).zzzzh().wwww().srl16(9);
		}

		if(sel.tfx != TFX_NONE)
		{
			if(sel.fst)
			{
				GSVector4i vt(scan.t);

				GSVector4i u = vt.xxxx() + GSVector4i::cast(m_local.d[skip].s);
				GSVector4i v = vt.yyyy(); 
				
				if(!sel.sprite || sel.mmin)
				{
					v += GSVector4i::cast(m_local.d[skip].t);
				}
				else if(sel.ltf)
				{
					vf = v.xxzzlh().srl16(1);
				}

				s = GSVector4::cast(u);
				t = GSVector4::cast(v);
			}
			else
			{
				s = scan.t.xxxx() + m_local.d[skip].s;
				t = scan.t.yyyy() + m_local.d[skip].t;
				q = scan.t.zzzz() + m_local.d[skip].q;
			}
		}

		if(!(sel.tfx == TFX_DECAL && sel.tcc))
		{
			if(sel.iip)
			{
				GSVector4i c(scan.c);

				c = c.upl16(c.zwxy());

				rbf = c.xxxx().add16(m_local.d[skip].rb);
				gaf = c.zzzz().add16(m_local.d[skip].ga);
			}
			else
			{
				rbf = m_local.c.rb;
				gaf = m_local.c.ga;
			}
		}
	}

	while(1)
	{
		do
		{
			int fa = 0, za = 0;
			GSVector4i fd, zs, zd;
			GSVector4i fm, zm;
			GSVector4i rb, ga;

			// TestZ

			if(sel.zb)
			{
				za = fza_base->y + fza_offset->y;

				if(!sel.sprite)
				{
					GSVector4 z = scan.p.zzzz() + zo;

					if(sel.zoverflow)
					{
						zs = (GSVector4i(z * 0.5f) << 1) | (GSVector4i(z) & GSVector4i::x00000001());
					}
					else
					{
						zs = GSVector4i(z);
					}
				}
				else
				{
					zs = m_local.p.z;
				}

				if(sel.ztest)
				{
					zd = GSVector4i::load((uint8*)m_global.vm + za * 2, (uint8*)m_global.vm + za * 2 + 16);

					switch(sel.zpsm)
					{
					case 1: zd = zd.sll32(8).srl32(8); break;
					case 2: zd = zd.sll32(16).srl32(16); break;
					default: break;
					}

					GSVector4i zso = zs;
					GSVector4i zdo = zd;

					if(sel.zoverflow || sel.zpsm == 0)
					{
						zso -= GSVector4i::x80000000();
						zdo -= GSVector4i::x80000000();
					}

					switch(sel.ztst)
					{
					case ZTST_GEQUAL: test |= zso < zdo; break;
					case ZTST_GREATER: test |= zso <= zdo; break;
					}

					if(test.alltrue()) continue;
				}
			}

			// SampleTexture

			if(sel.fb && sel.tfx != TFX_NONE)
			{
				GSVector4i u, v, uv[2];
				GSVector4i lodi, lodf;
				GSVector4i minuv, maxuv;
				GSVector4i addr00, addr01, addr10, addr11;
				GSVector4i c00, c01, c10, c11;

				if(sel.mmin)
				{
					if(!sel.fst)
					{
						GSVector4 qrcp = q.rcp();

						u = GSVector4i(s * qrcp);
						v = GSVector4i(t * qrcp);
					}
					else
					{
						u = GSVector4i::cast(s);
						v = GSVector4i::cast(t);
					}

					if(!sel.lcm)
					{
						GSVector4 tmp = q.log2(3) * m_global.l + m_global.k; // (-log2(Q) * (1 << L) + K) * 0x10000
					
						GSVector4i lod = GSVector4i(tmp.sat(GSVector4::zero(), m_global.mxl), false);

						if(sel.mmin == 1) // round-off mode
						{
							lod += 0x8000;
						}

						lodi = lod.srl32(16);

						if(sel.mmin == 2) // trilinear mode
						{
							lodf = lod.xxzzlh();
						}

						// shift u/v by (int)lod

						GSVector4i aabb = u.upl32(v);
						GSVector4i ccdd = u.uph32(v);
					
						GSVector4i aaxx = aabb.sra32(lodi.x);
						GSVector4i xxbb = aabb.sra32(lodi.y);
						GSVector4i ccxx = ccdd.sra32(lodi.z);
						GSVector4i xxdd = ccdd.sra32(lodi.w);

						GSVector4i acac = aaxx.upl32(ccxx);
						GSVector4i bdbd = xxbb.uph32(xxdd);

						u = acac.upl32(bdbd);
						v = acac.uph32(bdbd);
					
						uv[0] = u;
						uv[1] = v;

						GSVector4i minmax = m_global.t.minmax;

						GSVector4i v0 = minmax.srl16(lodi.x);
						GSVector4i v1 = minmax.srl16(lodi.y);
						GSVector4i v2 = minmax.srl16(lodi.z);
						GSVector4i v3 = minmax.srl16(lodi.w);

						v0 = v0.upl16(v1);
						v2 = v2.upl16(v3);

						minuv = v0.upl32(v2);
						maxuv = v0.uph32(v2);
					}
					else
					{
						lodi = m_global.lod.i;

						u = u.sra32(lodi.x);
						v = v.sra32(lodi.x);

						uv[0] = u;
						uv[1] = v;

						minuv = m_local.temp.uv_minmax[0];
						maxuv = m_local.temp.uv_minmax[1];
					}

					if(sel.ltf)
					{
						u -= 0x8000;
						v -= 0x8000;

						uf = u.xxzzlh().srl16(1);
						vf = v.xxzzlh().srl16(1);
					}

					GSVector4i uv0 = u.sra32(16).ps32(v.sra32(16));
					GSVector4i uv1 = uv0;

					{
						GSVector4i repeat = (uv0 & minuv) | maxuv;
						GSVector4i clamp = uv0.sat_i16(minuv, maxuv);
					
						uv0 = clamp.blend8(repeat, m_global.t.mask);
					}

					if(sel.ltf)
					{
						uv1 = uv1.add16(GSVector4i::x0001());

						GSVector4i repeat = (uv1 & minuv) | maxuv;
						GSVector4i clamp = uv1.sat_i16(minuv, maxuv);
					
						uv1 = clamp.blend8(repeat, m_global.t.mask);
					}

					GSVector4i y0 = uv0.uph16() << (sel.tw + 3);
					GSVector4i x0 = uv0.upl16();

					if(sel.ltf)
					{
						GSVector4i y1 = uv1.uph16() << (sel.tw + 3);
						GSVector4i x1 = uv1.upl16();

						addr00 = y0 + x0;
						addr01 = y0 + x1;
						addr10 = y1 + x0;
						addr11 = y1 + x1;

						if(sel.tlu)
						{
							for(int i = 0; i < 4; i++)
							{
								const uint8* tex = (const uint8*)m_global.tex[lodi.u32[i]];

								c00.u32[i] = m_global.clut[tex[addr00.u32[i]]];
								c01.u32[i] = m_global.clut[tex[addr01.u32[i]]];
								c10.u32[i] = m_global.clut[tex[addr10.u32[i]]];
								c11.u32[i] = m_global.clut[tex[addr11.u32[i]]];
							}
						}
						else
						{
							for(int i = 0; i < 4; i++)
							{
								const uint32* tex = (const uint32*)m_global.tex[lodi.u32[i]];

								c00.u32[i] = tex[addr00.u32[i]];
								c01.u32[i] = tex[addr01.u32[i]];
								c10.u32[i] = tex[addr10.u32[i]];
								c11.u32[i] = tex[addr11.u32[i]];
							}
						}
					
						GSVector4i rb00 = c00.sll16(8).srl16(8);
						GSVector4i ga00 = c00.srl16(8);
						GSVector4i rb01 = c01.sll16(8).srl16(8);
						GSVector4i ga01 = c01.srl16(8);

						rb00 = rb00.lerp16<0>(rb01, uf);
						ga00 = ga00.lerp16<0>(ga01, uf);

						GSVector4i rb10 = c10.sll16(8).srl16(8);
						GSVector4i ga10 = c10.srl16(8);
						GSVector4i rb11 = c11.sll16(8).srl16(8);
						GSVector4i ga11 = c11.srl16(8);

						rb10 = rb10.lerp16<0>(rb11, uf);
						ga10 = ga10.lerp16<0>(ga11, uf);

						rb = rb00.lerp16<0>(rb10, vf);
						ga = ga00.lerp16<0>(ga10, vf);
					}
					else
					{
						addr00 = y0 + x0;

						if(sel.tlu)
						{
							for(int i = 0; i < 4; i++)
							{
								c00.u32[i] = m_global.clut[((const uint8*)m_global.tex[lodi.u32[i]])[addr00.u32[i]]];
							}
						}
						else
						{
							for(int i = 0; i < 4; i++)
							{
								c00.u32[i] = ((const uint32*)m_global.tex[lodi.u32[i]])[addr00.u32[i]];
							}
						}

						rb = c00.sll16(8).srl16(8);
						ga = c00.srl16(8);
					}

					if(sel.mmin != 1) // !round-off mode
					{
						GSVector4i rb2, ga2;

						lodi += GSVector4i::x00000001();

						u = uv[0].sra32(1);
						v = uv[1].sra32(1);

						minuv = minuv.srl16(1);
						maxuv = maxuv.srl16(1);

						if(sel.ltf)
						{
							u -= 0x8000;
							v -= 0x8000;

							uf = u.xxzzlh().srl16(1);
							vf = v.xxzzlh().srl16(1);
						}

						GSVector4i uv0 = u.sra32(16).ps32(v.sra32(16));
						GSVector4i uv1 = uv0;

						{
							GSVector4i repeat = (uv0 & minuv) | maxuv;
							GSVector4i clamp = uv0.sat_i16(minuv, maxuv);
					
							uv0 = clamp.blend8(repeat, m_global.t.mask);
						}

						if(sel.ltf)
						{
							uv1 = uv1.add16(GSVector4i::x0001());

							GSVector4i repeat = (uv1 & minuv) | maxuv;
							GSVector4i clamp = uv1.sat_i16(minuv, maxuv);
					
							uv1 = clamp.blend8(repeat, m_global.t.mask);
						}

						GSVector4i y0 = uv0.uph16() << (sel.tw + 3);
						GSVector4i x0 = uv0.upl16();

						if(sel.ltf)
						{
							GSVector4i y1 = uv1.uph16() << (sel.tw + 3);
							GSVector4i x1 = uv1.upl16();

							addr00 = y0 + x0;
							addr01 = y0 + x1;
							addr10 = y1 + x0;
							addr11 = y1 + x1;

							if(sel.tlu)
							{
								for(int i = 0; i < 4; i++)
								{
									const uint8* tex = (const uint8*)m_global.tex[lodi.u32[i]];

									c00.u32[i] = m_global.clut[tex[addr00.u32[i]]];
									c01.u32[i] = m_global.clut[tex[addr01.u32[i]]];
									c10.u32[i] = m_global.clut[tex[addr10.u32[i]]];
									c11.u32[i] = m_global.clut[tex[addr11.u32[i]]];
								}
							}
							else
							{
								for(int i = 0; i < 4; i++)
								{
									const uint32* tex = (const uint32*)m_global.tex[lodi.u32[i]];

									c00.u32[i] = tex[addr00.u32[i]];
									c01.u32[i] = tex[addr01.u32[i]];
									c10.u32[i] = tex[addr10.u32[i]];
									c11.u32[i] = tex[addr11.u32[i]];
								}
							}
					
							GSVector4i rb00 = c00.sll16(8).srl16(8);
							GSVector4i ga00 = c00.srl16(8);
							GSVector4i rb01 = c01.sll16(8).srl16(8);
							GSVector4i ga01 = c01.srl16(8);

							rb00 = rb00.lerp16<0>(rb01, uf);
							ga00 = ga00.lerp16<0>(ga01, uf);

							GSVector4i rb10 = c10.sll16(8).srl16(8);
							GSVector4i ga10 = c10.srl16(8);
							GSVector4i rb11 = c11.sll16(8).srl16(8);
							GSVector4i ga11 = c11.srl16(8);

							rb10 = rb10.lerp16<0>(rb11, uf);
							ga10 = ga10.lerp16<0>(ga11, uf);

							rb2 = rb00.lerp16<0>(rb10, vf);
							ga2 = ga00.lerp16<0>(ga10, vf);
						}
						else
						{
							addr00 = y0 + x0;

							if(sel.tlu)
							{
								for(int i = 0; i < 4; i++)
								{
									c00.u32[i] = m_global.clut[((const uint8*)m_global.tex[lodi.u32[i]])[addr00.u32[i]]];
								}
							}
							else
							{
								for(int i = 0; i < 4; i++)
								{
									c00.u32[i] = ((const uint32*)m_global.tex[lodi.u32[i]])[addr00.u32[i]];
								}
							}

							rb2 = c00.sll16(8).srl16(8);
							ga2 = c00.srl16(8);
						}

						if(sel.lcm) lodf = m_global.lod.f;

						lodf = lodf.srl16(1);

						rb = rb.lerp16<0>(rb2, lodf);
						ga = ga.lerp16<0>(ga2, lodf);
					}				
				}
				else
				{
					if(!sel.fst)
					{
						GSVector4 qrcp = q.rcp();

						u = GSVector4i(s * qrcp);
						v = GSVector4i(t * qrcp);
					
						if(sel.ltf)
						{
							u -= 0x8000;
							v -= 0x8000;
						}
					}
					else
					{
						u = GSVector4i::cast(s);
						v = GSVector4i::cast(t);
					}

					if(sel.ltf)
					{
						uf = u.xxzzlh().srl16(1);
					
						if(!sel.sprite)
						{
							vf = v.xxzzlh().srl16(1);
						}
					}

					GSVector4i uv0 = u.sra32(16).ps32(v.sra32(16));
					GSVector4i uv1 = uv0;

					{
						GSVector4i repeat = (uv0 & m_global.t.min) | m_global.t.max;
						GSVector4i clamp = uv0.sat_i16(m_global.t.min, m_global.t.max);
					
						uv0 = clamp.blend8(repeat, m_global.t.mask);
					}

					if(sel.ltf)
					{
						uv1 = uv1.add16(GSVector4i::x0001());

						GSVector4i repeat = (uv1 & m_global.t.min) | m_global.t.max;
						GSVector4i clamp = uv1.sat_i16(m_global.t.min, m_global.t.max);
					
						uv1 = clamp.blend8(repeat, m_global.t.mask);
					}

					GSVector4i y0 = uv0.uph16() << (sel.tw + 3);
					GSVector4i x0 = uv0.upl16();

					if(sel.ltf)
					{
						GSVector4i y1 = uv1.uph16() << (sel.tw + 3);
						GSVector4i x1 = uv1.upl16();

						addr00 = y0 + x0;
						addr01 = y0 + x1;
						addr10 = y1 + x0;
						addr11 = y1 + x1;

						if(sel.tlu)
						{
							const uint8* tex = (const uint8*)m_global.tex[0];

							c00 = addr00.gather32_32(tex, m_global.clut);
							c01 = addr01.gather32_32(tex, m_global.clut);
							c10 = addr10.gather32_32(tex, m_global.clut);
							c11 = addr11.gather32_32(tex, m_global.clut);
						}
						else
						{
							const uint32* tex = (const uint32*)m_global.tex[0];

							c00 = addr00.gather32_32(tex);
							c01 = addr01.gather32_32(tex);
							c10 = addr10.gather32_32(tex);
							c11 = addr11.gather32_32(tex);
						}
					
						GSVector4i rb00 = c00.sll16(8).srl16(8);
						GSVector4i ga00 = c00.srl16(8);
						GSVector4i rb01 = c01.sll16(8).srl16(8);
						GSVector4i ga01 = c01.srl16(8);

						rb00 = rb00.lerp16<0>(rb01, uf);
						ga00 = ga00.lerp16<0>(ga01, uf);

						GSVector4i rb10 = c10.sll16(8).srl16(8);
						GSVector4i ga10 = c10.srl16(8);
						GSVector4i rb11 = c11.sll16(8).srl16(8);
						GSVector4i ga11 = c11.srl16(8);

						rb10 = rb10.lerp16<0>(rb11, uf);
						ga10 = ga10.lerp16<0>(ga11, uf);

						rb = rb00.lerp16<0>(rb10, vf);
						ga = ga00.lerp16<0>(ga10, vf);
					}
					else
					{
						addr00 = y0 + x0;

						if(sel.tlu)
						{
							c00 = addr00.gather32_32((const uint8*)m_global.tex[0], m_global.clut);
						}
						else
						{
							c00 = addr00.gather32_32((const uint32*)m_global.tex[0]);
						}

						rb = c00.sll16(8).srl16(8);
						ga = c00.srl16(8);
					}
				}
			}

			// AlphaTFX

			if(sel.fb)
			{
				switch(sel.tfx)
				{
				case TFX_MODULATE:
					ga = ga.modulate16<1>(gaf).clamp8();
					if(!sel.tcc) ga = ga.mix16(gaf.srl16(7));
					break;
				case TFX_DECAL:
					if(!sel.tcc) ga = ga.mix16(gaf.srl16(7));
					break;
				case TFX_HIGHLIGHT:
					ga = ga.mix16(!sel.tcc ? gaf.srl16(7) : ga.addus8(gaf.srl16(7)));
					break;
				case TFX_HIGHLIGHT2:
					if(!sel.tcc) ga = ga.mix16(gaf.srl16(7));
					break;
				case TFX_NONE:
					ga = sel.iip ? gaf.srl16(7) : gaf;
					break;
				}

				if(sel.aa1)
				{
					GSVector4i x00800080(0x00800080);

					GSVector4i a = sel.edge ? cov : x00800080;

					if(!sel.abe)
					{
						ga = ga.mix16(a);
					}
					else
					{
						ga = ga.blend8(a, ga.eq16(x00800080).srl32(16).sll32(16));
					}
				}
			}

			// ReadMask

			if(sel.fwrite)
			{
				fm = m_global.fm;
			}

			if(sel.zwrite)
			{
				zm = m_global.zm;
			}

			// TestAlpha

			if(!TestAlpha(test, fm, zm, ga)) continue;

			// ColorTFX

			if(sel.fwrite)
			{
				GSVector4i af;

				switch(sel.tfx)
				{
				case TFX_MODULATE:
					rb = rb.modulate16<1>(rbf).clamp8();
					break;
				case TFX_DECAL:
					break;
				case TFX_HIGHLIGHT:
				case TFX_HIGHLIGHT2:
					af = gaf.yywwlh().srl16(7);
					rb = rb.modulate16<1>(rbf).add16(af).clamp8();
					ga = ga.modulate16<1>(gaf).add16(af).clamp8().mix16(ga);
					break;
				case TFX_NONE:
					rb = sel.iip ? rbf.srl16(7) : rbf;
					break;
				}
			}

			// Fog

			if(sel.fwrite && sel.fge)
			{
				GSVector4i fog = !sel.sprite ? f : m_local.p.f;

				rb = m_global.frb.lerp16<0>(rb, fog);
				ga = m_global.fga.lerp16<0>(ga, fog).mix16(ga);
			}

			// ReadFrame

			if(sel.fb)
			{
				fa = fza_base->x + fza_offset->x;

				if(sel.rfb)
				{
					fd = GSVector4i::load((uint8*)m_global.vm + fa * 2, (uint8*)m_global.vm + fa * 2 + 16);
				}
			}

			// TestDestAlpha

			if(sel.date && (sel.fpsm == 0 || sel.fpsm == 2))
			{
				if(sel.datm)
				{
					if(sel.fpsm == 2)
					{
						test |= fd.srl32(15) == GSVector4i::zero();
					}
					else
					{
						test |= (~fd).sra32(31);
					}
				}
				else
				{
					if(sel.fpsm == 2)
					{
						test |= fd.sll32(16).sra32(31);
					}
					else
					{
						test |= fd.sra32(31);
					}
				}

				if(test.alltrue()) continue;
			}

			// WriteMask

			int fzm = 0;

			if(sel.fwrite)
			{
				fm |= test;
			}

			if(sel.zwrite)
			{
				zm |= test;
			}

			if(sel.fwrite && sel.zwrite)
			{
				fzm = ~(fm == GSVector4i::xffffffff()).ps32(zm == GSVector4i::xffffffff()).mask();
			}
			else if(sel.fwrite)
			{
				fzm = ~(fm == GSVector4i::xffffffff()).ps32().mask();
			}
			else if(sel.zwrite)
			{
				fzm = ~(zm == GSVector4i::xffffffff()).ps32().mask();
			}

			// WriteZBuf

			if(sel.zwrite)
			{
				if(sel.ztest && sel.zpsm < 2)
				{
					zs = zs.blend8(zd, zm);

					if(fzm & 0x0f00) GSVector4i::storel((uint8*)m_global.vm + za * 2, zs);
					if(fzm & 0xf000) GSVector4i::storeh((uint8*)m_global.vm + za * 2 + 16, zs);
				}
				else
				{
					if(fzm & 0x0300) WritePixel(zs, za, 0, sel.zpsm);
					if(fzm & 0x0c00) WritePixel(zs, za, 1, sel.zpsm);
					if(fzm & 0x3000) WritePixel(zs, za, 2, sel.zpsm);
					if(fzm & 0xc000) WritePixel(zs, za, 3, sel.zpsm);
				}
			}

			// AlphaBlend

			if(sel.fwrite && (sel.abe || sel.aa1))
			{
				GSVector4i rbs = rb, gas = ga, rbd, gad, a, mask;

				if(sel.aba != sel.abb && (sel.aba == 1 || sel.abb == 1 || sel.abc == 1) || sel.abd == 1)
				{
					switch(sel.fpsm)
					{
					case 0:
					case 1:
						rbd = fd.sll16(8).srl16(8);
						gad = fd.srl16(8);
						break;
					case 2:
						rbd = ((fd & 0x7c00) << 9) | ((fd & 0x001f) << 3);
						gad = ((fd & 0x8000) << 8) | ((fd & 0x03e0) >> 2);
						break;
					}
				}

				if(sel.aba != sel.abb)
				{
					switch(sel.aba)
					{
					case 0: break;
					case 1: rb = rbd; break;
					case 2: rb = GSVector4i::zero(); break;
					}

					switch(sel.abb)
					{
					case 0: rb = rb.sub16(rbs); break;
					case 1: rb = rb.sub16(rbd); break;
					case 2: break;
					}

					if(!(sel.fpsm == 1 && sel.abc == 1))
					{
						switch(sel.abc)
						{
						case 0: a = gas.yywwlh().sll16(7); break;
						case 1: a = gad.yywwlh().sll16(7); break;
						case 2: a = m_global.afix; break;
						}

						rb = rb.modulate16<1>(a);
					}

					switch(sel.abd)
					{
					case 0: rb = rb.add16(rbs); break;
					case 1: rb = rb.add16(rbd); break;
					case 2: break;
					}
				}
				else
				{
					switch(sel.abd)
					{
					case 0: break;
					case 1: rb = rbd; break;
					case 2: rb = GSVector4i::zero(); break;
					}
				}

				if(sel.pabe)
				{
					mask = (gas << 8).sra32(31);
					
					rb = rbs.blend8(rb, mask);
				}
				
				if(sel.aba != sel.abb)
				{
					switch(sel.aba)
					{
					case 0: break;
					case 1: ga = gad; break;
					case 2: ga = GSVector4i::zero(); break;
					}

					switch(sel.abb)
					{
					case 0: ga = ga.sub16(gas); break;
					case 1: ga = ga.sub16(gad); break;
					case 2: break;
					}

					if(!(sel.fpsm == 1 && sel.abc == 1))
					{
						ga = ga.modulate16<1>(a);
					}

					switch(sel.abd)
					{
					case 0: ga = ga.add16(gas); break;
					case 1: ga = ga.add16(gad); break;
					case 2: break;
					}
				}

				if(sel.pabe)
				{
					ga = gas.blend8(ga, mask >> 16);
				}
				else
				{
					if(sel.fpsm != 1)
					{
						ga = ga.mix16(gas);
					}
				}
			}

			// WriteFrame

			if(sel.fwrite)
			{
				if(sel.colclamp == 0)
				{
					rb &= GSVector4i::x00ff();
					ga &= GSVector4i::x00ff();
				}

				if(sel.fpsm == 2 && sel.dthe)
				{
					int y = (top & 3) << 1;

					rb = rb.add16(m_global.dimx[0 + y]);
					ga = ga.add16(m_global.dimx[1 + y]);
				}

				GSVector4i fs = rb.upl16(ga).pu16(rb.uph16(ga));

				if(sel.fba && sel.fpsm != 1)
				{
					fs |= GSVector4i::x80000000();
				}

				if(sel.fpsm == 2)
				{
					GSVector4i rb = fs & 0x00f800f8;
					GSVector4i ga = fs & 0x8000f800;

					fs = (ga >> 16) | (rb >> 9) | (ga >> 6) | (rb >> 3);
				}

				if(sel.rfb)
				{
					fs = fs.blend(fd, fm);
				}

				if(sel.rfb && sel.fpsm < 2)
				{
					if(fzm & 0x000f) GSVector4i::storel((uint8*)m_global.vm + fa * 2, fs);
					if(fzm & 0x00f0) GSVector4i::storeh((uint8*)m_global.vm + fa * 2 + 16, fs);
				}
				else
				{
					if(fzm & 0x0003) WritePixel(fs, fa, 0, sel.fpsm);
					if(fzm & 0x000c) WritePixel(fs, fa, 1, sel.fpsm);
					if(fzm & 0x0030) WritePixel(fs, fa, 2, sel.fpsm);
					if(fzm & 0x00c0) WritePixel(fs, fa, 3, sel.fpsm);
				}
			}
		}
		while(0);

		if(sel.edge) break;

		if(steps <= 0) break;

		// Step
		
		steps -= 4;

		fza_offset++;

		if(!sel.sprite)
		{
			if(sel.zb)
			{
				zo += m_local.d4.z;
			}

			if(sel.fwrite && sel.fge)
			{
				f = f.add16(m_local.d4.f);
			}
		}

		if(sel.fb)
		{
			if(sel.tfx != TFX_NONE)
			{
				if(sel.fst)
				{
					GSVector4i stq = GSVector4i::cast(m_local.d4.stq);

					s = GSVector4::cast(GSVector4i::cast(s) + stq.xxxx());
					
					if(!sel.sprite || sel.mmin)
					{
						t = GSVector4::cast(GSVector4i::cast(t) + stq.yyyy());
					}
				}
				else
				{
					GSVector4 stq = m_local.d4.stq;

					s += stq.xxxx();
					t += stq.yyyy();
					q += stq.zzzz();
				}
			}
		}

		if(!(sel.tfx == TFX_DECAL && sel.tcc))
		{
			if(sel.iip)
			{
				GSVector4i c = m_local.d4.c;

				rbf = rbf.add16(c.xxxx()).max_i16(GSVector4i::zero());
				gaf = gaf.add16(c.yyyy()).max_i16(GSVector4i::zero());
			}
		}

		test = GSDrawScanlineCodeGenerator::m_test[7 + (steps & (steps >> 31))];
	}
}

void GSDrawScanline::DrawEdge(int pixels, int left, int top, const GSVertexSW& scan)
{
	uint32 zwrite = m_global.sel.zwrite;
	uint32 edge = m_global.sel.edge;

	m_global.sel.zwrite = 0;
	m_global.sel.edge = 1;

	DrawScanline(pixels, left, top, scan);

	m_global.sel.zwrite = zwrite;
	m_global.sel.edge = edge;
}

bool GSDrawScanline::TestAlpha(GSVector4i& test, GSVector4i& fm, GSVector4i& zm, const GSVector4i& ga)
{
	GSScanlineSelector sel = m_global.sel;

	switch(sel.afail)
	{
	case AFAIL_FB_ONLY:
		if(!sel.zwrite) return true;
		break;

	case AFAIL_ZB_ONLY:
		if(!sel.fwrite) return true;
		break;

	case AFAIL_RGB_ONLY:
		if(!sel.zwrite && sel.fpsm == 1) return true;
		break;
	}

	GSVector4i t;

	switch(sel.atst)
	{
	case ATST_NEVER:
		t = GSVector4i::xffffffff();
		break;

	case ATST_ALWAYS:
		return true;

	case ATST_LESS:
	case ATST_LEQUAL:
		t = (ga >> 16) > m_global.aref;
		break;

	case ATST_EQUAL:
		t = (ga >> 16) != m_global.aref;
		break;

	case ATST_GEQUAL:
	case ATST_GREATER:
		t = (ga >> 16) < m_global.aref;
		break;

	case ATST_NOTEQUAL:
		t = (ga >> 16) == m_global.aref;
		break;

	default:
		__assume(0);
	}

	switch(sel.afail)
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
		zm |= t;
		fm |= t & GSVector4i::xff000000();
		break;

	default:
		__assume(0);
	}

	return true;
}

static const int s_offsets[4] = {0, 2, 8, 10};

void GSDrawScanline::WritePixel(const GSVector4i& src, int addr, int i, uint32 psm)
{
	uint8* dst = (uint8*)m_global.vm + addr * 2 + s_offsets[i] * 2;

	switch(psm)
	{
	case 0:
		*(uint32*)dst = src.u32[i];
		break;
	case 1:
		*(uint32*)dst = (src.u32[i] & 0xffffff) | (*(uint32*)dst & 0xff000000);
		break;
	case 2:
		*(uint16*)dst = src.u16[i * 2];
		break;
	}
}

#endif

void GSDrawScanline::DrawRect(const GSVector4i& r, const GSVertexSW& v)
{
	ASSERT(r.y >= 0);
	ASSERT(r.w >= 0);

	// FIXME: sometimes the frame and z buffer may overlap, the outcome is undefined

	uint32 m;

	m = m_global.zm.u32[0];

	if(m != 0xffffffff)
	{
		const int* zbr = m_global.zbr;
		const int* zbc = m_global.zbc;

		uint32 z = v.t.u32[3]; // (uint32)v.p.z;

		if(m_global.sel.zpsm != 2)
		{
			if(m == 0)
			{
				DrawRectT<uint32, false>(zbr, zbc, r, z, m);
			}
			else
			{
				DrawRectT<uint32, true>(zbr, zbc, r, z, m);
			}
		}
		else
		{
			if(m == 0)
			{
				DrawRectT<uint16, false>(zbr, zbc, r, z, m);
			}
			else
			{
				DrawRectT<uint16, true>(zbr, zbc, r, z, m);
			}
		}
	}

	m = m_global.fm.u32[0];

	if(m != 0xffffffff)
	{
		const int* fbr = m_global.fbr;
		const int* fbc = m_global.fbc;

		uint32 c = (GSVector4i(v.c) >> 7).rgba32();

		if(m_global.sel.fba)
		{
			c |= 0x80000000;
		}

		if(m_global.sel.fpsm != 2)
		{
			if(m == 0)
			{
				DrawRectT<uint32, false>(fbr, fbc, r, c, m);
			}
			else
			{
				DrawRectT<uint32, true>(fbr, fbc, r, c, m);
			}
		}
		else
		{
			c = ((c & 0xf8) >> 3) | ((c & 0xf800) >> 6) | ((c & 0xf80000) >> 9) | ((c & 0x80000000) >> 16);

			if(m == 0)
			{
				DrawRectT<uint16, false>(fbr, fbc, r, c, m);
			}
			else
			{
				DrawRectT<uint16, true>(fbr, fbc, r, c, m);
			}
		}
	}
}

template<class T, bool masked>
void GSDrawScanline::DrawRectT(const int* RESTRICT row, const int* RESTRICT col, const GSVector4i& r, uint32 c, uint32 m)
{
	if(m == 0xffffffff) return;

	GSVector4i color((int)c);
	GSVector4i mask((int)m);

	if(sizeof(T) == sizeof(uint16))
	{
		color = color.xxzzlh();
		mask = mask.xxzzlh();
	}

	color = color.andnot(mask);

	GSVector4i br = r.ralign<Align_Inside>(GSVector2i(8 * 4 / sizeof(T), 8));

	if(!br.rempty())
	{
		FillRect<T, masked>(row, col, GSVector4i(r.x, r.y, r.z, br.y), c, m);
		FillRect<T, masked>(row, col, GSVector4i(r.x, br.w, r.z, r.w), c, m);

		if(r.x < br.x || br.z < r.z)
		{
			FillRect<T, masked>(row, col, GSVector4i(r.x, br.y, br.x, br.w), c, m);
			FillRect<T, masked>(row, col, GSVector4i(br.z, br.y, r.z, br.w), c, m);
		}

		FillBlock<T, masked>(row, col, br, color, mask);
	}
	else
	{
		FillRect<T, masked>(row, col, r, c, m);
	}
}

template<class T, bool masked>
void GSDrawScanline::FillRect(const int* RESTRICT row, const int* RESTRICT col, const GSVector4i& r, uint32 c, uint32 m)
{
	if(r.x >= r.z) return;

	T* vm = (T*)m_global.vm;

	for(int y = r.y; y < r.w; y++)
	{
		T* RESTRICT d = &vm[row[y]];

		for(int x = r.x; x < r.z; x++)
		{
			d[col[x]] = (T)(!masked ? c : (c | (d[col[x]] & m)));
		}
	}
}

template<class T, bool masked>
void GSDrawScanline::FillBlock(const int* RESTRICT row, const int* RESTRICT col, const GSVector4i& r, const GSVector4i& c, const GSVector4i& m)
{
	if(r.x >= r.z) return;

	T* vm = (T*)m_global.vm;

	for(int y = r.y; y < r.w; y += 8)
	{
		T* RESTRICT d = &vm[row[y]];

		for(int x = r.x; x < r.z; x += 8 * 4 / sizeof(T))
		{
			GSVector4i* RESTRICT p = (GSVector4i*)&d[col[x]];

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
