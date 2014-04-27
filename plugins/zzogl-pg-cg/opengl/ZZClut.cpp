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
#include "Util.h"

#if defined(ZEROGS_SSE2)
#include <emmintrin.h>
#endif

// Local Clut buffer:
// It supports both 32 bits and 16 bits colors formats. The size of the buffer is 1KBytes.
// The 16 bits entries are arranged in 2 columns. One row is a 32 bits colors.
// 256  0
// 271  1
// ...  ..
// 510  254
// 511  255
//
// CSA -> clut buffer offset:
// 16 bits format: CSA < 32 <=> 16 entries, 16 half-row of the buffer (for example 0 to 15)
// 32 bits format: CSA < 16 <=> 16 entries, 16 full row of the buffer (for example 256|0 to 271|15)

static const __aligned16 int s_clut_16bits_mask[4] = { 0x0000ffff, 0x0000ffff, 0x0000ffff, 0x0000ffff };

template <class T>
__forceinline T* GetClutBufferAddress(u32 csa) { }

template <>
__forceinline u32* GetClutBufferAddress<u32>(u32 csa)
{
    return (u32*)(g_pbyGSClut + 64 * (csa & 15));
}

template <>
__forceinline u16* GetClutBufferAddress<u16>(u32 csa)
{
    return (u16*)(g_pbyGSClut + 64 * (csa & 15) + (csa >= 16 ? 2 : 0));
}

/* *****************************************************************
 * Local memory -> Clut buffer
 * *****************************************************************/

#ifdef ZEROGS_SSE2
__forceinline void GSMem_to_ClutBuffer__T32_I8_CSM1_sse2(u32* vm, u32 csa)
{
    u32* clut = GetClutBufferAddress<u32>(csa);

	__m128i* src = (__m128i*)vm;
	__m128i* dst = (__m128i*)clut;

	for (int j = 0; j < 64; j += 32, src += 32, dst += 32)
	{
		for (int i = 0; i < 16; i += 4)
		{
			__m128i r0 = _mm_load_si128(&src[i+0]);
			__m128i r1 = _mm_load_si128(&src[i+1]);
			__m128i r2 = _mm_load_si128(&src[i+2]);
			__m128i r3 = _mm_load_si128(&src[i+3]);

			_mm_store_si128(&dst[i*2+0], _mm_unpacklo_epi64(r0, r1));
			_mm_store_si128(&dst[i*2+1], _mm_unpacklo_epi64(r2, r3));
			_mm_store_si128(&dst[i*2+2], _mm_unpackhi_epi64(r0, r1));
			_mm_store_si128(&dst[i*2+3], _mm_unpackhi_epi64(r2, r3));

			__m128i r4 = _mm_load_si128(&src[i+0+16]);
			__m128i r5 = _mm_load_si128(&src[i+1+16]);
			__m128i r6 = _mm_load_si128(&src[i+2+16]);
			__m128i r7 = _mm_load_si128(&src[i+3+16]);

			_mm_store_si128(&dst[i*2+4], _mm_unpacklo_epi64(r4, r5));
			_mm_store_si128(&dst[i*2+5], _mm_unpacklo_epi64(r6, r7));
			_mm_store_si128(&dst[i*2+6], _mm_unpackhi_epi64(r4, r5));
			_mm_store_si128(&dst[i*2+7], _mm_unpackhi_epi64(r6, r7));
		}
	}
}

__forceinline void GSMem_to_ClutBuffer__T32_I4_CSM1_sse2(u32* vm, u32 csa)
{
    u32* clut = GetClutBufferAddress<u32>(csa);

	__m128i* src = (__m128i*)vm;
	__m128i* dst = (__m128i*)clut;

	__m128i r0 = _mm_load_si128(&src[0]);
	__m128i r1 = _mm_load_si128(&src[1]);
	__m128i r2 = _mm_load_si128(&src[2]);
	__m128i r3 = _mm_load_si128(&src[3]);

	_mm_store_si128(&dst[0], _mm_unpacklo_epi64(r0, r1));
	_mm_store_si128(&dst[1], _mm_unpacklo_epi64(r2, r3));
	_mm_store_si128(&dst[2], _mm_unpackhi_epi64(r0, r1));
	_mm_store_si128(&dst[3], _mm_unpackhi_epi64(r2, r3));
}


