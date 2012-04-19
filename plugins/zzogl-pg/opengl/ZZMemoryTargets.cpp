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
#include "targets.h"
#include "ZZClut.h"

#ifdef ZEROGS_SSE2
#include <immintrin.h>
#endif

extern int g_TransferredToGPU;

extern int VALIDATE_THRESH;
extern u32 TEXDESTROY_THRESH;
#define FORCE_TEXDESTROY_THRESH (3) // destroy texture after FORCE_TEXDESTROY_THRESH frames

void CMemoryTargetMngr::Destroy()
{
	FUNCLOG
	listTargets.clear();
	listClearedTargets.clear();
}

bool CMemoryTarget::ValidateTex(const tex0Info& tex0, int starttex, int endtex, bool bDeleteBadTex)
{
	FUNCLOG

	if (clearmaxy == 0) return true;

	int checkstarty = max(starttex, clearminy);
	int checkendy = min(endtex, clearmaxy);

	if (checkstarty >= checkendy) return true;

	if (validatecount++ > VALIDATE_THRESH)
	{
		height = 0;
		return false;
	}

	// lock and compare
	assert(ptex != NULL && ptex->memptr != NULL);

	int result = memcmp_mmx(ptex->memptr + MemorySize(checkstarty-realy), MemoryAddress(checkstarty), MemorySize(checkendy-checkstarty));
	
	if (result == 0)
	{
		clearmaxy = 0;
		return true;
	}

	if (!bDeleteBadTex) return false;

	// delete clearminy, clearmaxy range (not the checkstarty, checkendy range)
	//int newstarty = 0;
	if (clearminy <= starty)
	{
		if (clearmaxy < starty + height)
		{
			// preserve end
			height = starty + height - clearmaxy;
			starty = clearmaxy;
			assert(height > 0);
		}
		else
		{
			// destroy
			height = 0;
		}
	}
	else
	{
		// beginning can be preserved
		height = clearminy - starty;
	}

	clearmaxy = 0;

	assert((starty >= realy) && ((starty + height) <= (realy + realheight)));

	return false;
}

#define TARGET_THRESH 0x500

extern int g_MaxTexWidth, g_MaxTexHeight; // Maximum height & width of supported texture.

//#define SORT_TARGETS
inline list<CMemoryTarget>::iterator CMemoryTargetMngr::DestroyTargetIter(list<CMemoryTarget>::iterator& it)
{
	// find the target and destroy
	list<CMemoryTarget>::iterator itprev = it;
	++it;
	listClearedTargets.splice(listClearedTargets.end(), listTargets, itprev);

	if (listClearedTargets.size() > TEXDESTROY_THRESH)
	{
		listClearedTargets.pop_front();
	}

	return it;
}

// Compare target to current texture info
// Not same format -> 1
// Same format, not same data (clut only) -> 2
// identical -> 0
int CMemoryTargetMngr::CompareTarget(list<CMemoryTarget>::iterator& it, const tex0Info& tex0, int clutsize)
{
	if (PSMT_ISCLUT(it->psm) != PSMT_ISCLUT(tex0.psm))
		return 1;

	if (PSMT_ISCLUT(tex0.psm)) {
		if (it->psm != tex0.psm || it->cpsm != tex0.cpsm || it->clutsize != clutsize)
			return 1;

		if	(PSMT_IS32BIT(tex0.cpsm)) {
			if (Cmp_ClutBuffer_SavedClut<u32>((u32*)&it->clut[0], tex0.csa, clutsize))
				return 2;
		} else {
			if (Cmp_ClutBuffer_SavedClut<u16>((u16*)&it->clut[0], tex0.csa, clutsize))
				return 2;
		}

	} else {
		if (PSMT_IS16BIT(tex0.psm) != PSMT_IS16BIT(it->psm))
			return 1;
    }

	return 0;
}

void CMemoryTargetMngr::GetClutVariables(int& clutsize, const tex0Info& tex0)
{
	clutsize = 0;

	if (PSMT_ISCLUT(tex0.psm))
	{
		int entries = PSMT_IS8CLUT(tex0.psm) ? 256 : 16;

		if (PSMT_IS32BIT(tex0.cpsm))
			clutsize = min(entries, 256 - tex0.csa * 16) * 4;
		else
			clutsize = min(entries, 512 - tex0.csa * 16) * 2;
	}
}

void CMemoryTargetMngr::GetMemAddress(int& start, int& end,  const tex0Info& tex0)
{
	int nbStart, nbEnd;
	GetRectMemAddressZero(nbStart, nbEnd, tex0.psm, tex0.tw, tex0.th, tex0.tbp0, tex0.tbw);
	assert(nbStart < nbEnd);
	nbEnd = min(nbEnd, MEMORY_END);

	start = nbStart / (4 * GPU_TEXWIDTH);
	end = (nbEnd + GPU_TEXWIDTH * 4 - 1) / (4 * GPU_TEXWIDTH);
	assert(start < end);

}

