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

#if _M_SSE >= 0x501 && !(defined(_M_AMD64) || defined(_WIN64))

static const int _args = 16;
static const int _top = _args + 4;
static const int _v = _args + 8;

void GSDrawScanlineCodeGenerator::Generate()
{
//ret(8);

	push(ebx);
	push(esi);
	push(edi);
	push(ebp);

	//db(0xcc);

	Init();

	if(!m_sel.edge)
	{
		align(16);
	}

L("loop");

	// ecx = steps
	// esi = fzbr
	// edi = fzbc
	// ymm0 = z/zi
	// ymm2 = s/u (tme)
	// ymm3 = t/v (tme)
	// ymm4 = q (tme)
	// ymm5 = rb (!tme)
	// ymm6 = ga (!tme)
	// ymm7 = test

	bool tme = m_sel.tfx != TFX_NONE;

	TestZ(tme ? ymm5 : ymm2, tme ? ymm6 : ymm3);

	// ecx = steps
	// esi = fzbr
	// edi = fzbc
	// ebp = za
	// - ymm0
	// ymm2 = s/u (tme)
	// ymm3 = t/v (tme)
	// ymm4 = q (tme)
	// ymm5 = rb (!tme)
	// ymm6 = ga (!tme)
	// ymm7 = test

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
	// - ymm2
	// - ymm3
	// - ymm4
	// ymm5 = rb
	// ymm6 = ga
	// ymm7 = test

	AlphaTFX();

	// ecx = steps
	// esi = fzbr
	// edi = fzbc
	// ebp = za
	// ymm2 = gaf (TFX_HIGHLIGHT || TFX_HIGHLIGHT2 && !tcc)
	// ymm5 = rb
	// ymm6 = ga
	// ymm7 = test

	ReadMask();

	// ecx = steps
	// esi = fzbr
	// edi = fzbc
	// ebp = za
	// ymm2 = gaf (TFX_HIGHLIGHT || TFX_HIGHLIGHT2 && !tcc)
	// ymm3 = fm
	// ymm4 = zm
	// ymm5 = rb
	// ymm6 = ga
	// ymm7 = test

	TestAlpha();

	// ecx = steps
	// esi = fzbr
	// edi = fzbc
	// ebp = za
	// ymm2 = gaf (TFX_HIGHLIGHT || TFX_HIGHLIGHT2 && !tcc)
	// ymm3 = fm
	// ymm4 = zm
	// ymm5 = rb
	// ymm6 = ga
	// ymm7 = test

	ColorTFX();

	// ecx = steps
	// esi = fzbr
	// edi = fzbc
	// ebp = za
	// ymm3 = fm
	// ymm4 = zm
	// ymm5 = rb
	// ymm6 = ga
	// ymm7 = test

	Fog();

	// ecx = steps
	// esi = fzbr
	// edi = fzbc
	// ebp = za
	// ymm3 = fm
	// ymm4 = zm
	// ymm5 = rb
	// ymm6 = ga
	// ymm7 = test

	ReadFrame();

	// ecx = steps
	// esi = fzbr
	// edi = fzbc
	// ebp = za
	// ymm2 = fd
	// ymm3 = fm
	// ymm4 = zm
	// ymm5 = rb
	// ymm6 = ga
	// ymm7 = test

	TestDestAlpha();

	// ecx = steps
	// esi = fzbr
	// edi = fzbc
	// ebp = za
	// ymm2 = fd
	// ymm3 = fm
	// ymm4 = zm
	// ymm5 = rb
	// ymm6 = ga
	// ymm7 = test

	WriteMask();

	// ebx = fa
	// ecx = steps
	// edx = fzm
	// esi = fzbr
	// edi = fzbc
	// ebp = za
	// ymm2 = fd
	// ymm3 = fm
	// ymm4 = zm
	// ymm5 = rb
	// ymm6 = ga

	WriteZBuf();

	// ebx = fa
	// ecx = steps
	// edx = fzm
	// esi = fzbr
	// edi = fzbc
	// - ebp
	// ymm2 = fd
	// ymm3 = fm
	// - ymm4
	// ymm5 = rb
	// ymm6 = ga

	AlphaBlend();

	// ebx = fa
	// ecx = steps
	// edx = fzm
	// esi = fzbr
	// edi = fzbc
	// ymm2 = fd
	// ymm3 = fm
	// ymm5 = rb
	// ymm6 = ga

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
		// int skip = left & 7;

		mov(ebx, edx);
		and(edx, 7);

		// int steps = pixels + skip - 8;

		lea(ecx, ptr[ecx + edx - 8]);

		// left -= skip;

		sub(ebx, edx);

		// GSVector4i test = m_test[skip] | m_test[15 + (steps & (steps >> 31))];
		
		mov(eax, ecx);
		sar(eax, 31);
		and(eax, ecx);

		vpmovsxbd(ymm7, ptr[edx * 8 + (size_t)&m_test[0]]);
		vpmovsxbd(ymm0, ptr[eax * 8 + (size_t)&m_test[15]]);
		vpor(ymm7, ymm0);

		shl(edx, 5);
	}
	else
	{
		mov(ebx, edx); // left
		xor(edx, edx); // skip
		lea(ecx, ptr[ecx - 8]); // steps
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
			vbroadcastf128(ymm0, ptr[ebx + offsetof(GSVertexSW, p)]); // v.p

			if(m_sel.fwrite && m_sel.fge)
			{
				// f = GSVector8i(vp).zzzzh().zzzz().add16(m_local.d[skip].f);

				vcvttps2dq(ymm1, ymm0);
				vpshufhw(ymm1, ymm1, _MM_SHUFFLE(2, 2, 2, 2));
				vpshufd(ymm1, ymm1, _MM_SHUFFLE(2, 2, 2, 2));
				vpaddw(ymm1, ptr[edx + offsetof(GSScanlineLocalData::skip, f)]);

				vmovdqa(ptr[&m_local.temp.f], ymm1);
			}

			if(m_sel.zb)
			{
				// z = vp.zzzz() + m_local.d[skip].z;

				vshufps(ymm0, ymm0, _MM_SHUFFLE(2, 2, 2, 2));
				vmovaps(ptr[&m_local.temp.z], ymm0);
				vmovaps(ymm2, ptr[edx + offsetof(GSScanlineLocalData::skip, z)]);
				vmovaps(ptr[&m_local.temp.zo], ymm2);
				vaddps(ymm0, ymm2);
			}
		}
	}
	else
	{
		if(m_sel.ztest)
		{
			vpbroadcastd(ymm0, ptr[&m_local.p.z]);
		}
	}

	if(m_sel.fb)
	{
		if(m_sel.edge || m_sel.tfx != TFX_NONE)
		{
			vbroadcastf128(ymm4, ptr[ebx + offsetof(GSVertexSW, t)]); // v.t
		}

		if(m_sel.edge)
		{
			// m_local.temp.cov = GSVector4i::cast(v.t).zzzzh().wwww().srl16(9);

			vpshufhw(ymm3, ymm4, _MM_SHUFFLE(2, 2, 2, 2));
			vpshufd(ymm3, ymm3, _MM_SHUFFLE(3, 3, 3, 3));
			vpsrlw(ymm3, 9);

			vmovdqa(ptr[&m_local.temp.cov], ymm3);
		}

		if(m_sel.tfx != TFX_NONE)
		{
			if(m_sel.fst)
			{
				// GSVector4i vti(vt);

				vcvttps2dq(ymm6, ymm4);

				// s = vti.xxxx() + m_local.d[skip].s;
				// t = vti.yyyy(); if(!sprite) t += m_local.d[skip].t;

				vpshufd(ymm2, ymm6, _MM_SHUFFLE(0, 0, 0, 0));
				vpshufd(ymm3, ymm6, _MM_SHUFFLE(1, 1, 1, 1));

				vpaddd(ymm2, ptr[edx + offsetof(GSScanlineLocalData::skip, s)]);

				if(m_sel.prim != GS_SPRITE_CLASS || m_sel.mmin)
				{
					vpaddd(ymm3, ptr[edx + offsetof(GSScanlineLocalData::skip, t)]);
				}
				else
				{
					if(m_sel.ltf)
					{
						vpshuflw(ymm6, ymm3, _MM_SHUFFLE(2, 2, 0, 0));
						vpshufhw(ymm6, ymm6, _MM_SHUFFLE(2, 2, 0, 0));
						vpsrlw(ymm6, 12);
						vmovdqa(ptr[&m_local.temp.vf], ymm6);
					}
				}

				vmovdqa(ptr[&m_local.temp.s], ymm2);
				vmovdqa(ptr[&m_local.temp.t], ymm3);
			}
			else
			{
				// s = vt.xxxx() + m_local.d[skip].s;
				// t = vt.yyyy() + m_local.d[skip].t;
				// q = vt.zzzz() + m_local.d[skip].q;

				vshufps(ymm2, ymm4, ymm4, _MM_SHUFFLE(0, 0, 0, 0));
				vshufps(ymm3, ymm4, ymm4, _MM_SHUFFLE(1, 1, 1, 1));
				vshufps(ymm4, ymm4, ymm4, _MM_SHUFFLE(2, 2, 2, 2));

				vaddps(ymm2, ptr[edx + offsetof(GSScanlineLocalData::skip, s)]);
				vaddps(ymm3, ptr[edx + offsetof(GSScanlineLocalData::skip, t)]);
				vaddps(ymm4, ptr[edx + offsetof(GSScanlineLocalData::skip, q)]);

				vmovaps(ptr[&m_local.temp.s], ymm2);
				vmovaps(ptr[&m_local.temp.t], ymm3);
				vmovaps(ptr[&m_local.temp.q], ymm4);
			}
		}

		if(!(m_sel.tfx == TFX_DECAL && m_sel.tcc))
		{
			if(m_sel.iip)
			{
				// GSVector4i vc = GSVector4i(v.c);

				vbroadcastf128(ymm6, ptr[ebx + offsetof(GSVertexSW, c)]); // v.c
				vcvttps2dq(ymm6, ymm6);

				// vc = vc.upl16(vc.zwxy());

				vpshufd(ymm5, ymm6, _MM_SHUFFLE(1, 0, 3, 2));
				vpunpcklwd(ymm6, ymm5);

				// rb = vc.xxxx().add16(m_local.d[skip].rb);
				// ga = vc.zzzz().add16(m_local.d[skip].ga);

				vpshufd(ymm5, ymm6, _MM_SHUFFLE(0, 0, 0, 0));
				vpshufd(ymm6, ymm6, _MM_SHUFFLE(2, 2, 2, 2));

				vpaddw(ymm5, ptr[edx + offsetof(GSScanlineLocalData::skip, rb)]);
				vpaddw(ymm6, ptr[edx + offsetof(GSScanlineLocalData::skip, ga)]);

				vmovdqa(ptr[&m_local.temp.rb], ymm5);
				vmovdqa(ptr[&m_local.temp.ga], ymm6);
			}
			else
			{
				if(m_sel.tfx == TFX_NONE)
				{
					vmovdqa(ymm5, ptr[&m_local.c.rb]);
					vmovdqa(ymm6, ptr[&m_local.c.ga]);
				}
			}
		}
	}
}

