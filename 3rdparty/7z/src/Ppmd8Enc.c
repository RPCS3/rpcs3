/* Sha1.c -- SHA-1 Hash
2024-03-01 : Igor Pavlov : Public domain
This code is based on public domain code of Steve Reid from Wei Dai's Crypto++ library. */

#include "Precomp.h"

#include <string.h>

#include "CpuArch.h"
#include "RotateDefs.h"
#include "Sha1.h"

#if defined(_MSC_VER) && (_MSC_VER < 1900)
// #define USE_MY_MM
#endif

#ifdef MY_CPU_X86_OR_AMD64
  #if   defined(Z7_LLVM_CLANG_VERSION)  && (Z7_LLVM_CLANG_VERSION  >= 30800) \
     || defined(Z7_APPLE_CLANG_VERSION) && (Z7_APPLE_CLANG_VERSION >= 50100) \
     || defined(Z7_GCC_VERSION)         && (Z7_GCC_VERSION         >= 40900) \
     || defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 1600) \
     || defined(_MSC_VER) && (_MSC_VER >= 1200)
      #define Z7_COMPILER_SHA1_SUPPORTED
  #endif
#elif defined(MY_CPU_ARM_OR_ARM64) && defined(MY_CPU_LE) \
   && (!defined(Z7_MSC_VER_ORIGINAL) || (_MSC_VER >= 1929) && (_MSC_FULL_VER >= 192930037))
  #if   defined(__ARM_FEATURE_SHA2) \
     || defined(__ARM_FEATURE_CRYPTO)
    #define Z7_COMPILER_SHA1_SUPPORTED
  #else
    #if  defined(MY_CPU_ARM64) \
      || defined(__ARM_ARCH) && (__ARM_ARCH >= 4) \
      || defined(Z7_MSC_VER_ORIGINAL)
    #if  defined(__ARM_FP) && \
          (   defined(Z7_CLANG_VERSION) && (Z7_CLANG_VERSION >= 30800) \
           || defined(__GNUC__) && (__GNUC__ >= 6) \
          ) \
      || defined(Z7_MSC_VER_ORIGINAL) && (_MSC_VER >= 1910)
    #if  defined(MY_CPU_ARM64) \
      || !defined(Z7_CLANG_VERSION) \
      || defined(__ARM_NEON) && \
          (Z7_CLANG_VERSION < 170000 || \
           Z7_CLANG_VERSION > 170001)
      #define Z7_COMPILER_SHA1_SUPPORTED
    #endif
    #endif
    #endif
  #endif
#endif

void Z7_FASTCALL Sha1_UpdateBlocks(UInt32 state[5], const Byte *data, size_t numBlocks);

#ifdef Z7_COMPILER_SHA1_SUPPORTED
  void Z7_FASTCALL Sha1_UpdateBlocks_HW(UInt32 state[5], const Byte *data, size_t numBlocks);

  static SHA1_FUNC_UPDATE_BLOCKS g_SHA1_FUNC_UPDATE_BLOCKS = Sha1_UpdateBlocks;
  static SHA1_FUNC_UPDATE_BLOCKS g_SHA1_FUNC_UPDATE_BLOCKS_HW;

  #define SHA1_UPDATE_BLOCKS(p) p->func_UpdateBlocks
#else
  #define SHA1_UPDATE_BLOCKS(p) Sha1_UpdateBlocks
#endif


BoolInt Sha1_SetFunction(CSha1 *p, unsigned algo)
{
  SHA1_FUNC_UPDATE_BLOCKS func = Sha1_UpdateBlocks;
  
  #ifdef Z7_COMPILER_SHA1_SUPPORTED
    if (algo != SHA1_ALGO_SW)
    {
      if (algo == SHA1_ALGO_DEFAULT)
        func = g_SHA1_FUNC_UPDATE_BLOCKS;
      else
      {
        if (algo != SHA1_ALGO_HW)
          return False;
        func = g_SHA1_FUNC_UPDATE_BLOCKS_HW;
        if (!func)
          return False;
      }
    }
  #else
    if (algo > 1)
      return False;
  #endif

  p->func_UpdateBlocks = func;
  return True;
}