CMemoryTarget* CMemoryTargetMngr::SearchExistTarget(int start, int end, int clutsize, const tex0Info& tex0, int forcevalidate)
{
	for (list<CMemoryTarget>::iterator it = listTargets.begin(); it != listTargets.end();)
	{

		if (it->starty <= start && it->starty + it->height >= end)
		{

			int res = CompareTarget(it, tex0, clutsize);

			if (res == 1)
			{
				if (it->validatecount++ > VALIDATE_THRESH)
				{
					it = DestroyTargetIter(it);

					if (listTargets.size() == 0) break;
				}
				else
					++it;

				continue;
			}
			else if (res == 2)
			{
				++it;
				continue;
			}

			if (forcevalidate)   //&& listTargets.size() < TARGET_THRESH ) {
			{
				// do more validation checking. delete if not been used for a while

				if (!it->ValidateTex(tex0, start, end, curstamp > it->usedstamp + FORCE_TEXDESTROY_THRESH))
				{

					if (it->height <= 0)
					{
						it = DestroyTargetIter(it);

						if (listTargets.size() == 0) break;
					}
					else
						++it;

					continue;
				}
			}

			it->usedstamp = curstamp;

			it->validatecount = 0;

			return &(*it);
		}

#ifdef SORT_TARGETS
		else if (it->starty >= end) break;

#endif

		++it;
	}

	return NULL;
}

CMemoryTarget* CMemoryTargetMngr::ClearedTargetsSearch(int fmt, int widthmult, int channels, int height)
{
	CMemoryTarget* targ = NULL;

	if (listClearedTargets.size() > 0)
	{
		list<CMemoryTarget>::iterator itbest = listClearedTargets.begin();

		while (itbest != listClearedTargets.end())
		{
			if ((height == itbest->realheight) && (itbest->fmt == fmt) && (itbest->widthmult == widthmult) && (itbest->channels == channels))
			{
				// check channels
				if (PIXELS_PER_WORD(itbest->psm) == channels) break;
			}

			++itbest;
		}

		if (itbest != listClearedTargets.end())
		{
			listTargets.splice(listTargets.end(), listClearedTargets, itbest);
			targ = &listTargets.back();
			targ->validatecount = 0;
		}
		else
		{
			// create a new
			listTargets.push_back(CMemoryTarget());
			targ = &listTargets.back();
		}
	}
	else
	{
		listTargets.push_back(CMemoryTarget());
		targ = &listTargets.back();
	}

	return targ;
}

