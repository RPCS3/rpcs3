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

// TODO: x64

#include "StdAfx.h"
#include "GPUDrawScanlineCodeGenerator.h"

GPUDrawScanlineCodeGenerator::GPUDrawScanlineCodeGenerator(GPUScanlineEnvironment& env, void* ptr, size_t maxsize)
	: CodeGenerator(maxsize, ptr)
	, m_env(env)
{
	#if _M_AMD64
	#error TODO
	#endif

	Generate();
}

void GPUDrawScanlineCodeGenerator::Generate()
{
	push(esi);
	push(edi);

	const int params = 8;

	Init(params);

	align(16);

L("loop");

	// GSVector4i test = m_test[7 + (steps & (steps >> 31))];

	mov(edx, ecx);
	sar(edx, 31);
	and(edx, ecx);
	shl(edx, 4);

	movdqa(xmm7, xmmword[edx + (size_t)&m_test[7]]);

	// movdqu(xmm1, xmmword[edi]);

	movq(xmm1, qword[edi]);
	movhps(xmm1, qword[edi + 8]);

	// ecx = steps
	// esi = tex (tme)
	// edi = fb
	// xmm1 = fd
	// xmm2 = s
	// xmm3 = t
	// xmm4 = r
	// xmm5 = g
	// xmm6 = b
	// xmm7 = test

	TestMask();

	SampleTexture();

	// xmm1 = fd
	// xmm3 = a
	// xmm4 = r
	// xmm5 = g
	// xmm6 = b
	// xmm7 = test
	// xmm0, xmm2 = free

	ColorTFX();

	AlphaBlend();

	Dither();

	WriteFrame();

L("step");
	
	// if(steps <= 0) break;

	test(ecx, ecx);
	jle("exit", T_NEAR);

	Step();

	jmp("loop", T_NEAR);

L("exit");

	pop(edi);
	pop(esi);

	ret(8);
}

void GPUDrawScanlineCodeGenerator::Init(int params)
{
	const int _top = params + 4;
	const int _v = params + 8;

	mov(eax, dword[esp + _top]);

	// WORD* fb = &m_env.vm[(top << (10 + m_env.sel.scalex)) + left];

	mov(edi, eax);
	shl(edi, 10 + m_env.sel.scalex);
	add(edi, edx);
	lea(edi, ptr[edi * 2 + (size_t)m_env.vm]);

	// int steps = right - left - 8;

	sub(ecx, edx);
	sub(ecx, 8);

	if(m_env.sel.dtd)
	{
		// dither = GSVector4i::load<false>(&s_dither[top & 3][left & 3]);

		and(eax, 3);
		shl(eax, 5);
		and(edx, 3);
		shl(edx, 1);
		movdqu(xmm0, xmmword[eax + edx + (size_t)m_dither]);
		movdqa(xmmword[&m_env.temp.dither], xmm0);
	}

	mov(edx, dword[esp + _v]);

	if(m_env.sel.tme)
	{
		mov(esi, dword[&m_env.tex]);

		// GSVector4i vt = GSVector4i(v.t).xxzzl();

		cvttps2dq(xmm4, xmmword[edx + 32]);
		pshuflw(xmm4, xmm4, _MM_SHUFFLE(2, 2, 0, 0));

		// s = vt.xxxx().add16(m_env.d.s);
		// t = vt.yyyy().add16(m_env.d.t);

		pshufd(xmm2, xmm4, _MM_SHUFFLE(0, 0, 0, 0));
		pshufd(xmm3, xmm4, _MM_SHUFFLE(1, 1, 1, 1));

		paddw(xmm2, xmmword[&m_env.d.s]);
		
		if(!m_env.sel.sprite)
		{
			paddw(xmm3, xmmword[&m_env.d.t]); 
		}
		else
		{
			if(m_env.sel.ltf)
			{
				movdqa(xmm0, xmm3);
				psllw(xmm0, 8);
				psrlw(xmm0, 1);
				movdqa(xmmword[&m_env.temp.vf], xmm0);
			}
		}

		movdqa(xmmword[&m_env.temp.s], xmm2);
		movdqa(xmmword[&m_env.temp.t], xmm3);
	}

	if(m_env.sel.tfx != 3) // != decal
	{
		// GSVector4i vc = GSVector4i(v.c).xxzzlh();

		cvttps2dq(xmm6, xmmword[edx]);
		pshuflw(xmm6, xmm6, _MM_SHUFFLE(2, 2, 0, 0));
		pshufhw(xmm6, xmm6, _MM_SHUFFLE(2, 2, 0, 0));

		// r = vc.xxxx();
		// g = vc.yyyy();
		// b = vc.zzzz();

		pshufd(xmm4, xmm6, _MM_SHUFFLE(0, 0, 0, 0));
		pshufd(xmm5, xmm6, _MM_SHUFFLE(1, 1, 1, 1));
		pshufd(xmm6, xmm6, _MM_SHUFFLE(2, 2, 2, 2));

		if(m_env.sel.iip)
		{
			// r = r.add16(m_env.d.r);
			// g = g.add16(m_env.d.g);
			// b = b.add16(m_env.d.b);

			paddw(xmm4, xmmword[&m_env.d.r]);
			paddw(xmm5, xmmword[&m_env.d.g]);
			paddw(xmm6, xmmword[&m_env.d.b]);
		}

		movdqa(xmmword[&m_env.temp.r], xmm4);
		movdqa(xmmword[&m_env.temp.g], xmm5);
		movdqa(xmmword[&m_env.temp.b], xmm6);
	}
}

