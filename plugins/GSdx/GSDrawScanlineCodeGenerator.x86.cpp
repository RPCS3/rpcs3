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
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "GSDrawScanlineCodeGenerator.h"
#include "GSVertexSW.h"

#if _M_SSE < 0x500 && !(defined(_M_AMD64) || defined(_WIN64))

static const int _args = 16;
static const int _top = _args + 4;
static const int _v = _args + 8;

void GSDrawScanlineCodeGenerator::Generate()
{
	push(ebx);
	push(esi);
	push(edi);
	push(ebp);

	Init();

	if(!m_sel.edge)
	{
		align(16);
	}

L("loop");

	// ecx = steps
	// esi = fzbr
	// edi = fzbc
	// xmm0 = z/zi
	// xmm2 = s/u (tme)
	// xmm3 = t/v (tme)
	// xmm4 = q (tme)
	// xmm5 = rb (!tme)
	// xmm6 = ga (!tme)
	// xmm7 = test

	bool tme = m_sel.tfx != TFX_NONE;

	TestZ(tme ? xmm5 : xmm2, tme ? xmm6 : xmm3);

	// ecx = steps
	// esi = fzbr
	// edi = fzbc
	// ebp = za
	// - xmm0
	// xmm2 = s/u (tme)
	// xmm3 = t/v (tme)
	// xmm4 = q (tme)
	// xmm5 = rb (!tme)
	// xmm6 = ga (!tme)
	// xmm7 = test

	if(m_sel.mmin)
	{
		SampleTextureLOD();
	}
	else
	{
		SampleTexture();
	}

	// ecx = steps
	// esi = fzbr
	// edi = fzbc
	// ebp = za
	// - xmm2
	// - xmm3
	// - xmm4
	// xmm5 = rb
	// xmm6 = ga
	// xmm7 = test

	AlphaTFX();

	// ecx = steps
	// esi = fzbr
	// edi = fzbc
	// ebp = za
	// xmm2 = gaf (TFX_HIGHLIGHT || TFX_HIGHLIGHT2 && !tcc)
	// xmm5 = rb
	// xmm6 = ga
	// xmm7 = test

	ReadMask();

	// ecx = steps
	// esi = fzbr
	// edi = fzbc
	// ebp = za
	// xmm2 = gaf (TFX_HIGHLIGHT || TFX_HIGHLIGHT2 && !tcc)
	// xmm3 = fm
	// xmm4 = zm
	// xmm5 = rb
	// xmm6 = ga
	// xmm7 = test

	TestAlpha();

	// ecx = steps
	// esi = fzbr
	// edi = fzbc
	// ebp = za
	// xmm2 = gaf (TFX_HIGHLIGHT || TFX_HIGHLIGHT2 && !tcc)
	// xmm3 = fm
	// xmm4 = zm
	// xmm5 = rb
	// xmm6 = ga
	// xmm7 = test

	ColorTFX();

	// ecx = steps
	// esi = fzbr
	// edi = fzbc
	// ebp = za
	// xmm3 = fm
	// xmm4 = zm
	// xmm5 = rb
	// xmm6 = ga
	// xmm7 = test

	Fog();

	// ecx = steps
	// esi = fzbr
	// edi = fzbc
	// ebp = za
	// xmm3 = fm
	// xmm4 = zm
	// xmm5 = rb
	// xmm6 = ga
	// xmm7 = test

	ReadFrame();

	// ecx = steps
	// esi = fzbr
	// edi = fzbc
	// ebp = za
	// xmm2 = fd
	// xmm3 = fm
	// xmm4 = zm
	// xmm5 = rb
	// xmm6 = ga
	// xmm7 = test

	TestDestAlpha();

	// ecx = steps
	// esi = fzbr
	// edi = fzbc
	// ebp = za
	// xmm2 = fd
	// xmm3 = fm
	// xmm4 = zm
	// xmm5 = rb
	// xmm6 = ga
	// xmm7 = test

	WriteMask();

	// ebx = fa
	// ecx = steps
	// edx = fzm
	// esi = fzbr
	// edi = fzbc
	// ebp = za
	// xmm2 = fd
	// xmm3 = fm
	// xmm4 = zm
	// xmm5 = rb
	// xmm6 = ga

	WriteZBuf();

	// ebx = fa
	// ecx = steps
	// edx = fzm
	// esi = fzbr
	// edi = fzbc
	// - ebp
	// xmm2 = fd
	// xmm3 = fm
	// - xmm4
	// xmm5 = rb
	// xmm6 = ga

	AlphaBlend();

	// ebx = fa
	// ecx = steps
	// edx = fzm
	// esi = fzbr
	// edi = fzbc
	// xmm2 = fd
	// xmm3 = fm
	// xmm5 = rb
	// xmm6 = ga

	WriteFrame();

L("step");

	// if(steps <= 0) break;

	if(!m_sel.edge)
	{
		test(ecx, ecx);

		jle("exit", T_NEAR);

		Step();

		jmp("loop", T_NEAR);
	}

L("exit");

	// vzeroupper();

	pop(ebp);
	pop(edi);
	pop(esi);
	pop(ebx);

	ret(8);
}

void GSDrawScanlineCodeGenerator::Init()
{
	if(!m_sel.notest)
	{
		// int skip = left & 3;

		mov(ebx, edx);
		and(edx, 3);

		// int steps = pixels + skip - 4;

		lea(ecx, ptr[ecx + edx - 4]);

		// left -= skip;

		sub(ebx, edx);

		// GSVector4i test = m_test[skip] | m_test[7 + (steps & (steps >> 31))];

		shl(edx, 4);

		movdqa(xmm7, ptr[edx + (size_t)&m_test[0]]);

		mov(eax, ecx);
		sar(eax, 31);
		and(eax, ecx);
		shl(eax, 4);

		por(xmm7, ptr[eax + (size_t)&m_test[7]]);
	}
	else
	{
		mov(ebx, edx); // left
		xor(edx, edx); // skip
		lea(ecx, ptr[ecx - 4]); // steps
	}

	// GSVector2i* fza_base = &m_local.gd->fzbr[top];

	mov(esi, ptr[esp + _top]);
	lea(esi, ptr[esi * 8]);
	add(esi, ptr[&m_local.gd->fzbr]);

	// GSVector2i* fza_offset = &m_local.gd->fzbc[left >> 2];

	lea(edi, ptr[ebx * 2]);
	add(edi, ptr[&m_local.gd->fzbc]);

	if(m_sel.prim != GS_SPRITE_CLASS && (m_sel.fwrite && m_sel.fge || m_sel.zb) || m_sel.fb && (m_sel.edge || m_sel.tfx != TFX_NONE || m_sel.iip))
	{
		// edx = &m_local.d[skip]

		lea(edx, ptr[edx * 8 + (size_t)m_local.d]);

		// ebx = &v

		mov(ebx, ptr[esp + _v]);
	}

	if(m_sel.prim != GS_SPRITE_CLASS)
	{
		if(m_sel.fwrite && m_sel.fge || m_sel.zb)
		{
			movaps(xmm0, ptr[ebx + offsetof(GSVertexSW, p)]); // v.p

			if(m_sel.fwrite && m_sel.fge)
			{
				// f = GSVector4i(vp).zzzzh().zzzz().add16(m_local.d[skip].f);

				cvttps2dq(xmm1, xmm0);
				pshufhw(xmm1, xmm1, _MM_SHUFFLE(2, 2, 2, 2));
				pshufd(xmm1, xmm1, _MM_SHUFFLE(2, 2, 2, 2));
				paddw(xmm1, ptr[edx + offsetof(GSScanlineLocalData::skip, f)]);

				movdqa(ptr[&m_local.temp.f], xmm1);
			}

			if(m_sel.zb)
			{
				// z = vp.zzzz() + m_local.d[skip].z;

				shufps(xmm0, xmm0, _MM_SHUFFLE(2, 2, 2, 2));
				movaps(ptr[&m_local.temp.z], xmm0);
				movaps(xmm2, ptr[edx + offsetof(GSScanlineLocalData::skip, z)]);
				movaps(ptr[&m_local.temp.zo], xmm2);
				addps(xmm0, xmm2);
			}
		}
	}
	else
	{
		if(m_sel.ztest)
		{
			movdqa(xmm0, ptr[&m_local.p.z]);
		}
	}

	if(m_sel.fb)
	{
		if(m_sel.edge || m_sel.tfx != TFX_NONE)
		{
			movaps(xmm4, ptr[ebx + offsetof(GSVertexSW, t)]); // v.t
		}

		if(m_sel.edge)
		{
			// m_local.temp.cov = GSVector4i::cast(v.t).zzzzh().wwww().srl16(9);

			pshufhw(xmm3, xmm4, _MM_SHUFFLE(2, 2, 2, 2));
			pshufd(xmm3, xmm3, _MM_SHUFFLE(3, 3, 3, 3));
			psrlw(xmm3, 9);

			movdqa(ptr[&m_local.temp.cov], xmm3);
		}

		if(m_sel.tfx != TFX_NONE)
		{
			if(m_sel.fst)
			{
				// GSVector4i vti(vt);

				cvttps2dq(xmm6, xmm4);

				// s = vti.xxxx() + m_local.d[skip].s;
				// t = vti.yyyy(); if(!sprite) t += m_local.d[skip].t;

				pshufd(xmm2, xmm6, _MM_SHUFFLE(0, 0, 0, 0));
				pshufd(xmm3, xmm6, _MM_SHUFFLE(1, 1, 1, 1));

				paddd(xmm2, ptr[edx + offsetof(GSScanlineLocalData::skip, s)]);

				if(m_sel.prim != GS_SPRITE_CLASS || m_sel.mmin)
				{
					paddd(xmm3, ptr[edx + offsetof(GSScanlineLocalData::skip, t)]);
				}
				else
				{
					if(m_sel.ltf)
					{
						pshuflw(xmm6, xmm3, _MM_SHUFFLE(2, 2, 0, 0));
						pshufhw(xmm6, xmm6, _MM_SHUFFLE(2, 2, 0, 0));
						psrlw(xmm6, 12);
						movdqa(ptr[&m_local.temp.vf], xmm6);
					}
				}

				movdqa(ptr[&m_local.temp.s], xmm2);
				movdqa(ptr[&m_local.temp.t], xmm3);
			}
			else
			{
				// s = vt.xxxx() + m_local.d[skip].s;
				// t = vt.yyyy() + m_local.d[skip].t;
				// q = vt.zzzz() + m_local.d[skip].q;

				movaps(xmm2, xmm4);
				movaps(xmm3, xmm4);

				shufps(xmm2, xmm2, _MM_SHUFFLE(0, 0, 0, 0));
				shufps(xmm3, xmm3, _MM_SHUFFLE(1, 1, 1, 1));
				shufps(xmm4, xmm4, _MM_SHUFFLE(2, 2, 2, 2));

				addps(xmm2, ptr[edx + offsetof(GSScanlineLocalData::skip, s)]);
				addps(xmm3, ptr[edx + offsetof(GSScanlineLocalData::skip, t)]);
				addps(xmm4, ptr[edx + offsetof(GSScanlineLocalData::skip, q)]);

				movaps(ptr[&m_local.temp.s], xmm2);
				movaps(ptr[&m_local.temp.t], xmm3);
				movaps(ptr[&m_local.temp.q], xmm4);
			}
		}

		if(!(m_sel.tfx == TFX_DECAL && m_sel.tcc))
		{
			if(m_sel.iip)
			{
				// GSVector4i vc = GSVector4i(v.c);

				cvttps2dq(xmm6, ptr[ebx + offsetof(GSVertexSW, c)]); // v.c

				// vc = vc.upl16(vc.zwxy());

				pshufd(xmm5, xmm6, _MM_SHUFFLE(1, 0, 3, 2));
				punpcklwd(xmm6, xmm5);

				// rb = vc.xxxx().add16(m_local.d[skip].rb);
				// ga = vc.zzzz().add16(m_local.d[skip].ga);

				pshufd(xmm5, xmm6, _MM_SHUFFLE(0, 0, 0, 0));
				pshufd(xmm6, xmm6, _MM_SHUFFLE(2, 2, 2, 2));

				paddw(xmm5, ptr[edx + offsetof(GSScanlineLocalData::skip, rb)]);
				paddw(xmm6, ptr[edx + offsetof(GSScanlineLocalData::skip, ga)]);

				movdqa(ptr[&m_local.temp.rb], xmm5);
				movdqa(ptr[&m_local.temp.ga], xmm6);
			}
			else
			{
				if(m_sel.tfx == TFX_NONE)
				{
					movdqa(xmm5, ptr[&m_local.c.rb]);
					movdqa(xmm6, ptr[&m_local.c.ga]);
				}
			}
		}
	}
}