CMemoryTarget* CMemoryTargetMngr::GetMemoryTarget(const tex0Info& tex0, int forcevalidate)
{
	FUNCLOG
	int start, end, clutsize;

	GetClutVariables(clutsize, tex0);
	GetMemAddress(start, end, tex0);

	CMemoryTarget* it = SearchExistTarget(start, end, clutsize, tex0, forcevalidate);

	if (it != NULL) return it;

	// couldn't find so create
	CMemoryTarget* targ;

	u32 fmt;
    u32 internal_fmt;
	if (PSMT_ISHALF_STORAGE(tex0)) {
        // RGBA_5551 storage format
        fmt = GL_UNSIGNED_SHORT_1_5_5_5_REV;
        internal_fmt = GL_RGB5_A1;
    } else {
        // RGBA_8888 storage format
        fmt = GL_UNSIGNED_BYTE;
        internal_fmt = GL_RGBA;
    }

	int widthmult = 1, channels = 1;

	// If our texture is too big and could not be placed in 1 GPU texture. Pretty rare in modern cards.
	if ((g_MaxTexHeight < 4096) && (end - start > g_MaxTexHeight)) 
	{
		// In this rare case we made a texture of half height and place it on the screen.
		ZZLog::Debug_Log("Making a half height texture (start - end == 0x%x)", (end-start));
		widthmult = 2;
	}
	
	channels = PIXELS_PER_WORD(tex0.psm);

	targ = ClearedTargetsSearch(fmt, widthmult, channels, end - start);

	if (targ->ptex != NULL)
	{
		assert(end - start <= targ->realheight && targ->fmt == fmt && targ->widthmult == widthmult);

		// good enough, so init
		targ->realy = targ->starty = start;
		targ->usedstamp = curstamp;
		targ->psm = tex0.psm;
		targ->cpsm = tex0.cpsm;
		targ->height = end - start;
	} else {
		// not initialized yet
		targ->fmt = fmt;
		targ->realy = targ->starty = start;
		targ->realheight = targ->height = end - start;
		targ->usedstamp = curstamp;
		targ->psm = tex0.psm;
		targ->cpsm = tex0.cpsm;
		targ->widthmult = widthmult;
		targ->channels = channels;
		targ->texH = (targ->realheight + widthmult - 1)/widthmult;
		targ->texW = GPU_TEXWIDTH *  widthmult * channels;

		// alloc the mem
		targ->ptex = new CMemoryTarget::TEXTURE();
		targ->ptex->ref = 1;
	}

#if defined(ZEROGS_DEVBUILD)
	g_TransferredToGPU += MemorySize(channels * targ->height);
#endif

	// fill with data
	if (targ->ptex->memptr == NULL)
	{
		targ->ptex->memptr = (u8*)_aligned_malloc(MemorySize(targ->realheight), 16);
		assert(targ->ptex->ref > 0);
	}

	memcpy_amd(targ->ptex->memptr, MemoryAddress(targ->realy), MemorySize(targ->height));

	__aligned16 u8* ptexdata = NULL;
	bool has_data = false;

	if (PSMT_ISCLUT(tex0.psm))
	{
		assert(clutsize > 0);

        // Local clut parameter
		targ->cpsm = tex0.cpsm;

        // Allocate a local clut array
        targ->clutsize = clutsize;
        if(targ->clut == NULL)
            targ->clut = (u8*)_aligned_malloc(clutsize, 16);
        else {
            // In case it could occured
            // realloc would be better but you need to get it from libutilies first
            // _aligned_realloc is brought in from ScopedAlloc.h now. --arcum42
            _aligned_free(targ->clut);
            targ->clut = (u8*)_aligned_malloc(clutsize, 16);
        }

        // texture parameter
		ptexdata = (u8*)_aligned_malloc(CLUT_PIXEL_SIZE(tex0.cpsm) * targ->texH * targ->texW, 16);
		has_data = true;

		u8* psrc = (u8*)(MemoryAddress(targ->realy));

        // Fill a local clut then build the real texture
		if (PSMT_IS32BIT(tex0.cpsm))
		{
            ClutBuffer_to_Array<u32>((u32*)targ->clut, tex0.csa, clutsize);
			Build_Clut_Texture<u32>(tex0.psm, targ->height, (u32*)targ->clut, psrc, (u32*)ptexdata);
		}
		else
		{
            ClutBuffer_to_Array<u16>((u16*)targ->clut, tex0.csa, clutsize);
			Build_Clut_Texture<u16>(tex0.psm, targ->height, (u16*)targ->clut, psrc, (u16*)ptexdata);
		}

        assert(targ->clutsize > 0);
	}
	else if (tex0.psm == PSMT16Z || tex0.psm == PSMT16SZ)
    {
        ptexdata = (u8*)_aligned_malloc(4 * targ->texH * targ->texW, 16);
        has_data = true;

        // needs to be 8 bit, use xmm for unpacking
        u16* dst = (u16*)ptexdata;
        u16* src = (u16*)(MemoryAddress(targ->realy));

#ifdef ZEROGS_SSE2
        assert(((u32)(uptr)dst) % 16 == 0);

        __m128i zero_128 = _mm_setzero_si128();
        // NOTE: future performance improvement
        // SSE4.1 support uncacheable load 128bits. Maybe it can
        // avoid some cache pollution
        // NOTE2: I create multiple _n variable to mimic the previous ASM behavior
        // but I'm not sure there are real gains.
        for (int i = targ->height * GPU_TEXWIDTH/16 ; i > 0 ; --i)
        {
            // Convert 16 bits pixels to 32bits (zero extended)
            // Batch 64 bytes (32 pixels) at once.
            __m128i pixels_1 = _mm_load_si128((__m128i*)src);
            __m128i pixels_2 = _mm_load_si128((__m128i*)(src+8));
            __m128i pixels_3 = _mm_load_si128((__m128i*)(src+16));
            __m128i pixels_4 = _mm_load_si128((__m128i*)(src+24));

            __m128i pix_low_1 = _mm_unpacklo_epi16(pixels_1, zero_128);
            __m128i pix_high_1 = _mm_unpackhi_epi16(pixels_1, zero_128);
            __m128i pix_low_2 = _mm_unpacklo_epi16(pixels_2, zero_128);
            __m128i pix_high_2 = _mm_unpackhi_epi16(pixels_2, zero_128);

            // Note: bypass cache
            _mm_stream_si128((__m128i*)dst, pix_low_1);
            _mm_stream_si128((__m128i*)(dst+8), pix_high_1);
            _mm_stream_si128((__m128i*)(dst+16), pix_low_2);
            _mm_stream_si128((__m128i*)(dst+24), pix_high_2);

            __m128i pix_low_3 = _mm_unpacklo_epi16(pixels_3, zero_128);
            __m128i pix_high_3 = _mm_unpackhi_epi16(pixels_3, zero_128);
            __m128i pix_low_4 = _mm_unpacklo_epi16(pixels_4, zero_128);
            __m128i pix_high_4 = _mm_unpackhi_epi16(pixels_4, zero_128);

            // Note: bypass cache
            _mm_stream_si128((__m128i*)(dst+32), pix_low_3);
            _mm_stream_si128((__m128i*)(dst+40), pix_high_3);
            _mm_stream_si128((__m128i*)(dst+48), pix_low_4);
            _mm_stream_si128((__m128i*)(dst+56), pix_high_4);

            src += 32;
            dst += 64;
        }
        // It is advise to use a fence instruction after non temporal move (mm_stream) instruction...
        // store fence insures that previous store are finish before execute new one.
        _mm_sfence();
#else // ZEROGS_SSE2

        for (int i = 0; i < targ->height; ++i)
        {
            for (int j = 0; j < GPU_TEXWIDTH; ++j)
            {
                dst[0] = src[0];
                dst[1] = 0;
                dst[2] = src[1];
                dst[3] = 0;
                dst += 4;
                src += 2;
            }
        }

#endif // ZEROGS_SSE2
    }
    else
    {
        ptexdata = targ->ptex->memptr;
        // We really don't want to deallocate memptr. As a reminder...
        has_data = false;
    }

	// create the texture
	GL_REPORT_ERRORD();

	assert(ptexdata != NULL);

	if (targ->ptex->tex == 0) glGenTextures(1, &targ->ptex->tex);

	glBindTexture(GL_TEXTURE_RECTANGLE_NV, targ->ptex->tex);

    TextureRect(internal_fmt, targ->texW, targ->texH, GL_RGBA, fmt, ptexdata);

	while (glGetError() != GL_NO_ERROR)
	{
		// release resources until can create
		if (listClearedTargets.size() > 0)
		{
			listClearedTargets.pop_front();
		}
		else
		{
			if (listTargets.size() == 0)
			{
				ZZLog::Error_Log("Failed to create %dx%x texture.", targ->texW, targ->texH);
				channels = 1;
				if (has_data) _aligned_free(ptexdata);
				return NULL;
			}

			DestroyOldest();
		}

        TextureRect(internal_fmt, targ->texW, targ->texH, GL_RGBA, fmt, ptexdata);
	}

	setRectWrap(GL_CLAMP);
	if (has_data) _aligned_free(ptexdata);

	assert(tex0.psm != 0xd);

	return targ;
}

