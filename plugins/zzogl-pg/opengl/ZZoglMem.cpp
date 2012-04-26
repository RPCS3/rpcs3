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

#include "GS.h"
#include "Util.h"
#include "ZZoglMem.h"
#include "targets.h"
#include "x86.h"

#include "Mem_Swizzle.h"

#ifndef ZZNORMAL_MEMORY

bool allowed_psm[256] = {false, };			// Sometimes we got strange unknown psm
PSM_value PSM_value_Table[64] = {PSMT_BAD_PSM, };	// for int -> PSM_value

// return array of pointer of array string,
// We SHOULD do memory allocation for u32** -- otherwize we have a lot of trouble!
// if bw and bh are set correctly, as dimensions of table, than array have pointers
// to table rows, so array[i][j] = table[i][j];
inline u32** InitTable(int bh, int bw, u32* table) {
	u32** array = (u32**)malloc(bh * sizeof(u32*));
	for (int i = 0; i < bh; i++) {
		array[i] = &table[i * bw];
	}
	return array;
}

// initialize dynamic arrays (u32**) for each regular psm.
inline void SetTable(int psm) {
	switch (psm) {
		case PSMCT32:
			g_pageTable[psm]   = InitTable( 32,  64, &g_pageTable32[0][0]);
			g_blockTable[psm]  = InitTable(  4,   8, &g_blockTable32[0][0]);
			g_columnTable[psm] = InitTable(  8,   8, &g_columnTable32[0][0]);
			break;
			
		case PSMCT24:
			g_pageTable[psm]   = g_pageTable[PSMCT32];;
			g_blockTable[psm]  = InitTable(  4,   8, &g_blockTable32[0][0]);
			g_columnTable[psm] = InitTable(  8,   8, &g_columnTable32[0][0]);
			break;
			
		case PSMCT16:
			g_pageTable[psm]   = InitTable( 64,  64, &g_pageTable16[0][0]);
			g_blockTable[psm]  = InitTable(  8,   4, &g_blockTable16[0][0]);
			g_columnTable[psm] = InitTable(  8,  16, &g_columnTable16[0][0]);
			break;
			
		case PSMCT16S:
			g_pageTable[psm]   = InitTable( 64,  64, &g_pageTable16S[0][0]);
			g_blockTable[psm]  = InitTable(  8,   4, &g_blockTable16S[0][0]);
			g_columnTable[psm] = InitTable(  8,  16, &g_columnTable16[0][0]);
			break;
			
		case PSMT8:
			g_pageTable[psm]   = InitTable( 64, 128, &g_pageTable8[0][0]);
			g_blockTable[psm]  = InitTable(  4,   8, &g_blockTable8[0][0]);
			g_columnTable[psm] = InitTable( 16,  16, &g_columnTable8[0][0]);
			break;
			
		case PSMT8H:
			g_pageTable[psm]   = g_pageTable[PSMCT32];
			g_blockTable[psm]  = InitTable(  4,   8, &g_blockTable8[0][0]);
			g_columnTable[psm] = InitTable( 16,  16, &g_columnTable8[0][0]);
			break;
			
		case PSMT4:
			g_pageTable[psm]   = InitTable(128, 128, &g_pageTable4[0][0]);
			g_blockTable[psm]  = InitTable(  8,   4, &g_blockTable4[0][0]);
			g_columnTable[psm] = InitTable( 16,  32, &g_columnTable4[0][0]);
			break;	
					
		case PSMT4HL:
		case PSMT4HH:
			g_pageTable[psm]   = g_pageTable[PSMCT32];
			g_blockTable[psm]  = InitTable(  8,   4, &g_blockTable4[0][0]);
			g_columnTable[psm] = InitTable( 16,  32, &g_columnTable4[0][0]);
			break;
			
		case PSMT32Z:
			g_pageTable[psm]   = InitTable( 32,  64, &g_pageTable32Z[0][0]);
			g_blockTable[psm]  = InitTable(  4,   8, &g_blockTable32Z[0][0]);
			g_columnTable[psm] = InitTable(  8,   8, &g_columnTable32[0][0]);
			break;
			
		case PSMT24Z:
			g_pageTable[psm]   = g_pageTable[PSMT32Z];
			g_blockTable[psm]  = InitTable(  4,   8, &g_blockTable32Z[0][0]);
			g_columnTable[psm] = InitTable(  8,   8, &g_columnTable32[0][0]);
			break;
			
		case PSMT16Z:
			g_pageTable[psm]   = InitTable( 64,  64, &g_pageTable16Z[0][0]);
			g_blockTable[psm]  = InitTable(  8,   4, &g_blockTable16Z[0][0]);
			g_columnTable[psm] = InitTable(  8,  16, &g_columnTable16[0][0]);
			break;
			
		case PSMT16SZ:
			g_pageTable[psm]   = InitTable( 64,  64, &g_pageTable16SZ[0][0]);
			g_blockTable[psm]  = InitTable(  8,   4, &g_blockTable16SZ[0][0]);
			g_columnTable[psm] = InitTable(  8,  16, &g_columnTable16[0][0]);
			break;
	}
}

