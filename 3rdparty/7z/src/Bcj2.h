/* Blake2s.c -- BLAKE2sp Hash
2024-01-29 : Igor Pavlov : Public domain
2015-2019 : Samuel Neves : original code : CC0 1.0 Universal (CC0 1.0). */

#include "Precomp.h"

// #include <stdio.h>
#include <string.h>

#include "Blake2.h"
#include "RotateDefs.h"
#include "Compiler.h"
#include "CpuArch.h"

#if defined(__SSE2__)
    #define Z7_BLAKE2S_USE_VECTORS
#elif defined(MY_CPU_X86_OR_AMD64)
  #if  defined(_MSC_VER) && _MSC_VER > 1200 \
    || defined(Z7_GCC_VERSION) && (Z7_GCC_VERSION >= 30300) \
    || defined(__clang__) \
    || defined(__INTEL_COMPILER)
    #define Z7_BLAKE2S_USE_VECTORS
  #endif
#endif

#ifdef Z7_BLAKE2S_USE_VECTORS

#define Z7_BLAKE2SP_USE_FUNCTIONS

//  define Z7_BLAKE2SP_STRUCT_IS_NOT_ALIGNED, if CBlake2sp can be non aligned for 32-bytes.
// #define Z7_BLAKE2SP_STRUCT_IS_NOT_ALIGNED

// SSSE3 : for _mm_shuffle_epi8 (pshufb) that improves the performance for 5-15%.
#if defined(__SSSE3__)
  #define Z7_BLAKE2S_USE_SSSE3
#elif  defined(Z7_MSC_VER_ORIGINAL) && (Z7_MSC_VER_ORIGINAL >= 1500) \
    || defined(Z7_GCC_VERSION) && (Z7_GCC_VERSION >= 40300) \
    || defined(Z7_APPLE_CLANG_VERSION) && (Z7_APPLE_CLANG_VERSION >= 40000) \
    || defined(Z7_LLVM_CLANG_VERSION) && (Z7_LLVM_CLANG_VERSION >= 20300) \
    || defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 1000)
  #define Z7_BLAKE2S_USE_SSSE3
#endif

#ifdef Z7_BLAKE2S_USE_SSSE3
/* SSE41 : for _mm_insert_epi32 (pinsrd)
  it can slightly reduce code size and improves the performance in some cases.
    it's used only for last 512-1024 bytes, if FAST versions (2 or 3) of vector algos are used.
    it can be used for all blocks in another algos (4+).
*/
#if defined(__SSE4_1__)
  #define Z7_BLAKE2S_USE_SSE41
#elif  defined(Z7_MSC_VER_ORIGINAL) && (Z7_MSC_VER_ORIGINAL >= 1500) \
    || defined(Z7_GCC_VERSION) && (Z7_GCC_VERSION >= 40300) \
    || defined(Z7_APPLE_CLANG_VERSION) && (Z7_APPLE_CLANG_VERSION >= 40000) \
    || defined(Z7_LLVM_CLANG_VERSION) && (Z7_LLVM_CLANG_VERSION >= 20300) \
    || defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 1000)
  #define Z7_BLAKE2S_USE_SSE41
#endif
#endif // SSSE3

#if defined(__GNUC__) || defined(__clang__)
  #if defined(Z7_BLAKE2S_USE_SSE41)
    #define BLAKE2S_ATTRIB_128BIT  __attribute__((__target__("sse4.1")))
  #elif defined(Z7_BLAKE2S_USE_SSSE3)
    #define BLAKE2S_ATTRIB_128BIT  __attribute__((__target__("ssse3")))
  #else
    #define BLAKE2S_ATTRIB_128BIT  __attribute__((__target__("sse2")))
  #endif
#endif


#if defined(__AVX2__)
  #define Z7_BLAKE2S_USE_AVX2
#else
  #if    defined(Z7_GCC_VERSION) && (Z7_GCC_VERSION >= 40900) \
      || defined(Z7_APPLE_CLANG_VERSION) && (Z7_APPLE_CLANG_VERSION >= 40600) \
      || defined(Z7_LLVM_CLANG_VERSION) && (Z7_LLVM_CLANG_VERSION >= 30100)
    #define Z7_BLAKE2S_USE_AVX2
    #ifdef Z7_BLAKE2S_USE_AVX2
      #define BLAKE2S_ATTRIB_AVX2  __attribute__((__target__("avx2")))
    #endif
  #elif  defined(Z7_MSC_VER_ORIGINAL) && (Z7_MSC_VER_ORIGINAL >= 1800) \
      || defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 1400)
    #if (Z7_MSC_VER_ORIGINAL == 1900)
      #pragma warning(disable : 4752) // found Intel(R) Advanced Vector Extensions; consider using /arch:AVX
    #endif
    #define Z7_BLAKE2S_USE_AVX2
  #endif
#endif

#ifdef Z7_BLAKE2S_USE_SSE41
#include <smmintrin.h> // SSE4.1
#elif defined(Z7_BLAKE2S_USE_SSSE3)
#include <tmmintrin.h> // SSSE3
#else
#include <emmintrin.h> // SSE2
#endif

#ifdef Z7_BLAKE2S_USE_AVX2
#include <immintrin.h>
#if defined(__clang__)
#include <avxintrin.h>
#include <avx2intrin.h>
#endif
#endif // avx2


#if defined(__AVX512F__) && defined(__AVX512VL__)
   // && defined(Z7_MSC_VER_ORIGINAL) && (Z7_MSC_VER_ORIGINAL > 1930)
  #define Z7_BLAKE2S_USE_AVX512_ALWAYS
  // #pragma message ("=== Blake2s AVX512")
#endif


#define Z7_BLAKE2S_USE_V128_FAST
// for speed optimization for small messages:
// #define Z7_BLAKE2S_USE_V128_WAY2

#ifdef Z7_BLAKE2S_USE_AVX2

// for debug:
// gather is slow
// #define Z7_BLAKE2S_USE_GATHER

  #define   Z7_BLAKE2S_USE_AVX2_FAST
// for speed optimization for small messages:
//   #define   Z7_BLAKE2S_USE_AVX2_WAY2
//   #define   Z7_BLAKE2S_USE_AVX2_WAY4
#if defined(Z7_BLAKE2S_USE_AVX2_WAY2) || \
    defined(Z7_BLAKE2S_USE_AVX2_WAY4)
  #define   Z7_BLAKE2S_USE_AVX2_WAY_SLOW
#endif
#endif

  #define Z7_BLAKE2SP_ALGO_DEFAULT    0
  #define Z7_BLAKE2SP_ALGO_SCALAR     1
#ifdef Z7_BLAKE2S_USE_V128_FAST
  #define Z7_BLAKE2SP_ALGO_V128_FAST  2
#endif
#ifdef Z7_BLAKE2S_USE_AVX2_FAST
  #define Z7_BLAKE2SP_ALGO_V256_FAST  3
#endif
  #define Z7_BLAKE2SP_ALGO_V128_WAY1  4
#ifdef Z7_BLAKE2S_USE_V128_WAY2
  #define Z7_BLAKE2SP_ALGO_V128_WAY2  5
#endif
#ifdef Z7_BLAKE2S_USE_AVX2_WAY2
  #define Z7_BLAKE2SP_ALGO_V256_WAY2  6
#endif
#ifdef Z7_BLAKE2S_USE_AVX2_WAY4
  #define Z7_BLAKE2SP_ALGO_V256_WAY4  7
#endif

#endif // Z7_BLAKE2S_USE_VECTORS




#define BLAKE2S_FINAL_FLAG  (~(UInt32)0)
#define NSW                 Z7_BLAKE2SP_NUM_STRUCT_WORDS
#define SUPER_BLOCK_SIZE    (Z7_BLAKE2S_BLOCK_SIZE * Z7_BLAKE2SP_PARALLEL_DEGREE)
#define SUPER_BLOCK_MASK    (SUPER_BLOCK_SIZE - 1)

#define V_INDEX_0_0   0
#define V_INDEX_1_0   1
#define V_INDEX_2_0   2
#define V_INDEX_3_0   3
#define V_INDEX_0_1   4
#define V_INDEX_1_1   5
#define V_INDEX_2_1   6
#define V_INDEX_3_1   7
#define V_INDEX_0_2   8
#define V_INDEX_1_2   9
#define V_INDEX_2_2  10
#define V_INDEX_3_2  11
#define V_INDEX_0_3  12
#define V_INDEX_1_3  13
#define V_INDEX_2_3  14
#define V_INDEX_3_3  15
#define V_INDEX_4_0   0
#define V_INDEX_5_0   1
#define V_INDEX_6_0   2
#define V_INDEX_7_0   3
#define V_INDEX_7_1   4
#define V_INDEX_4_1   5
#define V_INDEX_5_1   6
#define V_INDEX_6_1   7
#define V_INDEX_6_2   8
#define V_INDEX_7_2   9
#define V_INDEX_4_2  10
#define V_INDEX_5_2  11
#define V_INDEX_5_3  12
#define V_INDEX_6_3  13
#define V_INDEX_7_3  14
#define V_INDEX_4_3  15

#define V(row, col)  v[V_INDEX_ ## row ## _ ## col]

#define k_Blake2s_IV_0  0x6A09E667UL
#define k_Blake2s_IV_1  0xBB67AE85UL
#define k_Blake2s_IV_2  0x3C6EF372UL
#define k_Blake2s_IV_3  0xA54FF53AUL
#define k_Blake2s_IV_4  0x510E527FUL
#define k_Blake2s_IV_5  0x9B05688CUL
#define k_Blake2s_IV_6  0x1F83D9ABUL
#define k_Blake2s_IV_7  0x5BE0CD19UL

#define KIV(n)  (k_Blake2s_IV_## n)

#ifdef Z7_BLAKE2S_USE_VECTORS
MY_ALIGN(16)
static const UInt32 k_Blake2s_IV[8] =
{
  KIV(0), KIV(1), KIV(2), KIV(3), KIV(4), KIV(5), KIV(6), KIV(7)
};
#endif

#define STATE_T(s)        ((s) + 8)
#define STATE_F(s)        ((s) + 10)

#ifdef Z7_BLAKE2S_USE_VECTORS

#define LOAD_128(p)    _mm_load_si128 ((const __m128i *)(const void *)(p))
#define LOADU_128(p)   _mm_loadu_si128((const __m128i *)(const void *)(p))
#ifdef Z7_BLAKE2SP_STRUCT_IS_NOT_ALIGNED
  // here we use unaligned load and stores
  // use this branch if CBlake2sp can be unaligned for 16 bytes
  #define STOREU_128(p, r)  _mm_storeu_si128((__m128i *)(void *)(p), r)
  #define LOAD_128_FROM_STRUCT(p)     LOADU_128(p)
  #define STORE_128_TO_STRUCT(p, r)   STOREU_128(p, r)
#else
  // here we use aligned load and stores
  // use this branch if CBlake2sp is aligned for 16 bytes
  #define STORE_128(p, r)  _mm_store_si128((__m128i *)(void *)(p), r)
  #define LOAD_128_FROM_STRUCT(p)     LOAD_128(p)
  #define STORE_128_TO_STRUCT(p, r)   STORE_128(p, r)
#endif

#endif // Z7_BLAKE2S_USE_VECTORS


#if 0
static void PrintState(const UInt32 *s, unsigned num)
{
  unsigned i;
  printf("\n");
  for (i = 0; i < num; i++)
    printf(" %08x", (unsigned)s[i]);
}
static void PrintStates2(const UInt32 *s, unsigned x, unsigned y)
{
  unsigned i;
  for (i = 0; i < y; i++)
    PrintState(s + i * x, x);
  printf("\n");
}
#endif


#define REP8_MACRO(m)  { m(0) m(1) m(2) m(3) m(4) m(5) m(6) m(7) }

#define BLAKE2S_NUM_ROUNDS  10

#if defined(Z7_BLAKE2S_USE_VECTORS)
#define ROUNDS_LOOP(mac) \
  { unsigned r; for (r = 0; r < BLAKE2S_NUM_ROUNDS; r++) mac(r) }
#endif
/*
#define ROUNDS_LOOP_2(mac) \
  { unsigned r; for (r = 0; r < BLAKE2S_NUM_ROUNDS; r += 2) { mac(r) mac(r + 1) } }
*/
#if 0 || 1 && !defined(Z7_BLAKE2S_USE_VECTORS)
#define ROUNDS_LOOP_UNROLLED(m) \
  { m(0) m(1) m(2) m(3) m(4) m(5) m(6) m(7) m(8) m(9) }
#endif

#define SIGMA_TABLE(M) \
  M(  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15 ), \
  M( 14, 10,  4,  8,  9, 15, 13,  6,  1, 12,  0,  2, 11,  7,  5,  3 ), \
  M( 11,  8, 12,  0,  5,  2, 15, 13, 10, 14,  3,  6,  7,  1,  9,  4 ), \
  M(  7,  9,  3,  1, 13, 12, 11, 14,  2,  6,  5, 10,  4,  0, 15,  8 ), \
  M(  9,  0,  5,  7,  2,  4, 10, 15, 14,  1, 11, 12,  6,  8,  3, 13 ), \
  M(  2, 12,  6, 10,  0, 11,  8,  3,  4, 13,  7,  5, 15, 14,  1,  9 ), \
  M( 12,  5,  1, 15, 14, 13,  4, 10,  0,  7,  6,  3,  9,  2,  8, 11 ), \
  M( 13, 11,  7, 14, 12,  1,  3,  9,  5,  0, 15,  4,  8,  6,  2, 10 ), \
  M(  6, 15, 14,  9, 11,  3,  0,  8, 12,  2, 13,  7,  1,  4, 10,  5 ), \
  M( 10,  2,  8,  4,  7,  6,  1,  5, 15, 11,  9, 14,  3, 12, 13,  0 )

#define SIGMA_TABLE_MULT(m, a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15) \
  { a0*m,a1*m,a2*m,a3*m,a4*m,a5*m,a6*m,a7*m,a8*m,a9*m,a10*m,a11*m,a12*m,a13*m,a14*m,a15*m }
#define SIGMA_TABLE_MULT_4( a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15) \
        SIGMA_TABLE_MULT(4, a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15)

// MY_ALIGN(32)
MY_ALIGN(16)
static const Byte k_Blake2s_Sigma_4[BLAKE2S_NUM_ROUNDS][16] =
  { SIGMA_TABLE(SIGMA_TABLE_MULT_4) };

#define GET_SIGMA_PTR(p, index) \
    ((const void *)((const Byte *)(const void *)(p) + (index)))

#define GET_STATE_TABLE_PTR_FROM_BYTE_POS(s, pos) \
    ((UInt32 *)(void *)((Byte *)(void *)(s) + (pos)))


#ifdef Z7_BLAKE2S_USE_VECTORS


#if 0
  // use loading constants from memory
  // is faster for some compilers.
  #define KK4(n)  KIV(n), KIV(n), KIV(n), KIV(n)
MY_ALIGN(64)
static const UInt32 k_Blake2s_IV_WAY4[]=
{
  KK4(0), KK4(1), KK4(2), KK4(3), KK4(4), KK4(5), KK4(6), KK4(7)
};
  #define GET_128_IV_WAY4(i)  LOAD_128(k_Blake2s_IV_WAY4 + 4 * (i))
#else
  // use constant generation:
  #define GET_128_IV_WAY4(i)  _mm_set1_epi32((Int32)KIV(i))
