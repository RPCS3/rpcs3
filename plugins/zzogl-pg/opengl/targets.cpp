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

#include <stdlib.h>
#include <math.h>

#include "GS.h"
#include "Mem.h"
#include "x86.h"
#include "targets.h"
#include "ZZoglShaders.h"
#include "ZZClut.h"
#include "ZZoglVB.h"

#ifdef ZEROGS_SSE2
#include <immintrin.h>
#endif

#define RHA
//#define RW

extern int g_TransferredToGPU;

#if !defined(ZEROGS_DEVBUILD)
#	define INC_RESOLVE()
#else
#	define INC_RESOLVE() ++g_nResolve
#endif

extern int s_nResolved;
extern u32 g_nResolve;
extern bool g_bSaveTrans;

CRenderTargetMngr s_RTs, s_DepthRTs;
CBitwiseTextureMngr s_BitwiseTextures;
CMemoryTargetMngr g_MemTargs;

//extern u32 s_ptexCurSet[2];
bool g_bSaveZUpdate = 0;

int VALIDATE_THRESH = 8;
u32 TEXDESTROY_THRESH = 16;
#define FORCE_TEXDESTROY_THRESH (3) // destroy texture after FORCE_TEXDESTROY_THRESH frames

void _Resolve(const void* psrc, int fbp, int fbw, int fbh, int psm, u32 fbm, bool mode);
void SetWriteDepth();
bool IsWriteDepth();
bool IsWriteDestAlphaTest();

//--------------------------------------------------


inline bool CheckWidthIsSame(const frameInfo& frame, CRenderTarget* ptarg)
{
	if (PSMT_ISHALF(frame.psm) == PSMT_ISHALF(ptarg->psm))
		return (frame.fbw == ptarg->fbw);

	if (PSMT_ISHALF(frame.psm))
		return (frame.fbw == 2 * ptarg->fbw);
	else
		return (2 * frame.fbw == ptarg->fbw);
}

//////////////////////////////////////
// Texture Mngr For Bitwise AND Ops //
//////////////////////////////////////
void CBitwiseTextureMngr::Destroy()
{
	FUNCLOG

	for (map<u32, u32>::iterator it = mapTextures.begin(); it != mapTextures.end(); ++it)
	{
			glDeleteTextures(1, &it->second);
	}

	mapTextures.clear();
}

u32 CBitwiseTextureMngr::GetTexInt(u32 bitvalue, u32 ptexDoNotDelete)
{
	FUNCLOG

	if (mapTextures.size() > 32)
	{
		// randomly delete 8
		for (map<u32, u32>::iterator it = mapTextures.begin(); it != mapTextures.end();)
		{
			if (!(rand()&3) && it->second != ptexDoNotDelete)
			{
				glDeleteTextures(1, &it->second);
				mapTextures.erase(it++);
			}
			else
			{
				++it;
			}
		}
	}

	if (glGetError() != GL_NO_ERROR) ZZLog::Error_Log("Error before creation of bitmask texture.");

	// create a new tex
	u32 ptex;

	glGenTextures(1, &ptex);

	if (glGetError() != GL_NO_ERROR) ZZLog::Error_Log("Error on generation of bitmask texture.");

	vector<u16> data(GPU_TEXMASKWIDTH);

	for (u32 i = 0; i < GPU_TEXMASKWIDTH; ++i)
	{
		data[i] = (((i << MASKDIVISOR) & bitvalue) << 6); // add the 1/2 offset so that
	}

	//	data[GPU_TEXMASKWIDTH] = 0;	// I remove GPU_TEXMASKWIDTH+1 element of this texture, because it was a reason of FFC crush
									// Probably, some sort of PoT incompability in drivers.

	glBindTexture(GL_TEXTURE_RECTANGLE, ptex);
	if (glGetError() != GL_NO_ERROR) ZZLog::Error_Log("Error on binding bitmask texture.");

	TextureRect2(GL_LUMINANCE16, GPU_TEXMASKWIDTH, 1, GL_LUMINANCE, GL_UNSIGNED_SHORT, &data[0]);
	if (glGetError() != GL_NO_ERROR) ZZLog::Error_Log("Error on applying bitmask texture.");

//	Removing clamping, as it seems lead to numerous troubles at some drivers
//	Need to observe, may be clamping is not really needed.
	/* setRectWrap2(GL_REPEAT);

	GLint Error = glGetError();
	if( Error != GL_NO_ERROR ) {
		ERROR_LOG_SPAM_TEST("Failed to create bitmask texture; \t");
		if (SPAM_PASS) {
			ZZLog::Log("bitmask cache %d; \t",  mapTextures.size());
			switch (Error) {
				case GL_INVALID_ENUM: ZZLog::Error_Log("Invalid enumerator.") ; break;
				case GL_INVALID_VALUE: ZZLog::Error_Log("Invalid value."); break;
				case GL_INVALID_OPERATION: ZZLog::Error_Log("Invalid operation."); break;
				default: ZZLog::Error_Log("Error number: %d.", Error);
			}
		}
		return 0;
	}*/

	mapTextures[bitvalue] = ptex;

	return ptex;
}

void CRangeManager::RangeSanityCheck()
{
#ifdef _DEBUG
	// sanity check

	for (int i = 0; i < (int)ranges.size() - 1; ++i)
	{
		assert(ranges[i].end < ranges[i+1].start);
	}

#endif
}