template<bool CSA_0_15, bool HIGH_16BITS_VM>
__forceinline void GSMem_to_ClutBuffer__T16_I4_CSM1_core_sse2(u32* vm, u32* clut)
{
    __m128i vm_0;
    __m128i vm_1;
    __m128i vm_2;
    __m128i vm_3;
    __m128i clut_0;
    __m128i clut_1;
    __m128i clut_2;
    __m128i clut_3;

    __m128i clut_mask = _mm_load_si128((__m128i*)s_clut_16bits_mask);

    // !HIGH_16BITS_VM
    // CSA in 0-15
    // Replace lower 16 bits of clut with lower 16 bits of vm
    // CSA in 16-31
    // Replace higher 16 bits of clut with lower 16 bits of vm

    // HIGH_16BITS_VM
    // CSA in 0-15
    // Replace lower 16 bits of clut with higher 16 bits of vm
    // CSA in 16-31
    // Replace higher 16 bits of clut with higher 16 bits of vm
    if(HIGH_16BITS_VM && CSA_0_15) {
        // move up to low
        vm_0 = _mm_load_si128((__m128i*)vm); // 9 8 1 0
        vm_1 = _mm_load_si128((__m128i*)vm+1); // 11 10 3 2
        vm_2 = _mm_load_si128((__m128i*)vm+2); // 13 12 5 4
        vm_3 = _mm_load_si128((__m128i*)vm+3); // 15 14 7 6
        vm_0 = _mm_srli_epi32(vm_0, 16);
        vm_1 = _mm_srli_epi32(vm_1, 16);
        vm_2 = _mm_srli_epi32(vm_2, 16);
        vm_3 = _mm_srli_epi32(vm_3, 16);
    } else if(HIGH_16BITS_VM && !CSA_0_15) {
        // Remove lower 16 bits
        vm_0 = _mm_andnot_si128(clut_mask, _mm_load_si128((__m128i*)vm)); // 9 8 1 0
        vm_1 = _mm_andnot_si128(clut_mask, _mm_load_si128((__m128i*)vm+1)); // 11 10 3 2
        vm_2 = _mm_andnot_si128(clut_mask, _mm_load_si128((__m128i*)vm+2)); // 13 12 5 4
        vm_3 = _mm_andnot_si128(clut_mask, _mm_load_si128((__m128i*)vm+3)); // 15 14 7 6
    } else if(!HIGH_16BITS_VM && CSA_0_15) {
        // Remove higher 16 bits
        vm_0 = _mm_and_si128(clut_mask, _mm_load_si128((__m128i*)vm)); // 9 8 1 0
        vm_1 = _mm_and_si128(clut_mask, _mm_load_si128((__m128i*)vm+1)); // 11 10 3 2
        vm_2 = _mm_and_si128(clut_mask, _mm_load_si128((__m128i*)vm+2)); // 13 12 5 4
        vm_3 = _mm_and_si128(clut_mask, _mm_load_si128((__m128i*)vm+3)); // 15 14 7 6
    } else if(!HIGH_16BITS_VM && !CSA_0_15) {
        // move low to high
        vm_0 = _mm_load_si128((__m128i*)vm); // 9 8 1 0
        vm_1 = _mm_load_si128((__m128i*)vm+1); // 11 10 3 2
        vm_2 = _mm_load_si128((__m128i*)vm+2); // 13 12 5 4
        vm_3 = _mm_load_si128((__m128i*)vm+3); // 15 14 7 6
        vm_0 = _mm_slli_epi32(vm_0, 16);
        vm_1 = _mm_slli_epi32(vm_1, 16);
        vm_2 = _mm_slli_epi32(vm_2, 16);
        vm_3 = _mm_slli_epi32(vm_3, 16);
    }

    // Unsizzle the data
    __m128i row_0 = _mm_unpacklo_epi64(vm_0, vm_1); // 3 2 1 0
    __m128i row_1 = _mm_unpacklo_epi64(vm_2, vm_3); // 7 6 5 4
    __m128i row_2 = _mm_unpackhi_epi64(vm_0, vm_1); // 11 10 9 8
    __m128i row_3 = _mm_unpackhi_epi64(vm_2, vm_3); // 15 14 13 12

    // load old data & remove useless part
    if(CSA_0_15) {
        // Remove lower 16 bits
        clut_0 = _mm_andnot_si128(clut_mask, _mm_load_si128((__m128i*)clut));
        clut_1 = _mm_andnot_si128(clut_mask, _mm_load_si128((__m128i*)clut+1));
        clut_2 = _mm_andnot_si128(clut_mask, _mm_load_si128((__m128i*)clut+2));
        clut_3 = _mm_andnot_si128(clut_mask, _mm_load_si128((__m128i*)clut+3));
    } else {
        // Remove higher 16 bits
        clut_0 = _mm_and_si128(clut_mask, _mm_load_si128((__m128i*)clut));
        clut_1 = _mm_and_si128(clut_mask, _mm_load_si128((__m128i*)clut+1));
        clut_2 = _mm_and_si128(clut_mask, _mm_load_si128((__m128i*)clut+2));
        clut_3 = _mm_and_si128(clut_mask, _mm_load_si128((__m128i*)clut+3));
    }

    // Merge old & new data
    clut_0 = _mm_or_si128(clut_0, row_0);
    clut_1 = _mm_or_si128(clut_1, row_1);
    clut_2 = _mm_or_si128(clut_2, row_2);
    clut_3 = _mm_or_si128(clut_3, row_3);

    _mm_store_si128((__m128i*)clut, clut_0);
    _mm_store_si128((__m128i*)clut+1, clut_1);
    _mm_store_si128((__m128i*)clut+2, clut_2);
    _mm_store_si128((__m128i*)clut+3, clut_3);
}

__forceinline void GSMem_to_ClutBuffer__T16_I4_CSM1_sse2(u32* vm, u32 csa)
{
    u32* clut = GetClutBufferAddress<u32>(csa); // Keep aligned version for sse2

    if (csa > 15) {
        GSMem_to_ClutBuffer__T16_I4_CSM1_core_sse2<false, false>(vm, clut);
    } else {
        GSMem_to_ClutBuffer__T16_I4_CSM1_core_sse2<true, false>(vm, clut);
    }
}