void GPUDrawScanlineCodeGenerator::Step()
{
	// steps -= 8;

	sub(ecx, 8);

	// fb += 8;

	add(edi, 8 * sizeof(WORD));

	if(m_env.sel.tme)
	{
		// GSVector4i st = m_env.d8.st;

		movdqa(xmm4, xmmword[&m_env.d8.st]);

		// s = s.add16(st.xxxx());
		// t = t.add16(st.yyyy());

		pshufd(xmm2, xmm4, _MM_SHUFFLE(0, 0, 0, 0));
		paddw(xmm2, xmmword[&m_env.temp.s]);
		movdqa(xmmword[&m_env.temp.s], xmm2);

		// TODO: if(!sprite) ... else reload t

		pshufd(xmm3, xmm4, _MM_SHUFFLE(1, 1, 1, 1));
		paddw(xmm3, xmmword[&m_env.temp.t]);
		movdqa(xmmword[&m_env.temp.t], xmm3);
	}

	if(m_env.sel.tfx != 3) // != decal
	{
		if(m_env.sel.iip)
		{
			// GSVector4i c = m_env.d8.c;

			// r = r.add16(c.xxxx());
			// g = g.add16(c.yyyy());
			// b = b.add16(c.zzzz());

			movdqa(xmm6, xmmword[&m_env.d8.c]);

			pshufd(xmm4, xmm6, _MM_SHUFFLE(0, 0, 0, 0));
			pshufd(xmm5, xmm6, _MM_SHUFFLE(1, 1, 1, 1));
			pshufd(xmm6, xmm6, _MM_SHUFFLE(2, 2, 2, 2));

			paddw(xmm4, xmmword[&m_env.temp.r]);
			paddw(xmm5, xmmword[&m_env.temp.g]);
			paddw(xmm6, xmmword[&m_env.temp.b]);

			movdqa(xmmword[&m_env.temp.r], xmm4);
			movdqa(xmmword[&m_env.temp.g], xmm5);
			movdqa(xmmword[&m_env.temp.b], xmm6);
		}
		else
		{
			movdqa(xmm4, xmmword[&m_env.temp.r]);
			movdqa(xmm5, xmmword[&m_env.temp.g]);
			movdqa(xmm6, xmmword[&m_env.temp.b]);
		}
	}
}