// After this, the function arrays with u32** have memory set and filled. 
void FillBlockTables() {
	for (int i = 0; i < MAX_PSM; i++) 
		SetTable(i);
}

// Deallocate memory for u32** arrays.
void DestroyBlockTables() {
	for (int i = 0; i < MAX_PSM; i++) {
		if (g_pageTable[i] != NULL && (i != PSMT8H && i != PSMT4HL && i != PSMT4HH && i != PSMCT24 && i != PSMT24Z))
			free(g_pageTable[i]);
			
		if (g_blockTable[i] != NULL)
		      	free(g_blockTable[i]);
		      	
		if (g_columnTable[i] != NULL)
			free(g_columnTable[i]);
	}
}

void FillNewPageTable() {
	int k = 0;
	for (int psm = 0; psm < MAX_PSM; psm ++)
		if (allowed_psm[psm]) {
			for (u32 i = 0; i < 127; i++)
				for(u32 j = 0; j < 127; j++) {
					u32 address;
					u32 shift;
					
					address = g_pageTable[psm][i & ZZ_DT[psm][3]][j & ZZ_DT[psm][4]];
					shift = (((address << ZZ_DT[psm][5]) & 0x7 ) << 3)+ ZZ_DT[psm][7]; 				// last part is for 8H, 4HL and 4HH -- they have data from 24 and 28 byte
					g_pageTable2[k][i][j] = (address >> ZZ_DT[psm][0]) + (shift << 16); 			// now lower 16 byte of page table is 32-bit aligned address, and upper -- 
																	// shift.
				}
			g_pageTableNew[psm]   = InitTable( 128,  128, &g_pageTable2[k][0][0]);
			k++;;
		}
}

BLOCK m_Blocks[MAX_PSM]; // Do so that blocks are indexable.

// At the begining and the end of each string we should made unaligned writes, with nSize checks. We should be sure that all
// these pixels are inside one widthlimit space.
template <int psm>
inline bool DoOneTransmitStep(void* pstart, int& nSize, int endj, const void* pbuf, int& k, int& i, int& j, int widthlimit) {
	for (; j < endj && nSize > 0; j++, k++, nSize -= 1) { 
		writePixelMem<psm, false>((u32*)pstart, j%2048, i%2048, (u32*)(pbuf), k, gs.dstbuf.bw); 
	}
	
	return (nSize == 0);
}

// FFX has PSMT8 transmit (starting intro -- sword and hairs).
// Persona 4 texts at start are PSMCT32 (and there is also PSMCT16 transmit somwhere after that).
// Tekken V has PSMCT24 and PSMT4 transfers

