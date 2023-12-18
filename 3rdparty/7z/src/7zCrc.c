/* 7zCrc.c -- CRC32 calculation and init
2023-04-02 : Igor Pavlov : Public domain */

#include "Precomp.h"

#include "7zCrc.h"
#include "CpuArch.h"

#define kCrcPoly 0xEDB88320

#ifdef MY_CPU_LE
  #define CRC_NUM_TABLES 8
#else
  #define CRC_NUM_TABLES 9

  UInt32 Z7_FASTCALL CrcUpdateT1_BeT4(UInt32 v, const void *data, size_t size, const UInt32 *table);
  UInt32 Z7_FASTCALL CrcUpdateT1_BeT8(UInt32 v, const void *data, size_t size, const UInt32 *table);
#endif

#ifndef MY_CPU_BE
  UInt32 Z7_FASTCALL CrcUpdateT4(UInt32 v, const void *data, size_t size, const UInt32 *table);
  UInt32 Z7_FASTCALL CrcUpdateT8(UInt32 v, const void *data, size_t size, const UInt32 *table);
#endif

/*
extern
CRC_FUNC g_CrcUpdateT4;
CRC_FUNC g_CrcUpdateT4;
*/
extern
CRC_FUNC g_CrcUpdateT8;
CRC_FUNC g_CrcUpdateT8;
extern
CRC_FUNC g_CrcUpdateT0_32;
CRC_FUNC g_CrcUpdateT0_32;
extern
CRC_FUNC g_CrcUpdateT0_64;
CRC_FUNC g_CrcUpdateT0_64;
extern
CRC_FUNC g_CrcUpdate;
CRC_FUNC g_CrcUpdate;

UInt32 g_CrcTable[256 * CRC_NUM_TABLES];

UInt32 Z7_FASTCALL CrcUpdate(UInt32 v, const void *data, size_t size)
{
  return g_CrcUpdate(v, data, size, g_CrcTable);
}

UInt32 Z7_FASTCALL CrcCalc(const void *data, size_t size)
{
  return g_CrcUpdate(CRC_INIT_VAL, data, size, g_CrcTable) ^ CRC_INIT_VAL;
}

#if CRC_NUM_TABLES < 4 \
   || (CRC_NUM_TABLES == 4 && defined(MY_CPU_BE)) \
   || (!defined(MY_CPU_LE) && !defined(MY_CPU_BE))
#define CRC_UPDATE_BYTE_2(crc, b) (table[((crc) ^ (b)) & 0xFF] ^ ((crc) >> 8))
UInt32 Z7_FASTCALL CrcUpdateT1(UInt32 v, const void *data, size_t size, const UInt32 *table);
UInt32 Z7_FASTCALL CrcUpdateT1(UInt32 v, const void *data, size_t size, const UInt32 *table)
{
  const Byte *p = (const Byte *)data;
  const Byte *pEnd = p + size;
  for (; p != pEnd; p++)
    v = CRC_UPDATE_BYTE_2(v, *p);
  return v;
}
#endif

/* ---------- hardware CRC ---------- */

#ifdef MY_CPU_LE

#if defined(MY_CPU_ARM_OR_ARM64)

// #pragma message("ARM*")

  #if defined(_MSC_VER)
    #if defined(MY_CPU_ARM64)
    #if (_MSC_VER >= 1910)
    #ifndef __clang__
        #define USE_ARM64_CRC
        #include <intrin.h>
    #endif
    #endif
    #endif
  #elif (defined(__clang__) && (__clang_major__ >= 3)) \
     || (defined(__GNUC__) && (__GNUC__ > 4))
      #if !defined(__ARM_FEATURE_CRC32)
        #define __ARM_FEATURE_CRC32 1
        #if defined(__clang__)
          #if defined(MY_CPU_ARM64)
            #define ATTRIB_CRC __attribute__((__target__("crc")))
          #else
            #define ATTRIB_CRC __attribute__((__target__("armv8-a,crc")))
          #endif
        #else
          #if defined(MY_CPU_ARM64)
            #define ATTRIB_CRC __attribute__((__target__("+crc")))
          #else
            #define ATTRIB_CRC __attribute__((__target__("arch=armv8-a+crc")))
          #endif
        #endif
      #endif
      #if defined(__ARM_FEATURE_CRC32)
        #define USE_ARM64_CRC
        #include <arm_acle.h>
      #endif
  #endif

#else

// no hardware CRC