void CRangeManager::Insert(int start, int end)
{
	FUNCLOG
	int imin = 0, imax = (int)ranges.size(), imid;

	RangeSanityCheck();

	switch (ranges.size())
	{

		case 0:
			ranges.push_back(RANGE(start, end));
			return;

		case 1:
			if (end < ranges.front().start)
			{
				ranges.insert(ranges.begin(), RANGE(start, end));
			}
			else if (start > ranges.front().end)
			{
				ranges.push_back(RANGE(start, end));
			}
			else
			{
				if (start < ranges.front().start) ranges.front().start = start;
				if (end > ranges.front().end) ranges.front().end = end;
			}

			return;
	}

	// find where start is
	while (imin < imax)
	{
		imid = (imin + imax) >> 1;

		assert(imid < (int)ranges.size());

		if ((ranges[imid].end >= start) && ((imid == 0) || (ranges[imid-1].end < start)))
		{
			imin = imid;
			break;
		}
		else if (ranges[imid].start > start)
		{
			imax = imid;
		}
		else
		{
			imin = imid + 1;
		}
	}

	int startindex = imin;

	if (startindex >= (int)ranges.size())
	{
		// non intersecting
		assert(start > ranges.back().end);
		ranges.push_back(RANGE(start, end));
		return;
	}

	if (startindex == 0 && end < ranges.front().start)
	{
		ranges.insert(ranges.begin(), RANGE(start, end));
		RangeSanityCheck();
		return;
	}

	imin = 0;
	imax = (int)ranges.size();

	// find where end is

	while (imin < imax)
	{
		imid = (imin + imax) >> 1;

		assert(imid < (int)ranges.size());

		if ((ranges[imid].end <= end) && ((imid == ranges.size() - 1) || (ranges[imid+1].start > end)))
		{
			imin = imid;
			break;
		}
		else if (ranges[imid].start >= end)
		{
			imax = imid;
		}
		else
		{
			imin = imid + 1;
		}
	}

	int endindex = imin;

	if (startindex > endindex)
	{
		// create a new range
		ranges.insert(ranges.begin() + startindex, RANGE(start, end));
		RangeSanityCheck();
		return;
	}

	if (endindex >= (int)ranges.size() - 1)
	{
		// pop until startindex is reached
		int lastend = ranges.back().end;
		int numpop = (int)ranges.size() - startindex - 1;

		while (numpop-- > 0)
		{
			ranges.pop_back();
		}

		assert(start <= ranges.back().end);

		if (start < ranges.back().start) ranges.back().start = start;
		if (lastend > ranges.back().end) ranges.back().end = lastend;
		if (end > ranges.back().end) ranges.back().end = end;

		RangeSanityCheck();

		return;
	}

	if (endindex == 0)
	{
		assert(end >= ranges.front().start);

		if (start < ranges.front().start) ranges.front().start = start;
		if (end > ranges.front().end) ranges.front().end = end;

		RangeSanityCheck();
	}

	// somewhere in the middle
	if (ranges[startindex].start < start) start = ranges[startindex].start;

	if (startindex < endindex)
	{
		ranges.erase(ranges.begin() + startindex, ranges.begin() + endindex);
	}

	if (start < ranges[startindex].start) ranges[startindex].start = start;
	if (end > ranges[startindex].end) ranges[startindex].end = end;

	RangeSanityCheck();
}

CRangeManager s_RangeMngr; // manages overwritten memory

void ResolveInRange(int start, int end)
{
	FUNCLOG
	list<CRenderTarget*> listTargs = CreateTargetsList(start, end);
	/*	s_DepthRTs.GetTargs(start, end, listTargs);
		s_RTs.GetTargs(start, end, listTargs);*/

	if (listTargs.size() > 0)
	{
		FlushBoth();

		// We need another list, because old one could be brocken by Flush().
		listTargs.clear();
		listTargs = CreateTargetsList(start, end);
		/*		s_DepthRTs.GetTargs(start, end, listTargs_1);
				s_RTs.GetTargs(start, end, listTargs_1);*/

		for (list<CRenderTarget*>::iterator it = listTargs.begin(); it != listTargs.end(); ++it)
		{
			// only resolve if not completely covered
			if ((*it)->created == 123)
				(*it)->Resolve();
			else
				ZZLog::Debug_Log("Resolving non-existing object! Destroy code %d.", (*it)->created);
		}
	}
}

//////////////////
// Transferring //
//////////////////
void FlushTransferRange(CRenderTarget* ptarg, int start, int end, int texstart, int texend)
{
	int range_size = end - start;
	
	if (!(ptarg->start < texend && ptarg->end > texstart))
	{
		// check if target is currently being used

		if (!(conf.settings().no_quick_resolve))
		{
			if (ptarg->fbp != vb[0].gsfb.fbp)
			{
				if (ptarg->fbp != vb[1].gsfb.fbp)
				{
					// this render target currently isn't used and is not in the texture's way, so can safely ignore
					// resolving it. Also the range has to be big enough compared to the target to really call it resolved
					// (ffx changing screens, shadowhearts)
					// start == ptarg->start, used for kh to transfer text

					if (ptarg->IsDepth() || range_size > 0x50000 || ((conf.settings().quick_resolve_1) && start == ptarg->start))
						ptarg->status |= CRenderTarget::TS_NeedUpdate | CRenderTarget::TS_Resolved;

					return;
				}
			}
		}
	}

	// the first range check was very rough; some games (dragonball z) have the zbuf in the same page as textures (but not overlapping)
	// so detect that condition
	if (ptarg->fbh % m_Blocks[ptarg->psm].height)
	{
		// get start of left-most boundry page
		int targstart, targend;
		GetRectMemAddressZero(targstart, targend, ptarg->psm, ptarg->fbw, ptarg->fbh & ~(m_Blocks[ptarg->psm].height - 1), ptarg->fbp, ptarg->fbw);

		if (start >= targend)
		{
			// don't bother
			if ((ptarg->fbh % m_Blocks[ptarg->psm].height) <= 2) return;

			// calc how many bytes of the block that the page spans
		}
	}
	
	if (start < ptarg->end && end > ptarg->start)
	{
		ptarg->status |= CRenderTarget::TS_Resolved;

		if (conf.settings().no_depth_update || conf.settings().gust)
		{
			if (conf.settings().gust)
			{
				if (range_size > 0x40000) 
				{
					ptarg->status |= CRenderTarget::TS_NeedUpdate;
					return;
				}
				/*else
				{
					ZZLog::WriteLn("FlushTransferRange: Gust Hack - No update!");
				}*/
			}
				
			if (conf.settings().no_depth_update)
			{
				if (!ptarg->IsDepth() || range_size > 0x1000)
				{
					ptarg->status |= CRenderTarget::TS_NeedUpdate;
					return;
				} 
			}
		}
		else
		{
			ptarg->status |= CRenderTarget::TS_NeedUpdate;
		}
	}
}

