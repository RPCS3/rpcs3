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
#include "Mem_Swizzle.h"
#ifdef ZEROGS_SSE2
#include <emmintrin.h>
#endif

// WARNING a sfence instruction must be call after SwizzleBlock sse2 function

// Current port of the ASM function to intrinsic
#define INTRINSIC_PORT_16
#define INTRINSIC_PORT_8
#define INTRINSIC_PORT_4
#ifdef ZEROGS_SSE2
static const __aligned16 u32 mask_24b_H[4] = {0xFF000000, 0x0000FFFF, 0xFF000000, 0x0000FFFF};
static const __aligned16 u32 mask_24b_L[4] = {0x00FFFFFF, 0x00000000, 0x00FFFFFF, 0x00000000};

template<bool aligned>
__forceinline void SwizzleBlock32_sse2_I(u8 *dst, u8 *src, int pitch)
{
    __m128i src_0;
    __m128i src_1;
    __m128i src_2;
    __m128i src_3;

    for (int i=3 ; i >= 0 ; --i) {
        // load
        if (aligned) {
            src_0 = _mm_load_si128((__m128i*)src); // 5 4 1 0
            src_1 = _mm_load_si128((__m128i*)(src+16)); // 13 12 9 8
            src_2 = _mm_load_si128((__m128i*)(src+pitch)); // 7 6 3 2
            src_3 = _mm_load_si128((__m128i*)(src+16+pitch)); // 15 14 11 10
        } else {
            src_0 = _mm_loadu_si128((__m128i*)src); // 5 4 1 0
            src_1 = _mm_loadu_si128((__m128i*)(src+16)); // 13 12 9 8
            src_2 = _mm_loadu_si128((__m128i*)(src+pitch)); // 7 6 3 2
            src_3 = _mm_loadu_si128((__m128i*)(src+16+pitch)); // 15 14 11 10
        }

        //  Reorder
        __m128i dst_0 = _mm_unpacklo_epi64(src_0, src_2); // 3 2 1 0
        __m128i dst_1 = _mm_unpackhi_epi64(src_0, src_2); // 7 6 5 4
        __m128i dst_2 = _mm_unpacklo_epi64(src_1, src_3); // 11 10 9 8
        __m128i dst_3 = _mm_unpackhi_epi64(src_1, src_3); // 15 14 13 12

        // store
        _mm_stream_si128((__m128i*)dst, dst_0);
        _mm_stream_si128(((__m128i*)dst)+1, dst_1);
        _mm_stream_si128(((__m128i*)dst)+2, dst_2);
        _mm_stream_si128(((__m128i*)dst)+3, dst_3);

        // update the pointer
        dst += 64;
        src += 2*pitch;
    }
}

template<bool aligned>
__forceinline void SwizzleBlock16_sse2_I(u8 *dst, u8 *src, int pitch)
{
    __m128i src_0_L;
    __m128i src_0_H;
    __m128i src_2_L;
    __m128i src_2_H;

    for (int i=3 ; i >= 0 ; --i) {
        // load
        if (aligned) {
            src_0_L = _mm_load_si128((__m128i*)src); // 13L 12L 9L 8L 5L 4L 1L 0L
            src_0_H = _mm_load_si128((__m128i*)(src+16)); // 13H 12H 9H 8H 5H 4H 1H 0H
            src_2_L = _mm_load_si128((__m128i*)(src+pitch)); // 15L 14L 11L 10L 7L 6L 3L 2L
            src_2_H = _mm_load_si128((__m128i*)(src+16+pitch)); // 15H 14H 11H 10H 7H 6H 3H 2H
        } else {
            src_0_L = _mm_loadu_si128((__m128i*)src); // 13L 12L 9L 8L 5L 4L 1L 0L
            src_0_H = _mm_loadu_si128((__m128i*)(src+16)); // 13H 12H 9H 8H 5H 4H 1H 0H
            src_2_L = _mm_loadu_si128((__m128i*)(src+pitch)); // 15L 14L 11L 10L 7L 6L 3L 2L
            src_2_H = _mm_loadu_si128((__m128i*)(src+16+pitch)); // 15H 14H 11H 10H 7H 6H 3H 2H
        }

        // Interleave L and H to obtains 32 bits packets
        __m128i dst_0_tmp = _mm_unpacklo_epi16(src_0_L, src_0_H); // 5H 5L 4H 4L 1H 1L 0H 0L
        __m128i dst_1_tmp = _mm_unpacklo_epi16(src_2_L, src_2_H); // 7H 7L 6H 6L 3H 3L 2H 2L
        __m128i dst_2_tmp = _mm_unpackhi_epi16(src_0_L, src_0_H); // 13H 13L 12H 12L 9H 9L 8H 8L
        __m128i dst_3_tmp = _mm_unpackhi_epi16(src_2_L, src_2_H); // 15H 15L 14H 14L 11H 11L 10H 10L

        //  Reorder
        __m128i dst_0 = _mm_unpacklo_epi64(dst_0_tmp, dst_1_tmp); // 3 2 1 0
        __m128i dst_1 = _mm_unpackhi_epi64(dst_0_tmp, dst_1_tmp); // 7 6 5 4
        __m128i dst_2 = _mm_unpacklo_epi64(dst_2_tmp, dst_3_tmp); // 11 10 9 8
        __m128i dst_3 = _mm_unpackhi_epi64(dst_2_tmp, dst_3_tmp); // 15 14 13 12

        // store
        _mm_stream_si128((__m128i*)dst, dst_0);
        _mm_stream_si128(((__m128i*)dst)+1, dst_1);
        _mm_stream_si128(((__m128i*)dst)+2, dst_2);
        _mm_stream_si128(((__m128i*)dst)+3, dst_3);

        // update the pointer
        dst += 64;
        src += 2*pitch;
    }
}

