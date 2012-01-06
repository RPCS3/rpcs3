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

#include "GS.h"
#include "GSTables.h"
#include "GSVector.h"
#include "GSBlock.h"
#include "GSClut.h"
#include "GSThread.h"

class GSOffset : public GSAlignedClass<32>
{
public:
	__aligned(struct, 32) Block
	{
		short row[256]; // yn (n = 0 8 16 ...)
		short* col; // blockOffset*
	};
	
	__aligned(struct, 32) Pixel
	{
		int row[4096]; // yn (n = 0 1 2 ...) NOTE: this wraps around above 2048, only transfers should address the upper half (dark cloud 2 inventing)
		int* col[8]; // rowOffset*
	};

	union {uint32 hash; struct {uint32 bp:14, bw:6, psm:6;};};

	Block block;
	Pixel pixel;

	GSOffset(uint32 bp, uint32 bw, uint32 psm);
	virtual ~GSOffset();

	enum {EOP = 0xffffffff};

	uint32* GetPages(const GSVector4i& rect, uint32* pages = NULL, GSVector4i* bbox = NULL);
};

struct GSPixelOffset4
{
	// 16 bit offsets (m_vm16[...])

	GSVector2i row[2048]; // f yn | z yn (n = 0 1 2 ...)
	GSVector2i col[512]; // f xn | z xn (n = 0 4 8 ...)
	uint32 hash;
};

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
	typedef void (GSLocalMemory::*writeImage)(int& tx, int& ty, const uint8* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG);
	typedef void (GSLocalMemory::*readImage)(int& tx, int& ty, uint8* dst, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG) const;
	typedef void (GSLocalMemory::*readTexture)(const GSOffset* RESTRICT o, const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEXA& TEXA);
	typedef void (GSLocalMemory::*readTextureBlock)(uint32 bp, uint8* dst, int dstpitch, const GIFRegTEXA& TEXA) const;

	__aligned(struct, 128) psm_t
	{
		pixelAddress pa, bn;
		readPixel rp;
		readPixelAddr rpa;
		writePixel wp;
		writePixelAddr wpa;
		readTexel rt;
		readTexelAddr rta;
		writeFrameAddr wfa;
		writeImage wi;
		readImage ri;
		readTexture rtx, rtxP;
		readTextureBlock rtxb, rtxbP;
		uint16 bpp, trbpp, pal, fmt;
		GSVector2i bs, pgs;
		int* rowOffset[8];
		short* blockOffset;
	};

	static psm_t m_psm[64];

	static const int m_vmsize = 1024 * 1024 * 4;

	uint8* m_vm8; 
	uint16* m_vm16; 
	uint32* m_vm32;

	GSClut m_clut;

protected:
	static uint32 pageOffset32[32][32][64];
	static uint32 pageOffset32Z[32][32][64];
	static uint32 pageOffset16[32][64][64];
	static uint32 pageOffset16S[32][64][64];
	static uint32 pageOffset16Z[32][64][64];
	static uint32 pageOffset16SZ[32][64][64];
	static uint32 pageOffset8[32][64][128];
	static uint32 pageOffset4[32][128][128];

	static int rowOffset32[4096];
	static int rowOffset32Z[4096];
	static int rowOffset16[4096];
	static int rowOffset16S[4096];
	static int rowOffset16Z[4096];
	static int rowOffset16SZ[4096];
	static int rowOffset8[2][4096];
	static int rowOffset4[2][4096];

	static short blockOffset32[256];
	static short blockOffset32Z[256];
	static short blockOffset16[256];
	static short blockOffset16S[256];
	static short blockOffset16Z[256];
	static short blockOffset16SZ[256];
	static short blockOffset8[256];
	static short blockOffset4[256];

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

	hash_map<uint32, GSOffset*> m_omap;
	hash_map<uint32, GSPixelOffset4*> m_po4map;
	hash_map<uint64, vector<GSVector2i>*> m_p2tmap;