void GSDrawScanlineCodeGenerator::Step()
{
	// steps -= 4;

	sub(ecx, 4);

	// fza_offset++;

	add(edi, 8);

	if(m_sel.prim != GS_SPRITE_CLASS)
	{
		// z += m_local.d4.z;

		if(m_sel.zb)
		{
			movaps(xmm0, ptr[&m_local.temp.zo]);
			addps(xmm0, ptr[&m_local.d4.z]);
			movaps(ptr[&m_local.temp.zo], xmm0);
			addps(xmm0, ptr[&m_local.temp.z]);
		}

		// f = f.add16(m_local.d4.f);

		if(m_sel.fwrite && m_sel.fge)
		{
			movdqa(xmm1, ptr[&m_local.temp.f]);
			paddw(xmm1, ptr[&m_local.d4.f]);
			movdqa(ptr[&m_local.temp.f], xmm1);
		}
	}
	else
	{
		if(m_sel.ztest)
		{
			movdqa(xmm0, ptr[&m_local.p.z]);
		}
	}

	if(m_sel.fb)
	{
		if(m_sel.tfx != TFX_NONE)
		{
			if(m_sel.fst)
			{
				// GSVector4i stq = m_local.d4.stq;

				// s += stq.xxxx();
				// if(!sprite) t += stq.yyyy();

				movdqa(xmm4, ptr[&m_local.d4.stq]);

				pshufd(xmm2, xmm4, _MM_SHUFFLE(0, 0, 0, 0));
				paddd(xmm2, ptr[&m_local.temp.s]);
				movdqa(ptr[&m_local.temp.s], xmm2);

				if(m_sel.prim != GS_SPRITE_CLASS || m_sel.mmin)
				{
					pshufd(xmm3, xmm4, _MM_SHUFFLE(1, 1, 1, 1));
					paddd(xmm3, ptr[&m_local.temp.t]);
					movdqa(ptr[&m_local.temp.t], xmm3);
				}
				else
				{
					movdqa(xmm3, ptr[&m_local.temp.t]);
				}
			}
			else
			{
				// GSVector4 stq = m_local.d4.stq;

				// s += stq.xxxx();
				// t += stq.yyyy();
				// q += stq.zzzz();

				movaps(xmm4, ptr[&m_local.d4.stq]);
				movaps(xmm2, xmm4);
				movaps(xmm3, xmm4);

				shufps(xmm2, xmm2, _MM_SHUFFLE(0, 0, 0, 0));
				shufps(xmm3, xmm3, _MM_SHUFFLE(1, 1, 1, 1));
				shufps(xmm4, xmm4, _MM_SHUFFLE(2, 2, 2, 2));

				addps(xmm2, ptr[&m_local.temp.s]);
				addps(xmm3, ptr[&m_local.temp.t]);
				addps(xmm4, ptr[&m_local.temp.q]);

				movaps(ptr[&m_local.temp.s], xmm2);
				movaps(ptr[&m_local.temp.t], xmm3);
				movaps(ptr[&m_local.temp.q], xmm4);
			}
		}

		if(!(m_sel.tfx == TFX_DECAL && m_sel.tcc))
		{
			if(m_sel.iip)
			{
				// GSVector4i c = m_local.d4.c;

				// rb = rb.add16(c.xxxx());
				// ga = ga.add16(c.yyyy());

				movdqa(xmm7, ptr[&m_local.d4.c]);

				pshufd(xmm5, xmm7, _MM_SHUFFLE(0, 0, 0, 0));
				pshufd(xmm6, xmm7, _MM_SHUFFLE(1, 1, 1, 1));

				paddw(xmm5, ptr[&m_local.temp.rb]);
				paddw(xmm6, ptr[&m_local.temp.ga]);

				// FIXME: color may underflow and roll over at the end of the line, if decreasing

				pxor(xmm7, xmm7);
				pmaxsw(xmm5, xmm7);
				pmaxsw(xmm6, xmm7);

				movdqa(ptr[&m_local.temp.rb], xmm5);
				movdqa(ptr[&m_local.temp.ga], xmm6);
			}
			else
			{
				if(m_sel.tfx == TFX_NONE)
				{
					movdqa(xmm5, ptr[&m_local.c.rb]);
					movdqa(xmm6, ptr[&m_local.c.ga]);
				}
			}
		}
	}

	if(!m_sel.notest)
	{
		// test = m_test[7 + (steps & (steps >> 31))];

		mov(edx, ecx);
		sar(edx, 31);
		and(edx, ecx);
		shl(edx, 4);

		movdqa(xmm7, ptr[edx + (size_t)&m_test[7]]);
	}
}

void GSDrawScanlineCodeGenerator::TestZ(const Xmm& temp1, const Xmm& temp2)
{
	if(!m_sel.zb)
	{
		return;
	}

	// int za = fza_base.y + fza_offset->y;

	mov(ebp, ptr[esi + 4]);
	add(ebp, ptr[edi + 4]);

	// GSVector4i zs = zi;

	if(m_sel.prim != GS_SPRITE_CLASS)
	{
		if(m_sel.zoverflow)
		{
			// zs = (GSVector4i(z * 0.5f) << 1) | (GSVector4i(z) & GSVector4i::x00000001());

			movaps(temp1, ptr[&GSVector4::m_half]);
			mulps(temp1, xmm0);
			cvttps2dq(temp1, temp1);
			pslld(temp1, 1);

			cvttps2dq(xmm0, xmm0);
			pcmpeqd(temp2, temp2);
			psrld(temp2, 31);
			pand(xmm0, temp2);

			por(xmm0, temp1);
		}
		else
		{
			// zs = GSVector4i(z);

			cvttps2dq(xmm0, xmm0);
		}

		if(m_sel.zwrite)
		{
			movdqa(ptr[&m_local.temp.zs], xmm0);
		}
	}

	if(m_sel.ztest)
	{
		ReadPixel(xmm1, ebp);

		if(m_sel.zwrite && m_sel.zpsm < 2)
		{
			movdqa(ptr[&m_local.temp.zd], xmm1);
		}

		// zd &= 0xffffffff >> m_sel.zpsm * 8;

		if(m_sel.zpsm)
		{
			pslld(xmm1, m_sel.zpsm * 8);
			psrld(xmm1, m_sel.zpsm * 8);
		}

		if(m_sel.zoverflow || m_sel.zpsm == 0)
		{
			// GSVector4i o = GSVector4i::x80000000();

			pcmpeqd(temp1, temp1);
			pslld(temp1, 31);

			// GSVector4i zso = zs - o;
			// GSVector4i zdo = zd - o;

			psubd(xmm0, temp1);
			psubd(xmm1, temp1);
		}

		switch(m_sel.ztst)
		{
		case ZTST_GEQUAL:
			// test |= zso < zdo; // ~(zso >= zdo)
			pcmpgtd(xmm1, xmm0);
			por(xmm7, xmm1);
			break;

		case ZTST_GREATER: // TODO: tidus hair and chocobo wings only appear fully when this is tested as ZTST_GEQUAL
			// test |= zso <= zdo; // ~(zso > zdo)
			pcmpgtd(xmm0, xmm1);
			pcmpeqd(temp1, temp1);
			pxor(xmm0, temp1);
			por(xmm7, xmm0);
			break;
		}

		alltrue();
	}
}