void FlushTransferRanges(const tex0Info* ptex)
{
	FUNCLOG
	assert(s_RangeMngr.ranges.size() > 0);
	//bool bHasFlushed = false;
	list<CRenderTarget*> listTransmissionUpdateTargs;

	int texstart = -1, texend = -1;

	if (ptex != NULL) // If ptex is NULL, texstart & texend will be -1.
	{
		GetRectMemAddressZero(texstart, texend, ptex->psm, ptex->tw, ptex->th, ptex->tbp0, ptex->tbw);
	}

	for (vector<CRangeManager::RANGE>::iterator itrange = s_RangeMngr.ranges.begin(); itrange != s_RangeMngr.ranges.end(); ++itrange)
	{

		int start = itrange->start;
		int end = itrange->end;

		listTransmissionUpdateTargs.clear();
		listTransmissionUpdateTargs = CreateTargetsList(start, end);

		for (list<CRenderTarget*>::iterator it = listTransmissionUpdateTargs.begin(); it != listTransmissionUpdateTargs.end(); ++it)
		{
			CRenderTarget* ptarg = *it;

			if ((ptarg->status & CRenderTarget::TS_Virtual)) continue;
			FlushTransferRange(ptarg, start, end, texstart, texend);
		}

		g_MemTargs.ClearRange(start, end);
	}

	s_RangeMngr.Clear();
}


#if 0
// I removed some code here that wasn't getting called. The old versions #if'ed out below this.
#define RESOLVE_32_BIT(PSM, T, Tsrc, convfn) \
	{ \
		u32 mask, imask; \
		\
		if (PSMT_ISHALF(psm)) /* 16 bit */ \
		{\
			/* mask is shifted*/ \
			imask = RGBA32to16(fbm);\
			mask = (~imask)&0xffff;\
		}\
		else \
		{\
			mask = ~fbm;\
			imask = fbm;\
		}\
		\
		Tsrc* src = (Tsrc*)(psrc); \
		T* pPageOffset = (T*)g_pbyGSMemory + fbp*(256/sizeof(T)), *dst; \
		int maxfbh = (MEMORY_END-fbp*256) / (sizeof(T) * fbw); \
		if( maxfbh > fbh ) maxfbh = fbh; \
		\
		for(int i = 0; i < maxfbh; ++i) { \
			for(int j = 0; j < fbw; ++j) { \
				T dsrc = convfn(src[RW(j)]); \
				dst = pPageOffset + getPixelAddress##PSM##_0(j, i, fbw); \
				*dst = (dsrc & mask) | (*dst & imask); \
			} \
			src += RH(Pitch(fbw))/sizeof(Tsrc); \
		} \
	} \

#endif

#ifdef __LINUX__
//#define LOG_RESOLVE_PROFILE
#endif

template <typename Tdst, bool do_conversion>
inline void Resolve_32_Bit(const void* psrc, int fbp, int fbw, int fbh, const int psm, u32 fbm)
{
    u32 mask, imask;
#ifdef LOG_RESOLVE_PROFILE
     u32 startime = timeGetPreciseTime();
#endif

    if (PSMT_ISHALF(psm)) /* 16 bit */
    {
        /* mask is shifted*/
        imask = RGBA32to16(fbm);
        mask = (~imask)&0xffff;
    }
    else
    {
        mask = ~fbm;
        imask = fbm;
    }

    Tdst* pPageOffset = (Tdst*)g_pbyGSMemory + fbp*(256/sizeof(Tdst));
    Tdst* dst;
    Tdst  dsrc;

    int maxfbh = (MEMORY_END-fbp*256) / (sizeof(Tdst) * fbw);
    if( maxfbh > fbh ) maxfbh = fbh;
    
#ifdef LOG_RESOLVE_PROFILE
    ZZLog::Dev_Log("*** Resolve 32 bits: %dx%d in %x", maxfbh, fbw, psm);
#endif

    // Start the src array at the end to reduce testing in loop
    u32 raw_size = RH(Pitch(fbw))/sizeof(u32);
    u32* src = (u32*)(psrc) + (maxfbh-1)*raw_size;

    for(int i = maxfbh-1; i >= 0; --i) {
        for(int j = fbw-1; j >= 0; --j) {
            if (do_conversion) {
                dsrc = RGBA32to16(src[RW(j)]);
            } else {
                dsrc = (Tdst)src[RW(j)];
            }
            // They are 3 methods to call the functions
            // macro (compact, inline) but need a nice psm ; swich (inline) ; function pointer (compact)
            // Use a switch to allow inlining of the getPixel function.
            // Note: psm is const so the switch is completely optimized
            // Function method example:
            // dst = pPageOffset + getPixelFun_0[psm](j, i, fbw);
            switch (psm)
            {
                case PSMCT32:
                case PSMCT24:
                    dst = pPageOffset + getPixelAddress32_0(j, i, fbw);
                    break;

                case PSMCT16:
                    dst = pPageOffset + getPixelAddress16_0(j, i, fbw);
                    break;

                case PSMCT16S:
                    dst = pPageOffset + getPixelAddress16S_0(j, i, fbw);
                    break;

                case PSMT32Z:
                case PSMT24Z:
                    dst = pPageOffset + getPixelAddress32Z_0(j, i, fbw);
                    break;

                case PSMT16Z:
                    dst = pPageOffset + getPixelAddress16Z_0(j, i, fbw);
                    break;

                case PSMT16SZ:
                    dst = pPageOffset + getPixelAddress16SZ_0(j, i, fbw);
                    break;
            }
            *dst = (dsrc & mask) | (*dst & imask);
        }
        src -= raw_size;
    }
#ifdef LOG_RESOLVE_PROFILE
    ZZLog::Dev_Log("*** 32 bits: execution time %d", timeGetPreciseTime()-startime);
#endif
}

static const __aligned16 unsigned int pixel_5b_mask[4] = {0x0000001F, 0x0000001F, 0x0000001F, 0x0000001F};