#endif


#ifdef Z7_BLAKE2S_USE_AVX2_WAY_SLOW
#define GET_CONST_128_FROM_ARRAY32(k) \
    _mm_set_epi32((Int32)(k)[3], (Int32)(k)[2], (Int32)(k)[1], (Int32)(k)[0])
#endif


#if 0
#define k_r8    _mm_set_epi8(12, 15, 14, 13, 8, 11, 10, 9, 4, 7, 6, 5, 0, 3, 2, 1)
#define k_r16   _mm_set_epi8(13, 12, 15, 14, 9, 8, 11, 10, 5, 4, 7, 6, 1, 0, 3, 2)
#define k_inc   _mm_set_epi32(0, 0, 0, Z7_BLAKE2S_BLOCK_SIZE)
#define k_iv0_128  GET_CONST_128_FROM_ARRAY32(k_Blake2s_IV + 0)
#define k_iv4_128  GET_CONST_128_FROM_ARRAY32(k_Blake2s_IV + 4)
#else
#if  defined(Z7_BLAKE2S_USE_SSSE3) && \
    !defined(Z7_BLAKE2S_USE_AVX512_ALWAYS)
MY_ALIGN(16) static const Byte k_r8_arr [16] = { 1, 2, 3, 0, 5, 6, 7, 4, 9, 10, 11, 8 ,13, 14, 15, 12 };
MY_ALIGN(16) static const Byte k_r16_arr[16] = { 2, 3, 0, 1, 6, 7, 4, 5, 10, 11, 8, 9, 14, 15, 12, 13 };
#define k_r8    LOAD_128(k_r8_arr)
#define k_r16   LOAD_128(k_r16_arr)
#endif
MY_ALIGN(16) static const UInt32 k_inc_arr[4] = { Z7_BLAKE2S_BLOCK_SIZE, 0, 0, 0 };
#define k_inc   LOAD_128(k_inc_arr)
#define k_iv0_128  LOAD_128(k_Blake2s_IV + 0)
#define k_iv4_128  LOAD_128(k_Blake2s_IV + 4)
#endif


#ifdef Z7_BLAKE2S_USE_AVX2_WAY_SLOW

#ifdef Z7_BLAKE2S_USE_AVX2
#if defined(Z7_GCC_VERSION) && (Z7_GCC_VERSION < 80000)
  #define MY_mm256_set_m128i(hi, lo)  _mm256_insertf128_si256(_mm256_castsi128_si256(lo), (hi), 1)
#else
  #define MY_mm256_set_m128i  _mm256_set_m128i
#endif

#define SET_FROM_128(a)  MY_mm256_set_m128i(a, a)

#ifndef Z7_BLAKE2S_USE_AVX512_ALWAYS
MY_ALIGN(32) static const Byte k_r8_arr_256 [32] =
{
  1, 2, 3, 0, 5, 6, 7, 4, 9, 10, 11, 8 ,13, 14, 15, 12,
  1, 2, 3, 0, 5, 6, 7, 4, 9, 10, 11, 8 ,13, 14, 15, 12
};
MY_ALIGN(32) static const Byte k_r16_arr_256[32] =
{
  2, 3, 0, 1, 6, 7, 4, 5, 10, 11, 8, 9, 14, 15, 12, 13,
  2, 3, 0, 1, 6, 7, 4, 5, 10, 11, 8, 9, 14, 15, 12, 13
};
#define k_r8_256    LOAD_256(k_r8_arr_256)
#define k_r16_256   LOAD_256(k_r16_arr_256)
#endif

// #define k_r8_256    SET_FROM_128(_mm_set_epi8(12, 15, 14, 13, 8, 11, 10, 9, 4, 7, 6, 5, 0, 3, 2, 1))
// #define k_r16_256   SET_FROM_128(_mm_set_epi8(13, 12, 15, 14, 9, 8, 11, 10, 5, 4, 7, 6, 1, 0, 3, 2))
// #define k_inc_256   SET_FROM_128(_mm_set_epi32(0, 0, 0, Z7_BLAKE2S_BLOCK_SIZE))
// #define k_iv0_256   SET_FROM_128(GET_CONST_128_FROM_ARRAY32(k_Blake2s_IV + 0))
#define k_iv4_256   SET_FROM_128(GET_CONST_128_FROM_ARRAY32(k_Blake2s_IV + 4))
#endif // Z7_BLAKE2S_USE_AVX2_WAY_SLOW
#endif


/*
IPC(TP) ports:
1 p__5  : skl-      : SSE   : shufps  : _mm_shuffle_ps
2 p_15  : icl+
1 p__5  : nhm-bdw   : SSE   : xorps   : _mm_xor_ps
3 p015  : skl+

3 p015              : SSE2  : pxor    : _mm_xor_si128
2 p_15:   snb-bdw   : SSE2  : padd    : _mm_add_epi32
2 p0_5:   mrm-wsm   :
3 p015  : skl+

2 p_15  : ivb-,icl+ : SSE2  : punpcklqdq, punpckhqdq, punpckldq, punpckhdq
2 p_15  :           : SSE2  : pshufd  : _mm_shuffle_epi32
2 p_15  :           : SSE2  : pshuflw : _mm_shufflelo_epi16
2 p_15  :           : SSE2  : psrldq  :
2 p_15  :           : SSE3  : pshufb  : _mm_shuffle_epi8
2 p_15  :           : SSE4  : pblendw : _mm_blend_epi16
1 p__5  : hsw-skl   : *

1 p0                : SSE2  : pslld (i8) : _mm_slli_si128
2 p01   : skl+      :

2 p_15  : ivb-      : SSE3  : palignr
1 p__5  : hsw+

2 p_15 + p23 : ivb-, icl+ : SSE4   : pinsrd  : _mm_insert_epi32(xmm, m32, i8)
1 p__5 + p23 : hsw-skl
1 p_15 + p5  : ivb-, ice+ : SSE4   : pinsrd  : _mm_insert_epi32(xmm, r32, i8)
0.5    2*p5  : hsw-skl

2 p23               : SSE2   : movd (m32)
3 p23A  : adl       :
1 p5:               : SSE2   : movd (r32)
*/

#if 0 && defined(__XOP__)
// we must debug and test __XOP__ instruction
#include <x86intrin.h>
#include <ammintrin.h>
    #define LOAD_ROTATE_CONSTS
    #define MM_ROR_EPI32(r, c)  _mm_roti_epi32(r, -(c))
    #define Z7_BLAKE2S_MM_ROR_EPI32_IS_SUPPORTED
#elif 1 && defined(Z7_BLAKE2S_USE_AVX512_ALWAYS)
    #define LOAD_ROTATE_CONSTS
    #define MM_ROR_EPI32(r, c)  _mm_ror_epi32(r, c)
    #define Z7_BLAKE2S_MM_ROR_EPI32_IS_SUPPORTED
#else

// MSVC_1937+ uses "orps" instruction for _mm_or_si128().
// But "orps" has low throughput: TP=1 for bdw-nhm.
// So it can be better to use _mm_add_epi32()/"paddd" (TP=2 for bdw-nhm) instead of "xorps".
// But "orps" is fast for modern cpus (skl+).
// So we are default with "or" version:
#if 0 || 0 && defined(Z7_MSC_VER_ORIGINAL) && Z7_MSC_VER_ORIGINAL > 1937
  // minor optimization for some old cpus, if "xorps" is slow.
  #define MM128_EPI32_OR_or_ADD  _mm_add_epi32
#else
  #define MM128_EPI32_OR_or_ADD  _mm_or_si128
#endif

  #define MM_ROR_EPI32_VIA_SHIFT(r, c)( \
    MM128_EPI32_OR_or_ADD( \
      _mm_srli_epi32((r), (c)), \
      _mm_slli_epi32((r), 32-(c))))
  #if defined(Z7_BLAKE2S_USE_SSSE3) || defined(Z7_BLAKE2S_USE_SSE41)
    #define LOAD_ROTATE_CONSTS \
      const __m128i  r8 = k_r8; \
      const __m128i r16 = k_r16;
    #define MM_ROR_EPI32(r, c) ( \
      ( 8==(c)) ? _mm_shuffle_epi8(r,r8) \
    : (16==(c)) ? _mm_shuffle_epi8(r,r16) \
    : MM_ROR_EPI32_VIA_SHIFT(r, c))
  #else
    #define LOAD_ROTATE_CONSTS
    #define  MM_ROR_EPI32(r, c) ( \
      (16==(c)) ? _mm_shufflehi_epi16(_mm_shufflelo_epi16(r, 0xb1), 0xb1) \
    : MM_ROR_EPI32_VIA_SHIFT(r, c))
  #endif
#endif

/*
we have 3 main ways to load 4 32-bit integers to __m128i:
  1) SSE2:  _mm_set_epi32()
  2) SSE2:  _mm_unpacklo_epi64() / _mm_unpacklo_epi32 / _mm_cvtsi32_si128()
  3) SSE41: _mm_insert_epi32() and _mm_cvtsi32_si128()
good compiler for _mm_set_epi32() generates these instructions:
{
  movd xmm, [m32]; vpunpckldq; vpunpckldq; vpunpcklqdq;
}
good new compiler generates one instruction
{
  for _mm_insert_epi32()  : { pinsrd xmm, [m32], i }
  for _mm_cvtsi32_si128() : { movd xmm, [m32] }
}
but vc2010 generates slow pair of instructions:
{
  for _mm_insert_epi32()  : { mov r32, [m32];  pinsrd xmm, r32, i  }
  for _mm_cvtsi32_si128() : { mov r32, [m32];  movd  xmm, r32 }
}
_mm_insert_epi32() (pinsrd) code reduces xmm register pressure
in comparison with _mm_set_epi32() (movd + vpunpckld) code.
Note that variant with "movd xmm, r32" can be more slow,
but register pressure can be more important.
So we can force to "pinsrd" always.
*/
// #if !defined(Z7_MSC_VER_ORIGINAL) || Z7_MSC_VER_ORIGINAL > 1600 || defined(MY_CPU_X86)
#ifdef Z7_BLAKE2S_USE_SSE41
  /* _mm_set_epi32() can be more effective for GCC and CLANG
     _mm_insert_epi32()  is more effective for MSVC */
  #if 0 || 1 && defined(Z7_MSC_VER_ORIGINAL)
    #define Z7_BLAKE2S_USE_INSERT_INSTRUCTION
  #endif
#endif // USE_SSE41
// #endif

#ifdef Z7_BLAKE2S_USE_INSERT_INSTRUCTION
  // for SSE4.1
#define MM_LOAD_EPI32_FROM_4_POINTERS(p0, p1, p2, p3)  \
    _mm_insert_epi32( \
    _mm_insert_epi32( \
    _mm_insert_epi32( \
    _mm_cvtsi32_si128( \
        *(const Int32 *)p0), \
        *(const Int32 *)p1, 1), \
        *(const Int32 *)p2, 2), \
        *(const Int32 *)p3, 3)
#elif 0 || 1 && defined(Z7_MSC_VER_ORIGINAL)
/* MSVC 1400 implements _mm_set_epi32() via slow memory write/read.
   Also _mm_unpacklo_epi32 is more effective for another MSVC compilers.
   But _mm_set_epi32() is more effective for GCC and CLANG.
   So we use _mm_unpacklo_epi32 for MSVC only */
#define MM_LOAD_EPI32_FROM_4_POINTERS(p0, p1, p2, p3)  \
    _mm_unpacklo_epi64(  \
        _mm_unpacklo_epi32( _mm_cvtsi32_si128(*(const Int32 *)p0),  \
                            _mm_cvtsi32_si128(*(const Int32 *)p1)), \
        _mm_unpacklo_epi32( _mm_cvtsi32_si128(*(const Int32 *)p2),  \
                            _mm_cvtsi32_si128(*(const Int32 *)p3)))
#else
#define MM_LOAD_EPI32_FROM_4_POINTERS(p0, p1, p2, p3)  \
    _mm_set_epi32( \
        *(const Int32 *)p3, \
        *(const Int32 *)p2, \
        *(const Int32 *)p1, \
        *(const Int32 *)p0)
#endif

#define SET_ROW_FROM_SIGMA_BASE(input, i0, i1, i2, i3)  \
      MM_LOAD_EPI32_FROM_4_POINTERS( \
        GET_SIGMA_PTR(input, i0), \
        GET_SIGMA_PTR(input, i1), \
        GET_SIGMA_PTR(input, i2), \
        GET_SIGMA_PTR(input, i3))

#define SET_ROW_FROM_SIGMA(input, sigma_index)  \
        SET_ROW_FROM_SIGMA_BASE(input, \
            sigma[(sigma_index)        ], \
            sigma[(sigma_index) + 2 * 1], \
            sigma[(sigma_index) + 2 * 2], \
            sigma[(sigma_index) + 2 * 3]) \


#define ADD_128(a, b)   _mm_add_epi32(a, b)
#define XOR_128(a, b)   _mm_xor_si128(a, b)

#define D_ADD_128(dest, src)        dest = ADD_128(dest, src)
#define D_XOR_128(dest, src)        dest = XOR_128(dest, src)
#define D_ROR_128(dest, shift)      dest = MM_ROR_EPI32(dest, shift)
#define D_ADD_EPI64_128(dest, src)  dest = _mm_add_epi64(dest, src)


#define AXR(a, b, d, shift) \
    D_ADD_128(a, b); \
    D_XOR_128(d, a); \
    D_ROR_128(d, shift);

#define AXR2(a, b, c, d, input, sigma_index, shift1, shift2) \
    a = _mm_add_epi32 (a, SET_ROW_FROM_SIGMA(input, sigma_index)); \
    AXR(a, b, d, shift1) \
    AXR(c, d, b, shift2)

#define ROTATE_WORDS_TO_RIGHT(a, n) \
    a = _mm_shuffle_epi32(a, _MM_SHUFFLE((3+n)&3, (2+n)&3, (1+n)&3, (0+n)&3));

#define AXR4(a, b, c, d, input, sigma_index)  \
    AXR2(a, b, c, d, input, sigma_index,     16, 12) \
    AXR2(a, b, c, d, input, sigma_index + 1,  8,  7) \

#define RR2(a, b, c, d, input) \
  { \
    AXR4(a, b, c, d, input, 0) \
      ROTATE_WORDS_TO_RIGHT(b, 1) \
      ROTATE_WORDS_TO_RIGHT(c, 2) \
      ROTATE_WORDS_TO_RIGHT(d, 3) \
    AXR4(a, b, c, d, input, 8) \
      ROTATE_WORDS_TO_RIGHT(b, 3) \
      ROTATE_WORDS_TO_RIGHT(c, 2) \
      ROTATE_WORDS_TO_RIGHT(d, 1) \
  }


/*
Way1:
per 64 bytes block:
10 rounds * 4 iters * (7 + 2) = 360 cycles = if pslld TP=1
                    * (7 + 1) = 320 cycles = if pslld TP=2 (skl+)
additional operations per 7_op_iter :
4 movzx   byte mem
1 movd    mem
3 pinsrd  mem
1.5 pshufd
*/

static
#if 0 || 0 && (defined(Z7_BLAKE2S_USE_V128_WAY2) || \
               defined(Z7_BLAKE2S_USE_V256_WAY2))
  Z7_NO_INLINE
#else
  Z7_FORCE_INLINE