// This function transfers "Y" block pixels. I use little another code than Zerofrog. My code often uses widthmult != 1 addition (Zerofrog's code
// have an strict condition for fast path: width of transferred data should be widthlimit multiplied by j; EndY also should be multiplied. But
// the usual data block of 255 pixels becomes transfered by 1.
// I should check, maybe Unaligned_Start and Unaligned_End often == 0, and I could try a fastpath -- with this block off.
template <int psm, int widthlimit>
inline bool TRANSMIT_HOSTLOCAL_Y(u32* pbuf, int& nSize, u8* pstart, int endY, int& i, int& j, int& k) {
//	if (psm != PSMT8 && psm != 0 && psm != PSMT4 && psm != PSMCT24)
//		ERROR_LOG("This is usable function TRANSMIT_HOSTLOCAL_Y at ZZoglMem.cpp %d %d %d %d %d\n", psm, widthlimit, i, j, nSize);

	int q = (gs.trxpos.dx - j) % widthlimit; 
	if (DoOneTransmitStep<psm>(pstart, nSize, q, pbuf, k, i, j, widthlimit)) return true;						// After this j and dx are compatible by modyle of widthlimit
	
	int Unaligned_Start = (gs.trxpos.dx % widthlimit == 0) ? 0 : widthlimit - gs.trxpos.dx % widthlimit;					// gs.trpos.dx + Unaligned_Start is multiple of widthlimit
	for (; i < endY; ++i) {
		if (DoOneTransmitStep<psm>(pstart, nSize, j + Unaligned_Start, pbuf, k, i, j, widthlimit)) return true;			// This operation made j % widthlimit == 0.
		//assert (j % widthlimit != 0);												 

		for (; j < gs.imageEnd.x - widthlimit + 1 && nSize >= widthlimit; j += widthlimit, nSize -= widthlimit) { 			
			writePixelsFromMemory<psm, true, widthlimit>(pstart, pbuf, k, j % 2048, i % 2048,  gs.dstbuf.bw);
		}
	
		assert ( gs.imageEnd.x - j < widthlimit || nSize < widthlimit);	
		if (DoOneTransmitStep<psm>(pstart, nSize, gs.imageEnd.x, pbuf, k, i, j, widthlimit)) return true;				// There are 2 reasons for finish of previous for: 1) nSize < widthlimit
																		// 2) j > gs.imageEnd.x - widthlimit + 1. We would try to write pixels up do
																		// EndX, it's no more widthlimit pixels																		
		j = gs.trxpos.dx; 
	}	

	return false;
}

// PSMT4 -- Tekken V
template <int psm, int widthlimit>
inline void TRANSMIT_HOSTLOCAL_X(u32* pbuf, int& nSize, u8* pstart, int& i, int& j, int& k, int blockheight, int startX, int pitch, int fracX) {
	if (psm != PSMT8 && psm != PSMT4)
		ZZLog::Error_Log("This is usable function TRANSMIT_HOSTLOCAL_X at ZZoglMem.cpp %d %d %d %d %d\n", psm, widthlimit, i, j, nSize);

	for(int tempi = 0; tempi < blockheight; ++tempi) { 
		for(j = startX; j < gs.imageEnd.x; j++, k++) { 
			writePixelMem<psm, false>((u32*)pstart, j%2048, (i + tempi)%2048, (u32*)(pbuf), k, gs.dstbuf.bw); 
		} 
		k += ( pitch - fracX ); 
	} 
} 

template <int psm>
inline int TRANSMIT_PITCH(int pitch) {
	return (PSM_BITS_PER_PIXEL<psm>() * pitch) >> 3;
}

// ------------------------
// |              Y       |
// ------------------------
// |        block     |   |
// |   aligned area   | X |
// |                  |   |
// ------------------------
// |              Y       |
// ------------------------


template <int psmX>
int FinishTransfer(int i, int j, int nSize, int nLeftOver)
{
	if( i >= gs.imageEnd.y ) 
	{
		assert( gs.transferring == false || i == gs.imageEnd.y );
		gs.transferring = false;
	}
	else {
		/* update new params */
		gs.image.y = i;
		gs.image.x = j;
	}
	
	return (nSize * TRANSMIT_PITCH<psmX>(2) + nLeftOver)/2;
}
 