__forceinline void GSMem_to_ClutBuffer__T16_I8_CSM1_sse2(u32* vm, u32 csa)
{
    // update the right clut column (csa < 16)
    u32* clut = GetClutBufferAddress<u32>(csa); // Keep aligned version for sse2

    u32 csa_right = (csa < 16) ? 16 - csa : 0;

    for(int i = (csa_right/2); i > 0 ; --i) {
        GSMem_to_ClutBuffer__T16_I4_CSM1_core_sse2<true,false>(vm, clut);
        clut += 16;
        GSMem_to_ClutBuffer__T16_I4_CSM1_core_sse2<true,true>(vm, clut);
        clut += 16;
        vm += 16; // go down one column
    }

    // update the left clut column
    u32 csa_left = (csa >= 16) ? 16 : csa;

    // In case csa_right is odd (so csa_left is also odd), we cross the clut column
    if(csa_right & 0x1) {
        GSMem_to_ClutBuffer__T16_I4_CSM1_core_sse2<true,false>(vm, clut);
        // go back to the base before processing left clut column
        clut = GetClutBufferAddress<u32>(0); // Keep aligned version for sse2

        GSMem_to_ClutBuffer__T16_I4_CSM1_core_sse2<false,true>(vm, clut);
        clut += 16;
        vm += 16; // go down one column
    } else if(csa_right != 0) {
        // go back to the base before processing left clut column
        clut = GetClutBufferAddress<u32>(0); // Keep aligned version for sse2

    }

    for(int i = (csa_left/2); i > 0 ; --i) {
        GSMem_to_ClutBuffer__T16_I4_CSM1_core_sse2<false,false>(vm, clut);
        clut += 16;
        GSMem_to_ClutBuffer__T16_I4_CSM1_core_sse2<false,true>(vm, clut);
        clut += 16;
        vm += 16; // go down one column
    }
}

#endif // ZEROGS_SSE2

__forceinline void GSMem_to_ClutBuffer__T16_I8_CSM1_c(u32* _vm, u32 csa)
{
	const static u32 map[] =
	{
		0, 2, 8, 10, 16, 18, 24, 26,
		4, 6, 12, 14, 20, 22, 28, 30,
		1, 3, 9, 11, 17, 19, 25, 27,
		5, 7, 13, 15, 21, 23, 29, 31
	};

	u16* vm = (u16*)_vm;
	u16* clut = GetClutBufferAddress<u16>(csa);

	int left = ((u32)(uptr)clut & 2) ? 512 : 512 - (((u32)(uptr)clut) & 0x3ff) / 2;

	for (int j = 0; j < 8; j++, vm += 32, clut += 64, left -= 32)
	{
		if (left == 32)
		{
			assert(left == 32);

			for (int i = 0; i < 16; i++)
				clut[2*i] = vm[map[i]];

			clut = (u16*)((uptr)clut & ~0x3ff) + 1;

			for (int i = 16; i < 32; i++)
				clut[2*i] = vm[map[i]];
		}
		else
		{
			if (left == 0)
			{
				clut = (u16*)((uptr)clut & ~0x3ff) + 1;
				left = -1;
			}

			for (int i = 0; i < 32; i++)
				clut[2*i] = vm[map[i]];
		}
	}
}

__forceinline void GSMem_to_ClutBuffer__T32_I8_CSM1_c(u32* vm, u32 csa)
{
	u64* src = (u64*)vm;
	u64* dst = (u64*)GetClutBufferAddress<u32>(csa);

	for (int j = 0; j < 2; j++, src += 32)
	{
		for (int i = 0; i < 4; i++, dst += 16, src += 8)
		{
			dst[0] = src[0];
			dst[1] = src[2];
			dst[2] = src[4];
			dst[3] = src[6];
			dst[4] = src[1];
			dst[5] = src[3];
			dst[6] = src[5];
			dst[7] = src[7];

			dst[8] = src[32];
			dst[9] = src[32+2];
			dst[10] = src[32+4];
			dst[11] = src[32+6];
			dst[12] = src[32+1];
			dst[13] = src[32+3];
			dst[14] = src[32+5];
			dst[15] = src[32+7];
		}
	}
}

__forceinline void GSMem_to_ClutBuffer__T16_I4_CSM1_c(u32* _vm, u32 csa)
{
	u16* dst = GetClutBufferAddress<u16>(csa);
	u16* src = (u16*)_vm;

	dst[0] = src[0];
	dst[2] = src[2];
	dst[4] = src[8];
	dst[6] = src[10];
	dst[8] = src[16];
	dst[10] = src[18];
	dst[12] = src[24];
	dst[14] = src[26];
	dst[16] = src[4];
	dst[18] = src[6];
	dst[20] = src[12];
	dst[22] = src[14];
	dst[24] = src[20];
	dst[26] = src[22];
	dst[28] = src[28];
	dst[30] = src[30];
}

__forceinline void GSMem_to_ClutBuffer__T32_I4_CSM1_c(u32* vm, u32 csa)
{
	u64* src = (u64*)vm;
	u64* dst = (u64*)GetClutBufferAddress<u32>(csa);

	dst[0] = src[0];
	dst[1] = src[2];
	dst[2] = src[4];
	dst[3] = src[6];
	dst[4] = src[1];
	dst[5] = src[3];
	dst[6] = src[5];
	dst[7] = src[7];
}