// Template the code to improve reuse of code
template<bool aligned, u32 INDEX>
__forceinline void SwizzleColumn8_sse2_I(u8 *dst, u8 *src, int pitch)
{
    __m128i src_0;
    __m128i src_1;
    __m128i src_2;
    __m128i src_3;

    // load 4 line of 16*8 bits packets
    if (aligned) {
        src_0 = _mm_load_si128((__m128i*)src);
        src_2 = _mm_load_si128((__m128i*)(src+pitch));
        src_1 = _mm_load_si128((__m128i*)(src+2*pitch));
        src_3 = _mm_load_si128((__m128i*)(src+3*pitch));
    } else {
        src_0 = _mm_loadu_si128((__m128i*)src);
        src_2 = _mm_loadu_si128((__m128i*)(src+pitch));
        src_1 = _mm_loadu_si128((__m128i*)(src+2*pitch));
        src_3 = _mm_loadu_si128((__m128i*)(src+3*pitch));
    }

    // shuffle 2 lines to align pixels
    if (INDEX == 0 || INDEX == 2) {
        src_1 = _mm_shuffle_epi32(src_1, 0xB1); // 13 12 9 8 5 4 1 0 ... (byte 3 & 1)
        src_3 = _mm_shuffle_epi32(src_3, 0xB1); // 15 14 11 10 7 6 3 2 ... (byte 3 & 1)
    } else if (INDEX == 1 || INDEX == 3) {
        src_0 = _mm_shuffle_epi32(src_0, 0xB1); // 13 12 9 8 5 4 1 0 ... (byte 2 & 0)
        src_2 = _mm_shuffle_epi32(src_2, 0xB1); // 15 14 11 10 7 6 3 2 ... (byte 2 & 0)
    } else {
        assert(0);
    }
    // src_0 = 13 12 9 8  5 4 1 0 ... (byte 2 & 0)
    // src_1 = 13 12 9 8  5 4 1 0 ... (byte 3 & 1)
    // src_2 = 15 14 11 10  7 6 3 2 ... (byte 2 & 0)
    // src_3 = 15 14 11 10  7 6 3 2 ... (byte 3 & 1)

    // Interleave byte 1 & 0 to obtain 16 bits packets
    __m128i src_0_L = _mm_unpacklo_epi8(src_0, src_1); // 13L 12L 9L 8L 5L 4L 1L 0L
    __m128i src_1_L = _mm_unpacklo_epi8(src_2, src_3); // 15L 14L 11L 10L 7L 6L 3L 2L
    // Interleave byte 3 & 2 to obtain 16 bits packets
    __m128i src_0_H = _mm_unpackhi_epi8(src_0, src_1); // 13H 12H 9H 8H 5H 4H 1H 0H
    __m128i src_1_H = _mm_unpackhi_epi8(src_2, src_3); // 15H 14H 11H 10H 7H 6H 3H 2H

    // Interleave H and L to obtain 32 bits packets
    __m128i dst_0_tmp = _mm_unpacklo_epi16(src_0_L, src_0_H); // 5 4 1 0
    __m128i dst_1_tmp = _mm_unpacklo_epi16(src_1_L, src_1_H); // 7 6 3 2
    __m128i dst_2_tmp = _mm_unpackhi_epi16(src_0_L, src_0_H); // 13 12 9 8
    __m128i dst_3_tmp = _mm_unpackhi_epi16(src_1_L, src_1_H); // 15 14 11 10

    // Reorder the 32 bits packets
    __m128i dst_0 = _mm_unpacklo_epi64(dst_0_tmp, dst_1_tmp); // 3 2 1 0
    __m128i dst_1 = _mm_unpackhi_epi64(dst_0_tmp, dst_1_tmp); // 7 6 5 4
    __m128i dst_2 = _mm_unpacklo_epi64(dst_2_tmp, dst_3_tmp); // 11 10 9 8
    __m128i dst_3 = _mm_unpackhi_epi64(dst_2_tmp, dst_3_tmp); // 15 14 13 12

    // store
    _mm_stream_si128((__m128i*)dst, dst_0);
    _mm_stream_si128(((__m128i*)dst)+1, dst_1);
    _mm_stream_si128(((__m128i*)dst)+2, dst_2);
    _mm_stream_si128(((__m128i*)dst)+3, dst_3);
}

