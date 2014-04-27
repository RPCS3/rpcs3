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
#include "GSClut.h"
#include "GSLocalMemory.h"

#define CLUT_ALLOC_SIZE (2 * 4096)

GSClut::GSClut(GSLocalMemory* mem)
	: m_mem(mem)
{
	uint8* p = (uint8*)vmalloc(CLUT_ALLOC_SIZE, false);

	m_clut = (uint16*)&p[0]; // 1k + 1k for mirrored area simulating wrapping memory
	m_buff32 = (uint32*)&p[2048]; // 1k
	m_buff64 = (uint64*)&p[4096]; // 2k
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
	vmfree(m_clut, CLUT_ALLOC_SIZE);
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
	case 6: ASSERT(0); return false; // ffx2 menu
	case 7: ASSERT(0); return false; // ford mustang racing
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

	// Mirror write to other half of buffer to simulate wrapping memory

	int offset = (TEX0.CSA & (TEX0.CPSM < PSM_PSMCT16 ? 15 : 31)) * 16;

	if(TEX0.PSM == PSM_PSMT8 || TEX0.PSM == PSM_PSMT8H)
	{
		int size = TEX0.CPSM < PSM_PSMCT16 ? 512 : 256;

		memcpy(m_clut + 512 + offset, m_clut + offset, sizeof(*m_clut) * min(size, 512 - offset));
		memcpy(m_clut, m_clut + 512, sizeof(*m_clut) * max(0, size + offset - 512));
	}
	else
	{
		int size = 16;

		memcpy(m_clut + 512 + offset, m_clut + offset, sizeof(*m_clut) * size);
		
		if(TEX0.CPSM < PSM_PSMCT16)
		{
			memcpy(m_clut + 512 + 256 + offset, m_clut + 256 + offset, sizeof(*m_clut) * size);
		}
	}
}

void GSClut::WriteCLUT32_I8_CSM1(const GIFRegTEX0& TEX0, const GIFRegTEXCLUT& TEXCLUT)
{
	ALIGN_STACK(32);

	WriteCLUT_T32_I8_CSM1((uint32*)m_mem->BlockPtr32(0, 0, TEX0.CBP, 1), m_clut + ((TEX0.CSA & 15) << 4));
}

void GSClut::WriteCLUT32_I4_CSM1(const GIFRegTEX0& TEX0, const GIFRegTEXCLUT& TEXCLUT)
{
	ALIGN_STACK(32);

	WriteCLUT_T32_I4_CSM1((uint32*)m_mem->BlockPtr32(0, 0, TEX0.CBP, 1), m_clut + ((TEX0.CSA & 15) << 4));
}

void GSClut::WriteCLUT16_I8_CSM1(const GIFRegTEX0& TEX0, const GIFRegTEXCLUT& TEXCLUT)
{
	WriteCLUT_T16_I8_CSM1((uint16*)m_mem->BlockPtr16(0, 0, TEX0.CBP, 1), m_clut + (TEX0.CSA << 4));
}

void GSClut::WriteCLUT16_I4_CSM1(const GIFRegTEX0& TEX0, const GIFRegTEXCLUT& TEXCLUT)
{
	WriteCLUT_T16_I4_CSM1((uint16*)m_mem->BlockPtr16(0, 0, TEX0.CBP, 1), m_clut + (TEX0.CSA << 4));
}

void GSClut::WriteCLUT16S_I8_CSM1(const GIFRegTEX0& TEX0, const GIFRegTEXCLUT& TEXCLUT)
{
	WriteCLUT_T16_I8_CSM1((uint16*)m_mem->BlockPtr16S(0, 0, TEX0.CBP, 1), m_clut + (TEX0.CSA << 4));
}

void GSClut::WriteCLUT16S_I4_CSM1(const GIFRegTEX0& TEX0, const GIFRegTEXCLUT& TEXCLUT)
{
	WriteCLUT_T16_I4_CSM1((uint16*)m_mem->BlockPtr16S(0, 0, TEX0.CBP, 1), m_clut + (TEX0.CSA << 4));
}