#endif
#ifdef BLAKE2S_ATTRIB_128BIT
       BLAKE2S_ATTRIB_128BIT
#endif
void
Z7_FASTCALL
Blake2s_Compress_V128_Way1(UInt32 * const s, const Byte * const input)
{
  __m128i a, b, c, d;
  __m128i f0, f1;

  LOAD_ROTATE_CONSTS
  d = LOAD_128_FROM_STRUCT(STATE_T(s));
  c = k_iv0_128;
  a = f0 = LOAD_128_FROM_STRUCT(s);
  b = f1 = LOAD_128_FROM_STRUCT(s + 4);
  D_ADD_EPI64_128(d, k_inc);
  STORE_128_TO_STRUCT (STATE_T(s), d);
  D_XOR_128(d, k_iv4_128);

#define RR(r) { const Byte * const sigma = k_Blake2s_Sigma_4[r]; \
      RR2(a, b, c, d, input) }

  ROUNDS_LOOP(RR)
#undef RR

  STORE_128_TO_STRUCT(s    , XOR_128(f0, XOR_128(a, c)));
  STORE_128_TO_STRUCT(s + 4, XOR_128(f1, XOR_128(b, d)));
}


static
Z7_NO_INLINE
#ifdef BLAKE2S_ATTRIB_128BIT
       BLAKE2S_ATTRIB_128BIT
#endif
void
Z7_FASTCALL
Blake2sp_Compress2_V128_Way1(UInt32 *s_items, const Byte *data, const Byte *end)
{
  size_t pos = 0;
  do
  {
    UInt32 * const s = GET_STATE_TABLE_PTR_FROM_BYTE_POS(s_items, pos);
    Blake2s_Compress_V128_Way1(s, data);
    data += Z7_BLAKE2S_BLOCK_SIZE;
    pos  += Z7_BLAKE2S_BLOCK_SIZE;
    pos &= SUPER_BLOCK_MASK;
  }
  while (data != end);
}


#if defined(Z7_BLAKE2S_USE_V128_WAY2) || \
    defined(Z7_BLAKE2S_USE_AVX2_WAY2)
#if 1
  #define Z7_BLAKE2S_CompressSingleBlock(s, data) \
    Blake2sp_Compress2_V128_Way1(s, data, \
        (const Byte *)(const void *)(data) + Z7_BLAKE2S_BLOCK_SIZE)
#else
  #define Z7_BLAKE2S_CompressSingleBlock  Blake2s_Compress_V128_Way1
#endif
#endif


#if (defined(Z7_BLAKE2S_USE_AVX2_WAY_SLOW) || \
     defined(Z7_BLAKE2S_USE_V128_WAY2)) && \
    !defined(Z7_BLAKE2S_USE_GATHER)
#define AXR2_LOAD_INDEXES(sigma_index) \
      const unsigned i0 = sigma[(sigma_index)]; \
      const unsigned i1 = sigma[(sigma_index) + 2 * 1]; \
      const unsigned i2 = sigma[(sigma_index) + 2 * 2]; \
      const unsigned i3 = sigma[(sigma_index) + 2 * 3]; \

#define SET_ROW_FROM_SIGMA_W(input) \
    SET_ROW_FROM_SIGMA_BASE(input, i0, i1, i2, i3)
#endif


#ifdef Z7_BLAKE2S_USE_V128_WAY2

#if 1 || !defined(Z7_BLAKE2S_USE_SSE41)
/* we use SET_ROW_FROM_SIGMA_BASE, that uses
   (SSE4) _mm_insert_epi32(), if Z7_BLAKE2S_USE_INSERT_INSTRUCTION is defined
   (SSE2) _mm_set_epi32()
   MSVC can be faster for this branch:
*/
#define AXR2_W(sigma_index, shift1, shift2) \
  { \
    AXR2_LOAD_INDEXES(sigma_index) \
    a0 = _mm_add_epi32(a0, SET_ROW_FROM_SIGMA_W(data)); \
    a1 = _mm_add_epi32(a1, SET_ROW_FROM_SIGMA_W(data + Z7_BLAKE2S_BLOCK_SIZE)); \
    AXR(a0, b0, d0, shift1) \
    AXR(a1, b1, d1, shift1) \
    AXR(c0, d0, b0, shift2) \
    AXR(c1, d1, b1, shift2) \
  }
#else
/* we use interleaved _mm_insert_epi32():
   GCC can be faster for this branch:
*/
#define AXR2_W_PRE_INSERT(sigma_index, i) \
  { const unsigned ii = sigma[(sigma_index) + i * 2]; \
    t0 = _mm_insert_epi32(t0, *(const Int32 *)GET_SIGMA_PTR(data, ii),                      i); \
    t1 = _mm_insert_epi32(t1, *(const Int32 *)GET_SIGMA_PTR(data, Z7_BLAKE2S_BLOCK_SIZE + ii), i); \
  }
#define AXR2_W(sigma_index, shift1, shift2) \
  { __m128i t0, t1; \
    { const unsigned ii = sigma[sigma_index]; \
      t0 = _mm_cvtsi32_si128(*(const Int32 *)GET_SIGMA_PTR(data, ii)); \
      t1 = _mm_cvtsi32_si128(*(const Int32 *)GET_SIGMA_PTR(data, Z7_BLAKE2S_BLOCK_SIZE + ii)); \
    } \
    AXR2_W_PRE_INSERT(sigma_index, 1) \
    AXR2_W_PRE_INSERT(sigma_index, 2) \
    AXR2_W_PRE_INSERT(sigma_index, 3) \
    a0 = _mm_add_epi32(a0, t0); \
    a1 = _mm_add_epi32(a1, t1); \
    AXR(a0, b0, d0, shift1) \
    AXR(a1, b1, d1, shift1) \
    AXR(c0, d0, b0, shift2) \
    AXR(c1, d1, b1, shift2) \
  }
#endif


#define AXR4_W(sigma_index) \
    AXR2_W(sigma_index,     16, 12) \
    AXR2_W(sigma_index + 1,  8,  7) \

#define WW(r) \
  { const Byte * const sigma = k_Blake2s_Sigma_4[r]; \
    AXR4_W(0) \
      ROTATE_WORDS_TO_RIGHT(b0, 1) \
      ROTATE_WORDS_TO_RIGHT(b1, 1) \
      ROTATE_WORDS_TO_RIGHT(c0, 2) \
      ROTATE_WORDS_TO_RIGHT(c1, 2) \
      ROTATE_WORDS_TO_RIGHT(d0, 3) \
      ROTATE_WORDS_TO_RIGHT(d1, 3) \
    AXR4_W(8) \
      ROTATE_WORDS_TO_RIGHT(b0, 3) \
      ROTATE_WORDS_TO_RIGHT(b1, 3) \
      ROTATE_WORDS_TO_RIGHT(c0, 2) \
      ROTATE_WORDS_TO_RIGHT(c1, 2) \
      ROTATE_WORDS_TO_RIGHT(d0, 1) \
      ROTATE_WORDS_TO_RIGHT(d1, 1) \
  }


static
Z7_NO_INLINE
#ifdef BLAKE2S_ATTRIB_128BIT
       BLAKE2S_ATTRIB_128BIT
#endif
void
Z7_FASTCALL
Blake2sp_Compress2_V128_Way2(UInt32 *s_items, const Byte *data, const Byte *end)
{
  size_t pos = 0;
  end -= Z7_BLAKE2S_BLOCK_SIZE;

  if (data != end)
  {
    LOAD_ROTATE_CONSTS
    do
    {
      UInt32 * const s = GET_STATE_TABLE_PTR_FROM_BYTE_POS(s_items, pos);
      __m128i a0, b0, c0, d0;
      __m128i a1, b1, c1, d1;
      {
        const __m128i inc = k_inc;
        const __m128i temp = k_iv4_128;
        d0 = LOAD_128_FROM_STRUCT (STATE_T(s));
        d1 = LOAD_128_FROM_STRUCT (STATE_T(s + NSW));
        D_ADD_EPI64_128(d0, inc);
        D_ADD_EPI64_128(d1, inc);
        STORE_128_TO_STRUCT (STATE_T(s      ), d0);
        STORE_128_TO_STRUCT (STATE_T(s + NSW), d1);
        D_XOR_128(d0, temp);
        D_XOR_128(d1, temp);
      }
      c1 = c0 = k_iv0_128;
      a0 = LOAD_128_FROM_STRUCT(s);
      b0 = LOAD_128_FROM_STRUCT(s + 4);
      a1 = LOAD_128_FROM_STRUCT(s + NSW);
      b1 = LOAD_128_FROM_STRUCT(s + NSW + 4);
      
      ROUNDS_LOOP (WW)

#undef WW
      
      D_XOR_128(a0, c0);
      D_XOR_128(b0, d0);
      D_XOR_128(a1, c1);
      D_XOR_128(b1, d1);
      
      D_XOR_128(a0, LOAD_128_FROM_STRUCT(s));
      D_XOR_128(b0, LOAD_128_FROM_STRUCT(s + 4));
      D_XOR_128(a1, LOAD_128_FROM_STRUCT(s + NSW));
      D_XOR_128(b1, LOAD_128_FROM_STRUCT(s + NSW + 4));
      
      STORE_128_TO_STRUCT(s,           a0);
      STORE_128_TO_STRUCT(s + 4,       b0);
      STORE_128_TO_STRUCT(s + NSW,     a1);
      STORE_128_TO_STRUCT(s + NSW + 4, b1);
      
      data += Z7_BLAKE2S_BLOCK_SIZE * 2;
      pos  += Z7_BLAKE2S_BLOCK_SIZE * 2;
      pos &= SUPER_BLOCK_MASK;
    }
    while (data < end);
    if (data != end)
      return;
  }
  {
    UInt32 * const s = GET_STATE_TABLE_PTR_FROM_BYTE_POS(s_items, pos);
    Z7_BLAKE2S_CompressSingleBlock(s, data);
  }
}
#endif // Z7_BLAKE2S_USE_V128_WAY2


#ifdef Z7_BLAKE2S_USE_V128_WAY2
  #define Z7_BLAKE2S_Compress2_V128  Blake2sp_Compress2_V128_Way2
#else
  #define Z7_BLAKE2S_Compress2_V128  Blake2sp_Compress2_V128_Way1
#endif



#ifdef Z7_BLAKE2S_MM_ROR_EPI32_IS_SUPPORTED
  #define ROT_128_8(x)    MM_ROR_EPI32(x, 8)
  #define ROT_128_16(x)   MM_ROR_EPI32(x, 16)
  #define ROT_128_7(x)    MM_ROR_EPI32(x, 7)
  #define ROT_128_12(x)   MM_ROR_EPI32(x, 12)
#else
#if defined(Z7_BLAKE2S_USE_SSSE3) || defined(Z7_BLAKE2S_USE_SSE41)
  #define ROT_128_8(x)    _mm_shuffle_epi8(x, r8)   // k_r8
  #define ROT_128_16(x)   _mm_shuffle_epi8(x, r16)  // k_r16
#else
  #define ROT_128_8(x)    MM_ROR_EPI32_VIA_SHIFT(x, 8)
  #define ROT_128_16(x)   MM_ROR_EPI32_VIA_SHIFT(x, 16)
#endif
  #define ROT_128_7(x)    MM_ROR_EPI32_VIA_SHIFT(x, 7)
  #define ROT_128_12(x)   MM_ROR_EPI32_VIA_SHIFT(x, 12)
#endif


#if 1
// this branch can provide similar speed on x86* in most cases,
// because [base + index*4] provides same speed as [base + index].
// but some compilers can generate different code with this branch, that can be faster sometimes.
// this branch uses additional table of 10*16=160 bytes.
#define SIGMA_TABLE_MULT_16( a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15) \
        SIGMA_TABLE_MULT(16, a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15)
MY_ALIGN(16)
static const Byte k_Blake2s_Sigma_16[BLAKE2S_NUM_ROUNDS][16] =
  { SIGMA_TABLE(SIGMA_TABLE_MULT_16) };
#define GET_SIGMA_PTR_128(r)  const Byte * const sigma = k_Blake2s_Sigma_16[r];
#define GET_SIGMA_VAL_128(n)  (sigma[n])
#else
#define GET_SIGMA_PTR_128(r)  const Byte * const sigma = k_Blake2s_Sigma_4[r];
#define GET_SIGMA_VAL_128(n)  (4 * (size_t)sigma[n])
#endif


#ifdef Z7_BLAKE2S_USE_AVX2_FAST
#if 1
#define SIGMA_TABLE_MULT_32( a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15) \
        SIGMA_TABLE_MULT(32, a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15)
MY_ALIGN(64)
static const UInt16 k_Blake2s_Sigma_32[BLAKE2S_NUM_ROUNDS][16] =
  { SIGMA_TABLE(SIGMA_TABLE_MULT_32) };
#define GET_SIGMA_PTR_256(r)  const UInt16 * const sigma = k_Blake2s_Sigma_32[r];
#define GET_SIGMA_VAL_256(n)  (sigma[n])
#else
#define GET_SIGMA_PTR_256(r)  const Byte * const sigma = k_Blake2s_Sigma_4[r];
#define GET_SIGMA_VAL_256(n)  (8 * (size_t)sigma[n])
#endif
#endif // Z7_BLAKE2S_USE_AVX2_FAST


#define D_ROT_128_7(dest)     dest = ROT_128_7(dest)
#define D_ROT_128_8(dest)     dest = ROT_128_8(dest)
#define D_ROT_128_12(dest)    dest = ROT_128_12(dest)
#define D_ROT_128_16(dest)    dest = ROT_128_16(dest)

#define OP_L(a, i)   D_ADD_128 (V(a, 0), \
    LOAD_128((const Byte *)(w) + GET_SIGMA_VAL_128(2*(a)+(i))));

#define OP_0(a)   OP_L(a, 0)
#define OP_7(a)   OP_L(a, 1)

#define OP_1(a)   D_ADD_128 (V(a, 0), V(a, 1));
#define OP_2(a)   D_XOR_128 (V(a, 3), V(a, 0));
#define OP_4(a)   D_ADD_128 (V(a, 2), V(a, 3));
#define OP_5(a)   D_XOR_128 (V(a, 1), V(a, 2));

#define OP_3(a)   D_ROT_128_16 (V(a, 3));
#define OP_6(a)   D_ROT_128_12 (V(a, 1));
#define OP_8(a)   D_ROT_128_8  (V(a, 3));
#define OP_9(a)   D_ROT_128_7  (V(a, 1));


// for 32-bit x86 : interleave mode works slower, because of register pressure.

#if 0 || 1 && (defined(MY_CPU_X86) \
  || defined(__GNUC__) && !defined(__clang__))
// non-inteleaved version:
// is fast for x86 32-bit.
// is fast for GCC x86-64.

#define V4G(a) \
  OP_0 (a) \
  OP_1 (a) \
  OP_2 (a) \
  OP_3 (a) \
  OP_4 (a) \
  OP_5 (a) \
  OP_6 (a) \
  OP_7 (a) \
  OP_1 (a) \
  OP_2 (a) \
  OP_8 (a) \
  OP_4 (a) \
  OP_5 (a) \
  OP_9 (a) \

#define V4R \
{ \
  V4G (0) \
  V4G (1) \
  V4G (2) \
  V4G (3) \
  V4G (4) \
  V4G (5) \
  V4G (6) \
  V4G (7) \
}

#elif 0 || 1 && defined(MY_CPU_X86)