template<bool aligned>
__forceinline void SwizzleBlock8_sse2_I(u8 *dst, u8 *src, int pitch)
{
    SwizzleColumn8_sse2_I<aligned, 0>(dst, src, pitch);

    dst += 64;
    src += 4*pitch;
    SwizzleColumn8_sse2_I<aligned, 1>(dst, src, pitch);

    dst += 64;
    src += 4*pitch;
    SwizzleColumn8_sse2_I<aligned, 2>(dst, src, pitch);

    dst += 64;
    src += 4*pitch;
    SwizzleColumn8_sse2_I<aligned, 3>(dst, src, pitch);
}

// Template the code to improve reuse of code
template<bool aligned, u32 INDEX>
__forceinline void SwizzleColumn4_sse2_I(u8 *dst, u8 *src, int pitch)
{
    __m128i src_0;
    __m128i src_1;
    __m128i src_2;
    __m128i src_3;

    // Build a mask (tranform a u32 to a 4 packets u32)
    const u32 mask_template = 0x0f0f0f0f;
    __m128i mask = _mm_cvtsi32_si128(mask_template);
    mask = _mm_shuffle_epi32(mask, 0);

    // load 4 line of 32*4 bits packets
    if (aligned) {
        src_0 = _mm_load_si128((__m128i*)src);
        src_2 = _mm_load_si128((__m128i*)(src+pitch));
        src_1 = _mm_load_si128((__m128i*)(src+2*pitch));
        src_3 = _mm_load_si128((__m128i*)(src+3*pitch));
    } else {
        src_0 = _mm_loadu_si128((__m128i*)src);
        src_2 = _mm_loadu_si128((__m128i*)(src+pitch));
        src_1 = _mm_loadu_si128((__m128i*)(src+2*pitch));
        src_3 = _mm_loadu_si128((__m128i*)(src+3*pitch));
    }

    // shuffle 2 lines to align pixels
    if (INDEX == 0 || INDEX == 2) {
        src_1 = _mm_shufflelo_epi16(src_1, 0xB1);
        src_1 = _mm_shufflehi_epi16(src_1, 0xB1); // 13 12 9 8  5 4 1 0 ... (Half-byte 7 & 5 & 3 & 1)
        src_3 = _mm_shufflelo_epi16(src_3, 0xB1);
        src_3 = _mm_shufflehi_epi16(src_3, 0xB1); // 15 14 11 10  7 6 3 2 ... (Half-byte 7 & 5 & 3 & 1)
    } else if (INDEX == 1 || INDEX == 3) {
        src_0 = _mm_shufflelo_epi16(src_0, 0xB1);
        src_0 = _mm_shufflehi_epi16(src_0, 0xB1); // 13 12 9 8  5 4 1 0 ... (Half-byte 6 & 4 & 2 & 0)
        src_2 = _mm_shufflelo_epi16(src_2, 0xB1);
        src_2 = _mm_shufflehi_epi16(src_2, 0xB1); // 15 14 11 10  7 6 3 2 ... (Half-byte 6 & 4 & 2 & 0)
    } else {
        assert(0);
    }
    // src_0 = 13 12 9 8  5 4 1 0 ... (Half-byte 6 & 4 & 2 & 0)
    // src_1 = 13 12 9 8  5 4 1 0 ... (Half-byte 7 & 5 & 3 & 1)
    // src_2 = 15 14 11 10  7 6 3 2 ... (Half-byte 6 & 4 & 2 & 0)
    // src_3 = 15 14 11 10  7 6 3 2 ... (Half-byte 7 & 5 & 3 & 1)

    // ** Interleave Half-byte to obtain 8 bits packets
    // Shift value to ease 4 bits filter.
    // Note use a packet shift to allow a 4bits shifts
    __m128i src_0_shift = _mm_srli_epi64(src_0, 4); // ? 13 12 9   8 5 4 1 ... (Half-byte 6 & 4 & 2 & 0)
    __m128i src_1_shift = _mm_slli_epi64(src_1, 4); // 12 9 8 5    4 1 0 ? ... (Half-byte 7 & 5 & 3 & 1)
    __m128i src_2_shift = _mm_srli_epi64(src_2, 4); // ? 15 14 11  10 7 6 3 ... (Half-byte 6 & 4 & 2 & 0)
    __m128i src_3_shift = _mm_slli_epi64(src_3, 4); // 14 11 10 7  6 3 2 ? ... (Half-byte 7 & 5 & 3 & 1)

    // 12 - 8 - 4 - 0 - (HB odd) || - 12 - 8 - 4 - 0 (HB even) => 12 8 4 0 (byte 3 & 2 & 1 & 0)
    src_0 = _mm_or_si128(_mm_andnot_si128(mask, src_1_shift), _mm_and_si128(mask, src_0));
    // - 13 - 9 - 5 - 1 (HB even) || 13 - 9 - 5 - 1 - (HB odd) => 13 9 5 1 (byte 3 & 2 & 1 & 0)
    src_1 = _mm_or_si128(_mm_and_si128(mask, src_0_shift), _mm_andnot_si128(mask, src_1));

    // 14 - 10 - 6 - 2 - (HB odd) || - 14 - 10 - 6 - 2 (HB even) => 14 10 6 2 (byte 3 & 2 & 1 & 0)
    src_2 = _mm_or_si128(_mm_andnot_si128(mask, src_3_shift), _mm_and_si128(mask, src_2));
    // - 15 - 11 - 7 - 3 (HB even) || 15 - 11 - 7 - 3 - (HB odd) => 15 11 7 3 (byte 3 & 2 & 1 & 0)
    src_3 = _mm_or_si128(_mm_and_si128(mask, src_2_shift), _mm_andnot_si128(mask, src_3));


    // reorder the 8 bits packets
    __m128i src_0_tmp = _mm_unpacklo_epi8(src_0, src_1); // 13 12 9 8 5 4 1 0 (byte 1 & 0)
    __m128i src_1_tmp = _mm_unpackhi_epi8(src_0, src_1); // 13 12 9 8 5 4 1 0 (byte 3 & 2)
    __m128i src_2_tmp = _mm_unpacklo_epi8(src_2, src_3); // 15 14 11 10 7 6 3 2 (byte 1 & 0)
    __m128i src_3_tmp = _mm_unpackhi_epi8(src_2, src_3); // 15 14 11 10 7 6 3 2 (byte 3 & 2)

    // interleave byte to obtain 32 bits packets
    __m128i src_0_L = _mm_unpacklo_epi8(src_0_tmp, src_1_tmp); // 2.13 0.13 2.12 0.12 2.9 0.9 2.8 0.8 2.5 0.5 2.4 0.4 2.1 0.1 2.0 0.0
    __m128i src_0_H = _mm_unpackhi_epi8(src_0_tmp, src_1_tmp); // 3.13 1.13 3.12 1.12 3.9 1.9 3.8 1.8 3.5 1.5 3.4 1.4 3.1 1.1 3.0 1.0
    __m128i src_1_L = _mm_unpacklo_epi8(src_2_tmp, src_3_tmp); // 2.15 0.15 2.14 0.14 2.11 0.11 2.10 0.10 2.7 0.7 2.6 0.6 2.3 0.3 2.2 0.2
    __m128i src_1_H = _mm_unpackhi_epi8(src_2_tmp, src_3_tmp); // 3.15 1.15 3.14 1.14 3.11 1.11 3.10 1.10 3.7 1.7 3.6 1.6 3.3 1.3 3.2 1.2

    __m128i dst_0_tmp = _mm_unpacklo_epi8(src_0_L, src_0_H); // 5 4 1 0
    __m128i dst_1_tmp = _mm_unpacklo_epi8(src_1_L, src_1_H); // 7 6 3 2
    __m128i dst_2_tmp = _mm_unpackhi_epi8(src_0_L, src_0_H); // 13 12 9 8
    __m128i dst_3_tmp = _mm_unpackhi_epi8(src_1_L, src_1_H); // 15 14 11 10

    // Reorder the 32 bits packets
    __m128i dst_0 = _mm_unpacklo_epi64(dst_0_tmp, dst_1_tmp); // 3 2 1 0
    __m128i dst_1 = _mm_unpackhi_epi64(dst_0_tmp, dst_1_tmp); // 7 6 5 4
    __m128i dst_2 = _mm_unpacklo_epi64(dst_2_tmp, dst_3_tmp); // 11 10 9 8
    __m128i dst_3 = _mm_unpackhi_epi64(dst_2_tmp, dst_3_tmp); // 15 14 13 12

    // store
    _mm_stream_si128((__m128i*)dst, dst_0);
    _mm_stream_si128(((__m128i*)dst)+1, dst_1);
    _mm_stream_si128(((__m128i*)dst)+2, dst_2);
    _mm_stream_si128(((__m128i*)dst)+3, dst_3);
}