void GSDrawScanlineCodeGenerator::SampleTexture()
{
	if(!m_sel.fb || m_sel.tfx == TFX_NONE)
	{
		return;
	}

	mov(ebx, ptr[&m_local.gd->tex[0]]);

	if(m_sel.tlu)
	{
		mov(edx, ptr[&m_local.gd->clut]);
	}

	// ebx = tex
	// edx = clut

	if(!m_sel.fst)
	{
		rcpps(xmm4, xmm4);

		mulps(xmm2, xmm4);
		mulps(xmm3, xmm4);

		cvttps2dq(xmm2, xmm2);
		cvttps2dq(xmm3, xmm3);

		if(m_sel.ltf)
		{
			// u -= 0x8000;
			// v -= 0x8000;

			mov(eax, 0x8000);
			movd(xmm4, eax);
			pshufd(xmm4, xmm4, _MM_SHUFFLE(0, 0, 0, 0));

			psubd(xmm2, xmm4);
			psubd(xmm3, xmm4);
		}
	}

	// xmm2 = u
	// xmm3 = v

	if(m_sel.ltf)
	{
		// GSVector4i uf = u.xxzzlh().srl16(1);

		pshuflw(xmm0, xmm2, _MM_SHUFFLE(2, 2, 0, 0));
		pshufhw(xmm0, xmm0, _MM_SHUFFLE(2, 2, 0, 0));
		psrlw(xmm0, 12);
		movdqa(ptr[&m_local.temp.uf], xmm0);

		if(m_sel.prim != GS_SPRITE_CLASS)
		{
			// GSVector4i vf = v.xxzzlh().srl16(1);

			pshuflw(xmm0, xmm3, _MM_SHUFFLE(2, 2, 0, 0));
			pshufhw(xmm0, xmm0, _MM_SHUFFLE(2, 2, 0, 0));
			psrlw(xmm0, 12);
			movdqa(ptr[&m_local.temp.vf], xmm0);
		}
	}

	// GSVector4i uv0 = u.sra32(16).ps32(v.sra32(16));

	psrad(xmm2, 16);
	psrad(xmm3, 16);
	packssdw(xmm2, xmm3);

	if(m_sel.ltf)
	{
		// GSVector4i uv1 = uv0.add16(GSVector4i::x0001());

		movdqa(xmm3, xmm2);
		pcmpeqd(xmm1, xmm1);
		psrlw(xmm1, 15);
		paddw(xmm3, xmm1);

		// uv0 = Wrap(uv0);
		// uv1 = Wrap(uv1);

		Wrap(xmm2, xmm3);
	}
	else
	{
		// uv0 = Wrap(uv0);

		Wrap(xmm2);
	}

	// xmm2 = uv0
	// xmm3 = uv1 (ltf)
	// xmm0, xmm1, xmm4, xmm5, xmm6 = free
	// xmm7 = used

	// GSVector4i y0 = uv0.uph16() << tw;
	// GSVector4i x0 = uv0.upl16();

	pxor(xmm0, xmm0);

	movdqa(xmm4, xmm2);
	punpckhwd(xmm2, xmm0);
	punpcklwd(xmm4, xmm0);
	pslld(xmm2, m_sel.tw + 3);

	// xmm0 = 0
	// xmm2 = y0
	// xmm3 = uv1 (ltf)
	// xmm4 = x0
	// xmm1, xmm5, xmm6 = free
	// xmm7 = used

	if(m_sel.ltf)
	{
		// GSVector4i y1 = uv1.uph16() << tw;
		// GSVector4i x1 = uv1.upl16();

		movdqa(xmm6, xmm3);
		punpckhwd(xmm3, xmm0);
		punpcklwd(xmm6, xmm0);
		pslld(xmm3, m_sel.tw + 3);

		// xmm2 = y0
		// xmm3 = y1
		// xmm4 = x0
		// xmm6 = x1
		// xmm0, xmm5, xmm6 = free
		// xmm7 = used

		// GSVector4i addr00 = y0 + x0;
		// GSVector4i addr01 = y0 + x1;
		// GSVector4i addr10 = y1 + x0;
		// GSVector4i addr11 = y1 + x1;

		movdqa(xmm5, xmm2);
		paddd(xmm5, xmm4);
		paddd(xmm2, xmm6);

		movdqa(xmm0, xmm3);
		paddd(xmm0, xmm4);
		paddd(xmm3, xmm6);

		// xmm5 = addr00
		// xmm2 = addr01
		// xmm0 = addr10
		// xmm3 = addr11
		// xmm1, xmm4, xmm6 = free
		// xmm7 = used

		// c00 = addr00.gather32_32((const uint32/uint8*)tex[, clut]);
		// c01 = addr01.gather32_32((const uint32/uint8*)tex[, clut]);
		// c10 = addr10.gather32_32((const uint32/uint8*)tex[, clut]);
		// c11 = addr11.gather32_32((const uint32/uint8*)tex[, clut]);

		ReadTexel(4, 0);

		// xmm6 = c00
		// xmm4 = c01
		// xmm1 = c10
		// xmm5 = c11
		// xmm0, xmm2, xmm3 = free
		// xmm7 = used

		movdqa(xmm0, ptr[&m_local.temp.uf]);

		// GSVector4i rb00 = c00 & mask;
		// GSVector4i ga00 = (c00 >> 8) & mask;

		movdqa(xmm2, xmm6);
		psllw(xmm2, 8);
		psrlw(xmm2, 8);
		psrlw(xmm6, 8);

		// GSVector4i rb01 = c01 & mask;
		// GSVector4i ga01 = (c01 >> 8) & mask;

		movdqa(xmm3, xmm4);
		psllw(xmm3, 8);
		psrlw(xmm3, 8);
		psrlw(xmm4, 8);

		// xmm0 = uf
		// xmm2 = rb00
		// xmm3 = rb01
		// xmm6 = ga00
		// xmm4 = ga01
		// xmm1 = c10
		// xmm5 = c11
		// xmm7 = used

		// rb00 = rb00.lerp_4(rb01, uf);
		// ga00 = ga00.lerp_4(ga01, uf);

		lerp16_4(xmm3, xmm2, xmm0);
		lerp16_4(xmm4, xmm6, xmm0);

		// xmm0 = uf
		// xmm3 = rb00
		// xmm4 = ga00
		// xmm1 = c10
		// xmm5 = c11
		// xmm2, xmm6 = free
		// xmm7 = used

		// GSVector4i rb10 = c10 & mask;
		// GSVector4i ga10 = (c10 >> 8) & mask;

		movdqa(xmm2, xmm1);
		psllw(xmm1, 8);
		psrlw(xmm1, 8);
		psrlw(xmm2, 8);

		// GSVector4i rb11 = c11 & mask;
		// GSVector4i ga11 = (c11 >> 8) & mask;

		movdqa(xmm6, xmm5);
		psllw(xmm5, 8);
		psrlw(xmm5, 8);
		psrlw(xmm6, 8);

		// xmm0 = uf
		// xmm3 = rb00
		// xmm4 = ga00
		// xmm1 = rb10
		// xmm5 = rb11
		// xmm2 = ga10
		// xmm6 = ga11
		// xmm7 = used

		// rb10 = rb10.lerp_4(rb11, uf);
		// ga10 = ga10.lerp_4(ga11, uf);

		lerp16_4(xmm5, xmm1, xmm0);
		lerp16_4(xmm6, xmm2, xmm0);

		// xmm3 = rb00
		// xmm4 = ga00
		// xmm5 = rb10
		// xmm6 = ga10
		// xmm0, xmm1, xmm2 = free
		// xmm7 = used

		// rb00 = rb00.lerp_4(rb10, vf);
		// ga00 = ga00.lerp_4(ga10, vf);

		movdqa(xmm0, ptr[&m_local.temp.vf]);

		lerp16_4(xmm5, xmm3, xmm0);
		lerp16_4(xmm6, xmm4, xmm0);
	}
	else
	{
		// GSVector4i addr00 = y0 + x0;

		paddd(xmm2, xmm4);
		movdqa(xmm5, xmm2);

		// c00 = addr00.gather32_32((const uint32/uint8*)tex[, clut]);

		ReadTexel(1, 0);

		// GSVector4i mask = GSVector4i::x00ff();

		// c[0] = c00 & mask;
		// c[1] = (c00 >> 8) & mask;

		movdqa(xmm5, xmm6);
		psllw(xmm5, 8);
		psrlw(xmm5, 8);
		psrlw(xmm6, 8);
	}
}

void GSDrawScanlineCodeGenerator::Wrap(const Xmm& uv)
{
	// xmm0, xmm1, xmm4, xmm5, xmm6 = free

	int wms_clamp = ((m_sel.wms + 1) >> 1) & 1;
	int wmt_clamp = ((m_sel.wmt + 1) >> 1) & 1;

	int region = ((m_sel.wms | m_sel.wmt) >> 1) & 1;

	if(wms_clamp == wmt_clamp)
	{
		if(wms_clamp)
		{
			if(region)
			{
				pmaxsw(uv, ptr[&m_local.gd->t.min]);
			}
			else
			{
				pxor(xmm0, xmm0);
				pmaxsw(uv, xmm0);
			}

			pminsw(uv, ptr[&m_local.gd->t.max]);
		}
		else
		{
			pand(uv, ptr[&m_local.gd->t.min]);

			if(region)
			{
				por(uv, ptr[&m_local.gd->t.max]);
			}
		}
	}
	else
	{
		movdqa(xmm4, ptr[&m_local.gd->t.min]);
		movdqa(xmm5, ptr[&m_local.gd->t.max]);
		movdqa(xmm0, ptr[&m_local.gd->t.mask]);

		// GSVector4i repeat = (t & m_local.gd->t.min) | m_local.gd->t.max;

		movdqa(xmm1, uv);

		pand(xmm1, xmm4);

		if(region)
		{
			por(xmm1, xmm5);
		}

		// GSVector4i clamp = t.sat_i16(m_local.gd->t.min, m_local.gd->t.max);

		pmaxsw(uv, xmm4);
		pminsw(uv, xmm5);

		// clamp.blend8(repeat, m_local.gd->t.mask);

		blend8(uv, xmm1);
	}
}

void GSDrawScanlineCodeGenerator::Wrap(const Xmm& uv0, const Xmm& uv1)
{
	// xmm0, xmm1, xmm4, xmm5, xmm6 = free

	int wms_clamp = ((m_sel.wms + 1) >> 1) & 1;
	int wmt_clamp = ((m_sel.wmt + 1) >> 1) & 1;

	int region = ((m_sel.wms | m_sel.wmt) >> 1) & 1;

	if(wms_clamp == wmt_clamp)
	{
		if(wms_clamp)
		{
			if(region)
			{
				movdqa(xmm4, ptr[&m_local.gd->t.min]);
				pmaxsw(uv0, xmm4);
				pmaxsw(uv1, xmm4);
			}
			else
			{
				pxor(xmm0, xmm0);
				pmaxsw(uv0, xmm0);
				pmaxsw(uv1, xmm0);
			}

			movdqa(xmm5, ptr[&m_local.gd->t.max]);
			pminsw(uv0, xmm5);
			pminsw(uv1, xmm5);
		}
		else
		{
			movdqa(xmm4, ptr[&m_local.gd->t.min]);
			pand(uv0, xmm4);
			pand(uv1, xmm4);

			if(region)
			{
				movdqa(xmm5, ptr[&m_local.gd->t.max]);
				por(uv0, xmm5);
				por(uv1, xmm5);
			}
		}
	}
	else
	{
		movdqa(xmm4, ptr[&m_local.gd->t.min]);
		movdqa(xmm5, ptr[&m_local.gd->t.max]);

		#if _M_SSE >= 0x401
		
		movdqa(xmm0, ptr[&m_local.gd->t.mask]);
		
		#else
		
		movdqa(xmm0, ptr[&m_local.gd->t.invmask]);
		movdqa(xmm6, xmm0);
		
		#endif

		// uv0

		// GSVector4i repeat = (t & m_local.gd->t.min) | m_local.gd->t.max;

		movdqa(xmm1, uv0);

		pand(xmm1, xmm4);

		if(region)
		{
			por(xmm1, xmm5);
		}

		// GSVector4i clamp = t.sat_i16(m_local.gd->t.min, m_local.gd->t.max);

		pmaxsw(uv0, xmm4);
		pminsw(uv0, xmm5);

		// clamp.blend8(repeat, m_local.gd->t.mask);

		#if _M_SSE >= 0x401
		
		pblendvb(uv0, xmm1);

		#else

		blendr(uv0, xmm1, xmm0);

		#endif

		// uv1

		// GSVector4i repeat = (t & m_local.gd->t.min) | m_local.gd->t.max;

		movdqa(xmm1, uv1);

		pand(xmm1, xmm4);

		if(region)
		{
			por(xmm1, xmm5);
		}

		// GSVector4i clamp = t.sat_i16(m_local.gd->t.min, m_local.gd->t.max);

		pmaxsw(uv1, xmm4);
		pminsw(uv1, xmm5);

		// clamp.blend8(repeat, m_local.gd->t.mask);

		#if _M_SSE >= 0x401

		pblendvb(uv1, xmm1);

		#else
		
		blendr(uv1, xmm1, xmm6);

		#endif
	}
}