#ifdef ZEROGS_SSE2
// The function process 2*2 pixels in 32bits. And 2*4 pixels in 16bits
template <u32 psm, u32 size, u32 pageTable[size][64], bool null_second_line, u32 INDEX>
__forceinline void update_8pixels_sse2(u32* src, u32* basepage, u32 i_msk, u32 j, u32 pix_mask, u32 src_pitch)
{
    u32* base_ptr;
    __m128i pixels_0;
    __m128i pixel_0_low;
    __m128i pixel_0_high;

    __m128i pixels_1;
    __m128i pixel_1_low;
    __m128i pixel_1_high;

    assert((i_msk&0x1) == 0); // Failure => wrong line selected

    // Note: pixels have a special arrangement in column. Here a short description when AA.x = 0
    //
    // 32 bits format: 8x2 pixels: the idea is to read pixels 0-3
    // It is easier to process 4 bits (we can not cross column bondary)
    //      0 1 4 5 8  9  12 13
    //      2 3 6 7 10 11 14 15
    //
    // 16 bits format: 16x2 pixels, each pixels have a lower and higher part.
    // Here the idea to read 0L-3L & 0H-3H to combine lower and higher part this avoid
    // data interleaving and useless read/write
    //      0L 1L 4L 5L 8L  9L  12L 13L  0H 1H 4H 5H 8H  9H  12H 13H
    //      2L 3L 6L 7L 10L 11L 14L 15L  2H 3H 6H 7H 10H 11H 14H 15H
    //
    if (AA.x == 2) {
        // Note: pixels (32bits) are stored like that:
        // p0 p0 p0 p0  p1 p1 p1 p1  p4 p4 p4 p4  p5 p5 p5 p5
        // ...
        // p2 p2 p2 p2  p3 p3 p3 p3  p6 p6 p6 p6  p7 p7 p7 p7
        base_ptr = &src[((j+INDEX)<<2)];
        pixel_0_low = _mm_loadl_epi64((__m128i*)(base_ptr + 3));
        if (!null_second_line) pixel_0_high = _mm_loadl_epi64((__m128i*)(base_ptr + 3 + src_pitch));

        if (PSMT_ISHALF(psm)) {
            pixel_1_low = _mm_loadl_epi64((__m128i*)(base_ptr + 3 + 32));
            if (!null_second_line) pixel_1_high = _mm_loadl_epi64((__m128i*)(base_ptr + 3 + 32 + src_pitch));
        }
    } else if(AA.x ==1) {
        // Note: pixels (32bits) are stored like that:
        // p0 p0 p1 p1  p4 p4 p5 p5
        // ...
        // p2 p2 p3 p3  p6 p6 p7 p7
        base_ptr = &src[((j+INDEX)<<1)];
        pixel_0_low = _mm_loadl_epi64((__m128i*)(base_ptr + 1));
        if (!null_second_line) pixel_0_high = _mm_loadl_epi64((__m128i*)(base_ptr + 1 + src_pitch));

        if (PSMT_ISHALF(psm)) {
            pixel_1_low = _mm_loadl_epi64((__m128i*)(base_ptr + 1 + 16));
            if (!null_second_line) pixel_1_high = _mm_loadl_epi64((__m128i*)(base_ptr + 1 + 16 + src_pitch));
        }
    } else {
        // Note: pixels (32bits) are stored like that:
        // p0 p1  p4 p5
        // p2 p3  p6 p7
        base_ptr = &src[(j+INDEX)];
        pixel_0_low = _mm_loadl_epi64((__m128i*)base_ptr);
        if (!null_second_line) pixel_0_high = _mm_loadl_epi64((__m128i*)(base_ptr + src_pitch));

        if (PSMT_ISHALF(psm)) {
            pixel_1_low = _mm_loadl_epi64((__m128i*)(base_ptr + 8));
            if (!null_second_line) pixel_1_high = _mm_loadl_epi64((__m128i*)(base_ptr + 8 + src_pitch));
        }
    }

    // 2nd line does not exist... Just duplicate the pixel value
    if(null_second_line) {
        pixel_0_high = pixel_0_low;
        if (PSMT_ISHALF(psm)) pixel_1_high = pixel_1_low;
    }

    // Merge the 2 dword
    pixels_0 = _mm_unpacklo_epi64(pixel_0_low, pixel_0_high);
    if (PSMT_ISHALF(psm)) pixels_1 = _mm_unpacklo_epi64(pixel_1_low, pixel_1_high);

    // transform pixel from ARGB:8888 to ARGB:1555
    if (psm == PSMCT16 || psm == PSMCT16S) {
        // shift pixel instead of the mask. It allow to keep 1 mask into a register
        // instead of 4 (not enough room on x86...).
        __m128i pixel_mask = _mm_load_si128((__m128i*)pixel_5b_mask);

        __m128i pixel_0_B = _mm_srli_epi32(pixels_0, 3);
        pixel_0_B = _mm_and_si128(pixel_0_B, pixel_mask);

        __m128i pixel_0_G = _mm_srli_epi32(pixels_0, 11);
        pixel_0_G = _mm_and_si128(pixel_0_G, pixel_mask);

        __m128i pixel_0_R = _mm_srli_epi32(pixels_0, 19);
        pixel_0_R = _mm_and_si128(pixel_0_R, pixel_mask);

        // Note: because of the logical shift we do not need to mask the value
        __m128i pixel_0_A = _mm_srli_epi32(pixels_0, 31);

        // Realignment of pixels
        pixel_0_A = _mm_slli_epi32(pixel_0_A, 15);
        pixel_0_R = _mm_slli_epi32(pixel_0_R, 10);
        pixel_0_G = _mm_slli_epi32(pixel_0_G, 5);

        // rebuild a complete pixel
        pixels_0 = _mm_or_si128(pixel_0_A, pixel_0_B);
        pixels_0 = _mm_or_si128(pixels_0, pixel_0_G);
        pixels_0 = _mm_or_si128(pixels_0, pixel_0_R);

        // do the same for pixel_1
        __m128i pixel_1_B = _mm_srli_epi32(pixels_1, 3);
        pixel_1_B = _mm_and_si128(pixel_1_B, pixel_mask);

        __m128i pixel_1_G = _mm_srli_epi32(pixels_1, 11);
        pixel_1_G = _mm_and_si128(pixel_1_G, pixel_mask);

        __m128i pixel_1_R = _mm_srli_epi32(pixels_1, 19);
        pixel_1_R = _mm_and_si128(pixel_1_R, pixel_mask);

        __m128i pixel_1_A = _mm_srli_epi32(pixels_1, 31);

        // Realignment of pixels
        pixel_1_A = _mm_slli_epi32(pixel_1_A, 15);
        pixel_1_R = _mm_slli_epi32(pixel_1_R, 10);
        pixel_1_G = _mm_slli_epi32(pixel_1_G, 5);

        // rebuild a complete pixel
        pixels_1 = _mm_or_si128(pixel_1_A, pixel_1_B);
        pixels_1 = _mm_or_si128(pixels_1, pixel_1_G);
        pixels_1 = _mm_or_si128(pixels_1, pixel_1_R);
    }

    // Move the pixels to higher parts and merge it with pixels_0
    if (PSMT_ISHALF(psm)) {
        pixels_1 = _mm_slli_epi32(pixels_1, 16);
        pixels_0 = _mm_or_si128(pixels_0, pixels_1);
    }

    // Status 16 bits
    // pixels_0 = p3H p3L p2H p2L  p1H p1L p0H p0L
    // Status 32 bits
    // pixels_0 = p3 p2 p1 p0

    // load the destination add
    u32* dst_add;
    if (PSMT_ISHALF(psm))
        dst_add = basepage + (pageTable[i_msk][(INDEX)] >> 1);
    else
        dst_add = basepage + pageTable[i_msk][(INDEX)];

    // Save some memory access when pix_mask is 0.
    if (pix_mask) {
        // Build fbm mask (tranform a u32 to a 4 packets u32)
        // In 16 bits texture one packet is "0000 DATA"
        __m128i imask = _mm_cvtsi32_si128(pix_mask);
        imask = _mm_shuffle_epi32(imask, 0);

        // apply the mask on new values
        pixels_0 = _mm_andnot_si128(imask, pixels_0);

        __m128i old_pixels_0;
        __m128i final_pixels_0;

        old_pixels_0 = _mm_and_si128(imask, _mm_load_si128((__m128i*)dst_add));
        final_pixels_0 = _mm_or_si128(old_pixels_0, pixels_0);

        _mm_store_si128((__m128i*)dst_add, final_pixels_0);
    } else {
        // Note: because we did not read the previous value of add. We could bypass the cache.
        // We gains a few percents
        _mm_stream_si128((__m128i*)dst_add, pixels_0);
    }

}

