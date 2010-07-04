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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "GS.h"
#include "Mem.h"
#include "zerogs.h"
#include "targets.h"
#include "x86.h"

#include "Mem_Transmit.h"
#include "Mem_Swizzle.h"

BLOCK m_Blocks[0x40]; // do so blocks are indexable

PCSX2_ALIGNED16(u32 tempblock[64]);

// Add a bunch of local variables that used to be in the TransferHostLocal
// functions, in order to de-macro the TransmitHostLocal macros.
// May be in a class or namespace eventually.
int tempX, tempY;
int pitch, area, fracX;
int nSize;
u8* pstart;

// ------------------------
// |              Y       |
// ------------------------
// |        block     |   |
// |   aligned area   | X |
// |                  |   |
// ------------------------
// |              Y       |
// ------------------------

template <class T>
static __forceinline const T* AlignOnBlockBoundry(TransferData data, TransferFuncts fun, Point alignedPt, int& endY, const T* pbuf)
{
	bool bCanAlign = ((MOD_POW2(gs.trxpos.dx, data.blockwidth) == 0) && (gs.imageX == gs.trxpos.dx) &&
					  (alignedPt.y > endY) && (alignedPt.x > gs.trxpos.dx));

	if ((gs.imageEndX - gs.trxpos.dx) % data.widthlimit)
	{
		/* hack */
		int testwidth = (int)nSize -
						(gs.imageEndY - gs.imageY) * (gs.imageEndX - gs.trxpos.dx)
						+ (gs.imageX - gs.trxpos.dx);

		if ((testwidth <= data.widthlimit) && (testwidth >= -data.widthlimit))
		{
			/* don't transfer */
			/*ZZLog::Debug_Log("Bad texture %s: %d %d %d", #psm, gs.trxpos.dx, gs.imageEndX, nQWordSize);*/
			//ZZLog::Error_Log("Bad texture: testwidth = %d; data.widthlimit = %d", testwidth, data.widthlimit);
			gs.imageTransfer = -1;
		}

		bCanAlign = false;
	}

	/* first align on block boundary */
	if (MOD_POW2(gs.imageY, data.blockheight) || !bCanAlign)
	{
		u32 transwidth;

		if (!bCanAlign)
			endY = gs.imageEndY; /* transfer the whole image */
		else
			assert(endY < gs.imageEndY);  /* part of alignment condition */

		if (((gs.imageEndX - gs.trxpos.dx) % data.widthlimit) || ((gs.imageEndX - gs.imageX) % data.widthlimit))
		{
			/* transmit with a width of 1 */
			transwidth = (1 + (DSTPSM == PSMT4));
		}
		else
		{
			transwidth = data.widthlimit;
		}

		pbuf = TransmitHostLocalY<T>(data, fun.wp, transwidth, endY, pbuf);

		if (pbuf == NULL) return NULL;

		if (nSize == 0 || tempY == gs.imageEndY) return NULL;
	}

	return pbuf;
}

