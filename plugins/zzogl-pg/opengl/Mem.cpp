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

		pbuf = TransmitHostLocalY<T>(data.psm, fun.wp, transwidth, endY, pbuf);

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
			pbuf = TransmitHostLocalX<T>(data.psm, fun.wp, data.widthlimit, data.blockheight, alignedPt.x, pbuf);

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
static __forceinline int RealTransfer(u32 psm, const void* pbyMem, u32 nQWordSize)
{
	assert(gs.imageTransfer == 0);
	TransferData data = tData[psm];
	TransferFuncts fun(psm);
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
		pbuf = TransmitHostLocalY<T>(psm, fun.wp, data.widthlimit, gs.imageEndY, pbuf);

		if (pbuf == NULL) return FinishTransfer(data, nLeftOver);

		/* sometimes wrong sizes are sent (tekken tag) */
		assert(gs.imageTransfer == -1 || TransPitch(nSize, data.transfersize) / 4 <= 2);
	}

	return FinishTransfer(data, nLeftOver);
}

int TransferHostLocal32(const void* pbyMem, u32 nQWordSize)  { return RealTransfer<u32>(PSMCT32, pbyMem, nQWordSize);  }
int TransferHostLocal32Z(const void* pbyMem, u32 nQWordSize) { return RealTransfer<u32>(PSMT32Z, pbyMem, nQWordSize);  }
int TransferHostLocal24(const void* pbyMem, u32 nQWordSize)  { return RealTransfer<u8>(PSMCT24, pbyMem, nQWordSize);   }
int TransferHostLocal24Z(const void* pbyMem, u32 nQWordSize) { return RealTransfer<u8>(PSMT24Z, pbyMem, nQWordSize);   }
int TransferHostLocal16(const void* pbyMem, u32 nQWordSize)  { return RealTransfer<u16>(PSMCT16, pbyMem, nQWordSize);  }
int TransferHostLocal16S(const void* pbyMem, u32 nQWordSize) { return RealTransfer<u16>(PSMCT16S, pbyMem, nQWordSize); }
int TransferHostLocal16Z(const void* pbyMem, u32 nQWordSize) { return RealTransfer<u16>(PSMT16Z, pbyMem, nQWordSize);  }
int TransferHostLocal16SZ(const void* pbyMem, u32 nQWordSize){ return RealTransfer<u16>(PSMT16SZ, pbyMem, nQWordSize); }
int TransferHostLocal8(const void* pbyMem, u32 nQWordSize)   { return RealTransfer<u8>(PSMT8, pbyMem, nQWordSize);     }
int TransferHostLocal4(const void* pbyMem, u32 nQWordSize)   { return RealTransfer<u8>(PSMT4, pbyMem, nQWordSize);     }
int TransferHostLocal8H(const void* pbyMem, u32 nQWordSize)  { return RealTransfer<u8>(PSMT8H, pbyMem, nQWordSize);    }
int TransferHostLocal4HL(const void* pbyMem, u32 nQWordSize) { return RealTransfer<u8>(PSMT4HL, pbyMem, nQWordSize);   }
int TransferHostLocal4HH(const void* pbyMem, u32 nQWordSize) { return RealTransfer<u8>(PSMT4HH, pbyMem, nQWordSize);   }

void TransferLocalHost32(void* pbyMem, u32 nQWordSize) 		{FUNCLOG}
void TransferLocalHost24(void* pbyMem, u32 nQWordSize) 		{FUNCLOG}
void TransferLocalHost16(void* pbyMem, u32 nQWordSize)		{FUNCLOG}
void TransferLocalHost16S(void* pbyMem, u32 nQWordSize)		{FUNCLOG}
void TransferLocalHost8(void* pbyMem, u32 nQWordSize)		{FUNCLOG}
void TransferLocalHost4(void* pbyMem, u32 nQWordSize)		{FUNCLOG}
void TransferLocalHost8H(void* pbyMem, u32 nQWordSize)		{FUNCLOG}
void TransferLocalHost4HL(void* pbyMem, u32 nQWordSize)		{FUNCLOG}
void TransferLocalHost4HH(void* pbyMem, u32 nQWordSize)		{FUNCLOG}
void TransferLocalHost32Z(void* pbyMem, u32 nQWordSize)		{FUNCLOG}
void TransferLocalHost24Z(void* pbyMem, u32 nQWordSize)		{FUNCLOG}
void TransferLocalHost16Z(void* pbyMem, u32 nQWordSize)		{FUNCLOG}
void TransferLocalHost16SZ(void* pbyMem, u32 nQWordSize)	{FUNCLOG}