template<int psmX, int widthlimit, int blockbits, int blockwidth, int blockheight>
int TransferHostLocal(const void* pbyMem, u32 nQWordSize) 
{ 
	assert( gs.imageTransfer == XFER_HOST_TO_LOCAL );
	u8* pstart = g_pbyGSMemory + gs.dstbuf.bp*256;

	int i = gs.image.y, j = gs.image.x;
	
	const u8* pbuf = (const u8*)pbyMem;
	int nLeftOver = (nQWordSize*4*2)%(TRANSMIT_PITCH<psmX>(2));
	int nSize = nQWordSize*4*2/TRANSMIT_PITCH<psmX>(2);
	nSize = min(nSize, gs.imageNew.w * gs.imageNew.h);

	int pitch, area, fracX;
	int endY = ROUND_UPPOW2(i, blockheight);
	Point alignedPt;
	
	alignedPt.x = ROUND_DOWNPOW2(gs.imageEnd.x, blockwidth);
	alignedPt.y = ROUND_DOWNPOW2(gs.imageEnd.y, blockheight);
	
	bool bAligned;
	bool bCanAlign = MOD_POW2(gs.trxpos.dx, blockwidth) == 0 && (j == gs.trxpos.dx) && (alignedPt.y > endY) && alignedPt.x > gs.trxpos.dx;

	if( (gs.imageEnd.x - gs.trxpos.dx) % widthlimit ) {
		/* hack */
		int testwidth = (int)nSize - (gs.imageEnd.y - i) * (gs.imageEnd.x - gs.trxpos.dx) + (j - gs.trxpos.dx);
		if((testwidth <= widthlimit) && (testwidth >= -widthlimit)) {
			/* don't transfer */
			/*ZZLog::Debug_Log("bad texture %s: %d %d %d\n", #psm, gs.trxpos.dx, gs.imageEnd.x, nQWordSize);*/
			gs.transferring = false;
		}
		bCanAlign = false;
	}

	/* first align on block boundary */
	if( MOD_POW2(i, blockheight) || !bCanAlign ) {
	
		if( !bCanAlign )
			endY = gs.imageEnd.y; /* transfer the whole image */
		else
			assert( endY < gs.imageEnd.y); /* part of alignment condition */
		
		int limit = widthlimit;
		if (((gs.imageEnd.x - gs.trxpos.dx) % widthlimit) || ((gs.imageEnd.x - j) % widthlimit)) 
			/* transmit with a width of 1 */
			limit = 1 + (gs.dstbuf.psm == PSMT4);
		/*TRANSMIT_HOSTLOCAL_Y##TransSfx(psm, T, limit, endY)*/
		int k = 0;
		
		if (TRANSMIT_HOSTLOCAL_Y<psmX, widthlimit>((u32*)pbuf, nSize, pstart, endY, i, j, k)) 
			return FinishTransfer<psmX>(i, j, nSize, nLeftOver);
		
		pbuf += TRANSMIT_PITCH<psmX>(k);
		
		if (nSize == 0 || i == gs.imageEnd.y) return FinishTransfer<psmX>(i, j, nSize, nLeftOver);
	}

	assert( MOD_POW2(i, blockheight) == 0 && j == gs.trxpos.dx);

	/* can align! */
	pitch = gs.imageEnd.x - gs.trxpos.dx;
	area = pitch * blockheight;
	fracX = gs.imageEnd.x - alignedPt.x;

	/* on top of checking whether pbuf is aligned, make sure that the width is at least aligned to its limits (due to bugs in pcsx2) */
	bAligned = !((uptr)pbuf & 0xf) && ((TRANSMIT_PITCH<psmX>(pitch)&0xf) == 0);
	
	/* transfer aligning to blocks */
	for(; i < alignedPt.y && nSize >= area; i += blockheight, nSize -= area) {
	
		for(int tempj = gs.trxpos.dx; tempj < alignedPt.x; tempj += blockwidth, pbuf += TRANSMIT_PITCH<psmX>(blockwidth)) {
			SwizzleBlock<psmX>((u32*)(pstart + getPixelAddress<psmX>(tempj, i, gs.dstbuf.bw)*blockbits/8),
				(u32*)pbuf, TRANSMIT_PITCH<psmX>(pitch));
		}
	
		/* transfer the rest */
		if( alignedPt.x < gs.imageEnd.x ) {
			int k = 0;
			TRANSMIT_HOSTLOCAL_X<psmX, widthlimit>((u32*)pbuf, nSize, pstart, i, j, k, blockheight, alignedPt.x, pitch, fracX);
			pbuf += TRANSMIT_PITCH<psmX>(k - alignedPt.x + gs.trxpos.dx);
		}
		else pbuf += (blockheight-1)*TRANSMIT_PITCH<psmX>(pitch);
		j = gs.trxpos.dx;
	}

	if( TRANSMIT_PITCH<psmX>(nSize)/4 > 0 ) {
		int k = 0;
		TRANSMIT_HOSTLOCAL_Y<psmX, widthlimit>((u32*)pbuf, nSize, pstart, gs.imageEnd.y, i, j, k);
		pbuf += TRANSMIT_PITCH<psmX>(k);
		/* sometimes wrong sizes are sent (tekken tag) */
		assert( gs.transferring == false || TRANSMIT_PITCH<psmX>(nSize)/4 <= 2 );
	}
	
	return FinishTransfer<psmX>(i, j, nSize, nLeftOver);
}