// #define USE_CRC_EMU

#ifdef USE_CRC_EMU

#pragma message("ARM64 CRC emulation")

Z7_FORCE_INLINE
UInt32 __crc32b(UInt32 v, UInt32 data)
{
  const UInt32 *table = g_CrcTable;
  v = CRC_UPDATE_BYTE_2(v, (Byte)data);
  return v;
}

Z7_FORCE_INLINE
UInt32 __crc32w(UInt32 v, UInt32 data)
{
  const UInt32 *table = g_CrcTable;
  v = CRC_UPDATE_BYTE_2(v, (Byte)data); data >>= 8;
  v = CRC_UPDATE_BYTE_2(v, (Byte)data); data >>= 8;
  v = CRC_UPDATE_BYTE_2(v, (Byte)data); data >>= 8;
  v = CRC_UPDATE_BYTE_2(v, (Byte)data); data >>= 8;
  return v;
}

Z7_FORCE_INLINE
UInt32 __crc32d(UInt32 v, UInt64 data)
{
  const UInt32 *table = g_CrcTable;
  v = CRC_UPDATE_BYTE_2(v, (Byte)data); data >>= 8;
  v = CRC_UPDATE_BYTE_2(v, (Byte)data); data >>= 8;
  v = CRC_UPDATE_BYTE_2(v, (Byte)data); data >>= 8;
  v = CRC_UPDATE_BYTE_2(v, (Byte)data); data >>= 8;
  v = CRC_UPDATE_BYTE_2(v, (Byte)data); data >>= 8;
  v = CRC_UPDATE_BYTE_2(v, (Byte)data); data >>= 8;
  v = CRC_UPDATE_BYTE_2(v, (Byte)data); data >>= 8;
  v = CRC_UPDATE_BYTE_2(v, (Byte)data); data >>= 8;
  return v;
}

#endif // USE_CRC_EMU

#endif // defined(MY_CPU_ARM64) && defined(MY_CPU_LE)



#if defined(USE_ARM64_CRC) || defined(USE_CRC_EMU)

#define T0_32_UNROLL_BYTES (4 * 4)
#define T0_64_UNROLL_BYTES (4 * 8)

#ifndef ATTRIB_CRC
#define ATTRIB_CRC
#endif
// #pragma message("USE ARM HW CRC")

ATTRIB_CRC
UInt32 Z7_FASTCALL CrcUpdateT0_32(UInt32 v, const void *data, size_t size, const UInt32 *table);
ATTRIB_CRC
UInt32 Z7_FASTCALL CrcUpdateT0_32(UInt32 v, const void *data, size_t size, const UInt32 *table)
{
  const Byte *p = (const Byte *)data;
  UNUSED_VAR(table);

  for (; size != 0 && ((unsigned)(ptrdiff_t)p & (T0_32_UNROLL_BYTES - 1)) != 0; size--)
    v = __crc32b(v, *p++);

  if (size >= T0_32_UNROLL_BYTES)
  {
    const Byte *lim = p + size;
    size &= (T0_32_UNROLL_BYTES - 1);
    lim -= size;
    do
    {
      v = __crc32w(v, *(const UInt32 *)(const void *)(p));
      v = __crc32w(v, *(const UInt32 *)(const void *)(p + 4)); p += 2 * 4;
      v = __crc32w(v, *(const UInt32 *)(const void *)(p));
      v = __crc32w(v, *(const UInt32 *)(const void *)(p + 4)); p += 2 * 4;
    }
    while (p != lim);
  }
  
  for (; size != 0; size--)
    v = __crc32b(v, *p++);

  return v;
}

ATTRIB_CRC
UInt32 Z7_FASTCALL CrcUpdateT0_64(UInt32 v, const void *data, size_t size, const UInt32 *table);
ATTRIB_CRC
UInt32 Z7_FASTCALL CrcUpdateT0_64(UInt32 v, const void *data, size_t size, const UInt32 *table)
{
  const Byte *p = (const Byte *)data;
  UNUSED_VAR(table);

  for (; size != 0 && ((unsigned)(ptrdiff_t)p & (T0_64_UNROLL_BYTES - 1)) != 0; size--)
    v = __crc32b(v, *p++);

  if (size >= T0_64_UNROLL_BYTES)
  {
    const Byte *lim = p + size;
    size &= (T0_64_UNROLL_BYTES - 1);
    lim -= size;
    do
    {
      v = __crc32d(v, *(const UInt64 *)(const void *)(p));
      v = __crc32d(v, *(const UInt64 *)(const void *)(p + 8)); p += 2 * 8;
      v = __crc32d(v, *(const UInt64 *)(const void *)(p));
      v = __crc32d(v, *(const UInt64 *)(const void *)(p + 8)); p += 2 * 8;
    }
    while (p != lim);
  }
  
  for (; size != 0; size--)
    v = __crc32b(v, *p++);

  return v;
}

