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

#if _M_SSE >= 0x501

__aligned(const uint8, 8) GSDrawScanlineCodeGenerator::m_test[16][8] =
{
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00},
	{0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00},
	{0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00},
	{0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00},
	{0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
	{0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
	{0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff},
	{0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff},
	{0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff},
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff},
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff},
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
};

const GSVector8 GSDrawScanlineCodeGenerator::m_log2_coef[4] = 
{
	GSVector8(0.204446009836232697516f),
	GSVector8(-1.04913055217340124191f),
	GSVector8(2.28330284476918490682f),
	GSVector8(1.0f),
};

#else

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

const GSVector4 GSDrawScanlineCodeGenerator::m_log2_coef[4] = 
{
	GSVector4(0.204446009836232697516f),
	GSVector4(-1.04913055217340124191f),
	GSVector4(2.28330284476918490682f),
	GSVector4(1.0f),
};

#endif

GSDrawScanlineCodeGenerator::GSDrawScanlineCodeGenerator(void* param, uint64 key, void* code, size_t maxsize)
	: GSCodeGenerator(code, maxsize)
	, m_local(*(GSScanlineLocalData*)param)
{
	m_sel.key = key;

	Generate();
}

#if _M_SSE >= 0x501

void GSDrawScanlineCodeGenerator::modulate16(const Ymm& a, const Operand& f, int shift)
{
	if(shift == 0)
	{
		vpmulhrsw(a, f);
	}
	else
	{
		vpsllw(a, (uint8)(shift + 1));
		vpmulhw(a, f);
	}
}

void GSDrawScanlineCodeGenerator::lerp16(const Ymm& a, const Ymm& b, const Ymm& f, int shift)
{
	vpsubw(a, b);
	modulate16(a, f, shift);
	vpaddw(a, b);
}

void GSDrawScanlineCodeGenerator::lerp16_4(const Ymm& a, const Ymm& b, const Ymm& f)
{
	vpsubw(a, b);
	vpmullw(a, f);
	vpsraw(a, 4);
	vpaddw(a, b);
}

void GSDrawScanlineCodeGenerator::mix16(const Ymm& a, const Ymm& b, const Ymm& temp)
{
	vpblendw(a, b, 0xaa);
}

void GSDrawScanlineCodeGenerator::clamp16(const Ymm& a, const Ymm& temp)
{
	vpackuswb(a, a);
	vpermq(a, a, _MM_SHUFFLE(3, 1, 2, 0)); // this sucks
	vpmovzxbw(a, a);
}

void GSDrawScanlineCodeGenerator::alltrue()
{
	vpmovmskb(eax, ymm7);
	cmp(eax, 0xffffffff);
	je("step", T_NEAR);
}

void GSDrawScanlineCodeGenerator::blend(const Ymm& a, const Ymm& b, const Ymm& mask)
{
	vpand(b, mask);
	vpandn(mask, a);
	vpor(a, b, mask);
}

void GSDrawScanlineCodeGenerator::blendr(const Ymm& b, const Ymm& a, const Ymm& mask)
{
	vpand(b, mask);
	vpandn(mask, a);
	vpor(b, mask);
}

void GSDrawScanlineCodeGenerator::blend8(const Ymm& a, const Ymm& b)
{
	vpblendvb(a, a, b, xmm0);
}

void GSDrawScanlineCodeGenerator::blend8r(const Ymm& b, const Ymm& a)
{
	vpblendvb(b, a, b, xmm0);
}

#else

void GSDrawScanlineCodeGenerator::modulate16(const Xmm& a, const Operand& f, int shift)
{
	#if _M_SSE >= 0x500

	if(shift == 0)
	{
		vpmulhrsw(a, f);
	}
	else
	{
		vpsllw(a, shift + 1);
		vpmulhw(a, f);
	}

	#else

	if(shift == 0 && m_cpu.has(util::Cpu::tSSSE3))
	{
		pmulhrsw(a, f);
	}
	else
	{
		psllw(a, shift + 1);
		pmulhw(a, f);
	}

	#endif
}

void GSDrawScanlineCodeGenerator::lerp16(const Xmm& a, const Xmm& b, const Xmm& f, int shift)
{
	#if _M_SSE >= 0x500

	vpsubw(a, b);
	modulate16(a, f, shift);
	vpaddw(a, b);

	#else

	psubw(a, b);
	modulate16(a, f, shift);
	paddw(a, b);

	#endif
}

void GSDrawScanlineCodeGenerator::lerp16_4(const Xmm& a, const Xmm& b, const Xmm& f)
{
	#if _M_SSE >= 0x500

	vpsubw(a, b);
	vpmullw(a, f);
	vpsraw(a, 4);
	vpaddw(a, b);

	#else

	psubw(a, b);
	pmullw(a, f);
	psraw(a, 4);
	paddw(a, b);

	#endif
}

void GSDrawScanlineCodeGenerator::mix16(const Xmm& a, const Xmm& b, const Xmm& temp)
{
	#if _M_SSE >= 0x500

	vpblendw(a, b, 0xaa);
	
	#elif _M_SSE >= 0x401

	pblendw(a, b, 0xaa);

	#else

	pcmpeqd(temp, temp);
	psrld(temp, 16);
	pand(a, temp);
	pandn(temp, b);
	por(a, temp);
	
	#endif
}

void GSDrawScanlineCodeGenerator::clamp16(const Xmm& a, const Xmm& temp)
{
	#if _M_SSE >= 0x500
	
	vpackuswb(a, a);
	vpmovzxbw(a, a);

	#elif _M_SSE >= 0x401

	packuswb(a, a);
	pmovzxbw(a, a);

	#else

	packuswb(a, a);
	pxor(temp, temp);
	punpcklbw(a, temp);

	#endif
}

void GSDrawScanlineCodeGenerator::alltrue()
{
	#if _M_SSE >= 0x500
	
	vpmovmskb(eax, xmm7);
	cmp(eax, 0xffff);
	je("step", T_NEAR);

	#else

	pmovmskb(eax, xmm7);
	cmp(eax, 0xffff);
	je("step", T_NEAR);

	#endif
}

void GSDrawScanlineCodeGenerator::blend(const Xmm& a, const Xmm& b, const Xmm& mask)
{
	#if _M_SSE >= 0x500

	vpand(b, mask);
	vpandn(mask, a);
	vpor(a, b, mask);

	#else

	pand(b, mask);
	pandn(mask, a);
	por(b, mask);
	movdqa(a, b);

	#endif
}

void GSDrawScanlineCodeGenerator::blendr(const Xmm& b, const Xmm& a, const Xmm& mask)
{
	#if _M_SSE >= 0x500

	vpand(b, mask);
	vpandn(mask, a);
	vpor(b, mask);

	#else

	pand(b, mask);
	pandn(mask, a);
	por(b, mask);

	#endif
}

void GSDrawScanlineCodeGenerator::blend8(const Xmm& a, const Xmm& b)
{
	#if _M_SSE >= 0x500
	
	vpblendvb(a, a, b, xmm0);

	#elif _M_SSE >= 0x401
	
	pblendvb(a, b);

	#else

	blend(a, b, xmm0);

	#endif
}

void GSDrawScanlineCodeGenerator::blend8r(const Xmm& b, const Xmm& a)
{
	#if _M_SSE >= 0x500
	
	vpblendvb(b, a, b, xmm0);

	#elif _M_SSE >= 0x401

	pblendvb(a, b);
	movdqa(b, a);

	#else

	blendr(b, a, xmm0);

	#endif
}

#endif