/* 
 *	Copyright (C) 2007-2009 Gabest
 *	http://www.gabest.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once

// sse2

#if _M_SSE >= 0x200

	#include <xmmintrin.h>
	#include <emmintrin.h>

	#ifndef _MM_DENORMALS_ARE_ZERO
	#define _MM_DENORMALS_ARE_ZERO 0x0040
	#endif

	#define MXCSR (_MM_DENORMALS_ARE_ZERO | _MM_MASK_MASK | _MM_ROUND_NEAREST | _MM_FLUSH_ZERO_ON)

	#if _MSC_VER < 1500

	__forceinline __m128i _mm_castps_si128(__m128 a) {return *(__m128i*)&a;}
	__forceinline __m128 _mm_castsi128_ps(__m128i a) {return *(__m128*)&a;}
	__forceinline __m128i _mm_castpd_si128(__m128d a) {return *(__m128i*)&a;}
	__forceinline __m128d _mm_castsi128_pd(__m128i a) {return *(__m128d*)&a;}
	__forceinline __m128d _mm_castps_pd(__m128 a) {return *(__m128d*)&a;}
	__forceinline __m128 _mm_castpd_ps(__m128d a) {return *(__m128*)&a;}

	#endif

	const __m128 ps_3f800000 = _mm_castsi128_ps(_mm_set1_epi32(0x3f800000));
	const __m128 ps_4b000000 = _mm_castsi128_ps(_mm_set1_epi32(0x4b000000));
	const __m128 ps_7fffffff = _mm_castsi128_ps(_mm_set1_epi32(0x7fffffff));
	const __m128 ps_80000000 = _mm_castsi128_ps(_mm_set1_epi32(0x80000000));
	const __m128 ps_ffffffff = _mm_castsi128_ps(_mm_set1_epi32(0xffffffff));

	__forceinline __m128 _mm_neg_ps(__m128 r)
	{
		return _mm_xor_ps(ps_80000000, r);
	}

	__forceinline __m128 _mm_abs_ps(__m128 r)
	{
		return _mm_and_ps(ps_7fffffff, r);
	}

	#define _MM_TRANSPOSE4_SI128(row0, row1, row2, row3) \
	{ \
		__m128 tmp0 = _mm_shuffle_ps(_mm_castsi128_ps(row0), _mm_castsi128_ps(row1), 0x44); \
		__m128 tmp2 = _mm_shuffle_ps(_mm_castsi128_ps(row0), _mm_castsi128_ps(row1), 0xEE); \
		__m128 tmp1 = _mm_shuffle_ps(_mm_castsi128_ps(row2), _mm_castsi128_ps(row3), 0x44); \
		__m128 tmp3 = _mm_shuffle_ps(_mm_castsi128_ps(row2), _mm_castsi128_ps(row3), 0xEE); \
		(row0) = _mm_castps_si128(_mm_shuffle_ps(tmp0, tmp1, 0x88)); \
		(row1) = _mm_castps_si128(_mm_shuffle_ps(tmp0, tmp1, 0xDD)); \
		(row2) = _mm_castps_si128(_mm_shuffle_ps(tmp2, tmp3, 0x88)); \
		(row3) = _mm_castps_si128(_mm_shuffle_ps(tmp2, tmp3, 0xDD)); \
	}

	__forceinline __m128 _mm_rcpnr_ps(__m128 r)
	{
	  __m128 t = _mm_rcp_ps(r);

	  return _mm_sub_ps(_mm_add_ps(t, t), _mm_mul_ps(_mm_mul_ps(t, t), r));
	}


#else

#error TODO: GSVector4 and GSRasterizer needs SSE2

#endif

// sse3

#if _M_SSE >= 0x301

	#include <tmmintrin.h>

#endif

// sse4

#if _M_SSE >= 0x401

	#include <smmintrin.h>

#else

	// not an equal replacement for sse4's blend but for our needs it is ok

	#define _mm_blendv_ps(a, b, mask) _mm_or_ps(_mm_andnot_ps(mask, a), _mm_and_ps(mask, b))
	#define _mm_blendv_epi8(a, b, mask) _mm_or_si128(_mm_andnot_si128(mask, a), _mm_and_si128(mask, b))

	__forceinline __m128 _mm_round_ps(__m128 x)
	{
		__m128 t = _mm_or_ps(_mm_and_ps(ps_80000000, x), ps_4b000000);

		return _mm_sub_ps(_mm_add_ps(x, t), t);
	}

	__forceinline __m128 _mm_floor_ps(__m128 x)
	{
		__m128 t = _mm_round_ps(x);

		return _mm_sub_ps(t, _mm_and_ps(_mm_cmplt_ps(x, t), ps_3f800000));
	}

	__forceinline __m128 _mm_ceil_ps(__m128 x)
	{
		__m128 t = _mm_round_ps(x);

		return _mm_add_ps(t, _mm_and_ps(_mm_cmpgt_ps(x, t), ps_3f800000));
	}

#endif