inline int TransferHostLocal32(const void* pbyMem, u32 nQWordSize) 
{
	return TransferHostLocal<PSMCT32, 2, 32, 8, 8>( pbyMem, nQWordSize);
}

inline int TransferHostLocal32Z(const void* pbyMem, u32 nQWordSize) 
{
	return TransferHostLocal<PSMT32Z, 2, 32, 8, 8>( pbyMem, nQWordSize);
}

inline int TransferHostLocal24(const void* pbyMem, u32 nQWordSize) 
{
	return TransferHostLocal<PSMCT24, 8, 32, 8, 8>( pbyMem, nQWordSize);
}

inline int TransferHostLocal24Z(const void* pbyMem, u32 nQWordSize) 
{
	return TransferHostLocal<PSMT24Z, 8, 32, 8, 8>( pbyMem, nQWordSize);
}

inline int TransferHostLocal16(const void* pbyMem, u32 nQWordSize) 
{
	return TransferHostLocal<PSMCT16, 4, 16, 16, 8>( pbyMem, nQWordSize);
}

inline int TransferHostLocal16S(const void* pbyMem, u32 nQWordSize) 
{
	return TransferHostLocal<PSMCT16S, 4, 16, 16, 8>( pbyMem, nQWordSize);
}

inline int TransferHostLocal16Z(const void* pbyMem, u32 nQWordSize) 
{
	return TransferHostLocal<PSMT16Z, 4, 16, 16, 8>( pbyMem, nQWordSize);
}

inline int TransferHostLocal16SZ(const void* pbyMem, u32 nQWordSize) 
{
	return TransferHostLocal<PSMT16SZ, 4, 16, 16, 8>( pbyMem, nQWordSize);
}

inline int TransferHostLocal8(const void* pbyMem, u32 nQWordSize) 
{
	return TransferHostLocal<PSMT8, 4, 8, 16, 16>( pbyMem, nQWordSize);
}

inline int TransferHostLocal4(const void* pbyMem, u32 nQWordSize) 
{
	return TransferHostLocal<PSMT4, 8, 4, 32, 16>( pbyMem, nQWordSize);
}

inline int TransferHostLocal8H(const void* pbyMem, u32 nQWordSize) 
{
	return TransferHostLocal<PSMT8H, 4, 32, 8, 8>( pbyMem, nQWordSize);
}

inline int TransferHostLocal4HL(const void* pbyMem, u32 nQWordSize) 
{
	return TransferHostLocal<PSMT4HL, 8, 32, 8, 8>( pbyMem, nQWordSize);
}

inline int TransferHostLocal4HH(const void* pbyMem, u32 nQWordSize) 
{
	return TransferHostLocal<PSMT4HH, 8, 32, 8, 8>( pbyMem, nQWordSize);
}