// Update 2 lines of a page (2*64 pixels)
template <u32 psm, u32 size, u32 pageTable[size][64], bool null_second_line>
__forceinline void update_pixels_row_sse2(u32* src, u32* basepage, u32 i_msk, u32 j, u32 pix_mask, u32 raw_size)
{
    update_8pixels_sse2<psm, size, pageTable, null_second_line, 0>(src, basepage, i_msk, j, pix_mask, raw_size);
    update_8pixels_sse2<psm, size, pageTable, null_second_line, 2>(src, basepage, i_msk, j, pix_mask, raw_size);
    update_8pixels_sse2<psm, size, pageTable, null_second_line, 4>(src, basepage, i_msk, j, pix_mask, raw_size);
    update_8pixels_sse2<psm, size, pageTable, null_second_line, 6>(src, basepage, i_msk, j, pix_mask, raw_size);

    if(!PSMT_ISHALF(psm)) {
        update_8pixels_sse2<psm, size, pageTable, null_second_line, 8>(src, basepage, i_msk, j, pix_mask, raw_size);
        update_8pixels_sse2<psm, size, pageTable, null_second_line, 10>(src, basepage, i_msk, j, pix_mask, raw_size);
        update_8pixels_sse2<psm, size, pageTable, null_second_line, 12>(src, basepage, i_msk, j, pix_mask, raw_size);
        update_8pixels_sse2<psm, size, pageTable, null_second_line, 14>(src, basepage, i_msk, j, pix_mask, raw_size);
    }

    update_8pixels_sse2<psm, size, pageTable, null_second_line, 16>(src, basepage, i_msk, j, pix_mask, raw_size);
    update_8pixels_sse2<psm, size, pageTable, null_second_line, 18>(src, basepage, i_msk, j, pix_mask, raw_size);
    update_8pixels_sse2<psm, size, pageTable, null_second_line, 20>(src, basepage, i_msk, j, pix_mask, raw_size);
    update_8pixels_sse2<psm, size, pageTable, null_second_line, 22>(src, basepage, i_msk, j, pix_mask, raw_size);

    if(!PSMT_ISHALF(psm)) {
        update_8pixels_sse2<psm, size, pageTable, null_second_line, 24>(src, basepage, i_msk, j, pix_mask, raw_size);
        update_8pixels_sse2<psm, size, pageTable, null_second_line, 26>(src, basepage, i_msk, j, pix_mask, raw_size);
        update_8pixels_sse2<psm, size, pageTable, null_second_line, 28>(src, basepage, i_msk, j, pix_mask, raw_size);
        update_8pixels_sse2<psm, size, pageTable, null_second_line, 30>(src, basepage, i_msk, j, pix_mask, raw_size);
    }

    update_8pixels_sse2<psm, size, pageTable, null_second_line, 32>(src, basepage, i_msk, j, pix_mask, raw_size);
    update_8pixels_sse2<psm, size, pageTable, null_second_line, 34>(src, basepage, i_msk, j, pix_mask, raw_size);
    update_8pixels_sse2<psm, size, pageTable, null_second_line, 36>(src, basepage, i_msk, j, pix_mask, raw_size);
    update_8pixels_sse2<psm, size, pageTable, null_second_line, 38>(src, basepage, i_msk, j, pix_mask, raw_size);

    if(!PSMT_ISHALF(psm)) {
        update_8pixels_sse2<psm, size, pageTable, null_second_line, 40>(src, basepage, i_msk, j, pix_mask, raw_size);
        update_8pixels_sse2<psm, size, pageTable, null_second_line, 42>(src, basepage, i_msk, j, pix_mask, raw_size);
        update_8pixels_sse2<psm, size, pageTable, null_second_line, 44>(src, basepage, i_msk, j, pix_mask, raw_size);
        update_8pixels_sse2<psm, size, pageTable, null_second_line, 46>(src, basepage, i_msk, j, pix_mask, raw_size);
    }

    update_8pixels_sse2<psm, size, pageTable, null_second_line, 48>(src, basepage, i_msk, j, pix_mask, raw_size);
    update_8pixels_sse2<psm, size, pageTable, null_second_line, 50>(src, basepage, i_msk, j, pix_mask, raw_size);
    update_8pixels_sse2<psm, size, pageTable, null_second_line, 52>(src, basepage, i_msk, j, pix_mask, raw_size);
    update_8pixels_sse2<psm, size, pageTable, null_second_line, 54>(src, basepage, i_msk, j, pix_mask, raw_size);

    if(!PSMT_ISHALF(psm)) {
        update_8pixels_sse2<psm, size, pageTable, null_second_line, 56>(src, basepage, i_msk, j, pix_mask, raw_size);
        update_8pixels_sse2<psm, size, pageTable, null_second_line, 58>(src, basepage, i_msk, j, pix_mask, raw_size);
        update_8pixels_sse2<psm, size, pageTable, null_second_line, 60>(src, basepage, i_msk, j, pix_mask, raw_size);
        update_8pixels_sse2<psm, size, pageTable, null_second_line, 62>(src, basepage, i_msk, j, pix_mask, raw_size);
    }
}

