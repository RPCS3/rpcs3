/* ZstdDec.c -- Zstd Decoder
2024-01-21 : the code was developed by Igor Pavlov, using Zstandard format
             specification and original zstd decoder code as reference code.
original zstd decoder code: Copyright (c) Facebook, Inc. All rights reserved.
This source code is licensed under BSD 3-Clause License.
*/

#include "Precomp.h"

#include <string.h>
#include <stdlib.h>
// #include <stdio.h>

#include "Alloc.h"
#include "Xxh64.h"
#include "ZstdDec.h"
#include "CpuArch.h"

#if defined(MY_CPU_ARM64)
#include <arm_neon.h>
#endif

/* original-zstd still doesn't support window larger than 2 GiB.
   So we also limit our decoder for 2 GiB window: */
#if defined(MY_CPU_64BIT) && 0 == 1
  #define MAX_WINDOW_SIZE_LOG  41
#else
  #define MAX_WINDOW_SIZE_LOG  31
#endif

typedef
  #if MAX_WINDOW_SIZE_LOG < 32
    UInt32
  #else
    size_t
  #endif
    CZstdDecOffset;

// for debug: simpler and smaller code but slow:
// #define Z7_ZSTD_DEC_USE_HUF_STREAM1_ALWAYS

// #define SHOW_STAT
#ifdef SHOW_STAT
#include <stdio.h>
static unsigned g_Num_Blocks_Compressed = 0;
static unsigned g_Num_Blocks_memcpy = 0;
static unsigned g_Num_Wrap_memmove_Num = 0;
static unsigned g_Num_Wrap_memmove_Bytes = 0;
static unsigned g_NumSeqs_total = 0;
// static unsigned g_NumCopy = 0;
static unsigned g_NumOver = 0;
static unsigned g_NumOver2 = 0;
static unsigned g_Num_Match = 0;
static unsigned g_Num_Lits = 0;
static unsigned g_Num_LitsBig = 0;
static unsigned g_Num_Lit0 = 0;
static unsigned g_Num_Rep0 = 0;
static unsigned g_Num_Rep1 = 0;
static unsigned g_Num_Rep2 = 0;
static unsigned g_Num_Rep3 = 0;
static unsigned g_Num_Threshold_0 = 0;
static unsigned g_Num_Threshold_1 = 0;
static unsigned g_Num_Threshold_0sum = 0;
static unsigned g_Num_Threshold_1sum = 0;
#define STAT_UPDATE(v) v
#else
#define STAT_UPDATE(v)
#endif
#define STAT_INC(v)  STAT_UPDATE(v++;)


typedef struct
{
  const Byte *ptr;
  size_t len;
}
CInBufPair;


#if defined(MY_CPU_ARM_OR_ARM64) || defined(MY_CPU_X86_OR_AMD64)
  #if (defined(__clang__) && (__clang_major__ >= 6)) \
   || (defined(__GNUC__) && (__GNUC__ >= 6))
    // disable for debug:
    #define Z7_ZSTD_DEC_USE_BSR
  #elif defined(_MSC_VER) && (_MSC_VER >= 1300)
    // #if defined(MY_CPU_ARM_OR_ARM64)
    #if (_MSC_VER >= 1600)
      #include <intrin.h>
    #endif
    // disable for debug:
    #define Z7_ZSTD_DEC_USE_BSR
  #endif
#endif

#ifdef Z7_ZSTD_DEC_USE_BSR
  #if defined(__clang__) || defined(__GNUC__)
    #define MY_clz(x)  ((unsigned)__builtin_clz((UInt32)x))
  #else  // #if defined(_MSC_VER)
    #ifdef MY_CPU_ARM_OR_ARM64
      #define MY_clz  _CountLeadingZeros
    #endif // MY_CPU_X86_OR_AMD64
  #endif // _MSC_VER
#elif !defined(Z7_ZSTD_DEC_USE_LOG_TABLE)
  #define Z7_ZSTD_DEC_USE_LOG_TABLE
#endif


static
Z7_FORCE_INLINE
unsigned GetHighestSetBit_32_nonzero_big(UInt32 num)
{
  // (num != 0)
  #ifdef MY_clz
    return 31 - MY_clz(num);
  #elif defined(Z7_ZSTD_DEC_USE_BSR)
  {
    unsigned long zz;
    _BitScanReverse(&zz, num);
    return zz;
  }
  #else
  {
    int i = -1;
    for (;;)
    {
      i++;
      num >>= 1;
      if (num == 0)
        return (unsigned)i;
    }
  }
  #endif
}

#ifdef Z7_ZSTD_DEC_USE_LOG_TABLE

#define R1(a)  a, a
#define R2(a)  R1(a), R1(a)
#define R3(a)  R2(a), R2(a)
#define R4(a)  R3(a), R3(a)
#define R5(a)  R4(a), R4(a)
#define R6(a)  R5(a), R5(a)
#define R7(a)  R6(a), R6(a)
#define R8(a)  R7(a), R7(a)
#define R9(a)  R8(a), R8(a)

#define Z7_ZSTD_FSE_MAX_ACCURACY  9
// states[] values in FSE_Generate() can use (Z7_ZSTD_FSE_MAX_ACCURACY + 1) bits.
static const Byte k_zstd_LogTable[2 << Z7_ZSTD_FSE_MAX_ACCURACY] =
{
  R1(0), R1(1), R2(2), R3(3), R4(4), R5(5), R6(6), R7(7), R8(8), R9(9)
};

#define GetHighestSetBit_32_nonzero_small(num)  (k_zstd_LogTable[num])
#else
#define GetHighestSetBit_32_nonzero_small  GetHighestSetBit_32_nonzero_big
#endif


#ifdef MY_clz
  #define UPDATE_BIT_OFFSET_FOR_PADDING(b, bitOffset) \
    bitOffset -= (CBitCtr)(MY_clz(b) - 23);
#elif defined(Z7_ZSTD_DEC_USE_BSR)
  #define UPDATE_BIT_OFFSET_FOR_PADDING(b, bitOffset) \
    { unsigned long zz;  _BitScanReverse(&zz, b);  bitOffset -= 8;  bitOffset += zz; }
#else
  #define UPDATE_BIT_OFFSET_FOR_PADDING(b, bitOffset) \
    for (;;) { bitOffset--;  if (b & 0x80) { break; }  b <<= 1; }
#endif

#define SET_bitOffset_TO_PAD(bitOffset, src, srcLen) \
{ \
  unsigned lastByte = (src)[(size_t)(srcLen) - 1]; \
  if (lastByte == 0) return SZ_ERROR_DATA; \
  bitOffset = (CBitCtr)((srcLen) * 8); \
  UPDATE_BIT_OFFSET_FOR_PADDING(lastByte, bitOffset) \
}

#ifndef Z7_ZSTD_DEC_USE_HUF_STREAM1_ALWAYS

#define SET_bitOffset_TO_PAD_and_SET_BIT_SIZE(bitOffset, src, srcLen_res) \
{ \
  unsigned lastByte = (src)[(size_t)(srcLen_res) - 1]; \
  if (lastByte == 0) return SZ_ERROR_DATA; \
  srcLen_res *= 8; \
  bitOffset = (CBitCtr)srcLen_res; \
  UPDATE_BIT_OFFSET_FOR_PADDING(lastByte, bitOffset) \
}

#endif

/*
typedef Int32 CBitCtr_signed;
typedef Int32 CBitCtr;
*/
// /*
typedef ptrdiff_t CBitCtr_signed;
typedef ptrdiff_t CBitCtr;
// */


#define MATCH_LEN_MIN  3
#define kBlockSizeMax  (1u << 17)

// #define Z7_ZSTD_DEC_PRINT_TABLE

#ifdef Z7_ZSTD_DEC_PRINT_TABLE
#define NUM_OFFSET_SYMBOLS_PREDEF 29
#endif
#define NUM_OFFSET_SYMBOLS_MAX    (MAX_WINDOW_SIZE_LOG + 1)  // 32
#define NUM_LL_SYMBOLS            36
#define NUM_ML_SYMBOLS            53
#define FSE_NUM_SYMBOLS_MAX       53  // NUM_ML_SYMBOLS

// /*
#if !defined(MY_CPU_X86) || defined(__PIC__) || defined(MY_CPU_64BIT)
#define Z7_ZSTD_DEC_USE_BASES_IN_OBJECT
#endif
// */
// for debug:
// #define Z7_ZSTD_DEC_USE_BASES_LOCAL
// #define Z7_ZSTD_DEC_USE_BASES_IN_OBJECT

#define GLOBAL_TABLE(n)  k_ ## n

#if defined(Z7_ZSTD_DEC_USE_BASES_LOCAL)
  #define BASES_TABLE(n)  a_ ## n
#elif defined(Z7_ZSTD_DEC_USE_BASES_IN_OBJECT)
  #define BASES_TABLE(n)  p->m_ ## n
#else
  #define BASES_TABLE(n)  GLOBAL_TABLE(n)
#endif

#define Z7_ZSTD_DEC_USE_ML_PLUS3

#if defined(Z7_ZSTD_DEC_USE_BASES_LOCAL) || \
    defined(Z7_ZSTD_DEC_USE_BASES_IN_OBJECT)
  
#define SEQ_EXTRA_TABLES(n) \
  Byte   n ## SEQ_LL_EXTRA [NUM_LL_SYMBOLS]; \
  Byte   n ## SEQ_ML_EXTRA [NUM_ML_SYMBOLS]; \
  UInt32 n ## SEQ_LL_BASES [NUM_LL_SYMBOLS]; \
  UInt32 n ## SEQ_ML_BASES [NUM_ML_SYMBOLS]; \

#define Z7_ZSTD_DEC_USE_BASES_CALC

