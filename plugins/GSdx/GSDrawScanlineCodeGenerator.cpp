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

#include "StdAfx.h"
#include "GSDrawScanlineCodeGenerator.h"

GSDrawScanlineCodeGenerator::GSDrawScanlineCodeGenerator(GSScanlineEnvironment& env, void* ptr, size_t maxsize)
	: CodeGenerator(maxsize, ptr)
	, m_env(env)
{
	#if _M_AMD64
	#error TODO
	#endif

	Generate();
}

void GSDrawScanlineCodeGenerator::Generate()
{
	push(ebx);
	push(esi);
	push(edi);
	push(ebp);

	const int params = 16;

	Init(params);

	align(16);

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

	bool tme = m_env.sel.tfx != TFX_NONE;

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

	if(m_env.sel.fwrite)
	{
		movdqa(xmm3, xmmword[&m_env.fm]);
	}

	if(m_env.sel.zwrite)
	{
		movdqa(xmm4, xmmword[&m_env.zm]);
	}

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

	// fm |= test;
	// zm |= test;

	if(m_env.sel.fwrite)
	{
		por(xmm3, xmm7);
	}

	if(m_env.sel.zwrite)
	{
		por(xmm4, xmm7);
	}

	// int fzm = ~(fm == GSVector4i::xffffffff()).ps32(zm == GSVector4i::xffffffff()).mask();

	pcmpeqd(xmm1, xmm1);

	if(m_env.sel.fwrite && m_env.sel.zwrite)
	{
		movdqa(xmm0, xmm1);
		pcmpeqd(xmm1, xmm3);
		pcmpeqd(xmm0, xmm4);
		packssdw(xmm1, xmm0);
	}
	else if(m_env.sel.fwrite)
	{
		pcmpeqd(xmm1, xmm3);
		packssdw(xmm1, xmm1);
	}
	else if(m_env.sel.zwrite)
	{
		pcmpeqd(xmm1, xmm4);
		packssdw(xmm1, xmm1);
	}

	pmovmskb(edx, xmm1);
	not(edx);

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

	WriteFrame(params);

L("step");
	
	// if(steps <= 0) break;

	test(ecx, ecx);
	jle("exit", T_NEAR);

	Step();

	jmp("loop", T_NEAR);

L("exit");

	pop(ebp);
	pop(edi);
	pop(esi);
	pop(ebx);

	ret(8);
}

