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
#include "GSDrawScanlineCodeGenerator.h"

#if _M_SSE >= 0x500 && (defined(_M_AMD64) || defined(_WIN64))

#error TODO: this is still bogus somewhere

void GSDrawScanlineCodeGenerator::Generate()
{
	// TODO: on linux/mac rsi, rdi, xmm6-xmm15 are all caller saved

	push(rbx);
	push(rsi);
	push(rdi);
	push(rbp);
	push(r12);
	push(r13);

	enter(10 * 16, true);
	
	for(int i = 6; i < 16; i++)
	{
		vmovdqa(ptr[rsp + (i - 6) * 16], Xmm(i));
	}

	movsxd(rcx, ecx); // right
	movsxd(rdx, edx); // left
	movsxd(r8, r8d); // top

	mov(r10, (size_t)&m_test[0]);
	mov(r11, (size_t)&m_local);
	mov(r12, (size_t)m_local.gd);
	mov(r13, (size_t)m_local.gd->vm);

	Init();

	// rcx = steps
	// rsi = fza_base
	// rdi = fza_offset
	// r10 = &m_test[0]
	// r11 = &m_local
	// r12 = m_local->gd
	// r13 = m_local->gd.vm
	// xmm7 = vf (sprite && ltf)
	// xmm8 = z
	// xmm9 = f
	// xmm10 = s
	// xmm11 = t
	// xmm12 = q
	// xmm13 = rb
	// xmm14 = ga 
	// xmm15 = test

	if(!m_sel.edge)
	{
		align(16);
	}

L("loop");

	TestZ(xmm5, xmm6);

	// ebp = za

	SampleTexture();

	// ebp = za
	// xmm2 = rb
	// xmm3 = ga

	AlphaTFX();

	// ebp = za
	// xmm2 = rb
	// xmm3 = ga

	ReadMask();

	// ebp = za
	// xmm2 = rb
	// xmm3 = ga
	// xmm4 = fm
	// xmm5 = zm

	TestAlpha();

	// ebp = za
	// xmm2 = rb
	// xmm3 = ga
	// xmm4 = fm
	// xmm5 = zm

	ColorTFX();

	// ebp = za
	// xmm2 = rb
	// xmm3 = ga
	// xmm4 = fm
	// xmm5 = zm

	Fog();

	// ebp = za
	// xmm2 = rb
	// xmm3 = ga
	// xmm4 = fm
	// xmm5 = zm

	ReadFrame();

	// ebx = fa
	// ebp = za
	// xmm2 = rb
	// xmm3 = ga
	// xmm4 = fm
	// xmm5 = zm
	// xmm6 = fd

	TestDestAlpha();

	// ebx = fa
	// ebp = za
	// xmm2 = rb
	// xmm3 = ga
	// xmm4 = fm
	// xmm5 = zm
	// xmm6 = fd

	WriteMask();

	// ebx = fa
	// edx = fzm
	// ebp = za
	// xmm2 = rb
	// xmm3 = ga
	// xmm4 = fm
	// xmm5 = zm
	// xmm6 = fd

	WriteZBuf();

	// ebx = fa
	// edx = fzm
	// xmm2 = rb
	// xmm3 = ga
	// xmm4 = fm
	// xmm6 = fd

	AlphaBlend();

	// ebx = fa
	// edx = fzm
	// xmm2 = rb
	// xmm3 = ga
	// xmm4 = fm
	// xmm6 = fd

	WriteFrame();

L("step");

	// if(steps <= 0) break;

	if(!m_sel.edge)
	{
		test(rcx, rcx);

		jle("exit", T_NEAR);

		Step();

		jmp("loop", T_NEAR);
	}

L("exit");

	for(int i = 6; i < 16; i++)
	{
		vmovdqa(Xmm(i), ptr[rsp + (i - 6) * 16]);
	}

	leave();

	pop(r13);
	pop(r12);
	pop(rbp);
	pop(rdi);
	pop(rsi);
	pop(rbx);

	ret();
}