void GSDrawScanlineCodeGenerator::Step()
{
	// steps -= 8;

	sub(ecx, 8);

	// fza_offset += 2;

	add(edi, 16);

	if(m_sel.prim != GS_SPRITE_CLASS)
	{
		// z += m_local.d8.z;

		if(m_sel.zb)
		{
			vmovaps(ymm0, ptr[&m_local.temp.zo]);
			vaddps(ymm0, ptr[&m_local.d8.z]);
			vmovaps(ptr[&m_local.temp.zo], ymm0);
			vaddps(ymm0, ptr[&m_local.temp.z]);
		}

		// f = f.add16(m_local.d4.f);

		if(m_sel.fwrite && m_sel.fge)
		{
			vmovdqa(ymm1, ptr[&m_local.temp.f]);
			vpaddw(ymm1, ptr[&m_local.d8.f]);
			vmovdqa(ptr[&m_local.temp.f], ymm1);
		}
	}
	else
	{
		if(m_sel.ztest)
		{
			vpbroadcastd(ymm0, ptr[&m_local.p.z]);
		}
	}

	if(m_sel.fb)
	{
		if(m_sel.tfx != TFX_NONE)
		{
			if(m_sel.fst)
			{
				// GSVector8i stq = m_local.d8.stq;

				// s += stq.xxxx();
				// if(!sprite) t += stq.yyyy();

				vmovdqa(ymm4, ptr[&m_local.d8.stq]);

				vpshufd(ymm2, ymm4, _MM_SHUFFLE(0, 0, 0, 0));
				vpaddd(ymm2, ptr[&m_local.temp.s]);
				vmovdqa(ptr[&m_local.temp.s], ymm2);

				if(m_sel.prim != GS_SPRITE_CLASS || m_sel.mmin)
				{
					vpshufd(ymm3, ymm4, _MM_SHUFFLE(1, 1, 1, 1));
					vpaddd(ymm3, ptr[&m_local.temp.t]);
					vmovdqa(ptr[&m_local.temp.t], ymm3);
				}
				else
				{
					vmovdqa(ymm3, ptr[&m_local.temp.t]);
				}
			}
			else
			{
				// GSVector8 stq = m_local.d8.stq;

				// s += stq.xxxx();
				// t += stq.yyyy();
				// q += stq.zzzz();

				vmovaps(ymm4, ptr[&m_local.d8.stq]);

				vshufps(ymm2, ymm4, ymm4, _MM_SHUFFLE(0, 0, 0, 0));
				vshufps(ymm3, ymm4, ymm4, _MM_SHUFFLE(1, 1, 1, 1));
				vshufps(ymm4, ymm4, ymm4, _MM_SHUFFLE(2, 2, 2, 2));

				vaddps(ymm2, ptr[&m_local.temp.s]);
				vaddps(ymm3, ptr[&m_local.temp.t]);
				vaddps(ymm4, ptr[&m_local.temp.q]);

				vmovaps(ptr[&m_local.temp.s], ymm2);
				vmovaps(ptr[&m_local.temp.t], ymm3);
				vmovaps(ptr[&m_local.temp.q], ymm4);
			}
		}

		if(!(m_sel.tfx == TFX_DECAL && m_sel.tcc))
		{
			if(m_sel.iip)
			{
				// GSVector8i c = m_local.d8.c;

				// rb = rb.add16(c.xxxx());
				// ga = ga.add16(c.yyyy());

				vmovdqa(ymm7, ptr[&m_local.d8.c]);

				vpshufd(ymm5, ymm7, _MM_SHUFFLE(0, 0, 0, 0));
				vpshufd(ymm6, ymm7, _MM_SHUFFLE(1, 1, 1, 1));

				vpaddw(ymm5, ptr[&m_local.temp.rb]);
				vpaddw(ymm6, ptr[&m_local.temp.ga]);

				// FIXME: color may underflow and roll over at the end of the line, if decreasing

				vpxor(ymm7, ymm7);
				vpmaxsw(ymm5, ymm7);
				vpmaxsw(ymm6, ymm7);

				vmovdqa(ptr[&m_local.temp.rb], ymm5);
				vmovdqa(ptr[&m_local.temp.ga], ymm6);
			}
			else
			{
				if(m_sel.tfx == TFX_NONE)
				{
					vmovdqa(ymm5, ptr[&m_local.c.rb]);
					vmovdqa(ymm6, ptr[&m_local.c.ga]);
				}
			}
		}
	}

	if(!m_sel.notest)
	{
		// test = m_test[15 + (steps & (steps >> 31))];

		mov(edx, ecx);
		sar(edx, 31);
		and(edx, ecx);

		vpmovsxbd(ymm7, ptr[edx * 8 + (size_t)&m_test[15]]);
	}
}