#undef T0_32_UNROLL_BYTES
#undef T0_64_UNROLL_BYTES

#endif // defined(USE_ARM64_CRC) || defined(USE_CRC_EMU)

#endif // MY_CPU_LE




void Z7_FASTCALL CrcGenerateTable(void)
{
  UInt32 i;
  for (i = 0; i < 256; i++)
  {
    UInt32 r = i;
    unsigned j;
    for (j = 0; j < 8; j++)
      r = (r >> 1) ^ (kCrcPoly & ((UInt32)0 - (r & 1)));
    g_CrcTable[i] = r;
  }
  for (i = 256; i < 256 * CRC_NUM_TABLES; i++)
  {
    const UInt32 r = g_CrcTable[(size_t)i - 256];
    g_CrcTable[i] = g_CrcTable[r & 0xFF] ^ (r >> 8);
  }

  #if CRC_NUM_TABLES < 4
    g_CrcUpdate = CrcUpdateT1;
  #elif defined(MY_CPU_LE)
    // g_CrcUpdateT4 = CrcUpdateT4;
    #if CRC_NUM_TABLES < 8
      g_CrcUpdate = CrcUpdateT4;
    #else // CRC_NUM_TABLES >= 8
      g_CrcUpdateT8 = CrcUpdateT8;
      /*
      #ifdef MY_CPU_X86_OR_AMD64
      if (!CPU_Is_InOrder())
      #endif
      */
      g_CrcUpdate = CrcUpdateT8;
    #endif
  #else
  {
   #ifndef MY_CPU_BE
    UInt32 k = 0x01020304;
    const Byte *p = (const Byte *)&k;
    if (p[0] == 4 && p[1] == 3)
    {
      #if CRC_NUM_TABLES < 8
        // g_CrcUpdateT4 = CrcUpdateT4;
        g_CrcUpdate   = CrcUpdateT4;
      #else  // CRC_NUM_TABLES >= 8
        g_CrcUpdateT8 = CrcUpdateT8;
        g_CrcUpdate   = CrcUpdateT8;
      #endif
    }
    else if (p[0] != 1 || p[1] != 2)
      g_CrcUpdate = CrcUpdateT1;
    else
   #endif // MY_CPU_BE
    {
      for (i = 256 * CRC_NUM_TABLES - 1; i >= 256; i--)
      {
        const UInt32 x = g_CrcTable[(size_t)i - 256];
        g_CrcTable[i] = Z7_BSWAP32(x);
      }
      #if CRC_NUM_TABLES <= 4
        g_CrcUpdate = CrcUpdateT1;
      #elif CRC_NUM_TABLES <= 8
        // g_CrcUpdateT4 = CrcUpdateT1_BeT4;
        g_CrcUpdate   = CrcUpdateT1_BeT4;
      #else  // CRC_NUM_TABLES > 8
        g_CrcUpdateT8 = CrcUpdateT1_BeT8;
        g_CrcUpdate   = CrcUpdateT1_BeT8;
      #endif
    }
  }
  #endif // CRC_NUM_TABLES < 4

  #ifdef MY_CPU_LE
    #ifdef USE_ARM64_CRC
      if (CPU_IsSupported_CRC32())
      {
        g_CrcUpdateT0_32 = CrcUpdateT0_32;
        g_CrcUpdateT0_64 = CrcUpdateT0_64;
        g_CrcUpdate =
          #if defined(MY_CPU_ARM)
            CrcUpdateT0_32;
          #else
            CrcUpdateT0_64;
          #endif
      }
    #endif
    
    #ifdef USE_CRC_EMU
      g_CrcUpdateT0_32 = CrcUpdateT0_32;
      g_CrcUpdateT0_64 = CrcUpdateT0_64;
      g_CrcUpdate = CrcUpdateT0_64;
    #endif
  #endif
}

#undef kCrcPoly
#undef CRC64_NUM_TABLES
#undef CRC_UPDATE_BYTE_2
