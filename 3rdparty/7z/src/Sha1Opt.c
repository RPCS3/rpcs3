/* Sha1Opt.c -- SHA-1 optimized code for SHA-1 hardware instructions
2023-04-02 : Igor Pavlov : Public domain */

#include "Precomp.h"
#include "Compiler.h"
#include "CpuArch.h"

#if defined(_MSC_VER)
#if (_MSC_VER < 1900) && (_MSC_VER >= 1200)
// #define USE_MY_MM
#endif
#endif

#ifdef MY_CPU_X86_OR_AMD64
  #if defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 1600) // fix that check
      #define USE_HW_SHA
  #elif defined(Z7_LLVM_CLANG_VERSION)  && (Z7_LLVM_CLANG_VERSION  >= 30800) \
     || defined(Z7_APPLE_CLANG_VERSION) && (Z7_APPLE_CLANG_VERSION >= 50100) \
     || defined(Z7_GCC_VERSION)         && (Z7_GCC_VERSION         >= 40900)
      #define USE_HW_SHA
      #if !defined(_INTEL_COMPILER)
      // icc defines __GNUC__, but icc doesn't support __attribute__(__target__)
      #if !defined(__SHA__) || !defined(__SSSE3__)
        #define ATTRIB_SHA __attribute__((__target__("sha,ssse3")))
      #endif
      #endif
  #elif defined(_MSC_VER)
    #ifdef USE_MY_MM
      #define USE_VER_MIN 1300
    #else
      #define USE_VER_MIN 1900
    #endif
    #if (_MSC_VER >= USE_VER_MIN)
      #define USE_HW_SHA
    #endif
  #endif
// #endif // MY_CPU_X86_OR_AMD64

#ifdef USE_HW_SHA

// #pragma message("Sha1 HW")

// sse/sse2/ssse3:
#include <tmmintrin.h>
// sha*:
#include <immintrin.h>

#if defined (__clang__) && defined(_MSC_VER)
  // #if !defined(__SSSE3__)
  // #endif
  #if !defined(__SHA__)
    #include <shaintrin.h>
  #endif
#else

#ifdef USE_MY_MM
#include "My_mm.h"
#endif

#endif

/*
SHA1 uses:
SSE2:
  _mm_loadu_si128
  _mm_storeu_si128
  _mm_set_epi32
  _mm_add_epi32
  _mm_shuffle_epi32 / pshufd
  _mm_xor_si128
  _mm_cvtsi128_si32
  _mm_cvtsi32_si128
SSSE3:
  _mm_shuffle_epi8 / pshufb

SHA:
  _mm_sha1*
*/


#define XOR_SI128(dest, src)      dest = _mm_xor_si128(dest, src);
#define SHUFFLE_EPI8(dest, mask)  dest = _mm_shuffle_epi8(dest, mask);
#define SHUFFLE_EPI32(dest, mask) dest = _mm_shuffle_epi32(dest, mask);
#ifdef __clang__
#define SHA1_RNDS4_RET_TYPE_CAST (__m128i)
#else
#define SHA1_RNDS4_RET_TYPE_CAST
#endif
#define SHA1_RND4(abcd, e0, f)    abcd = SHA1_RNDS4_RET_TYPE_CAST _mm_sha1rnds4_epu32(abcd, e0, f);
#define SHA1_NEXTE(e, m)          e = _mm_sha1nexte_epu32(e, m);
#define ADD_EPI32(dest, src)      dest = _mm_add_epi32(dest, src);
#define SHA1_MSG1(dest, src)      dest = _mm_sha1msg1_epu32(dest, src);
#define SHA1_MSG2(dest, src)      dest = _mm_sha1msg2_epu32(dest, src);


#define LOAD_SHUFFLE(m, k) \
    m = _mm_loadu_si128((const __m128i *)(const void *)(data + (k) * 16)); \
    SHUFFLE_EPI8(m, mask) \

#define SM1(m0, m1, m2, m3) \
    SHA1_MSG1(m0, m1) \

#define SM2(m0, m1, m2, m3) \
    XOR_SI128(m3, m1) \
    SHA1_MSG2(m3, m2) \

#define SM3(m0, m1, m2, m3) \
    XOR_SI128(m3, m1) \
    SM1(m0, m1, m2, m3) \
    SHA1_MSG2(m3, m2) \

#define NNN(m0, m1, m2, m3)

















#define R4(k, e0, e1, m0, m1, m2, m3, OP) \
    e1 = abcd; \
    SHA1_RND4(abcd, e0, (k) / 5) \
    SHA1_NEXTE(e1, m1) \
    OP(m0, m1, m2, m3) \