void GSDrawScanlineCodeGenerator::SampleTextureLOD()
{
	if(!m_sel.fb || m_sel.tfx == TFX_NONE)
	{
		return;
	}

	push(ebp);

	mov(ebp, (size_t)m_local.gd->tex);		

	if(m_sel.tlu)
	{
		mov(edx, ptr[&m_local.gd->clut]);
	}

	if(!m_sel.fst)
	{
		rcpps(xmm0, xmm4);

		mulps(xmm2, xmm0);
		mulps(xmm3, xmm0);

		cvttps2dq(xmm2, xmm2);
		cvttps2dq(xmm3, xmm3);
	}

	// xmm2 = u
	// xmm3 = v
	// xmm4 = q
	// xmm0 = xmm1 = xmm5 = xmm6 = free

	// TODO: if the fractional part is not needed in round-off mode then there is a faster integer log2 (just take the exp) (but can we round it?)

	if(!m_sel.lcm)
	{
		// store u/v

		movdqa(xmm0, xmm2);
		punpckldq(xmm2, xmm3);
		movdqa(ptr[&m_local.temp.uv[0]], xmm2);
		punpckhdq(xmm0, xmm3);
		movdqa(ptr[&m_local.temp.uv[1]], xmm0);

		// lod = -log2(Q) * (1 << L) + K

		movdqa(xmm0, xmm4);
		pcmpeqd(xmm1, xmm1);
		psrld(xmm1, 25);
		pslld(xmm0, 1);
		psrld(xmm0, 24);
		psubd(xmm0, xmm1);
		cvtdq2ps(xmm0, xmm0); 

		// xmm0 = (float)(exp(q) - 127)

		pslld(xmm4, 9);
		psrld(xmm4, 9);
		orps(xmm4, ptr[&GSDrawScanlineCodeGenerator::m_log2_coef[3]]); 
			
		// xmm4 = mant(q) | 1.0f

		movdqa(xmm5, xmm4);
		mulps(xmm5, ptr[&GSDrawScanlineCodeGenerator::m_log2_coef[0]]);
		addps(xmm5, ptr[&GSDrawScanlineCodeGenerator::m_log2_coef[1]]);
		mulps(xmm5, xmm4);
		subps(xmm4, ptr[&GSDrawScanlineCodeGenerator::m_log2_coef[3]]); 
		addps(xmm5, ptr[&GSDrawScanlineCodeGenerator::m_log2_coef[2]]);
		mulps(xmm4, xmm5);
		addps(xmm4, xmm0);

		// xmm4 = log2(Q) = ((((c0 * xmm4) + c1) * xmm4) + c2) * (xmm4 - 1.0f) + xmm0

		mulps(xmm4, ptr[&m_local.gd->l]);
		addps(xmm4, ptr[&m_local.gd->k]);

		// xmm4 = (-log2(Q) * (1 << L) + K) * 0x10000

		xorps(xmm0, xmm0);
		minps(xmm4, ptr[&m_local.gd->mxl]);
		maxps(xmm4, xmm0);
		cvtps2dq(xmm4, xmm4);

		if(m_sel.mmin == 1) // round-off mode
		{
			mov(eax, 0x8000);
			movd(xmm0, eax);
			pshufd(xmm0, xmm0, _MM_SHUFFLE(0, 0, 0, 0));
			paddd(xmm4, xmm0);
		}

		movdqa(xmm0, xmm4);
		psrld(xmm4, 16);
		movdqa(ptr[&m_local.temp.lod.i], xmm4);

		if(m_sel.mmin == 2) // trilinear mode
		{
			pshuflw(xmm0, xmm0, _MM_SHUFFLE(2, 2, 0, 0));
			pshufhw(xmm0, xmm0, _MM_SHUFFLE(2, 2, 0, 0));
			movdqa(ptr[&m_local.temp.lod.f], xmm0);
		}

		// shift u/v by (int)lod

		movq(xmm4, ptr[&m_local.gd->t.minmax]);

		movdqa(xmm2, ptr[&m_local.temp.uv[0]]);
		movdqa(xmm5, xmm2);
		movdqa(xmm3, ptr[&m_local.temp.uv[1]]);
		movdqa(xmm6, xmm3);

		movd(xmm0, ptr[&m_local.temp.lod.i.u32[0]]); 
		psrad(xmm2, xmm0);
		movdqa(xmm1, xmm4);
		psrlw(xmm1, xmm0);
		movq(ptr[&m_local.temp.uv_minmax[0].u32[0]], xmm1);

		movd(xmm0, ptr[&m_local.temp.lod.i.u32[1]]);
		psrad(xmm5, xmm0);
		movdqa(xmm1, xmm4);
		psrlw(xmm1, xmm0);
		movq(ptr[&m_local.temp.uv_minmax[1].u32[0]], xmm1);

		movd(xmm0, ptr[&m_local.temp.lod.i.u32[2]]);
		psrad(xmm3, xmm0);
		movdqa(xmm1, xmm4);
		psrlw(xmm1, xmm0);
		movq(ptr[&m_local.temp.uv_minmax[0].u32[2]], xmm1);

		movd(xmm0, ptr[&m_local.temp.lod.i.u32[3]]);
		psrad(xmm6, xmm0);
		movdqa(xmm1, xmm4);
		psrlw(xmm1, xmm0);
		movq(ptr[&m_local.temp.uv_minmax[1].u32[2]], xmm1);

		punpckldq(xmm2, xmm3);
		punpckhdq(xmm5, xmm6);
		movdqa(xmm3, xmm2);
		punpckldq(xmm2, xmm5);
		punpckhdq(xmm3, xmm5);

		movdqa(ptr[&m_local.temp.uv[0]], xmm2);
		movdqa(ptr[&m_local.temp.uv[1]], xmm3);

		movdqa(xmm5, ptr[&m_local.temp.uv_minmax[0]]);
		movdqa(xmm6, ptr[&m_local.temp.uv_minmax[1]]);

		movdqa(xmm0, xmm5);
		punpcklwd(xmm5, xmm6);
		punpckhwd(xmm0, xmm6);
		movdqa(xmm6, xmm5);
		punpckldq(xmm5, xmm0);
		punpckhdq(xmm6, xmm0);

		movdqa(ptr[&m_local.temp.uv_minmax[0]], xmm5);
		movdqa(ptr[&m_local.temp.uv_minmax[1]], xmm6);
	}
	else
	{
		// lod = K

		movd(xmm0, ptr[&m_local.gd->lod.i.u32[0]]);

		psrad(xmm2, xmm0);
		psrad(xmm3, xmm0);

		movdqa(ptr[&m_local.temp.uv[0]], xmm2);
		movdqa(ptr[&m_local.temp.uv[1]], xmm3);

		movdqa(xmm5, ptr[&m_local.temp.uv_minmax[0]]);
		movdqa(xmm6, ptr[&m_local.temp.uv_minmax[1]]);
	}

	// xmm2 = m_local.temp.uv[0] = u (level m)
	// xmm3 = m_local.temp.uv[1] = v (level m)
	// xmm5 = minuv
	// xmm6 = maxuv

	if(m_sel.ltf)
	{
		// u -= 0x8000;
		// v -= 0x8000;

		mov(eax, 0x8000);
		movd(xmm4, eax);
		pshufd(xmm4, xmm4, _MM_SHUFFLE(0, 0, 0, 0));

		psubd(xmm2, xmm4);
		psubd(xmm3, xmm4);

		// GSVector4i uf = u.xxzzlh().srl16(1);
	
		pshuflw(xmm0, xmm2, _MM_SHUFFLE(2, 2, 0, 0));
		pshufhw(xmm0, xmm0, _MM_SHUFFLE(2, 2, 0, 0));
		psrlw(xmm0, 12);
		movdqa(ptr[&m_local.temp.uf], xmm0);

		// GSVector4i vf = v.xxzzlh().srl16(1);

		pshuflw(xmm0, xmm3, _MM_SHUFFLE(2, 2, 0, 0));
		pshufhw(xmm0, xmm0, _MM_SHUFFLE(2, 2, 0, 0));
		psrlw(xmm0, 12);
		movdqa(ptr[&m_local.temp.vf], xmm0);
	}

	// GSVector4i uv0 = u.sra32(16).ps32(v.sra32(16));

	psrad(xmm2, 16);
	psrad(xmm3, 16);
	packssdw(xmm2, xmm3);

	if(m_sel.ltf)
	{
		// GSVector4i uv1 = uv0.add16(GSVector4i::x0001());

		movdqa(xmm3, xmm2);
		pcmpeqd(xmm1, xmm1);
		psrlw(xmm1, 15);
		paddw(xmm3, xmm1);

		// uv0 = Wrap(uv0);
		// uv1 = Wrap(uv1);

		WrapLOD(xmm2, xmm3);
	}
	else
	{
		// uv0 = Wrap(uv0);

		WrapLOD(xmm2);
	}

	// xmm2 = uv0
	// xmm3 = uv1 (ltf)
	// xmm0, xmm1, xmm4, xmm5, xmm6 = free
	// xmm7 = used

	// GSVector4i x0 = uv0.upl16();
	// GSVector4i y0 = uv0.uph16() << tw;

	pxor(xmm0, xmm0);

	movdqa(xmm4, xmm2);
	punpckhwd(xmm2, xmm0);
	punpcklwd(xmm4, xmm0);
	pslld(xmm2, m_sel.tw + 3);

	// xmm0 = 0
	// xmm2 = y0
	// xmm3 = uv1 (ltf)
	// xmm4 = x0
	// xmm1, xmm5, xmm6 = free
	// xmm7 = used

	if(m_sel.ltf)
	{
		// GSVector4i x1 = uv1.upl16();
		// GSVector4i y1 = uv1.uph16() << tw;

		movdqa(xmm6, xmm3);
		punpcklwd(xmm6, xmm0);
		punpckhwd(xmm3, xmm0);
		pslld(xmm3, m_sel.tw + 3);

		// xmm2 = y0
		// xmm3 = y1
		// xmm4 = x0
		// xmm6 = x1
		// xmm0, xmm5, xmm6 = free
		// xmm7 = used

		// GSVector4i addr00 = y0 + x0;
		// GSVector4i addr01 = y0 + x1;
		// GSVector4i addr10 = y1 + x0;
		// GSVector4i addr11 = y1 + x1;

		movdqa(xmm5, xmm2);
		paddd(xmm5, xmm4);
		paddd(xmm2, xmm6);

		movdqa(xmm0, xmm3);
		paddd(xmm0, xmm4);
		paddd(xmm3, xmm6);

		// xmm5 = addr00
		// xmm2 = addr01
		// xmm0 = addr10
		// xmm3 = addr11
		// xmm1, xmm4, xmm6 = free
		// xmm7 = used

		// c00 = addr00.gather32_32((const uint32/uint8*)tex[, clut]);
		// c01 = addr01.gather32_32((const uint32/uint8*)tex[, clut]);
		// c10 = addr10.gather32_32((const uint32/uint8*)tex[, clut]);
		// c11 = addr11.gather32_32((const uint32/uint8*)tex[, clut]);

		ReadTexel(4, 0);

		// xmm6 = c00
		// xmm4 = c01
		// xmm1 = c10
		// xmm5 = c11
		// xmm0, xmm2, xmm3 = free
		// xmm7 = used

		movdqa(xmm0, ptr[&m_local.temp.uf]);

		// GSVector4i rb00 = c00 & mask;
		// GSVector4i ga00 = (c00 >> 8) & mask;

		movdqa(xmm2, xmm6);
		psrlw(xmm6, 8);
		psllw(xmm2, 8);
		psrlw(xmm2, 8);

		// GSVector4i rb01 = c01 & mask;
		// GSVector4i ga01 = (c01 >> 8) & mask;

		movdqa(xmm3, xmm4);
		psrlw(xmm4, 8);
		psllw(xmm3, 8);
		psrlw(xmm3, 8);

		// xmm0 = uf
		// xmm2 = rb00
		// xmm3 = rb01
		// xmm6 = ga00
		// xmm4 = ga01
		// xmm1 = c10
		// xmm5 = c11
		// xmm7 = used

		// rb00 = rb00.lerp_4(rb01, uf);
		// ga00 = ga00.lerp_4(ga01, uf);

		lerp16_4(xmm3, xmm2, xmm0);
		lerp16_4(xmm4, xmm6, xmm0);

		// xmm0 = uf
		// xmm3 = rb00
		// xmm4 = ga00
		// xmm1 = c10
		// xmm5 = c11
		// xmm2, xmm6 = free
		// xmm7 = used

		// GSVector4i rb10 = c10 & mask;
		// GSVector4i ga10 = (c10 >> 8) & mask;

		movdqa(xmm2, xmm1);
		psllw(xmm1, 8);
		psrlw(xmm1, 8);
		psrlw(xmm2, 8);

		// GSVector4i rb11 = c11 & mask;
		// GSVector4i ga11 = (c11 >> 8) & mask;

		movdqa(xmm6, xmm5);
		psllw(xmm5, 8);
		psrlw(xmm5, 8);
		psrlw(xmm6, 8);

		// xmm0 = uf
		// xmm3 = rb00
		// xmm4 = ga00
		// xmm1 = rb10
		// xmm5 = rb11
		// xmm2 = ga10
		// xmm6 = ga11
		// xmm7 = used

		// rb10 = rb10.lerp_4(rb11, uf);
		// ga10 = ga10.lerp_4(ga11, uf);

		lerp16_4(xmm5, xmm1, xmm0);
		lerp16_4(xmm6, xmm2, xmm0);

		// xmm3 = rb00
		// xmm4 = ga00
		// xmm5 = rb10
		// xmm6 = ga10
		// xmm0, xmm1, xmm2 = free
		// xmm7 = used

		// rb00 = rb00.lerp_4(rb10, vf);
		// ga00 = ga00.lerp_4(ga10, vf);

		movdqa(xmm0, ptr[&m_local.temp.vf]);

		lerp16_4(xmm5, xmm3, xmm0);
		lerp16_4(xmm6, xmm4, xmm0);
	}
	else
	{
		// GSVector4i addr00 = y0 + x0;

		paddd(xmm2, xmm4);
		movdqa(xmm5, xmm2);

		// c00 = addr00.gather32_32((const uint32/uint8*)tex[, clut]);

		ReadTexel(1, 0);

		// GSVector4i mask = GSVector4i::x00ff();

		// c[0] = c00 & mask;
		// c[1] = (c00 >> 8) & mask;

		movdqa(xmm5, xmm6);
		psllw(xmm5, 8);
		psrlw(xmm5, 8);
		psrlw(xmm6, 8);
	}

	if(m_sel.mmin != 1) // !round-off mode
	{
		movdqa(ptr[&m_local.temp.trb], xmm5);
		movdqa(ptr[&m_local.temp.tga], xmm6);

		movdqa(xmm2, ptr[&m_local.temp.uv[0]]);
		movdqa(xmm3, ptr[&m_local.temp.uv[1]]);

		psrad(xmm2, 1);
		psrad(xmm3, 1);

		movdqa(xmm5, ptr[&m_local.temp.uv_minmax[0]]);
		movdqa(xmm6, ptr[&m_local.temp.uv_minmax[1]]);

		psrlw(xmm5, 1);
		psrlw(xmm6, 1);

		if(m_sel.ltf)
		{
			// u -= 0x8000;
			// v -= 0x8000;

			mov(eax, 0x8000);
			movd(xmm4, eax);
			pshufd(xmm4, xmm4, _MM_SHUFFLE(0, 0, 0, 0));

			psubd(xmm2, xmm4);
			psubd(xmm3, xmm4);

			// GSVector4i uf = u.xxzzlh().srl16(1);
	
			pshuflw(xmm0, xmm2, _MM_SHUFFLE(2, 2, 0, 0));
			pshufhw(xmm0, xmm0, _MM_SHUFFLE(2, 2, 0, 0));
			psrlw(xmm0, 12);
			movdqa(ptr[&m_local.temp.uf], xmm0);

			// GSVector4i vf = v.xxzzlh().srl16(1);

			pshuflw(xmm0, xmm3, _MM_SHUFFLE(2, 2, 0, 0));
			pshufhw(xmm0, xmm0, _MM_SHUFFLE(2, 2, 0, 0));
			psrlw(xmm0, 12);
			movdqa(ptr[&m_local.temp.vf], xmm0);
		}

		// GSVector4i uv0 = u.sra32(16).ps32(v.sra32(16));

		psrad(xmm2, 16);
		psrad(xmm3, 16);
		packssdw(xmm2, xmm3);

		if(m_sel.ltf)
		{
			// GSVector4i uv1 = uv0.add16(GSVector4i::x0001());

			movdqa(xmm3, xmm2);
			pcmpeqd(xmm1, xmm1);
			psrlw(xmm1, 15);
			paddw(xmm3, xmm1);

			// uv0 = Wrap(uv0);
			// uv1 = Wrap(uv1);

			WrapLOD(xmm2, xmm3);
		}
		else
		{
			// uv0 = Wrap(uv0);

			WrapLOD(xmm2);
		}

		// xmm2 = uv0
		// xmm3 = uv1 (ltf)
		// xmm0, xmm1, xmm4, xmm5, xmm6 = free
		// xmm7 = used

		// GSVector4i x0 = uv0.upl16();
		// GSVector4i y0 = uv0.uph16() << tw;

		pxor(xmm0, xmm0);

		movdqa(xmm4, xmm2);
		punpckhwd(xmm2, xmm0);
		punpcklwd(xmm4, xmm0);
		pslld(xmm2, m_sel.tw + 3);

		// xmm0 = 0
		// xmm2 = y0
		// xmm3 = uv1 (ltf)
		// xmm4 = x0
		// xmm1, xmm5, xmm6 = free
		// xmm7 = used

		if(m_sel.ltf)
		{
			// GSVector4i x1 = uv1.upl16();
			// GSVector4i y1 = uv1.uph16() << tw;

			movdqa(xmm6, xmm3);
			punpckhwd(xmm3, xmm0);
			punpcklwd(xmm6, xmm0);
			pslld(xmm3, m_sel.tw + 3);

			// xmm2 = y0
			// xmm3 = y1
			// xmm4 = x0
			// xmm6 = x1
			// xmm0, xmm5, xmm6 = free
			// xmm7 = used

			// GSVector4i addr00 = y0 + x0;
			// GSVector4i addr01 = y0 + x1;
			// GSVector4i addr10 = y1 + x0;
			// GSVector4i addr11 = y1 + x1;

			movdqa(xmm5, xmm2);
			paddd(xmm5, xmm4);
			paddd(xmm2, xmm6);

			movdqa(xmm0, xmm3);
			paddd(xmm0, xmm4);
			paddd(xmm3, xmm6);

			// xmm5 = addr00
			// xmm2 = addr01
			// xmm0 = addr10
			// xmm3 = addr11
			// xmm1, xmm4, xmm6 = free
			// xmm7 = used

			// c00 = addr00.gather32_32((const uint32/uint8*)tex[, clut]);
			// c01 = addr01.gather32_32((const uint32/uint8*)tex[, clut]);
			// c10 = addr10.gather32_32((const uint32/uint8*)tex[, clut]);
			// c11 = addr11.gather32_32((const uint32/uint8*)tex[, clut]);

			ReadTexel(4, 1);

			// xmm6 = c00
			// xmm4 = c01
			// xmm1 = c10
			// xmm5 = c11
			// xmm0, xmm2, xmm3 = free
			// xmm7 = used

			movdqa(xmm0, ptr[&m_local.temp.uf]);

			// GSVector4i rb00 = c00 & mask;
			// GSVector4i ga00 = (c00 >> 8) & mask;

			movdqa(xmm2, xmm6);
			psllw(xmm2, 8);
			psrlw(xmm2, 8);
			psrlw(xmm6, 8);

			// GSVector4i rb01 = c01 & mask;
			// GSVector4i ga01 = (c01 >> 8) & mask;

			movdqa(xmm3, xmm4);
			psllw(xmm3, 8);
			psrlw(xmm3, 8);
			psrlw(xmm4, 8);

			// xmm0 = uf
			// xmm2 = rb00
			// xmm3 = rb01
			// xmm6 = ga00
			// xmm4 = ga01
			// xmm1 = c10
			// xmm5 = c11
			// xmm7 = used

			// rb00 = rb00.lerp_4(rb01, uf);
			// ga00 = ga00.lerp_4(ga01, uf);

			lerp16_4(xmm3, xmm2, xmm0);
			lerp16_4(xmm4, xmm6, xmm0);

			// xmm0 = uf
			// xmm3 = rb00
			// xmm4 = ga00
			// xmm1 = c10
			// xmm5 = c11
			// xmm2, xmm6 = free
			// xmm7 = used

			// GSVector4i rb10 = c10 & mask;
			// GSVector4i ga10 = (c10 >> 8) & mask;

			movdqa(xmm2, xmm1);
			psllw(xmm1, 8);
			psrlw(xmm1, 8);
			psrlw(xmm2, 8);

			// GSVector4i rb11 = c11 & mask;
			// GSVector4i ga11 = (c11 >> 8) & mask;

			movdqa(xmm6, xmm5);
			psllw(xmm5, 8);
			psrlw(xmm5, 8);
			psrlw(xmm6, 8);

			// xmm0 = uf
			// xmm3 = rb00
			// xmm4 = ga00
			// xmm1 = rb10
			// xmm5 = rb11
			// xmm2 = ga10
			// xmm6 = ga11
			// xmm7 = used

			// rb10 = rb10.lerp_4(rb11, uf);
			// ga10 = ga10.lerp_4(ga11, uf);

			lerp16_4(xmm5, xmm1, xmm0);
			lerp16_4(xmm6, xmm2, xmm0);

			// xmm3 = rb00
			// xmm4 = ga00
			// xmm5 = rb10
			// xmm6 = ga10
			// xmm0, xmm1, xmm2 = free
			// xmm7 = used

			// rb00 = rb00.lerp_4(rb10, vf);
			// ga00 = ga00.lerp_4(ga10, vf);

			movdqa(xmm0, ptr[&m_local.temp.vf]);

			lerp16_4(xmm5, xmm3, xmm0);
			lerp16_4(xmm6, xmm4, xmm0);
		}
		else
		{
			// GSVector4i addr00 = y0 + x0;

			paddd(xmm2, xmm4);
			movdqa(xmm5, xmm2);

			// c00 = addr00.gather32_32((const uint32/uint8*)tex[, clut]);

			ReadTexel(1, 1);

			// GSVector4i mask = GSVector4i::x00ff();

			// c[0] = c00 & mask;
			// c[1] = (c00 >> 8) & mask;

			movdqa(xmm5, xmm6);
			psllw(xmm5, 8);
			psrlw(xmm5, 8);
			psrlw(xmm6, 8);
		}

		movdqa(xmm0, ptr[m_sel.lcm ? &m_local.gd->lod.f : &m_local.temp.lod.f]);
		psrlw(xmm0, 1);

		movdqa(xmm2, ptr[&m_local.temp.trb]);
		movdqa(xmm3, ptr[&m_local.temp.tga]);

		lerp16(xmm5, xmm2, xmm0, 0);
		lerp16(xmm6, xmm3, xmm0, 0);
	}

	pop(ebp);
}