public:
	GSLocalMemory();
	virtual ~GSLocalMemory();

	GSOffset* GetOffset(uint32 bp, uint32 bw, uint32 psm);
	GSPixelOffset4* GetPixelOffset4(const GIFRegFRAME& FRAME, const GIFRegZBUF& ZBUF);
	vector<GSVector2i>* GetPage2TileMap(const GIFRegTEX0& TEX0);

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
		// ASSERT((bw & 1) == 0); // allowed for mipmap levels

		return bp + ((y >> 1) & ~0x1f) * (bw >> 1) + ((x >> 2) & ~0x1f) + blockTable8[(y >> 4) & 3][(x >> 4) & 7];
	}

	static uint32 BlockNumber4(int x, int y, uint32 bp, uint32 bw)
	{
		// ASSERT((bw & 1) == 0); // allowed for mipmap levels

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
		// ASSERT((bw & 1) == 0); // allowed for mipmap levels

		uint32 page = (bp >> 5) + (y >> 6) * (bw >> 1) + (x >> 7);
		uint32 word = (page << 13) + pageOffset8[bp & 0x1f][y & 0x3f][x & 0x7f];

		return word;
	}

	static __forceinline uint32 PixelAddress4(int x, int y, uint32 bp, uint32 bw)
	{
		// ASSERT((bw & 1) == 0); // allowed for mipmap levels

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

	__forceinline void WritePixel32(uint8* RESTRICT src, uint32 pitch, GSOffset* o, const GSVector4i& r)
	{
		src -= r.left * sizeof(uint32);

		for(int y = r.top; y < r.bottom; y++, src += pitch)
		{
			uint32* RESTRICT s = (uint32*)src;
			uint32* RESTRICT d = &m_vm32[o->pixel.row[y]];
			int* RESTRICT col = o->pixel.col[0];

			for(int x = r.left; x < r.right; x++)
			{
				d[col[x]] = s[x];
			}
		}
	}

	__forceinline void WritePixel24(uint8* RESTRICT src, uint32 pitch, GSOffset* o, const GSVector4i& r)
	{
		src -= r.left * sizeof(uint32);

		for(int y = r.top; y < r.bottom; y++, src += pitch)
		{
			uint32* RESTRICT s = (uint32*)src;
			uint32* RESTRICT d = &m_vm32[o->pixel.row[y]];
			int* RESTRICT col = o->pixel.col[0];

			for(int x = r.left; x < r.right; x++)
			{
				d[col[x]] = (d[col[x]] & 0xff000000) | (s[x] & 0x00ffffff);
			}
		}
	}

	__forceinline void WritePixel16(uint8* RESTRICT src, uint32 pitch, GSOffset* o, const GSVector4i& r)
	{
		src -= r.left * sizeof(uint16);

		for(int y = r.top; y < r.bottom; y++, src += pitch)
		{
			uint16* RESTRICT s = (uint16*)src;
			uint16* RESTRICT d = &m_vm16[o->pixel.row[y]];
			int* RESTRICT col = o->pixel.col[0];

			for(int x = r.left; x < r.right; x++)
			{
				d[col[x]] = s[x];
			}
		}
	}

	__forceinline void WriteFrame16(uint8* RESTRICT src, uint32 pitch, GSOffset* o, const GSVector4i& r)
	{
		src -= r.left * sizeof(uint32);

		for(int y = r.top; y < r.bottom; y++, src += pitch)
		{
			uint32* RESTRICT s = (uint32*)src;
			uint16* RESTRICT d = &m_vm16[o->pixel.row[y]];
			int* RESTRICT col = o->pixel.col[0];

			for(int x = r.left; x < r.right; x++)
			{
				uint32 rb = s[x] & 0x00f800f8;
				uint32 ga = s[x] & 0x8000f800;

				d[col[x]] = (uint16)((ga >> 16) | (rb >> 9) | (ga >> 6) | (rb >> 3));
			}
		}
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

	//

	template<int psm, int bsx, int bsy, bool aligned>
	void WriteImageColumn(int l, int r, int y, int h, const uint8* src, int srcpitch, const GIFRegBITBLTBUF& BITBLTBUF);

	template<int psm, int bsx, int bsy, bool aligned>
	void WriteImageBlock(int l, int r, int y, int h, const uint8* src, int srcpitch, const GIFRegBITBLTBUF& BITBLTBUF);

	template<int psm, int bsx, int bsy>
	void WriteImageLeftRight(int l, int r, int y, int h, const uint8* src, int srcpitch, const GIFRegBITBLTBUF& BITBLTBUF);

	template<int psm, int bsx, int bsy, int trbpp>
	void WriteImageTopBottom(int l, int r, int y, int h, const uint8* src, int srcpitch, const GIFRegBITBLTBUF& BITBLTBUF);

	template<int psm, int bsx, int bsy, int trbpp>
	void WriteImage(int& tx, int& ty, const uint8* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG);

	void WriteImage24(int& tx, int& ty, const uint8* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG);
	void WriteImage8H(int& tx, int& ty, const uint8* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG);
	void WriteImage4HL(int& tx, int& ty, const uint8* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG);
	void WriteImage4HH(int& tx, int& ty, const uint8* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG);
	void WriteImage24Z(int& tx, int& ty, const uint8* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG);
	void WriteImageX(int& tx, int& ty, const uint8* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG);

	// TODO: ReadImage32/24/...

	void ReadImageX(int& tx, int& ty, uint8* dst, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG) const;

	// * => 32

	void ReadTexture32(const GSOffset* RESTRICT o, const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEXA& TEXA);
	void ReadTexture24(const GSOffset* RESTRICT o, const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEXA& TEXA);
	void ReadTexture16(const GSOffset* RESTRICT o, const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEXA& TEXA);
	void ReadTexture16S(const GSOffset* RESTRICT o, const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEXA& TEXA);
	void ReadTexture8(const GSOffset* RESTRICT o, const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEXA& TEXA);
	void ReadTexture4(const GSOffset* RESTRICT o, const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEXA& TEXA);
	void ReadTexture8H(const GSOffset* RESTRICT o, const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEXA& TEXA);
	void ReadTexture4HL(const GSOffset* RESTRICT o, const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEXA& TEXA);
	void ReadTexture4HH(const GSOffset* RESTRICT o, const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEXA& TEXA);
	void ReadTexture32Z(const GSOffset* RESTRICT o, const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEXA& TEXA);
	void ReadTexture24Z(const GSOffset* RESTRICT o, const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEXA& TEXA);
	void ReadTexture16Z(const GSOffset* RESTRICT o, const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEXA& TEXA);
	void ReadTexture16SZ(const GSOffset* RESTRICT o, const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEXA& TEXA);

	void ReadTexture(const GSOffset* RESTRICT o, const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEXA& TEXA);

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

	// pal ? 8 : 32

	void ReadTexture8P(const GSOffset* RESTRICT o, const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEXA& TEXA);
	void ReadTexture4P(const GSOffset* RESTRICT o, const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEXA& TEXA);
	void ReadTexture8HP(const GSOffset* RESTRICT o, const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEXA& TEXA);
	void ReadTexture4HLP(const GSOffset* RESTRICT o, const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEXA& TEXA);
	void ReadTexture4HHP(const GSOffset* RESTRICT o, const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEXA& TEXA);

	void ReadTextureBlock8P(uint32 bp, uint8* dst, int dstpitch, const GIFRegTEXA& TEXA) const;
	void ReadTextureBlock4P(uint32 bp, uint8* dst, int dstpitch, const GIFRegTEXA& TEXA) const;
	void ReadTextureBlock8HP(uint32 bp, uint8* dst, int dstpitch, const GIFRegTEXA& TEXA) const;
	void ReadTextureBlock4HLP(uint32 bp, uint8* dst, int dstpitch, const GIFRegTEXA& TEXA) const;
	void ReadTextureBlock4HHP(uint32 bp, uint8* dst, int dstpitch, const GIFRegTEXA& TEXA) const;

	//

	template<typename T> void ReadTexture(const GSOffset* RESTRICT o, const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEXA& TEXA);

	//

	void SaveBMP(const string& fn, uint32 bp, uint32 bw, uint32 psm, int w, int h);
};