// Main GSmem to Clutbuffer function
/*__forceinline*/ void GSMem_to_ClutBuffer(tex0Info &tex0)
{
	int entries = PSMT_IS8CLUT(tex0.psm) ? 256 : 16;

    u8* _src = g_pbyGSMemory + 256 * tex0.cbp;

	if (tex0.csm)
	{
		switch (tex0.cpsm)
		{
            // 16bit psm
            // eggomania uses non16bit textures for csm2

			case PSMCT16:
			{
				u16* src = (u16*)_src;
				u16 *dst = GetClutBufferAddress<u16>(tex0.csa);

				for (int i = 0; i < entries; ++i)
				{
					*dst = src[getPixelAddress16_0(gs.clut.cou+i, gs.clut.cov, gs.clut.cbw)];
					dst += 2;

					// check for wrapping
					if (((u32)dst & 0x3ff) == 0) dst = GetClutBufferAddress<u16>(16);
				}
				break;
			}

			case PSMCT16S:
			{
				u16* src = (u16*)_src;
				u16 *dst = GetClutBufferAddress<u16>(tex0.csa);

				for (int i = 0; i < entries; ++i)
				{
					*dst = src[getPixelAddress16S_0(gs.clut.cou+i, gs.clut.cov, gs.clut.cbw)];
					dst += 2;

					// check for wrapping
					if (((u32)dst & 0x3ff) == 0) dst = GetClutBufferAddress<u16>(16);
				}
				break;
			}

			case PSMCT32:
			case PSMCT24:
			{
				u32* src = (u32*)_src;
				u32 *dst = GetClutBufferAddress<u32>(tex0.csa);

				// check if address exceeds src

				if (src + getPixelAddress32_0(gs.clut.cou + entries - 1, gs.clut.cov, gs.clut.cbw) >= (u32*)g_pbyGSMemory + 0x00100000)
					ZZLog::Error_Log("texClutWrite out of bounds.");
				else
					for (int i = 0; i < entries; ++i)
					{
						*dst = src[getPixelAddress32_0(gs.clut.cou+i, gs.clut.cov, gs.clut.cbw)];
						dst++;
					}
				break;
			}

			default:
			{
				//ZZLog::Debug_Log("Unknown cpsm: %x (%x).", tex0.cpsm, tex0.psm);
				break;
			}
		}
	}
	else
	{
        u32* src = (u32*)_src;

		if (entries == 16)
		{
            if (tex0.cpsm < 2) {
#ifdef ZEROGS_SSE2
					GSMem_to_ClutBuffer__T32_I4_CSM1_sse2(src, tex0.csa);
#else
					GSMem_to_ClutBuffer__T32_I4_CSM1_c(src, tex0.csa);
#endif
            } else {
#ifdef ZEROGS_SSE2
					GSMem_to_ClutBuffer__T16_I4_CSM1_sse2(src, tex0.csa);
#else
					GSMem_to_ClutBuffer__T16_I4_CSM1_c(src, tex0.csa);
#endif
			}
		}
		else
		{
            if (tex0.cpsm < 2) {
#ifdef ZEROGS_SSE2
					GSMem_to_ClutBuffer__T32_I8_CSM1_sse2(src, tex0.csa);
#else
					GSMem_to_ClutBuffer__T32_I8_CSM1_c(src, tex0.csa);
#endif
            } else {
#ifdef ZEROGS_SSE2
					GSMem_to_ClutBuffer__T16_I8_CSM1_sse2(src, tex0.csa);
#else
					GSMem_to_ClutBuffer__T16_I8_CSM1_c(src, tex0.csa);
#endif
			}

		}
	}
}

/* *****************************************************************
 * Clut buffer -> local C array (linear)
 * *****************************************************************/
template <class T>
/*__forceinline*/ void ClutBuffer_to_Array(T* dst, u32 csa, u32 clutsize) {}

template <>
/*__forceinline*/ void ClutBuffer_to_Array<u32>(u32* dst, u32 csa, u32 clutsize)
{
    u8* clut = (u8*)GetClutBufferAddress<u32>(csa);
    memcpy_amd((u8*)dst, clut, clutsize);
}

template <>
/*__forceinline*/ void ClutBuffer_to_Array<u16>(u16* dst, u32 csa, u32 clutsize)
{
    u16* clut = (u16*)GetClutBufferAddress<u32>(csa); // Keep aligned version for sse2

    // which side to copy
    s32 clutsize_right;
    s32 clutsize_left;
    if (csa < 16) {
        clutsize_right = min(clutsize, (16-csa)*64);
        clutsize_left = clutsize - clutsize_right;
    } else {
        clutsize_right = 0;
        clutsize_left = clutsize;
    }

    while (clutsize_right > 0)
    {
#ifdef ZEROGS_SSE2
        // only lower 16 bits of dword are valid
        __m128i clut_0 = _mm_load_si128((__m128i*)clut);
        __m128i clut_1 = _mm_load_si128((__m128i*)clut+1);
        __m128i clut_2 = _mm_load_si128((__m128i*)clut+2);
        __m128i clut_3 = _mm_load_si128((__m128i*)clut+3);

        clut_0 = _mm_shufflelo_epi16(clut_0, 0x88);
        clut_1 = _mm_shufflelo_epi16(clut_1, 0x88);
        clut_2 = _mm_shufflelo_epi16(clut_2, 0x88);
        clut_3 = _mm_shufflelo_epi16(clut_3, 0x88);

        clut_0 = _mm_shufflehi_epi16(clut_0, 0x88); // - - 3 2 1 0 - -
        clut_1 = _mm_shufflehi_epi16(clut_1, 0x88);
        clut_2 = _mm_shufflehi_epi16(clut_2, 0x88);
        clut_3 = _mm_shufflehi_epi16(clut_3, 0x88);

        clut_0 = _mm_srli_si128(clut_0, 4);
        clut_1 = _mm_srli_si128(clut_1, 4);
        clut_2 = _mm_srli_si128(clut_2, 4);
        clut_3 = _mm_srli_si128(clut_3, 4);

        _mm_store_si128((__m128i*)dst, _mm_unpacklo_epi64(clut_0, clut_1));
        _mm_store_si128((__m128i*)dst+1, _mm_unpacklo_epi64(clut_2, clut_3));
#else
        for(int i = 0; i < 16; ++i)
            dst[i] = clut[2*i];
#endif

        dst += 16;
        clut += 32;
        clutsize_right -= 32;
    }

    if(csa < 16) {
        // go back to the base before processing left clut column
        clut = (u16*)GetClutBufferAddress<u32>(0); // Keep aligned version for sse2
    }

    while (clutsize_left > 0)
    {
#ifdef ZEROGS_SSE2
        // only higher 16 bits of dword are valid
        __m128i clut_0 = _mm_load_si128((__m128i*)clut);
        __m128i clut_1 = _mm_load_si128((__m128i*)clut+1);
        __m128i clut_2 = _mm_load_si128((__m128i*)clut+2);
        __m128i clut_3 = _mm_load_si128((__m128i*)clut+3);

        clut_0 = _mm_shufflelo_epi16(clut_0, 0x88);
        clut_1 = _mm_shufflelo_epi16(clut_1, 0x88);
        clut_2 = _mm_shufflelo_epi16(clut_2, 0x88);
        clut_3 = _mm_shufflelo_epi16(clut_3, 0x88);

        clut_0 = _mm_shufflehi_epi16(clut_0, 0x88); // - - 3 2 1 0 - -
        clut_1 = _mm_shufflehi_epi16(clut_1, 0x88);
        clut_2 = _mm_shufflehi_epi16(clut_2, 0x88);
        clut_3 = _mm_shufflehi_epi16(clut_3, 0x88);

        clut_0 = _mm_srli_si128(clut_0, 4);
        clut_1 = _mm_srli_si128(clut_1, 4);
        clut_2 = _mm_srli_si128(clut_2, 4);
        clut_3 = _mm_srli_si128(clut_3, 4);

        _mm_store_si128((__m128i*)dst, _mm_unpacklo_epi64(clut_0, clut_1));
        _mm_store_si128((__m128i*)dst+1, _mm_unpacklo_epi64(clut_2, clut_3));
#else
        // Note +1 because we change higher 16 bits
        for(int i = 0; i < 16; ++i)
            dst[i] = clut[2*i+1];
#endif

        dst += 16;
        clut += 32;
        clutsize_left -= 32;
    }
}