void GSDrawScanlineCodeGenerator::WrapLOD(const Xmm& uv)
{
	// xmm5 = minuv
	// xmm6 = maxuv
	// xmm0, xmm1, xmm4 = free

	int wms_clamp = ((m_sel.wms + 1) >> 1) & 1;
	int wmt_clamp = ((m_sel.wmt + 1) >> 1) & 1;

	int region = ((m_sel.wms | m_sel.wmt) >> 1) & 1;

	if(wms_clamp == wmt_clamp)
	{
		if(wms_clamp)
		{
			if(region)
			{
				pmaxsw(uv, xmm5);
			}
			else
			{
				pxor(xmm0, xmm0);
				pmaxsw(uv, xmm0);
			}

			pminsw(uv, xmm6);
		}
		else
		{
			pand(uv, xmm5);

			if(region)
			{
				por(uv, xmm6);
			}
		}
	}
	else
	{
		movdqa(xmm0, ptr[&m_local.gd->t.mask]);

		// GSVector4i repeat = (t & m_local.gd->t.min) | m_local.gd->t.max;

		movdqa(xmm1, uv);

		pand(xmm1, xmm5);

		if(region)
		{
			por(xmm1, xmm6);
		}

		// GSVector4i clamp = t.sat_i16(m_local.gd->t.min, m_local.gd->t.max);

		pmaxsw(uv, xmm5);
		pminsw(uv, xmm6);

		// clamp.blend8(repeat, m_local.gd->t.mask);

		blend8(uv, xmm1);
	}
}

