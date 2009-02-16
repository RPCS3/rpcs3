/**
*** Copyright (C) 1985-2007 Intel Corporation.  All rights reserved.
***
*** The information and source code contained herein is the exclusive
*** property of Intel Corporation and may not be disclosed, examined
*** or reproduced in whole or in part without explicit written authorization
*** from the company.
***
**/

/*
 * smmintrin.h
 *
 * Principal header file for Intel(R) Core(TM) 2 Duo processor
 * SSE4.1 intrinsics
 */

// Gsdx Note: This header file has been "borrowed" from the MSVC install and bugfixed to
// allow for proper code compilation.  The original version of the header includes semicolons
// after several macros defined below, which causes compiler errors when using them in
// inline object construction situations. -- Air

#pragma once
#ifndef __midl
#ifndef _INCLUDED_SMM
#define _INCLUDED_SMM

#if defined(_M_CEE_PURE)
        #error ERROR: EMM intrinsics not supported in the pure mode!
#else

#include <tmmintrin.h>


/*
 * Rounding mode macros
 */

#define _MM_FROUND_TO_NEAREST_INT    0x00
#define _MM_FROUND_TO_NEG_INF        0x01
#define _MM_FROUND_TO_POS_INF        0x02
#define _MM_FROUND_TO_ZERO           0x03
#define _MM_FROUND_CUR_DIRECTION     0x04

#define _MM_FROUND_RAISE_EXC         0x00
#define _MM_FROUND_NO_EXC            0x08

#define _MM_FROUND_NINT      _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_RAISE_EXC
#define _MM_FROUND_FLOOR     _MM_FROUND_TO_NEG_INF     | _MM_FROUND_RAISE_EXC
#define _MM_FROUND_CEIL      _MM_FROUND_TO_POS_INF     | _MM_FROUND_RAISE_EXC
#define _MM_FROUND_TRUNC     _MM_FROUND_TO_ZERO        | _MM_FROUND_RAISE_EXC
#define _MM_FROUND_RINT      _MM_FROUND_CUR_DIRECTION  | _MM_FROUND_RAISE_EXC
#define _MM_FROUND_NEARBYINT _MM_FROUND_CUR_DIRECTION  | _MM_FROUND_NO_EXC

/*
 * MACRO functions for ceil/floor intrinsics
 */

#define _mm_ceil_pd(val)       _mm_round_pd((val), _MM_FROUND_CEIL)
#define _mm_ceil_sd(dst, val)  _mm_round_sd((dst), (val), _MM_FROUND_CEIL)

#define _mm_floor_pd(val)      _mm_round_pd((val), _MM_FROUND_FLOOR)
#define _mm_floor_sd(dst, val) _mm_round_sd((dst), (val), _MM_FROUND_FLOOR)

#define _mm_ceil_ps(val)       _mm_round_ps((val), _MM_FROUND_CEIL)
#define _mm_ceil_ss(dst, val)  _mm_round_ss((dst), (val), _MM_FROUND_CEIL)

#define _mm_floor_ps(val)      _mm_round_ps((val), _MM_FROUND_FLOOR)
#define _mm_floor_ss(dst, val) _mm_round_ss((dst), (val), _MM_FROUND_FLOOR)

#define _mm_test_all_zeros(mask, val)      _mm_testz_si128((mask), (val))

/*
 * MACRO functions for packed integer 128-bit comparison intrinsics.
 */

#define _mm_test_all_ones(val) \
              _mm_testc_si128((val), _mm_cmpeq_epi32((val),(val)))

#define _mm_test_mix_ones_zeros(mask, val) _mm_testnzc_si128((mask), (val))