void CMemoryTargetMngr::ClearRange(int nbStartY, int nbEndY)
{
	FUNCLOG
	int starty = nbStartY / (4 * GPU_TEXWIDTH);
	int endy = (nbEndY + 4 * GPU_TEXWIDTH - 1) / (4 * GPU_TEXWIDTH);

	for (list<CMemoryTarget>::iterator it = listTargets.begin(); it != listTargets.end();)
	{

		if (it->starty < endy && (it->starty + it->height) > starty)
		{

			// intersects, reduce valid texture mem (or totally delete texture)
			// there are 4 cases
			int miny = max(it->starty, starty);
			int maxy = min(it->starty + it->height, endy);
			assert(miny < maxy);

			if (it->clearmaxy == 0)
			{
				it->clearminy = miny;
				it->clearmaxy = maxy;
			}
			else
			{
				if (it->clearminy > miny) it->clearminy = miny;
				if (it->clearmaxy < maxy) it->clearmaxy = maxy;
			}
		}

		++it;
	}
}

void CMemoryTargetMngr::DestroyCleared()
{
	FUNCLOG

	for (list<CMemoryTarget>::iterator it = listClearedTargets.begin(); it != listClearedTargets.end();)
	{
		if (it->usedstamp < curstamp - (FORCE_TEXDESTROY_THRESH -1))
		{
			it = listClearedTargets.erase(it);
			continue;
		}

		++it;
	}

	if ((curstamp % FORCE_TEXDESTROY_THRESH) == 0)
	{
		// purge old targets every FORCE_TEXDESTROY_THRESH frames
		for (list<CMemoryTarget>::iterator it = listTargets.begin(); it != listTargets.end();)
		{
			if (it->usedstamp < curstamp - FORCE_TEXDESTROY_THRESH)
			{
				it = listTargets.erase(it);
				continue;
			}

			++it;
		}
	}

	++curstamp;
}

void CMemoryTargetMngr::DestroyOldest()
{
	FUNCLOG

	if (listTargets.size() == 0)
		return;

	list<CMemoryTarget>::iterator it, itbest;

	it = itbest = listTargets.begin();

	while (it != listTargets.end())
	{
		if (it->usedstamp < itbest->usedstamp) itbest = it;
		++it;
	}

	listTargets.erase(itbest);
}