template <u32 psm, u32 size, u32 pageTable[size][64]>
void Resolve_32_Bit_sse2(const void* psrc, int fbp, int fbw, int fbh, u32 fbm)
{
    // Note a basic implementation was done in Resolve_32_Bit function
#ifdef LOG_RESOLVE_PROFILE
    u32 startime = timeGetPreciseTime();
#endif
    u32 pix_mask;
    if (PSMT_ISHALF(psm)) /* 16 bit format */
    {
        /* Use 2 16bits mask */
        u32 pix16_mask = RGBA32to16(fbm);
        pix_mask = (pix16_mask<<16) | pix16_mask;
    }
    else
        pix_mask = fbm;

    // Note GS register: frame_register__fbp is specified in units of the 32 bits address divided by 2048
    // fbp is stored as 32*frame_register__fbp
    u32* pPageOffset = (u32*)g_pbyGSMemory + (fbp/32)*2048;

    int maxfbh;
    int memory_space = MEMORY_END-(fbp/32)*2048*4;
    if (PSMT_ISHALF(psm))
        maxfbh = memory_space / (2*fbw);
    else
        maxfbh = memory_space / (4*fbw);

    if( maxfbh > fbh ) maxfbh = fbh;
    
#ifdef LOG_RESOLVE_PROFILE
    ZZLog::Dev_Log("*** Resolve 32 to 32 bits: %dx%d. Frame Mask %x. Format %x", maxfbh, fbw, pix_mask, psm);
#endif

    // Start the src array at the end to reduce testing in loop
    // If maxfbh is odd, proces maxfbh -1 alone and then go back to maxfbh -3
    u32 raw_size = RH(Pitch(fbw))/sizeof(u32);
    u32* src;
    if (maxfbh&0x1) {
        ZZLog::Dev_Log("*** Warning resolve 32bits have an odd number of lines");

        // decrease maxfbh to process the bottom line (maxfbh-1)
        maxfbh--;

        src = (u32*)(psrc) + maxfbh*raw_size;
        u32 i_msk = maxfbh & (size-1);
        // Note fbw is a multiple of 64. So you can unroll the loop 64 times
        for(int j = (fbw - 64); j >= 0; j -= 64) {
            u32* basepage = pPageOffset + ((maxfbh/size) * (fbw/64) + (j/64)) * 2048;
            update_pixels_row_sse2<psm, size, pageTable, true>(src, basepage, i_msk, j, pix_mask, raw_size);
        }
        // realign the src pointer to process others lines
        src -= 2*raw_size;
    } else {
        // Because we process 2 lines at once go back to maxfbh-2.
        src = (u32*)(psrc) + (maxfbh-2)*raw_size;
    }

    // Note i must be even for the update_8pixels functions
    assert((maxfbh&0x1) == 0);
    for(int i = (maxfbh-2); i >= 0; i -= 2) {
        u32 i_msk = i & (size-1);
        // Note fbw is a multiple of 64. So you can unroll the loop 64 times
        for(int j = (fbw - 64); j >= 0; j -= 64) {
            u32* basepage = pPageOffset + ((i/size) * (fbw/64) + (j/64)) * 2048;
            update_pixels_row_sse2<psm, size, pageTable, false>(src, basepage, i_msk, j, pix_mask, raw_size);
        }

        // Note update_8pixels process 2 lines at onces hence the factor 2
        src -= 2*raw_size;
    }

    if(!pix_mask) {
        // Ensure that previous (out of order) write are done. It must be done after non temporal instruction
        // (or *_stream_* intrinsic)
        _mm_sfence();
    }

#ifdef LOG_RESOLVE_PROFILE
    ZZLog::Dev_Log("*** 32 bits: execution time %d", timeGetPreciseTime()-startime);
#endif
}
#endif