template <class T>
static __forceinline const T* TransferAligningToBlocks(TransferData data, TransferFuncts fun, Point alignedPt, const T* pbuf)
{
	bool bAligned;
	const u32 TSize = sizeof(T);
	_SwizzleBlock swizzle;

	/* can align! */
	pitch = gs.imageEndX - gs.trxpos.dx;
	area = pitch * data.blockheight;
	fracX = gs.imageEndX - alignedPt.x;

	/* on top of checking whether pbuf is aligned, make sure that the width is at least aligned to its limits (due to bugs in pcsx2) */
	bAligned = !((uptr)pbuf & 0xf) && (TransPitch(pitch, data.transfersize) & 0xf) == 0;

	if (bAligned || ((DSTPSM == PSMCT24) || (DSTPSM == PSMT8H) || (DSTPSM == PSMT4HH) || (DSTPSM == PSMT4HL)))
		swizzle = (fun.Swizzle);
	else
		swizzle = (fun.Swizzle_u);

	//Transfer aligning to blocks.
	for (; tempY < alignedPt.y && nSize >= area; tempY += data.blockheight, nSize -= area)
	{
		for (int tempj = gs.trxpos.dx; tempj < alignedPt.x; tempj += data.blockwidth, pbuf += TransPitch(data.blockwidth, data.transfersize) / TSize)
		{
			u8 *temp = pstart + fun.gp(tempj, tempY, gs.dstbuf.bw) * data.blockbits / 8;
			swizzle(temp, (u8*)pbuf, TransPitch(pitch, data.transfersize), 0xffffffff);
		}

		/* transfer the rest */
		if (alignedPt.x < gs.imageEndX)
		{
			pbuf = TransmitHostLocalX<T>(data, fun.wp, data.widthlimit, data.blockheight, alignedPt.x, pbuf);

			if (pbuf == NULL) return NULL;

			pbuf -= TransPitch((alignedPt.x - gs.trxpos.dx), data.transfersize) / TSize;
		}
		else
		{
			pbuf += (data.blockheight - 1) * TransPitch(pitch, data.transfersize) / TSize;
		}

		tempX = gs.trxpos.dx;
	}

	return pbuf;
}

static __forceinline int FinishTransfer(TransferData data, int nLeftOver)
{
	if (tempY >= gs.imageEndY)
	{
		assert(gs.imageTransfer == -1 || tempY == gs.imageEndY);
		gs.imageTransfer = -1;
		/*int start, end;
		ZeroGS::GetRectMemAddress(start, end, gs.dstbuf.psm, gs.trxpos.dx, gs.trxpos.dy, gs.imageWnew, gs.imageHnew, gs.dstbuf.bp, gs.dstbuf.bw);
		ZeroGS::g_MemTargs.ClearRange(start, end);*/
	}
	else
	{
		/* update new params */
		gs.imageY = tempY;
		gs.imageX = tempX;
	}

	return (nSize * TransPitch(2, data.transfersize) + nLeftOver) / 2;
}

template <class T>
static __forceinline int RealTransfer(TransferData data, TransferFuncts fun, const void* pbyMem, u32 nQWordSize)
{
	assert(gs.imageTransfer == 0);

	pstart = g_pbyGSMemory + gs.dstbuf.bp * 256;
	const T* pbuf = (const T*)pbyMem;
	const int tp2 = TransPitch(2, data.transfersize);
	int nLeftOver = (nQWordSize * 4 * 2) % tp2;
	tempY = gs.imageY;
	tempX = gs.imageX;
	Point alignedPt;

	nSize = (nQWordSize * 4 * 2) / tp2;
	nSize = min(nSize, gs.imageWnew * gs.imageHnew);

	int endY = ROUND_UPPOW2(gs.imageY, data.blockheight);
	alignedPt.y = ROUND_DOWNPOW2(gs.imageEndY, data.blockheight);
	alignedPt.x = ROUND_DOWNPOW2(gs.imageEndX, data.blockwidth);

	pbuf = AlignOnBlockBoundry<T>(data, fun, alignedPt, endY, pbuf);

	if (pbuf == NULL) return FinishTransfer(data, nLeftOver);

	pbuf = TransferAligningToBlocks<T>(data, fun, alignedPt, pbuf);

	if (pbuf == NULL) return FinishTransfer(data, nLeftOver);

	if (TransPitch(nSize, data.transfersize) / 4 > 0)
	{
		pbuf = TransmitHostLocalY<T>(data, fun.wp, data.widthlimit, gs.imageEndY, pbuf);

		if (pbuf == NULL) return FinishTransfer(data, nLeftOver);

		/* sometimes wrong sizes are sent (tekken tag) */
		assert(gs.imageTransfer == -1 || TransPitch(nSize, data.transfersize) / 4 <= 2);
	}

	return FinishTransfer(data, nLeftOver);
}

