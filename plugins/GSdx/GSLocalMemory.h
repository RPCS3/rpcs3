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

#pragma once

#pragma warning(disable: 4100) // warning C4100: 'TEXA' : unreferenced formal parameter

#include "GS.h"
#include "GSTables.h"
#include "GSVector.h"
#include "GSBlock.h"
#include "GSClut.h"

class GSLocalMemory : public GSBlock
{
public:
	typedef DWORD (*pixelAddress)(int x, int y, DWORD bp, DWORD bw);
	typedef void (GSLocalMemory::*writePixel)(int x, int y, DWORD c, DWORD bp, DWORD bw);
	typedef void (GSLocalMemory::*writeFrame)(int x, int y, DWORD c, DWORD bp, DWORD bw);
	typedef DWORD (GSLocalMemory::*readPixel)(int x, int y, DWORD bp, DWORD bw) const;
	typedef DWORD (GSLocalMemory::*readTexel)(int x, int y, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const;
	typedef void (GSLocalMemory::*writePixelAddr)(DWORD addr, DWORD c);
	typedef void (GSLocalMemory::*writeFrameAddr)(DWORD addr, DWORD c);
	typedef DWORD (GSLocalMemory::*readPixelAddr)(DWORD addr) const;
	typedef DWORD (GSLocalMemory::*readTexelAddr)(DWORD addr, const GIFRegTEXA& TEXA) const;
	typedef void (GSLocalMemory::*writeImage)(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG);
	typedef void (GSLocalMemory::*readImage)(int& tx, int& ty, BYTE* dst, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG) const;
	typedef void (GSLocalMemory::*readTexture)(const CRect& r, BYTE* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const;

	typedef union 
	{
		struct
		{
			pixelAddress pa, ba, pga, pgn;
			readPixel rp;
			readPixelAddr rpa;
			writePixel wp;
			writePixelAddr wpa;
			readTexel rt, rtNP;
			readTexelAddr rta;
			writeFrameAddr wfa;
			writeImage wi;
			readImage ri;
			readTexture rtx, rtxNP, rtxP;
			DWORD bpp, pal, trbpp; 
			CSize bs, pgs;
			int* rowOffset[8];
		};
		BYTE dummy[128];
	} psm_t;

	static psm_t m_psm[64];

	static const int m_vmsize = 1024 * 1024 * 4;

	union {BYTE* m_vm8; WORD* m_vm16; DWORD* m_vm32;};

	GSClut m_clut;

	struct Offset
	{
		GSVector4i row[2048]; // 0 | 0 | 0 | 0
		int* col[4]; // x | x+1 | x+2 | x+3
		DWORD hash;
	};

	struct Offset4
	{
		// 16 bit offsets (m_vm16[...])

		GSVector2i row[2048]; // f yn | z yn (n = 0 1 2 ...)
		GSVector2i col[512]; // f xn | z xn (n = 0 4 8 ...)
		DWORD hash;
	};

protected:
	static DWORD pageOffset32[32][32][64];
	static DWORD pageOffset32Z[32][32][64];
	static DWORD pageOffset16[32][64][64];
	static DWORD pageOffset16S[32][64][64];
	static DWORD pageOffset16Z[32][64][64];
	static DWORD pageOffset16SZ[32][64][64];
	static DWORD pageOffset8[32][64][128];
	static DWORD pageOffset4[32][128][128];

	static int rowOffset32[2048];
	static int rowOffset32Z[2048];
	static int rowOffset16[2048];
	static int rowOffset16S[2048];
	static int rowOffset16Z[2048];
	static int rowOffset16SZ[2048];
	static int rowOffset8[2][2048];
	static int rowOffset4[2][2048];

	__forceinline static DWORD Expand24To32(DWORD c, const GIFRegTEXA& TEXA)
	{
		return (((!TEXA.AEM | (c & 0xffffff)) ? TEXA.TA0 : 0) << 24) | (c & 0xffffff);
	}

	__forceinline static DWORD Expand16To32(WORD c, const GIFRegTEXA& TEXA)
	{
		return (((c & 0x8000) ? TEXA.TA1 : (!TEXA.AEM | c) ? TEXA.TA0 : 0) << 24) | ((c & 0x7c00) << 9) | ((c & 0x03e0) << 6) | ((c & 0x001f) << 3);
	}

	// TODO

	friend class GSClut;

	//

	CRBMapC<DWORD, Offset*> m_omap;
	CRBMapC<DWORD, Offset4*> m_o4map;

public:
	GSLocalMemory();
	virtual ~GSLocalMemory();

	Offset* GetOffset(DWORD bp, DWORD bw, DWORD psm);
	Offset4* GetOffset4(const GIFRegFRAME& FRAME, const GIFRegZBUF& ZBUF);

	// address

	static DWORD PageNumber32(int x, int y, DWORD bp, DWORD bw)
	{
		return (bp >> 5) + (y >> 5) * bw + (x >> 6); 
	}

	static DWORD PageNumber16(int x, int y, DWORD bp, DWORD bw)
	{
		return (bp >> 5) + (y >> 6) * bw + (x >> 6);
	}

	static DWORD PageNumber8(int x, int y, DWORD bp, DWORD bw)
	{
		ASSERT((bw & 1) == 0);

		return (bp >> 5) + (y >> 6) * (bw >> 1) + (x >> 7); 
	}

	static DWORD PageNumber4(int x, int y, DWORD bp, DWORD bw)
	{
		ASSERT((bw & 1) == 0);

		return (bp >> 5) + (y >> 7) * (bw >> 1) + (x >> 7);
	}

	static DWORD PageAddress32(int x, int y, DWORD bp, DWORD bw)
	{
		return PageNumber32(x, y, bp, bw) << 11; 
	}

	static DWORD PageAddress16(int x, int y, DWORD bp, DWORD bw)
	{
		return PageNumber16(x, y, bp, bw) << 12;
	}

	static DWORD PageAddress8(int x, int y, DWORD bp, DWORD bw)
	{
		return PageNumber8(x, y, bp, bw) << 13; 
	}

	static DWORD PageAddress4(int x, int y, DWORD bp, DWORD bw)
	{
		return PageNumber4(x, y, bp, bw) << 14;
	}

	static DWORD BlockAddress32(int x, int y, DWORD bp, DWORD bw)
	{
		DWORD page = bp + (y & ~0x1f) * bw + ((x >> 1) & ~0x1f);
		DWORD block = blockTable32[(y >> 3) & 3][(x >> 3) & 7];
		return (page + block) << 6;
	}

	static DWORD BlockAddress16(int x, int y, DWORD bp, DWORD bw)
	{
		DWORD page = bp + ((y >> 1) & ~0x1f) * bw + ((x >> 1) & ~0x1f); 
		DWORD block = blockTable16[(y >> 3) & 7][(x >> 4) & 3];
		return (page + block) << 7;
	}

	static DWORD BlockAddress16S(int x, int y, DWORD bp, DWORD bw)
	{
		DWORD page = bp + ((y >> 1) & ~0x1f) * bw + ((x >> 1) & ~0x1f); 
		DWORD block = blockTable16S[(y >> 3) & 7][(x >> 4) & 3];
		return (page + block) << 7;
	}

	static DWORD BlockAddress8(int x, int y, DWORD bp, DWORD bw)
	{
		ASSERT((bw & 1) == 0);
		DWORD page = bp + ((y >> 1) & ~0x1f) * (bw >> 1) + ((x >> 2) & ~0x1f); 
		DWORD block = blockTable8[(y >> 4) & 3][(x >> 4) & 7];
		return (page + block) << 8;
	}

	static DWORD BlockAddress4(int x, int y, DWORD bp, DWORD bw)
	{
		ASSERT((bw & 1) == 0);
		DWORD page = bp + ((y >> 2) & ~0x1f) * (bw >> 1) + ((x >> 2) & ~0x1f); 
		DWORD block = blockTable4[(y >> 4) & 7][(x >> 5) & 3];
		return (page + block) << 9;
	}

	static DWORD BlockAddress32Z(int x, int y, DWORD bp, DWORD bw)
	{
		DWORD page = bp + (y & ~0x1f) * bw + ((x >> 1) & ~0x1f); 
		DWORD block = blockTable32Z[(y >> 3) & 3][(x >> 3) & 7];
		return (page + block) << 6;
	}

	static DWORD BlockAddress16Z(int x, int y, DWORD bp, DWORD bw)
	{
		DWORD page = bp + ((y >> 1) & ~0x1f) * bw + ((x >> 1) & ~0x1f); 
		DWORD block = blockTable16Z[(y >> 3) & 7][(x >> 4) & 3];
		return (page + block) << 7;
	}

	static DWORD BlockAddress16SZ(int x, int y, DWORD bp, DWORD bw)
	{
		DWORD page = bp + ((y >> 1) & ~0x1f) * bw + ((x >> 1) & ~0x1f); 
		DWORD block = blockTable16SZ[(y >> 3) & 7][(x >> 4) & 3];
		return (page + block) << 7;
	}

	static DWORD PixelAddressOrg32(int x, int y, DWORD bp, DWORD bw)
	{
		DWORD page = bp + (y & ~0x1f) * bw + ((x >> 1) & ~0x1f);
		DWORD block = blockTable32[(y >> 3) & 3][(x >> 3) & 7];
		DWORD word = ((page + block) << 6) + columnTable32[y & 7][x & 7];
		ASSERT(word < 1024*1024);
		return word;
	}

	static DWORD PixelAddressOrg16(int x, int y, DWORD bp, DWORD bw)
	{
		DWORD page = bp + ((y >> 1) & ~0x1f) * bw + ((x >> 1) & ~0x1f); 
		DWORD block = blockTable16[(y >> 3) & 7][(x >> 4) & 3];
		DWORD word = ((page + block) << 7) + columnTable16[y & 7][x & 15];
		ASSERT(word < 1024*1024*2);
		return word;
	}

	static DWORD PixelAddressOrg16S(int x, int y, DWORD bp, DWORD bw)
	{
		DWORD page = bp + ((y >> 1) & ~0x1f) * bw + ((x >> 1) & ~0x1f); 
		DWORD block = blockTable16S[(y >> 3) & 7][(x >> 4) & 3];
		DWORD word = ((page + block) << 7) + columnTable16[y & 7][x & 15];
		ASSERT(word < 1024*1024*2);
		return word;
	}

	static DWORD PixelAddressOrg8(int x, int y, DWORD bp, DWORD bw)
	{
		ASSERT((bw & 1) == 0);
		DWORD page = bp + ((y >> 1) & ~0x1f) * (bw >> 1) + ((x >> 2) & ~0x1f); 
		DWORD block = blockTable8[(y >> 4) & 3][(x >> 4) & 7];
		DWORD word = ((page + block) << 8) + columnTable8[y & 15][x & 15];
		ASSERT(word < 1024*1024*4);
		return word;
	}

	static DWORD PixelAddressOrg4(int x, int y, DWORD bp, DWORD bw)
	{
		ASSERT((bw & 1) == 0);
		DWORD page = bp + ((y >> 2) & ~0x1f) * (bw >> 1) + ((x >> 2) & ~0x1f); 
		DWORD block = blockTable4[(y >> 4) & 7][(x >> 5) & 3];
		DWORD word = ((page + block) << 9) + columnTable4[y & 15][x & 31];
		ASSERT(word < 1024*1024*8);
		return word;
	}

	static DWORD PixelAddressOrg32Z(int x, int y, DWORD bp, DWORD bw)
	{
		DWORD page = bp + (y & ~0x1f) * bw + ((x >> 1) & ~0x1f); 
		DWORD block = blockTable32Z[(y >> 3) & 3][(x >> 3) & 7];
		DWORD word = ((page + block) << 6) + columnTable32[y & 7][x & 7];
		ASSERT(word < 1024*1024);
		return word;
	}

	static DWORD PixelAddressOrg16Z(int x, int y, DWORD bp, DWORD bw)
	{
		DWORD page = bp + ((y >> 1) & ~0x1f) * bw + ((x >> 1) & ~0x1f); 
		DWORD block = blockTable16Z[(y >> 3) & 7][(x >> 4) & 3];
		DWORD word = ((page + block) << 7) + columnTable16[y & 7][x & 15];
		ASSERT(word < 1024*1024*2);
		return word;
	}

	static DWORD PixelAddressOrg16SZ(int x, int y, DWORD bp, DWORD bw)
	{
		DWORD page = bp + ((y >> 1) & ~0x1f) * bw + ((x >> 1) & ~0x1f); 
		DWORD block = blockTable16SZ[(y >> 3) & 7][(x >> 4) & 3];
		DWORD word = ((page + block) << 7) + columnTable16[y & 7][x & 15];
		ASSERT(word < 1024*1024*2);
		return word;
	}

	static __forceinline DWORD PixelAddress32(int x, int y, DWORD bp, DWORD bw)
	{
		DWORD page = (bp >> 5) + (y >> 5) * bw + (x >> 6); 
		DWORD word = (page << 11) + pageOffset32[bp & 0x1f][y & 0x1f][x & 0x3f];
		return word;
	}

	static __forceinline DWORD PixelAddress16(int x, int y, DWORD bp, DWORD bw)
	{
		DWORD page = (bp >> 5) + (y >> 6) * bw + (x >> 6); 
		DWORD word = (page << 12) + pageOffset16[bp & 0x1f][y & 0x3f][x & 0x3f];
		return word;
	}

	static __forceinline DWORD PixelAddress16S(int x, int y, DWORD bp, DWORD bw)
	{
		DWORD page = (bp >> 5) + (y >> 6) * bw + (x >> 6); 
		DWORD word = (page << 12) + pageOffset16S[bp & 0x1f][y & 0x3f][x & 0x3f];
		return word;
	}

	static __forceinline DWORD PixelAddress8(int x, int y, DWORD bp, DWORD bw)
	{
		ASSERT((bw & 1) == 0);
		DWORD page = (bp >> 5) + (y >> 6) * (bw >> 1) + (x >> 7); 
		DWORD word = (page << 13) + pageOffset8[bp & 0x1f][y & 0x3f][x & 0x7f];
		return word;
	}

	static __forceinline DWORD PixelAddress4(int x, int y, DWORD bp, DWORD bw)
	{
		ASSERT((bw & 1) == 0);
		DWORD page = (bp >> 5) + (y >> 7) * (bw >> 1) + (x >> 7);
		DWORD word = (page << 14) + pageOffset4[bp & 0x1f][y & 0x7f][x & 0x7f];
		return word;
	}

	static __forceinline DWORD PixelAddress32Z(int x, int y, DWORD bp, DWORD bw)
	{
		DWORD page = (bp >> 5) + (y >> 5) * bw + (x >> 6); 
		DWORD word = (page << 11) + pageOffset32Z[bp & 0x1f][y & 0x1f][x & 0x3f];
		return word;
	}

	static __forceinline DWORD PixelAddress16Z(int x, int y, DWORD bp, DWORD bw)
	{
		DWORD page = (bp >> 5) + (y >> 6) * bw + (x >> 6); 
		DWORD word = (page << 12) + pageOffset16Z[bp & 0x1f][y & 0x3f][x & 0x3f];
		return word;
	}

	static __forceinline DWORD PixelAddress16SZ(int x, int y, DWORD bp, DWORD bw)
	{
		DWORD page = (bp >> 5) + (y >> 6) * bw + (x >> 6); 
		DWORD word = (page << 12) + pageOffset16SZ[bp & 0x1f][y & 0x3f][x & 0x3f];
		return word;
	}

	// pixel R/W

	__forceinline DWORD ReadPixel32(DWORD addr) const
	{
		return m_vm32[addr];
	}

	__forceinline DWORD ReadPixel24(DWORD addr) const 
	{
		return m_vm32[addr] & 0x00ffffff;
	}

	__forceinline DWORD ReadPixel16(DWORD addr) const 
	{
		return (DWORD)m_vm16[addr];
	}

	__forceinline DWORD ReadPixel8(DWORD addr) const 
	{
		return (DWORD)m_vm8[addr];
	}

	__forceinline DWORD ReadPixel4(DWORD addr) const 
	{
		return (m_vm8[addr >> 1] >> ((addr & 1) << 2)) & 0x0f;
	}

	__forceinline DWORD ReadPixel8H(DWORD addr) const 
	{
		return m_vm32[addr] >> 24;
	}

	__forceinline DWORD ReadPixel4HL(DWORD addr) const 
	{
		return (m_vm32[addr] >> 24) & 0x0f;
	}

	__forceinline DWORD ReadPixel4HH(DWORD addr) const 
	{
		return (m_vm32[addr] >> 28) & 0x0f;
	}

	__forceinline DWORD ReadFrame24(DWORD addr) const 
	{
		return 0x80000000 | (m_vm32[addr] & 0xffffff);
	}

	__forceinline DWORD ReadFrame16(DWORD addr) const 
	{
		DWORD c = (DWORD)m_vm16[addr];

		return ((c & 0x8000) << 16) | ((c & 0x7c00) << 9) | ((c & 0x03e0) << 6) | ((c & 0x001f) << 3);
	}

	__forceinline DWORD ReadPixel32(int x, int y, DWORD bp, DWORD bw) const
	{
		return ReadPixel32(PixelAddress32(x, y, bp, bw));
	}

	__forceinline DWORD ReadPixel24(int x, int y, DWORD bp, DWORD bw) const
	{
		return ReadPixel24(PixelAddress32(x, y, bp, bw));
	}

	__forceinline DWORD ReadPixel16(int x, int y, DWORD bp, DWORD bw) const
	{
		return ReadPixel16(PixelAddress16(x, y, bp, bw));
	}

	__forceinline DWORD ReadPixel16S(int x, int y, DWORD bp, DWORD bw) const
	{
		return ReadPixel16(PixelAddress16S(x, y, bp, bw));
	}

	__forceinline DWORD ReadPixel8(int x, int y, DWORD bp, DWORD bw) const
	{
		return ReadPixel8(PixelAddress8(x, y, bp, bw));
	}

	__forceinline DWORD ReadPixel4(int x, int y, DWORD bp, DWORD bw) const
	{
		return ReadPixel4(PixelAddress4(x, y, bp, bw));
	}

	__forceinline DWORD ReadPixel8H(int x, int y, DWORD bp, DWORD bw) const
	{
		return ReadPixel8H(PixelAddress32(x, y, bp, bw));
	}

	__forceinline DWORD ReadPixel4HL(int x, int y, DWORD bp, DWORD bw) const
	{
		return ReadPixel4HL(PixelAddress32(x, y, bp, bw));
	}

	__forceinline DWORD ReadPixel4HH(int x, int y, DWORD bp, DWORD bw) const
	{
		return ReadPixel4HH(PixelAddress32(x, y, bp, bw));
	}

	__forceinline DWORD ReadPixel32Z(int x, int y, DWORD bp, DWORD bw) const
	{
		return ReadPixel32(PixelAddress32Z(x, y, bp, bw));
	}

	__forceinline DWORD ReadPixel24Z(int x, int y, DWORD bp, DWORD bw) const
	{
		return ReadPixel24(PixelAddress32Z(x, y, bp, bw));
	}

	__forceinline DWORD ReadPixel16Z(int x, int y, DWORD bp, DWORD bw) const
	{
		return ReadPixel16(PixelAddress16Z(x, y, bp, bw));
	}

	__forceinline DWORD ReadPixel16SZ(int x, int y, DWORD bp, DWORD bw) const
	{
		return ReadPixel16(PixelAddress16SZ(x, y, bp, bw));
	}

	__forceinline DWORD ReadFrame24(int x, int y, DWORD bp, DWORD bw) const
	{
		return ReadFrame24(PixelAddress32(x, y, bp, bw));
	}

	__forceinline DWORD ReadFrame16(int x, int y, DWORD bp, DWORD bw) const
	{
		return ReadFrame16(PixelAddress16(x, y, bp, bw));
	}

	__forceinline DWORD ReadFrame16S(int x, int y, DWORD bp, DWORD bw) const
	{
		return ReadFrame16(PixelAddress16S(x, y, bp, bw));
	}

	__forceinline DWORD ReadFrame24Z(int x, int y, DWORD bp, DWORD bw) const
	{
		return ReadFrame24(PixelAddress32Z(x, y, bp, bw));
	}

	__forceinline DWORD ReadFrame16Z(int x, int y, DWORD bp, DWORD bw) const
	{
		return ReadFrame16(PixelAddress16Z(x, y, bp, bw));
	}

	__forceinline DWORD ReadFrame16SZ(int x, int y, DWORD bp, DWORD bw) const
	{
		return ReadFrame16(PixelAddress16SZ(x, y, bp, bw));
	}

	__forceinline void WritePixel32(DWORD addr, DWORD c) 
	{
		m_vm32[addr] = c;
	}

	__forceinline void WritePixel24(DWORD addr, DWORD c) 
	{
		m_vm32[addr] = (m_vm32[addr] & 0xff000000) | (c & 0x00ffffff);
	}

	__forceinline void WritePixel16(DWORD addr, DWORD c) 
	{
		m_vm16[addr] = (WORD)c;
	}

	__forceinline void WritePixel8(DWORD addr, DWORD c)
	{
		m_vm8[addr] = (BYTE)c;
	}

	__forceinline void WritePixel4(DWORD addr, DWORD c) 
	{
		int shift = (addr & 1) << 2; addr >>= 1; 

		m_vm8[addr] = (BYTE)((m_vm8[addr] & (0xf0 >> shift)) | ((c & 0x0f) << shift));
	}

	__forceinline void WritePixel8H(DWORD addr, DWORD c)
	{
		m_vm32[addr] = (m_vm32[addr] & 0x00ffffff) | (c << 24);
	}

	__forceinline void WritePixel4HL(DWORD addr, DWORD c) 
	{
		m_vm32[addr] = (m_vm32[addr] & 0xf0ffffff) | ((c & 0x0f) << 24);
	}

	__forceinline void WritePixel4HH(DWORD addr, DWORD c)
	{
		m_vm32[addr] = (m_vm32[addr] & 0x0fffffff) | ((c & 0x0f) << 28);
	}

	__forceinline void WriteFrame16(DWORD addr, DWORD c) 
	{
		DWORD rb = c & 0x00f800f8;
		DWORD ga = c & 0x8000f800;

		WritePixel16(addr, (ga >> 16) | (rb >> 9) | (ga >> 6) | (rb >> 3));
	}

	__forceinline void WritePixel32(int x, int y, DWORD c, DWORD bp, DWORD bw)
	{
		WritePixel32(PixelAddress32(x, y, bp, bw), c);
	}

	__forceinline void WritePixel24(int x, int y, DWORD c, DWORD bp, DWORD bw)
	{
		WritePixel24(PixelAddress32(x, y, bp, bw), c);
	}

	__forceinline void WritePixel16(int x, int y, DWORD c, DWORD bp, DWORD bw)
	{
		WritePixel16(PixelAddress16(x, y, bp, bw), c);
	}

	__forceinline void WritePixel16S(int x, int y, DWORD c, DWORD bp, DWORD bw)
	{
		WritePixel16(PixelAddress16S(x, y, bp, bw), c);
	}

	__forceinline void WritePixel8(int x, int y, DWORD c, DWORD bp, DWORD bw)
	{
		WritePixel8(PixelAddress8(x, y, bp, bw), c);
	}

	__forceinline void WritePixel4(int x, int y, DWORD c, DWORD bp, DWORD bw)
	{
		WritePixel4(PixelAddress4(x, y, bp, bw), c);
	}

	__forceinline void WritePixel8H(int x, int y, DWORD c, DWORD bp, DWORD bw)
	{
		WritePixel8H(PixelAddress32(x, y, bp, bw), c);
	}

    __forceinline void WritePixel4HL(int x, int y, DWORD c, DWORD bp, DWORD bw)
	{
		WritePixel4HL(PixelAddress32(x, y, bp, bw), c);
	}

	__forceinline void WritePixel4HH(int x, int y, DWORD c, DWORD bp, DWORD bw)
	{
		WritePixel4HH(PixelAddress32(x, y, bp, bw), c);
	}

	__forceinline void WritePixel32Z(int x, int y, DWORD c, DWORD bp, DWORD bw)
	{
		WritePixel32(PixelAddress32Z(x, y, bp, bw), c);
	}

	__forceinline void WritePixel24Z(int x, int y, DWORD c, DWORD bp, DWORD bw)
	{
		WritePixel24(PixelAddress32Z(x, y, bp, bw), c);
	}

	__forceinline void WritePixel16Z(int x, int y, DWORD c, DWORD bp, DWORD bw)
	{
		WritePixel16(PixelAddress16Z(x, y, bp, bw), c);
	}

	__forceinline void WritePixel16SZ(int x, int y, DWORD c, DWORD bp, DWORD bw)
	{
		WritePixel16(PixelAddress16SZ(x, y, bp, bw), c);
	}

	__forceinline void WriteFrame16(int x, int y, DWORD c, DWORD bp, DWORD bw)
	{
		WriteFrame16(PixelAddress16(x, y, bp, bw), c);
	}

	__forceinline void WriteFrame16S(int x, int y, DWORD c, DWORD bp, DWORD bw)
	{
		WriteFrame16(PixelAddress16S(x, y, bp, bw), c);
	}

	__forceinline void WriteFrame16Z(int x, int y, DWORD c, DWORD bp, DWORD bw)
	{
		WriteFrame16(PixelAddress16Z(x, y, bp, bw), c);
	}

	__forceinline void WriteFrame16SZ(int x, int y, DWORD c, DWORD bp, DWORD bw)
	{
		WriteFrame16(PixelAddress16SZ(x, y, bp, bw), c);
	}

	__forceinline DWORD ReadTexel32(DWORD addr, const GIFRegTEXA& TEXA) const 
	{
		return m_vm32[addr];
	}

	__forceinline DWORD ReadTexel24(DWORD addr, const GIFRegTEXA& TEXA) const 
	{
		return Expand24To32(m_vm32[addr], TEXA);
	}

	__forceinline DWORD ReadTexel16(DWORD addr, const GIFRegTEXA& TEXA) const 
	{
		return Expand16To32(m_vm16[addr], TEXA);
	}

	__forceinline DWORD ReadTexel8(DWORD addr, const GIFRegTEXA& TEXA) const 
	{
		return m_clut[ReadPixel8(addr)];
	}

	__forceinline DWORD ReadTexel4(DWORD addr, const GIFRegTEXA& TEXA) const 
	{
		return m_clut[ReadPixel4(addr)];
	}

	__forceinline DWORD ReadTexel8H(DWORD addr, const GIFRegTEXA& TEXA) const 
	{
		return m_clut[ReadPixel8H(addr)];
	}

	__forceinline DWORD ReadTexel4HL(DWORD addr, const GIFRegTEXA& TEXA) const
	{
		return m_clut[ReadPixel4HL(addr)];
	}

	__forceinline DWORD ReadTexel4HH(DWORD addr, const GIFRegTEXA& TEXA) const 
	{
		return m_clut[ReadPixel4HH(addr)];
	}

	__forceinline DWORD ReadTexel32(int x, int y, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
	{
		return ReadTexel32(PixelAddress32(x, y, TEX0.TBP0, TEX0.TBW), TEXA);
	}

	__forceinline DWORD ReadTexel24(int x, int y, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
	{
		return ReadTexel24(PixelAddress32(x, y, TEX0.TBP0, TEX0.TBW), TEXA);
	}

	__forceinline DWORD ReadTexel16(int x, int y, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
	{
		return ReadTexel16(PixelAddress16(x, y, TEX0.TBP0, TEX0.TBW), TEXA);
	}

	__forceinline DWORD ReadTexel16S(int x, int y, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
	{
		return ReadTexel16(PixelAddress16S(x, y, TEX0.TBP0, TEX0.TBW), TEXA);
	}

	__forceinline DWORD ReadTexel8(int x, int y, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
	{
		return ReadTexel8(PixelAddress8(x, y, TEX0.TBP0, TEX0.TBW), TEXA);
	}

	__forceinline DWORD ReadTexel4(int x, int y, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
	{
		return ReadTexel4(PixelAddress4(x, y, TEX0.TBP0, TEX0.TBW), TEXA);
	}

	__forceinline DWORD ReadTexel8H(int x, int y, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
	{
		return ReadTexel8H(PixelAddress32(x, y, TEX0.TBP0, TEX0.TBW), TEXA);
	}

	__forceinline DWORD ReadTexel4HL(int x, int y, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
	{
		return ReadTexel4HL(PixelAddress32(x, y, TEX0.TBP0, TEX0.TBW), TEXA);
	}

	__forceinline DWORD ReadTexel4HH(int x, int y, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
	{
		return ReadTexel4HH(PixelAddress32(x, y, TEX0.TBP0, TEX0.TBW), TEXA);
	}

	__forceinline DWORD ReadTexel32Z(int x, int y, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
	{
		return ReadTexel32(PixelAddress32Z(x, y, TEX0.TBP0, TEX0.TBW), TEXA);
	}

	__forceinline DWORD ReadTexel24Z(int x, int y, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
	{
		return ReadTexel24(PixelAddress32Z(x, y, TEX0.TBP0, TEX0.TBW), TEXA);
	}

	__forceinline DWORD ReadTexel16Z(int x, int y, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
	{
		return ReadTexel16(PixelAddress16Z(x, y, TEX0.TBP0, TEX0.TBW), TEXA);
	}

	__forceinline DWORD ReadTexel16SZ(int x, int y, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
	{
		return ReadTexel16(PixelAddress16SZ(x, y, TEX0.TBP0, TEX0.TBW), TEXA);
	}

	__forceinline DWORD ReadTexel16NP(int x, int y, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
	{
		return ReadPixel16(x, y, TEX0.TBP0, TEX0.TBW);
	}

	__forceinline DWORD ReadTexel16SNP(int x, int y, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
	{
		return ReadPixel16S(x, y, TEX0.TBP0, TEX0.TBW);
	}

	__forceinline DWORD ReadTexel16ZNP(int x, int y, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
	{
		return ReadPixel16Z(x, y, TEX0.TBP0, TEX0.TBW);
	}

	__forceinline DWORD ReadTexel16SZNP(int x, int y, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
	{
		return ReadPixel16SZ(x, y, TEX0.TBP0, TEX0.TBW);
	}

	//

	__forceinline DWORD PixelAddressX(int PSM, int x, int y, DWORD bp, DWORD bw)
	{
		switch(PSM)
		{
		case PSM_PSMCT32: return PixelAddress32(x, y, bp, bw); 
		case PSM_PSMCT24: return PixelAddress32(x, y, bp, bw); 
		case PSM_PSMCT16: return PixelAddress16(x, y, bp, bw);
		case PSM_PSMCT16S: return PixelAddress16S(x, y, bp, bw);
		case PSM_PSMT8: return PixelAddress8(x, y, bp, bw);
		case PSM_PSMT4: return PixelAddress4(x, y, bp, bw);
		case PSM_PSMT8H: return PixelAddress32(x, y, bp, bw);
		case PSM_PSMT4HL: return PixelAddress32(x, y, bp, bw);
		case PSM_PSMT4HH: return PixelAddress32(x, y, bp, bw);
		case PSM_PSMZ32: return PixelAddress32Z(x, y, bp, bw);
		case PSM_PSMZ24: return PixelAddress32Z(x, y, bp, bw);
		case PSM_PSMZ16: return PixelAddress16Z(x, y, bp, bw);
		case PSM_PSMZ16S: return PixelAddress16SZ(x, y, bp, bw);
		default: ASSERT(0); return PixelAddress32(x, y, bp, bw);
		}
	}

	__forceinline DWORD ReadPixelX(int PSM, DWORD addr) const
	{
		switch(PSM)
		{
		case PSM_PSMCT32: return ReadPixel32(addr); 
		case PSM_PSMCT24: return ReadPixel24(addr); 
		case PSM_PSMCT16: return ReadPixel16(addr);
		case PSM_PSMCT16S: return ReadPixel16(addr);
		case PSM_PSMT8: return ReadPixel8(addr);
		case PSM_PSMT4: return ReadPixel4(addr);
		case PSM_PSMT8H: return ReadPixel8H(addr);
		case PSM_PSMT4HL: return ReadPixel4HL(addr);
		case PSM_PSMT4HH: return ReadPixel4HH(addr);
		case PSM_PSMZ32: return ReadPixel32(addr);
		case PSM_PSMZ24: return ReadPixel24(addr);
		case PSM_PSMZ16: return ReadPixel16(addr);
		case PSM_PSMZ16S: return ReadPixel16(addr);
		default: ASSERT(0); return ReadPixel32(addr);
		}
	}
	
	__forceinline DWORD ReadFrameX(int PSM, DWORD addr) const
	{
		switch(PSM)
		{
		case PSM_PSMCT32: return ReadPixel32(addr);
		case PSM_PSMCT24: return ReadFrame24(addr);
		case PSM_PSMCT16: return ReadFrame16(addr);
		case PSM_PSMCT16S: return ReadFrame16(addr);
		case PSM_PSMZ32: return ReadPixel32(addr);
		case PSM_PSMZ24: return ReadFrame24(addr);
		case PSM_PSMZ16: return ReadFrame16(addr);
		case PSM_PSMZ16S: return ReadFrame16(addr);
		default: ASSERT(0); return ReadPixel32(addr);
		}
	}

	__forceinline DWORD ReadTexelX(int PSM, DWORD addr, const GIFRegTEXA& TEXA) const
	{
		switch(PSM)
		{
		case PSM_PSMCT32: return ReadTexel32(addr, TEXA);
		case PSM_PSMCT24: return ReadTexel24(addr, TEXA);
		case PSM_PSMCT16: return ReadTexel16(addr, TEXA);
		case PSM_PSMCT16S: return ReadTexel16(addr, TEXA);
		case PSM_PSMT8: return ReadTexel8(addr, TEXA);
		case PSM_PSMT4: return ReadTexel4(addr, TEXA);
		case PSM_PSMT8H: return ReadTexel8H(addr, TEXA);
		case PSM_PSMT4HL: return ReadTexel4HL(addr, TEXA);
		case PSM_PSMT4HH: return ReadTexel4HH(addr, TEXA);
		case PSM_PSMZ32: return ReadTexel32(addr, TEXA);
		case PSM_PSMZ24: return ReadTexel24(addr, TEXA);
		case PSM_PSMZ16: return ReadTexel16(addr, TEXA);
		case PSM_PSMZ16S: return ReadTexel16(addr, TEXA);
		default: ASSERT(0); return ReadTexel32(addr, TEXA);
		}
	}

	__forceinline DWORD ReadTexelX(int PSM, int x, int y, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
	{
		switch(PSM)
		{
		case PSM_PSMCT32: return ReadTexel32(x, y, TEX0, TEXA);
		case PSM_PSMCT24: return ReadTexel24(x, y, TEX0, TEXA);
		case PSM_PSMCT16: return ReadTexel16(x, y, TEX0, TEXA);
		case PSM_PSMCT16S: return ReadTexel16(x, y, TEX0, TEXA);
		case PSM_PSMT8: return ReadTexel8(x, y, TEX0, TEXA);
		case PSM_PSMT4: return ReadTexel4(x, y, TEX0, TEXA);
		case PSM_PSMT8H: return ReadTexel8H(x, y, TEX0, TEXA);
		case PSM_PSMT4HL: return ReadTexel4HL(x, y, TEX0, TEXA);
		case PSM_PSMT4HH: return ReadTexel4HH(x, y, TEX0, TEXA);
		case PSM_PSMZ32: return ReadTexel32Z(x, y, TEX0, TEXA);
		case PSM_PSMZ24: return ReadTexel24Z(x, y, TEX0, TEXA);
		case PSM_PSMZ16: return ReadTexel16Z(x, y, TEX0, TEXA);
		case PSM_PSMZ16S: return ReadTexel16Z(x, y, TEX0, TEXA);
		default: ASSERT(0); return ReadTexel32(x, y, TEX0, TEXA);
		}
	}

	__forceinline void WritePixelX(int PSM, DWORD addr, DWORD c)
	{
		switch(PSM)
		{
		case PSM_PSMCT32: WritePixel32(addr, c); break; 
		case PSM_PSMCT24: WritePixel24(addr, c); break; 
		case PSM_PSMCT16: WritePixel16(addr, c); break;
		case PSM_PSMCT16S: WritePixel16(addr, c); break;
		case PSM_PSMT8: WritePixel8(addr, c); break;
		case PSM_PSMT4: WritePixel4(addr, c); break;
		case PSM_PSMT8H: WritePixel8H(addr, c); break;
		case PSM_PSMT4HL: WritePixel4HL(addr, c); break;
		case PSM_PSMT4HH: WritePixel4HH(addr, c); break;
		case PSM_PSMZ32: WritePixel32(addr, c); break;
		case PSM_PSMZ24: WritePixel24(addr, c); break;
		case PSM_PSMZ16: WritePixel16(addr, c); break;
		case PSM_PSMZ16S: WritePixel16(addr, c); break;
		default: ASSERT(0); WritePixel32(addr, c); break;
		}
	}

	__forceinline void WriteFrameX(int PSM, DWORD addr, DWORD c)
	{
		switch(PSM)
		{
		case PSM_PSMCT32: WritePixel32(addr, c); break; 
		case PSM_PSMCT24: WritePixel24(addr, c); break; 
		case PSM_PSMCT16: WriteFrame16(addr, c); break;
		case PSM_PSMCT16S: WriteFrame16(addr, c); break;
		case PSM_PSMZ32: WritePixel32(addr, c); break; 
		case PSM_PSMZ24: WritePixel24(addr, c); break; 
		case PSM_PSMZ16: WriteFrame16(addr, c); break;
		case PSM_PSMZ16S: WriteFrame16(addr, c); break;
		default: ASSERT(0); WritePixel32(addr, c); break;
		}
	}

	// FillRect

	bool FillRect(const GSVector4i& r, DWORD c, DWORD psm, DWORD bp, DWORD bw);

	//

	template<int psm, int bsx, int bsy, bool aligned>
	void WriteImageColumn(int l, int r, int y, int h, BYTE* src, int srcpitch, const GIFRegBITBLTBUF& BITBLTBUF);

	template<int psm, int bsx, int bsy, bool aligned>
	void WriteImageBlock(int l, int r, int y, int h, BYTE* src, int srcpitch, const GIFRegBITBLTBUF& BITBLTBUF);

	template<int psm, int bsx, int bsy>
	void WriteImageLeftRight(int l, int r, int y, int h, BYTE* src, int srcpitch, const GIFRegBITBLTBUF& BITBLTBUF);

	template<int psm, int bsx, int bsy, int trbpp>
	void WriteImageTopBottom(int l, int r, int y, int h, BYTE* src, int srcpitch, const GIFRegBITBLTBUF& BITBLTBUF);

	template<int psm, int bsx, int bsy, int trbpp>
	void WriteImage(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG);

	void WriteImage24(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG);
	void WriteImage8H(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG);
	void WriteImage4HL(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG);
	void WriteImage4HH(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG);
	void WriteImage24Z(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG);
	void WriteImageX(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG);

	// TODO: ReadImage32/24/...

	void ReadImageX(int& tx, int& ty, BYTE* dst, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG) const;

	//

	void ReadTexture32(const CRect& r, BYTE* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const;
	void ReadTexture24(const CRect& r, BYTE* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const;
	void ReadTexture16(const CRect& r, BYTE* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const;
	void ReadTexture16S(const CRect& r, BYTE* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const;
	void ReadTexture8(const CRect& r, BYTE* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const;
	void ReadTexture4(const CRect& r, BYTE* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const;
	void ReadTexture8H(const CRect& r, BYTE* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const;
	void ReadTexture4HL(const CRect& r, BYTE* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const;
	void ReadTexture4HH(const CRect& r, BYTE* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const;
	void ReadTexture32Z(const CRect& r, BYTE* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const;
	void ReadTexture24Z(const CRect& r, BYTE* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const;
	void ReadTexture16Z(const CRect& r, BYTE* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const;
	void ReadTexture16SZ(const CRect& r, BYTE* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const;

	void ReadTexture(const CRect& r, BYTE* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA, const GIFRegCLAMP& CLAMP);
	void ReadTextureNC(const CRect& r, BYTE* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA, const GIFRegCLAMP& CLAMP);

	// 32/16

	void ReadTexture16NP(const CRect& r, BYTE* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const;
	void ReadTexture16SNP(const CRect& r, BYTE* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const;
	void ReadTexture8NP(const CRect& r, BYTE* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const;
	void ReadTexture4NP(const CRect& r, BYTE* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const;
	void ReadTexture8HNP(const CRect& r, BYTE* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const;
	void ReadTexture4HLNP(const CRect& r, BYTE* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const;
	void ReadTexture4HHNP(const CRect& r, BYTE* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const;
	void ReadTexture16ZNP(const CRect& r, BYTE* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const;
	void ReadTexture16SZNP(const CRect& r, BYTE* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const;

	void ReadTextureNP(const CRect& r, BYTE* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA, const GIFRegCLAMP& CLAMP);
	void ReadTextureNPNC(const CRect& r, BYTE* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA, const GIFRegCLAMP& CLAMP);

	// 32/8

	void ReadTexture8P(const CRect& r, BYTE* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const;
	void ReadTexture4P(const CRect& r, BYTE* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const;
	void ReadTexture8HP(const CRect& r, BYTE* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const;
	void ReadTexture4HLP(const CRect& r, BYTE* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const;
	void ReadTexture4HHP(const CRect& r, BYTE* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const;

	//

	static DWORD m_xtbl[1024], m_ytbl[1024]; 

	template<typename T> void ReadTexture(CRect r, BYTE* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA, const GIFRegCLAMP& CLAMP, readTexel rt, readTexture rtx);
	template<typename T> void ReadTextureNC(CRect r, BYTE* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA, readTexel rt, readTexture rtx);

	//

	HRESULT SaveBMP(LPCTSTR fn, DWORD bp, DWORD bw, DWORD psm, int w, int h);
};

#pragma warning(default: 4244)