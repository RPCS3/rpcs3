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

// TODO: x64 (use the extra regs to avoid spills of zs, zd, uf, vf, rb, ga and keep a few constants in the last two like aref or afix)
// TODO: for edges doing 4 pixels is wasteful (needed memory access * 4)

#include "StdAfx.h"
#include "GSDrawScanlineCodeGenerator.h"

static const int _args = 16;
static const int _top = _args + 4;
static const int _v = _args + 8;

GSDrawScanlineCodeGenerator::GSDrawScanlineCodeGenerator(void* param, uint64 key, void* code, size_t maxsize)
	: GSCodeGenerator(code, maxsize)
	, m_local(*(GSScanlineLocalData*)param)
{
	#if _M_AMD64
	#error TODO
	#endif

	m_sel.key = key;

	// ret(8);

	Generate();
}

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
	// xmm2 = u (tme)
	// xmm3 = v (tme)
	// xmm5 = rb (!tme)
	// xmm6 = ga (!tme)
	// xmm7 = test

	bool tme = m_sel.tfx != TFX_NONE;

	TestZ(tme ? xmm5 : xmm2, tme ? xmm6 : xmm3);

	// ecx = steps
	// esi = fzbr
	// edi = fzbc
	// - xmm0
	// xmm2 = u (tme)
	// xmm3 = v (tme)
	// xmm5 = rb (!tme)
	// xmm6 = ga (!tme)
	// xmm7 = test

	SampleTexture();

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
	// int skip = left & 3;

	mov(ebx, edx);
	and(edx, 3);

	// left -= skip;

	sub(ebx, edx);

	// int steps = right - left - 4;

	sub(ecx, ebx);
	sub(ecx, 4);

	if(m_cpu.has(util::Cpu::tAVX))
	{
		// GSVector4i test = m_test[skip] | m_test[7 + (steps & (steps >> 31))];

		shl(edx, 4);

		vmovdqa(xmm7, ptr[edx + (size_t)&m_test[0]]);

		mov(eax, ecx);
		sar(eax, 31);
		and(eax, ecx);
		shl(eax, 4);

		vpor(xmm7, ptr[eax + (size_t)&m_test[7]]);

		// GSVector2i* fza_base = &m_local.gd->fzbr[top];

		mov(esi, dword[esp + _top]);
		lea(esi, ptr[esi * 8]);
		add(esi, dword[&m_local.gd->fzbr]);

		// GSVector2i* fza_offset = &m_local.gd->fzbc[left >> 2];

		lea(edi, ptr[ebx * 2]);
		add(edi, dword[&m_local.gd->fzbc]);

		if(!m_sel.sprite && (m_sel.fwrite && m_sel.fge || m_sel.zb) || m_sel.fb && (m_sel.edge || m_sel.tfx != TFX_NONE || m_sel.iip))
		{
			// edx = &m_local.d[skip]

			shl(edx, 4);
			lea(edx, ptr[edx + (size_t)m_local.d]);

			// ebx = &v

			mov(ebx, dword[esp + _v]);
		}

		if(!m_sel.sprite)
		{
			if(m_sel.fwrite && m_sel.fge || m_sel.zb)
			{
				vmovaps(xmm0, ptr[ebx + 16]); // v.p

				if(m_sel.fwrite && m_sel.fge)
				{
					// f = GSVector4i(vp).zzzzh().zzzz().add16(m_local.d[skip].f);

					vcvttps2dq(xmm1, xmm0);
					vpshufhw(xmm1, xmm1, _MM_SHUFFLE(2, 2, 2, 2));
					vpshufd(xmm1, xmm1, _MM_SHUFFLE(2, 2, 2, 2));
					vpaddw(xmm1, ptr[edx + 16 * 6]);

					vmovdqa(ptr[&m_local.temp.f], xmm1);
				}

				if(m_sel.zb)
				{
					// z = vp.zzzz() + m_local.d[skip].z;

					vshufps(xmm0, xmm0, _MM_SHUFFLE(2, 2, 2, 2));
					vaddps(xmm0, ptr[edx]);

					vmovaps(ptr[&m_local.temp.z], xmm0);
				}
			}
		}
		else
		{
			if(m_sel.ztest)
			{
				vmovdqa(xmm0, ptr[&m_local.p.z]);
			}
		}

		if(m_sel.fb)
		{
			if(m_sel.edge || m_sel.tfx != TFX_NONE)
			{
				vmovaps(xmm4, ptr[ebx + 32]); // v.t
			}

			if(m_sel.edge)
			{
				vpshufhw(xmm3, xmm4, _MM_SHUFFLE(2, 2, 2, 2));
				vpshufd(xmm3, xmm3, _MM_SHUFFLE(3, 3, 3, 3));
				vpsrlw(xmm3, 9);

				vmovdqa(ptr[&m_local.temp.cov], xmm3);
			}

			if(m_sel.tfx != TFX_NONE)
			{
				if(m_sel.fst)
				{
					// GSVector4i vti(vt);

					vcvttps2dq(xmm4, xmm4);

					// si = vti.xxxx() + m_local.d[skip].si;
					// ti = vti.yyyy(); if(!sprite) ti += m_local.d[skip].ti;

					vpshufd(xmm2, xmm4, _MM_SHUFFLE(0, 0, 0, 0));
					vpshufd(xmm3, xmm4, _MM_SHUFFLE(1, 1, 1, 1));

					vpaddd(xmm2, ptr[edx + 16 * 7]);

					if(!m_sel.sprite)
					{
						vpaddd(xmm3, ptr[edx + 16 * 8]);
					}
					else
					{
						if(m_sel.ltf)
						{
							vpshuflw(xmm4, xmm3, _MM_SHUFFLE(2, 2, 0, 0));
							vpshufhw(xmm4, xmm4, _MM_SHUFFLE(2, 2, 0, 0));
							vpsrlw(xmm4, 1);
							vmovdqa(ptr[&m_local.temp.vf], xmm4);
						}
					}

					vmovdqa(ptr[&m_local.temp.s], xmm2);
					vmovdqa(ptr[&m_local.temp.t], xmm3);
				}
				else
				{
					// s = vt.xxxx() + m_local.d[skip].s;
					// t = vt.yyyy() + m_local.d[skip].t;
					// q = vt.zzzz() + m_local.d[skip].q;

					vshufps(xmm2, xmm4, xmm4, _MM_SHUFFLE(0, 0, 0, 0));
					vshufps(xmm3, xmm4, xmm4, _MM_SHUFFLE(1, 1, 1, 1));
					vshufps(xmm4, xmm4, xmm4, _MM_SHUFFLE(2, 2, 2, 2));

					vaddps(xmm2, ptr[edx + 16 * 1]);
					vaddps(xmm3, ptr[edx + 16 * 2]);
					vaddps(xmm4, ptr[edx + 16 * 3]);

					vmovaps(ptr[&m_local.temp.s], xmm2);
					vmovaps(ptr[&m_local.temp.t], xmm3);
					vmovaps(ptr[&m_local.temp.q], xmm4);

					vrcpps(xmm4, xmm4);
					vmulps(xmm2, xmm4);
					vmulps(xmm3, xmm4);
				}
			}

			if(!(m_sel.tfx == TFX_DECAL && m_sel.tcc))
			{
				if(m_sel.iip)
				{
					// GSVector4i vc = GSVector4i(v.c);

					vcvttps2dq(xmm6, ptr[ebx]); // v.c

					// vc = vc.upl16(vc.zwxy());

					vpshufd(xmm5, xmm6, _MM_SHUFFLE(1, 0, 3, 2));
					vpunpcklwd(xmm6, xmm5);

					// rb = vc.xxxx().add16(m_local.d[skip].rb);
					// ga = vc.zzzz().add16(m_local.d[skip].ga);

					vpshufd(xmm5, xmm6, _MM_SHUFFLE(0, 0, 0, 0));
					vpshufd(xmm6, xmm6, _MM_SHUFFLE(2, 2, 2, 2));

					vpaddw(xmm5, ptr[edx + 16 * 4]);
					vpaddw(xmm6, ptr[edx + 16 * 5]);

					vmovdqa(ptr[&m_local.temp.rb], xmm5);
					vmovdqa(ptr[&m_local.temp.ga], xmm6);
				}
				else
				{
					if(m_sel.tfx == TFX_NONE)
					{
						vmovdqa(xmm5, ptr[&m_local.c.rb]);
						vmovdqa(xmm6, ptr[&m_local.c.ga]);
					}
				}
			}
		}
	}
	else
	{
		// GSVector4i test = m_test[skip] | m_test[7 + (steps & (steps >> 31))];

		shl(edx, 4);

		movdqa(xmm7, ptr[edx + (size_t)&m_test[0]]);

		mov(eax, ecx);
		sar(eax, 31);
		and(eax, ecx);
		shl(eax, 4);

		por(xmm7, ptr[eax + (size_t)&m_test[7]]);

		// GSVector2i* fza_base = &m_local.gd->fzbr[top];

		mov(esi, dword[esp + _top]);
		lea(esi, ptr[esi * 8]);
		add(esi, dword[&m_local.gd->fzbr]);

		// GSVector2i* fza_offset = &m_local.gd->fzbc[left >> 2];

		lea(edi, ptr[ebx * 2]);
		add(edi, dword[&m_local.gd->fzbc]);

		if(!m_sel.sprite && (m_sel.fwrite && m_sel.fge || m_sel.zb) || m_sel.fb && (m_sel.edge || m_sel.tfx != TFX_NONE || m_sel.iip))
		{
			// edx = &m_local.d[skip]

			shl(edx, 4);
			lea(edx, ptr[edx + (size_t)m_local.d]);

			// ebx = &v

			mov(ebx, dword[esp + _v]);
		}

		if(!m_sel.sprite)
		{
			if(m_sel.fwrite && m_sel.fge || m_sel.zb)
			{
				movaps(xmm0, ptr[ebx + 16]); // v.p

				if(m_sel.fwrite && m_sel.fge)
				{
					// f = GSVector4i(vp).zzzzh().zzzz().add16(m_local.d[skip].f);

					cvttps2dq(xmm1, xmm0);
					pshufhw(xmm1, xmm1, _MM_SHUFFLE(2, 2, 2, 2));
					pshufd(xmm1, xmm1, _MM_SHUFFLE(2, 2, 2, 2));
					paddw(xmm1, ptr[edx + 16 * 6]);

					movdqa(ptr[&m_local.temp.f], xmm1);
				}

				if(m_sel.zb)
				{
					// z = vp.zzzz() + m_local.d[skip].z;

					shufps(xmm0, xmm0, _MM_SHUFFLE(2, 2, 2, 2));
					addps(xmm0, ptr[edx]);

					movaps(ptr[&m_local.temp.z], xmm0);
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
				movaps(xmm4, ptr[ebx + 32]); // v.t
			}

			if(m_sel.edge)
			{
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

					cvttps2dq(xmm4, xmm4);

					// si = vti.xxxx() + m_local.d[skip].si;
					// ti = vti.yyyy(); if(!sprite) ti += m_local.d[skip].ti;

					pshufd(xmm2, xmm4, _MM_SHUFFLE(0, 0, 0, 0));
					pshufd(xmm3, xmm4, _MM_SHUFFLE(1, 1, 1, 1));

					paddd(xmm2, ptr[edx + 16 * 7]);

					if(!m_sel.sprite)
					{
						paddd(xmm3, ptr[edx + 16 * 8]);
					}
					else
					{
						if(m_sel.ltf)
						{
							movdqa(xmm4, xmm3);
							pshuflw(xmm4, xmm4, _MM_SHUFFLE(2, 2, 0, 0));
							pshufhw(xmm4, xmm4, _MM_SHUFFLE(2, 2, 0, 0));
							psrlw(xmm4, 1);
							movdqa(ptr[&m_local.temp.vf], xmm4);
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

					addps(xmm2, ptr[edx + 16 * 1]);
					addps(xmm3, ptr[edx + 16 * 2]);
					addps(xmm4, ptr[edx + 16 * 3]);

					movaps(ptr[&m_local.temp.s], xmm2);
					movaps(ptr[&m_local.temp.t], xmm3);
					movaps(ptr[&m_local.temp.q], xmm4);

					rcpps(xmm4, xmm4);
					mulps(xmm2, xmm4);
					mulps(xmm3, xmm4);
				}
			}

			if(!(m_sel.tfx == TFX_DECAL && m_sel.tcc))
			{
				if(m_sel.iip)
				{
					// GSVector4i vc = GSVector4i(v.c);

					cvttps2dq(xmm6, ptr[ebx]); // v.c

					// vc = vc.upl16(vc.zwxy());

					pshufd(xmm5, xmm6, _MM_SHUFFLE(1, 0, 3, 2));
					punpcklwd(xmm6, xmm5);

					// rb = vc.xxxx().add16(m_local.d[skip].rb);
					// ga = vc.zzzz().add16(m_local.d[skip].ga);

					pshufd(xmm5, xmm6, _MM_SHUFFLE(0, 0, 0, 0));
					pshufd(xmm6, xmm6, _MM_SHUFFLE(2, 2, 2, 2));

					paddw(xmm5, ptr[edx + 16 * 4]);
					paddw(xmm6, ptr[edx + 16 * 5]);

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
}

void GSDrawScanlineCodeGenerator::Step()
{
	// steps -= 4;

	sub(ecx, 4);

	// fza_offset++;

	add(edi, 8);

	if(m_cpu.has(util::Cpu::tAVX))
	{
		if(!m_sel.sprite)
		{
			// z += m_local.d4.z;

			if(m_sel.zb)
			{
				vmovaps(xmm0, ptr[&m_local.temp.z]);
				vaddps(xmm0, ptr[&m_local.d4.z]);
				vmovaps(ptr[&m_local.temp.z], xmm0);
			}

			// f = f.add16(m_local.d4.f);

			if(m_sel.fwrite && m_sel.fge)
			{
				vmovdqa(xmm1, ptr[&m_local.temp.f]);
				vpaddw(xmm1, ptr[&m_local.d4.f]);
				vmovdqa(ptr[&m_local.temp.f], xmm1);
			}
		}
		else
		{
			if(m_sel.ztest)
			{
				vmovdqa(xmm0, ptr[&m_local.p.z]);
			}
		}

		if(m_sel.fb)
		{
			if(m_sel.tfx != TFX_NONE)
			{
				if(m_sel.fst)
				{
					// GSVector4i st = m_local.d4.st;

					// si += st.xxxx();
					// if(!sprite) ti += st.yyyy();

					vmovdqa(xmm4, ptr[&m_local.d4.st]);

					vpshufd(xmm2, xmm4, _MM_SHUFFLE(0, 0, 0, 0));
					vpaddd(xmm2, ptr[&m_local.temp.s]);
					vmovdqa(ptr[&m_local.temp.s], xmm2);

					if(!m_sel.sprite)
					{
						vpshufd(xmm3, xmm4, _MM_SHUFFLE(1, 1, 1, 1));
						vpaddd(xmm3, ptr[&m_local.temp.t]);
						vmovdqa(ptr[&m_local.temp.t], xmm3);
					}
					else
					{
						vmovdqa(xmm3, ptr[&m_local.temp.t]);
					}
				}
				else
				{
					// GSVector4 stq = m_local.d4.stq;

					// s += stq.xxxx();
					// t += stq.yyyy();
					// q += stq.zzzz();

					vmovaps(xmm4, ptr[&m_local.d4.stq]);
				
					vshufps(xmm2, xmm4, xmm4, _MM_SHUFFLE(0, 0, 0, 0));
					vshufps(xmm3, xmm4, xmm4, _MM_SHUFFLE(1, 1, 1, 1));
					vshufps(xmm4, xmm4, xmm4, _MM_SHUFFLE(2, 2, 2, 2));

					vaddps(xmm2, ptr[&m_local.temp.s]);
					vaddps(xmm3, ptr[&m_local.temp.t]);
					vaddps(xmm4, ptr[&m_local.temp.q]);

					vmovaps(ptr[&m_local.temp.s], xmm2);
					vmovaps(ptr[&m_local.temp.t], xmm3);
					vmovaps(ptr[&m_local.temp.q], xmm4);

					vrcpps(xmm4, xmm4);
					vmulps(xmm2, xmm4);
					vmulps(xmm3, xmm4);
				}
			}

			if(!(m_sel.tfx == TFX_DECAL && m_sel.tcc))
			{
				if(m_sel.iip)
				{
					// GSVector4i c = m_local.d4.c;

					// rb = rb.add16(c.xxxx());
					// ga = ga.add16(c.yyyy());

					vmovdqa(xmm7, ptr[&m_local.d4.c]);

					vpshufd(xmm5, xmm7, _MM_SHUFFLE(0, 0, 0, 0));
					vpshufd(xmm6, xmm7, _MM_SHUFFLE(1, 1, 1, 1));

					vpaddw(xmm5, ptr[&m_local.temp.rb]);
					vpaddw(xmm6, ptr[&m_local.temp.ga]);

					vmovdqa(ptr[&m_local.temp.rb], xmm5);
					vmovdqa(ptr[&m_local.temp.ga], xmm6);
				}
				else
				{
					if(m_sel.tfx == TFX_NONE)
					{
						vmovdqa(xmm5, ptr[&m_local.c.rb]);
						vmovdqa(xmm6, ptr[&m_local.c.ga]);
					}
				}
			}
		}

		// test = m_test[7 + (steps & (steps >> 31))];

		mov(edx, ecx);
		sar(edx, 31);
		and(edx, ecx);
		shl(edx, 4);

		vmovdqa(xmm7, ptr[edx + (size_t)&m_test[7]]);
	}
	else
	{
		if(!m_sel.sprite)
		{
			// z += m_local.d4.z;

			if(m_sel.zb)
			{
				movaps(xmm0, ptr[&m_local.temp.z]);
				addps(xmm0, ptr[&m_local.d4.z]);
				movaps(ptr[&m_local.temp.z], xmm0);
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
					// GSVector4i st = m_local.d4.st;

					// si += st.xxxx();
					// if(!sprite) ti += st.yyyy();

					movdqa(xmm4, ptr[&m_local.d4.st]);

					pshufd(xmm2, xmm4, _MM_SHUFFLE(0, 0, 0, 0));
					paddd(xmm2, ptr[&m_local.temp.s]);
					movdqa(ptr[&m_local.temp.s], xmm2);

					if(!m_sel.sprite)
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

					movaps(xmm2, ptr[&m_local.d4.stq]);
					movaps(xmm3, xmm2);
					movaps(xmm4, xmm2);

					shufps(xmm2, xmm2, _MM_SHUFFLE(0, 0, 0, 0));
					shufps(xmm3, xmm3, _MM_SHUFFLE(1, 1, 1, 1));
					shufps(xmm4, xmm4, _MM_SHUFFLE(2, 2, 2, 2));

					addps(xmm2, ptr[&m_local.temp.s]);
					addps(xmm3, ptr[&m_local.temp.t]);
					addps(xmm4, ptr[&m_local.temp.q]);

					movaps(ptr[&m_local.temp.s], xmm2);
					movaps(ptr[&m_local.temp.t], xmm3);
					movaps(ptr[&m_local.temp.q], xmm4);

					rcpps(xmm4, xmm4);
					mulps(xmm2, xmm4);
					mulps(xmm3, xmm4);
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

	mov(ebp, dword[esi + 4]);
	add(ebp, dword[edi + 4]);

	if(m_cpu.has(util::Cpu::tAVX))
	{
		// GSVector4i zs = zi;

		if(!m_sel.sprite)
		{
			if(m_sel.zoverflow)
			{
				// zs = (GSVector4i(z * 0.5f) << 1) | (GSVector4i(z) & GSVector4i::x00000001());

				static float half = 0.5f;

				vbroadcastss(temp1, dword[&half]);
				vmulps(temp1, xmm0);
				vcvttps2dq(temp1, temp1);
				vpslld(temp1, 1);

				vcvttps2dq(xmm0, xmm0);
				vpcmpeqd(temp2, temp2);
				vpsrld(temp2, 31);
				vpand(xmm0, temp2);

				vpor(xmm0, temp1);
			}
			else
			{
				// zs = GSVector4i(z);

				vcvttps2dq(xmm0, xmm0);
			}

			if(m_sel.zwrite)
			{
				vmovdqa(ptr[&m_local.temp.zs], xmm0);
			}
		}

		if(m_sel.ztest)
		{
			ReadPixel(xmm1, ebp);

			if(m_sel.zwrite && m_sel.zpsm < 2)
			{
				vmovdqa(ptr[&m_local.temp.zd], xmm1);
			}

			// zd &= 0xffffffff >> m_sel.zpsm * 8;

			if(m_sel.zpsm)
			{
				vpslld(xmm1, m_sel.zpsm * 8);
				vpsrld(xmm1, m_sel.zpsm * 8);
			}

			if(m_sel.zoverflow || m_sel.zpsm == 0)
			{
				// GSVector4i o = GSVector4i::x80000000();

				vpcmpeqd(xmm4, xmm4);
				vpslld(xmm4, 31);

				// GSVector4i zso = zs - o;

				vpsubd(xmm0, xmm4);

				// GSVector4i zdo = zd - o;

				vpsubd(xmm1, xmm4);
			}

			switch(m_sel.ztst)
			{
			case ZTST_GEQUAL:
				// test |= zso < zdo; // ~(zso >= zdo)
				vpcmpgtd(xmm1, xmm0);
				vpor(xmm7, xmm1);
				break;

			case ZTST_GREATER: // TODO: tidus hair and chocobo wings only appear fully when this is tested as ZTST_GEQUAL
				// test |= zso <= zdo; // ~(zso > zdo)
				vpcmpgtd(xmm0, xmm1);
				vpcmpeqd(xmm4, xmm4);
				vpxor(xmm0, xmm4);
				vpor(xmm7, xmm0);
				break;
			}

			alltrue();
		}
	}
	else
	{
		// GSVector4i zs = zi;

		if(!m_sel.sprite)
		{
			if(m_sel.zoverflow)
			{
				// zs = (GSVector4i(z * 0.5f) << 1) | (GSVector4i(z) & GSVector4i::x00000001());

				static float half = 0.5f;

				movss(temp1, dword[&half]);
				shufps(temp1, temp1, _MM_SHUFFLE(0, 0, 0, 0));
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

				pcmpeqd(xmm4, xmm4);
				pslld(xmm4, 31);

				// GSVector4i zso = zs - o;

				psubd(xmm0, xmm4);

				// GSVector4i zdo = zd - o;

				psubd(xmm1, xmm4);
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
				pcmpeqd(xmm4, xmm4);
				pxor(xmm0, xmm4);
				por(xmm7, xmm0);
				break;
			}

			alltrue();
		}
	}
}

void GSDrawScanlineCodeGenerator::SampleTexture()
{
	if(!m_sel.fb || m_sel.tfx == TFX_NONE)
	{
		return;
	}

	mov(ebx, dword[&m_local.gd->tex]);

	// ebx = tex

	if(m_cpu.has(util::Cpu::tAVX))
	{
		if(!m_sel.fst)
		{
			// TODO: move these into Init/Step too?

			vcvttps2dq(xmm2, xmm2);
			vcvttps2dq(xmm3, xmm3);

			if(m_sel.ltf)
			{
				// u -= 0x8000;
				// v -= 0x8000;

				mov(eax, 0x8000);
				vmovd(xmm4, eax);
				vpshufd(xmm4, xmm4, _MM_SHUFFLE(0, 0, 0, 0));

				vpsubd(xmm2, xmm4);
				vpsubd(xmm3, xmm4);
			}
		}

		// xmm2 = u
		// xmm3 = v

		if(m_sel.ltf)
		{
			// GSVector4i uf = u.xxzzlh().srl16(1);

			vpshuflw(xmm0, xmm2, _MM_SHUFFLE(2, 2, 0, 0));
			vpshufhw(xmm0, xmm0, _MM_SHUFFLE(2, 2, 0, 0));
			vpsrlw(xmm0, 1);
			vmovdqa(ptr[&m_local.temp.uf], xmm0);

			if(!m_sel.sprite)
			{
				// GSVector4i vf = v.xxzzlh().srl16(1);

				vpshuflw(xmm1, xmm3, _MM_SHUFFLE(2, 2, 0, 0));
				vpshufhw(xmm1, xmm1, _MM_SHUFFLE(2, 2, 0, 0));
				vpsrlw(xmm1, 1);
				vmovdqa(ptr[&m_local.temp.vf], xmm1);
			}
		}

		// GSVector4i uv0 = u.sra32(16).ps32(v.sra32(16));

		vpsrad(xmm2, 16);
		vpsrad(xmm3, 16);
		vpackssdw(xmm2, xmm3);

		if(m_sel.ltf)
		{
			// GSVector4i uv1 = uv0.add16(GSVector4i::x0001());

			vpcmpeqd(xmm1, xmm1);
			vpsrlw(xmm1, 15);
			vpaddw(xmm3, xmm2, xmm1);

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

		// GSVector4i x0 = uv0.upl16();
		// GSVector4i y0 = uv0.uph16() << tw;

		vpxor(xmm0, xmm0);

		vpunpcklwd(xmm4, xmm2, xmm0);
		vpunpckhwd(xmm2, xmm2, xmm0);
		vpslld(xmm2, m_sel.tw + 3);

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

			vpunpcklwd(xmm6, xmm3, xmm0);
			vpunpckhwd(xmm3, xmm3, xmm0);
			vpslld(xmm3, m_sel.tw + 3);

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

			vpaddd(xmm5, xmm2, xmm4);
			vpaddd(xmm2, xmm2, xmm6);
			vpaddd(xmm0, xmm3, xmm4);
			vpaddd(xmm3, xmm3, xmm6);

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

			ReadTexel(xmm6, xmm5, xmm1, xmm4);

			// xmm2, xmm5, xmm1 = free

			ReadTexel(xmm4, xmm2, xmm5, xmm1);

			// xmm0, xmm2, xmm5 = free

			ReadTexel(xmm1, xmm0, xmm2, xmm5);

			// xmm3, xmm0, xmm2 = free

			ReadTexel(xmm5, xmm3, xmm0, xmm2);

			// xmm6 = c00
			// xmm4 = c01
			// xmm1 = c10
			// xmm5 = c11
			// xmm0, xmm2, xmm3 = free
			// xmm7 = used

			vmovdqa(xmm0, ptr[&m_local.temp.uf]);

			// GSVector4i rb00 = c00 & mask;
			// GSVector4i ga00 = (c00 >> 8) & mask;

			vpsllw(xmm2, xmm6, 8);
			vpsrlw(xmm2, 8);
			vpsrlw(xmm6, 8);

			// GSVector4i rb01 = c01 & mask;
			// GSVector4i ga01 = (c01 >> 8) & mask;

			vpsllw(xmm3, xmm4, 8);
			vpsrlw(xmm3, 8);
			vpsrlw(xmm4, 8);

			// xmm0 = uf
			// xmm2 = rb00
			// xmm3 = rb01
			// xmm6 = ga00
			// xmm4 = ga01
			// xmm1 = c10
			// xmm5 = c11
			// xmm7 = used

			// rb00 = rb00.lerp16<0>(rb01, uf);
			// ga00 = ga00.lerp16<0>(ga01, uf);

			lerp16<0>(xmm3, xmm2, xmm0);
			lerp16<0>(xmm4, xmm6, xmm0);

			// xmm0 = uf
			// xmm3 = rb00
			// xmm4 = ga00
			// xmm1 = c10
			// xmm5 = c11
			// xmm2, xmm6 = free
			// xmm7 = used

			// GSVector4i rb10 = c10 & mask;
			// GSVector4i ga10 = (c10 >> 8) & mask;

			vpsrlw(xmm2, xmm1, 8);
			vpsllw(xmm1, 8);
			vpsrlw(xmm1, 8);

			// GSVector4i rb11 = c11 & mask;
			// GSVector4i ga11 = (c11 >> 8) & mask;

			vpsrlw(xmm6, xmm5, 8);
			vpsllw(xmm5, 8);
			vpsrlw(xmm5, 8);

			// xmm0 = uf
			// xmm3 = rb00
			// xmm4 = ga00
			// xmm1 = rb10
			// xmm5 = rb11
			// xmm2 = ga10
			// xmm6 = ga11
			// xmm7 = used

			// rb10 = rb10.lerp16<0>(rb11, uf);
			// ga10 = ga10.lerp16<0>(ga11, uf);

			lerp16<0>(xmm5, xmm1, xmm0);
			lerp16<0>(xmm6, xmm2, xmm0);

			// xmm3 = rb00
			// xmm4 = ga00
			// xmm5 = rb10
			// xmm6 = ga10
			// xmm0, xmm1, xmm2 = free
			// xmm7 = used

			// rb00 = rb00.lerp16<0>(rb10, vf);
			// ga00 = ga00.lerp16<0>(ga10, vf);

			vmovdqa(xmm0, ptr[&m_local.temp.vf]);

			lerp16<0>(xmm5, xmm3, xmm0);
			lerp16<0>(xmm6, xmm4, xmm0);
		}
		else
		{
			// GSVector4i addr00 = y0 + x0;

			vpaddd(xmm2, xmm4);

			// c00 = addr00.gather32_32((const uint32/uint8*)tex[, clut]);

			ReadTexel(xmm5, xmm2, xmm0, xmm1);

			// GSVector4i mask = GSVector4i::x00ff();

			// c[0] = c00 & mask;
			// c[1] = (c00 >> 8) & mask;

			vpsrlw(xmm6, xmm5, 8);
			vpsllw(xmm5, 8);
			vpsrlw(xmm5, 8);
		}
	}
	else
	{
		if(!m_sel.fst)
		{
			// TODO: move these into Init/Step too?

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

			movdqa(xmm0, xmm2);
			pshuflw(xmm0, xmm0, _MM_SHUFFLE(2, 2, 0, 0));
			pshufhw(xmm0, xmm0, _MM_SHUFFLE(2, 2, 0, 0));
			psrlw(xmm0, 1);
			movdqa(ptr[&m_local.temp.uf], xmm0);

			if(!m_sel.sprite)
			{
				// GSVector4i vf = v.xxzzlh().srl16(1);

				movdqa(xmm1, xmm3);
				pshuflw(xmm1, xmm1, _MM_SHUFFLE(2, 2, 0, 0));
				pshufhw(xmm1, xmm1, _MM_SHUFFLE(2, 2, 0, 0));
				psrlw(xmm1, 1);
				movdqa(ptr[&m_local.temp.vf], xmm1);
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

			ReadTexel(xmm6, xmm5, xmm1, xmm4);

			// xmm2, xmm5, xmm1 = free

			ReadTexel(xmm4, xmm2, xmm5, xmm1);

			// xmm0, xmm2, xmm5 = free

			ReadTexel(xmm1, xmm0, xmm2, xmm5);

			// xmm3, xmm0, xmm2 = free

			ReadTexel(xmm5, xmm3, xmm0, xmm2);

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

			// rb00 = rb00.lerp16<0>(rb01, uf);
			// ga00 = ga00.lerp16<0>(ga01, uf);

			lerp16<0>(xmm3, xmm2, xmm0);
			lerp16<0>(xmm4, xmm6, xmm0);

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

			// rb10 = rb10.lerp16<0>(rb11, uf);
			// ga10 = ga10.lerp16<0>(ga11, uf);

			lerp16<0>(xmm5, xmm1, xmm0);
			lerp16<0>(xmm6, xmm2, xmm0);

			// xmm3 = rb00
			// xmm4 = ga00
			// xmm5 = rb10
			// xmm6 = ga10
			// xmm0, xmm1, xmm2 = free
			// xmm7 = used

			// rb00 = rb00.lerp16<0>(rb10, vf);
			// ga00 = ga00.lerp16<0>(ga10, vf);

			movdqa(xmm0, ptr[&m_local.temp.vf]);

			lerp16<0>(xmm5, xmm3, xmm0);
			lerp16<0>(xmm6, xmm4, xmm0);
		}
		else
		{
			// GSVector4i addr00 = y0 + x0;

			paddd(xmm2, xmm4);

			// c00 = addr00.gather32_32((const uint32/uint8*)tex[, clut]);

			ReadTexel(xmm5, xmm2, xmm0, xmm1);

			// GSVector4i mask = GSVector4i::x00ff();

			// c[0] = c00 & mask;
			// c[1] = (c00 >> 8) & mask;

			movdqa(xmm6, xmm5);

			psllw(xmm5, 8);
			psrlw(xmm5, 8);
			psrlw(xmm6, 8);
		}
	}
}

void GSDrawScanlineCodeGenerator::Wrap(const Xmm& uv)
{
	// xmm0, xmm1, xmm4, xmm5, xmm6 = free

	int wms_clamp = ((m_sel.wms + 1) >> 1) & 1;
	int wmt_clamp = ((m_sel.wmt + 1) >> 1) & 1;

	int region = ((m_sel.wms | m_sel.wmt) >> 1) & 1;

	if(m_cpu.has(util::Cpu::tAVX))
	{
		if(wms_clamp == wmt_clamp)
		{
			if(wms_clamp)
			{
				if(region)
				{
					vpmaxsw(uv, ptr[&m_local.gd->t.min]);
				}
				else
				{
					vpxor(xmm0, xmm0);
					vpmaxsw(uv, xmm0);
				}

				vpminsw(uv, ptr[&m_local.gd->t.max]);
			}
			else
			{
				vpand(uv, ptr[&m_local.gd->t.min]);

				if(region)
				{
					vpor(uv, ptr[&m_local.gd->t.max]);
				}
			}
		}
		else
		{
			vmovdqa(xmm1, uv);

			vmovdqa(xmm4, ptr[&m_local.gd->t.min]);
			vmovdqa(xmm5, ptr[&m_local.gd->t.max]);

			// GSVector4i clamp = t.sat_i16(m_local.gd->t.min, m_local.gd->t.max);

			vpmaxsw(uv, xmm4);
			vpminsw(uv, xmm5);

			// GSVector4i repeat = (t & m_local.gd->t.min) | m_local.gd->t.max;

			vpand(xmm1, xmm4);

			if(region)
			{
				vpor(xmm1, xmm5);
			}

			// clamp.blend8(repeat, m_local.gd->t.mask);

			vmovdqa(xmm0, ptr[&m_local.gd->t.mask]);

			vpblendvb(uv, xmm1, xmm0);
		}
	}
	else
	{
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
			movdqa(xmm1, uv);

			movdqa(xmm4, ptr[&m_local.gd->t.min]);
			movdqa(xmm5, ptr[&m_local.gd->t.max]);

			// GSVector4i clamp = t.sat_i16(m_local.gd->t.min, m_local.gd->t.max);

			pmaxsw(uv, xmm4);
			pminsw(uv, xmm5);

			// GSVector4i repeat = (t & m_local.gd->t.min) | m_local.gd->t.max;

			pand(xmm1, xmm4);

			if(region)
			{
				por(xmm1, xmm5);
			}

			// clamp.blend8(repeat, m_local.gd->t.mask);

			movdqa(xmm0, ptr[&m_local.gd->t.mask]);
			blend8(uv, xmm1);
		}
	}
}

void GSDrawScanlineCodeGenerator::Wrap(const Xmm& uv0, const Xmm& uv1)
{
	// xmm0, xmm1, xmm4, xmm5, xmm6 = free

	int wms_clamp = ((m_sel.wms + 1) >> 1) & 1;
	int wmt_clamp = ((m_sel.wmt + 1) >> 1) & 1;

	int region = ((m_sel.wms | m_sel.wmt) >> 1) & 1;

	if(m_cpu.has(util::Cpu::tAVX))
	{
		if(wms_clamp == wmt_clamp)
		{
			if(wms_clamp)
			{
				if(region)
				{
					vmovdqa(xmm4, ptr[&m_local.gd->t.min]);

					vpmaxsw(uv0, xmm4);
					vpmaxsw(uv1, xmm4);
				}
				else
				{
					vpxor(xmm0, xmm0);

					vpmaxsw(uv0, xmm0);
					vpmaxsw(uv1, xmm0);
				}

				vmovdqa(xmm5, ptr[&m_local.gd->t.max]);

				vpminsw(uv0, xmm5);
				vpminsw(uv1, xmm5);
			}
			else
			{
				vmovdqa(xmm4, ptr[&m_local.gd->t.min]);

				vpand(uv0, xmm4);
				vpand(uv1, xmm4);

				if(region)
				{
					vmovdqa(xmm5, ptr[&m_local.gd->t.max]);

					vpor(uv0, xmm5);
					vpor(uv1, xmm5);
				}
			}
		}
		else
		{
			vmovdqa(xmm1, uv0);
			vmovdqa(xmm6, uv1);

			vmovdqa(xmm4, ptr[&m_local.gd->t.min]);
			vmovdqa(xmm5, ptr[&m_local.gd->t.max]);

			// GSVector4i clamp = t.sat_i16(m_local.gd->t.min, m_local.gd->t.max);

			vpmaxsw(uv0, xmm4);
			vpmaxsw(uv1, xmm4);
			vpminsw(uv0, xmm5);
			vpminsw(uv1, xmm5);

			// GSVector4i repeat = (t & m_local.gd->t.min) | m_local.gd->t.max;

			vpand(xmm1, xmm4);
			vpand(xmm6, xmm4);

			if(region)
			{
				vpor(xmm1, xmm5);
				vpor(xmm6, xmm5);
			}

			// clamp.blend8(repeat, m_local.gd->t.mask);

			vmovdqa(xmm0, ptr[&m_local.gd->t.mask]);

			vpblendvb(uv0, xmm1, xmm0);
			vpblendvb(uv1, xmm6, xmm0);
		}
	}
	else
	{
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
			movdqa(xmm1, uv0);
			movdqa(xmm6, uv1);

			movdqa(xmm4, ptr[&m_local.gd->t.min]);
			movdqa(xmm5, ptr[&m_local.gd->t.max]);

			// GSVector4i clamp = t.sat_i16(m_local.gd->t.min, m_local.gd->t.max);

			pmaxsw(uv0, xmm4);
			pmaxsw(uv1, xmm4);
			pminsw(uv0, xmm5);
			pminsw(uv1, xmm5);

			// GSVector4i repeat = (t & m_local.gd->t.min) | m_local.gd->t.max;

			pand(xmm1, xmm4);
			pand(xmm6, xmm4);

			if(region)
			{
				por(xmm1, xmm5);
				por(xmm6, xmm5);
			}

			// clamp.blend8(repeat, m_local.gd->t.mask);

			if(m_cpu.has(util::Cpu::tSSE41))
			{
				movdqa(xmm0, ptr[&m_local.gd->t.mask]);

				pblendvb(uv0, xmm1);
				pblendvb(uv1, xmm6);
			}
			else
			{
				movdqa(xmm0, ptr[&m_local.gd->t.invmask]);
				movdqa(xmm4, xmm0);

				pand(uv0, xmm0);
				pandn(xmm0, xmm1);
				por(uv0, xmm0);

				pand(uv1, xmm4);
				pandn(xmm4, xmm6);
				por(uv1, xmm4);
			}
		}
	}
}

void GSDrawScanlineCodeGenerator::AlphaTFX()
{
	if(!m_sel.fb)
	{
		return;
	}

	if(m_cpu.has(util::Cpu::tAVX))
	{
		switch(m_sel.tfx)
		{
		case TFX_MODULATE:

			// GSVector4i ga = iip ? gaf : m_local.c.ga;

			vmovdqa(xmm4, ptr[m_sel.iip ? &m_local.temp.ga : &m_local.c.ga]);

			// gat = gat.modulate16<1>(ga).clamp8();

			modulate16<1>(xmm6, xmm4);

			clamp16(xmm6, xmm3);

			// if(!tcc) gat = gat.mix16(ga.srl16(7));

			if(!m_sel.tcc)
			{
				vpsrlw(xmm4, 7);

				mix16(xmm6, xmm4, xmm3);
			}

			break;

		case TFX_DECAL:

			// if(!tcc) gat = gat.mix16(ga.srl16(7));

			if(!m_sel.tcc)
			{
				// GSVector4i ga = iip ? gaf : m_local.c.ga;

				vmovdqa(xmm4, ptr[m_sel.iip ? &m_local.temp.ga : &m_local.c.ga]);

				vpsrlw(xmm4, 7);

				mix16(xmm6, xmm4, xmm3);
			}

			break;

		case TFX_HIGHLIGHT:

			// GSVector4i ga = iip ? gaf : m_local.c.ga;

			vmovdqa(xmm4, ptr[m_sel.iip ? &m_local.temp.ga : &m_local.c.ga]);
			vmovdqa(xmm2, xmm4);

			// gat = gat.mix16(!tcc ? ga.srl16(7) : gat.addus8(ga.srl16(7)));

			vpsrlw(xmm4, 7);

			if(m_sel.tcc)
			{
				vpaddusb(xmm4, xmm6);
			}

			mix16(xmm6, xmm4, xmm3);

			break;

		case TFX_HIGHLIGHT2:

			// if(!tcc) gat = gat.mix16(ga.srl16(7));

			if(!m_sel.tcc)
			{
				// GSVector4i ga = iip ? gaf : m_local.c.ga;

				vmovdqa(xmm4, ptr[m_sel.iip ? &m_local.temp.ga : &m_local.c.ga]);
				vmovdqa(xmm2, xmm4);

				vpsrlw(xmm4, 7);

				mix16(xmm6, xmm4, xmm3);
			}

			break;

		case TFX_NONE:

			// gat = iip ? ga.srl16(7) : ga;

			if(m_sel.iip)
			{
				vpsrlw(xmm6, 7);
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
					vmovdqa(xmm0, ptr[&m_local.temp.cov]);
				}
				else
				{
					vpcmpeqd(xmm0, xmm0);
					vpsllw(xmm0, 15);
					vpsrlw(xmm0, 8);
				}

				mix16(xmm6, xmm0, xmm1);
			}
			else
			{
				// a = a == 0x80 ? cov : a

				vpcmpeqd(xmm0, xmm0);
				vpsllw(xmm0, 15);
				vpsrlw(xmm0, 8);

				if(m_sel.edge)
				{
					vmovdqa(xmm1, ptr[&m_local.temp.cov]);
				}
				else
				{
					vmovdqa(xmm1, xmm0);
				}

				vpcmpeqw(xmm0, xmm6);
				vpsrld(xmm0, 16);
				vpslld(xmm0, 16);

				vpblendvb(xmm6, xmm1, xmm0);
			}
		}
	}
	else
	{
		switch(m_sel.tfx)
		{
		case TFX_MODULATE:

			// GSVector4i ga = iip ? gaf : m_local.c.ga;

			movdqa(xmm4, ptr[m_sel.iip ? &m_local.temp.ga : &m_local.c.ga]);

			// gat = gat.modulate16<1>(ga).clamp8();

			modulate16<1>(xmm6, xmm4);

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
}

void GSDrawScanlineCodeGenerator::ReadMask()
{
	if(m_cpu.has(util::Cpu::tAVX))
	{
		if(m_sel.fwrite)
		{
			vmovdqa(xmm3, ptr[&m_local.gd->fm]);
		}

		if(m_sel.zwrite)
		{
			vmovdqa(xmm4, ptr[&m_local.gd->zm]);
		}
	}
	else
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

	if(m_cpu.has(util::Cpu::tAVX))
	{
		switch(m_sel.atst)
		{
		case ATST_NEVER:
			// t = GSVector4i::xffffffff();
			vpcmpeqd(xmm1, xmm1);
			break;

		case ATST_ALWAYS:
			return;

		case ATST_LESS:
		case ATST_LEQUAL:
			// t = (ga >> 16) > m_local.gd->aref;
			vpsrld(xmm1, xmm6, 16);
			vpcmpgtd(xmm1, ptr[&m_local.gd->aref]);
			break;

		case ATST_EQUAL:
			// t = (ga >> 16) != m_local.gd->aref;
			vpsrld(xmm1, xmm6, 16);
			vpcmpeqd(xmm1, ptr[&m_local.gd->aref]);
			vpcmpeqd(xmm0, xmm0);
			vpxor(xmm1, xmm0);
			break;

		case ATST_GEQUAL:
		case ATST_GREATER:
			// t = (ga >> 16) < m_local.gd->aref;
			vpsrld(xmm0, xmm6, 16);
			vmovdqa(xmm1, ptr[&m_local.gd->aref]);
			vpcmpgtd(xmm1, xmm0);
			break;

		case ATST_NOTEQUAL:
			// t = (ga >> 16) == m_local.gd->aref;
			vpsrld(xmm1, xmm6, 16);
			vpcmpeqd(xmm1, ptr[&m_local.gd->aref]);
			break;
		}

		switch(m_sel.afail)
		{
		case AFAIL_KEEP:
			// test |= t;
			vpor(xmm7, xmm1);
			alltrue();
			break;

		case AFAIL_FB_ONLY:
			// zm |= t;
			vpor(xmm4, xmm1);
			break;

		case AFAIL_ZB_ONLY:
			// fm |= t;
			vpor(xmm3, xmm1);
			break;

		case AFAIL_RGB_ONLY:
			// zm |= t;
			vpor(xmm4, xmm1);
			// fm |= t & GSVector4i::xff000000();
			vpsrld(xmm1, 24);
			vpslld(xmm1, 24);
			vpor(xmm3, xmm1);
			break;
		}
	}
	else
	{
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
}

void GSDrawScanlineCodeGenerator::ColorTFX()
{
	if(!m_sel.fwrite)
	{
		return;
	}

	if(m_cpu.has(util::Cpu::tAVX))
	{
		switch(m_sel.tfx)
		{
		case TFX_MODULATE:

			// GSVector4i rb = iip ? rbf : m_local.c.rb;

			// rbt = rbt.modulate16<1>(rb).clamp8();

			modulate16<1>(xmm5, ptr[m_sel.iip ? &m_local.temp.rb : &m_local.c.rb]);

			clamp16(xmm5, xmm1);

			break;

		case TFX_DECAL:

			break;

		case TFX_HIGHLIGHT:
		case TFX_HIGHLIGHT2:

			if(m_sel.tfx == TFX_HIGHLIGHT2 && m_sel.tcc)
			{
				// GSVector4i ga = iip ? gaf : m_local.c.ga;

				vmovdqa(xmm2, ptr[m_sel.iip ? &m_local.temp.ga : &m_local.c.ga]);
			}

			// gat = gat.modulate16<1>(ga).add16(af).clamp8().mix16(gat);

			vmovdqa(xmm1, xmm6);

			modulate16<1>(xmm6, xmm2);

			vpshuflw(xmm2, xmm2, _MM_SHUFFLE(3, 3, 1, 1));
			vpshufhw(xmm2, xmm2, _MM_SHUFFLE(3, 3, 1, 1));
			vpsrlw(xmm2, 7);

			vpaddw(xmm6, xmm2);

			clamp16(xmm6, xmm0);

			mix16(xmm6, xmm1, xmm0);

			// GSVector4i rb = iip ? rbf : m_local.c.rb;

			// rbt = rbt.modulate16<1>(rb).add16(af).clamp8();

			modulate16<1>(xmm5, ptr[m_sel.iip ? &m_local.temp.rb : &m_local.c.rb]);

			vpaddw(xmm5, xmm2);

			clamp16(xmm5, xmm0);

			break;

		case TFX_NONE:

			// rbt = iip ? rb.srl16(7) : rb;

			if(m_sel.iip)
			{
				vpsrlw(xmm5, 7);
			}

			break;
		}
	}
	else
	{
		switch(m_sel.tfx)
		{
		case TFX_MODULATE:

			// GSVector4i rb = iip ? rbf : m_local.c.rb;

			// rbt = rbt.modulate16<1>(rb).clamp8();

			modulate16<1>(xmm5, ptr[m_sel.iip ? &m_local.temp.rb : &m_local.c.rb]);

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

			modulate16<1>(xmm6, xmm2);

			pshuflw(xmm2, xmm2, _MM_SHUFFLE(3, 3, 1, 1));
			pshufhw(xmm2, xmm2, _MM_SHUFFLE(3, 3, 1, 1));
			psrlw(xmm2, 7);

			paddw(xmm6, xmm2);

			clamp16(xmm6, xmm0);

			mix16(xmm6, xmm1, xmm0);

			// GSVector4i rb = iip ? rbf : m_local.c.rb;

			// rbt = rbt.modulate16<1>(rb).add16(af).clamp8();

			modulate16<1>(xmm5, ptr[m_sel.iip ? &m_local.temp.rb : &m_local.c.rb]);

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
}

void GSDrawScanlineCodeGenerator::Fog()
{
	if(!m_sel.fwrite || !m_sel.fge)
	{
		return;
	}

	if(m_cpu.has(util::Cpu::tAVX))
	{
		// rb = m_local.gd->frb.lerp16<0>(rb, f);
		// ga = m_local.gd->fga.lerp16<0>(ga, f).mix16(ga);

		vmovdqa(xmm0, ptr[!m_sel.sprite ? &m_local.temp.f : &m_local.p.f]);
		vmovdqa(xmm1, xmm6);

		vmovdqa(xmm2, ptr[&m_local.gd->frb]);
		lerp16<0>(xmm5, xmm2, xmm0);

		vmovdqa(xmm2, ptr[&m_local.gd->fga]);
		lerp16<0>(xmm6, xmm2, xmm0);
		mix16(xmm6, xmm1, xmm0);
	}
	else
	{
		// rb = m_local.gd->frb.lerp16<0>(rb, f);
		// ga = m_local.gd->fga.lerp16<0>(ga, f).mix16(ga);

		movdqa(xmm0, ptr[!m_sel.sprite ? &m_local.temp.f : &m_local.p.f]);
		movdqa(xmm1, xmm6);

		movdqa(xmm2, ptr[&m_local.gd->frb]);
		lerp16<0>(xmm5, xmm2, xmm0);

		movdqa(xmm2, ptr[&m_local.gd->fga]);
		lerp16<0>(xmm6, xmm2, xmm0);
		mix16(xmm6, xmm1, xmm0);
	}
}

void GSDrawScanlineCodeGenerator::ReadFrame()
{
	if(!m_sel.fb)
	{
		return;
	}

	// int fa = fza_base.x + fza_offset->x;

	mov(ebx, dword[esi]);
	add(ebx, dword[edi]);

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

	if(m_cpu.has(util::Cpu::tAVX))
	{
		// test |= ((fd [<< 16]) ^ m_local.gd->datm).sra32(31);

		if(m_sel.datm)
		{
			if(m_sel.fpsm == 2)
			{
				vpxor(xmm0, xmm0);
				vpsrld(xmm1, xmm2, 15);
				vpcmpeqd(xmm1, xmm0);
			}
			else
			{
				vpcmpeqd(xmm0, xmm0);
				vpxor(xmm1, xmm2, xmm0);
				vpsrad(xmm1, 31);
			}
		}
		else
		{
			if(m_sel.fpsm == 2)
			{
				vpslld(xmm1, xmm2, 16);
				vpsrad(xmm1, 31);
			}
			else
			{
				vpsrad(xmm1, xmm2, 31);
			}
		}

		vpor(xmm7, xmm1);
	}
	else
	{
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
	}

	alltrue();
}

void GSDrawScanlineCodeGenerator::WriteMask()
{
	if(m_cpu.has(util::Cpu::tAVX))
	{
		// fm |= test;
		// zm |= test;

		if(m_sel.fwrite)
		{
			vpor(xmm3, xmm7);
		}

		if(m_sel.zwrite)
		{
			vpor(xmm4, xmm7);
		}

		// int fzm = ~(fm == GSVector4i::xffffffff()).ps32(zm == GSVector4i::xffffffff()).mask();

		vpcmpeqd(xmm1, xmm1);

		if(m_sel.fwrite && m_sel.zwrite)
		{
			vpcmpeqd(xmm0, xmm1, xmm4);
			vpcmpeqd(xmm1, xmm3);
			vpackssdw(xmm1, xmm0);
		}
		else if(m_sel.fwrite)
		{
			vpcmpeqd(xmm1, xmm3);
			vpackssdw(xmm1, xmm1);
		}
		else if(m_sel.zwrite)
		{
			vpcmpeqd(xmm1, xmm4);
			vpackssdw(xmm1, xmm1);
		}
		
		vpmovmskb(edx, xmm1);

		not(edx);
	}
	else
	{
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
}

void GSDrawScanlineCodeGenerator::WriteZBuf()
{
	if(!m_sel.zwrite)
	{
		return;
	}

	bool fast = m_sel.ztest && m_sel.zpsm < 2;

	if(m_cpu.has(util::Cpu::tAVX))
	{
		vmovdqa(xmm1, ptr[!m_sel.sprite ? &m_local.temp.zs : &m_local.p.z]);

		if(fast)
		{
			// zs = zs.blend8(zd, zm);

			vpblendvb(xmm1, ptr[&m_local.temp.zd], xmm4);
		}
	}
	else
	{
		movdqa(xmm1, ptr[!m_sel.sprite ? &m_local.temp.zs : &m_local.p.z]);

		if(fast)
		{
			// zs = zs.blend8(zd, zm);

			movdqa(xmm0, xmm4);
			movdqa(xmm7, ptr[&m_local.temp.zd]);
			blend8(xmm1, xmm7);
		}
	}

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

	if(m_cpu.has(util::Cpu::tAVX))
	{
		if((m_sel.aba != m_sel.abb) && (m_sel.aba == 1 || m_sel.abb == 1 || m_sel.abc == 1) || m_sel.abd == 1)
		{
			switch(m_sel.fpsm)
			{
			case 0:
			case 1:

				// c[2] = fd & mask;
				// c[3] = (fd >> 8) & mask;

				vpsllw(xmm0, xmm2, 8);
				vpsrlw(xmm0, 8);
				vpsrlw(xmm1, xmm2, 8);

				break;

			case 2:

				// c[2] = ((fd & 0x7c00) << 9) | ((fd & 0x001f) << 3);
				// c[3] = ((fd & 0x8000) << 8) | ((fd & 0x03e0) >> 2);

				vpcmpeqd(xmm7, xmm7);

				vpsrld(xmm7, 27); // 0x0000001f
				vpand(xmm0, xmm2, xmm7);
				vpslld(xmm0, 3);

				vpslld(xmm7, 10); // 0x00007c00
				vpand(xmm4, xmm2, xmm7);
				vpslld(xmm4, 9);

				vpor(xmm0, xmm4);

				vpsrld(xmm7, 5); // 0x000003e0
				vpand(xmm1, xmm2, xmm7);
				vpsrld(xmm1, 2);

				vpsllw(xmm7, 10); // 0x00008000
				vpand(xmm4, xmm2, xmm7);
				vpslld(xmm4, 8);

				vpor(xmm1, xmm4);

				break;
			}
		}

		// xmm5, xmm6 = src rb, ga
		// xmm0, xmm1 = dst rb, ga
		// xmm2, xmm3 = used
		// xmm4, xmm7 = free

		if(m_sel.pabe || (m_sel.aba != m_sel.abb) && (m_sel.abb == 0 || m_sel.abd == 0))
		{
			vmovdqa(xmm4, xmm5);
		}

		if(m_sel.aba != m_sel.abb)
		{
			// rb = c[aba * 2 + 0];

			switch(m_sel.aba)
			{
			case 0: break;
			case 1: vmovdqa(xmm5, xmm0); break;
			case 2: vpxor(xmm5, xmm5); break;
			}

			// rb = rb.sub16(c[abb * 2 + 0]);

			switch(m_sel.abb)
			{
			case 0: vpsubw(xmm5, xmm4); break;
			case 1: vpsubw(xmm5, xmm0); break;
			case 2: break;
			}

			if(!(m_sel.fpsm == 1 && m_sel.abc == 1))
			{
				// GSVector4i a = abc < 2 ? c[abc * 2 + 1].yywwlh().sll16(7) : m_local.gd->afix;

				switch(m_sel.abc)
				{
				case 0:
				case 1:
					vpshuflw(xmm7, m_sel.abc ? xmm1 : xmm6, _MM_SHUFFLE(3, 3, 1, 1));
					vpshufhw(xmm7, xmm7, _MM_SHUFFLE(3, 3, 1, 1));
					vpsllw(xmm7, 7);
					break;
				case 2:
					vmovdqa(xmm7, ptr[&m_local.gd->afix]);
					break;
				}

				// rb = rb.modulate16<1>(a);

				modulate16<1>(xmm5, xmm7);
			}

			// rb = rb.add16(c[abd * 2 + 0]);

			switch(m_sel.abd)
			{
			case 0: vpaddw(xmm5, xmm4); break;
			case 1: vpaddw(xmm5, xmm0); break;
			case 2: break;
			}
		}
		else
		{
			// rb = c[abd * 2 + 0];

			switch(m_sel.abd)
			{
			case 0: break;
			case 1: vmovdqa(xmm5, xmm0); break;
			case 2: vpxor(xmm5, xmm5); break;
			}
		}

		if(m_sel.pabe)
		{
			// mask = (c[1] << 8).sra32(31);

			vpslld(xmm0, xmm6, 8);
			vpsrad(xmm0, 31);

			// rb = c[0].blend8(rb, mask);

			vpblendvb(xmm5, xmm4, xmm5, xmm0);
		}

		// xmm6 = src ga
		// xmm1 = dst ga
		// xmm5 = rb
		// xmm7 = a
		// xmm2, xmm3 = used
		// xmm0, xmm4 = free

		vmovdqa(xmm4, xmm6);

		if(m_sel.aba != m_sel.abb)
		{
			// ga = c[aba * 2 + 1];

			switch(m_sel.aba)
			{
			case 0: break;
			case 1: vmovdqa(xmm6, xmm1); break;
			case 2: vpxor(xmm6, xmm6); break;
			}

			// ga = ga.sub16(c[abeb * 2 + 1]);

			switch(m_sel.abb)
			{
			case 0: vpsubw(xmm6, xmm4); break;
			case 1: vpsubw(xmm6, xmm1); break;
			case 2: break;
			}

			if(!(m_sel.fpsm == 1 && m_sel.abc == 1))
			{
				// ga = ga.modulate16<1>(a);

				modulate16<1>(xmm6, xmm7);
			}

			// ga = ga.add16(c[abd * 2 + 1]);

			switch(m_sel.abd)
			{
			case 0: vpaddw(xmm6, xmm4); break;
			case 1: vpaddw(xmm6, xmm1); break;
			case 2: break;
			}
		}
		else
		{
			// ga = c[abd * 2 + 1];

			switch(m_sel.abd)
			{
			case 0: break;
			case 1: vmovdqa(xmm6, xmm1); break;
			case 2: vpxor(xmm6, xmm6); break;
			}
		}

		// xmm4 = src ga
		// xmm5 = rb
		// xmm6 = ga
		// xmm2, xmm3 = used
		// xmm0, xmm1, xmm7 = free

		if(m_sel.pabe)
		{
			vpsrld(xmm0, 16); // zero out high words to select the source alpha in blend (so it also does mix16)

			// ga = c[1].blend8(ga, mask).mix16(c[1]);

			vpblendvb(xmm6, xmm4, xmm6, xmm0);
		}
		else
		{
			if(m_sel.fpsm != 1) // TODO: fm == 0xffxxxxxx
			{
				mix16(xmm6, xmm4, xmm7);
			}
		}
	}
	else
	{
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
					movdqa(xmm7, m_sel.abc ? xmm1 : xmm6);
					pshuflw(xmm7, xmm7, _MM_SHUFFLE(3, 3, 1, 1));
					pshufhw(xmm7, xmm7, _MM_SHUFFLE(3, 3, 1, 1));
					psllw(xmm7, 7);
					break;
				case 2:
					movdqa(xmm7, ptr[&m_local.gd->afix]);
					break;
				}

				// rb = rb.modulate16<1>(a);

				modulate16<1>(xmm5, xmm7);
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

				modulate16<1>(xmm6, xmm7);
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
			if(!m_cpu.has(util::Cpu::tSSE41))
			{
				// doh, previous blend8r overwrote xmm0 (sse41 uses pblendvb)

				movdqa(xmm0, xmm4);
				pslld(xmm0, 8);
				psrad(xmm0, 31);
			}

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
}

void GSDrawScanlineCodeGenerator::WriteFrame()
{
	if(!m_sel.fwrite)
	{
		return;
	}

	if(m_cpu.has(util::Cpu::tAVX))
	{
		if(m_sel.colclamp == 0)
		{
			// c[0] &= 0x000000ff;
			// c[1] &= 0x000000ff;

			vpcmpeqd(xmm7, xmm7);
			vpsrlw(xmm7, 8);
			vpand(xmm5, xmm7);
			vpand(xmm6, xmm7);
		}

		if(m_sel.fpsm == 2 && m_sel.dthe)
		{
			mov(eax, dword[esp + _top]);
			and(eax, 3);
			shl(eax, 5);
			vpaddw(xmm5, ptr[eax + (size_t)&m_local.gd->dimx[0]]);
			vpaddw(xmm6, ptr[eax + (size_t)&m_local.gd->dimx[1]]);
		}

		// GSVector4i fs = c[0].upl16(c[1]).pu16(c[0].uph16(c[1]));

		vpunpckhwd(xmm7, xmm5, xmm6);
		vpunpcklwd(xmm5, xmm6);
		vpackuswb(xmm5, xmm7);

		if(m_sel.fba && m_sel.fpsm != 1)
		{
			// fs |= 0x80000000;

			vpcmpeqd(xmm7, xmm7);
			vpslld(xmm7, 31);
			vpor(xmm5, xmm7);
		}

		if(m_sel.fpsm == 2)
		{
			// GSVector4i rb = fs & 0x00f800f8;
			// GSVector4i ga = fs & 0x8000f800;

			mov(eax, 0x00f800f8);
			vmovd(xmm6, eax);
			vpshufd(xmm6, xmm6, _MM_SHUFFLE(0, 0, 0, 0));

			mov(eax, 0x8000f800);
			vmovd(xmm7, eax);
			vpshufd(xmm7, xmm7, _MM_SHUFFLE(0, 0, 0, 0));

			vpand(xmm4, xmm5, xmm6);
			vpand(xmm5, xmm7);

			// fs = (ga >> 16) | (rb >> 9) | (ga >> 6) | (rb >> 3);

			vpsrld(xmm6, xmm4, 9);
			vpsrld(xmm4, 3);
			vpsrld(xmm7, xmm5, 16);
			vpsrld(xmm5, 6);

			vpor(xmm5, xmm4);
			vpor(xmm7, xmm6);
			vpor(xmm5, xmm7);
		}
	}
	else
	{
		if(m_sel.colclamp == 0)
		{
			// c[0] &= 0x000000ff;
			// c[1] &= 0x000000ff;

			pcmpeqd(xmm7, xmm7);
			psrlw(xmm7, 8);
			pand(xmm5, xmm7);
			pand(xmm6, xmm7);
		}

		if(m_sel.fpsm == 2 && m_sel.dthe)
		{
			mov(eax, dword[esp + _top]);
			and(eax, 3);
			shl(eax, 5);
			paddw(xmm5, ptr[eax + (size_t)&m_local.gd->dimx[0]]);
			paddw(xmm6, ptr[eax + (size_t)&m_local.gd->dimx[1]]);
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
	}

	if(m_sel.rfb)
	{
		// fs = fs.blend(fd, fm);

		blend(xmm5, xmm2, xmm3); // TODO: could be skipped in certain cases, depending on fpsm and fm
	}

	bool fast = m_sel.rfb && m_sel.fpsm < 2;

	WritePixel(xmm5, ebx, dl, fast, m_sel.fpsm, 0);
}

void GSDrawScanlineCodeGenerator::ReadPixel(const Xmm& dst, const Reg32& addr)
{
	if(m_cpu.has(util::Cpu::tAVX))
	{
		vmovq(dst, qword[addr * 2 + (size_t)m_local.gd->vm]);
		vmovhps(dst, qword[addr * 2 + (size_t)m_local.gd->vm + 8 * 2]);
	}
	else
	{
		movq(dst, qword[addr * 2 + (size_t)m_local.gd->vm]);
		movhps(dst, qword[addr * 2 + (size_t)m_local.gd->vm + 8 * 2]);
	}
}

void GSDrawScanlineCodeGenerator::WritePixel(const Xmm& src, const Reg32& addr, const Reg8& mask, bool fast, int psm, int fz)
{
	if(fast)
	{
		// if(fzm & 0x0f) GSVector4i::storel(&vm16[addr + 0], fs);
		// if(fzm & 0xf0) GSVector4i::storeh(&vm16[addr + 8], fs);

		if(m_cpu.has(util::Cpu::tAVX))
		{
			test(mask, 0x0f);
			je("@f");
			vmovq(qword[addr * 2 + (size_t)m_local.gd->vm], src);
			L("@@");

			test(mask, 0xf0);
			je("@f");
			vmovhps(qword[addr * 2 + (size_t)m_local.gd->vm + 8 * 2], src);
			L("@@");

			// vmaskmovps?
		}
		else
		{
			test(mask, 0x0f);
			je("@f");
			movq(qword[addr * 2 + (size_t)m_local.gd->vm], src);
			L("@@");

			test(mask, 0xf0);
			je("@f");
			movhps(qword[addr * 2 + (size_t)m_local.gd->vm + 8 * 2], src);
			L("@@");
		}
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

static const int s_offsets[4] = {0, 2, 8, 10};

void GSDrawScanlineCodeGenerator::WritePixel(const Xmm& src, const Reg32& addr, uint8 i, int psm)
{
	Address dst = ptr[addr * 2 + (size_t)m_local.gd->vm + s_offsets[i] * 2];

	if(m_cpu.has(util::Cpu::tAVX))
	{
		switch(psm)
		{
		case 0:
			if(i == 0) vmovd(dst, src);
			else vpextrd(dst, src, i);
			break;
		case 1:
			if(i == 0) vmovd(eax, src);
			else vpextrd(eax, src, i);
			xor(eax, dst);
			and(eax, 0xffffff);
			xor(dst, eax);
			break;
		case 2:
			vpextrw(eax, src, i * 2);
			mov(dst, ax);
			break;
		}
	}
	else if(m_cpu.has(util::Cpu::tSSE41))
	{
		switch(psm)
		{
		case 0:
			if(i == 0) movd(dst, src);
			else pextrd(dst, src, i);
			break;
		case 1:
			if(i == 0) movd(eax, src);
			else pextrd(eax, src, i);
			xor(eax, dst);
			and(eax, 0xffffff);
			xor(dst, eax);
			break;
		case 2:
			pextrw(eax, src, i * 2);
			mov(dst, ax);
			break;
		}
	}
	else
	{
		switch(psm)
		{
		case 0:
			if(i == 0) movd(dst, src);
			else {pshufd(xmm0, src, _MM_SHUFFLE(i, i, i, i)); movd(dst, xmm0);}
			break;
		case 1:
			if(i == 0) movd(eax, src);
			else {pshufd(xmm0, src, _MM_SHUFFLE(i, i, i, i)); movd(eax, xmm0);}
			xor(eax, dst);
			and(eax, 0xffffff);
			xor(dst, eax);
			break;
		case 2:
			pextrw(eax, src, i * 2);
			mov(dst, ax);
			break;
		}
	}
}

void GSDrawScanlineCodeGenerator::ReadTexel(const Xmm& dst, const Xmm& addr, const Xmm& temp1, const Xmm& temp2)
{
	if(m_cpu.has(util::Cpu::tSSE41))
	{
		ReadTexel(dst, addr, 0);
		ReadTexel(dst, addr, 1);
		ReadTexel(dst, addr, 2);
		ReadTexel(dst, addr, 3);
	}
	else
	{
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
}

void GSDrawScanlineCodeGenerator::ReadTexel(const Xmm& dst, const Xmm& addr, uint8 i)
{
	const Address& src = m_sel.tlu ? ptr[eax * 4 + (size_t)m_local.gd->clut] : ptr[ebx + eax * 4];

	if(m_cpu.has(util::Cpu::tAVX))
	{
		if(i == 0) vmovd(eax, addr);
		else vpextrd(eax, addr, i);

		if(m_sel.tlu) movzx(eax, byte[ebx + eax]);

		if(i == 0) vmovd(dst, src);
		else vpinsrd(dst, src, i);
	}
	else
	{
		if(!m_cpu.has(util::Cpu::tSSE41) && i > 0)
		{
			ASSERT(0);
		}

		if(i == 0) movd(eax, addr);
		else pextrd(eax, addr, i);

		if(m_sel.tlu) movzx(eax, byte[ebx + eax]);

		if(i == 0) movd(dst, src);
		else pinsrd(dst, src, i);
	}
}

template<int shift>
void GSDrawScanlineCodeGenerator::modulate16(const Xmm& a, const Operand& f)
{
	if(shift == 0 && m_cpu.has(util::Cpu::tSSSE3))
	{
		if(m_cpu.has(util::Cpu::tAVX))
		{
			vpmulhrsw(a, f);
		}
		else
		{
			pmulhrsw(a, f);
		}
	}
	else
	{
		if(m_cpu.has(util::Cpu::tAVX))
		{
			vpsllw(a, shift + 1);
			vpmulhw(a, f);
		}
		else
		{
			psllw(a, shift + 1);
			pmulhw(a, f);
		}
	}
}

template<int shift>
void GSDrawScanlineCodeGenerator::lerp16(const Xmm& a, const Xmm& b, const Xmm& f)
{
	if(m_cpu.has(util::Cpu::tAVX))
	{
		vpsubw(a, b);
		modulate16<shift>(a, f);
		vpaddw(a, b);
	}
	else
	{
		psubw(a, b);
		modulate16<shift>(a, f);
		paddw(a, b);
	}
}

void GSDrawScanlineCodeGenerator::mix16(const Xmm& a, const Xmm& b, const Xmm& temp)
{
	if(m_cpu.has(util::Cpu::tAVX))
	{
		vpblendw(a, b, 0xaa);
	}
	else if(m_cpu.has(util::Cpu::tSSE41))
	{
		pblendw(a, b, 0xaa);
	}
	else
	{
		pcmpeqd(temp, temp);
		psrld(temp, 16);
		pand(a, temp);
		pandn(temp, b);
		por(a, temp);
	}
}

void GSDrawScanlineCodeGenerator::clamp16(const Xmm& a, const Xmm& temp)
{
	if(m_cpu.has(util::Cpu::tAVX))
	{
		vpackuswb(a, a);
		vpmovzxbw(a, a);
	}
	else if(m_cpu.has(util::Cpu::tSSE41))
	{
		packuswb(a, a);
		pmovzxbw(a, a);
	}
	else
	{
		packuswb(a, a);
		pxor(temp, temp);
		punpcklbw(a, temp);
	}
}

void GSDrawScanlineCodeGenerator::alltrue()
{
	if(m_cpu.has(util::Cpu::tAVX))
	{
		vpmovmskb(eax, xmm7);
		cmp(eax, 0xffff);
		je("step", T_NEAR);
	}
	else
	{
		pmovmskb(eax, xmm7);
		cmp(eax, 0xffff);
		je("step", T_NEAR);
	}
}

void GSDrawScanlineCodeGenerator::blend(const Xmm& a, const Xmm& b, const Xmm& mask)
{
	if(m_cpu.has(util::Cpu::tAVX))
	{
		vpand(b, mask);
		vpandn(mask, a);
		vpor(a, b, mask);
	}
	else
	{
		pand(b, mask);
		pandn(mask, a);
		por(b, mask);
		movdqa(a, b);
	}
}

void GSDrawScanlineCodeGenerator::blendr(const Xmm& b, const Xmm& a, const Xmm& mask)
{
	if(m_cpu.has(util::Cpu::tAVX))
	{
		vpand(b, mask);
		vpandn(mask, a);
		vpor(b, mask);
	}
	else
	{
		pand(b, mask);
		pandn(mask, a);
		por(b, mask);
	}
}

void GSDrawScanlineCodeGenerator::blend8(const Xmm& a, const Xmm& b)
{
	if(m_cpu.has(util::Cpu::tSSE41))
	{
		pblendvb(a, b);
	}
	else
	{
		blend(a, b, xmm0);
	}
}

void GSDrawScanlineCodeGenerator::blend8r(const Xmm& b, const Xmm& a)
{
	if(m_cpu.has(util::Cpu::tSSE41))
	{
		pblendvb(a, b);
		movdqa(b, a);
	}
	else
	{
		blendr(b, a, xmm0);
	}
}

const GSVector4i GSDrawScanlineCodeGenerator::m_test[8] =
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
