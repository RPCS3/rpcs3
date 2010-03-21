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
#define DEFINE_TRANSFERLOCAL(psm, transfersize, T, widthlimit, blockbits, blockwidth, blockheight, TransSfx, SwizzleBlock) \
int TransferHostLocal##psm(const void* pbyMem, u32 nQWordSize) \
{ \
	assert( gs.imageTransfer == 0 ); \
	pstart = g_pbyGSMemory + gs.dstbuf.bp*256; \
	\
	/*const u8* pendbuf = (const u8*)pbyMem + nQWordSize*4;*/ \
	tempY = gs.imageY; tempX = gs.imageX; \
	const u32 TSize = sizeof(T); \
	_writePixel_0 wp = writePixel##psm##_0; \
	\
	const T* pbuf = (const T*)pbyMem; \
	const int tp = TransPitch(2, transfersize); \
	int nLeftOver = (nQWordSize*4*2)%tp; \
	nSize = nQWordSize*4*2/tp; \
	nSize = min(nSize, gs.imageWnew * gs.imageHnew); \
	\
	int endY = ROUND_UPPOW2(tempY, blockheight); \
	int alignedY = ROUND_DOWNPOW2(gs.imageEndY, blockheight); \
	int alignedX = ROUND_DOWNPOW2(gs.imageEndX, blockwidth); \
	bool bAligned, bCanAlign = MOD_POW2(gs.trxpos.dx, blockwidth) == 0 && (tempX == gs.trxpos.dx) && (alignedY > endY) && alignedX > gs.trxpos.dx; \
	\
	if( (gs.imageEndX-gs.trxpos.dx)%widthlimit ) { \
		/* hack */ \
		int testwidth = (int)nSize - (gs.imageEndY-tempY)*(gs.imageEndX-gs.trxpos.dx)+(tempX-gs.trxpos.dx); \
		if((testwidth <= widthlimit) && (testwidth >= -widthlimit)) { \
			/* don't transfer */ \
			/*DEBUG_LOG("bad texture %s: %d %d %d\n", #psm, gs.trxpos.dx, gs.imageEndX, nQWordSize);*/ \
			gs.imageTransfer = -1; \
		} \
		bCanAlign = false; \
	} \
	\
	/* first align on block boundary */ \
	if( MOD_POW2(tempY, blockheight) || !bCanAlign ) { \
		\
		if( !bCanAlign ) \
			endY = gs.imageEndY; /* transfer the whole image */ \
		else \
			assert( endY < gs.imageEndY); /* part of alignment condition */ \
		\
		if( ((gs.imageEndX-gs.trxpos.dx)%widthlimit) || ((gs.imageEndX-tempX)%widthlimit) ) { \
			/* transmit with a width of 1 */ \
			if (!TransmitHostLocalY##TransSfx<T>(wp, (1 + (DSTPSM == 0x14)), endY, pbuf)) goto End; \
		} \
		else { \
			if (!TransmitHostLocalY##TransSfx<T>(wp, widthlimit, endY, pbuf)) goto End; \
		} \
		\
		if( nSize == 0 || tempY == gs.imageEndY ) \
			goto End; \
	} \
	\
	assert( MOD_POW2(tempY, blockheight) == 0 && tempX == gs.trxpos.dx); \
	\
	/* can align! */ \
	pitch = gs.imageEndX-gs.trxpos.dx; \
	area = pitch*blockheight; \
	fracX = gs.imageEndX-alignedX; \
	\
	/* on top of checking whether pbuf is aligned, make sure that the width is at least aligned to its limits (due to bugs in pcsx2) */ \
	bAligned = !((uptr)pbuf & 0xf) && (TransPitch(pitch, transfersize) & 0xf) == 0; \
	\
	/* transfer aligning to blocks */ \
	for(; tempY < alignedY && nSize >= area; tempY += blockheight, nSize -= area) { \
		\
		if( bAligned || ((DSTPSM==PSMCT24) || (DSTPSM==PSMT8H) || (DSTPSM==PSMT4HH) || (DSTPSM==PSMT4HL)) ) { \
			for(int tempj = gs.trxpos.dx; tempj < alignedX; tempj += blockwidth, pbuf += TransPitch(blockwidth, transfersize)/TSize) { \
				SwizzleBlock(pstart + getPixelAddress_0(psm,tempj, tempY, gs.dstbuf.bw)*blockbits/8, \
					(u8*)pbuf, TransPitch(pitch, transfersize)); \
			} \
		} \
		else { \
			for(int tempj = gs.trxpos.dx; tempj < alignedX; tempj += blockwidth, pbuf += TransPitch(blockwidth, transfersize)/TSize) { \
				SwizzleBlock##u(pstart + getPixelAddress_0(psm,tempj, tempY, gs.dstbuf.bw)*blockbits/8, \
					(u8*)pbuf, TransPitch(pitch, transfersize)); \
			} \
		} \
		\
		/* transfer the rest */ \
		if( alignedX < gs.imageEndX ) { \
			if (!TransmitHostLocalX##TransSfx<T>(wp, widthlimit, blockheight, alignedX, pbuf)) goto End; \
			pbuf -= TransPitch(alignedX-gs.trxpos.dx, transfersize)/TSize; \
		} \
		else pbuf += (blockheight-1)* TransPitch(pitch, transfersize)/TSize; \
		tempX = gs.trxpos.dx; \
	} \
	\
	if( TransPitch(nSize, transfersize)/4 > 0 ) { \
		if (!TransmitHostLocalY##TransSfx<T>(wp, widthlimit, gs.imageEndY, pbuf)) goto End; \
		/* sometimes wrong sizes are sent (tekken tag) */ \
		assert( gs.imageTransfer == -1 || TransPitch(nSize, transfersize)/4 <= 2 ); \
	} \
	\
End: \
	if( tempY >= gs.imageEndY ) { \
		assert( gs.imageTransfer == -1 || tempY == gs.imageEndY ); \
		gs.imageTransfer = -1; \
		/*int start, end; \
		ZeroGS::GetRectMemAddress(start, end, gs.dstbuf.psm, gs.trxpos.dx, gs.trxpos.dy, gs.imageWnew, gs.imageHnew, gs.dstbuf.bp, gs.dstbuf.bw); \
		ZeroGS::g_MemTargs.ClearRange(start, end);*/ \
	} \
	else { \
		/* update new params */ \
		gs.imageY = tempY; \
		gs.imageX = tempX; \
	} \
	return (nSize * TransPitch(2, transfersize) + nLeftOver)/2; \
} \

#define NEW_TRANSFER
#ifdef NEW_TRANSFER

//DEFINE_TRANSFERLOCAL(32, u32, 2, 32, 8, 8, _, SwizzleBlock32);
int TransferHostLocal32(const void* pbyMem, u32 nQWordSize) 
{ 
	const u32 widthlimit = 2;
	const u32 blockbits = 32;
	const u32 blockwidth = 8;
	const u32 blockheight = 8;
	const u32 TSize = sizeof(u32);
	const u32 transfersize = 32;
	_SwizzleBlock swizzle;
	_writePixel_0 wp = writePixel32_0;

	assert( gs.imageTransfer == 0 ); 
	pstart = g_pbyGSMemory + gs.dstbuf.bp*256; 
	
	/*const u8* pendbuf = (const u8*)pbyMem + nQWordSize*4;*/ 
	tempY = gs.imageY; tempX = gs.imageX; 
	
	const u32* pbuf = (const u32*)pbyMem; 
	const int tp2 = TransPitch(2, transfersize);
	int nLeftOver = (nQWordSize*4*2)%tp2; 
	nSize = (nQWordSize*4*2)/tp2; 
	nSize = min(nSize, gs.imageWnew * gs.imageHnew); 
	
	int endY = ROUND_UPPOW2(gs.imageY, blockheight); 
	int alignedY = ROUND_DOWNPOW2(gs.imageEndY, blockheight); 
	int alignedX = ROUND_DOWNPOW2(gs.imageEndX, blockwidth); 
	bool bAligned;
	bool bCanAlign = ((MOD_POW2(gs.trxpos.dx, blockwidth) == 0) && (gs.imageX == gs.trxpos.dx) && (alignedY > endY) && (alignedX > gs.trxpos.dx)); 
	
	if ((gs.imageEndX - gs.trxpos.dx) % widthlimit) 
	{ 
		/* hack */ 
		int testwidth = (int)nSize - 
			(gs.imageEndY - gs.imageY) * (gs.imageEndX - gs.trxpos.dx)
			 + (gs.imageX - gs.trxpos.dx); 
			 
		if((testwidth <= widthlimit) && (testwidth >= -widthlimit)) 
		{ 
			/* don't transfer */ 
			/*DEBUG_LOG("bad texture %s: %d %d %d\n", #psm, gs.trxpos.dx, gs.imageEndX, nQWordSize);*/ 
			gs.imageTransfer = -1; 
		} 
		bCanAlign = false; 
	} 
	
	/* first align on block boundary */ 
	if ( MOD_POW2(gs.imageY, blockheight) || !bCanAlign ) 
	{ 
		
		if( !bCanAlign ) 
			endY = gs.imageEndY; /* transfer the whole image */ 
		else 
			assert( endY < gs.imageEndY); /* part of alignment condition */ 
		
		if (((gs.imageEndX - gs.trxpos.dx) % widthlimit) || ((gs.imageEndX - gs.imageX) % widthlimit)) 
		{ 
			/* transmit with a width of 1 */ 
			if (!TransmitHostLocalY_<u32>(wp, (1 + (DSTPSM == 0x14)), endY, pbuf)) goto End;
		} 
		else 
		{ 
			if (!TransmitHostLocalY_<u32>(wp, widthlimit, endY, pbuf)) goto End;
		} 
		
		if( nSize == 0 || tempY == gs.imageEndY ) goto End; 
	} 
	
	//assert( MOD_POW2(tempY, blockheight) == 0 && tempX == gs.trxpos.dx); 
	
	/* can align! */ 
	pitch = gs.imageEndX - gs.trxpos.dx; 
	area = pitch * blockheight; 
	fracX = gs.imageEndX - alignedX; 
	
	/* on top of checking whether pbuf is aligned, make sure that the width is at least aligned to its limits (due to bugs in pcsx2) */ 
	bAligned = !((uptr)pbuf & 0xf) && (TransPitch(pitch, transfersize) & 0xf) == 0; 
	
	/* transfer aligning to blocks */ 
	for(; tempY < alignedY && nSize >= area; tempY += blockheight, nSize -= area) 
	{ 
		if ( bAligned || ((DSTPSM==PSMCT24) || (DSTPSM==PSMT8H) || (DSTPSM==PSMT4HH) || (DSTPSM==PSMT4HL))) 
			swizzle = SwizzleBlock32;
		else 
			swizzle = SwizzleBlock32u;
			
		for(int tempj = gs.trxpos.dx; tempj < alignedX; tempj += blockwidth, pbuf += TransPitch(blockwidth, transfersize)/TSize) 
		{ 
				u8 *temp = pstart + getPixelAddress_0(32, tempj, tempY, gs.dstbuf.bw)*blockbits/8;
				swizzle(temp, (u8*)pbuf, TransPitch(pitch, transfersize), 0xffffffff); 
		} 
		
		/* transfer the rest */ 
		if( alignedX < gs.imageEndX ) 
		{ 
			if (!TransmitHostLocalX_<u32>(wp, widthlimit, blockheight, alignedX, pbuf)) goto End;
			pbuf -= TransPitch((alignedX - gs.trxpos.dx), transfersize)/TSize; 
		} 
		else 
		{
			pbuf += (blockheight - 1)* TransPitch(pitch, transfersize)/TSize; 
		}
		
		tempX = gs.trxpos.dx; 
	} 
	
	if (TransPitch(nSize, transfersize)/4 > 0) 
	{ 
		if (!TransmitHostLocalY_<u32>(wp, widthlimit, gs.imageEndY, pbuf)) goto End;
		/* sometimes wrong sizes are sent (tekken tag) */ 
		assert( gs.imageTransfer == -1 || TransPitch(nSize, transfersize)/4 <= 2 ); 
	} 
	
End: if( tempY >= gs.imageEndY ) 
	{ 
		assert( gs.imageTransfer == -1 || tempY == gs.imageEndY ); 
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
	
	return (nSize * TransPitch(2, transfersize) + nLeftOver)/2; 
} 

//DEFINE_TRANSFERLOCAL(32Z, u32, 2, 32, 8, 8, _, SwizzleBlock32);
int TransferHostLocal32Z(const void* pbyMem, u32 nQWordSize) 
{ 
	const u32 widthlimit = 2;
	const u32 blockbits = 32;
	const u32 blockwidth = 8;
	const u32 blockheight = 8;
	const u32 TSize = sizeof(u32);
	const u32 transfersize = 32;
	_writePixel_0 wp = writePixel32Z_0;
	
	assert( gs.imageTransfer == 0 ); 
	pstart = g_pbyGSMemory + gs.dstbuf.bp*256; 
	
	/*const u8* pendbuf = (const u8*)pbyMem + nQWordSize*4;*/ 
	tempY = gs.imageY; tempX = gs.imageX; 
	
	const u32* pbuf = (const u32*)pbyMem; 
	int nLeftOver = (nQWordSize*4*2)%(TransPitch(2, transfersize)); 
	nSize = nQWordSize*4*2/ TransPitch(2, transfersize); 
	nSize = min(nSize, gs.imageWnew * gs.imageHnew); 
	
	int endY = ROUND_UPPOW2(tempY, blockheight); 
	int alignedY = ROUND_DOWNPOW2(gs.imageEndY, blockheight); 
	int alignedX = ROUND_DOWNPOW2(gs.imageEndX, blockwidth); 
	bool bAligned, bCanAlign = MOD_POW2(gs.trxpos.dx, blockwidth) == 0 && (tempX == gs.trxpos.dx) && (alignedY > endY) && alignedX > gs.trxpos.dx; 
	
	if ((gs.imageEndX-gs.trxpos.dx)%widthlimit) 
	{ 
		/* hack */ 
		int testwidth = (int)nSize - (gs.imageEndY-tempY)*(gs.imageEndX-gs.trxpos.dx)+(tempX-gs.trxpos.dx); 
		if ((testwidth <= widthlimit) && (testwidth >= -widthlimit)) 
		{ 
			/* don't transfer */ 
			/*DEBUG_LOG("bad texture %s: %d %d %d\n", #psm, gs.trxpos.dx, gs.imageEndX, nQWordSize);*/ 
			gs.imageTransfer = -1; 
		} 
		bCanAlign = false; 
	} 
	
	/* first align on block boundary */ 
	if ( MOD_POW2(tempY, blockheight) || !bCanAlign ) 
	{ 
		
		if ( !bCanAlign ) 
			endY = gs.imageEndY; /* transfer the whole image */ 
		else 
			assert( endY < gs.imageEndY); /* part of alignment condition */ 
		
		if (((gs.imageEndX-gs.trxpos.dx)%widthlimit) || ((gs.imageEndX-tempX)%widthlimit)) 
		{ 
			/* transmit with a width of 1 */ 
			if (!TransmitHostLocalY_<u32>(wp, (1+(DSTPSM == 0x14)), endY, pbuf)) goto End;
		} 
		else 
		{ 
			if (!TransmitHostLocalY_<u32>(wp, widthlimit, endY, pbuf)) goto End;
		} 
		
		if ( nSize == 0 || tempY == gs.imageEndY ) goto End; 
	} 
	
	assert( MOD_POW2(tempY, blockheight) == 0 && tempX == gs.trxpos.dx); 
	
	/* can align! */ 
	pitch = gs.imageEndX-gs.trxpos.dx; 
	area = pitch*blockheight; 
	fracX = gs.imageEndX-alignedX; 
	
	/* on top of checking whether pbuf is aligned, make sure that the width is at least aligned to its limits (due to bugs in pcsx2) */ 
	bAligned = !((uptr)pbuf & 0xf) && (TransPitch(pitch, transfersize)&0xf) == 0; 
	
	/* transfer aligning to blocks */ 
	for(; tempY < alignedY && nSize >= area; tempY += blockheight, nSize -= area) 
	{ 
		if( bAligned || ((DSTPSM==PSMCT24) || (DSTPSM==PSMT8H) || (DSTPSM==PSMT4HH) || (DSTPSM==PSMT4HL)) ) 
		{ 
			for(int tempj = gs.trxpos.dx; tempj < alignedX; tempj += blockwidth, pbuf += TransPitch(blockwidth, transfersize)/TSize) 
			{ 
				SwizzleBlock32(pstart + getPixelAddress_0(32Z,tempj, tempY, gs.dstbuf.bw)*blockbits/8,  (u8*)pbuf, TransPitch(pitch, transfersize)); 
			} 
		} 
		else 
		{ 
			for(int tempj = gs.trxpos.dx; tempj < alignedX; tempj += blockwidth, pbuf += TransPitch(blockwidth, transfersize)/TSize) 
			{ 
				SwizzleBlock32u(pstart + getPixelAddress_0(32Z,tempj, tempY, gs.dstbuf.bw)*blockbits/8, (u8*)pbuf, TransPitch(pitch, transfersize)); 
			} 
		} 
		
		/* transfer the rest */ 
		if ( alignedX < gs.imageEndX ) 
		{ 
			if (!TransmitHostLocalX_<u32>(wp, widthlimit, blockheight, alignedX, pbuf)) goto End;
			pbuf -= TransPitch((alignedX - gs.trxpos.dx), transfersize)/TSize; 
		} 
		else 
		{
			pbuf += (blockheight-1)*TransPitch(pitch, transfersize)/TSize;
		}
		tempX = gs.trxpos.dx; 
	} 
	
	if (TransPitch(nSize, transfersize)/4 > 0 ) 
	{ 
		if (!TransmitHostLocalY_<u32>(wp, widthlimit, gs.imageEndY, pbuf)) goto End;
		/* sometimes wrong sizes are sent (tekken tag) */ 
		assert( gs.imageTransfer == -1 || TransPitch(nSize, transfersize)/4 <= 2 ); 
	} 
	
	End: 
	if( tempY >= gs.imageEndY ) { 
		assert( gs.imageTransfer == -1 || tempY == gs.imageEndY ); 
		gs.imageTransfer = -1; 
		/*int start, end; 
		ZeroGS::GetRectMemAddress(start, end, gs.dstbuf.psm, gs.trxpos.dx, gs.trxpos.dy, gs.imageWnew, gs.imageHnew, gs.dstbuf.bp, gs.dstbuf.bw); 
		ZeroGS::g_MemTargs.ClearRange(start, end);*/ 
	} 
	else { 
		/* update new params */ 
		gs.imageY = tempY; 
		gs.imageX = tempX; 
	} 
	return (nSize * TransPitch(2, transfersize) + nLeftOver)/2; 
} 

//DEFINE_TRANSFERLOCAL(24, u8, 8, 32, 8, 8, _24, SwizzleBlock24);
int TransferHostLocal24(const void* pbyMem, u32 nQWordSize) 
{ 
	const u32 widthlimit = 8;
	const u32 blockbits = 32;
	const u32 blockwidth = 8;
	const u32 blockheight = 8;
	const u32 TSize = sizeof(u8);
	const u32 transfersize = 24;
	_writePixel_0 wp = writePixel24_0;
	
	assert( gs.imageTransfer == 0 ); 
	pstart = g_pbyGSMemory + gs.dstbuf.bp*256; 
	
	/*const u8* pendbuf = (const u8*)pbyMem + nQWordSize*4;*/ 
	tempY = gs.imageY; tempX = gs.imageX; 
	
	const u8* pbuf = (const u8*)pbyMem; 
	int nLeftOver = (nQWordSize*4*2)%(TransPitch(2, transfersize)); 
	nSize = nQWordSize*4*2/ TransPitch(2, transfersize); 
	nSize = min(nSize, gs.imageWnew * gs.imageHnew); 
	
	int endY = ROUND_UPPOW2(tempY, blockheight); 
	int alignedY = ROUND_DOWNPOW2(gs.imageEndY, blockheight); 
	int alignedX = ROUND_DOWNPOW2(gs.imageEndX, blockwidth); 
	bool bAligned, bCanAlign = MOD_POW2(gs.trxpos.dx, blockwidth) == 0 && (tempX == gs.trxpos.dx) && (alignedY > endY) && alignedX > gs.trxpos.dx; 
	
	if ((gs.imageEndX-gs.trxpos.dx)%widthlimit) 
	{ 
		/* hack */ 
		int testwidth = (int)nSize - (gs.imageEndY-tempY)*(gs.imageEndX-gs.trxpos.dx)+(tempX-gs.trxpos.dx); 
		if ((testwidth <= widthlimit) && (testwidth >= -widthlimit)) 
		{ 
			/* don't transfer */ 
			/*DEBUG_LOG("bad texture %s: %d %d %d\n", #psm, gs.trxpos.dx, gs.imageEndX, nQWordSize);*/ 
			gs.imageTransfer = -1; 
		} 
		bCanAlign = false; 
	} 
	
	/* first align on block boundary */ 
	if ( MOD_POW2(tempY, blockheight) || !bCanAlign ) 
	{ 
		
		if ( !bCanAlign ) 
			endY = gs.imageEndY; /* transfer the whole image */ 
		else 
			assert( endY < gs.imageEndY); /* part of alignment condition */ 
		
		if (((gs.imageEndX-gs.trxpos.dx)%widthlimit) || ((gs.imageEndX-tempX)%widthlimit)) 
		{ 
			/* transmit with a width of 1 */ 
			if (!TransmitHostLocalY_24<u8>(wp, (1+(DSTPSM == 0x14)), endY, pbuf)) goto End;
		} 
		else 
		{ 
			if (!TransmitHostLocalY_24<u8>(wp, widthlimit, endY, pbuf)) goto End;
		} 
		
		if( nSize == 0 || tempY == gs.imageEndY ) goto End; 
	} 
	
	assert( MOD_POW2(tempY, blockheight) == 0 && tempX == gs.trxpos.dx); 
	
	/* can align! */ 
	pitch = gs.imageEndX-gs.trxpos.dx; 
	area = pitch*blockheight; 
	fracX = gs.imageEndX-alignedX; 
	
	/* on top of checking whether pbuf is aligned, make sure that the width is at least aligned to its limits (due to bugs in pcsx2) */ 
	bAligned = !((uptr)pbuf & 0xf) && (TransPitch(pitch, transfersize) & 0xf) == 0; 
	
	/* transfer aligning to blocks */ 
	for(; tempY < alignedY && nSize >= area; tempY += blockheight, nSize -= area) 
	{ 
		if( bAligned || ((DSTPSM==PSMCT24) || (DSTPSM==PSMT8H) || (DSTPSM==PSMT4HH) || (DSTPSM==PSMT4HL)) ) 
		{ 
			for(int tempj = gs.trxpos.dx; tempj < alignedX; tempj += blockwidth, pbuf += TransPitch(blockwidth, transfersize)/TSize) 
			{ 
				SwizzleBlock24(pstart + getPixelAddress_0(24,tempj, tempY, gs.dstbuf.bw)*blockbits/8,  (u8*)pbuf, TransPitch(pitch, transfersize)); 
			} 
		} 
		else 
		{ 
			for(int tempj = gs.trxpos.dx; tempj < alignedX; tempj += blockwidth, pbuf += TransPitch(blockwidth, transfersize)/TSize) 
			{ 
				SwizzleBlock24u(pstart + getPixelAddress_0(24,tempj, tempY, gs.dstbuf.bw)*blockbits/8, (u8*)pbuf, TransPitch(pitch, transfersize)); 
			} 
		} 
		
		/* transfer the rest */ 
		if ( alignedX < gs.imageEndX ) 
		{ 
			if (!TransmitHostLocalX_24<u8>(wp, widthlimit, blockheight, alignedX, pbuf)) goto End;
			pbuf -= TransPitch((alignedX-gs.trxpos.dx), transfersize)/TSize; 
		} 
		else 
		{
			pbuf += (blockheight-1)*TransPitch(pitch, transfersize)/TSize;
		}
		tempX = gs.trxpos.dx; 
	} 
	
	if (TransPitch(nSize, transfersize)/4 > 0 ) 
	{ 
		if (!TransmitHostLocalY_24<u8>(wp, widthlimit, gs.imageEndY, pbuf)) goto End;
		/* sometimes wrong sizes are sent (tekken tag) */ 
		assert( gs.imageTransfer == -1 || TransPitch(nSize, transfersize)/4 <= 2 ); 
	} 
	
	End: 
	if( tempY >= gs.imageEndY ) { 
		assert( gs.imageTransfer == -1 || tempY == gs.imageEndY ); 
		gs.imageTransfer = -1; 
		/*int start, end; 
		ZeroGS::GetRectMemAddress(start, end, gs.dstbuf.psm, gs.trxpos.dx, gs.trxpos.dy, gs.imageWnew, gs.imageHnew, gs.dstbuf.bp, gs.dstbuf.bw); 
		ZeroGS::g_MemTargs.ClearRange(start, end);*/ 
	} 
	else { 
		/* update new params */ 
		gs.imageY = tempY; 
		gs.imageX = tempX; 
	} 
	return (nSize * TransPitch(2, transfersize) + nLeftOver)/2; 
} 

//DEFINE_TRANSFERLOCAL(24Z, u8, 8, 32, 8, 8, _24, SwizzleBlock24);
int TransferHostLocal24Z(const void* pbyMem, u32 nQWordSize) 
{ 
	const u32 widthlimit = 8;
	const u32 blockbits = 32;
	const u32 blockwidth = 8;
	const u32 blockheight = 8;
	const u32 TSize = sizeof(u8);
	const u32 transfersize = 24;
	_writePixel_0 wp = writePixel24Z_0;
	
	assert( gs.imageTransfer == 0 ); 
	pstart = g_pbyGSMemory + gs.dstbuf.bp*256; 
	
	/*const u8* pendbuf = (const u8*)pbyMem + nQWordSize*4;*/ 
	tempY = gs.imageY; tempX = gs.imageX; 
	
	const u8* pbuf = (const u8*)pbyMem; 
	int nLeftOver = (nQWordSize*4*2)%(TransPitch(2, transfersize)); 
	nSize = nQWordSize*4*2/ TransPitch(2, transfersize); 
	nSize = min(nSize, gs.imageWnew * gs.imageHnew); 
	
	int endY = ROUND_UPPOW2(tempY, blockheight); 
	int alignedY = ROUND_DOWNPOW2(gs.imageEndY, blockheight); 
	int alignedX = ROUND_DOWNPOW2(gs.imageEndX, blockwidth); 
	bool bAligned, bCanAlign = MOD_POW2(gs.trxpos.dx, blockwidth) == 0 && (tempX == gs.trxpos.dx) && (alignedY > endY) && alignedX > gs.trxpos.dx; 
	
	if ((gs.imageEndX-gs.trxpos.dx)%widthlimit) 
	{ 
		/* hack */ 
		int testwidth = (int)nSize - (gs.imageEndY-tempY)*(gs.imageEndX-gs.trxpos.dx)+(tempX-gs.trxpos.dx); 
		if ((testwidth <= widthlimit) && (testwidth >= -widthlimit)) 
		{ 
			/* don't transfer */ 
			/*DEBUG_LOG("bad texture %s: %d %d %d\n", #psm, gs.trxpos.dx, gs.imageEndX, nQWordSize);*/ 
			gs.imageTransfer = -1; 
		} 
		bCanAlign = false; 
	} 
	
	/* first align on block boundary */ 
	if ( MOD_POW2(tempY, blockheight) || !bCanAlign ) 
	{ 
		
		if ( !bCanAlign ) 
			endY = gs.imageEndY; /* transfer the whole image */ 
		else 
			assert( endY < gs.imageEndY); /* part of alignment condition */ 
		
		if (((gs.imageEndX-gs.trxpos.dx)%widthlimit) || ((gs.imageEndX-tempX)%widthlimit)) 
		{ 
			/* transmit with a width of 1 */ 
			if (!TransmitHostLocalY_24<u8>(wp, (1+(DSTPSM == 0x14)), endY, pbuf)) goto End;
		} 
		else 
		{ 
			if (!TransmitHostLocalY_24<u8>(wp, widthlimit, endY, pbuf)) goto End;
		} 
		
		if( nSize == 0 || tempY == gs.imageEndY ) goto End; 
	} 
	
	assert( MOD_POW2(tempY, blockheight) == 0 && tempX == gs.trxpos.dx); 
	
	/* can align! */ 
	pitch = gs.imageEndX-gs.trxpos.dx; 
	area = pitch*blockheight; 
	fracX = gs.imageEndX-alignedX; 
	
	/* on top of checking whether pbuf is aligned, make sure that the width is at least aligned to its limits (due to bugs in pcsx2) */ 
	bAligned = !((uptr)pbuf & 0xf) && (TransPitch(pitch, transfersize)&0xf) == 0; 
	
	/* transfer aligning to blocks */ 
	for(; tempY < alignedY && nSize >= area; tempY += blockheight, nSize -= area) 
	{ 
		if( bAligned || ((DSTPSM==PSMCT24) || (DSTPSM==PSMT8H) || (DSTPSM==PSMT4HH) || (DSTPSM==PSMT4HL)) ) 
		{ 
			for(int tempj = gs.trxpos.dx; tempj < alignedX; tempj += blockwidth, pbuf += TransPitch(blockwidth, transfersize)/TSize) 
			{ 
				SwizzleBlock24(pstart + getPixelAddress_0(16,tempj, tempY, gs.dstbuf.bw)*blockbits/8,  (u8*)pbuf, TransPitch(pitch, transfersize)); 
			} 
		} 
		else 
		{ 
			for(int tempj = gs.trxpos.dx; tempj < alignedX; tempj += blockwidth, pbuf += TransPitch(blockwidth, transfersize)/TSize) 
			{ 
				SwizzleBlock24u(pstart + getPixelAddress_0(16,tempj, tempY, gs.dstbuf.bw)*blockbits/8, (u8*)pbuf, TransPitch(pitch, transfersize)); 
			} 
		} 
		
		/* transfer the rest */ 
		if ( alignedX < gs.imageEndX ) 
		{ 
			if (!TransmitHostLocalX_24<u8>(wp, widthlimit, blockheight, alignedX, pbuf)) goto End;
			pbuf -= TransPitch((alignedX-gs.trxpos.dx), transfersize)/TSize; 
		} 
		else 
		{
			pbuf += (blockheight-1)*TransPitch(pitch, transfersize)/TSize;
		}
		tempX = gs.trxpos.dx; 
	} 
	
	if (TransPitch(nSize, transfersize)/4 > 0) 
	{ 
		if (!TransmitHostLocalY_24<u8>(wp, widthlimit, gs.imageEndY, pbuf)) goto End;
		/* sometimes wrong sizes are sent (tekken tag) */ 
		assert( gs.imageTransfer == -1 || TransPitch(nSize, transfersize)/4 <= 2 ); 
	} 
	
	End: 
	if( tempY >= gs.imageEndY ) { 
		assert( gs.imageTransfer == -1 || tempY == gs.imageEndY ); 
		gs.imageTransfer = -1; 
		/*int start, end; 
		ZeroGS::GetRectMemAddress(start, end, gs.dstbuf.psm, gs.trxpos.dx, gs.trxpos.dy, gs.imageWnew, gs.imageHnew, gs.dstbuf.bp, gs.dstbuf.bw); 
		ZeroGS::g_MemTargs.ClearRange(start, end);*/ 
	} 
	else { 
		/* update new params */ 
		gs.imageY = tempY; 
		gs.imageX = tempX; 
	} 
	return (nSize * TransPitch(2, transfersize) + nLeftOver)/2; 
} 

//DEFINE_TRANSFERLOCAL(16, u16, 4, 16, 16, 8, _, SwizzleBlock16);
int TransferHostLocal16(const void* pbyMem, u32 nQWordSize) 
{ 
	const u32 widthlimit = 4;
	const u32 blockbits = 16;
	const u32 blockwidth = 16;
	const u32 blockheight = 8;
	const u32 TSize = sizeof(u16);
	const u32 transfersize = 16;
	_writePixel_0 wp = writePixel16_0;
	
	assert( gs.imageTransfer == 0 ); 
	pstart = g_pbyGSMemory + gs.dstbuf.bp*256; 
	
	/*const u8* pendbuf = (const u8*)pbyMem + nQWordSize*4;*/ 
	tempY = gs.imageY; tempX = gs.imageX; 
	
	const u16* pbuf = (const u16*)pbyMem; 
	int nLeftOver = (nQWordSize*4*2)%(TransPitch(2, transfersize)); 
	nSize = nQWordSize*4*2/ TransPitch(2, transfersize); 
	nSize = min(nSize, gs.imageWnew * gs.imageHnew); 
	
	int endY = ROUND_UPPOW2(tempY, blockheight); 
	int alignedY = ROUND_DOWNPOW2(gs.imageEndY, blockheight); 
	int alignedX = ROUND_DOWNPOW2(gs.imageEndX, blockwidth); 
	bool bAligned, bCanAlign = MOD_POW2(gs.trxpos.dx, blockwidth) == 0 && (tempX == gs.trxpos.dx) && (alignedY > endY) && alignedX > gs.trxpos.dx; 
	
	if ((gs.imageEndX-gs.trxpos.dx)%widthlimit) 
	{ 
		/* hack */ 
		int testwidth = (int)nSize - (gs.imageEndY-tempY)*(gs.imageEndX-gs.trxpos.dx)+(tempX-gs.trxpos.dx); 
		if ((testwidth <= widthlimit) && (testwidth >= -widthlimit)) 
		{ 
			/* don't transfer */ 
			/*DEBUG_LOG("bad texture %s: %d %d %d\n", #psm, gs.trxpos.dx, gs.imageEndX, nQWordSize);*/ 
			gs.imageTransfer = -1; 
		} 
		bCanAlign = false; 
	} 
	
	/* first align on block boundary */ 
	if ( MOD_POW2(tempY, blockheight) || !bCanAlign ) 
	{ 
		
		if ( !bCanAlign ) 
			endY = gs.imageEndY; /* transfer the whole image */ 
		else 
			assert( endY < gs.imageEndY); /* part of alignment condition */ 
		
		if (((gs.imageEndX-gs.trxpos.dx)%widthlimit) || ((gs.imageEndX-tempX)%widthlimit)) 
		{ 
			/* transmit with a width of 1 */ 
			if (!TransmitHostLocalY_<u16>(wp, (1+(DSTPSM == 0x14)), endY, pbuf)) goto End;
		} 
		else 
		{ 
			if (!TransmitHostLocalY_<u16>(wp, widthlimit, endY, pbuf)) goto End;
		} 
		
		if( nSize == 0 || tempY == gs.imageEndY ) goto End; 
	} 
	
	assert( MOD_POW2(tempY, blockheight) == 0 && tempX == gs.trxpos.dx); 
	
	/* can align! */ 
	pitch = gs.imageEndX-gs.trxpos.dx; 
	area = pitch*blockheight; 
	fracX = gs.imageEndX-alignedX; 
	
	/* on top of checking whether pbuf is aligned, make sure that the width is at least aligned to its limits (due to bugs in pcsx2) */ 
	bAligned = !((uptr)pbuf & 0xf) && (TransPitch(pitch, transfersize)&0xf) == 0; 
	
	/* transfer aligning to blocks */ 
	for(; tempY < alignedY && nSize >= area; tempY += blockheight, nSize -= area) 
	{ 
		if( bAligned || ((DSTPSM==PSMCT24) || (DSTPSM==PSMT8H) || (DSTPSM==PSMT4HH) || (DSTPSM==PSMT4HL)) ) 
		{ 
			for(int tempj = gs.trxpos.dx; tempj < alignedX; tempj += blockwidth, pbuf += TransPitch(blockwidth, transfersize)/TSize) 
			{ 
				SwizzleBlock16(pstart + getPixelAddress_0(16,tempj, tempY, gs.dstbuf.bw)*blockbits/8,  (u8*)pbuf, TransPitch(pitch, transfersize)); 
			} 
		} 
		else 
		{ 
			for(int tempj = gs.trxpos.dx; tempj < alignedX; tempj += blockwidth, pbuf += TransPitch(blockwidth, transfersize)/TSize) 
			{ 
				SwizzleBlock16u(pstart + getPixelAddress_0(16,tempj, tempY, gs.dstbuf.bw)*blockbits/8, (u8*)pbuf, TransPitch(pitch, transfersize)); 
			} 
		} 
		
		/* transfer the rest */ 
		if ( alignedX < gs.imageEndX ) 
		{ 
			if (!TransmitHostLocalX_<u16>(wp, widthlimit, blockheight, alignedX, pbuf)) goto End;
			pbuf -= TransPitch((alignedX-gs.trxpos.dx), transfersize)/TSize; 
		} 
		else 
		{
			pbuf += (blockheight-1)* TransPitch(pitch, transfersize)/TSize;
		}
		tempX = gs.trxpos.dx; 
	} 
	
	if (TransPitch(pitch, transfersize)/4 > 0 ) 
	{ 
		if (!TransmitHostLocalY_<u16>(wp, widthlimit, gs.imageEndY, pbuf)) goto End;
		/* sometimes wrong sizes are sent (tekken tag) */ 
		assert( gs.imageTransfer == -1 || TransPitch(nSize, transfersize)/4 <= 2 ); 
	} 
	
	End: 
	if( tempY >= gs.imageEndY ) { 
		assert( gs.imageTransfer == -1 || tempY == gs.imageEndY ); 
		gs.imageTransfer = -1; 
		/*int start, end; 
		ZeroGS::GetRectMemAddress(start, end, gs.dstbuf.psm, gs.trxpos.dx, gs.trxpos.dy, gs.imageWnew, gs.imageHnew, gs.dstbuf.bp, gs.dstbuf.bw); 
		ZeroGS::g_MemTargs.ClearRange(start, end);*/ 
	} 
	else { 
		/* update new params */ 
		gs.imageY = tempY; 
		gs.imageX = tempX; 
	} 
	return (nSize * TransPitch(pitch, transfersize) + nLeftOver)/2; 
} 

//DEFINE_TRANSFERLOCAL(16S, u16, 4, 16, 16, 8, _, SwizzleBlock16);
int TransferHostLocal16S(const void* pbyMem, u32 nQWordSize) 
{ 
	const u32 widthlimit = 4;
	const u32 blockbits = 16;
	const u32 blockwidth = 16;
	const u32 blockheight = 8;
	const u32 TSize = sizeof(u16);
	const u32 transfersize = 16;
	_writePixel_0 wp = writePixel16S_0;
	
	assert( gs.imageTransfer == 0 ); 
	pstart = g_pbyGSMemory + gs.dstbuf.bp*256; 
	
	/*const u8* pendbuf = (const u8*)pbyMem + nQWordSize*4;*/ 
	tempY = gs.imageY; tempX = gs.imageX; 
	
	const u16* pbuf = (const u16*)pbyMem; 
	int nLeftOver = (nQWordSize*4*2)%(TransPitch(2, transfersize)); 
	nSize = nQWordSize*4*2/ TransPitch(2, transfersize); 
	nSize = min(nSize, gs.imageWnew * gs.imageHnew); 
	
	int endY = ROUND_UPPOW2(tempY, blockheight); 
	int alignedY = ROUND_DOWNPOW2(gs.imageEndY, blockheight); 
	int alignedX = ROUND_DOWNPOW2(gs.imageEndX, blockwidth); 
	bool bAligned, bCanAlign = MOD_POW2(gs.trxpos.dx, blockwidth) == 0 && (tempX == gs.trxpos.dx) && (alignedY > endY) && alignedX > gs.trxpos.dx; 
	
	if ((gs.imageEndX-gs.trxpos.dx)%widthlimit) 
	{ 
		/* hack */ 
		int testwidth = (int)nSize - (gs.imageEndY-tempY)*(gs.imageEndX-gs.trxpos.dx)+(tempX-gs.trxpos.dx); 
		if ((testwidth <= widthlimit) && (testwidth >= -widthlimit)) 
		{ 
			/* don't transfer */ 
			/*DEBUG_LOG("bad texture %s: %d %d %d\n", #psm, gs.trxpos.dx, gs.imageEndX, nQWordSize);*/ 
			gs.imageTransfer = -1; 
		} 
		bCanAlign = false; 
	} 
	
	/* first align on block boundary */ 
	if ( MOD_POW2(tempY, blockheight) || !bCanAlign ) 
	{ 
		
		if ( !bCanAlign ) 
			endY = gs.imageEndY; /* transfer the whole image */ 
		else 
			assert( endY < gs.imageEndY); /* part of alignment condition */ 
		
		if (((gs.imageEndX-gs.trxpos.dx)%widthlimit) || ((gs.imageEndX-tempX)%widthlimit)) 
		{ 
			/* transmit with a width of 1 */ 
			if (!TransmitHostLocalY_<u16>(wp, (1+(DSTPSM == 0x14)), endY, pbuf)) goto End;
		} 
		else 
		{ 
			if (!TransmitHostLocalY_<u16>(wp, widthlimit, endY, pbuf)) goto End;
		} 
		
		if( nSize == 0 || tempY == gs.imageEndY ) goto End; 
	} 
	
	assert( MOD_POW2(tempY, blockheight) == 0 && tempX == gs.trxpos.dx); 
	
	/* can align! */ 
	pitch = gs.imageEndX-gs.trxpos.dx; 
	area = pitch*blockheight; 
	fracX = gs.imageEndX-alignedX; 
	
	/* on top of checking whether pbuf is aligned, make sure that the width is at least aligned to its limits (due to bugs in pcsx2) */ 
	bAligned = !((uptr)pbuf & 0xf) && (TransPitch(pitch, transfersize)&0xf) == 0; 
	
	/* transfer aligning to blocks */ 
	for(; tempY < alignedY && nSize >= area; tempY += blockheight, nSize -= area) 
	{ 
		if( bAligned || ((DSTPSM==PSMCT24) || (DSTPSM==PSMT8H) || (DSTPSM==PSMT4HH) || (DSTPSM==PSMT4HL)) ) 
		{ 
			for(int tempj = gs.trxpos.dx; tempj < alignedX; tempj += blockwidth, pbuf += TransPitch(blockwidth, transfersize)/TSize) 
			{ 
				SwizzleBlock16(pstart + getPixelAddress_0(16S,tempj, tempY, gs.dstbuf.bw)*blockbits/8,  (u8*)pbuf, TransPitch(pitch, transfersize)); 
			} 
		} 
		else 
		{ 
			for(int tempj = gs.trxpos.dx; tempj < alignedX; tempj += blockwidth, pbuf += TransPitch(blockwidth, transfersize)/TSize) 
			{ 
				SwizzleBlock16u(pstart + getPixelAddress_0(16S,tempj, tempY, gs.dstbuf.bw)*blockbits/8, (u8*)pbuf, TransPitch(pitch, transfersize)); 
			} 
		} 
		
		/* transfer the rest */ 
		if ( alignedX < gs.imageEndX ) 
		{ 
			if (!TransmitHostLocalX_<u16>(wp, widthlimit, blockheight, alignedX, pbuf)) goto End;
			pbuf -= TransPitch((alignedX-gs.trxpos.dx), transfersize)/TSize; 
		} 
		else 
		{
			pbuf += (blockheight-1) * TransPitch(pitch, transfersize)/TSize;
		}
		tempX = gs.trxpos.dx; 
	} 
	
	if (TransPitch(nSize, transfersize)/4 > 0 ) 
	{ 
		if (!TransmitHostLocalY_<u16>(wp, widthlimit, gs.imageEndY, pbuf)) goto End;
		/* sometimes wrong sizes are sent (tekken tag) */ 
		assert( gs.imageTransfer == -1 || TransPitch(nSize, transfersize)/4 <= 2 ); 
	} 
	
	End: 
	if( tempY >= gs.imageEndY ) { 
		assert( gs.imageTransfer == -1 || tempY == gs.imageEndY ); 
		gs.imageTransfer = -1; 
		/*int start, end; 
		ZeroGS::GetRectMemAddress(start, end, gs.dstbuf.psm, gs.trxpos.dx, gs.trxpos.dy, gs.imageWnew, gs.imageHnew, gs.dstbuf.bp, gs.dstbuf.bw); 
		ZeroGS::g_MemTargs.ClearRange(start, end);*/ 
	} 
	else { 
		/* update new params */ 
		gs.imageY = tempY; 
		gs.imageX = tempX; 
	} 
	return (nSize * TransPitch(2, transfersize) + nLeftOver)/2; 
} 

//DEFINE_TRANSFERLOCAL(16Z, u16, 4, 16, 16, 8, _, SwizzleBlock16);
int TransferHostLocal16Z(const void* pbyMem, u32 nQWordSize) 
{ 
	const u32 widthlimit = 4;
	const u32 blockbits = 16;
	const u32 blockwidth = 16;
	const u32 blockheight = 8;
	const u32 TSize = sizeof(u16);
	const u32 transfersize = 16;
	_writePixel_0 wp = writePixel16Z_0;
	
	assert( gs.imageTransfer == 0 ); 
	pstart = g_pbyGSMemory + gs.dstbuf.bp*256; 
	
	/*const u8* pendbuf = (const u8*)pbyMem + nQWordSize*4;*/ 
	tempY = gs.imageY; tempX = gs.imageX; 
	
	const u16* pbuf = (const u16*)pbyMem; 
	int nLeftOver = (nQWordSize*4*2)%(TransPitch(2, transfersize)); 
	nSize = nQWordSize*4*2/ TransPitch(2, transfersize); 
	nSize = min(nSize, gs.imageWnew * gs.imageHnew); 
	
	int endY = ROUND_UPPOW2(tempY, blockheight); 
	int alignedY = ROUND_DOWNPOW2(gs.imageEndY, blockheight); 
	int alignedX = ROUND_DOWNPOW2(gs.imageEndX, blockwidth); 
	bool bAligned, bCanAlign = MOD_POW2(gs.trxpos.dx, blockwidth) == 0 && (tempX == gs.trxpos.dx) && (alignedY > endY) && alignedX > gs.trxpos.dx; 
	
	if ((gs.imageEndX-gs.trxpos.dx)%widthlimit) 
	{ 
		/* hack */ 
		int testwidth = (int)nSize - (gs.imageEndY-tempY)*(gs.imageEndX-gs.trxpos.dx)+(tempX-gs.trxpos.dx); 
		if ((testwidth <= widthlimit) && (testwidth >= -widthlimit)) 
		{ 
			/* don't transfer */ 
			/*DEBUG_LOG("bad texture %s: %d %d %d\n", #psm, gs.trxpos.dx, gs.imageEndX, nQWordSize);*/ 
			gs.imageTransfer = -1; 
		} 
		bCanAlign = false; 
	} 
	
	/* first align on block boundary */ 
	if ( MOD_POW2(tempY, blockheight) || !bCanAlign ) 
	{ 
		
		if ( !bCanAlign ) 
			endY = gs.imageEndY; /* transfer the whole image */ 
		else 
			assert( endY < gs.imageEndY); /* part of alignment condition */ 
		
		if (((gs.imageEndX-gs.trxpos.dx)%widthlimit) || ((gs.imageEndX-tempX)%widthlimit)) 
		{ 
			/* transmit with a width of 1 */ 
			if (!TransmitHostLocalY_<u16>(wp, (1+(DSTPSM == 0x14)), endY, pbuf)) goto End;
		} 
		else 
		{ 
			if (!TransmitHostLocalY_<u16>(wp, widthlimit, endY, pbuf)) goto End;
		} 
		
		if( nSize == 0 || tempY == gs.imageEndY ) goto End; 
	} 
	
	assert( MOD_POW2(tempY, blockheight) == 0 && tempX == gs.trxpos.dx); 
	
	/* can align! */ 
	pitch = gs.imageEndX-gs.trxpos.dx; 
	area = pitch*blockheight; 
	fracX = gs.imageEndX-alignedX; 
	
	/* on top of checking whether pbuf is aligned, make sure that the width is at least aligned to its limits (due to bugs in pcsx2) */ 
	bAligned = !((uptr)pbuf & 0xf) && (TransPitch(pitch, transfersize)&0xf) == 0; 
	
	/* transfer aligning to blocks */ 
	for(; tempY < alignedY && nSize >= area; tempY += blockheight, nSize -= area) 
	{ 
		if( bAligned || ((DSTPSM==PSMCT24) || (DSTPSM==PSMT8H) || (DSTPSM==PSMT4HH) || (DSTPSM==PSMT4HL)) ) 
		{ 
			for(int tempj = gs.trxpos.dx; tempj < alignedX; tempj += blockwidth, pbuf += TransPitch(blockwidth, transfersize)/TSize) 
			{ 
				SwizzleBlock16(pstart + getPixelAddress_0(16Z,tempj, tempY, gs.dstbuf.bw)*blockbits/8,  (u8*)pbuf, TransPitch(pitch, transfersize)); 
			} 
		} 
		else 
		{ 
			for(int tempj = gs.trxpos.dx; tempj < alignedX; tempj += blockwidth, pbuf += TransPitch(blockwidth, transfersize)/TSize) 
			{ 
				SwizzleBlock16u(pstart + getPixelAddress_0(16Z,tempj, tempY, gs.dstbuf.bw)*blockbits/8, (u8*)pbuf, TransPitch(pitch, transfersize)); 
			} 
		} 
		
		/* transfer the rest */ 
		if ( alignedX < gs.imageEndX ) 
		{ 
			if (!TransmitHostLocalX_<u16>(wp, widthlimit, blockheight, alignedX, pbuf)) goto End;
			pbuf -= TransPitch((alignedX-gs.trxpos.dx), transfersize)/TSize; 
		} 
		else 
		{
			pbuf += (blockheight-1)*TransPitch(pitch, transfersize)/TSize;
		}
		tempX = gs.trxpos.dx; 
	} 
	
	if (TransPitch(nSize, transfersize)/4 > 0 ) 
	{ 
		if (!TransmitHostLocalY_<u16>(wp, widthlimit, gs.imageEndY, pbuf)) goto End;
		/* sometimes wrong sizes are sent (tekken tag) */ 
		assert( gs.imageTransfer == -1 || TransPitch(nSize, transfersize)/4 <= 2 ); 
	} 
	
	End: 
	if( tempY >= gs.imageEndY ) { 
		assert( gs.imageTransfer == -1 || tempY == gs.imageEndY ); 
		gs.imageTransfer = -1; 
		/*int start, end; 
		ZeroGS::GetRectMemAddress(start, end, gs.dstbuf.psm, gs.trxpos.dx, gs.trxpos.dy, gs.imageWnew, gs.imageHnew, gs.dstbuf.bp, gs.dstbuf.bw); 
		ZeroGS::g_MemTargs.ClearRange(start, end);*/ 
	} 
	else { 
		/* update new params */ 
		gs.imageY = tempY; 
		gs.imageX = tempX; 
	} 
	return (nSize * TransPitch(2, transfersize) + nLeftOver)/2; 
} 

//DEFINE_TRANSFERLOCAL(16SZ, u16, 4, 16, 16, 8, _, SwizzleBlock16);
int TransferHostLocal16SZ(const void* pbyMem, u32 nQWordSize) 
{ 
	const u32 widthlimit = 4;
	const u32 blockbits = 16;
	const u32 blockwidth = 16;
	const u32 blockheight = 8;
	const u32 TSize = sizeof(u16);
	const u32 transfersize = 16;
	_writePixel_0 wp = writePixel16SZ_0;
	
	assert( gs.imageTransfer == 0 ); 
	pstart = g_pbyGSMemory + gs.dstbuf.bp*256; 
	
	/*const u8* pendbuf = (const u8*)pbyMem + nQWordSize*4;*/ 
	tempY = gs.imageY; tempX = gs.imageX; 
	
	const u16* pbuf = (const u16*)pbyMem; 
	int nLeftOver = (nQWordSize*4*2)%(TransPitch(2, transfersize)); 
	nSize = nQWordSize*4*2/TransPitch(2, transfersize); 
	nSize = min(nSize, gs.imageWnew * gs.imageHnew); 
	
	int endY = ROUND_UPPOW2(tempY, blockheight); 
	int alignedY = ROUND_DOWNPOW2(gs.imageEndY, blockheight); 
	int alignedX = ROUND_DOWNPOW2(gs.imageEndX, blockwidth); 
	bool bAligned, bCanAlign = MOD_POW2(gs.trxpos.dx, blockwidth) == 0 && (tempX == gs.trxpos.dx) && (alignedY > endY) && alignedX > gs.trxpos.dx; 
	
	if ((gs.imageEndX-gs.trxpos.dx)%widthlimit) 
	{ 
		/* hack */ 
		int testwidth = (int)nSize - (gs.imageEndY-tempY)*(gs.imageEndX-gs.trxpos.dx)+(tempX-gs.trxpos.dx); 
		if ((testwidth <= widthlimit) && (testwidth >= -widthlimit)) 
		{ 
			/* don't transfer */ 
			/*DEBUG_LOG("bad texture %s: %d %d %d\n", #psm, gs.trxpos.dx, gs.imageEndX, nQWordSize);*/ 
			gs.imageTransfer = -1; 
		} 
		bCanAlign = false; 
	} 
	
	/* first align on block boundary */ 
	if ( MOD_POW2(tempY, blockheight) || !bCanAlign ) 
	{ 
		
		if ( !bCanAlign ) 
			endY = gs.imageEndY; /* transfer the whole image */ 
		else 
			assert( endY < gs.imageEndY); /* part of alignment condition */ 
		
		if (((gs.imageEndX-gs.trxpos.dx)%widthlimit) || ((gs.imageEndX-tempX)%widthlimit)) 
		{ 
			/* transmit with a width of 1 */ 
			if (!TransmitHostLocalY_<u16>(wp, (1+(DSTPSM == 0x14)), endY, pbuf)) goto End;
		} 
		else 
		{ 
			if (!TransmitHostLocalY_<u16>(wp, widthlimit, endY, pbuf)) goto End;
		} 
		
		if( nSize == 0 || tempY == gs.imageEndY ) goto End; 
	} 
	
	assert( MOD_POW2(tempY, blockheight) == 0 && tempX == gs.trxpos.dx); 
	
	/* can align! */ 
	pitch = gs.imageEndX-gs.trxpos.dx; 
	area = pitch*blockheight; 
	fracX = gs.imageEndX-alignedX; 
	
	/* on top of checking whether pbuf is aligned, make sure that the width is at least aligned to its limits (due to bugs in pcsx2) */ 
	bAligned = !((uptr)pbuf & 0xf) && (TransPitch(pitch, transfersize)&0xf) == 0; 
	
	/* transfer aligning to blocks */ 
	for(; tempY < alignedY && nSize >= area; tempY += blockheight, nSize -= area) 
	{ 
		if( bAligned || ((DSTPSM==PSMCT24) || (DSTPSM==PSMT8H) || (DSTPSM==PSMT4HH) || (DSTPSM==PSMT4HL)) ) 
		{ 
			for(int tempj = gs.trxpos.dx; tempj < alignedX; tempj += blockwidth, pbuf += TransPitch(blockwidth, transfersize)/TSize) 
			{ 
				SwizzleBlock16(pstart + getPixelAddress_0(16SZ,tempj, tempY, gs.dstbuf.bw)*blockbits/8,  (u8*)pbuf, TransPitch(pitch, transfersize)); 
			} 
		} 
		else 
		{ 
			for(int tempj = gs.trxpos.dx; tempj < alignedX; tempj += blockwidth, pbuf += TransPitch(blockwidth, transfersize)/TSize) 
			{ 
				SwizzleBlock16u(pstart + getPixelAddress_0(16SZ,tempj, tempY, gs.dstbuf.bw)*blockbits/8, (u8*)pbuf, TransPitch(pitch, transfersize)); 
			} 
		} 
		
		/* transfer the rest */ 
		if ( alignedX < gs.imageEndX ) 
		{ 
			if (!TransmitHostLocalX_<u16>(wp, widthlimit, blockheight, alignedX, pbuf)) goto End;
			pbuf -= TransPitch((alignedX-gs.trxpos.dx), transfersize)/TSize; 
		} 
		else 
		{
			pbuf += (blockheight-1)*TransPitch(pitch, transfersize)/TSize;
		}
		tempX = gs.trxpos.dx; 
	} 
	
	if (TransPitch(nSize, transfersize)/4 > 0 ) 
	{ 
		if (!TransmitHostLocalY_<u16>(wp, widthlimit, gs.imageEndY, pbuf)) goto End;
		/* sometimes wrong sizes are sent (tekken tag) */ 
		assert( gs.imageTransfer == -1 || TransPitch(nSize, transfersize)/4 <= 2 ); 
	} 
	
	End: 
	if( tempY >= gs.imageEndY ) { 
		assert( gs.imageTransfer == -1 || tempY == gs.imageEndY ); 
		gs.imageTransfer = -1; 
		/*int start, end; 
		ZeroGS::GetRectMemAddress(start, end, gs.dstbuf.psm, gs.trxpos.dx, gs.trxpos.dy, gs.imageWnew, gs.imageHnew, gs.dstbuf.bp, gs.dstbuf.bw); 
		ZeroGS::g_MemTargs.ClearRange(start, end);*/ 
	} 
	else { 
		/* update new params */ 
		gs.imageY = tempY; 
		gs.imageX = tempX; 
	} 
	return (nSize * TransPitch(2, transfersize) + nLeftOver)/2; 
} 

//DEFINE_TRANSFERLOCAL(8, u8, 4, 8, 16, 16, _, SwizzleBlock8);
int TransferHostLocal8(const void* pbyMem, u32 nQWordSize) 
{ 
	const u32 widthlimit = 4;
	const u32 blockbits = 8;
	const u32 blockwidth = 16;
	const u32 blockheight = 16;
	const u32 TSize = sizeof(u8);
	const u32 transfersize = 8;
	_writePixel_0 wp = writePixel8_0;
	
	assert( gs.imageTransfer == 0 ); 
	pstart = g_pbyGSMemory + gs.dstbuf.bp*256; 
	
	/*const u8* pendbuf = (const u8*)pbyMem + nQWordSize*4;*/ 
	tempY = gs.imageY; tempX = gs.imageX; 
	
	const u8* pbuf = (const u8*)pbyMem; 
	int nLeftOver = (nQWordSize*4*2)%(TransPitch(2, transfersize)); 
	nSize = nQWordSize*4*2/TransPitch(2, transfersize); 
	nSize = min(nSize, gs.imageWnew * gs.imageHnew); 
	
	int endY = ROUND_UPPOW2(tempY, blockheight); 
	int alignedY = ROUND_DOWNPOW2(gs.imageEndY, blockheight); 
	int alignedX = ROUND_DOWNPOW2(gs.imageEndX, blockwidth); 
	bool bAligned, bCanAlign = MOD_POW2(gs.trxpos.dx, blockwidth) == 0 && (tempX == gs.trxpos.dx) && (alignedY > endY) && alignedX > gs.trxpos.dx; 
	
	if ((gs.imageEndX-gs.trxpos.dx)%widthlimit) 
	{ 
		/* hack */ 
		int testwidth = (int)nSize - (gs.imageEndY-tempY)*(gs.imageEndX-gs.trxpos.dx)+(tempX-gs.trxpos.dx); 
		if ((testwidth <= widthlimit) && (testwidth >= -widthlimit)) 
		{ 
			/* don't transfer */ 
			/*DEBUG_LOG("bad texture %s: %d %d %d\n", #psm, gs.trxpos.dx, gs.imageEndX, nQWordSize);*/ 
			gs.imageTransfer = -1; 
		} 
		bCanAlign = false; 
	} 
	
	/* first align on block boundary */ 
	if ( MOD_POW2(tempY, blockheight) || !bCanAlign ) 
	{ 
		
		if ( !bCanAlign ) 
			endY = gs.imageEndY; /* transfer the whole image */ 
		else 
			assert( endY < gs.imageEndY); /* part of alignment condition */ 
		
		if (((gs.imageEndX-gs.trxpos.dx)%widthlimit) || ((gs.imageEndX-tempX)%widthlimit)) 
		{ 
			/* transmit with a width of 1 */ 
			if (!TransmitHostLocalY_<u8>(wp, (1+(DSTPSM == 0x14)), endY, pbuf)) goto End;
		} 
		else 
		{ 
			if (!TransmitHostLocalY_<u8>(wp, widthlimit, endY, pbuf)) goto End;
		} 
		
		if( nSize == 0 || tempY == gs.imageEndY ) goto End; 
	} 
	
	assert( MOD_POW2(tempY, blockheight) == 0 && tempX == gs.trxpos.dx); 
	
	/* can align! */ 
	pitch = gs.imageEndX-gs.trxpos.dx; 
	area = pitch*blockheight; 
	fracX = gs.imageEndX-alignedX; 
	
	/* on top of checking whether pbuf is aligned, make sure that the width is at least aligned to its limits (due to bugs in pcsx2) */ 
	bAligned = !((uptr)pbuf & 0xf) && (TransPitch(pitch, transfersize)&0xf) == 0; 
	
	/* transfer aligning to blocks */ 
	for(; tempY < alignedY && nSize >= area; tempY += blockheight, nSize -= area) 
	{ 
		if( bAligned || ((DSTPSM==PSMCT24) || (DSTPSM==PSMT8H) || (DSTPSM==PSMT4HH) || (DSTPSM==PSMT4HL)) ) 
		{ 
			for(int tempj = gs.trxpos.dx; tempj < alignedX; tempj += blockwidth, pbuf +=TransPitch(blockwidth, transfersize)/TSize) 
			{ 
				SwizzleBlock8(pstart + getPixelAddress_0(8,tempj, tempY, gs.dstbuf.bw)*blockbits/8,  (u8*)pbuf, TransPitch(pitch, transfersize)); 
			} 
		} 
		else 
		{ 
			for(int tempj = gs.trxpos.dx; tempj < alignedX; tempj += blockwidth, pbuf += TransPitch(blockwidth, transfersize)/TSize) 
			{ 
				SwizzleBlock8u(pstart + getPixelAddress_0(8,tempj, tempY, gs.dstbuf.bw)*blockbits/8, (u8*)pbuf, TransPitch(pitch, transfersize)); 
			} 
		} 
		
		/* transfer the rest */ 
		if ( alignedX < gs.imageEndX ) 
		{ 
			if (!TransmitHostLocalX_<u8>(wp, widthlimit, blockheight, alignedX, pbuf)) goto End;
			pbuf -= TransPitch(alignedX-gs.trxpos.dx, transfersize)/TSize; 
		} 
		else 
		{
			pbuf += (blockheight-1)*TransPitch(pitch, transfersize)/TSize;
		}
		tempX = gs.trxpos.dx; 
	} 
	
	if (TransPitch(nSize, transfersize)/4 > 0 ) 
	{ 
		if (!TransmitHostLocalY_<u8>(wp, widthlimit, gs.imageEndY, pbuf)) goto End;
		/* sometimes wrong sizes are sent (tekken tag) */ 
		assert( gs.imageTransfer == -1 || TransPitch(nSize, transfersize)/4 <= 2 ); 
	} 
	
	End: 
	if( tempY >= gs.imageEndY ) { 
		assert( gs.imageTransfer == -1 || tempY == gs.imageEndY ); 
		gs.imageTransfer = -1; 
		/*int start, end; 
		ZeroGS::GetRectMemAddress(start, end, gs.dstbuf.psm, gs.trxpos.dx, gs.trxpos.dy, gs.imageWnew, gs.imageHnew, gs.dstbuf.bp, gs.dstbuf.bw); 
		ZeroGS::g_MemTargs.ClearRange(start, end);*/ 
	} 
	else { 
		/* update new params */ 
		gs.imageY = tempY; 
		gs.imageX = tempX; 
	} 
	return (nSize * TransPitch(2, transfersize) + nLeftOver)/2; 
} 

//DEFINE_TRANSFERLOCAL(4, u8, 8, 4, 32, 16, _4, SwizzleBlock4);
int TransferHostLocal4(const void* pbyMem, u32 nQWordSize) 
{ 
	const u32 widthlimit = 8;
	const u32 blockbits = 4;
	const u32 blockwidth = 32;
	const u32 blockheight = 16;
	const u32 TSize = sizeof(u8);
	const u32 transfersize = 4;
	_writePixel_0 wp = writePixel4_0;
	
	assert( gs.imageTransfer == 0 ); 
	pstart = g_pbyGSMemory + gs.dstbuf.bp*256; 
	
	/*const u8* pendbuf = (const u8*)pbyMem + nQWordSize*4;*/ 
	tempY = gs.imageY; tempX = gs.imageX; 
	
	const u8* pbuf = (const u8*)pbyMem; 
	int nLeftOver = (nQWordSize*4*2)%(TransPitch(2, transfersize)); 
	nSize = nQWordSize*4*2/TransPitch(2, transfersize); 
	nSize = min(nSize, gs.imageWnew * gs.imageHnew); 
	
	int endY = ROUND_UPPOW2(tempY, blockheight); 
	int alignedY = ROUND_DOWNPOW2(gs.imageEndY, blockheight); 
	int alignedX = ROUND_DOWNPOW2(gs.imageEndX, blockwidth); 
	bool bAligned, bCanAlign = MOD_POW2(gs.trxpos.dx, blockwidth) == 0 && (tempX == gs.trxpos.dx) && (alignedY > endY) && alignedX > gs.trxpos.dx; 
	
	if ((gs.imageEndX-gs.trxpos.dx)%widthlimit) 
	{ 
		/* hack */ 
		int testwidth = (int)nSize - (gs.imageEndY-tempY)*(gs.imageEndX-gs.trxpos.dx)+(tempX-gs.trxpos.dx); 
		if ((testwidth <= widthlimit) && (testwidth >= -widthlimit)) 
		{ 
			/* don't transfer */ 
			/*DEBUG_LOG("bad texture %s: %d %d %d\n", #psm, gs.trxpos.dx, gs.imageEndX, nQWordSize);*/ 
			gs.imageTransfer = -1; 
		} 
		bCanAlign = false; 
	} 
	
	/* first align on block boundary */ 
	if ( MOD_POW2(tempY, blockheight) || !bCanAlign ) 
	{ 
		
		if ( !bCanAlign ) 
			endY = gs.imageEndY; /* transfer the whole image */ 
		else 
			assert( endY < gs.imageEndY); /* part of alignment condition */ 
		
		if (((gs.imageEndX-gs.trxpos.dx)%widthlimit) || ((gs.imageEndX-tempX)%widthlimit)) 
		{ 
			/* transmit with a width of 1 */ 
			if (!TransmitHostLocalY_4<u8>(wp, (1+(DSTPSM == 0x14)), endY, pbuf)) goto End;
		} 
		else 
		{ 
			if (!TransmitHostLocalY_4<u8>(wp, widthlimit, endY, pbuf)) goto End;
		} 
		
		if( nSize == 0 || tempY == gs.imageEndY ) goto End; 
	} 
	
	assert( MOD_POW2(tempY, blockheight) == 0 && tempX == gs.trxpos.dx); 
	
	/* can align! */ 
	pitch = gs.imageEndX-gs.trxpos.dx; 
	area = pitch*blockheight; 
	fracX = gs.imageEndX-alignedX; 
	
	/* on top of checking whether pbuf is aligned, make sure that the width is at least aligned to its limits (due to bugs in pcsx2) */ 
	bAligned = !((uptr)pbuf & 0xf) && (TransPitch(pitch, transfersize)&0xf) == 0; 
	
	/* transfer aligning to blocks */ 
	for(; tempY < alignedY && nSize >= area; tempY += blockheight, nSize -= area) 
	{ 
		if( bAligned || ((DSTPSM==PSMCT24) || (DSTPSM==PSMT8H) || (DSTPSM==PSMT4HH) || (DSTPSM==PSMT4HL)) ) 
		{ 
			for(int tempj = gs.trxpos.dx; tempj < alignedX; tempj += blockwidth, pbuf += TransPitch(blockwidth, transfersize)/TSize) 
			{ 
				SwizzleBlock4(pstart + getPixelAddress_0(4,tempj, tempY, gs.dstbuf.bw)*blockbits/8,  (u8*)pbuf, TransPitch(pitch, transfersize)); 
			} 
		} 
		else 
		{ 
			for(int tempj = gs.trxpos.dx; tempj < alignedX; tempj += blockwidth, pbuf += TransPitch(blockwidth, transfersize)/TSize) 
			{ 
				SwizzleBlock4u(pstart + getPixelAddress_0(4,tempj, tempY, gs.dstbuf.bw)*blockbits/8, (u8*)pbuf, TransPitch(pitch, transfersize)); 
			} 
		} 
		
		/* transfer the rest */ 
		if ( alignedX < gs.imageEndX ) 
		{ 
			if (!TransmitHostLocalX_4<u8>(wp, widthlimit, blockheight, alignedX, pbuf)) goto End;
			pbuf -= TransPitch((alignedX-gs.trxpos.dx), transfersize)/TSize; 
		} 
		else 
		{
			pbuf += (blockheight-1)*TransPitch(pitch, transfersize)/TSize;
		}
		tempX = gs.trxpos.dx; 
	} 
	
	if (TransPitch(nSize, transfersize)/4 > 0 ) 
	{ 
		if (!TransmitHostLocalY_4<u8>(wp, widthlimit, gs.imageEndY, pbuf)) goto End;
		/* sometimes wrong sizes are sent (tekken tag) */ 
		assert( gs.imageTransfer == -1 || TransPitch(nSize, transfersize)/4 <= 2 ); 
	} 
	
	End: 
	if( tempY >= gs.imageEndY ) { 
		assert( gs.imageTransfer == -1 || tempY == gs.imageEndY ); 
		gs.imageTransfer = -1; 
		/*int start, end; 
		ZeroGS::GetRectMemAddress(start, end, gs.dstbuf.psm, gs.trxpos.dx, gs.trxpos.dy, gs.imageWnew, gs.imageHnew, gs.dstbuf.bp, gs.dstbuf.bw); 
		ZeroGS::g_MemTargs.ClearRange(start, end);*/ 
	} 
	else { 
		/* update new params */ 
		gs.imageY = tempY; 
		gs.imageX = tempX; 
	} 
	return (nSize * TransPitch(2, transfersize) + nLeftOver)/2; 
} 

//DEFINE_TRANSFERLOCAL(8H, u8, 4, 32, 8, 8, _, SwizzleBlock8H);
int TransferHostLocal8H(const void* pbyMem, u32 nQWordSize) 
{ 
	const u32 widthlimit = 4;
	const u32 blockbits = 32;
	const u32 blockwidth = 8;
	const u32 blockheight = 8;
	const u32 TSize = sizeof(u8);
	const u32 transfersize = 8;
	_writePixel_0 wp = writePixel8H_0;
	
	assert( gs.imageTransfer == 0 ); 
	pstart = g_pbyGSMemory + gs.dstbuf.bp*256; 
	
	/*const u8* pendbuf = (const u8*)pbyMem + nQWordSize*4;*/ 
	tempY = gs.imageY; tempX = gs.imageX; 
	
	const u8* pbuf = (const u8*)pbyMem; 
	int nLeftOver = (nQWordSize*4*2)%(TransPitch(2, transfersize)); 
	nSize = nQWordSize*4*2/TransPitch(2, transfersize); 
	nSize = min(nSize, gs.imageWnew * gs.imageHnew); 
	
	int endY = ROUND_UPPOW2(tempY, blockheight); 
	int alignedY = ROUND_DOWNPOW2(gs.imageEndY, blockheight); 
	int alignedX = ROUND_DOWNPOW2(gs.imageEndX, blockwidth); 
	bool bAligned, bCanAlign = MOD_POW2(gs.trxpos.dx, blockwidth) == 0 && (tempX == gs.trxpos.dx) && (alignedY > endY) && alignedX > gs.trxpos.dx; 
	
	if ((gs.imageEndX-gs.trxpos.dx)%widthlimit) 
	{ 
		/* hack */ 
		int testwidth = (int)nSize - (gs.imageEndY-tempY)*(gs.imageEndX-gs.trxpos.dx)+(tempX-gs.trxpos.dx); 
		if ((testwidth <= widthlimit) && (testwidth >= -widthlimit)) 
		{ 
			/* don't transfer */ 
			/*DEBUG_LOG("bad texture %s: %d %d %d\n", #psm, gs.trxpos.dx, gs.imageEndX, nQWordSize);*/ 
			gs.imageTransfer = -1; 
		} 
		bCanAlign = false; 
	} 
	
	/* first align on block boundary */ 
	if ( MOD_POW2(tempY, blockheight) || !bCanAlign ) 
	{ 
		
		if ( !bCanAlign ) 
			endY = gs.imageEndY; /* transfer the whole image */ 
		else 
			assert( endY < gs.imageEndY); /* part of alignment condition */ 
		
		if (((gs.imageEndX-gs.trxpos.dx)%widthlimit) || ((gs.imageEndX-tempX)%widthlimit)) 
		{ 
			/* transmit with a width of 1 */ 
			if (!TransmitHostLocalY_<u8>(wp, (1+(DSTPSM == 0x14)), endY, pbuf)) goto End;
		} 
		else 
		{ 
			if (!TransmitHostLocalY_<u8>(wp, widthlimit, endY, pbuf)) goto End;
		} 
		
		if( nSize == 0 || tempY == gs.imageEndY )  goto End; 
	} 
	
	assert( MOD_POW2(tempY, blockheight) == 0 && tempX == gs.trxpos.dx); 
	
	/* can align! */ 
	pitch = gs.imageEndX-gs.trxpos.dx; 
	area = pitch*blockheight; 
	fracX = gs.imageEndX-alignedX; 
	
	/* on top of checking whether pbuf is aligned, make sure that the width is at least aligned to its limits (due to bugs in pcsx2) */ 
	bAligned = !((uptr)pbuf & 0xf) && (TransPitch(pitch, transfersize)&0xf) == 0; 
	
	/* transfer aligning to blocks */ 
	for(; tempY < alignedY && nSize >= area; tempY += blockheight, nSize -= area) 
	{ 
		if( bAligned || ((DSTPSM==PSMCT24) || (DSTPSM==PSMT8H) || (DSTPSM==PSMT4HH) || (DSTPSM==PSMT4HL)) ) 
		{ 
			for(int tempj = gs.trxpos.dx; tempj < alignedX; tempj += blockwidth, pbuf += TransPitch(blockwidth, transfersize)/TSize) 
			{ 
				SwizzleBlock8H(pstart + getPixelAddress_0(8H,tempj, tempY, gs.dstbuf.bw)*blockbits/8,  (u8*)pbuf, TransPitch(pitch, transfersize)); 
			} 
		} 
		else 
		{ 
			for(int tempj = gs.trxpos.dx; tempj < alignedX; tempj += blockwidth, pbuf += TransPitch(blockwidth, transfersize)/TSize) 
			{ 
				SwizzleBlock8Hu(pstart + getPixelAddress_0(8H,tempj, tempY, gs.dstbuf.bw)*blockbits/8, (u8*)pbuf, TransPitch(pitch, transfersize)); 
			} 
		} 
		
		/* transfer the rest */ 
		if ( alignedX < gs.imageEndX ) 
		{ 
			if (!TransmitHostLocalX_<u8>(wp, widthlimit, blockheight, alignedX, pbuf)) goto End;
			pbuf -= TransPitch((alignedX-gs.trxpos.dx), transfersize)/TSize; 
		} 
		else 
		{
			pbuf += (blockheight-1)*TransPitch(pitch, transfersize)/TSize;
		}
		tempX = gs.trxpos.dx; 
	} 
	
	if (TransPitch(nSize, transfersize)/4 > 0 ) 
	{ 
		if (!TransmitHostLocalY_<u8>(wp, widthlimit, gs.imageEndY, pbuf)) goto End;
		/* sometimes wrong sizes are sent (tekken tag) */ 
		assert( gs.imageTransfer == -1 || TransPitch(nSize, transfersize)/4 <= 2 ); 
	} 
	
	End: 
	if( tempY >= gs.imageEndY ) { 
		assert( gs.imageTransfer == -1 || tempY == gs.imageEndY ); 
		gs.imageTransfer = -1; 
		/*int start, end; 
		ZeroGS::GetRectMemAddress(start, end, gs.dstbuf.psm, gs.trxpos.dx, gs.trxpos.dy, gs.imageWnew, gs.imageHnew, gs.dstbuf.bp, gs.dstbuf.bw); 
		ZeroGS::g_MemTargs.ClearRange(start, end);*/ 
	} 
	else { 
		/* update new params */ 
		gs.imageY = tempY; 
		gs.imageX = tempX; 
	} 
	return (nSize * TransPitch(2, transfersize) + nLeftOver)/2; 
} 

//DEFINE_TRANSFERLOCAL(4HL, u8, 8, 32, 8, 8, _4, SwizzleBlock4HL);
int TransferHostLocal4HL(const void* pbyMem, u32 nQWordSize) 
{ 
	const u32 widthlimit = 8;
	const u32 blockbits = 32;
	const u32 blockwidth = 8;
	const u32 blockheight = 8;
	const u32 TSize = sizeof(u8);
	const u32 transfersize = 4;
	_writePixel_0 wp = writePixel4HL_0;
	
	assert( gs.imageTransfer == 0 ); 
	pstart = g_pbyGSMemory + gs.dstbuf.bp*256; 
	
	/*const u8* pendbuf = (const u8*)pbyMem + nQWordSize*4;*/ 
	tempY = gs.imageY; tempX = gs.imageX; 
	
	const u8* pbuf = (const u8*)pbyMem; 
	int nLeftOver = (nQWordSize*4*2)%(TransPitch(2, transfersize)); 
	nSize = nQWordSize*4*2/ TransPitch(2, transfersize); 
	nSize = min(nSize, gs.imageWnew * gs.imageHnew); 
	
	int endY = ROUND_UPPOW2(tempY, blockheight); 
	int alignedY = ROUND_DOWNPOW2(gs.imageEndY, blockheight); 
	int alignedX = ROUND_DOWNPOW2(gs.imageEndX, blockwidth); 
	bool bAligned, bCanAlign = MOD_POW2(gs.trxpos.dx, blockwidth) == 0 && (tempX == gs.trxpos.dx) && (alignedY > endY) && alignedX > gs.trxpos.dx; 
	
	if ((gs.imageEndX-gs.trxpos.dx)%widthlimit) 
	{ 
		/* hack */ 
		int testwidth = (int)nSize - (gs.imageEndY-tempY)*(gs.imageEndX-gs.trxpos.dx)+(tempX-gs.trxpos.dx); 
		if ((testwidth <= widthlimit) && (testwidth >= -widthlimit)) 
		{ 
			/* don't transfer */ 
			/*DEBUG_LOG("bad texture %s: %d %d %d\n", #psm, gs.trxpos.dx, gs.imageEndX, nQWordSize);*/ 
			gs.imageTransfer = -1; 
		} 
		bCanAlign = false; 
	} 
	
	/* first align on block boundary */ 
	if ( MOD_POW2(tempY, blockheight) || !bCanAlign ) 
	{ 
		
		if ( !bCanAlign ) 
			endY = gs.imageEndY; /* transfer the whole image */ 
		else 
			assert( endY < gs.imageEndY); /* part of alignment condition */ 
		
		if (((gs.imageEndX-gs.trxpos.dx)%widthlimit) || ((gs.imageEndX-tempX)%widthlimit)) 
		{ 
			/* transmit with a width of 1 */ 
			if (!TransmitHostLocalY_4<u8>(wp, (1+(DSTPSM == 0x14)), endY, pbuf)) goto End;
		} 
		else 
		{ 
			if (!TransmitHostLocalY_4<u8>(wp, widthlimit, endY, pbuf)) goto End; 
		} 
		
		if( nSize == 0 || tempY == gs.imageEndY ) goto End; 
	} 
	
	assert( MOD_POW2(tempY, blockheight) == 0 && tempX == gs.trxpos.dx); 
	
	/* can align! */ 
	pitch = gs.imageEndX-gs.trxpos.dx; 
	area = pitch*blockheight; 
	fracX = gs.imageEndX-alignedX; 
	
	/* on top of checking whether pbuf is aligned, make sure that the width is at least aligned to its limits (due to bugs in pcsx2) */ 
	bAligned = !((uptr)pbuf & 0xf) && (TransPitch(pitch, transfersize)&0xf) == 0; 
	
	/* transfer aligning to blocks */ 
	for(; tempY < alignedY && nSize >= area; tempY += blockheight, nSize -= area) 
	{ 
		if( bAligned || ((DSTPSM==PSMCT24) || (DSTPSM==PSMT8H) || (DSTPSM==PSMT4HH) || (DSTPSM==PSMT4HL)) ) 
		{ 
			for(int tempj = gs.trxpos.dx; tempj < alignedX; tempj += blockwidth, pbuf += TransPitch(blockwidth, transfersize)/TSize) 
			{ 
				SwizzleBlock4HL(pstart + getPixelAddress_0(4HL,tempj, tempY, gs.dstbuf.bw)*blockbits/8,  (u8*)pbuf, TransPitch(pitch, transfersize)); 
			} 
		} 
		else 
		{ 
			for(int tempj = gs.trxpos.dx; tempj < alignedX; tempj += blockwidth, pbuf += TransPitch(blockwidth, transfersize)/TSize) 
			{ 
				SwizzleBlock4HLu(pstart + getPixelAddress_0(4HL,tempj, tempY, gs.dstbuf.bw)*blockbits/8, (u8*)pbuf, TransPitch(pitch, transfersize)); 
			} 
		} 
		
		/* transfer the rest */ 
		if ( alignedX < gs.imageEndX ) 
		{ 
			if (!TransmitHostLocalX_4<u8>(wp, widthlimit, blockheight, alignedX, pbuf)) goto End;
			pbuf -= TransPitch((alignedX-gs.trxpos.dx), transfersize)/TSize; 
		} 
		else 
		{
			pbuf += (blockheight-1)*TransPitch(pitch, transfersize)/TSize;
		}
		tempX = gs.trxpos.dx; 
	} 
	
	if (TransPitch(nSize, transfersize)/4 > 0 ) 
	{ 
		if (!TransmitHostLocalY_4<u8>(wp, widthlimit, gs.imageEndY, pbuf)) goto End; 
		/* sometimes wrong sizes are sent (tekken tag) */ 
		assert( gs.imageTransfer == -1 || TransPitch(nSize, transfersize)/4 <= 2 ); 
	} 
	
	End: 
	if( tempY >= gs.imageEndY ) { 
		assert( gs.imageTransfer == -1 || tempY == gs.imageEndY ); 
		gs.imageTransfer = -1; 
		/*int start, end; 
		ZeroGS::GetRectMemAddress(start, end, gs.dstbuf.psm, gs.trxpos.dx, gs.trxpos.dy, gs.imageWnew, gs.imageHnew, gs.dstbuf.bp, gs.dstbuf.bw); 
		ZeroGS::g_MemTargs.ClearRange(start, end);*/ 
	} 
	else { 
		/* update new params */ 
		gs.imageY = tempY; 
		gs.imageX = tempX; 
	} 
	return (nSize * TransPitch(2, transfersize) + nLeftOver)/2; 
} 

//DEFINE_TRANSFERLOCAL(4HH, u8, 8, 32, 8, 8, _4, SwizzleBlock4HH);
int TransferHostLocal4HH(const void* pbyMem, u32 nQWordSize) 
{ 
	const u32 widthlimit = 8;
	const u32 blockbits = 32;
	const u32 blockwidth = 8;
	const u32 blockheight = 8;
	const u32 TSize = sizeof(u8);
	const u32 transfersize = 4;
	_writePixel_0 wp = writePixel4HH_0;
	
	assert( gs.imageTransfer == 0 ); 
	pstart = g_pbyGSMemory + gs.dstbuf.bp*256; 
	
	/*const u8* pendbuf = (const u8*)pbyMem + nQWordSize*4;*/ 
	tempY = gs.imageY; tempX = gs.imageX; 
	
	const u8* pbuf = (const u8*)pbyMem; 
	int nLeftOver = (nQWordSize*4*2)%(TransPitch(2, transfersize)); 
	nSize = nQWordSize*4*2/TransPitch(2, transfersize); 
	nSize = min(nSize, gs.imageWnew * gs.imageHnew); 
	
	int endY = ROUND_UPPOW2(tempY, blockheight); 
	int alignedY = ROUND_DOWNPOW2(gs.imageEndY, blockheight); 
	int alignedX = ROUND_DOWNPOW2(gs.imageEndX, blockwidth); 
	bool bAligned, bCanAlign = MOD_POW2(gs.trxpos.dx, blockwidth) == 0 && (tempX == gs.trxpos.dx) && (alignedY > endY) && alignedX > gs.trxpos.dx; 
	
	if ((gs.imageEndX-gs.trxpos.dx)%widthlimit) 
	{ 
		/* hack */ 
		int testwidth = (int)nSize - (gs.imageEndY-tempY)*(gs.imageEndX-gs.trxpos.dx)+(tempX-gs.trxpos.dx); 
		if ((testwidth <= widthlimit) && (testwidth >= -widthlimit)) 
		{ 
			/* don't transfer */ 
			/*DEBUG_LOG("bad texture %s: %d %d %d\n", #psm, gs.trxpos.dx, gs.imageEndX, nQWordSize);*/ 
			gs.imageTransfer = -1; 
		} 
		bCanAlign = false; 
	} 
	
	/* first align on block boundary */ 
	if ( MOD_POW2(tempY, blockheight) || !bCanAlign ) 
	{ 
		
		if ( !bCanAlign ) 
			endY = gs.imageEndY; /* transfer the whole image */ 
		else 
			assert( endY < gs.imageEndY); /* part of alignment condition */ 
		
		if (((gs.imageEndX-gs.trxpos.dx)%widthlimit) || ((gs.imageEndX-tempX)%widthlimit)) 
		{ 
			/* transmit with a width of 1 */ 
			if (!TransmitHostLocalY_4<u8>(wp, (1+(DSTPSM == 0x14)), endY, pbuf)) goto End; 
		} 
		else 
		{ 
			if (!TransmitHostLocalY_4<u8>(wp, widthlimit, endY, pbuf)) goto End; 
		} 
		
		if( nSize == 0 || tempY == gs.imageEndY )  goto End; 
	} 
	
	assert( MOD_POW2(tempY, blockheight) == 0 && tempX == gs.trxpos.dx); 
	
	/* can align! */ 
	pitch = gs.imageEndX-gs.trxpos.dx; 
	area = pitch*blockheight; 
	fracX = gs.imageEndX-alignedX; 
	
	/* on top of checking whether pbuf is aligned, make sure that the width is at least aligned to its limits (due to bugs in pcsx2) */ 
	bAligned = !((uptr)pbuf & 0xf) && (TransPitch(pitch, transfersize)&0xf) == 0; 
	
	/* transfer aligning to blocks */ 
	for(; tempY < alignedY && nSize >= area; tempY += blockheight, nSize -= area) 
	{ 
		if( bAligned || ((DSTPSM==PSMCT24) || (DSTPSM==PSMT8H) || (DSTPSM==PSMT4HH) || (DSTPSM==PSMT4HL)) ) 
		{ 
			for(int tempj = gs.trxpos.dx; tempj < alignedX; tempj += blockwidth, pbuf += TransPitch(blockwidth, transfersize)/TSize) 
			{ 
				SwizzleBlock4HH(pstart + getPixelAddress_0(4HH,tempj, tempY, gs.dstbuf.bw)*blockbits/8,  (u8*)pbuf, TransPitch(pitch, transfersize)); 
			} 
		} 
		else 
		{ 
			for(int tempj = gs.trxpos.dx; tempj < alignedX; tempj += blockwidth, pbuf += TransPitch(blockwidth, transfersize)/TSize) 
			{ 
				SwizzleBlock4HHu(pstart + getPixelAddress_0(4HH,tempj, tempY, gs.dstbuf.bw)*blockbits/8, (u8*)pbuf, TransPitch(pitch, transfersize)); 
			} 
		} 
		
		/* transfer the rest */ 
		if ( alignedX < gs.imageEndX ) 
		{ 
			if (!TransmitHostLocalX_4<u8>(wp, widthlimit, blockheight, alignedX, pbuf)) goto End;
			pbuf -= TransPitch((alignedX-gs.trxpos.dx), transfersize)/TSize; 
		} 
		else 
		{
			pbuf += (blockheight-1)*TransPitch(pitch, transfersize)/TSize;
		}
		tempX = gs.trxpos.dx; 
	} 
	
	if (TransPitch(nSize, transfersize)/4 > 0 ) 
	{ 
		if (!TransmitHostLocalY_4<u8>(wp, widthlimit, gs.imageEndY, pbuf)) goto End; 
		/* sometimes wrong sizes are sent (tekken tag) */ 
		assert( gs.imageTransfer == -1 || TransPitch(nSize, transfersize)/4 <= 2 ); 
	} 
	
	End: 
	if( tempY >= gs.imageEndY ) { 
		assert( gs.imageTransfer == -1 || tempY == gs.imageEndY ); 
		gs.imageTransfer = -1; 
		/*int start, end; 
		ZeroGS::GetRectMemAddress(start, end, gs.dstbuf.psm, gs.trxpos.dx, gs.trxpos.dy, gs.imageWnew, gs.imageHnew, gs.dstbuf.bp, gs.dstbuf.bw); 
		ZeroGS::g_MemTargs.ClearRange(start, end);*/ 
	} 
	else { 
		/* update new params */ 
		gs.imageY = tempY; 
		gs.imageX = tempX; 
	} 
	return (nSize * TransPitch(2, transfersize) + nLeftOver)/2; 
} 
#else

DEFINE_TRANSFERLOCAL(32,	32,	u32,	2,	32,	8,	8,	_,		SwizzleBlock32);	// 32/8/4 = 1
DEFINE_TRANSFERLOCAL(32Z,	32,	u32,	2,	32,	8,	8,	_,		SwizzleBlock32);	// 32/8/4 = 1
DEFINE_TRANSFERLOCAL(24,	24,	u8,		8,	32,	8,	8,	_24,	SwizzleBlock24);	// 24/8/1 = 3
DEFINE_TRANSFERLOCAL(24Z,	24,	u8,		8,	32,	8,	8,	_24,	SwizzleBlock24);	// 24/8/1 = 3
DEFINE_TRANSFERLOCAL(16,	16,	u16,	4,	16,	16,	8,	_,		SwizzleBlock16);	// 16/8/2 = 1
DEFINE_TRANSFERLOCAL(16S,	16,	u16,	4,	16,	16,	8,	_,		SwizzleBlock16);	// 16/8/2 = 1
DEFINE_TRANSFERLOCAL(16Z,	16,	u16,	4,	16,	16,	8,	_,		SwizzleBlock16);	// 16/8/2 = 1
DEFINE_TRANSFERLOCAL(16SZ,	16,	u16,	4,	16,	16,	8,	_,		SwizzleBlock16);	// 16/8/2 = 1
DEFINE_TRANSFERLOCAL(8,		8,	u8,		4,	8,	16,	16,	_,		SwizzleBlock8);		// 8/8/1  = 1
DEFINE_TRANSFERLOCAL(4,		4,	u8,		8,	4,	32,	16,	_4,		SwizzleBlock4);		// 4/8/1  = 1/2
DEFINE_TRANSFERLOCAL(8H,	8,	u8,		4,	32,	8,	8,	_,		SwizzleBlock8H);	// 8/8/1  = 1
DEFINE_TRANSFERLOCAL(4HL,	4,	u8,		8,	32,	8,	8,	_4,		SwizzleBlock4HL);	// 4/8/1  = 1/2
DEFINE_TRANSFERLOCAL(4HH,	4,	u8,		8,	32,	8,	8,	_4,		SwizzleBlock4HH);	// 4/8/1  = 1/2

#endif

void TransferLocalHost32(void* pbyMem, u32 nQWordSize)
{FUNCLOG
}

void TransferLocalHost24(void* pbyMem, u32 nQWordSize)
{FUNCLOG
}

void TransferLocalHost16(void* pbyMem, u32 nQWordSize)
{FUNCLOG
}

void TransferLocalHost16S(void* pbyMem, u32 nQWordSize)
{FUNCLOG
}

void TransferLocalHost8(void* pbyMem, u32 nQWordSize)
{
}

void TransferLocalHost4(void* pbyMem, u32 nQWordSize)
{FUNCLOG
}

void TransferLocalHost8H(void* pbyMem, u32 nQWordSize)
{FUNCLOG
}

void TransferLocalHost4HL(void* pbyMem, u32 nQWordSize)
{FUNCLOG
}

void TransferLocalHost4HH(void* pbyMem, u32 nQWordSize)
{
}

void TransferLocalHost32Z(void* pbyMem, u32 nQWordSize)
{FUNCLOG
}

void TransferLocalHost24Z(void* pbyMem, u32 nQWordSize)
{FUNCLOG
}

void TransferLocalHost16Z(void* pbyMem, u32 nQWordSize)
{FUNCLOG
}

void TransferLocalHost16SZ(void* pbyMem, u32 nQWordSize)
{FUNCLOG
}

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
	if( floatfmt )
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
