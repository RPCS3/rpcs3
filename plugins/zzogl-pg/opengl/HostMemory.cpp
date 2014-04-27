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

#include <stdlib.h>
#include "Mem.h"
#include "x86.h"
#include "targets.h"
#include "ZZoglVB.h"

// flush current vertices, call before setting new registers (the main render method)
extern void Flush(int context);

u8* g_pbyGSMemory = NULL;   // 4Mb GS system mem

void GSMemory::init()
{
    const u32 mem_size = MEMORY_END + 0x10000; // leave some room for out of range accesses (saves on the checks)

    // clear
    g_pbyGSMemory = (u8*)_aligned_malloc(mem_size, 1024);
    memset(g_pbyGSMemory, 0, mem_size);
}

void GSMemory::destroy()
{
    _aligned_free(g_pbyGSMemory);
    g_pbyGSMemory = NULL;
}

u8* GSMemory::get()
{
    return g_pbyGSMemory;
}

u8* GSMemory::get(u32 addr)
{
    return &g_pbyGSMemory[addr*8];
}
u8* GSMemory::get_raw(u32 addr)
{
    return &g_pbyGSMemory[addr];
}

u8* g_pbyGSClut = NULL;		// ZZ

void GSClut::init()
{
    g_pbyGSClut = (u8*)_aligned_malloc(256 * 8, 1024); // need 512 alignment!
    memset(g_pbyGSClut, 0, 256*8);
}

void GSClut::destroy()
{
    _aligned_free(g_pbyGSClut);
    g_pbyGSClut = NULL;
}

u8* GSClut::get()
{
    return g_pbyGSClut;
}

u8* GSClut::get(u32 addr)
{
    return &g_pbyGSClut[addr*8];
}
u8* GSClut::get_raw(u32 addr)
{
    return &g_pbyGSClut[addr];
}

extern _getPixelAddress getPixelFun[64];

extern CRangeManager s_RangeMngr; // manages overwritten memory
extern void ResolveInRange(int start, int end);

static vector<u8> s_vTempBuffer, s_vTransferCache;
static int gs_imageEnd = 0;

//	From the start of monster labs. In all 3 cases, psm == 0.
//	ZZogl-PG:  GetRectMemAddress(0x3f4000, 0x404000, 0x0, 0x0, 0x0, 0x100, 0x40, 0x3f40, 0x100);
//	ZZogl-PG:  GetRectMemAddress(0x3f8000, 0x408000, 0x0, 0x0, 0x0, 0x100, 0x40, 0x3f80, 0x100);
//	ZZogl-PG:  GetRectMemAddress(0x3fc000, 0x40c000, 0x0, 0x0, 0x0, 0x100, 0x40, 0x3fc0, 0x100);

void GetRectMemAddress(int& start, int& end, int psm, int x, int y, int w, int h, int bp, int bw)
{
    FUNCLOG
    u32 bits = 0;

    if (m_Blocks[psm].bpp == 0)
    {
        ZZLog::Error_Log("ZeroGS: Bad psm 0x%x.", psm);
        start = 0;
        end = MEMORY_END;
        return;
    }

    if (PSMT_ISZTEX(psm))
    {
    	// This still needs an eye kept on it.
        const BLOCK& b = m_Blocks[psm];
		const int x2 = x + w + b.width - 1;
		const int y2 = y + h - 1;
        bw = bw / b.width;
        
        start = (bp + ((y / b.height) * bw + (x / b.width)) * 0x20) * 0x100;
        end = (bp + ((y2 / b.height) * bw + (x2 / b.width)) * 0x20) * 0x100;
        return;
    }
 
    bits = PSMT_BITS_NUM(psm);
    start = getPixelFun[psm](x, y, bp, bw);
    end = getPixelFun[psm](x + w - 1, y + h - 1, bp, bw) + 1;
 
    if (bits > 0)
    {
        start *= bits;
        end *= bits;
    }
    else
    {
        start /= 2;
        end /= 2;
    }
}