template<int n> void GSClut::WriteCLUT32_CSM2(const GIFRegTEX0& TEX0, const GIFRegTEXCLUT& TEXCLUT)
{
	GSOffset* o = m_mem->GetOffset(TEX0.CBP, TEXCLUT.CBW, PSM_PSMCT32);

	uint32* RESTRICT s = &m_mem->m_vm32[o->pixel.row[TEXCLUT.COV]];
	int* RESTRICT col = &o->pixel.col[0][TEXCLUT.COU << 4];

	uint16* RESTRICT clut = m_clut + ((TEX0.CSA & 15) << 4);

	for(int i = 0; i < n; i++)
	{
		uint32 c = s[col[i]];

		clut[i] = (uint16)(c & 0xffff);
		clut[i + 256] = (uint16)(c >> 16);
	}
}

template<int n> void GSClut::WriteCLUT16_CSM2(const GIFRegTEX0& TEX0, const GIFRegTEXCLUT& TEXCLUT)
{
	GSOffset* o = m_mem->GetOffset(TEX0.CBP, TEXCLUT.CBW, PSM_PSMCT16);

	uint16* RESTRICT s = &m_mem->m_vm16[o->pixel.row[TEXCLUT.COV]];
	int* RESTRICT col = &o->pixel.col[0][TEXCLUT.COU << 4];

	uint16* RESTRICT clut = m_clut + (TEX0.CSA << 4);

	for(int i = 0; i < n; i++)
	{
		clut[i] = s[col[i]];
	}
}

template<int n> void GSClut::WriteCLUT16S_CSM2(const GIFRegTEX0& TEX0, const GIFRegTEXCLUT& TEXCLUT)
{
	GSOffset* o = m_mem->GetOffset(TEX0.CBP, TEXCLUT.CBW, PSM_PSMCT16S);

	uint16* RESTRICT s = &m_mem->m_vm16[o->pixel.row[TEXCLUT.COV]];
	int* RESTRICT col = &o->pixel.col[0][TEXCLUT.COU << 4];

	uint16* RESTRICT clut = m_clut + (TEX0.CSA << 4);

	for(int i = 0; i < n; i++)
	{
		clut[i] = s[col[i]];
	}
}