template<bool aligned>
__forceinline void SwizzleBlock4_sse2_I(u8 *dst, u8 *src, int pitch)
{
    SwizzleColumn4_sse2_I<aligned, 0>(dst, src, pitch);

    dst += 64;
    src += 4*pitch;
    SwizzleColumn4_sse2_I<aligned, 1>(dst, src, pitch);

    dst += 64;
    src += 4*pitch;
    SwizzleColumn4_sse2_I<aligned, 2>(dst, src, pitch);

    dst += 64;
    src += 4*pitch;
    SwizzleColumn4_sse2_I<aligned, 3>(dst, src, pitch);
}

template<bool FOUR_BIT, bool UPPER>
__forceinline void SwizzleBlock8H_4H(u8 *dst, u8 *src, int pitch)
{
    __m128i zero_128 = _mm_setzero_si128();
    __m128i src_0;
    __m128i src_1;
    __m128i src_2;
    __m128i src_3;
    __m128i src_0_init_H;
    __m128i src_0_init_L;
    __m128i src_2_init_H;
    __m128i src_2_init_L;
    __m128i src_0_init;
    __m128i src_2_init;

    __m128i upper_mask = _mm_cvtsi32_si128(0xF0F0F0F0);
    // Build the write_mask (tranform a u32 to a 4 packets u32)
    __m128i write_mask;
    if (FOUR_BIT) {
        if (UPPER) write_mask = _mm_cvtsi32_si128(0xF0000000);
        else write_mask = _mm_cvtsi32_si128(0x0F000000);
    } else {
        write_mask = _mm_cvtsi32_si128(0xFF000000);
    }
    write_mask = _mm_shuffle_epi32(write_mask, 0);

    for (int i=3 ; i >= 0 ; --i) {
        if (FOUR_BIT) {
            src_0_init = _mm_cvtsi32_si128(*(u32*)src);
            src_2_init = _mm_cvtsi32_si128(*(u32*)(src + pitch));
        } else {
            src_0_init = _mm_loadl_epi64((__m128i*)src);
            src_2_init = _mm_loadl_epi64((__m128i*)(src + pitch));
        }

        // Convert to 8 bits
        if (FOUR_BIT) {
            src_0_init_H = _mm_and_si128(upper_mask, src_0_init);
            src_0_init_L = _mm_andnot_si128(upper_mask, src_0_init);
            src_2_init_H = _mm_and_si128(upper_mask, src_2_init);
            src_2_init_L = _mm_andnot_si128(upper_mask, src_2_init);

            if (UPPER) {
                src_0_init_L = _mm_slli_epi32(src_0_init_L, 4);
                src_2_init_L = _mm_slli_epi32(src_2_init_L, 4);
            } else {
                src_0_init_H = _mm_srli_epi32(src_0_init_H, 4);
                src_2_init_H = _mm_srli_epi32(src_2_init_H, 4);
            }

            // Repack the src to keep HByte order
            src_0_init = _mm_unpacklo_epi8(src_0_init_L, src_0_init_H);
            src_2_init = _mm_unpacklo_epi8(src_2_init_L, src_2_init_H);
        }

        // transform to 16 bits (add 0 in low bits)
        src_0_init = _mm_unpacklo_epi8(zero_128, src_0_init);
        src_2_init = _mm_unpacklo_epi8(zero_128, src_2_init);

        // transform to 32 bits (add 0 in low bits)
        src_0 = _mm_unpacklo_epi16(zero_128, src_0_init);
        src_1 = _mm_unpackhi_epi16(zero_128, src_0_init);
        src_2 = _mm_unpacklo_epi16(zero_128, src_2_init);
        src_3 = _mm_unpackhi_epi16(zero_128, src_2_init);

        // Reorder the data (same as 32 bits format)
        __m128i dst_0 = _mm_unpacklo_epi64(src_0, src_2);
        __m128i dst_1 = _mm_unpackhi_epi64(src_0, src_2);
        __m128i dst_2 = _mm_unpacklo_epi64(src_1, src_3);
        __m128i dst_3 = _mm_unpackhi_epi64(src_1, src_3);

        // Load previous value and apply the ~write_mask
        __m128i old_dst_0 = _mm_andnot_si128(write_mask, _mm_load_si128((__m128i*)dst));
        dst_0 = _mm_or_si128(dst_0, old_dst_0);

        __m128i old_dst_1 = _mm_andnot_si128(write_mask, _mm_load_si128(((__m128i*)dst)+1));
        dst_1 = _mm_or_si128(dst_1, old_dst_1);

        __m128i old_dst_2 = _mm_andnot_si128(write_mask, _mm_load_si128(((__m128i*)dst)+2));
        dst_2 = _mm_or_si128(dst_2, old_dst_2);

        __m128i old_dst_3 = _mm_andnot_si128(write_mask, _mm_load_si128(((__m128i*)dst)+3));
        dst_3 = _mm_or_si128(dst_3, old_dst_3);

        // store
        _mm_stream_si128((__m128i*)dst, dst_0);
        _mm_stream_si128(((__m128i*)dst)+1, dst_1);
        _mm_stream_si128(((__m128i*)dst)+2, dst_2);
        _mm_stream_si128(((__m128i*)dst)+3, dst_3);

        // update the pointer
        dst += 64;
        src += 2*pitch;
    }
}