#if __cplusplus
extern "C" {
#endif

        // Integer blend instructions - select data from 2 sources
        // using constant/variable mask

        extern __m128i _mm_blend_epi16 (__m128i v1, __m128i v2,
                                        const int mask);
        extern __m128i _mm_blendv_epi8 (__m128i v1, __m128i v2, __m128i mask);

        // Float single precision blend instructions - select data
        // from 2 sources using constant/variable mask

        extern __m128  _mm_blend_ps (__m128  v1, __m128  v2, const int mask);
        extern __m128  _mm_blendv_ps(__m128  v1, __m128  v2, __m128 v3);

        // Float double precision blend instructions - select data
        // from 2 sources using constant/variable mask

        extern __m128d _mm_blend_pd (__m128d v1, __m128d v2, const int mask);
        extern __m128d _mm_blendv_pd(__m128d v1, __m128d v2, __m128d v3);

        // Dot product instructions with mask-defined summing and zeroing
        // of result's parts

        extern __m128  _mm_dp_ps(__m128  val1, __m128  val2, const int mask);
        extern __m128d _mm_dp_pd(__m128d val1, __m128d val2, const int mask);

        // Packed integer 64-bit comparison, zeroing or filling with ones
        // corresponding parts of result

        extern __m128i _mm_cmpeq_epi64(__m128i val1, __m128i val2);

        // Min/max packed integer instructions

        extern __m128i _mm_min_epi8 (__m128i val1, __m128i val2);
        extern __m128i _mm_max_epi8 (__m128i val1, __m128i val2);

        extern __m128i _mm_min_epu16(__m128i val1, __m128i val2);
        extern __m128i _mm_max_epu16(__m128i val1, __m128i val2);

        extern __m128i _mm_min_epi32(__m128i val1, __m128i val2);
        extern __m128i _mm_max_epi32(__m128i val1, __m128i val2);
        extern __m128i _mm_min_epu32(__m128i val1, __m128i val2);
        extern __m128i _mm_max_epu32(__m128i val1, __m128i val2);

        // Packed integer 32-bit multiplication with truncation
        // of upper halves of results

        extern __m128i _mm_mullo_epi32(__m128i a, __m128i b);

        // Packed integer 32-bit multiplication of 2 pairs of operands
        // producing two 64-bit results

        extern __m128i _mm_mul_epi32(__m128i a, __m128i b);

        // Packed integer 128-bit bitwise comparison.
        // return 1 if (val 'and' mask) == 0

        extern int _mm_testz_si128(__m128i mask, __m128i val);

        // Packed integer 128-bit bitwise comparison.
        // return 1 if (val 'and_not' mask) == 0

        extern int _mm_testc_si128(__m128i mask, __m128i val);

        // Packed integer 128-bit bitwise comparison
        // ZF = ((val 'and' mask) == 0)  CF = ((val 'and_not' mask) == 0)
        // return 1 if both ZF and CF are 0

        extern int _mm_testnzc_si128(__m128i mask, __m128i s2);

        // Insert single precision float into packed single precision
        // array element selected by index.
        // The bits [7-6] of the 3d parameter define src index,
        // the bits [5-4] define dst index, and bits [3-0] define zeroing
        // mask for dst

        extern __m128 _mm_insert_ps(__m128 dst, __m128 src, const int ndx);

        // Helper macro to create ndx-parameter value for _mm_insert_ps

#define _MM_MK_INSERTPS_NDX(srcField, dstField, zeroMask) \
        (((srcField)<<6) | ((dstField)<<4) | (zeroMask))

        // Extract binary representation of single precision float from
        // packed single precision array element selected by index

        extern int _mm_extract_ps(__m128 src, const int ndx);

        // Extract single precision float from packed single precision
        // array element selected by index into dest

#define _MM_EXTRACT_FLOAT(dest, src, ndx) \
        *((int*)&(dest)) = _mm_extract_ps((src), (ndx))

        // Extract specified single precision float element
        // into the lower part of __m128

#define _MM_PICK_OUT_PS(src, num) \
        _mm_insert_ps(_mm_setzero_ps(), (src), \
                      _MM_MK_INSERTPS_NDX((num), 0, 0x0e));

        // Insert integer into packed integer array element
        // selected by index

        extern __m128i _mm_insert_epi8 (__m128i dst, int s, const int ndx);
        extern __m128i _mm_insert_epi32(__m128i dst, int s, const int ndx);

#if defined(_M_X64)
        extern __m128i _mm_insert_epi64(__m128i dst, __int64 s, const int ndx);
#endif
        // Extract integer from packed integer array element
        // selected by index

        extern int   _mm_extract_epi8 (__m128i src, const int ndx);
        extern int   _mm_extract_epi32(__m128i src, const int ndx);

#if defined(_M_X64)
        extern __int64 _mm_extract_epi64(__m128i src, const int ndx);
#endif

        // Horizontal packed word minimum and its index in
        // result[15:0] and result[18:16] respectively

        extern __m128i _mm_minpos_epu16(__m128i shortValues);

        // Packed/single float double precision rounding

        extern __m128d _mm_round_pd(__m128d val, int iRoundMode);
        extern __m128d _mm_round_sd(__m128d dst, __m128d val, int iRoundMode);

        // Packed/single float single precision rounding

        extern __m128  _mm_round_ps(__m128  val, int iRoundMode);
        extern __m128  _mm_round_ss(__m128 dst, __m128  val, int iRoundMode);

        // Packed integer sign-extension

        extern __m128i _mm_cvtepi8_epi32 (__m128i byteValues);
        extern __m128i _mm_cvtepi16_epi32(__m128i shortValues);
        extern __m128i _mm_cvtepi8_epi64 (__m128i byteValues); 
        extern __m128i _mm_cvtepi32_epi64(__m128i intValues);
        extern __m128i _mm_cvtepi16_epi64(__m128i shortValues);
        extern __m128i _mm_cvtepi8_epi16 (__m128i byteValues);

        // Packed integer zero-extension

        extern __m128i _mm_cvtepu8_epi32 (__m128i byteValues);
        extern __m128i _mm_cvtepu16_epi32(__m128i shortValues);
        extern __m128i _mm_cvtepu8_epi64 (__m128i shortValues);
        extern __m128i _mm_cvtepu32_epi64(__m128i intValues);
        extern __m128i _mm_cvtepu16_epi64(__m128i shortValues);
        extern __m128i _mm_cvtepu8_epi16 (__m128i byteValues);


        // Pack 8 double words from 2 operands into 8 words of result
        // with unsigned saturation

        extern __m128i _mm_packus_epi32(__m128i val1, __m128i val2);

        // Sum absolute 8-bit integer difference of adjacent groups of 4 byte
        // integers in operands. Starting offsets within operands are
        // determined by mask

        extern __m128i _mm_mpsadbw_epu8(__m128i s1, __m128i s2, const int msk);

        /*
         * Load double quadword using non-temporal aligned hint
         */

        extern __m128i _mm_stream_load_si128(__m128i* v1);

#if defined __cplusplus
}; /* End "C" */
#endif /* __cplusplus */

#endif /* defined(_M_CEE_PURE) */

#endif
#endif /* _INCLUDED_SMM */