void GSDrawScanlineCodeGenerator::TestZ(const Ymm& temp1, const Ymm& temp2)
{
	if(!m_sel.zb)
	{
		return;
	}

	// int za = fza_base.y + fza_offset->y;

	mov(ebp, ptr[esi + 4]);
	add(ebp, ptr[edi + 4]);

	// GSVector8i zs = zi;

	if(m_sel.prim != GS_SPRITE_CLASS)
	{
		if(m_sel.zoverflow)
		{
			// zs = (GSVector8i(z * 0.5f) << 1) | (GSVector8i(z) & GSVector8i::x00000001());

			vbroadcastss(temp1, ptr[&GSVector8::m_half]);
			vmulps(temp1, ymm0);
			vcvttps2dq(temp1, temp1);
			vpslld(temp1, 1);

			vcvttps2dq(ymm0, ymm0);
			vpcmpeqd(temp2, temp2);
			vpsrld(temp2, 31);
			vpand(ymm0, temp2);

			vpor(ymm0, temp1);
		}
		else
		{
			// zs = GSVector8i(z);

			vcvttps2dq(ymm0, ymm0);
		}

		if(m_sel.zwrite)
		{
			vmovdqa(ptr[&m_local.temp.zs], ymm0);
		}
	}

	if(m_sel.ztest)
	{
		ReadPixel(ymm1, temp1, ebp);

		if(m_sel.zwrite && m_sel.zpsm < 2)
		{
			vmovdqa(ptr[&m_local.temp.zd], ymm1);
		}

		// zd &= 0xffffffff >> m_sel.zpsm * 8;

		if(m_sel.zpsm)
		{
			vpslld(ymm1, (uint8)(m_sel.zpsm * 8));
			vpsrld(ymm1, (uint8)(m_sel.zpsm * 8));
		}

		if(m_sel.zoverflow || m_sel.zpsm == 0)
		{
			// GSVector8i o = GSVector8i::x80000000();

			vpcmpeqd(temp1, temp1);
			vpslld(temp1, 31);

			// GSVector8i zso = zs - o;
			// GSVector8i zdo = zd - o;

			vpsubd(ymm0, temp1);
			vpsubd(ymm1, temp1);
		}

		switch(m_sel.ztst)
		{
		case ZTST_GEQUAL:
			// test |= zso < zdo; // ~(zso >= zdo)
			vpcmpgtd(ymm1, ymm0);
			vpor(ymm7, ymm1);
			break;

		case ZTST_GREATER: // TODO: tidus hair and chocobo wings only appear fully when this is tested as ZTST_GEQUAL
			// test |= zso <= zdo; // ~(zso > zdo)
			vpcmpgtd(ymm0, ymm1);
			vpcmpeqd(temp1, temp1);
			vpxor(ymm0, temp1);
			vpor(ymm7, ymm0);
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
		vrcpps(ymm0, ymm4);

		vmulps(ymm2, ymm0);
		vmulps(ymm3, ymm0);

		vcvttps2dq(ymm2, ymm2);
		vcvttps2dq(ymm3, ymm3);

		if(m_sel.ltf)
		{
			// u -= 0x8000;
			// v -= 0x8000;

			mov(eax, 0x8000);
			vmovd(xmm4, eax);
			vpbroadcastd(ymm4, xmm4);

			vpsubd(ymm2, ymm4);
			vpsubd(ymm3, ymm4);
		}
	}

	// ymm2 = u
	// ymm3 = v

	if(m_sel.ltf)
	{
		// GSVector8i uf = u.xxzzlh().srl16(1);

		vpshuflw(ymm0, ymm2, _MM_SHUFFLE(2, 2, 0, 0));
		vpshufhw(ymm0, ymm0, _MM_SHUFFLE(2, 2, 0, 0));
		vpsrlw(ymm0, 12);
		vmovdqa(ptr[&m_local.temp.uf], ymm0);

		if(m_sel.prim != GS_SPRITE_CLASS)
		{
			// GSVector8i vf = v.xxzzlh().srl16(1);

			vpshuflw(ymm0, ymm3, _MM_SHUFFLE(2, 2, 0, 0));
			vpshufhw(ymm0, ymm0, _MM_SHUFFLE(2, 2, 0, 0));
			vpsrlw(ymm0, 12);
			vmovdqa(ptr[&m_local.temp.vf], ymm0);
		}
	}

	// GSVector8i uv0 = u.sra32(16).ps32(v.sra32(16));

	vpsrad(ymm2, 16);
	vpsrad(ymm3, 16);
	vpackssdw(ymm2, ymm3);

	if(m_sel.ltf)
	{
		// GSVector8i uv1 = uv0.add16(GSVector8i::x0001());

		vpcmpeqd(ymm1, ymm1);
		vpsrlw(ymm1, 15);
		vpaddw(ymm3, ymm2, ymm1);

		// uv0 = Wrap(uv0);
		// uv1 = Wrap(uv1);

		Wrap(ymm2, ymm3);
	}
	else
	{
		// uv0 = Wrap(uv0);

		Wrap(ymm2);
	}

	// ymm2 = uv0
	// ymm3 = uv1 (ltf)
	// ymm0, ymm1, ymm4, ymm5, ymm6 = free
	// ymm7 = used

	// GSVector8i y0 = uv0.uph16() << tw;
	// GSVector8i x0 = uv0.upl16();

	vpxor(ymm0, ymm0);

	vpunpcklwd(ymm4, ymm2, ymm0);
	vpunpckhwd(ymm2, ymm2, ymm0);
	vpslld(ymm2, (uint8)(m_sel.tw + 3));

	// ymm0 = 0
	// ymm2 = y0
	// ymm3 = uv1 (ltf)
	// ymm4 = x0
	// ymm1, ymm5, ymm6 = free
	// ymm7 = used

	if(m_sel.ltf)
	{
		// GSVector8i y1 = uv1.uph16() << tw;
		// GSVector8i x1 = uv1.upl16();

		vpunpcklwd(ymm6, ymm3, ymm0);
		vpunpckhwd(ymm3, ymm3, ymm0);
		vpslld(ymm3, (uint8)(m_sel.tw + 3));

		// ymm2 = y0
		// ymm3 = y1
		// ymm4 = x0
		// ymm6 = x1
		// ymm0, ymm5, ymm6 = free
		// ymm7 = used

		// GSVector8i addr00 = y0 + x0;
		// GSVector8i addr01 = y0 + x1;
		// GSVector8i addr10 = y1 + x0;
		// GSVector8i addr11 = y1 + x1;

		vpaddd(ymm5, ymm2, ymm4);
		vpaddd(ymm2, ymm2, ymm6);
		vpaddd(ymm0, ymm3, ymm4);
		vpaddd(ymm3, ymm3, ymm6);

		// ymm5 = addr00
		// ymm2 = addr01
		// ymm0 = addr10
		// ymm3 = addr11
		// ymm1, ymm4, ymm6 = free
		// ymm7 = used

		// c00 = addr00.gather32_32((const uint32/uint8*)tex[, clut]);
		// c01 = addr01.gather32_32((const uint32/uint8*)tex[, clut]);
		// c10 = addr10.gather32_32((const uint32/uint8*)tex[, clut]);
		// c11 = addr11.gather32_32((const uint32/uint8*)tex[, clut]);

		ReadTexel(4, 0);

		// ymm6 = c00
		// ymm4 = c01
		// ymm1 = c10
		// ymm5 = c11
		// ymm0, ymm2, ymm3 = free
		// ymm7 = used

		vmovdqa(ymm0, ptr[&m_local.temp.uf]);

		// GSVector8i rb00 = c00 & mask;
		// GSVector8i ga00 = (c00 >> 8) & mask;

		vpsllw(ymm2, ymm6, 8);
		vpsrlw(ymm2, 8);
		vpsrlw(ymm6, 8);

		// GSVector8i rb01 = c01 & mask;
		// GSVector8i ga01 = (c01 >> 8) & mask;

		vpsllw(ymm3, ymm4, 8);
		vpsrlw(ymm3, 8);
		vpsrlw(ymm4, 8);

		// ymm0 = uf
		// ymm2 = rb00
		// ymm3 = rb01
		// ymm6 = ga00
		// ymm4 = ga01
		// ymm1 = c10
		// ymm5 = c11
		// ymm7 = used

		// rb00 = rb00.lerp16_4(rb01, uf);
		// ga00 = ga00.lerp16_4(ga01, uf);

		lerp16_4(ymm3, ymm2, ymm0);
		lerp16_4(ymm4, ymm6, ymm0);

		// ymm0 = uf
		// ymm3 = rb00
		// ymm4 = ga00
		// ymm1 = c10
		// ymm5 = c11
		// ymm2, ymm6 = free
		// ymm7 = used

		// GSVector8i rb10 = c10 & mask;
		// GSVector8i ga10 = (c10 >> 8) & mask;

		vpsrlw(ymm2, ymm1, 8);
		vpsllw(ymm1, 8);
		vpsrlw(ymm1, 8);

		// GSVector8i rb11 = c11 & mask;
		// GSVector8i ga11 = (c11 >> 8) & mask;

		vpsrlw(ymm6, ymm5, 8);
		vpsllw(ymm5, 8);
		vpsrlw(ymm5, 8);

		// ymm0 = uf
		// ymm3 = rb00
		// ymm4 = ga00
		// ymm1 = rb10
		// ymm5 = rb11
		// ymm2 = ga10
		// ymm6 = ga11
		// ymm7 = used

		// rb10 = rb10.lerp16_4(rb11, uf);
		// ga10 = ga10.lerp16_4(ga11, uf);

		lerp16_4(ymm5, ymm1, ymm0);
		lerp16_4(ymm6, ymm2, ymm0);

		// ymm3 = rb00
		// ymm4 = ga00
		// ymm5 = rb10
		// ymm6 = ga10
		// ymm0, ymm1, ymm2 = free
		// ymm7 = used

		// rb00 = rb00.lerp16_4(rb10, vf);
		// ga00 = ga00.lerp16_4(ga10, vf);

		vmovdqa(ymm0, ptr[&m_local.temp.vf]);

		lerp16_4(ymm5, ymm3, ymm0);
		lerp16_4(ymm6, ymm4, ymm0);
	}
	else
	{
		// GSVector8i addr00 = y0 + x0;

		vpaddd(ymm5, ymm2, ymm4);

		// c00 = addr00.gather32_32((const uint32/uint8*)tex[, clut]);

		ReadTexel(1, 0);

		// GSVector8i mask = GSVector8i::x00ff();

		// c[0] = c00 & mask;
		// c[1] = (c00 >> 8) & mask;

		vpsllw(ymm5, ymm6, 8);
		vpsrlw(ymm5, 8);
		vpsrlw(ymm6, 8);
	}
}

void GSDrawScanlineCodeGenerator::Wrap(const Ymm& uv)
{
	// ymm0, ymm1, ymm4, ymm5, ymm6 = free

	int wms_clamp = ((m_sel.wms + 1) >> 1) & 1;
	int wmt_clamp = ((m_sel.wmt + 1) >> 1) & 1;

	int region = ((m_sel.wms | m_sel.wmt) >> 1) & 1;

	if(wms_clamp == wmt_clamp)
	{
		if(wms_clamp)
		{
			if(region)
			{
				vbroadcasti128(ymm0, ptr[&m_local.gd->t.min]);
				vpmaxsw(uv, ymm0);
			}
			else
			{
				vpxor(ymm0, ymm0);
				vpmaxsw(uv, ymm0);
			}

			vbroadcasti128(ymm0, ptr[&m_local.gd->t.max]);
			vpminsw(uv, ymm0);
		}
		else
		{
			vbroadcasti128(ymm0, ptr[&m_local.gd->t.min]);
			vpand(uv, ymm0);

			if(region)
			{
				vbroadcasti128(ymm0, ptr[&m_local.gd->t.max]);
				vpor(uv, ymm0);
			}
		}
	}
	else
	{
		vbroadcasti128(ymm4, ptr[&m_local.gd->t.min]);
		vbroadcasti128(ymm5, ptr[&m_local.gd->t.max]);
		vbroadcasti128(ymm0, ptr[&m_local.gd->t.mask]);

		// GSVector8i repeat = (t & m_local.gd->t.min) | m_local.gd->t.max;

		vpand(ymm1, uv, ymm4);

		if(region)
		{
			vpor(ymm1, ymm5);
		}

		// GSVector8i clamp = t.sat_i16(m_local.gd->t.min, m_local.gd->t.max);

		vpmaxsw(uv, ymm4);
		vpminsw(uv, ymm5);

		// clamp.blend8(repeat, m_local.gd->t.mask);

		vpblendvb(uv, ymm1, ymm0);
	}
}

void GSDrawScanlineCodeGenerator::Wrap(const Ymm& uv0, const Ymm& uv1)
{
	// ymm0, ymm1, ymm4, ymm5, ymm6 = free

	int wms_clamp = ((m_sel.wms + 1) >> 1) & 1;
	int wmt_clamp = ((m_sel.wmt + 1) >> 1) & 1;

	int region = ((m_sel.wms | m_sel.wmt) >> 1) & 1;

	if(wms_clamp == wmt_clamp)
	{
		if(wms_clamp)
		{
			if(region)
			{
				vbroadcasti128(ymm4, ptr[&m_local.gd->t.min]);
				vpmaxsw(uv0, ymm4);
				vpmaxsw(uv1, ymm4);
			}
			else
			{
				vpxor(ymm0, ymm0);
				vpmaxsw(uv0, ymm0);
				vpmaxsw(uv1, ymm0);
			}

			vbroadcasti128(ymm5, ptr[&m_local.gd->t.max]);
			vpminsw(uv0, ymm5);
			vpminsw(uv1, ymm5);
		}
		else
		{
			vbroadcasti128(ymm4, ptr[&m_local.gd->t.min]);
			vpand(uv0, ymm4);
			vpand(uv1, ymm4);

			if(region)
			{
				vbroadcasti128(ymm5, ptr[&m_local.gd->t.max]);
				vpor(uv0, ymm5);
				vpor(uv1, ymm5);
			}
		}
	}
	else
	{
		vbroadcasti128(ymm4, ptr[&m_local.gd->t.min]);
		vbroadcasti128(ymm5, ptr[&m_local.gd->t.max]);
		vbroadcasti128(ymm0, ptr[&m_local.gd->t.mask]);

		// uv0

		// GSVector8i repeat = (t & m_local.gd->t.min) | m_local.gd->t.max;

		vpand(ymm1, uv0, ymm4);

		if(region)
		{
			vpor(ymm1, ymm5);
		}

		// GSVector8i clamp = t.sat_i16(m_local.gd->t.min, m_local.gd->t.max);

		vpmaxsw(uv0, ymm4);
		vpminsw(uv0, ymm5);

		// clamp.blend8(repeat, m_local.gd->t.mask);

		vpblendvb(uv0, ymm1, ymm0);

		// uv1

		// GSVector8i repeat = (t & m_local.gd->t.min) | m_local.gd->t.max;

		vpand(ymm1, uv1, ymm4);

		if(region)
		{
			vpor(ymm1, ymm5);
		}

		// GSVector4i clamp = t.sat_i16(m_local.gd->t.min, m_local.gd->t.max);

		vpmaxsw(uv1, ymm4);
		vpminsw(uv1, ymm5);

		// clamp.blend8(repeat, m_local.gd->t.mask);

		vpblendvb(uv1, ymm1, ymm0);
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
		vrcpps(ymm0, ymm4);

		vmulps(ymm2, ymm0);
		vmulps(ymm3, ymm0);

		vcvttps2dq(ymm2, ymm2);
		vcvttps2dq(ymm3, ymm3);
	}

	// ymm2 = u
	// ymm3 = v
	// ymm4 = q
	// ymm0 = ymm1 = ymm5 = ymm6 = free

	// TODO: if the fractional part is not needed in round-off mode then there is a faster integer log2 (just take the exp) (but can we round it?)

	if(!m_sel.lcm)
	{
		// lod = -log2(Q) * (1 << L) + K

		vpcmpeqd(ymm1, ymm1);
		vpsrld(ymm1, ymm1, 25);
		vpslld(ymm0, ymm4, 1);
		vpsrld(ymm0, ymm0, 24);
		vpsubd(ymm0, ymm1);
		vcvtdq2ps(ymm0, ymm0); 

		// ymm0 = (float)(exp(q) - 127)

		vpslld(ymm4, ymm4, 9);
		vpsrld(ymm4, ymm4, 9);
		vorps(ymm4, ptr[&GSDrawScanlineCodeGenerator::m_log2_coef[3]]); 
			
		// ymm4 = mant(q) | 1.0f

		if(m_cpu.has(util::Cpu::tFMA))
		{
			vmovaps(ymm5, ptr[&GSDrawScanlineCodeGenerator::m_log2_coef[0]]); // c0
			vfmadd213ps(ymm5, ymm4, ptr[&GSDrawScanlineCodeGenerator::m_log2_coef[1]]); // c0 * ymm4 + c1
			vfmadd213ps(ymm5, ymm4, ptr[&GSDrawScanlineCodeGenerator::m_log2_coef[2]]); // (c0 * ymm4 + c1) * ymm4 + c2
			vsubps(ymm4, ptr[&GSDrawScanlineCodeGenerator::m_log2_coef[3]]); // ymm4 - 1.0f
			vfmadd213ps(ymm4, ymm5, ymm0); // ((c0 * ymm4 + c1) * ymm4 + c2) * (ymm4 - 1.0f) + ymm0
		}
		else
		{
			vmulps(ymm5, ymm4, ptr[&GSDrawScanlineCodeGenerator::m_log2_coef[0]]);
			vaddps(ymm5, ptr[&GSDrawScanlineCodeGenerator::m_log2_coef[1]]);
			vmulps(ymm5, ymm4);
			vsubps(ymm4, ptr[&GSDrawScanlineCodeGenerator::m_log2_coef[3]]); 
			vaddps(ymm5, ptr[&GSDrawScanlineCodeGenerator::m_log2_coef[2]]);
			vmulps(ymm4, ymm5);
			vaddps(ymm4, ymm0);
		}

		// ymm4 = log2(Q) = ((((c0 * ymm4) + c1) * ymm4) + c2) * (ymm4 - 1.0f) + ymm0

		if(m_cpu.has(util::Cpu::tFMA))
		{
			vmovaps(ymm5, ptr[&m_local.gd->l]);
			vfmadd213ps(ymm4, ymm5, ptr[&m_local.gd->k]); 
		}
		else
		{
			vmulps(ymm4, ptr[&m_local.gd->l]);
			vaddps(ymm4, ptr[&m_local.gd->k]);
		}

		// ymm4 = (-log2(Q) * (1 << L) + K) * 0x10000

		vxorps(ymm0, ymm0);
		vminps(ymm4, ptr[&m_local.gd->mxl]);
		vmaxps(ymm4, ymm0);
		vcvtps2dq(ymm4, ymm4);

		if(m_sel.mmin == 1) // round-off mode
		{
			mov(eax, 0x8000);
			vmovd(xmm0, eax);
			vpbroadcastd(ymm0, xmm0);
			vpaddd(ymm4, ymm0);
		}

		vpsrld(ymm0, ymm4, 16);

		vmovdqa(ptr[&m_local.temp.lod.i], ymm0);
/*
vpslld(ymm5, ymm0, 6);
vpslld(ymm6, ymm4, 16);
vpsrld(ymm6, ymm6, 24);
return;
*/
		if(m_sel.mmin == 2) // trilinear mode
		{
			vpshuflw(ymm1, ymm4, _MM_SHUFFLE(2, 2, 0, 0));
			vpshufhw(ymm1, ymm1, _MM_SHUFFLE(2, 2, 0, 0));
			vmovdqa(ptr[&m_local.temp.lod.f], ymm1);
		}

		// shift u/v/minmax by (int)lod

		vpsravd(ymm2, ymm2, ymm0);
		vpsravd(ymm3, ymm3, ymm0);

		vmovdqa(ptr[&m_local.temp.uv[0]], ymm2);
		vmovdqa(ptr[&m_local.temp.uv[1]], ymm3);

		// m_local.gd->t.minmax => m_local.temp.uv_minmax[0/1]

		vpxor(ymm1, ymm1);

		vbroadcasti128(ymm4, ptr[&m_local.gd->t.min]);
		vpunpcklwd(ymm5, ymm4, ymm1); // minu
		vpunpckhwd(ymm6, ymm4, ymm1); // minv
		vpsrlvd(ymm5, ymm5, ymm0);
		vpsrlvd(ymm6, ymm6, ymm0);
		vpackusdw(ymm5, ymm6);

		vbroadcasti128(ymm4, ptr[&m_local.gd->t.max]);
		vpunpcklwd(ymm6, ymm4, ymm1); // maxu
		vpunpckhwd(ymm4, ymm4, ymm1); // maxv
		vpsrlvd(ymm6, ymm6, ymm0);
		vpsrlvd(ymm4, ymm4, ymm0);
		vpackusdw(ymm6, ymm4);

		vmovdqa(ptr[&m_local.temp.uv_minmax[0]], ymm5);
		vmovdqa(ptr[&m_local.temp.uv_minmax[1]], ymm6);
	}
	else
	{
		// lod = K

		vmovd(xmm0, ptr[&m_local.gd->lod.i.u32[0]]);

		vpsrad(ymm2, xmm0);
		vpsrad(ymm3, xmm0);

		vmovdqa(ptr[&m_local.temp.uv[0]], ymm2);
		vmovdqa(ptr[&m_local.temp.uv[1]], ymm3);

		vmovdqa(ymm5, ptr[&m_local.temp.uv_minmax[0]]);
		vmovdqa(ymm6, ptr[&m_local.temp.uv_minmax[1]]);
	}

	// ymm2 = m_local.temp.uv[0] = u (level m)
	// ymm3 = m_local.temp.uv[1] = v (level m)
	// ymm5 = minuv
	// ymm6 = maxuv

	if(m_sel.ltf)
	{
		// u -= 0x8000;
		// v -= 0x8000;

		mov(eax, 0x8000);
		vmovd(xmm4, eax);
		vpbroadcastd(ymm4, xmm4);

		vpsubd(ymm2, ymm4);
		vpsubd(ymm3, ymm4);

		// GSVector8i uf = u.xxzzlh().srl16(1);
	
		vpshuflw(ymm0, ymm2, _MM_SHUFFLE(2, 2, 0, 0));
		vpshufhw(ymm0, ymm0, _MM_SHUFFLE(2, 2, 0, 0));
		vpsrlw(ymm0, 12);
		vmovdqa(ptr[&m_local.temp.uf], ymm0);

		// GSVector8i vf = v.xxzzlh().srl16(1);

		vpshuflw(ymm0, ymm3, _MM_SHUFFLE(2, 2, 0, 0));
		vpshufhw(ymm0, ymm0, _MM_SHUFFLE(2, 2, 0, 0));
		vpsrlw(ymm0, 12);
		vmovdqa(ptr[&m_local.temp.vf], ymm0);
	}

	// GSVector8i uv0 = u.sra32(16).ps32(v.sra32(16));

	vpsrad(ymm2, 16);
	vpsrad(ymm3, 16);
	vpackssdw(ymm2, ymm3);

	if(m_sel.ltf)
	{
		// GSVector8i uv1 = uv0.add16(GSVector8i::x0001());

		vpcmpeqd(ymm1, ymm1);
		vpsrlw(ymm1, 15);
		vpaddw(ymm3, ymm2, ymm1);

		// uv0 = Wrap(uv0);
		// uv1 = Wrap(uv1);

		WrapLOD(ymm2, ymm3);
	}
	else
	{
		// uv0 = Wrap(uv0);

		WrapLOD(ymm2);
	}

	// ymm2 = uv0
	// ymm3 = uv1 (ltf)
	// ymm0, ymm1, ymm4, ymm5, ymm6 = free
	// ymm7 = used

	// GSVector8i x0 = uv0.upl16();
	// GSVector8i y0 = uv0.uph16() << tw;

	vpxor(ymm0, ymm0);

	vpunpcklwd(ymm4, ymm2, ymm0);
	vpunpckhwd(ymm2, ymm2, ymm0);
	vpslld(ymm2, (uint8)(m_sel.tw + 3));

	// ymm0 = 0
	// ymm2 = y0
	// ymm3 = uv1 (ltf)
	// ymm4 = x0
	// ymm1, ymm5, ymm6 = free
	// ymm7 = used

	if(m_sel.ltf)
	{
		// GSVector8i x1 = uv1.upl16();
		// GSVector8i y1 = uv1.uph16() << tw;

		vpunpcklwd(ymm6, ymm3, ymm0);
		vpunpckhwd(ymm3, ymm3, ymm0);
		vpslld(ymm3, (uint8)(m_sel.tw + 3));

		// ymm2 = y0
		// ymm3 = y1
		// ymm4 = x0
		// ymm6 = x1
		// ymm0, ymm5, ymm6 = free
		// ymm7 = used

		// GSVector8i addr00 = y0 + x0;
		// GSVector8i addr01 = y0 + x1;
		// GSVector8i addr10 = y1 + x0;
		// GSVector8i addr11 = y1 + x1;

		vpaddd(ymm5, ymm2, ymm4);
		vpaddd(ymm2, ymm2, ymm6);
		vpaddd(ymm0, ymm3, ymm4);
		vpaddd(ymm3, ymm3, ymm6);

		// ymm5 = addr00
		// ymm2 = addr01
		// ymm0 = addr10
		// ymm3 = addr11
		// ymm1, ymm4, ymm6 = free
		// ymm7 = used

		// c00 = addr00.gather32_32((const uint32/uint8*)tex[, clut]);
		// c01 = addr01.gather32_32((const uint32/uint8*)tex[, clut]);
		// c10 = addr10.gather32_32((const uint32/uint8*)tex[, clut]);
		// c11 = addr11.gather32_32((const uint32/uint8*)tex[, clut]);

		ReadTexel(4, 0);

		// ymm6 = c00
		// ymm4 = c01
		// ymm1 = c10
		// ymm5 = c11
		// ymm0, ymm2, ymm3 = free
		// ymm7 = used

		vmovdqa(ymm0, ptr[&m_local.temp.uf]);

		// GSVector8i rb00 = c00 & mask;
		// GSVector8i ga00 = (c00 >> 8) & mask;

		vpsllw(ymm2, ymm6, 8);
		vpsrlw(ymm2, 8);
		vpsrlw(ymm6, 8);

		// GSVector8i rb01 = c01 & mask;
		// GSVector8i ga01 = (c01 >> 8) & mask;

		vpsllw(ymm3, ymm4, 8);
		vpsrlw(ymm3, 8);
		vpsrlw(ymm4, 8);

		// ymm0 = uf
		// ymm2 = rb00
		// ymm3 = rb01
		// ymm6 = ga00
		// ymm4 = ga01
		// ymm1 = c10
		// ymm5 = c11
		// ymm7 = used

		// rb00 = rb00.lerp16_4(rb01, uf);
		// ga00 = ga00.lerp16_4(ga01, uf);

		lerp16_4(ymm3, ymm2, ymm0);
		lerp16_4(ymm4, ymm6, ymm0);

		// ymm0 = uf
		// ymm3 = rb00
		// ymm4 = ga00
		// ymm1 = c10
		// ymm5 = c11
		// ymm2, ymm6 = free
		// ymm7 = used

		// GSVector8i rb10 = c10 & mask;
		// GSVector8i ga10 = (c10 >> 8) & mask;

		vpsrlw(ymm2, ymm1, 8);
		vpsllw(ymm1, 8);
		vpsrlw(ymm1, 8);

		// GSVector8i rb11 = c11 & mask;
		// GSVector8i ga11 = (c11 >> 8) & mask;

		vpsrlw(ymm6, ymm5, 8);
		vpsllw(ymm5, 8);
		vpsrlw(ymm5, 8);

		// ymm0 = uf
		// ymm3 = rb00
		// ymm4 = ga00
		// ymm1 = rb10
		// ymm5 = rb11
		// ymm2 = ga10
		// ymm6 = ga11
		// ymm7 = used

		// rb10 = rb10.lerp16_4(rb11, uf);
		// ga10 = ga10.lerp16_4(ga11, uf);

		lerp16_4(ymm5, ymm1, ymm0);
		lerp16_4(ymm6, ymm2, ymm0);

		// ymm3 = rb00
		// ymm4 = ga00
		// ymm5 = rb10
		// ymm6 = ga10
		// ymm0, ymm1, ymm2 = free
		// ymm7 = used

		// rb00 = rb00.lerp16_4(rb10, vf);
		// ga00 = ga00.lerp16_4(ga10, vf);

		vmovdqa(ymm0, ptr[&m_local.temp.vf]);

		lerp16_4(ymm5, ymm3, ymm0);
		lerp16_4(ymm6, ymm4, ymm0);
	}
	else
	{
		// GSVector8i addr00 = y0 + x0;

		vpaddd(ymm5, ymm2, ymm4);

		// c00 = addr00.gather32_32((const uint32/uint8*)tex[, clut]);

		ReadTexel(1, 0);

		// GSVector8i mask = GSVector8i::x00ff();

		// c[0] = c00 & mask;
		// c[1] = (c00 >> 8) & mask;

		vpsllw(ymm5, ymm6, 8);
		vpsrlw(ymm5, 8);
		vpsrlw(ymm6, 8);
	}

	if(m_sel.mmin != 1) // !round-off mode
	{
		vmovdqa(ptr[&m_local.temp.trb], ymm5);
		vmovdqa(ptr[&m_local.temp.tga], ymm6);

		vmovdqa(ymm2, ptr[&m_local.temp.uv[0]]);
		vmovdqa(ymm3, ptr[&m_local.temp.uv[1]]);

		vpsrad(ymm2, 1);
		vpsrad(ymm3, 1);

		vmovdqa(ymm5, ptr[&m_local.temp.uv_minmax[0]]);
		vmovdqa(ymm6, ptr[&m_local.temp.uv_minmax[1]]);

		vpsrlw(ymm5, 1);
		vpsrlw(ymm6, 1);

		if(m_sel.ltf)
		{
			// u -= 0x8000;
			// v -= 0x8000;

			mov(eax, 0x8000);
			vmovd(xmm4, eax);
			vpbroadcastd(ymm4, xmm4);

			vpsubd(ymm2, ymm4);
			vpsubd(ymm3, ymm4);

			// GSVector8i uf = u.xxzzlh().srl16(1);
	
			vpshuflw(ymm0, ymm2, _MM_SHUFFLE(2, 2, 0, 0));
			vpshufhw(ymm0, ymm0, _MM_SHUFFLE(2, 2, 0, 0));
			vpsrlw(ymm0, 12);
			vmovdqa(ptr[&m_local.temp.uf], ymm0);

			// GSVector8i vf = v.xxzzlh().srl16(1);

			vpshuflw(ymm0, ymm3, _MM_SHUFFLE(2, 2, 0, 0));
			vpshufhw(ymm0, ymm0, _MM_SHUFFLE(2, 2, 0, 0));
			vpsrlw(ymm0, 12);
			vmovdqa(ptr[&m_local.temp.vf], ymm0);
		}

		// GSVector8i uv0 = u.sra32(16).ps32(v.sra32(16));

		vpsrad(ymm2, 16);
		vpsrad(ymm3, 16);
		vpackssdw(ymm2, ymm3);

		if(m_sel.ltf)
		{
			// GSVector8i uv1 = uv0.add16(GSVector4i::x0001());

			vpcmpeqd(ymm1, ymm1);
			vpsrlw(ymm1, 15);
			vpaddw(ymm3, ymm2, ymm1);

			// uv0 = Wrap(uv0);
			// uv1 = Wrap(uv1);

			WrapLOD(ymm2, ymm3);
		}
		else
		{
			// uv0 = Wrap(uv0);

			WrapLOD(ymm2);
		}

		// ymm2 = uv0
		// ymm3 = uv1 (ltf)
		// ymm0, ymm1, ymm4, ymm5, ymm6 = free
		// ymm7 = used

		// GSVector8i x0 = uv0.upl16();
		// GSVector8i y0 = uv0.uph16() << tw;

		vpxor(ymm0, ymm0);

		vpunpcklwd(ymm4, ymm2, ymm0);
		vpunpckhwd(ymm2, ymm2, ymm0);
		vpslld(ymm2, (uint8)(m_sel.tw + 3));

		// ymm0 = 0
		// ymm2 = y0
		// ymm3 = uv1 (ltf)
		// ymm4 = x0
		// ymm1, ymm5, ymm6 = free
		// ymm7 = used

		if(m_sel.ltf)
		{
			// GSVector8i x1 = uv1.upl16();
			// GSVector8i y1 = uv1.uph16() << tw;

			vpunpcklwd(ymm6, ymm3, ymm0);
			vpunpckhwd(ymm3, ymm3, ymm0);
			vpslld(ymm3, (uint8)(m_sel.tw + 3));

			// ymm2 = y0
			// ymm3 = y1
			// ymm4 = x0
			// ymm6 = x1
			// ymm0, ymm5, ymm6 = free
			// ymm7 = used

			// GSVector8i addr00 = y0 + x0;
			// GSVector8i addr01 = y0 + x1;
			// GSVector8i addr10 = y1 + x0;
			// GSVector8i addr11 = y1 + x1;

			vpaddd(ymm5, ymm2, ymm4);
			vpaddd(ymm2, ymm2, ymm6);
			vpaddd(ymm0, ymm3, ymm4);
			vpaddd(ymm3, ymm3, ymm6);

			// ymm5 = addr00
			// ymm2 = addr01
			// ymm0 = addr10
			// ymm3 = addr11
			// ymm1, ymm4, ymm6 = free
			// ymm7 = used

			// c00 = addr00.gather32_32((const uint32/uint8*)tex[, clut]);
			// c01 = addr01.gather32_32((const uint32/uint8*)tex[, clut]);
			// c10 = addr10.gather32_32((const uint32/uint8*)tex[, clut]);
			// c11 = addr11.gather32_32((const uint32/uint8*)tex[, clut]);

			ReadTexel(4, 1);

			// ymm6 = c00
			// ymm4 = c01
			// ymm1 = c10
			// ymm5 = c11
			// ymm0, ymm2, ymm3 = free
			// ymm7 = used

			vmovdqa(ymm0, ptr[&m_local.temp.uf]);

			// GSVector8i rb00 = c00 & mask;
			// GSVector8i ga00 = (c00 >> 8) & mask;

			vpsllw(ymm2, ymm6, 8);
			vpsrlw(ymm2, 8);
			vpsrlw(ymm6, 8);

			// GSVector8i rb01 = c01 & mask;
			// GSVector8i ga01 = (c01 >> 8) & mask;

			vpsllw(ymm3, ymm4, 8);
			vpsrlw(ymm3, 8);
			vpsrlw(ymm4, 8);

			// ymm0 = uf
			// ymm2 = rb00
			// ymm3 = rb01
			// ymm6 = ga00
			// ymm4 = ga01
			// ymm1 = c10
			// ymm5 = c11
			// ymm7 = used

			// rb00 = rb00.lerp16_4(rb01, uf);
			// ga00 = ga00.lerp16_4(ga01, uf);

			lerp16_4(ymm3, ymm2, ymm0);
			lerp16_4(ymm4, ymm6, ymm0);

			// ymm0 = uf
			// ymm3 = rb00
			// ymm4 = ga00
			// ymm1 = c10
			// ymm5 = c11
			// ymm2, ymm6 = free
			// ymm7 = used

			// GSVector8i rb10 = c10 & mask;
			// GSVector8i ga10 = (c10 >> 8) & mask;

			vpsrlw(ymm2, ymm1, 8);
			vpsllw(ymm1, 8);
			vpsrlw(ymm1, 8);

			// GSVector8i rb11 = c11 & mask;
			// GSVector8i ga11 = (c11 >> 8) & mask;

			vpsrlw(ymm6, ymm5, 8);
			vpsllw(ymm5, 8);
			vpsrlw(ymm5, 8);

			// ymm0 = uf
			// ymm3 = rb00
			// ymm4 = ga00
			// ymm1 = rb10
			// ymm5 = rb11
			// ymm2 = ga10
			// ymm6 = ga11
			// ymm7 = used

			// rb10 = rb10.lerp16_4(rb11, uf);
			// ga10 = ga10.lerp16_4(ga11, uf);

			lerp16_4(ymm5, ymm1, ymm0);
			lerp16_4(ymm6, ymm2, ymm0);

			// ymm3 = rb00
			// ymm4 = ga00
			// ymm5 = rb10
			// ymm6 = ga10
			// ymm0, ymm1, ymm2 = free
			// ymm7 = used

			// rb00 = rb00.lerp16_4(rb10, vf);
			// ga00 = ga00.lerp16_4(ga10, vf);

			vmovdqa(ymm0, ptr[&m_local.temp.vf]);

			lerp16_4(ymm5, ymm3, ymm0);
			lerp16_4(ymm6, ymm4, ymm0);
		}
		else
		{
			// GSVector8i addr00 = y0 + x0;

			vpaddd(ymm5, ymm2, ymm4);

			// c00 = addr00.gather32_32((const uint32/uint8*)tex[, clut]);

			ReadTexel(1, 1);

			// GSVector8i mask = GSVector8i::x00ff();

			// c[0] = c00 & mask;
			// c[1] = (c00 >> 8) & mask;

			vpsllw(ymm5, ymm6, 8);
			vpsrlw(ymm5, 8);
			vpsrlw(ymm6, 8);
		}

		vmovdqa(ymm0, ptr[m_sel.lcm ? &m_local.gd->lod.f : &m_local.temp.lod.f]);
		vpsrlw(ymm0, ymm0, 1);

		vmovdqa(ymm2, ptr[&m_local.temp.trb]);
		vmovdqa(ymm3, ptr[&m_local.temp.tga]);

		lerp16(ymm5, ymm2, ymm0, 0);
		lerp16(ymm6, ymm3, ymm0, 0);
	}

	pop(ebp);
}

void GSDrawScanlineCodeGenerator::WrapLOD(const Ymm& uv)
{
	// ymm5 = minuv
	// ymm6 = maxuv
	// ymm0, ymm1, ymm4 = free

	int wms_clamp = ((m_sel.wms + 1) >> 1) & 1;
	int wmt_clamp = ((m_sel.wmt + 1) >> 1) & 1;

	int region = ((m_sel.wms | m_sel.wmt) >> 1) & 1;

	if(wms_clamp == wmt_clamp)
	{
		if(wms_clamp)
		{
			if(region)
			{
				vpmaxsw(uv, ymm5);
			}
			else
			{
				vpxor(ymm0, ymm0);
				vpmaxsw(uv, ymm0);
			}

			vpminsw(uv, ymm6);
		}
		else
		{
			vpand(uv, ymm5);

			if(region)
			{
				vpor(uv, ymm6);
			}
		}
	}
	else
	{
		vbroadcasti128(ymm0, ptr[&m_local.gd->t.mask]);

		// GSVector8i repeat = (t & m_local.gd->t.min) | m_local.gd->t.max;

		vpand(ymm1, uv, ymm5);

		if(region)
		{
			vpor(ymm1, ymm6);
		}

		// GSVector8i clamp = t.sat_i16(m_local.gd->t.min, m_local.gd->t.max);

		vpmaxsw(uv, ymm5);
		vpminsw(uv, ymm6);

		// clamp.blend8(repeat, m_local.gd->t.mask);

		vpblendvb(uv, ymm1, ymm0);
	}
}

void GSDrawScanlineCodeGenerator::WrapLOD(const Ymm& uv0, const Ymm& uv1)
{
	// ymm5 = minuv
	// ymm6 = maxuv
	// ymm0, ymm1, ymm4 = free

	int wms_clamp = ((m_sel.wms + 1) >> 1) & 1;
	int wmt_clamp = ((m_sel.wmt + 1) >> 1) & 1;

	int region = ((m_sel.wms | m_sel.wmt) >> 1) & 1;

	if(wms_clamp == wmt_clamp)
	{
		if(wms_clamp)
		{
			if(region)
			{
				vpmaxsw(uv0, ymm5);
				vpmaxsw(uv1, ymm5);
			}
			else
			{
				vpxor(ymm0, ymm0);
				vpmaxsw(uv0, ymm0);
				vpmaxsw(uv1, ymm0);
			}

			vpminsw(uv0, ymm6);
			vpminsw(uv1, ymm6);
		}
		else
		{
			vpand(uv0, ymm5);
			vpand(uv1, ymm5);

			if(region)
			{
				vpor(uv0, ymm6);
				vpor(uv1, ymm6);
			}
		}
	}
	else
	{
		vbroadcasti128(ymm0, ptr[&m_local.gd->t.mask]);

		// uv0

		// GSVector8i repeat = (t & m_local.gd->t.min) | m_local.gd->t.max;

		vpand(ymm1, uv0, ymm5);

		if(region)
		{
			vpor(ymm1, ymm6);
		}

		// GSVector8i clamp = t.sat_i16(m_local.gd->t.min, m_local.gd->t.max);

		vpmaxsw(uv0, ymm5);
		vpminsw(uv0, ymm6);

		// clamp.blend8(repeat, m_local.gd->t.mask);

		vpblendvb(uv0, ymm1, ymm0);

		// uv1

		// GSVector8i repeat = (t & m_local.gd->t.min) | m_local.gd->t.max;

		vpand(ymm1, uv1, ymm5);

		if(region)
		{
			vpor(ymm1, ymm6);
		}

		// GSVector8i clamp = t.sat_i16(m_local.gd->t.min, m_local.gd->t.max);

		vpmaxsw(uv1, ymm5);
		vpminsw(uv1, ymm6);

		// clamp.blend8(repeat, m_local.gd->t.mask);

		vpblendvb(uv1, ymm1, ymm0);
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

		// GSVector8i ga = iip ? gaf : m_local.c.ga;

		vmovdqa(ymm4, ptr[m_sel.iip ? &m_local.temp.ga : &m_local.c.ga]);

		// gat = gat.modulate16<1>(ga).clamp8();

		modulate16(ymm6, ymm4, 1);

		clamp16(ymm6, ymm3);

		// if(!tcc) gat = gat.mix16(ga.srl16(7));

		if(!m_sel.tcc)
		{
			vpsrlw(ymm4, 7);

			mix16(ymm6, ymm4, ymm3);
		}

		break;

	case TFX_DECAL:

		// if(!tcc) gat = gat.mix16(ga.srl16(7));

		if(!m_sel.tcc)
		{
			// GSVector4i ga = iip ? gaf : m_local.c.ga;

			vmovdqa(ymm4, ptr[m_sel.iip ? &m_local.temp.ga : &m_local.c.ga]);

			vpsrlw(ymm4, 7);

			mix16(ymm6, ymm4, ymm3);
		}

		break;

	case TFX_HIGHLIGHT:

		// GSVector4i ga = iip ? gaf : m_local.c.ga;

		vmovdqa(ymm4, ptr[m_sel.iip ? &m_local.temp.ga : &m_local.c.ga]);
		vmovdqa(ymm2, ymm4);

		// gat = gat.mix16(!tcc ? ga.srl16(7) : gat.addus8(ga.srl16(7)));

		vpsrlw(ymm4, 7);

		if(m_sel.tcc)
		{
			vpaddusb(ymm4, ymm6);
		}

		mix16(ymm6, ymm4, ymm3);

		break;

	case TFX_HIGHLIGHT2:

		// if(!tcc) gat = gat.mix16(ga.srl16(7));

		if(!m_sel.tcc)
		{
			// GSVector4i ga = iip ? gaf : m_local.c.ga;

			vmovdqa(ymm4, ptr[m_sel.iip ? &m_local.temp.ga : &m_local.c.ga]);
			vmovdqa(ymm2, ymm4);

			vpsrlw(ymm4, 7);

			mix16(ymm6, ymm4, ymm3);
		}

		break;

	case TFX_NONE:

		// gat = iip ? ga.srl16(7) : ga;

		if(m_sel.iip)
		{
			vpsrlw(ymm6, 7);
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
				vmovdqa(ymm0, ptr[&m_local.temp.cov]);
			}
			else
			{
				vpcmpeqd(ymm0, ymm0);
				vpsllw(ymm0, 15);
				vpsrlw(ymm0, 8);
			}

			mix16(ymm6, ymm0, ymm1);
		}
		else
		{
			// a = a == 0x80 ? cov : a

			vpcmpeqd(ymm0, ymm0);
			vpsllw(ymm0, 15);
			vpsrlw(ymm0, 8);

			if(m_sel.edge)
			{
				vmovdqa(ymm1, ptr[&m_local.temp.cov]);
			}
			else
			{
				vmovdqa(ymm1, ymm0);
			}

			vpcmpeqw(ymm0, ymm6);
			vpsrld(ymm0, 16);
			vpslld(ymm0, 16);

			vpblendvb(ymm6, ymm1, ymm0);
		}
	}
}