// special swizzle macros - which I converted to functions.

__forceinline void SwizzleBlock32(u8 *dst, u8 *src, int pitch)
{
    SwizzleBlock32_sse2_I<true>(dst, src, pitch);
}

__forceinline void SwizzleBlock24(u8 *dst, u8 *src, int pitch)
{
    __m128i mask_H = _mm_load_si128((__m128i*)mask_24b_H);
    __m128i mask_L = _mm_load_si128((__m128i*)mask_24b_L);
    // Build the write_mask (tranform a u32 to a 4 packets u32)
    __m128i write_mask = _mm_cvtsi32_si128(0x00FFFFFF);
    write_mask = _mm_shuffle_epi32(write_mask, 0);

    for (int i=3 ; i >= 0 ; --i) {
        //  Note src can be out of bound of GS memory (but there is some spare allocation
        //  to avoid a tricky corner case)
        __m128i src_0 = _mm_loadu_si128((__m128i*)src);
        __m128i src_1 = _mm_loadu_si128((__m128i*)(src+12));
        __m128i src_2 = _mm_loadu_si128((__m128i*)(src+pitch));
        __m128i src_3 = _mm_loadu_si128((__m128i*)(src+pitch+12));

        // transform 24 bits value to 32 bits one
        // 1/ Align a little the data
        src_0 = _mm_slli_si128(src_0, 2);
        src_0 = _mm_shufflelo_epi16(src_0, 0x39);

        src_1 = _mm_slli_si128(src_1, 2);
        src_1 = _mm_shufflelo_epi16(src_1, 0x39);

        src_2 = _mm_slli_si128(src_2, 2);
        src_2 = _mm_shufflelo_epi16(src_2, 0x39);

        src_3 = _mm_slli_si128(src_3, 2);
        src_3 = _mm_shufflelo_epi16(src_3, 0x39);

        // 2/ Filter the 24 bits pixels & do the conversion
        __m128i src_0_H = _mm_and_si128(src_0, mask_H);
        __m128i src_0_L = _mm_and_si128(src_0, mask_L);
        src_0_H = _mm_slli_si128(src_0_H, 1);
        src_0 = _mm_or_si128(src_0_H, src_0_L);

        __m128i src_1_H = _mm_and_si128(src_1, mask_H);
        __m128i src_1_L = _mm_and_si128(src_1, mask_L);
        src_1_H = _mm_slli_si128(src_1_H, 1);
        src_1 = _mm_or_si128(src_1_H, src_1_L);

        __m128i src_2_H = _mm_and_si128(src_2, mask_H);
        __m128i src_2_L = _mm_and_si128(src_2, mask_L);
        src_2_H = _mm_slli_si128(src_2_H, 1);
        src_2 = _mm_or_si128(src_2_H, src_2_L);

        __m128i src_3_H = _mm_and_si128(src_3, mask_H);
        __m128i src_3_L = _mm_and_si128(src_3, mask_L);
        src_3_H = _mm_slli_si128(src_3_H, 1);
        src_3 = _mm_or_si128(src_3_H, src_3_L);

        // Reorder the data (same as 32 bits format)
        __m128i dst_0 = _mm_unpacklo_epi64(src_0, src_2);
        __m128i dst_1 = _mm_unpackhi_epi64(src_0, src_2);
        __m128i dst_2 = _mm_unpacklo_epi64(src_1, src_3);
        __m128i dst_3 = _mm_unpackhi_epi64(src_1, src_3);

        // Load previous value and apply the ~write_mask
        __m128i old_dst_0 = _mm_andnot_si128(write_mask, _mm_load_si128((__m128i*)dst));
        dst_0 = _mm_or_si128(dst_0, old_dst_0);

        __m128i old_dst_1 = _mm_andnot_si128(write_mask, _mm_load_si128(((__m128i*)dst)+1));
        dst_1 = _mm_or_si128(dst_1, old_dst_1);

        __m128i old_dst_2 = _mm_andnot_si128(write_mask, _mm_load_si128(((__m128i*)dst)+2));
        dst_2 = _mm_or_si128(dst_2, old_dst_2);

        __m128i old_dst_3 = _mm_andnot_si128(write_mask, _mm_load_si128(((__m128i*)dst)+3));
        dst_3 = _mm_or_si128(dst_3, old_dst_3);

        // store
        _mm_stream_si128((__m128i*)dst, dst_0);
        _mm_stream_si128(((__m128i*)dst)+1, dst_1);
        _mm_stream_si128(((__m128i*)dst)+2, dst_2);
        _mm_stream_si128(((__m128i*)dst)+3, dst_3);

        // update the pointer
        dst += 64;
        src += 2*pitch;
    }
}