void GSDrawScanlineCodeGenerator::Init()
{
	// int skip = left & 3;

	mov(rbx, rdx);
	and(rdx, 3);

	// left -= skip;

	sub(rbx, rdx);

	// int steps = right - left - 4;

	sub(rcx, rbx);
	sub(rcx, 4);

	// GSVector4i test = m_test[skip] | m_test[7 + (steps & (steps >> 31))];

	shl(rdx, 4);

	vmovdqa(xmm15, ptr[rdx + r10]);

	mov(rax, rcx);
	sar(rax, 63);
	and(rax, rcx);
	add(rax, 7);
	shl(rax, 4);

	vpor(xmm15, ptr[rax + r10]);

	// GSVector2i* fza_base = &m_local.gd->fzbr[top];

	mov(rax, (size_t)m_local.gd->fzbr);
	lea(rsi, ptr[rax + r8 * 8]);

	// GSVector2i* fza_offset = &m_local.gd->fzbc[left >> 2];

	mov(rax, (size_t)m_local.gd->fzbc);
	lea(rdi, ptr[rax + rbx * 2]);

	if(!m_sel.sprite && (m_sel.fwrite && m_sel.fge || m_sel.zb) || m_sel.fb && (m_sel.edge || m_sel.tfx != TFX_NONE || m_sel.iip))
	{
		// edx = &m_local.d[skip]

		shl(rdx, 4);
		lea(rdx, ptr[rdx + r11 + offsetof(GSScanlineLocalData, d)]);
	}

	if(!m_sel.sprite)
	{
		if(m_sel.fwrite && m_sel.fge || m_sel.zb)
		{
			vmovaps(xmm0, ptr[r9 + 16]); // v.p

			if(m_sel.fwrite && m_sel.fge)
			{
				// f = GSVector4i(vp).zzzzh().zzzz().add16(m_local.d[skip].f);

				vcvttps2dq(xmm9, xmm0);
				vpshufhw(xmm9, xmm9, _MM_SHUFFLE(2, 2, 2, 2));
				vpshufd(xmm9, xmm9, _MM_SHUFFLE(2, 2, 2, 2));
				vpaddw(xmm9, ptr[rdx + 16 * 6]);
			}

			if(m_sel.zb)
			{
				// z = vp.zzzz() + m_local.d[skip].z;

				vshufps(xmm8, xmm0, xmm0, _MM_SHUFFLE(2, 2, 2, 2));
				vaddps(xmm8, ptr[rdx]);
			}
		}
	}
	else
	{
		if(m_sel.ztest)
		{
			vmovdqa(xmm8, ptr[r11 + offsetof(GSScanlineLocalData, p.z)]);
		}
	}

	if(m_sel.fb)
	{
		if(m_sel.edge || m_sel.tfx != TFX_NONE)
		{
			vmovaps(xmm0, ptr[r9 + 32]); // v.t
		}

		if(m_sel.edge)
		{
			vpshufhw(xmm1, xmm0, _MM_SHUFFLE(2, 2, 2, 2));
			vpshufd(xmm1, xmm1, _MM_SHUFFLE(3, 3, 3, 3));
			vpsrlw(xmm1, 9);

			vmovdqa(ptr[r11 + offsetof(GSScanlineLocalData, temp.cov)], xmm1);
		}

		if(m_sel.tfx != TFX_NONE)
		{
			if(m_sel.fst)
			{
				// GSVector4i vti(vt);

				vcvttps2dq(xmm0, xmm0);

				// si = vti.xxxx() + m_local.d[skip].si;
				// ti = vti.yyyy(); if(!sprite) ti += m_local.d[skip].ti;

				vpshufd(xmm10, xmm0, _MM_SHUFFLE(0, 0, 0, 0));
				vpshufd(xmm11, xmm0, _MM_SHUFFLE(1, 1, 1, 1));

				vpaddd(xmm10, ptr[rdx + 16 * 7]);

				if(!m_sel.sprite)
				{
					vpaddd(xmm11, ptr[rdx + 16 * 8]);
				}
				else
				{
					if(m_sel.ltf)
					{
						vpshuflw(xmm6, xmm11, _MM_SHUFFLE(2, 2, 0, 0));
						vpshufhw(xmm6, xmm6, _MM_SHUFFLE(2, 2, 0, 0));
						vpsrlw(xmm6, 1);
					}
				}
			}
			else
			{
				// s = vt.xxxx() + m_local.d[skip].s;
				// t = vt.yyyy() + m_local.d[skip].t;
				// q = vt.zzzz() + m_local.d[skip].q;

				vshufps(xmm10, xmm0, xmm0, _MM_SHUFFLE(0, 0, 0, 0));
				vshufps(xmm11, xmm0, xmm0, _MM_SHUFFLE(1, 1, 1, 1));
				vshufps(xmm12, xmm0, xmm0, _MM_SHUFFLE(2, 2, 2, 2));

				vaddps(xmm10, ptr[rdx + 16 * 1]);
				vaddps(xmm11, ptr[rdx + 16 * 2]);
				vaddps(xmm12, ptr[rdx + 16 * 3]);
			}
		}

		if(!(m_sel.tfx == TFX_DECAL && m_sel.tcc))
		{
			if(m_sel.iip)
			{
				// GSVector4i vc = GSVector4i(v.c);

				vcvttps2dq(xmm0, ptr[r9]); // v.c

				// vc = vc.upl16(vc.zwxy());

				vpshufd(xmm1, xmm0, _MM_SHUFFLE(1, 0, 3, 2));
				vpunpcklwd(xmm0, xmm1);

				// rb = vc.xxxx().add16(m_local.d[skip].rb);
				// ga = vc.zzzz().add16(m_local.d[skip].ga);

				vpshufd(xmm13, xmm0, _MM_SHUFFLE(0, 0, 0, 0));
				vpshufd(xmm14, xmm0, _MM_SHUFFLE(2, 2, 2, 2));

				vpaddw(xmm13, ptr[rdx + 16 * 4]);
				vpaddw(xmm14, ptr[rdx + 16 * 5]);
			}
			else
			{
				vmovdqa(xmm13, ptr[&m_local.c.rb]);
				vmovdqa(xmm14, ptr[&m_local.c.ga]);
			}
		}
	}
}

void GSDrawScanlineCodeGenerator::Step()
{
	// steps -= 4;

	sub(rcx, 4);

	// fza_offset++;

	add(rdi, 8);

	if(!m_sel.sprite)
	{
		// z += m_local.d4.z;

		if(m_sel.zb)
		{
			vaddps(xmm8, ptr[r11 + offsetof(GSScanlineLocalData, d4.z)]);
		}

		// f = f.add16(m_local.d4.f);

		if(m_sel.fwrite && m_sel.fge)
		{
			vpaddw(xmm9, ptr[r11 + offsetof(GSScanlineLocalData, d4.f)]);
		}
	}
	else
	{
		if(m_sel.ztest)
		{
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

				vmovdqa(xmm0, ptr[r11 + offsetof(GSScanlineLocalData, d4.st)]);

				vpshufd(xmm1, xmm0, _MM_SHUFFLE(0, 0, 0, 0));
				vpaddd(xmm10, xmm1);

				if(!m_sel.sprite)
				{
					vpshufd(xmm1, xmm0, _MM_SHUFFLE(1, 1, 1, 1));
					vpaddd(xmm11, xmm1);
				}
			}
			else
			{
				// GSVector4 stq = m_local.d4.stq;

				// s += stq.xxxx();
				// t += stq.yyyy();
				// q += stq.zzzz();

				vmovaps(xmm0, ptr[r11 + offsetof(GSScanlineLocalData, d4.stq)]);

				vshufps(xmm1, xmm0, xmm0, _MM_SHUFFLE(0, 0, 0, 0));
				vshufps(xmm2, xmm0, xmm0, _MM_SHUFFLE(1, 1, 1, 1));
				vshufps(xmm3, xmm0, xmm0, _MM_SHUFFLE(2, 2, 2, 2));

				vaddps(xmm10, xmm1);
				vaddps(xmm11, xmm2);
				vaddps(xmm12, xmm3);
			}
		}

		if(!(m_sel.tfx == TFX_DECAL && m_sel.tcc))
		{
			if(m_sel.iip)
			{
				// GSVector4i c = m_local.d4.c;

				// rb = rb.add16(c.xxxx());
				// ga = ga.add16(c.yyyy());

				vmovdqa(xmm0, ptr[r11 + offsetof(GSScanlineLocalData, d4.c)]);

				vpshufd(xmm1, xmm0, _MM_SHUFFLE(0, 0, 0, 0));
				vpshufd(xmm2, xmm0, _MM_SHUFFLE(1, 1, 1, 1));

				vpaddw(xmm13, xmm1);
				vpaddw(xmm14, xmm2);
			}
			else
			{
				if(m_sel.tfx == TFX_NONE)
				{
				}
			}
		}
	}

	// test = m_test[7 + (steps & (steps >> 31))];

	mov(rdx, rcx);
	sar(rdx, 63);
	and(rdx, rcx);
	add(rdx, 7);
	shl(rdx, 4);

	vmovdqa(xmm15, ptr[rdx + r10]);
}

