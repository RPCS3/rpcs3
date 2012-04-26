/*  ZZ Open GL graphics plugin
 *  Copyright (c)2009-2010 zeydlitz@gmail.com, arcum42@gmail.com
 *  Based on Zerofrog's ZeroGS KOSMOS (c)2005-2008
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef __MEM_H__
#define __MEM_H__

#include <assert.h>
#include <vector>

// works only when base is a power of 2
static __forceinline int ROUND_UPPOW2(int val, int base) { return (((val) + (base - 1))&~(base - 1)); }
static __forceinline int ROUND_DOWNPOW2(int val, int base) { return ((val)&~(base - 1)); }
static __forceinline int MOD_POW2(int val, int base) { return ((val)&(base - 1)); }

// d3d texture dims
const int BLOCK_TEXWIDTH = 128;
const int BLOCK_TEXHEIGHT = 512;

// PSM is u6 value, so we MUST guarantee, that we don't crush on incorrect psm.
#define MAX_PSM 64
#define TABLE_WIDTH 8

#ifndef ZZNORMAL_MEMORY
#include "ZZoglMem.h"
#endif

typedef u32(*_getPixelAddress)(int x, int y, u32 bp, u32 bw);
typedef u32(*_getPixelAddress_0)(int x, int y, u32 bw);
typedef void (*_writePixel)(void* pmem, int x, int y, u32 pixel, u32 bp, u32 bw);
typedef void (*_writePixel_0)(void* pmem, int x, int y, u32 pixel, u32 bw);
typedef u32(*_readPixel)(const void* pmem, int x, int y, u32 bp, u32 bw);
typedef u32(*_readPixel_0)(const void* pmem, int x, int y, u32 bw);
typedef int (*_TransferHostLocal)(const void* pbyMem, u32 nQWordSize);
typedef void (*_TransferLocalHost)(void* pbyMem, u32 nQWordSize);
typedef void (*_SwizzleBlock)(u8 *dst, u8 *src, int pitch);

extern _getPixelAddress_0 getPixelFun_0[64];
extern _writePixel_0 writePixelFun_0[64];
extern _readPixel_0 readPixelFun_0[64];
extern _writePixel writePixelFun[64];
extern _readPixel readPixelFun[64];
extern _SwizzleBlock swizzleBlockFun[64];
extern _SwizzleBlock swizzleBlockUnFun[64];
extern _TransferHostLocal TransferHostLocalFun[64];
extern _TransferLocalHost TransferLocalHostFun[64];


// Both of the following structs should probably be local class variables or in a namespace,
// but this works for the moment.

struct TransferData
{
	// Signed because Visual C++ is weird.
	s32 widthlimit;
	u32 blockbits;
	u32 blockwidth;
	u32 blockheight;
	u32 transfersize;
	u32 psm;
};

#ifdef ZZNORMAL_MEMORY
extern PCSX2_ALIGNED16(u32 tempblock[64]);

struct TransferFuncts
{
	_writePixel_0 wp;
	_getPixelAddress_0 gp;
	_SwizzleBlock Swizzle, Swizzle_u;
	__forceinline TransferFuncts(_writePixel_0 writePix, _getPixelAddress_0 readPix, _SwizzleBlock s,  _SwizzleBlock su)
	{
		wp = writePix;
		gp = readPix;
		Swizzle = s;
		Swizzle_u = su;
	}
	__forceinline TransferFuncts(u32 psm)
	{
		wp = writePixelFun_0[psm];
		gp = getPixelFun_0[psm];
		Swizzle = swizzleBlockFun[psm];
		Swizzle_u = swizzleBlockUnFun[psm];
	}
};

extern TransferData tData[64];
// rest not visible externally

extern u32 g_blockTable32[4][8];
extern u32 g_blockTable32Z[4][8];
extern u32 g_blockTable16[8][4];
extern u32 g_blockTable16S[8][4];
extern u32 g_blockTable16Z[8][4];
extern u32 g_blockTable16SZ[8][4];
extern u32 g_blockTable8[4][8];
extern u32 g_blockTable4[8][4];

extern u32 g_columnTable32[8][8];
extern u32 g_columnTable16[8][16];
extern u32 g_columnTable8[16][16];
extern u32 g_columnTable4[16][32];

extern u32 g_pageTable32[32][64];
extern u32 g_pageTable32Z[32][64];
extern u32 g_pageTable16[64][64];
extern u32 g_pageTable16S[64][64];
extern u32 g_pageTable16Z[64][64];
extern u32 g_pageTable16SZ[64][64];
extern u32 g_pageTable8[64][128];
extern u32 g_pageTable4[128][128];

struct BLOCK
{
	BLOCK() { memset(this, 0, sizeof(BLOCK)); }

	// shader constants for this block
	float4 vTexBlock;
	float4 vTexDims;
	int width, height;	// dims of one page in pixels
	int ox, oy, mult;
	int bpp;
	int colwidth, colheight;
	u32* pageTable;	// offset inside each page
	u32* blockTable;
	u32* columnTable;

	_getPixelAddress getPixelAddress;
	_getPixelAddress_0 getPixelAddress_0;
	_writePixel writePixel;
	_writePixel_0 writePixel_0;
	_readPixel readPixel;
	_readPixel_0 readPixel_0;
	_TransferHostLocal TransferHostLocal;
	_TransferLocalHost TransferLocalHost;

	// texture must be of dims BLOCK_TEXWIDTH and BLOCK_TEXHEIGHT
	static void FillBlocks(std::vector<char>& vBlockData, std::vector<char>& vBilinearData, int floatfmt);
	
	void SetDim(u32 bw, u32 bh, u32 ox2, u32 oy2, u32 mult2)
	{
		ox = ox2;
		oy = oy2;
		mult = mult2;
		vTexDims = float4(BLOCK_TEXWIDTH/(float)(bw), BLOCK_TEXHEIGHT/(float)bh, 0, 0); 
		vTexBlock = float4((float)bw/BLOCK_TEXWIDTH, (float)bh/BLOCK_TEXHEIGHT, ((float)ox+0.2f)/BLOCK_TEXWIDTH, ((float)oy+0.05f)/BLOCK_TEXHEIGHT);
		width = bw;
		height = bh;
		colwidth = bh / 4;
		colheight = bw / 8;
		bpp = 32/mult;
	}
	
	void SetFun(u32 psm)
	{
		writePixel = writePixelFun[psm];
		writePixel_0 = writePixelFun_0[psm];
		readPixel = readPixelFun[psm];
		readPixel_0 = readPixelFun_0[psm];
		TransferHostLocal = TransferHostLocalFun[psm];
		TransferLocalHost = TransferLocalHostFun[psm];
	}

    void SetTable(u32 psm)
    {
        switch (psm) {
            case PSMCT32:
                assert( sizeof(g_pageTable32) == width * height * sizeof(g_pageTable32[0][0]) );
                pageTable = &g_pageTable32[0][0];
                blockTable = &g_blockTable32[0][0];
                columnTable = &g_columnTable32[0][0];
                break;
            case PSMT32Z:
                assert( sizeof(g_pageTable32Z) == width * height * sizeof(g_pageTable32Z[0][0]) );
                pageTable = &g_pageTable32Z[0][0];
                blockTable = &g_blockTable32Z[0][0];
                columnTable = &g_columnTable32[0][0];
                break;
            case PSMCT16:
                assert( sizeof(g_pageTable16) == width * height * sizeof(g_pageTable16[0][0]) );
                pageTable = &g_pageTable16[0][0];
                blockTable = &g_blockTable16[0][0];
                columnTable = &g_columnTable16[0][0];
                break;
            case PSMCT16S:
                assert( sizeof(g_pageTable16S) == width * height * sizeof(g_pageTable16S[0][0]) );
                pageTable = &g_pageTable16S[0][0];
                blockTable = &g_blockTable16S[0][0];
                columnTable = &g_columnTable16[0][0];
                break;
            case PSMT16Z:
                assert( sizeof(g_pageTable16Z) == width * height * sizeof(g_pageTable16Z[0][0]) );
                pageTable = &g_pageTable16Z[0][0];
                blockTable = &g_blockTable16Z[0][0];
                columnTable = &g_columnTable16[0][0];
                break;
            case PSMT16SZ:
                assert( sizeof(g_pageTable16SZ) == width * height * sizeof(g_pageTable16SZ[0][0]) );
                pageTable = &g_pageTable16SZ[0][0];
                blockTable = &g_blockTable16SZ[0][0];
                columnTable = &g_columnTable16[0][0];
                break;
            case PSMT8:
                assert( sizeof(g_pageTable8) == width * height * sizeof(g_pageTable8[0][0]) );
                pageTable = &g_pageTable8[0][0];
                blockTable = &g_blockTable8[0][0];
                columnTable = &g_columnTable8[0][0];
                break;
            case PSMT4:
                assert( sizeof(g_pageTable4) == width * height * sizeof(g_pageTable4[0][0]) );
                pageTable = &g_pageTable4[0][0];
                blockTable = &g_blockTable4[0][0];
                columnTable = &g_columnTable4[0][0];
                break;
            default:
                pageTable = NULL;
                blockTable = NULL;
                columnTable = NULL;
                break;
        }
    }
};

extern BLOCK m_Blocks[];

#define getPixelAddress24 getPixelAddress32
#define getPixelAddress24_0 getPixelAddress32_0
#define getPixelAddress8H getPixelAddress32
#define getPixelAddress8H_0 getPixelAddress32_0
#define getPixelAddress4HL getPixelAddress32
#define getPixelAddress4HL_0 getPixelAddress32_0
#define getPixelAddress4HH getPixelAddress32
#define getPixelAddress4HH_0 getPixelAddress32_0
#define getPixelAddress24Z getPixelAddress32Z
#define getPixelAddress24Z_0 getPixelAddress32Z_0

static __forceinline u32 getPixelAddress32(int x, int y, u32 bp, u32 bw)
{
	u32 basepage = ((y >> 5) * (bw >> 6)) + (x >> 6);
	u32 word = bp * 64 + basepage * 2048 + g_pageTable32[y&31][x&63];
	return word;
}

static __forceinline u32 getPixelAddress16(int x, int y, u32 bp, u32 bw)
{
	u32 basepage = ((y >> 6) * (bw >> 6)) + (x >> 6);
	u32 word = bp * 128 + basepage * 4096 + g_pageTable16[y&63][x&63];
	return word;
}

static __forceinline u32 getPixelAddress16S(int x, int y, u32 bp, u32 bw)
{
	u32 basepage = ((y >> 6) * (bw >> 6)) + (x >> 6);
	u32 word = bp * 128 + basepage * 4096 + g_pageTable16S[y&63][x&63];
	return word;
}

static __forceinline u32 getPixelAddress8(int x, int y, u32 bp, u32 bw)
{
	u32 basepage = ((y >> 6) * ((bw + 127) >> 7)) + (x >> 7);
	u32 word = bp * 256 + basepage * 8192 + g_pageTable8[y&63][x&127];
	return word;
}

static __forceinline u32 getPixelAddress4(int x, int y, u32 bp, u32 bw)
{
	u32 basepage = ((y >> 7) * ((bw + 127) >> 7)) + (x >> 7);
	u32 word = bp * 512 + basepage * 16384 + g_pageTable4[y&127][x&127];
	return word;
}

static __forceinline u32 getPixelAddress32Z(int x, int y, u32 bp, u32 bw)
{
	u32 basepage = ((y >> 5) * (bw >> 6)) + (x >> 6);
	u32 word = bp * 64 + basepage * 2048 + g_pageTable32Z[y&31][x&63];
	return word;
}

static __forceinline u32 getPixelAddress16Z(int x, int y, u32 bp, u32 bw)
{
	u32 basepage = ((y >> 6) * (bw >> 6)) + (x >> 6);
	u32 word = bp * 128 + basepage * 4096 + g_pageTable16Z[y&63][x&63];
	return word;
}

static __forceinline u32 getPixelAddress16SZ(int x, int y, u32 bp, u32 bw)
{
	u32 basepage = ((y >> 6) * (bw >> 6)) + (x >> 6);
	u32 word = bp * 128 + basepage * 4096 + g_pageTable16SZ[y&63][x&63];
	return word;
}

///////////////

static __forceinline void writePixel32(void* pmem, int x, int y, u32 pixel, u32 bp, u32 bw)
{
	((u32*)pmem)[getPixelAddress32(x, y, bp, bw)] = pixel;
}

static __forceinline void writePixel24(void* pmem, int x, int y, u32 pixel, u32 bp, u32 bw)
{
	u8 *buf = (u8*) & ((u32*)pmem)[getPixelAddress32(x, y, bp, bw)];
	u8 *pix = (u8*) & pixel;
	buf[0] = pix[0];
	buf[1] = pix[1];
	buf[2] = pix[2];
}

static __forceinline void writePixel16(void* pmem, int x, int y, u32 pixel, u32 bp, u32 bw)
{
	((u16*)pmem)[getPixelAddress16(x, y, bp, bw)] = pixel;
}

static __forceinline void writePixel16S(void* pmem, int x, int y, u32 pixel, u32 bp, u32 bw)
{
	((u16*)pmem)[getPixelAddress16S(x, y, bp, bw)] = pixel;
}

static __forceinline void writePixel8(void* pmem, int x, int y, u32 pixel, u32 bp, u32 bw)
{
	((u8*)pmem)[getPixelAddress8(x, y, bp, bw)] = pixel;
}

static __forceinline void writePixel8H(void* pmem, int x, int y, u32 pixel, u32 bp, u32 bw)
{
	((u8*)pmem)[4*getPixelAddress32(x, y, bp, bw)+3] = pixel;
}

static __forceinline void writePixel4(void* pmem, int x, int y, u32 pixel, u32 bp, u32 bw)
{
	u32 addr = getPixelAddress4(x, y, bp, bw);
	u8 pix = ((u8*)pmem)[addr/2];

	if (addr & 0x1)((u8*)pmem)[addr/2] = (pix & 0x0f) | (pixel << 4);
	else ((u8*)pmem)[addr/2] = (pix & 0xf0) | (pixel);
}

static __forceinline void writePixel4HL(void* pmem, int x, int y, u32 pixel, u32 bp, u32 bw)
{
	u8 *p = (u8*)pmem + 4 * getPixelAddress4HL(x, y, bp, bw) + 3;
	*p = (*p & 0xf0) | pixel;
}

static __forceinline void writePixel4HH(void* pmem, int x, int y, u32 pixel, u32 bp, u32 bw)
{
	u8 *p = (u8*)pmem + 4 * getPixelAddress4HH(x, y, bp, bw) + 3;
	*p = (*p & 0x0f) | (pixel << 4);
}

static __forceinline void writePixel32Z(void* pmem, int x, int y, u32 pixel, u32 bp, u32 bw)
{
	((u32*)pmem)[getPixelAddress32Z(x, y, bp, bw)] = pixel;
}

static __forceinline void writePixel24Z(void* pmem, int x, int y, u32 pixel, u32 bp, u32 bw)
{
	u8 *buf = (u8*)pmem + 4 * getPixelAddress32Z(x, y, bp, bw);
	u8 *pix = (u8*) & pixel;
	buf[0] = pix[0];
	buf[1] = pix[1];
	buf[2] = pix[2];
}

static __forceinline void writePixel16Z(void* pmem, int x, int y, u32 pixel, u32 bp, u32 bw)
{
	((u16*)pmem)[getPixelAddress16Z(x, y, bp, bw)] = pixel;
}

static __forceinline void writePixel16SZ(void* pmem, int x, int y, u32 pixel, u32 bp, u32 bw)
{
	((u16*)pmem)[getPixelAddress16SZ(x, y, bp, bw)] = pixel;
}

///////////////

static __forceinline u32 readPixel32(const void* pmem, int x, int y, u32 bp, u32 bw)
{
	return ((const u32*)pmem)[getPixelAddress32(x, y, bp, bw)];
}

static __forceinline u32 readPixel24(const void* pmem, int x, int y, u32 bp, u32 bw)
{
	return ((const u32*)pmem)[getPixelAddress32(x, y, bp, bw)] & 0xffffff;
}

static __forceinline u32 readPixel16(const void* pmem, int x, int y, u32 bp, u32 bw)
{
	return ((const u16*)pmem)[getPixelAddress16(x, y, bp, bw)];
}

static __forceinline u32 readPixel16S(const void* pmem, int x, int y, u32 bp, u32 bw)
{
	return ((const u16*)pmem)[getPixelAddress16S(x, y, bp, bw)];
}

static __forceinline u32 readPixel8(const void* pmem, int x, int y, u32 bp, u32 bw)
{
	return ((const u8*)pmem)[getPixelAddress8(x, y, bp, bw)];
}

static __forceinline u32 readPixel8H(const void* pmem, int x, int y, u32 bp, u32 bw)
{
	return ((const u8*)pmem)[4*getPixelAddress32(x, y, bp, bw) + 3];
}

static __forceinline u32 readPixel4(const void* pmem, int x, int y, u32 bp, u32 bw)
{
	u32 addr = getPixelAddress4(x, y, bp, bw);
	u8 pix = ((const u8*)pmem)[addr/2];

	if (addr & 0x1)
		return pix >> 4;
	else 
		return pix & 0xf;
}

static __forceinline u32 readPixel4HL(const void* pmem, int x, int y, u32 bp, u32 bw)
{
	const u8 *p = (const u8*)pmem + 4 * getPixelAddress4HL(x, y, bp, bw) + 3;
	return *p & 0x0f;
}

static __forceinline u32 readPixel4HH(const void* pmem, int x, int y, u32 bp, u32 bw)
{
	const u8 *p = (const u8*)pmem + 4 * getPixelAddress4HH(x, y, bp, bw) + 3;
	return *p >> 4;
}

///////////////

static __forceinline u32 readPixel32Z(const void* pmem, int x, int y, u32 bp, u32 bw)
{
	return ((const u32*)pmem)[getPixelAddress32Z(x, y, bp, bw)];
}

static __forceinline u32 readPixel24Z(const void* pmem, int x, int y, u32 bp, u32 bw)
{
	return ((const u32*)pmem)[getPixelAddress32Z(x, y, bp, bw)] & 0xffffff;
}

static __forceinline u32 readPixel16Z(const void* pmem, int x, int y, u32 bp, u32 bw)
{
	return ((const u16*)pmem)[getPixelAddress16Z(x, y, bp, bw)];
}

static __forceinline u32 readPixel16SZ(const void* pmem, int x, int y, u32 bp, u32 bw)
{
	return ((const u16*)pmem)[getPixelAddress16SZ(x, y, bp, bw)];
}

///////////////////////////////
// Functions that take 0 bps //
///////////////////////////////

static __forceinline u32 getPixelAddress32_0(int x, int y, u32 bw) { return getPixelAddress32(x, y, 0, bw); }
static __forceinline u32 getPixelAddress16_0(int x, int y, u32 bw) { return getPixelAddress16(x, y, 0, bw); }
static __forceinline u32 getPixelAddress16S_0(int x, int y, u32 bw) { return getPixelAddress16S(x, y, 0, bw); }
static __forceinline u32 getPixelAddress8_0(int x, int y, u32 bw) { return getPixelAddress8(x, y, 0, bw); }
static __forceinline u32 getPixelAddress4_0(int x, int y, u32 bw) { return getPixelAddress4(x, y, 0, bw); }
static __forceinline u32 getPixelAddress32Z_0(int x, int y, u32 bw) { return getPixelAddress32Z(x, y, 0, bw); }
static __forceinline u32 getPixelAddress16Z_0(int x, int y, u32 bw) { return getPixelAddress16Z(x, y, 0, bw); }
static __forceinline u32 getPixelAddress16SZ_0(int x, int y, u32 bw) { return getPixelAddress16SZ(x, y, 0, bw); }

///////////////

static __forceinline void writePixel32_0(void* pmem, int x, int y, u32 pixel, u32 bw) { writePixel32(pmem, x, y, pixel, 0, bw); }
static __forceinline void writePixel24_0(void* pmem, int x, int y, u32 pixel, u32 bw) { writePixel24(pmem, x, y, pixel, 0, bw); }
static __forceinline void writePixel16_0(void* pmem, int x, int y, u32 pixel, u32 bw) { writePixel16(pmem, x, y, pixel, 0, bw); }
static __forceinline void writePixel16S_0(void* pmem, int x, int y, u32 pixel, u32 bw) { writePixel16S(pmem, x, y, pixel, 0, bw); }
static __forceinline void writePixel8_0(void* pmem, int x, int y, u32 pixel, u32 bw) { writePixel8(pmem, x, y, pixel, 0, bw); }
static __forceinline void writePixel8H_0(void* pmem, int x, int y, u32 pixel, u32 bw) { writePixel8H(pmem, x, y, pixel, 0, bw); }
static __forceinline void writePixel4_0(void* pmem, int x, int y, u32 pixel, u32 bw) { writePixel4(pmem, x, y, pixel, 0, bw); }
static __forceinline void writePixel4HL_0(void* pmem, int x, int y, u32 pixel, u32 bw) { writePixel4HL(pmem, x, y, pixel, 0, bw); }
static __forceinline void writePixel4HH_0(void* pmem, int x, int y, u32 pixel, u32 bw) { writePixel4HH(pmem, x, y, pixel, 0, bw); }
static __forceinline void writePixel32Z_0(void* pmem, int x, int y, u32 pixel, u32 bw) { writePixel32Z(pmem, x, y, pixel, 0, bw); }
static __forceinline void writePixel24Z_0(void* pmem, int x, int y, u32 pixel, u32 bw) { writePixel24Z(pmem, x, y, pixel, 0, bw); }
static __forceinline void writePixel16Z_0(void* pmem, int x, int y, u32 pixel, u32 bw) { writePixel16Z(pmem, x, y, pixel, 0, bw); }
static __forceinline void writePixel16SZ_0(void* pmem, int x, int y, u32 pixel, u32 bw) { writePixel16SZ(pmem, x, y, pixel, 0, bw); }

///////////////

static __forceinline u32 readPixel32_0(const void* pmem, int x, int y, u32 bw) { return readPixel32(pmem, x, y, 0, bw); }
static __forceinline u32 readPixel24_0(const void* pmem, int x, int y, u32 bw) { return readPixel24(pmem, x, y, 0, bw); }
static __forceinline u32 readPixel16_0(const void* pmem, int x, int y, u32 bw) { return readPixel16(pmem, x, y, 0, bw); }
static __forceinline u32 readPixel16S_0(const void* pmem, int x, int y, u32 bw) { return readPixel16S(pmem, x, y, 0, bw); }
static __forceinline u32 readPixel8_0(const void* pmem, int x, int y, u32 bw) { return readPixel8(pmem, x, y, 0, bw); }
static __forceinline u32 readPixel8H_0(const void* pmem, int x, int y, u32 bw) { return readPixel8H(pmem, x, y, 0, bw); }
static __forceinline u32 readPixel4_0(const void* pmem, int x, int y, u32 bw) { return readPixel4(pmem, x, y, 0, bw); }
static __forceinline u32 readPixel4HL_0(const void* pmem, int x, int y, u32 bw) { return readPixel4HL(pmem, x, y, 0, bw); }
static __forceinline u32 readPixel4HH_0(const void* pmem, int x, int y, u32 bw) { return readPixel4HH(pmem, x, y, 0, bw); }
static __forceinline u32 readPixel32Z_0(const void* pmem, int x, int y, u32 bw) { return readPixel32Z(pmem, x, y, 0, bw); }
static __forceinline u32 readPixel24Z_0(const void* pmem, int x, int y, u32 bw) { return readPixel24Z(pmem, x, y, 0, bw); }
static __forceinline u32 readPixel16Z_0(const void* pmem, int x, int y, u32 bw) { return readPixel16Z(pmem, x, y, 0, bw); }
static __forceinline u32 readPixel16SZ_0(const void* pmem, int x, int y, u32 bw) { return readPixel16SZ(pmem, x, y, 0, bw); }

///////////////

#endif

extern int TransferHostLocal32(const void* pbyMem, u32 nQWordSize);
extern int TransferHostLocal32Z(const void* pbyMem, u32 nQWordSize);
extern int TransferHostLocal24(const void* pbyMem, u32 nQWordSize);
extern int TransferHostLocal24Z(const void* pbyMem, u32 nQWordSize);
extern int TransferHostLocal16(const void* pbyMem, u32 nQWordSize);
extern int TransferHostLocal16S(const void* pbyMem, u32 nQWordSize);
extern int TransferHostLocal16Z(const void* pbyMem, u32 nQWordSize);
extern int TransferHostLocal16SZ(const void* pbyMem, u32 nQWordSize);
extern int TransferHostLocal8(const void* pbyMem, u32 nQWordSize);
extern int TransferHostLocal4(const void* pbyMem, u32 nQWordSize);
extern int TransferHostLocal8H(const void* pbyMem, u32 nQWordSize);
extern int TransferHostLocal4HL(const void* pbyMem, u32 nQWordSize);
extern int TransferHostLocal4HH(const void* pbyMem, u32 nQWordSize);

extern void TransferLocalHost32(void* pbyMem, u32 nQWordSize);
extern void TransferLocalHost24(void* pbyMem, u32 nQWordSize);
extern void TransferLocalHost16(void* pbyMem, u32 nQWordSize);
extern void TransferLocalHost16S(void* pbyMem, u32 nQWordSize);
extern void TransferLocalHost8(void* pbyMem, u32 nQWordSize);
extern void TransferLocalHost4(void* pbyMem, u32 nQWordSize);
extern void TransferLocalHost8H(void* pbyMem, u32 nQWordSize);
extern void TransferLocalHost4HL(void* pbyMem, u32 nQWordSize);
extern void TransferLocalHost4HH(void* pbyMem, u32 nQWordSize);
extern void TransferLocalHost32Z(void* pbyMem, u32 nQWordSize);
extern void TransferLocalHost24Z(void* pbyMem, u32 nQWordSize);
extern void TransferLocalHost16Z(void* pbyMem, u32 nQWordSize);
extern void TransferLocalHost16SZ(void* pbyMem, u32 nQWordSize);

#endif /* __MEM_H__ */