void TransferLocalHost32(void* pbyMem, u32 nQWordSize) { FUNCLOG }
void TransferLocalHost24(void* pbyMem, u32 nQWordSize) { FUNCLOG }
void TransferLocalHost16(void* pbyMem, u32 nQWordSize) { FUNCLOG }
void TransferLocalHost16S(void* pbyMem, u32 nQWordSize) { FUNCLOG }
void TransferLocalHost8(void* pbyMem, u32 nQWordSize) { FUNCLOG }
void TransferLocalHost4(void* pbyMem, u32 nQWordSize) { FUNCLOG }
void TransferLocalHost8H(void* pbyMem, u32 nQWordSize) { FUNCLOG }
void TransferLocalHost4HL(void* pbyMem, u32 nQWordSize) { FUNCLOG }
void TransferLocalHost4HH(void* pbyMem, u32 nQWordSize) { FUNCLOG }
void TransferLocalHost32Z(void* pbyMem, u32 nQWordSize) { FUNCLOG }
void TransferLocalHost24Z(void* pbyMem, u32 nQWordSize) { FUNCLOG }
void TransferLocalHost16Z(void* pbyMem, u32 nQWordSize) { FUNCLOG }
void TransferLocalHost16SZ(void* pbyMem, u32 nQWordSize) { FUNCLOG }

inline void FILL_BLOCK(BLOCK& b, int floatfmt, vector<char>& vBlockData, vector<char>& vBilinearData, int ox, int oy, int psmX) { 
	int bw = ZZ_DT[psmX][4] + 1;
	int bh = ZZ_DT[psmX][3] + 1;
	int mult = 1 << ZZ_DT[psmX][0];

	b.vTexDims = float4 (BLOCK_TEXWIDTH/(float)(bw), BLOCK_TEXHEIGHT/(float)(bh), 0, 0); 
	b.vTexBlock = float4( (float)bw/BLOCK_TEXWIDTH, (float)bh/BLOCK_TEXHEIGHT, ((float)ox+0.2f)/BLOCK_TEXWIDTH, ((float)oy+0.05f)/BLOCK_TEXHEIGHT); 
	b.width = bw; 
	b.height = bh; 
	b.colwidth = bh / 4; 
	b.colheight = bw / 8; 
	b.bpp = 32/mult; 
	
	b.pageTable = g_pageTable[psmX]; 
	b.blockTable = g_blockTable[psmX]; 
	b.columnTable = g_columnTable[psmX]; 
	
	// This is never true.
	//assert( sizeof(g_pageTable[psmX]) == bw*bh*sizeof(g_pageTable[psmX][0][0]) ); 
	float* psrcf = (float*)&vBlockData[0] + ox + oy * BLOCK_TEXWIDTH; 
	u16* psrcw = (u16*)&vBlockData[0] + ox + oy * BLOCK_TEXWIDTH; 
	for(int i = 0; i < bh; ++i) { 
		for(int j = 0; j < bw; ++j) { 
			/* fill the table */ 
			u32 u = g_blockTable[psmX][(i / b.colheight)][(j / b.colwidth)] * 64 * mult + g_columnTable[psmX][i%b.colheight][j%b.colwidth]; 
			b.pageTable[i][j] = u; 
			if( floatfmt ) { 
				psrcf[i*BLOCK_TEXWIDTH+j] = (float)(u) / (float)(GPU_TEXWIDTH*mult); 
			} 
			else { 
				psrcw[i*BLOCK_TEXWIDTH+j] = u; 
			} 
		} 
	} 
	
	if( floatfmt ) { 
		float4* psrcv = (float4*)&vBilinearData[0] + ox + oy * BLOCK_TEXWIDTH; 
		for(int i = 0; i < bh; ++i) { 
			for(int j = 0; j < bw; ++j) { 
				float4* pv = &psrcv[i*BLOCK_TEXWIDTH+j]; 
				pv->x = psrcf[i*BLOCK_TEXWIDTH+j]; 
				pv->y = psrcf[i*BLOCK_TEXWIDTH+((j+1)%bw)]; 
				pv->z = psrcf[((i+1)%bh)*BLOCK_TEXWIDTH+j]; 
				pv->w = psrcf[((i+1)%bh)*BLOCK_TEXWIDTH+((j+1)%bw)]; 
			} 
		} 
	} 
}