// Same as GetRectMemAddress, except that we know x & y are zero, so it's simplified a bit.
void GetRectMemAddressZero(int& start, int& end, int psm, int w, int h, int bp, int bw)
{
    FUNCLOG
    u32 bits = 0;

    if (m_Blocks[psm].bpp == 0)
    {
        ZZLog::Error_Log("ZeroGS: Bad psm 0x%x.", psm);
        start = 0;
        end = MEMORY_END;
        return;
    }

    if (PSMT_ISZTEX(psm))
    {
    	// This still needs an eye kept on it.
        const BLOCK& b = m_Blocks[psm];
		const int x2 = w + b.width - 1;
		const int y2 = h - 1;
        bw = bw / b.width;
        
		start = bp * 0x100;
        end = (bp + ((y2 / b.height) * bw + (x2 / b.width)) * 0x20) * 0x100;
        return;
    }
 
    bits = PSMT_BITS_NUM(psm);
    start = getPixelFun[psm](0, 0, bp, bw);
    end = getPixelFun[psm](w - 1, h - 1, bp, bw) + 1;
 
    if (bits > 0)
    {
        start *= bits;
        end *= bits;
    }
    else
    {
        start /= 2;
        end /= 2;
    }
}
 

void GetRectMemAddress(int& start, int& end, int psm, Point p, Size s, int bp, int bw)
{
	GetRectMemAddress(start, end, psm, p.x, p.y, s.w, s.h, bp, bw);
}

void GetRectMemAddress(int& start, int& end, int psm, int x, int y, Size s, int bp, int bw)
{
	GetRectMemAddress(start, end, psm, x, y, s.w, s.h, bp, bw);
}

void GetRectMemAddressZero(int& start, int& end, int psm, Size s, int bp, int bw)
{
	GetRectMemAddressZero(start, end, psm, s.w, s.h, bp, bw);
}

void InitTransferHostLocal()
{
    FUNCLOG
 
#if defined(_DEBUG)
    // Xenosaga 1.
    if (gs.trxpos.dx + gs.imageNew.w > gs.dstbuf.bw)
        ZZLog::Debug_Log("Transfer error, width exceeded. (0x%x > 0X%x)", gs.trxpos.dx + gs.imageNew.w, gs.dstbuf.bw);
#endif
 
    //bool bHasFlushed = false;
 
    gs.image.x = gs.trxpos.dx;
    gs.image.y = gs.trxpos.dy;
 
    gs.imageEnd.x = gs.image.x + gs.imageNew.w;
    gs.imageEnd.y = gs.image.y + gs.imageNew.h;
 
    assert(gs.imageEnd.x < 2048 && gs.imageEnd.y < 2048);
 
    // This needs to be looked in to, since psm should *not* be 63.
    // hack! viewful joe
    if (gs.dstbuf.psm == 63) 
    {
    	ZZLog::WriteLn("gs.dstbuf.psm set to 0!");
    	gs.dstbuf.psm = 0;
    }
 
    int start, end;
 
    GetRectMemAddress(start, end, gs.dstbuf.psm, gs.trxpos.dx, gs.trxpos.dy, gs.imageNew, gs.dstbuf.bp, gs.dstbuf.bw);
 
    if (end > MEMORY_END)
    {
        // Monster Lab - the screwed up title screen
        // Init host local out of bounds! (end == 0x404000)
        // Init host local out of bounds! (end == 0x408000)
        // Init host local out of bounds! (end == 0x40c000)
        // MEMORY_END is 0x400000...
 
        ZZLog::Warn_Log("Init host local out of bounds! (end == 0x%x)", end);
		//gs.transferring = false;
        end = MEMORY_END;
    }
 
    gs_imageEnd = end;
 
    if (vb[0].nCount > 0) Flush(0);
    if (vb[1].nCount > 0) Flush(1);
 
    //ZZLog::Prim_Log("trans: bp:%x x:%x y:%x w:%x h:%x\n", gs.dstbuf.bp, gs.trxpos.dx, gs.trxpos.dy, gs.imageNew.w, gs.imageNew.h);
}
 
void TransferHostLocal(const void* pbyMem, u32 nQWordSize)
{
    FUNCLOG
 
    int start = -1, end = -1;
 
    GetRectMemAddress(start, end, gs.dstbuf.psm, gs.image, gs.imageNew, gs.dstbuf.bp, gs.dstbuf.bw);
	
	if ((start == -1) || (end == -1)) ZZLog::WriteLn("start == %d, end == %d", start, end);
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
        t.tw = gs.imageNew.w;
        t.th = gs.imageNew.h;
        t.tbw = gs.dstbuf.bw;
        t.psm = gs.dstbuf.psm;
        SaveTex(&t, 0);
    }
 