__forceinline void SwizzleBlock16(u8 *dst, u8 *src, int pitch)
{
#ifdef INTRINSIC_PORT_16
	SwizzleBlock16_sse2_I<true>(dst, src, pitch);
#else
	SwizzleBlock16_sse2(dst, src, pitch);
#endif
}

__forceinline void SwizzleBlock8(u8 *dst, u8 *src, int pitch)
{
#ifdef INTRINSIC_PORT_8
	SwizzleBlock8_sse2_I<true>(dst, src, pitch);
#else
	SwizzleBlock8_sse2(dst, src, pitch);
#endif
}

__forceinline void SwizzleBlock4(u8 *dst, u8 *src, int pitch)
{
#ifdef INTRINSIC_PORT_4
	SwizzleBlock4_sse2_I<true>(dst, src, pitch);
#else
	SwizzleBlock4_sse2(dst, src, pitch);
#endif
}

__forceinline void SwizzleBlock32u(u8 *dst, u8 *src, int pitch)
{
    SwizzleBlock32_sse2_I<false>(dst, src, pitch);
}

__forceinline void SwizzleBlock16u(u8 *dst, u8 *src, int pitch)
{
#ifdef INTRINSIC_PORT_16
	SwizzleBlock16_sse2_I<false>(dst, src, pitch);
#else
	SwizzleBlock16u_sse2(dst, src, pitch);
#endif
}