/* *****************************************************************
 * Compare: Clut buffer <-> Local Memory
 * *****************************************************************/
// false -> identical
// true  -> different
template <class T>
/*__forceinline*/ bool Cmp_ClutBuffer_GSMem(T* GSmem, u32 csa, u32 clutsize);

template <>
/*__forceinline*/ bool Cmp_ClutBuffer_GSMem<u32>(u32* GSmem, u32 csa, u32 clutsize)
{
    u64* _GSmem = (u64*) GSmem;
    u64* clut = (u64*)GetClutBufferAddress<u32>(csa);

    while(clutsize > 0) {
#ifdef ZEROGS_SSE2
        // Note: local memory datas are swizzles
        __m128i GSmem_0 = _mm_load_si128((__m128i*)_GSmem);   // 9  8  1 0
        __m128i GSmem_1 = _mm_load_si128((__m128i*)_GSmem+1); // 11 10 3 2
        __m128i GSmem_2 = _mm_load_si128((__m128i*)_GSmem+2); // 13 12 5 4
        __m128i GSmem_3 = _mm_load_si128((__m128i*)_GSmem+3); // 15 14 7 6

        __m128i clut_0 = _mm_load_si128((__m128i*)clut);
        __m128i clut_1 = _mm_load_si128((__m128i*)clut+1);
        __m128i clut_2 = _mm_load_si128((__m128i*)clut+2);
        __m128i clut_3 = _mm_load_si128((__m128i*)clut+3);

        __m128i result = _mm_cmpeq_epi32(_mm_unpacklo_epi64(GSmem_0, GSmem_1), clut_0);

        __m128i result_tmp = _mm_cmpeq_epi32(_mm_unpacklo_epi64(GSmem_2, GSmem_3), clut_1);
        result = _mm_and_si128(result, result_tmp);

        result_tmp = _mm_cmpeq_epi32(_mm_unpackhi_epi64(GSmem_0, GSmem_1), clut_2);
        result = _mm_and_si128(result, result_tmp);

        result_tmp = _mm_cmpeq_epi32(_mm_unpackhi_epi64(GSmem_2, GSmem_3), clut_3);
        result = _mm_and_si128(result, result_tmp);

        u32 result_int = _mm_movemask_epi8(result);
        if (result_int != 0xFFFF)
            return true;
#else
        // I see no point to keep an mmx version. SSE2 versions is probably faster.
        // Keep a slow portable C version for reference/debug
        // Note: local memory datas are swizzles
        if (clut[0] != _GSmem[0] || clut[1] != _GSmem[2] || clut[2] != _GSmem[4] || clut[3] != _GSmem[6]
                || clut[4] != _GSmem[1] || clut[5] != _GSmem[3] || clut[6] != _GSmem[5] || clut[7] != _GSmem[7])
            return true;
#endif

        // go to the next memory block
        _GSmem += 32;

        // go back to the previous memory block then down one memory column
        if (clutsize & 0x40) {
            _GSmem -= (64-8);
        }
        // In case previous operation (down one column) cross the block boundary
        // Go to the next block
        if (clutsize == 0x240) {
            _GSmem += 32;
        }

        clut += 8;
        clutsize -= 64;
    }

    return false;
}