//DEFINE_TRANSFERLOCAL(32, u32, 2, 32, 8, 8, _, SwizzleBlock32);
int TransferHostLocal32(const void* pbyMem, u32 nQWordSize)
{
	TransferData data(2, 32, 8, 8, 32, PSM_);
	TransferFuncts fun(writePixel32_0, getPixelAddress32_0, SwizzleBlock32, SwizzleBlock32u);

	return RealTransfer<u32>(data, fun, pbyMem, nQWordSize);
}

//DEFINE_TRANSFERLOCAL(32Z, u32, 2, 32, 8, 8, _, SwizzleBlock32);
int TransferHostLocal32Z(const void* pbyMem, u32 nQWordSize)
{
	TransferData data(2, 32, 8, 8, 32, PSM_);
	TransferFuncts fun(writePixel32Z_0, getPixelAddress32Z_0, SwizzleBlock32, SwizzleBlock32u);

	return RealTransfer<u32>(data, fun, pbyMem, nQWordSize);
}

//DEFINE_TRANSFERLOCAL(24, u8, 8, 32, 8, 8, _24, SwizzleBlock24);
int TransferHostLocal24(const void* pbyMem, u32 nQWordSize)
{
	TransferData data(8, 32, 8, 8, 24, PSM_24_);
	TransferFuncts fun(writePixel24_0, getPixelAddress24_0, SwizzleBlock24, SwizzleBlock24u);

	return RealTransfer<u8>(data, fun, pbyMem, nQWordSize);
}

//DEFINE_TRANSFERLOCAL(24Z, u8, 8, 32, 8, 8, _24, SwizzleBlock24);
int TransferHostLocal24Z(const void* pbyMem, u32 nQWordSize)
{
	TransferData data(8, 32, 8, 8, 24, PSM_24_);
	TransferFuncts fun(writePixel24Z_0, getPixelAddress24Z_0, SwizzleBlock24, SwizzleBlock24u);

	return RealTransfer<u8>(data, fun, pbyMem, nQWordSize);
}

//DEFINE_TRANSFERLOCAL(16, u16, 4, 16, 16, 8, _, SwizzleBlock16);
int TransferHostLocal16(const void* pbyMem, u32 nQWordSize)
{
	TransferData data(4, 16, 16, 8, 16, PSM_);
	TransferFuncts fun(writePixel16_0, getPixelAddress16_0, SwizzleBlock16, SwizzleBlock16u);

	return RealTransfer<u16>(data, fun, pbyMem, nQWordSize);
}

//DEFINE_TRANSFERLOCAL(16S, u16, 4, 16, 16, 8, _, SwizzleBlock16);
int TransferHostLocal16S(const void* pbyMem, u32 nQWordSize)
{
	TransferData data(4, 16, 16, 8, 16, PSM_);
	TransferFuncts fun(writePixel16S_0, getPixelAddress16S_0, SwizzleBlock16, SwizzleBlock16u);

	return RealTransfer<u16>(data, fun, pbyMem, nQWordSize);
}

//DEFINE_TRANSFERLOCAL(16Z, u16, 4, 16, 16, 8, _, SwizzleBlock16);
int TransferHostLocal16Z(const void* pbyMem, u32 nQWordSize)
{
	TransferData data(4, 16, 16, 8, 16, PSM_);
	TransferFuncts fun(writePixel16Z_0, getPixelAddress16Z_0, SwizzleBlock16, SwizzleBlock16u);

	return RealTransfer<u16>(data, fun, pbyMem, nQWordSize);
}

//DEFINE_TRANSFERLOCAL(16SZ, u16, 4, 16, 16, 8, _, SwizzleBlock16);
int TransferHostLocal16SZ(const void* pbyMem, u32 nQWordSize)
{
	TransferData data(4, 16, 16, 8, 16, PSM_);
	TransferFuncts fun(writePixel16SZ_0, getPixelAddress16SZ_0, SwizzleBlock16, SwizzleBlock16u);

	return RealTransfer<u16>(data, fun, pbyMem, nQWordSize);
}