#define OP_INTER_2(op, a,b) \
  op (a) \
  op (b) \

#define V4G(a,b) \
  OP_INTER_2 (OP_0, a,b) \
  OP_INTER_2 (OP_1, a,b) \
  OP_INTER_2 (OP_2, a,b) \
  OP_INTER_2 (OP_3, a,b) \
  OP_INTER_2 (OP_4, a,b) \
  OP_INTER_2 (OP_5, a,b) \
  OP_INTER_2 (OP_6, a,b) \
  OP_INTER_2 (OP_7, a,b) \
  OP_INTER_2 (OP_1, a,b) \
  OP_INTER_2 (OP_2, a,b) \
  OP_INTER_2 (OP_8, a,b) \
  OP_INTER_2 (OP_4, a,b) \
  OP_INTER_2 (OP_5, a,b) \
  OP_INTER_2 (OP_9, a,b) \

#define V4R \
{ \
  V4G (0, 1) \
  V4G (2, 3) \
  V4G (4, 5) \
  V4G (6, 7) \
}

#else
// iterleave-4 version is fast for x64 (MSVC/CLANG)

#define OP_INTER_4(op, a,b,c,d) \
  op (a) \
  op (b) \
  op (c) \
  op (d) \

#define V4G(a,b,c,d) \
  OP_INTER_4 (OP_0, a,b,c,d) \
  OP_INTER_4 (OP_1, a,b,c,d) \
  OP_INTER_4 (OP_2, a,b,c,d) \
  OP_INTER_4 (OP_3, a,b,c,d) \
  OP_INTER_4 (OP_4, a,b,c,d) \
  OP_INTER_4 (OP_5, a,b,c,d) \
  OP_INTER_4 (OP_6, a,b,c,d) \
  OP_INTER_4 (OP_7, a,b,c,d) \
  OP_INTER_4 (OP_1, a,b,c,d) \
  OP_INTER_4 (OP_2, a,b,c,d) \
  OP_INTER_4 (OP_8, a,b,c,d) \
  OP_INTER_4 (OP_4, a,b,c,d) \
  OP_INTER_4 (OP_5, a,b,c,d) \
  OP_INTER_4 (OP_9, a,b,c,d) \

#define V4R \
{ \
  V4G (0, 1, 2, 3) \
  V4G (4, 5, 6, 7) \
}

#endif

#define V4_ROUND(r)  { GET_SIGMA_PTR_128(r); V4R }


#define V4_LOAD_MSG_1(w, m, i) \
{ \
  __m128i m0, m1, m2, m3; \
  __m128i t0, t1, t2, t3; \
  m0 = LOADU_128((m) + ((i) + 0 * 4) * 16); \
  m1 = LOADU_128((m) + ((i) + 1 * 4) * 16); \
  m2 = LOADU_128((m) + ((i) + 2 * 4) * 16); \
  m3 = LOADU_128((m) + ((i) + 3 * 4) * 16); \
  t0 = _mm_unpacklo_epi32(m0, m1); \
  t1 = _mm_unpackhi_epi32(m0, m1); \
  t2 = _mm_unpacklo_epi32(m2, m3); \
  t3 = _mm_unpackhi_epi32(m2, m3); \
  w[(i) * 4 + 0] = _mm_unpacklo_epi64(t0, t2); \
  w[(i) * 4 + 1] = _mm_unpackhi_epi64(t0, t2); \
  w[(i) * 4 + 2] = _mm_unpacklo_epi64(t1, t3); \
  w[(i) * 4 + 3] = _mm_unpackhi_epi64(t1, t3); \
}

#define V4_LOAD_MSG(w, m) \
{ \
  V4_LOAD_MSG_1 (w, m, 0) \
  V4_LOAD_MSG_1 (w, m, 1) \
  V4_LOAD_MSG_1 (w, m, 2) \
  V4_LOAD_MSG_1 (w, m, 3) \
}

#define V4_LOAD_UNPACK_PAIR_128(src32, i, d0, d1) \
{ \
  const __m128i v0 = LOAD_128_FROM_STRUCT((src32) + (i    ) * 4);  \
  const __m128i v1 = LOAD_128_FROM_STRUCT((src32) + (i + 1) * 4);  \
  d0 = _mm_unpacklo_epi32(v0, v1);  \
  d1 = _mm_unpackhi_epi32(v0, v1);  \
}

#define V4_UNPACK_PAIR_128(dest32, i, s0, s1) \
{ \
  STORE_128_TO_STRUCT((dest32) + i * 4     , _mm_unpacklo_epi64(s0, s1));  \
  STORE_128_TO_STRUCT((dest32) + i * 4 + 16, _mm_unpackhi_epi64(s0, s1));  \
}

#define V4_UNPACK_STATE(dest32, src32) \
{ \
  __m128i t0, t1, t2, t3, t4, t5, t6, t7; \
  V4_LOAD_UNPACK_PAIR_128(src32, 0, t0, t1)  \
  V4_LOAD_UNPACK_PAIR_128(src32, 2, t2, t3)  \
  V4_LOAD_UNPACK_PAIR_128(src32, 4, t4, t5)  \
  V4_LOAD_UNPACK_PAIR_128(src32, 6, t6, t7)  \
  V4_UNPACK_PAIR_128(dest32, 0, t0, t2)  \
  V4_UNPACK_PAIR_128(dest32, 8, t1, t3)  \
  V4_UNPACK_PAIR_128(dest32, 1, t4, t6)  \
  V4_UNPACK_PAIR_128(dest32, 9, t5, t7)  \
}


static
Z7_NO_INLINE
#ifdef BLAKE2S_ATTRIB_128BIT
       BLAKE2S_ATTRIB_128BIT
#endif
void
Z7_FASTCALL
Blake2sp_Compress2_V128_Fast(UInt32 *s_items, const Byte *data, const Byte *end)
{
  // PrintStates2(s_items, 8, 16);
  size_t pos = 0;
  pos /= 2;
  do
  {
#if defined(Z7_BLAKE2S_USE_SSSE3) && \
   !defined(Z7_BLAKE2S_MM_ROR_EPI32_IS_SUPPORTED)
    const __m128i  r8 = k_r8;
    const __m128i r16 = k_r16;
#endif
    __m128i w[16];
    __m128i v[16];
    UInt32 *s;
    V4_LOAD_MSG(w, data)
    s = GET_STATE_TABLE_PTR_FROM_BYTE_POS(s_items, pos);
    {
      __m128i ctr = LOAD_128_FROM_STRUCT(s + 64);
      D_ADD_EPI64_128 (ctr, k_inc);
      STORE_128_TO_STRUCT(s + 64, ctr);
      v[12] = XOR_128 (GET_128_IV_WAY4(4), _mm_shuffle_epi32(ctr, _MM_SHUFFLE(0, 0, 0, 0)));
      v[13] = XOR_128 (GET_128_IV_WAY4(5), _mm_shuffle_epi32(ctr, _MM_SHUFFLE(1, 1, 1, 1)));
    }
    v[ 8] = GET_128_IV_WAY4(0);
    v[ 9] = GET_128_IV_WAY4(1);
    v[10] = GET_128_IV_WAY4(2);
    v[11] = GET_128_IV_WAY4(3);
    v[14] = GET_128_IV_WAY4(6);
    v[15] = GET_128_IV_WAY4(7);
    
#define LOAD_STATE_128_FROM_STRUCT(i) \
      v[i] = LOAD_128_FROM_STRUCT(s + (i) * 4);

#define UPDATE_STATE_128_IN_STRUCT(i) \
      STORE_128_TO_STRUCT(s + (i) * 4, XOR_128( \
      XOR_128(v[i], v[(i) + 8]), \
      LOAD_128_FROM_STRUCT(s + (i) * 4)));
    
    REP8_MACRO (LOAD_STATE_128_FROM_STRUCT)
    ROUNDS_LOOP (V4_ROUND)
    REP8_MACRO (UPDATE_STATE_128_IN_STRUCT)

    data += Z7_BLAKE2S_BLOCK_SIZE * 4;
    pos  += Z7_BLAKE2S_BLOCK_SIZE * 4 / 2;
    pos &= SUPER_BLOCK_SIZE / 2 - 1;
  }
  while (data != end);
}


static
Z7_NO_INLINE
#ifdef BLAKE2S_ATTRIB_128BIT
       BLAKE2S_ATTRIB_128BIT
#endif
void
Z7_FASTCALL
Blake2sp_Final_V128_Fast(UInt32 *states)
{
  const __m128i ctr = LOAD_128_FROM_STRUCT(states + 64);
  // printf("\nBlake2sp_Compress2_V128_Fast_Final4\n");
  // PrintStates2(states, 8, 16);
  {
    ptrdiff_t pos = 8 * 4;
    do
    {
      UInt32 *src32  = states + (size_t)(pos * 1);
      UInt32 *dest32 = states + (size_t)(pos * 2);
      V4_UNPACK_STATE(dest32, src32)
      pos -= 8 * 4;
    }
    while (pos >= 0);
  }
  {
    unsigned k;
    for (k = 0; k < 8; k++)
    {
      UInt32 *s = states + (size_t)k * 16;
      STORE_128_TO_STRUCT (STATE_T(s), ctr);
    }
  }
  // PrintStates2(states, 8, 16);
}



#ifdef Z7_BLAKE2S_USE_AVX2

#define ADD_256(a, b)  _mm256_add_epi32(a, b)
#define XOR_256(a, b)  _mm256_xor_si256(a, b)

#if 1 && defined(Z7_BLAKE2S_USE_AVX512_ALWAYS)
  #define MM256_ROR_EPI32  _mm256_ror_epi32
  #define Z7_MM256_ROR_EPI32_IS_SUPPORTED
  #define LOAD_ROTATE_CONSTS_256
#else
#ifdef Z7_BLAKE2S_USE_AVX2_WAY_SLOW
#ifdef Z7_BLAKE2S_USE_AVX2_WAY2
  #define LOAD_ROTATE_CONSTS_256 \
      const __m256i  r8 = k_r8_256; \
      const __m256i r16 = k_r16_256;
#endif // AVX2_WAY2

  #define MM256_ROR_EPI32(r, c) ( \
      ( 8==(c)) ? _mm256_shuffle_epi8(r,r8) \
    : (16==(c)) ? _mm256_shuffle_epi8(r,r16) \
    : _mm256_or_si256( \
      _mm256_srli_epi32((r), (c)), \
      _mm256_slli_epi32((r), 32-(c))))
#endif // WAY_SLOW
#endif


#define D_ADD_256(dest, src)  dest = ADD_256(dest, src)
#define D_XOR_256(dest, src)  dest = XOR_256(dest, src)

#define LOADU_256(p)     _mm256_loadu_si256((const __m256i *)(const void *)(p))

#ifdef Z7_BLAKE2S_USE_AVX2_FAST

#ifdef Z7_MM256_ROR_EPI32_IS_SUPPORTED
#define ROT_256_16(x) MM256_ROR_EPI32((x), 16)
#define ROT_256_12(x) MM256_ROR_EPI32((x), 12)
#define ROT_256_8(x)  MM256_ROR_EPI32((x),  8)
#define ROT_256_7(x)  MM256_ROR_EPI32((x),  7)
#else
#define ROTATE8  _mm256_set_epi8(12, 15, 14, 13, 8, 11, 10, 9, 4, 7, 6, 5, 0, 3, 2, 1, \
                                 12, 15, 14, 13, 8, 11, 10, 9, 4, 7, 6, 5, 0, 3, 2, 1)
#define ROTATE16 _mm256_set_epi8(13, 12, 15, 14, 9, 8, 11, 10, 5, 4, 7, 6, 1, 0, 3, 2, \
                                 13, 12, 15, 14, 9, 8, 11, 10, 5, 4, 7, 6, 1, 0, 3, 2)
#define ROT_256_16(x) _mm256_shuffle_epi8((x), ROTATE16)
#define ROT_256_12(x) _mm256_or_si256(_mm256_srli_epi32((x), 12), _mm256_slli_epi32((x), 20))
#define ROT_256_8(x)  _mm256_shuffle_epi8((x), ROTATE8)
#define ROT_256_7(x)  _mm256_or_si256(_mm256_srli_epi32((x),  7), _mm256_slli_epi32((x), 25))
#endif

#define D_ROT_256_7(dest)     dest = ROT_256_7(dest)
#define D_ROT_256_8(dest)     dest = ROT_256_8(dest)
#define D_ROT_256_12(dest)    dest = ROT_256_12(dest)
#define D_ROT_256_16(dest)    dest = ROT_256_16(dest)

#define LOAD_256(p)      _mm256_load_si256((const __m256i *)(const void *)(p))
#ifdef Z7_BLAKE2SP_STRUCT_IS_NOT_ALIGNED
  #define STOREU_256(p, r) _mm256_storeu_si256((__m256i *)(void *)(p), r)
  #define LOAD_256_FROM_STRUCT(p)     LOADU_256(p)
  #define STORE_256_TO_STRUCT(p, r)   STOREU_256(p, r)
#else
  // if struct is aligned for 32-bytes
  #define STORE_256(p, r)  _mm256_store_si256((__m256i *)(void *)(p), r)
  #define LOAD_256_FROM_STRUCT(p)     LOAD_256(p)
  #define STORE_256_TO_STRUCT(p, r)   STORE_256(p, r)
#endif

#endif // Z7_BLAKE2S_USE_AVX2_FAST



#ifdef Z7_BLAKE2S_USE_AVX2_WAY_SLOW

#if 0
    #define DIAG_PERM2(s) \
    { \
      const __m256i a = LOAD_256_FROM_STRUCT((s)      ); \
      const __m256i b = LOAD_256_FROM_STRUCT((s) + NSW); \
      STORE_256_TO_STRUCT((s      ), _mm256_permute2x128_si256(a, b, 0x20)); \
      STORE_256_TO_STRUCT((s + NSW), _mm256_permute2x128_si256(a, b, 0x31)); \
    }
#else
    #define DIAG_PERM2(s) \
    { \
      const __m128i a = LOAD_128_FROM_STRUCT((s) + 4); \
      const __m128i b = LOAD_128_FROM_STRUCT((s) + NSW); \
      STORE_128_TO_STRUCT((s) + NSW, a); \
      STORE_128_TO_STRUCT((s) + 4  , b); \
    }
#endif
    #define DIAG_PERM8(s_items) \
    { \
      DIAG_PERM2(s_items) \
      DIAG_PERM2(s_items + NSW * 2) \
      DIAG_PERM2(s_items + NSW * 4) \
      DIAG_PERM2(s_items + NSW * 6) \
    }


#define AXR256(a, b, d, shift) \
    D_ADD_256(a, b); \
    D_XOR_256(d, a); \
    d = MM256_ROR_EPI32(d, shift); \



#ifdef Z7_BLAKE2S_USE_GATHER

  #define TABLE_GATHER_256_4(a0,a1,a2,a3) \
    a0,a1,a2,a3, a0+16,a1+16,a2+16,a3+16
  #define TABLE_GATHER_256( \
    a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15) \
  { TABLE_GATHER_256_4(a0,a2,a4,a6), \
    TABLE_GATHER_256_4(a1,a3,a5,a7), \
    TABLE_GATHER_256_4(a8,a10,a12,a14), \
    TABLE_GATHER_256_4(a9,a11,a13,a15) }
