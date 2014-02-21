#pragma once

#if defined(__GNUG__)
#include <math.h>
#define _fpclass(x) fpclassify(x)
#define __forceinline __attribute__((always_inline))
#define _byteswap_ushort(x) __builtin_bswap16(x)
#define _byteswap_ulong(x) __builtin_bswap32(x)
#define _byteswap_uint64(x) __builtin_bswap64(x)
#define Sleep(x) usleep(x * 1000)
#define mkdir(x) mkdir(x, 0777)
#define INFINITE 0xFFFFFFFF
#define _CRT_ALIGN(x) __attribute__((aligned(x)))
#endif
