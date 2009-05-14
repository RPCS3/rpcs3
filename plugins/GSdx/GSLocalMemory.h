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
	typedef uint32 (*pixelAddress)(int x, int y, uint32 bp, uint32 bw);
	typedef void (GSLocalMemory::*writePixel)(int x, int y, uint32 c, uint32 bp, uint32 bw);
	typedef void (GSLocalMemory::*writeFrame)(int x, int y, uint32 c, uint32 bp, uint32 bw);
	typedef uint32 (GSLocalMemory::*readPixel)(int x, int y, uint32 bp, uint32 bw) const;
	typedef uint32 (GSLocalMemory::*readTexel)(int x, int y, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const;
	typedef void (GSLocalMemory::*writePixelAddr)(uint32 addr, uint32 c);
	typedef void (GSLocalMemory::*writeFrameAddr)(uint32 addr, uint32 c);
	typedef uint32 (GSLocalMemory::*readPixelAddr)(uint32 addr) const;
	typedef uint32 (GSLocalMemory::*readTexelAddr)(uint32 addr, const GIFRegTEXA& TEXA) const;
	typedef void (GSLocalMemory::*writeImage)(int& tx, int& ty, uint8* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG);
	typedef void (GSLocalMemory::*readImage)(int& tx, int& ty, uint8* dst, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG) const;
	typedef void (GSLocalMemory::*readTexture)(const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const;
	typedef void (GSLocalMemory::*readTextureBlock)(uint32 bp, uint8* dst, int dstpitch, const GIFRegTEXA& TEXA) const;

	typedef union 
	{
		struct
		{
			pixelAddress pa, bn;
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
			readTextureBlock rtxb, rtxbP;
			short bpp, trbpp;
			int pal; 
			GSVector2i bs, pgs;
			int* rowOffset[8];
			int* blockOffset;
		};

		uint8 dummy[128];

	} psm_t;

	static psm_t m_psm[64];

	static const int m_vmsize = 1024 * 1024 * 4;

	union {uint8* m_vm8; uint16* m_vm16; uint32* m_vm32;};

	GSClut m_clut;

	struct Offset
	{
		GSVector4i row[2048]; // 0 | 0 | 0 | 0
		int* col[4]; // x | x+1 | x+2 | x+3
		uint32 hash;
	};

	struct Offset4
	{
		// 16 bit offsets (m_vm16[...])

		GSVector2i row[2048]; // f yn | z yn (n = 0 1 2 ...)
		GSVector2i col[512]; // f xn | z xn (n = 0 4 8 ...)
		uint32 hash;
	};

protected:
	static uint32 pageOffset32[32][32][64];
	static uint32 pageOffset32Z[32][32][64];
	static uint32 pageOffset16[32][64][64];
	static uint32 pageOffset16S[32][64][64];
	static uint32 pageOffset16Z[32][64][64];
	static uint32 pageOffset16SZ[32][64][64];
	static uint32 pageOffset8[32][64][128];
	static uint32 pageOffset4[32][128][128];

	static int rowOffset32[2048];
	static int rowOffset32Z[2048];
	static int rowOffset16[2048];
	static int rowOffset16S[2048];
	static int rowOffset16Z[2048];
	static int rowOffset16SZ[2048];
	static int rowOffset8[2][2048];
	static int rowOffset4[2][2048];

	static int blockOffset32[256];
	static int blockOffset32Z[256];
	static int blockOffset16[256];
	static int blockOffset16S[256];
	static int blockOffset16Z[256];
	static int blockOffset16SZ[256];
	static int blockOffset8[256];
	static int blockOffset4[256];

	__forceinline static uint32 Expand24To32(uint32 c, const GIFRegTEXA& TEXA)
	{
		return (((!TEXA.AEM | (c & 0xffffff)) ? TEXA.TA0 : 0) << 24) | (c & 0xffffff);
	}

	__forceinline static uint32 Expand16To32(uint16 c, const GIFRegTEXA& TEXA)
	{
		return (((c & 0x8000) ? TEXA.TA1 : (!TEXA.AEM | c) ? TEXA.TA0 : 0) << 24) | ((c & 0x7c00) << 9) | ((c & 0x03e0) << 6) | ((c & 0x001f) << 3);
	}

	// TODO

	friend class GSClut;

	//

	hash_map<uint32, Offset*> m_omap;
	hash_map<uint32, Offset4*> m_o4map;

public:
	GSLocalMemory();
	virtual ~GSLocalMemory();

	Offset* GetOffset(uint32 bp, uint32 bw, uint32 psm);
	Offset4* GetOffset4(const GIFRegFRAME& FRAME, const GIFRegZBUF& ZBUF);

	// address

	static uint32 BlockNumber32(int x, int y, uint32 bp, uint32 bw)
	{
		return bp + (y & ~0x1f) * bw + ((x >> 1) & ~0x1f) + blockTable32[(y >> 3) & 3][(x >> 3) & 7];
	}

	static uint32 BlockNumber16(int x, int y, uint32 bp, uint32 bw)
	{
		return bp + ((y >> 1) & ~0x1f) * bw + ((x >> 1) & ~0x1f) + blockTable16[(y >> 3) & 7][(x >> 4) & 3];
	}

	static uint32 BlockNumber16S(int x, int y, uint32 bp, uint32 bw)
	{
		return bp + ((y >> 1) & ~0x1f) * bw + ((x >> 1) & ~0x1f) + blockTable16S[(y >> 3) & 7][(x >> 4) & 3];
	}

	static uint32 BlockNumber8(int x, int y, uint32 bp, uint32 bw)
	{
		ASSERT((bw & 1) == 0);

		return bp + ((y >> 1) & ~0x1f) * (bw >> 1) + ((x >> 2) & ~0x1f) + blockTable8[(y >> 4) & 3][(x >> 4) & 7];
	}

	static uint32 BlockNumber4(int x, int y, uint32 bp, uint32 bw)
	{
		ASSERT((bw & 1) == 0);

		return bp + ((y >> 2) & ~0x1f) * (bw >> 1) + ((x >> 2) & ~0x1f) + blockTable4[(y >> 4) & 7][(x >> 5) & 3];
	}

	static uint32 BlockNumber32Z(int x, int y, uint32 bp, uint32 bw)
	{
		return bp + (y & ~0x1f) * bw + ((x >> 1) & ~0x1f) + blockTable32Z[(y >> 3) & 3][(x >> 3) & 7];
	}

	static uint32 BlockNumber16Z(int x, int y, uint32 bp, uint32 bw)
	{
		return bp + ((y >> 1) & ~0x1f) * bw + ((x >> 1) & ~0x1f) + blockTable16Z[(y >> 3) & 7][(x >> 4) & 3];
	}

	static uint32 BlockNumber16SZ(int x, int y, uint32 bp, uint32 bw)
	{
		return bp + ((y >> 1) & ~0x1f) * bw + ((x >> 1) & ~0x1f) + blockTable16SZ[(y >> 3) & 7][(x >> 4) & 3];
	}

	uint8* BlockPtr(uint32 bp) const
	{
		ASSERT(bp < 16384);

		return &m_vm8[bp << 8];
	}

	uint8* BlockPtr32(int x, int y, uint32 bp, uint32 bw) const
	{
		return &m_vm8[BlockNumber32(x, y, bp, bw) << 8];
	}

	uint8* BlockPtr16(int x, int y, uint32 bp, uint32 bw) const
	{
		return &m_vm8[BlockNumber16(x, y, bp, bw) << 8];
	}

	uint8* BlockPtr16S(int x, int y, uint32 bp, uint32 bw) const
	{
		return &m_vm8[BlockNumber16S(x, y, bp, bw) << 8];
	}

	uint8* BlockPtr8(int x, int y, uint32 bp, uint32 bw) const
	{
		return &m_vm8[BlockNumber8(x, y, bp, bw) << 8];
	}

	uint8* BlockPtr4(int x, int y, uint32 bp, uint32 bw) const
	{
		return &m_vm8[BlockNumber4(x, y, bp, bw) << 8];
	}

	uint8* BlockPtr32Z(int x, int y, uint32 bp, uint32 bw) const
	{
		return &m_vm8[BlockNumber32Z(x, y, bp, bw) << 8];
	}

	uint8* BlockPtr16Z(int x, int y, uint32 bp, uint32 bw) const
	{
		return &m_vm8[BlockNumber16Z(x, y, bp, bw) << 8];
	}

	uint8* BlockPtr16SZ(int x, int y, uint32 bp, uint32 bw) const
	{
		return &m_vm8[BlockNumber16SZ(x, y, bp, bw) << 8];
	}

	static uint32 PixelAddressOrg32(int x, int y, uint32 bp, uint32 bw)
	{
		return (BlockNumber32(x, y, bp, bw) << 6) + columnTable32[y & 7][x & 7];
	}

	static uint32 PixelAddressOrg16(int x, int y, uint32 bp, uint32 bw)
	{
		return (BlockNumber16(x, y, bp, bw) << 7) + columnTable16[y & 7][x & 15];
	}

	static uint32 PixelAddressOrg16S(int x, int y, uint32 bp, uint32 bw)
	{
		return (BlockNumber16S(x, y, bp, bw) << 7) + columnTable16[y & 7][x & 15];
	}

	static uint32 PixelAddressOrg8(int x, int y, uint32 bp, uint32 bw)
	{
		return (BlockNumber8(x, y, bp, bw) << 8) + columnTable8[y & 15][x & 15];
	}

	static uint32 PixelAddressOrg4(int x, int y, uint32 bp, uint32 bw)
	{
		return (BlockNumber4(x, y, bp, bw) << 9) + columnTable4[y & 15][x & 31];
	}

	static uint32 PixelAddressOrg32Z(int x, int y, uint32 bp, uint32 bw)
	{
		return (BlockNumber32Z(x, y, bp, bw) << 6) + columnTable32[y & 7][x & 7];
	}

	static uint32 PixelAddressOrg16Z(int x, int y, uint32 bp, uint32 bw)
	{
		return (BlockNumber16Z(x, y, bp, bw) << 7) + columnTable16[y & 7][x & 15];
	}

	static uint32 PixelAddressOrg16SZ(int x, int y, uint32 bp, uint32 bw)
	{
		return (BlockNumber16SZ(x, y, bp, bw) << 7) + columnTable16[y & 7][x & 15];
	}

	static __forceinline uint32 PixelAddress32(int x, int y, uint32 bp, uint32 bw)
	{
		uint32 page = (bp >> 5) + (y >> 5) * bw + (x >> 6); 
		uint32 word = (page << 11) + pageOffset32[bp & 0x1f][y & 0x1f][x & 0x3f];
		return word;
	}

	static __forceinline uint32 PixelAddress16(int x, int y, uint32 bp, uint32 bw)
	{
		uint32 page = (bp >> 5) + (y >> 6) * bw + (x >> 6); 
		uint32 word = (page << 12) + pageOffset16[bp & 0x1f][y & 0x3f][x & 0x3f];
		return word;
	}

	static __forceinline uint32 PixelAddress16S(int x, int y, uint32 bp, uint32 bw)
	{
		uint32 page = (bp >> 5) + (y >> 6) * bw + (x >> 6); 
		uint32 word = (page << 12) + pageOffset16S[bp & 0x1f][y & 0x3f][x & 0x3f];
		return word;
	}

	static __forceinline uint32 PixelAddress8(int x, int y, uint32 bp, uint32 bw)
	{
		ASSERT((bw & 1) == 0);
		uint32 page = (bp >> 5) + (y >> 6) * (bw >> 1) + (x >> 7); 
		uint32 word = (page << 13) + pageOffset8[bp & 0x1f][y & 0x3f][x & 0x7f];
		return word;
	}

	static __forceinline uint32 PixelAddress4(int x, int y, uint32 bp, uint32 bw)
	{
		ASSERT((bw & 1) == 0);
		uint32 page = (bp >> 5) + (y >> 7) * (bw >> 1) + (x >> 7);
		uint32 word = (page << 14) + pageOffset4[bp & 0x1f][y & 0x7f][x & 0x7f];
		return word;
	}

	static __forceinline uint32 PixelAddress32Z(int x, int y, uint32 bp, uint32 bw)
	{
		uint32 page = (bp >> 5) + (y >> 5) * bw + (x >> 6); 
		uint32 word = (page << 11) + pageOffset32Z[bp & 0x1f][y & 0x1f][x & 0x3f];
		return word;
	}

	static __forceinline uint32 PixelAddress16Z(int x, int y, uint32 bp, uint32 bw)
	{
		uint32 page = (bp >> 5) + (y >> 6) * bw + (x >> 6); 
		uint32 word = (page << 12) + pageOffset16Z[bp & 0x1f][y & 0x3f][x & 0x3f];
		return word;
	}

	static __forceinline uint32 PixelAddress16SZ(int x, int y, uint32 bp, uint32 bw)
	{
		uint32 page = (bp >> 5) + (y >> 6) * bw + (x >> 6); 
		uint32 word = (page << 12) + pageOffset16SZ[bp & 0x1f][y & 0x3f][x & 0x3f];
		return word;
	}

	// pixel R/W

	__forceinline uint32 ReadPixel32(uint32 addr) const
	{
		return m_vm32[addr];
	}

	__forceinline uint32 ReadPixel24(uint32 addr) const 
	{
		return m_vm32[addr] & 0x00ffffff;
	}

	__forceinline uint32 ReadPixel16(uint32 addr) const 
	{
		return (uint32)m_vm16[addr];
	}

	__forceinline uint32 ReadPixel8(uint32 addr) const 
	{
		return (uint32)m_vm8[addr];
	}

	__forceinline uint32 ReadPixel4(uint32 addr) const 
	{
		return (m_vm8[addr >> 1] >> ((addr & 1) << 2)) & 0x0f;
	}

	__forceinline uint32 ReadPixel8H(uint32 addr) const 
	{
		return m_vm32[addr] >> 24;
	}

	__forceinline uint32 ReadPixel4HL(uint32 addr) const 
	{
		return (m_vm32[addr] >> 24) & 0x0f;
	}

	__forceinline uint32 ReadPixel4HH(uint32 addr) const 
	{
		return (m_vm32[addr] >> 28) & 0x0f;
	}

	__forceinline uint32 ReadFrame24(uint32 addr) const 
	{
		return 0x80000000 | (m_vm32[addr] & 0xffffff);
	}

	__forceinline uint32 ReadFrame16(uint32 addr) const 
	{
		uint32 c = (uint32)m_vm16[addr];

		return ((c & 0x8000) << 16) | ((c & 0x7c00) << 9) | ((c & 0x03e0) << 6) | ((c & 0x001f) << 3);
	}

	__forceinline uint32 ReadPixel32(int x, int y, uint32 bp, uint32 bw) const
	{
		return ReadPixel32(PixelAddress32(x, y, bp, bw));
	}

	__forceinline uint32 ReadPixel24(int x, int y, uint32 bp, uint32 bw) const
	{
		return ReadPixel24(PixelAddress32(x, y, bp, bw));
	}

	__forceinline uint32 ReadPixel16(int x, int y, uint32 bp, uint32 bw) const
	{
		return ReadPixel16(PixelAddress16(x, y, bp, bw));
	}

	__forceinline uint32 ReadPixel16S(int x, int y, uint32 bp, uint32 bw) const
	{
		return ReadPixel16(PixelAddress16S(x, y, bp, bw));
	}

	__forceinline uint32 ReadPixel8(int x, int y, uint32 bp, uint32 bw) const
	{
		return ReadPixel8(PixelAddress8(x, y, bp, bw));
	}

	__forceinline uint32 ReadPixel4(int x, int y, uint32 bp, uint32 bw) const
	{
		return ReadPixel4(PixelAddress4(x, y, bp, bw));
	}

	__forceinline uint32 ReadPixel8H(int x, int y, uint32 bp, uint32 bw) const
	{
		return ReadPixel8H(PixelAddress32(x, y, bp, bw));
	}

	__forceinline uint32 ReadPixel4HL(int x, int y, uint32 bp, uint32 bw) const
	{
		return ReadPixel4HL(PixelAddress32(x, y, bp, bw));
	}

	__forceinline uint32 ReadPixel4HH(int x, int y, uint32 bp, uint32 bw) const
	{
		return ReadPixel4HH(PixelAddress32(x, y, bp, bw));
	}

	__forceinline uint32 ReadPixel32Z(int x, int y, uint32 bp, uint32 bw) const
	{
		return ReadPixel32(PixelAddress32Z(x, y, bp, bw));
	}

	__forceinline uint32 ReadPixel24Z(int x, int y, uint32 bp, uint32 bw) const
	{
		return ReadPixel24(PixelAddress32Z(x, y, bp, bw));
	}

	__forceinline uint32 ReadPixel16Z(int x, int y, uint32 bp, uint32 bw) const
	{
		return ReadPixel16(PixelAddress16Z(x, y, bp, bw));
	}

	__forceinline uint32 ReadPixel16SZ(int x, int y, uint32 bp, uint32 bw) const
	{
		return ReadPixel16(PixelAddress16SZ(x, y, bp, bw));
	}

	__forceinline uint32 ReadFrame24(int x, int y, uint32 bp, uint32 bw) const
	{
		return ReadFrame24(PixelAddress32(x, y, bp, bw));
	}

	__forceinline uint32 ReadFrame16(int x, int y, uint32 bp, uint32 bw) const
	{
		return ReadFrame16(PixelAddress16(x, y, bp, bw));
	}

	__forceinline uint32 ReadFrame16S(int x, int y, uint32 bp, uint32 bw) const
	{
		return ReadFrame16(PixelAddress16S(x, y, bp, bw));
	}

	__forceinline uint32 ReadFrame24Z(int x, int y, uint32 bp, uint32 bw) const
	{
		return ReadFrame24(PixelAddress32Z(x, y, bp, bw));
	}

	__forceinline uint32 ReadFrame16Z(int x, int y, uint32 bp, uint32 bw) const
	{
		return ReadFrame16(PixelAddress16Z(x, y, bp, bw));
	}

	__forceinline uint32 ReadFrame16SZ(int x, int y, uint32 bp, uint32 bw) const
	{
		return ReadFrame16(PixelAddress16SZ(x, y, bp, bw));
	}

	__forceinline void WritePixel32(uint32 addr, uint32 c) 
	{
		m_vm32[addr] = c;
	}

	__forceinline void WritePixel24(uint32 addr, uint32 c) 
	{
		m_vm32[addr] = (m_vm32[addr] & 0xff000000) | (c & 0x00ffffff);
	}

	__forceinline void WritePixel16(uint32 addr, uint32 c) 
	{
		m_vm16[addr] = (uint16)c;
	}

	__forceinline void WritePixel8(uint32 addr, uint32 c)
	{
		m_vm8[addr] = (uint8)c;
	}

	__forceinline void WritePixel4(uint32 addr, uint32 c) 
	{
		int shift = (addr & 1) << 2; addr >>= 1; 

		m_vm8[addr] = (uint8)((m_vm8[addr] & (0xf0 >> shift)) | ((c & 0x0f) << shift));
	}

	__forceinline void WritePixel8H(uint32 addr, uint32 c)
	{
		m_vm32[addr] = (m_vm32[addr] & 0x00ffffff) | (c << 24);
	}

	__forceinline void WritePixel4HL(uint32 addr, uint32 c) 
	{
		m_vm32[addr] = (m_vm32[addr] & 0xf0ffffff) | ((c & 0x0f) << 24);
	}

	__forceinline void WritePixel4HH(uint32 addr, uint32 c)
	{
		m_vm32[addr] = (m_vm32[addr] & 0x0fffffff) | ((c & 0x0f) << 28);
	}

	__forceinline void WriteFrame16(uint32 addr, uint32 c) 
	{
		uint32 rb = c & 0x00f800f8;
		uint32 ga = c & 0x8000f800;

		WritePixel16(addr, (ga >> 16) | (rb >> 9) | (ga >> 6) | (rb >> 3));
	}

	__forceinline void WritePixel32(int x, int y, uint32 c, uint32 bp, uint32 bw)
	{
		WritePixel32(PixelAddress32(x, y, bp, bw), c);
	}

	__forceinline void WritePixel24(int x, int y, uint32 c, uint32 bp, uint32 bw)
	{
		WritePixel24(PixelAddress32(x, y, bp, bw), c);
	}

	__forceinline void WritePixel16(int x, int y, uint32 c, uint32 bp, uint32 bw)
	{
		WritePixel16(PixelAddress16(x, y, bp, bw), c);
	}

	__forceinline void WritePixel16S(int x, int y, uint32 c, uint32 bp, uint32 bw)
	{
		WritePixel16(PixelAddress16S(x, y, bp, bw), c);
	}

	__forceinline void WritePixel8(int x, int y, uint32 c, uint32 bp, uint32 bw)
	{
		WritePixel8(PixelAddress8(x, y, bp, bw), c);
	}

	__forceinline void WritePixel4(int x, int y, uint32 c, uint32 bp, uint32 bw)
	{
		WritePixel4(PixelAddress4(x, y, bp, bw), c);
	}

	__forceinline void WritePixel8H(int x, int y, uint32 c, uint32 bp, uint32 bw)
	{
		WritePixel8H(PixelAddress32(x, y, bp, bw), c);
	}

    __forceinline void WritePixel4HL(int x, int y, uint32 c, uint32 bp, uint32 bw)
	{
		WritePixel4HL(PixelAddress32(x, y, bp, bw), c);
	}

	__forceinline void WritePixel4HH(int x, int y, uint32 c, uint32 bp, uint32 bw)
	{
		WritePixel4HH(PixelAddress32(x, y, bp, bw), c);
	}

	__forceinline void WritePixel32Z(int x, int y, uint32 c, uint32 bp, uint32 bw)
	{
		WritePixel32(PixelAddress32Z(x, y, bp, bw), c);
	}

	__forceinline void WritePixel24Z(int x, int y, uint32 c, uint32 bp, uint32 bw)
	{
		WritePixel24(PixelAddress32Z(x, y, bp, bw), c);
	}

	__forceinline void WritePixel16Z(int x, int y, uint32 c, uint32 bp, uint32 bw)
	{
		WritePixel16(PixelAddress16Z(x, y, bp, bw), c);
	}

	__forceinline void WritePixel16SZ(int x, int y, uint32 c, uint32 bp, uint32 bw)
	{
		WritePixel16(PixelAddress16SZ(x, y, bp, bw), c);
	}

	__forceinline void WriteFrame16(int x, int y, uint32 c, uint32 bp, uint32 bw)
	{
		WriteFrame16(PixelAddress16(x, y, bp, bw), c);
	}

	__forceinline void WriteFrame16S(int x, int y, uint32 c, uint32 bp, uint32 bw)
	{
		WriteFrame16(PixelAddress16S(x, y, bp, bw), c);
	}

	__forceinline void WriteFrame16Z(int x, int y, uint32 c, uint32 bp, uint32 bw)
	{
		WriteFrame16(PixelAddress16Z(x, y, bp, bw), c);
	}

	__forceinline void WriteFrame16SZ(int x, int y, uint32 c, uint32 bp, uint32 bw)
	{
		WriteFrame16(PixelAddress16SZ(x, y, bp, bw), c);
	}

	__forceinline uint32 ReadTexel32(uint32 addr, const GIFRegTEXA& TEXA) const 
	{
		return m_vm32[addr];
	}

	__forceinline uint32 ReadTexel24(uint32 addr, const GIFRegTEXA& TEXA) const 
	{
		return Expand24To32(m_vm32[addr], TEXA);
	}

	__forceinline uint32 ReadTexel16(uint32 addr, const GIFRegTEXA& TEXA) const 
	{
		return Expand16To32(m_vm16[addr], TEXA);
	}

	__forceinline uint32 ReadTexel8(uint32 addr, const GIFRegTEXA& TEXA) const 
	{
		return m_clut[ReadPixel8(addr)];
	}

	__forceinline uint32 ReadTexel4(uint32 addr, const GIFRegTEXA& TEXA) const 
	{
		return m_clut[ReadPixel4(addr)];
	}

	__forceinline uint32 ReadTexel8H(uint32 addr, const GIFRegTEXA& TEXA) const 
	{
		return m_clut[ReadPixel8H(addr)];
	}

	__forceinline uint32 ReadTexel4HL(uint32 addr, const GIFRegTEXA& TEXA) const
	{
		return m_clut[ReadPixel4HL(addr)];
	}

	__forceinline uint32 ReadTexel4HH(uint32 addr, const GIFRegTEXA& TEXA) const 
	{
		return m_clut[ReadPixel4HH(addr)];
	}

	__forceinline uint32 ReadTexel32(int x, int y, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
	{
		return ReadTexel32(PixelAddress32(x, y, TEX0.TBP0, TEX0.TBW), TEXA);
	}

	__forceinline uint32 ReadTexel24(int x, int y, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
	{
		return ReadTexel24(PixelAddress32(x, y, TEX0.TBP0, TEX0.TBW), TEXA);
	}

	__forceinline uint32 ReadTexel16(int x, int y, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
	{
		return ReadTexel16(PixelAddress16(x, y, TEX0.TBP0, TEX0.TBW), TEXA);
	}

	__forceinline uint32 ReadTexel16S(int x, int y, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
	{
		return ReadTexel16(PixelAddress16S(x, y, TEX0.TBP0, TEX0.TBW), TEXA);
	}

	__forceinline uint32 ReadTexel8(int x, int y, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
	{
		return ReadTexel8(PixelAddress8(x, y, TEX0.TBP0, TEX0.TBW), TEXA);
	}

	__forceinline uint32 ReadTexel4(int x, int y, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
	{
		return ReadTexel4(PixelAddress4(x, y, TEX0.TBP0, TEX0.TBW), TEXA);
	}

	__forceinline uint32 ReadTexel8H(int x, int y, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
	{
		return ReadTexel8H(PixelAddress32(x, y, TEX0.TBP0, TEX0.TBW), TEXA);
	}

	__forceinline uint32 ReadTexel4HL(int x, int y, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
	{
		return ReadTexel4HL(PixelAddress32(x, y, TEX0.TBP0, TEX0.TBW), TEXA);
	}

	__forceinline uint32 ReadTexel4HH(int x, int y, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
	{
		return ReadTexel4HH(PixelAddress32(x, y, TEX0.TBP0, TEX0.TBW), TEXA);
	}

	__forceinline uint32 ReadTexel32Z(int x, int y, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
	{
		return ReadTexel32(PixelAddress32Z(x, y, TEX0.TBP0, TEX0.TBW), TEXA);
	}

	__forceinline uint32 ReadTexel24Z(int x, int y, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
	{
		return ReadTexel24(PixelAddress32Z(x, y, TEX0.TBP0, TEX0.TBW), TEXA);
	}

	__forceinline uint32 ReadTexel16Z(int x, int y, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
	{
		return ReadTexel16(PixelAddress16Z(x, y, TEX0.TBP0, TEX0.TBW), TEXA);
	}

	__forceinline uint32 ReadTexel16SZ(int x, int y, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
	{
		return ReadTexel16(PixelAddress16SZ(x, y, TEX0.TBP0, TEX0.TBW), TEXA);
	}

	__forceinline uint32 ReadTexel16NP(int x, int y, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
	{
		return ReadPixel16(x, y, TEX0.TBP0, TEX0.TBW);
	}

	__forceinline uint32 ReadTexel16SNP(int x, int y, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
	{
		return ReadPixel16S(x, y, TEX0.TBP0, TEX0.TBW);
	}

	__forceinline uint32 ReadTexel16ZNP(int x, int y, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
	{
		return ReadPixel16Z(x, y, TEX0.TBP0, TEX0.TBW);
	}

	__forceinline uint32 ReadTexel16SZNP(int x, int y, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
	{
		return ReadPixel16SZ(x, y, TEX0.TBP0, TEX0.TBW);
	}

	//

	__forceinline uint32 PixelAddressX(int PSM, int x, int y, uint32 bp, uint32 bw)
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

	__forceinline uint32 ReadPixelX(int PSM, uint32 addr) const
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
	
	__forceinline uint32 ReadFrameX(int PSM, uint32 addr) const
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

	__forceinline uint32 ReadTexelX(int PSM, uint32 addr, const GIFRegTEXA& TEXA) const
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

	__forceinline uint32 ReadTexelX(int PSM, int x, int y, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
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

	__forceinline void WritePixelX(int PSM, uint32 addr, uint32 c)
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

	__forceinline void WriteFrameX(int PSM, uint32 addr, uint32 c)
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

	bool FillRect(const GSVector4i& r, uint32 c, uint32 psm, uint32 bp, uint32 bw);

	//

	template<int psm, int bsx, int bsy, bool aligned>
	void WriteImageColumn(int l, int r, int y, int h, uint8* src, int srcpitch, const GIFRegBITBLTBUF& BITBLTBUF);

	template<int psm, int bsx, int bsy, bool aligned>
	void WriteImageBlock(int l, int r, int y, int h, uint8* src, int srcpitch, const GIFRegBITBLTBUF& BITBLTBUF);

	template<int psm, int bsx, int bsy>
	void WriteImageLeftRight(int l, int r, int y, int h, uint8* src, int srcpitch, const GIFRegBITBLTBUF& BITBLTBUF);

	template<int psm, int bsx, int bsy, int trbpp>
	void WriteImageTopBottom(int l, int r, int y, int h, uint8* src, int srcpitch, const GIFRegBITBLTBUF& BITBLTBUF);

	template<int psm, int bsx, int bsy, int trbpp>
	void WriteImage(int& tx, int& ty, uint8* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG);

	void WriteImage24(int& tx, int& ty, uint8* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG);
	void WriteImage8H(int& tx, int& ty, uint8* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG);
	void WriteImage4HL(int& tx, int& ty, uint8* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG);
	void WriteImage4HH(int& tx, int& ty, uint8* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG);
	void WriteImage24Z(int& tx, int& ty, uint8* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG);
	void WriteImageX(int& tx, int& ty, uint8* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG);

	// TODO: ReadImage32/24/...

	void ReadImageX(int& tx, int& ty, uint8* dst, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG) const;

	// * => 32

	void ReadTexture32(const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const;
	void ReadTexture24(const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const;
	void ReadTexture16(const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const;
	void ReadTexture16S(const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const;
	void ReadTexture8(const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const;
	void ReadTexture4(const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const;
	void ReadTexture8H(const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const;
	void ReadTexture4HL(const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const;
	void ReadTexture4HH(const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const;
	void ReadTexture32Z(const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const;
	void ReadTexture24Z(const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const;
	void ReadTexture16Z(const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const;
	void ReadTexture16SZ(const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const;

	void ReadTexture(const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA, const GIFRegCLAMP& CLAMP);
	void ReadTextureNC(const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA, const GIFRegCLAMP& CLAMP);

	void ReadTextureBlock32(uint32 bp, uint8* dst, int dstpitch, const GIFRegTEXA& TEXA) const;
	void ReadTextureBlock24(uint32 bp, uint8* dst, int dstpitch, const GIFRegTEXA& TEXA) const;
	void ReadTextureBlock16(uint32 bp, uint8* dst, int dstpitch, const GIFRegTEXA& TEXA) const;
	void ReadTextureBlock16S(uint32 bp, uint8* dst, int dstpitch, const GIFRegTEXA& TEXA) const;
	void ReadTextureBlock8(uint32 bp, uint8* dst, int dstpitch, const GIFRegTEXA& TEXA) const;
	void ReadTextureBlock4(uint32 bp, uint8* dst, int dstpitch, const GIFRegTEXA& TEXA) const;
	void ReadTextureBlock8H(uint32 bp, uint8* dst, int dstpitch, const GIFRegTEXA& TEXA) const;
	void ReadTextureBlock4HL(uint32 bp, uint8* dst, int dstpitch, const GIFRegTEXA& TEXA) const;
	void ReadTextureBlock4HH(uint32 bp, uint8* dst, int dstpitch, const GIFRegTEXA& TEXA) const;
	void ReadTextureBlock32Z(uint32 bp, uint8* dst, int dstpitch, const GIFRegTEXA& TEXA) const;
	void ReadTextureBlock24Z(uint32 bp, uint8* dst, int dstpitch, const GIFRegTEXA& TEXA) const;
	void ReadTextureBlock16Z(uint32 bp, uint8* dst, int dstpitch, const GIFRegTEXA& TEXA) const;
	void ReadTextureBlock16SZ(uint32 bp, uint8* dst, int dstpitch, const GIFRegTEXA& TEXA) const;

	// * => 32/16

	void ReadTexture16NP(const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const;
	void ReadTexture16SNP(const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const;
	void ReadTexture8NP(const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const;
	void ReadTexture4NP(const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const;
	void ReadTexture8HNP(const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const;
	void ReadTexture4HLNP(const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const;
	void ReadTexture4HHNP(const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const;
	void ReadTexture16ZNP(const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const;
	void ReadTexture16SZNP(const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const;

	void ReadTextureNP(const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA, const GIFRegCLAMP& CLAMP);
	void ReadTextureNPNC(const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA, const GIFRegCLAMP& CLAMP);

	// pal ? 8 : 32

	void ReadTexture8P(const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const;
	void ReadTexture4P(const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const;
	void ReadTexture8HP(const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const;
	void ReadTexture4HLP(const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const;
	void ReadTexture4HHP(const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const;

	void ReadTextureBlock8P(uint32 bp, uint8* dst, int dstpitch, const GIFRegTEXA& TEXA) const;
	void ReadTextureBlock4P(uint32 bp, uint8* dst, int dstpitch, const GIFRegTEXA& TEXA) const;
	void ReadTextureBlock8HP(uint32 bp, uint8* dst, int dstpitch, const GIFRegTEXA& TEXA) const;
	void ReadTextureBlock4HLP(uint32 bp, uint8* dst, int dstpitch, const GIFRegTEXA& TEXA) const;
	void ReadTextureBlock4HHP(uint32 bp, uint8* dst, int dstpitch, const GIFRegTEXA& TEXA) const;

	//

	static uint32 m_xtbl[1024], m_ytbl[1024]; 

	template<typename T> void ReadTexture(const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA, const GIFRegCLAMP& CLAMP, readTexel rt, readTexture rtx);
	template<typename T> void ReadTextureNC(const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA, readTexel rt, readTexture rtx);

	//

	HRESULT SaveBMP(const string& fn, uint32 bp, uint32 bw, uint32 psm, int w, int h);
};

#pragma warning(default: 4244)