MY_ALIGN(64)
static const UInt32 k_Blake2s_Sigma_gather256[BLAKE2S_NUM_ROUNDS][16 * 2] =
  { SIGMA_TABLE(TABLE_GATHER_256) };
  #define GET_SIGMA(r) \
    const UInt32 * const sigma = k_Blake2s_Sigma_gather256[r];
  #define AXR2_LOAD_INDEXES_AVX(sigma_index) \
    const __m256i i01234567 = LOAD_256(sigma + (sigma_index));
  #define SET_ROW_FROM_SIGMA_AVX(in) \
    _mm256_i32gather_epi32((const void *)(in), i01234567, 4)
  #define SIGMA_INTERLEAVE    8
  #define SIGMA_HALF_ROW_SIZE 16

#else // !Z7_BLAKE2S_USE_GATHER

  #define GET_SIGMA(r) \
    const Byte * const sigma = k_Blake2s_Sigma_4[r];
  #define AXR2_LOAD_INDEXES_AVX(sigma_index) \
    AXR2_LOAD_INDEXES(sigma_index)
  #define SET_ROW_FROM_SIGMA_AVX(in) \
    MY_mm256_set_m128i( \
        SET_ROW_FROM_SIGMA_W((in) + Z7_BLAKE2S_BLOCK_SIZE), \
        SET_ROW_FROM_SIGMA_W(in))
  #define SIGMA_INTERLEAVE    1
  #define SIGMA_HALF_ROW_SIZE 8
#endif // !Z7_BLAKE2S_USE_GATHER


#define ROTATE_WORDS_TO_RIGHT_256(a, n) \
    a = _mm256_shuffle_epi32(a, _MM_SHUFFLE((3+n)&3, (2+n)&3, (1+n)&3, (0+n)&3));



#ifdef Z7_BLAKE2S_USE_AVX2_WAY2

#define AXR2_A(sigma_index, shift1, shift2) \
    AXR2_LOAD_INDEXES_AVX(sigma_index) \
    D_ADD_256( a0, SET_ROW_FROM_SIGMA_AVX(data)); \
    AXR256(a0, b0, d0, shift1) \
    AXR256(c0, d0, b0, shift2) \

#define AXR4_A(sigma_index) \
    { AXR2_A(sigma_index, 16, 12) } \
    { AXR2_A(sigma_index + SIGMA_INTERLEAVE, 8, 7) }

#define EE1(r) \
    { GET_SIGMA(r) \
      AXR4_A(0) \
        ROTATE_WORDS_TO_RIGHT_256(b0, 1) \
        ROTATE_WORDS_TO_RIGHT_256(c0, 2) \
        ROTATE_WORDS_TO_RIGHT_256(d0, 3) \
      AXR4_A(SIGMA_HALF_ROW_SIZE) \
        ROTATE_WORDS_TO_RIGHT_256(b0, 3) \
        ROTATE_WORDS_TO_RIGHT_256(c0, 2) \
        ROTATE_WORDS_TO_RIGHT_256(d0, 1) \
    }

static
Z7_NO_INLINE
#ifdef BLAKE2S_ATTRIB_AVX2
       BLAKE2S_ATTRIB_AVX2
#endif
void
Z7_FASTCALL
Blake2sp_Compress2_AVX2_Way2(UInt32 *s_items, const Byte *data, const Byte *end)
{
  size_t pos = 0;
  end -= Z7_BLAKE2S_BLOCK_SIZE;

  if (data != end)
  {
    LOAD_ROTATE_CONSTS_256
    DIAG_PERM8(s_items)
    do
    {
      UInt32 * const s = GET_STATE_TABLE_PTR_FROM_BYTE_POS(s_items, pos);
      __m256i a0, b0, c0, d0;
      {
        const __m128i inc = k_inc;
        __m128i d0_128 = LOAD_128_FROM_STRUCT (STATE_T(s));
        __m128i d1_128 = LOAD_128_FROM_STRUCT (STATE_T(s + NSW));
        D_ADD_EPI64_128(d0_128, inc);
        D_ADD_EPI64_128(d1_128, inc);
        STORE_128_TO_STRUCT (STATE_T(s      ), d0_128);
        STORE_128_TO_STRUCT (STATE_T(s + NSW), d1_128);
        d0 = MY_mm256_set_m128i(d1_128, d0_128);
        D_XOR_256(d0, k_iv4_256);
      }
      c0 = SET_FROM_128(k_iv0_128);
      a0 = LOAD_256_FROM_STRUCT(s + NSW * 0);
      b0 = LOAD_256_FROM_STRUCT(s + NSW * 1);
      
      ROUNDS_LOOP (EE1)
      
      D_XOR_256(a0, c0);
      D_XOR_256(b0, d0);
      
      D_XOR_256(a0, LOAD_256_FROM_STRUCT(s + NSW * 0));
      D_XOR_256(b0, LOAD_256_FROM_STRUCT(s + NSW * 1));
      
      STORE_256_TO_STRUCT(s + NSW * 0, a0);
      STORE_256_TO_STRUCT(s + NSW * 1, b0);
      
      data += Z7_BLAKE2S_BLOCK_SIZE * 2;
      pos  += Z7_BLAKE2S_BLOCK_SIZE * 2;
      pos &= SUPER_BLOCK_MASK;
    }
    while (data < end);
    DIAG_PERM8(s_items)
    if (data != end)
      return;
  }
  {
    UInt32 * const s = GET_STATE_TABLE_PTR_FROM_BYTE_POS(s_items, pos);
    Z7_BLAKE2S_CompressSingleBlock(s, data);
  }
}

#endif // Z7_BLAKE2S_USE_AVX2_WAY2



#ifdef Z7_BLAKE2S_USE_AVX2_WAY4

#define AXR2_X(sigma_index, shift1, shift2) \
    AXR2_LOAD_INDEXES_AVX(sigma_index) \
    D_ADD_256( a0, SET_ROW_FROM_SIGMA_AVX(data)); \
    D_ADD_256( a1, SET_ROW_FROM_SIGMA_AVX((data) + Z7_BLAKE2S_BLOCK_SIZE * 2)); \
    AXR256(a0, b0, d0, shift1) \
    AXR256(a1, b1, d1, shift1) \
    AXR256(c0, d0, b0, shift2) \
    AXR256(c1, d1, b1, shift2) \

#define AXR4_X(sigma_index) \
    { AXR2_X(sigma_index, 16, 12) } \
    { AXR2_X(sigma_index + SIGMA_INTERLEAVE, 8, 7) }

#define EE2(r) \
    { GET_SIGMA(r) \
      AXR4_X(0) \
        ROTATE_WORDS_TO_RIGHT_256(b0, 1) \
        ROTATE_WORDS_TO_RIGHT_256(b1, 1) \
        ROTATE_WORDS_TO_RIGHT_256(c0, 2) \
        ROTATE_WORDS_TO_RIGHT_256(c1, 2) \
        ROTATE_WORDS_TO_RIGHT_256(d0, 3) \
        ROTATE_WORDS_TO_RIGHT_256(d1, 3) \
      AXR4_X(SIGMA_HALF_ROW_SIZE) \
        ROTATE_WORDS_TO_RIGHT_256(b0, 3) \
        ROTATE_WORDS_TO_RIGHT_256(b1, 3) \
        ROTATE_WORDS_TO_RIGHT_256(c0, 2) \
        ROTATE_WORDS_TO_RIGHT_256(c1, 2) \
        ROTATE_WORDS_TO_RIGHT_256(d0, 1) \
        ROTATE_WORDS_TO_RIGHT_256(d1, 1) \
    }

static
Z7_NO_INLINE
#ifdef BLAKE2S_ATTRIB_AVX2
       BLAKE2S_ATTRIB_AVX2
#endif
void
Z7_FASTCALL
Blake2sp_Compress2_AVX2_Way4(UInt32 *s_items, const Byte *data, const Byte *end)
{
  size_t pos = 0;

  if ((size_t)(end - data) >= Z7_BLAKE2S_BLOCK_SIZE * 4)
  {
#ifndef Z7_MM256_ROR_EPI32_IS_SUPPORTED
    const __m256i  r8 = k_r8_256;
    const __m256i r16 = k_r16_256;
#endif
    end -= Z7_BLAKE2S_BLOCK_SIZE * 3;
    DIAG_PERM8(s_items)
    do
    {
      UInt32 * const s = GET_STATE_TABLE_PTR_FROM_BYTE_POS(s_items, pos);
      __m256i a0, b0, c0, d0;
      __m256i a1, b1, c1, d1;
      {
        const __m128i inc = k_inc;
        __m128i d0_128 = LOAD_128_FROM_STRUCT (STATE_T(s));
        __m128i d1_128 = LOAD_128_FROM_STRUCT (STATE_T(s + NSW));
        __m128i d2_128 = LOAD_128_FROM_STRUCT (STATE_T(s + NSW * 2));
        __m128i d3_128 = LOAD_128_FROM_STRUCT (STATE_T(s + NSW * 3));
        D_ADD_EPI64_128(d0_128, inc);
        D_ADD_EPI64_128(d1_128, inc);
        D_ADD_EPI64_128(d2_128, inc);
        D_ADD_EPI64_128(d3_128, inc);
        STORE_128_TO_STRUCT (STATE_T(s          ), d0_128);
        STORE_128_TO_STRUCT (STATE_T(s + NSW * 1), d1_128);
        STORE_128_TO_STRUCT (STATE_T(s + NSW * 2), d2_128);
        STORE_128_TO_STRUCT (STATE_T(s + NSW * 3), d3_128);
        d0 = MY_mm256_set_m128i(d1_128, d0_128);
        d1 = MY_mm256_set_m128i(d3_128, d2_128);
        D_XOR_256(d0, k_iv4_256);
        D_XOR_256(d1, k_iv4_256);
      }
      c1 = c0 = SET_FROM_128(k_iv0_128);
      a0 = LOAD_256_FROM_STRUCT(s + NSW * 0);
      b0 = LOAD_256_FROM_STRUCT(s + NSW * 1);
      a1 = LOAD_256_FROM_STRUCT(s + NSW * 2);
      b1 = LOAD_256_FROM_STRUCT(s + NSW * 3);
      
      ROUNDS_LOOP (EE2)

      D_XOR_256(a0, c0);
      D_XOR_256(b0, d0);
      D_XOR_256(a1, c1);
      D_XOR_256(b1, d1);
      
      D_XOR_256(a0, LOAD_256_FROM_STRUCT(s + NSW * 0));
      D_XOR_256(b0, LOAD_256_FROM_STRUCT(s + NSW * 1));
      D_XOR_256(a1, LOAD_256_FROM_STRUCT(s + NSW * 2));
      D_XOR_256(b1, LOAD_256_FROM_STRUCT(s + NSW * 3));
      
      STORE_256_TO_STRUCT(s + NSW * 0, a0);
      STORE_256_TO_STRUCT(s + NSW * 1, b0);
      STORE_256_TO_STRUCT(s + NSW * 2, a1);
      STORE_256_TO_STRUCT(s + NSW * 3, b1);
      
      data += Z7_BLAKE2S_BLOCK_SIZE * 4;
      pos  += Z7_BLAKE2S_BLOCK_SIZE * 4;
      pos &= SUPER_BLOCK_MASK;
    }
    while (data < end);
    DIAG_PERM8(s_items)
    end += Z7_BLAKE2S_BLOCK_SIZE * 3;
  }
  if (data == end)
    return;
  // Z7_BLAKE2S_Compress2_V128(s_items, data, end, pos);
  do
  {
    UInt32 * const s = GET_STATE_TABLE_PTR_FROM_BYTE_POS(s_items, pos);
    Z7_BLAKE2S_CompressSingleBlock(s, data);
    data += Z7_BLAKE2S_BLOCK_SIZE;
    pos  += Z7_BLAKE2S_BLOCK_SIZE;
    pos &= SUPER_BLOCK_MASK;
  }
  while (data != end);
}

#endif // Z7_BLAKE2S_USE_AVX2_WAY4
#endif // Z7_BLAKE2S_USE_AVX2_WAY_SLOW


// ---------------------------------------------------------

#ifdef Z7_BLAKE2S_USE_AVX2_FAST

#define OP256_L(a, i)   D_ADD_256 (V(a, 0), \
    LOAD_256((const Byte *)(w) + GET_SIGMA_VAL_256(2*(a)+(i))));

#define OP256_0(a)   OP256_L(a, 0)
#define OP256_7(a)   OP256_L(a, 1)

#define OP256_1(a)   D_ADD_256 (V(a, 0), V(a, 1));
#define OP256_2(a)   D_XOR_256 (V(a, 3), V(a, 0));
#define OP256_4(a)   D_ADD_256 (V(a, 2), V(a, 3));
#define OP256_5(a)   D_XOR_256 (V(a, 1), V(a, 2));

#define OP256_3(a)   D_ROT_256_16 (V(a, 3));
#define OP256_6(a)   D_ROT_256_12 (V(a, 1));
#define OP256_8(a)   D_ROT_256_8  (V(a, 3));
#define OP256_9(a)   D_ROT_256_7  (V(a, 1));


#if 0 || 1 && defined(MY_CPU_X86)

#define V8_G(a) \
  OP256_0 (a) \
  OP256_1 (a) \
  OP256_2 (a) \
  OP256_3 (a) \
  OP256_4 (a) \
  OP256_5 (a) \
  OP256_6 (a) \
  OP256_7 (a) \
  OP256_1 (a) \
  OP256_2 (a) \
  OP256_8 (a) \
  OP256_4 (a) \
  OP256_5 (a) \
  OP256_9 (a) \

#define V8R { \
  V8_G (0); \
  V8_G (1); \
  V8_G (2); \
  V8_G (3); \
  V8_G (4); \
  V8_G (5); \
  V8_G (6); \
  V8_G (7); \
}

#else

#define OP256_INTER_4(op, a,b,c,d) \
  op (a) \
  op (b) \
  op (c) \
  op (d) \

#define V8_G(a,b,c,d) \
  OP256_INTER_4 (OP256_0, a,b,c,d) \
  OP256_INTER_4 (OP256_1, a,b,c,d) \
  OP256_INTER_4 (OP256_2, a,b,c,d) \
  OP256_INTER_4 (OP256_3, a,b,c,d) \
  OP256_INTER_4 (OP256_4, a,b,c,d) \
  OP256_INTER_4 (OP256_5, a,b,c,d) \
  OP256_INTER_4 (OP256_6, a,b,c,d) \
  OP256_INTER_4 (OP256_7, a,b,c,d) \
  OP256_INTER_4 (OP256_1, a,b,c,d) \
  OP256_INTER_4 (OP256_2, a,b,c,d) \
  OP256_INTER_4 (OP256_8, a,b,c,d) \
  OP256_INTER_4 (OP256_4, a,b,c,d) \
  OP256_INTER_4 (OP256_5, a,b,c,d) \
  OP256_INTER_4 (OP256_9, a,b,c,d) \

#define V8R { \
  V8_G (0, 1, 2, 3) \
  V8_G (4, 5, 6, 7) \
}
#endif

#define V8_ROUND(r)  { GET_SIGMA_PTR_256(r); V8R }


// for debug:
// #define Z7_BLAKE2S_PERMUTE_WITH_GATHER
#if defined(Z7_BLAKE2S_PERMUTE_WITH_GATHER)
// gather instruction is slow.
#define V8_LOAD_MSG(w, m) \
{ \
  unsigned i; \
  for (i = 0; i < 16; ++i) { \
    w[i] = _mm256_i32gather_epi32( \
      (const void *)((m) + i * sizeof(UInt32)),\
      _mm256_set_epi32(0x70, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00), \
      sizeof(UInt32)); \
  } \
}
#else // !Z7_BLAKE2S_PERMUTE_WITH_GATHER