void GSDrawScanlineCodeGenerator::TestZ(const Xmm& temp1, const Xmm& temp2)
{
	if(!m_sel.zb)
	{
		return;
	}

	// int za = fza_base.y + fza_offset->y;

	movsxd(rbp, dword[rsi + 4]);
	movsxd(rax, dword[rdi + 4]);
	add(rbp, rax);

	// GSVector4i zs = zi;

	if(!m_sel.sprite)
	{
		if(m_sel.zoverflow)
		{
			// zs = (GSVector4i(z * 0.5f) << 1) | (GSVector4i(z) & GSVector4i::x00000001());
			
			mov(rax, (size_t)&GSVector4::m_half);

			vbroadcastss(xmm0, ptr[rax]);
			vmulps(xmm0, xmm8);
			vcvttps2dq(xmm0, xmm0);
			vpslld(xmm0, 1);

			vcvttps2dq(xmm1, xmm8);
			vpcmpeqd(xmm2, xmm2);
			vpsrld(xmm2, 31);
			vpand(xmm1, xmm2);

			vpor(xmm0, xmm1);
		}
		else
		{
			// zs = GSVector4i(z);

			vcvttps2dq(xmm0, xmm8);
		}

		if(m_sel.zwrite)
		{
			vmovdqa(ptr[r11 + offsetof(GSScanlineLocalData, temp.zs)], xmm0);
		}
	}

	if(m_sel.ztest)
	{
		ReadPixel(xmm1, rbp);

		if(m_sel.zwrite && m_sel.zpsm < 2)
		{
			vmovdqa(ptr[r11 + offsetof(GSScanlineLocalData, temp.zd)], xmm1);
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

			vpcmpeqd(xmm2, xmm2);
			vpslld(xmm2, 31);

			// GSVector4i zso = zs - o;

			vpsubd(xmm0, xmm2);

			// GSVector4i zdo = zd - o;

			vpsubd(xmm1, xmm2);
		}

		switch(m_sel.ztst)
		{
		case ZTST_GEQUAL:
			// test |= zso < zdo; // ~(zso >= zdo)
			vpcmpgtd(xmm1, xmm0);
			vpor(xmm15, xmm1);
			break;

		case ZTST_GREATER: // TODO: tidus hair and chocobo wings only appear fully when this is tested as ZTST_GEQUAL
			// test |= zso <= zdo; // ~(zso > zdo)
			vpcmpgtd(xmm0, xmm1);
			vpcmpeqd(xmm2, xmm2);
			vpxor(xmm0, xmm2);
			vpor(xmm15, xmm0);
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

	mov(rbx, ptr[r12 + offsetof(GSScanlineGlobalData, tex)]);

	// ebx = tex

	if(!m_sel.fst)
	{
		vrcpps(xmm0, xmm12);

		vmulps(xmm4, xmm10, xmm0);
		vmulps(xmm5, xmm11, xmm0);

		vcvttps2dq(xmm4, xmm4);
		vcvttps2dq(xmm5, xmm5);

		if(m_sel.ltf)
		{
			// u -= 0x8000;
			// v -= 0x8000;

			mov(eax, 0x8000);
			vmovd(xmm0, eax);
			vpshufd(xmm0, xmm0, _MM_SHUFFLE(0, 0, 0, 0));

			vpsubd(xmm4, xmm0);
			vpsubd(xmm5, xmm0);
		}
	}
	else
	{
		vmovdqa(xmm4, xmm10);
		vmovdqa(xmm5, xmm11);
	}

	if(m_sel.ltf)
	{
		// GSVector4i uf = u.xxzzlh().srl16(1);

		vpshuflw(xmm6, xmm4, _MM_SHUFFLE(2, 2, 0, 0));
		vpshufhw(xmm6, xmm6, _MM_SHUFFLE(2, 2, 0, 0));
		vpsrlw(xmm6, 1);

		if(!m_sel.sprite)
		{
			// GSVector4i vf = v.xxzzlh().srl16(1);

			vpshuflw(xmm7, xmm5, _MM_SHUFFLE(2, 2, 0, 0));
			vpshufhw(xmm7, xmm7, _MM_SHUFFLE(2, 2, 0, 0));
			vpsrlw(xmm7, 1);
		}
	}

	// GSVector4i uv0 = u.sra32(16).ps32(v.sra32(16));

	vpsrad(xmm4, 16);
	vpsrad(xmm5, 16);
	vpackssdw(xmm4, xmm5);

	if(m_sel.ltf)
	{
		// GSVector4i uv1 = uv0.add16(GSVector4i::x0001());

		vpcmpeqd(xmm0, xmm0);
		vpsrlw(xmm0, 15);
		vpaddw(xmm5, xmm4, xmm0);

		// uv0 = Wrap(uv0);
		// uv1 = Wrap(uv1);

		Wrap(xmm4, xmm5);
	}
	else
	{
		// uv0 = Wrap(uv0);

		Wrap(xmm4);
	}

	// xmm4 = uv0
	// xmm5 = uv1 (ltf)
	// xmm6 = uf
	// xmm7 = vf

	// GSVector4i x0 = uv0.upl16();
	// GSVector4i y0 = uv0.uph16() << tw;

	vpxor(xmm0, xmm0);

	vpunpcklwd(xmm2, xmm4, xmm0);
	vpunpckhwd(xmm3, xmm4, xmm0);
	vpslld(xmm3, m_sel.tw + 3);

	// xmm0 = 0
	// xmm2 = x0
	// xmm3 = y0
	// xmm5 = uv1 (ltf)
	// xmm6 = uf
	// xmm7 = vf

	if(m_sel.ltf)
	{
		// GSVector4i x1 = uv1.upl16();
		// GSVector4i y1 = uv1.uph16() << tw;

		vpunpcklwd(xmm4, xmm5, xmm0);
		vpunpckhwd(xmm5, xmm5, xmm0);
		vpslld(xmm5, m_sel.tw + 3);

		// xmm2 = x0
		// xmm3 = y0
		// xmm4 = x1
		// xmm5 = y1
		// xmm6 = uf
		// xmm7 = vf

		// GSVector4i addr00 = y0 + x0;
		// GSVector4i addr01 = y0 + x1;
		// GSVector4i addr10 = y1 + x0;
		// GSVector4i addr11 = y1 + x1;

		vpaddd(xmm0, xmm3, xmm2);
		vpaddd(xmm1, xmm3, xmm4);
		vpaddd(xmm2, xmm5, xmm2);
		vpaddd(xmm3, xmm5, xmm4);

		// xmm0 = addr00
		// xmm1 = addr01
		// xmm2 = addr10
		// xmm3 = addr11
		// xmm6 = uf
		// xmm7 = vf

		// c00 = addr00.gather32_32((const uint32/uint8*)tex[, clut]);
		// c01 = addr01.gather32_32((const uint32/uint8*)tex[, clut]);
		// c10 = addr10.gather32_32((const uint32/uint8*)tex[, clut]);
		// c11 = addr11.gather32_32((const uint32/uint8*)tex[, clut]);
		
		ReadTexel(xmm0, xmm0, xmm4, xmm5);
		ReadTexel(xmm1, xmm1, xmm4, xmm5);
		ReadTexel(xmm2, xmm2, xmm4, xmm5);
		ReadTexel(xmm3, xmm3, xmm4, xmm5);

		// xmm0 = c00
		// xmm1 = c01
		// xmm2 = c10
		// xmm3 = c11
		// xmm6 = uf
		// xmm7 = vf

		// GSVector4i rb00 = c00 & mask;
		// GSVector4i ga00 = (c00 >> 8) & mask;

		vpsllw(xmm4, xmm0, 8);
		vpsrlw(xmm4, 8);
		vpsrlw(xmm5, xmm0, 8);

		// GSVector4i rb01 = c01 & mask;
		// GSVector4i ga01 = (c01 >> 8) & mask;

		vpsllw(xmm0, xmm1, 8);
		vpsrlw(xmm0, 8);
		vpsrlw(xmm1, 8);

		// xmm0 = rb01
		// xmm1 = ga01
		// xmm2 = c10
		// xmm3 = c11
		// xmm4 = rb00
		// xmm5 = ga00
		// xmm6 = uf
		// xmm7 = vf

		// rb00 = rb00.lerp16<0>(rb01, uf);
		// ga00 = ga00.lerp16<0>(ga01, uf);

		lerp16(xmm0, xmm4, xmm6, 0);
		lerp16(xmm1, xmm5, xmm6, 0);

		// xmm0 = rb00
		// xmm1 = ga00
		// xmm2 = c10
		// xmm3 = c11
		// xmm6 = uf
		// xmm7 = vf

		// GSVector4i rb10 = c10 & mask;
		// GSVector4i ga10 = (c10 >> 8) & mask;

		vpsrlw(xmm5, xmm2, 8);
		vpsllw(xmm2, 8);
		vpsrlw(xmm4, xmm2, 8);

		// GSVector4i rb11 = c11 & mask;
		// GSVector4i ga11 = (c11 >> 8) & mask;

		vpsrlw(xmm2, xmm3, 8);
		vpsllw(xmm3, 8);
		vpsrlw(xmm3, 8);

		// xmm0 = rb00
		// xmm1 = ga00
		// xmm2 = rb11
		// xmm3 = ga11
		// xmm4 = rb10
		// xmm5 = ga10
		// xmm6 = uf
		// xmm7 = vf

		// rb10 = rb10.lerp16<0>(rb11, uf);
		// ga10 = ga10.lerp16<0>(ga11, uf);

		lerp16(xmm2, xmm4, xmm6, 0);
		lerp16(xmm3, xmm5, xmm6, 0);

		// xmm0 = rb00
		// xmm1 = ga00
		// xmm2 = rb10
		// xmm3 = ga10
		// xmm7 = vf

		// rb00 = rb00.lerp16<0>(rb10, vf);
		// ga00 = ga00.lerp16<0>(ga10, vf);

		lerp16(xmm2, xmm0, xmm7, 0);
		lerp16(xmm3, xmm1, xmm7, 0);
	}
	else
	{
		// GSVector4i addr00 = y0 + x0;

		vpaddd(xmm3, xmm2);

		// c00 = addr00.gather32_32((const uint32/uint8*)tex[, clut]);

		ReadTexel(xmm2, xmm3, xmm0, xmm1);

		// GSVector4i mask = GSVector4i::x00ff();

		// c[0] = c00 & mask;
		// c[1] = (c00 >> 8) & mask;

		vpsrlw(xmm3, xmm2, 8);
		vpsllw(xmm2, 8);
		vpsrlw(xmm2, 8);
	}

	// xmm2 = rb
	// xmm3 = ga
}

void GSDrawScanlineCodeGenerator::Wrap(const Xmm& uv)
{
	// xmm0, xmm1, xmm2, xmm3 = free

	int wms_clamp = ((m_sel.wms + 1) >> 1) & 1;
	int wmt_clamp = ((m_sel.wmt + 1) >> 1) & 1;

	int region = ((m_sel.wms | m_sel.wmt) >> 1) & 1;

	if(wms_clamp == wmt_clamp)
	{
		if(wms_clamp)
		{
			if(region)
			{
				vpmaxsw(uv, ptr[r12 + offsetof(GSScanlineGlobalData, t.min)]);
			}
			else
			{
				vpxor(xmm0, xmm0);
				vpmaxsw(uv, xmm0);
			}

			vpminsw(uv, ptr[r12 + offsetof(GSScanlineGlobalData, t.max)]);
		}
		else
		{
			vpand(uv, ptr[r12 + offsetof(GSScanlineGlobalData, t.min)]);

			if(region)
			{
				vpor(uv, ptr[r12 + offsetof(GSScanlineGlobalData, t.max)]);
			}
		}
	}
	else
	{
		vmovdqa(xmm2, ptr[r12 + offsetof(GSScanlineGlobalData, t.min)]);
		vmovdqa(xmm3, ptr[r12 + offsetof(GSScanlineGlobalData, t.max)]);
		vmovdqa(xmm0, ptr[r12 + offsetof(GSScanlineGlobalData, t.mask)]);

		// GSVector4i repeat = (t & m_local.gd->t.min) | m_local.gd->t.max;

		vpand(xmm1, uv, xmm2);

		if(region)
		{
			vpor(xmm1, xmm3);
		}

		// GSVector4i clamp = t.sat_i16(m_local.gd->t.min, m_local.gd->t.max);

		vpmaxsw(uv, xmm2);
		vpminsw(uv, xmm3);

		// clamp.blend8(repeat, m_local.gd->t.mask);

		vpblendvb(uv, xmm1, xmm0);
	}
}

void GSDrawScanlineCodeGenerator::Wrap(const Xmm& uv0, const Xmm& uv1)
{
	// xmm0, xmm1, xmm2, xmm3 = free

	int wms_clamp = ((m_sel.wms + 1) >> 1) & 1;
	int wmt_clamp = ((m_sel.wmt + 1) >> 1) & 1;

	int region = ((m_sel.wms | m_sel.wmt) >> 1) & 1;

	if(wms_clamp == wmt_clamp)
	{
		if(wms_clamp)
		{
			if(region)
			{
				vmovdqa(xmm0, ptr[r12 + offsetof(GSScanlineGlobalData, t.min)]);
				vpmaxsw(uv0, xmm0);
				vpmaxsw(uv1, xmm0);
			}
			else
			{
				vpxor(xmm0, xmm0);
				vpmaxsw(uv0, xmm0);
				vpmaxsw(uv1, xmm0);
			}

			vmovdqa(xmm0, ptr[r12 + offsetof(GSScanlineGlobalData, t.max)]);
			vpminsw(uv0, xmm0);
			vpminsw(uv1, xmm0);
		}
		else
		{
			vmovdqa(xmm0, ptr[r12 + offsetof(GSScanlineGlobalData, t.min)]);
			vpand(uv0, xmm0);
			vpand(uv1, xmm0);

			if(region)
			{
				vmovdqa(xmm0, ptr[r12 + offsetof(GSScanlineGlobalData, t.max)]);
				vpor(uv0, xmm0);
				vpor(uv1, xmm0);
			}
		}
	}
	else
	{
		vmovdqa(xmm2, ptr[r12 + offsetof(GSScanlineGlobalData, t.min)]);
		vmovdqa(xmm3, ptr[r12 + offsetof(GSScanlineGlobalData, t.max)]);
		vmovdqa(xmm0, ptr[r12 + offsetof(GSScanlineGlobalData, t.mask)]);

		// uv0

		// GSVector4i repeat = (t & m_local.gd->t.min) | m_local.gd->t.max;

		vpand(xmm1, uv0, xmm2);

		if(region)
		{
			vpor(xmm1, xmm3);
		}

		// GSVector4i clamp = t.sat_i16(m_local.gd->t.min, m_local.gd->t.max);

		vpmaxsw(uv0, xmm2);
		vpminsw(uv0, xmm3);

		// clamp.blend8(repeat, m_local.gd->t.mask);

		vpblendvb(uv0, xmm1, xmm0);

		// uv1

		// GSVector4i repeat = (t & m_local.gd->t.min) | m_local.gd->t.max;

		vpand(xmm1, uv1, xmm2);

		if(region)
		{
			vpor(xmm1, xmm3);
		}

		// GSVector4i clamp = t.sat_i16(m_local.gd->t.min, m_local.gd->t.max);

		vpmaxsw(uv1, xmm2);
		vpminsw(uv1, xmm3);

		// clamp.blend8(repeat, m_local.gd->t.mask);

		vpblendvb(uv1, xmm1, xmm0);
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

		// gat = gat.modulate16<1>(ga).clamp8();

		modulate16(xmm3, xmm14, 1);
		clamp16(xmm3, xmm0);

		// if(!tcc) gat = gat.mix16(ga.srl16(7));

		if(!m_sel.tcc)
		{
			vpsrlw(xmm1, xmm14, 7);
			mix16(xmm3, xmm1, xmm0);
		}

		break;

	case TFX_DECAL:

		// if(!tcc) gat = gat.mix16(ga.srl16(7));

		if(!m_sel.tcc)
		{
			vpsrlw(xmm1, xmm14, 7);
			mix16(xmm3, xmm1, xmm0);
		}

		break;

	case TFX_HIGHLIGHT:

		// gat = gat.mix16(!tcc ? ga.srl16(7) : gat.addus8(ga.srl16(7)));

		vpsrlw(xmm1, xmm14, 7);
		if(m_sel.tcc) vpaddusb(xmm1, xmm3);
		mix16(xmm3, xmm1, xmm0);

		break;

	case TFX_HIGHLIGHT2:

		// if(!tcc) gat = gat.mix16(ga.srl16(7));

		if(!m_sel.tcc)
		{
			vpsrlw(xmm1, xmm14, 7);
			mix16(xmm3, xmm1, xmm0);
		}

		break;

	case TFX_NONE:

		// gat = iip ? ga.srl16(7) : ga;

		if(m_sel.iip)
		{
			vpsrlw(xmm3, xmm14, 7);
		}

		break;
	}
}

void GSDrawScanlineCodeGenerator::ReadMask()
{
	if(m_sel.fwrite)
	{
		vmovdqa(xmm4, ptr[r12 + offsetof(GSScanlineGlobalData, fm)]);
	}

	if(m_sel.zwrite)
	{
		vmovdqa(xmm5, ptr[r12 + offsetof(GSScanlineGlobalData, zm)]);
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
		vpcmpeqd(xmm1, xmm1);
		break;

	case ATST_ALWAYS:
		return;

	case ATST_LESS:
	case ATST_LEQUAL:
		// t = (ga >> 16) > m_local.gd->aref;
		vpsrld(xmm1, xmm3, 16);
		vpcmpgtd(xmm1, ptr[r12 + offsetof(GSScanlineGlobalData, aref)]);
		break;

	case ATST_EQUAL:
		// t = (ga >> 16) != m_local.gd->aref;
		vpsrld(xmm1, xmm3, 16);
		vpcmpeqd(xmm1, ptr[r12 + offsetof(GSScanlineGlobalData, aref)]);
		vpcmpeqd(xmm0, xmm0);
		vpxor(xmm1, xmm0);
		break;

	case ATST_GEQUAL:
	case ATST_GREATER:
		// t = (ga >> 16) < m_local.gd->aref;
		vpsrld(xmm0, xmm3, 16);
		vmovdqa(xmm1, ptr[r12 + offsetof(GSScanlineGlobalData, aref)]);
		vpcmpgtd(xmm1, xmm0);
		break;

	case ATST_NOTEQUAL:
		// t = (ga >> 16) == m_local.gd->aref;
		vpsrld(xmm1, xmm3, 16);
		vpcmpeqd(xmm1, ptr[r12 + offsetof(GSScanlineGlobalData, aref)]);
		break;
	}

	switch(m_sel.afail)
	{
	case AFAIL_KEEP:
		// test |= t;
		vpor(xmm15, xmm1);
		alltrue();
		break;

	case AFAIL_FB_ONLY:
		// zm |= t;
		vpor(xmm5, xmm1);
		break;

	case AFAIL_ZB_ONLY:
		// fm |= t;
		vpor(xmm4, xmm1);
		break;

	case AFAIL_RGB_ONLY:
		// zm |= t;
		vpor(xmm5, xmm1);
		// fm |= t & GSVector4i::xff000000();
		vpsrld(xmm1, 24);
		vpslld(xmm1, 24);
		vpor(xmm4, xmm1);
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

		// rbt = rbt.modulate16<1>(rb).clamp8();

		modulate16(xmm2, xmm13, 1);
		clamp16(xmm2, xmm0);

		break;

	case TFX_DECAL:

		break;

	case TFX_HIGHLIGHT:
	case TFX_HIGHLIGHT2:

		vpshuflw(xmm6, xmm14, _MM_SHUFFLE(3, 3, 1, 1));
		vpshufhw(xmm6, xmm6, _MM_SHUFFLE(3, 3, 1, 1));
		vpsrlw(xmm6, 7);

		// gat = gat.modulate16<1>(ga).add16(af).clamp8().mix16(gat);

		vmovdqa(xmm1, xmm3);
		modulate16(xmm3, xmm14, 1);
		vpaddw(xmm3, xmm6);
		clamp16(xmm3, xmm0);
		mix16(xmm3, xmm1, xmm0);

		// rbt = rbt.modulate16<1>(rb).add16(af).clamp8();

		modulate16(xmm2, xmm13, 1);
		vpaddw(xmm2, xmm6);
		clamp16(xmm2, xmm0);

		break;

	case TFX_NONE:

		// rbt = iip ? rb.srl16(7) : rb;

		if(m_sel.iip)
		{
			vpsrlw(xmm2, xmm13, 7);
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

	vmovdqa(xmm0, ptr[r12 + offsetof(GSScanlineGlobalData, frb)]);
	vmovdqa(xmm1, ptr[r12 + offsetof(GSScanlineGlobalData, fga)]);

	vmovdqa(xmm6, xmm3);

	lerp16(xmm2, xmm0, xmm9, 0);
	lerp16(xmm3, xmm1, xmm9, 0);

	mix16(xmm3, xmm6, xmm9);
}

void GSDrawScanlineCodeGenerator::ReadFrame()
{
	if(!m_sel.fb)
	{
		return;
	}

	// int fa = fza_base.x + fza_offset->x;

	mov(ebx, dword[rsi]);
	add(ebx, dword[rdi]);
	movsxd(rbx, ebx);

	if(!m_sel.rfb)
	{
		return;
	}

	ReadPixel(xmm6, rbx);
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
			vpxor(xmm0, xmm0);
			vpsrld(xmm1, xmm6, 15);
			vpcmpeqd(xmm1, xmm0);
		}
		else
		{
			vpcmpeqd(xmm0, xmm0);
			vpxor(xmm1, xmm6, xmm0);
			vpsrad(xmm1, 31);
		}
	}
	else
	{
		if(m_sel.fpsm == 2)
		{
			vpslld(xmm1, xmm6, 16);
			vpsrad(xmm1, 31);
		}
		else
		{
			vpsrad(xmm1, xmm6, 31);
		}
	}

	vpor(xmm15, xmm1);

	alltrue();
}

void GSDrawScanlineCodeGenerator::WriteMask()
{
	// fm |= test;
	// zm |= test;

	if(m_sel.fwrite)
	{
		vpor(xmm4, xmm15);
	}

	if(m_sel.zwrite)
	{
		vpor(xmm5, xmm15);
	}

	// int fzm = ~(fm == GSVector4i::xffffffff()).ps32(zm == GSVector4i::xffffffff()).mask();

	vpcmpeqd(xmm1, xmm1);

	if(m_sel.fwrite && m_sel.zwrite)
	{
		vpcmpeqd(xmm0, xmm1, xmm5);
		vpcmpeqd(xmm1, xmm4);
		vpackssdw(xmm1, xmm0);
	}
	else if(m_sel.fwrite)
	{
		vpcmpeqd(xmm1, xmm4);
		vpackssdw(xmm1, xmm1);
	}
	else if(m_sel.zwrite)
	{
		vpcmpeqd(xmm1, xmm5);
		vpackssdw(xmm1, xmm1);
	}

	vpmovmskb(edx, xmm1);

	not(edx);
}

void GSDrawScanlineCodeGenerator::WriteZBuf()
{
	if(!m_sel.zwrite)
	{
		return;
	}

	bool fast = m_sel.ztest && m_sel.zpsm < 2;

	vmovdqa(xmm1, ptr[r11 + offsetof(GSScanlineLocalData, temp.zs)]);

	if(fast)
	{
		// zs = zs.blend8(zd, zm);

		vpblendvb(xmm1, ptr[r11 + offsetof(GSScanlineLocalData, temp.zd)], xmm4);
	}

	WritePixel(xmm1, rbp, dh, fast, m_sel.zpsm, 1);
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

			vpsllw(xmm0, xmm6, 8);
			vpsrlw(xmm0, 8);
			vpsrlw(xmm1, xmm6, 8);

			break;

		case 2:

			// c[2] = ((fd & 0x7c00) << 9) | ((fd & 0x001f) << 3);
			// c[3] = ((fd & 0x8000) << 8) | ((fd & 0x03e0) >> 2);

			vpcmpeqd(xmm15, xmm15);

			vpsrld(xmm15, 27); // 0x0000001f
			vpand(xmm0, xmm6, xmm15);
			vpslld(xmm0, 3);

			vpslld(xmm15, 10); // 0x00007c00
			vpand(xmm5, xmm6, xmm15);
			vpslld(xmm5, 9);

			vpor(xmm0, xmm1);

			vpsrld(xmm15, 5); // 0x000003e0
			vpand(xmm1, xmm6, xmm15);
			vpsrld(xmm1, 2);

			vpsllw(xmm15, 10); // 0x00008000
			vpand(xmm5, xmm6, xmm15);
			vpslld(xmm5, 8);

			vpor(xmm1, xmm5);

			break;
		}
	}

	// xmm2, xmm3 = src rb, ga
	// xmm0, xmm1 = dst rb, ga
	// xmm5, xmm15 = free

	if(m_sel.pabe || (m_sel.aba != m_sel.abb) && (m_sel.abb == 0 || m_sel.abd == 0))
	{
		vmovdqa(xmm5, xmm2);
	}

	if(m_sel.aba != m_sel.abb)
	{
		// rb = c[aba * 2 + 0];

		switch(m_sel.aba)
		{
		case 0: break;
		case 1: vmovdqa(xmm2, xmm0); break;
		case 2: vpxor(xmm2, xmm2); break;
		}

		// rb = rb.sub16(c[abb * 2 + 0]);

		switch(m_sel.abb)
		{
		case 0: vpsubw(xmm2, xmm5); break;
		case 1: vpsubw(xmm2, xmm0); break;
		case 2: break;
		}

		if(!(m_sel.fpsm == 1 && m_sel.abc == 1))
		{
			// GSVector4i a = abc < 2 ? c[abc * 2 + 1].yywwlh().sll16(7) : m_local.gd->afix;

			switch(m_sel.abc)
			{
			case 0:
			case 1:
				vpshuflw(xmm15, m_sel.abc ? xmm1 : xmm3, _MM_SHUFFLE(3, 3, 1, 1));
				vpshufhw(xmm15, xmm15, _MM_SHUFFLE(3, 3, 1, 1));
				vpsllw(xmm15, 7);
				break;
			case 2:
				vmovdqa(xmm15, ptr[r12 + offsetof(GSScanlineGlobalData, afix)]);
				break;
			}

			// rb = rb.modulate16<1>(a);

			modulate16(xmm2, xmm15, 1);
		}

		// rb = rb.add16(c[abd * 2 + 0]);

		switch(m_sel.abd)
		{
		case 0: vpaddw(xmm2, xmm5); break;
		case 1: vpaddw(xmm2, xmm0); break;
		case 2: break;
		}
	}
	else
	{
		// rb = c[abd * 2 + 0];

		switch(m_sel.abd)
		{
		case 0: break;
		case 1: vmovdqa(xmm2, xmm0); break;
		case 2: vpxor(xmm2, xmm2); break;
		}
	}

	if(m_sel.pabe)
	{
		// mask = (c[1] << 8).sra32(31);

		vpslld(xmm0, xmm3, 8);
		vpsrad(xmm0, 31);

		// rb = c[0].blend8(rb, mask);

		vpblendvb(xmm2, xmm5, xmm2, xmm0);
	}

	// xmm0 = pabe mask
	// xmm3 = src ga
	// xmm1 = dst ga
	// xmm2 = rb
	// xmm15 = a
	// xmm5 = free

	vmovdqa(xmm5, xmm3);

	if(m_sel.aba != m_sel.abb)
	{
		// ga = c[aba * 2 + 1];

		switch(m_sel.aba)
		{
		case 0: break;
		case 1: vmovdqa(xmm3, xmm1); break;
		case 2: vpxor(xmm3, xmm3); break;
		}

		// ga = ga.sub16(c[abeb * 2 + 1]);

		switch(m_sel.abb)
		{
		case 0: vpsubw(xmm3, xmm5); break;
		case 1: vpsubw(xmm3, xmm1); break;
		case 2: break;
		}

		if(!(m_sel.fpsm == 1 && m_sel.abc == 1))
		{
			// ga = ga.modulate16<1>(a);

			modulate16(xmm3, xmm15, 1);
		}

		// ga = ga.add16(c[abd * 2 + 1]);

		switch(m_sel.abd)
		{
		case 0: vpaddw(xmm3, xmm5); break;
		case 1: vpaddw(xmm3, xmm1); break;
		case 2: break;
		}
	}
	else
	{
		// ga = c[abd * 2 + 1];

		switch(m_sel.abd)
		{
		case 0: break;
		case 1: vmovdqa(xmm3, xmm1); break;
		case 2: vpxor(xmm3, xmm3); break;
		}
	}

	// xmm0 = pabe mask
	// xmm5 = src ga
	// xmm2 = rb
	// xmm3 = ga
	// xmm1, xmm15 = free

	if(m_sel.pabe)
	{
		vpsrld(xmm0, 16); // zero out high words to select the source alpha in blend (so it also does mix16)

		// ga = c[1].blend8(ga, mask).mix16(c[1]);

		vpblendvb(xmm3, xmm5, xmm3, xmm0);
	}
	else
	{
		if(m_sel.fpsm != 1) // TODO: fm == 0xffxxxxxx
		{
			mix16(xmm3, xmm5, xmm15);
		}
	}
}