#define FILL_BLOCK(psm, psmcol) \
{ \
	b.pageTable = &g_pageTable##psm[0][0]; \
	b.blockTable = &g_blockTable##psm[0][0]; \
	b.columnTable = &g_columnTable##psmcol[0][0]; \
	\
	assert( sizeof(g_pageTable##psm) == b.width * b.height * sizeof(g_pageTable##psm[0][0]) ); \
	\
	psrcf = (float*)&vBlockData[0] + b.ox + b.oy * BLOCK_TEXWIDTH; \
	psrcw = (u16*)&vBlockData[0] + b.ox + b.oy * BLOCK_TEXWIDTH; \
	\
	for(i = 0; i < b.height; ++i) \
	{ \
		u32 i_width = i*BLOCK_TEXWIDTH; \
		for(j = 0; j < b.width; ++j) \
		{ \
			/* fill the table */ \
			u32 u = g_blockTable##psm[(i / b.colheight)][(j / b.colwidth)] * 64 * b.mult + g_columnTable##psmcol[i%b.colheight][j%b.colwidth]; \
			b.pageTable[i * b.width + j] = u; \
			psrcf[i_width + j] = (float)(u) / (float)(GPU_TEXWIDTH * b.mult); \
		} \
	} \
	\
	psrcv = (float4*)&vBilinearData[0] + b.ox + b.oy * BLOCK_TEXWIDTH; \
	\
	for(i = 0; i < b.height; ++i) \
	{ \
		u32 i_width = i*BLOCK_TEXWIDTH; \
		u32 i_width2 = ((i+1)%b.height)*BLOCK_TEXWIDTH; \
		for(j = 0; j < b.width; ++j) \
		{ \
			u32 temp = ((j + 1) % b.width); \
			float4* pv = &psrcv[i_width + j]; \
			pv->x = psrcf[i_width + j]; \
			pv->y = psrcf[i_width + temp]; \
			pv->z = psrcf[i_width2 + j]; \
			pv->w = psrcf[i_width2 + temp]; \
		} \
	} \
} 

#define FILL_BLOCK_NF(psm, psmcol) \
{ \
	b.pageTable = &g_pageTable##psm[0][0]; \
	b.blockTable = &g_blockTable##psm[0][0]; \
	b.columnTable = &g_columnTable##psmcol[0][0]; \
	\
	assert( sizeof(g_pageTable##psm) == b.width * b.height * sizeof(g_pageTable##psm[0][0]) ); \
	\
	psrcf = (float*)&vBlockData[0] + b.ox + b.oy * BLOCK_TEXWIDTH; \
	psrcw = (u16*)&vBlockData[0] + b.ox + b.oy * BLOCK_TEXWIDTH; \
	\
	for(i = 0; i < b.height; ++i) \
	{ \
		u32 i_width = i*BLOCK_TEXWIDTH; \
		for(j = 0; j < b.width; ++j) \
		{ \
			/* fill the table */ \
			u32 u = g_blockTable##psm[(i / b.colheight)][(j / b.colwidth)] * 64 * b.mult + g_columnTable##psmcol[i%b.colheight][j%b.colwidth]; \
			b.pageTable[i * b.width + j] = u; \
			psrcw[i_width + j] = u; \
		} \
	} \
} 

void FillBlocksNF(vector<char>& vBlockData, vector<char>& vBilinearData)
{
	FUNCLOG
	vBlockData.resize(BLOCK_TEXWIDTH * BLOCK_TEXHEIGHT * 2);

	int i, j;
	BLOCK b;
	float* psrcf = NULL;
	u16* psrcw = NULL;

	memset(m_Blocks, 0, sizeof(m_Blocks));

	// 32
	b.SetDim(64, 32, 0, 0, 1);
	FILL_BLOCK_NF(32, 32);
	m_Blocks[PSMCT32] = b;
	m_Blocks[PSMCT32].SetFun(PSMCT32);

	// 24 (same as 32 except write/readPixel are different)
	m_Blocks[PSMCT24] = b;
	m_Blocks[PSMCT24].SetFun(PSMCT24);

	// 8H (same as 32 except write/readPixel are different)
	m_Blocks[PSMT8H] = b;
	m_Blocks[PSMT8H].SetFun(PSMT8H);

	m_Blocks[PSMT4HL] = b;
	m_Blocks[PSMT4HL].SetFun(PSMT4HL);
	
	m_Blocks[PSMT4HH] = b;
	m_Blocks[PSMT4HH].SetFun(PSMT4HH);

	// 32z
	b.SetDim(64, 32, 64, 0, 1);
	FILL_BLOCK_NF(32Z, 32);
	m_Blocks[PSMT32Z] = b;
	m_Blocks[PSMT32Z].SetFun(PSMT32Z);

	// 24Z (same as 32Z except write/readPixel are different)
	m_Blocks[PSMT24Z] = b;
	m_Blocks[PSMT24Z].SetFun(PSMT24Z);

	// 16
	b.SetDim(64, 64, 0, 32, 2);
	FILL_BLOCK_NF(16, 16);
	m_Blocks[PSMCT16] = b;
	m_Blocks[PSMCT16].SetFun(PSMCT16);

	// 16s
	b.SetDim(64, 64, 64, 32, 2);
	FILL_BLOCK_NF(16S, 16);
	m_Blocks[PSMCT16S] = b;
	m_Blocks[PSMCT16S].SetFun(PSMCT16S);

	// 16z
	b.SetDim(64, 64, 0, 96, 2);
	FILL_BLOCK_NF(16Z, 16);
	m_Blocks[PSMT16Z] = b;
	m_Blocks[PSMT16Z].SetFun(PSMT16Z);

	// 16sz
	b.SetDim(64, 64, 64, 96, 2);
	FILL_BLOCK_NF(16SZ, 16);
	m_Blocks[PSMT16SZ] = b;
	m_Blocks[PSMT16SZ].SetFun(PSMT16SZ);

	// 8
	b.SetDim(128, 64, 0, 160, 4);
	FILL_BLOCK_NF(8, 8);
	m_Blocks[PSMT8] = b;
	m_Blocks[PSMT8].SetFun(PSMT8);

	// 4
	b.SetDim(128, 128, 0, 224, 8);
	FILL_BLOCK_NF(4, 4);
	m_Blocks[PSMT4] = b;
	m_Blocks[PSMT4].SetFun(PSMT4);
}


void FillBlocksF(vector<char>& vBlockData, vector<char>& vBilinearData)
{
	FUNCLOG
	vBlockData.resize(BLOCK_TEXWIDTH * BLOCK_TEXHEIGHT * 4);
	vBilinearData.resize(BLOCK_TEXWIDTH * BLOCK_TEXHEIGHT * sizeof(float4));

	int i, j;
	BLOCK b;
	float* psrcf = NULL;
	u16* psrcw = NULL;
	float4* psrcv = NULL;

	memset(m_Blocks, 0, sizeof(m_Blocks));

	// 32
	b.SetDim(64, 32, 0, 0, 1);
	FILL_BLOCK(32, 32);
	m_Blocks[PSMCT32] = b;
	m_Blocks[PSMCT32].SetFun(PSMCT32);

	// 24 (same as 32 except write/readPixel are different)
	m_Blocks[PSMCT24] = b;
	m_Blocks[PSMCT24].SetFun(PSMCT24);

	// 8H (same as 32 except write/readPixel are different)
	m_Blocks[PSMT8H] = b;
	m_Blocks[PSMT8H].SetFun(PSMT8H);

	m_Blocks[PSMT4HL] = b;
	m_Blocks[PSMT4HL].SetFun(PSMT4HL);
	
	m_Blocks[PSMT4HH] = b;
	m_Blocks[PSMT4HH].SetFun(PSMT4HH);

	// 32z
	b.SetDim(64, 32, 64, 0, 1);
	FILL_BLOCK(32Z, 32);
	m_Blocks[PSMT32Z] = b;
	m_Blocks[PSMT32Z].SetFun(PSMT32Z);

	// 24Z (same as 32Z except write/readPixel are different)
	m_Blocks[PSMT24Z] = b;
	m_Blocks[PSMT24Z].SetFun(PSMT24Z);

	// 16
	b.SetDim(64, 64, 0, 32, 2);
	FILL_BLOCK(16, 16);
	m_Blocks[PSMCT16] = b;
	m_Blocks[PSMCT16].SetFun(PSMCT16);

	// 16s
	b.SetDim(64, 64, 64, 32, 2);
	FILL_BLOCK(16S, 16);
	m_Blocks[PSMCT16S] = b;
	m_Blocks[PSMCT16S].SetFun(PSMCT16S);

	// 16z
	b.SetDim(64, 64, 0, 96, 2);
	FILL_BLOCK(16Z, 16);
	m_Blocks[PSMT16Z] = b;
	m_Blocks[PSMT16Z].SetFun(PSMT16Z);

	// 16sz
	b.SetDim(64, 64, 64, 96, 2);
	FILL_BLOCK(16SZ, 16);
	m_Blocks[PSMT16SZ] = b;
	m_Blocks[PSMT16SZ].SetFun(PSMT16SZ);

	// 8
	b.SetDim(128, 64, 0, 160, 4);
	FILL_BLOCK(8, 8);
	m_Blocks[PSMT8] = b;
	m_Blocks[PSMT8].SetFun(PSMT8);

	// 4
	b.SetDim(128, 128, 0, 224, 8);
	FILL_BLOCK(4, 4);
	m_Blocks[PSMT4] = b;
	m_Blocks[PSMT4].SetFun(PSMT4);
}

void BLOCK::FillBlocks(vector<char>& vBlockData, vector<char>& vBilinearData, int floatfmt)
{
	FUNCLOG
	if (floatfmt) 
		FillBlocksF(vBlockData, vBilinearData);
	else
		FillBlocksNF(vBlockData, vBilinearData);
}