#define V8_LOAD_MSG_2(w, a0, a1) \
{ \
  (w)[0] = _mm256_permute2x128_si256(a0, a1, 0x20);  \
  (w)[4] = _mm256_permute2x128_si256(a0, a1, 0x31);  \
}

#define V8_LOAD_MSG_4(w, z0, z1, z2, z3) \
{ \
  __m256i s0, s1, s2, s3;  \
  s0 = _mm256_unpacklo_epi64(z0, z1);  \
  s1 = _mm256_unpackhi_epi64(z0, z1);  \
  s2 = _mm256_unpacklo_epi64(z2, z3);  \
  s3 = _mm256_unpackhi_epi64(z2, z3);  \
  V8_LOAD_MSG_2((w) + 0, s0, s2)   \
  V8_LOAD_MSG_2((w) + 1, s1, s3)   \
}

#define V8_LOAD_MSG_0(t0, t1, m) \
{ \
  __m256i m0, m1;  \
  m0 = LOADU_256(m);  \
  m1 = LOADU_256((m) + 2 * 32);  \
  t0 = _mm256_unpacklo_epi32(m0, m1);  \
  t1 = _mm256_unpackhi_epi32(m0, m1);  \
}

#define V8_LOAD_MSG_8(w, m) \
{ \
  __m256i t0, t1, t2, t3, t4, t5, t6, t7;  \
  V8_LOAD_MSG_0(t0, t4, (m) + 0 * 4 * 32)  \
  V8_LOAD_MSG_0(t1, t5, (m) + 1 * 4 * 32)  \
  V8_LOAD_MSG_0(t2, t6, (m) + 2 * 4 * 32)  \
  V8_LOAD_MSG_0(t3, t7, (m) + 3 * 4 * 32)  \
  V8_LOAD_MSG_4((w)    , t0, t1, t2, t3)   \
  V8_LOAD_MSG_4((w) + 2, t4, t5, t6, t7)   \
}

#define V8_LOAD_MSG(w, m) \
{ \
  V8_LOAD_MSG_8(w, m)  \
  V8_LOAD_MSG_8((w) + 8, (m) + 32)  \
}

#endif // !Z7_BLAKE2S_PERMUTE_WITH_GATHER


#define V8_PERM_PAIR_STORE(u, a0, a2) \
{ \
  STORE_256_TO_STRUCT((u),     _mm256_permute2x128_si256(a0, a2, 0x20));  \
  STORE_256_TO_STRUCT((u) + 8, _mm256_permute2x128_si256(a0, a2, 0x31));  \
}

#define V8_UNPACK_STORE_4(u, z0, z1, z2, z3) \
{ \
  __m256i s0, s1, s2, s3;  \
  s0 = _mm256_unpacklo_epi64(z0, z1);  \
  s1 = _mm256_unpackhi_epi64(z0, z1);  \
  s2 = _mm256_unpacklo_epi64(z2, z3);  \
  s3 = _mm256_unpackhi_epi64(z2, z3);  \
  V8_PERM_PAIR_STORE(u + 0, s0, s2)  \
  V8_PERM_PAIR_STORE(u + 2, s1, s3)  \
}

#define V8_UNPACK_STORE_0(src32, d0, d1) \
{ \
  const __m256i v0 = LOAD_256_FROM_STRUCT ((src32)    );  \
  const __m256i v1 = LOAD_256_FROM_STRUCT ((src32) + 8);  \
  d0 = _mm256_unpacklo_epi32(v0, v1);  \
  d1 = _mm256_unpackhi_epi32(v0, v1);  \
}

#define V8_UNPACK_STATE(dest32, src32) \
{ \
  __m256i t0, t1, t2, t3, t4, t5, t6, t7;  \
  V8_UNPACK_STORE_0 ((src32) + 16 * 0, t0, t4)  \
  V8_UNPACK_STORE_0 ((src32) + 16 * 1, t1, t5)  \
  V8_UNPACK_STORE_0 ((src32) + 16 * 2, t2, t6)  \
  V8_UNPACK_STORE_0 ((src32) + 16 * 3, t3, t7)  \
  V8_UNPACK_STORE_4 ((__m256i *)(void *)(dest32)    , t0, t1, t2, t3)  \
  V8_UNPACK_STORE_4 ((__m256i *)(void *)(dest32) + 4, t4, t5, t6, t7)  \
}



#define V8_LOAD_STATE_256_FROM_STRUCT(i) \
      v[i] = LOAD_256_FROM_STRUCT(s_items + (i) * 8);

#if 0 || 0 && defined(MY_CPU_X86)
#define Z7_BLAKE2S_AVX2_FAST_USE_STRUCT
#endif

#ifdef Z7_BLAKE2S_AVX2_FAST_USE_STRUCT
// this branch doesn't use (iv) array
// so register pressure can be lower.
// it can be faster sometimes
#define V8_LOAD_STATE_256(i)  V8_LOAD_STATE_256_FROM_STRUCT(i)
#define V8_UPDATE_STATE_256(i) \
{ \
    STORE_256_TO_STRUCT(s_items + (i) * 8, XOR_256( \
    XOR_256(v[i], v[(i) + 8]), \
    LOAD_256_FROM_STRUCT(s_items + (i) * 8))); \
}
#else
// it uses more variables (iv) registers
// it's better for gcc
// maybe that branch is better, if register pressure will be lower (avx512)
#define V8_LOAD_STATE_256(i)   { iv[i] = v[i]; }
#define V8_UPDATE_STATE_256(i) { v[i] = XOR_256(XOR_256(v[i], v[i + 8]), iv[i]); }
#define V8_STORE_STATE_256(i)  { STORE_256_TO_STRUCT(s_items + (i) * 8, v[i]); }
#endif


#if 0
  // use loading constants from memory
  #define KK8(n)  KIV(n), KIV(n), KIV(n), KIV(n), KIV(n), KIV(n), KIV(n), KIV(n)
MY_ALIGN(64)
static const UInt32 k_Blake2s_IV_WAY8[]=
{
  KK8(0), KK8(1), KK8(2), KK8(3), KK8(4), KK8(5), KK8(6), KK8(7)
};
  #define GET_256_IV_WAY8(i)  LOAD_256(k_Blake2s_IV_WAY8 + 8 * (i))
#else
  // use constant generation:
  #define GET_256_IV_WAY8(i)  _mm256_set1_epi32((Int32)KIV(i))
#endif


static
Z7_NO_INLINE
#ifdef BLAKE2S_ATTRIB_AVX2
       BLAKE2S_ATTRIB_AVX2
#endif
void
Z7_FASTCALL
Blake2sp_Compress2_AVX2_Fast(UInt32 *s_items, const Byte *data, const Byte *end)
{
#ifndef Z7_BLAKE2S_AVX2_FAST_USE_STRUCT
  __m256i v[16];
#endif

  // PrintStates2(s_items, 8, 16);

#ifndef Z7_BLAKE2S_AVX2_FAST_USE_STRUCT
  REP8_MACRO (V8_LOAD_STATE_256_FROM_STRUCT)
#endif

  do
  {
    __m256i w[16];
#ifdef Z7_BLAKE2S_AVX2_FAST_USE_STRUCT
    __m256i v[16];
#else
    __m256i iv[8];
#endif
    V8_LOAD_MSG(w, data)
    {
      // we use load/store ctr inside loop to reduce register pressure:
#if 1 || 1 && defined(MY_CPU_X86)
      const __m256i ctr = _mm256_add_epi64(
          LOAD_256_FROM_STRUCT(s_items + 64),
          _mm256_set_epi32(
              0, 0, 0, Z7_BLAKE2S_BLOCK_SIZE,
              0, 0, 0, Z7_BLAKE2S_BLOCK_SIZE));
      STORE_256_TO_STRUCT(s_items + 64, ctr);
#else
      const UInt64 ctr64 = *(const UInt64 *)(const void *)(s_items + 64)
          + Z7_BLAKE2S_BLOCK_SIZE;
      const __m256i ctr = _mm256_set_epi64x(0, (Int64)ctr64, 0, (Int64)ctr64);
      *(UInt64 *)(void *)(s_items + 64) = ctr64;
#endif
      v[12] = XOR_256 (GET_256_IV_WAY8(4), _mm256_shuffle_epi32(ctr, _MM_SHUFFLE(0, 0, 0, 0)));
      v[13] = XOR_256 (GET_256_IV_WAY8(5), _mm256_shuffle_epi32(ctr, _MM_SHUFFLE(1, 1, 1, 1)));
    }
    v[ 8] = GET_256_IV_WAY8(0);
    v[ 9] = GET_256_IV_WAY8(1);
    v[10] = GET_256_IV_WAY8(2);
    v[11] = GET_256_IV_WAY8(3);
    v[14] = GET_256_IV_WAY8(6);
    v[15] = GET_256_IV_WAY8(7);

    REP8_MACRO (V8_LOAD_STATE_256)
    ROUNDS_LOOP (V8_ROUND)
    REP8_MACRO (V8_UPDATE_STATE_256)
    data += SUPER_BLOCK_SIZE;
  }
  while (data != end);

#ifndef Z7_BLAKE2S_AVX2_FAST_USE_STRUCT
  REP8_MACRO (V8_STORE_STATE_256)
#endif
}


static
Z7_NO_INLINE
#ifdef BLAKE2S_ATTRIB_AVX2
       BLAKE2S_ATTRIB_AVX2
#endif
void
Z7_FASTCALL
Blake2sp_Final_AVX2_Fast(UInt32 *states)
{
  const __m128i ctr = LOAD_128_FROM_STRUCT(states + 64);
  // PrintStates2(states, 8, 16);
  V8_UNPACK_STATE(states, states)
  // PrintStates2(states, 8, 16);
  {
    unsigned k;
    for (k = 0; k < 8; k++)
    {
      UInt32 *s = states + (size_t)k * 16;
      STORE_128_TO_STRUCT (STATE_T(s), ctr);
    }
  }
  // PrintStates2(states, 8, 16);
  // printf("\nafter V8_UNPACK_STATE \n");
}

#endif // Z7_BLAKE2S_USE_AVX2_FAST
#endif // avx2
#endif // vector


/*
#define Blake2s_Increment_Counter(s, inc) \
  { STATE_T(s)[0] += (inc);  STATE_T(s)[1] += (STATE_T(s)[0] < (inc)); }
#define Blake2s_Increment_Counter_Small(s, inc) \
  { STATE_T(s)[0] += (inc); }
*/

#define Blake2s_Set_LastBlock(s) \
  { STATE_F(s)[0] = BLAKE2S_FINAL_FLAG; /* STATE_F(s)[1] = p->u.header.lastNode_f1; */ }


#if 0 || 1 && defined(Z7_MSC_VER_ORIGINAL) && Z7_MSC_VER_ORIGINAL >= 1600
  // good for vs2022
  #define LOOP_8(mac) { unsigned kkk; for (kkk = 0; kkk < 8; kkk++) mac(kkk) }
#else
   // good for Z7_BLAKE2S_UNROLL for GCC9 (arm*/x86*) and MSC_VER_1400-x64.
  #define LOOP_8(mac) { REP8_MACRO(mac) }
#endif


static
Z7_FORCE_INLINE
// Z7_NO_INLINE
void
Z7_FASTCALL
Blake2s_Compress(UInt32 *s, const Byte *input)
{
  UInt32 m[16];
  UInt32 v[16];
  {
    unsigned i;
    for (i = 0; i < 16; i++)
      m[i] = GetUi32(input + i * 4);
  }

#define INIT_v_FROM_s(i)  v[i] = s[i];
  
  LOOP_8(INIT_v_FROM_s)
 
  // Blake2s_Increment_Counter(s, Z7_BLAKE2S_BLOCK_SIZE)
  {
    const UInt32 t0 = STATE_T(s)[0] + Z7_BLAKE2S_BLOCK_SIZE;
    const UInt32 t1 = STATE_T(s)[1] + (t0 < Z7_BLAKE2S_BLOCK_SIZE);
    STATE_T(s)[0] = t0;
    STATE_T(s)[1] = t1;
    v[12] = t0 ^ KIV(4);
    v[13] = t1 ^ KIV(5);
  }
  // v[12] = STATE_T(s)[0] ^ KIV(4);
  // v[13] = STATE_T(s)[1] ^ KIV(5);
  v[14] = STATE_F(s)[0] ^ KIV(6);
  v[15] = STATE_F(s)[1] ^ KIV(7);

  v[ 8] = KIV(0);
  v[ 9] = KIV(1);
  v[10] = KIV(2);
  v[11] = KIV(3);
  // PrintStates2((const UInt32 *)v, 1, 16);

  #define ADD_SIGMA(a, index)  V(a, 0) += *(const UInt32 *)GET_SIGMA_PTR(m, sigma[index]);
  #define ADD32M(dest, src, a)    V(a, dest) += V(a, src);
  #define XOR32M(dest, src, a)    V(a, dest) ^= V(a, src);
  #define RTR32M(dest, shift, a)  V(a, dest) = rotrFixed(V(a, dest), shift);

// big interleaving can provides big performance gain, if scheduler queues are small.
#if 0 || 1 && defined(MY_CPU_X86)
  // interleave-1: for small register number (x86-32bit)
  #define G2(index, a, x, y) \
    ADD_SIGMA (a, (index) + 2 * 0) \
    ADD32M (0, 1, a) \
    XOR32M (3, 0, a) \
    RTR32M (3, x, a) \
    ADD32M (2, 3, a) \
    XOR32M (1, 2, a) \
    RTR32M (1, y, a) \

  #define G(a) \
    G2(a * 2    , a, 16, 12) \
    G2(a * 2 + 1, a,  8,  7) \

  #define R2 \
    G(0) \
    G(1) \
    G(2) \
    G(3) \
    G(4) \
    G(5) \
    G(6) \
    G(7) \

#elif 0 || 1 && defined(MY_CPU_X86_OR_AMD64)
  // interleave-2: is good if the number of registers is not big (x86-64).

  #define REP2(mac, dest, src, a, b) \
      mac(dest, src, a) \
      mac(dest, src, b)

  #define G2(index, a, b, x, y) \
    ADD_SIGMA (a, (index) + 2 * 0) \
    ADD_SIGMA (b, (index) + 2 * 1) \
    REP2 (ADD32M, 0, 1, a, b) \
    REP2 (XOR32M, 3, 0, a, b) \
    REP2 (RTR32M, 3, x, a, b) \
    REP2 (ADD32M, 2, 3, a, b) \
    REP2 (XOR32M, 1, 2, a, b) \
    REP2 (RTR32M, 1, y, a, b) \

  #define G(a, b) \
    G2(a * 2    , a, b, 16, 12) \
    G2(a * 2 + 1, a, b,  8,  7) \

  #define R2 \
    G(0, 1) \
    G(2, 3) \
    G(4, 5) \
    G(6, 7) \

#else
  // interleave-4:
  // it has big register pressure for x86/x64.
  // and MSVC compilers for x86/x64 are slow for this branch.
  // but if we have big number of registers, this branch can be faster.

  #define REP4(mac, dest, src, a, b, c, d) \
      mac(dest, src, a) \
      mac(dest, src, b) \
      mac(dest, src, c) \
      mac(dest, src, d)

  #define G2(index, a, b, c, d, x, y) \
    ADD_SIGMA (a, (index) + 2 * 0) \
    ADD_SIGMA (b, (index) + 2 * 1) \
    ADD_SIGMA (c, (index) + 2 * 2) \
    ADD_SIGMA (d, (index) + 2 * 3) \
    REP4 (ADD32M, 0, 1, a, b, c, d) \
    REP4 (XOR32M, 3, 0, a, b, c, d) \
    REP4 (RTR32M, 3, x, a, b, c, d) \
    REP4 (ADD32M, 2, 3, a, b, c, d) \
    REP4 (XOR32M, 1, 2, a, b, c, d) \
    REP4 (RTR32M, 1, y, a, b, c, d) \

  #define G(a, b, c, d) \
    G2(a * 2    , a, b, c, d, 16, 12) \
    G2(a * 2 + 1, a, b, c, d,  8,  7) \

  #define R2 \
    G(0, 1, 2, 3) \
    G(4, 5, 6, 7) \

#endif

  #define R(r)  { const Byte *sigma = k_Blake2s_Sigma_4[r];  R2 }

  // Z7_BLAKE2S_UNROLL gives 5-6 KB larger code, but faster:
  //   20-40% faster for (x86/x64) VC2010+/GCC/CLANG.
  //   30-60% faster for (arm64-arm32) GCC.
  //    5-11% faster for (arm64) CLANG-MAC.
  // so Z7_BLAKE2S_UNROLL is good optimization, if there is no vector branch.
  // But if there is vectors branch (for x86*), this scalar code will be unused mostly.
  // So we want smaller code (without unrolling) in that case (x86*).
#if 0 || 1 && !defined(Z7_BLAKE2S_USE_VECTORS)
  #define Z7_BLAKE2S_UNROLL
#endif

#ifdef Z7_BLAKE2S_UNROLL
    ROUNDS_LOOP_UNROLLED (R)
#else
    ROUNDS_LOOP (R)
#endif
  
  #undef G
  #undef G2
  #undef R
  #undef R2

  // printf("\n v after: \n");
  // PrintStates2((const UInt32 *)v, 1, 16);
#define XOR_s_PAIR_v(i)  s[i] ^= v[i] ^ v[i + 8];

  LOOP_8(XOR_s_PAIR_v)
  // printf("\n s after:\n");
  // PrintStates2((const UInt32 *)s, 1, 16);
}