#endif
}
 
void InitTransferLocalHost()
{
    FUNCLOG
    assert(gs.trxpos.sx + gs.imageNew.w <= 2048 && gs.trxpos.sy + gs.imageNew.h <= 2048);
 
#if defined(_DEBUG)
    if (gs.trxpos.sx + gs.imageNew.w > gs.srcbuf.bw)
        ZZLog::Debug_Log("Transfer error, width exceeded. (0x%x > 0x%x)", gs.trxpos.sx + gs.imageNew.w, gs.srcbuf.bw);
#endif
 
    gs.image.x = gs.trxpos.sx;
    gs.image.y = gs.trxpos.sy;
 
    gs.imageEnd.x = gs.image.x + gs.imageNew.w;
    gs.imageEnd.y = gs.image.y + gs.imageNew.h;
 
    s_vTransferCache.resize(0);
 
    int start, end;
 
    GetRectMemAddress(start, end, gs.srcbuf.psm, gs.trxpos.sx, gs.trxpos.sy, gs.imageNew, gs.srcbuf.bp, gs.srcbuf.bw);
 
    ResolveInRange(start, end);
}
 
template <class T>
void TransferLocalHost(void* pbyMem, u32 nQWordSize, int& x, int& y, u8 *pstart)
{
    _readPixel_0 rp = readPixelFun_0[gs.srcbuf.psm];
 
    int i = x, j = y;
    T* pbuf = (T*)pbyMem;
    u32 nSize = nQWordSize * 16 / sizeof(T);
 
    for (; i < gs.imageEnd.y; ++i)
    {
        for (; j < gs.imageEnd.x && nSize > 0; ++j, --nSize)
        {
            *pbuf++ = rp(pstart, j % 2048, i % 2048, gs.srcbuf.bw);
        }
 
        if (j >= gs.imageEnd.x)
        {
            assert(j == gs.imageEnd.x);
            j = gs.trxpos.sx;
        }
        else
        {
            assert(nSize == 0);
            break;
        }
    }
}
 
