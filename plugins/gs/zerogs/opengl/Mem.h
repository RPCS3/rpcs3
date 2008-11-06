/*  ZeroGS KOSMOS
 *  Copyright (C) 2005-2006 zerofrog@gmail.com
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __MEM_H__
#define __MEM_H__

#include <assert.h>
#include <vector>

// works only when base is a power of 2
#define ROUND_UPPOW2(val, base)	(((val)+(base-1))&~(base-1))
#define ROUND_DOWNPOW2(val, base)	((val)&~(base-1))
#define MOD_POW2(val, base) ((val)&(base-1))

// d3d texture dims
#define BLOCK_TEXWIDTH	128
#define BLOCK_TEXHEIGHT 512

// rest not visible externally
struct BLOCK
{
	BLOCK() { memset(this, 0, sizeof(BLOCK)); }

	// shader constants for this block
	Vector vTexBlock;
	Vector vTexDims;
	int width, height;	// dims of one page in pixels
	int bpp;
	int colwidth, colheight;
	u32* pageTable;	// offset inside each page
	u32* blockTable;
	u32* columnTable;

	u32 (*getPixelAddress)(int x, int y, u32 bp, u32 bw);
	u32 (*getPixelAddress_0)(int x, int y, u32 bw);
	void (*writePixel)(void* pmem, int x, int y, u32 pixel, u32 bp, u32 bw);
	void (*writePixel_0)(void* pmem, int x, int y, u32 pixel, u32 bw);
	u32 (*readPixel)(const void* pmem, int x, int y, u32 bp, u32 bw);
	u32 (*readPixel_0)(const void* pmem, int x, int y, u32 bw);
	int (*TransferHostLocal)(const void* pbyMem, u32 nQWordSize);
	void (*TransferLocalHost)(void* pbyMem, u32 nQWordSize);

	// texture must be of dims BLOCK_TEXWIDTH and BLOCK_TEXHEIGHT
    static void FillBlocks(std::vector<char>& vBlockData, std::vector<char>& vBilinearData, int floatfmt);
};

extern BLOCK m_Blocks[];

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

__forceinline u32 getPixelAddress32(int x, int y, u32 bp, u32 bw) {
	u32 basepage = ((y>>5) * (bw>>6)) + (x>>6);
	u32 word = bp * 64 + basepage * 2048 + g_pageTable32[y&31][x&63];
	//assert (word < 0x100000);
	//word = min(word, 0xfffff);
	return word;
}

__forceinline u32 getPixelAddress32_0(int x, int y, u32 bw) {
	u32 basepage = ((y>>5) * (bw>>6)) + (x>>6);
	u32 word = basepage * 2048 + g_pageTable32[y&31][x&63];
	//assert (word < 0x100000);
	//word = min(word, 0xfffff);
	return word;
}

#define getPixelAddress24 getPixelAddress32
#define getPixelAddress24_0 getPixelAddress32_0
#define getPixelAddress8H getPixelAddress32
#define getPixelAddress8H_0 getPixelAddress32_0
#define getPixelAddress4HL getPixelAddress32
#define getPixelAddress4HL_0 getPixelAddress32_0
#define getPixelAddress4HH getPixelAddress32
#define getPixelAddress4HH_0 getPixelAddress32_0

__forceinline u32 getPixelAddress16(int x, int y, u32 bp, u32 bw) {
	u32 basepage = ((y>>6) * (bw>>6)) + (x>>6);
	u32 word = bp * 128 + basepage * 4096 + g_pageTable16[y&63][x&63];
	//assert (word < 0x200000);
	//word = min(word, 0x1fffff);
	return word;
}

__forceinline u32 getPixelAddress16_0(int x, int y, u32 bw) {
	u32 basepage = ((y>>6) * (bw>>6)) + (x>>6);
	u32 word = basepage * 4096 + g_pageTable16[y&63][x&63];
	//assert (word < 0x200000);
	//word = min(word, 0x1fffff);
	return word;
}

__forceinline u32 getPixelAddress16S(int x, int y, u32 bp, u32 bw) {
	u32 basepage = ((y>>6) * (bw>>6)) + (x>>6);
	u32 word = bp * 128 + basepage * 4096 + g_pageTable16S[y&63][x&63];
	//assert (word < 0x200000);
	//word = min(word, 0x1fffff);
	return word;
}

__forceinline u32 getPixelAddress16S_0(int x, int y, u32 bw) {
	u32 basepage = ((y>>6) * (bw>>6)) + (x>>6);
	u32 word = basepage * 4096 + g_pageTable16S[y&63][x&63];
	//assert (word < 0x200000);
	//word = min(word, 0x1fffff);
	return word;
}

__forceinline u32 getPixelAddress8(int x, int y, u32 bp, u32 bw) {
	u32 basepage = ((y>>6) * ((bw+127)>>7)) + (x>>7);
	u32 word = bp * 256 + basepage * 8192 + g_pageTable8[y&63][x&127];
	//assert (word < 0x400000);
	//word = min(word, 0x3fffff);
	return word;
}

__forceinline u32 getPixelAddress8_0(int x, int y, u32 bw) {
	u32 basepage = ((y>>6) * ((bw+127)>>7)) + (x>>7);
	u32 word = basepage * 8192 + g_pageTable8[y&63][x&127];
	//assert (word < 0x400000);
	//word = min(word, 0x3fffff);
	return word;
}

__forceinline u32 getPixelAddress4(int x, int y, u32 bp, u32 bw) {
	u32 basepage = ((y>>7) * ((bw+127)>>7)) + (x>>7);
	u32 word = bp * 512 + basepage * 16384 + g_pageTable4[y&127][x&127];
	//assert (word < 0x800000);
	//word = min(word, 0x7fffff);
	return word;
}

__forceinline u32 getPixelAddress4_0(int x, int y, u32 bw) {
	u32 basepage = ((y>>7) * ((bw+127)>>7)) + (x>>7);
	u32 word = basepage * 16384 + g_pageTable4[y&127][x&127];
	//assert (word < 0x800000);
	//word = min(word, 0x7fffff);
	return word;
}

__forceinline u32 getPixelAddress32Z(int x, int y, u32 bp, u32 bw) {
	u32 basepage = ((y>>5) * (bw>>6)) + (x>>6);
	u32 word = bp * 64 + basepage * 2048 + g_pageTable32Z[y&31][x&63];
	//assert (word < 0x100000);
	//word = min(word, 0xfffff);
	return word;
}

__forceinline u32 getPixelAddress32Z_0(int x, int y, u32 bw) {
	u32 basepage = ((y>>5) * (bw>>6)) + (x>>6);
	u32 word = basepage * 2048 + g_pageTable32Z[y&31][x&63];
	//assert (word < 0x100000);
	//word = min(word, 0xfffff);
	return word;
}

#define getPixelAddress24Z getPixelAddress32Z
#define getPixelAddress24Z_0 getPixelAddress32Z_0

__forceinline u32 getPixelAddress16Z(int x, int y, u32 bp, u32 bw) {
	u32 basepage = ((y>>6) * (bw>>6)) + (x>>6);
	u32 word = bp * 128 + basepage * 4096 + g_pageTable16Z[y&63][x&63];
	//assert (word < 0x200000);
	//word = min(word, 0x1fffff);
	return word;
}

__forceinline u32 getPixelAddress16Z_0(int x, int y, u32 bw) {
	u32 basepage = ((y>>6) * (bw>>6)) + (x>>6);
	u32 word = basepage * 4096 + g_pageTable16Z[y&63][x&63];
	//assert (word < 0x200000);
	//word = min(word, 0x1fffff);
	return word;
}

__forceinline u32 getPixelAddress16SZ(int x, int y, u32 bp, u32 bw) {
	u32 basepage = ((y>>6) * (bw>>6)) + (x>>6);
	u32 word = bp * 128 + basepage * 4096 + g_pageTable16SZ[y&63][x&63];
	//assert (word < 0x200000);
	//word = min(word, 0x1fffff);
	return word;
}

__forceinline u32 getPixelAddress16SZ_0(int x, int y, u32 bw) {
	u32 basepage = ((y>>6) * (bw>>6)) + (x>>6);
	u32 word = basepage * 4096 + g_pageTable16SZ[y&63][x&63];
	//assert (word < 0x200000);
	//word = min(word, 0x1fffff);
	return word;
}

__forceinline void writePixel32(void* pmem, int x, int y, u32 pixel, u32 bp, u32 bw) {
	((u32*)pmem)[getPixelAddress32(x, y, bp, bw)] = pixel;
}

__forceinline void writePixel24(void* pmem, int x, int y, u32 pixel, u32 bp, u32 bw) {
	u8 *buf = (u8*)&((u32*)pmem)[getPixelAddress32(x, y, bp, bw)];
	u8 *pix = (u8*)&pixel;
	buf[0] = pix[0]; buf[1] = pix[1]; buf[2] = pix[2];
}

__forceinline void writePixel16(void* pmem, int x, int y, u32 pixel, u32 bp, u32 bw) {
	((u16*)pmem)[getPixelAddress16(x, y, bp, bw)] = pixel;
}

__forceinline void writePixel16S(void* pmem, int x, int y, u32 pixel, u32 bp, u32 bw) {
	((u16*)pmem)[getPixelAddress16S(x, y, bp, bw)] = pixel;
}

__forceinline void writePixel8(void* pmem, int x, int y, u32 pixel, u32 bp, u32 bw) {
	((u8*)pmem)[getPixelAddress8(x, y, bp, bw)] = pixel;
}

__forceinline void writePixel8H(void* pmem, int x, int y, u32 pixel, u32 bp, u32 bw) {
	((u8*)pmem)[4*getPixelAddress32(x, y, bp, bw)+3] = pixel;
}

__forceinline void writePixel4(void* pmem, int x, int y, u32 pixel, u32 bp, u32 bw) {
	u32 addr = getPixelAddress4(x, y, bp, bw);
	u8 pix = ((u8*)pmem)[addr/2];
	if (addr & 0x1) ((u8*)pmem)[addr/2] = (pix & 0x0f) | (pixel << 4);
	else ((u8*)pmem)[addr/2] = (pix & 0xf0) | (pixel);
}

__forceinline void writePixel4HL(void* pmem, int x, int y, u32 pixel, u32 bp, u32 bw) {
	u8 *p = (u8*)pmem + 4*getPixelAddress4HL(x, y, bp, bw)+3;
	*p = (*p & 0xf0) | pixel;
}

__forceinline void writePixel4HH(void* pmem, int x, int y, u32 pixel, u32 bp, u32 bw) {
	u8 *p = (u8*)pmem + 4*getPixelAddress4HH(x, y, bp, bw)+3;
	*p = (*p & 0x0f) | (pixel<<4);
}

__forceinline void writePixel32Z(void* pmem, int x, int y, u32 pixel, u32 bp, u32 bw) {
	((u32*)pmem)[getPixelAddress32Z(x, y, bp, bw)] = pixel;
}

__forceinline void writePixel24Z(void* pmem, int x, int y, u32 pixel, u32 bp, u32 bw) {
	u8 *buf = (u8*)pmem + 4*getPixelAddress32Z(x, y, bp, bw);
	u8 *pix = (u8*)&pixel;
	buf[0] = pix[0]; buf[1] = pix[1]; buf[2] = pix[2];
}

__forceinline void writePixel16Z(void* pmem, int x, int y, u32 pixel, u32 bp, u32 bw) {
	((u16*)pmem)[getPixelAddress16Z(x, y, bp, bw)] = pixel;
}

__forceinline void writePixel16SZ(void* pmem, int x, int y, u32 pixel, u32 bp, u32 bw) {
	((u16*)pmem)[getPixelAddress16SZ(x, y, bp, bw)] = pixel;
}


///////////////

__forceinline u32  readPixel32(const void* pmem, int x, int y, u32 bp, u32 bw) {
	return ((const u32*)pmem)[getPixelAddress32(x, y, bp, bw)];
}

__forceinline u32  readPixel24(const void* pmem, int x, int y, u32 bp, u32 bw) {
	return ((const u32*)pmem)[getPixelAddress32(x, y, bp, bw)] & 0xffffff;
}

__forceinline u32  readPixel16(const void* pmem, int x, int y, u32 bp, u32 bw) {
	return ((const u16*)pmem)[getPixelAddress16(x, y, bp, bw)];
}

__forceinline u32  readPixel16S(const void* pmem, int x, int y, u32 bp, u32 bw) {
	return ((const u16*)pmem)[getPixelAddress16S(x, y, bp, bw)];
}

__forceinline u32  readPixel8(const void* pmem, int x, int y, u32 bp, u32 bw) {
	return ((const u8*)pmem)[getPixelAddress8(x, y, bp, bw)];
}

__forceinline u32  readPixel8H(const void* pmem, int x, int y, u32 bp, u32 bw) {
	return ((const u8*)pmem)[4*getPixelAddress32(x, y, bp, bw) + 3];
}

__forceinline u32  readPixel4(const void* pmem, int x, int y, u32 bp, u32 bw) {
	u32 addr = getPixelAddress4(x, y, bp, bw);
	u8 pix = ((const u8*)pmem)[addr/2];
	if (addr & 0x1)
	     return pix >> 4;
	else return pix & 0xf;
}

__forceinline u32  readPixel4HL(const void* pmem, int x, int y, u32 bp, u32 bw) {
	const u8 *p = (const u8*)pmem+4*getPixelAddress4HL(x, y, bp, bw)+3;
	return *p & 0x0f;
}

__forceinline u32  readPixel4HH(const void* pmem, int x, int y, u32 bp, u32 bw) {
	const u8 *p = (const u8*)pmem+4*getPixelAddress4HH(x, y, bp, bw) + 3;
	return *p >> 4;
}

///////////////

__forceinline u32  readPixel32Z(const void* pmem, int x, int y, u32 bp, u32 bw) {
	return ((const u32*)pmem)[getPixelAddress32Z(x, y, bp, bw)];
}

__forceinline u32  readPixel24Z(const void* pmem, int x, int y, u32 bp, u32 bw) {
	return ((const u32*)pmem)[getPixelAddress32Z(x, y, bp, bw)] & 0xffffff;
}

__forceinline u32  readPixel16Z(const void* pmem, int x, int y, u32 bp, u32 bw) {
	return ((const u16*)pmem)[getPixelAddress16Z(x, y, bp, bw)];
}

__forceinline u32  readPixel16SZ(const void* pmem, int x, int y, u32 bp, u32 bw) {
	return ((const u16*)pmem)[getPixelAddress16SZ(x, y, bp, bw)];
}

///////////////////////////////
// Functions that take 0 bps //
///////////////////////////////

__forceinline void writePixel32_0(void* pmem, int x, int y, u32 pixel, u32 bw) {
	((u32*)pmem)[getPixelAddress32_0(x, y, bw)] = pixel;
}

__forceinline void writePixel24_0(void* pmem, int x, int y, u32 pixel, u32 bw) {
	u8 *buf = (u8*)&((u32*)pmem)[getPixelAddress32_0(x, y, bw)];
	u8 *pix = (u8*)&pixel;
#if defined(_MSC_VER) && defined(__x86_64__)
	memcpy(buf, pix, 3);
#else
	buf[0] = pix[0]; buf[1] = pix[1]; buf[2] = pix[2];
#endif
}

__forceinline void writePixel16_0(void* pmem, int x, int y, u32 pixel, u32 bw) {
	((u16*)pmem)[getPixelAddress16_0(x, y, bw)] = pixel;
}

__forceinline void writePixel16S_0(void* pmem, int x, int y, u32 pixel, u32 bw) {
	((u16*)pmem)[getPixelAddress16S_0(x, y, bw)] = pixel;
}

__forceinline void writePixel8_0(void* pmem, int x, int y, u32 pixel, u32 bw) {
	((u8*)pmem)[getPixelAddress8_0(x, y, bw)] = pixel;
}

__forceinline void writePixel8H_0(void* pmem, int x, int y, u32 pixel, u32 bw) {
	((u8*)pmem)[4*getPixelAddress32_0(x, y, bw)+3] = pixel;
}

__forceinline void writePixel4_0(void* pmem, int x, int y, u32 pixel, u32 bw) {
	u32 addr = getPixelAddress4_0(x, y, bw);
	u8 pix = ((u8*)pmem)[addr/2];
	if (addr & 0x1) ((u8*)pmem)[addr/2] = (pix & 0x0f) | (pixel << 4);
	else ((u8*)pmem)[addr/2] = (pix & 0xf0) | (pixel);
}

__forceinline void writePixel4HL_0(void* pmem, int x, int y, u32 pixel, u32 bw) {
	u8 *p = (u8*)pmem + 4*getPixelAddress4HL_0(x, y, bw)+3;
	*p = (*p & 0xf0) | pixel;
}

__forceinline void writePixel4HH_0(void* pmem, int x, int y, u32 pixel, u32 bw) {
	u8 *p = (u8*)pmem + 4*getPixelAddress4HH_0(x, y, bw)+3;
	*p = (*p & 0x0f) | (pixel<<4);
}

__forceinline void writePixel32Z_0(void* pmem, int x, int y, u32 pixel, u32 bw) {
	((u32*)pmem)[getPixelAddress32Z_0(x, y, bw)] = pixel;
}

__forceinline void writePixel24Z_0(void* pmem, int x, int y, u32 pixel, u32 bw) {
	u8 *buf = (u8*)pmem + 4*getPixelAddress32Z_0(x, y, bw);
	u8 *pix = (u8*)&pixel;
#if defined(_MSC_VER) && defined(__x86_64__)
	memcpy(buf, pix, 3);
#else
	buf[0] = pix[0]; buf[1] = pix[1]; buf[2] = pix[2];
#endif
}

__forceinline void writePixel16Z_0(void* pmem, int x, int y, u32 pixel, u32 bw) {
	((u16*)pmem)[getPixelAddress16Z_0(x, y, bw)] = pixel;
}

__forceinline void writePixel16SZ_0(void* pmem, int x, int y, u32 pixel, u32 bw) {
	((u16*)pmem)[getPixelAddress16SZ_0(x, y, bw)] = pixel;
}


///////////////

__forceinline u32  readPixel32_0(const void* pmem, int x, int y, u32 bw) {
	return ((const u32*)pmem)[getPixelAddress32_0(x, y, bw)];
}

__forceinline u32  readPixel24_0(const void* pmem, int x, int y, u32 bw) {
	return ((const u32*)pmem)[getPixelAddress32_0(x, y, bw)] & 0xffffff;
}

__forceinline u32  readPixel16_0(const void* pmem, int x, int y, u32 bw) {
	return ((const u16*)pmem)[getPixelAddress16_0(x, y, bw)];
}

__forceinline u32  readPixel16S_0(const void* pmem, int x, int y, u32 bw) {
	return ((const u16*)pmem)[getPixelAddress16S_0(x, y, bw)];
}

__forceinline u32  readPixel8_0(const void* pmem, int x, int y, u32 bw) {
	return ((const u8*)pmem)[getPixelAddress8_0(x, y, bw)];
}

__forceinline u32  readPixel8H_0(const void* pmem, int x, int y, u32 bw) {
	return ((const u8*)pmem)[4*getPixelAddress32_0(x, y, bw) + 3];
}

__forceinline u32  readPixel4_0(const void* pmem, int x, int y, u32 bw) {
	u32 addr = getPixelAddress4_0(x, y, bw);
	u8 pix = ((const u8*)pmem)[addr/2];
	if (addr & 0x1)
	     return pix >> 4;
	else return pix & 0xf;
}

__forceinline u32  readPixel4HL_0(const void* pmem, int x, int y, u32 bw) {
	const u8 *p = (const u8*)pmem+4*getPixelAddress4HL_0(x, y, bw)+3;
	return *p & 0x0f;
}

__forceinline u32  readPixel4HH_0(const void* pmem, int x, int y, u32 bw) {
	const u8 *p = (const u8*)pmem+4*getPixelAddress4HH_0(x, y, bw) + 3;
	return *p >> 4;
}

///////////////

__forceinline u32  readPixel32Z_0(const void* pmem, int x, int y, u32 bw) {
	return ((const u32*)pmem)[getPixelAddress32Z_0(x, y, bw)];
}

__forceinline u32  readPixel24Z_0(const void* pmem, int x, int y, u32 bw) {
	return ((const u32*)pmem)[getPixelAddress32Z_0(x, y, bw)] & 0xffffff;
}

__forceinline u32  readPixel16Z_0(const void* pmem, int x, int y, u32 bw) {
	return ((const u16*)pmem)[getPixelAddress16Z_0(x, y, bw)];
}

__forceinline u32  readPixel16SZ_0(const void* pmem, int x, int y, u32 bw) {
	return ((const u16*)pmem)[getPixelAddress16SZ_0(x, y, bw)];
}

#endif /* __MEM_H__ */