/* define it for speed optimization */
// #define Z7_SHA1_UNROLL

// allowed unroll steps: (1, 2, 4, 5, 20)

#undef Z7_SHA1_BIG_W
#ifdef Z7_SHA1_UNROLL
  #define STEP_PRE  20
  #define STEP_MAIN 20
#else
  #define Z7_SHA1_BIG_W
  #define STEP_PRE  5
  #define STEP_MAIN 5
#endif


#ifdef Z7_SHA1_BIG_W
  #define kNumW 80
  #define w(i) W[i]
#else
  #define kNumW 16
  #define w(i) W[(i)&15]
#endif

#define w0(i) (W[i] = GetBe32(data + (size_t)(i) * 4))
#define w1(i) (w(i) = rotlFixed(w((size_t)(i)-3) ^ w((size_t)(i)-8) ^ w((size_t)(i)-14) ^ w((size_t)(i)-16), 1))

#define f0(x,y,z)  ( 0x5a827999 + (z^(x&(y^z))) )
#define f1(x,y,z)  ( 0x6ed9eba1 + (x^y^z) )
#define f2(x,y,z)  ( 0x8f1bbcdc + ((x&y)|(z&(x|y))) )
#define f3(x,y,z)  ( 0xca62c1d6 + (x^y^z) )

/*
#define T1(fx, ww) \
    tmp = e + fx(b,c,d) + ww + rotlFixed(a, 5); \
    e = d; \
    d = c; \
    c = rotlFixed(b, 30); \
    b = a; \
    a = tmp; \
*/

#define T5(a,b,c,d,e, fx, ww) \
    e += fx(b,c,d) + ww + rotlFixed(a, 5); \
    b = rotlFixed(b, 30); \


/*
#define R1(i, fx, wx) \
    T1 ( fx, wx(i)); \

#define R2(i, fx, wx) \
    R1 ( (i)    , fx, wx); \
    R1 ( (i) + 1, fx, wx); \

#define R4(i, fx, wx) \
    R2 ( (i)    , fx, wx); \
    R2 ( (i) + 2, fx, wx); \
*/

#define M5(i, fx, wx0, wx1) \
    T5 ( a,b,c,d,e, fx, wx0((i)  ) ) \
    T5 ( e,a,b,c,d, fx, wx1((i)+1) ) \
    T5 ( d,e,a,b,c, fx, wx1((i)+2) ) \
    T5 ( c,d,e,a,b, fx, wx1((i)+3) ) \
    T5 ( b,c,d,e,a, fx, wx1((i)+4) ) \

#define R5(i, fx, wx) \
    M5 ( i, fx, wx, wx) \


#if STEP_PRE > 5

  #define R20_START \
    R5 (  0, f0, w0) \
    R5 (  5, f0, w0) \
    R5 ( 10, f0, w0) \
    M5 ( 15, f0, w0, w1) \
  
  #elif STEP_PRE == 5
  
  #define R20_START \
    { size_t i; for (i = 0; i < 15; i += STEP_PRE) \
      { R5(i, f0, w0) } } \
    M5 ( 15, f0, w0, w1) \

#else

  #if STEP_PRE == 1
    #define R_PRE R1
  #elif STEP_PRE == 2
    #define R_PRE R2
  #elif STEP_PRE == 4
    #define R_PRE R4
  #endif

  #define R20_START \
    { size_t i; for (i = 0; i < 16; i += STEP_PRE) \
      { R_PRE(i, f0, w0) } } \
    R4 ( 16, f0, w1) \

#endif



#if STEP_MAIN > 5

  #define R20(ii, fx) \
    R5 ( (ii)     , fx, w1) \
    R5 ( (ii) + 5 , fx, w1) \
    R5 ( (ii) + 10, fx, w1) \
    R5 ( (ii) + 15, fx, w1) \

#else

  #if STEP_MAIN == 1
    #define R_MAIN R1
  #elif STEP_MAIN == 2
    #define R_MAIN R2
  #elif STEP_MAIN == 4
    #define R_MAIN R4
  #elif STEP_MAIN == 5
    #define R_MAIN R5
  #endif

  #define R20(ii, fx)  \
    { size_t i; for (i = (ii); i < (ii) + 20; i += STEP_MAIN) \
      { R_MAIN(i, fx, w1) } } \