void _Resolve(const void* psrc, int fbp, int fbw, int fbh, int psm, u32 fbm, bool mode = true)
{
	FUNCLOG

	int start, end;

	s_nResolved += 2;

	// align the rect to the nearest page
	// note that fbp is always aligned on page boundaries
	GetRectMemAddressZero(start, end, psm, fbw, fbh, fbp, fbw);

    // Comment this to restore the previous resolve_32 version
#define OPTI_RESOLVE_32
    // start the conversion process A8R8G8B8 -> psm
    switch (psm)
    {

        // NOTE pass psm as a constant value otherwise gcc does not do its job. It keep
        // the psm switch in Resolve_32_Bit
        case PSMCT32:
        case PSMCT24:
#if defined(ZEROGS_SSE2) && defined(OPTI_RESOLVE_32)
            Resolve_32_Bit_sse2<PSMCT32, 32, g_pageTable32 >(psrc, fbp, fbw, fbh, fbm);
#else
            Resolve_32_Bit<u32, false >(psrc, fbp, fbw, fbh, PSMCT32, fbm);
#endif
            break;

        case PSMCT16:
#if defined(ZEROGS_SSE2) && defined(OPTI_RESOLVE_32)
            Resolve_32_Bit_sse2<PSMCT16, 64, g_pageTable16 >(psrc, fbp, fbw, fbh, fbm);
#else
            Resolve_32_Bit<u16, true >(psrc, fbp, fbw, fbh, PSMCT16, fbm);
#endif
            break;

        case PSMCT16S:
#if defined(ZEROGS_SSE2) && defined(OPTI_RESOLVE_32)
            Resolve_32_Bit_sse2<PSMCT16S, 64, g_pageTable16S >(psrc, fbp, fbw, fbh, fbm);
#else
            Resolve_32_Bit<u16, true >(psrc, fbp, fbw, fbh, PSMCT16S, fbm);
#endif
            break;

        case PSMT32Z:
        case PSMT24Z:
#if defined(ZEROGS_SSE2) && defined(OPTI_RESOLVE_32)
            Resolve_32_Bit_sse2<PSMT32Z, 32, g_pageTable32Z >(psrc, fbp, fbw, fbh, fbm);
#else
            Resolve_32_Bit<u32, false >(psrc, fbp, fbw, fbh, PSMT32Z, fbm);
#endif
            break;

        case PSMT16Z:
#if defined(ZEROGS_SSE2) && defined(OPTI_RESOLVE_32)
            Resolve_32_Bit_sse2<PSMT16Z, 64, g_pageTable16Z >(psrc, fbp, fbw, fbh, fbm);
#else
            Resolve_32_Bit<u16, false >(psrc, fbp, fbw, fbh, PSMT16Z, fbm);
#endif
            break;

        case PSMT16SZ:
#if defined(ZEROGS_SSE2) && defined(OPTI_RESOLVE_32)
            Resolve_32_Bit_sse2<PSMT16SZ, 64, g_pageTable16SZ >(psrc, fbp, fbw, fbh, fbm);
#else
            Resolve_32_Bit<u16, false >(psrc, fbp, fbw, fbh, PSMT16SZ, fbm);
#endif
            break;
    }

	g_MemTargs.ClearRange(start, end);

	INC_RESOLVE();
}