void GSDrawScanlineCodeGenerator::WriteFrame()
{
	if(!m_sel.fwrite)
	{
		return;
	}

	if(m_sel.colclamp == 0)
	{
		// c[0] &= 0x000000ff;
		// c[1] &= 0x000000ff;

		vpcmpeqd(xmm15, xmm15);
		vpsrlw(xmm15, 8);
		vpand(xmm2, xmm15);
		vpand(xmm3, xmm15);
	}

	if(m_sel.fpsm == 2 && m_sel.dthe)
	{
		mov(rax, r8);
		and(rax, 3);
		shl(rax, 5);
		vpaddw(xmm2, ptr[r12 + rax + offsetof(GSScanlineGlobalData, dimx[0])]);
		vpaddw(xmm3, ptr[r12 + rax + offsetof(GSScanlineGlobalData, dimx[1])]);
	}

	// GSVector4i fs = c[0].upl16(c[1]).pu16(c[0].uph16(c[1]));

	vpunpckhwd(xmm15, xmm2, xmm3);
	vpunpcklwd(xmm2, xmm3);
	vpackuswb(xmm2, xmm15);

	if(m_sel.fba && m_sel.fpsm != 1)
	{
		// fs |= 0x80000000;

		vpcmpeqd(xmm15, xmm15);
		vpslld(xmm15, 31);
		vpor(xmm2, xmm15);
	}

	// xmm2 = fs
	// xmm4 = fm
	// xmm6 = fd

	if(m_sel.fpsm == 2)
	{
		// GSVector4i rb = fs & 0x00f800f8;
		// GSVector4i ga = fs & 0x8000f800;

		mov(eax, 0x00f800f8);
		vmovd(xmm0, eax);
		vpshufd(xmm0, xmm0, _MM_SHUFFLE(0, 0, 0, 0));

		mov(eax, 0x8000f800);
		vmovd(xmm1, eax);
		vpshufd(xmm1, xmm1, _MM_SHUFFLE(0, 0, 0, 0));

		vpand(xmm0, xmm2);
		vpand(xmm1, xmm2);

		// fs = (ga >> 16) | (rb >> 9) | (ga >> 6) | (rb >> 3);

		vpsrld(xmm2, xmm0, 9);
		vpsrld(xmm0, 3);
		vpsrld(xmm3, xmm1, 16);
		vpsrld(xmm1, 6);

		vpor(xmm0, xmm1);
		vpor(xmm2, xmm3);
		vpor(xmm2, xmm0);
	}

	if(m_sel.rfb)
	{
		// fs = fs.blend(fd, fm);

		blend(xmm2, xmm6, xmm4); // TODO: could be skipped in certain cases, depending on fpsm and fm
	}

	bool fast = m_sel.rfb && m_sel.fpsm < 2;

	WritePixel(xmm2, rbx, dl, fast, m_sel.fpsm, 0);
}