#define R16(k, mx, OP0, OP1, OP2, OP3) \
    R4 ( (k)*4+0, e0,e1, m0,m1,m2,m3, OP0 ) \
    R4 ( (k)*4+1, e1,e0, m1,m2,m3,m0, OP1 ) \
    R4 ( (k)*4+2, e0,e1, m2,m3,m0,m1, OP2 ) \
    R4 ( (k)*4+3, e1,e0, m3,mx,m1,m2, OP3 ) \

#define PREPARE_STATE \
    SHUFFLE_EPI32 (abcd, 0x1B) \
    SHUFFLE_EPI32 (e0,   0x1B) \





void Z7_FASTCALL Sha1_UpdateBlocks_HW(UInt32 state[5], const Byte *data, size_t numBlocks);
#ifdef ATTRIB_SHA
ATTRIB_SHA
#endif
void Z7_FASTCALL Sha1_UpdateBlocks_HW(UInt32 state[5], const Byte *data, size_t numBlocks)
{
  const __m128i mask = _mm_set_epi32(0x00010203, 0x04050607, 0x08090a0b, 0x0c0d0e0f);

  __m128i abcd, e0;
  
  if (numBlocks == 0)
    return;
  
  abcd = _mm_loadu_si128((const __m128i *) (const void *) &state[0]); // dbca
  e0 = _mm_cvtsi32_si128((int)state[4]); // 000e
  
  PREPARE_STATE
  
  do
  {
    __m128i abcd_save, e2;
    __m128i m0, m1, m2, m3;
    __m128i e1;
    

    abcd_save = abcd;
    e2 = e0;

    LOAD_SHUFFLE (m0, 0)
    LOAD_SHUFFLE (m1, 1)
    LOAD_SHUFFLE (m2, 2)
    LOAD_SHUFFLE (m3, 3)

    ADD_EPI32(e0, m0)
    
    R16 ( 0, m0, SM1, SM3, SM3, SM3 )
    R16 ( 1, m0, SM3, SM3, SM3, SM3 )
    R16 ( 2, m0, SM3, SM3, SM3, SM3 )
    R16 ( 3, m0, SM3, SM3, SM3, SM3 )
    R16 ( 4, e2, SM2, NNN, NNN, NNN )
    
    ADD_EPI32(abcd, abcd_save)
    
    data += 64;
  }
  while (--numBlocks);

  PREPARE_STATE

  _mm_storeu_si128((__m128i *) (void *) state, abcd);
  *(state+4) = (UInt32)_mm_cvtsi128_si32(e0);
}

#endif // USE_HW_SHA

#elif defined(MY_CPU_ARM_OR_ARM64)

  #if defined(__clang__)
    #if (__clang_major__ >= 8) // fix that check
      #define USE_HW_SHA
    #endif
  #elif defined(__GNUC__)
    #if (__GNUC__ >= 6) // fix that check
      #define USE_HW_SHA
    #endif
  #elif defined(_MSC_VER)
    #if _MSC_VER >= 1910
      #define USE_HW_SHA
    #endif
  #endif

#ifdef USE_HW_SHA

// #pragma message("=== Sha1 HW === ")

#if defined(__clang__) || defined(__GNUC__)
  #ifdef MY_CPU_ARM64
    #define ATTRIB_SHA __attribute__((__target__("+crypto")))
  #else
    #define ATTRIB_SHA __attribute__((__target__("fpu=crypto-neon-fp-armv8")))
  #endif
#else
  // _MSC_VER
  // for arm32
  #define _ARM_USE_NEW_NEON_INTRINSICS
#endif

#if defined(_MSC_VER) && defined(MY_CPU_ARM64)
#include <arm64_neon.h>
#else
#include <arm_neon.h>
#endif

typedef uint32x4_t v128;
// typedef __n128 v128; // MSVC

#ifdef MY_CPU_BE
  #define MY_rev32_for_LE(x)
#else
  #define MY_rev32_for_LE(x) x = vreinterpretq_u32_u8(vrev32q_u8(vreinterpretq_u8_u32(x)))
#endif

#define LOAD_128(_p)      (*(const v128 *)(const void *)(_p))
#define STORE_128(_p, _v) *(v128 *)(void *)(_p) = (_v)

#define LOAD_SHUFFLE(m, k) \
    m = LOAD_128((data + (k) * 16)); \
    MY_rev32_for_LE(m); \

#define SU0(dest, src2, src3) dest = vsha1su0q_u32(dest, src2, src3);
#define SU1(dest, src)        dest = vsha1su1q_u32(dest, src);
#define C(e)                  abcd = vsha1cq_u32(abcd, e, t);
#define P(e)                  abcd = vsha1pq_u32(abcd, e, t);
#define M(e)                  abcd = vsha1mq_u32(abcd, e, t);
#define H(e)                  e = vsha1h_u32(vgetq_lane_u32(abcd, 0))
#define T(m, c)               t = vaddq_u32(m, c)