void GSDrawScanlineCodeGenerator::WrapLOD(const Xmm& uv0, const Xmm& uv1)
{
	// xmm5 = minuv
	// xmm6 = maxuv
	// xmm0, xmm1, xmm4 = free

	int wms_clamp = ((m_sel.wms + 1) >> 1) & 1;
	int wmt_clamp = ((m_sel.wmt + 1) >> 1) & 1;

	int region = ((m_sel.wms | m_sel.wmt) >> 1) & 1;

	if(wms_clamp == wmt_clamp)
	{
		if(wms_clamp)
		{
			if(region)
			{
				pmaxsw(uv0, xmm5);
				pmaxsw(uv1, xmm5);
			}
			else
			{
				pxor(xmm0, xmm0);
				pmaxsw(uv0, xmm0);
				pmaxsw(uv1, xmm0);
			}

			pminsw(uv0, xmm6);
			pminsw(uv1, xmm6);
		}
		else
		{
			pand(uv0, xmm5);
			pand(uv1, xmm5);

			if(region)
			{
				por(uv0, xmm6);
				por(uv1, xmm6);
			}
		}
	}
	else
	{
		#if _M_SSE >= 0x401
		
		movdqa(xmm0, ptr[&m_local.gd->t.mask]);

		#else
		
		movdqa(xmm0, ptr[&m_local.gd->t.invmask]);
		movdqa(xmm4, xmm0);

		#endif

		// uv0

		// GSVector4i repeat = (t & m_local.gd->t.min) | m_local.gd->t.max;

		movdqa(xmm1, uv0);

		pand(xmm1, xmm5);

		if(region)
		{
			por(xmm1, xmm6);
		}

		// GSVector4i clamp = t.sat_i16(m_local.gd->t.min, m_local.gd->t.max);

		pmaxsw(uv0, xmm5);
		pminsw(uv0, xmm6);

		// clamp.blend8(repeat, m_local.gd->t.mask);

		#if _M_SSE >= 0x401

		pblendvb(uv0, xmm1);

		#else
		
		blendr(uv0, xmm1, xmm0);

		#endif

		// uv1

		// GSVector4i repeat = (t & m_local.gd->t.min) | m_local.gd->t.max;

		movdqa(xmm1, uv1);

		pand(xmm1, xmm5);

		if(region)
		{
			por(xmm1, xmm6);
		}

		// GSVector4i clamp = t.sat_i16(m_local.gd->t.min, m_local.gd->t.max);

		pmaxsw(uv1, xmm5);
		pminsw(uv1, xmm6);

		// clamp.blend8(repeat, m_local.gd->t.mask);

		#if _M_SSE >= 0x401
		
		pblendvb(uv1, xmm1);

		#else
		
		blendr(uv1, xmm1, xmm4);

		#endif
	}
}

void GSDrawScanlineCodeGenerator::AlphaTFX()
{
	if(!m_sel.fb)
	{
		return;
	}

	switch(m_sel.tfx)
	{
	case TFX_MODULATE:

		// GSVector4i ga = iip ? gaf : m_local.c.ga;

		movdqa(xmm4, ptr[m_sel.iip ? &m_local.temp.ga : &m_local.c.ga]);

		// gat = gat.modulate16<1>(ga).clamp8();

		modulate16(xmm6, xmm4, 1);

		clamp16(xmm6, xmm3);

		// if(!tcc) gat = gat.mix16(ga.srl16(7));

		if(!m_sel.tcc)
		{
			psrlw(xmm4, 7);

			mix16(xmm6, xmm4, xmm3);
		}

		break;

	case TFX_DECAL:

		// if(!tcc) gat = gat.mix16(ga.srl16(7));

		if(!m_sel.tcc)
		{
			// GSVector4i ga = iip ? gaf : m_local.c.ga;

			movdqa(xmm4, ptr[m_sel.iip ? &m_local.temp.ga : &m_local.c.ga]);

			psrlw(xmm4, 7);

			mix16(xmm6, xmm4, xmm3);
		}

		break;

	case TFX_HIGHLIGHT:

		// GSVector4i ga = iip ? gaf : m_local.c.ga;

		movdqa(xmm4, ptr[m_sel.iip ? &m_local.temp.ga : &m_local.c.ga]);
		movdqa(xmm2, xmm4);

		// gat = gat.mix16(!tcc ? ga.srl16(7) : gat.addus8(ga.srl16(7)));

		psrlw(xmm4, 7);

		if(m_sel.tcc)
		{
			paddusb(xmm4, xmm6);
		}

		mix16(xmm6, xmm4, xmm3);

		break;

	case TFX_HIGHLIGHT2:

		// if(!tcc) gat = gat.mix16(ga.srl16(7));

		if(!m_sel.tcc)
		{
			// GSVector4i ga = iip ? gaf : m_local.c.ga;

			movdqa(xmm4, ptr[m_sel.iip ? &m_local.temp.ga : &m_local.c.ga]);
			movdqa(xmm2, xmm4);

			psrlw(xmm4, 7);

			mix16(xmm6, xmm4, xmm3);
		}

		break;

	case TFX_NONE:

		// gat = iip ? ga.srl16(7) : ga;

		if(m_sel.iip)
		{
			psrlw(xmm6, 7);
		}

		break;
	}

	if(m_sel.aa1)
	{
		// gs_user figure 3-2: anti-aliasing after tfx, before tests, modifies alpha

		// FIXME: bios config screen cubes

		if(!m_sel.abe)
		{
			// a = cov

			if(m_sel.edge)
			{
				movdqa(xmm0, ptr[&m_local.temp.cov]);
			}
			else
			{
				pcmpeqd(xmm0, xmm0);
				psllw(xmm0, 15);
				psrlw(xmm0, 8);
			}

			mix16(xmm6, xmm0, xmm1);
		}
		else
		{
			// a = a == 0x80 ? cov : a

			pcmpeqd(xmm0, xmm0);
			psllw(xmm0, 15);
			psrlw(xmm0, 8);

			if(m_sel.edge)
			{
				movdqa(xmm1, ptr[&m_local.temp.cov]);
			}
			else
			{
				movdqa(xmm1, xmm0);
			}

			pcmpeqw(xmm0, xmm6);
			psrld(xmm0, 16);
			pslld(xmm0, 16);

			blend8(xmm6, xmm1);
		}
	}
}

void GSDrawScanlineCodeGenerator::ReadMask()
{
	if(m_sel.fwrite)
	{
		movdqa(xmm3, ptr[&m_local.gd->fm]);
	}

	if(m_sel.zwrite)
	{
		movdqa(xmm4, ptr[&m_local.gd->zm]);
	}
}

void GSDrawScanlineCodeGenerator::TestAlpha()
{
	switch(m_sel.afail)
	{
	case AFAIL_FB_ONLY:
		if(!m_sel.zwrite) return;
		break;

	case AFAIL_ZB_ONLY:
		if(!m_sel.fwrite) return;
		break;

	case AFAIL_RGB_ONLY:
		if(!m_sel.zwrite && m_sel.fpsm == 1) return;
		break;
	}

	switch(m_sel.atst)
	{
	case ATST_NEVER:
		// t = GSVector4i::xffffffff();
		pcmpeqd(xmm1, xmm1);
		break;

	case ATST_ALWAYS:
		return;

	case ATST_LESS:
	case ATST_LEQUAL:
		// t = (ga >> 16) > m_local.gd->aref;
		movdqa(xmm1, xmm6);
		psrld(xmm1, 16);
		pcmpgtd(xmm1, ptr[&m_local.gd->aref]);
		break;

	case ATST_EQUAL:
		// t = (ga >> 16) != m_local.gd->aref;
		movdqa(xmm1, xmm6);
		psrld(xmm1, 16);
		pcmpeqd(xmm1, ptr[&m_local.gd->aref]);
		pcmpeqd(xmm0, xmm0);
		pxor(xmm1, xmm0);
		break;

	case ATST_GEQUAL:
	case ATST_GREATER:
		// t = (ga >> 16) < m_local.gd->aref;
		movdqa(xmm0, xmm6);
		psrld(xmm0, 16);
		movdqa(xmm1, ptr[&m_local.gd->aref]);
		pcmpgtd(xmm1, xmm0);
		break;

	case ATST_NOTEQUAL:
		// t = (ga >> 16) == m_local.gd->aref;
		movdqa(xmm1, xmm6);
		psrld(xmm1, 16);
		pcmpeqd(xmm1, ptr[&m_local.gd->aref]);
		break;
	}

	switch(m_sel.afail)
	{
	case AFAIL_KEEP:
		// test |= t;
		por(xmm7, xmm1);
		alltrue();
		break;

	case AFAIL_FB_ONLY:
		// zm |= t;
		por(xmm4, xmm1);
		break;

	case AFAIL_ZB_ONLY:
		// fm |= t;
		por(xmm3, xmm1);
		break;

	case AFAIL_RGB_ONLY:
		// zm |= t;
		por(xmm4, xmm1);
		// fm |= t & GSVector4i::xff000000();
		psrld(xmm1, 24);
		pslld(xmm1, 24);
		por(xmm3, xmm1);
		break;
	}
}

void GSDrawScanlineCodeGenerator::ColorTFX()
{
	if(!m_sel.fwrite)
	{
		return;
	}

	switch(m_sel.tfx)
	{
	case TFX_MODULATE:

		// GSVector4i rb = iip ? rbf : m_local.c.rb;

		// rbt = rbt.modulate16<1>(rb).clamp8();

		modulate16(xmm5, ptr[m_sel.iip ? &m_local.temp.rb : &m_local.c.rb], 1);

		clamp16(xmm5, xmm1);

		break;

	case TFX_DECAL:

		break;

	case TFX_HIGHLIGHT:
	case TFX_HIGHLIGHT2:

		if(m_sel.tfx == TFX_HIGHLIGHT2 && m_sel.tcc)
		{
			// GSVector4i ga = iip ? gaf : m_local.c.ga;

			movdqa(xmm2, ptr[m_sel.iip ? &m_local.temp.ga : &m_local.c.ga]);
		}

		// gat = gat.modulate16<1>(ga).add16(af).clamp8().mix16(gat);

		movdqa(xmm1, xmm6);

		modulate16(xmm6, xmm2, 1);

		pshuflw(xmm2, xmm2, _MM_SHUFFLE(3, 3, 1, 1));
		pshufhw(xmm2, xmm2, _MM_SHUFFLE(3, 3, 1, 1));
		psrlw(xmm2, 7);

		paddw(xmm6, xmm2);

		clamp16(xmm6, xmm0);

		mix16(xmm6, xmm1, xmm0);

		// GSVector4i rb = iip ? rbf : m_local.c.rb;

		// rbt = rbt.modulate16<1>(rb).add16(af).clamp8();

		modulate16(xmm5, ptr[m_sel.iip ? &m_local.temp.rb : &m_local.c.rb], 1);

		paddw(xmm5, xmm2);

		clamp16(xmm5, xmm0);

		break;

	case TFX_NONE:

		// rbt = iip ? rb.srl16(7) : rb;

		if(m_sel.iip)
		{
			psrlw(xmm5, 7);
		}

		break;
	}
}

void GSDrawScanlineCodeGenerator::Fog()
{
	if(!m_sel.fwrite || !m_sel.fge)
	{
		return;
	}

	// rb = m_local.gd->frb.lerp16<0>(rb, f);
	// ga = m_local.gd->fga.lerp16<0>(ga, f).mix16(ga);

	movdqa(xmm0, ptr[m_sel.prim != GS_SPRITE_CLASS ? &m_local.temp.f : &m_local.p.f]);
	movdqa(xmm1, xmm6);

	movdqa(xmm2, ptr[&m_local.gd->frb]);
	lerp16(xmm5, xmm2, xmm0, 0);

	movdqa(xmm2, ptr[&m_local.gd->fga]);
	lerp16(xmm6, xmm2, xmm0, 0);
	mix16(xmm6, xmm1, xmm0);
}

void GSDrawScanlineCodeGenerator::ReadFrame()
{
	if(!m_sel.fb)
	{
		return;
	}

	// int fa = fza_base.x + fza_offset->x;

	mov(ebx, ptr[esi]);
	add(ebx, ptr[edi]);

	if(!m_sel.rfb)
	{
		return;
	}

	ReadPixel(xmm2, ebx);
}