//DEFINE_TRANSFERLOCAL(8, u8, 4, 8, 16, 16, _, SwizzleBlock8);
int TransferHostLocal8(const void* pbyMem, u32 nQWordSize)
{
	TransferData data(4, 8, 16, 16, 8, PSM_);
	TransferFuncts fun(writePixel8_0, getPixelAddress8_0, SwizzleBlock8, SwizzleBlock8u);

	return RealTransfer<u8>(data, fun, pbyMem, nQWordSize);
}

//DEFINE_TRANSFERLOCAL(4, u8, 8, 4, 32, 16, _4, SwizzleBlock4);
int TransferHostLocal4(const void* pbyMem, u32 nQWordSize)
{
	TransferData data(8, 4, 32, 16, 4, PSM_4_);
	TransferFuncts fun(writePixel4_0, getPixelAddress4_0, SwizzleBlock4, SwizzleBlock4u);

	return RealTransfer<u8>(data, fun, pbyMem, nQWordSize);
}

//DEFINE_TRANSFERLOCAL(8H, u8, 4, 32, 8, 8, _, SwizzleBlock8H);
int TransferHostLocal8H(const void* pbyMem, u32 nQWordSize)
{
	TransferData data(4, 32, 8, 8, 8, PSM_);
	TransferFuncts fun(writePixel8H_0, getPixelAddress8H_0, SwizzleBlock8H, SwizzleBlock8Hu);

	return RealTransfer<u8>(data, fun, pbyMem, nQWordSize);
}

//DEFINE_TRANSFERLOCAL(4HL, u8, 8, 32, 8, 8, _4, SwizzleBlock4HL);
int TransferHostLocal4HL(const void* pbyMem, u32 nQWordSize)
{
	TransferData data(8, 32, 8, 8, 4, PSM_4_);
	TransferFuncts fun(writePixel4HL_0, getPixelAddress4HL_0, SwizzleBlock4HL, SwizzleBlock4HLu);

	return RealTransfer<u8>(data, fun, pbyMem, nQWordSize);
}

//DEFINE_TRANSFERLOCAL(4HH, u8, 8, 32, 8, 8, _4, SwizzleBlock4HH);
int TransferHostLocal4HH(const void* pbyMem, u32 nQWordSize)
{
	TransferData data(8, 32, 8, 8, 4, PSM_4_);
	TransferFuncts fun(writePixel4HH_0, getPixelAddress4HH_0, SwizzleBlock4HH, SwizzleBlock4HHu);

	return RealTransfer<u8>(data, fun, pbyMem, nQWordSize);
}

void TransferLocalHost32(void* pbyMem, u32 nQWordSize) 		{ FUNCLOG }
void TransferLocalHost24(void* pbyMem, u32 nQWordSize) 		{FUNCLOG}
void TransferLocalHost16(void* pbyMem, u32 nQWordSize)		{FUNCLOG}
void TransferLocalHost16S(void* pbyMem, u32 nQWordSize)		{FUNCLOG}
void TransferLocalHost8(void* pbyMem, u32 nQWordSize)		{}
void TransferLocalHost4(void* pbyMem, u32 nQWordSize)		{FUNCLOG}
void TransferLocalHost8H(void* pbyMem, u32 nQWordSize)		{FUNCLOG}
void TransferLocalHost4HL(void* pbyMem, u32 nQWordSize)		{FUNCLOG}
void TransferLocalHost4HH(void* pbyMem, u32 nQWordSize)		{}
void TransferLocalHost32Z(void* pbyMem, u32 nQWordSize)		{FUNCLOG}
void TransferLocalHost24Z(void* pbyMem, u32 nQWordSize)		{FUNCLOG}
void TransferLocalHost16Z(void* pbyMem, u32 nQWordSize)		{FUNCLOG}
void TransferLocalHost16SZ(void* pbyMem, u32 nQWordSize)	{FUNCLOG}