void Z7_FASTCALL Sha1_UpdateBlocks_HW(UInt32 state[8], const Byte *data, size_t numBlocks);
#ifdef ATTRIB_SHA
ATTRIB_SHA
#endif
void Z7_FASTCALL Sha1_UpdateBlocks_HW(UInt32 state[8], const Byte *data, size_t numBlocks)
{
  v128 abcd;
  v128 c0, c1, c2, c3;
  uint32_t e0;

  if (numBlocks == 0)
    return;

  c0 = vdupq_n_u32(0x5a827999);
  c1 = vdupq_n_u32(0x6ed9eba1);
  c2 = vdupq_n_u32(0x8f1bbcdc);
  c3 = vdupq_n_u32(0xca62c1d6);

  abcd = LOAD_128(&state[0]);
  e0 = state[4];
  
  do
  {
    v128 abcd_save;
    v128 m0, m1, m2, m3;
    v128 t;
    uint32_t e0_save, e1;

    abcd_save = abcd;
    e0_save = e0;
    
    LOAD_SHUFFLE (m0, 0)
    LOAD_SHUFFLE (m1, 1)
    LOAD_SHUFFLE (m2, 2)
    LOAD_SHUFFLE (m3, 3)
                     
    T(m0, c0);                                  H(e1); C(e0);
    T(m1, c0);  SU0(m0, m1, m2);                H(e0); C(e1);
    T(m2, c0);  SU0(m1, m2, m3);  SU1(m0, m3);  H(e1); C(e0);
    T(m3, c0);  SU0(m2, m3, m0);  SU1(m1, m0);  H(e0); C(e1);
    T(m0, c0);  SU0(m3, m0, m1);  SU1(m2, m1);  H(e1); C(e0);
    T(m1, c1);  SU0(m0, m1, m2);  SU1(m3, m2);  H(e0); P(e1);
    T(m2, c1);  SU0(m1, m2, m3);  SU1(m0, m3);  H(e1); P(e0);
    T(m3, c1);  SU0(m2, m3, m0);  SU1(m1, m0);  H(e0); P(e1);
    T(m0, c1);  SU0(m3, m0, m1);  SU1(m2, m1);  H(e1); P(e0);
    T(m1, c1);  SU0(m0, m1, m2);  SU1(m3, m2);  H(e0); P(e1);
    T(m2, c2);  SU0(m1, m2, m3);  SU1(m0, m3);  H(e1); M(e0);
    T(m3, c2);  SU0(m2, m3, m0);  SU1(m1, m0);  H(e0); M(e1);
    T(m0, c2);  SU0(m3, m0, m1);  SU1(m2, m1);  H(e1); M(e0);
    T(m1, c2);  SU0(m0, m1, m2);  SU1(m3, m2);  H(e0); M(e1);
    T(m2, c2);  SU0(m1, m2, m3);  SU1(m0, m3);  H(e1); M(e0);
    T(m3, c3);  SU0(m2, m3, m0);  SU1(m1, m0);  H(e0); P(e1);
    T(m0, c3);  SU0(m3, m0, m1);  SU1(m2, m1);  H(e1); P(e0);
    T(m1, c3);                    SU1(m3, m2);  H(e0); P(e1);
    T(m2, c3);                                  H(e1); P(e0);
    T(m3, c3);                                  H(e0); P(e1);
                                                                                                                     
    abcd = vaddq_u32(abcd, abcd_save);
    e0 += e0_save;
    
    data += 64;
  }
  while (--numBlocks);

  STORE_128(&state[0], abcd);
  state[4] = e0;
}

#endif // USE_HW_SHA

#endif // MY_CPU_ARM_OR_ARM64


#ifndef USE_HW_SHA

// #error Stop_Compiling_UNSUPPORTED_SHA
// #include <stdlib.h>

// #include "Sha1.h"
void Z7_FASTCALL Sha1_UpdateBlocks(UInt32 state[5], const Byte *data, size_t numBlocks);

#pragma message("Sha1   HW-SW stub was used")

void Z7_FASTCALL Sha1_UpdateBlocks_HW(UInt32 state[5], const Byte *data, size_t numBlocks);
void Z7_FASTCALL Sha1_UpdateBlocks_HW(UInt32 state[5], const Byte *data, size_t numBlocks)
{
  Sha1_UpdateBlocks(state, data, numBlocks);
  /*
  UNUSED_VAR(state);
  UNUSED_VAR(data);
  UNUSED_VAR(numBlocks);
  exit(1);
  return;
  */
}

#endif

#undef SU0
#undef SU1
#undef C
#undef P
#undef M
#undef H
#undef T
#undef MY_rev32_for_LE
#undef NNN
#undef LOAD_128
#undef STORE_128
#undef LOAD_SHUFFLE
#undef SM1
#undef SM2
#undef SM3
#undef NNN
#undef R4
#undef R16
#undef PREPARE_STATE
#undef USE_HW_SHA
#undef ATTRIB_SHA
#undef USE_VER_MIN