void GPUDrawScanlineCodeGenerator::TestMask()
{
	if(!m_env.sel.me)
	{
		return;
	}

	// test |= fd.sra16(15);

	movdqa(xmm0, xmm1);
	psraw(xmm0, 15);
	por(xmm7, xmm0);

	alltrue();
}

void GPUDrawScanlineCodeGenerator::SampleTexture()
{
	if(!m_env.sel.tme)
	{
		return;		
	}

	// xmm2 = s
	// xmm3 = t
	// xmm7 = test
	// xmm0, xmm4, xmm5, xmm6 = free
	// xmm1 = used

	if(m_env.sel.ltf)
	{
		// GSVector4i u = s.sub16(GSVector4i(0x00200020)); // - 0.125f
		// GSVector4i v = t.sub16(GSVector4i(0x00200020)); // - 0.125f

		mov(eax, 0x00200020);
		movd(xmm0, eax);
		pshufd(xmm0, xmm0, _MM_SHUFFLE(0, 0, 0, 0));

		psubw(xmm2, xmm0);
		psubw(xmm3, xmm0);

		// GSVector4i uf = (u & GSVector4i::x00ff()) << 7;
		// GSVector4i vf = (v & GSVector4i::x00ff()) << 7;

		movdqa(xmm0, xmm2);
		psllw(xmm0, 8);
		psrlw(xmm0, 1);
		movdqa(xmmword[&m_env.temp.uf], xmm0);

		if(!m_env.sel.sprite)
		{
			movdqa(xmm0, xmm3);
			psllw(xmm0, 8);
			psrlw(xmm0, 1);
			movdqa(xmmword[&m_env.temp.vf], xmm0);
		}
	}

	// GSVector4i u0 = s.srl16(8);
	// GSVector4i v0 = t.srl16(8);

	psrlw(xmm2, 8);
	psrlw(xmm3, 8);

	// xmm2 = u
	// xmm3 = v
	// xmm7 = test
	// xmm0, xmm4, xmm5, xmm6 = free
	// xmm1 = used

	if(m_env.sel.ltf)
	{
		// GSVector4i u1 = u0.add16(GSVector4i::x0001());
		// GSVector4i v1 = v0.add16(GSVector4i::x0001());

		movdqa(xmm4, xmm2);
		movdqa(xmm5, xmm3);

		pcmpeqd(xmm0, xmm0);
		psrlw(xmm0, 15);
		paddw(xmm4, xmm0);
		paddw(xmm5, xmm0);

		if(m_env.sel.twin)
		{
			// u0 = (u0 & m_env.twin[0].u).add16(m_env.twin[1].u);
			// v0 = (v0 & m_env.twin[0].v).add16(m_env.twin[1].v);
			// u1 = (u1 & m_env.twin[0].u).add16(m_env.twin[1].u);
			// v1 = (v1 & m_env.twin[0].v).add16(m_env.twin[1].v);

			movdqa(xmm0, xmmword[&m_env.twin[0].u]);
			movdqa(xmm6, xmmword[&m_env.twin[1].u]);

			pand(xmm2, xmm0);
			paddw(xmm2, xmm6);
			pand(xmm4, xmm0);
			paddw(xmm4, xmm6);

			movdqa(xmm0, xmmword[&m_env.twin[0].v]);
			movdqa(xmm6, xmmword[&m_env.twin[1].v]);

			pand(xmm3, xmm0);
			paddw(xmm3, xmm6);
			pand(xmm5, xmm0);
			paddw(xmm5, xmm6);
		}
		else
		{
			// u0 = u0.min_i16(m_env.twin[2].u);
			// v0 = v0.min_i16(m_env.twin[2].v);
			// u1 = u1.min_i16(m_env.twin[2].u);
			// v1 = v1.min_i16(m_env.twin[2].v);

			// TODO: if(!sprite) clamp16 else:

			movdqa(xmm0, xmmword[&m_env.twin[2].u]);
			movdqa(xmm6, xmmword[&m_env.twin[2].v]);

			pminsw(xmm2, xmm0);
			pminsw(xmm3, xmm6);
			pminsw(xmm4, xmm0);
			pminsw(xmm5, xmm6);
		}

		// xmm2 = u0
		// xmm3 = v0
		// xmm4 = u1
		// xmm5 = v1
		// xmm7 = test
		// xmm0, xmm6 = free
		// xmm1 = used

		// GSVector4i addr00 = v0.sll16(8) | u0;
		// GSVector4i addr01 = v0.sll16(8) | u1;
		// GSVector4i addr10 = v1.sll16(8) | u0;
		// GSVector4i addr11 = v1.sll16(8) | u1;

		psllw(xmm3, 8);
		movdqa(xmm0, xmm3);
		por(xmm3, xmm2);
		por(xmm0, xmm4);

		psllw(xmm5, 8);
		movdqa(xmm6, xmm5);
		por(xmm5, xmm2);
		por(xmm6, xmm4);

		// xmm3 = addr00
		// xmm0 = addr01
		// xmm5 = addr10
		// xmm6 = addr11
		// xmm7 = test
		// xmm2, xmm4 = free
		// xmm1 = used

		ReadTexel(xmm2, xmm3);
		ReadTexel(xmm4, xmm0);
		ReadTexel(xmm3, xmm5);
		ReadTexel(xmm5, xmm6);

		// xmm2 = c00
		// xmm4 = c01
		// xmm3 = c10
		// xmm5 = c11
		// xmm7 = test
		// xmm0, xmm6 = free
		// xmm1 = used
		
		// spill (TODO)

		movdqa(xmmword[&m_env.temp.fd], xmm1);
		movdqa(xmmword[&m_env.temp.test], xmm7);

		// xmm2 = c00
		// xmm4 = c01
		// xmm3 = c10
		// xmm5 = c11
		// xmm0, xmm1, xmm6, xmm7 = free

		movdqa(xmm1, xmm2);
		psllw(xmm1, 11);
		psrlw(xmm1, 8);

		movdqa(xmm0, xmm4);
		psllw(xmm0, 11);
		psrlw(xmm0, 8);

		lerp16<0>(xmm0, xmm1, xmmword[&m_env.temp.uf]);

		movdqa(xmm6, xmm2);
		psllw(xmm6, 6);
		psrlw(xmm6, 11);
		psllw(xmm6, 3);

		movdqa(xmm1, xmm4);
		psllw(xmm1, 6);
		psrlw(xmm1, 11);
		psllw(xmm1, 3);

		lerp16<0>(xmm1, xmm6, xmmword[&m_env.temp.uf]);

		movdqa(xmm7, xmm2);
		psllw(xmm7, 1);
		psrlw(xmm7, 11);
		psllw(xmm7, 3);

		movdqa(xmm6, xmm4);
		psllw(xmm6, 1);
		psrlw(xmm6, 11);
		psllw(xmm6, 3);

		lerp16<0>(xmm6, xmm7, xmmword[&m_env.temp.uf]);

		psraw(xmm2, 15);
		psrlw(xmm2, 8);
		psraw(xmm4, 15);
		psrlw(xmm4, 8);

		lerp16<0>(xmm4, xmm2, xmmword[&m_env.temp.uf]);

		// xmm0 = r00
		// xmm1 = g00
		// xmm6 = b00
		// xmm4 = a00
		// xmm3 = c10
		// xmm5 = c11
		// xmm2, xmm7 = free

		movdqa(xmm7, xmm3);
		psllw(xmm7, 11);
		psrlw(xmm7, 8);

		movdqa(xmm2, xmm5);
		psllw(xmm2, 11);
		psrlw(xmm2, 8);

		lerp16<0>(xmm2, xmm7, xmmword[&m_env.temp.uf]);
		lerp16<0>(xmm2, xmm0, xmmword[&m_env.temp.vf]);

		// xmm2 = r
		// xmm1 = g00
		// xmm6 = b00
		// xmm4 = a00
		// xmm3 = c10
		// xmm5 = c11
		// xmm0, xmm7 = free

		movdqa(xmm7, xmm3);
		psllw(xmm7, 6);
		psrlw(xmm7, 11);
		psllw(xmm7, 3);

		movdqa(xmm0, xmm5);
		psllw(xmm0, 6);
		psrlw(xmm0, 11);
		psllw(xmm0, 3);

		lerp16<0>(xmm0, xmm7, xmmword[&m_env.temp.uf]);
		lerp16<0>(xmm0, xmm1, xmmword[&m_env.temp.vf]);

		// xmm2 = r
		// xmm0 = g
		// xmm6 = b00
		// xmm4 = a00
		// xmm3 = c10
		// xmm5 = c11
		// xmm1, xmm7 = free

		movdqa(xmm7, xmm3);
		psllw(xmm7, 1);
		psrlw(xmm7, 11);
		psllw(xmm7, 3);

		movdqa(xmm1, xmm5);
		psllw(xmm1, 1);
		psrlw(xmm1, 11);
		psllw(xmm1, 3);

		lerp16<0>(xmm1, xmm7, xmmword[&m_env.temp.uf]);
		lerp16<0>(xmm1, xmm6, xmmword[&m_env.temp.vf]);

		// xmm2 = r
		// xmm0 = g
		// xmm1 = b
		// xmm4 = a00
		// xmm3 = c10
		// xmm5 = c11
		// xmm6, xmm7 = free

		psraw(xmm3, 15);
		psrlw(xmm3, 8);
		psraw(xmm5, 15);
		psrlw(xmm5, 8);

		lerp16<0>(xmm5, xmm3, xmmword[&m_env.temp.uf]);
		lerp16<0>(xmm5, xmm4, xmmword[&m_env.temp.vf]);

		// xmm2 = r
		// xmm0 = g
		// xmm1 = b
		// xmm5 = a
		// xmm3, xmm4, xmm6, xmm7 = free

		// TODO
		movdqa(xmm3, xmm5); // a
		movdqa(xmm4, xmm2); // r
		movdqa(xmm6, xmm1); // b
		movdqa(xmm5, xmm0); // g

		// reload test

		movdqa(xmm7, xmmword[&m_env.temp.test]);

		// xmm4 = r
		// xmm5 = g
		// xmm6 = b
		// xmm3 = a
		// xmm7 = test
		// xmm0, xmm1, xmm2 = free

		// test |= (c[0] | c[1] | c[2] | c[3]).eq16(GSVector4i::zero()); // mask out blank pixels (not perfect)

		movdqa(xmm1, xmm3);
		por(xmm1, xmm4);
		movdqa(xmm2, xmm5);
		por(xmm2, xmm6);
		por(xmm1, xmm2);

		pxor(xmm0, xmm0);
		pcmpeqw(xmm1, xmm0);
		por(xmm7, xmm1);

		// a = a.gt16(GSVector4i::zero());

		pcmpgtw(xmm3, xmm0);

		// reload fd

		movdqa(xmm1, xmmword[&m_env.temp.fd]);
	}
	else
	{
		if(m_env.sel.twin)
		{
			// u = (u & m_env.twin[0].u).add16(m_env.twin[1].u);
			// v = (v & m_env.twin[0].v).add16(m_env.twin[1].v);

			pand(xmm2, xmmword[&m_env.twin[0].u]);
			paddw(xmm2, xmmword[&m_env.twin[1].u]);
			pand(xmm3, xmmword[&m_env.twin[0].v]);
			paddw(xmm3, xmmword[&m_env.twin[1].v]);
		}
		else
		{
			// u = u.min_i16(m_env.twin[2].u);
			// v = v.min_i16(m_env.twin[2].v);

			// TODO: if(!sprite) clamp16 else:

			pminsw(xmm2, xmmword[&m_env.twin[2].u]);
			pminsw(xmm3, xmmword[&m_env.twin[2].v]);
		}

		// xmm2 = u
		// xmm3 = v
		// xmm7 = test
		// xmm0, xmm4, xmm5, xmm6 = free
		// xmm1 = used

		// GSVector4i addr = v.sll16(8) | u;

		psllw(xmm3, 8);
		por(xmm3, xmm2);

		// xmm3 = addr
		// xmm7 = test
		// xmm0, xmm2, xmm4, xmm5, xmm6 = free
		// xmm1 = used

		ReadTexel(xmm6, xmm3);

		// xmm3 = c00
		// xmm7 = test
		// xmm0, xmm2, xmm4, xmm5, xmm6 = free
		// xmm1 = used

		// test |= c00.eq16(GSVector4i::zero()); // mask out blank pixels

		pxor(xmm0, xmm0);
		pcmpeqw(xmm0, xmm6);
		por(xmm7, xmm0);

		// c[0] = (c00 << 3) & 0x00f800f8;
		// c[1] = (c00 >> 2) & 0x00f800f8;
		// c[2] = (c00 >> 7) & 0x00f800f8;
		// c[3] = c00.sra16(15);

		movdqa(xmm3, xmm6);
		psraw(xmm3, 15); // a

		pcmpeqd(xmm0, xmm0);
		psrlw(xmm0, 11);
		psllw(xmm0, 3); // 0x00f8

		movdqa(xmm4, xmm6);
		psllw(xmm4, 3);
		pand(xmm4, xmm0); // r

		movdqa(xmm5, xmm6);
		psrlw(xmm5, 2);
		pand(xmm5, xmm0); // g

		psrlw(xmm6, 7);
		pand(xmm6, xmm0); // b
	}
}

