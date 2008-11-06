/* 
 *	Copyright (C) 2003-2005 Gabest
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
 *	Special Notes: 
 *
 *	Based on Page.c from GSSoft
 *	Copyright (C) 2002-2004 GSsoft Team
 *
 */
 
#include "StdAfx.h"
#include "GSLocalMemory.h"
#include "x86.h"

//

static class cdscd
{
	CString toBin(UINT64 n, int bits)
	{
		CString str;
		while(bits-- > 0) str += n&(1i64<<bits) ? '1' : '0';
		return str;
	}
public:

	cdscd()
	{
		return;
/*
		union
		{
			struct {DWORD mant:23; DWORD exp:8; DWORD sign:1;} fi;
			DWORD i;
			float f;
		} nf, nf2;

		union
		{
			struct {UINT64 mant:52; UINT64 exp:11; UINT64 sign:1;} di;
			UINT64 i;
			double d;
		} nd;

		nd.di.mant = 0;
		nd.di.exp = 1023+128;
		nd.di.sign = 0;

		nd.d = pow(2.0, 128);

		TRACE(_T("%016I64x (%I64x:%I64x:%I64x), %f\n"), nd.i, nd.di.sign, nd.di.exp, nd.di.mant, nd.d);
*/
/*
		unsigned int control_word = 0;
		int err;
		
//		nd.d = 0;
//		TRACE(_T("%016I64x (%I64x:%I64x:%I64x), %f\n"), nd.i, nd.di.sign, nd.di.exp, nd.di.mant, nd.d);

//		nf.f = nd.d;
//		TRACE(_T("%08x (%x:%x:%x), %f\n"), nf.i, nf.fi.sign, nf.fi.exp, nf.fi.mant, nf.f);

		nf2.fi.sign = 0;
		nf2.fi.exp = 0x7f;
		nf2.fi.mant = 1;

		//err = _controlfp_s(&control_word, _RC_CHOP, _MCW_RC);

		nf.f = 0;

		for(int i = 0; i < 20; i++)
		{
			nd.d = (double)nf.f + (double)nf2.f;
			TRACE(_T("%016I64x (%I64x:%03I64x:%s), %f\n"), nd.i, nd.di.sign, nd.di.exp, toBin(nd.di.mant, 52), nd.d);
			*((UINT64*)&nd.d) &= ~(1ui64<<(52-24));
			TRACE(_T("%016I64x (%I64x:%03I64x:%s), %f\n"), nd.i, nd.di.sign, nd.di.exp, toBin(nd.di.mant, 52), nd.d);
			nf.f = (float)nd.d;
			TRACE(_T("%08x         (%x:%02x :%s                             ), %f\n"), nf.i, nf.fi.sign, nf.fi.exp, toBin(nf.fi.mant, 23), nf.f);
		}
*/
/*
		GSLocalMemory lm;

		for(int i = 0; i < 1024*1024; i++)
			((DWORD*)lm.GetVM())[i] = rand()*0x12345678;


		GIFRegTEX0 TEX0;
		TEX0.TBP0 = 0;
		TEX0.TBW = 16;
		GIFRegTEXA TEXA;
		TEXA.AEM = 0; // 1

		__declspec(align(32)) static DWORD dst[1024*1024];

		for(int j = 0; j < 10; j++)
		{
			clock_t start = clock();

			for(int i = 0; i < 1000; i++)
			{
				lm.unSwizzleTexture32(CRect(0,0,1024,1024), (BYTE*)dst, 1024*4, TEX0, TEXA);
			}

			clock_t diff = clock() - start;

			CString str;
			str.Format(_T("%d"), diff);
			AfxMessageBox(str);
		}
*/
/*
		__declspec(align(16)) static vertex_t v[100000];
		for(int i = 0; i < countof(v); i++)
		{
			v[i].u = 1.0f*rand() - RAND_MAX/2;
			v[i].v = 1.0f*rand() - RAND_MAX/2;
		}

		for(int j = 0; j < 10; j++)
		{
			clock_t start = clock();

			uvmm_t uvmm;
			for(int i = 0; i < 1000; i++)
				UVMinMax_sse2(countof(v), v, &uvmm);

			clock_t diff = clock() - start;

			CString str;
			str.Format(_T("%d, u %.2f %.2f, v %.2f %.2f"), diff, uvmm.umin, uvmm.umax, uvmm.vmin, uvmm.vmax);
			AfxMessageBox(str);
		}
*/
/*
		GIFRegTEX0 TEX0;
		GIFRegTEXCLUT TEXCLUT;
		TEX0.PSM = PSM_PSMT8;
		TEX0.CLD = 1;
		TEX0.CSA = 0;
		TEX0.CPSM = PSM_PSMCT32; // PSM_PSMCT16S
		TEX0.CBP = 0;
		TEX0.CSM = 0; // 0
		TEXCLUT.CBW = 1;
		TEXCLUT.COU = 0;
		TEXCLUT.COV = 0;

		for(int j = 0; j < 10; j++)
		{
			clock_t start = clock();

			for(int i = 0; i < 10000000; i++)
				lm.WriteCLUT(TEX0, TEXCLUT);

			clock_t diff = clock() - start;

			CString str;
			str.Format(_T("%d"), diff);
			AfxMessageBox(str);
		}
*/
/*
		GIFRegTEXA TEXA;
		TEXA.AEM = 0; // 1
		TEXA.TA0 = 0x7f;
		TEXA.TA1 = 0x80;

		int w = 1024*1024;

		WORD* src = (WORD*)lm.GetVM();
		DWORD* dst = (DWORD*)_aligned_malloc(w*4, 16);

		for(int j = 0; j < 10; j++)
		{
			clock_t start = clock();

			for(int i = 0; i < 1000; i++)
			{
				//Expand16_sse2(src, dst, w, &TEXA);
				Expand16_c(src, dst, w, &TEXA);
			}

			clock_t diff = clock() - start;

			CString str;
			str.Format(_T("%d"), diff);
			AfxMessageBox(str);
		}

		_aligned_free(dst);
*/
	}
} sddscsd;

//

#define ASSERT_BLOCK(r, w, h) \
	ASSERT((r).Width() >= w && (r).Height() >= h && !((r).left&(w-1)) && !((r).top&(h-1)) && !((r).right&(w-1)) && !((r).bottom&(h-1))); \

#if defined(_M_AMD64) || _M_IX86_FP >= 2
#define BLOCK_PREFETCH(mem) \
	_mm_prefetch(&mem[16*0], _MM_HINT_T0); \
	_mm_prefetch(&mem[16*2], _MM_HINT_T0); \
	_mm_prefetch(&mem[16*4], _MM_HINT_T0); \
	_mm_prefetch(&mem[16*6], _MM_HINT_T0); \
	_mm_prefetch(&mem[16*8], _MM_HINT_T0); \
	_mm_prefetch(&mem[16*10], _MM_HINT_T0); \
	_mm_prefetch(&mem[16*12], _MM_HINT_T0); \
	_mm_prefetch(&mem[16*14], _MM_HINT_T0); \

#define BLOCK_PREFETCH_32(x, y, w) {const char* next = (const char*)&m_vm32[blockAddress32(x + (w), y, TEX0.TBP0, TEX0.TBW)]; BLOCK_PREFETCH(next);}
#define BLOCK_PREFETCH_16(x, y, w) {const char* next = (const char*)&m_vm16[blockAddress16(x + (w), y, TEX0.TBP0, TEX0.TBW)]; BLOCK_PREFETCH(next);}
#define BLOCK_PREFETCH_16S(x, y, w) {const char* next = (const char*)&m_vm16[blockAddress16S(x + (w), y, TEX0.TBP0, TEX0.TBW)]; BLOCK_PREFETCH(next);}
#define BLOCK_PREFETCH_8(x, y, w) {const char* next = (const char*)&m_vm8[blockAddress8(x + (w), y, TEX0.TBP0, TEX0.TBW)]; BLOCK_PREFETCH(next);}
#define BLOCK_PREFETCH_4(x, y, w) {const char* next = (const char*)&m_vm8[blockAddress4(x + (w), y, TEX0.TBP0, TEX0.TBW)>>1]; BLOCK_PREFETCH(next);}
#else 
#define BLOCK_PREFETCH_32(x, y, w)
#define BLOCK_PREFETCH_16(x, y, w)
#define BLOCK_PREFETCH_16S(x, y, w)
#define BLOCK_PREFETCH_8(x, y, w)
#define BLOCK_PREFETCH_4(x, y, w)
#endif