void GSClut::Read(const GIFRegTEX0& TEX0)
{
	if(m_read.IsDirty(TEX0))
	{
		m_read.TEX0 = TEX0;
		m_read.dirty = false;

		uint16* clut = m_clut;

		if(TEX0.CPSM == PSM_PSMCT32 || TEX0.CPSM == PSM_PSMCT24)
		{
			switch(TEX0.PSM)
			{
			case PSM_PSMT8:
			case PSM_PSMT8H:
				clut += (TEX0.CSA & 15) << 4;
				ReadCLUT_T32_I8(clut, m_buff32);
				break;
			case PSM_PSMT4:
			case PSM_PSMT4HL:
			case PSM_PSMT4HH:
				clut += (TEX0.CSA & 15) << 4;
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
				clut += TEX0.CSA << 4;
				ReadCLUT_T16_I8(clut, m_buff32);
				break;
			case PSM_PSMT4:
			case PSM_PSMT4HL:
			case PSM_PSMT4HH:
				clut += TEX0.CSA << 4;
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

		uint16* clut = m_clut;

		if(TEX0.CPSM == PSM_PSMCT32 || TEX0.CPSM == PSM_PSMCT24)
		{
			switch(TEX0.PSM)
			{
			case PSM_PSMT8:
			case PSM_PSMT8H:
				clut += (TEX0.CSA & 15) << 4; // disney golf title screen
				ReadCLUT_T32_I8(clut, m_buff32);
				break;
			case PSM_PSMT4:
			case PSM_PSMT4HL:
			case PSM_PSMT4HH:
				clut += (TEX0.CSA & 15) << 4;
				// TODO: merge these functions
				ReadCLUT_T32_I4(clut, m_buff32);
				ExpandCLUT64_T32_I8(m_buff32, (uint64*)m_buff64); // sw renderer does not need m_buff64 anymore
				break;
			}
		}
		else if(TEX0.CPSM == PSM_PSMCT16 || TEX0.CPSM == PSM_PSMCT16S)
		{
			switch(TEX0.PSM)
			{
			case PSM_PSMT8:
			case PSM_PSMT8H:
				clut += TEX0.CSA << 4;
				Expand16(clut, m_buff32, 256, TEXA);
				break;
			case PSM_PSMT4:
			case PSM_PSMT4HL:
			case PSM_PSMT4HH:
				clut += TEX0.CSA << 4;
				// TODO: merge these functions
				Expand16(clut, m_buff32, 16, TEXA);
				ExpandCLUT64_T32_I8(m_buff32, (uint64*)m_buff64); // sw renderer does not need m_buff64 anymore
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

		if(GSLocalMemory::m_psm[m_read.TEX0.CPSM].trbpp == 24 && m_read.TEXA.AEM == 0)
		{
			m_read.amin = m_read.TEXA.TA0;
			m_read.amax = m_read.TEXA.TA0;
		}
		else
		{
			const GSVector4i* p = (const GSVector4i*)m_buff32;

			GSVector4i amin, amax;

			if(GSLocalMemory::m_psm[m_read.TEX0.PSM].pal == 256)
			{
				amin = GSVector4i::xffffffff();
				amax = GSVector4i::zero();

				for(int i = 0; i < 16; i++)
				{
					GSVector4i v0 = (p[i * 4 + 0] >> 24).ps32(p[i * 4 + 1] >> 24);
					GSVector4i v1 = (p[i * 4 + 2] >> 24).ps32(p[i * 4 + 3] >> 24);
					GSVector4i v2 = v0.pu16(v1);

					amin = amin.min_u8(v2);
					amax = amax.max_u8(v2);
				}
			}
			else
			{
				ASSERT(GSLocalMemory::m_psm[m_read.TEX0.PSM].pal == 16);

				GSVector4i v0 = (p[0] >> 24).ps32(p[1] >> 24);
				GSVector4i v1 = (p[2] >> 24).ps32(p[3] >> 24);
				GSVector4i v2 = v0.pu16(v1);

				amin = v2;
				amax = v2;
			}

			amin = amin.min_u8(amin.zwxy());
			amax = amax.max_u8(amax.zwxy());
			amin = amin.min_u8(amin.zwxyl());
			amax = amax.max_u8(amax.zwxyl());
			amin = amin.min_u8(amin.yxwzl());
			amax = amax.max_u8(amax.yxwzl());

			GSVector4i v0 = amin.upl8(amax).u8to16();
			GSVector4i v1 = v0.yxwz();

			m_read.amin = v0.min_i16(v1).extract16<0>();
			m_read.amax = v0.max_i16(v1).extract16<1>();
		}
	}

	amin = m_read.amin;
	amax = m_read.amax;
}

//

void GSClut::WriteCLUT_T32_I8_CSM1(const uint32* RESTRICT src, uint16* RESTRICT clut)
{
	// 4 blocks

	for(int i = 0; i < 64; i += 16)
	{
		WriteCLUT_T32_I4_CSM1(&src[i +   0], &clut[i * 2 +   0]);
		WriteCLUT_T32_I4_CSM1(&src[i +  64], &clut[i * 2 +  16]);
		WriteCLUT_T32_I4_CSM1(&src[i + 128], &clut[i * 2 + 128]);
		WriteCLUT_T32_I4_CSM1(&src[i + 192], &clut[i * 2 + 144]);
	}
}

__forceinline void GSClut::WriteCLUT_T32_I4_CSM1(const uint32* RESTRICT src, uint16* RESTRICT clut)
{
	// 1 block

	#if _M_SSE >= 0x501

	GSVector8i* s = (GSVector8i*)src;
	GSVector8i* d = (GSVector8i*)clut;

	GSVector8i v0 = s[0].acbd();
	GSVector8i v1 = s[1].acbd();

	GSVector8i::sw16(v0, v1);
	GSVector8i::sw16(v0, v1);
	GSVector8i::sw16(v0, v1);

	d[0] = v0;
	d[16] = v1;

	#else

	GSVector4i* s = (GSVector4i*)src;
	GSVector4i* d = (GSVector4i*)clut;

	GSVector4i v0 = s[0];
	GSVector4i v1 = s[1];
	GSVector4i v2 = s[2];
	GSVector4i v3 = s[3];

	GSVector4i::sw16(v0, v1, v2, v3);
	GSVector4i::sw32(v0, v1, v2, v3);
	GSVector4i::sw16(v0, v2, v1, v3);

	d[0] = v0;
	d[1] = v2;
	d[32] = v1;
	d[33] = v3;

	#endif
}

void GSClut::WriteCLUT_T16_I8_CSM1(const uint16* RESTRICT src, uint16* RESTRICT clut)
{
	// 2 blocks

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
}

__forceinline void GSClut::WriteCLUT_T16_I4_CSM1(const uint16* RESTRICT src, uint16* RESTRICT clut)
{
	// 1 block (half)

	for(int i = 0; i < 16; i++)
	{
		clut[i] = src[clutTableT16I4[i]];
	}
}

void GSClut::ReadCLUT_T32_I8(const uint16* RESTRICT clut, uint32* RESTRICT dst)
{
	for(int i = 0; i < 256; i += 16)
	{
		ReadCLUT_T32_I4(&clut[i], &dst[i]);
	}
}

__forceinline void GSClut::ReadCLUT_T32_I4(const uint16* RESTRICT clut, uint32* RESTRICT dst)
{
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
}

__forceinline void GSClut::ReadCLUT_T32_I4(const uint16* RESTRICT clut, uint32* RESTRICT dst32, uint64* RESTRICT dst64)
{
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
}

void GSClut::ReadCLUT_T16_I8(const uint16* RESTRICT clut, uint32* RESTRICT dst)
{
	for(int i = 0; i < 256; i += 16)
	{
		ReadCLUT_T16_I4(&clut[i], &dst[i]);
	}
}

__forceinline void GSClut::ReadCLUT_T16_I4(const uint16* RESTRICT clut, uint32* RESTRICT dst)
{
	GSVector4i* s = (GSVector4i*)clut;
	GSVector4i* d = (GSVector4i*)dst;

	GSVector4i v0 = s[0];
	GSVector4i v1 = s[1];

	d[0] = v0.upl16();
	d[1] = v0.uph16();
	d[2] = v1.upl16();
	d[3] = v1.uph16();
}

__forceinline void GSClut::ReadCLUT_T16_I4(const uint16* RESTRICT clut, uint32* RESTRICT dst32, uint64* RESTRICT dst64)
{
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
}

void GSClut::ExpandCLUT64_T32_I8(const uint32* RESTRICT src, uint64* RESTRICT dst)
{
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

void GSClut::ExpandCLUT64_T16_I8(const uint32* RESTRICT src, uint64* RESTRICT dst)
{
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

static const GSVector4i s_bm(0x00007c00);
static const GSVector4i s_gm(0x000003e0);
static const GSVector4i s_rm(0x0000001f);

void GSClut::Expand16(const uint16* RESTRICT src, uint32* RESTRICT dst, int w, const GIFRegTEXA& TEXA)
{
	ASSERT((w & 7) == 0);

	const GSVector4i rm = s_rm;
	const GSVector4i gm = s_gm;
	const GSVector4i bm = s_bm;

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
			cl = c.upl16(c);
			ch = c.uph16(c);
			d[i * 2 + 0] = ((cl & rm) << 3) | ((cl & gm) << 6) | ((cl & bm) << 9) | TA0.blend8(TA1, cl.sra16(15)).andnot(cl == GSVector4i::zero());
			d[i * 2 + 1] = ((ch & rm) << 3) | ((ch & gm) << 6) | ((ch & bm) << 9) | TA0.blend8(TA1, ch.sra16(15)).andnot(ch == GSVector4i::zero());
		}
	}
}

//

bool GSClut::WriteState::IsDirty(const GIFRegTEX0& TEX0, const GIFRegTEXCLUT& TEXCLUT)
{
	return dirty || !GSVector4i::load<true>(this).eq(GSVector4i::load(&TEX0, &TEXCLUT));
}

bool GSClut::ReadState::IsDirty(const GIFRegTEX0& TEX0)
{
	return dirty || !GSVector4i::load<true>(this).eq(GSVector4i::load(&TEX0, &this->TEXA));
}

bool GSClut::ReadState::IsDirty(const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA)
{
	return dirty || !GSVector4i::load<true>(this).eq(GSVector4i::load(&TEX0, &TEXA));
}