#define FILL_BLOCK(bw, bh, ox, oy, mult, psm, psmcol) { \
	b.vTexDims = Vector(BLOCK_TEXWIDTH/(float)(bw), BLOCK_TEXHEIGHT/(float)bh, 0, 0); \
	b.vTexBlock = Vector((float)bw/BLOCK_TEXWIDTH, (float)bh/BLOCK_TEXHEIGHT, ((float)ox+0.2f)/BLOCK_TEXWIDTH, ((float)oy+0.05f)/BLOCK_TEXHEIGHT); \
	b.width = bw; \
	b.height = bh; \
	b.colwidth = bh / 4; \
	b.colheight = bw / 8; \
	b.bpp = 32/mult; \
	\
	b.pageTable = &g_pageTable##psm[0][0]; \
	b.blockTable = &g_blockTable##psm[0][0]; \
	b.columnTable = &g_columnTable##psmcol[0][0]; \
	assert( sizeof(g_pageTable##psm) == bw*bh*sizeof(g_pageTable##psm[0][0]) ); \
	psrcf = (float*)&vBlockData[0] + ox + oy * BLOCK_TEXWIDTH; \
	psrcw = (u16*)&vBlockData[0] + ox + oy * BLOCK_TEXWIDTH; \
	for(i = 0; i < bh; ++i) { \
		for(j = 0; j < bw; ++j) { \
			/* fill the table */ \
			u32 u = g_blockTable##psm[(i / b.colheight)][(j / b.colwidth)] * 64 * mult + g_columnTable##psmcol[i%b.colheight][j%b.colwidth]; \
			b.pageTable[i*bw+j] = u; \
			if( floatfmt ) { \
				psrcf[i*BLOCK_TEXWIDTH+j] = (float)(u) / (float)(GPU_TEXWIDTH*mult); \
			} \
			else { \
				psrcw[i*BLOCK_TEXWIDTH+j] = u; \
			} \
		} \
	} \
	\
	if( floatfmt ) { \
		assert( floatfmt ); \
		psrcv = (Vector*)&vBilinearData[0] + ox + oy * BLOCK_TEXWIDTH; \
		for(i = 0; i < bh; ++i) { \
			for(j = 0; j < bw; ++j) { \
				Vector* pv = &psrcv[i*BLOCK_TEXWIDTH+j]; \
				pv->x = psrcf[i*BLOCK_TEXWIDTH+j]; \
				pv->y = psrcf[i*BLOCK_TEXWIDTH+((j+1)%bw)]; \
				pv->z = psrcf[((i+1)%bh)*BLOCK_TEXWIDTH+j]; \
				pv->w = psrcf[((i+1)%bh)*BLOCK_TEXWIDTH+((j+1)%bw)]; \
			} \
		} \
	} \
	b.getPixelAddress = getPixelAddress##psm; \
	b.getPixelAddress_0 = getPixelAddress##psm##_0; \
	b.writePixel = writePixel##psm; \
	b.writePixel_0 = writePixel##psm##_0; \
	b.readPixel = readPixel##psm; \
	b.readPixel_0 = readPixel##psm##_0; \
	b.TransferHostLocal = TransferHostLocal##psm; \
	b.TransferLocalHost = TransferLocalHost##psm; \
} \
 
void BLOCK::FillBlocks(vector<char>& vBlockData, vector<char>& vBilinearData, int floatfmt)
{
	FUNCLOG
	vBlockData.resize(BLOCK_TEXWIDTH * BLOCK_TEXHEIGHT * (floatfmt ? 4 : 2));

	if (floatfmt)
		vBilinearData.resize(BLOCK_TEXWIDTH * BLOCK_TEXHEIGHT * sizeof(Vector));

	int i, j;

	BLOCK b;

	float* psrcf = NULL;

	u16* psrcw = NULL;

	Vector* psrcv = NULL;

	memset(m_Blocks, 0, sizeof(m_Blocks));

	// 32
	FILL_BLOCK(64, 32, 0, 0, 1, 32, 32);

	m_Blocks[PSMCT32] = b;

	// 24 (same as 32 except write/readPixel are different)
	m_Blocks[PSMCT24] = b;
	m_Blocks[PSMCT24].writePixel = writePixel24;
	m_Blocks[PSMCT24].writePixel_0 = writePixel24_0;
	m_Blocks[PSMCT24].readPixel = readPixel24;
	m_Blocks[PSMCT24].readPixel_0 = readPixel24_0;
	m_Blocks[PSMCT24].TransferHostLocal = TransferHostLocal24;
	m_Blocks[PSMCT24].TransferLocalHost = TransferLocalHost24;

	// 8H (same as 32 except write/readPixel are different)
	m_Blocks[PSMT8H] = b;
	m_Blocks[PSMT8H].writePixel = writePixel8H;
	m_Blocks[PSMT8H].writePixel_0 = writePixel8H_0;
	m_Blocks[PSMT8H].readPixel = readPixel8H;
	m_Blocks[PSMT8H].readPixel_0 = readPixel8H_0;
	m_Blocks[PSMT8H].TransferHostLocal = TransferHostLocal8H;
	m_Blocks[PSMT8H].TransferLocalHost = TransferLocalHost8H;

	m_Blocks[PSMT4HL] = b;
	m_Blocks[PSMT4HL].writePixel = writePixel4HL;
	m_Blocks[PSMT4HL].writePixel_0 = writePixel4HL_0;
	m_Blocks[PSMT4HL].readPixel = readPixel4HL;
	m_Blocks[PSMT4HL].readPixel_0 = readPixel4HL_0;
	m_Blocks[PSMT4HL].TransferHostLocal = TransferHostLocal4HL;
	m_Blocks[PSMT4HL].TransferLocalHost = TransferLocalHost4HL;
	m_Blocks[PSMT4HH] = b;

	m_Blocks[PSMT4HH].writePixel = writePixel4HH;
	m_Blocks[PSMT4HH].writePixel_0 = writePixel4HH_0;
	m_Blocks[PSMT4HH].readPixel = readPixel4HH;
	m_Blocks[PSMT4HH].readPixel_0 = readPixel4HH_0;
	m_Blocks[PSMT4HH].TransferHostLocal = TransferHostLocal4HH;
	m_Blocks[PSMT4HH].TransferLocalHost = TransferLocalHost4HH;

	// 32z
	FILL_BLOCK(64, 32, 64, 0, 1, 32Z, 32);
	m_Blocks[PSMT32Z] = b;

	// 24Z (same as 32Z except write/readPixel are different)
	m_Blocks[PSMT24Z] = b;
	m_Blocks[PSMT24Z].writePixel = writePixel24Z;
	m_Blocks[PSMT24Z].writePixel_0 = writePixel24Z_0;
	m_Blocks[PSMT24Z].readPixel = readPixel24Z;
	m_Blocks[PSMT24Z].readPixel_0 = readPixel24Z_0;
	m_Blocks[PSMT24Z].TransferHostLocal = TransferHostLocal24Z;
	m_Blocks[PSMT24Z].TransferLocalHost = TransferLocalHost24Z;

	// 16
	FILL_BLOCK(64, 64, 0, 32, 2, 16, 16);
	m_Blocks[PSMCT16] = b;

	// 16s
	FILL_BLOCK(64, 64, 64, 32, 2, 16S, 16);
	m_Blocks[PSMCT16S] = b;

	// 16z
	FILL_BLOCK(64, 64, 0, 96, 2, 16Z, 16);
	m_Blocks[PSMT16Z] = b;

	// 16sz
	FILL_BLOCK(64, 64, 64, 96, 2, 16SZ, 16);
	m_Blocks[PSMT16SZ] = b;

	// 8
	FILL_BLOCK(128, 64, 0, 160, 4, 8, 8);
	m_Blocks[PSMT8] = b;

	// 4
	FILL_BLOCK(128, 128, 0, 224, 8, 4, 4);
	m_Blocks[PSMT4] = b;
}