void GPUDrawScanlineCodeGenerator::ColorTFX()
{
	switch(m_env.sel.tfx)
	{
	case 0: // none (tfx = 0)
	case 1: // none (tfx = tge)
		// c[0] = r.srl16(7);
		// c[1] = g.srl16(7);
		// c[2] = b.srl16(7);
		psrlw(xmm4, 7);
		psrlw(xmm5, 7);
		psrlw(xmm6, 7);
		break;
	case 2: // modulate (tfx = tme | tge)
		// c[0] = c[0].modulate16<1>(r).clamp8();
		// c[1] = c[1].modulate16<1>(g).clamp8();
		// c[2] = c[2].modulate16<1>(b).clamp8();
		if(!m_cpu.has(util::Cpu::tSSE41)) pxor(xmm0, xmm0);
		modulate16<1>(xmm4, xmmword[&m_env.temp.r]);
		clamp16(xmm4, xmm0);
		modulate16<1>(xmm5, xmmword[&m_env.temp.g]);
		clamp16(xmm5, xmm0);
		modulate16<1>(xmm6, xmmword[&m_env.temp.b]);
		clamp16(xmm6, xmm0);
		break;
	case 3: // decal (tfx = tme)
		break;
	}
}

void GPUDrawScanlineCodeGenerator::AlphaBlend()
{
	if(!m_env.sel.abe)
	{
		return;
	}

	// xmm1 = fd
	// xmm3 = a
	// xmm4 = r
	// xmm5 = g
	// xmm6 = b
	// xmm7 = test
	// xmm0, xmm2 = free

	// GSVector4i r = (d & 0x001f001f) << 3;

	pcmpeqd(xmm0, xmm0);
	psrlw(xmm0, 11); // 0x001f
	movdqa(xmm2, xmm1);
	pand(xmm2, xmm0);
	psllw(xmm2, 3);

	switch(m_env.sel.abr)
	{
	case 0:
		// r = r.avg8(c[0]);
		pavgb(xmm2, xmm4);
		break;
	case 1:
		// r = r.addus8(c[0]);
		paddusb(xmm2, xmm4);
		break;
	case 2:
		// r = r.subus8(c[0]);
		psubusb(xmm2, xmm4);
		break;
	case 3:
		// r = r.addus8(c[0].srl16(2));
		movdqa(xmm0, xmm4);
		psrlw(xmm0, 2);
		paddusb(xmm2, xmm0);
		break;
	}

	if(m_env.sel.tme)
	{
		movdqa(xmm0, xmm3);
		blend8(xmm4, xmm2);
	}
	else
	{
		movdqa(xmm4, xmm2);
	}

	// GSVector4i g = (d & 0x03e003e0) >> 2;

	pcmpeqd(xmm0, xmm0);
	psrlw(xmm0, 11);
	psllw(xmm0, 5); // 0x03e0
	movdqa(xmm2, xmm1);
	pand(xmm2, xmm0);
	psrlw(xmm2, 2);

	switch(m_env.sel.abr)
	{
	case 0:
		// g = g.avg8(c[2]);
		pavgb(xmm2, xmm5);
		break;
	case 1:
		// g = g.addus8(c[2]);
		paddusb(xmm2, xmm5);
		break;
	case 2:
		// g = g.subus8(c[2]);
		psubusb(xmm2, xmm5);
		break;
	case 3:
		// g = g.addus8(c[2].srl16(2));
		movdqa(xmm0, xmm5);
		psrlw(xmm0, 2);
		paddusb(xmm2, xmm0);
		break;
	}

	if(m_env.sel.tme)
	{
		movdqa(xmm0, xmm3);
		blend8(xmm5, xmm2);
	}
	else
	{
		movdqa(xmm5, xmm2);
	}

	// GSVector4i b = (d & 0x7c007c00) >> 7;

	pcmpeqd(xmm0, xmm0);
	psrlw(xmm0, 11);
	psllw(xmm0, 10); // 0x7c00
	movdqa(xmm2, xmm1);
	pand(xmm2, xmm0);
	psrlw(xmm2, 7);

	switch(m_env.sel.abr)
	{
	case 0:
		// b = b.avg8(c[2]);
		pavgb(xmm2, xmm6);
		break;
	case 1:
		// b = b.addus8(c[2]);
		paddusb(xmm2, xmm6);
		break;
	case 2:
		// b = b.subus8(c[2]);
		psubusb(xmm2, xmm6);
		break;
	case 3:
		// b = b.addus8(c[2].srl16(2));
		movdqa(xmm0, xmm6);
		psrlw(xmm0, 2);
		paddusb(xmm2, xmm0);
		break;
	}

	if(m_env.sel.tme)
	{
		movdqa(xmm0, xmm3);
		blend8(xmm6, xmm2);
	}
	else
	{
		movdqa(xmm6, xmm2);
	}
}