__forceinline void SwizzleBlock8u(u8 *dst, u8 *src, int pitch)
{
#ifdef INTRINSIC_PORT_8
	SwizzleBlock8_sse2_I<false>(dst, src, pitch);
#else
	SwizzleBlock8u_sse2(dst, src, pitch);
#endif
}

__forceinline void SwizzleBlock4u(u8 *dst, u8 *src, int pitch)
{
#ifdef INTRINSIC_PORT_4
	SwizzleBlock4_sse2_I<false>(dst, src, pitch);
#else
	SwizzleBlock4u_sse2(dst, src, pitch);
#endif
}

__forceinline void SwizzleBlock8H(u8 *dst, u8 *src, int pitch)
{
    SwizzleBlock8H_4H<false, false>(dst, src, pitch);
}

__forceinline void SwizzleBlock4HH(u8 *dst, u8 *src, int pitch)
{
    SwizzleBlock8H_4H<true, true>(dst, src, pitch);
}

__forceinline void SwizzleBlock4HL(u8 *dst, u8 *src, int pitch)
{
    SwizzleBlock8H_4H<true, false>(dst, src, pitch);
}

#else

__forceinline void SwizzleBlock32(u8 *dst, u8 *src, int pitch)
{
	SwizzleBlock32_c(dst, src, pitch);
}

__forceinline void SwizzleBlock16(u8 *dst, u8 *src, int pitch)
{
	SwizzleBlock16_c(dst, src, pitch);
}

__forceinline void SwizzleBlock8(u8 *dst, u8 *src, int pitch)
{
	SwizzleBlock8_c(dst, src, pitch);
}

__forceinline void SwizzleBlock4(u8 *dst, u8 *src, int pitch)
{
	SwizzleBlock4_c(dst, src, pitch);
}

__forceinline void SwizzleBlock32u(u8 *dst, u8 *src, int pitch)
{
	SwizzleBlock32_c(dst, src, pitch);
}

__forceinline void SwizzleBlock16u(u8 *dst, u8 *src, int pitch)
{
	SwizzleBlock16_c(dst, src, pitch);
}

__forceinline void SwizzleBlock8u(u8 *dst, u8 *src, int pitch)
{
	SwizzleBlock8_c(dst, src, pitch);
}

__forceinline void SwizzleBlock4u(u8 *dst, u8 *src, int pitch)
{
	SwizzleBlock4_c(dst, src, pitch);
}