void GSDrawScanlineCodeGenerator::ReadMask()
{
	if(m_sel.fwrite)
	{
		vpbroadcastd(ymm3, ptr[&m_local.gd->fm]);
	}

	if(m_sel.zwrite)
	{
		vpbroadcastd(ymm4, ptr[&m_local.gd->zm]);
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
		// t = GSVector8i::xffffffff();
		vpcmpeqd(ymm1, ymm1);
		break;

	case ATST_ALWAYS:
		return;

	case ATST_LESS:
	case ATST_LEQUAL:
		// t = (ga >> 16) > m_local.gd->aref;
		vpsrld(ymm1, ymm6, 16);
		vbroadcasti128(ymm0, ptr[&m_local.gd->aref]);
		vpcmpgtd(ymm1, ymm0);
		break;

	case ATST_EQUAL:
		// t = (ga >> 16) != m_local.gd->aref;
		vpsrld(ymm1, ymm6, 16);
		vbroadcasti128(ymm0, ptr[&m_local.gd->aref]);
		vpcmpeqd(ymm1, ymm0);
		vpcmpeqd(ymm0, ymm0);
		vpxor(ymm1, ymm0);
		break;

	case ATST_GEQUAL:
	case ATST_GREATER:
		// t = (ga >> 16) < m_local.gd->aref;
		vpsrld(ymm0, ymm6, 16);
		vbroadcasti128(ymm1, ptr[&m_local.gd->aref]);
		vpcmpgtd(ymm1, ymm0);
		break;

	case ATST_NOTEQUAL:
		// t = (ga >> 16) == m_local.gd->aref;
		vpsrld(ymm1, ymm6, 16);
		vbroadcasti128(ymm0, ptr[&m_local.gd->aref]);
		vpcmpeqd(ymm1, ymm0);
		break;
	}

	switch(m_sel.afail)
	{
	case AFAIL_KEEP:
		// test |= t;
		vpor(ymm7, ymm1);
		alltrue();
		break;

	case AFAIL_FB_ONLY:
		// zm |= t;
		vpor(ymm4, ymm1);
		break;

	case AFAIL_ZB_ONLY:
		// fm |= t;
		vpor(ymm3, ymm1);
		break;

	case AFAIL_RGB_ONLY:
		// zm |= t;
		vpor(ymm4, ymm1);
		// fm |= t & GSVector8i::xff000000();
		vpsrld(ymm1, 24);
		vpslld(ymm1, 24);
		vpor(ymm3, ymm1);
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

		// GSVector8i rb = iip ? rbf : m_local.c.rb;

		// rbt = rbt.modulate16<1>(rb).clamp8();

		modulate16(ymm5, ptr[m_sel.iip ? &m_local.temp.rb : &m_local.c.rb], 1);

		clamp16(ymm5, ymm1);

		break;

	case TFX_DECAL:

		break;

	case TFX_HIGHLIGHT:
	case TFX_HIGHLIGHT2:

		if(m_sel.tfx == TFX_HIGHLIGHT2 && m_sel.tcc)
		{
			// GSVector8i ga = iip ? gaf : m_local.c.ga;

			vmovdqa(ymm2, ptr[m_sel.iip ? &m_local.temp.ga : &m_local.c.ga]);
		}

		// gat = gat.modulate16<1>(ga).add16(af).clamp8().mix16(gat);

		vmovdqa(ymm1, ymm6);

		modulate16(ymm6, ymm2, 1);

		vpshuflw(ymm2, ymm2, _MM_SHUFFLE(3, 3, 1, 1));
		vpshufhw(ymm2, ymm2, _MM_SHUFFLE(3, 3, 1, 1));
		vpsrlw(ymm2, 7);

		vpaddw(ymm6, ymm2);

		clamp16(ymm6, ymm0);

		mix16(ymm6, ymm1, ymm0);

		// GSVector8i rb = iip ? rbf : m_local.c.rb;

		// rbt = rbt.modulate16<1>(rb).add16(af).clamp8();

		modulate16(ymm5, ptr[m_sel.iip ? &m_local.temp.rb : &m_local.c.rb], 1);

		vpaddw(ymm5, ymm2);

		clamp16(ymm5, ymm0);

		break;

	case TFX_NONE:

		// rbt = iip ? rb.srl16(7) : rb;

		if(m_sel.iip)
		{
			vpsrlw(ymm5, 7);
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

	if(m_sel.prim != GS_SPRITE_CLASS)
	{
		vmovdqa(ymm0, ptr[&m_local.temp.f]);
	}
	else
	{
		vpbroadcastw(ymm0, ptr[&m_local.p.f]);
	}

	vmovdqa(ymm1, ymm6);

	vpbroadcastd(ymm2, ptr[&m_local.gd->frb]);
	lerp16(ymm5, ymm2, ymm0, 0);

	vpbroadcastd(ymm2, ptr[&m_local.gd->fga]);
	lerp16(ymm6, ymm2, ymm0, 0);
	mix16(ymm6, ymm1, ymm0);
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

	ReadPixel(ymm2, ymm0, ebx);
}

void GSDrawScanlineCodeGenerator::TestDestAlpha()
{
	if(!m_sel.date || m_sel.fpsm != 0 && m_sel.fpsm != 2)
	{
		return;
	}

	// test |= ((fd [<< 16]) ^ m_local.gd->datm).sra32(31);

	if(m_sel.datm)
	{
		if(m_sel.fpsm == 2)
		{
			vpxor(ymm0, ymm0);
			vpsrld(ymm1, ymm2, 15);
			vpcmpeqd(ymm1, ymm0);
		}
		else
		{
			vpcmpeqd(ymm0, ymm0);
			vpxor(ymm1, ymm2, ymm0);
			vpsrad(ymm1, 31);
		}
	}
	else
	{
		if(m_sel.fpsm == 2)
		{
			vpslld(ymm1, ymm2, 16);
			vpsrad(ymm1, 31);
		}
		else
		{
			vpsrad(ymm1, ymm2, 31);
		}
	}

	vpor(ymm7, ymm1);

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
		vpor(ymm3, ymm7);
	}

	if(m_sel.zwrite)
	{
		vpor(ymm4, ymm7);
	}

	// int fzm = ~(fm == GSVector8i::xffffffff()).ps32(zm == GSVector8i::xffffffff()).mask();

	vpcmpeqd(ymm1, ymm1);

	if(m_sel.fwrite && m_sel.zwrite)
	{
		vpcmpeqd(ymm0, ymm1, ymm4);
		vpcmpeqd(ymm1, ymm3);
		vpackssdw(ymm1, ymm0);
	}
	else if(m_sel.fwrite)
	{
		vpcmpeqd(ymm1, ymm3);
		vpackssdw(ymm1, ymm1);
	}
	else if(m_sel.zwrite)
	{
		vpcmpeqd(ymm1, ymm4);
		vpackssdw(ymm1, ymm1);
	}

	vpmovmskb(edx, ymm1);

	not(edx);
}

void GSDrawScanlineCodeGenerator::WriteZBuf()
{
	if(!m_sel.zwrite)
	{
		return;
	}

	if(m_sel.prim != GS_SPRITE_CLASS)
	{
		vmovdqa(ymm1, ptr[&m_local.temp.zs]);
	}
	else
	{
		vpbroadcastd(ymm1, ptr[&m_local.p.z]);
	}

	if(m_sel.ztest && m_sel.zpsm < 2)
	{
		// zs = zs.blend8(zd, zm);

		vpblendvb(ymm1, ptr[&m_local.temp.zd], ymm4);
	}

	bool fast = m_sel.ztest ? m_sel.zpsm < 2 : m_sel.zpsm == 0 && m_sel.notest;

	WritePixel(ymm1, ymm0, ebp, edx, fast, m_sel.zpsm, 1);
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

			vpsllw(ymm0, ymm2, 8);
			vpsrlw(ymm0, 8);
			vpsrlw(ymm1, ymm2, 8);

			break;

		case 2:

			// c[2] = ((fd & 0x7c00) << 9) | ((fd & 0x001f) << 3);
			// c[3] = ((fd & 0x8000) << 8) | ((fd & 0x03e0) >> 2);

			vpcmpeqd(ymm7, ymm7);

			vpsrld(ymm7, 27); // 0x0000001f
			vpand(ymm0, ymm2, ymm7);
			vpslld(ymm0, 3);

			vpslld(ymm7, 10); // 0x00007c00
			vpand(ymm4, ymm2, ymm7);
			vpslld(ymm4, 9);

			vpor(ymm0, ymm4);

			vpsrld(ymm7, 5); // 0x000003e0
			vpand(ymm1, ymm2, ymm7);
			vpsrld(ymm1, 2);

			vpsllw(ymm7, 10); // 0x00008000
			vpand(ymm4, ymm2, ymm7);
			vpslld(ymm4, 8);

			vpor(ymm1, ymm4);

			break;
		}
	}

	// ymm5, ymm6 = src rb, ga
	// ymm0, ymm1 = dst rb, ga
	// ymm2, ymm3 = used
	// ymm4, ymm7 = free

	if(m_sel.pabe || (m_sel.aba != m_sel.abb) && (m_sel.abb == 0 || m_sel.abd == 0))
	{
		vmovdqa(ymm4, ymm5);
	}

	if(m_sel.aba != m_sel.abb)
	{
		// rb = c[aba * 2 + 0];

		switch(m_sel.aba)
		{
		case 0: break;
		case 1: vmovdqa(ymm5, ymm0); break;
		case 2: vpxor(ymm5, ymm5); break;
		}

		// rb = rb.sub16(c[abb * 2 + 0]);

		switch(m_sel.abb)
		{
		case 0: vpsubw(ymm5, ymm4); break;
		case 1: vpsubw(ymm5, ymm0); break;
		case 2: break;
		}

		if(!(m_sel.fpsm == 1 && m_sel.abc == 1))
		{
			// GSVector4i a = abc < 2 ? c[abc * 2 + 1].yywwlh().sll16(7) : m_local.gd->afix;

			switch(m_sel.abc)
			{
			case 0:
			case 1:
				vpshuflw(ymm7, m_sel.abc ? ymm1 : ymm6, _MM_SHUFFLE(3, 3, 1, 1));
				vpshufhw(ymm7, ymm7, _MM_SHUFFLE(3, 3, 1, 1));
				vpsllw(ymm7, 7);
				break;
			case 2:
				vpbroadcastw(ymm7, ptr[&m_local.gd->afix]);
				break;
			}

			// rb = rb.modulate16<1>(a);

			modulate16(ymm5, ymm7, 1);
		}

		// rb = rb.add16(c[abd * 2 + 0]);

		switch(m_sel.abd)
		{
		case 0: vpaddw(ymm5, ymm4); break;
		case 1: vpaddw(ymm5, ymm0); break;
		case 2: break;
		}
	}
	else
	{
		// rb = c[abd * 2 + 0];

		switch(m_sel.abd)
		{
		case 0: break;
		case 1: vmovdqa(ymm5, ymm0); break;
		case 2: vpxor(ymm5, ymm5); break;
		}
	}

	if(m_sel.pabe)
	{
		// mask = (c[1] << 8).sra32(31);

		vpslld(ymm0, ymm6, 8);
		vpsrad(ymm0, 31);

		// rb = c[0].blend8(rb, mask);

		vpblendvb(ymm5, ymm4, ymm5, ymm0);
	}

	// ymm6 = src ga
	// ymm1 = dst ga
	// ymm5 = rb
	// ymm7 = a
	// ymm2, ymm3 = used
	// ymm0, ymm4 = free

	vmovdqa(ymm4, ymm6);

	if(m_sel.aba != m_sel.abb)
	{
		// ga = c[aba * 2 + 1];

		switch(m_sel.aba)
		{
		case 0: break;
		case 1: vmovdqa(ymm6, ymm1); break;
		case 2: vpxor(ymm6, ymm6); break;
		}

		// ga = ga.sub16(c[abeb * 2 + 1]);

		switch(m_sel.abb)
		{
		case 0: vpsubw(ymm6, ymm4); break;
		case 1: vpsubw(ymm6, ymm1); break;
		case 2: break;
		}

		if(!(m_sel.fpsm == 1 && m_sel.abc == 1))
		{
			// ga = ga.modulate16<1>(a);

			modulate16(ymm6, ymm7, 1);
		}

		// ga = ga.add16(c[abd * 2 + 1]);

		switch(m_sel.abd)
		{
		case 0: vpaddw(ymm6, ymm4); break;
		case 1: vpaddw(ymm6, ymm1); break;
		case 2: break;
		}
	}
	else
	{
		// ga = c[abd * 2 + 1];

		switch(m_sel.abd)
		{
		case 0: break;
		case 1: vmovdqa(ymm6, ymm1); break;
		case 2: vpxor(ymm6, ymm6); break;
		}
	}

	// ymm4 = src ga
	// ymm5 = rb
	// ymm6 = ga
	// ymm2, ymm3 = used
	// ymm0, ymm1, ymm7 = free

	if(m_sel.pabe)
	{
		vpsrld(ymm0, 16); // zero out high words to select the source alpha in blend (so it also does mix16)

		// ga = c[1].blend8(ga, mask).mix16(c[1]);

		vpblendvb(ymm6, ymm4, ymm6, ymm0);
	}
	else
	{
		if(m_sel.fpsm != 1) // TODO: fm == 0xffxxxxxx
		{
			mix16(ymm6, ymm4, ymm7);
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
		vbroadcasti128(ymm7, ptr[ebp + eax + sizeof(GSVector4i) * 0]);
		vpaddw(ymm5, ymm7);
		vbroadcasti128(ymm7, ptr[ebp + eax + sizeof(GSVector4i) * 1]);
		vpaddw(ymm6, ymm7);
	}

	if(m_sel.colclamp == 0)
	{
		// c[0] &= 0x00ff00ff;
		// c[1] &= 0x00ff00ff;

		vpcmpeqd(ymm7, ymm7);
		vpsrlw(ymm7, 8);
		vpand(ymm5, ymm7);
		vpand(ymm6, ymm7);
	}

	// GSVector8i fs = c[0].upl16(c[1]).pu16(c[0].uph16(c[1]));

	vpunpckhwd(ymm7, ymm5, ymm6);
	vpunpcklwd(ymm5, ymm6);
	vpackuswb(ymm5, ymm7);

	if(m_sel.fba && m_sel.fpsm != 1)
	{
		// fs |= 0x80000000;

		vpcmpeqd(ymm7, ymm7);
		vpslld(ymm7, 31);
		vpor(ymm5, ymm7);
	}

	if(m_sel.fpsm == 2)
	{
		// GSVector8i rb = fs & 0x00f800f8;
		// GSVector8i ga = fs & 0x8000f800;

		mov(eax, 0x00f800f8);
		vmovd(xmm6, eax);
		vpbroadcastd(ymm6, xmm6);

		mov(eax, 0x8000f800);
		vmovd(xmm7, eax);
		vpbroadcastd(ymm7, xmm7);

		vpand(ymm4, ymm5, ymm6);
		vpand(ymm5, ymm7);

		// fs = (ga >> 16) | (rb >> 9) | (ga >> 6) | (rb >> 3);

		vpsrld(ymm6, ymm4, 9);
		vpsrld(ymm4, 3);
		vpsrld(ymm7, ymm5, 16);
		vpsrld(ymm5, 6);

		vpor(ymm5, ymm4);
		vpor(ymm7, ymm6);
		vpor(ymm5, ymm7);
	}

	if(m_sel.rfb)
	{
		// fs = fs.blend(fd, fm);

		blend(ymm5, ymm2, ymm3); // TODO: could be skipped in certain cases, depending on fpsm and fm
	}

	bool fast = m_sel.rfb ? m_sel.fpsm < 2 : m_sel.fpsm == 0 && m_sel.notest;

	WritePixel(ymm5, ymm0, ebx, edx, fast, m_sel.fpsm, 0);
}