void GSDrawScanlineCodeGenerator::ReadPixel(const Xmm& dst, const Reg64& addr)
{
	vmovq(dst, qword[r13 + addr * 2]);
	vmovhps(dst, qword[r13 + addr * 2 + 8 * 2]);
}

void GSDrawScanlineCodeGenerator::WritePixel(const Xmm& src, const Reg64& addr, const Reg8& mask, bool fast, int psm, int fz)
{
	if(fast)
	{
		// if(fzm & 0x0f) GSVector4i::storel(&vm16[addr + 0], fs);
		// if(fzm & 0xf0) GSVector4i::storeh(&vm16[addr + 8], fs);

		test(mask, 0x0f);
		je("@f");
		vmovq(qword[r13 + addr * 2], src);
		L("@@");

		test(mask, 0xf0);
		je("@f");
		vmovhps(qword[r13 + addr * 2 + 8 * 2], src);
		L("@@");

		// vmaskmovps?
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

void GSDrawScanlineCodeGenerator::WritePixel(const Xmm& src, const Reg64& addr, uint8 i, int psm)
{
	Address dst = ptr[r13 + addr * 2 + s_offsets[i] * 2];

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

void GSDrawScanlineCodeGenerator::ReadTexel(const Xmm& dst, const Xmm& addr, const Xmm& temp1, const Xmm& temp2)
{
	ReadTexel(dst, addr, 0);
	ReadTexel(dst, addr, 1);
	ReadTexel(dst, addr, 2);
	ReadTexel(dst, addr, 3);
}

void GSDrawScanlineCodeGenerator::ReadTexel(const Xmm& dst, const Xmm& addr, uint8 i)
{
	const Address& src = m_sel.tlu ? ptr[r12 + rax * 4 + offsetof(GSScanlineGlobalData, clut)] : ptr[rbx + rax * 4];

	vpextrd(eax, addr, i);

	movsxd(rax, eax);

	if(m_sel.tlu) movzx(rax, byte[rbx + rax]);

	vpinsrd(dst, src, i);
}

#endif