void GSDrawScanlineCodeGenerator::TestDestAlpha()
{
	if(!m_sel.date || m_sel.fpsm != 0 && m_sel.fpsm != 2)
	{
		return;
	}

	// test |= ((fd [<< 16]) ^ m_local.gd->datm).sra32(31);

	movdqa(xmm1, xmm2);

	if(m_sel.datm)
	{
		if(m_sel.fpsm == 2)
		{
			pxor(xmm0, xmm0);
			psrld(xmm1, 15);
			pcmpeqd(xmm1, xmm0);
		}
		else
		{
			pcmpeqd(xmm0, xmm0);
			pxor(xmm1, xmm0);
			psrad(xmm1, 31);
		}
	}
	else
	{
		if(m_sel.fpsm == 2)
		{
			pslld(xmm1, 16);
		}

		psrad(xmm1, 31);
	}

	por(xmm7, xmm1);

	alltrue();
}

void GSDrawScanlineCodeGenerator::WriteMask()
{
	if(m_sel.notest)
	{
		return;
	}

	// fm |= test;
	// zm |= test;

	if(m_sel.fwrite)
	{
		por(xmm3, xmm7);
	}

	if(m_sel.zwrite)
	{
		por(xmm4, xmm7);
	}

	// int fzm = ~(fm == GSVector4i::xffffffff()).ps32(zm == GSVector4i::xffffffff()).mask();

	pcmpeqd(xmm1, xmm1);

	if(m_sel.fwrite && m_sel.zwrite)
	{
		movdqa(xmm0, xmm1);
		pcmpeqd(xmm1, xmm3);
		pcmpeqd(xmm0, xmm4);
		packssdw(xmm1, xmm0);
	}
	else if(m_sel.fwrite)
	{
		pcmpeqd(xmm1, xmm3);
		packssdw(xmm1, xmm1);
	}
	else if(m_sel.zwrite)
	{
		pcmpeqd(xmm1, xmm4);
		packssdw(xmm1, xmm1);
	}

	pmovmskb(edx, xmm1);

	not(edx);
}

void GSDrawScanlineCodeGenerator::WriteZBuf()
{
	if(!m_sel.zwrite)
	{
		return;
	}

	movdqa(xmm1, ptr[m_sel.prim != GS_SPRITE_CLASS ? &m_local.temp.zs : &m_local.p.z]);

	if(m_sel.ztest && m_sel.zpsm < 2)
	{
		// zs = zs.blend8(zd, zm);

		movdqa(xmm0, xmm4);
		movdqa(xmm7, ptr[&m_local.temp.zd]);
		blend8(xmm1, xmm7);
	}

	bool fast = m_sel.ztest ? m_sel.zpsm < 2 : m_sel.zpsm == 0 && m_sel.notest;

	WritePixel(xmm1, ebp, dh, fast, m_sel.zpsm, 1);
}

void GSDrawScanlineCodeGenerator::AlphaBlend()
{
	if(!m_sel.fwrite)
	{
		return;
	}

	if(m_sel.abe == 0 && m_sel.aa1 == 0)
	{
		return;
	}

	if((m_sel.aba != m_sel.abb) && (m_sel.aba == 1 || m_sel.abb == 1 || m_sel.abc == 1) || m_sel.abd == 1)
	{
		switch(m_sel.fpsm)
		{
		case 0:
		case 1:

			// c[2] = fd & mask;
			// c[3] = (fd >> 8) & mask;

			movdqa(xmm0, xmm2);
			movdqa(xmm1, xmm2);

			psllw(xmm0, 8);
			psrlw(xmm0, 8);
			psrlw(xmm1, 8);

			break;

		case 2:

			// c[2] = ((fd & 0x7c00) << 9) | ((fd & 0x001f) << 3);
			// c[3] = ((fd & 0x8000) << 8) | ((fd & 0x03e0) >> 2);

			movdqa(xmm0, xmm2);
			movdqa(xmm1, xmm2);
			movdqa(xmm4, xmm2);

			pcmpeqd(xmm7, xmm7);
			psrld(xmm7, 27); // 0x0000001f
			pand(xmm0, xmm7);
			pslld(xmm0, 3);

			pslld(xmm7, 10); // 0x00007c00
			pand(xmm4, xmm7);
			pslld(xmm4, 9);

			por(xmm0, xmm4);

			movdqa(xmm4, xmm1);

			psrld(xmm7, 5); // 0x000003e0
			pand(xmm1, xmm7);
			psrld(xmm1, 2);

			psllw(xmm7, 10); // 0x00008000
			pand(xmm4, xmm7);
			pslld(xmm4, 8);

			por(xmm1, xmm4);

			break;
		}
	}

	// xmm5, xmm6 = src rb, ga
	// xmm0, xmm1 = dst rb, ga
	// xmm2, xmm3 = used
	// xmm4, xmm7 = free

	if(m_sel.pabe || (m_sel.aba != m_sel.abb) && (m_sel.abb == 0 || m_sel.abd == 0))
	{
		movdqa(xmm4, xmm5);
	}

	if(m_sel.aba != m_sel.abb)
	{
		// rb = c[aba * 2 + 0];

		switch(m_sel.aba)
		{
		case 0: break;
		case 1: movdqa(xmm5, xmm0); break;
		case 2: pxor(xmm5, xmm5); break;
		}

		// rb = rb.sub16(c[abb * 2 + 0]);

		switch(m_sel.abb)
		{
		case 0: psubw(xmm5, xmm4); break;
		case 1: psubw(xmm5, xmm0); break;
		case 2: break;
		}

		if(!(m_sel.fpsm == 1 && m_sel.abc == 1))
		{
			// GSVector4i a = abc < 2 ? c[abc * 2 + 1].yywwlh().sll16(7) : m_local.gd->afix;

			switch(m_sel.abc)
			{
			case 0:
			case 1:
				pshuflw(xmm7, m_sel.abc ? xmm1 : xmm6, _MM_SHUFFLE(3, 3, 1, 1));
				pshufhw(xmm7, xmm7, _MM_SHUFFLE(3, 3, 1, 1));
				psllw(xmm7, 7);
				break;
			case 2:
				movdqa(xmm7, ptr[&m_local.gd->afix]);
				break;
			}

			// rb = rb.modulate16<1>(a);

			modulate16(xmm5, xmm7, 1);
		}

		// rb = rb.add16(c[abd * 2 + 0]);

		switch(m_sel.abd)
		{
		case 0: paddw(xmm5, xmm4); break;
		case 1: paddw(xmm5, xmm0); break;
		case 2: break;
		}
	}
	else
	{
		// rb = c[abd * 2 + 0];

		switch(m_sel.abd)
		{
		case 0: break;
		case 1: movdqa(xmm5, xmm0); break;
		case 2: pxor(xmm5, xmm5); break;
		}
	}

	if(m_sel.pabe)
	{
		// mask = (c[1] << 8).sra32(31);

		movdqa(xmm0, xmm6);
		pslld(xmm0, 8);
		psrad(xmm0, 31);

		// rb = c[0].blend8(rb, mask);

		blend8r(xmm5, xmm4);
	}

	// xmm6 = src ga
	// xmm1 = dst ga
	// xmm5 = rb
	// xmm7 = a
	// xmm2, xmm3 = used
	// xmm0, xmm4 = free

	movdqa(xmm4, xmm6);

	if(m_sel.aba != m_sel.abb)
	{
		// ga = c[aba * 2 + 1];

		switch(m_sel.aba)
		{
		case 0: break;
		case 1: movdqa(xmm6, xmm1); break;
		case 2: pxor(xmm6, xmm6); break;
		}

		// ga = ga.sub16(c[abeb * 2 + 1]);

		switch(m_sel.abb)
		{
		case 0: psubw(xmm6, xmm4); break;
		case 1: psubw(xmm6, xmm1); break;
		case 2: break;
		}

		if(!(m_sel.fpsm == 1 && m_sel.abc == 1))
		{
			// ga = ga.modulate16<1>(a);

			modulate16(xmm6, xmm7, 1);
		}

		// ga = ga.add16(c[abd * 2 + 1]);

		switch(m_sel.abd)
		{
		case 0: paddw(xmm6, xmm4); break;
		case 1: paddw(xmm6, xmm1); break;
		case 2: break;
		}
	}
	else
	{
		// ga = c[abd * 2 + 1];

		switch(m_sel.abd)
		{
		case 0: break;
		case 1: movdqa(xmm6, xmm1); break;
		case 2: pxor(xmm6, xmm6); break;
		}
	}

	// xmm4 = src ga
	// xmm5 = rb
	// xmm6 = ga
	// xmm2, xmm3 = used
	// xmm0, xmm1, xmm7 = free

	if(m_sel.pabe)
	{
		#if _M_SSE < 0x401

		// doh, previous blend8r overwrote xmm0 (sse41 uses pblendvb)

		movdqa(xmm0, xmm4);
		pslld(xmm0, 8);
		psrad(xmm0, 31);

		#endif

		psrld(xmm0, 16); // zero out high words to select the source alpha in blend (so it also does mix16)

		// ga = c[1].blend8(ga, mask).mix16(c[1]);

		blend8r(xmm6, xmm4);
	}
	else
	{
		if(m_sel.fpsm != 1) // TODO: fm == 0xffxxxxxx
		{
			mix16(xmm6, xmm4, xmm7);
		}
	}
}

void GSDrawScanlineCodeGenerator::WriteFrame()
{
	if(!m_sel.fwrite)
	{
		return;
	}

	if(m_sel.fpsm == 2 && m_sel.dthe)
	{
		mov(eax, ptr[esp + _top]);
		and(eax, 3);
		shl(eax, 5);
		mov(ebp, ptr[&m_local.gd->dimx]);
		paddw(xmm5, ptr[ebp + eax + sizeof(GSVector4i) * 0]);
		paddw(xmm6, ptr[ebp + eax + sizeof(GSVector4i) * 1]);
	}

	if(m_sel.colclamp == 0)
	{
		// c[0] &= 0x000000ff;
		// c[1] &= 0x000000ff;

		pcmpeqd(xmm7, xmm7);
		psrlw(xmm7, 8);
		pand(xmm5, xmm7);
		pand(xmm6, xmm7);
	}

	// GSVector4i fs = c[0].upl16(c[1]).pu16(c[0].uph16(c[1]));

	movdqa(xmm7, xmm5);
	punpcklwd(xmm5, xmm6);
	punpckhwd(xmm7, xmm6);
	packuswb(xmm5, xmm7);

	if(m_sel.fba && m_sel.fpsm != 1)
	{
		// fs |= 0x80000000;

		pcmpeqd(xmm7, xmm7);
		pslld(xmm7, 31);
		por(xmm5, xmm7);
	}

	if(m_sel.fpsm == 2)
	{
		// GSVector4i rb = fs & 0x00f800f8;
		// GSVector4i ga = fs & 0x8000f800;

		mov(eax, 0x00f800f8);
		movd(xmm6, eax);
		pshufd(xmm6, xmm6, _MM_SHUFFLE(0, 0, 0, 0));

		mov(eax, 0x8000f800);
		movd(xmm7, eax);
		pshufd(xmm7, xmm7, _MM_SHUFFLE(0, 0, 0, 0));

		movdqa(xmm4, xmm5);
		pand(xmm4, xmm6);
		pand(xmm5, xmm7);

		// fs = (ga >> 16) | (rb >> 9) | (ga >> 6) | (rb >> 3);

		movdqa(xmm6, xmm4);
		movdqa(xmm7, xmm5);

		psrld(xmm4, 3);
		psrld(xmm6, 9);
		psrld(xmm5, 6);
		psrld(xmm7, 16);

		por(xmm5, xmm4);
		por(xmm7, xmm6);
		por(xmm5, xmm7);
	}

	if(m_sel.rfb)
	{
		// fs = fs.blend(fd, fm);

		blend(xmm5, xmm2, xmm3); // TODO: could be skipped in certain cases, depending on fpsm and fm
	}

	bool fast = m_sel.rfb ? m_sel.fpsm < 2 : m_sel.fpsm == 0 && m_sel.notest;

	WritePixel(xmm5, ebx, dl, fast, m_sel.fpsm, 0);
}