void GSDrawScanlineCodeGenerator::ReadPixel(const Ymm& dst, const Ymm& temp, const Reg32& addr)
{
	vmovq(Xmm(dst.getIdx()), qword[addr * 2 + (size_t)m_local.gd->vm]);
	vmovhps(Xmm(dst.getIdx()), qword[addr * 2 + (size_t)m_local.gd->vm + 8 * 2]);	
	vmovq(Xmm(temp.getIdx()), qword[addr * 2 + (size_t)m_local.gd->vm + 16 * 2]);
	vmovhps(Xmm(temp.getIdx()), qword[addr * 2 + (size_t)m_local.gd->vm + 24 * 2]);	
	vinserti128(dst, dst, temp, 1);	
/*
	vmovdqu(dst, ptr[addr * 2 + (size_t)m_local.gd->vm]);
	vmovdqu(temp, ptr[addr * 2 + (size_t)m_local.gd->vm + 16 * 2]);
	vpunpcklqdq(dst, dst, temp);
	vpermq(dst, dst, _MM_SHUFFLE(3, 1, 2, 0));
*/
}

void GSDrawScanlineCodeGenerator::WritePixel(const Ymm& src, const Ymm& temp, const Reg32& addr, const Reg32& mask, bool fast, int psm, int fz)
{
	Xmm src1 = Xmm(src.getIdx());
	Xmm src2 = Xmm(temp.getIdx());

	vextracti128(src2, src, 1); 

	if(m_sel.notest)
	{
		if(fast)
		{
			vmovq(qword[addr * 2 + (size_t)m_local.gd->vm], src1);
			vmovhps(qword[addr * 2 + (size_t)m_local.gd->vm + 8 * 2], src1);
			vmovq(qword[addr * 2 + (size_t)m_local.gd->vm + 16 * 2], src2);
			vmovhps(qword[addr * 2 + (size_t)m_local.gd->vm + 24 * 2], src2);
		}
		else
		{
			WritePixel(src1, addr, 0, 0, psm);
			WritePixel(src1, addr, 1, 1, psm);
			WritePixel(src1, addr, 2, 2, psm);
			WritePixel(src1, addr, 3, 3, psm);
			WritePixel(src2, addr, 4, 0, psm);
			WritePixel(src2, addr, 5, 1, psm);
			WritePixel(src2, addr, 6, 2, psm);
			WritePixel(src2, addr, 7, 3, psm);
		}
	}
	else
	{
		// cascade tests?

		if(fast)
		{
			test(mask, 0x0000000f << (fz * 8));
			je("@f");
			vmovq(qword[addr * 2 + (size_t)m_local.gd->vm], src1);
			L("@@");

			test(mask, 0x000000f0 << (fz * 8));
			je("@f");
			vmovhps(qword[addr * 2 + (size_t)m_local.gd->vm + 8 * 2], src1);
			L("@@");

			test(mask, 0x000f0000 << (fz * 8));
			je("@f");
			vmovq(qword[addr * 2 + (size_t)m_local.gd->vm + 16 * 2], src2);
			L("@@");

			test(mask, 0x00f00000 << (fz * 8));
			je("@f");
			vmovhps(qword[addr * 2 + (size_t)m_local.gd->vm + 24 * 2], src2);
			L("@@");

			// vmaskmovps?
		}
		else
		{
			test(mask, 0x00000003 << (fz * 8));
			je("@f");
			WritePixel(src1, addr, 0, 0, psm);
			L("@@");

			test(mask, 0x0000000c << (fz * 8));
			je("@f");
			WritePixel(src1, addr, 1, 1, psm);
			L("@@");

			test(mask, 0x00000030 << (fz * 8));
			je("@f");
			WritePixel(src1, addr, 2, 2, psm);
			L("@@");

			test(mask, 0x000000c0 << (fz * 8));
			je("@f");
			WritePixel(src1, addr, 3, 3, psm);
			L("@@");

			test(mask, 0x00030000 << (fz * 8));
			je("@f");
			WritePixel(src2, addr, 4, 0, psm);
			L("@@");

			test(mask, 0x000c0000 << (fz * 8));
			je("@f");
			WritePixel(src2, addr, 5, 1, psm);
			L("@@");

			test(mask, 0x00300000 << (fz * 8));
			je("@f");
			WritePixel(src2, addr, 6, 2, psm);
			L("@@");

			test(mask, 0x00c00000 << (fz * 8));
			je("@f");
			WritePixel(src2, addr, 7, 3, psm);
			L("@@");
		}
	}
}

