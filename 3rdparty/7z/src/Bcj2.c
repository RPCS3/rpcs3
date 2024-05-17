/* Blake2.h -- BLAKE2sp Hash
2024-01-17 : Igor Pavlov : Public domain */

#ifndef ZIP7_INC_BLAKE2_H
#define ZIP7_INC_BLAKE2_H

#include "7zTypes.h"

#if 0
#include "Compiler.h"
#include "CpuArch.h"
#if defined(MY_CPU_X86_OR_AMD64)
#if defined(__SSE2__) \
    || defined(_MSC_VER) && _MSC_VER > 1200 \
    || defined(Z7_GCC_VERSION) && (Z7_GCC_VERSION >= 30300) \
    || defined(__clang__) \
    || defined(__INTEL_COMPILER)
#include <emmintrin.h> // SSE2
#endif

#if defined(__AVX2__) \
    || defined(Z7_GCC_VERSION) && (Z7_GCC_VERSION >= 40900) \
    || defined(Z7_APPLE_CLANG_VERSION) && (Z7_APPLE_CLANG_VERSION >= 40600) \
    || defined(Z7_LLVM_CLANG_VERSION) && (Z7_LLVM_CLANG_VERSION >= 30100) \
    || defined(Z7_MSC_VER_ORIGINAL) && (Z7_MSC_VER_ORIGINAL >= 1800) \
    || defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 1400)
#include <immintrin.h>
#if defined(__clang__)
#include <avxintrin.h>
#include <avx2intrin.h>
#endif
#endif // avx2
#endif // MY_CPU_X86_OR_AMD64
#endif // 0

EXTERN_C_BEGIN

#define Z7_BLAKE2S_BLOCK_SIZE         64
#define Z7_BLAKE2S_DIGEST_SIZE        32
#define Z7_BLAKE2SP_PARALLEL_DEGREE   8
#define Z7_BLAKE2SP_NUM_STRUCT_WORDS  16

#if 1 || defined(Z7_BLAKE2SP_USE_FUNCTIONS)
typedef void (Z7_FASTCALL *Z7_BLAKE2SP_FUNC_COMPRESS)(UInt32 *states, const Byte *data, const Byte *end);
typedef void (Z7_FASTCALL *Z7_BLAKE2SP_FUNC_INIT)(UInt32 *states);
#endif

// it's required that CBlake2sp is aligned for 32-bytes,
// because the code can use unaligned access with sse and avx256.
// but 64-bytes alignment can be better.
MY_ALIGN(64)
typedef struct
{
  union
  {
#if 0
#if defined(MY_CPU_X86_OR_AMD64)
#if defined(__SSE2__) \
    || defined(_MSC_VER) && _MSC_VER > 1200 \
    || defined(Z7_GCC_VERSION) && (Z7_GCC_VERSION >= 30300) \
    || defined(__clang__) \
    || defined(__INTEL_COMPILER)
    __m128i _pad_align_128bit[4];
#endif // sse2
#if defined(__AVX2__) \
    || defined(Z7_GCC_VERSION) && (Z7_GCC_VERSION >= 40900) \
    || defined(Z7_APPLE_CLANG_VERSION) && (Z7_APPLE_CLANG_VERSION >= 40600) \
    || defined(Z7_LLVM_CLANG_VERSION) && (Z7_LLVM_CLANG_VERSION >= 30100) \
    || defined(Z7_MSC_VER_ORIGINAL) && (Z7_MSC_VER_ORIGINAL >= 1800) \
    || defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 1400)
    __m256i _pad_align_256bit[2];
#endif // avx2
#endif // x86
#endif // 0

    void * _pad_align_ptr[8];
    UInt32 _pad_align_32bit[16];
    struct
    {
      unsigned cycPos;
      unsigned _pad_unused;
#if 1 || defined(Z7_BLAKE2SP_USE_FUNCTIONS)
      Z7_BLAKE2SP_FUNC_COMPRESS func_Compress_Fast;
      Z7_BLAKE2SP_FUNC_COMPRESS func_Compress_Single;
      Z7_BLAKE2SP_FUNC_INIT func_Init;
      Z7_BLAKE2SP_FUNC_INIT func_Final;
#endif
    } header;
  } u;
  // MY_ALIGN(64)
  UInt32 states[Z7_BLAKE2SP_PARALLEL_DEGREE * Z7_BLAKE2SP_NUM_STRUCT_WORDS];
  // MY_ALIGN(64)
  UInt32  buf32[Z7_BLAKE2SP_PARALLEL_DEGREE * Z7_BLAKE2SP_NUM_STRUCT_WORDS * 2];
} CBlake2sp;

BoolInt Blake2sp_SetFunction(CBlake2sp *p, unsigned algo);
void Blake2sp_Init(CBlake2sp *p);
void Blake2sp_InitState(CBlake2sp *p);
void Blake2sp_Update(CBlake2sp *p, const Byte *data, size_t size);
void Blake2sp_Final(CBlake2sp *p, Byte *digest);
void z7_Black2sp_Prepare(void);

EXTERN_C_END

#endif
