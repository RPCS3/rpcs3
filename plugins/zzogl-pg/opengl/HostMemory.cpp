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
#include <Cg/cg.h>
#include <Cg/cgGL.h>

#include <stdlib.h>
#include "Mem.h"
#include "x86.h"
#include "zerogs.h"
#include "targets.h"


 namespace ZeroGS
 {
	extern CRangeManager s_RangeMngr; // manages overwritten memory
	extern void ResolveInRange(int start, int end);
	
	static vector<u8> s_vTempBuffer, s_vTransferCache;
	static int gs_imageEnd = 0;
	
	void GetRectMemAddress(int& start, int& end, int psm, int x, int y, int w, int h, int bp, int bw)
	{
		FUNCLOG

		if (m_Blocks[psm].bpp == 0)
		{
			ZZLog::Error_Log("ZeroGS: Bad psm 0x%x.", psm);
			start = 0;
			end = 0x00400000;
			return;
		}

		if (PSMT_ISZTEX(psm) || psm == PSMCT16S)
		{

			const BLOCK& b = m_Blocks[psm];

			bw = (bw + b.width - 1) / b.width;
			start = bp * 256 + ((y / b.height) * bw + (x / b.width)) * 0x2000;
			end = bp * 256  + (((y + h - 1) / b.height) * bw + (x + w + b.width - 1) / b.width) * 0x2000;
		}
		else
		{
			// just take the addresses
			switch (psm)
			{
				case PSMCT32:
				case PSMCT24:
				case PSMT8H:
				case PSMT4HL:
				case PSMT4HH:
					start = 4 * getPixelAddress32(x, y, bp, bw);
					end = 4 * getPixelAddress32(x + w - 1, y + h - 1, bp, bw) + 4;
					break;

				case PSMCT16:
					start = 2 * getPixelAddress16(x, y, bp, bw);
					end = 2 * getPixelAddress16(x + w - 1, y + h - 1, bp, bw) + 2;
					break;

				case PSMT8:
					start = getPixelAddress8(x, y, bp, bw);
					end = getPixelAddress8(x + w - 1, y + h - 1, bp, bw) + 1;
					break;

				case PSMT4:
				{
					start = getPixelAddress4(x, y, bp, bw) / 2;
					int newx = ((x + w - 1 + 31) & ~31) - 1;
					int newy = ((y + h - 1 + 15) & ~15) - 1;
					end = (getPixelAddress4(max(newx, x), max(newy, y), bp, bw) + 2) / 2;
					break;
				}
			}
		}
	}

	void InitTransferHostLocal()
	{
		FUNCLOG

		//if (g_bIsLost) return;

	#if defined(ZEROGS_DEVBUILD)
		if (gs.trxpos.dx + gs.imageWnew > gs.dstbuf.bw)
			ZZLog::Warn_Log("Transfer error, width exceeded.");

	#endif

		//bool bHasFlushed = false;

		gs.imageX = gs.trxpos.dx;
		gs.imageY = gs.trxpos.dy;

		gs.imageEndX = gs.imageX + gs.imageWnew;
		gs.imageEndY = gs.imageY + gs.imageHnew;

		assert(gs.imageEndX < 2048 && gs.imageEndY < 2048);

		// hack! viewful joe
		if (gs.dstbuf.psm == 63) gs.dstbuf.psm = 0;

		int start, end;

		GetRectMemAddress(start, end, gs.dstbuf.psm, gs.trxpos.dx, gs.trxpos.dy, gs.imageWnew, gs.imageHnew, gs.dstbuf.bp, gs.dstbuf.bw);

		if (end > 0x00400000)
		{
			ZZLog::Warn_Log("Host local out of bounds!");
			//gs.imageTransfer = -1;
			end = 0x00400000;
		}

		gs_imageEnd = end;

		if (vb[0].nCount > 0) Flush(0);
		if (vb[1].nCount > 0) Flush(1);

		//ZZLog::Prim_Log("trans: bp:%x x:%x y:%x w:%x h:%x\n", gs.dstbuf.bp, gs.trxpos.dx, gs.trxpos.dy, gs.imageWnew, gs.imageHnew);
	}

	void TransferHostLocal(const void* pbyMem, u32 nQWordSize)
	{
		FUNCLOG

	//	if (g_bIsLost) return;

		int start, end;

		GetRectMemAddress(start, end, gs.dstbuf.psm, gs.imageX, gs.imageY, gs.imageWnew, gs.imageHnew, gs.dstbuf.bp, gs.dstbuf.bw);

		assert(start < gs_imageEnd);

		end = gs_imageEnd;

		// sometimes games can decompress to alpha channel of render target only, in this case
		// do a resolve right away. wolverine x2
		if (((gs.dstbuf.psm == PSMT8H) || (gs.dstbuf.psm == PSMT4HL) || (gs.dstbuf.psm == PSMT4HH)) && !(conf.settings().gust))
		{
			list<CRenderTarget*> listTransmissionUpdateTargs;
			s_RTs.GetTargs(start, end, listTransmissionUpdateTargs);

			for (list<CRenderTarget*>::iterator it = listTransmissionUpdateTargs.begin(); it != listTransmissionUpdateTargs.end(); ++it)
			{
				CRenderTarget* ptarg = *it;

				if ((ptarg->status & CRenderTarget::TS_Virtual)) continue;

				//ZZLog::Error_Log("Resolving to alpha channel.");
				ptarg->Resolve();
			}
		}

		s_RangeMngr.Insert(start, min(end, start + (int)nQWordSize*16));

		const u8* porgend = (const u8*)pbyMem + 4 * nQWordSize;

		if (s_vTransferCache.size() > 0)
		{

			int imagecache = s_vTransferCache.size();
			s_vTempBuffer.resize(imagecache + nQWordSize*4);
			memcpy(&s_vTempBuffer[0], &s_vTransferCache[0], imagecache);
			memcpy(&s_vTempBuffer[imagecache], pbyMem, nQWordSize*4);

			pbyMem = (const void*) & s_vTempBuffer[0];
			porgend = &s_vTempBuffer[0] + s_vTempBuffer.size();

			int wordinc = imagecache / 4;

			if ((nQWordSize * 4 + imagecache) / 3 == ((nQWordSize + wordinc) * 4) / 3)
			{
				// can use the data
				nQWordSize += wordinc;
			}
		}

		int leftover = m_Blocks[gs.dstbuf.psm].TransferHostLocal(pbyMem, nQWordSize);

		if (leftover > 0)
		{
			// copy the last gs.image24bitOffset to the cache
			s_vTransferCache.resize(leftover);
			memcpy(&s_vTransferCache[0], porgend - leftover, leftover);
		}
		else 
		{
			s_vTransferCache.resize(0);
		}

	#if defined(_DEBUG)
		if (g_bSaveTrans)
		{
			tex0Info t;
			t.tbp0 = gs.dstbuf.bp;
			t.tw = gs.imageWnew;
			t.th = gs.imageHnew;
			t.tbw = gs.dstbuf.bw;
			t.psm = gs.dstbuf.psm;
			SaveTex(&t, 0);
		}

	#endif
	}

	void InitTransferLocalHost()
	{
		FUNCLOG
		assert(gs.trxpos.sx + gs.imageWnew <= 2048 && gs.trxpos.sy + gs.imageHnew <= 2048);

	#if defined(ZEROGS_DEVBUILD)

		if (gs.trxpos.sx + gs.imageWnew > gs.srcbuf.bw)
			ZZLog::Warn_Log("Transfer error, width exceeded.");

	#endif

		gs.imageX = gs.trxpos.sx;
		gs.imageY = gs.trxpos.sy;

		gs.imageEndX = gs.imageX + gs.imageWnew;
		gs.imageEndY = gs.imageY + gs.imageHnew;

		s_vTransferCache.resize(0);

		int start, end;

		GetRectMemAddress(start, end, gs.srcbuf.psm, gs.trxpos.sx, gs.trxpos.sy, gs.imageWnew, gs.imageHnew, gs.srcbuf.bp, gs.srcbuf.bw);

		ResolveInRange(start, end);
	}

	template <class T>
	void TransferLocalHost(void* pbyMem, u32 nQWordSize, int& x, int& y, u8 *pstart, _readPixel_0 rp)
	{
		int i = x, j = y;
		T* pbuf = (T*)pbyMem;
		u32 nSize = nQWordSize * 16 / sizeof(T);

		for (; i < gs.imageEndY; ++i)
		{
			for (; j < gs.imageEndX && nSize > 0; ++j, --nSize)
			{
				*pbuf++ = rp(pstart, j % 2048, i % 2048, gs.srcbuf.bw);
			}

			if (j >= gs.imageEndX)
			{
				assert(j == gs.imageEndX);
				j = gs.trxpos.sx;
			}
			else
			{
				assert(nSize == 0);
				break;
			}
		}
	}

	void TransferLocalHost_24(void* pbyMem, u32 nQWordSize, int& x, int& y, u8 *pstart, _readPixel_0 rp)
	{
		int i = x, j = y;
		u8* pbuf = (u8*)pbyMem;
		u32 nSize = nQWordSize * 16 / 3;

		for (; i < gs.imageEndY; ++i)
		{
			for (; j < gs.imageEndX && nSize > 0; ++j, --nSize)
			{
				u32 p = rp(pstart, j % 2048, i % 2048, gs.srcbuf.bw);
				pbuf[0] = (u8)p;
				pbuf[1] = (u8)(p >> 8);
				pbuf[2] = (u8)(p >> 16);
				pbuf += 3;
			}

			if (j >= gs.imageEndX)
			{
				assert(j == gs.imageEndX);
				j = gs.trxpos.sx;
			}
			else
			{
				assert(nSize == 0);
				break;
			}
		}
	}

	// left/right, top/down
	void TransferLocalHost(void* pbyMem, u32 nQWordSize)
	{
		FUNCLOG
		assert(gs.imageTransfer == 1);

		u8* pstart = g_pbyGSMemory + 256 * gs.srcbuf.bp;
		int i = gs.imageY, j = gs.imageX;

		switch (gs.srcbuf.psm)
		{

			case PSMCT32:
				TransferLocalHost<u32>(pbyMem, nQWordSize, i, j, pstart, readPixel32_0);
				break;

			case PSMCT24:
				TransferLocalHost_24(pbyMem, nQWordSize, i, j, pstart, readPixel24_0);
				break;

			case PSMCT16:
				TransferLocalHost<u16>(pbyMem, nQWordSize, i, j, pstart, readPixel16_0);
				break;

			case PSMCT16S:
				TransferLocalHost<u16>(pbyMem, nQWordSize, i, j, pstart, readPixel16S_0);
				break;

			case PSMT8:
				TransferLocalHost<u8>(pbyMem, nQWordSize, i, j, pstart, readPixel8_0);
				break;

			case PSMT8H:
				TransferLocalHost<u8>(pbyMem, nQWordSize, i, j, pstart, readPixel8H_0);
				break;

			case PSMT32Z:
				TransferLocalHost<u32>(pbyMem, nQWordSize, i, j, pstart, readPixel32Z_0);
				break;

			case PSMT24Z:
				TransferLocalHost_24(pbyMem, nQWordSize, i, j, pstart, readPixel24Z_0);
				break;

			case PSMT16Z:
				TransferLocalHost<u16>(pbyMem, nQWordSize, i, j, pstart, readPixel16Z_0);
				break;

			case PSMT16SZ:
				TransferLocalHost<u16>(pbyMem, nQWordSize, i, j, pstart, readPixel16SZ_0);
				break;

			default:
				assert(0);
		}

		gs.imageY = i;
		gs.imageX = j;

		if (gs.imageY >= gs.imageEndY)
		{
			assert(gs.imageY == gs.imageEndY);
			gs.imageTransfer = -1;
		}
	}

	// dir depends on trxpos.dirx & trxpos.diry
	void TransferLocalLocal()
	{
		FUNCLOG
		
		//ZZLog::Error_Log("I'z in your code, transferring your memory...");
		assert(gs.imageTransfer == 2);
		assert(gs.trxpos.sx + gs.imageWnew < 2048 && gs.trxpos.sy + gs.imageHnew < 2048);
		assert(gs.trxpos.dx + gs.imageWnew < 2048 && gs.trxpos.dy + gs.imageHnew < 2048);
		assert((gs.srcbuf.psm&0x7) == (gs.dstbuf.psm&0x7));

		if (gs.trxpos.sx + gs.imageWnew > gs.srcbuf.bw)
			ZZLog::Warn_Log("Transfer error, src width exceeded.");

		if (gs.trxpos.dx + gs.imageWnew > gs.dstbuf.bw)
			ZZLog::Warn_Log("Transfer error, dst width exceeded.");

		int srcstart, srcend, dststart, dstend;

		GetRectMemAddress(srcstart, srcend, gs.srcbuf.psm, gs.trxpos.sx, gs.trxpos.sy, gs.imageWnew, gs.imageHnew, gs.srcbuf.bp, gs.srcbuf.bw);
		GetRectMemAddress(dststart, dstend, gs.dstbuf.psm, gs.trxpos.dx, gs.trxpos.dy, gs.imageWnew, gs.imageHnew, gs.dstbuf.bp, gs.dstbuf.bw);

		// resolve the targs
		ResolveInRange(srcstart, srcend);

		list<CRenderTarget*> listTargs;

		s_RTs.GetTargs(dststart, dstend, listTargs);

		for (list<CRenderTarget*>::iterator it = listTargs.begin(); it != listTargs.end(); ++it)
		{
			if (!((*it)->status & CRenderTarget::TS_Virtual))
			{
				(*it)->Resolve();
				//(*it)->status |= CRenderTarget::TS_NeedUpdate;
			}
		}

		u8* pSrcBuf = g_pbyGSMemory + gs.srcbuf.bp * 256;
		u8* pDstBuf = g_pbyGSMemory + gs.dstbuf.bp * 256;

	#define TRANSFERLOCALLOCAL(srcpsm, dstpsm, widthlimit) { \
		if( (gs.imageWnew&widthlimit)!=0 ) break; \
		assert( (gs.imageWnew&widthlimit)==0 && widthlimit <= 4); \
		for(int i = gs.trxpos.sy, i2 = gs.trxpos.dy; i < gs.trxpos.sy+gs.imageHnew; i++, i2++) { \
			for(int j = gs.trxpos.sx, j2 = gs.trxpos.dx; j < gs.trxpos.sx+gs.imageWnew; j+=widthlimit, j2+=widthlimit) { \
				\
				writePixel##dstpsm##_0(pDstBuf, j2%2048, i2%2048, \
					readPixel##srcpsm##_0(pSrcBuf, j%2048, i%2048, gs.srcbuf.bw), gs.dstbuf.bw); \
				\
				if( widthlimit > 1 ) { \
					writePixel##dstpsm##_0(pDstBuf, (j2+1)%2048, i2%2048, \
						readPixel##srcpsm##_0(pSrcBuf, (j+1)%2048, i%2048, gs.srcbuf.bw), gs.dstbuf.bw); \
					\
					if( widthlimit > 2 ) { \
						writePixel##dstpsm##_0(pDstBuf, (j2+2)%2048, i2%2048, \
							readPixel##srcpsm##_0(pSrcBuf, (j+2)%2048, i%2048, gs.srcbuf.bw), gs.dstbuf.bw); \
						\
						if( widthlimit > 3 ) { \
							writePixel##dstpsm##_0(pDstBuf, (j2+3)%2048, i2%2048, \
								readPixel##srcpsm##_0(pSrcBuf, (j+3)%2048, i%2048, gs.srcbuf.bw), gs.dstbuf.bw); \
						} \
					} \
				} \
			} \
		} \
	} \
	 
	#define TRANSFERLOCALLOCAL_4(srcpsm, dstpsm) { \
		assert( (gs.imageWnew%8) == 0 ); \
		for(int i = gs.trxpos.sy, i2 = gs.trxpos.dy; i < gs.trxpos.sy+gs.imageHnew; ++i, ++i2) { \
			for(int j = gs.trxpos.sx, j2 = gs.trxpos.dx; j < gs.trxpos.sx+gs.imageWnew; j+=8, j2+=8) { \
				/* NOTE: the 2 conseq 4bit values are in NOT in the same byte */ \
				u32 read = getPixelAddress##srcpsm##_0(j%2048, i%2048, gs.srcbuf.bw); \
				u32 write = getPixelAddress##dstpsm##_0(j2%2048, i2%2048, gs.dstbuf.bw); \
				pDstBuf[write] = (pDstBuf[write]&0xf0)|(pSrcBuf[read]&0x0f); \
	 \
				read = getPixelAddress##srcpsm##_0((j+1)%2048, i%2048, gs.srcbuf.bw); \
				write = getPixelAddress##dstpsm##_0((j2+1)%2048, i2%2048, gs.dstbuf.bw); \
				pDstBuf[write] = (pDstBuf[write]&0x0f)|(pSrcBuf[read]&0xf0); \
	 \
				read = getPixelAddress##srcpsm##_0((j+2)%2048, i%2048, gs.srcbuf.bw); \
				write = getPixelAddress##dstpsm##_0((j2+2)%2048, i2%2048, gs.dstbuf.bw); \
				pDstBuf[write] = (pDstBuf[write]&0xf0)|(pSrcBuf[read]&0x0f); \
	 \
				read = getPixelAddress##srcpsm##_0((j+3)%2048, i%2048, gs.srcbuf.bw); \
				write = getPixelAddress##dstpsm##_0((j2+3)%2048, i2%2048, gs.dstbuf.bw); \
				pDstBuf[write] = (pDstBuf[write]&0x0f)|(pSrcBuf[read]&0xf0); \
	 \
				read = getPixelAddress##srcpsm##_0((j+2)%2048, i%2048, gs.srcbuf.bw); \
				write = getPixelAddress##dstpsm##_0((j2+2)%2048, i2%2048, gs.dstbuf.bw); \
				pDstBuf[write] = (pDstBuf[write]&0xf0)|(pSrcBuf[read]&0x0f); \
	 \
				read = getPixelAddress##srcpsm##_0((j+3)%2048, i%2048, gs.srcbuf.bw); \
				write = getPixelAddress##dstpsm##_0((j2+3)%2048, i2%2048, gs.dstbuf.bw); \
				pDstBuf[write] = (pDstBuf[write]&0x0f)|(pSrcBuf[read]&0xf0); \
	 \
				read = getPixelAddress##srcpsm##_0((j+2)%2048, i%2048, gs.srcbuf.bw); \
				write = getPixelAddress##dstpsm##_0((j2+2)%2048, i2%2048, gs.dstbuf.bw); \
				pDstBuf[write] = (pDstBuf[write]&0xf0)|(pSrcBuf[read]&0x0f); \
	 \
				read = getPixelAddress##srcpsm##_0((j+3)%2048, i%2048, gs.srcbuf.bw); \
				write = getPixelAddress##dstpsm##_0((j2+3)%2048, i2%2048, gs.dstbuf.bw); \
				pDstBuf[write] = (pDstBuf[write]&0x0f)|(pSrcBuf[read]&0xf0); \
			} \
		} \
	} \
	 
		switch (gs.srcbuf.psm) 
		{
			case PSMCT32:
				if (gs.dstbuf.psm == PSMCT32)
				{
					TRANSFERLOCALLOCAL(32, 32, 2);
				}
				else
				{
					TRANSFERLOCALLOCAL(32, 32Z, 2);
				}
				break;

			case PSMCT24:
				if (gs.dstbuf.psm == PSMCT24)
				{
					TRANSFERLOCALLOCAL(24, 24, 4);
				}
				else
				{
					TRANSFERLOCALLOCAL(24, 24Z, 4);
				}
				break;

			case PSMCT16:
				switch (gs.dstbuf.psm)
				{
					case PSMCT16:
						TRANSFERLOCALLOCAL(16, 16, 4);
						break;

					case PSMCT16S:
						TRANSFERLOCALLOCAL(16, 16S, 4);
						break;

					case PSMT16Z:
						TRANSFERLOCALLOCAL(16, 16Z, 4);
						break;

					case PSMT16SZ:
						TRANSFERLOCALLOCAL(16, 16SZ, 4);
						break;
				}
				break;

			case PSMCT16S:
				switch (gs.dstbuf.psm)
				{
					case PSMCT16:
						TRANSFERLOCALLOCAL(16S, 16, 4);
						break;

					case PSMCT16S:
						TRANSFERLOCALLOCAL(16S, 16S, 4);
						break;

					case PSMT16Z:
						TRANSFERLOCALLOCAL(16S, 16Z, 4);
						break;

					case PSMT16SZ:
						TRANSFERLOCALLOCAL(16S, 16SZ, 4);
						break;
				}
				break;

			case PSMT8:
				if (gs.dstbuf.psm == PSMT8)
				{
					TRANSFERLOCALLOCAL(8, 8, 4);
				}
				else
				{
					TRANSFERLOCALLOCAL(8, 8H, 4);
				}
				break;

			case PSMT4:
				switch (gs.dstbuf.psm)
				{

					case PSMT4:
						TRANSFERLOCALLOCAL_4(4, 4);
						break;

					case PSMT4HL:
						TRANSFERLOCALLOCAL_4(4, 4HL);
						break;

					case PSMT4HH:
						TRANSFERLOCALLOCAL_4(4, 4HH);
						break;
				}
				break;

			case PSMT8H:
				if (gs.dstbuf.psm == PSMT8)
				{
					TRANSFERLOCALLOCAL(8H, 8, 4);
				}
				else
				{
					TRANSFERLOCALLOCAL(8H, 8H, 4);
				}
				break;

			case PSMT4HL:
				switch (gs.dstbuf.psm)
				{
					case PSMT4:
						TRANSFERLOCALLOCAL_4(4HL, 4);
						break;

					case PSMT4HL:
						TRANSFERLOCALLOCAL_4(4HL, 4HL);
						break;

					case PSMT4HH:
						TRANSFERLOCALLOCAL_4(4HL, 4HH);
						break;
				}
				break;

			case PSMT4HH:
				switch (gs.dstbuf.psm)
				{
					case PSMT4:
						TRANSFERLOCALLOCAL_4(4HH, 4);
						break;

					case PSMT4HL:
						TRANSFERLOCALLOCAL_4(4HH, 4HL);
						break;

					case PSMT4HH:
						TRANSFERLOCALLOCAL_4(4HH, 4HH);
						break;
				}
				break;

			case PSMT32Z:
				if (gs.dstbuf.psm == PSMCT32)
				{
					TRANSFERLOCALLOCAL(32Z, 32, 2);
				}
				else
				{
					TRANSFERLOCALLOCAL(32Z, 32Z, 2);
				}
				break;

			case PSMT24Z:
				if (gs.dstbuf.psm == PSMCT24)
				{
					TRANSFERLOCALLOCAL(24Z, 24, 4);
				}
				else
				{
					TRANSFERLOCALLOCAL(24Z, 24Z, 4);
				}
				break;

			case PSMT16Z:
				switch (gs.dstbuf.psm)
				{
					case PSMCT16:
						TRANSFERLOCALLOCAL(16Z, 16, 4);
						break;

					case PSMCT16S:
						TRANSFERLOCALLOCAL(16Z, 16S, 4);
						break;

					case PSMT16Z:
						TRANSFERLOCALLOCAL(16Z, 16Z, 4);
						break;

					case PSMT16SZ:
						TRANSFERLOCALLOCAL(16Z, 16SZ, 4);
						break;
				}
				break;

			case PSMT16SZ:
				switch (gs.dstbuf.psm)
				{
					case PSMCT16:
						TRANSFERLOCALLOCAL(16SZ, 16, 4);
						break;

					case PSMCT16S:
						TRANSFERLOCALLOCAL(16SZ, 16S, 4);
						break;

					case PSMT16Z:
						TRANSFERLOCALLOCAL(16SZ, 16Z, 4);
						break;

					case PSMT16SZ:
						TRANSFERLOCALLOCAL(16SZ, 16SZ, 4);
						break;
				}
				break;
		}

		g_MemTargs.ClearRange(dststart, dstend);

	#ifdef DEVBUILD

		if (g_bSaveTrans)
		{
			tex0Info t;
			t.tbp0 = gs.dstbuf.bp;
			t.tw = gs.imageWnew;
			t.th = gs.imageHnew;
			t.tbw = gs.dstbuf.bw;
			t.psm = gs.dstbuf.psm;
			SaveTex(&t, 0);

			t.tbp0 = gs.srcbuf.bp;
			t.tw = gs.imageWnew;
			t.th = gs.imageHnew;
			t.tbw = gs.srcbuf.bw;
			t.psm = gs.srcbuf.psm;
			SaveTex(&t, 0);
		}

	#endif
	}

 }