void GPUDrawScanlineCodeGenerator::Dither()
{
	if(!m_env.sel.dtd)
	{
		return;
	}

	// c[0] = c[0].addus8(dither);
	// c[1] = c[1].addus8(dither);
	// c[2] = c[2].addus8(dither);

	movdqa(xmm0, xmmword[&m_env.temp.dither]);

	paddusb(xmm4, xmm0);
	paddusb(xmm5, xmm0);
	paddusb(xmm6, xmm0);
}

void GPUDrawScanlineCodeGenerator::WriteFrame()
{
	// GSVector4i fs = r | g | b | (m_env.sel.md ? GSVector4i(0x80008000) : m_env.sel.tme ? a : 0);

	pcmpeqd(xmm0, xmm0);
	
	if(m_env.sel.md || m_env.sel.tme) 
	{
		movdqa(xmm2, xmm0); 
		psllw(xmm2, 15);
	}
	
	psrlw(xmm0, 11);
	psllw(xmm0, 3);

	// xmm0 = 0x00f8
	// xmm2 = 0x8000 (md)

	// GSVector4i r = (c[0] & 0x00f800f8) >> 3;

	pand(xmm4, xmm0);
	psrlw(xmm4, 3);

	// GSVector4i g = (c[1] & 0x00f800f8) << 2;

	pand(xmm5, xmm0);
	psllw(xmm5, 2);
	por(xmm4, xmm5);

	// GSVector4i b = (c[2] & 0x00f800f8) << 7;

	pand(xmm6, xmm0);
	psllw(xmm6, 7);
	por(xmm4, xmm6);

	if(m_env.sel.md)
	{
		// GSVector4i a = GSVector4i(0x80008000);

		por(xmm4, xmm2);
	}
	else if(m_env.sel.tme)
	{
		// GSVector4i a = (c[3] << 8) & 0x80008000;

		psllw(xmm3, 8);
		pand(xmm3, xmm2);
		por(xmm4, xmm3);
	}

	// fs = fs.blend8(fd, test);

	movdqa(xmm0, xmm7);
	blend8(xmm4, xmm1);

	// GSVector4i::store<false>(fb, fs);

	// movdqu(xmmword[edi], xmm4);

	movq(qword[edi], xmm4);
	movhps(qword[edi + 8], xmm4);
}