void BLOCK::FillBlocks(vector<char>& vBlockData, vector<char>& vBilinearData, int floatfmt)
{
	FUNCLOG
	vBlockData.resize(BLOCK_TEXWIDTH * BLOCK_TEXHEIGHT * (floatfmt ? 4 : 2));
	
	if (floatfmt)
		vBilinearData.resize(BLOCK_TEXWIDTH * BLOCK_TEXHEIGHT * sizeof(float4));

	BLOCK b;

	memset(m_Blocks, 0, sizeof(m_Blocks));

	// 32
	FILL_BLOCK(b, floatfmt,  vBlockData, vBilinearData, 0, 0, PSMCT32);
	b.TransferHostLocal = TransferHostLocal32;
	b.TransferLocalHost = TransferLocalHost32;
	m_Blocks[PSMCT32] = b;

	// 24 (same as 32 except write/readPixel are different)
	b.TransferHostLocal = TransferHostLocal24;
	b.TransferLocalHost = TransferLocalHost24;
	 m_Blocks[PSMCT24] = b;

	// 8H (same as 32 except write/readPixel are different)
	b.TransferHostLocal = TransferHostLocal8H;
	b.TransferLocalHost = TransferLocalHost8H;	 
	m_Blocks[PSMT8H] = b;

	b.TransferHostLocal = TransferHostLocal4HL;
	b.TransferLocalHost = TransferLocalHost4HL;
	m_Blocks[PSMT4HL] = b;

	b.TransferHostLocal = TransferHostLocal4HH;
	b.TransferLocalHost = TransferLocalHost4HH;
	m_Blocks[PSMT4HH] = b;

	// 32z
	FILL_BLOCK(b, floatfmt, vBlockData, vBilinearData, 64, 0, PSMT32Z);
	b.TransferHostLocal = TransferHostLocal32Z;
	b.TransferLocalHost = TransferLocalHost32Z;
	m_Blocks[PSMT32Z] = b;

	// 24Z (same as 32Z except write/readPixel are different)
	b.TransferHostLocal = TransferHostLocal24Z;
	b.TransferLocalHost = TransferLocalHost24Z;
	m_Blocks[PSMT24Z] = b;

	// 16
	FILL_BLOCK(b, floatfmt,  vBlockData, vBilinearData,  0, 32, PSMCT16);
	b.TransferHostLocal = TransferHostLocal16;
	b.TransferLocalHost = TransferLocalHost16;
	m_Blocks[PSMCT16] = b;

	// 16s
	FILL_BLOCK(b, floatfmt,  vBlockData, vBilinearData,  64, 32, PSMCT16S);
	b.TransferHostLocal = TransferHostLocal16S;
	b.TransferLocalHost = TransferLocalHost16S;
	m_Blocks[PSMCT16S] = b;

	// 16z
	FILL_BLOCK(b, floatfmt,  vBlockData, vBilinearData,  0, 96, PSMT16Z);
	b.TransferHostLocal = TransferHostLocal16Z;
	b.TransferLocalHost = TransferLocalHost16Z;
	m_Blocks[PSMT16Z] = b;

	// 16sz
	FILL_BLOCK(b, floatfmt,  vBlockData, vBilinearData, 64, 96, PSMT16SZ);
	b.TransferHostLocal = TransferHostLocal16SZ;
	b.TransferLocalHost = TransferLocalHost16SZ;
	m_Blocks[PSMT16SZ] = b;

	// 8
	FILL_BLOCK(b, floatfmt,  vBlockData, vBilinearData,  0, 160, PSMT8);
	b.TransferHostLocal = TransferHostLocal8;
	b.TransferLocalHost = TransferLocalHost8;
	m_Blocks[PSMT8] = b;

	// 4
	FILL_BLOCK(b, floatfmt,  vBlockData, vBilinearData,  0, 224, PSMT4);
	b.TransferHostLocal = TransferHostLocal4;
	b.TransferLocalHost = TransferLocalHost4;
	m_Blocks[PSMT4] = b;
}

#endif