static const int s_offsets[] = {0, 2, 8, 10, 16, 18, 24, 26};

void GSDrawScanlineCodeGenerator::WritePixel(const Xmm& src, const Reg32& addr, uint8 i, uint8 j, int psm)
{
	Address dst = ptr[addr * 2 + (size_t)m_local.gd->vm + s_offsets[i] * 2];

	switch(psm)
	{
	case 0:
		if(j == 0) vmovd(dst, src);
		else vpextrd(dst, src, j);
		break;
	case 1:
		if(j == 0) vmovd(eax, src);
		else vpextrd(eax, src, j);
		xor(eax, dst);
		and(eax, 0xffffff);
		xor(dst, eax);
		break;
	case 2:
		if(j == 0) vmovd(eax, src);
		else vpextrw(eax, src, j * 2);
		mov(dst, ax);
		break;
	}
}

void GSDrawScanlineCodeGenerator::ReadTexel(int pixels, int mip_offset)
{
	// in
	// ymm5 = addr00
	// ymm2 = addr01
	// ymm0 = addr10
	// ymm3 = addr11
	// ebx = m_local.tex[0] (!m_sel.mmin)
	// ebp = m_local.tex (m_sel.mmin)
	// edx = m_local.clut (m_sel.tlu)

	// out
	// ymm6 = c00
	// ymm4 = c01
	// ymm1 = c10
	// ymm5 = c11

	ASSERT(pixels == 1 || pixels == 4);

	mip_offset *= sizeof(void*);

	const GSVector8i* lod_i = m_sel.lcm ? &m_local.gd->lod.i : &m_local.temp.lod.i;

	if(m_sel.mmin && !m_sel.lcm)
	{
		const int r[] = {5, 6, 2, 4, 0, 1, 3, 5};
		const int t[] = {1, 4, 5, 1, 2, 5, 0, 2};

		for(int i = 0; i < pixels; i++)
		{
			Ymm src = Ymm(r[i * 2 + 0]);
			Ymm dst = Ymm(r[i * 2 + 1]);
			Ymm t1 = Ymm(t[i * 2 + 0]);
			Ymm t2 = Ymm(t[i * 2 + 1]);

			vextracti128(Xmm(t1.getIdx()), src, 1);

			for(uint8 j = 0; j < 4; j++)
			{
				mov(ebx, ptr[&lod_i->u32[j + 0]]);
				mov(ebx, ptr[ebp + ebx * sizeof(void*) + mip_offset]);

				ReadTexel(dst, src, j);

				mov(ebx, ptr[&lod_i->u32[j + 4]]);
				mov(ebx, ptr[ebp + ebx * sizeof(void*) + mip_offset]);

				ReadTexel(t2, t1, j);
			}

			vinserti128(dst, dst, t2, 1);
		}
	}
	else 
	{
		const int r[] = {5, 6, 2, 4, 0, 1, 3, 5};
		const int t[] = {1, 4, 5, 1, 2, 5, 0, 2};
		
		if(m_sel.mmin && m_sel.lcm)
		{
			mov(ebx, ptr[&lod_i->u32[0]]);
			mov(ebx, ptr[ebp + ebx * sizeof(void*) + mip_offset]);
		}

		for(int i = 0; i < pixels; i++)
		{
			Ymm src = Ymm(r[i * 2 + 0]);
			Ymm dst = Ymm(r[i * 2 + 1]);
			Ymm t1 = Ymm(t[i * 2 + 0]);
			Ymm t2 = Ymm(t[i * 2 + 1]);

			if(!m_sel.tlu)
			{
				vpcmpeqd(t1, t1);
				vpgatherdd(dst, ptr[ebx + src * 4], t1);
			}
			else
			{
				vextracti128(Xmm(t1.getIdx()), src, 1);

				for(uint8 j = 0; j < 4; j++)
				{
					ReadTexel(dst, src, j);
					ReadTexel(t2, t1, j);
				}

				vinserti128(dst, dst, t2, 1);
				/*
				vpcmpeqd(t1, t1);
				vpgatherdd(t2, ptr[ebx + src * 1], t1); // either this 1x scale, or the latency of two dependendent gathers are too slow
				vpslld(t2, 24);
				vpsrld(t2, 24);
				vpcmpeqd(t1, t1);
				vpgatherdd(dst, ptr[edx + t2 * 4], t1);
				*/
			}
		}
	}
}

void GSDrawScanlineCodeGenerator::ReadTexel(const Ymm& dst, const Ymm& addr, uint8 i)
{
	ASSERT(i < 4);

	const Address& src = m_sel.tlu ? ptr[edx + eax * 4] : ptr[ebx + eax * 4];

	if(i == 0) vmovd(eax, Xmm(addr.getIdx()));
	else vpextrd(eax, Xmm(addr.getIdx()), i);
	
	if(m_sel.tlu) movzx(eax, byte[ebx + eax]);

	if(i == 0) vmovd(Xmm(dst.getIdx()), src);
	else vpinsrd(Xmm(dst.getIdx()), src, i);
}


#endif