void GSDrawScanlineCodeGenerator::Init(int params)
{
	const int _top = params + 4;
	const int _v = params + 8;

	// int skip = left & 3;

	mov(ebx, edx);
	and(edx, 3);

	// left -= skip;

	sub(ebx, edx);

	// int steps = right - left - 4;

	sub(ecx, ebx);
	sub(ecx, 4);

	// GSVector4i test = m_test[skip] | m_test[7 + (steps & (steps >> 31))];

	shl(edx, 4);

	movdqa(xmm7, xmmword[edx + (size_t)&m_test[0]]);

	mov(eax, ecx);
	sar(eax, 31);
	and(eax, ecx);
	shl(eax, 4);

	por(xmm7, xmmword[eax + (size_t)&m_test[7]]);

	// GSVector2i* fza_base = &m_env.fzbr[top];

	mov(esi, dword[esp + _top]);
	lea(esi, ptr[esi * 8]);
	add(esi, dword[&m_env.fzbr]);

	// GSVector2i* fza_offset = &m_env.fzbc[left >> 2];

	lea(edi, ptr[ebx * 2]);
	add(edi, dword[&m_env.fzbc]);

	if(!m_env.sel.sprite && (m_env.sel.fwrite && m_env.sel.fge || m_env.sel.zb) || m_env.sel.fb && (m_env.sel.tfx != TFX_NONE || m_env.sel.iip))
	{
		// edx = &m_env.d[skip]

		shl(edx, 4);
		lea(edx, ptr[edx + (size_t)m_env.d]);

		// ebx = &v

		mov(ebx, dword[esp + _v]);
	}

	if(!m_env.sel.sprite)
	{
		if(m_env.sel.fwrite && m_env.sel.fge || m_env.sel.zb)
		{
			movaps(xmm0, xmmword[ebx + 16]); // v.p

			if(m_env.sel.fwrite && m_env.sel.fge)
			{
				// f = GSVector4i(vp).zzzzh().zzzz().add16(m_env.d[skip].f);

				cvttps2dq(xmm1, xmm0);
				pshufhw(xmm1, xmm1, _MM_SHUFFLE(2, 2, 2, 2));
				pshufd(xmm1, xmm1, _MM_SHUFFLE(2, 2, 2, 2));
				paddw(xmm1, xmmword[edx + 16 * 6]);

				movdqa(xmmword[&m_env.temp.f], xmm1);
			}

			if(m_env.sel.zb)
			{
				// z = vp.zzzz() + m_env.d[skip].z;

				shufps(xmm0, xmm0, _MM_SHUFFLE(2, 2, 2, 2));
				addps(xmm0, xmmword[edx]);

				movaps(xmmword[&m_env.temp.z], xmm0);
			}
		}
	}
	else
	{
		if(m_env.sel.ztest)
		{
			movdqa(xmm0, xmmword[&m_env.p.z]);
		}
	}

	if(m_env.sel.fb)
	{
		if(m_env.sel.tfx != TFX_NONE)
		{
			movaps(xmm4, xmmword[ebx + 32]); // v.t

			if(m_env.sel.fst)
			{
				// GSVector4i vti(vt);

				cvttps2dq(xmm4, xmm4);

				// si = vti.xxxx() + m_env.d[skip].si;
				// ti = vti.yyyy(); if(!sprite) ti += m_env.d[skip].ti;

				pshufd(xmm2, xmm4, _MM_SHUFFLE(0, 0, 0, 0));
				pshufd(xmm3, xmm4, _MM_SHUFFLE(1, 1, 1, 1));

				paddd(xmm2, xmmword[edx + 16 * 7]);
				
				if(!m_env.sel.sprite)
				{
					paddd(xmm3, xmmword[edx + 16 * 8]);
				}
				else
				{
					if(m_env.sel.ltf)
					{
						movdqa(xmm4, xmm3);
						pshuflw(xmm4, xmm4, _MM_SHUFFLE(2, 2, 0, 0));
						pshufhw(xmm4, xmm4, _MM_SHUFFLE(2, 2, 0, 0));
						psrlw(xmm4, 1);
						movdqa(xmmword[&m_env.temp.vf], xmm4);
					}
				}

				movdqa(xmmword[&m_env.temp.s], xmm2);
				movdqa(xmmword[&m_env.temp.t], xmm3);
			}
			else
			{
				// s = vt.xxxx() + m_env.d[skip].s; 
				// t = vt.yyyy() + m_env.d[skip].t;
				// q = vt.zzzz() + m_env.d[skip].q;

				movaps(xmm2, xmm4);
				movaps(xmm3, xmm4);

				shufps(xmm2, xmm2, _MM_SHUFFLE(0, 0, 0, 0));
				shufps(xmm3, xmm3, _MM_SHUFFLE(1, 1, 1, 1));
				shufps(xmm4, xmm4, _MM_SHUFFLE(2, 2, 2, 2));

				addps(xmm2, xmmword[edx + 16 * 1]);
				addps(xmm3, xmmword[edx + 16 * 2]);
				addps(xmm4, xmmword[edx + 16 * 3]);

				movaps(xmmword[&m_env.temp.s], xmm2);
				movaps(xmmword[&m_env.temp.t], xmm3);
				movaps(xmmword[&m_env.temp.q], xmm4);

				rcpps(xmm4, xmm4);
				mulps(xmm2, xmm4);
				mulps(xmm3, xmm4);
			}
		}

		if(m_env.sel.tfx != TFX_DECAL)
		{
			if(m_env.sel.iip)
			{
				// GSVector4i vc = GSVector4i(v.c);

				cvttps2dq(xmm6, xmmword[ebx]); // v.c

				// vc = vc.upl16(vc.zwxy());

				pshufd(xmm5, xmm6, _MM_SHUFFLE(1, 0, 3, 2));
				punpcklwd(xmm6, xmm5);

				// rb = vc.xxxx().add16(m_env.d[skip].rb);
				// ga = vc.zzzz().add16(m_env.d[skip].ga);

				pshufd(xmm5, xmm6, _MM_SHUFFLE(0, 0, 0, 0));
				pshufd(xmm6, xmm6, _MM_SHUFFLE(2, 2, 2, 2));

				paddw(xmm5, xmmword[edx + 16 * 4]);
				paddw(xmm6, xmmword[edx + 16 * 5]);

				movdqa(xmmword[&m_env.temp.rb], xmm5);
				movdqa(xmmword[&m_env.temp.ga], xmm6);
			}
			else
			{
				if(m_env.sel.tfx == TFX_NONE)
				{
					movdqa(xmm5, xmmword[&m_env.c.rb]);
					movdqa(xmm6, xmmword[&m_env.c.ga]);
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

	if(!m_env.sel.sprite)
	{
		// z += m_env.d4.z;

		if(m_env.sel.zb)
		{
			movaps(xmm0, xmmword[&m_env.temp.z]);
			addps(xmm0, xmmword[&m_env.d4.z]);
			movaps(xmmword[&m_env.temp.z], xmm0);
		}

		// f = f.add16(m_env.d4.f);

		if(m_env.sel.fwrite && m_env.sel.fge)
		{
			movdqa(xmm1, xmmword[&m_env.temp.f]);
			paddw(xmm1, xmmword[&m_env.d4.f]);
			movdqa(xmmword[&m_env.temp.f], xmm1);
		}
	}
	else
	{
		if(m_env.sel.ztest)
		{
			movdqa(xmm0, xmmword[&m_env.p.z]);
		}
	}

	if(m_env.sel.fb)
	{
		if(m_env.sel.tfx != TFX_NONE)
		{
			if(m_env.sel.fst)
			{
				// GSVector4i st = m_env.d4.st;

				// si += st.xxxx();
				// if(!sprite) ti += st.yyyy();

				movdqa(xmm4, xmmword[&m_env.d4.st]);

				pshufd(xmm2, xmm4, _MM_SHUFFLE(0, 0, 0, 0));
				paddd(xmm2, xmmword[&m_env.temp.s]);
				movdqa(xmmword[&m_env.temp.s], xmm2);

				if(!m_env.sel.sprite)
				{
					pshufd(xmm3, xmm4, _MM_SHUFFLE(1, 1, 1, 1));
					paddd(xmm3, xmmword[&m_env.temp.t]);
					movdqa(xmmword[&m_env.temp.t], xmm3);
				}
				else
				{
					movdqa(xmm3, xmmword[&m_env.temp.t]);
				}
			}
			else
			{
				// GSVector4 stq = m_env.d4.stq;

				// s += stq.xxxx();
				// t += stq.yyyy();
				// q += stq.zzzz();

				movaps(xmm2, xmmword[&m_env.d4.stq]);
				movaps(xmm3, xmm2);
				movaps(xmm4, xmm2);

				shufps(xmm2, xmm2, _MM_SHUFFLE(0, 0, 0, 0));
				shufps(xmm3, xmm3, _MM_SHUFFLE(1, 1, 1, 1));
				shufps(xmm4, xmm4, _MM_SHUFFLE(2, 2, 2, 2));

				addps(xmm2, xmmword[&m_env.temp.s]);
				addps(xmm3, xmmword[&m_env.temp.t]);
				addps(xmm4, xmmword[&m_env.temp.q]);

				movaps(xmmword[&m_env.temp.s], xmm2);
				movaps(xmmword[&m_env.temp.t], xmm3);
				movaps(xmmword[&m_env.temp.q], xmm4);

				rcpps(xmm4, xmm4);
				mulps(xmm2, xmm4);
				mulps(xmm3, xmm4);
			}
		}

		if(m_env.sel.tfx != TFX_DECAL)
		{
			if(m_env.sel.iip)
			{
				// GSVector4i c = m_env.d4.c;

				// rb = rb.add16(c.xxxx());
				// ga = ga.add16(c.yyyy());

				movdqa(xmm7, xmmword[&m_env.d4.c]);

				pshufd(xmm5, xmm7, _MM_SHUFFLE(0, 0, 0, 0));
				pshufd(xmm6, xmm7, _MM_SHUFFLE(1, 1, 1, 1));

				paddw(xmm5, xmmword[&m_env.temp.rb]);
				paddw(xmm6, xmmword[&m_env.temp.ga]);

				movdqa(xmmword[&m_env.temp.rb], xmm5);
				movdqa(xmmword[&m_env.temp.ga], xmm6);
			}
			else
			{
				if(m_env.sel.tfx == TFX_NONE)
				{
					movdqa(xmm5, xmmword[&m_env.c.rb]);
					movdqa(xmm6, xmmword[&m_env.c.ga]);
				}
			}
		}
	}

	// test = m_test[7 + (steps & (steps >> 31))];

	mov(edx, ecx);
	sar(edx, 31);
	and(edx, ecx);
	shl(edx, 4);

	movdqa(xmm7, xmmword[edx + (size_t)&m_test[7]]);
}

void GSDrawScanlineCodeGenerator::TestZ(const Xmm& temp1, const Xmm& temp2)
{
	if(!m_env.sel.zb) 
	{
		return;
	}

	// int za = fza_base.y + fza_offset->y;

	mov(ebp, dword[esi + 4]);
	add(ebp, dword[edi + 4]);

	// GSVector4i zs = zi;

	if(!m_env.sel.sprite)
	{
		if(m_env.sel.zoverflow)
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

		if(m_env.sel.zwrite)
		{
			movdqa(xmmword[&m_env.temp.zs], xmm0);
		}
	}

	if(m_env.sel.ztest)
	{
		ReadPixel(xmm1, ebp);

		if(m_env.sel.zwrite && m_env.sel.zpsm < 2)
		{
			movdqa(xmmword[&m_env.temp.zd], xmm1);
		}

		// zd &= 0xffffffff >> m_env.sel.zpsm * 8;

		if(m_env.sel.zpsm)
		{
			pslld(xmm1, m_env.sel.zpsm * 8);
			psrld(xmm1, m_env.sel.zpsm * 8);
		}

		if(m_env.sel.zoverflow || m_env.sel.zpsm == 0)
		{
			// GSVector4i o = GSVector4i::x80000000();

			pcmpeqd(xmm4, xmm4);
			pslld(xmm4, 31);

			// GSVector4i zso = zs - o;

			psubd(xmm0, xmm4);

			// GSVector4i zdo = zd - o;

			psubd(xmm1, xmm4);
		}

		switch(m_env.sel.ztst)
		{
		case ZTST_GEQUAL: 
			// test |= zso < zdo; 
			pcmpgtd(xmm1, xmm0);
			por(xmm7, xmm1);
			break;

		case ZTST_GREATER: 
			// test |= zso <= zdo; 
			movdqa(xmm4, xmm1);
			pcmpgtd(xmm1, xmm0);
			por(xmm7, xmm1);
			pcmpeqd(xmm4, xmm0);
			por(xmm7, xmm1);
			break;
		}

		alltrue();
	}
}

void GSDrawScanlineCodeGenerator::SampleTexture()
{
	if(!m_env.sel.fb || m_env.sel.tfx == TFX_NONE)
	{
		return;
	}

	mov(ebx, dword[&m_env.tex]);

	// ebx = tex

	if(!m_env.sel.fst)
	{
		// TODO: move these into Init/Step too?

		cvttps2dq(xmm2, xmm2);
		cvttps2dq(xmm3, xmm3);

		if(m_env.sel.ltf)
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

	if(m_env.sel.ltf)
	{
		// GSVector4i uf = u.xxzzlh().srl16(1);

		movdqa(xmm0, xmm2);
		pshuflw(xmm0, xmm0, _MM_SHUFFLE(2, 2, 0, 0));
		pshufhw(xmm0, xmm0, _MM_SHUFFLE(2, 2, 0, 0));
		psrlw(xmm0, 1);
		movdqa(xmmword[&m_env.temp.uf], xmm0);

		if(!m_env.sel.sprite)
		{
			// GSVector4i vf = v.xxzzlh().srl16(1);

			movdqa(xmm1, xmm3);
			pshuflw(xmm1, xmm1, _MM_SHUFFLE(2, 2, 0, 0));
			pshufhw(xmm1, xmm1, _MM_SHUFFLE(2, 2, 0, 0));
			psrlw(xmm1, 1);
			movdqa(xmmword[&m_env.temp.vf], xmm1);
		}
	}

	// GSVector4i uv0 = u.sra32(16).ps32(v.sra32(16));

	psrad(xmm2, 16);
	psrad(xmm3, 16);
	packssdw(xmm2, xmm3);

	if(m_env.sel.ltf)
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
	movd(xmm1, ptr[&m_env.tw]);
	
	movdqa(xmm4, xmm2);
	punpckhwd(xmm2, xmm0);
	punpcklwd(xmm4, xmm0);
	pslld(xmm2, xmm1);

	// xmm0 = 0
	// xmm1 = tw
	// xmm2 = y0
	// xmm3 = uv1 (ltf)
	// xmm4 = x0
	// xmm5, xmm6 = free
	// xmm7 = used

	if(m_env.sel.ltf)
	{
		// GSVector4i y1 = uv1.uph16() << tw;
		// GSVector4i x1 = uv1.upl16();

		movdqa(xmm6, xmm3);
		punpckhwd(xmm3, xmm0);
		punpcklwd(xmm6, xmm0);
		pslld(xmm3, xmm1);

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

		// c00 = addr00.gather32_32((const DWORD/BYTE*)tex[, clut]);
		// c01 = addr01.gather32_32((const DWORD/BYTE*)tex[, clut]);
		// c10 = addr10.gather32_32((const DWORD/BYTE*)tex[, clut]);
		// c11 = addr11.gather32_32((const DWORD/BYTE*)tex[, clut]);

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

		movdqa(xmm0, xmmword[&m_env.temp.uf]);

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

		movdqa(xmm0, xmmword[&m_env.temp.vf]);

		lerp16<0>(xmm5, xmm3, xmm0);
		lerp16<0>(xmm6, xmm4, xmm0);
	}
	else
	{
		// GSVector4i addr00 = y0 + x0;

		paddd(xmm2, xmm4);

		// c00 = addr00.gather32_32((const DWORD/BYTE*)tex[, clut]);

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

void GSDrawScanlineCodeGenerator::Wrap(const Xmm& uv)
{
	// xmm0, xmm1, xmm4, xmm5, xmm6 = free

	int wms_clamp = ((m_env.sel.wms + 1) >> 1) & 1;
	int wmt_clamp = ((m_env.sel.wmt + 1) >> 1) & 1;

	int region = ((m_env.sel.wms | m_env.sel.wmt) >> 1) & 1;

	if(wms_clamp == wmt_clamp)
	{
		if(wms_clamp)
		{
			if(region)
			{
				pmaxsw(uv, xmmword[&m_env.t.min]);
			}
			else
			{
				pxor(xmm0, xmm0);
				pmaxsw(uv, xmm0);
			}

			pminsw(uv, xmmword[&m_env.t.max]);
		}
		else
		{
			pand(uv, xmmword[&m_env.t.min]);

			if(region)
			{
				por(uv, xmmword[&m_env.t.max]);
			}
		}
	}
	else
	{
		movdqa(xmm1, uv);

		movdqa(xmm4, xmmword[&m_env.t.min]);
		movdqa(xmm5, xmmword[&m_env.t.max]);

		// GSVector4i clamp = t.sat_i16(m_env.t.min, m_env.t.max);
		
		pmaxsw(uv, xmm4);
		pminsw(uv, xmm5);

		// GSVector4i repeat = (t & m_env.t.min) | m_env.t.max;

		pand(xmm1, xmm4);
		
		if(region)
		{
			por(xmm1, xmm5);
		}

		// clamp.blend8(repeat, m_env.t.mask);

		movdqa(xmm0, xmmword[&m_env.t.mask]);
		blend8(uv, xmm1);
	}
}

void GSDrawScanlineCodeGenerator::Wrap(const Xmm& uv0, const Xmm& uv1)
{
	// xmm0, xmm1, xmm4, xmm5, xmm6 = free

	int wms_clamp = ((m_env.sel.wms + 1) >> 1) & 1;
	int wmt_clamp = ((m_env.sel.wmt + 1) >> 1) & 1;

	int region = ((m_env.sel.wms | m_env.sel.wmt) >> 1) & 1;

	if(wms_clamp == wmt_clamp)
	{
		if(wms_clamp)
		{
			if(region)
			{
				movdqa(xmm4, xmmword[&m_env.t.min]);

				pmaxsw(uv0, xmm4);
				pmaxsw(uv1, xmm4);
			}
			else
			{
				pxor(xmm0, xmm0);
				pmaxsw(uv0, xmm0);
				pmaxsw(uv1, xmm0);
			}

			movdqa(xmm5, xmmword[&m_env.t.max]);

			pminsw(uv0, xmm5);
			pminsw(uv1, xmm5);
		}
		else
		{
			movdqa(xmm4, xmmword[&m_env.t.min]);

			pand(uv0, xmm4);
			pand(uv1, xmm4);

			if(region)
			{
				movdqa(xmm5, xmmword[&m_env.t.max]);

				por(uv0, xmm5);
				por(uv1, xmm5);
			}
		}
	}
	else
	{
		movdqa(xmm1, uv0);
		movdqa(xmm6, uv1);

		movdqa(xmm4, xmmword[&m_env.t.min]);
		movdqa(xmm5, xmmword[&m_env.t.max]);

		// GSVector4i clamp = t.sat_i16(m_env.t.min, m_env.t.max);
		
		pmaxsw(uv0, xmm4);
		pmaxsw(uv1, xmm4);
		pminsw(uv0, xmm5);
		pminsw(uv1, xmm5);

		// GSVector4i repeat = (t & m_env.t.min) | m_env.t.max;

		pand(xmm1, xmm4);
		pand(xmm6, xmm4);

		if(region)
		{
			por(xmm1, xmm5);
			por(xmm6, xmm5);
		}

		// clamp.blend8(repeat, m_env.t.mask);

		if(m_cpu.has(util::Cpu::tSSE41))
		{
			movdqa(xmm0, xmmword[&m_env.t.mask]);

			pblendvb(uv0, xmm1);
			pblendvb(uv1, xmm6);
		}
		else
		{
			movdqa(xmm0, xmmword[&m_env.t.invmask]);
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

void GSDrawScanlineCodeGenerator::AlphaTFX()
{
	if(!m_env.sel.fb)
	{
		return;
	}

	switch(m_env.sel.tfx)
	{
	case TFX_MODULATE:

		// GSVector4i ga = iip ? gaf : m_env.c.ga;

		movdqa(xmm4, xmmword[m_env.sel.iip ? &m_env.temp.ga : &m_env.c.ga]);

		// gat = gat.modulate16<1>(ga).clamp8();

		modulate16<1>(xmm6, xmm4);
		
		clamp16(xmm6, xmm3);

		// if(!tcc) gat = gat.mix16(ga.srl16(7));
		
		if(!m_env.sel.tcc)
		{
			psrlw(xmm4, 7);

			mix16(xmm6, xmm4, xmm3);
		}

		break;

	case TFX_DECAL:

		break;

	case TFX_HIGHLIGHT: 

		// GSVector4i ga = iip ? gaf : m_env.c.ga;

		movdqa(xmm4, xmmword[m_env.sel.iip ? &m_env.temp.ga : &m_env.c.ga]);
		movdqa(xmm2, xmm4);

		// gat = gat.mix16(!tcc ? ga.srl16(7) : gat.addus8(ga.srl16(7)));

		psrlw(xmm4, 7);

		if(m_env.sel.tcc)
		{
			paddusb(xmm4, xmm6);
		}

		mix16(xmm6, xmm4, xmm3);

		break;

	case TFX_HIGHLIGHT2:

		// if(!tcc) gat = gat.mix16(ga.srl16(7));

		if(!m_env.sel.tcc)
		{
			// GSVector4i ga = iip ? gaf : m_env.c.ga;

			movdqa(xmm4, xmmword[m_env.sel.iip ? &m_env.temp.ga : &m_env.c.ga]);
			movdqa(xmm2, xmm4);

			psrlw(xmm4, 7);

			mix16(xmm6, xmm4, xmm3);
		}

		break;

	case TFX_NONE: 

		// gat = iip ? ga.srl16(7) : ga;

		if(m_env.sel.iip)
		{
			psrlw(xmm6, 7);
		}

		break; 
	}
}

void GSDrawScanlineCodeGenerator::TestAlpha()
{
	switch(m_env.sel.afail)
	{
	case AFAIL_FB_ONLY:
		if(!m_env.sel.zwrite) return;
		break;

	case AFAIL_ZB_ONLY:
		if(!m_env.sel.fwrite) return;
		break;

	case AFAIL_RGB_ONLY:
		if(!m_env.sel.zwrite && m_env.sel.fpsm == 1) return;
		break;
	}

	switch(m_env.sel.atst)
	{
	case ATST_NEVER:
		// t = GSVector4i::xffffffff();
		pcmpeqd(xmm1, xmm1);
		break;

	case ATST_ALWAYS:
		return;

	case ATST_LESS:
	case ATST_LEQUAL:
		// t = (ga >> 16) > m_env.aref;
		movdqa(xmm1, xmm6);
		psrld(xmm1, 16);
		pcmpgtd(xmm1, xmmword[&m_env.aref]);
		break;

	case ATST_EQUAL:
		// t = (ga >> 16) != m_env.aref;
		movdqa(xmm1, xmm6);
		psrld(xmm1, 16);
		pcmpeqd(xmm1, xmmword[&m_env.aref]);
		pcmpeqd(xmm0, xmm0);
		pxor(xmm1, xmm0);
		break;

	case ATST_GEQUAL: 
	case ATST_GREATER:
		// t = (ga >> 16) < m_env.aref;
		movdqa(xmm0, xmm6);
		psrld(xmm0, 16);
		movdqa(xmm1, xmmword[&m_env.aref]);
		pcmpgtd(xmm1, xmm0);
		break;

	case ATST_NOTEQUAL:
		// t = (ga >> 16) == m_env.aref;
		movdqa(xmm1, xmm6);
		psrld(xmm1, 16);
		pcmpeqd(xmm1, xmmword[&m_env.aref]);
		break;
	}

	switch(m_env.sel.afail)
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
	if(!m_env.sel.fwrite)
	{
		return;
	}

	switch(m_env.sel.tfx)
	{
	case TFX_MODULATE:

		// GSVector4i rb = iip ? rbf : m_env.c.rb;

		// rbt = rbt.modulate16<1>(rb).clamp8();

		modulate16<1>(xmm5, xmmword[m_env.sel.iip ? &m_env.temp.rb : &m_env.c.rb]);

		clamp16(xmm5, xmm1);

		break;

	case TFX_DECAL:

		break;

	case TFX_HIGHLIGHT:
	case TFX_HIGHLIGHT2:

		if(m_env.sel.tfx == TFX_HIGHLIGHT2 && m_env.sel.tcc)
		{
			// GSVector4i ga = iip ? gaf : m_env.c.ga;

			movdqa(xmm2, xmmword[m_env.sel.iip ? &m_env.temp.ga : &m_env.c.ga]);
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

		// GSVector4i rb = iip ? rbf : m_env.c.rb;

		// rbt = rbt.modulate16<1>(rb).add16(af).clamp8();

		modulate16<1>(xmm5, xmmword[m_env.sel.iip ? &m_env.temp.rb : &m_env.c.rb]);

		paddw(xmm5, xmm2);

		clamp16(xmm5, xmm0);

		break;

	case TFX_NONE: 

		// rbt = iip ? rb.srl16(7) : rb;

		if(m_env.sel.iip)
		{
			psrlw(xmm5, 7);
		}

		break; 
	}
}

void GSDrawScanlineCodeGenerator::Fog()
{
	if(!m_env.sel.fwrite || !m_env.sel.fge)
	{
		return;
	}

	// rb = m_env.frb.lerp16<0>(rb, f);
	// ga = m_env.fga.lerp16<0>(ga, f).mix16(ga);

	movdqa(xmm0, xmmword[!m_env.sel.sprite ? &m_env.temp.f : &m_env.p.f]);
	movdqa(xmm1, xmm6);

	movdqa(xmm2, xmmword[&m_env.frb]);
	lerp16<0>(xmm5, xmm2, xmm0);

	movdqa(xmm2, xmmword[&m_env.fga]);
	lerp16<0>(xmm6, xmm2, xmm0);

	mix16(xmm6, xmm1, xmm0);
}

void GSDrawScanlineCodeGenerator::ReadFrame()
{
	if(!m_env.sel.fb)
	{
		return;
	}

	// int fa = fza_base.x + fza_offset->x;

	mov(ebx, dword[esi]);
	add(ebx, dword[edi]);

	if(!m_env.sel.rfb)
	{
		return;
	}

	ReadPixel(xmm2, ebx);
}

void GSDrawScanlineCodeGenerator::TestDestAlpha()
{
	if(!m_env.sel.date || m_env.sel.fpsm != 0 && m_env.sel.fpsm != 2)
	{
		return;
	}

	// test |= ((fd [<< 16]) ^ m_env.datm).sra32(31);

	movdqa(xmm1, xmm2);

	if(m_env.sel.datm)
	{
		if(m_env.sel.fpsm == 2)
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
		if(m_env.sel.fpsm == 2)
		{
			pslld(xmm1, 16);
		}

		psrad(xmm1, 31);
	}

	por(xmm7, xmm1);

	alltrue();
}

void GSDrawScanlineCodeGenerator::WriteZBuf()
{
	if(!m_env.sel.zwrite)
	{
		return;
	}

	movdqa(xmm1, xmmword[!m_env.sel.sprite ? &m_env.temp.zs : &m_env.p.z]);

	bool fast = false;

	if(m_env.sel.ztest && m_env.sel.zpsm < 2)
	{
		// zs = zs.blend8(zd, zm);

		movdqa(xmm0, xmm4);
		movdqa(xmm7, xmmword[&m_env.temp.zd]);
		blend8(xmm1, xmm7);

		fast = true;
	}

	WritePixel(xmm1, xmm0, ebp, dh, fast, m_env.sel.zpsm);
}

void GSDrawScanlineCodeGenerator::AlphaBlend()
{
	if(!m_env.sel.fwrite)
	{
		return;
	}
/*
	if(m_env.sel.aa1)
	{
		// hmm, the playstation logo does not look good...

		printf("aa1 %016I64x\n", m_env.sel.key);

		if(m_env.sel.fpsm != 1) // TODO: fm == 0xffxxxxxx
		{
			// a = 0x80

			pcmpeqd(xmm0, xmm0);
			psllw(xmm0, 15);
			psrlw(xmm0, 8);
			mix16(xmm6, xmm0, xmm1);
		}

		return;
	}
*/
	if(m_env.sel.abe == 255)
	{
		return;
	}

	if((m_env.sel.abea != m_env.sel.abeb) && (m_env.sel.abea == 1 || m_env.sel.abeb == 1 || m_env.sel.abec == 1) || m_env.sel.abed == 1)
	{
		switch(m_env.sel.fpsm)
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

	if(m_env.sel.pabe || (m_env.sel.abea != m_env.sel.abeb) && (m_env.sel.abeb == 0 || m_env.sel.abed == 0))
	{
		movdqa(xmm4, xmm5);
	}

	if(m_env.sel.abea != m_env.sel.abeb)
	{
		// rb = c[abea * 2 + 0];

		switch(m_env.sel.abea)
		{
		case 0: break;
		case 1: movdqa(xmm5, xmm0); break;
		case 2: pxor(xmm5, xmm5); break;
		}

		// rb = rb.sub16(c[abeb * 2 + 0]);

		switch(m_env.sel.abeb)
		{
		case 0: psubw(xmm5, xmm4); break;
		case 1: psubw(xmm5, xmm0); break;
		case 2: break;
		}

		if(!(m_env.sel.fpsm == 1 && m_env.sel.abec == 1))
		{
			// GSVector4i a = abec < 2 ? c[abec * 2 + 1].yywwlh().sll16(7) : m_env.afix;

			switch(m_env.sel.abec)
			{
			case 0: 
			case 1: 
				movdqa(xmm7, m_env.sel.abec ? xmm1 : xmm6);
				pshuflw(xmm7, xmm7, _MM_SHUFFLE(3, 3, 1, 1));
				pshufhw(xmm7, xmm7, _MM_SHUFFLE(3, 3, 1, 1));
				psllw(xmm7, 7);
				break;
			case 2: 
				movdqa(xmm7, xmmword[&m_env.afix]); 
				break;
			}

			// rb = rb.modulate16<1>(a);

			modulate16<1>(xmm5, xmm7);
		}

		// rb = rb.add16(c[abed * 2 + 0]);

		switch(m_env.sel.abed)
		{
		case 0: paddw(xmm5, xmm4); break;
		case 1: paddw(xmm5, xmm0); break;
		case 2: break;
		}
	}
	else
	{
		// rb = c[abed * 2 + 0];

		switch(m_env.sel.abed)
		{
		case 0: break;
		case 1: movdqa(xmm5, xmm0); break;
		case 2: pxor(xmm5, xmm5); break;
		}
	}

	if(m_env.sel.pabe)
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

	if(m_env.sel.abea != m_env.sel.abeb)
	{
		// ga = c[abea * 2 + 1];

		switch(m_env.sel.abea)
		{
		case 0: break;
		case 1: movdqa(xmm6, xmm1); break;
		case 2: pxor(xmm6, xmm6); break;
		}

		// ga = ga.sub16(c[abeb * 2 + 1]);

		switch(m_env.sel.abeb)
		{
		case 0: psubw(xmm6, xmm4); break;
		case 1: psubw(xmm6, xmm1); break;
		case 2: break;
		}

		if(!(m_env.sel.fpsm == 1 && m_env.sel.abec == 1))
		{
			// ga = ga.modulate16<1>(a);

			modulate16<1>(xmm6, xmm7);
		}

		// ga = ga.add16(c[abed * 2 + 1]);

		switch(m_env.sel.abed)
		{
		case 0: paddw(xmm6, xmm4); break;
		case 1: paddw(xmm6, xmm1); break;
		case 2: break;
		}
	}
	else
	{
		// ga = c[abed * 2 + 1];

		switch(m_env.sel.abed)
		{
		case 0: break;
		case 1: movdqa(xmm6, xmm1); break;
		case 2: pxor(xmm6, xmm6); break;
		}
	}

	if(m_env.sel.pabe)
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
		if(m_env.sel.fpsm != 1) // TODO: fm == 0xffxxxxxx
		{
			mix16(xmm6, xmm4, xmm7);
		}
	}
}

void GSDrawScanlineCodeGenerator::WriteFrame(int params)
{
	const int _top = params + 4;

	if(!m_env.sel.fwrite)
	{
		return;
	}

	if(m_env.sel.colclamp == 0)
	{
		// c[0] &= 0x000000ff;
		// c[1] &= 0x000000ff;

		pcmpeqd(xmm7, xmm7);
		psrlw(xmm7, 8);
		pand(xmm5, xmm7);
		pand(xmm6, xmm7);
	}

	if(m_env.sel.fpsm == 2 && m_env.sel.dthe)
	{
		mov(eax, dword[esp + _top]);
		and(eax, 3);
		shl(eax, 5);
		paddw(xmm5, xmmword[eax + (size_t)&m_env.dimx[0]]);
		paddw(xmm6, xmmword[eax + (size_t)&m_env.dimx[1]]);
	}

	// GSVector4i fs = c[0].upl16(c[1]).pu16(c[0].uph16(c[1]));

	movdqa(xmm7, xmm5);
	punpcklwd(xmm5, xmm6);
	punpckhwd(xmm7, xmm6);
	packuswb(xmm5, xmm7);

	if(m_env.sel.fba && m_env.sel.fpsm != 1)
	{
		// fs |= 0x80000000;

		pcmpeqd(xmm7, xmm7);
		pslld(xmm7, 31);
		por(xmm5, xmm7);
	}

	if(m_env.sel.fpsm == 2)
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

	if(m_env.sel.rfb)
	{
		// fs = fs.blend(fd, fm);

		blend(xmm5, xmm2, xmm3); // TODO: could be skipped in certain cases, depending on fpsm and fm
	}

	bool fast = m_env.sel.rfb && m_env.sel.fpsm < 2;

	WritePixel(xmm5, xmm0, ebx, dl, fast, m_env.sel.fpsm);
}

void GSDrawScanlineCodeGenerator::ReadPixel(const Xmm& dst, const Reg32& addr)
{
	movq(dst, qword[addr * 2 + (size_t)m_env.vm]);
	movhps(dst, qword[addr * 2 + (size_t)m_env.vm + 8 * 2]);
}

void GSDrawScanlineCodeGenerator::WritePixel(const Xmm& src, const Xmm& temp, const Reg32& addr, const Reg8& mask, bool fast, int psm)
{
	if(fast)
	{
		// if(fzm & 0x0f) GSVector4i::storel(&vm16[addr + 0], fs); 
		// if(fzm & 0xf0) GSVector4i::storeh(&vm16[addr + 8], fs);

		test(mask, 0x0f);
		je("@f");
		movq(qword[addr * 2 + (size_t)m_env.vm], src);
		L("@@");

		test(mask, 0xf0);
		je("@f");
		movhps(qword[addr * 2 + (size_t)m_env.vm + 8 * 2], src);
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
		WritePixel(src, temp, addr, 0, psm);
		L("@@");

		test(mask, 0x0c);
		je("@f");
		WritePixel(src, temp, addr, 1, psm);
		L("@@");

		test(mask, 0x30);
		je("@f");
		WritePixel(src, temp, addr, 2, psm);
		L("@@");

		test(mask, 0xc0);
		je("@f");
		WritePixel(src, temp, addr, 3, psm);
		L("@@");
	}
}

void GSDrawScanlineCodeGenerator::WritePixel(const Xmm& src, const Xmm& temp, const Reg32& addr, uint8 i, int psm)
{
	static const int offsets[4] = {0, 2, 8, 10};

	Address dst = ptr[addr * 2 + (size_t)m_env.vm + offsets[i] * 2];

	if(m_cpu.has(util::Cpu::tSSE41))
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
			else {pshufd(temp, src, _MM_SHUFFLE(i, i, i, i)); movd(dst, temp);}
			break;
		case 1:
			if(i == 0) movd(eax, src);
			else {pshufd(temp, src, _MM_SHUFFLE(i, i, i, i)); movd(eax, temp);}
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
	if(!m_cpu.has(util::Cpu::tSSE41) && i > 0)
	{
		ASSERT(0);
	}

	if(i == 0) movd(eax, addr);
	else pextrd(eax, addr, i);

	if(m_env.sel.tlu) movzx(eax, byte[ebx + eax]);

	const Address& src = m_env.sel.tlu ? ptr[eax * 4 + (size_t)m_env.clut] : ptr[ebx + eax * 4];

	if(i == 0) movd(dst, src);
	else pinsrd(dst, src, i);
}

template<int shift> 
void GSDrawScanlineCodeGenerator::modulate16(const Xmm& a, const Operand& f)
{
	if(shift == 0 && m_cpu.has(util::Cpu::tSSSE3))
	{
		pmulhrsw(a, f);
	}
	else
	{
		psllw(a, shift + 1);
		pmulhw(a, f);
	}
}

template<int shift> 
void GSDrawScanlineCodeGenerator::lerp16(const Xmm& a, const Xmm& b, const Xmm& f)
{
	psubw(a, b);
	modulate16<shift>(a, f);
	paddw(a, b);
}

void GSDrawScanlineCodeGenerator::mix16(const Xmm& a, const Xmm& b, const Xmm& temp)
{
	if(m_cpu.has(util::Cpu::tSSE41))
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
	packuswb(a, a);

	if(m_cpu.has(util::Cpu::tSSE41))
	{
		pmovzxbw(a, a);
	}
	else
	{
		pxor(temp, temp);
		punpcklbw(a, temp);
	}
}

void GSDrawScanlineCodeGenerator::alltrue()
{
	pmovmskb(eax, xmm7);
	cmp(eax, 0xffff);
	je("step", T_NEAR);
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

void GSDrawScanlineCodeGenerator::blend(const Xmm& a, const Xmm& b, const Xmm& mask)
{
	pand(b, mask);
	pandn(mask, a);
	por(b, mask);
	movdqa(a, b);
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

void GSDrawScanlineCodeGenerator::blendr(const Xmm& b, const Xmm& a, const Xmm& mask)
{
	pand(b, mask);
	pandn(mask, a);
	por(b, mask);
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