void GSDrawScanlineCodeGenerator::ReadPixel(const Xmm& dst, const Reg32& addr)
{
	movq(dst, qword[addr * 2 + (size_t)m_local.gd->vm]);
	movhps(dst, qword[addr * 2 + (size_t)m_local.gd->vm + 8 * 2]);
}

void GSDrawScanlineCodeGenerator::WritePixel(const Xmm& src, const Reg32& addr, const Reg8& mask, bool fast, int psm, int fz)
{
	if(m_sel.notest)
	{
		if(fast)
		{
			movq(qword[addr * 2 + (size_t)m_local.gd->vm], src);
			movhps(qword[addr * 2 + (size_t)m_local.gd->vm + 8 * 2], src);
		}
		else
		{
			WritePixel(src, addr, 0, psm);
			WritePixel(src, addr, 1, psm);
			WritePixel(src, addr, 2, psm);
			WritePixel(src, addr, 3, psm);
		}
	}
	else
	{
		if(fast)
		{
			// if(fzm & 0x0f) GSVector4i::storel(&vm16[addr + 0], fs);
			// if(fzm & 0xf0) GSVector4i::storeh(&vm16[addr + 8], fs);

			test(mask, 0x0f);
			je("@f");
			movq(qword[addr * 2 + (size_t)m_local.gd->vm], src);
			L("@@");

			test(mask, 0xf0);
			je("@f");
			movhps(qword[addr * 2 + (size_t)m_local.gd->vm + 8 * 2], src);
			L("@@");
		}
		else
		{
			// if(fzm & 0x03) WritePixel(fpsm, &vm16[addr + 0], fs.extract32<0>());
			// if(fzm & 0x0c) WritePixel(fpsm, &vm16[addr + 2], fs.extract32<1>());
			// if(fzm & 0x30) WritePixel(fpsm, &vm16[addr + 8], fs.extract32<2>());
			// if(fzm & 0xc0) WritePixel(fpsm, &vm16[addr + 10], fs.extract32<3>());

			test(mask, 0x03);
			je("@f");
			WritePixel(src, addr, 0, psm);
			L("@@");

			test(mask, 0x0c);
			je("@f");
			WritePixel(src, addr, 1, psm);
			L("@@");

			test(mask, 0x30);
			je("@f");
			WritePixel(src, addr, 2, psm);
			L("@@");

			test(mask, 0xc0);
			je("@f");
			WritePixel(src, addr, 3, psm);
			L("@@");
		}
	}
}

static const int s_offsets[4] = {0, 2, 8, 10};

void GSDrawScanlineCodeGenerator::WritePixel(const Xmm& src, const Reg32& addr, uint8 i, int psm)
{
	Address dst = ptr[addr * 2 + (size_t)m_local.gd->vm + s_offsets[i] * 2];

	switch(psm)
	{
	case 0:
		if(i == 0) movd(dst, src);
		#if _M_SSE >= 0x401
		else pextrd(dst, src, i);
		#else
		else {pshufd(xmm0, src, _MM_SHUFFLE(i, i, i, i)); movd(dst, xmm0);}
		#endif
		break;
	case 1:
		if(i == 0) movd(eax, src);
		#if _M_SSE >= 0x401
		else pextrd(eax, src, i);
		#else
		else {pshufd(xmm0, src, _MM_SHUFFLE(i, i, i, i)); movd(eax, xmm0);}
		#endif
		xor(eax, dst);
		and(eax, 0xffffff);
		xor(dst, eax);
		break;
	case 2:
		if(i == 0) movd(eax, src);
		else pextrw(eax, src, i * 2);
		mov(dst, ax);
		break;
	}
}

void GSDrawScanlineCodeGenerator::ReadTexel(int pixels, int mip_offset)
{
	// in
	// xmm5 = addr00
	// xmm2 = addr01
	// xmm0 = addr10
	// xmm3 = addr11
	// ebx = m_local.tex[0] (!m_sel.mmin)
	// ebp = m_local.tex (m_sel.mmin)
	// edx = m_local.clut (m_sel.tlu)

	// out
	// xmm6 = c00
	// xmm4 = c01
	// xmm1 = c10
	// xmm5 = c11

	ASSERT(pixels == 1 || pixels == 4);

	mip_offset *= sizeof(void*);

	const GSVector4i* lod_i = m_sel.lcm ? &m_local.gd->lod.i : &m_local.temp.lod.i;

	if(m_sel.mmin && !m_sel.lcm)
	{
		#if _M_SSE >= 0x401

		const int r[] = {5, 6, 2, 4, 0, 1, 3, 7};

		if(pixels == 4)
		{
			movdqa(ptr[&m_local.temp.test], xmm7);
		}

		for(int j = 0; j < 4; j++)
		{
			mov(ebx, ptr[&lod_i->u32[j]]);
			mov(ebx, ptr[ebp + ebx * sizeof(void*) + mip_offset]);

			for(int i = 0; i < pixels; i++)
			{
				ReadTexel(Xmm(r[i * 2 + 1]), Xmm(r[i * 2 + 0]), j);
			}
		}

		if(pixels == 4)
		{
			movdqa(xmm5, xmm7);
			movdqa(xmm7, ptr[&m_local.temp.test]);
		}

		#else

		if(pixels == 4)
		{
			movdqa(ptr[&m_local.temp.test], xmm7);

			mov(ebx, ptr[&lod_i->u32[0]]);
			mov(ebx, ptr[ebp + ebx * sizeof(void*) + mip_offset]);

			ReadTexel(xmm6, xmm5, 0);
			psrldq(xmm5, 4);
			ReadTexel(xmm4, xmm2, 0);
			psrldq(xmm2, 4);

			mov(ebx, ptr[&lod_i->u32[1]]);
			mov(ebx, ptr[ebp + ebx * sizeof(void*) + mip_offset]);

			ReadTexel(xmm1, xmm5, 0);
			psrldq(xmm5, 4);
			ReadTexel(xmm7, xmm2, 0);
			psrldq(xmm2, 4);

			punpckldq(xmm6, xmm1);
			punpckldq(xmm4, xmm7);

			mov(ebx, ptr[&lod_i->u32[2]]);
			mov(ebx, ptr[ebp + ebx * sizeof(void*) + mip_offset]);

			ReadTexel(xmm1, xmm5, 0);
			psrldq(xmm5, 4);
			ReadTexel(xmm7, xmm2, 0);
			psrldq(xmm2, 4);

			mov(ebx, ptr[&lod_i->u32[3]]);
			mov(ebx, ptr[ebp + ebx * sizeof(void*) + mip_offset]);

			ReadTexel(xmm5, xmm5, 0);
			ReadTexel(xmm2, xmm2, 0);

			punpckldq(xmm1, xmm5);
			punpckldq(xmm7, xmm2);

			punpcklqdq(xmm6, xmm1);
			punpcklqdq(xmm4, xmm7);

			mov(ebx, ptr[&lod_i->u32[0]]);
			mov(ebx, ptr[ebp + ebx * sizeof(void*) + mip_offset]);

			ReadTexel(xmm1, xmm0, 0);
			psrldq(xmm0, 4);
			ReadTexel(xmm5, xmm3, 0);
			psrldq(xmm3, 4);

			mov(ebx, ptr[&lod_i->u32[1]]);
			mov(ebx, ptr[ebp + ebx * sizeof(void*) + mip_offset]);

			ReadTexel(xmm2, xmm0, 0);
			psrldq(xmm0, 4);
			ReadTexel(xmm7, xmm3, 0);
			psrldq(xmm3, 4);

			punpckldq(xmm1, xmm2);
			punpckldq(xmm5, xmm7);

			mov(ebx, ptr[&lod_i->u32[2]]);
			mov(ebx, ptr[ebp + ebx * sizeof(void*) + mip_offset]);

			ReadTexel(xmm2, xmm0, 0);
			psrldq(xmm0, 4);
			ReadTexel(xmm7, xmm3, 0);
			psrldq(xmm3, 4);

			mov(ebx, ptr[&lod_i->u32[3]]);
			mov(ebx, ptr[ebp + ebx * sizeof(void*) + mip_offset]);

			ReadTexel(xmm0, xmm0, 0);
			ReadTexel(xmm3, xmm3, 0);

			punpckldq(xmm2, xmm0);
			punpckldq(xmm7, xmm3);

			punpcklqdq(xmm1, xmm2);
			punpcklqdq(xmm5, xmm7);

			movdqa(xmm7, ptr[&m_local.temp.test]);
		}
		else
		{
			mov(ebx, ptr[&lod_i->u32[0]]);
			mov(ebx, ptr[ebp + ebx * sizeof(void*) + mip_offset]);

			ReadTexel(xmm6, xmm5, 0);
			psrldq(xmm5, 4); // shuffle instead? (1 2 3 0 ~ rotation)

			mov(ebx, ptr[&lod_i->u32[1]]);
			mov(ebx, ptr[ebp + ebx * sizeof(void*) + mip_offset]);

			ReadTexel(xmm1, xmm5, 0);
			psrldq(xmm5, 4);

			punpckldq(xmm6, xmm1);

			mov(ebx, ptr[&lod_i->u32[2]]);
			mov(ebx, ptr[ebp + ebx * sizeof(void*) + mip_offset]);

			ReadTexel(xmm1, xmm5, 0);
			psrldq(xmm5, 4);

			mov(ebx, ptr[&lod_i->u32[3]]);
			mov(ebx, ptr[ebp + ebx * sizeof(void*) + mip_offset]);

			ReadTexel(xmm4, xmm5, 0);
			// psrldq(xmm5, 4);

			punpckldq(xmm1, xmm4);

			punpcklqdq(xmm6, xmm1);
		}

		#endif
	}
	else
	{
		if(m_sel.mmin && m_sel.lcm)
		{
			mov(ebx, ptr[&lod_i->u32[0]]);
			mov(ebx, ptr[ebp + ebx * sizeof(void*) + mip_offset]);
		}

		const int r[] = {5, 6, 2, 4, 0, 1, 3, 5};

		#if _M_SSE >= 0x401

		for(int i = 0; i < pixels; i++)
		{
			for(int j = 0; j < 4; j++)
			{
				ReadTexel(Xmm(r[i * 2 + 1]), Xmm(r[i * 2 + 0]), j);
			}
		}
		
		#else
		
		const int t[] = {1, 4, 1, 5, 2, 5, 2, 0};

		for(int i = 0; i < pixels; i++)
		{
			const Xmm& addr = Xmm(r[i * 2 + 0]);
			const Xmm& dst = Xmm(r[i * 2 + 1]);
			const Xmm& temp1 = Xmm(t[i * 2 + 0]);
			const Xmm& temp2 = Xmm(t[i * 2 + 1]);

			ReadTexel(dst, addr, 0);
			psrldq(addr, 4); // shuffle instead? (1 2 3 0 ~ rotation)
			ReadTexel(temp1, addr, 0);
			psrldq(addr, 4);
			punpckldq(dst, temp1);

			ReadTexel(temp1, addr, 0);
			psrldq(addr, 4);
			ReadTexel(temp2, addr, 0);
			// psrldq(addr, 4);
			punpckldq(temp1, temp2);

			punpcklqdq(dst, temp1);
		}

		#endif
	}
}

void GSDrawScanlineCodeGenerator::ReadTexel(const Xmm& dst, const Xmm& addr, uint8 i)
{
	const Address& src = m_sel.tlu ? ptr[edx + eax * 4] : ptr[ebx + eax * 4];

	#if _M_SSE < 0x401
	
	ASSERT(i == 0);

	#endif

	if(i == 0) movd(eax, addr);
	else pextrd(eax, addr, i);

	if(m_sel.tlu) movzx(eax, byte[ebx + eax]);

	if(i == 0) movd(dst, src);
	else pinsrd(dst, src, i);
}

#endif