void GPUDrawScanlineCodeGenerator::ReadTexel(const Xmm& dst, const Xmm& addr)
{
	for(int i = 0; i < 8; i++)
	{
		pextrw(eax, addr, (uint8)i);

		if(m_env.sel.tlu) movzx(eax, byte[esi + eax]);

		const Address& src = m_env.sel.tlu ? ptr[eax * 2 + (size_t)m_env.clut] : ptr[esi + eax * 2];

		if(i == 0) movd(dst, src);
		else pinsrw(dst, src, (uint8)i);
	}
}

template<int shift> 
void GPUDrawScanlineCodeGenerator::modulate16(const Xmm& a, const Operand& f)
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
void GPUDrawScanlineCodeGenerator::lerp16(const Xmm& a, const Xmm& b, const Operand& f)
{
	psubw(a, b);
	modulate16<shift>(a, f);
	paddw(a, b);
}

void GPUDrawScanlineCodeGenerator::clamp16(const Xmm& a, const Xmm& zero)
{
	packuswb(a, a);

	if(m_cpu.has(util::Cpu::tSSE41))
	{
		pmovzxbw(a, a);
	}
	else
	{
		punpcklbw(a, zero);
	}
}

void GPUDrawScanlineCodeGenerator::alltrue()
{
	pmovmskb(eax, xmm7);
	cmp(eax, 0xffff);
	je("step", T_NEAR);
}