#endif



void Sha1_InitState(CSha1 *p)
{
  p->count = 0;
  p->state[0] = 0x67452301;
  p->state[1] = 0xEFCDAB89;
  p->state[2] = 0x98BADCFE;
  p->state[3] = 0x10325476;
  p->state[4] = 0xC3D2E1F0;
}

void Sha1_Init(CSha1 *p)
{
  p->func_UpdateBlocks =
  #ifdef Z7_COMPILER_SHA1_SUPPORTED
      g_SHA1_FUNC_UPDATE_BLOCKS;
  #else
      NULL;
  #endif
  Sha1_InitState(p);
}


Z7_NO_INLINE
void Z7_FASTCALL Sha1_UpdateBlocks(UInt32 state[5], const Byte *data, size_t numBlocks)
{
  UInt32 a, b, c, d, e;
  UInt32 W[kNumW];
  // if (numBlocks != 0x1264378347) return;
  if (numBlocks == 0)
    return;

  a = state[0];
  b = state[1];
  c = state[2];
  d = state[3];
  e = state[4];

  do
  {
  #if STEP_PRE < 5 || STEP_MAIN < 5
  UInt32 tmp;
  #endif

  R20_START
  R20(20, f1)
  R20(40, f2)
  R20(60, f3)

  a += state[0];
  b += state[1];
  c += state[2];
  d += state[3];
  e += state[4];

  state[0] = a;
  state[1] = b;
  state[2] = c;
  state[3] = d;
  state[4] = e;

  data += 64;
  }
  while (--numBlocks);
}


#define Sha1_UpdateBlock(p) SHA1_UPDATE_BLOCKS(p)(p->state, p->buffer, 1)

void Sha1_Update(CSha1 *p, const Byte *data, size_t size)
{
  if (size == 0)
    return;

  {
    unsigned pos = (unsigned)p->count & 0x3F;
    unsigned num;
    
    p->count += size;
    
    num = 64 - pos;
    if (num > size)
    {
      memcpy(p->buffer + pos, data, size);
      return;
    }
    
    if (pos != 0)
    {
      size -= num;
      memcpy(p->buffer + pos, data, num);
      data += num;
      Sha1_UpdateBlock(p);
    }
  }
  {
    size_t numBlocks = size >> 6;
    SHA1_UPDATE_BLOCKS(p)(p->state, data, numBlocks);
    size &= 0x3F;
    if (size == 0)
      return;
    data += (numBlocks << 6);
    memcpy(p->buffer, data, size);
  }
}


void Sha1_Final(CSha1 *p, Byte *digest)
{
  unsigned pos = (unsigned)p->count & 0x3F;
  

  p->buffer[pos++] = 0x80;
  
  if (pos > (64 - 8))
  {
    while (pos != 64) { p->buffer[pos++] = 0; }
    // memset(&p->buf.buffer[pos], 0, 64 - pos);
    Sha1_UpdateBlock(p);
    pos = 0;
  }

  /*
  if (pos & 3)
  {
    p->buffer[pos] = 0;
    p->buffer[pos + 1] = 0;
    p->buffer[pos + 2] = 0;
    pos += 3;
    pos &= ~3;
  }
  {
    for (; pos < 64 - 8; pos += 4)
      *(UInt32 *)(&p->buffer[pos]) = 0;
  }
  */

  memset(&p->buffer[pos], 0, (64 - 8) - pos);

  {
    const UInt64 numBits = (p->count << 3);
    SetBe32(p->buffer + 64 - 8, (UInt32)(numBits >> 32))
    SetBe32(p->buffer + 64 - 4, (UInt32)(numBits))
  }
  
  Sha1_UpdateBlock(p);

  SetBe32(digest,      p->state[0])
  SetBe32(digest + 4,  p->state[1])
  SetBe32(digest + 8,  p->state[2])
  SetBe32(digest + 12, p->state[3])
  SetBe32(digest + 16, p->state[4])
  



  Sha1_InitState(p);
}