__forceinline void __fastcall SwizzleBlock32_mask(u8* dst, u8* src, int srcpitch, u32 WriteMask)
{
	u32* d = &g_columnTable32[0][0];

	if (WriteMask == 0xffffffff)
	{
		for (int j = 0; j < 8; j++, d += 8, src += srcpitch)
			for (int i = 0; i < 8; i++)
				((u32*)dst)[d[i]] = ((u32*)src)[i];
	}
	else
	{
		for (int j = 0; j < 8; j++, d += 8, src += srcpitch)
			for (int i = 0; i < 8; i++)
				((u32*)dst)[d[i]] = (((u32*)dst)[d[i]] & ~WriteMask) | (((u32*)src)[i] & WriteMask);
	}
}

__forceinline void __fastcall SwizzleBlock32_c(u8* dst, u8* src, int srcpitch)
{
    SwizzleBlock32_mask(dst, src, srcpitch, 0xffffffff);
}

__forceinline void __fastcall SwizzleBlock16_c(u8* dst, u8* src, int srcpitch)
{
	u32* d = &g_columnTable16[0][0];

	for (int j = 0; j < 8; j++, d += 16, src += srcpitch)
		for (int i = 0; i < 16; i++)
			((u16*)dst)[d[i]] = ((u16*)src)[i];
}

__forceinline void __fastcall SwizzleBlock8_c(u8* dst, u8* src, int srcpitch)
{
	u32* d = &g_columnTable8[0][0];

	for (int j = 0; j < 16; j++, d += 16, src += srcpitch)
		for (int i = 0; i < 16; i++)
			dst[d[i]] = src[i];
}

__forceinline void __fastcall SwizzleBlock4_c(u8* dst, u8* src, int srcpitch)
{
	u32* d = &g_columnTable4[0][0];

	for (int j = 0; j < 16; j++, d += 32, src += srcpitch)
	{
		for (int i = 0; i < 32; i++)
		{
			u32 addr = d[i];
			u8 c = (src[i>>1] >> ((i & 1) << 2)) & 0x0f;
			u32 shift = (addr & 1) << 2;
			dst[addr >> 1] = (dst[addr >> 1] & (0xf0 >> shift)) | (c << shift);
		}
	}
}

__forceinline void SwizzleBlock24(u8 *dst, u8 *src, int pitch)
{
	u8* pnewsrc = src;
	u32* pblock = tempblock;

    //  Note src can be out of bound of GS memory (but there is some spare allocation
    //  to avoid a tricky corner case)
	for (int by = 0; by < 8; ++by, pblock += 8, pnewsrc += pitch - 24)
	{
		for (int bx = 0; bx < 8; ++bx, pnewsrc += 3)
		{
			pblock[bx] = *(u32*)pnewsrc;
		}
	}

	SwizzleBlock32_mask((u8*)dst, (u8*)tempblock, 32, 0x00ffffff);
}

__forceinline void SwizzleBlock8H(u8 *dst, u8 *src, int pitch)
{
	u8* pnewsrc = src;
	u32* pblock = tempblock;

	for (int by = 0; by < 8; ++by, pblock += 8, pnewsrc += pitch)
	{
		u32 u = *(u32*)pnewsrc;
		pblock[0] = u << 24;
		pblock[1] = u << 16;
		pblock[2] = u << 8;
		pblock[3] = u;
		u = *(u32*)(pnewsrc + 4);
		pblock[4] = u << 24;
		pblock[5] = u << 16;
		pblock[6] = u << 8;
		pblock[7] = u;
	}

	SwizzleBlock32_mask((u8*)dst, (u8*)tempblock, 32, 0xff000000);
}

__forceinline void SwizzleBlock4HH(u8 *dst, u8 *src, int pitch)
{
	u8* pnewsrc = src;
	u32* pblock = tempblock;

	for (int by = 0; by < 8; ++by, pblock += 8, pnewsrc += pitch)
	{
		u32 u = *(u32*)pnewsrc;
		pblock[0] = u << 28;
		pblock[1] = u << 24;
		pblock[2] = u << 20;
		pblock[3] = u << 16;
		pblock[4] = u << 12;
		pblock[5] = u << 8;
		pblock[6] = u << 4;
		pblock[7] = u;
	}

	SwizzleBlock32_mask((u8*)dst, (u8*)tempblock, 32, 0xf0000000);
}

__forceinline void SwizzleBlock4HL(u8 *dst, u8 *src, int pitch)
{
	u8* pnewsrc = src;
	u32* pblock = tempblock;

	for (int by = 0; by < 8; ++by, pblock += 8, pnewsrc += pitch)
	{
		u32 u = *(u32*)pnewsrc;
		pblock[0] = u << 24;
		pblock[1] = u << 20;
		pblock[2] = u << 16;
		pblock[3] = u << 12;
		pblock[4] = u << 8;
		pblock[5] = u << 4;
		pblock[6] = u;
		pblock[7] = u >> 4;
	}

	SwizzleBlock32_mask((u8*)dst, (u8*)tempblock, 32, 0x0f000000);
}
#endif