#ifdef ZEROGS_SSE2
template<bool CSA_0_15, bool HIGH_16BITS_VM>
__forceinline bool Cmp_ClutBuffer_GSMem_core(u16* GSmem, u16* clut)
{
    __m128i GSmem_0;
    __m128i GSmem_1;
    __m128i GSmem_2;
    __m128i GSmem_3;
    __m128i clut_0;
    __m128i clut_1;
    __m128i clut_2;
    __m128i clut_3;

    __m128i clut_mask = _mm_load_si128((__m128i*)s_clut_16bits_mask);

    // !HIGH_16BITS_VM
    // CSA in 0-15
    // cmp lower 16 bits of clut with lower 16 bits of GSmem
    // CSA in 16-31
    // cmp higher 16 bits of clut with lower 16 bits of GSmem

    // HIGH_16BITS_VM
    // CSA in 0-15
    // cmp lower 16 bits of clut with higher 16 bits of GSmem
    // CSA in 16-31
    // cmp higher 16 bits of clut with higher 16 bits of GSmem
    if(HIGH_16BITS_VM && CSA_0_15) {
        // move up to low
        GSmem_0 = _mm_load_si128((__m128i*)GSmem); // 9 8 1 0
        GSmem_1 = _mm_load_si128((__m128i*)GSmem+1); // 11 10 3 2
        GSmem_2 = _mm_load_si128((__m128i*)GSmem+2); // 13 12 5 4
        GSmem_3 = _mm_load_si128((__m128i*)GSmem+3); // 15 14 7 6
        GSmem_0 = _mm_srli_epi32(GSmem_0, 16);
        GSmem_1 = _mm_srli_epi32(GSmem_1, 16);
        GSmem_2 = _mm_srli_epi32(GSmem_2, 16);
        GSmem_3 = _mm_srli_epi32(GSmem_3, 16);
    } else if(HIGH_16BITS_VM && !CSA_0_15) {
        // Remove lower 16 bits
        GSmem_0 = _mm_andnot_si128(clut_mask, _mm_load_si128((__m128i*)GSmem)); // 9 8 1 0
        GSmem_1 = _mm_andnot_si128(clut_mask, _mm_load_si128((__m128i*)GSmem+1)); // 11 10 3 2
        GSmem_2 = _mm_andnot_si128(clut_mask, _mm_load_si128((__m128i*)GSmem+2)); // 13 12 5 4
        GSmem_3 = _mm_andnot_si128(clut_mask, _mm_load_si128((__m128i*)GSmem+3)); // 15 14 7 6
    } else if(!HIGH_16BITS_VM && CSA_0_15) {
        // Remove higher 16 bits
        GSmem_0 = _mm_and_si128(clut_mask, _mm_load_si128((__m128i*)GSmem)); // 9 8 1 0
        GSmem_1 = _mm_and_si128(clut_mask, _mm_load_si128((__m128i*)GSmem+1)); // 11 10 3 2
        GSmem_2 = _mm_and_si128(clut_mask, _mm_load_si128((__m128i*)GSmem+2)); // 13 12 5 4
        GSmem_3 = _mm_and_si128(clut_mask, _mm_load_si128((__m128i*)GSmem+3)); // 15 14 7 6
    } else if(!HIGH_16BITS_VM && !CSA_0_15) {
        // move low to high
        GSmem_0 = _mm_load_si128((__m128i*)GSmem); // 9 8 1 0
        GSmem_1 = _mm_load_si128((__m128i*)GSmem+1); // 11 10 3 2
        GSmem_2 = _mm_load_si128((__m128i*)GSmem+2); // 13 12 5 4
        GSmem_3 = _mm_load_si128((__m128i*)GSmem+3); // 15 14 7 6
        GSmem_0 = _mm_slli_epi32(GSmem_0, 16);
        GSmem_1 = _mm_slli_epi32(GSmem_1, 16);
        GSmem_2 = _mm_slli_epi32(GSmem_2, 16);
        GSmem_3 = _mm_slli_epi32(GSmem_3, 16);
    }

    // Unsizzle the data
    __m128i row_0 = _mm_unpacklo_epi64(GSmem_0, GSmem_1); // 3 2 1 0
    __m128i row_1 = _mm_unpacklo_epi64(GSmem_2, GSmem_3); // 7 6 5 4
    __m128i row_2 = _mm_unpackhi_epi64(GSmem_0, GSmem_1); // 11 10 9 8
    __m128i row_3 = _mm_unpackhi_epi64(GSmem_2, GSmem_3); // 15 14 13 12

    // load old data & remove useless part
    if(!CSA_0_15) {
        // Remove lower 16 bits
        clut_0 = _mm_andnot_si128(clut_mask, _mm_load_si128((__m128i*)clut));
        clut_1 = _mm_andnot_si128(clut_mask, _mm_load_si128((__m128i*)clut+1));
        clut_2 = _mm_andnot_si128(clut_mask, _mm_load_si128((__m128i*)clut+2));
        clut_3 = _mm_andnot_si128(clut_mask, _mm_load_si128((__m128i*)clut+3));
    } else {
        // Remove higher 16 bits
        clut_0 = _mm_and_si128(clut_mask, _mm_load_si128((__m128i*)clut));
        clut_1 = _mm_and_si128(clut_mask, _mm_load_si128((__m128i*)clut+1));
        clut_2 = _mm_and_si128(clut_mask, _mm_load_si128((__m128i*)clut+2));
        clut_3 = _mm_and_si128(clut_mask, _mm_load_si128((__m128i*)clut+3));
    }

    // Do the comparaison
    __m128i result = _mm_cmpeq_epi16(row_0, clut_0);
    __m128i result_tmp = _mm_cmpeq_epi16(row_1, clut_1);
    result = _mm_and_si128(result, result_tmp);

    result_tmp = _mm_cmpeq_epi16(row_2, clut_2);
    result = _mm_and_si128(result, result_tmp);

    result_tmp = _mm_cmpeq_epi16(row_3, clut_3);
    result = _mm_and_si128(result, result_tmp);

    u32 result_int = _mm_movemask_epi8(result);
    if(CSA_0_15) {
        // only lower 16bits must be checked
        if ((result_int&0x3333) != 0x3333)
            return true;
    } else {
        // only higher 16bits must be checked
        if ((result_int&0xCCCC) != 0xCCCC)
            return true;
    }

    return false;
}
#endif