// Leaving this code in for reference for the moment.
#if 0
void _Resolve(const void* psrc, int fbp, int fbw, int fbh, int psm, u32 fbm, bool mode)
{
	FUNCLOG
	//GL_REPORT_ERRORD();
	s_nResolved += 2;

	// align the rect to the nearest page
	// note that fbp is always aligned on page boundaries
	int start, end;
	GetRectMemAddressZero(start, end, psm, fbw, fbh, fbp, fbw);

	int i, j;
	//short smask1 = gs.smask&1;
	//short smask2 = gs.smask&2;
	u32 mask, imask;

	if (PSMT_ISHALF(psm))   // 16 bit
	{
		// mask is shifted
		imask = RGBA32to16(fbm);
		mask = (~imask) & 0xffff;
	}
	else
	{
		mask = ~fbm;
		imask = fbm;

		if ((psm&0xf) > 0  && 0)
		{
			// preserve the alpha?
			mask &= 0x00ffffff;
			imask |= 0xff000000;
		}
	}

	// Targets over 2000 should be shuffle. FFX and KH2 (0x2100)
	int X = (psm == 0) ? 0 : 0;

//if (X == 1)
//ZZLog::Error_Log("resolve: %x %x %x %x (%x-%x).", psm, fbp, fbw, fbh, start, end);


#define RESOLVE_32BIT(psm, T, Tsrc, blockbits, blockwidth, blockheight, convfn, frame, aax, aay) \
	{ \
		Tsrc* src = (Tsrc*)(psrc); \
		T* pPageOffset = (T*)g_pbyGSMemory + fbp*(256/sizeof(T)), *dst; \
		int srcpitch = Pitch(fbw) * blockheight/sizeof(Tsrc); \
		int maxfbh = (MEMORY_END-fbp*256) / (sizeof(T) * fbw); \
		if( maxfbh > fbh ) maxfbh = fbh; \
		for(i = 0; i < (maxfbh&~(blockheight-1))*X; i += blockheight) { \
			/*if( smask2 && (i&1) == smask1 ) continue; */ \
			for(j = 0; j < fbw; j += blockwidth) { \
				/* have to write in the tiled format*/ \
				frame##SwizzleBlock##blockbits(pPageOffset + getPixelAddress##psm##_0(j, i, fbw), \
					src+RW(j), Pitch(fbw)/sizeof(Tsrc), mask); \
			} \
			src += RH(srcpitch); \
		} \
		for(; i < maxfbh; ++i) { \
			for(j = 0; j < fbw; ++j) { \
				T dsrc = convfn(src[RW(j)]); \
				dst = pPageOffset + getPixelAddress##psm##_0(j, i, fbw); \
				*dst = (dsrc & mask) | (*dst & imask); \
			} \
			src += RH(Pitch(fbw))/sizeof(Tsrc); \
		} \
	} \

	if( GetRenderFormat() == RFT_byte8 ) {
		// start the conversion process A8R8G8B8 -> psm
		switch (psm)
		{

			case PSMCT32:

			case PSMCT24:

				if (AA.y)
				{
					RESOLVE_32BIT(32, u32, u32, 32A4, 8, 8, (u32), Frame, AA.x, AA.y);
				}
				else if (AA.x)
				{
					RESOLVE_32BIT(32, u32, u32, 32A2, 8, 8, (u32), Frame, 1, 0);
				}
				else
				{
					RESOLVE_32BIT(32, u32, u32, 32, 8, 8, (u32), Frame, 0, 0);
				}

				break;

			case PSMCT16:

				if (AA.y)
				{
					RESOLVE_32BIT(16, u16, u32, 16A4, 16, 8, RGBA32to16, Frame, AA.x, AA.y);
				}
				else if (AA.x)
				{
					RESOLVE_32BIT(16, u16, u32, 16A2, 16, 8, RGBA32to16, Frame, 1, 0);
				}
				else
				{
					RESOLVE_32BIT(16, u16, u32, 16, 16, 8, RGBA32to16, Frame, 0, 0);
				}

				break;

			case PSMCT16S:

				if (AA.y)
				{
					RESOLVE_32BIT(16S, u16, u32, 16A4, 16, 8, RGBA32to16, Frame, AA.x, AA.y);
				}
				else if (AA.x)
				{
					RESOLVE_32BIT(16S, u16, u32, 16A2, 16, 8, RGBA32to16, Frame, 1, 0);
				}
				else
				{
					RESOLVE_32BIT(16S, u16, u32, 16, 16, 8, RGBA32to16, Frame, 0, 0);
				}

				break;

			case PSMT32Z:

			case PSMT24Z:

				if (AA.y)
				{
					RESOLVE_32BIT(32Z, u32, u32, 32A4, 8, 8, (u32), Frame, AA.x, AA.y);
				}
				else if (AA.x)
				{
					RESOLVE_32BIT(32Z, u32, u32, 32A2, 8, 8, (u32), Frame, 1, 0);
				}
				else
				{
					RESOLVE_32BIT(32Z, u32, u32, 32, 8, 8, (u32), Frame, 0, 0);
				}

				break;

			case PSMT16Z:

				if (AA.y)
				{
					RESOLVE_32BIT(16Z, u16, u32, 16A4, 16, 8, (u16), Frame, AA.x, AA.y);
				}
				else if (AA.x)
				{
					RESOLVE_32BIT(16Z, u16, u32, 16A2, 16, 8, (u16), Frame, 1, 0);
				}
				else
				{
					RESOLVE_32BIT(16Z, u16, u32, 16, 16, 8, (u16), Frame, 0, 0);
				}

				break;

			case PSMT16SZ:

				if (AA.y)
				{
					RESOLVE_32BIT(16SZ, u16, u32, 16A4, 16, 8, (u16), Frame, AA.x, AA.y);
				}
				else if (AA.x)
				{
					RESOLVE_32BIT(16SZ, u16, u32, 16A2, 16, 8, (u16), Frame, 1, 0);
				}
				else
				{
					RESOLVE_32BIT(16SZ, u16, u32, 16, 16, 8, (u16), Frame, 0, 0);
				}

				break;
		}
	}
	else   // float16
	{
		switch (psm)
		{

			case PSMCT32:

			case PSMCT24:

				if (AA.y)
				{
					RESOLVE_32BIT(32, u32, Vector_16F, 32A4, 8, 8, Float16ToARGB, Frame16, 1, 1);
				}
				else if (AA.x)
				{
					RESOLVE_32BIT(32, u32, Vector_16F, 32A2, 8, 8, Float16ToARGB, Frame16, 1, 0);
				}
				else
				{
					RESOLVE_32BIT(32, u32, Vector_16F, 32, 8, 8, Float16ToARGB, Frame16, 0, 0);
				}

				break;

			case PSMCT16:

				if (AA.y)
				{
					RESOLVE_32BIT(16, u16, Vector_16F, 16A4, 16, 8, Float16ToARGB16, Frame16, 1, 1);
				}
				else if (AA.x)
				{
					RESOLVE_32BIT(16, u16, Vector_16F, 16A2, 16, 8, Float16ToARGB16, Frame16, 1, 0);
				}
				else
				{
					RESOLVE_32BIT(16, u16, Vector_16F, 16, 16, 8, Float16ToARGB16, Frame16, 0, 0);
				}

				break;

			case PSMCT16S:

				if (AA.y)
				{
					RESOLVE_32BIT(16S, u16, Vector_16F, 16A4, 16, 8, Float16ToARGB16, Frame16, 1, 1);
				}
				else if (AA.x)
				{
					RESOLVE_32BIT(16S, u16, Vector_16F, 16A2, 16, 8, Float16ToARGB16, Frame16, 1, 0);
				}
				else
				{
					RESOLVE_32BIT(16S, u16, Vector_16F, 16, 16, 8, Float16ToARGB16, Frame16, 0, 0);
				}

				break;

			case PSMT32Z:

			case PSMT24Z:

				if (AA.y)
				{
					RESOLVE_32BIT(32Z, u32, Vector_16F, 32ZA4, 8, 8, Float16ToARGB_Z, Frame16, 1, 1);
				}
				else if (AA.x)
				{
					RESOLVE_32BIT(32Z, u32, Vector_16F, 32ZA2, 8, 8, Float16ToARGB_Z, Frame16, 1, 0);
				}
				else
				{
					RESOLVE_32BIT(32Z, u32, Vector_16F, 32Z, 8, 8, Float16ToARGB_Z, Frame16, 0, 0);
				}

				break;

			case PSMT16Z:

				if (AA.y)
				{
					RESOLVE_32BIT(16Z, u16, Vector_16F, 16ZA4, 16, 8, Float16ToARGB16_Z, Frame16, 1, 1);
				}
				else if (AA.x)
				{
					RESOLVE_32BIT(16Z, u16, Vector_16F, 16ZA2, 16, 8, Float16ToARGB16_Z, Frame16, 1, 0);
				}
				else
				{
					RESOLVE_32BIT(16Z, u16, Vector_16F, 16Z, 16, 8, Float16ToARGB16_Z, Frame16, 0, 0);
				}

				break;

			case PSMT16SZ:

				if (AA.y)
				{
					RESOLVE_32BIT(16SZ, u16, Vector_16F, 16ZA4, 16, 8, Float16ToARGB16_Z, Frame16, 1, 1);
				}
				else if (AA.x)
				{
					RESOLVE_32BIT(16SZ, u16, Vector_16F, 16ZA2, 16, 8, Float16ToARGB16_Z, Frame16, 1, 0);
				}
				else
				{
					RESOLVE_32BIT(16SZ, u16, Vector_16F, 16Z, 16, 8, Float16ToARGB16_Z, Frame16, 0, 0);
				}

				break;
		}
	}

	g_MemTargs.ClearRange(start, end);

	INC_RESOLVE();
}

#endif
