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
#include "GSClut.h"
#include "GSLocalMemory.h"

GSClut::GSClut(const GSLocalMemory* mem)
	: m_mem(mem)
{
	BYTE* p = (BYTE*)VirtualAlloc(NULL, 2 * 4096, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

	m_clut = (WORD*)&p[0]; // 1k + 1k for buffer overruns (sfex: PSM == PSM_PSMT8, CPSM == PSM_PSMCT32, CSA != 0)
	m_buff32 = (DWORD*)&p[2048]; // 1k
	m_buff64 = (UINT64*)&p[4096]; // 2k
	m_write.dirty = true;
	m_read.dirty = true;

	for(int i = 0; i < 16; i++)
	{
		for(int j = 0; j < 64; j++)
		{
			m_wc[0][i][j] = &GSClut::WriteCLUT_NULL;
			m_wc[1][i][j] = &GSClut::WriteCLUT_NULL;
		}
	}

	m_wc[0][PSM_PSMCT32][PSM_PSMT8] = &GSClut::WriteCLUT32_I8_CSM1;
	m_wc[0][PSM_PSMCT32][PSM_PSMT8H] = &GSClut::WriteCLUT32_I8_CSM1;
	m_wc[0][PSM_PSMCT32][PSM_PSMT4] = &GSClut::WriteCLUT32_I4_CSM1;
	m_wc[0][PSM_PSMCT32][PSM_PSMT4HL] = &GSClut::WriteCLUT32_I4_CSM1;
	m_wc[0][PSM_PSMCT32][PSM_PSMT4HH] = &GSClut::WriteCLUT32_I4_CSM1;
	m_wc[0][PSM_PSMCT24][PSM_PSMT8] = &GSClut::WriteCLUT32_I8_CSM1;
	m_wc[0][PSM_PSMCT24][PSM_PSMT8H] = &GSClut::WriteCLUT32_I8_CSM1;
	m_wc[0][PSM_PSMCT24][PSM_PSMT4] = &GSClut::WriteCLUT32_I4_CSM1;
	m_wc[0][PSM_PSMCT24][PSM_PSMT4HL] = &GSClut::WriteCLUT32_I4_CSM1;
	m_wc[0][PSM_PSMCT24][PSM_PSMT4HH] = &GSClut::WriteCLUT32_I4_CSM1;
	m_wc[0][PSM_PSMCT16][PSM_PSMT8] = &GSClut::WriteCLUT16_I8_CSM1;
	m_wc[0][PSM_PSMCT16][PSM_PSMT8H] = &GSClut::WriteCLUT16_I8_CSM1;
	m_wc[0][PSM_PSMCT16][PSM_PSMT4] = &GSClut::WriteCLUT16_I4_CSM1;
	m_wc[0][PSM_PSMCT16][PSM_PSMT4HL] = &GSClut::WriteCLUT16_I4_CSM1;
	m_wc[0][PSM_PSMCT16][PSM_PSMT4HH] = &GSClut::WriteCLUT16_I4_CSM1;
	m_wc[0][PSM_PSMCT16S][PSM_PSMT8] = &GSClut::WriteCLUT16S_I8_CSM1;
	m_wc[0][PSM_PSMCT16S][PSM_PSMT8H] = &GSClut::WriteCLUT16S_I8_CSM1;
	m_wc[0][PSM_PSMCT16S][PSM_PSMT4] = &GSClut::WriteCLUT16S_I4_CSM1;
	m_wc[0][PSM_PSMCT16S][PSM_PSMT4HL] = &GSClut::WriteCLUT16S_I4_CSM1;
	m_wc[0][PSM_PSMCT16S][PSM_PSMT4HH] = &GSClut::WriteCLUT16S_I4_CSM1;

	m_wc[1][PSM_PSMCT32][PSM_PSMT8] = &GSClut::WriteCLUT32_CSM2<256>;
	m_wc[1][PSM_PSMCT32][PSM_PSMT8H] = &GSClut::WriteCLUT32_CSM2<256>;
	m_wc[1][PSM_PSMCT32][PSM_PSMT4] = &GSClut::WriteCLUT32_CSM2<16>;
	m_wc[1][PSM_PSMCT32][PSM_PSMT4HL] = &GSClut::WriteCLUT32_CSM2<16>;
	m_wc[1][PSM_PSMCT32][PSM_PSMT4HH] = &GSClut::WriteCLUT32_CSM2<16>;
	m_wc[1][PSM_PSMCT24][PSM_PSMT8] = &GSClut::WriteCLUT32_CSM2<256>;
	m_wc[1][PSM_PSMCT24][PSM_PSMT8H] = &GSClut::WriteCLUT32_CSM2<256>;
	m_wc[1][PSM_PSMCT24][PSM_PSMT4] = &GSClut::WriteCLUT32_CSM2<16>;
	m_wc[1][PSM_PSMCT24][PSM_PSMT4HL] = &GSClut::WriteCLUT32_CSM2<16>;
	m_wc[1][PSM_PSMCT24][PSM_PSMT4HH] = &GSClut::WriteCLUT32_CSM2<16>;
	m_wc[1][PSM_PSMCT16][PSM_PSMT8] = &GSClut::WriteCLUT16_CSM2<256>;
	m_wc[1][PSM_PSMCT16][PSM_PSMT8H] = &GSClut::WriteCLUT16_CSM2<256>;
	m_wc[1][PSM_PSMCT16][PSM_PSMT4] = &GSClut::WriteCLUT16_CSM2<16>;
	m_wc[1][PSM_PSMCT16][PSM_PSMT4HL] = &GSClut::WriteCLUT16_CSM2<16>;
	m_wc[1][PSM_PSMCT16][PSM_PSMT4HH] = &GSClut::WriteCLUT16_CSM2<16>;
	m_wc[1][PSM_PSMCT16S][PSM_PSMT8] = &GSClut::WriteCLUT16S_CSM2<256>;
	m_wc[1][PSM_PSMCT16S][PSM_PSMT8H] = &GSClut::WriteCLUT16S_CSM2<256>;
	m_wc[1][PSM_PSMCT16S][PSM_PSMT4] = &GSClut::WriteCLUT16S_CSM2<16>;
	m_wc[1][PSM_PSMCT16S][PSM_PSMT4HL] = &GSClut::WriteCLUT16S_CSM2<16>;
	m_wc[1][PSM_PSMCT16S][PSM_PSMT4HH] = &GSClut::WriteCLUT16S_CSM2<16>;
}

GSClut::~GSClut()
{
	VirtualFree(m_clut, 0, MEM_RELEASE);
}

void GSClut::Invalidate() 
{
	m_write.dirty = true;
}

bool GSClut::WriteTest(const GIFRegTEX0& TEX0, const GIFRegTEXCLUT& TEXCLUT)
{
	switch(TEX0.CLD)
	{
	case 0: return false;
	case 1: break;
	case 2: m_CBP[0] = TEX0.CBP; break;
	case 3: m_CBP[1] = TEX0.CBP; break;
	case 4: if(m_CBP[0] == TEX0.CBP) return false; m_CBP[0] = TEX0.CBP; break;
	case 5: if(m_CBP[1] == TEX0.CBP) return false; m_CBP[1] = TEX0.CBP; break;
	case 6: ASSERT(0); return false;
	case 7: ASSERT(0); return false;
	default: __assume(0);
	}

	return m_write.IsDirty(TEX0, TEXCLUT);
}

void GSClut::Write(const GIFRegTEX0& TEX0, const GIFRegTEXCLUT& TEXCLUT)
{
	m_write.TEX0 = TEX0;
	m_write.TEXCLUT = TEXCLUT;
	m_write.dirty = false;
	m_read.dirty = true;

	(this->*m_wc[TEX0.CSM][TEX0.CPSM][TEX0.PSM])(TEX0, TEXCLUT);
}

void GSClut::WriteCLUT32_I8_CSM1(const GIFRegTEX0& TEX0, const GIFRegTEXCLUT& TEXCLUT)
{
	ASSERT(TEX0.CSA == 0);

	WriteCLUT_T32_I8_CSM1(&m_mem->m_vm32[m_mem->BlockAddress32(0, 0, TEX0.CBP, 1)], m_clut + (TEX0.CSA << 4));
}

void GSClut::WriteCLUT32_I4_CSM1(const GIFRegTEX0& TEX0, const GIFRegTEXCLUT& TEXCLUT)
{
	ASSERT(TEX0.CSA < 16);

	GSVector4i dummy; // this just forces stack alignment and enables inlining the next call

	WriteCLUT_T32_I4_CSM1(&m_mem->m_vm32[m_mem->BlockAddress32(0, 0, TEX0.CBP, 1)], m_clut + (TEX0.CSA << 4));
}

void GSClut::WriteCLUT16_I8_CSM1(const GIFRegTEX0& TEX0, const GIFRegTEXCLUT& TEXCLUT)
{
	ASSERT(TEX0.CSA < 16);

	WriteCLUT_T16_I8_CSM1(&m_mem->m_vm16[m_mem->BlockAddress16(0, 0, TEX0.CBP, 1)], m_clut + (TEX0.CSA << 4));
}

void GSClut::WriteCLUT16_I4_CSM1(const GIFRegTEX0& TEX0, const GIFRegTEXCLUT& TEXCLUT)
{
	ASSERT(TEX0.CSA < 32);

	WriteCLUT_T16_I4_CSM1(&m_mem->m_vm16[m_mem->BlockAddress16(0, 0, TEX0.CBP, 1)], m_clut + (TEX0.CSA << 4));
}

void GSClut::WriteCLUT16S_I8_CSM1(const GIFRegTEX0& TEX0, const GIFRegTEXCLUT& TEXCLUT)
{
	WriteCLUT_T16_I8_CSM1(&m_mem->m_vm16[m_mem->BlockAddress16S(0, 0, TEX0.CBP, 1)], m_clut + (TEX0.CSA << 4));
}

void GSClut::WriteCLUT16S_I4_CSM1(const GIFRegTEX0& TEX0, const GIFRegTEXCLUT& TEXCLUT)
{
	WriteCLUT_T16_I4_CSM1(&m_mem->m_vm16[m_mem->BlockAddress16S(0, 0, TEX0.CBP, 1)], m_clut + (TEX0.CSA << 4));
}

template<int n> void GSClut::WriteCLUT32_CSM2(const GIFRegTEX0& TEX0, const GIFRegTEXCLUT& TEXCLUT)
{
	WORD* RESTRICT clut = m_clut + (TEX0.CSA << 4);

	DWORD base = m_mem->PixelAddress32(0, TEXCLUT.COV, TEX0.CBP, TEXCLUT.CBW);
	int* offset = &m_mem->rowOffset32[TEXCLUT.COU << 4];

	for(int i = 0; i < n; i++)
	{
		DWORD c = m_mem->ReadPixel32(base + offset[i]);

		clut[i] = (WORD)(c & 0xffff);
		clut[i + 256] = (WORD)(c >> 16);
	}
}

template<int n> void GSClut::WriteCLUT16_CSM2(const GIFRegTEX0& TEX0, const GIFRegTEXCLUT& TEXCLUT)
{
	WORD* RESTRICT clut = m_clut + (TEX0.CSA << 4);

	DWORD base = m_mem->PixelAddress16(0, TEXCLUT.COV, TEX0.CBP, TEXCLUT.CBW);
	int* offset = &m_mem->rowOffset16[TEXCLUT.COU << 4];

	for(int i = 0; i < n; i++)
	{
		clut[i] = (WORD)m_mem->ReadPixel16(base + offset[i]);
	}
}

template<int n> void GSClut::WriteCLUT16S_CSM2(const GIFRegTEX0& TEX0, const GIFRegTEXCLUT& TEXCLUT)
{
	WORD* RESTRICT clut = m_clut + (TEX0.CSA << 4);

	DWORD base = m_mem->PixelAddress16S(0, TEXCLUT.COV, TEX0.CBP, TEXCLUT.CBW);
	int* offset = &m_mem->rowOffset16S[TEXCLUT.COU << 4];

	for(int i = 0; i < n; i++)
	{
		clut[i] = (WORD)m_mem->ReadPixel16(base + offset[i]);
	}
}

void GSClut::Read(const GIFRegTEX0& TEX0)
{
	if(m_read.IsDirty(TEX0))
	{
		m_read.TEX0 = TEX0;
		m_read.dirty = false;

		WORD* clut = m_clut + (TEX0.CSA << 4);

		if(TEX0.CPSM == PSM_PSMCT32 || TEX0.CPSM == PSM_PSMCT24)
		{
			switch(TEX0.PSM)
			{
			case PSM_PSMT8:
			case PSM_PSMT8H:
				ReadCLUT_T32_I8(clut, m_buff32);
				break;
			case PSM_PSMT4:
			case PSM_PSMT4HL:
			case PSM_PSMT4HH:
				ReadCLUT_T32_I4(clut, m_buff32, m_buff64);
				break;
			}
		}
		else if(TEX0.CPSM == PSM_PSMCT16 || TEX0.CPSM == PSM_PSMCT16S)
		{
			switch(TEX0.PSM)
			{
			case PSM_PSMT8:
			case PSM_PSMT8H:
				ReadCLUT_T16_I8(clut, m_buff32);
				break;
			case PSM_PSMT4:
			case PSM_PSMT4HL:
			case PSM_PSMT4HH:
				ReadCLUT_T16_I4(clut, m_buff32, m_buff64);
				break;
			}
		}
	}
}

void GSClut::Read32(const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA)
{
	if(m_read.IsDirty(TEX0, TEXA))
	{
		m_read.TEX0 = TEX0;
		m_read.TEXA = TEXA;
		m_read.dirty = false;
		m_read.adirty = true;

		WORD* clut = m_clut + (TEX0.CSA << 4);

		if(TEX0.CPSM == PSM_PSMCT32 || TEX0.CPSM == PSM_PSMCT24)
		{
			switch(TEX0.PSM)
			{
			case PSM_PSMT8:
			case PSM_PSMT8H:
				ReadCLUT_T32_I8(clut, m_buff32);
				break;
			case PSM_PSMT4:
			case PSM_PSMT4HL:
			case PSM_PSMT4HH:
				// TODO: merge these functions
				ReadCLUT_T32_I4(clut, m_buff32);
				ExpandCLUT64_T32_I8(m_buff32, (UINT64*)m_buff64); // sw renderer does not need m_buff64 anymore
				break;
			}
		}
		else if(TEX0.CPSM == PSM_PSMCT16 || TEX0.CPSM == PSM_PSMCT16S)
		{
			switch(TEX0.PSM)
			{
			case PSM_PSMT8:
			case PSM_PSMT8H:
				Expand16(clut, m_buff32, 256, TEXA);
				break;
			case PSM_PSMT4:
			case PSM_PSMT4HL:
			case PSM_PSMT4HH:
				// TODO: merge these functions
				Expand16(clut, m_buff32, 16, TEXA);
				ExpandCLUT64_T32_I8(m_buff32, (UINT64*)m_buff64); // sw renderer does not need m_buff64 anymore
				break;
			}
		}
	}
}

void GSClut::GetAlphaMinMax32(int& amin, int& amax)
{
	// call only after Read32

	ASSERT(!m_read.dirty);

	if(m_read.adirty)
	{
		m_read.adirty = false;

		// DWORD bpp = GSLocalMemory::m_psm[m_read.TEX0.PSM].trbpp;
		DWORD cbpp = GSLocalMemory::m_psm[m_read.TEX0.CPSM].trbpp;
		DWORD pal = GSLocalMemory::m_psm[m_read.TEX0.PSM].pal;

		if(cbpp == 24 && m_read.TEXA.AEM == 0)
		{
			m_read.amin = m_read.TEXA.TA0;
			m_read.amax = m_read.TEXA.TA0;
		}
		else
		{
			int amin = 255;
			int amax = 0;

			const GSVector4i* p = (const GSVector4i*)m_buff32;

			for(int i = 0, j = pal >> 4; i < j; i++)
			{
				GSVector4i v0 = (p[i * 4 + 0] >> 24).ps32(p[i * 4 + 1] >> 24);
				GSVector4i v1 = (p[i * 4 + 2] >> 24).ps32(p[i * 4 + 3] >> 24);

				GSVector4i v2 = v0.min_i16(v1);
				GSVector4i v3 = v0.max_i16(v1);

				v2 = v2.min_i16(v2.zwxy());
				v3 = v3.max_i16(v3.zwxy());
				v2 = v2.min_i16(v2.zwxyl());
				v3 = v3.max_i16(v3.zwxyl());
				v2 = v2.min_i16(v2.yxwzl());
				v3 = v3.max_i16(v3.yxwzl());

				amin = min(amin, v2.extract16<0>());
				amax = max(amax, v3.extract16<0>());
			}

			m_read.amin = amin;
			m_read.amax = amax;
		}
	}

	amin = m_read.amin;
	amax = m_read.amax;
}

//

void GSClut::WriteCLUT_T32_I8_CSM1(const DWORD* RESTRICT src, WORD* RESTRICT clut)
{
	#if _M_SSE >= 0x200

	for(int i = 0; i < 64; i += 16)
	{
		WriteCLUT_T32_I4_CSM1(&src[i +   0], &clut[i * 2 +   0]);
		WriteCLUT_T32_I4_CSM1(&src[i +  64], &clut[i * 2 +  16]);
		WriteCLUT_T32_I4_CSM1(&src[i + 128], &clut[i * 2 + 128]);
		WriteCLUT_T32_I4_CSM1(&src[i + 192], &clut[i * 2 + 144]);
	}

	#else

	for(int j = 0; j < 2; j++, src += 128, clut += 128)
	{
		for(int i = 0; i < 128; i++) 
		{
			DWORD c = src[clutTableT32I8[i]];
			clut[i] = (WORD)(c & 0xffff);
			clut[i + 256] = (WORD)(c >> 16);
		}
	}

	#endif
}

__forceinline void GSClut::WriteCLUT_T32_I4_CSM1(const DWORD* RESTRICT src, WORD* RESTRICT clut)
{
	#if _M_SSE >= 0x200

	GSVector4i* s = (GSVector4i*)src;
	GSVector4i* d = (GSVector4i*)clut;

	GSVector4i v0 = s[0];
	GSVector4i v1 = s[1];
	GSVector4i v2 = s[2];
	GSVector4i v3 = s[3];

	GSVector4i::sw64(v0, v1, v2, v3);
	GSVector4i::sw16(v0, v1, v2, v3);
	GSVector4i::sw16(v0, v2, v1, v3);
	GSVector4i::sw16(v0, v1, v2, v3);

	d[0] = v0;
	d[1] = v1;
	d[32] = v2;
	d[33] = v3;

	#else

	for(int i = 0; i < 16; i++) 
	{
		DWORD c = src[clutTableT32I4[i]];
		clut[i] = (WORD)(c & 0xffff);
		clut[i + 256] = (WORD)(c >> 16);
	}

	#endif
}

void GSClut::WriteCLUT_T16_I8_CSM1(const WORD* RESTRICT src, WORD* RESTRICT clut)
{
	#if _M_SSE >= 0x200

	GSVector4i* s = (GSVector4i*)src;
	GSVector4i* d = (GSVector4i*)clut;

	for(int i = 0; i < 32; i += 4)
	{
		GSVector4i v0 = s[i + 0];
		GSVector4i v1 = s[i + 1];
		GSVector4i v2 = s[i + 2];
		GSVector4i v3 = s[i + 3];

		GSVector4i::sw16(v0, v1, v2, v3);
		GSVector4i::sw32(v0, v1, v2, v3);
		GSVector4i::sw16(v0, v2, v1, v3);

		d[i + 0] = v0;
		d[i + 1] = v2;
		d[i + 2] = v1;
		d[i + 3] = v3;
	}

	#else

	for(int j = 0; j < 8; j++, src += 32, clut += 32) 
	{
		for(int i = 0; i < 32; i++)
		{
			clut[i] = src[clutTableT16I8[i]];
		}
	}

	#endif
}

__forceinline void GSClut::WriteCLUT_T16_I4_CSM1(const WORD* RESTRICT src, WORD* RESTRICT clut)
{
	for(int i = 0; i < 16; i++) 
	{
		clut[i] = src[clutTableT16I4[i]];
	}
}

void GSClut::ReadCLUT_T32_I8(const WORD* RESTRICT clut, DWORD* RESTRICT dst)
{
	#if _M_SSE >= 0x200

	for(int i = 0; i < 256; i += 16)
	{
		ReadCLUT_T32_I4(&clut[i], &dst[i]);
	}

	#else 

	for(int i = 0; i < 256; i++)
	{
		dst[i] = ((DWORD)clut[i + 256] << 16) | clut[i];
	}

	#endif
}

__forceinline void GSClut::ReadCLUT_T32_I4(const WORD* RESTRICT clut, DWORD* RESTRICT dst)
{
	#if _M_SSE >= 0x200

	GSVector4i* s = (GSVector4i*)clut;
	GSVector4i* d = (GSVector4i*)dst;

	GSVector4i v0 = s[0];
	GSVector4i v1 = s[1];
	GSVector4i v2 = s[32];
	GSVector4i v3 = s[33];

	GSVector4i::sw16(v0, v2, v1, v3);

	d[0] = v0;
	d[1] = v1;
	d[2] = v2;
	d[3] = v3;

	#else 

	for(int i = 0; i < 16; i++)
	{
		dst[i] = ((DWORD)clut[i + 256] << 16) | clut[i];
	}

	#endif
}

__forceinline void GSClut::ReadCLUT_T32_I4(const WORD* RESTRICT clut, DWORD* RESTRICT dst32, UINT64* RESTRICT dst64)
{
	#if _M_SSE >= 0x200

	GSVector4i* s = (GSVector4i*)clut;
	GSVector4i* d32 = (GSVector4i*)dst32;
	GSVector4i* d64 = (GSVector4i*)dst64;

	GSVector4i s0 = s[0];
	GSVector4i s1 = s[1];
	GSVector4i s2 = s[32];
	GSVector4i s3 = s[33];

	GSVector4i::sw16(s0, s2, s1, s3);

	d32[0] = s0;
	d32[1] = s1;
	d32[2] = s2;
	d32[3] = s3;

	ExpandCLUT64_T32(s0, s0, s1, s2, s3, &d64[0]);
	ExpandCLUT64_T32(s1, s0, s1, s2, s3, &d64[32]);
	ExpandCLUT64_T32(s2, s0, s1, s2, s3, &d64[64]);
	ExpandCLUT64_T32(s3, s0, s1, s2, s3, &d64[96]);

	#else 

	for(int i = 0; i < 16; i++)
	{
		dst[i] = ((DWORD)clut[i + 256] << 16) | clut[i];
	}

	DWORD* d = (DWORD*)dst64;

	for(int j = 0; j < 16; j++, d += 32)
	{
		DWORD hi = dst32[j];

		for(int i = 0; i < 16; i++)
		{
			d[i * 2 + 0] = dst32[i];
			d[i * 2 + 1] = hi;
		}
	}

	#endif
}

void GSClut::ReadCLUT_T16_I8(const WORD* RESTRICT clut, DWORD* RESTRICT dst)
{
	#if _M_SSE >= 0x200

	for(int i = 0; i < 256; i += 16)
	{
		ReadCLUT_T16_I4(&clut[i], &dst[i]);
	}

	#else 

	for(int i = 0; i < 256; i++)
	{
		dst[i] = (DWORD)clut[i];
	}

	#endif
}

__forceinline void GSClut::ReadCLUT_T16_I4(const WORD* RESTRICT clut, DWORD* RESTRICT dst)
{
	#if _M_SSE >= 0x200

	GSVector4i* s = (GSVector4i*)clut;
	GSVector4i* d = (GSVector4i*)dst;

	GSVector4i v0 = s[0];
	GSVector4i v1 = s[1];

	d[0] = v0.upl16();
	d[1] = v0.uph16();
	d[2] = v1.upl16();
	d[3] = v1.uph16();

	#else 

	for(int i = 0; i < 16; i++)
	{
		dst[i] = (DWORD)clut[i];
	}

	#endif
}

__forceinline void GSClut::ReadCLUT_T16_I4(const WORD* RESTRICT clut, DWORD* RESTRICT dst32, UINT64* RESTRICT dst64)
{
	#if _M_SSE >= 0x200

	GSVector4i* s = (GSVector4i*)clut;
	GSVector4i* d32 = (GSVector4i*)dst32;
	GSVector4i* d64 = (GSVector4i*)dst64;

	GSVector4i v0 = s[0];
	GSVector4i v1 = s[1];

	GSVector4i s0 = v0.upl16();
	GSVector4i s1 = v0.uph16();
	GSVector4i s2 = v1.upl16();
	GSVector4i s3 = v1.uph16();

	d32[0] = s0;
	d32[1] = s1;
	d32[2] = s2;
	d32[3] = s3;

	ExpandCLUT64_T16(s0, s0, s1, s2, s3, &d64[0]);
	ExpandCLUT64_T16(s1, s0, s1, s2, s3, &d64[32]);
	ExpandCLUT64_T16(s2, s0, s1, s2, s3, &d64[64]);
	ExpandCLUT64_T16(s3, s0, s1, s2, s3, &d64[96]);

	#else 

	for(int i = 0; i < 16; i++)
	{
		dst32[i] = (DWORD)clut[i];
	}

	DWORD* d = (DWORD*)dst64;

	for(int j = 0; j < 16; j++, d += 32)
	{
		DWORD hi = dst32[j] << 16;

		for(int i = 0; i < 16; i++)
		{
			d[i * 2 + 0] = hi | (dst32[i] & 0xffff);
		}
	}

	#endif
}

void GSClut::ExpandCLUT64_T32_I8(const DWORD* RESTRICT src, UINT64* RESTRICT dst)
{
	#if _M_SSE >= 0x200

	GSVector4i* s = (GSVector4i*)src;
	GSVector4i* d = (GSVector4i*)dst;

	GSVector4i s0 = s[0];
	GSVector4i s1 = s[1];
	GSVector4i s2 = s[2];
	GSVector4i s3 = s[3];

	ExpandCLUT64_T32(s0, s0, s1, s2, s3, &d[0]);
	ExpandCLUT64_T32(s1, s0, s1, s2, s3, &d[32]);
	ExpandCLUT64_T32(s2, s0, s1, s2, s3, &d[64]);
	ExpandCLUT64_T32(s3, s0, s1, s2, s3, &d[96]);

	#else 

	DWORD* d = (DWORD*)dst;

	for(int j = 0; j < 16; j++, d += 32)
	{
		DWORD hi = src[j];

		for(int i = 0; i < 16; i++)
		{
			d[i * 2 + 0] = src[i];
			d[i * 2 + 1] = hi;
		}
	}

	#endif
}

__forceinline void GSClut::ExpandCLUT64_T32(const GSVector4i& hi, const GSVector4i& lo0, const GSVector4i& lo1, const GSVector4i& lo2, const GSVector4i& lo3, GSVector4i* dst)
{
	ExpandCLUT64_T32(hi.xxxx(), lo0, &dst[0]);
	ExpandCLUT64_T32(hi.xxxx(), lo1, &dst[2]);
	ExpandCLUT64_T32(hi.xxxx(), lo2, &dst[4]);
	ExpandCLUT64_T32(hi.xxxx(), lo3, &dst[6]);
	ExpandCLUT64_T32(hi.yyyy(), lo0, &dst[8]);
	ExpandCLUT64_T32(hi.yyyy(), lo1, &dst[10]);
	ExpandCLUT64_T32(hi.yyyy(), lo2, &dst[12]);
	ExpandCLUT64_T32(hi.yyyy(), lo3, &dst[14]);
	ExpandCLUT64_T32(hi.zzzz(), lo0, &dst[16]);
	ExpandCLUT64_T32(hi.zzzz(), lo1, &dst[18]);
	ExpandCLUT64_T32(hi.zzzz(), lo2, &dst[20]);
	ExpandCLUT64_T32(hi.zzzz(), lo3, &dst[22]);
	ExpandCLUT64_T32(hi.wwww(), lo0, &dst[24]);
	ExpandCLUT64_T32(hi.wwww(), lo1, &dst[26]);
	ExpandCLUT64_T32(hi.wwww(), lo2, &dst[28]);
	ExpandCLUT64_T32(hi.wwww(), lo3, &dst[30]);
}

__forceinline void GSClut::ExpandCLUT64_T32(const GSVector4i& hi, const GSVector4i& lo, GSVector4i* dst)
{
	dst[0] = lo.upl32(hi);
	dst[1] = lo.uph32(hi);
}

void GSClut::ExpandCLUT64_T16_I8(const DWORD* RESTRICT src, UINT64* RESTRICT dst)
{
	#if _M_SSE >= 0x200

	GSVector4i* s = (GSVector4i*)src;
	GSVector4i* d = (GSVector4i*)dst;

	GSVector4i s0 = s[0];
	GSVector4i s1 = s[1];
	GSVector4i s2 = s[2];
	GSVector4i s3 = s[3];

	ExpandCLUT64_T16(s0, s0, s1, s2, s3, &d[0]);
	ExpandCLUT64_T16(s1, s0, s1, s2, s3, &d[32]);
	ExpandCLUT64_T16(s2, s0, s1, s2, s3, &d[64]);
	ExpandCLUT64_T16(s3, s0, s1, s2, s3, &d[96]);

	#else

	DWORD* d = (DWORD*)dst;

	for(int j = 0; j < 16; j++, d += 32)
	{
		DWORD hi = src[j] << 16;

		for(int i = 0; i < 16; i++)
		{
			d[i * 2 + 0] = hi | (src[i] & 0xffff);
		}
	}

	#endif
}

__forceinline void GSClut::ExpandCLUT64_T16(const GSVector4i& hi, const GSVector4i& lo0, const GSVector4i& lo1, const GSVector4i& lo2, const GSVector4i& lo3, GSVector4i* dst)
{
	ExpandCLUT64_T16(hi.xxxx(), lo0, &dst[0]);
	ExpandCLUT64_T16(hi.xxxx(), lo1, &dst[2]);
	ExpandCLUT64_T16(hi.xxxx(), lo2, &dst[4]);
	ExpandCLUT64_T16(hi.xxxx(), lo3, &dst[6]);
	ExpandCLUT64_T16(hi.yyyy(), lo0, &dst[8]);
	ExpandCLUT64_T16(hi.yyyy(), lo1, &dst[10]);
	ExpandCLUT64_T16(hi.yyyy(), lo2, &dst[12]);
	ExpandCLUT64_T16(hi.yyyy(), lo3, &dst[14]);
	ExpandCLUT64_T16(hi.zzzz(), lo0, &dst[16]);
	ExpandCLUT64_T16(hi.zzzz(), lo1, &dst[18]);
	ExpandCLUT64_T16(hi.zzzz(), lo2, &dst[20]);
	ExpandCLUT64_T16(hi.zzzz(), lo3, &dst[22]);
	ExpandCLUT64_T16(hi.wwww(), lo0, &dst[24]);
	ExpandCLUT64_T16(hi.wwww(), lo1, &dst[26]);
	ExpandCLUT64_T16(hi.wwww(), lo2, &dst[28]);
	ExpandCLUT64_T16(hi.wwww(), lo3, &dst[30]);
}

__forceinline void GSClut::ExpandCLUT64_T16(const GSVector4i& hi, const GSVector4i& lo, GSVector4i* dst)
{
	dst[0] = lo.upl16(hi);
	dst[1] = lo.uph16(hi);
}

// TODO

static const GSVector4i s_am(0x00008000);
static const GSVector4i s_bm(0x00007c00);
static const GSVector4i s_gm(0x000003e0);
static const GSVector4i s_rm(0x0000001f);

void GSClut::Expand16(const WORD* RESTRICT src, DWORD* RESTRICT dst, int w, const GIFRegTEXA& TEXA)
{
	#if _M_SSE >= 0x200

	ASSERT((w & 7) == 0);

	const GSVector4i rm = s_rm;
	const GSVector4i gm = s_gm;
	const GSVector4i bm = s_bm;
	// const GSVector4i am = s_am;

	GSVector4i TA0(TEXA.TA0 << 24);
	GSVector4i TA1(TEXA.TA1 << 24);

	GSVector4i c, cl, ch;

	const GSVector4i* s = (const GSVector4i*)src;
	GSVector4i* d = (GSVector4i*)dst;

	if(!TEXA.AEM)
	{
		for(int i = 0, j = w >> 3; i < j; i++)
		{
			c = s[i];
			/*
			cl = c.upl16();
			ch = c.uph16();
			d[i * 2 + 0] = ((cl & rm) << 3) | ((cl & gm) << 6) | ((cl & bm) << 9) | TA1.blend(TA0, cl < am);
			d[i * 2 + 1] = ((ch & rm) << 3) | ((ch & gm) << 6) | ((ch & bm) << 9) | TA1.blend(TA0, ch < am);
			*/
			cl = c.upl16(c);
			ch = c.uph16(c);
			d[i * 2 + 0] = ((cl & rm) << 3) | ((cl & gm) << 6) | ((cl & bm) << 9) | TA0.blend8(TA1, cl.sra16(15));
			d[i * 2 + 1] = ((ch & rm) << 3) | ((ch & gm) << 6) | ((ch & bm) << 9) | TA0.blend8(TA1, ch.sra16(15));
		}
	}
	else
	{
		for(int i = 0, j = w >> 3; i < j; i++)
		{
			c = s[i];
			/*
			cl = c.upl16();
			ch = c.uph16();
			d[i * 2 + 0] = ((cl & rm) << 3) | ((cl & gm) << 6) | ((cl & bm) << 9) | TA1.blend(TA0, cl < am).andnot(cl == GSVector4i::zero());
			d[i * 2 + 1] = ((ch & rm) << 3) | ((ch & gm) << 6) | ((ch & bm) << 9) | TA1.blend(TA0, ch < am).andnot(ch == GSVector4i::zero());
			*/
			cl = c.upl16(c);
			ch = c.uph16(c);
			d[i * 2 + 0] = ((cl & rm) << 3) | ((cl & gm) << 6) | ((cl & bm) << 9) | TA0.blend8(TA1, cl.sra16(15)).andnot(cl == GSVector4i::zero());
			d[i * 2 + 1] = ((ch & rm) << 3) | ((ch & gm) << 6) | ((ch & bm) << 9) | TA0.blend8(TA1, ch.sra16(15)).andnot(ch == GSVector4i::zero());
		}
	}

	#else

	DWORD TA0 = (DWORD)TEXA.TA0 << 24;
	DWORD TA1 = (DWORD)TEXA.TA1 << 24;

	if(!TEXA.AEM)
	{
		for(int i = 0; i < w; i++)
		{
			dst[i] = ((src[i] & 0x8000) ? TA1 : TA0) | ((src[i] & 0x7c00) << 9) | ((src[i] & 0x03e0) << 6) | ((src[i] & 0x001f) << 3);
		}
	}
	else
	{
		for(int i = 0; i < w; i++)
		{
			dst[i] = ((src[i] & 0x8000) ? TA1 : src[i] ? TA0 : 0) | ((src[i] & 0x7c00) << 9) | ((src[i] & 0x03e0) << 6) | ((src[i] & 0x001f) << 3);
		}
	}

	#endif
}

//

bool GSClut::WriteState::IsDirty(const GIFRegTEX0& TEX0, const GIFRegTEXCLUT& TEXCLUT)
{
	return dirty || !(GSVector4i::load<true>(this) == GSVector4i::load(&TEX0, &TEXCLUT)).alltrue();
}

bool GSClut::ReadState::IsDirty(const GIFRegTEX0& TEX0)
{
	return dirty || !(GSVector4i::load<true>(this) == GSVector4i::load(&TEX0, &this->TEXA)).alltrue();
}

bool GSClut::ReadState::IsDirty(const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA)
{
	return dirty || !(GSVector4i::load<true>(this) == GSVector4i::load(&TEX0, &TEXA)).alltrue();
}