void GPUDrawScanlineCodeGenerator::blend8(const Xmm& a, const Xmm& b)
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

void GPUDrawScanlineCodeGenerator::blend(const Xmm& a, const Xmm& b, const Xmm& mask)
{
	pand(b, mask);
	pandn(mask, a);
	por(b, mask);
	movdqa(a, b);
}

const GSVector4i GPUDrawScanlineCodeGenerator::m_test[8] = 
{
	GSVector4i(0xffff0000, 0xffffffff, 0xffffffff, 0xffffffff),
	GSVector4i(0x00000000, 0xffffffff, 0xffffffff, 0xffffffff),
	GSVector4i(0x00000000, 0xffff0000, 0xffffffff, 0xffffffff),
	GSVector4i(0x00000000, 0x00000000, 0xffffffff, 0xffffffff),
	GSVector4i(0x00000000, 0x00000000, 0xffff0000, 0xffffffff),
	GSVector4i(0x00000000, 0x00000000, 0x00000000, 0xffffffff),
	GSVector4i(0x00000000, 0x00000000, 0x00000000, 0xffff0000),
	GSVector4i::zero(),
};

__declspec(align(16)) const WORD GPUDrawScanlineCodeGenerator::m_dither[4][16] = 
{
	{7, 0, 6, 1, 7, 0, 6, 1, 7, 0, 6, 1, 7, 0, 6, 1},
	{2, 5, 3, 4, 2, 5, 3, 4, 2, 5, 3, 4, 2, 5, 3, 4}, 
	{1, 6, 0, 7, 1, 6, 0, 7, 1, 6, 0, 7, 1, 6, 0, 7}, 
	{4, 3, 5, 2, 4, 3, 5, 2, 4, 3, 5, 2, 4, 3, 5, 2}, 
};