template <>
/*__forceinline*/ bool Cmp_ClutBuffer_GSMem<u16>(u16* GSmem, u32 csa, u32 clutsize)
{
#ifdef ZEROGS_SSE2
    u16* clut = (u16*)GetClutBufferAddress<u32>(csa); // Keep aligned version for sse2

    // Special case only one CSA block to check
    if(clutsize == 32) {
        if (csa < 16)
            return Cmp_ClutBuffer_GSMem_core<true, false>(GSmem, clut);
        else
            return Cmp_ClutBuffer_GSMem_core<false, false>(GSmem, clut);
    }

    // which side to cmp
    s32 clutsize_right; // Note clutsize_right could be negative !
    u32 clutsize_left;
    if (csa < 16) {
        // the '-32' is a trick to handle easily when csa is odd
        clutsize_right = min(clutsize, (16-csa)*32) -32;
        clutsize_left = clutsize - clutsize_right;
    } else {
        clutsize_right = 0;
        clutsize_left = clutsize;
    }

    while(clutsize_right > 0) {
        if (Cmp_ClutBuffer_GSMem_core<true, false>(GSmem, clut))
            return true;
        clut += 32;

        if (Cmp_ClutBuffer_GSMem_core<true, true>(GSmem, clut))
            return true;
        clut += 32;

        GSmem += 32; // go down one column
        clutsize_right -= 64;
	}

    if(csa < 16) {
        // because of the extra -32, csa_righ is null when csa is odd
        if (clutsize_right == 0) {
            // cross the clut
            if (Cmp_ClutBuffer_GSMem_core<true, false>(GSmem, clut))
                return true;
            clut += 32;

            if (Cmp_ClutBuffer_GSMem_core<false, true>(GSmem, clut))
                return true;

            GSmem += 32; // go down one column
            clutsize_left -= 32;
        }

        // go back to the base before processing left clut column
        clut = (u16*)GetClutBufferAddress<u32>(0); // Keep aligned version for sse2
    }

    while(clutsize_left > 0) {
        if (Cmp_ClutBuffer_GSMem_core<false, false>(GSmem, clut))
            return true;
        clut += 32;

        if (Cmp_ClutBuffer_GSMem_core<false, true>(GSmem, clut))
            return true;
        clut += 32;

        GSmem += 32; // go down one column
        clutsize_left -= 64;
	}

    return false;
#else
    // This function is only useful for performance. So just return 
    // for a plain c build
    return true;
#endif
}

/* *****************************************************************
 * Compare: Clut buffer <-> local C array (linear)
 * *****************************************************************/
// false -> identical
// true  -> different
template <class T>
/*__forceinline*/ bool Cmp_ClutBuffer_SavedClut(T* saved_clut, u32 csa, u32 clutsize);

template <>
/*__forceinline*/ bool Cmp_ClutBuffer_SavedClut<u32>(u32* saved_clut, u32 csa, u32 clutsize)
{
    u32* clut = GetClutBufferAddress<u32>(csa);
    return !!memcmp_mmx(saved_clut, clut, clutsize);
}

template <>
/*__forceinline*/ bool Cmp_ClutBuffer_SavedClut<u16>(u16* saved_clut, u32 csa, u32 clutsize)
{
	assert((clutsize&31) == 0);

#ifdef ZEROGS_SSE2
    __m128i zero_128 = _mm_setzero_si128();
#endif
    u16* clut = (u16*)GetClutBufferAddress<u32>(csa); // Keep aligned version for sse2

    // which side to cmp
    u32 clutsize_right;
    u32 clutsize_left;
    if (csa < 16) {
        clutsize_right = min(clutsize, (16-csa)*32);
        clutsize_left = clutsize - clutsize_right;
    } else {
        clutsize_right = 0;
        clutsize_left = clutsize;
    }

    while (clutsize_right > 0)
    {
#ifdef ZEROGS_SSE2
        // only lower 16 bits of dword are valid
        __m128i clut_0 = _mm_load_si128((__m128i*)clut);
        __m128i clut_1 = _mm_load_si128((__m128i*)clut+1);
        __m128i clut_2 = _mm_load_si128((__m128i*)clut+2);
        __m128i clut_3 = _mm_load_si128((__m128i*)clut+3);

        // value must converted to 32 bits
        __m128i saved_clut_0 = _mm_load_si128((__m128i*)saved_clut);
        __m128i saved_clut_1 = _mm_load_si128((__m128i*)saved_clut+1);

        __m128i result = _mm_cmpeq_epi16(_mm_unpacklo_epi16(saved_clut_0, zero_128), clut_0);
        __m128i result_tmp = _mm_cmpeq_epi16(_mm_unpackhi_epi16(saved_clut_0, zero_128), clut_1);
        result = _mm_and_si128(result, result_tmp);

        result_tmp = _mm_cmpeq_epi16(_mm_unpacklo_epi16(saved_clut_1, zero_128), clut_2);
        result = _mm_and_si128(result, result_tmp);

        result_tmp = _mm_cmpeq_epi16(_mm_unpackhi_epi16(saved_clut_1, zero_128), clut_3);
        result = _mm_and_si128(result, result_tmp);

        u32 result_int = _mm_movemask_epi8(result);
        // only lower 16bits must be checked
        if ((result_int&0x3333) != 0x3333)
            return true;
#else
		for (int i = 0; i < 16; ++i)
			if (saved_clut[i] != clut[2*i]) return true;
#endif

		saved_clut += 16;
		clut += 32;
        clutsize_right -= 32;
	}

    if(csa < 16) {
        // go back to the base before processing left clut column
        clut = (u16*)GetClutBufferAddress<u32>(0); // Keep aligned version for sse2
    }

    while (clutsize_left > 0)
    {
#ifdef ZEROGS_SSE2
        // only higher 16 bits of dword are valid
        __m128i clut_0 = _mm_load_si128((__m128i*)clut);
        __m128i clut_1 = _mm_load_si128((__m128i*)clut+1);
        __m128i clut_2 = _mm_load_si128((__m128i*)clut+2);
        __m128i clut_3 = _mm_load_si128((__m128i*)clut+3);

        // value must converted to 32 bits (with 0 in lower 16 bits)
        __m128i saved_clut_0 = _mm_load_si128((__m128i*)saved_clut);
        __m128i saved_clut_1 = _mm_load_si128((__m128i*)saved_clut+1);

        __m128i result = _mm_cmpeq_epi16(_mm_unpacklo_epi16(zero_128, saved_clut_0), clut_0);
        __m128i result_tmp = _mm_cmpeq_epi16(_mm_unpackhi_epi16(zero_128, saved_clut_0), clut_1);
        result = _mm_and_si128(result, result_tmp);

        result_tmp = _mm_cmpeq_epi16(_mm_unpacklo_epi16(zero_128, saved_clut_1), clut_2);
        result = _mm_and_si128(result, result_tmp);

        result_tmp = _mm_cmpeq_epi16(_mm_unpackhi_epi16(zero_128, saved_clut_1), clut_3);
        result = _mm_and_si128(result, result_tmp);

        u32 result_int = _mm_movemask_epi8(result);
        // only higher 16bits must be checked
        if ((result_int&0xCCCC) != 0xCCCC)
            return true;
#else
        // Note +1 because we change higher 16 bits
        for (int i = 0; i < 16; ++i)
            if (saved_clut[i] != clut[2*i+1]) return true;
#endif

        saved_clut += 16;
        clut += 32;
        clutsize_left -= 32;
    }

	return false;
}