#define FOREACH_BLOCK_START(r, w, h, t) \
	for(int y = (r).top; y < (r).bottom; y += (h)) \
	{ 	ASSERT_BLOCK(r, w, h); \
		BYTE* ptr = dst + (y-(r).top)*dstpitch; \
		for(int x = (r).left; x < (r).right; x += (w)) \
		{ \
			BLOCK_PREFETCH_##t##(x + (w), y, w) \

#define FOREACH_BLOCK_END }}

//

DWORD GSLocalMemory::pageOffset32[32][32][64];
DWORD GSLocalMemory::pageOffset32Z[32][32][64];
DWORD GSLocalMemory::pageOffset16[32][64][64];
DWORD GSLocalMemory::pageOffset16S[32][64][64];
DWORD GSLocalMemory::pageOffset16Z[32][64][64];
DWORD GSLocalMemory::pageOffset16SZ[32][64][64];
DWORD GSLocalMemory::pageOffset8[32][64][128];
DWORD GSLocalMemory::pageOffset4[32][128][128];

int GSLocalMemory::rowOffset32[2048];
int GSLocalMemory::rowOffset32Z[2048];
int GSLocalMemory::rowOffset16[2048];
int GSLocalMemory::rowOffset16S[2048];
int GSLocalMemory::rowOffset16Z[2048];
int GSLocalMemory::rowOffset16SZ[2048];
int GSLocalMemory::rowOffset8[2][2048];
int GSLocalMemory::rowOffset4[2][2048];

//

DWORD GSLocalMemory::m_xtbl[1024];
DWORD GSLocalMemory::m_ytbl[1024]; 

//

GSLocalMemory::psmtbl_t GSLocalMemory::m_psmtbl[64];

//

GSLocalMemory::GSLocalMemory()
	: m_fCLUTMayBeDirty(true)
{
	int len = 1024*1024*4*2; // *2 for safety...

	m_vm8 = (BYTE*)_aligned_malloc(len, 16);
	memset(m_vm8, 0, len);

	m_pCLUT = (WORD*)_aligned_malloc(256*2*sizeof(WORD)*2, 16);
	m_pCLUT32 = (DWORD*)_aligned_malloc(256*sizeof(DWORD), 16);
	m_pCLUT64 = (UINT64*)_aligned_malloc(256*sizeof(UINT64), 16);

	for(int bp = 0; bp < 32; bp++)
	{
		for(int y = 0; y < 32; y++) for(int x = 0; x < 64; x++)
		{
			pageOffset32[bp][y][x] = pixelAddressOrg32(x, y, bp, 0);
			pageOffset32Z[bp][y][x] = pixelAddressOrg32Z(x, y, bp, 0);
		}

		for(int y = 0; y < 64; y++) for(int x = 0; x < 64; x++) 
		{
			pageOffset16[bp][y][x] = pixelAddressOrg16(x, y, bp, 0);
			pageOffset16S[bp][y][x] = pixelAddressOrg16S(x, y, bp, 0);
			pageOffset16Z[bp][y][x] = pixelAddressOrg16Z(x, y, bp, 0);
			pageOffset16SZ[bp][y][x] = pixelAddressOrg16SZ(x, y, bp, 0);
		}

		for(int y = 0; y < 64; y++) for(int x = 0; x < 128; x++)
		{
			pageOffset8[bp][y][x] = pixelAddressOrg8(x, y, bp, 0);
		}

		for(int y = 0; y < 128; y++) for(int x = 0; x < 128; x++)
		{
			pageOffset4[bp][y][x] = pixelAddressOrg4(x, y, bp, 0);
		}
	}

	{
		for(int x = 0; x < countof(rowOffset32); x++)
			rowOffset32[x] = (int)pixelAddress32(x, 0, 0, 32) - (int)pixelAddress32(0, 0, 0, 32);

		for(int x = 0; x < countof(rowOffset32Z); x++)
			rowOffset32Z[x] = (int)pixelAddress32Z(x, 0, 0, 32) - (int)pixelAddress32Z(0, 0, 0, 32);

		for(int x = 0; x < countof(rowOffset16); x++)
			rowOffset16[x] = (int)pixelAddress16(x, 0, 0, 32) - (int)pixelAddress16(0, 0, 0, 32);

		for(int x = 0; x < countof(rowOffset16S); x++)
			rowOffset16S[x] = (int)pixelAddress16S(x, 0, 0, 32) - (int)pixelAddress16S(0, 0, 0, 32);

		for(int x = 0; x < countof(rowOffset16Z); x++)
			rowOffset16Z[x] = (int)pixelAddress16Z(x, 0, 0, 32) - (int)pixelAddress16Z(0, 0, 0, 32);

		for(int x = 0; x < countof(rowOffset16SZ); x++)
			rowOffset16SZ[x] = (int)pixelAddress16SZ(x, 0, 0, 32) - (int)pixelAddress16SZ(0, 0, 0, 32);

		for(int x = 0; x < countof(rowOffset8[0]); x++)
			rowOffset8[0][x] = (int)pixelAddress8(x, 0, 0, 32) - (int)pixelAddress8(0, 0, 0, 32),
			rowOffset8[1][x] = (int)pixelAddress8(x, 2, 0, 32) - (int)pixelAddress8(0, 2, 0, 32);

		for(int x = 0; x < countof(rowOffset4[0]); x++)
			rowOffset4[0][x] = (int)pixelAddress4(x, 0, 0, 32) - (int)pixelAddress4(0, 0, 0, 32),
			rowOffset4[1][x] = (int)pixelAddress4(x, 2, 0, 32) - (int)pixelAddress4(0, 2, 0, 32);
	}

	for(int i = 0; i < countof(m_psmtbl); i++)
	{
		m_psmtbl[i].pa = &GSLocalMemory::pixelAddress32;
		m_psmtbl[i].ba = &GSLocalMemory::blockAddress32;
		m_psmtbl[i].pga = &GSLocalMemory::pageAddress32;
		m_psmtbl[i].rp = &GSLocalMemory::readPixel32;
		m_psmtbl[i].rpa = &GSLocalMemory::readPixel32;
		m_psmtbl[i].wp = &GSLocalMemory::writePixel32;
		m_psmtbl[i].wpa = &GSLocalMemory::writePixel32;
		m_psmtbl[i].rt = m_psmtbl[i].rtNP = m_psmtbl[i].rtP = &GSLocalMemory::readTexel32;
		m_psmtbl[i].rta = &GSLocalMemory::readTexel32;
		m_psmtbl[i].wfa = &GSLocalMemory::writePixel32;
		m_psmtbl[i].st = &GSLocalMemory::SwizzleTexture32;
		m_psmtbl[i].ust = m_psmtbl[i].ustP = m_psmtbl[i].ustNP = &GSLocalMemory::unSwizzleTexture32;
		m_psmtbl[i].bpp = m_psmtbl[i].trbpp = 32;
		m_psmtbl[i].pal = 0;
		m_psmtbl[i].bs = CSize(8, 8);
		for(int j = 0; j < 8; j++) m_psmtbl[i].rowOffset[j] = rowOffset32;
	}

	m_psmtbl[PSM_PSMCT16].pa = &GSLocalMemory::pixelAddress16;
	m_psmtbl[PSM_PSMCT16S].pa = &GSLocalMemory::pixelAddress16S;
	m_psmtbl[PSM_PSMT8].pa = &GSLocalMemory::pixelAddress8;
	m_psmtbl[PSM_PSMT4].pa = &GSLocalMemory::pixelAddress4;
	m_psmtbl[PSM_PSMZ32].pa = &GSLocalMemory::pixelAddress32Z;
	m_psmtbl[PSM_PSMZ24].pa = &GSLocalMemory::pixelAddress32Z;
	m_psmtbl[PSM_PSMZ16].pa = &GSLocalMemory::pixelAddress16Z;
	m_psmtbl[PSM_PSMZ16S].pa = &GSLocalMemory::pixelAddress16SZ;

	m_psmtbl[PSM_PSMCT16].ba = &GSLocalMemory::blockAddress16;
	m_psmtbl[PSM_PSMCT16S].ba = &GSLocalMemory::blockAddress16S;
	m_psmtbl[PSM_PSMT8].ba = &GSLocalMemory::blockAddress8;
	m_psmtbl[PSM_PSMT4].ba = &GSLocalMemory::blockAddress4;
	m_psmtbl[PSM_PSMZ32].ba = &GSLocalMemory::blockAddress32Z;
	m_psmtbl[PSM_PSMZ24].ba = &GSLocalMemory::blockAddress32Z;
	m_psmtbl[PSM_PSMZ16].ba = &GSLocalMemory::blockAddress16Z;
	m_psmtbl[PSM_PSMZ16S].ba = &GSLocalMemory::blockAddress16SZ;

	m_psmtbl[PSM_PSMCT16].pga = &GSLocalMemory::pageAddress16;
	m_psmtbl[PSM_PSMCT16S].pga = &GSLocalMemory::pageAddress16;
	m_psmtbl[PSM_PSMZ16].pga = &GSLocalMemory::pageAddress16;
	m_psmtbl[PSM_PSMZ16S].pga = &GSLocalMemory::pageAddress16;
	m_psmtbl[PSM_PSMT8].pga = &GSLocalMemory::pageAddress8;
	m_psmtbl[PSM_PSMT4].pga = &GSLocalMemory::pageAddress4;

	m_psmtbl[PSM_PSMCT24].rp = &GSLocalMemory::readPixel24;
	m_psmtbl[PSM_PSMCT16].rp = &GSLocalMemory::readPixel16;
	m_psmtbl[PSM_PSMCT16S].rp = &GSLocalMemory::readPixel16S;
	m_psmtbl[PSM_PSMT8].rp = &GSLocalMemory::readPixel8;
	m_psmtbl[PSM_PSMT4].rp = &GSLocalMemory::readPixel4;
	m_psmtbl[PSM_PSMT8H].rp = &GSLocalMemory::readPixel8H;
	m_psmtbl[PSM_PSMT4HL].rp = &GSLocalMemory::readPixel4HL;
	m_psmtbl[PSM_PSMT4HH].rp = &GSLocalMemory::readPixel4HH;
	m_psmtbl[PSM_PSMZ32].rp = &GSLocalMemory::readPixel32Z;
	m_psmtbl[PSM_PSMZ24].rp = &GSLocalMemory::readPixel24Z;
	m_psmtbl[PSM_PSMZ16].rp = &GSLocalMemory::readPixel16Z;
	m_psmtbl[PSM_PSMZ16S].rp = &GSLocalMemory::readPixel16SZ;

	m_psmtbl[PSM_PSMCT24].rpa = &GSLocalMemory::readPixel24;
	m_psmtbl[PSM_PSMCT16].rpa = &GSLocalMemory::readPixel16;
	m_psmtbl[PSM_PSMCT16S].rpa = &GSLocalMemory::readPixel16S;
	m_psmtbl[PSM_PSMT8].rpa = &GSLocalMemory::readPixel8;
	m_psmtbl[PSM_PSMT4].rpa = &GSLocalMemory::readPixel4;
	m_psmtbl[PSM_PSMT8H].rpa = &GSLocalMemory::readPixel8H;
	m_psmtbl[PSM_PSMT4HL].rpa = &GSLocalMemory::readPixel4HL;
	m_psmtbl[PSM_PSMT4HH].rpa = &GSLocalMemory::readPixel4HH;
	m_psmtbl[PSM_PSMZ32].rpa = &GSLocalMemory::readPixel32Z;
	m_psmtbl[PSM_PSMZ24].rpa = &GSLocalMemory::readPixel24Z;
	m_psmtbl[PSM_PSMZ16].rpa = &GSLocalMemory::readPixel16Z;
	m_psmtbl[PSM_PSMZ16S].rpa = &GSLocalMemory::readPixel16SZ;

	m_psmtbl[PSM_PSMCT32].wp = &GSLocalMemory::writePixel32;
	m_psmtbl[PSM_PSMCT24].wp = &GSLocalMemory::writePixel24;
	m_psmtbl[PSM_PSMCT16].wp = &GSLocalMemory::writePixel16;
	m_psmtbl[PSM_PSMCT16S].wp = &GSLocalMemory::writePixel16S;
	m_psmtbl[PSM_PSMT8].wp = &GSLocalMemory::writePixel8;
	m_psmtbl[PSM_PSMT4].wp = &GSLocalMemory::writePixel4;
	m_psmtbl[PSM_PSMT8H].wp = &GSLocalMemory::writePixel8H;
	m_psmtbl[PSM_PSMT4HL].wp = &GSLocalMemory::writePixel4HL;
	m_psmtbl[PSM_PSMT4HH].wp = &GSLocalMemory::writePixel4HH;
	m_psmtbl[PSM_PSMZ32].wp = &GSLocalMemory::writePixel32Z;
	m_psmtbl[PSM_PSMZ24].wp = &GSLocalMemory::writePixel24Z;
	m_psmtbl[PSM_PSMZ16].wp = &GSLocalMemory::writePixel16Z;
	m_psmtbl[PSM_PSMZ16S].wp = &GSLocalMemory::writePixel16SZ;

	m_psmtbl[PSM_PSMCT32].wpa = &GSLocalMemory::writePixel32;
	m_psmtbl[PSM_PSMCT24].wpa = &GSLocalMemory::writePixel24;
	m_psmtbl[PSM_PSMCT16].wpa = &GSLocalMemory::writePixel16;
	m_psmtbl[PSM_PSMCT16S].wpa = &GSLocalMemory::writePixel16S;
	m_psmtbl[PSM_PSMT8].wpa = &GSLocalMemory::writePixel8;
	m_psmtbl[PSM_PSMT4].wpa = &GSLocalMemory::writePixel4;
	m_psmtbl[PSM_PSMT8H].wpa = &GSLocalMemory::writePixel8H;
	m_psmtbl[PSM_PSMT4HL].wpa = &GSLocalMemory::writePixel4HL;
	m_psmtbl[PSM_PSMT4HH].wpa = &GSLocalMemory::writePixel4HH;
	m_psmtbl[PSM_PSMZ32].wpa = &GSLocalMemory::writePixel32Z;
	m_psmtbl[PSM_PSMZ24].wpa = &GSLocalMemory::writePixel24Z;
	m_psmtbl[PSM_PSMZ16].wpa = &GSLocalMemory::writePixel16Z;
	m_psmtbl[PSM_PSMZ16S].wpa = &GSLocalMemory::writePixel16SZ;

	m_psmtbl[PSM_PSMCT24].rt = &GSLocalMemory::readTexel24;
	m_psmtbl[PSM_PSMCT16].rt = &GSLocalMemory::readTexel16;
	m_psmtbl[PSM_PSMCT16S].rt = &GSLocalMemory::readTexel16S;
	m_psmtbl[PSM_PSMT8].rt = &GSLocalMemory::readTexel8;
	m_psmtbl[PSM_PSMT4].rt = &GSLocalMemory::readTexel4;
	m_psmtbl[PSM_PSMT8H].rt = &GSLocalMemory::readTexel8H;
	m_psmtbl[PSM_PSMT4HL].rt = &GSLocalMemory::readTexel4HL;
	m_psmtbl[PSM_PSMT4HH].rt = &GSLocalMemory::readTexel4HH;

	m_psmtbl[PSM_PSMCT24].rta = &GSLocalMemory::readTexel24;
	m_psmtbl[PSM_PSMCT16].rta = &GSLocalMemory::readTexel16;
	m_psmtbl[PSM_PSMCT16S].rta = &GSLocalMemory::readTexel16S;
	m_psmtbl[PSM_PSMT8].rta = &GSLocalMemory::readTexel8;
	m_psmtbl[PSM_PSMT4].rta = &GSLocalMemory::readTexel4;
	m_psmtbl[PSM_PSMT8H].rta = &GSLocalMemory::readTexel8H;
	m_psmtbl[PSM_PSMT4HL].rta = &GSLocalMemory::readTexel4HL;
	m_psmtbl[PSM_PSMT4HH].rta = &GSLocalMemory::readTexel4HH;

	m_psmtbl[PSM_PSMCT24].wfa = &GSLocalMemory::writePixel24;
	m_psmtbl[PSM_PSMCT16].wfa = &GSLocalMemory::writeFrame16;
	m_psmtbl[PSM_PSMCT16S].wfa = &GSLocalMemory::writeFrame16S;

	m_psmtbl[PSM_PSMCT16].rtP = &GSLocalMemory::readTexel16P;
	m_psmtbl[PSM_PSMCT16S].rtP = &GSLocalMemory::readTexel16SP;
	m_psmtbl[PSM_PSMT8].rtP = &GSLocalMemory::readTexel8P;
	m_psmtbl[PSM_PSMT4].rtP = &GSLocalMemory::readTexel4P;
	m_psmtbl[PSM_PSMT8H].rtP = &GSLocalMemory::readTexel8HP;
	m_psmtbl[PSM_PSMT4HL].rtP = &GSLocalMemory::readTexel4HLP;
	m_psmtbl[PSM_PSMT4HH].rtP = &GSLocalMemory::readTexel4HHP;

	m_psmtbl[PSM_PSMCT16].rtNP = &GSLocalMemory::readTexel16P;
	m_psmtbl[PSM_PSMCT16S].rtNP = &GSLocalMemory::readTexel16SP;
	m_psmtbl[PSM_PSMT8].rtNP = &GSLocalMemory::readTexel8;
	m_psmtbl[PSM_PSMT4].rtNP = &GSLocalMemory::readTexel4;
	m_psmtbl[PSM_PSMT8H].rtNP = &GSLocalMemory::readTexel8H;
	m_psmtbl[PSM_PSMT4HL].rtNP = &GSLocalMemory::readTexel4HL;
	m_psmtbl[PSM_PSMT4HH].rtNP = &GSLocalMemory::readTexel4HH;

	m_psmtbl[PSM_PSMCT24].st = &GSLocalMemory::SwizzleTexture24;
	m_psmtbl[PSM_PSMCT16].st = &GSLocalMemory::SwizzleTexture16;
	m_psmtbl[PSM_PSMCT16S].st = &GSLocalMemory::SwizzleTexture16S;
	m_psmtbl[PSM_PSMT8].st = &GSLocalMemory::SwizzleTexture8;
	m_psmtbl[PSM_PSMT4].st = &GSLocalMemory::SwizzleTexture4;
	m_psmtbl[PSM_PSMT8H].st = &GSLocalMemory::SwizzleTexture8H;
	m_psmtbl[PSM_PSMT4HL].st = &GSLocalMemory::SwizzleTexture4HL;
	m_psmtbl[PSM_PSMT4HH].st = &GSLocalMemory::SwizzleTexture4HH;

	m_psmtbl[PSM_PSMCT24].ust = &GSLocalMemory::unSwizzleTexture24;
	m_psmtbl[PSM_PSMCT16].ust = &GSLocalMemory::unSwizzleTexture16;
	m_psmtbl[PSM_PSMCT16S].ust = &GSLocalMemory::unSwizzleTexture16S;
	m_psmtbl[PSM_PSMT8].ust = &GSLocalMemory::unSwizzleTexture8;
	m_psmtbl[PSM_PSMT4].ust = &GSLocalMemory::unSwizzleTexture4;
	m_psmtbl[PSM_PSMT8H].ust = &GSLocalMemory::unSwizzleTexture8H;
	m_psmtbl[PSM_PSMT4HL].ust = &GSLocalMemory::unSwizzleTexture4HL;
	m_psmtbl[PSM_PSMT4HH].ust = &GSLocalMemory::unSwizzleTexture4HH;

	m_psmtbl[PSM_PSMCT16].ustP = &GSLocalMemory::unSwizzleTexture16P;
	m_psmtbl[PSM_PSMCT16S].ustP = &GSLocalMemory::unSwizzleTexture16SP;
	m_psmtbl[PSM_PSMT8].ustP = &GSLocalMemory::unSwizzleTexture8P;
	m_psmtbl[PSM_PSMT4].ustP = &GSLocalMemory::unSwizzleTexture4P;
	m_psmtbl[PSM_PSMT8H].ustP = &GSLocalMemory::unSwizzleTexture8HP;
	m_psmtbl[PSM_PSMT4HL].ustP = &GSLocalMemory::unSwizzleTexture4HLP;
	m_psmtbl[PSM_PSMT4HH].ustP = &GSLocalMemory::unSwizzleTexture4HHP;

	m_psmtbl[PSM_PSMCT16].ustNP = &GSLocalMemory::unSwizzleTexture16P;
	m_psmtbl[PSM_PSMCT16S].ustNP = &GSLocalMemory::unSwizzleTexture16SP;
	m_psmtbl[PSM_PSMT8].ustNP = &GSLocalMemory::unSwizzleTexture8NP;
	m_psmtbl[PSM_PSMT4].ustNP = &GSLocalMemory::unSwizzleTexture4NP;
	m_psmtbl[PSM_PSMT8H].ustNP = &GSLocalMemory::unSwizzleTexture8HNP;
	m_psmtbl[PSM_PSMT4HL].ustNP = &GSLocalMemory::unSwizzleTexture4HLNP;
	m_psmtbl[PSM_PSMT4HH].ustNP = &GSLocalMemory::unSwizzleTexture4HHNP;

	m_psmtbl[PSM_PSMT8].pal = m_psmtbl[PSM_PSMT8H].pal = 256;
	m_psmtbl[PSM_PSMT4].pal = m_psmtbl[PSM_PSMT4HL].pal = m_psmtbl[PSM_PSMT4HH].pal = 16;

	m_psmtbl[PSM_PSMCT16].bpp = m_psmtbl[PSM_PSMCT16S].bpp = 16;
	m_psmtbl[PSM_PSMT8].bpp = 8;
	m_psmtbl[PSM_PSMT4].bpp = 4;
	m_psmtbl[PSM_PSMZ16].bpp = m_psmtbl[PSM_PSMZ16S].bpp = 16;

	m_psmtbl[PSM_PSMCT24].trbpp = 24;
	m_psmtbl[PSM_PSMCT16].trbpp = m_psmtbl[PSM_PSMCT16S].trbpp = 16;
	m_psmtbl[PSM_PSMT8].trbpp = m_psmtbl[PSM_PSMT8H].trbpp = 8;
	m_psmtbl[PSM_PSMT4].trbpp = m_psmtbl[PSM_PSMT4HL].trbpp = m_psmtbl[PSM_PSMT4HH].trbpp = 4;
	m_psmtbl[PSM_PSMZ24].trbpp = 24;
	m_psmtbl[PSM_PSMZ16].trbpp = m_psmtbl[PSM_PSMZ16S].trbpp = 16;

	m_psmtbl[PSM_PSMCT16].bs = m_psmtbl[PSM_PSMCT16S].bs = CSize(16, 8);
	m_psmtbl[PSM_PSMT8].bs = CSize(16, 16);
	m_psmtbl[PSM_PSMT4].bs = CSize(32, 32);
	m_psmtbl[PSM_PSMZ16].bs = m_psmtbl[PSM_PSMZ16S].bs = CSize(16, 8);

	for(int i = 0; i < 8; i++) m_psmtbl[PSM_PSMCT16].rowOffset[i] = rowOffset16;
	for(int i = 0; i < 8; i++) m_psmtbl[PSM_PSMCT16S].rowOffset[i] = rowOffset16S;
	for(int i = 0; i < 8; i++) m_psmtbl[PSM_PSMT8].rowOffset[i] = rowOffset8[((i+2)>>2)&1];
	for(int i = 0; i < 8; i++) m_psmtbl[PSM_PSMT4].rowOffset[i] = rowOffset4[((i+2)>>2)&1];
	for(int i = 0; i < 8; i++) m_psmtbl[PSM_PSMZ32].rowOffset[i] = rowOffset32Z;
	for(int i = 0; i < 8; i++) m_psmtbl[PSM_PSMZ24].rowOffset[i] = rowOffset32Z;
	for(int i = 0; i < 8; i++) m_psmtbl[PSM_PSMZ16].rowOffset[i] = rowOffset16Z;
	for(int i = 0; i < 8; i++) m_psmtbl[PSM_PSMZ16S].rowOffset[i] = rowOffset16SZ;


#ifdef _OPENMP
	// need real cpus, with HT it is only going to be slower now
	omp_set_num_threads(omp_get_num_procs());
#endif

/*	
//	return; 
//	omp_set_num_threads(2);
//
	{
		int tmpsize = 1024*1024*2;
		BYTE* tmp1 = new BYTE[tmpsize];
		BYTE* tmp2 = new BYTE[tmpsize];

		for(int i = 0; i < 1024*1024; i++)
			((DWORD*)GetVM())[i] = rand()*0x12345678;

		GIFRegTEX0 TEX0;
		TEX0.TBP0 = 0;
		TEX0.TBW = 16;
		GIFRegTEXA TEXA;
		TEXA.AEM = 0; // 1

		__declspec(align(32)) static DWORD dst[1024*1024];

		int nthreads[] = {1,2,4,8};
		for(int j = 0; j < countof(nthreads); j++)
		{
			omp_set_num_threads(nthreads[j]);

			clock_t start = clock();

			for(int i = 0; i < 1000; i++)
			{
				unSwizzleTexture32(CRect(0,0,1024,1024), (BYTE*)dst, 1024*4, TEX0, TEXA);
				// for(int k = 0; k < tmpsize; k++) tmp2[k] = tmp1[k];
			}

			clock_t diff = clock() - start;

			CString str;
			str.Format(_T("%d threads: %d ms"), nthreads[j], diff);
			AfxMessageBox(str);
		}
			
		omp_set_num_threads(2);

		delete [] tmp1;
		delete [] tmp2;
	}
*/
}

GSLocalMemory::~GSLocalMemory()
{
	_aligned_free(m_vm8);
	_aligned_free(m_pCLUT);
	_aligned_free(m_pCLUT32);
	_aligned_free(m_pCLUT64);	
}

////////////////////

void GSLocalMemory::RoundDown(CSize& s, CSize bs)
{
	s.cx &= ~(bs.cx-1);
	s.cy &= ~(bs.cy-1);
}

void GSLocalMemory::RoundUp(CSize& s, CSize bs)
{
	s.cx = (s.cx + (bs.cx-1)) & ~(bs.cx-1);
	s.cy = (s.cy + (bs.cy-1)) & ~(bs.cy-1);
}

////////////////////

DWORD __fastcall GSLocalMemory::pageAddress32(int x, int y, DWORD bp, DWORD bw)
{
	return ((bp >> 5) + (y >> 5) * bw + (x >> 6)) << 11; 
}

DWORD __fastcall GSLocalMemory::pageAddress16(int x, int y, DWORD bp, DWORD bw)
{
	return ((bp >> 5) + (y >> 6) * bw + (x >> 6)) << 12;
}

DWORD __fastcall GSLocalMemory::pageAddress8(int x, int y, DWORD bp, DWORD bw)
{
	return ((bp >> 5) + (y >> 6) * ((bw+1)>>1) + (x >> 7)) << 13; 
}

DWORD __fastcall GSLocalMemory::pageAddress4(int x, int y, DWORD bp, DWORD bw)
{
	return ((bp >> 5) + (y >> 7) * ((bw+1)>>1) + (x >> 7)) << 14;
}

////////////////////

DWORD __fastcall GSLocalMemory::blockAddress32(int x, int y, DWORD bp, DWORD bw)
{
	DWORD page = bp + (y & ~0x1f) * bw + ((x >> 1) & ~0x1f);
	DWORD block = blockTable32[(y >> 3) & 3][(x >> 3) & 7];
	return (page + block) << 6;
}

DWORD __fastcall GSLocalMemory::blockAddress16(int x, int y, DWORD bp, DWORD bw)
{
	DWORD page = bp + ((y >> 1) & ~0x1f) * bw + ((x >> 1) & ~0x1f); 
	DWORD block = blockTable16[(y >> 3) & 7][(x >> 4) & 3];
	return (page + block) << 7;
}

DWORD __fastcall GSLocalMemory::blockAddress16S(int x, int y, DWORD bp, DWORD bw)
{
	DWORD page = bp + ((y >> 1) & ~0x1f) * bw + ((x >> 1) & ~0x1f); 
	DWORD block = blockTable16S[(y >> 3) & 7][(x >> 4) & 3];
	return (page + block) << 7;
}

DWORD __fastcall GSLocalMemory::blockAddress8(int x, int y, DWORD bp, DWORD bw)
{
	DWORD page = bp + ((y >> 1) & ~0x1f) * ((bw+1)>>1) + ((x >> 2) & ~0x1f); 
	DWORD block = blockTable8[(y >> 4) & 3][(x >> 4) & 7];
	return (page + block) << 8;
}

DWORD __fastcall GSLocalMemory::blockAddress4(int x, int y, DWORD bp, DWORD bw)
{
	DWORD page = bp + ((y >> 2) & ~0x1f) * ((bw+1)>>1) + ((x >> 2) & ~0x1f); 
	DWORD block = blockTable4[(y >> 4) & 7][(x >> 5) & 3];
	return (page + block) << 9;
}

DWORD __fastcall GSLocalMemory::blockAddress32Z(int x, int y, DWORD bp, DWORD bw)
{
	DWORD page = bp + (y & ~0x1f) * bw + ((x >> 1) & ~0x1f); 
	DWORD block = blockTable32Z[(y >> 3) & 3][(x >> 3) & 7];
	return (page + block) << 6;
}

DWORD __fastcall GSLocalMemory::blockAddress16Z(int x, int y, DWORD bp, DWORD bw)
{
	DWORD page = bp + ((y >> 1) & ~0x1f) * bw + ((x >> 1) & ~0x1f); 
	DWORD block = blockTable16Z[(y >> 3) & 7][(x >> 4) & 3];
	return (page + block) << 7;
}

DWORD __fastcall GSLocalMemory::blockAddress16SZ(int x, int y, DWORD bp, DWORD bw)
{
	DWORD page = bp + ((y >> 1) & ~0x1f) * bw + ((x >> 1) & ~0x1f); 
	DWORD block = blockTable16SZ[(y >> 3) & 7][(x >> 4) & 3];
	return (page + block) << 7;
}

////////////////////

DWORD __fastcall GSLocalMemory::pixelAddressOrg32(int x, int y, DWORD bp, DWORD bw)
{
//	DWORD page = (bp >> 5) + (y >> 5) * bw + (x >> 6); 
//	DWORD block = (bp & 0x1f) + blockTable32[(y >> 3) & 3][(x >> 3) & 7];
//	DWORD word = (((page << 5) + block) << 6) + columnTable32[y & 7][x & 7];
	DWORD page = bp + (y & ~0x1f) * bw + ((x >> 1) & ~0x1f);
	DWORD block = blockTable32[(y >> 3) & 3][(x >> 3) & 7];
	DWORD word = ((page + block) << 6) + columnTable32[y & 7][x & 7];
	ASSERT(word < 1024*1024);
	return word;
}

DWORD __fastcall GSLocalMemory::pixelAddressOrg16(int x, int y, DWORD bp, DWORD bw)
{
//	DWORD page = (bp >> 5) + (y >> 6) * bw + (x >> 6); 
//	DWORD block = (bp & 0x1f) + blockTable16[(y >> 3) & 7][(x >> 4) & 3];
//	DWORD word = (((page << 5) + block) << 7) + columnTable16[y & 7][x & 15];
	DWORD page = bp + ((y >> 1) & ~0x1f) * bw + ((x >> 1) & ~0x1f); 
	DWORD block = blockTable16[(y >> 3) & 7][(x >> 4) & 3];
	DWORD word = ((page + block) << 7) + columnTable16[y & 7][x & 15];
	ASSERT(word < 1024*1024*2);
	return word;
}

DWORD __fastcall GSLocalMemory::pixelAddressOrg16S(int x, int y, DWORD bp, DWORD bw)
{
//	DWORD page = (bp >> 5) + (y >> 6) * bw + (x >> 6); 
//	DWORD block = (bp & 0x1f) + blockTable16S[(y >> 3) & 7][(x >> 4) & 3];
//	DWORD word = (((page << 5) + block) << 7) + columnTable16[y & 7][x & 15];
	DWORD page = bp + ((y >> 1) & ~0x1f) * bw + ((x >> 1) & ~0x1f); 
	DWORD block = blockTable16S[(y >> 3) & 7][(x >> 4) & 3];
	DWORD word = ((page + block) << 7) + columnTable16[y & 7][x & 15];
	ASSERT(word < 1024*1024*2);
	return word;
}

DWORD __fastcall GSLocalMemory::pixelAddressOrg8(int x, int y, DWORD bp, DWORD bw)
{
//	DWORD page = (bp >> 5) + (y >> 6) * ((bw+1)>>1) + (x >> 7); 
//	DWORD block = (bp & 0x1f) + blockTable8[(y >> 4) & 3][(x >> 4) & 7];
//	DWORD word = (((page << 5) + block) << 8) + columnTable8[y & 15][x & 15];
	DWORD page = bp + ((y >> 1) & ~0x1f) * ((bw+1)>>1) + ((x >> 2) & ~0x1f); 
	DWORD block = blockTable8[(y >> 4) & 3][(x >> 4) & 7];
	DWORD word = ((page + block) << 8) + columnTable8[y & 15][x & 15];
//	ASSERT(word < 1024*1024*4);
	return word;
}

DWORD __fastcall GSLocalMemory::pixelAddressOrg4(int x, int y, DWORD bp, DWORD bw)
{
//	DWORD page = (bp >> 5) + (y >> 7) * ((bw+1)>>1) + (x >> 7);
//	DWORD block = (bp & 0x1f) + blockTable4[(y >> 4) & 7][(x >> 5) & 3];
//	DWORD word = (((page << 5) + block) << 9) + columnTable4[y & 15][x & 31];
	DWORD page = bp + ((y >> 2) & ~0x1f) * ((bw+1)>>1) + ((x >> 2) & ~0x1f); 
	DWORD block = blockTable4[(y >> 4) & 7][(x >> 5) & 3];
	DWORD word = ((page + block) << 9) + columnTable4[y & 15][x & 31];
	ASSERT(word < 1024*1024*8);
	return word;
}

DWORD __fastcall GSLocalMemory::pixelAddressOrg32Z(int x, int y, DWORD bp, DWORD bw)
{
//	DWORD page = (bp >> 5) + (y >> 5) * bw + (x >> 6); 
//	DWORD block = (bp & 0x1f) + blockTable32Z[(y >> 3) & 3][(x >> 3) & 7];
//	DWORD word = (((page << 5) + block) << 6) + ((y & 7) << 3) + (x & 7);
	DWORD page = bp + (y & ~0x1f) * bw + ((x >> 1) & ~0x1f); 
	DWORD block = blockTable32Z[(y >> 3) & 3][(x >> 3) & 7];
	DWORD word = ((page + block) << 6) + ((y & 7) << 3) + (x & 7);
	ASSERT(word < 1024*1024);
	return word;
}

DWORD __fastcall GSLocalMemory::pixelAddressOrg16Z(int x, int y, DWORD bp, DWORD bw)
{
//	DWORD page = (bp >> 5) + (y >> 6) * bw + (x >> 6); 
//	DWORD block = (bp & 0x1f) + blockTable16Z[(y >> 3) & 7][(x >> 4) & 3];
//	DWORD word = (((page << 5) + block) << 7) + ((y & 7) << 4) + (x & 15);
	DWORD page = bp + ((y >> 1) & ~0x1f) * bw + ((x >> 1) & ~0x1f); 
	DWORD block = blockTable16Z[(y >> 3) & 7][(x >> 4) & 3];
	DWORD word = ((page + block) << 7) + ((y & 7) << 4) + (x & 15);
	ASSERT(word < 1024*1024*2);
	return word;
}

DWORD __fastcall GSLocalMemory::pixelAddressOrg16SZ(int x, int y, DWORD bp, DWORD bw)
{
//	DWORD page = (bp >> 5) + (y >> 6) * bw + (x >> 6); 
//	DWORD block = (bp & 0x1f) + blockTable16SZ[(y >> 3) & 7][(x >> 4) & 3];
//	DWORD word = (((page << 5) + block) << 7) + ((y & 7) << 4) + (x & 15);
	DWORD page = bp + ((y >> 1) & ~0x1f) * bw + ((x >> 1) & ~0x1f); 
	DWORD block = blockTable16SZ[(y >> 3) & 7][(x >> 4) & 3];
	DWORD word = ((page + block) << 7) + ((y & 7) << 4) + (x & 15);
	ASSERT(word < 1024*1024*2);
	return word;
}

////////////////////

DWORD __fastcall GSLocalMemory::pixelAddress32(int x, int y, DWORD bp, DWORD bw)
{
	DWORD page = (bp >> 5) + (y >> 5) * bw + (x >> 6); 
	DWORD word = (page << 11) + pageOffset32[bp & 0x1f][y & 0x1f][x & 0x3f];
	ASSERT(word < 1024*1024);
	return word;
}

DWORD __fastcall GSLocalMemory::pixelAddress16(int x, int y, DWORD bp, DWORD bw)
{
	DWORD page = (bp >> 5) + (y >> 6) * bw + (x >> 6); 
	DWORD word = (page << 12) + pageOffset16[bp & 0x1f][y & 0x3f][x & 0x3f];
	ASSERT(word < 1024*1024*2);
	return word;
}

DWORD __fastcall GSLocalMemory::pixelAddress16S(int x, int y, DWORD bp, DWORD bw)
{
	DWORD page = (bp >> 5) + (y >> 6) * bw + (x >> 6); 
	DWORD word = (page << 12) + pageOffset16S[bp & 0x1f][y & 0x3f][x & 0x3f];
	ASSERT(word < 1024*1024*2);
	return word;
}

DWORD __fastcall GSLocalMemory::pixelAddress8(int x, int y, DWORD bp, DWORD bw)
{
	DWORD page = (bp >> 5) + (y >> 6) * ((bw+1)>>1) + (x >> 7); 
	DWORD word = (page << 13) + pageOffset8[bp & 0x1f][y & 0x3f][x & 0x7f];
	ASSERT(word < 1024*1024*4);
	return word;
}

DWORD __fastcall GSLocalMemory::pixelAddress4(int x, int y, DWORD bp, DWORD bw)
{
	DWORD page = (bp >> 5) + (y >> 7) * ((bw+1)>>1) + (x >> 7);
	DWORD word = (page << 14) + pageOffset4[bp & 0x1f][y & 0x7f][x & 0x7f];
	ASSERT(word < 1024*1024*8);
	return word;
}

DWORD __fastcall GSLocalMemory::pixelAddress32Z(int x, int y, DWORD bp, DWORD bw)
{
	DWORD page = (bp >> 5) + (y >> 5) * bw + (x >> 6); 
	DWORD word = (page << 11) + pageOffset32Z[bp & 0x1f][y & 0x1f][x & 0x3f];
	ASSERT(word < 1024*1024);
	return word;
}

DWORD __fastcall GSLocalMemory::pixelAddress16Z(int x, int y, DWORD bp, DWORD bw)
{
	DWORD page = (bp >> 5) + (y >> 6) * bw + (x >> 6); 
	DWORD word = (page << 12) + pageOffset16Z[bp & 0x1f][y & 0x3f][x & 0x3f];
	ASSERT(word < 1024*1024*2);
	return word;
}

DWORD __fastcall GSLocalMemory::pixelAddress16SZ(int x, int y, DWORD bp, DWORD bw)
{
	DWORD page = (bp >> 5) + (y >> 6) * bw + (x >> 6); 
	DWORD word = (page << 12) + pageOffset16SZ[bp & 0x1f][y & 0x3f][x & 0x3f];
	ASSERT(word < 1024*1024*2);
	return word;
}

////////////////////

void GSLocalMemory::writePixel32(int x, int y, DWORD c, DWORD bp, DWORD bw)
{
	DWORD addr = pixelAddress32(x, y, bp, bw);
	m_vm32[addr] = c;
}

void GSLocalMemory::writePixel24(int x, int y, DWORD c, DWORD bp, DWORD bw)
{
	DWORD addr = pixelAddress32(x, y, bp, bw);
	m_vm32[addr] = (m_vm32[addr] & 0xff000000) | (c & 0x00ffffff);
}

void GSLocalMemory::writePixel16(int x, int y, DWORD c, DWORD bp, DWORD bw)
{
	DWORD addr = pixelAddress16(x, y, bp, bw);
	m_vm16[addr] = (WORD)c;
}

void GSLocalMemory::writePixel16S(int x, int y, DWORD c, DWORD bp, DWORD bw)
{
	DWORD addr = pixelAddress16S(x, y, bp, bw);
	m_vm16[addr] = (WORD)c;
}

void GSLocalMemory::writePixel8(int x, int y, DWORD c, DWORD bp, DWORD bw)
{
	DWORD addr = pixelAddress8(x, y, bp, bw);
	m_vm8[addr] = (BYTE)c;
}

void GSLocalMemory::writePixel8H(int x, int y, DWORD c, DWORD bp, DWORD bw)
{
	DWORD addr = pixelAddress32(x, y, bp, bw);
	m_vm32[addr] = (m_vm32[addr] & 0x00ffffff) | (c << 24);
}

void GSLocalMemory::writePixel4(int x, int y, DWORD c, DWORD bp, DWORD bw)
{
	DWORD addr = pixelAddress4(x, y, bp, bw);
	int shift = (addr&1) << 2; addr >>= 1;
	m_vm8[addr] = (BYTE)((m_vm8[addr] & (0xf0 >> shift)) | ((c & 0x0f) << shift));
}

void GSLocalMemory::writePixel4HL(int x, int y, DWORD c, DWORD bp, DWORD bw)
{
	DWORD addr = pixelAddress32(x, y, bp, bw);
	m_vm32[addr] = (m_vm32[addr] & 0xf0ffffff) | ((c & 0x0f) << 24);
}

void GSLocalMemory::writePixel4HH(int x, int y, DWORD c, DWORD bp, DWORD bw)
{
	DWORD addr = pixelAddress32(x, y, bp, bw);
	m_vm32[addr] = (m_vm32[addr] & 0x0fffffff) | ((c & 0x0f) << 28);
}

void GSLocalMemory::writePixel32Z(int x, int y, DWORD c, DWORD bp, DWORD bw)
{
	DWORD addr = pixelAddress32Z(x, y, bp, bw);
	m_vm32[addr] = c;
}

void GSLocalMemory::writePixel24Z(int x, int y, DWORD c, DWORD bp, DWORD bw)
{
	DWORD addr = pixelAddress32Z(x, y, bp, bw);
	m_vm32[addr] = (m_vm32[addr] & 0xff000000) | (c & 0x00ffffff);
}

void GSLocalMemory::writePixel16Z(int x, int y, DWORD c, DWORD bp, DWORD bw)
{
	DWORD addr = pixelAddress16Z(x, y, bp, bw);
	m_vm16[addr] = (WORD)c;
}

void GSLocalMemory::writePixel16SZ(int x, int y, DWORD c, DWORD bp, DWORD bw)
{
	DWORD addr = pixelAddress16SZ(x, y, bp, bw);
	m_vm16[addr] = (WORD)c;
}

////////////////////

void GSLocalMemory::writeFrame16(int x, int y, DWORD c, DWORD bp, DWORD bw)
{
	c = ((c>>16)&0x8000)|((c>>9)&0x7c00)|((c>>6)&0x03e0)|((c>>3)&0x001f);
	writePixel16(x, y, c, bp, bw);
}

void GSLocalMemory::writeFrame16S(int x, int y, DWORD c, DWORD bp, DWORD bw)
{
	c = ((c>>16)&0x8000)|((c>>9)&0x7c00)|((c>>6)&0x03e0)|((c>>3)&0x001f);
	writePixel16S(x, y, c, bp, bw);
}

////////////////////

DWORD GSLocalMemory::readPixel32(int x, int y, DWORD bp, DWORD bw)
{
	return m_vm32[pixelAddress32(x, y, bp, bw)];
}

DWORD GSLocalMemory::readPixel24(int x, int y, DWORD bp, DWORD bw)
{
	return m_vm32[pixelAddress32(x, y, bp, bw)] & 0x00ffffff;
}

DWORD GSLocalMemory::readPixel16(int x, int y, DWORD bp, DWORD bw)
{
	return (DWORD)m_vm16[pixelAddress16(x, y, bp, bw)];
}

DWORD GSLocalMemory::readPixel16S(int x, int y, DWORD bp, DWORD bw)
{
	return (DWORD)m_vm16[pixelAddress16S(x, y, bp, bw)];
}

DWORD GSLocalMemory::readPixel8(int x, int y, DWORD bp, DWORD bw)
{
	return (DWORD)m_vm8[pixelAddress8(x, y, bp, bw)];
}

DWORD GSLocalMemory::readPixel8H(int x, int y, DWORD bp, DWORD bw)
{
	return m_vm32[pixelAddress32(x, y, bp, bw)] >> 24;
}

DWORD GSLocalMemory::readPixel4(int x, int y, DWORD bp, DWORD bw)
{
	DWORD addr = pixelAddress4(x, y, bp, bw);
	return (m_vm8[addr>>1] >> ((addr&1) << 2)) & 0x0f;
}

DWORD GSLocalMemory::readPixel4HL(int x, int y, DWORD bp, DWORD bw)
{
	return (m_vm32[pixelAddress32(x, y, bp, bw)] >> 24) & 0x0f;
}

DWORD GSLocalMemory::readPixel4HH(int x, int y, DWORD bp, DWORD bw)
{
	return (m_vm32[pixelAddress32(x, y, bp, bw)] >> 28) & 0x0f;
}

DWORD GSLocalMemory::readPixel32Z(int x, int y, DWORD bp, DWORD bw)
{
	return m_vm32[pixelAddress32Z(x, y, bp, bw)];
}

DWORD GSLocalMemory::readPixel24Z(int x, int y, DWORD bp, DWORD bw)
{
	return m_vm32[pixelAddress32Z(x, y, bp, bw)] & 0x00ffffff;
}

DWORD GSLocalMemory::readPixel16Z(int x, int y, DWORD bp, DWORD bw)
{
	return (DWORD)m_vm16[pixelAddress16Z(x, y, bp, bw)];
}

DWORD GSLocalMemory::readPixel16SZ(int x, int y, DWORD bp, DWORD bw)
{
	return (DWORD)m_vm16[pixelAddress16SZ(x, y, bp, bw)];
}

////////////////////
/*
void GSLocalMemory::writePixel32(DWORD addr, DWORD c)
{
	m_vm32[addr] = c;
}

void GSLocalMemory::writePixel24(DWORD addr, DWORD c)
{
	m_vm32[addr] = (m_vm32[addr] & 0xff000000) | (c & 0x00ffffff);
}

void GSLocalMemory::writePixel16(DWORD addr, DWORD c)
{
	m_vm16[addr] = (WORD)c;
}

void GSLocalMemory::writePixel16S(DWORD addr, DWORD c)
{
	m_vm16[addr] = (WORD)c;
}

void GSLocalMemory::writePixel8(DWORD addr, DWORD c)
{
	m_vm8[addr] = (BYTE)c;
}

void GSLocalMemory::writePixel8H(DWORD addr, DWORD c)
{
	m_vm32[addr] = (m_vm32[addr] & 0x00ffffff) | (c << 24);
}

void GSLocalMemory::writePixel4(DWORD addr, DWORD c)
{
	int shift = (addr&1) << 2; addr >>= 1;
	m_vm8[addr] = (BYTE)((m_vm8[addr] & (0xf0 >> shift)) | ((c & 0x0f) << shift));
}

void GSLocalMemory::writePixel4HL(DWORD addr, DWORD c)
{
	m_vm32[addr] = (m_vm32[addr] & 0xf0ffffff) | ((c & 0x0f) << 24);
}

void GSLocalMemory::writePixel4HH(DWORD addr, DWORD c)
{
	m_vm32[addr] = (m_vm32[addr] & 0x0fffffff) | ((c & 0x0f) << 28);
}

void GSLocalMemory::writePixel32Z(DWORD addr, DWORD c)
{
	m_vm32[addr] = c;
}

void GSLocalMemory::writePixel24Z(DWORD addr, DWORD c)
{
	m_vm32[addr] = (m_vm32[addr] & 0xff000000) | (c & 0x00ffffff);
}

void GSLocalMemory::writePixel16Z(DWORD addr, DWORD c)
{
	m_vm16[addr] = (WORD)c;
}

void GSLocalMemory::writePixel16SZ(DWORD addr, DWORD c)
{
	m_vm16[addr] = (WORD)c;
}

////////////////////

void GSLocalMemory::writeFrame16(DWORD addr, DWORD c)
{
	writePixel16(addr, ((c>>16)&0x8000)|((c>>9)&0x7c00)|((c>>6)&0x03e0)|((c>>3)&0x001f));
}

void GSLocalMemory::writeFrame16S(DWORD addr, DWORD c)
{
	writePixel16S(addr, ((c>>16)&0x8000)|((c>>9)&0x7c00)|((c>>6)&0x03e0)|((c>>3)&0x001f));
}

////////////////////

DWORD GSLocalMemory::readPixel32(DWORD addr)
{
	return m_vm32[addr];
}

DWORD GSLocalMemory::readPixel24(DWORD addr)
{
	return m_vm32[addr] & 0x00ffffff;
}

DWORD GSLocalMemory::readPixel16(DWORD addr)
{
	return (DWORD)m_vm16[addr];
}

DWORD GSLocalMemory::readPixel16S(DWORD addr)
{
	return (DWORD)m_vm16[addr];
}

DWORD GSLocalMemory::readPixel8(DWORD addr)
{
	return (DWORD)m_vm8[addr];
}

DWORD GSLocalMemory::readPixel8H(DWORD addr)
{
	return m_vm32[addr] >> 24;
}

DWORD GSLocalMemory::readPixel4(DWORD addr)
{
	return (m_vm8[addr>>1] >> ((addr&1) << 2)) & 0x0f;
}

DWORD GSLocalMemory::readPixel4HL(DWORD addr)
{
	return (m_vm32[addr] >> 24) & 0x0f;
}

DWORD GSLocalMemory::readPixel4HH(DWORD addr)
{
	return (m_vm32[addr] >> 28) & 0x0f;
}

DWORD GSLocalMemory::readPixel32Z(DWORD addr)
{
	return m_vm32[addr];
}

DWORD GSLocalMemory::readPixel24Z(DWORD addr)
{
	return m_vm32[addr] & 0x00ffffff;
}

DWORD GSLocalMemory::readPixel16Z(DWORD addr)
{
	return (DWORD)m_vm16[addr];
}

DWORD GSLocalMemory::readPixel16SZ(DWORD addr)
{
	return (DWORD)m_vm16[addr];
}
*/
////////////////////

bool GSLocalMemory::FillRect(const CRect& r, DWORD c, DWORD psm, DWORD fbp, DWORD fbw)
{
	const psmtbl_t& tbl = m_psmtbl[psm];

	writePixel wp = tbl.wp;
	pixelAddress ba = tbl.ba;

	int w = tbl.bs.cx;
	int h = tbl.bs.cy;
	int bpp = tbl.bpp;

	int shift = 0;

	switch(bpp)
	{
	case 32: shift = 0; break;
	case 16: shift = 1; c = (c&0xffff)*0x00010001; break;
	case 8: shift = 2; c = (c&0xff)*0x01010101; break;
	case 4: shift = 3; c = (c&0xf)*0x11111111; break;
	}

	CRect clip((r.left+(w-1))&~(w-1), (r.top+(h-1))&~(h-1), r.right&~(w-1), r.bottom&~(h-1));

	for(int y = r.top; y < clip.top; y++)
		for(int x = r.left; x < r.right; x++)
			(this->*wp)(x, y, c, fbp, fbw);

	for(int y = clip.top; y < clip.bottom; y += h)
	{
		for(int ys = y, ye = y + h; ys < ye; ys++)
		{
			for(int x = r.left; x < clip.left; x++)
				(this->*wp)(x, ys, c, fbp, fbw);
			for(int x = clip.right; x < r.right; x++)
				(this->*wp)(x, ys, c, fbp, fbw);
		}
	}

	if(psm == PSM_PSMCT24 || psm == PSM_PSMZ24)
	{
		c &= 0x00ffffff;
		for(int y = clip.top; y < clip.bottom; y += h)
		{
			for(int x = clip.left; x < clip.right; x += w)
			{
				DWORD* p = &m_vm32[ba(x, y, fbp, fbw)];
				for(int i = 0; i < 64; i++)
					p[i] = (p[i]&0xff000000) | c;
			}
		}
	}
	else
	{
		for(int y = clip.top; y < clip.bottom; y += h)
			for(int x = clip.left; x < clip.right; x += w)
				memsetd(&m_vm8[ba(x, y, fbp, fbw) << 2 >> shift], c, 64);
	}

	for(int y = clip.bottom; y < r.bottom; y++)
		for(int x = r.left; x < r.right; x++)
			(this->*wp)(x, y, c, fbp, fbw);

	return(true);
}

////////////////////

void GSLocalMemory::WriteCLUT(GIFRegTEX0 TEX0, GIFRegTEXCLUT TEXCLUT)
{
	switch(TEX0.CLD)
	{
	default:
	case 0: return;
	case 1: break;
	case 2: m_CBP[0] = TEX0.CBP; break;
	case 3: m_CBP[1] = TEX0.CBP; break;
	case 4: if(m_CBP[0] == TEX0.CBP) return; break;
	case 5: if(m_CBP[1] == TEX0.CBP) return; break;
	}

	{
		if(!m_fCLUTMayBeDirty && m_prevTEX0.i64 == TEX0.i64 && m_prevTEXCLUT.i64 == TEXCLUT.i64)
			return;

		m_prevTEX0 = TEX0;
		m_prevTEXCLUT = TEXCLUT;

		m_fCLUTMayBeDirty = false;
	}

	DWORD bp = TEX0.CBP;
	DWORD bw = TEX0.CSM == 0 ? 1 : TEXCLUT.CBW;

	WORD* pCLUT = m_pCLUT + (TEX0.CSA<<4);

	// NOTE: TEX0.CPSM == PSM_PSMCT24 is non-standard, KH uses it

	if(TEX0.CSM == 0)
	{
		if(TEX0.CPSM == PSM_PSMCT16 || TEX0.CPSM == PSM_PSMCT16S)
		{
			WORD* vm = &m_vm16[TEX0.CPSM == PSM_PSMCT16 ? blockAddress16(0, 0, bp, bw) : blockAddress16S(0, 0, bp, bw)];

			if(TEX0.PSM == PSM_PSMT8 || TEX0.PSM == PSM_PSMT8H)
			{
				WriteCLUT_T16_I8_CSM1(vm, pCLUT);
			}
			else if(TEX0.PSM == PSM_PSMT4HH || TEX0.PSM == PSM_PSMT4HL || TEX0.PSM == PSM_PSMT4)
			{
				WriteCLUT_T16_I4_CSM1(vm, pCLUT);
			}
		}
		else if(TEX0.CPSM == PSM_PSMCT32 || TEX0.CPSM == PSM_PSMCT24)
		{
			DWORD* vm = &m_vm32[blockAddress32(0, 0, bp, bw)];

			if(TEX0.PSM == PSM_PSMT8 || TEX0.PSM == PSM_PSMT8H)
			{
				WriteCLUT_T32_I8_CSM1(vm, pCLUT);
			}
			else if(TEX0.PSM == PSM_PSMT4HH || TEX0.PSM == PSM_PSMT4HL || TEX0.PSM == PSM_PSMT4)
			{
				WriteCLUT_T32_I4_CSM1(vm, pCLUT);
			}
		}
	}
	else
	{
		readPixel rp = m_psmtbl[TEX0.CPSM].rp;

		int nPaletteEntries = m_psmtbl[TEX0.PSM].pal;

		ASSERT(nPaletteEntries == 0 || TEX0.CPSM == PSM_PSMCT16); // this is the only allowed format for CSM2, but we implement all of them, just in case...

		if(TEX0.CPSM == PSM_PSMCT16 || TEX0.CPSM == PSM_PSMCT16S)
		{
			for(int i = 0; i < nPaletteEntries; i++)
			{
				pCLUT[i] = (WORD)(this->*rp)((TEXCLUT.COU<<4) + i, TEXCLUT.COV, bp, bw);
			}
		}
		else if(TEX0.CPSM == PSM_PSMCT32 || TEX0.CPSM == PSM_PSMCT24)
		{
			for(int i = 0; i < nPaletteEntries; i++)
			{
				DWORD dw = (this->*rp)((TEXCLUT.COU<<4) + i, TEXCLUT.COV, bp, bw);
				pCLUT[i] = (WORD)(dw & 0xffff);
				pCLUT[i+256] = (WORD)(dw >> 16);
			}
		}
	}
}

//

void GSLocalMemory::ReadCLUT(GIFRegTEX0 TEX0, GIFRegTEXA TEXA, DWORD* pCLUT32)
{
	ASSERT(pCLUT32);

	WORD* pCLUT = m_pCLUT + (TEX0.CSA<<4);

	if(TEX0.CPSM == PSM_PSMCT32)
	{
		switch(TEX0.PSM)
		{
		case PSM_PSMT8:
		case PSM_PSMT8H:
			ReadCLUT32_T32_I8(pCLUT, pCLUT32);
			break;
		case PSM_PSMT4:
		case PSM_PSMT4HL:
		case PSM_PSMT4HH:
			ReadCLUT32_T32_I4(pCLUT, pCLUT32);
			break;
		}
	}
	else if(TEX0.CPSM == PSM_PSMCT16 || TEX0.CPSM == PSM_PSMCT16S)
	{
		switch(TEX0.PSM)
		{
		case PSM_PSMT8:
		case PSM_PSMT8H:
			ReadCLUT32_T16_I8(pCLUT, pCLUT32);
			break;
		case PSM_PSMT4:
		case PSM_PSMT4HL:
		case PSM_PSMT4HH:
			ReadCLUT32_T16_I4(pCLUT, pCLUT32);
			break;
		}
	}
}

void GSLocalMemory::SetupCLUT(GIFRegTEX0 TEX0, GIFRegTEXA TEXA)
{
	// TODO: cache m_pCLUT*

	ReadCLUT(TEX0, TEXA, m_pCLUT32);

	switch(TEX0.PSM)
	{
	case PSM_PSMT4:
	case PSM_PSMT4HL:
	case PSM_PSMT4HH:
		// sse2?
		if(TEX0.CPSM == PSM_PSMCT32)
		{
			for(int j = 0, k = 0; j < 16; j++)
				for(int i = 0; i < 16; i++, k++)
					m_pCLUT64[k] = ((UINT64)m_pCLUT32[j] << 32) | m_pCLUT32[i];
		}
		else
		{
			for(int j = 0, k = 0; j < 16; j++)
				for(int i = 0; i < 16; i++, k++)
					m_pCLUT64[k] = ((UINT64)m_pCLUT32[j] << 16) | (m_pCLUT32[i]&0xffff);
		}
		break;
	}
}

//

void GSLocalMemory::ReadCLUT32(GIFRegTEX0 TEX0, GIFRegTEXA TEXA, DWORD* pCLUT32)
{
	ASSERT(pCLUT32);

	WORD* pCLUT = m_pCLUT + (TEX0.CSA<<4);

	if(TEX0.CPSM == PSM_PSMCT32)
	{
		switch(TEX0.PSM)
		{
		case PSM_PSMT8:
		case PSM_PSMT8H:
			ReadCLUT32_T32_I8(pCLUT, pCLUT32);
			break;
		case PSM_PSMT4:
		case PSM_PSMT4HL:
		case PSM_PSMT4HH:
			ReadCLUT32_T32_I4(pCLUT, pCLUT32);
			break;
		}
	}
	else if(TEX0.CPSM == PSM_PSMCT16 || TEX0.CPSM == PSM_PSMCT16S)
	{
		Expand16(pCLUT, pCLUT32, m_psmtbl[TEX0.PSM].pal, &TEXA);
	}
}

void GSLocalMemory::SetupCLUT32(GIFRegTEX0 TEX0, GIFRegTEXA TEXA)
{
	// TODO: cache m_pCLUT*

	ReadCLUT32(TEX0, TEXA, m_pCLUT32);

	switch(TEX0.PSM)
	{
	case PSM_PSMT4:
	case PSM_PSMT4HL:
	case PSM_PSMT4HH:
		// sse2?
		for(int j = 0, k = 0; j < 16; j++)
			for(int i = 0; i < 16; i++, k++)
				m_pCLUT64[k] = ((UINT64)m_pCLUT32[j] << 32) | m_pCLUT32[i];
		break;
	}
}

void GSLocalMemory::CopyCLUT32(DWORD* pCLUT32, int nPaletteEntries)
{
	memcpy(pCLUT32, m_pCLUT32, sizeof(DWORD)*nPaletteEntries);
}

////////////////////

DWORD GSLocalMemory::readTexel32(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return m_vm32[pixelAddress32(x, y, TEX0.TBP0, TEX0.TBW)];
}

DWORD GSLocalMemory::readTexel24(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return Expand24To32(m_vm32[pixelAddress32(x, y, TEX0.TBP0, TEX0.TBW)], TEX0.ai32[1]&4, TEXA);
}

DWORD GSLocalMemory::readTexel16(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return Expand16To32(m_vm16[pixelAddress16(x, y, TEX0.TBP0, TEX0.TBW)], TEXA);
}

DWORD GSLocalMemory::readTexel16S(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return Expand16To32(m_vm16[pixelAddress16S(x, y, TEX0.TBP0, TEX0.TBW)], TEXA);
}

DWORD GSLocalMemory::readTexel8(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return m_pCLUT32[readPixel8(x, y, TEX0.TBP0, TEX0.TBW)];
}

DWORD GSLocalMemory::readTexel8H(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return m_pCLUT32[readPixel8H(x, y, TEX0.TBP0, TEX0.TBW)];
}

DWORD GSLocalMemory::readTexel4(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return m_pCLUT32[readPixel4(x, y, TEX0.TBP0, TEX0.TBW)];
}

DWORD GSLocalMemory::readTexel4HL(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return m_pCLUT32[readPixel4HL(x, y, TEX0.TBP0, TEX0.TBW)];
}

DWORD GSLocalMemory::readTexel4HH(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return m_pCLUT32[readPixel4HH(x, y, TEX0.TBP0, TEX0.TBW)];
}

////////////////////
/*
DWORD GSLocalMemory::readTexel32(DWORD addr, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return m_vm32[addr];
}

DWORD GSLocalMemory::readTexel24(DWORD addr, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return Expand24To32(m_vm32[addr], TEX0.ai32[1]&4, TEXA);
}

DWORD GSLocalMemory::readTexel16(DWORD addr, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return Expand16To32(m_vm16[addr], TEXA);
}

DWORD GSLocalMemory::readTexel16S(DWORD addr, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return Expand16To32(m_vm16[addr], TEXA);
}

DWORD GSLocalMemory::readTexel8(DWORD addr, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return m_pCLUT32[readPixel8(addr)];
}

DWORD GSLocalMemory::readTexel8H(DWORD addr, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return m_pCLUT32[readPixel8H(addr)];
}

DWORD GSLocalMemory::readTexel4(DWORD addr, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return m_pCLUT32[readPixel4(addr)];
}

DWORD GSLocalMemory::readTexel4HL(DWORD addr, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return m_pCLUT32[readPixel4HL(addr)];
}

DWORD GSLocalMemory::readTexel4HH(DWORD addr, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return m_pCLUT32[readPixel4HH(addr)];
}
*/
////////////////////

static void SwizzleTextureStep(int& tx, int& ty, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG)
{
//	if(ty == TRXREG.RRH && tx == TRXPOS.DSAX) ASSERT(0);

	if(++tx == TRXREG.RRW)
	{
		tx = TRXPOS.DSAX;
		ty++;
	}
}

#define IsTopLeftAligned(dsax, tx, ty, bw, bh) \
	(((dsax) & ((bw)-1)) == 0 && ((tx) & ((bw)-1)) == 0 && (dsax) == (tx) && ((ty) & ((bh)-1)) == 0)

void GSLocalMemory::SwizzleTexture32(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG)
{
	if(TRXREG.RRW == 0) return;

	int tw = TRXREG.RRW, srcpitch = (TRXREG.RRW - TRXPOS.DSAX)*4;
	int th = len / srcpitch;

	bool fTopLeftAligned = IsTopLeftAligned(TRXPOS.DSAX, tx, ty, 8, 8);

	if(!fTopLeftAligned || (tw & 7) || (th & 7) || (len % srcpitch))
	{
		if(fTopLeftAligned && tw >= 8 && th >= 8)
		{
			int twa = tw & ~7;
			int tha = th & ~7;

			len -= tha * srcpitch;
			th -= tha;

			for(int j = 0; j < tha; j += 8)
			{
				for(int x = tx; x < twa; x += 8)
					SwizzleBlock32u((BYTE*)&m_vm32[blockAddress32(x, ty, BITBLTBUF.DBP, BITBLTBUF.DBW)], src + (x - tx)*4, srcpitch);

				for(int i = 0; i < 8; i++, ty++, src += srcpitch)
					for(int x = twa; x < tw; x++)
						writePixel32(x, ty, ((DWORD*)src)[x - tx], BITBLTBUF.DBP, BITBLTBUF.DBW);
			}
		}

		if(len > 0 && tw >= 8 && th >= 2 && IsTopLeftAligned(TRXPOS.DSAX, tx, ty, 8, 2))
		{
			int twa = tw & ~7;
			int tha = th & ~1;

			len -= tha * srcpitch;
			th -= tha;

			for(int j = 0; j < tha; j += 2)
			{
				for(int x = tx; x < twa; x += 8)
					SwizzleColumn32(ty, (BYTE*)&m_vm32[blockAddress32(x, ty&~7, BITBLTBUF.DBP, BITBLTBUF.DBW)], src + (x - tx)*4, srcpitch);

				for(int i = 0; i < 2; i++, ty++, src += srcpitch)
					for(int x = twa; x < tw; x++)
						writePixel32(x, ty, ((DWORD*)src)[x - tx], BITBLTBUF.DBP, BITBLTBUF.DBW);
			}
		}

		SwizzleTextureX(tx, ty, src, len, BITBLTBUF, TRXPOS, TRXREG);
	}
	else
	{
		th += ty;

		if((DWORD_PTR)src & 0xf)
		{
			for(int y = ty; y < th; y += 8, src += srcpitch*8)
				for(int x = tx; x < tw; x += 8)
					SwizzleBlock32u((BYTE*)&m_vm32[blockAddress32(x, y, BITBLTBUF.DBP, BITBLTBUF.DBW)], src + (x - tx)*4, srcpitch);
		}
		else
		{
			for(int y = ty; y < th; y += 8, src += srcpitch*8)
				for(int x = tx; x < tw; x += 8)
					SwizzleBlock32((BYTE*)&m_vm32[blockAddress32(x, y, BITBLTBUF.DBP, BITBLTBUF.DBW)], src + (x - tx)*4, srcpitch);
		}

		ty = th;
	}
}

void GSLocalMemory::SwizzleTexture24(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG)
{
	if(TRXREG.RRW == 0) return;

	int tw = TRXREG.RRW, srcpitch = (TRXREG.RRW - TRXPOS.DSAX)*3;
	int th = len / srcpitch;

	bool fTopLeftAligned = IsTopLeftAligned(TRXPOS.DSAX, tx, ty, 8, 8);

	if(!fTopLeftAligned || (tw & 7) || (th & 7) || (len % srcpitch))
	{
		// TODO

		SwizzleTextureX(tx, ty, src, len, BITBLTBUF, TRXPOS, TRXREG);
	}
	else
	{
		__declspec(align(16)) DWORD block[8*8];

		th += ty;

		for(int y = ty; y < th; y += 8, src += srcpitch*8)
		{
			for(int x = tx; x < tw; x += 8)
			{
				BYTE* s = src + (x - tx)*3;
				DWORD* d = block;

				for(int j = 0, diff = srcpitch - 8*3; j < 8; j++, s += diff, d += 8)
					for(int i = 0; i < 8; i++, s += 3)
						d[i] = (s[2]<<16)|(s[1]<<8)|s[0];

				SwizzleBlock32((BYTE*)&m_vm32[blockAddress32(x, y, BITBLTBUF.DBP, BITBLTBUF.DBW)], (BYTE*)block, sizeof(block)/8, 0x00ffffff);
			}
		}

		ty = th;
	}
}

void GSLocalMemory::SwizzleTexture16(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG)
{
	if(TRXREG.RRW == 0) return;

	int tw = TRXREG.RRW, srcpitch = (TRXREG.RRW - TRXPOS.DSAX)*2;
	int th = len / srcpitch;

	bool fTopLeftAligned = IsTopLeftAligned(TRXPOS.DSAX, tx, ty, 16, 8);

	if(!fTopLeftAligned || (tw & 15) || (th & 7) || (len % srcpitch))
	{
		if(fTopLeftAligned && tw >= 16 && th >= 8)
		{
			int twa = tw & ~15;
			int tha = th & ~7;

			len -= tha * srcpitch;
			th -= tha;

			for(int j = 0; j < tha; j += 8)
			{
				for(int x = tx; x < twa; x += 16)
					SwizzleBlock16u((BYTE*)&m_vm16[blockAddress16(x, ty, BITBLTBUF.DBP, BITBLTBUF.DBW)], src + (x - tx)*2, srcpitch);

				for(int i = 0; i < 8; i++, ty++, src += srcpitch)
					for(int x = twa; x < tw; x++)
						writePixel16(x, ty, ((WORD*)src)[x - tx], BITBLTBUF.DBP, BITBLTBUF.DBW);
			}
		}

		if(len > 0 && tw >= 16 && th >= 2 && IsTopLeftAligned(TRXPOS.DSAX, tx, ty, 16, 2))
		{
			int twa = tw & ~15;
			int tha = th & ~1;

			len -= tha * srcpitch;
			th -= tha;

			for(int j = 0; j < tha; j += 2)
			{
				for(int x = tx; x < twa; x += 16)
					SwizzleColumn16(ty, (BYTE*)&m_vm16[blockAddress16(x, ty&~7, BITBLTBUF.DBP, BITBLTBUF.DBW)], src + (x - tx)*2, srcpitch);

				for(int i = 0; i < 2; i++, ty++, src += srcpitch)
					for(int x = twa; x < tw; x++)
						writePixel16(x, ty, ((WORD*)src)[x - tx], BITBLTBUF.DBP, BITBLTBUF.DBW);
			}
		}

		SwizzleTextureX(tx, ty, src, len, BITBLTBUF, TRXPOS, TRXREG);
	}
	else
	{
		th += ty;

		if((DWORD_PTR)src & 0xf)
		{
			for(int y = ty; y < th; y += 8, src += srcpitch*8)
				for(int x = tx; x < tw; x += 16)
					SwizzleBlock16u((BYTE*)&m_vm16[blockAddress16(x, y, BITBLTBUF.DBP, BITBLTBUF.DBW)], src + (x - tx)*2, srcpitch);
		}
		else
		{
			for(int y = ty; y < th; y += 8, src += srcpitch*8)
				for(int x = tx; x < tw; x += 16)
					SwizzleBlock16((BYTE*)&m_vm16[blockAddress16(x, y, BITBLTBUF.DBP, BITBLTBUF.DBW)], src + (x - tx)*2, srcpitch);
		}

		ty = th;
	}
}

void GSLocalMemory::SwizzleTexture16S(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG)
{
	if(TRXREG.RRW == 0) return;

	int tw = TRXREG.RRW, srcpitch = (TRXREG.RRW - TRXPOS.DSAX)*2;
	int th = len / srcpitch;

	bool fTopLeftAligned = IsTopLeftAligned(TRXPOS.DSAX, tx, ty, 16, 8);

	if(!fTopLeftAligned || (tw & 15) || (th & 7) || (len % srcpitch))
	{
		if(fTopLeftAligned && tw >= 16 && th >= 8)
		{
			int twa = tw & ~15;
			int tha = th & ~7;

			len -= tha * srcpitch;
			th -= tha;

			for(int j = 0; j < tha; j += 8)
			{
				for(int x = tx; x < twa; x += 16)
					SwizzleBlock16u((BYTE*)&m_vm16[blockAddress16S(x, ty, BITBLTBUF.DBP, BITBLTBUF.DBW)], src + (x - tx)*2, srcpitch);

				for(int i = 0; i < 8; i++, ty++, src += srcpitch)
					for(int x = twa; x < tw; x++)
						writePixel16S(x, ty, ((WORD*)src)[x - tx], BITBLTBUF.DBP, BITBLTBUF.DBW);
			}
		}

		if(len > 0 && tw >= 16 && th >= 2 && IsTopLeftAligned(TRXPOS.DSAX, tx, ty, 16, 2))
		{
			int twa = tw & ~15;
			int tha = th & ~1;

			len -= tha * srcpitch;
			th -= tha;

			for(int j = 0; j < tha; j += 2)
			{
				for(int x = tx; x < twa; x += 16)
					SwizzleColumn16(ty, (BYTE*)&m_vm16[blockAddress16S(x, ty&~7, BITBLTBUF.DBP, BITBLTBUF.DBW)], src + (x - tx)*2, srcpitch);

				for(int i = 0; i < 2; i++, ty++, src += srcpitch)
					for(int x = twa; x < tw; x++)
						writePixel16S(x, ty, ((WORD*)src)[x - tx], BITBLTBUF.DBP, BITBLTBUF.DBW);
			}
		}

		SwizzleTextureX(tx, ty, src, len, BITBLTBUF, TRXPOS, TRXREG);
	}
	else
	{
		th += ty;

		if((DWORD_PTR)src & 0xf)
		{
			for(int y = ty; y < th; y += 8, src += srcpitch*8)
				for(int x = tx; x < tw; x += 16)
					SwizzleBlock16((BYTE*)&m_vm16[blockAddress16S(x, y, BITBLTBUF.DBP, BITBLTBUF.DBW)], src + (x - tx)*2, srcpitch);
		}
		else
		{
			for(int y = ty; y < th; y += 8, src += srcpitch*8)
				for(int x = tx; x < tw; x += 16)
					SwizzleBlock16((BYTE*)&m_vm16[blockAddress16S(x, y, BITBLTBUF.DBP, BITBLTBUF.DBW)], src + (x - tx)*2, srcpitch);
		}

		ty = th;
	}
}

void GSLocalMemory::SwizzleTexture8(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG)
{
	if(TRXREG.RRW == 0) return;

	int tw = TRXREG.RRW, srcpitch = TRXREG.RRW - TRXPOS.DSAX;
	int th = len / srcpitch;

	bool fTopLeftAligned = IsTopLeftAligned(TRXPOS.DSAX, tx, ty, 16, 16);

	if(!fTopLeftAligned || (tw & 15) || (th & 15) || (len % srcpitch))
	{
		if(fTopLeftAligned && tw >= 16 && th >= 16)
		{
			int twa = tw & ~15;
			int tha = th & ~15;

			len -= tha * srcpitch;
			th -= tha;

			for(int j = 0; j < tha; j += 16)
			{
				for(int x = tx; x < twa; x += 16)
					SwizzleBlock8u((BYTE*)&m_vm8[blockAddress8(x, ty, BITBLTBUF.DBP, BITBLTBUF.DBW)], src + (x - tx), srcpitch);

				for(int i = 0; i < 16; i++, ty++, src += srcpitch)
					for(int x = twa; x < tw; x++)
						writePixel8(x, ty, src[x - tx], BITBLTBUF.DBP, BITBLTBUF.DBW);
			}
		}

		if(len > 0 && tw >= 16 && th >= 4 && IsTopLeftAligned(TRXPOS.DSAX, tx, ty, 16, 4))
		{
			int twa = tw & ~15;
			int tha = th & ~3;

			len -= tha * srcpitch;
			th -= tha;

			for(int j = 0; j < tha; j += 4)
			{
				for(int x = tx; x < twa; x += 16)
					SwizzleColumn8(ty, (BYTE*)&m_vm8[blockAddress8(x, ty&~15, BITBLTBUF.DBP, BITBLTBUF.DBW)], src + (x - tx), srcpitch);

				for(int i = 0; i < 4; i++, ty++, src += srcpitch)
					for(int x = twa; x < tw; x++)
						writePixel8(x, ty, src[x - tx], BITBLTBUF.DBP, BITBLTBUF.DBW);
			}
		}

		SwizzleTextureX(tx, ty, src, len, BITBLTBUF, TRXPOS, TRXREG);
	}
	else
	{
		th += ty;

		if((DWORD_PTR)src & 0xf)
		{
			for(int y = ty; y < th; y += 16, src += srcpitch*16)
				for(int x = tx; x < tw; x += 16)
					SwizzleBlock8u((BYTE*)&m_vm8[blockAddress8(x, y, BITBLTBUF.DBP, BITBLTBUF.DBW)], src + (x - tx), srcpitch);
		}
		else
		{
			for(int y = ty; y < th; y += 16, src += srcpitch*16)
				for(int x = tx; x < tw; x += 16)
					SwizzleBlock8((BYTE*)&m_vm8[blockAddress8(x, y, BITBLTBUF.DBP, BITBLTBUF.DBW)], src + (x - tx), srcpitch);
		}

		ty = th;
	}
}

void GSLocalMemory::SwizzleTexture8H(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG)
{
	if(TRXREG.RRW == 0) return;

	int tw = TRXREG.RRW, srcpitch = TRXREG.RRW - TRXPOS.DSAX;
	int th = len / srcpitch;

	bool fTopLeftAligned = IsTopLeftAligned(TRXPOS.DSAX, tx, ty, 8, 8);

	if(!fTopLeftAligned || (tw & 7) || (th & 7) || (len % srcpitch))
	{
		// TODO

		SwizzleTextureX(tx, ty, src, len, BITBLTBUF, TRXPOS, TRXREG);
	}
	else
	{
		__declspec(align(16)) DWORD block[8*8];

		th += ty;

		for(int y = ty; y < th; y += 8, src += srcpitch*8)
		{
			for(int x = tx; x < tw; x += 8)
			{
				BYTE* s = src + (x - tx);
				DWORD* d = block;

				for(int j = 0; j < 8; j++, s += srcpitch, d += 8)
					for(int i = 0; i < 8; i++)
						d[i] = s[i] << 24;

				SwizzleBlock32((BYTE*)&m_vm32[blockAddress32(x, y, BITBLTBUF.DBP, BITBLTBUF.DBW)], (BYTE*)block, sizeof(block)/8, 0xff000000);
			}
		}

		ty = th;
	}
}

void GSLocalMemory::SwizzleTexture4(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG)
{
	if(TRXREG.RRW == 0) return;

	int tw = TRXREG.RRW, srcpitch = (TRXREG.RRW - TRXPOS.DSAX)/2;
	int th = len / srcpitch;

	bool fTopLeftAligned = IsTopLeftAligned(TRXPOS.DSAX, tx, ty, 32, 16);

	if(!fTopLeftAligned || (tw & 31) || (th & 15) || (len % srcpitch))
	{
		if(fTopLeftAligned && tw >= 32 && th >= 16)
		{
			int twa = tw & ~31;
			int tha = th & ~15;

			len -= tha * srcpitch;
			th -= tha;

			for(int j = 0; j < tha; j += 16)
			{
				for(int x = tx; x < twa; x += 32)
					SwizzleBlock4u((BYTE*)&m_vm8[blockAddress4(x, ty, BITBLTBUF.DBP, BITBLTBUF.DBW)>>1], src + (x - tx)/2, srcpitch);

				for(int i = 0; i < 16; i++, ty++, src += srcpitch)
				{
					BYTE* s = src + (twa - tx)/2;

					for(int x = twa; x < tw; x += 2, s++)
					{
						writePixel4(x, ty, *s&0xf, BITBLTBUF.DBP, BITBLTBUF.DBW),
						writePixel4(x+1, ty, *s>>4, BITBLTBUF.DBP, BITBLTBUF.DBW);
					}
				}
			}
		}

		if(len > 0 && tw >= 32 && th >= 4 && IsTopLeftAligned(TRXPOS.DSAX, tx, ty, 32, 4))
		{
			int twa = tw & ~31;
			int tha = th & ~3;

			len -= tha * srcpitch;
			th -= tha;

			for(int j = 0; j < tha; j += 4)
			{
				for(int x = tx; x < twa; x += 32)
					SwizzleColumn4(ty, (BYTE*)&m_vm8[blockAddress4(x, ty&~15, BITBLTBUF.DBP, BITBLTBUF.DBW)>>1], src + (x - tx)/2, srcpitch);

				for(int i = 0; i < 4; i++, ty++, src += srcpitch)
				{
					BYTE* s = src + (twa - tx)/2;

					for(int x = twa; x < tw; x += 2, s++)
					{
						writePixel4(x, ty, *s&0xf, BITBLTBUF.DBP, BITBLTBUF.DBW),
						writePixel4(x+1, ty, *s>>4, BITBLTBUF.DBP, BITBLTBUF.DBW);
					}
				}
			}
		}

		SwizzleTextureX(tx, ty, src, len, BITBLTBUF, TRXPOS, TRXREG);
	}
	else
	{
		th += ty;

		if((DWORD_PTR)src & 0xf)
		{
			for(int y = ty; y < th; y += 16, src += srcpitch*16)
				for(int x = tx; x < tw; x += 32)
					SwizzleBlock4u((BYTE*)&m_vm8[blockAddress4(x, y, BITBLTBUF.DBP, BITBLTBUF.DBW)>>1], src + (x - tx)/2, srcpitch);
		}
		else
		{
			for(int y = ty; y < th; y += 16, src += srcpitch*16)
				for(int x = tx; x < tw; x += 32)
					SwizzleBlock4((BYTE*)&m_vm8[blockAddress4(x, y, BITBLTBUF.DBP, BITBLTBUF.DBW)>>1], src + (x - tx)/2, srcpitch);
		}

		ty = th;
	}
}

void GSLocalMemory::SwizzleTexture4HL(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG)
{
	if(TRXREG.RRW == 0) return;

	int tw = TRXREG.RRW, srcpitch = (TRXREG.RRW - TRXPOS.DSAX)/2;
	int th = len / srcpitch;

	bool fTopLeftAligned = IsTopLeftAligned(TRXPOS.DSAX, tx, ty, 8, 8);

	if(!fTopLeftAligned || (tw & 7) || (th & 7) || (len % srcpitch))
	{
		// TODO

		SwizzleTextureX(tx, ty, src, len, BITBLTBUF, TRXPOS, TRXREG);
	}
	else
	{
		__declspec(align(16)) DWORD block[8*8];

		th += ty;

		for(int y = ty; y < th; y += 8, src += srcpitch*8)
		{
			for(int x = tx; x < tw; x += 8)
			{
				BYTE* s = src + (x - tx)/2;
				DWORD* d = block;

				for(int j = 0; j < 8; j++, s += srcpitch, d += 8)
					for(int i = 0; i < 8/2; i++)
						d[i*2] = (s[i]&0x0f) << 24, 
						d[i*2+1] = (s[i]&0xf0) << 20;

				SwizzleBlock32((BYTE*)&m_vm32[blockAddress32(x, y, BITBLTBUF.DBP, BITBLTBUF.DBW)], (BYTE*)block, sizeof(block)/8, 0x0f000000);
			}
		}

		ty = th;
	}
}

void GSLocalMemory::SwizzleTexture4HH(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG)
{
	if(TRXREG.RRW == 0) return;

	int tw = TRXREG.RRW, srcpitch = (TRXREG.RRW - TRXPOS.DSAX)/2;
	int th = len / srcpitch;

	bool fTopLeftAligned = IsTopLeftAligned(TRXPOS.DSAX, tx, ty, 8, 8);

	if(!fTopLeftAligned || (tw & 7) || (th & 7) || (len % srcpitch))
	{
		// TODO

		SwizzleTextureX(tx, ty, src, len, BITBLTBUF, TRXPOS, TRXREG);
	}
	else
	{
		__declspec(align(16)) DWORD block[8*8];

		th += ty;

		for(int y = ty; y < th; y += 8, src += srcpitch*8)
		{
			for(int x = tx; x < tw; x += 8)
			{
				BYTE* s = src + (x - tx)/2;
				DWORD* d = block;

				for(int j = 0; j < 8; j++, s += srcpitch, d += 8)
					for(int i = 0; i < 8/2; i++)
						d[i*2] = (s[i]&0x0f) << 28, 
						d[i*2+1] = (s[i]&0xf0) << 24;

				SwizzleBlock32((BYTE*)&m_vm32[blockAddress32(x, y, BITBLTBUF.DBP, BITBLTBUF.DBW)], (BYTE*)block, sizeof(block)/8, 0xf0000000);
			}
		}

		ty = th;
	}
}

void GSLocalMemory::SwizzleTextureX(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG)
{
	if(len <= 0) return;

	BYTE* pb = (BYTE*)src;
	WORD* pw = (WORD*)src;
	DWORD* pd = (DWORD*)src;

	// if(ty >= (int)TRXREG.RRH) {ASSERT(0); return;}

	switch(BITBLTBUF.DPSM)
	{
	case PSM_PSMCT32:
		for(len /= 4; len-- > 0; SwizzleTextureStep(tx, ty, TRXPOS, TRXREG), pd++)
			writePixel32(tx, ty, *pd, BITBLTBUF.DBP, BITBLTBUF.DBW);
		break;
	case PSM_PSMCT24:
		for(len /= 3; len-- > 0; SwizzleTextureStep(tx, ty, TRXPOS, TRXREG), pb+=3)
			writePixel24(tx, ty, *(DWORD*)pb, BITBLTBUF.DBP, BITBLTBUF.DBW);
		break;
	case PSM_PSMCT16:
		for(len /= 2; len-- > 0; SwizzleTextureStep(tx, ty, TRXPOS, TRXREG), pw++)
			writePixel16(tx, ty, *pw, BITBLTBUF.DBP, BITBLTBUF.DBW);
		break;
	case PSM_PSMCT16S:
		for(len /= 2; len-- > 0; SwizzleTextureStep(tx, ty, TRXPOS, TRXREG), pw++)
			writePixel16S(tx, ty, *pw, BITBLTBUF.DBP, BITBLTBUF.DBW);
		break;
	case PSM_PSMT8:
		for(; len-- > 0; SwizzleTextureStep(tx, ty, TRXPOS, TRXREG), pb++)
			writePixel8(tx, ty, *pb, BITBLTBUF.DBP, BITBLTBUF.DBW);
		break;
	case PSM_PSMT4:
		for(; len-- > 0; SwizzleTextureStep(tx, ty, TRXPOS, TRXREG), SwizzleTextureStep(tx, ty, TRXPOS, TRXREG), pb++)
			writePixel4(tx, ty, *pb&0xf, BITBLTBUF.DBP, BITBLTBUF.DBW),
			writePixel4(tx+1, ty, *pb>>4, BITBLTBUF.DBP, BITBLTBUF.DBW);
		break;
	case PSM_PSMT8H:
		for(; len-- > 0; SwizzleTextureStep(tx, ty, TRXPOS, TRXREG), pb++)
			writePixel8H(tx, ty, *pb, BITBLTBUF.DBP, BITBLTBUF.DBW);
		break;
	case PSM_PSMT4HL:
		for(; len-- > 0; SwizzleTextureStep(tx, ty, TRXPOS, TRXREG), SwizzleTextureStep(tx, ty, TRXPOS, TRXREG), pb++)
			writePixel4HL(tx, ty, *pb&0xf, BITBLTBUF.DBP, BITBLTBUF.DBW),
			writePixel4HL(tx+1, ty, *pb>>4, BITBLTBUF.DBP, BITBLTBUF.DBW);
		break;
	case PSM_PSMT4HH:
		for(; len-- > 0; SwizzleTextureStep(tx, ty, TRXPOS, TRXREG), SwizzleTextureStep(tx, ty, TRXPOS, TRXREG), pb++)
			writePixel4HH(tx, ty, *pb&0xf, BITBLTBUF.DBP, BITBLTBUF.DBW),
			writePixel4HH(tx+1, ty, *pb>>4, BITBLTBUF.DBP, BITBLTBUF.DBW);
		break;
	case PSM_PSMZ32:
		for(len /= 4; len-- > 0; SwizzleTextureStep(tx, ty, TRXPOS, TRXREG), pd++)
			writePixel32Z(tx, ty, *pd, BITBLTBUF.DBP, BITBLTBUF.DBW);
		break;
	case PSM_PSMZ24:
		for(len /= 3; len-- > 0; SwizzleTextureStep(tx, ty, TRXPOS, TRXREG), pb+=3)
			writePixel24Z(tx, ty, *(DWORD*)pb, BITBLTBUF.DBP, BITBLTBUF.DBW);
		break;
	case PSM_PSMZ16:
		for(len /= 2; len-- > 0; SwizzleTextureStep(tx, ty, TRXPOS, TRXREG), pw++)
			writePixel16Z(tx, ty, *pw, BITBLTBUF.DBP, BITBLTBUF.DBW);
		break;
	case PSM_PSMZ16S:
		for(len /= 2; len-- > 0; SwizzleTextureStep(tx, ty, TRXPOS, TRXREG), pw++)
			writePixel16SZ(tx, ty, *pw, BITBLTBUF.DBP, BITBLTBUF.DBW);
		break;
	}
}

///////////////////

void GSLocalMemory::unSwizzleTexture32(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	#pragma omp parallel
	{
		#pragma omp for
		FOREACH_BLOCK_START(r, 8, 8, 32)
		{
			unSwizzleBlock32((BYTE*)&m_vm32[blockAddress32(x, y, TEX0.TBP0, TEX0.TBW)], ptr + (x-r.left)*4, dstpitch);
		}
		FOREACH_BLOCK_END
	}
}

void GSLocalMemory::unSwizzleTexture24(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	#pragma omp parallel
	{
		#pragma omp for
		FOREACH_BLOCK_START(r, 8, 8, 32)
		{
			__declspec(align(16)) DWORD block[8*8];
			unSwizzleBlock32((BYTE*)&m_vm32[blockAddress32(x, y, TEX0.TBP0, TEX0.TBW)], (BYTE*)block, sizeof(block)/8);
			ExpandBlock24(block, (DWORD*)ptr + (x-r.left), dstpitch, &TEXA);
		}
		FOREACH_BLOCK_END
	}
}

void GSLocalMemory::unSwizzleTexture16(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	#pragma omp parallel
	{
		#pragma omp for
		FOREACH_BLOCK_START(r, 16, 8, 16)
		{
			__declspec(align(16)) WORD block[16*8];
			unSwizzleBlock16((BYTE*)&m_vm16[blockAddress16(x, y, TEX0.TBP0, TEX0.TBW)], (BYTE*)block, sizeof(block)/8);
			ExpandBlock16(block, (DWORD*)ptr + (x-r.left), dstpitch, &TEXA);
		}
		FOREACH_BLOCK_END
	}
}

void GSLocalMemory::unSwizzleTexture16S(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	#pragma omp parallel
	{
		#pragma omp for
		FOREACH_BLOCK_START(r, 16, 8, 16S)
		{
			__declspec(align(16)) WORD block[16*8];
			unSwizzleBlock16((BYTE*)&m_vm16[blockAddress16S(x, y, TEX0.TBP0, TEX0.TBW)], (BYTE*)block, sizeof(block)/8);
			ExpandBlock16(block, (DWORD*)ptr + (x-r.left), dstpitch, &TEXA);
		}
		FOREACH_BLOCK_END
	}
}

void GSLocalMemory::unSwizzleTexture8(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	#pragma omp parallel
	{
		#pragma omp for
		FOREACH_BLOCK_START(r, 16, 16, 8)
		{
			__declspec(align(16)) BYTE block[16*16];
			unSwizzleBlock8((BYTE*)&m_vm8[blockAddress8(x, y, TEX0.TBP0, TEX0.TBW)], (BYTE*)block, sizeof(block)/16);

			BYTE* s = block;
			BYTE* d = ptr + (x-r.left)*4;

			for(int j = 0; j < 16; j++, s += 16, d += dstpitch)
				for(int i = 0; i < 16; i++)
					((DWORD*)d)[i] = m_pCLUT32[s[i]];
		}
		FOREACH_BLOCK_END
	}
}

void GSLocalMemory::unSwizzleTexture8H(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	#pragma omp parallel
	{
		#pragma omp for
		FOREACH_BLOCK_START(r, 8, 8, 32)
		{
			__declspec(align(16)) DWORD block[8*8];
			unSwizzleBlock32((BYTE*)&m_vm32[blockAddress32(x, y, TEX0.TBP0, TEX0.TBW)], (BYTE*)block, sizeof(block)/8);

			DWORD* s = block;
			BYTE* d = ptr + (x-r.left)*4;

			for(int j = 0; j < 8; j++, s += 8, d += dstpitch)
				for(int i = 0; i < 8; i++)
					((DWORD*)d)[i] = m_pCLUT32[s[i] >> 24];
		}
		FOREACH_BLOCK_END
	}
}

void GSLocalMemory::unSwizzleTexture4(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	#pragma omp parallel
	{
		#pragma omp for
		FOREACH_BLOCK_START(r, 32, 16, 4)
		{
			__declspec(align(16)) BYTE block[(32/2)*16];
			unSwizzleBlock4((BYTE*)&m_vm8[blockAddress4(x, y, TEX0.TBP0, TEX0.TBW)>>1], (BYTE*)block, sizeof(block)/16);

			BYTE* s = block;
			BYTE* d = ptr + (x-r.left)*4;

			for(int j = 0; j < 16; j++, s += 32/2, d += dstpitch)
				for(int i = 0; i < 32/2; i++)
					((UINT64*)d)[i] = m_pCLUT64[s[i]];
		}
		FOREACH_BLOCK_END
	}
}

void GSLocalMemory::unSwizzleTexture4HL(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	#pragma omp parallel
	{
		#pragma omp for
		FOREACH_BLOCK_START(r, 8, 8, 32)
		{
			__declspec(align(16)) DWORD block[8*8];
			unSwizzleBlock32((BYTE*)&m_vm32[blockAddress32(x, y, TEX0.TBP0, TEX0.TBW)], (BYTE*)block, sizeof(block)/8);

			DWORD* s = block;
			BYTE* d = ptr + (x-r.left)*4;

			for(int j = 0; j < 8; j++, s += 8, d += dstpitch)
				for(int i = 0; i < 8; i++)
					((DWORD*)d)[i] = m_pCLUT32[(s[i] >> 24)&0x0f];
		}
		FOREACH_BLOCK_END
	}
}

void GSLocalMemory::unSwizzleTexture4HH(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	#pragma omp parallel
	{
		#pragma omp for
		FOREACH_BLOCK_START(r, 8, 8, 32)
		{
			__declspec(align(16)) DWORD block[8*8];
			unSwizzleBlock32((BYTE*)&m_vm32[blockAddress32(x, y, TEX0.TBP0, TEX0.TBW)], (BYTE*)block, sizeof(block)/8);

			DWORD* s = block;
			BYTE* d = ptr + (x-r.left)*4;

			for(int j = 0; j < 8; j++, s += 8, d += dstpitch)
				for(int i = 0; i < 8; i++)
					((DWORD*)d)[i] = m_pCLUT32[s[i] >> 28];
		}
		FOREACH_BLOCK_END
	}
}

///////////////////

void GSLocalMemory::ReadTexture(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA, GIFRegCLAMP& CLAMP)
{
	unSwizzleTexture st = m_psmtbl[TEX0.PSM].ust;
	readTexel rt = m_psmtbl[TEX0.PSM].rt;
	CSize bs = m_psmtbl[TEX0.PSM].bs;

	if(r.Width() < bs.cx || r.Height() < bs.cy 
	|| (r.left & (bs.cx-1)) || (r.top & (bs.cy-1)) 
	|| (r.right & (bs.cx-1)) || (r.bottom & (bs.cy-1)) 
	|| (CLAMP.WMS&2) || (CLAMP.WMT&2))
	{
		ReadTexture<DWORD>(r, dst, dstpitch, TEX0, TEXA, CLAMP, rt, st);
	}
	else
	{
		(this->*st)(r, dst, dstpitch, TEX0, TEXA);
	}
}

////////////////////

DWORD GSLocalMemory::readTexel16P(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return readPixel16(x, y, TEX0.TBP0, TEX0.TBW);
}

DWORD GSLocalMemory::readTexel16SP(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return readPixel16S(x, y, TEX0.TBP0, TEX0.TBW);
}

DWORD GSLocalMemory::readTexel8P(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return readPixel8(x, y, TEX0.TBP0, TEX0.TBW);
}

DWORD GSLocalMemory::readTexel8HP(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return readPixel8H(x, y, TEX0.TBP0, TEX0.TBW);
}

DWORD GSLocalMemory::readTexel4P(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return readPixel4(x, y, TEX0.TBP0, TEX0.TBW);
}

DWORD GSLocalMemory::readTexel4HLP(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return readPixel4HL(x, y, TEX0.TBP0, TEX0.TBW);
}

DWORD GSLocalMemory::readTexel4HHP(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return readPixel4HH(x, y, TEX0.TBP0, TEX0.TBW);
}

///////////////////

void GSLocalMemory::unSwizzleTexture16P(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	#pragma omp parallel
	{
		#pragma omp for
		FOREACH_BLOCK_START(r, 16, 8, 16)
		{
			unSwizzleBlock16((BYTE*)&m_vm16[blockAddress16(x, y, TEX0.TBP0, TEX0.TBW)], ptr + (x-r.left)*2, dstpitch);
		}
		FOREACH_BLOCK_END
	}
}

void GSLocalMemory::unSwizzleTexture16SP(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	#pragma omp parallel
	{
		#pragma omp for
		FOREACH_BLOCK_START(r, 16, 8, 16S)
		{
			unSwizzleBlock16((BYTE*)&m_vm16[blockAddress16S(x, y, TEX0.TBP0, TEX0.TBW)], ptr + (x-r.left)*2, dstpitch);
		}
		FOREACH_BLOCK_END
	}
}

void GSLocalMemory::unSwizzleTexture8P(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	#pragma omp parallel
	{
		#pragma omp for
		FOREACH_BLOCK_START(r, 16, 16, 8)
		{
			unSwizzleBlock8((BYTE*)&m_vm8[blockAddress8(x, y, TEX0.TBP0, TEX0.TBW)], ptr + (x-r.left), dstpitch);
		}
		FOREACH_BLOCK_END
	}
}

void GSLocalMemory::unSwizzleTexture8HP(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	#pragma omp parallel
	{
		#pragma omp for
		FOREACH_BLOCK_START(r, 8, 8, 32)
		{
			unSwizzleBlock8HP((BYTE*)&m_vm32[blockAddress32(x, y, TEX0.TBP0, TEX0.TBW)], ptr + (x-r.left), dstpitch);
		}
		FOREACH_BLOCK_END
	}
}

void GSLocalMemory::unSwizzleTexture4P(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	#pragma omp parallel
	{
		#pragma omp for
		FOREACH_BLOCK_START(r, 32, 16, 4)
		{
			unSwizzleBlock4P((BYTE*)&m_vm8[blockAddress4(x, y, TEX0.TBP0, TEX0.TBW)>>1], ptr + (x-r.left), dstpitch);
		}
		FOREACH_BLOCK_END
	}
}

void GSLocalMemory::unSwizzleTexture4HLP(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	#pragma omp parallel
	{
		#pragma omp for
		FOREACH_BLOCK_START(r, 8, 8, 32)
		{
			unSwizzleBlock4HLP((BYTE*)&m_vm32[blockAddress32(x, y, TEX0.TBP0, TEX0.TBW)], ptr + (x-r.left), dstpitch);
		}
		FOREACH_BLOCK_END
	}
}

void GSLocalMemory::unSwizzleTexture4HHP(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	#pragma omp parallel
	{
		#pragma omp for
		FOREACH_BLOCK_START(r, 8, 8, 32)
		{
			unSwizzleBlock4HHP((BYTE*)&m_vm32[blockAddress32(x, y, TEX0.TBP0, TEX0.TBW)], ptr + (x-r.left), dstpitch);
		}
		FOREACH_BLOCK_END
	}
}

///////////////////

void GSLocalMemory::ReadTextureP(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA, GIFRegCLAMP& CLAMP)
{
	unSwizzleTexture st = m_psmtbl[TEX0.PSM].ustP;
	readTexel rt = m_psmtbl[TEX0.PSM].rtP;
	CSize bs = m_psmtbl[TEX0.PSM].bs;

	if(r.Width() < bs.cx || r.Height() < bs.cy 
	|| (r.left & (bs.cx-1)) || (r.top & (bs.cy-1)) 
	|| (r.right & (bs.cx-1)) || (r.bottom & (bs.cy-1)) 
	|| (CLAMP.WMS&2) || (CLAMP.WMT&2))
	{
		switch(TEX0.PSM)
		{
		default:
		case PSM_PSMCT32:
		case PSM_PSMCT24:
			ReadTexture<DWORD>(r, dst, dstpitch, TEX0, TEXA, CLAMP, rt, st);
			break;
		case PSM_PSMCT16:
		case PSM_PSMCT16S:
			ReadTexture<WORD>(r, dst, dstpitch, TEX0, TEXA, CLAMP, rt, st);
			break;
		case PSM_PSMT8:
		case PSM_PSMT8H:
		case PSM_PSMT4:
		case PSM_PSMT4HL:
		case PSM_PSMT4HH:
			ReadTexture<BYTE>(r, dst, dstpitch, TEX0, TEXA, CLAMP, rt, st);
			break;
		}
	}
	else
	{
		(this->*st)(r, dst, dstpitch, TEX0, TEXA);
	}
}

///////////////////

void GSLocalMemory::unSwizzleTexture8NP(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	#pragma omp parallel
	{
		#pragma omp for
		FOREACH_BLOCK_START(r, 16, 16, 8)
		{
			__declspec(align(16)) BYTE block[16*16];
			unSwizzleBlock8((BYTE*)&m_vm8[blockAddress8(x, y, TEX0.TBP0, TEX0.TBW)], (BYTE*)block, sizeof(block)/16);

			BYTE* s = block;

			if(TEX0.CPSM == PSM_PSMCT32)
			{
				BYTE* d = ptr + (x-r.left)*4;
				for(int j = 0; j < 16; j++, s += 16, d += dstpitch)
					for(int i = 0; i < 16; i++)
						((DWORD*)d)[i] = m_pCLUT32[s[i]];
			}
			else
			{
				BYTE* d = ptr + (x-r.left)*2;
				for(int j = 0; j < 16; j++, s += 16, d += dstpitch)
					for(int i = 0; i < 16; i++)
						((WORD*)d)[i] = (WORD)m_pCLUT32[s[i]];
			}
		}
		FOREACH_BLOCK_END
	}
}

void GSLocalMemory::unSwizzleTexture8HNP(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	#pragma omp parallel
	{
		#pragma omp for
		FOREACH_BLOCK_START(r, 8, 8, 32)
		{
			__declspec(align(16)) DWORD block[8*8];
			unSwizzleBlock32((BYTE*)&m_vm32[blockAddress32(x, y, TEX0.TBP0, TEX0.TBW)], (BYTE*)block, sizeof(block)/8);

			DWORD* s = block;

			if(TEX0.CPSM == PSM_PSMCT32)
			{
				BYTE* d = ptr + (x-r.left)*4;
				for(int j = 0; j < 8; j++, s += 8, d += dstpitch)
					for(int i = 0; i < 8; i++)
						((DWORD*)d)[i] = m_pCLUT32[s[i] >> 24];
			}
			else
			{
				BYTE* d = ptr + (x-r.left)*2;
				for(int j = 0; j < 8; j++, s += 8, d += dstpitch)
					for(int i = 0; i < 8; i++)
						((WORD*)d)[i] = (WORD)m_pCLUT32[s[i] >> 24];
			}
		}
		FOREACH_BLOCK_END
	}
}

void GSLocalMemory::unSwizzleTexture4NP(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	#pragma omp parallel
	{
		#pragma omp for
		FOREACH_BLOCK_START(r, 32, 16, 4)
		{
			__declspec(align(16)) BYTE block[(32/2)*16];
			unSwizzleBlock4((BYTE*)&m_vm8[blockAddress4(x, y, TEX0.TBP0, TEX0.TBW)>>1], (BYTE*)block, sizeof(block)/16);

			BYTE* s = block;

			if(TEX0.CPSM == PSM_PSMCT32)
			{
				BYTE* d = ptr + (x-r.left)*4;
				for(int j = 0; j < 16; j++, s += 32/2, d += dstpitch)
					for(int i = 0; i < 32/2; i++)
						((UINT64*)d)[i] = m_pCLUT64[s[i]];
			}
			else
			{
				BYTE* d = ptr + (x-r.left)*2;
				for(int j = 0; j < 16; j++, s += 32/2, d += dstpitch)
					for(int i = 0; i < 32/2; i++)
						((DWORD*)d)[i] = (DWORD)m_pCLUT64[s[i]];
			}
		}
		FOREACH_BLOCK_END
	}
}

void GSLocalMemory::unSwizzleTexture4HLNP(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	#pragma omp parallel
	{
		#pragma omp for
		FOREACH_BLOCK_START(r, 8, 8, 32)
		{
			__declspec(align(16)) DWORD block[8*8];
			unSwizzleBlock32((BYTE*)&m_vm32[blockAddress32(x, y, TEX0.TBP0, TEX0.TBW)], (BYTE*)block, sizeof(block)/8);

			DWORD* s = block;

			if(TEX0.CPSM == PSM_PSMCT32)
			{
				BYTE* d = ptr + (x-r.left)*4;
				for(int j = 0; j < 8; j++, s += 8, d += dstpitch)
					for(int i = 0; i < 8; i++)
						((DWORD*)d)[i] = m_pCLUT32[(s[i] >> 24)&0x0f];
			}
			else
			{
				BYTE* d = ptr + (x-r.left)*2;
				for(int j = 0; j < 8; j++, s += 8, d += dstpitch)
					for(int i = 0; i < 8; i++)
						((WORD*)d)[i] = (WORD)m_pCLUT32[(s[i] >> 24)&0x0f];
			}
		}
		FOREACH_BLOCK_END
	}
}

void GSLocalMemory::unSwizzleTexture4HHNP(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	#pragma omp parallel
	{
		#pragma omp for
		FOREACH_BLOCK_START(r, 8, 8, 32)
		{
			__declspec(align(16)) DWORD block[8*8];
			unSwizzleBlock32((BYTE*)&m_vm32[blockAddress32(x, y, TEX0.TBP0, TEX0.TBW)], (BYTE*)block, sizeof(block)/8);

			DWORD* s = block;

			if(TEX0.CPSM == PSM_PSMCT32)
			{
				BYTE* d = ptr + (x-r.left)*4;
				for(int j = 0; j < 8; j++, s += 8, d += dstpitch)
					for(int i = 0; i < 8; i++)
						((DWORD*)d)[i] = m_pCLUT32[s[i] >> 28];
			}
			else
			{
				BYTE* d = ptr + (x-r.left)*2;
				for(int j = 0; j < 8; j++, s += 8, d += dstpitch)
					for(int i = 0; i < 8; i++)
						((WORD*)d)[i] = (WORD)m_pCLUT32[s[i] >> 28];
			}
		}
		FOREACH_BLOCK_END
	}
}

///////////////////

void GSLocalMemory::ReadTextureNP(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA, GIFRegCLAMP& CLAMP)
{
	unSwizzleTexture st = m_psmtbl[TEX0.PSM].ustNP;
	readTexel rt = m_psmtbl[TEX0.PSM].rtNP;

	CSize bs = m_psmtbl[TEX0.PSM].bs;

	if(r.Width() < bs.cx || r.Height() < bs.cy 
	|| (r.left & (bs.cx-1)) || (r.top & (bs.cy-1)) 
	|| (r.right & (bs.cx-1)) || (r.bottom & (bs.cy-1)) 
	|| (CLAMP.WMS&2) || (CLAMP.WMT&2))
	{
		switch(TEX0.PSM)
		{
		default:
		case PSM_PSMCT32:
		case PSM_PSMCT24:
			ReadTexture<DWORD>(r, dst, dstpitch, TEX0, TEXA, CLAMP, rt, st);
			break;
		case PSM_PSMCT16:
		case PSM_PSMCT16S:
			ReadTexture<WORD>(r, dst, dstpitch, TEX0, TEXA, CLAMP, rt, st);
			break;
		case PSM_PSMT8:
		case PSM_PSMT8H:
		case PSM_PSMT4:
		case PSM_PSMT4HL:
		case PSM_PSMT4HH:
			switch(TEX0.CPSM)
			{
			default:
			case PSM_PSMCT32:
				ReadTexture<DWORD>(r, dst, dstpitch, TEX0, TEXA, CLAMP, rt, st);
				break;
			case PSM_PSMCT16:
			case PSM_PSMCT16S:
				ReadTexture<WORD>(r, dst, dstpitch, TEX0, TEXA, CLAMP, rt, st);
				break;
			}
			break;
		}
	}
	else
	{
		(this->*st)(r, dst, dstpitch, TEX0, TEXA);
	}
}

//

template<typename DstT> 
void GSLocalMemory::ReadTexture(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA, GIFRegCLAMP& CLAMP, readTexel rt, unSwizzleTexture st)
{
	// this function is not thread safe!

	CSize bs = m_psmtbl[TEX0.PSM].bs;

	CRect cr(
		(r.left + (bs.cx-1)) & ~(bs.cx-1), 
		(r.top + (bs.cy-1)) & ~(bs.cy-1), 
		r.right & ~(bs.cx-1), 
		r.bottom & ~(bs.cy-1));

	bool fAligned = ((DWORD_PTR)(dst + (cr.left-r.left)*sizeof(DstT)) & 0xf) == 0;

	if((CLAMP.WMS&2) || (CLAMP.WMT&2))
	{
		DWORD wms = CLAMP.WMS, wmt = CLAMP.WMT;
		DWORD minu = CLAMP.MINU, maxu = CLAMP.MAXU;
		DWORD minv = CLAMP.MINV, maxv = CLAMP.MAXV;

		switch(wms)
		{
		default: for(int x = r.left; x < r.right; x++) m_xtbl[x] = x; break;
		case 2: for(int x = r.left; x < r.right; x++) m_xtbl[x] = x < minu ? minu : x > maxu ? maxu : x; break;
		case 3: for(int x = r.left; x < r.right; x++) m_xtbl[x] = (x & minu) | maxu; break;
		}

		switch(wmt)
		{
		default: for(int y = r.top; y < r.bottom; y++) m_ytbl[y] = y; break;
		case 2: for(int y = r.top; y < r.bottom; y++) m_ytbl[y] = y < minv ? minv : y > maxv ? maxv : y;  break;
		case 3: for(int y = r.top; y < r.bottom; y++) m_ytbl[y] = (y & minv) | maxv; break;
		}

		if(fAligned && wms <= 2 && wmt <= 2)
		{
			// TODO: read clamped areas only once

			for(int y = r.top; y < cr.top; y++, dst += dstpitch)
				for(int x = r.left, i = 0; x < r.right; x++, i++)
					((DstT*)dst)[i] = (DstT)(this->*rt)(m_xtbl[x], m_ytbl[y], TEX0, TEXA);

			if(!cr.IsRectEmpty())
			(this->*st)(cr, dst + (cr.left - r.left)*sizeof(DstT), dstpitch, TEX0, TEXA);

			for(int y = cr.top; y < cr.bottom; y++, dst += dstpitch)
			{
				for(int x = r.left, i = 0; x < cr.left; x++, i++)
					((DstT*)dst)[i] = (DstT)(this->*rt)(m_xtbl[x], m_ytbl[y], TEX0, TEXA);
				for(int x = cr.right, i = x - r.left; x < r.right; x++, i++)
					((DstT*)dst)[i] = (DstT)(this->*rt)(m_xtbl[x], m_ytbl[y], TEX0, TEXA);
			}

			for(int y = cr.bottom; y < r.bottom; y++, dst += dstpitch)
				for(int x = r.left, i = 0; x < r.right; x++, i++)
					((DstT*)dst)[i] = (DstT)(this->*rt)(m_xtbl[x], m_ytbl[y], TEX0, TEXA);
		}
		else
		{
			for(int y = r.top; y < r.bottom; y++, dst += dstpitch)
				for(int x = r.left, i = 0; x < r.right; x++, i++)
					((DstT*)dst)[i] = (DstT)(this->*rt)(m_xtbl[x], m_ytbl[y], TEX0, TEXA);
		}
	}
	else
	{
		if(fAligned)
		{
			for(int y = r.top; y < cr.top; y++, dst += dstpitch)
				for(int x = r.left, i = 0; x < r.right; x++, i++)
					((DstT*)dst)[i] = (DstT)(this->*rt)(x, y, TEX0, TEXA);

			if(!cr.IsRectEmpty())
			(this->*st)(cr, dst + (cr.left - r.left)*sizeof(DstT), dstpitch, TEX0, TEXA);

			for(int y = cr.top; y < cr.bottom; y++, dst += dstpitch)
			{
				for(int x = r.left, i = 0; x < cr.left; x++, i++)
					((DstT*)dst)[i] = (DstT)(this->*rt)(x, y, TEX0, TEXA);
				for(int x = cr.right, i = x - r.left; x < r.right; x++, i++)
					((DstT*)dst)[i] = (DstT)(this->*rt)(x, y, TEX0, TEXA);
			}

			for(int y = cr.bottom; y < r.bottom; y++, dst += dstpitch)
				for(int x = r.left, i = 0; x < r.right; x++, i++)
					((DstT*)dst)[i] = (DstT)(this->*rt)(x, y, TEX0, TEXA);
		}
		else
		{
			for(int y = r.top; y < r.bottom; y++, dst += dstpitch)
				for(int x = r.left, i = 0; x < r.right; x++, i++)
					((DstT*)dst)[i] = (DstT)(this->*rt)(x, y, TEX0, TEXA);
		}
	}
}

//

HRESULT GSLocalMemory::SaveBMP(IDirect3DDevice9* pDev, LPCTSTR fn, DWORD bp, DWORD bw, DWORD psm, int w, int h)
{
	CComPtr<IDirect3DTexture9> pTexture;
	D3DLOCKED_RECT lr;
	HRESULT hr = pDev->CreateTexture(w, h, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &pTexture, NULL);
	if(FAILED(hr) || FAILED(pTexture->LockRect(0, &lr, NULL, 0))) return E_FAIL;

	readTexel rt = m_psmtbl[psm].rt;
	GIFRegTEX0 TEX0;
	TEX0.TBP0 = bp;
	TEX0.TBW = bw;
	TEX0.PSM = psm;
	GIFRegTEXA TEXA;
	TEXA.AEM = 0;
	TEXA.TA0 = 0;
	TEXA.TA1 = 0x80;

	BYTE* p = (BYTE*)lr.pBits;

	for(int j = 0; j < h; j++, p += lr.Pitch)
		for(int i = 0; i < w; i++)
			((DWORD*)p)[i] = (this->*rt)(i, j, TEX0, TEXA);

	pTexture->UnlockRect(0);

	return D3DXSaveTextureToFile(fn, D3DXIFF_BMP, pTexture, NULL);
}