static
Z7_NO_INLINE
void
Z7_FASTCALL
Blake2sp_Compress2(UInt32 *s_items, const Byte *data, const Byte *end)
{
  size_t pos = 0;
  // PrintStates2(s_items, 8, 16);
  do
  {
    UInt32 * const s = GET_STATE_TABLE_PTR_FROM_BYTE_POS(s_items, pos);
    Blake2s_Compress(s, data);
    data += Z7_BLAKE2S_BLOCK_SIZE;
    pos  += Z7_BLAKE2S_BLOCK_SIZE;
    pos &= SUPER_BLOCK_MASK;
  }
  while (data != end);
}


#ifdef Z7_BLAKE2S_USE_VECTORS

static Z7_BLAKE2SP_FUNC_COMPRESS g_Z7_BLAKE2SP_FUNC_COMPRESS_Fast   = Blake2sp_Compress2;
static Z7_BLAKE2SP_FUNC_COMPRESS g_Z7_BLAKE2SP_FUNC_COMPRESS_Single = Blake2sp_Compress2;
static Z7_BLAKE2SP_FUNC_INIT     g_Z7_BLAKE2SP_FUNC_INIT_Init;
static Z7_BLAKE2SP_FUNC_INIT     g_Z7_BLAKE2SP_FUNC_INIT_Final;
static unsigned g_z7_Blake2sp_SupportedFlags;

  #define Z7_BLAKE2SP_Compress_Fast(p)   (p)->u.header.func_Compress_Fast
  #define Z7_BLAKE2SP_Compress_Single(p) (p)->u.header.func_Compress_Single
#else
  #define Z7_BLAKE2SP_Compress_Fast(p)   Blake2sp_Compress2
  #define Z7_BLAKE2SP_Compress_Single(p) Blake2sp_Compress2
#endif // Z7_BLAKE2S_USE_VECTORS


#if 1 && defined(MY_CPU_LE)
    #define GET_DIGEST(_s, _digest) \
      { memcpy(_digest, _s, Z7_BLAKE2S_DIGEST_SIZE); }
#else
    #define GET_DIGEST(_s, _digest) \
    { unsigned _i; for (_i = 0; _i < 8; _i++) \
        { SetUi32((_digest) + 4 * _i, (_s)[_i]) } \
    }
#endif


/* ---------- BLAKE2s ---------- */
/*
// we need to xor CBlake2s::h[i] with input parameter block after Blake2s_Init0()
typedef struct
{
  Byte  digest_length;
  Byte  key_length;
  Byte  fanout;               // = 1 : in sequential mode
  Byte  depth;                // = 1 : in sequential mode
  UInt32 leaf_length;
  Byte  node_offset[6];       // 0 for the first, leftmost, leaf, or in sequential mode
  Byte  node_depth;           // 0 for the leaves, or in sequential mode
  Byte  inner_length;         // [0, 32], 0 in sequential mode
  Byte  salt[BLAKE2S_SALTBYTES];
  Byte  personal[BLAKE2S_PERSONALBYTES];
} CBlake2sParam;
*/

#define k_Blake2sp_IV_0  \
    (KIV(0) ^ (Z7_BLAKE2S_DIGEST_SIZE | ((UInt32)Z7_BLAKE2SP_PARALLEL_DEGREE << 16) | ((UInt32)2 << 24)))
#define k_Blake2sp_IV_3_FROM_NODE_DEPTH(node_depth)  \
    (KIV(3) ^ ((UInt32)(node_depth) << 16) ^ ((UInt32)Z7_BLAKE2S_DIGEST_SIZE << 24))

Z7_FORCE_INLINE
static void Blake2sp_Init_Spec(UInt32 *s, unsigned node_offset, unsigned node_depth)
{
  s[0] = k_Blake2sp_IV_0;
  s[1] = KIV(1);
  s[2] = KIV(2) ^ (UInt32)node_offset;
  s[3] = k_Blake2sp_IV_3_FROM_NODE_DEPTH(node_depth);
  s[4] = KIV(4);
  s[5] = KIV(5);
  s[6] = KIV(6);
  s[7] = KIV(7);

  STATE_T(s)[0] = 0;
  STATE_T(s)[1] = 0;
  STATE_F(s)[0] = 0;
  STATE_F(s)[1] = 0;
}


#ifdef Z7_BLAKE2S_USE_V128_FAST

static
Z7_NO_INLINE
#ifdef BLAKE2S_ATTRIB_128BIT
       BLAKE2S_ATTRIB_128BIT
#endif
void
Z7_FASTCALL
Blake2sp_InitState_V128_Fast(UInt32 *states)
{
#define STORE_128_PAIR_INIT_STATES_2(i, t0, t1) \
  { STORE_128_TO_STRUCT(states +  0 + 4 * (i), (t0)); \
    STORE_128_TO_STRUCT(states + 32 + 4 * (i), (t1)); \
  }
#define STORE_128_PAIR_INIT_STATES_1(i, mac) \
  { const __m128i t = mac; \
    STORE_128_PAIR_INIT_STATES_2(i, t, t) \
  }
#define STORE_128_PAIR_INIT_STATES_IV(i) \
    STORE_128_PAIR_INIT_STATES_1(i, GET_128_IV_WAY4(i))

  STORE_128_PAIR_INIT_STATES_1  (0, _mm_set1_epi32((Int32)k_Blake2sp_IV_0))
  STORE_128_PAIR_INIT_STATES_IV (1)
  {
    const __m128i t = GET_128_IV_WAY4(2);
    STORE_128_PAIR_INIT_STATES_2 (2,
        XOR_128(t, _mm_set_epi32(3, 2, 1, 0)),
        XOR_128(t, _mm_set_epi32(7, 6, 5, 4)))
  }
  STORE_128_PAIR_INIT_STATES_1  (3, _mm_set1_epi32((Int32)k_Blake2sp_IV_3_FROM_NODE_DEPTH(0)))
  STORE_128_PAIR_INIT_STATES_IV (4)
  STORE_128_PAIR_INIT_STATES_IV (5)
  STORE_128_PAIR_INIT_STATES_IV (6)
  STORE_128_PAIR_INIT_STATES_IV (7)
  STORE_128_PAIR_INIT_STATES_1  (16, _mm_set_epi32(0, 0, 0, 0))
  // printf("\n== exit Blake2sp_InitState_V128_Fast ctr=%d\n", states[64]);
}

#endif // Z7_BLAKE2S_USE_V128_FAST


#ifdef Z7_BLAKE2S_USE_AVX2_FAST

static
Z7_NO_INLINE
#ifdef BLAKE2S_ATTRIB_AVX2
       BLAKE2S_ATTRIB_AVX2
#endif
void
Z7_FASTCALL
Blake2sp_InitState_AVX2_Fast(UInt32 *states)
{
#define STORE_256_INIT_STATES(i, t) \
    STORE_256_TO_STRUCT(states + 8 * (i), t);
#define STORE_256_INIT_STATES_IV(i) \
    STORE_256_INIT_STATES(i, GET_256_IV_WAY8(i))

  STORE_256_INIT_STATES    (0,  _mm256_set1_epi32((Int32)k_Blake2sp_IV_0))
  STORE_256_INIT_STATES_IV (1)
  STORE_256_INIT_STATES    (2, XOR_256( GET_256_IV_WAY8(2),
                                _mm256_set_epi32(7, 6, 5, 4, 3, 2, 1, 0)))
  STORE_256_INIT_STATES    (3,  _mm256_set1_epi32((Int32)k_Blake2sp_IV_3_FROM_NODE_DEPTH(0)))
  STORE_256_INIT_STATES_IV (4)
  STORE_256_INIT_STATES_IV (5)
  STORE_256_INIT_STATES_IV (6)
  STORE_256_INIT_STATES_IV (7)
  STORE_256_INIT_STATES    (8, _mm256_set_epi32(0, 0, 0, 0, 0, 0, 0, 0))
  // printf("\n== exit Blake2sp_InitState_AVX2_Fast\n");
}

#endif // Z7_BLAKE2S_USE_AVX2_FAST



Z7_NO_INLINE
void Blake2sp_InitState(CBlake2sp *p)
{
  size_t i;
  // memset(p->states, 0, sizeof(p->states)); // for debug
  p->u.header.cycPos = 0;
#ifdef Z7_BLAKE2SP_USE_FUNCTIONS
  if (p->u.header.func_Init)
  {
    p->u.header.func_Init(p->states);
    return;
  }
#endif
  for (i = 0; i < Z7_BLAKE2SP_PARALLEL_DEGREE; i++)
    Blake2sp_Init_Spec(p->states + i * NSW, (unsigned)i, 0);
}

void Blake2sp_Init(CBlake2sp *p)
{
#ifdef Z7_BLAKE2SP_USE_FUNCTIONS
  p->u.header.func_Compress_Fast =
#ifdef Z7_BLAKE2S_USE_VECTORS
    g_Z7_BLAKE2SP_FUNC_COMPRESS_Fast;
#else
    NULL;
#endif

  p->u.header.func_Compress_Single =
#ifdef Z7_BLAKE2S_USE_VECTORS
    g_Z7_BLAKE2SP_FUNC_COMPRESS_Single;
#else
    NULL;
#endif

  p->u.header.func_Init =
#ifdef Z7_BLAKE2S_USE_VECTORS
    g_Z7_BLAKE2SP_FUNC_INIT_Init;
#else
    NULL;
#endif

  p->u.header.func_Final =
#ifdef Z7_BLAKE2S_USE_VECTORS
    g_Z7_BLAKE2SP_FUNC_INIT_Final;
#else
    NULL;
#endif
#endif

  Blake2sp_InitState(p);
}


void Blake2sp_Update(CBlake2sp *p, const Byte *data, size_t size)
{
  size_t pos;
  // printf("\nsize = 0x%6x, cycPos = %5u data = %p\n", (unsigned)size, (unsigned)p->u.header.cycPos, data);
  if (size == 0)
    return;
  pos = p->u.header.cycPos;
  // pos <  SUPER_BLOCK_SIZE * 2  : is expected
  // pos == SUPER_BLOCK_SIZE * 2  : is not expected, but is supported also
  {
    const size_t pos2 = pos & SUPER_BLOCK_MASK;
    if (pos2)
    {
      const size_t rem = SUPER_BLOCK_SIZE - pos2;
      if (rem > size)
      {
        p->u.header.cycPos = (unsigned)(pos + size);
        // cycPos < SUPER_BLOCK_SIZE * 2
        memcpy((Byte *)(void *)p->buf32 + pos, data, size);
        /* to simpilify the code here we don't try to process first superblock,
           if (cycPos > SUPER_BLOCK_SIZE * 2 - Z7_BLAKE2S_BLOCK_SIZE) */
        return;
      }
      // (rem <= size)
      memcpy((Byte *)(void *)p->buf32 + pos, data, rem);
      pos += rem;
      data += rem;
      size -= rem;
    }
  }

  // pos <= SUPER_BLOCK_SIZE * 2
  // pos  % SUPER_BLOCK_SIZE == 0
  if (pos)
  {
    /* pos == SUPER_BLOCK_SIZE ||
       pos == SUPER_BLOCK_SIZE * 2 */
    size_t end = pos;
    if (size > SUPER_BLOCK_SIZE - Z7_BLAKE2S_BLOCK_SIZE
        || (end -= SUPER_BLOCK_SIZE))
    {
      Z7_BLAKE2SP_Compress_Fast(p)(p->states,
          (const Byte *)(const void *)p->buf32,
          (const Byte *)(const void *)p->buf32 + end);
      if (pos -= end)
        memcpy(p->buf32, (const Byte *)(const void *)p->buf32
            + SUPER_BLOCK_SIZE, SUPER_BLOCK_SIZE);
    }
  }

  // pos == 0 || (pos == SUPER_BLOCK_SIZE && size <= SUPER_BLOCK_SIZE - Z7_BLAKE2S_BLOCK_SIZE)
  if (size > SUPER_BLOCK_SIZE * 2 - Z7_BLAKE2S_BLOCK_SIZE)
  {
    // pos == 0
    const Byte *end;
    const size_t size2 = (size - (SUPER_BLOCK_SIZE - Z7_BLAKE2S_BLOCK_SIZE + 1))
        & ~(size_t)SUPER_BLOCK_MASK;
    size -= size2;
    // size < SUPER_BLOCK_SIZE * 2
    end = data + size2;
    Z7_BLAKE2SP_Compress_Fast(p)(p->states, data, end);
    data = end;
  }
  
  if (size != 0)
  {
    memcpy((Byte *)(void *)p->buf32 + pos, data, size);
    pos += size;
  }
  p->u.header.cycPos = (unsigned)pos;
  // cycPos < SUPER_BLOCK_SIZE * 2
}