/* *****************************************************************
 * Resolve color of clut texture
 * *****************************************************************/

// used to build clut textures (note that this is for both 16 and 32 bit cluts)
template <class T>
/*__forceinline*/ void Build_Clut_Texture(u32 psm, u32 height, T* pclut, u8* psrc, T* pdst)
{
	switch (psm)
	{
		case PSMT8:
			for (u32 i = 0; i < height; ++i)
			{
				for (int j = 0; j < GPU_TEXWIDTH / 2; ++j)
				{
					pdst[0] = pclut[psrc[0]];
					pdst[1] = pclut[psrc[1]];
					pdst[2] = pclut[psrc[2]];
					pdst[3] = pclut[psrc[3]];
					pdst[4] = pclut[psrc[4]];
					pdst[5] = pclut[psrc[5]];
					pdst[6] = pclut[psrc[6]];
					pdst[7] = pclut[psrc[7]];
					pdst += 8;
					psrc += 8;
				}
			}
			break;

		case PSMT4:
			for (u32 i = 0; i < height; ++i)
			{
				for (int j = 0; j < GPU_TEXWIDTH; ++j)
				{
					pdst[0] = pclut[psrc[0] & 15];
					pdst[1] = pclut[psrc[0] >> 4];
					pdst[2] = pclut[psrc[1] & 15];
					pdst[3] = pclut[psrc[1] >> 4];
					pdst[4] = pclut[psrc[2] & 15];
					pdst[5] = pclut[psrc[2] >> 4];
					pdst[6] = pclut[psrc[3] & 15];
					pdst[7] = pclut[psrc[3] >> 4];

					pdst += 8;
					psrc += 4;
				}
			}
			break;

		case PSMT8H:
			for (u32 i = 0; i < height; ++i)
			{
				for (int j = 0; j < GPU_TEXWIDTH / 8; ++j)
				{
					pdst[0] = pclut[psrc[3]];
					pdst[1] = pclut[psrc[7]];
					pdst[2] = pclut[psrc[11]];
					pdst[3] = pclut[psrc[15]];
					pdst[4] = pclut[psrc[19]];
					pdst[5] = pclut[psrc[23]];
					pdst[6] = pclut[psrc[27]];
					pdst[7] = pclut[psrc[31]];
					pdst += 8;
					psrc += 32;
				}
			}
			break;

		case PSMT4HH:
			for (u32 i = 0; i < height; ++i)
			{
				for (int j = 0; j < GPU_TEXWIDTH / 8; ++j)
				{
					pdst[0] = pclut[psrc[3] >> 4];
					pdst[1] = pclut[psrc[7] >> 4];
					pdst[2] = pclut[psrc[11] >> 4];
					pdst[3] = pclut[psrc[15] >> 4];
					pdst[4] = pclut[psrc[19] >> 4];
					pdst[5] = pclut[psrc[23] >> 4];
					pdst[6] = pclut[psrc[27] >> 4];
					pdst[7] = pclut[psrc[31] >> 4];
					pdst += 8;
					psrc += 32;
				}
			}
			break;

		case PSMT4HL:
			for (u32 i = 0; i < height; ++i)
			{
				for (int j = 0; j < GPU_TEXWIDTH / 8; ++j)
				{
					pdst[0] = pclut[psrc[3] & 15];
					pdst[1] = pclut[psrc[7] & 15];
					pdst[2] = pclut[psrc[11] & 15];
					pdst[3] = pclut[psrc[15] & 15];
					pdst[4] = pclut[psrc[19] & 15];
					pdst[5] = pclut[psrc[23] & 15];
					pdst[6] = pclut[psrc[27] & 15];
					pdst[7] = pclut[psrc[31] & 15];
					pdst += 8;
					psrc += 32;
				}
			}
			break;

		default:
			assert(0);
	}
}

// Instantiate the Build_Clut_Texture template...
template void Build_Clut_Texture<u32>(u32 psm, u32 height, u32* pclut, u8* psrc, u32* pdst);
template void Build_Clut_Texture<u16>(u32 psm, u32 height, u16* pclut, u8* psrc, u16* pdst);