#ifdef Z7_ZSTD_DEC_USE_BASES_CALC

  #define FILL_LOC_BASES(n, startSum) \
    { unsigned i; UInt32 sum = startSum; \
      for (i = 0; i != Z7_ARRAY_SIZE(GLOBAL_TABLE(n ## _EXTRA)); i++) \
      { const unsigned a = GLOBAL_TABLE(n ## _EXTRA)[i]; \
        BASES_TABLE(n ## _BASES)[i] = sum; \
        /* if (sum != GLOBAL_TABLE(n ## _BASES)[i]) exit(1); */ \
        sum += (UInt32)1 << a; \
        BASES_TABLE(n ## _EXTRA)[i] = (Byte)a; }}

  #define FILL_LOC_BASES_ALL \
      FILL_LOC_BASES (SEQ_LL, 0) \
      FILL_LOC_BASES (SEQ_ML, MATCH_LEN_MIN) \

#else
  #define COPY_GLOBAL_ARR(n)  \
    memcpy(BASES_TABLE(n), GLOBAL_TABLE(n), sizeof(GLOBAL_TABLE(n)));
  #define FILL_LOC_BASES_ALL \
    COPY_GLOBAL_ARR (SEQ_LL_EXTRA) \
    COPY_GLOBAL_ARR (SEQ_ML_EXTRA) \
    COPY_GLOBAL_ARR (SEQ_LL_BASES) \
    COPY_GLOBAL_ARR (SEQ_ML_BASES) \

#endif

#endif


    
/// The sequence decoding baseline and number of additional bits to read/add
#if !defined(Z7_ZSTD_DEC_USE_BASES_CALC)
static const UInt32 GLOBAL_TABLE(SEQ_LL_BASES) [NUM_LL_SYMBOLS] =
{
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
  16, 18, 20, 22, 24, 28, 32, 40, 48, 64, 0x80, 0x100, 0x200, 0x400, 0x800, 0x1000,
  0x2000, 0x4000, 0x8000, 0x10000
};
#endif

static const Byte GLOBAL_TABLE(SEQ_LL_EXTRA) [NUM_LL_SYMBOLS] =
{
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  1, 1, 1, 1, 2, 2, 3, 3, 4, 6, 7, 8, 9, 10, 11, 12,
  13, 14, 15, 16
};

#if !defined(Z7_ZSTD_DEC_USE_BASES_CALC)
static const UInt32 GLOBAL_TABLE(SEQ_ML_BASES) [NUM_ML_SYMBOLS] =
{
  3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18,
  19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34,
  35, 37, 39, 41, 43, 47, 51, 59, 67, 83, 99, 0x83, 0x103, 0x203, 0x403, 0x803,
  0x1003, 0x2003, 0x4003, 0x8003, 0x10003
};
#endif

static const Byte GLOBAL_TABLE(SEQ_ML_EXTRA) [NUM_ML_SYMBOLS] =
{
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  1, 1, 1, 1, 2, 2, 3, 3, 4, 4, 5, 7, 8, 9, 10, 11,
  12, 13, 14, 15, 16
};


#ifdef Z7_ZSTD_DEC_PRINT_TABLE

static const Int16 SEQ_LL_PREDEF_DIST [NUM_LL_SYMBOLS] =
{
  4, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 2, 1, 1, 1, 1, 1,
 -1,-1,-1,-1
};
static const Int16 SEQ_OFFSET_PREDEF_DIST [NUM_OFFSET_SYMBOLS_PREDEF] =
{
  1, 1, 1, 1, 1, 1, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1
};
static const Int16 SEQ_ML_PREDEF_DIST [NUM_ML_SYMBOLS] =
{
  1, 4, 3, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,
 -1,-1,-1,-1,-1
};

#endif

// typedef int FastInt;
// typedef Int32 FastInt32;
typedef unsigned FastInt;
typedef UInt32 FastInt32;
typedef FastInt32 CFseRecord;


#define FSE_REC_LEN_OFFSET    8
#define FSE_REC_STATE_OFFSET  16
#define GET_FSE_REC_SYM(st)   ((Byte)(st))
#define GET_FSE_REC_LEN(st)   ((Byte)((st) >> FSE_REC_LEN_OFFSET))
#define GET_FSE_REC_STATE(st) ((st) >> FSE_REC_STATE_OFFSET)

// #define FSE_REC_SYM_MASK      (0xff)
// #define GET_FSE_REC_SYM(st)   (st & FSE_REC_SYM_MASK)

#define W_BASE(state, len, sym) \
    (((UInt32)state << (4 + FSE_REC_STATE_OFFSET)) + \
    (len << FSE_REC_LEN_OFFSET) + (sym))
#define W(state, len, sym)  W_BASE(state, len, sym)
static const CFseRecord k_PredefRecords_LL[1 << 6] = {
W(0,4, 0),W(1,4, 0),W(2,5, 1),W(0,5, 3),W(0,5, 4),W(0,5, 6),W(0,5, 7),W(0,5, 9),
W(0,5,10),W(0,5,12),W(0,6,14),W(0,5,16),W(0,5,18),W(0,5,19),W(0,5,21),W(0,5,22),
W(0,5,24),W(2,5,25),W(0,5,26),W(0,6,27),W(0,6,29),W(0,6,31),W(2,4, 0),W(0,4, 1),
W(0,5, 2),W(2,5, 4),W(0,5, 5),W(2,5, 7),W(0,5, 8),W(2,5,10),W(0,5,11),W(0,6,13),
W(2,5,16),W(0,5,17),W(2,5,19),W(0,5,20),W(2,5,22),W(0,5,23),W(0,4,25),W(1,4,25),
W(2,5,26),W(0,6,28),W(0,6,30),W(3,4, 0),W(1,4, 1),W(2,5, 2),W(2,5, 3),W(2,5, 5),
W(2,5, 6),W(2,5, 8),W(2,5, 9),W(2,5,11),W(2,5,12),W(0,6,15),W(2,5,17),W(2,5,18),
W(2,5,20),W(2,5,21),W(2,5,23),W(2,5,24),W(0,6,35),W(0,6,34),W(0,6,33),W(0,6,32)
};
static const CFseRecord k_PredefRecords_OF[1 << 5] = {
W(0,5, 0),W(0,4, 6),W(0,5, 9),W(0,5,15),W(0,5,21),W(0,5, 3),W(0,4, 7),W(0,5,12),
W(0,5,18),W(0,5,23),W(0,5, 5),W(0,4, 8),W(0,5,14),W(0,5,20),W(0,5, 2),W(1,4, 7),
W(0,5,11),W(0,5,17),W(0,5,22),W(0,5, 4),W(1,4, 8),W(0,5,13),W(0,5,19),W(0,5, 1),
W(1,4, 6),W(0,5,10),W(0,5,16),W(0,5,28),W(0,5,27),W(0,5,26),W(0,5,25),W(0,5,24)
};
#if defined(Z7_ZSTD_DEC_USE_ML_PLUS3)
#undef W
#define W(state, len, sym)  W_BASE(state, len, (sym + MATCH_LEN_MIN))
#endif
static const CFseRecord k_PredefRecords_ML[1 << 6] = {
W(0,6, 0),W(0,4, 1),W(2,5, 2),W(0,5, 3),W(0,5, 5),W(0,5, 6),W(0,5, 8),W(0,6,10),
W(0,6,13),W(0,6,16),W(0,6,19),W(0,6,22),W(0,6,25),W(0,6,28),W(0,6,31),W(0,6,33),
W(0,6,35),W(0,6,37),W(0,6,39),W(0,6,41),W(0,6,43),W(0,6,45),W(1,4, 1),W(0,4, 2),
W(2,5, 3),W(0,5, 4),W(2,5, 6),W(0,5, 7),W(0,6, 9),W(0,6,12),W(0,6,15),W(0,6,18),
W(0,6,21),W(0,6,24),W(0,6,27),W(0,6,30),W(0,6,32),W(0,6,34),W(0,6,36),W(0,6,38),
W(0,6,40),W(0,6,42),W(0,6,44),W(2,4, 1),W(3,4, 1),W(1,4, 2),W(2,5, 4),W(2,5, 5),
W(2,5, 7),W(2,5, 8),W(0,6,11),W(0,6,14),W(0,6,17),W(0,6,20),W(0,6,23),W(0,6,26),
W(0,6,29),W(0,6,52),W(0,6,51),W(0,6,50),W(0,6,49),W(0,6,48),W(0,6,47),W(0,6,46)
};


// sum of freqs[] must be correct
// (numSyms != 0)
// (accuracy >= 5)
static
Z7_NO_INLINE
// Z7_FORCE_INLINE
void FSE_Generate(CFseRecord *table,
    const Int16 *const freqs, const size_t numSyms,
    const unsigned accuracy, UInt32 delta)
{
  size_t size = (size_t)1 << accuracy;
  // max value in states[x] is ((1 << accuracy) * 2)
  UInt16 states[FSE_NUM_SYMBOLS_MAX];
  {
    /* Symbols with "less than 1" probability get a single cell,
       starting from the end of the table.
       These symbols define a full state reset, reading (accuracy) bits. */
    size_t threshold = size;
    {
      size_t s = 0;
      do
        if (freqs[s] == -1)
        {
          table[--threshold] = (CFseRecord)s;
          states[s] = 1;
        }
      while (++s != numSyms);
    }
 
    #ifdef SHOW_STAT
    if (threshold == size)
    {
      STAT_INC(g_Num_Threshold_0)
      STAT_UPDATE(g_Num_Threshold_0sum += (unsigned)size;)
    }
    else
    {
      STAT_INC(g_Num_Threshold_1)
      STAT_UPDATE(g_Num_Threshold_1sum += (unsigned)size;)
    }
    #endif

    // { unsigned uuu; for (uuu = 0; uuu < 400; uuu++)
    {
      // Each (symbol) gets freqs[symbol] cells.
      // Cell allocation is spread, not linear.
      const size_t step = (size >> 1) + (size >> 3) + 3;
      size_t pos = 0;
      // const unsigned mask = size - 1;
      /*
      if (threshold == size)
      {
        size_t s = 0;
        size--;
        do
        {
          int freq = freqs[s];
          if (freq <= 0)
            continue;
          states[s] = (UInt16)freq;
          do
          {
            table[pos] (CFseRecord)s;
            pos = (pos + step) & size; // & mask;
          }
          while (--freq);
        }
        while (++s != numSyms);
      }
      else
      */
      {
        size_t s = 0;
        size--;
        do
        {
          int freq = freqs[s];
          if (freq <= 0)
            continue;
          states[s] = (UInt16)freq;
          do
          {
            table[pos] = (CFseRecord)s;
            // we skip position, if it's already occupied by a "less than 1" probability symbol.
            // (step) is coprime to table size, so the cycle will visit each position exactly once
            do
              pos = (pos + step) & size; // & mask;
            while (pos >= threshold);
          }
          while (--freq);
        }
        while (++s != numSyms);
      }
      size++;
      // (pos != 0) is unexpected case that means that freqs[] are not correct.
      // so it's some failure in code (for example, incorrect predefined freq[] table)
      // if (pos != 0) return SZ_ERROR_FAIL;
    }
    // }
  }
  {
    const CFseRecord * const limit = table + size;
    delta = ((UInt32)size << FSE_REC_STATE_OFFSET) - delta;
    /* State increases by symbol over time, decreasing number of bits.
       Baseline increases until the bit threshold is passed, at which point it resets to 0 */
    do
    {
      #define TABLE_ITER(a) \
      { \
        const FastInt sym = (FastInt)table[a]; \
        const unsigned nextState = states[sym]; \
        unsigned nb; \
        states[sym] = (UInt16)(nextState + 1); \
        nb = accuracy - GetHighestSetBit_32_nonzero_small(nextState); \
        table[a] = (CFseRecord)(sym - delta \
            + ((UInt32)nb << FSE_REC_LEN_OFFSET) \
            + ((UInt32)nextState << FSE_REC_STATE_OFFSET << nb)); \
      }
      TABLE_ITER(0)
      TABLE_ITER(1)
      table += 2;
    }
    while (table != limit);
  }
}


#ifdef Z7_ZSTD_DEC_PRINT_TABLE

static void Print_Predef(unsigned predefAccuracy,
    const unsigned numSymsPredef,
    const Int16 * const predefFreqs,
    const CFseRecord *checkTable)
{
  CFseRecord table[1 << 6];
  unsigned i;
  FSE_Generate(table, predefFreqs, numSymsPredef, predefAccuracy,
        #if defined(Z7_ZSTD_DEC_USE_ML_PLUS3)
          numSymsPredef == NUM_ML_SYMBOLS ? MATCH_LEN_MIN :
        #endif
          0
    );
  if (memcmp(table, checkTable, sizeof(UInt32) << predefAccuracy) != 0)
    exit(1);
  for (i = 0; i < (1u << predefAccuracy); i++)
  {
    const UInt32 v = table[i];
    const unsigned state = (unsigned)(GET_FSE_REC_STATE(v));
    if (state & 0xf)
      exit(1);
    if (i != 0)
    {
      printf(",");
      if (i % 8 == 0)
        printf("\n");
    }
    printf("W(%d,%d,%2d)",
        (unsigned)(state >> 4),
        (unsigned)((v >> FSE_REC_LEN_OFFSET) & 0xff),
        (unsigned)GET_FSE_REC_SYM(v));
  }
  printf("\n\n");
}

#endif


#define GET16(dest, p)  { const Byte *ptr = p;  dest = GetUi16(ptr); }
#define GET32(dest, p)  { const Byte *ptr = p;  dest = GetUi32(ptr); }

// (1 <= numBits <= 9)
#define FORWARD_READ_BITS(destVal, numBits, mask) \
  { const CBitCtr_signed bos3 = (bitOffset) >> 3; \
    if (bos3 >= 0) return SZ_ERROR_DATA; \
    GET16(destVal, src + bos3) \
    destVal >>= (bitOffset) & 7; \
    bitOffset += (CBitCtr_signed)(numBits); \
    mask = (1u << (numBits)) - 1; \
    destVal &= mask; \
  }

#define FORWARD_READ_1BIT(destVal) \
  { const CBitCtr_signed bos3 = (bitOffset) >> 3; \
    if (bos3 >= 0) return SZ_ERROR_DATA; \
    destVal = *(src + bos3); \
    destVal >>= (bitOffset) & 7; \
    (bitOffset)++; \
    destVal &= 1; \
  }


// in: (accuracyMax <= 9)
// at least 2 bytes will be processed from (in) stream.
// at return: (in->len > 0)
static
Z7_NO_INLINE
SRes FSE_DecodeHeader(CFseRecord *const table,
    CInBufPair *const in,
    const unsigned accuracyMax,
    Byte *const accuracyRes,
    unsigned numSymbolsMax)
{
  unsigned accuracy;
  unsigned remain1;
  unsigned syms;
  Int16 freqs[FSE_NUM_SYMBOLS_MAX + 3]; // +3 for overwrite (repeat)
  const Byte *src = in->ptr;
  CBitCtr_signed bitOffset = (CBitCtr_signed)in->len - 1;
  if (bitOffset <= 0)
    return SZ_ERROR_DATA;
  accuracy = *src & 0xf;
  accuracy += 5;
  if (accuracy > accuracyMax)
    return SZ_ERROR_DATA;
  *accuracyRes = (Byte)accuracy;
  remain1 = (1u << accuracy) + 1; // (it's remain_freqs_sum + 1)
  syms = 0;
  src += bitOffset;  // src points to last byte
  bitOffset = 4 - (bitOffset << 3);
  
  for (;;)
  {
    // (2 <= remain1)
    const unsigned bits = GetHighestSetBit_32_nonzero_small((unsigned)remain1);
    // (1 <= bits <= accuracy)
    unsigned val; // it must be unsigned or int
    unsigned mask;
    FORWARD_READ_BITS(val, bits, mask)
    {
      const unsigned val2 = remain1 + val - mask;
      if (val2 > mask)
      {
        unsigned bit;
        FORWARD_READ_1BIT(bit)
        if (bit)
          val = val2;
      }
    }
    {
      // (remain1 >= 2)
      // (0 <= (int)val <= remain1)
      val = (unsigned)((int)val - 1);
      // val now is "probability" of symbol
      // (probability == -1) means "less than 1" frequency.
      // (-1 <= (int)val <= remain1 - 1)
      freqs[syms++] = (Int16)(int)val;
      if (val != 0)
      {
        remain1 -= (int)val < 0 ? 1u : (unsigned)val;
        // remain1 -= val;
        // val >>= (sizeof(val) * 8 - 2);
        // remain1 -= val & 2;
        // freqs[syms++] = (Int16)(int)val;
        // syms++;
        if (remain1 == 1)
          break;
        if (syms >= FSE_NUM_SYMBOLS_MAX)
          return SZ_ERROR_DATA;
      }
      else // if (val == 0)
      {
        // freqs[syms++] = 0;
        // syms++;
        for (;;)
        {
          unsigned repeat;
          FORWARD_READ_BITS(repeat, 2, mask)
          freqs[syms    ] = 0;
          freqs[syms + 1] = 0;
          freqs[syms + 2] = 0;
          syms += repeat;
          if (syms >= FSE_NUM_SYMBOLS_MAX)
            return SZ_ERROR_DATA;
          if (repeat != 3)
            break;
        }
      }
    }
  }

  if (syms > numSymbolsMax)
    return SZ_ERROR_DATA;
  bitOffset += 7;
  bitOffset >>= 3;
  if (bitOffset > 0)
    return SZ_ERROR_DATA;
  in->ptr = src + bitOffset;
  in->len = (size_t)(1 - bitOffset);
  {
    // unsigned uuu; for (uuu = 0; uuu < 50; uuu++)
    FSE_Generate(table, freqs, syms, accuracy,
        #if defined(Z7_ZSTD_DEC_USE_ML_PLUS3)
          numSymbolsMax == NUM_ML_SYMBOLS ? MATCH_LEN_MIN :
        #endif
          0
        );
  }
  return SZ_OK;
}


// ---------- HUFFMAN ----------

#define HUF_MAX_BITS    12
#define HUF_MAX_SYMBS   256
#define HUF_DUMMY_SIZE  (128 + 8 * 2)  // it must multiple of 8
// #define HUF_DUMMY_SIZE 0
#define HUF_TABLE_SIZE  ((2 << HUF_MAX_BITS) + HUF_DUMMY_SIZE)
#define HUF_GET_SYMBOLS(table)  ((table) + (1 << HUF_MAX_BITS) + HUF_DUMMY_SIZE)
// #define HUF_GET_LENS(table)  (table)

typedef struct
{
  // Byte table[HUF_TABLE_SIZE];
  UInt64 table64[HUF_TABLE_SIZE / sizeof(UInt64)];
}
CZstdDecHufTable;

/*
Input:
  numSyms != 0
  (bits) array size must be aligned for 2
  if (numSyms & 1), then bits[numSyms] == 0,
  Huffman tree must be correct before Huf_Build() call:
    (sum (1/2^bits[i]) == 1).
    && (bits[i] <= HUF_MAX_BITS)
*/
static
Z7_FORCE_INLINE
void Huf_Build(Byte * const table,
    const Byte *bits, const unsigned numSyms)
{
  unsigned counts0[HUF_MAX_BITS + 1];
  unsigned counts1[HUF_MAX_BITS + 1];
  const Byte * const bitsEnd = bits + numSyms;
  // /*
  {
    unsigned t;
    for (t = 0; t < Z7_ARRAY_SIZE(counts0); t++) counts0[t] = 0;
    for (t = 0; t < Z7_ARRAY_SIZE(counts1); t++) counts1[t] = 0;
  }
  // */
  // memset(counts0, 0, sizeof(counts0));
  // memset(counts1, 0, sizeof(counts1));
  {
    const Byte *bits2 = bits;
    // we access additional bits[symbol] if (numSyms & 1)
    do
    {
      counts0[bits2[0]]++;
      counts1[bits2[1]]++;
    }
    while ((bits2 += 2) < bitsEnd);
  }
  {
    unsigned r = 0;
    unsigned i = HUF_MAX_BITS;
    // Byte *lens = HUF_GET_LENS(symbols);
    do
    {
      const unsigned num = (counts0[i] + counts1[i]) << (HUF_MAX_BITS - i);
      counts0[i] = r;
      if (num)
      {
        Byte *lens = &table[r];
        r += num;
        memset(lens, (int)i, num);
      }
    }
    while (--i);
    counts0[0] = 0; // for speculated loads
    // no need for check:
    // if (r != (UInt32)1 << HUF_MAX_BITS) exit(0);
  }
  {
    #ifdef MY_CPU_64BIT
      UInt64
    #else
      UInt32
    #endif
        v = 0;
    Byte *symbols = HUF_GET_SYMBOLS(table);
    do
    {
      const unsigned nb = *bits++;
      if (nb)
      {
        const unsigned code = counts0[nb];
        const unsigned num = (1u << HUF_MAX_BITS) >> nb;
        counts0[nb] = code + num;
        // memset(&symbols[code], i, num);
        // /*
        {
          Byte *s2 = &symbols[code];
          if (num <= 2)
          {
            s2[0] = (Byte)v;
            s2[(size_t)num - 1] = (Byte)v;
          }
          else if (num <= 8)
          {
            *(UInt32 *)(void *)s2 = (UInt32)v;
            *(UInt32 *)(void *)(s2 + (size_t)num - 4) = (UInt32)v;
          }
          else
          {
            #ifdef MY_CPU_64BIT
              UInt64 *s = (UInt64 *)(void *)s2;
              const UInt64 *lim = (UInt64 *)(void *)(s2 + num);
              do
              {
                s[0] = v;  s[1] = v;  s += 2;
              }
              while (s != lim);
            #else
              UInt32 *s = (UInt32 *)(void *)s2;
              const UInt32 *lim = (const UInt32 *)(const void *)(s2 + num);
              do
              {
                s[0] = v;  s[1] = v;  s += 2;
                s[0] = v;  s[1] = v;  s += 2;
              }
              while (s != lim);
            #endif
          }
        }
        // */
      }
      v +=
        #ifdef MY_CPU_64BIT
          0x0101010101010101;
        #else
          0x01010101;
        #endif
    }
    while (bits != bitsEnd);
  }
}



// how many bytes (src) was moved back from original value.
// we need (HUF_SRC_OFFSET == 3) for optimized 32-bit memory access
#define HUF_SRC_OFFSET  3

// v <<= 8 - (bitOffset & 7) + numBits;
// v >>= 32 - HUF_MAX_BITS;
#define HUF_GET_STATE(v, bitOffset, numBits) \
  GET32(v, src + (HUF_SRC_OFFSET - 3) + ((CBitCtr_signed)bitOffset >> 3)) \
  v >>= 32 - HUF_MAX_BITS - 8 + ((unsigned)bitOffset & 7) - numBits; \
  v &= (1u << HUF_MAX_BITS) - 1; \


#ifndef Z7_ZSTD_DEC_USE_HUF_STREAM1_ALWAYS
#if defined(MY_CPU_AMD64) && defined(_MSC_VER) && _MSC_VER == 1400 \
  || !defined(MY_CPU_X86_OR_AMD64) \
  // || 1 == 1 /* for debug : to force STREAM4_PRELOAD mode */
  // we need big number (>=16) of registers for PRELOAD4
  #define Z7_ZSTD_DEC_USE_HUF_STREAM4_PRELOAD4
  // #define Z7_ZSTD_DEC_USE_HUF_STREAM4_PRELOAD2 // for debug
#endif
#endif

// for debug: simpler and smaller code but slow:
// #define Z7_ZSTD_DEC_USE_HUF_STREAM1_SIMPLE

#if  defined(Z7_ZSTD_DEC_USE_HUF_STREAM1_SIMPLE) || \
    !defined(Z7_ZSTD_DEC_USE_HUF_STREAM1_ALWAYS)
     
#define HUF_DECODE(bitOffset, dest) \
{ \
  UInt32 v; \
  HUF_GET_STATE(v, bitOffset, 0) \
  bitOffset -= table[v]; \
  *(dest) = symbols[v]; \
  if ((CBitCtr_signed)bitOffset < 0) return SZ_ERROR_DATA; \
}

#endif

#if !defined(Z7_ZSTD_DEC_USE_HUF_STREAM1_SIMPLE) || \
     defined(Z7_ZSTD_DEC_USE_HUF_STREAM4_PRELOAD4) || \
     defined(Z7_ZSTD_DEC_USE_HUF_STREAM4_PRELOAD2) \

#define HUF_DECODE_2_INIT(v, bitOffset) \
  HUF_GET_STATE(v, bitOffset, 0)

#define HUF_DECODE_2(v, bitOffset, dest) \
{ \
  unsigned numBits; \
  numBits = table[v]; \
  *(dest) = symbols[v]; \
  HUF_GET_STATE(v, bitOffset, numBits) \
  bitOffset -= (CBitCtr)numBits; \
  if ((CBitCtr_signed)bitOffset < 0) return SZ_ERROR_DATA; \
}

#endif


// src == ptr - HUF_SRC_OFFSET
// we are allowed to access 3 bytes before start of input buffer
static
Z7_NO_INLINE
SRes Huf_Decompress_1stream(const Byte * const table,
    const Byte *src, const size_t srcLen,
    Byte *dest, const size_t destLen)
{
  CBitCtr bitOffset;
  if (srcLen == 0)
    return SZ_ERROR_DATA;
  SET_bitOffset_TO_PAD (bitOffset, src + HUF_SRC_OFFSET, srcLen)
  if (destLen)
  {
    const Byte *symbols = HUF_GET_SYMBOLS(table);
    const Byte *destLim = dest + destLen;
    #ifdef Z7_ZSTD_DEC_USE_HUF_STREAM1_SIMPLE
    {
      do
      {
        HUF_DECODE (bitOffset, dest)
      }
      while (++dest != destLim);
    }
    #else
    {
      UInt32 v;
      HUF_DECODE_2_INIT (v, bitOffset)
      do
      {
        HUF_DECODE_2 (v, bitOffset, dest)
      }
      while (++dest != destLim);
    }
    #endif
  }
  return bitOffset == 0 ? SZ_OK : SZ_ERROR_DATA;
}


// for debug : it reduces register pressure : by array copy can be slow :
// #define Z7_ZSTD_DEC_USE_HUF_LOCAL

// src == ptr + (6 - HUF_SRC_OFFSET)
// srcLen >= 10
// we are allowed to access 3 bytes before start of input buffer
static
Z7_NO_INLINE
SRes Huf_Decompress_4stream(const Byte * const
  #ifdef Z7_ZSTD_DEC_USE_HUF_LOCAL
    table2,
  #else
    table,
  #endif
    const Byte *src, size_t srcLen,
    Byte *dest, size_t destLen)
{
 #ifdef Z7_ZSTD_DEC_USE_HUF_LOCAL
  Byte table[HUF_TABLE_SIZE];
 #endif
  UInt32 sizes[3];
  const size_t delta = (destLen + 3) / 4;
  if ((sizes[0] = GetUi16(src + (0 + HUF_SRC_OFFSET - 6))) == 0) return SZ_ERROR_DATA;
  if ((sizes[1] = GetUi16(src + (2 + HUF_SRC_OFFSET - 6))) == 0) return SZ_ERROR_DATA;
  sizes[1] += sizes[0];
  if ((sizes[2] = GetUi16(src + (4 + HUF_SRC_OFFSET - 6))) == 0) return SZ_ERROR_DATA;
  sizes[2] += sizes[1];
  srcLen -= 6;
  if (srcLen <= sizes[2])
    return SZ_ERROR_DATA;

 #ifdef Z7_ZSTD_DEC_USE_HUF_LOCAL
  {
    // unsigned i = 0; for(; i < 1000; i++)
    memcpy(table, table2, HUF_TABLE_SIZE);
  }
 #endif

  #ifndef Z7_ZSTD_DEC_USE_HUF_STREAM1_ALWAYS
  {
    CBitCtr bitOffset_0,
            bitOffset_1,
            bitOffset_2,
            bitOffset_3;
    {
      SET_bitOffset_TO_PAD_and_SET_BIT_SIZE (bitOffset_0, src + HUF_SRC_OFFSET, sizes[0])
      SET_bitOffset_TO_PAD_and_SET_BIT_SIZE (bitOffset_1, src + HUF_SRC_OFFSET, sizes[1])
      SET_bitOffset_TO_PAD_and_SET_BIT_SIZE (bitOffset_2, src + HUF_SRC_OFFSET, sizes[2])
      SET_bitOffset_TO_PAD                  (bitOffset_3, src + HUF_SRC_OFFSET, srcLen)
    }
    {
      const Byte * const symbols = HUF_GET_SYMBOLS(table);
      Byte *destLim = dest + destLen - delta * 3;

      if (dest != destLim)
    #ifdef Z7_ZSTD_DEC_USE_HUF_STREAM4_PRELOAD4
      {
        UInt32 v_0, v_1, v_2, v_3;
        HUF_DECODE_2_INIT (v_0, bitOffset_0)
        HUF_DECODE_2_INIT (v_1, bitOffset_1)
        HUF_DECODE_2_INIT (v_2, bitOffset_2)
        HUF_DECODE_2_INIT (v_3, bitOffset_3)
        // #define HUF_DELTA (1 << 17) / 4
        do
        {
          HUF_DECODE_2 (v_3, bitOffset_3, dest + delta * 3)
          HUF_DECODE_2 (v_2, bitOffset_2, dest + delta * 2)
          HUF_DECODE_2 (v_1, bitOffset_1, dest + delta)
          HUF_DECODE_2 (v_0, bitOffset_0, dest)
        }
        while (++dest != destLim);
        /*
        {// unsigned y = 0; for (;y < 1; y++)
        {
          const size_t num = destLen - delta * 3;
          Byte *orig = dest - num;
          memmove (orig + delta    , orig + HUF_DELTA,     num);
          memmove (orig + delta * 2, orig + HUF_DELTA * 2, num);
          memmove (orig + delta * 3, orig + HUF_DELTA * 3, num);
        }}
        */
      }
    #elif defined(Z7_ZSTD_DEC_USE_HUF_STREAM4_PRELOAD2)
      {
        UInt32 v_0, v_1, v_2, v_3;
        HUF_DECODE_2_INIT (v_0, bitOffset_0)
        HUF_DECODE_2_INIT (v_1, bitOffset_1)
        do
        {
          HUF_DECODE_2 (v_0, bitOffset_0, dest)
          HUF_DECODE_2 (v_1, bitOffset_1, dest + delta)
        }
        while (++dest != destLim);
        dest = destLim - (destLen - delta * 3);
        dest += delta * 2;
        destLim += delta * 2;
        HUF_DECODE_2_INIT (v_2, bitOffset_2)
        HUF_DECODE_2_INIT (v_3, bitOffset_3)
        do
        {
          HUF_DECODE_2 (v_2, bitOffset_2, dest)
          HUF_DECODE_2 (v_3, bitOffset_3, dest + delta)
        }
        while (++dest != destLim);
        dest -= delta * 2;
        destLim -= delta * 2;
      }
    #else
      {
        do
        {
          HUF_DECODE (bitOffset_3, dest + delta * 3)
          HUF_DECODE (bitOffset_2, dest + delta * 2)
          HUF_DECODE (bitOffset_1, dest + delta)
          HUF_DECODE (bitOffset_0, dest)
        }
        while (++dest != destLim);
      }
    #endif
    
      if (bitOffset_3 != (CBitCtr)sizes[2])
        return SZ_ERROR_DATA;
      if (destLen &= 3)
      {
        destLim = dest + 4 - destLen;
        do
        {
          HUF_DECODE (bitOffset_2, dest + delta * 2)
          HUF_DECODE (bitOffset_1, dest + delta)
          HUF_DECODE (bitOffset_0, dest)
        }
        while (++dest != destLim);
      }
      if (   bitOffset_0 != 0
          || bitOffset_1 != (CBitCtr)sizes[0]
          || bitOffset_2 != (CBitCtr)sizes[1])
        return SZ_ERROR_DATA;
    }
  }
  #else // Z7_ZSTD_DEC_USE_HUF_STREAM1_ALWAYS
  {
    unsigned i;
    for (i = 0; i < 4; i++)
    {
      size_t d = destLen;
      size_t size = srcLen;
      if (i != 3)
      {
        d = delta;
        size = sizes[i];
      }
      if (i != 0)
        size -= sizes[i - 1];
      destLen -= d;
      RINOK(Huf_Decompress_1stream(table, src, size, dest, d))
      dest += d;
      src += size;
    }
  }
  #endif

  return SZ_OK;
}



// (in->len != 0)
// we are allowed to access in->ptr[-3]
// at least 2 bytes in (in->ptr) will be processed
static SRes Huf_DecodeTable(CZstdDecHufTable *const p, CInBufPair *const in)
{
  Byte weights[HUF_MAX_SYMBS + 1];  // +1 for extra write for loop unroll
  unsigned numSyms;
  const unsigned header = *(in->ptr)++;
  in->len--;
  // memset(weights, 0, sizeof(weights));
  if (header >= 128)
  {
    // direct representation: 4 bits field (0-15) per weight
    numSyms = header - 127;
    // numSyms != 0
    {
      const size_t numBytes = (numSyms + 1) / 2;
      const Byte *const ws = in->ptr;
      size_t i = 0;
      if (in->len < numBytes)
        return SZ_ERROR_DATA;
      in->ptr += numBytes;
      in->len -= numBytes;
      do
      {
        const unsigned b = ws[i];
        weights[i * 2    ] = (Byte)(b >> 4);
        weights[i * 2 + 1] = (Byte)(b & 0xf);
      }
      while (++i != numBytes);
      /* 7ZIP: we can restore correct zero value for weights[numSyms],
         if we want to use zero values starting from numSyms in code below. */
      // weights[numSyms] = 0;
    }
  }
  else
  {
    #define MAX_ACCURACY_LOG_FOR_WEIGHTS 6
    CFseRecord table[1 << MAX_ACCURACY_LOG_FOR_WEIGHTS];

    Byte accuracy;
    const Byte *src;
    size_t srcLen;
    if (in->len < header)
      return SZ_ERROR_DATA;
    {
      CInBufPair fse_stream;
      fse_stream.len = header;
      fse_stream.ptr = in->ptr;
      in->ptr += header;
      in->len -= header;
      RINOK(FSE_DecodeHeader(table, &fse_stream,
          MAX_ACCURACY_LOG_FOR_WEIGHTS,
          &accuracy,
          16 // num weight symbols max (max-symbol is 15)
          ))
      // at least 2 bytes were processed in fse_stream.
      // (srcLen > 0) after FSE_DecodeHeader()
      // if (srcLen == 0) return SZ_ERROR_DATA;
      src = fse_stream.ptr;
      srcLen = fse_stream.len;
    }
    // we are allowed to access src[-5]
    {
      // unsigned yyy = 200; do {
      CBitCtr bitOffset;
      FastInt32 state1, state2;
      SET_bitOffset_TO_PAD (bitOffset, src, srcLen)
      state1 = accuracy;
      src -= state1 >> 2;  // src -= 1; // for GET16() optimization
      state1 <<= FSE_REC_LEN_OFFSET;
      state2 = state1;
      numSyms = 0;
      for (;;)
      {
        #define FSE_WEIGHT_DECODE(st) \
        { \
          const unsigned bits = GET_FSE_REC_LEN(st); \
          FastInt r; \
          GET16(r, src + (bitOffset >> 3)) \
          r >>= (unsigned)bitOffset & 7; \
          if ((CBitCtr_signed)(bitOffset -= (CBitCtr)bits) < 0) \
            { if (bitOffset + (CBitCtr)bits != 0) \
                return SZ_ERROR_DATA; \
              break; } \
          r &= 0xff; \
          r >>= 8 - bits; \
          st = table[GET_FSE_REC_STATE(st) + r]; \
          weights[numSyms++] = (Byte)GET_FSE_REC_SYM(st); \
        }
        FSE_WEIGHT_DECODE (state1)
        FSE_WEIGHT_DECODE (state2)
        if (numSyms == HUF_MAX_SYMBS)
          return SZ_ERROR_DATA;
      }
      // src += (unsigned)accuracy >> 2; } while (--yyy);
    }
  }
  
  // Build using weights:
  {
    UInt32 sum = 0;
    {
      // numSyms >= 1
      unsigned i = 0;
      weights[numSyms] = 0;
      do
      {
        sum += ((UInt32)1 << weights[i    ]) & ~(UInt32)1;
        sum += ((UInt32)1 << weights[i + 1]) & ~(UInt32)1;
        i += 2;
      }
      while (i < numSyms);
      if (sum == 0)
        return SZ_ERROR_DATA;
    }
    {
      const unsigned maxBits = GetHighestSetBit_32_nonzero_big(sum) + 1;
      {
        const UInt32 left = ((UInt32)1 << maxBits) - sum;
        // (left != 0)
        // (left) must be power of 2 in correct stream
        if (left & (left - 1))
          return SZ_ERROR_DATA;
        weights[numSyms++] = (Byte)GetHighestSetBit_32_nonzero_big(left);
      }
      // if (numSyms & 1)
        weights[numSyms] = 0; // for loop unroll
      // numSyms >= 2
      {
        unsigned i = 0;
        do
        {
          /*
          #define WEIGHT_ITER(a) \
            { unsigned w = weights[i + (a)]; \
              const unsigned t = maxBits - w; \
              w = w ? t: w; \
              if (w > HUF_MAX_BITS) return SZ_ERROR_DATA; \
              weights[i + (a)] = (Byte)w; }
          */
          // /*
          #define WEIGHT_ITER(a) \
            { unsigned w = weights[i + (a)]; \
              if (w) {  \
                w = maxBits - w; \
                if (w > HUF_MAX_BITS) return SZ_ERROR_DATA; \
                weights[i + (a)] = (Byte)w; }}
          // */
          WEIGHT_ITER(0)
          // WEIGHT_ITER(1)
          // i += 2;
        }
        while (++i != numSyms);
      }
    }
  }
  {
    // unsigned yyy; for (yyy = 0; yyy < 100; yyy++)
    Huf_Build((Byte *)(void *)p->table64, weights, numSyms);
  }
  return SZ_OK;
}


typedef enum
{
  k_SeqMode_Predef = 0,
  k_SeqMode_RLE    = 1,
  k_SeqMode_FSE    = 2,
  k_SeqMode_Repeat = 3
}
z7_zstd_enum_SeqMode;

// predefAccuracy == 5 for OFFSET symbols
// predefAccuracy == 6 for MATCH/LIT LEN symbols
static
SRes
Z7_NO_INLINE
// Z7_FORCE_INLINE
FSE_Decode_SeqTable(CFseRecord * const table,
    CInBufPair * const in,
    unsigned predefAccuracy,
    Byte * const accuracyRes,
    unsigned numSymbolsMax,
    const CFseRecord * const predefs,
    const unsigned seqMode)
{
  // UNUSED_VAR(numSymsPredef)
  // UNUSED_VAR(predefFreqs)
  if (seqMode == k_SeqMode_FSE)
  {
    // unsigned y = 50; CInBufPair in2 = *in; do { *in = in2; RINOK(
    return
    FSE_DecodeHeader(table, in,
        predefAccuracy + 3, // accuracyMax
        accuracyRes,
        numSymbolsMax)
    ;
    // )} while (--y); return SZ_OK;
  }
  // numSymsMax = numSymsPredef + ((predefAccuracy & 1) * (32 - 29))); // numSymsMax
  // numSymsMax == 32 for offsets

  if (seqMode == k_SeqMode_Predef)
  {
    *accuracyRes = (Byte)predefAccuracy;
    memcpy(table, predefs, sizeof(UInt32) << predefAccuracy);
    return SZ_OK;
  }

  // (seqMode == k_SeqMode_RLE)
  if (in->len == 0)
    return SZ_ERROR_DATA;
  in->len--;
  {
    const Byte *ptr = in->ptr;
    const Byte sym = ptr[0];
    in->ptr = ptr + 1;
    table[0] = (FastInt32)sym
      #if defined(Z7_ZSTD_DEC_USE_ML_PLUS3)
        + (numSymbolsMax == NUM_ML_SYMBOLS ? MATCH_LEN_MIN : 0)
      #endif
      ;
    *accuracyRes = 0;
  }
  return SZ_OK;
}


typedef struct
{
  CFseRecord of[1 << 8];
  CFseRecord ll[1 << 9];
  CFseRecord ml[1 << 9];
}
CZstdDecFseTables;


typedef struct
{
  Byte *win;
  SizeT cycSize;
  /*
    if (outBuf_fromCaller)  : cycSize = outBufSize_fromCaller
    else {
      if ( isCyclicMode) : cycSize = cyclic_buffer_size = (winSize + extra_space)
      if (!isCyclicMode) : cycSize = ContentSize,
      (isCyclicMode == true) if (ContetSize >= winSize) or ContetSize is unknown
    }
  */
  SizeT winPos;

  CZstdDecOffset reps[3];

  Byte ll_accuracy;
  Byte of_accuracy;
  Byte ml_accuracy;
  // Byte seqTables_wereSet;
  Byte litHuf_wasSet;
  
  Byte *literalsBase;

  size_t winSize;        // from header
  size_t totalOutCheck;  // totalOutCheck <= winSize

  #ifdef Z7_ZSTD_DEC_USE_BASES_IN_OBJECT
  SEQ_EXTRA_TABLES(m_)
  #endif
  // UInt64 _pad_Alignment;  // is not required now
  CZstdDecFseTables fse;
  CZstdDecHufTable huf;
}
CZstdDec1;

#define ZstdDec1_GET_BLOCK_SIZE_LIMIT(p) \
  ((p)->winSize < kBlockSizeMax ? (UInt32)(p)->winSize : kBlockSizeMax)

#define SEQ_TABLES_WERE_NOT_SET_ml_accuracy  1  // accuracy=1 is not used by zstd
#define IS_SEQ_TABLES_WERE_SET(p)  (((p)->ml_accuracy != SEQ_TABLES_WERE_NOT_SET_ml_accuracy))
// #define IS_SEQ_TABLES_WERE_SET(p)  ((p)->seqTables_wereSet)


static void ZstdDec1_Construct(CZstdDec1 *p)
{
  #ifdef Z7_ZSTD_DEC_PRINT_TABLE
  Print_Predef(6, NUM_LL_SYMBOLS, SEQ_LL_PREDEF_DIST, k_PredefRecords_LL);
  Print_Predef(5, NUM_OFFSET_SYMBOLS_PREDEF, SEQ_OFFSET_PREDEF_DIST, k_PredefRecords_OF);
  Print_Predef(6, NUM_ML_SYMBOLS, SEQ_ML_PREDEF_DIST, k_PredefRecords_ML);
  #endif

  p->win = NULL;
  p->cycSize = 0;
  p->literalsBase = NULL;
  #ifdef Z7_ZSTD_DEC_USE_BASES_IN_OBJECT
  FILL_LOC_BASES_ALL
  #endif
}


static void ZstdDec1_Init(CZstdDec1 *p)
{
  p->reps[0] = 1;
  p->reps[1] = 4;
  p->reps[2] = 8;
  // p->seqTables_wereSet = False;
  p->ml_accuracy = SEQ_TABLES_WERE_NOT_SET_ml_accuracy;
  p->litHuf_wasSet = False;
  p->totalOutCheck = 0;
}



#ifdef MY_CPU_LE_UNALIGN
  #define Z7_ZSTD_DEC_USE_UNALIGNED_COPY
#endif

#ifdef Z7_ZSTD_DEC_USE_UNALIGNED_COPY

  #define COPY_CHUNK_SIZE 16

    #define COPY_CHUNK_4_2(dest, src) \
    { \
      ((UInt32 *)(void *)dest)[0] = ((const UInt32 *)(const void *)src)[0]; \
      ((UInt32 *)(void *)dest)[1] = ((const UInt32 *)(const void *)src)[1]; \
      src += 4 * 2; \
      dest += 4 * 2; \
    }

  /* sse2 doesn't help here in GCC and CLANG.
     so we disabled sse2 here */
  /*
  #if defined(MY_CPU_AMD64)
    #define Z7_ZSTD_DEC_USE_SSE2
  #elif defined(MY_CPU_X86)
    #if defined(_MSC_VER) && _MSC_VER >= 1300 && defined(_M_IX86_FP) && (_M_IX86_FP >= 2) \
      || defined(__SSE2__) \
      // || 1 == 1  // for debug only
      #define Z7_ZSTD_DEC_USE_SSE2
    #endif
  #endif
  */

  #if defined(MY_CPU_ARM64)
    #define COPY_OFFSET_MIN  16
    #define COPY_CHUNK1(dest, src) \
    { \
      vst1q_u8((uint8_t *)(void *)dest, \
      vld1q_u8((const uint8_t *)(const void *)src)); \
      src += 16; \
      dest += 16; \
    }
    
    #define COPY_CHUNK(dest, src) \
    { \
      COPY_CHUNK1(dest, src) \
      if ((len -= COPY_CHUNK_SIZE) == 0) break; \
      COPY_CHUNK1(dest, src) \
    }

  #elif defined(Z7_ZSTD_DEC_USE_SSE2)
    #include <emmintrin.h> // sse2
    #define COPY_OFFSET_MIN  16

    #define COPY_CHUNK1(dest, src) \
    { \
      _mm_storeu_si128((__m128i *)(void *)dest, \
      _mm_loadu_si128((const __m128i *)(const void *)src)); \
      src += 16; \
      dest += 16; \
    }

    #define COPY_CHUNK(dest, src) \
    { \
      COPY_CHUNK1(dest, src) \
      if ((len -= COPY_CHUNK_SIZE) == 0) break; \
      COPY_CHUNK1(dest, src) \
    }

  #elif defined(MY_CPU_64BIT)
    #define COPY_OFFSET_MIN  8

    #define COPY_CHUNK(dest, src) \
    { \
      ((UInt64 *)(void *)dest)[0] = ((const UInt64 *)(const void *)src)[0]; \
      ((UInt64 *)(void *)dest)[1] = ((const UInt64 *)(const void *)src)[1]; \
      src += 8 * 2; \
      dest += 8 * 2; \
    }

  #else
    #define COPY_OFFSET_MIN  4

    #define COPY_CHUNK(dest, src) \
    { \
      COPY_CHUNK_4_2(dest, src); \
      COPY_CHUNK_4_2(dest, src); \
    }

  #endif
#endif


#ifndef COPY_CHUNK_SIZE
    #define COPY_OFFSET_MIN  4
    #define COPY_CHUNK_SIZE  8
    #define COPY_CHUNK_2(dest, src) \
    { \
      const Byte a0 = src[0]; \
      const Byte a1 = src[1]; \
      dest[0] = a0; \
      dest[1] = a1; \
      src += 2; \
      dest += 2; \
    }
    #define COPY_CHUNK(dest, src) \
    { \
      COPY_CHUNK_2(dest, src) \
      COPY_CHUNK_2(dest, src) \
      COPY_CHUNK_2(dest, src) \
      COPY_CHUNK_2(dest, src) \
    }
#endif


#define COPY_PREPARE \
  len += (COPY_CHUNK_SIZE - 1); \
  len &= ~(size_t)(COPY_CHUNK_SIZE - 1); \
  { if (len > rem) \
  { len = rem; \
    rem &= (COPY_CHUNK_SIZE - 1); \
    if (rem) {  \
      len -= rem; \
      Z7_PRAGMA_OPT_DISABLE_LOOP_UNROLL_VECTORIZE \
      do *dest++ = *src++; while (--rem); \
      if (len == 0) return; }}}

#define COPY_CHUNKS \
{ \
  Z7_PRAGMA_OPT_DISABLE_LOOP_UNROLL_VECTORIZE \
  do { COPY_CHUNK(dest, src) } \
  while (len -= COPY_CHUNK_SIZE); \
}

// (len != 0)
// (len <= rem)
static
Z7_FORCE_INLINE
// Z7_ATTRIB_NO_VECTOR
void CopyLiterals(Byte *dest, Byte const *src, size_t len, size_t rem)
{
  COPY_PREPARE
  COPY_CHUNKS
}


/* we can define Z7_STD_DEC_USE_AFTER_CYC_BUF, if we want to use additional
   space after cycSize that can be used to reduce the code in CopyMatch(): */
// for debug:
// #define Z7_STD_DEC_USE_AFTER_CYC_BUF

/*
CopyMatch()
if wrap (offset > winPos)
{
  then we have at least (COPY_CHUNK_SIZE) avail in (dest) before we will overwrite (src):
  (cycSize >= offset + COPY_CHUNK_SIZE)
  if defined(Z7_STD_DEC_USE_AFTER_CYC_BUF)
    we are allowed to read win[cycSize + COPY_CHUNK_SIZE - 1],
}
(len != 0)
*/
static
Z7_FORCE_INLINE
// Z7_ATTRIB_NO_VECTOR
void CopyMatch(size_t offset, size_t len,
    Byte *win, size_t winPos, size_t rem, const size_t cycSize)
{
  Byte *dest = win + winPos;
  const Byte *src;
  // STAT_INC(g_NumCopy)

  if (offset > winPos)
  {
    size_t back = offset - winPos;
    // src = win + cycSize - back;
    // cycSize -= offset;
    STAT_INC(g_NumOver)
    src = dest + (cycSize - offset);
    // (src >= dest) here
   #ifdef Z7_STD_DEC_USE_AFTER_CYC_BUF
    if (back < len)
    {
   #else
    if (back < len + (COPY_CHUNK_SIZE - 1))
    {
      if (back >= len)
      {
        Z7_PRAGMA_OPT_DISABLE_LOOP_UNROLL_VECTORIZE
        do
          *dest++ = *src++;
        while (--len);
        return;
      }
   #endif
      // back < len
      STAT_INC(g_NumOver2)
      len -= back;
      rem -= back;
      Z7_PRAGMA_OPT_DISABLE_LOOP_UNROLL_VECTORIZE
      do
        *dest++ = *src++;
      while (--back);
      src = dest - offset;
      // src = win;
      // we go to MAIN-COPY
    }
  }
  else
    src = dest - offset;

  // len != 0
  // do *dest++ = *src++; while (--len); return;

  // --- MAIN COPY ---
  // if (src >= dest), then ((size_t)(src - dest) >= COPY_CHUNK_SIZE)
  //   so we have at least COPY_CHUNK_SIZE space before overlap for writing.
  COPY_PREPARE

  /* now (len == COPY_CHUNK_SIZE * x)
     so we can unroll for aligned copy */
  {
    // const unsigned b0 = src[0];
    // (COPY_OFFSET_MIN >= 4)
 
    if (offset >= COPY_OFFSET_MIN)
    {
      COPY_CHUNKS
      // return;
    }
    else
  #if (COPY_OFFSET_MIN > 4)
    #if COPY_CHUNK_SIZE < 8
      #error Stop_Compiling_Bad_COPY_CHUNK_SIZE
    #endif
    if (offset >= 4)
    {
      Z7_PRAGMA_OPT_DISABLE_LOOP_UNROLL_VECTORIZE
      do
      {
        COPY_CHUNK_4_2(dest, src)
        #if COPY_CHUNK_SIZE != 16
          if (len == 8) break;
        #endif
        COPY_CHUNK_4_2(dest, src)
      }
      while (len -= 16);
      // return;
    }
    else
  #endif
    {
      // (offset < 4)
      const unsigned b0 = src[0];
      if (offset < 2)
      {
      #if defined(Z7_ZSTD_DEC_USE_UNALIGNED_COPY) && (COPY_CHUNK_SIZE == 16)
        #if defined(MY_CPU_64BIT)
        {
          const UInt64 v64 = (UInt64)b0 * 0x0101010101010101;
          Z7_PRAGMA_OPT_DISABLE_LOOP_UNROLL_VECTORIZE
          do
          {
            ((UInt64 *)(void *)dest)[0] = v64;
            ((UInt64 *)(void *)dest)[1] = v64;
            dest += 16;
          }
          while (len -= 16);
        }
        #else
        {
          UInt32 v = b0;
          v |= v << 8;
          v |= v << 16;
          do
          {
            ((UInt32 *)(void *)dest)[0] = v;
            ((UInt32 *)(void *)dest)[1] = v;
            dest += 8;
            ((UInt32 *)(void *)dest)[0] = v;
            ((UInt32 *)(void *)dest)[1] = v;
            dest += 8;
          }
          while (len -= 16);
        }
        #endif
      #else
        do
        {
          dest[0] = (Byte)b0;
          dest[1] = (Byte)b0;
          dest += 2;
          dest[0] = (Byte)b0;
          dest[1] = (Byte)b0;
          dest += 2;
        }
        while (len -= 4);
      #endif
      }
      else if (offset == 2)
      {
        const Byte b1 = src[1];
        {
          do
          {
            dest[0] = (Byte)b0;
            dest[1] = b1;
            dest += 2;
          }
          while (len -= 2);
        }
      }
      else // (offset == 3)
      {
        const Byte *lim = dest + len - 2;
        const Byte b1 = src[1];
        const Byte b2 = src[2];
        do
        {
          dest[0] = (Byte)b0;
          dest[1] = b1;
          dest[2] = b2;
          dest += 3;
        }
        while (dest < lim);
        lim++; // points to last byte that must be written
        if (dest <= lim)
        {
          *dest = (Byte)b0;
          if (dest != lim)
            dest[1] = b1;
        }
      }
    }
  }
}



#define UPDATE_TOTAL_OUT(p, size) \
{ \
  size_t _toc = (p)->totalOutCheck + (size); \
  const size_t _ws = (p)->winSize; \
  if (_toc >= _ws) _toc = _ws; \
  (p)->totalOutCheck = _toc; \
}


#if defined(MY_CPU_64BIT) && defined(MY_CPU_LE_UNALIGN)
// we can disable it for debug:
#define Z7_ZSTD_DEC_USE_64BIT_LOADS
#endif
// #define Z7_ZSTD_DEC_USE_64BIT_LOADS // for debug : slow in 32-bit

// SEQ_SRC_OFFSET: how many bytes (src) (seqSrc) was moved back from original value.
// we need (SEQ_SRC_OFFSET != 0) for optimized memory access
#ifdef Z7_ZSTD_DEC_USE_64BIT_LOADS
  #define SEQ_SRC_OFFSET 7
#else
  #define SEQ_SRC_OFFSET 3
#endif
#define SRC_PLUS_FOR_4BYTES(bitOffset)  (SEQ_SRC_OFFSET - 3) + ((CBitCtr_signed)(bitOffset) >> 3)
#define BIT_OFFSET_7BITS(bitOffset)  ((unsigned)(bitOffset) & 7)
/*
  if (BIT_OFFSET_DELTA_BITS == 0) : bitOffset == number_of_unprocessed_bits
  if (BIT_OFFSET_DELTA_BITS == 1) : bitOffset == number_of_unprocessed_bits - 1
      and we can read 1 bit more in that mode : (8 * n + 1).
*/
// #define BIT_OFFSET_DELTA_BITS 0
#define BIT_OFFSET_DELTA_BITS 1
#if BIT_OFFSET_DELTA_BITS == 1
  #define GET_SHIFT_FROM_BOFFS7(boff7)  (7 ^ (boff7))
#else
  #define GET_SHIFT_FROM_BOFFS7(boff7)  (8 - BIT_OFFSET_DELTA_BITS - (boff7))
#endif

#define UPDATE_BIT_OFFSET(bitOffset, numBits) \
    (bitOffset) -= (CBitCtr)(numBits);

#define GET_SHIFT(bitOffset)  GET_SHIFT_FROM_BOFFS7(BIT_OFFSET_7BITS(bitOffset))


#if defined(Z7_ZSTD_DEC_USE_64BIT_LOADS)
  #if (NUM_OFFSET_SYMBOLS_MAX - BIT_OFFSET_DELTA_BITS < 32)
    /* if (NUM_OFFSET_SYMBOLS_MAX == 32 && BIT_OFFSET_DELTA_BITS == 1),
       we have depth 31 + 9 + 9 + 8 = 57 bits that can b read with single read. */
    #define Z7_ZSTD_DEC_USE_64BIT_PRELOAD_OF
  #endif
  #ifndef Z7_ZSTD_DEC_USE_64BIT_PRELOAD_OF
    #if (BIT_OFFSET_DELTA_BITS == 1)
    /* if (winLimit - winPos <= (kBlockSizeMax = (1 << 17)))
       {
         the case (16 bits literal extra + 16 match extra) is not possible
         in correct stream. So error will be detected for (16 + 16) case.
         And longest correct sequence after offset reading is (31 + 9 + 9 + 8 = 57 bits).
         So we can use just one 64-bit load here in that case.
       }
    */
    #define Z7_ZSTD_DEC_USE_64BIT_PRELOAD_ML
    #endif
  #endif
#endif


#if !defined(Z7_ZSTD_DEC_USE_64BIT_LOADS) || \
    (!defined(Z7_ZSTD_DEC_USE_64BIT_PRELOAD_OF) && \
     !defined(Z7_ZSTD_DEC_USE_64BIT_PRELOAD_ML))
// in : (0 < bits <= (24 or 25)):
#define STREAM_READ_BITS(dest, bits) \
{ \
  GET32(dest, src + SRC_PLUS_FOR_4BYTES(bitOffset)) \
  dest <<= GET_SHIFT(bitOffset); \
  UPDATE_BIT_OFFSET(bitOffset, bits) \
  dest >>= 32 - bits; \
}
#endif


#define FSE_Peek_1(table, state)  table[state]

#define STATE_VAR(name)  state_ ## name

// in : (0 <= accuracy <= (24 or 25))
#define FSE_INIT_STATE(name, cond) \
{ \
  UInt32 r; \
  const unsigned bits = p->name ## _accuracy; \
  GET32(r, src + SRC_PLUS_FOR_4BYTES(bitOffset)) \
  r <<= GET_SHIFT(bitOffset); \
  r >>= 1; \
  r >>= 31 ^ bits; \
  UPDATE_BIT_OFFSET(bitOffset, bits) \
  cond \
  STATE_VAR(name) = FSE_Peek_1(FSE_TABLE(name), r); \
  /* STATE_VAR(name) = dest << 16; */ \
}


#define FSE_Peek_Plus(name, r)  \
  STATE_VAR(name) = FSE_Peek_1(FSE_TABLE(name), \
    GET_FSE_REC_STATE(STATE_VAR(name)) + r);

#define LZ_LOOP_ERROR_EXIT  { return SZ_ERROR_DATA; }

#define BO_OVERFLOW_CHECK \
  { if ((CBitCtr_signed)bitOffset < 0) LZ_LOOP_ERROR_EXIT }


#ifdef Z7_ZSTD_DEC_USE_64BIT_LOADS

#define GET64(dest, p)  { const Byte *ptr = p;  dest = GetUi64(ptr); }

#define FSE_PRELOAD \
{ \
  GET64(v, src - 4 + SRC_PLUS_FOR_4BYTES(bitOffset)) \
  v <<= GET_SHIFT(bitOffset); \
}

#define FSE_UPDATE_STATE_2(name, cond) \
{ \
  const unsigned bits = GET_FSE_REC_LEN(STATE_VAR(name)); \
  UInt64 r = v; \
  v <<= bits; \
  r >>= 1; \
  UPDATE_BIT_OFFSET(bitOffset, bits) \
  cond \
  r >>= 63 ^ bits; \
  FSE_Peek_Plus(name, r); \
}

#define FSE_UPDATE_STATES \
  FSE_UPDATE_STATE_2 (ll, {} ) \
  FSE_UPDATE_STATE_2 (ml, {} ) \
  FSE_UPDATE_STATE_2 (of, BO_OVERFLOW_CHECK) \

#else // Z7_ZSTD_DEC_USE_64BIT_LOADS

// it supports 8 bits accuracy for any code
// it supports 9 bits accuracy, if (BIT_OFFSET_DELTA_BITS == 1)
#define FSE_UPDATE_STATE_0(name, cond) \
{ \
  UInt32 r; \
  const unsigned bits = GET_FSE_REC_LEN(STATE_VAR(name)); \
  GET16(r, src + 2 + SRC_PLUS_FOR_4BYTES(bitOffset)) \
  r >>= (bitOffset & 7); \
  r &= (1 << (8 + BIT_OFFSET_DELTA_BITS)) - 1; \
  UPDATE_BIT_OFFSET(bitOffset, bits) \
  cond \
  r >>= (8 + BIT_OFFSET_DELTA_BITS) - bits; \
  FSE_Peek_Plus(name, r); \
}

// for debug (slow):
// #define Z7_ZSTD_DEC_USE_FSE_FUSION_FORCE
#if BIT_OFFSET_DELTA_BITS == 0 || defined(Z7_ZSTD_DEC_USE_FSE_FUSION_FORCE)
  #define Z7_ZSTD_DEC_USE_FSE_FUSION
#endif

#ifdef Z7_ZSTD_DEC_USE_FSE_FUSION
#define FSE_UPDATE_STATE_1(name) \
{ UInt32 rest2; \
{ \
  UInt32 r; \
  unsigned bits; \
  GET32(r, src + SRC_PLUS_FOR_4BYTES(bitOffset)) \
  bits = GET_FSE_REC_LEN(STATE_VAR(name)); \
  r <<= GET_SHIFT(bitOffset); \
  rest2 = r << bits; \
  r >>= 1; \
  UPDATE_BIT_OFFSET(bitOffset, bits) \
  r >>= 31 ^ bits; \
  FSE_Peek_Plus(name, r); \
}

#define FSE_UPDATE_STATE_3(name) \
{ \
  const unsigned bits = GET_FSE_REC_LEN(STATE_VAR(name)); \
  rest2 >>= 1; \
  UPDATE_BIT_OFFSET(bitOffset, bits) \
  rest2 >>= 31 ^ bits; \
  FSE_Peek_Plus(name, rest2); \
}}

#define FSE_UPDATE_STATES \
  FSE_UPDATE_STATE_1 (ll) \
  FSE_UPDATE_STATE_3 (ml) \
  FSE_UPDATE_STATE_0 (of, BO_OVERFLOW_CHECK) \

#else // Z7_ZSTD_DEC_USE_64BIT_LOADS

#define FSE_UPDATE_STATES \
  FSE_UPDATE_STATE_0 (ll, {} ) \
  FSE_UPDATE_STATE_0 (ml, {} ) \
  FSE_UPDATE_STATE_0 (of, BO_OVERFLOW_CHECK) \

#endif // Z7_ZSTD_DEC_USE_FSE_FUSION
#endif // Z7_ZSTD_DEC_USE_64BIT_LOADS



typedef struct
{
  UInt32 numSeqs;
  UInt32 literalsLen;
  const Byte *literals;
}
CZstdDec1_Vars;


// if (BIT_OFFSET_DELTA_BITS != 0), we need (BIT_OFFSET_DELTA_BYTES > 0)
#define BIT_OFFSET_DELTA_BYTES   BIT_OFFSET_DELTA_BITS

/* if (NUM_OFFSET_SYMBOLS_MAX == 32)
     max_seq_bit_length = (31) + 16 + 16 + 9 + 8 + 9 = 89 bits
   if defined(Z7_ZSTD_DEC_USE_64BIT_PRELOAD_OF) we have longest backward
     lookahead offset, and we read UInt64 after literal_len reading.
   if (BIT_OFFSET_DELTA_BITS == 1 && NUM_OFFSET_SYMBOLS_MAX == 32)
     MAX_BACKWARD_DEPTH = 16 bytes
*/
#define MAX_BACKWARD_DEPTH  \
    ((NUM_OFFSET_SYMBOLS_MAX - 1 + 16 + 16 + 7) / 8 + 7 + BIT_OFFSET_DELTA_BYTES)

/* srcLen != 0
   src == real_data_ptr - SEQ_SRC_OFFSET - BIT_OFFSET_DELTA_BYTES
   if defined(Z7_ZSTD_DEC_USE_64BIT_PRELOAD_ML) then
     (winLimit - p->winPos <= (1 << 17)) is required
*/
static
Z7_NO_INLINE
// Z7_ATTRIB_NO_VECTOR
SRes Decompress_Sequences(CZstdDec1 * const p,
    const Byte *src, const size_t srcLen,
    const size_t winLimit,
    const CZstdDec1_Vars * const vars)
{
#ifdef Z7_ZSTD_DEC_USE_BASES_LOCAL
  SEQ_EXTRA_TABLES(a_)
#endif

  // for debug:
  // #define Z7_ZSTD_DEC_USE_LOCAL_FSE_TABLES
#ifdef Z7_ZSTD_DEC_USE_LOCAL_FSE_TABLES
  #define FSE_TABLE(n)  fse. n
  const CZstdDecFseTables fse = p->fse;
  /*
  CZstdDecFseTables fse;
  #define COPY_FSE_TABLE(n) \
    memcpy(fse. n, p->fse. n, (size_t)4 << p-> n ## _accuracy);
  COPY_FSE_TABLE(of)
  COPY_FSE_TABLE(ll)
  COPY_FSE_TABLE(ml)
  */
#else
  #define FSE_TABLE(n)  (p->fse.  n)
#endif

#ifdef Z7_ZSTD_DEC_USE_BASES_LOCAL
  FILL_LOC_BASES_ALL
#endif

  {
    unsigned numSeqs = vars->numSeqs;
    const Byte *literals = vars->literals;
    ptrdiff_t literalsLen = (ptrdiff_t)vars->literalsLen;
    Byte * const win = p->win;
    size_t winPos = p->winPos;
    const size_t cycSize = p->cycSize;
    size_t totalOutCheck = p->totalOutCheck;
    const size_t winSize = p->winSize;
    size_t reps_0 = p->reps[0];
    size_t reps_1 = p->reps[1];
    size_t reps_2 = p->reps[2];
    UInt32 STATE_VAR(ll), STATE_VAR(of), STATE_VAR(ml);
    CBitCtr bitOffset;

    SET_bitOffset_TO_PAD (bitOffset, src + SEQ_SRC_OFFSET, srcLen + BIT_OFFSET_DELTA_BYTES)

    bitOffset -= BIT_OFFSET_DELTA_BITS;

    FSE_INIT_STATE(ll, {} )
    FSE_INIT_STATE(of, {} )
    FSE_INIT_STATE(ml, BO_OVERFLOW_CHECK)

    for (;;)
    {
      size_t matchLen;
    #ifdef Z7_ZSTD_DEC_USE_64BIT_LOADS
      UInt64 v;
    #endif

      #ifdef Z7_ZSTD_DEC_USE_64BIT_PRELOAD_OF
        FSE_PRELOAD
      #endif

      // if (of_code == 0)
      if ((Byte)STATE_VAR(of) == 0)
      {
        if (GET_FSE_REC_SYM(STATE_VAR(ll)) == 0)
        {
          const size_t offset = reps_1;
          reps_1 = reps_0;
          reps_0 = offset;
          STAT_INC(g_Num_Rep1)
        }
        STAT_UPDATE(else g_Num_Rep0++;)
      }
      else
      {
        const unsigned of_code = (Byte)STATE_VAR(of);

      #ifdef Z7_ZSTD_DEC_USE_64BIT_LOADS
        #if !defined(Z7_ZSTD_DEC_USE_64BIT_PRELOAD_OF)
          FSE_PRELOAD
        #endif
      #else
        UInt32 v;
        {
          const Byte *src4 = src + SRC_PLUS_FOR_4BYTES(bitOffset);
          const unsigned skip = GET_SHIFT(bitOffset);
          GET32(v, src4)
          v <<= skip;
          v |= (UInt32)src4[-1] >> (8 - skip);
        }
      #endif

        UPDATE_BIT_OFFSET(bitOffset, of_code)

        if (of_code == 1)
        {
          // read 1 bit
          #if defined(Z7_MSC_VER_ORIGINAL) || defined(MY_CPU_X86_OR_AMD64)
            #ifdef Z7_ZSTD_DEC_USE_64BIT_LOADS
              #define CHECK_HIGH_BIT_64(a)  ((Int64)(UInt64)(a) < 0)
            #else
              #define CHECK_HIGH_BIT_32(a)  ((Int32)(UInt32)(a) < 0)
            #endif
          #else
            #ifdef Z7_ZSTD_DEC_USE_64BIT_LOADS
              #define CHECK_HIGH_BIT_64(a)  ((UInt64)(a) & ((UInt64)1 << 63))
            #else
              #define CHECK_HIGH_BIT_32(a)  ((UInt32)(a) & ((UInt32)1 << 31))
            #endif
          #endif

          if
            #ifdef Z7_ZSTD_DEC_USE_64BIT_LOADS
              CHECK_HIGH_BIT_64 (((UInt64)GET_FSE_REC_SYM(STATE_VAR(ll)) - 1) ^ v)
            #else
              CHECK_HIGH_BIT_32 (((UInt32)GET_FSE_REC_SYM(STATE_VAR(ll)) - 1) ^ v)
            #endif
          {
            v <<= 1;
            {
              const size_t offset = reps_2;
              reps_2 = reps_1;
              reps_1 = reps_0;
              reps_0 = offset;
              STAT_INC(g_Num_Rep2)
            }
          }
          else
          {
            if (GET_FSE_REC_SYM(STATE_VAR(ll)) == 0)
            {
              // litLen == 0 && bit == 1
              STAT_INC(g_Num_Rep3)
              v <<= 1;
              reps_2 = reps_1;
              reps_1 = reps_0;
              if (--reps_0 == 0)
              {
                // LZ_LOOP_ERROR_EXIT
                // original-zstd decoder : input is corrupted; force offset to 1
                // reps_0 = 1;
                reps_0++;
              }
            }
            else
            {
              // litLen != 0 && bit == 0
              v <<= 1;
              {
                const size_t offset = reps_1;
                reps_1 = reps_0;
                reps_0 = offset;
                STAT_INC(g_Num_Rep1)
              }
            }
          }
        }
        else
        {
          // (2 <= of_code)
          // if (of_code >= 32) LZ_LOOP_ERROR_EXIT // optional check
          // we don't allow (of_code >= 32) cases in another code
          reps_2 = reps_1;
          reps_1 = reps_0;
          reps_0 = ((size_t)1 << of_code) - 3 + (size_t)
            #ifdef Z7_ZSTD_DEC_USE_64BIT_LOADS
              (v >> (64 - of_code));
              v <<= of_code;
            #else
              (v >> (32 - of_code));
            #endif
        }
      }

      #ifdef Z7_ZSTD_DEC_USE_64BIT_PRELOAD_ML
        FSE_PRELOAD
      #endif

      matchLen = (size_t)GET_FSE_REC_SYM(STATE_VAR(ml))
          #ifndef Z7_ZSTD_DEC_USE_ML_PLUS3
            + MATCH_LEN_MIN
          #endif
          ;
      {
        {
          if (matchLen >= 32 + MATCH_LEN_MIN) // if (state_ml & 0x20)
          {
            const unsigned extra = BASES_TABLE(SEQ_ML_EXTRA) [(size_t)matchLen - MATCH_LEN_MIN];
            matchLen = BASES_TABLE(SEQ_ML_BASES) [(size_t)matchLen - MATCH_LEN_MIN];
            #if defined(Z7_ZSTD_DEC_USE_64BIT_LOADS) && \
               (defined(Z7_ZSTD_DEC_USE_64BIT_PRELOAD_ML) || \
                defined(Z7_ZSTD_DEC_USE_64BIT_PRELOAD_OF))
            {
              UPDATE_BIT_OFFSET(bitOffset, extra)
              matchLen += (size_t)(v >> (64 - extra));
              #if defined(Z7_ZSTD_DEC_USE_64BIT_PRELOAD_OF)
                FSE_PRELOAD
              #else
                v <<= extra;
              #endif
            }
            #else
            {
              UInt32 v32;
              STREAM_READ_BITS(v32, extra)
              matchLen += v32;
            }
            #endif
            STAT_INC(g_Num_Match)
          }
        }
      }

      #if  defined(Z7_ZSTD_DEC_USE_64BIT_LOADS) && \
          !defined(Z7_ZSTD_DEC_USE_64BIT_PRELOAD_OF) && \
          !defined(Z7_ZSTD_DEC_USE_64BIT_PRELOAD_ML)
        FSE_PRELOAD
      #endif

      {
        size_t litLen = GET_FSE_REC_SYM(STATE_VAR(ll));
        if (litLen)
        {
          // if (STATE_VAR(ll) & 0x70)
          if (litLen >= 16)
          {
            const unsigned extra = BASES_TABLE(SEQ_LL_EXTRA) [litLen];
            litLen = BASES_TABLE(SEQ_LL_BASES) [litLen];
            #ifdef Z7_ZSTD_DEC_USE_64BIT_LOADS
            {
              UPDATE_BIT_OFFSET(bitOffset, extra)
              litLen += (size_t)(v >> (64 - extra));
              #if defined(Z7_ZSTD_DEC_USE_64BIT_PRELOAD_OF)
                FSE_PRELOAD
              #else
                v <<= extra;
              #endif
            }
            #else
            {
              UInt32 v32;
              STREAM_READ_BITS(v32, extra)
              litLen += v32;
            }
            #endif
            STAT_INC(g_Num_LitsBig)
          }

          if ((literalsLen -= (ptrdiff_t)litLen) < 0)
            LZ_LOOP_ERROR_EXIT
          totalOutCheck += litLen;
          {
            const size_t rem = winLimit - winPos;
            if (litLen > rem)
              LZ_LOOP_ERROR_EXIT
            {
              const Byte *literals_temp = literals;
              Byte *d = win + winPos;
              literals += litLen;
              winPos += litLen;
              CopyLiterals(d, literals_temp, litLen, rem);
            }
          }
        }
        STAT_UPDATE(else g_Num_Lit0++;)
      }

      #define COPY_MATCH \
        { if (reps_0 > winSize || reps_0 > totalOutCheck) LZ_LOOP_ERROR_EXIT \
        totalOutCheck += matchLen; \
        { const size_t rem = winLimit - winPos; \
        if (matchLen > rem) LZ_LOOP_ERROR_EXIT \
        { const size_t winPos_temp = winPos; \
        winPos += matchLen; \
        CopyMatch(reps_0, matchLen, win, winPos_temp, rem, cycSize); }}}

      if (--numSeqs == 0)
      {
        COPY_MATCH
        break;
      }
      FSE_UPDATE_STATES
      COPY_MATCH
    } // for

    if ((CBitCtr_signed)bitOffset != BIT_OFFSET_DELTA_BYTES * 8 - BIT_OFFSET_DELTA_BITS)
      return SZ_ERROR_DATA;
    
    if (literalsLen)
    {
      const size_t rem = winLimit - winPos;
      if ((size_t)literalsLen > rem)
        return SZ_ERROR_DATA;
      {
        Byte *d = win + winPos;
        winPos += (size_t)literalsLen;
        totalOutCheck += (size_t)literalsLen;
        CopyLiterals
        // memcpy
          (d, literals, (size_t)literalsLen, rem);
      }
    }
    if (totalOutCheck >= winSize)
      totalOutCheck = winSize;
    p->totalOutCheck = totalOutCheck;
    p->winPos = winPos;
    p->reps[0] = (CZstdDecOffset)reps_0;
    p->reps[1] = (CZstdDecOffset)reps_1;
    p->reps[2] = (CZstdDecOffset)reps_2;
  }
  return SZ_OK;
}


// for debug: define to check that ZstdDec1_NeedTempBufferForInput() works correctly:
// #define Z7_ZSTD_DEC_USE_CHECK_OF_NEED_TEMP // define it for debug only
#ifdef Z7_ZSTD_DEC_USE_CHECK_OF_NEED_TEMP
static unsigned g_numSeqs;
#endif


#define k_LitBlockType_Flag_RLE_or_Treeless  1
#define k_LitBlockType_Flag_Compressed       2

// outLimit : is strong limit
// outLimit <= ZstdDec1_GET_BLOCK_SIZE_LIMIT(p)
// inSize != 0
static
Z7_NO_INLINE
SRes ZstdDec1_DecodeBlock(CZstdDec1 *p,
    const Byte *src, SizeT inSize, SizeT afterAvail,
    const size_t outLimit)
{
  CZstdDec1_Vars vars;
  vars.literals = p->literalsBase;
  {
    const unsigned b0 = *src++;
    UInt32 numLits, compressedSize;
    const Byte *litStream;
    Byte *literalsDest;
    inSize--;
    
    if ((b0 & k_LitBlockType_Flag_Compressed) == 0)
    {
      // we need at least one additional byte for (numSeqs).
      // so we check for that additional byte in conditions.
      numLits = b0 >> 3;
      if (b0 & 4)
      {
        UInt32 v;
        if (inSize < 1 + 1) // we need at least 1 byte here and 1 byte for (numSeqs).
          return SZ_ERROR_DATA;
        numLits >>= 1;
        v = GetUi16(src);
        src += 2;
        inSize -= 2;
        if ((b0 & 8) == 0)
        {
          src--;
          inSize++;
          v = (Byte)v;
        }
        numLits += v << 4;
      }
      compressedSize = 1;
      if ((b0 & k_LitBlockType_Flag_RLE_or_Treeless) == 0)
        compressedSize = numLits;
    }
    else if (inSize < 4)
      return SZ_ERROR_DATA;
    else
    {
      const unsigned mode4Streams = b0 & 0xc;
      const unsigned numBytes = (3 * mode4Streams + 32) >> 4;
      const unsigned numBits = 4 * numBytes - 2;
      const UInt32 mask = ((UInt32)16 << numBits) - 1;
      compressedSize = GetUi32(src);
      numLits = ((
          #ifdef MY_CPU_LE_UNALIGN
            GetUi32(src - 1)
          #else
            ((compressedSize << 8) + b0)
          #endif
          ) >> 4) & mask;
      src += numBytes;
      inSize -= numBytes;
      compressedSize >>= numBits;
      compressedSize &= mask;
      /*
      if (numLits != 0) printf("inSize = %7u num_lits=%7u compressed=%7u ratio = %u  ratio2 = %u\n",
          i1, numLits, (unsigned)compressedSize * 1, (unsigned)compressedSize * 100 / numLits,
          (unsigned)numLits * 100 / (unsigned)inSize);
      }
      */
      if (compressedSize == 0)
        return SZ_ERROR_DATA; // (compressedSize == 0) is not allowed
    }

    STAT_UPDATE(g_Num_Lits += numLits;)

    vars.literalsLen = numLits;

    if (compressedSize >= inSize)
      return SZ_ERROR_DATA;
    litStream = src;
    src += compressedSize;
    inSize -= compressedSize;
    // inSize != 0
    {
      UInt32 numSeqs = *src++;
      inSize--;
      if (numSeqs > 127)
      {
        UInt32 b1;
        if (inSize == 0)
          return SZ_ERROR_DATA;
        numSeqs -= 128;
        b1 = *src++;
        inSize--;
        if (numSeqs == 127)
        {
          if (inSize == 0)
            return SZ_ERROR_DATA;
          numSeqs = (UInt32)(*src++) + 127;
          inSize--;
        }
        numSeqs = (numSeqs << 8) + b1;
      }
      if (numSeqs * MATCH_LEN_MIN + numLits > outLimit)
        return SZ_ERROR_DATA;
      vars.numSeqs = numSeqs;

      STAT_UPDATE(g_NumSeqs_total += numSeqs;)
      /*
        #ifdef SHOW_STAT
        printf("\n %5u : %8u, %8u : %5u", (int)g_Num_Blocks_Compressed, (int)numSeqs, (int)g_NumSeqs_total,
          (int)g_NumSeqs_total / g_Num_Blocks_Compressed);
        #endif
        // printf("\nnumSeqs2 = %d", numSeqs);
      */
    #ifdef Z7_ZSTD_DEC_USE_CHECK_OF_NEED_TEMP
      if (numSeqs != g_numSeqs) return SZ_ERROR_DATA; // for debug
    #endif
      if (numSeqs == 0)
      {
        if (inSize != 0)
          return SZ_ERROR_DATA;
        literalsDest = p->win + p->winPos;
      }
      else
        literalsDest = p->literalsBase;
    }
    
    if ((b0 & k_LitBlockType_Flag_Compressed) == 0)
    {
      if (b0 & k_LitBlockType_Flag_RLE_or_Treeless)
      {
        memset(literalsDest, litStream[0], numLits);
        if (vars.numSeqs)
        {
          // literalsDest == p->literalsBase == vars.literals
          #if COPY_CHUNK_SIZE > 1
            memset(p->literalsBase + numLits, 0, COPY_CHUNK_SIZE);
          #endif
        }
      }
      else
      {
        // unsigned y;
        // for (y = 0; y < 10000; y++)
        memcpy(literalsDest, litStream, numLits);
        if (vars.numSeqs)
        {
          /* we need up to (15 == COPY_CHUNK_SIZE - 1) space for optimized CopyLiterals().
             If we have additional space in input stream after literals stream,
             we use direct copy of rar literals in input stream */
          if ((size_t)(src + inSize - litStream) - numLits + afterAvail >= (COPY_CHUNK_SIZE - 1))
            vars.literals = litStream;
          else
          {
            // literalsDest == p->literalsBase == vars.literals
            #if COPY_CHUNK_SIZE > 1
            /* CopyLiterals():
                1) we don't want reading non-initialized data
                2) we will copy only zero byte after literals buffer */
              memset(p->literalsBase + numLits, 0, COPY_CHUNK_SIZE);
            #endif
          }
        }
      }
    }
    else
    {
      CInBufPair hufStream;
      hufStream.ptr = litStream;
      hufStream.len = compressedSize;
      
      if ((b0 & k_LitBlockType_Flag_RLE_or_Treeless) == 0)
      {
        // unsigned y = 100; CInBufPair hs2 = hufStream; do { hufStream = hs2;
        RINOK(Huf_DecodeTable(&p->huf, &hufStream))
        p->litHuf_wasSet = True;
        // } while (--y);
      }
      else if (!p->litHuf_wasSet)
        return SZ_ERROR_DATA;
      
      {
        // int yyy; for (yyy = 0; yyy < 34; yyy++) {
        SRes sres;
        if ((b0 & 0xc) == 0) // mode4Streams
          sres = Huf_Decompress_1stream((const Byte *)(const void *)p->huf.table64,
              hufStream.ptr - HUF_SRC_OFFSET, hufStream.len, literalsDest, numLits);
        else
        {
          // 6 bytes for the jump table + 4x1 bytes of end-padding Bytes)
          if (hufStream.len < 6 + 4)
            return SZ_ERROR_DATA;
          // the condition from original-zstd decoder:
          #define Z7_ZSTD_MIN_LITERALS_FOR_4_STREAMS 6
          if (numLits < Z7_ZSTD_MIN_LITERALS_FOR_4_STREAMS)
            return SZ_ERROR_DATA;
          sres = Huf_Decompress_4stream((const Byte *)(const void *)p->huf.table64,
              hufStream.ptr + (6 - HUF_SRC_OFFSET), hufStream.len, literalsDest, numLits);
        }
        RINOK(sres)
        // }
      }
    }

    if (vars.numSeqs == 0)
    {
      p->winPos += numLits;
      return SZ_OK;
    }
  }
  {
    CInBufPair in;
    unsigned mode;
    unsigned seqMode;
      
    in.ptr = src;
    in.len = inSize;
    if (in.len == 0)
      return SZ_ERROR_DATA;
    in.len--;
    mode = *in.ptr++;
    if (mode & 3) // Reserved bits
      return SZ_ERROR_DATA;
    
    seqMode = (mode >> 6);
    if (seqMode == k_SeqMode_Repeat)
      { if (!IS_SEQ_TABLES_WERE_SET(p)) return SZ_ERROR_DATA; }
    else RINOK(FSE_Decode_SeqTable(
        p->fse.ll,
        &in,
        6, // predefAccuracy
        &p->ll_accuracy,
        NUM_LL_SYMBOLS,
        k_PredefRecords_LL,
        seqMode))
      
    seqMode = (mode >> 4) & 3;
    if (seqMode == k_SeqMode_Repeat)
      { if (!IS_SEQ_TABLES_WERE_SET(p)) return SZ_ERROR_DATA; }
    else RINOK(FSE_Decode_SeqTable(
        p->fse.of,
        &in,
        5, // predefAccuracy
        &p->of_accuracy,
        NUM_OFFSET_SYMBOLS_MAX,
        k_PredefRecords_OF,
        seqMode))
       
    seqMode = (mode >> 2) & 3;
    if (seqMode == k_SeqMode_Repeat)
      { if (!IS_SEQ_TABLES_WERE_SET(p)) return SZ_ERROR_DATA; }
    else
    {
      RINOK(FSE_Decode_SeqTable(
        p->fse.ml,
        &in,
        6, // predefAccuracy
        &p->ml_accuracy,
        NUM_ML_SYMBOLS,
        k_PredefRecords_ML,
        seqMode))
      /*
      #if defined(Z7_ZSTD_DEC_USE_ML_PLUS3)
        // { unsigned y = 1 << 10; do
      {
        const unsigned accuracy = p->ml_accuracy;
        if (accuracy == 0)
          p->fse.ml[0] += 3;
        else
        #ifdef MY_CPU_64BIT
        {
          // alignemt (UInt64 _pad_Alignment) in fse.ml is required for that code
          UInt64 *table = (UInt64 *)(void *)p->fse.ml;
          const UInt64 *end = (const UInt64 *)(const void *)
            ((const Byte *)(const void *)table + ((size_t)sizeof(CFseRecord) << accuracy));
          do
          {
            table[0] += ((UInt64)MATCH_LEN_MIN << 32) + MATCH_LEN_MIN;
            table[1] += ((UInt64)MATCH_LEN_MIN << 32) + MATCH_LEN_MIN;
            table += 2;
          }
          while (table != end);
        }
        #else
        {
          UInt32 *table = p->fse.ml;
          const UInt32 *end = (const UInt32 *)(const void *)
            ((const Byte *)(const void *)table + ((size_t)sizeof(CFseRecord) << accuracy));
          do
          {
            table[0] += MATCH_LEN_MIN;
            table[1] += MATCH_LEN_MIN;
            table += 2;
            table[0] += MATCH_LEN_MIN;
            table[1] += MATCH_LEN_MIN;
            table += 2;
          }
          while (table != end);
        }
        #endif
      }
      // while (--y); }
      #endif
      */
    }
    
    // p->seqTables_wereSet = True;
    if (in.len == 0)
      return SZ_ERROR_DATA;
    return Decompress_Sequences(p,
        in.ptr - SEQ_SRC_OFFSET - BIT_OFFSET_DELTA_BYTES, in.len,
        p->winPos + outLimit, &vars);
  }
}




// inSize != 0
// it must do similar to ZstdDec1_DecodeBlock()
static size_t ZstdDec1_NeedTempBufferForInput(
    const SizeT beforeSize, const Byte * const src, const SizeT inSize)
{
  unsigned b0;
  UInt32 pos;

  #ifdef Z7_ZSTD_DEC_USE_CHECK_OF_NEED_TEMP
    g_numSeqs = 1 << 24;
  #else
  // we have at least 3 bytes before seq data: litBlockType, numSeqs, seqMode
  #define MIN_BLOCK_LZ_HEADERS_SIZE 3
  if (beforeSize >= MAX_BACKWARD_DEPTH - MIN_BLOCK_LZ_HEADERS_SIZE)
    return 0;
  #endif

  b0 = src[0];
  
  if ((b0 & k_LitBlockType_Flag_Compressed) == 0)
  {
    UInt32 numLits = b0 >> 3;
    pos = 1;
    if (b0 & 4)
    {
      UInt32 v;
      if (inSize < 3)
        return 0;
      numLits >>= 1;
      v = GetUi16(src + 1);
      pos = 3;
      if ((b0 & 8) == 0)
      {
        pos = 2;
        v = (Byte)v;
      }
      numLits += v << 4;
    }
    if (b0 & k_LitBlockType_Flag_RLE_or_Treeless)
      numLits = 1;
    pos += numLits;
  }
  else if (inSize < 5)
    return 0;
  else
  {
    const unsigned mode4Streams = b0 & 0xc;
    const unsigned numBytes = (3 * mode4Streams + 48) >> 4;
    const unsigned numBits = 4 * numBytes - 6;
    UInt32 cs = GetUi32(src + 1);
    cs >>= numBits;
    cs &= ((UInt32)16 << numBits) - 1;
    if (cs == 0)
      return 0;
    pos = numBytes + cs;
  }
  
  if (pos >= inSize)
    return 0;
  {
    UInt32 numSeqs = src[pos++];
    if (numSeqs > 127)
    {
      UInt32 b1;
      if (pos >= inSize)
        return 0;
      numSeqs -= 128;
      b1 = src[pos++];
      if (numSeqs == 127)
      {
        if (pos >= inSize)
          return 0;
        numSeqs = (UInt32)(src[pos++]) + 127;
      }
      numSeqs = (numSeqs << 8) + b1;
    }
    #ifdef Z7_ZSTD_DEC_USE_CHECK_OF_NEED_TEMP
      g_numSeqs = numSeqs; // for debug
    #endif
    if (numSeqs == 0)
      return 0;
  }
  /*
  if (pos >= inSize)
    return 0;
  pos++;
  */
  // we will have one additional byte for seqMode:
  if (beforeSize + pos >= MAX_BACKWARD_DEPTH - 1)
    return 0;
  return 1;
}



// ---------- ZSTD FRAME ----------

#define kBlockType_Raw          0
#define kBlockType_RLE          1
#define kBlockType_Compressed   2
#define kBlockType_Reserved     3

typedef enum
{
  // begin: states that require 4 bytes:
  ZSTD2_STATE_SIGNATURE,
  ZSTD2_STATE_HASH,
  ZSTD2_STATE_SKIP_HEADER,
  // end of states that require 4 bytes

  ZSTD2_STATE_SKIP_DATA,
  ZSTD2_STATE_FRAME_HEADER,
  ZSTD2_STATE_AFTER_HEADER,
  ZSTD2_STATE_BLOCK,
  ZSTD2_STATE_DATA,
  ZSTD2_STATE_FINISHED
} EZstd2State;


struct CZstdDec
{
  EZstd2State frameState;
  unsigned tempSize;
  
  Byte temp[14]; // 14 is required

  Byte descriptor;
  Byte windowDescriptor;
  Byte isLastBlock;
  Byte blockType;
  Byte isErrorState;
  Byte hashError;
  Byte disableHash;
  Byte isCyclicMode;
  
  UInt32 blockSize;
  UInt32 dictionaryId;
  UInt32 curBlockUnpackRem; // for compressed blocks only
  UInt32 inTempPos;

  UInt64 contentSize;
  UInt64 contentProcessed;
  CXxh64State xxh64;

  Byte *inTemp;
  SizeT winBufSize_Allocated;
  Byte *win_Base;

  ISzAllocPtr alloc_Small;
  ISzAllocPtr alloc_Big;

  CZstdDec1 decoder;
};

#define ZstdDec_GET_UNPROCESSED_XXH64_SIZE(p) \
  ((unsigned)(p)->contentProcessed & (Z7_XXH64_BLOCK_SIZE - 1))

#define ZSTD_DEC_IS_LAST_BLOCK(p) ((p)->isLastBlock)


static void ZstdDec_FreeWindow(CZstdDec * const p)
{
  if (p->win_Base)
  {
    ISzAlloc_Free(p->alloc_Big, p->win_Base);
    p->win_Base = NULL;
    // p->decoder.win = NULL;
    p->winBufSize_Allocated = 0;
  }
}


CZstdDecHandle ZstdDec_Create(ISzAllocPtr alloc_Small, ISzAllocPtr alloc_Big)
{
  CZstdDec *p = (CZstdDec *)ISzAlloc_Alloc(alloc_Small, sizeof(CZstdDec));
  if (!p)
    return NULL;
  p->alloc_Small = alloc_Small;
  p->alloc_Big = alloc_Big;
  // ZstdDec_CONSTRUCT(p)
  p->inTemp = NULL;
  p->win_Base = NULL;
  p->winBufSize_Allocated = 0;
  p->disableHash = False;
  ZstdDec1_Construct(&p->decoder);
  return p;
}

void ZstdDec_Destroy(CZstdDecHandle p)
{
  #ifdef SHOW_STAT
    #define PRINT_STAT1(name, v) \
      printf("\n%25s = %9u", name, v);
  PRINT_STAT1("g_Num_Blocks_Compressed", g_Num_Blocks_Compressed)
  PRINT_STAT1("g_Num_Blocks_memcpy", g_Num_Blocks_memcpy)
  PRINT_STAT1("g_Num_Wrap_memmove_Num", g_Num_Wrap_memmove_Num)
  PRINT_STAT1("g_Num_Wrap_memmove_Bytes", g_Num_Wrap_memmove_Bytes)
  if (g_Num_Blocks_Compressed)
  {
    #define PRINT_STAT(name, v) \
      printf("\n%17s = %9u, per_block = %8u", name, v, v / g_Num_Blocks_Compressed);
    PRINT_STAT("g_NumSeqs", g_NumSeqs_total)
    // PRINT_STAT("g_NumCopy", g_NumCopy)
    PRINT_STAT("g_NumOver", g_NumOver)
    PRINT_STAT("g_NumOver2", g_NumOver2)
    PRINT_STAT("g_Num_Match", g_Num_Match)
    PRINT_STAT("g_Num_Lits", g_Num_Lits)
    PRINT_STAT("g_Num_LitsBig", g_Num_LitsBig)
    PRINT_STAT("g_Num_Lit0", g_Num_Lit0)
    PRINT_STAT("g_Num_Rep_0", g_Num_Rep0)
    PRINT_STAT("g_Num_Rep_1", g_Num_Rep1)
    PRINT_STAT("g_Num_Rep_2", g_Num_Rep2)
    PRINT_STAT("g_Num_Rep_3", g_Num_Rep3)
    PRINT_STAT("g_Num_Threshold_0", g_Num_Threshold_0)
    PRINT_STAT("g_Num_Threshold_1", g_Num_Threshold_1)
    PRINT_STAT("g_Num_Threshold_0sum", g_Num_Threshold_0sum)
    PRINT_STAT("g_Num_Threshold_1sum", g_Num_Threshold_1sum)
  }
  printf("\n");
  #endif

  ISzAlloc_Free(p->alloc_Small, p->decoder.literalsBase);
  // p->->decoder.literalsBase = NULL;
  ISzAlloc_Free(p->alloc_Small, p->inTemp);
  // p->inTemp = NULL;
  ZstdDec_FreeWindow(p);
  ISzAlloc_Free(p->alloc_Small, p);
}



#define kTempBuffer_PreSize  (1u << 6)
#if kTempBuffer_PreSize < MAX_BACKWARD_DEPTH
  #error Stop_Compiling_Bad_kTempBuffer_PreSize
#endif

static SRes ZstdDec_AllocateMisc(CZstdDec *p)
{
  #define k_Lit_AfterAvail  (1u << 6)
  #if k_Lit_AfterAvail < (COPY_CHUNK_SIZE - 1)
    #error Stop_Compiling_Bad_k_Lit_AfterAvail
  #endif
  // return ZstdDec1_Allocate(&p->decoder, p->alloc_Small);
  if (!p->decoder.literalsBase)
  {
    p->decoder.literalsBase = (Byte *)ISzAlloc_Alloc(p->alloc_Small,
        kBlockSizeMax + k_Lit_AfterAvail);
    if (!p->decoder.literalsBase)
      return SZ_ERROR_MEM;
  }
  if (!p->inTemp)
  {
    // we need k_Lit_AfterAvail here for owerread from raw literals stream
    p->inTemp = (Byte *)ISzAlloc_Alloc(p->alloc_Small,
        kBlockSizeMax + kTempBuffer_PreSize + k_Lit_AfterAvail);
    if (!p->inTemp)
      return SZ_ERROR_MEM;
  }
  return SZ_OK;
}


static void ZstdDec_Init_ForNewFrame(CZstdDec *p)
{
  p->frameState = ZSTD2_STATE_SIGNATURE;
  p->tempSize = 0;

  p->isErrorState = False;
  p->hashError = False;
  p->isCyclicMode = False;
  p->contentProcessed = 0;
  Xxh64State_Init(&p->xxh64);
  ZstdDec1_Init(&p->decoder);
}


void ZstdDec_Init(CZstdDec *p)
{
  ZstdDec_Init_ForNewFrame(p);
  p->decoder.winPos = 0;
  memset(p->temp, 0, sizeof(p->temp));
}


#define DESCRIPTOR_Get_DictionaryId_Flag(d)   ((d) & 3)
#define DESCRIPTOR_FLAG_CHECKSUM              (1 << 2)
#define DESCRIPTOR_FLAG_RESERVED              (1 << 3)
// #define DESCRIPTOR_FLAG_UNUSED                (1 << 4)
#define DESCRIPTOR_FLAG_SINGLE                (1 << 5)
#define DESCRIPTOR_Get_ContentSize_Flag3(d)   ((d) >> 5)
#define DESCRIPTOR_Is_ContentSize_Defined(d)  (((d) & 0xe0) != 0)


static EZstd2State ZstdDec_UpdateState(CZstdDec * const p, const Byte b, CZstdDecInfo * const info)
{
  unsigned tempSize = p->tempSize;
  p->temp[tempSize++] = b;
  p->tempSize = tempSize;

  if (p->frameState == ZSTD2_STATE_BLOCK)
  {
    if (tempSize < 3)
      return ZSTD2_STATE_BLOCK;
    {
      UInt32 b0 = GetUi32(p->temp);
      const unsigned type = ((unsigned)b0 >> 1) & 3;
      if (type == kBlockType_RLE && tempSize == 3)
        return ZSTD2_STATE_BLOCK;
      // info->num_Blocks_forType[type]++;
      info->num_Blocks++;
      if (type == kBlockType_Reserved)
      {
        p->isErrorState = True; // SZ_ERROR_UNSUPPORTED
        return ZSTD2_STATE_BLOCK;
      }
      p->blockType = (Byte)type;
      p->isLastBlock = (Byte)(b0 & 1);
      p->inTempPos = 0;
      p->tempSize = 0;
      b0 >>= 3;
      b0 &= 0x1fffff;
      // info->num_BlockBytes_forType[type] += b0;
      if (b0 == 0)
      {
        // empty RAW/RLE blocks are allowed in original-zstd decoder
        if (type == kBlockType_Compressed)
        {
          p->isErrorState = True;
          return ZSTD2_STATE_BLOCK;
        }
        if (!ZSTD_DEC_IS_LAST_BLOCK(p))
          return ZSTD2_STATE_BLOCK;
        if (p->descriptor & DESCRIPTOR_FLAG_CHECKSUM)
          return ZSTD2_STATE_HASH;
        return ZSTD2_STATE_FINISHED;
      }
      p->blockSize = b0;
      {
        UInt32 blockLim = ZstdDec1_GET_BLOCK_SIZE_LIMIT(&p->decoder);
        // compressed and uncompressed block sizes cannot be larger than min(kBlockSizeMax, window_size)
        if (b0 > blockLim)
        {
          p->isErrorState = True; // SZ_ERROR_UNSUPPORTED;
          return ZSTD2_STATE_BLOCK;
        }
        if (DESCRIPTOR_Is_ContentSize_Defined(p->descriptor))
        {
          const UInt64 rem = p->contentSize - p->contentProcessed;
          if (blockLim > rem)
              blockLim = (UInt32)rem;
        }
        p->curBlockUnpackRem = blockLim;
        // uncompressed block size cannot be larger than remain data size:
        if (type != kBlockType_Compressed)
        {
          if (b0 > blockLim)
          {
            p->isErrorState = True; // SZ_ERROR_UNSUPPORTED;
            return ZSTD2_STATE_BLOCK;
          }
        }
      }
    }
    return ZSTD2_STATE_DATA;
  }
  
  if ((unsigned)p->frameState < ZSTD2_STATE_SKIP_DATA)
  {
    UInt32 v;
    if (tempSize != 4)
      return p->frameState;
    v = GetUi32(p->temp);
    if ((unsigned)p->frameState < ZSTD2_STATE_HASH) // == ZSTD2_STATE_SIGNATURE
    {
      if (v == 0xfd2fb528)
      {
        p->tempSize = 0;
        info->num_DataFrames++;
        return ZSTD2_STATE_FRAME_HEADER;
      }
      if ((v & 0xfffffff0) == 0x184d2a50)
      {
        p->tempSize = 0;
        info->num_SkipFrames++;
        return ZSTD2_STATE_SKIP_HEADER;
      }
      p->isErrorState = True;
      return ZSTD2_STATE_SIGNATURE;
      // return ZSTD2_STATE_ERROR; // is not ZSTD stream
    }
    if (p->frameState == ZSTD2_STATE_HASH)
    {
      info->checksum_Defined = True;
      info->checksum = v;
      // #ifndef DISABLE_XXH_CHECK
      if (!p->disableHash)
      {
        if (p->decoder.winPos < ZstdDec_GET_UNPROCESSED_XXH64_SIZE(p))
        {
          // unexpected code failure
          p->isErrorState = True;
          // SZ_ERROR_FAIL;
        }
        else
        if ((UInt32)Xxh64State_Digest(&p->xxh64,
            p->decoder.win + (p->decoder.winPos - ZstdDec_GET_UNPROCESSED_XXH64_SIZE(p)),
            p->contentProcessed) != v)
        {
          p->hashError = True;
          // return ZSTD2_STATE_ERROR; // hash error
        }
      }
      // #endif
      return ZSTD2_STATE_FINISHED;
    }
    // (p->frameState == ZSTD2_STATE_SKIP_HEADER)
    {
      p->blockSize = v;
      info->skipFrames_Size += v;
      p->tempSize = 0;
      /* we want the caller could know that there was finished frame
         finished frame. So we allow the case where
         we have ZSTD2_STATE_SKIP_DATA state with (blockSize == 0).
      */
      // if (v == 0) return ZSTD2_STATE_SIGNATURE;
      return ZSTD2_STATE_SKIP_DATA;
    }
  }

  // if (p->frameState == ZSTD2_STATE_FRAME_HEADER)
  {
    unsigned descriptor;
    const Byte *h;
    descriptor = p->temp[0];
    p->descriptor = (Byte)descriptor;
    if (descriptor & DESCRIPTOR_FLAG_RESERVED) // reserved bit
    {
      p->isErrorState = True;
      return ZSTD2_STATE_FRAME_HEADER;
      // return ZSTD2_STATE_ERROR;
    }
    {
      const unsigned n = DESCRIPTOR_Get_ContentSize_Flag3(descriptor);
      // tempSize -= 1 + ((1u << (n >> 1)) | ((n + 1) & 1));
      tempSize -= (0x9a563422u >> (n * 4)) & 0xf;
    }
    if (tempSize != (4u >> (3 - DESCRIPTOR_Get_DictionaryId_Flag(descriptor))))
      return ZSTD2_STATE_FRAME_HEADER;
    
    info->descriptor_OR     = (Byte)(info->descriptor_OR     |  descriptor);
    info->descriptor_NOT_OR = (Byte)(info->descriptor_NOT_OR | ~descriptor);

    h = &p->temp[1];
    {
      Byte w = 0;
      if ((descriptor & DESCRIPTOR_FLAG_SINGLE) == 0)
      {
        w = *h++;
        if (info->windowDescriptor_MAX < w)
            info->windowDescriptor_MAX = w;
        // info->are_WindowDescriptors = True;
        // info->num_WindowDescriptors++;
      }
      else
      {
        // info->are_SingleSegments = True;
        // info->num_SingleSegments++;
      }
      p->windowDescriptor = w;
    }
    {
      unsigned n = DESCRIPTOR_Get_DictionaryId_Flag(descriptor);
      UInt32 d = 0;
      if (n)
      {
        n = 1u << (n - 1);
        d = GetUi32(h) & ((UInt32)(Int32)-1 >> (32 - 8u * n));
        h += n;
      }
      p->dictionaryId = d;
      // info->dictionaryId_Cur = d;
      if (d != 0)
      {
        if (info->dictionaryId == 0)
          info->dictionaryId = d;
        else if (info->dictionaryId != d)
          info->are_DictionaryId_Different = True;
      }
    }
    {
      unsigned n = DESCRIPTOR_Get_ContentSize_Flag3(descriptor);
      UInt64 v = 0;
      if (n)
      {
        n >>= 1;
        if (n == 1)
          v = 256;
        v += GetUi64(h) & ((UInt64)(Int64)-1 >> (64 - (8u << n)));
        // info->are_ContentSize_Known = True;
        // info->num_Frames_with_ContentSize++;
        if (info->contentSize_MAX < v)
            info->contentSize_MAX = v;
        info->contentSize_Total += v;
      }
      else
      {
        info->are_ContentSize_Unknown = True;
        // info->num_Frames_without_ContentSize++;
      }
      p->contentSize = v;
    }
    // if ((size_t)(h - p->temp) != headerSize) return ZSTD2_STATE_ERROR; // it's unexpected internal code failure
    p->tempSize = 0;

    info->checksum_Defined = False;
    /*
    if (descriptor & DESCRIPTOR_FLAG_CHECKSUM)
      info->are_Checksums = True;
    else
      info->are_Non_Checksums = True;
    */

    return ZSTD2_STATE_AFTER_HEADER; // ZSTD2_STATE_BLOCK;
  }
}


static void ZstdDec_Update_XXH(CZstdDec * const p, size_t xxh64_winPos)
{
 /*
 #ifdef DISABLE_XXH_CHECK
  UNUSED_VAR(data)
 #else
 */
  if (!p->disableHash && (p->descriptor & DESCRIPTOR_FLAG_CHECKSUM))
  {
    // const size_t pos = p->xxh64_winPos;
    const size_t size = (p->decoder.winPos - xxh64_winPos) & ~(size_t)31;
    if (size)
    {
      // p->xxh64_winPos = pos + size;
      Xxh64State_UpdateBlocks(&p->xxh64,
          p->decoder.win + xxh64_winPos,
          p->decoder.win + xxh64_winPos + size);
    }
  }
}


/*
in:
  (winLimit) : is relaxed limit, where this function is allowed to stop writing of decoded data (if possible).
    - this function uses (winLimit) for RAW/RLE blocks only,
        because this function can decode single RAW/RLE block in several different calls.
    - this function DOESN'T use (winLimit) for Compressed blocks,
        because this function decodes full compressed block in single call.
  (CZstdDec1::winPos <= winLimit)
  (winLimit <= CZstdDec1::cycSize).
  Note: if (ds->outBuf_fromCaller) mode is used, then
  {
    (strong_limit) is stored in CZstdDec1::cycSize.
    So (winLimit) is more strong than (strong_limit).
  }

exit:
  Note: (CZstdDecState::winPos) will be set by caller after exit of this function.

  This function can exit for any of these conditions:
    - (frameState == ZSTD2_STATE_AFTER_HEADER)
    - (frameState == ZSTD2_STATE_FINISHED) : frame was finished : (status == ZSTD_STATUS_FINISHED_FRAME) is set
    - finished non-empty non-last block. So (CZstdDec1::winPos_atExit != winPos_atFuncStart).
    - ZSTD_STATUS_NEEDS_MORE_INPUT in src
    - (CZstdDec1::winPos) have reached (winLimit) in non-finished RAW/RLE block

  This function decodes no more than one non-empty block.
  So it fulfills the condition at exit:
    (CZstdDec1::winPos_atExit - winPos_atFuncStart <= block_size_max)
  Note: (winPos_atExit > winLimit) is possible in some cases after compressed block decoding.
      
  if (ds->outBuf_fromCaller) mode (useAdditionalWinLimit medo)
  {
    then this function uses additional strong limit from (CZstdDec1::cycSize).
    So this function will not write any data after (CZstdDec1::cycSize)
    And it fulfills the condition at exit:
      (CZstdDec1::winPos_atExit <= CZstdDec1::cycSize)
  }
*/
static SRes ZstdDec_DecodeBlock(CZstdDec * const p, CZstdDecState * const ds,
    SizeT winLimitAdd)
{
  const Byte *src = ds->inBuf;
  SizeT * const srcLen = &ds->inPos;
  const SizeT inSize = ds->inLim;
  // const int useAdditionalWinLimit = ds->outBuf_fromCaller ? 1 : 0;
  enum_ZstdStatus * const status = &ds->status;
  CZstdDecInfo * const info = &ds->info;
  SizeT winLimit;

  const SizeT winPos_atFuncStart = p->decoder.winPos;
  src += *srcLen;
  *status = ZSTD_STATUS_NOT_SPECIFIED;

  // finishMode = ZSTD_FINISH_ANY;
  if (ds->outSize_Defined)
  {
    if (ds->outSize < ds->outProcessed)
    {
      // p->isAfterSizeMode = 2; // we have extra bytes already
      *status = ZSTD_STATUS_OUT_REACHED;
      return SZ_OK;
      // size = 0;
    }
    else
    {
      // p->outSize >= p->outProcessed
      const UInt64 rem = ds->outSize - ds->outProcessed;
      /*
      if (rem == 0)
      p->isAfterSizeMode = 1; // we have reached exact required size
      */
      if (winLimitAdd >= rem)
      {
        winLimitAdd = (SizeT)rem;
        // if (p->finishMode) finishMode = ZSTD_FINISH_END;
      }
    }
  }

  winLimit = p->decoder.winPos + winLimitAdd;
  // (p->decoder.winPos <= winLimit)

  // while (p->frameState != ZSTD2_STATE_ERROR)
  while (!p->isErrorState)
  {
    SizeT inCur = inSize - *srcLen;

    if (p->frameState == ZSTD2_STATE_DATA)
    {
      /* (p->decoder.winPos == winPos_atFuncStart) is expected,
         because this function doesn't start new block.
         if it have finished some non-empty block in this call. */
      if (p->decoder.winPos != winPos_atFuncStart)
        return SZ_ERROR_FAIL; // it's unexpected

      /*
      if (p->decoder.winPos > winLimit)
      {
        // we can be here, if in this function call
        //      - we have extracted non-empty compressed block, and (winPos > winLimit) after that.
        //      - we have started new block decoding after that.
        // It's unexpected case, because we exit after non-empty non-last block.
        *status = (inSize == *srcLen) ?
            ZSTD_STATUS_NEEDS_MORE_INPUT :
            ZSTD_STATUS_NOT_FINISHED;
        return SZ_OK;
      }
      */
      // p->decoder.winPos <= winLimit
      
      if (p->blockType != kBlockType_Compressed)
      {
        // it's RLE or RAW block.
        // p->BlockSize != 0_
        // winLimit <= p->decoder.cycSize
        /* So here we use more strong (winLimit), even for
           (ds->outBuf_fromCaller) mode. */
        SizeT outCur = winLimit - p->decoder.winPos;
        {
          const UInt32 rem = p->blockSize;
          if (outCur > rem)
              outCur = rem;
        }
        if (p->blockType == kBlockType_Raw)
        {
          if (outCur > inCur)
              outCur = inCur;
          /* output buffer is better aligned for XXH code.
             So we use hash for output buffer data */
          // ZstdDec_Update_XXH(p, src, outCur); // for debug:
          memcpy(p->decoder.win + p->decoder.winPos, src, outCur);
          src += outCur;
          *srcLen += outCur;
        }
        else // kBlockType_RLE
        {
          #define RLE_BYTE_INDEX_IN_temp  3
          memset(p->decoder.win + p->decoder.winPos,
              p->temp[RLE_BYTE_INDEX_IN_temp], outCur);
        }
        {
          const SizeT xxh64_winPos = p->decoder.winPos - ZstdDec_GET_UNPROCESSED_XXH64_SIZE(p);
          p->decoder.winPos += outCur;
          p->contentProcessed += outCur;
          ZstdDec_Update_XXH(p, xxh64_winPos);
        }
        // ds->winPos = p->decoder.winPos;  // the caller does it instead. for debug:
        UPDATE_TOTAL_OUT(&p->decoder, outCur)
        ds->outProcessed += outCur;
        if (p->blockSize -= (UInt32)outCur)
        {
          /*
          if (ds->outSize_Defined)
          {
            if (ds->outSize <= ds->outProcessed) ds->isAfterSizeMode = (enum_ZstdStatus)
               (ds->outSize == ds->outProcessed ? 1u: 2u);
          }
          */
          *status = (enum_ZstdStatus)
              (ds->outSize_Defined && ds->outSize <= ds->outProcessed ?
                ZSTD_STATUS_OUT_REACHED : (p->blockType == kBlockType_Raw && inSize == *srcLen) ?
                ZSTD_STATUS_NEEDS_MORE_INPUT :
                ZSTD_STATUS_NOT_FINISHED);
          return SZ_OK;
        }
      }
      else // kBlockType_Compressed
      {
        // p->blockSize != 0
        // (uncompressed_size_of_block == 0) is allowed
        // (p->curBlockUnpackRem == 0) is allowed
        /*
        if (p->decoder.winPos >= winLimit)
        {
          if (p->decoder.winPos != winPos_atFuncStart)
          {
            // it's unexpected case
            // We already have some data in finished blocks in this function call.
            //   So we don't decompress new block after (>=winLimit),
            //   even if it's empty block.
            *status = (inSize == *srcLen) ?
                ZSTD_STATUS_NEEDS_MORE_INPUT :
                ZSTD_STATUS_NOT_FINISHED;
            return SZ_OK;
          }
          // (p->decoder.winPos == winLimit == winPos_atFuncStart)
          // we will decode current block, because that current
          //   block can be empty block and we want to make some visible
          //   change of (src) stream after function start.
        }
        */
        /*
        if (ds->outSize_Defined && ds->outSize < ds->outProcessed)
        {
          // we don't want to start new block, if we have more extra decoded bytes already
          *status = ZSTD_STATUS_OUT_REACHED;
          return SZ_OK;
        }
        */
        {
          const Byte *comprStream;
          size_t afterAvail;
          UInt32 inTempPos = p->inTempPos;
          const UInt32 rem = p->blockSize - inTempPos;
          // rem != 0
          if (inTempPos != 0  // (inTemp) buffer already contains some input data
              || inCur < rem  // available input data size is smaller than compressed block size
              || ZstdDec1_NeedTempBufferForInput(*srcLen, src, rem))
          {
            if (inCur > rem)
                inCur = rem;
            if (inCur)
            {
              STAT_INC(g_Num_Blocks_memcpy)
              // we clear data for backward lookahead reading
              if (inTempPos == 0)
                memset(p->inTemp + kTempBuffer_PreSize - MAX_BACKWARD_DEPTH, 0, MAX_BACKWARD_DEPTH);
              // { unsigned y = 0; for(;y < 1000; y++)
              memcpy(p->inTemp + inTempPos + kTempBuffer_PreSize, src, inCur);
              // }
              src += inCur;
              *srcLen += inCur;
              inTempPos += (UInt32)inCur;
              p->inTempPos = inTempPos;
            }
            if (inTempPos != p->blockSize)
            {
              *status = ZSTD_STATUS_NEEDS_MORE_INPUT;
              return SZ_OK;
            }
            #if COPY_CHUNK_SIZE > 1
              memset(p->inTemp + kTempBuffer_PreSize + inTempPos, 0, COPY_CHUNK_SIZE);
            #endif
            comprStream = p->inTemp + kTempBuffer_PreSize;
            afterAvail = k_Lit_AfterAvail;
            // we don't want to read non-initialized data or junk in CopyMatch():
          }
          else
          {
            // inCur >= rem
            // we use direct decoding from (src) buffer:
            afterAvail = inCur - rem;
            comprStream = src;
            src += rem;
            *srcLen += rem;
          }

          #ifdef Z7_ZSTD_DEC_USE_CHECK_OF_NEED_TEMP
            ZstdDec1_NeedTempBufferForInput(*srcLen, comprStream, p->blockSize);
          #endif
          // printf("\nblockSize=%u", p->blockSize);
          // printf("%x\n", (unsigned)p->contentProcessed);
          STAT_INC(g_Num_Blocks_Compressed)
          {
            SRes sres;
            const size_t winPos = p->decoder.winPos;
            /*
               if ( useAdditionalWinLimit), we use strong unpack limit: smallest from
                  - limit from stream : (curBlockUnpackRem)
                  - limit from caller : (cycSize - winPos)
               if (!useAdditionalWinLimit), we use only relaxed limit:
                  - limit from stream : (curBlockUnpackRem)
            */
            SizeT outLimit = p->curBlockUnpackRem;
            if (ds->outBuf_fromCaller)
            // if (useAdditionalWinLimit)
            {
              const size_t limit = p->decoder.cycSize - winPos;
              if (outLimit > limit)
                  outLimit = limit;
            }
            sres = ZstdDec1_DecodeBlock(&p->decoder,
                comprStream, p->blockSize, afterAvail, outLimit);
            // ds->winPos = p->decoder.winPos;  // the caller does it instead. for debug:
            if (sres)
            {
              p->isErrorState = True;
              return sres;
            }
            {
              const SizeT xxh64_winPos = winPos - ZstdDec_GET_UNPROCESSED_XXH64_SIZE(p);
              const size_t num = p->decoder.winPos - winPos;
              ds->outProcessed += num;
              p->contentProcessed += num;
              ZstdDec_Update_XXH(p, xxh64_winPos);
            }
          }
          // printf("\nwinPos=%x", (int)(unsigned)p->decoder.winPos);
        }
      }

      /*
      if (ds->outSize_Defined)
      {
        if (ds->outSize <= ds->outProcessed) ds->isAfterSizeMode = (enum_ZstdStatus)
           (ds->outSize == ds->outProcessed ? 1u: 2u);
      }
      */
      
      if (!ZSTD_DEC_IS_LAST_BLOCK(p))
      {
        p->frameState = ZSTD2_STATE_BLOCK;
        if (ds->outSize_Defined && ds->outSize < ds->outProcessed)
        {
          *status = ZSTD_STATUS_OUT_REACHED;
          return SZ_OK;
        }
        // we exit only if (winPos) was changed in this function call:
        if (p->decoder.winPos != winPos_atFuncStart)
        {
          // decoded block was not empty. So we exit:
          *status = (enum_ZstdStatus)(
              (inSize == *srcLen) ?
                ZSTD_STATUS_NEEDS_MORE_INPUT :
                ZSTD_STATUS_NOT_FINISHED);
          return SZ_OK;
        }
        // (p->decoder.winPos == winPos_atFuncStart)
        // so current decoded block was empty.
        // we will try to decode more blocks in this function.
        continue;
      }
      
      // decoded block was last in frame
      if (p->descriptor & DESCRIPTOR_FLAG_CHECKSUM)
      {
        p->frameState = ZSTD2_STATE_HASH;
        if (ds->outSize_Defined && ds->outSize < ds->outProcessed)
        {
          *status = ZSTD_STATUS_OUT_REACHED;
          return SZ_OK; // disable if want to
          /* We want to get same return codes for any input buffer sizes.
             We want to get faster ZSTD_STATUS_OUT_REACHED status.
             So we exit with ZSTD_STATUS_OUT_REACHED here,
             instead of ZSTD2_STATE_HASH and ZSTD2_STATE_FINISHED processing.
             that depends from input buffer size and that can set
             ZSTD_STATUS_NEEDS_MORE_INPUT or return SZ_ERROR_DATA or SZ_ERROR_CRC.
          */
        }
      }
      else
      {
        /* ZSTD2_STATE_FINISHED proccesing doesn't depend from input buffer */
        p->frameState = ZSTD2_STATE_FINISHED;
      }
      /*
      p->frameState = (p->descriptor & DESCRIPTOR_FLAG_CHECKSUM) ?
          ZSTD2_STATE_HASH :
          ZSTD2_STATE_FINISHED;
      */
      /* it's required to process ZSTD2_STATE_FINISHED state in this function call,
         because we must check contentSize and hashError in ZSTD2_STATE_FINISHED code,
         while the caller can reinit full state for ZSTD2_STATE_FINISHED
         So we can't exit from function here. */
      continue;
    }

    if (p->frameState == ZSTD2_STATE_FINISHED)
    {
      *status = ZSTD_STATUS_FINISHED_FRAME;
      if (DESCRIPTOR_Is_ContentSize_Defined(p->descriptor)
          && p->contentSize != p->contentProcessed)
        return SZ_ERROR_DATA;
      if (p->hashError) // for debug
        return SZ_ERROR_CRC;
      return SZ_OK;
      // p->frameState = ZSTD2_STATE_SIGNATURE;
      // continue;
    }
    
    if (p->frameState == ZSTD2_STATE_AFTER_HEADER)
      return SZ_OK; // we need memory allocation for that state

    if (p->frameState == ZSTD2_STATE_SKIP_DATA)
    {
      UInt32 blockSize = p->blockSize;
      // (blockSize == 0) is possible
      if (inCur > blockSize)
          inCur = blockSize;
      src += inCur;
      *srcLen += inCur;
      blockSize -= (UInt32)inCur;
      p->blockSize = blockSize;
      if (blockSize == 0)
      {
        p->frameState = ZSTD2_STATE_SIGNATURE;
        // continue; // for debug: we can continue without return to caller.
        // we notify the caller that skip frame was finished:
        *status = ZSTD_STATUS_FINISHED_FRAME;
        return SZ_OK;
      }
      // blockSize != 0
      // (inCur) was smaller than previous value of p->blockSize.
      // (inSize == *srcLen) now
      *status = ZSTD_STATUS_NEEDS_MORE_INPUT;
      return SZ_OK;
    }

    if (inCur == 0)
    {
      *status = ZSTD_STATUS_NEEDS_MORE_INPUT;
      return SZ_OK;
    }

    {
      (*srcLen)++;
      p->frameState = ZstdDec_UpdateState(p, *src++, info);
    }
  }
  
  *status = ZSTD_STATUS_NOT_SPECIFIED;
  p->isErrorState = True;
  // p->frameState = ZSTD2_STATE_ERROR;
  // if (p->frameState = ZSTD2_STATE_SIGNATURE)  return SZ_ERROR_NO_ARCHIVE
  return SZ_ERROR_DATA;
}




SRes ZstdDec_Decode(CZstdDecHandle dec, CZstdDecState *p)
{
  p->needWrite_Size = 0;
  p->status = ZSTD_STATUS_NOT_SPECIFIED;
  dec->disableHash = p->disableHash;

  if (p->outBuf_fromCaller)
  {
    dec->decoder.win = p->outBuf_fromCaller;
    dec->decoder.cycSize = p->outBufSize_fromCaller;
  }

  // p->winPos = dec->decoder.winPos;

  for (;;)
  {
    SizeT winPos, size;
    // SizeT outProcessed;
    SRes res;

    if (p->wrPos > dec->decoder.winPos)
      return SZ_ERROR_FAIL;

    if (dec->frameState == ZSTD2_STATE_FINISHED)
    {
      if (!p->outBuf_fromCaller)
      {
        // we need to set positions to zero for new frame.
        if (p->wrPos != dec->decoder.winPos)
        {
          /* We have already asked the caller to flush all data
             with (p->needWrite_Size) and (ZSTD_STATUS_FINISHED_FRAME) status.
             So it's unexpected case */
          // p->winPos = dec->decoder.winPos;
          // p->needWrite_Size = dec->decoder.winPos - p->wrPos; // flush size asking
          // return SZ_OK; // ask to flush again
          return SZ_ERROR_FAIL;
        }
        // (p->wrPos == dec->decoder.winPos), and we wrap to zero:
        dec->decoder.winPos = 0;
        p->winPos = 0;
        p->wrPos = 0;
      }
      ZstdDec_Init_ForNewFrame(dec);
      // continue;
    }

    winPos = dec->decoder.winPos;
    {
      SizeT next = dec->decoder.cycSize;
      /* cycSize == 0, if no buffer was allocated still,
         or, if (outBuf_fromCaller) mode and (outBufSize_fromCaller == 0) */
      if (!p->outBuf_fromCaller
          && next
          && next <= winPos
          && dec->isCyclicMode)
      {
        // (0 < decoder.cycSize <= winPos) in isCyclicMode.
        // so we need to wrap (winPos) and (wrPos) over (cycSize).
        const size_t delta = next;
        // (delta) is how many bytes we remove from buffer.
        /*
        // we don't need data older than last (cycSize) bytes.
        size_t delta = winPos - next; // num bytes after (cycSize)
        if (delta <= next) // it's expected case
          delta = next;
        // delta == Max(cycSize, winPos - cycSize)
        */
        if (p->wrPos < delta)
        {
          // (wrPos < decoder.cycSize)
          // We have asked already the caller to flush required data
          // p->status = ZSTD_STATUS_NOT_SPECIFIED;
          // p->winPos = winPos;
          // p->needWrite_Size = delta - p->wrPos; // flush size asking
          // return SZ_OK; // ask to flush again
          return SZ_ERROR_FAIL;
        }
        // p->wrPos >= decoder.cycSize
        // we move extra data after (decoder.cycSize) to start of cyclic buffer:
        winPos -= delta;
        if (winPos)
        {
          if (winPos >= delta)
            return SZ_ERROR_FAIL;
          memmove(dec->decoder.win, dec->decoder.win + delta, winPos);
          // printf("\nmemmove processed=%8x winPos=%8x\n", (unsigned)p->outProcessed, (unsigned)dec->decoder.winPos);
          STAT_INC(g_Num_Wrap_memmove_Num)
          STAT_UPDATE(g_Num_Wrap_memmove_Bytes += (unsigned)winPos;)
        }
        dec->decoder.winPos = winPos;
        p->winPos = winPos;
        p->wrPos -= delta;
        // dec->xxh64_winPos -= delta;

        // (winPos < delta)
        #ifdef Z7_STD_DEC_USE_AFTER_CYC_BUF
          /* we set the data after cycSize, because
             we don't want to read non-initialized data or junk in CopyMatch(). */
          memset(dec->decoder.win + next, 0, COPY_CHUNK_SIZE);
        #endif

        /*
        if (winPos == next)
        {
          if (winPos != p->wrPos)
          {
            // we already requested before to flush full data for that case.
            //   but we give the caller a second chance to flush data:
            p->needWrite_Size = winPos - p->wrPos;
            return SZ_OK;
          }
          // (decoder.cycSize == winPos == p->wrPos)
          // so we do second wrapping to zero:
          winPos = 0;
          dec->decoder.winPos = 0;
          p->winPos = 0;
          p->wrPos = 0;
        }
        */
        // (winPos < next)
      }

      if (winPos > next)
        return SZ_ERROR_FAIL; // it's unexpected case
      /*
        if (!outBuf_fromCaller && isCyclicMode && cycSize != 0)
          then (winPos <  cycSize)
          else (winPos <= cycSize)
      */
      if (!p->outBuf_fromCaller)
      {
        // that code is optional. We try to optimize write chunk sizes.
        /* (next2) is expected next write position in the caller,
           if the caller writes by kBlockSizeMax chunks.
        */
        /*
        const size_t next2 = (winPos + kBlockSizeMax) & (kBlockSizeMax - 1);
        if (winPos < next2 && next2 < next)
          next = next2;
        */
      }
      size = next - winPos;
    }

    // note: ZstdDec_DecodeBlock() uses (winLimit = winPos + size) only for RLE and RAW blocks
    res = ZstdDec_DecodeBlock(dec, p, size);
    /*
      after one block decoding:
      if (!outBuf_fromCaller && isCyclicMode && cycSize != 0)
        then (winPos <  cycSize + max_block_size)
        else (winPos <= cycSize)
    */

    if (!p->outBuf_fromCaller)
      p->win = dec->decoder.win;
    p->winPos = dec->decoder.winPos;

    // outProcessed = dec->decoder.winPos - winPos;
    // p->outProcessed += outProcessed;

    if (res != SZ_OK)
      return res;

    if (dec->frameState != ZSTD2_STATE_AFTER_HEADER)
    {
      if (p->outBuf_fromCaller)
        return SZ_OK;
      {
        // !p->outBuf_fromCaller
        /*
          if (ZSTD_STATUS_FINISHED_FRAME), we request full flushing here because
            1) it's simpler to work with allocation and extracting of next frame,
            2) it's better to start writing to next new frame with aligned memory
               for faster xxh 64-bit reads.
        */
        size_t end = dec->decoder.winPos;  // end pos for all data flushing
        if (p->status != ZSTD_STATUS_FINISHED_FRAME)
        {
          // we will request flush here only for cases when wrap in cyclic buffer can be required in next call.
          if (!dec->isCyclicMode)
            return SZ_OK;
          // isCyclicMode
          {
            const size_t delta = dec->decoder.cycSize;
            if (end < delta)
              return SZ_OK; // (winPos < cycSize). no need for flush
            // cycSize <= winPos
            // So we ask the caller to flush of (cycSize - wrPos) bytes,
            // and then we will wrap cylicBuffer in next call
            end = delta;
          }
        }
        p->needWrite_Size = end - p->wrPos;
      }
      return SZ_OK;
    }

    // ZSTD2_STATE_AFTER_HEADER
    {
      BoolInt useCyclic = False;
      size_t cycSize;

      // p->status = ZSTD_STATUS_NOT_FINISHED;
      if (dec->dictionaryId != 0)
      {
        /* actually we can try to decode some data,
           because it's possible that some data doesn't use dictionary */
        // p->status = ZSTD_STATUS_NOT_SPECIFIED;
        return SZ_ERROR_UNSUPPORTED;
      }

      {
        UInt64 winSize = dec->contentSize;
        UInt64 winSize_Allocate = winSize;
        const unsigned descriptor = dec->descriptor;
        
        if ((descriptor & DESCRIPTOR_FLAG_SINGLE) == 0)
        {
          const Byte wd = dec->windowDescriptor;
          winSize = (UInt64)(8 + (wd & 7)) << ((wd >> 3) + 10 - 3);
          if (!DESCRIPTOR_Is_ContentSize_Defined(descriptor)
              || winSize_Allocate > winSize)
          {
            winSize_Allocate = winSize;
            useCyclic = True;
          }
        }
        /*
        else
        {
          if (p->info.singleSegment_ContentSize_MAX < winSize)
              p->info.singleSegment_ContentSize_MAX = winSize;
          // p->info.num_SingleSegments++;
        }
        */
        if (p->info.windowSize_MAX < winSize)
            p->info.windowSize_MAX = winSize;
        if (p->info.windowSize_Allocate_MAX < winSize_Allocate)
            p->info.windowSize_Allocate_MAX = winSize_Allocate;
        /*
           winSize_Allocate is MIN(content_size, window_size_from_descriptor).
           Wven if (content_size < (window_size_from_descriptor))
             original-zstd still uses (window_size_from_descriptor) to check that decoding is allowed.
           We try to follow original-zstd, and here we check (winSize) instead of (winSize_Allocate))
        */
        if (
              // winSize_Allocate   // it's relaxed check
              winSize               // it's more strict check to be compatible with original-zstd
            > ((UInt64)1 << MAX_WINDOW_SIZE_LOG))
          return SZ_ERROR_UNSUPPORTED; // SZ_ERROR_MEM
        cycSize = (size_t)winSize_Allocate;
        if (cycSize != winSize_Allocate)
          return SZ_ERROR_MEM;
        // cycSize <= winSize
        /* later we will use (CZstdDec1::winSize) to check match offsets and check block sizes.
           if (there is window descriptor)
           {
             We will check block size with (window_size_from_descriptor) instead of (winSize_Allocate).
             Does original-zstd do it that way also?
           }
           Here we must reduce full real 64-bit (winSize) to size_t for (CZstdDec1::winSize).
           Also we don't want too big values for (CZstdDec1::winSize).
           our (CZstdDec1::winSize) will meet the condition:
             (CZstdDec1::winSize < kBlockSizeMax || CZstdDec1::winSize <= cycSize).
        */
        dec->decoder.winSize = (winSize < kBlockSizeMax) ? (size_t)winSize: cycSize;
        // note: (CZstdDec1::winSize > cycSize) is possible, if (!useCyclic)
      }

      RINOK(ZstdDec_AllocateMisc(dec))

      if (p->outBuf_fromCaller)
        dec->isCyclicMode = False;
      else
      {
        size_t d = cycSize;

        if (dec->decoder.winPos != p->wrPos)
          return SZ_ERROR_FAIL;

        dec->decoder.winPos = 0;
        p->wrPos = 0;
        p->winPos = dec->decoder.winPos;

        /*
        const size_t needWrite = dec->decoder.winPos - p->wrPos;
        if (!needWrite)
        {
          dec->decoder.winPos = 0;
          p->wrPos = 0;
          p->winPos = dec->decoder.winPos;
        }
        */
        /* if (!useCyclic) we allocate only cycSize = ContentSize.
           But if we want to support the case where new frame starts with winPos != 0,
           then we will wrap over zero, and we still need
           to set (useCyclic) and allocate additional buffer spaces.
           Now we don't allow new frame starting with (winPos != 0).
           so (dec->decoder->winPos == 0)
           can use (!useCyclic) with reduced buffer sizes.
        */
        /*
        if (dec->decoder->winPos != 0)
          useCyclic = True;
        */

        if (useCyclic)
        {
          /* cyclyc buffer size must be at least (COPY_CHUNK_SIZE - 1) bytes
             larger than window size, because CopyMatch() can write additional
             (COPY_CHUNK_SIZE - 1) bytes and overwrite oldests data in cyclyc buffer.
             But for performance reasons we align (cycSize) for (kBlockSizeMax).
             also we must provide (cycSize >= max_decoded_data_after_cycSize),
             because after data move wrapping over zero we must provide (winPos < cycSize).
          */
          const size_t alignSize = kBlockSizeMax;
          /* here we add (1 << 7) instead of (COPY_CHUNK_SIZE - 1), because
             we want to get same (cycSize) for different COPY_CHUNK_SIZE values. */
          // cycSize += (COPY_CHUNK_SIZE - 1) + (alignSize - 1); // for debug : we can get smallest (cycSize)
          cycSize += (1 << 7) + alignSize;
          cycSize &= ~(size_t)(alignSize - 1);
          // cycSize must be aligned for 32, because xxh requires 32-bytes blocks.
          // cycSize += 12345; // for debug
          // cycSize += 1 << 10; // for debug
          // cycSize += 32; // for debug
          // cycSize += kBlockSizeMax; // for debug
          if (cycSize < d)
            return SZ_ERROR_MEM;
          /*
             in cyclic buffer mode we allow to decode one additional block
             that exceeds (cycSize).
             So we must allocate additional (kBlockSizeMax) bytes after (cycSize).
             if defined(Z7_STD_DEC_USE_AFTER_CYC_BUF)
             {
               we can read (COPY_CHUNK_SIZE - 1) bytes after (cycSize)
               but we aready allocate additional kBlockSizeMax that
               is larger than COPY_CHUNK_SIZE.
               So we don't need additional space of COPY_CHUNK_SIZE after (cycSize).
             }
          */
          /*
          #ifdef Z7_STD_DEC_USE_AFTER_CYC_BUF
          d = cycSize + (1 << 7); // we must add at least (COPY_CHUNK_SIZE - 1)
          #endif
          */
          d = cycSize + kBlockSizeMax;
          if (d < cycSize)
            return SZ_ERROR_MEM;
        }

        {
          const size_t kMinWinAllocSize = 1 << 12;
          if (d < kMinWinAllocSize)
              d = kMinWinAllocSize;
        }

        if (d > dec->winBufSize_Allocated)
        {
          /*
          if (needWrite)
          {
            p->needWrite_Size = needWrite;
            return SZ_OK;
            // return SZ_ERROR_FAIL;
          }
          */

          if (dec->winBufSize_Allocated != 0)
          {
            const size_t k_extra = (useCyclic || d >= (1u << 20)) ?
                2 * kBlockSizeMax : 0;
            unsigned i = useCyclic ? 17 : 12;
            for (; i < sizeof(size_t) * 8; i++)
            {
              const size_t d2 = ((size_t)1 << i) + k_extra;
              if (d2 >= d)
              {
                d = d2;
                break;
              }
            }
          }
          // RINOK(ZstdDec_AllocateWindow(dec, d))
          ZstdDec_FreeWindow(dec);
          dec->win_Base = (Byte *)ISzAlloc_Alloc(dec->alloc_Big, d);
          if (!dec->win_Base)
            return SZ_ERROR_MEM;
          dec->decoder.win = dec->win_Base;
          dec->winBufSize_Allocated = d;
        }
        /*
        else
        {
          // for non-cyclycMode we want flush data, and set winPos = 0
          if (needWrite)
          {
            if (!useCyclic || dec->decoder.winPos >= cycSize)
            {
              p->needWrite_Size = needWrite;
              return SZ_OK;
              // return SZ_ERROR_FAIL;
            }
          }
        }
        */

        dec->decoder.cycSize = cycSize;
        p->win = dec->decoder.win;
        // p->cycSize = dec->decoder.cycSize;
        dec->isCyclicMode = (Byte)useCyclic;
      } // (!p->outBuf_fromCaller) end
      
      // p->winPos = dec->decoder.winPos;
      dec->frameState = ZSTD2_STATE_BLOCK;
      // continue;
    } // ZSTD2_STATE_AFTER_HEADER end
  }
}


void ZstdDec_GetResInfo(const CZstdDec *dec,
    const CZstdDecState *p,
    SRes res,
    CZstdDecResInfo *stat)
{
  // ZstdDecInfo_CLEAR(stat);
  stat->extraSize = 0;
  stat->is_NonFinishedFrame = False;
  if (dec->frameState != ZSTD2_STATE_FINISHED)
  {
    if (dec->frameState == ZSTD2_STATE_SIGNATURE)
    {
      stat->extraSize = (Byte)dec->tempSize;
      if (ZstdDecInfo_GET_NUM_FRAMES(&p->info) == 0)
        res = SZ_ERROR_NO_ARCHIVE;
    }
    else
    {
      stat->is_NonFinishedFrame = True;
      if (res == SZ_OK && p->status == ZSTD_STATUS_NEEDS_MORE_INPUT)
        res = SZ_ERROR_INPUT_EOF;
    }
  }
  stat->decode_SRes = res;
}


size_t ZstdDec_ReadUnusedFromInBuf(
    CZstdDecHandle dec,
    size_t afterDecoding_tempPos,
    void *data, size_t size)
{
  size_t processed = 0;
  if (dec->frameState == ZSTD2_STATE_SIGNATURE)
  {
    Byte *dest = (Byte *)data;
    const size_t tempSize = dec->tempSize;
    while (afterDecoding_tempPos < tempSize)
    {
      if (size == 0)
        break;
      size--;
      *dest++ = dec->temp[afterDecoding_tempPos++];
      processed++;
    }
  }
  return processed;
}


void ZstdDecState_Clear(CZstdDecState *p)
{
  memset(p, 0 , sizeof(*p));
}