void Blake2sp_Final(CBlake2sp *p, Byte *digest)
{
  // UInt32 * const R_states = p->states;
  // printf("\nBlake2sp_Final \n");
#ifdef Z7_BLAKE2SP_USE_FUNCTIONS
  if (p->u.header.func_Final)
      p->u.header.func_Final(p->states);
#endif
  // printf("\n=====\nBlake2sp_Final \n");
  // PrintStates(p->states, 32);

  // (p->u.header.cycPos == SUPER_BLOCK_SIZE) can be processed in any branch:
  if (p->u.header.cycPos <= SUPER_BLOCK_SIZE)
  {
    unsigned pos;
    memset((Byte *)(void *)p->buf32 + p->u.header.cycPos,
        0, SUPER_BLOCK_SIZE - p->u.header.cycPos);
    STATE_F(&p->states[(Z7_BLAKE2SP_PARALLEL_DEGREE - 1) * NSW])[1] = BLAKE2S_FINAL_FLAG;
    for (pos = 0; pos < SUPER_BLOCK_SIZE; pos += Z7_BLAKE2S_BLOCK_SIZE)
    {
      UInt32 * const s = GET_STATE_TABLE_PTR_FROM_BYTE_POS(p->states, pos);
      Blake2s_Set_LastBlock(s)
      if (pos + Z7_BLAKE2S_BLOCK_SIZE > p->u.header.cycPos)
      {
        UInt32 delta = Z7_BLAKE2S_BLOCK_SIZE;
        if (pos < p->u.header.cycPos)
          delta -= p->u.header.cycPos & (Z7_BLAKE2S_BLOCK_SIZE - 1);
        // 0 < delta <= Z7_BLAKE2S_BLOCK_SIZE
        {
          const UInt32 v = STATE_T(s)[0];
          STATE_T(s)[1] -= v < delta; //  (v < delta) is same condition here as (v == 0)
          STATE_T(s)[0]  = v - delta;
        }
      }
    }
    // PrintStates(p->states, 16);
    Z7_BLAKE2SP_Compress_Single(p)(p->states,
        (Byte *)(void *)p->buf32,
        (Byte *)(void *)p->buf32 + SUPER_BLOCK_SIZE);
    // PrintStates(p->states, 16);
  }
  else
  {
    // (p->u.header.cycPos > SUPER_BLOCK_SIZE)
    unsigned pos;
    for (pos = 0; pos < SUPER_BLOCK_SIZE; pos += Z7_BLAKE2S_BLOCK_SIZE)
    {
      UInt32 * const s = GET_STATE_TABLE_PTR_FROM_BYTE_POS(p->states, pos);
      if (pos + SUPER_BLOCK_SIZE >= p->u.header.cycPos)
        Blake2s_Set_LastBlock(s)
    }
    if (p->u.header.cycPos <= SUPER_BLOCK_SIZE * 2 - Z7_BLAKE2S_BLOCK_SIZE)
      STATE_F(&p->states[(Z7_BLAKE2SP_PARALLEL_DEGREE - 1) * NSW])[1] = BLAKE2S_FINAL_FLAG;

    Z7_BLAKE2SP_Compress_Single(p)(p->states,
        (Byte *)(void *)p->buf32,
        (Byte *)(void *)p->buf32 + SUPER_BLOCK_SIZE);

    // if (p->u.header.cycPos > SUPER_BLOCK_SIZE * 2 - Z7_BLAKE2S_BLOCK_SIZE;
      STATE_F(&p->states[(Z7_BLAKE2SP_PARALLEL_DEGREE - 1) * NSW])[1] = BLAKE2S_FINAL_FLAG;

    // if (p->u.header.cycPos != SUPER_BLOCK_SIZE)
    {
      pos = SUPER_BLOCK_SIZE;
      for (;;)
      {
        UInt32 * const s = GET_STATE_TABLE_PTR_FROM_BYTE_POS(p->states, pos & SUPER_BLOCK_MASK);
        Blake2s_Set_LastBlock(s)
        pos += Z7_BLAKE2S_BLOCK_SIZE;
        if (pos >= p->u.header.cycPos)
        {
          if (pos != p->u.header.cycPos)
          {
            const UInt32 delta = pos - p->u.header.cycPos;
            const UInt32 v = STATE_T(s)[0];
            STATE_T(s)[1] -= v < delta;
            STATE_T(s)[0]  = v - delta;
            memset((Byte *)(void *)p->buf32 + p->u.header.cycPos, 0, delta);
          }
          break;
        }
      }
      Z7_BLAKE2SP_Compress_Single(p)(p->states,
          (Byte *)(void *)p->buf32 + SUPER_BLOCK_SIZE,
          (Byte *)(void *)p->buf32 + pos);
    }
  }

  {
    size_t pos;
    for (pos = 0; pos < SUPER_BLOCK_SIZE / 2; pos += Z7_BLAKE2S_BLOCK_SIZE / 2)
    {
      const UInt32 * const s = GET_STATE_TABLE_PTR_FROM_BYTE_POS(p->states, (pos * 2));
      Byte *dest = (Byte *)(void *)p->buf32 + pos;
      GET_DIGEST(s, dest)
    }
  }
  Blake2sp_Init_Spec(p->states, 0, 1);
  {
    size_t pos;
    for (pos = 0; pos < (Z7_BLAKE2SP_PARALLEL_DEGREE * Z7_BLAKE2S_DIGEST_SIZE)
        - Z7_BLAKE2S_BLOCK_SIZE; pos += Z7_BLAKE2S_BLOCK_SIZE)
    {
      Z7_BLAKE2SP_Compress_Single(p)(p->states,
          (const Byte *)(const void *)p->buf32 + pos,
          (const Byte *)(const void *)p->buf32 + pos + Z7_BLAKE2S_BLOCK_SIZE);
    }
  }
  // Blake2s_Final(p->states, 0, digest, p, (Byte *)(void *)p->buf32 + i);
  Blake2s_Set_LastBlock(p->states)
  STATE_F(p->states)[1] = BLAKE2S_FINAL_FLAG;
  {
    Z7_BLAKE2SP_Compress_Single(p)(p->states,
        (const Byte *)(const void *)p->buf32 + Z7_BLAKE2SP_PARALLEL_DEGREE / 2 * Z7_BLAKE2S_BLOCK_SIZE - Z7_BLAKE2S_BLOCK_SIZE,
        (const Byte *)(const void *)p->buf32 + Z7_BLAKE2SP_PARALLEL_DEGREE / 2 * Z7_BLAKE2S_BLOCK_SIZE);
  }
  GET_DIGEST(p->states, digest)
  // printf("\n Blake2sp_Final 555 numDataInBufs = %5u\n", (unsigned)p->u.header.numDataInBufs);
}


BoolInt Blake2sp_SetFunction(CBlake2sp *p, unsigned algo)
{
  // printf("\n========== setfunction = %d ======== \n",  algo);
#ifdef Z7_BLAKE2SP_USE_FUNCTIONS
  Z7_BLAKE2SP_FUNC_COMPRESS func = NULL;
  Z7_BLAKE2SP_FUNC_COMPRESS func_Single = NULL;
  Z7_BLAKE2SP_FUNC_INIT     func_Final = NULL;
  Z7_BLAKE2SP_FUNC_INIT     func_Init = NULL;
#else
  UNUSED_VAR(p)
#endif
  
#ifdef Z7_BLAKE2S_USE_VECTORS

  func = func_Single = Blake2sp_Compress2;

  if (algo != Z7_BLAKE2SP_ALGO_SCALAR)
  {
    // printf("\n========== setfunction NON-SCALER ======== \n");
    if (algo == Z7_BLAKE2SP_ALGO_DEFAULT)
    {
      func        = g_Z7_BLAKE2SP_FUNC_COMPRESS_Fast;
      func_Single = g_Z7_BLAKE2SP_FUNC_COMPRESS_Single;
      func_Init   = g_Z7_BLAKE2SP_FUNC_INIT_Init;
      func_Final  = g_Z7_BLAKE2SP_FUNC_INIT_Final;
    }
    else
    {
      if ((g_z7_Blake2sp_SupportedFlags & (1u << algo)) == 0)
        return False;

#ifdef Z7_BLAKE2S_USE_AVX2

      func_Single =
#if defined(Z7_BLAKE2S_USE_AVX2_WAY2)
        Blake2sp_Compress2_AVX2_Way2;
#else
        Z7_BLAKE2S_Compress2_V128;
#endif

#ifdef Z7_BLAKE2S_USE_AVX2_FAST
      if (algo == Z7_BLAKE2SP_ALGO_V256_FAST)
      {
        func = Blake2sp_Compress2_AVX2_Fast;
        func_Final = Blake2sp_Final_AVX2_Fast;
        func_Init  = Blake2sp_InitState_AVX2_Fast;
      }
      else
#endif
#ifdef Z7_BLAKE2S_USE_AVX2_WAY2
      if (algo == Z7_BLAKE2SP_ALGO_V256_WAY2)
        func = Blake2sp_Compress2_AVX2_Way2;
      else
#endif
#ifdef Z7_BLAKE2S_USE_AVX2_WAY4
      if (algo == Z7_BLAKE2SP_ALGO_V256_WAY4)
      {
        func_Single = func = Blake2sp_Compress2_AVX2_Way4;
      }
      else
#endif
#endif // avx2
      {
        if (algo == Z7_BLAKE2SP_ALGO_V128_FAST)
        {
          func       = Blake2sp_Compress2_V128_Fast;
          func_Final = Blake2sp_Final_V128_Fast;
          func_Init  = Blake2sp_InitState_V128_Fast;
          func_Single = Z7_BLAKE2S_Compress2_V128;
        }
        else
#ifdef Z7_BLAKE2S_USE_V128_WAY2
        if (algo == Z7_BLAKE2SP_ALGO_V128_WAY2)
          func = func_Single = Blake2sp_Compress2_V128_Way2;
        else
#endif
        {
          if (algo != Z7_BLAKE2SP_ALGO_V128_WAY1)
            return False;
          func = func_Single = Blake2sp_Compress2_V128_Way1;
        }
      }
    }
  }
#else // !VECTORS
  if (algo > 1) // Z7_BLAKE2SP_ALGO_SCALAR
    return False;
#endif // !VECTORS

#ifdef Z7_BLAKE2SP_USE_FUNCTIONS
  p->u.header.func_Compress_Fast = func;
  p->u.header.func_Compress_Single = func_Single;
  p->u.header.func_Final = func_Final;
  p->u.header.func_Init = func_Init;
#endif
  // printf("\n p->u.header.func_Compress = %p", p->u.header.func_Compress);
  return True;
}


void z7_Black2sp_Prepare(void)
{
#ifdef Z7_BLAKE2S_USE_VECTORS
  unsigned flags = 0; // (1u << Z7_BLAKE2SP_ALGO_V128_SCALAR);

  Z7_BLAKE2SP_FUNC_COMPRESS func_Fast = Blake2sp_Compress2;
  Z7_BLAKE2SP_FUNC_COMPRESS func_Single = Blake2sp_Compress2;
  Z7_BLAKE2SP_FUNC_INIT func_Init = NULL;
  Z7_BLAKE2SP_FUNC_INIT func_Final = NULL;

#if defined(MY_CPU_X86_OR_AMD64)
    #if defined(Z7_BLAKE2S_USE_AVX512_ALWAYS)
      if (CPU_IsSupported_AVX512F_AVX512VL())
    #endif
    #if defined(Z7_BLAKE2S_USE_SSE41)
      if (CPU_IsSupported_SSE41())
    #elif defined(Z7_BLAKE2S_USE_SSSE3)
      if (CPU_IsSupported_SSSE3())
    #elif !defined(MY_CPU_AMD64)
      if (CPU_IsSupported_SSE2())
    #endif
#endif
  {
    #if defined(Z7_BLAKE2S_USE_SSE41)
      // printf("\n========== Blake2s SSE41 128-bit\n");
    #elif defined(Z7_BLAKE2S_USE_SSSE3)
      // printf("\n========== Blake2s SSSE3 128-bit\n");
    #else
      // printf("\n========== Blake2s SSE2 128-bit\n");
    #endif
    // func_Fast = f_vector = Blake2sp_Compress2_V128_Way2;
    // printf("\n========== Blake2sp_Compress2_V128_Way2\n");
    func_Fast   =
    func_Single = Z7_BLAKE2S_Compress2_V128;
    flags |= (1u << Z7_BLAKE2SP_ALGO_V128_WAY1);
#ifdef Z7_BLAKE2S_USE_V128_WAY2
    flags |= (1u << Z7_BLAKE2SP_ALGO_V128_WAY2);
#endif
#ifdef Z7_BLAKE2S_USE_V128_FAST
    flags |= (1u << Z7_BLAKE2SP_ALGO_V128_FAST);
    func_Fast  = Blake2sp_Compress2_V128_Fast;
    func_Init  = Blake2sp_InitState_V128_Fast;
    func_Final = Blake2sp_Final_V128_Fast;
#endif

#ifdef Z7_BLAKE2S_USE_AVX2
#if defined(MY_CPU_X86_OR_AMD64)
    if (
    #if 0 && defined(Z7_BLAKE2S_USE_AVX512_ALWAYS)
        CPU_IsSupported_AVX512F_AVX512VL() &&
    #endif
        CPU_IsSupported_AVX2()
        )
#endif
    {
    // #pragma message ("=== Blake2s AVX2")
    // printf("\n========== Blake2s AVX2\n");
    
#ifdef Z7_BLAKE2S_USE_AVX2_WAY2
      func_Single = Blake2sp_Compress2_AVX2_Way2;
      flags |= (1u << Z7_BLAKE2SP_ALGO_V256_WAY2);
#endif
#ifdef Z7_BLAKE2S_USE_AVX2_WAY4
      flags |= (1u << Z7_BLAKE2SP_ALGO_V256_WAY4);
#endif

#ifdef Z7_BLAKE2S_USE_AVX2_FAST
      flags |= (1u << Z7_BLAKE2SP_ALGO_V256_FAST);
      func_Fast  = Blake2sp_Compress2_AVX2_Fast;
      func_Init  = Blake2sp_InitState_AVX2_Fast;
      func_Final = Blake2sp_Final_AVX2_Fast;
#elif defined(Z7_BLAKE2S_USE_AVX2_WAY4)
      func_Fast  = Blake2sp_Compress2_AVX2_Way4;
#elif defined(Z7_BLAKE2S_USE_AVX2_WAY2)
      func_Fast  = Blake2sp_Compress2_AVX2_Way2;
#endif
    } // avx2
#endif // avx2
  } // sse*
  g_Z7_BLAKE2SP_FUNC_COMPRESS_Fast   = func_Fast;
  g_Z7_BLAKE2SP_FUNC_COMPRESS_Single = func_Single;
  g_Z7_BLAKE2SP_FUNC_INIT_Init       = func_Init;
  g_Z7_BLAKE2SP_FUNC_INIT_Final      = func_Final;
  g_z7_Blake2sp_SupportedFlags = flags;
  // printf("\nflags=%x\n", flags);
#endif // vectors
}

/*
#ifdef Z7_BLAKE2S_USE_VECTORS
void align_test2(CBlake2sp *sp);
void align_test2(CBlake2sp *sp)
{
  __m128i a = LOAD_128(sp->states);
  D_XOR_128(a, LOAD_128(sp->states + 4));
  STORE_128(sp->states, a);
}
void align_test2(void);
void align_test2(void)
{
  CBlake2sp sp;
  Blake2sp_Init(&sp);
  Blake2sp_Update(&sp, NULL, 0);
}
#endif
*/