void Sha1_PrepareBlock(const CSha1 *p, Byte *block, unsigned size)
{
  const UInt64 numBits = (p->count + size) << 3;
  SetBe32(&((UInt32 *)(void *)block)[SHA1_NUM_BLOCK_WORDS - 2], (UInt32)(numBits >> 32))
  SetBe32(&((UInt32 *)(void *)block)[SHA1_NUM_BLOCK_WORDS - 1], (UInt32)(numBits))
  // SetBe32((UInt32 *)(block + size), 0x80000000);
  SetUi32((UInt32 *)(void *)(block + size), 0x80)
  size += 4;
  while (size != (SHA1_NUM_BLOCK_WORDS - 2) * 4)
  {
    *((UInt32 *)(void *)(block + size)) = 0;
    size += 4;
  }
}

void Sha1_GetBlockDigest(const CSha1 *p, const Byte *data, Byte *destDigest)
{
  MY_ALIGN (16)
  UInt32 st[SHA1_NUM_DIGEST_WORDS];

  st[0] = p->state[0];
  st[1] = p->state[1];
  st[2] = p->state[2];
  st[3] = p->state[3];
  st[4] = p->state[4];
  
  SHA1_UPDATE_BLOCKS(p)(st, data, 1);

  SetBe32(destDigest + 0    , st[0])
  SetBe32(destDigest + 1 * 4, st[1])
  SetBe32(destDigest + 2 * 4, st[2])
  SetBe32(destDigest + 3 * 4, st[3])
  SetBe32(destDigest + 4 * 4, st[4])
}


void Sha1Prepare(void)
{
  #ifdef Z7_COMPILER_SHA1_SUPPORTED
  SHA1_FUNC_UPDATE_BLOCKS f, f_hw;
  f = Sha1_UpdateBlocks;
  f_hw = NULL;
  #ifdef MY_CPU_X86_OR_AMD64
  #ifndef USE_MY_MM
  if (CPU_IsSupported_SHA()
      && CPU_IsSupported_SSSE3()
      // && CPU_IsSupported_SSE41()
      )
  #endif
  #else
  if (CPU_IsSupported_SHA1())
  #endif
  {
    // printf("\n========== HW SHA1 ======== \n");
    #if 0 && defined(MY_CPU_ARM_OR_ARM64) && defined(_MSC_VER)
    /* there was bug in MSVC compiler for ARM64 -O2 before version VS2019 16.10 (19.29.30037).
       It generated incorrect SHA-1 code.
       21.03 : we test sha1-hardware code at runtime initialization */

      #pragma message("== SHA1 code: MSC compiler : failure-check code was inserted")

      UInt32 state[5] = { 0, 1, 2, 3, 4 } ;
      Byte data[64];
      unsigned i;
      for (i = 0; i < sizeof(data); i += 2)
      {
        data[i    ] = (Byte)(i);
        data[i + 1] = (Byte)(i + 1);
      }

      Sha1_UpdateBlocks_HW(state, data, sizeof(data) / 64);
    
      if (   state[0] != 0x9acd7297
          || state[1] != 0x4624d898
          || state[2] != 0x0bf079f0
          || state[3] != 0x031e61b3
          || state[4] != 0x8323fe20)
      {
        // printf("\n========== SHA-1 hardware version failure ======== \n");
      }
      else
    #endif
      {
        f = f_hw = Sha1_UpdateBlocks_HW;
      }
  }
  g_SHA1_FUNC_UPDATE_BLOCKS    = f;
  g_SHA1_FUNC_UPDATE_BLOCKS_HW = f_hw;
  #endif
}

#undef kNumW
#undef w
#undef w0
#undef w1
#undef f0
#undef f1
#undef f2
#undef f3
#undef T1
#undef T5
#undef M5
#undef R1
#undef R2
#undef R4
#undef R5
#undef R20_START
#undef R_PRE
#undef R_MAIN
#undef STEP_PRE
#undef STEP_MAIN
#undef Z7_SHA1_BIG_W
#undef Z7_SHA1_UNROLL
#undef Z7_COMPILER_SHA1_SUPPORTED