void TransferLocalHost_24(void* pbyMem, u32 nQWordSize, int& x, int& y, u8 *pstart)
{
    _readPixel_0 rp = readPixelFun_0[gs.srcbuf.psm];
 
    int i = x, j = y;
    u8* pbuf = (u8*)pbyMem;
    u32 nSize = nQWordSize * 16 / 3;
 
    for (; i < gs.imageEnd.y; ++i)
    {
        for (; j < gs.imageEnd.x && nSize > 0; ++j, --nSize)
        {
            u32 p = rp(pstart, j % 2048, i % 2048, gs.srcbuf.bw);
            pbuf[0] = (u8)p;
            pbuf[1] = (u8)(p >> 8);
            pbuf[2] = (u8)(p >> 16);
            pbuf += 3;
        }
 
        if (j >= gs.imageEnd.x)
        {
            assert(j == gs.imageEnd.x);
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
    assert(gs.imageTransfer == XFER_LOCAL_TO_HOST);
 
    u8* pstart = g_pbyGSMemory + 256 * gs.srcbuf.bp;
 
    switch(PSMT_BITMODE(gs.srcbuf.psm))
    {
		case 0:
			TransferLocalHost<u32>(pbyMem, nQWordSize, gs.image.y, gs.image.x, pstart);
			break;
		case 1:
			TransferLocalHost_24(pbyMem, nQWordSize, gs.image.y, gs.image.x, pstart);
			break;
		case 2:
			TransferLocalHost<u16>(pbyMem, nQWordSize, gs.image.y, gs.image.x, pstart);
			break;
		case 3:
			TransferLocalHost<u8>(pbyMem, nQWordSize, gs.image.y, gs.image.x, pstart);
			break;
		default:
			assert(0);
			break;
    }
 
    if (gs.image.y >= gs.imageEnd.y)
    {
        ZZLog::Error_Log("gs.image.y >= gs.imageEnd.y!");
        assert(gs.image.y == gs.imageEnd.y);
		gs.transferring = false;
    }
}
 
__forceinline void _TransferLocalLocal()
{
    //ZZLog::Error_Log("TransferLocalLocal(0x%x, 0x%x)", gs.srcbuf.psm, gs.dstbuf.psm);
    _writePixel_0 wp = writePixelFun_0[gs.srcbuf.psm];
    _readPixel_0 rp = readPixelFun_0[gs.dstbuf.psm];
    u8* pSrcBuf = g_pbyGSMemory + gs.srcbuf.bp * 256;
    u8* pDstBuf = g_pbyGSMemory + gs.dstbuf.bp * 256;
    u32 widthlimit = 4;
    u32 maxX = gs.trxpos.sx + gs.imageNew.w;
    u32 maxY = gs.trxpos.sy + gs.imageNew.h;
 
    if (PSMT_BITMODE(gs.srcbuf.psm) == 0) widthlimit = 2;
    if ((gs.imageNew.w & widthlimit) != 0) return;
 
    for(u32 i = gs.trxpos.sy, i2 = gs.trxpos.dy; i < maxY; i++, i2++)
    {
        for(u32 j = gs.trxpos.sx, j2 = gs.trxpos.dx; j < maxX; j += widthlimit, j2 += widthlimit)
        {
            wp(pDstBuf, j2%2048, i2%2048,
               rp(pSrcBuf, j%2048, i%2048, gs.srcbuf.bw), gs.dstbuf.bw);
 
            wp(pDstBuf, (j2+1)%2048, i2%2048,
               rp(pSrcBuf, (j+1)%2048, i%2048, gs.srcbuf.bw), gs.dstbuf.bw);
 
            if (widthlimit > 2)
            {
                // Then widthlimit == 4.
                wp(pDstBuf, (j2+2)%2048, i2%2048,
                   rp(pSrcBuf, (j+2)%2048, i%2048, gs.srcbuf.bw), gs.dstbuf.bw);
 
                wp(pDstBuf, (j2+3)%2048, i2%2048,
                   rp(pSrcBuf, (j+3)%2048, i%2048, gs.srcbuf.bw), gs.dstbuf.bw);
            }
        }
    }
}
 
__forceinline void _TransferLocalLocal_4()
{
    //ZZLog::Error_Log("TransferLocalLocal_4(0x%x, 0x%x)", gs.srcbuf.psm, gs.dstbuf.psm);
    _getPixelAddress_0 gsp = getPixelFun_0[gs.srcbuf.psm];
    _getPixelAddress_0 gdp = getPixelFun_0[gs.dstbuf.psm];
    u8* pSrcBuf = g_pbyGSMemory + gs.srcbuf.bp * 256;
    u8* pDstBuf = g_pbyGSMemory + gs.dstbuf.bp * 256;
    u32 maxX = gs.trxpos.sx + gs.imageNew.w;
    u32 maxY = gs.trxpos.sy + gs.imageNew.h;
 
    assert((gs.imageNew.w % 8) == 0);
 
    for(u32 i = gs.trxpos.sy, i2 = gs.trxpos.dy; i < maxY; ++i, ++i2)
    {
        for(u32 j = gs.trxpos.sx, j2 = gs.trxpos.dx; j < maxX; j += 8, j2 += 8)
        {
            /* NOTE: the 2 conseq 4bit values are in NOT in the same byte */
            u32 read = gsp(j%2048, i%2048, gs.srcbuf.bw);
            u32 write = gdp(j2%2048, i2%2048, gs.dstbuf.bw);
            pDstBuf[write] = (pDstBuf[write]&0xf0)|(pSrcBuf[read]&0x0f);
 
            read = gsp((j+1)%2048, i%2048, gs.srcbuf.bw);
            write = gdp((j2+1)%2048, i2%2048, gs.dstbuf.bw);
            pDstBuf[write] = (pDstBuf[write]&0x0f)|(pSrcBuf[read]&0xf0);
 
            read = gsp((j+2)%2048, i%2048, gs.srcbuf.bw);
            write = gdp((j2+2)%2048, i2%2048, gs.dstbuf.bw);
            pDstBuf[write] = (pDstBuf[write]&0xf0)|(pSrcBuf[read]&0x0f);
 
            read = gsp((j+3)%2048, i%2048, gs.srcbuf.bw);
            write = gdp((j2+3)%2048, i2%2048, gs.dstbuf.bw);
            pDstBuf[write] = (pDstBuf[write]&0x0f)|(pSrcBuf[read]&0xf0);
 
            read = gsp((j+4)%2048, i%2048, gs.srcbuf.bw);
            write = gdp((j2+4)%2048, i2%2048, gs.dstbuf.bw);
            pDstBuf[write] = (pDstBuf[write]&0xf0)|(pSrcBuf[read]&0x0f);
 
            read = gsp((j+5)%2048, i%2048, gs.srcbuf.bw);
            write = gdp((j2+5)%2048, i2%2048, gs.dstbuf.bw);
            pDstBuf[write] = (pDstBuf[write]&0x0f)|(pSrcBuf[read]&0xf0);
 
            read = gsp((j+6)%2048, i%2048, gs.srcbuf.bw);
            write = gdp((j2+6)%2048, i2%2048, gs.dstbuf.bw);
            pDstBuf[write] = (pDstBuf[write]&0xf0)|(pSrcBuf[read]&0x0f);
 
            read = gsp((j+7)%2048, i%2048, gs.srcbuf.bw);
            write = gdp((j2+7)%2048, i2%2048, gs.dstbuf.bw);
            pDstBuf[write] = (pDstBuf[write]&0x0f)|(pSrcBuf[read]&0xf0);
        }
    }
}
 
// dir depends on trxpos.dirx & trxpos.diry
void TransferLocalLocal()
{
    FUNCLOG
 
    //ZZLog::Error_Log("I'z in your code, transferring your memory...");
    assert(gs.imageTransfer == XFER_LOCAL_TO_LOCAL);
    assert(gs.trxpos.sx + gs.imageNew.w < 2048 && gs.trxpos.sy + gs.imageNew.h < 2048);
    assert(gs.trxpos.dx + gs.imageNew.w < 2048 && gs.trxpos.dy + gs.imageNew.h < 2048);
    assert((gs.srcbuf.psm&0x7) == (gs.dstbuf.psm&0x7));
 
    if (gs.trxpos.sx + gs.imageNew.w > gs.srcbuf.bw)
        ZZLog::Debug_Log("Transfer error, src width exceeded.(0x%x > 0x%x)", gs.trxpos.sx + gs.imageNew.w, gs.srcbuf.bw);
 
    if (gs.trxpos.dx + gs.imageNew.w > gs.dstbuf.bw)
        ZZLog::Debug_Log("Transfer error, dst width exceeded.(0x%x > 0x%x)", gs.trxpos.dx + gs.imageNew.w, gs.dstbuf.bw);
 
    int srcstart, srcend, dststart, dstend;
 
    GetRectMemAddress(srcstart, srcend, gs.srcbuf.psm, gs.trxpos.sx, gs.trxpos.sy, gs.imageNew, gs.srcbuf.bp, gs.srcbuf.bw);
    GetRectMemAddress(dststart, dstend, gs.dstbuf.psm, gs.trxpos.dx, gs.trxpos.dy, gs.imageNew, gs.dstbuf.bp, gs.dstbuf.bw);
 
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
 
    if (PSMT_BITMODE(gs.srcbuf.psm) != 4)
    {
        _TransferLocalLocal();
    }
    else
    {
        _TransferLocalLocal_4();
    }
 
    g_MemTargs.ClearRange(dststart, dstend);
 
#ifdef ZEROGS_DEVBUILD
 
    if (g_bSaveTrans)
    {
        tex0Info t;
        t.tbp0 = gs.dstbuf.bp;
        t.tw = gs.imageNew.w;
        t.th = gs.imageNew.h;
        t.tbw = gs.dstbuf.bw;
        t.psm = gs.dstbuf.psm;
        SaveTex(&t, 0);
 
        t.tbp0 = gs.srcbuf.bp;
        t.tw = gs.imageNew.w;
        t.th = gs.imageNew.h;
        t.tbw = gs.srcbuf.bw;
        t.psm = gs.srcbuf.psm;
        SaveTex(&t, 0);
    }
 
#endif
}

