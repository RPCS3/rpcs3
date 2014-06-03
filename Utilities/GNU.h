#pragma once

#if defined(__GNUG__)
#include <cmath>
#include <stdlib.h>
#include <cstdint>

#ifndef __APPLE__
#include <malloc.h>
#endif

#define _fpclass(x) std::fpclassify(x)
#define __forceinline __attribute__((always_inline))
#define _byteswap_ushort(x) __builtin_bswap16(x)
#define _byteswap_ulong(x) __builtin_bswap32(x)
#define _byteswap_uint64(x) __builtin_bswap64(x)
#define Sleep(x) usleep(x * 1000)
#define mkdir(x) mkdir(x, 0777)
#define INFINITE 0xFFFFFFFF
#define _CRT_ALIGN(x) __attribute__((aligned(x)))
#define InterlockedCompareExchange(ptr,new_val,old_val)  __sync_val_compare_and_swap(ptr,old_val,new_val)
#define InterlockedCompareExchange64(ptr,new_val,old_val)  __sync_val_compare_and_swap(ptr,old_val,new_val)

inline int64_t InterlockedOr64(volatile int64_t *dest, int64_t val)
{
	int64_t olderval;
	int64_t oldval = *dest;
	do
	{
		olderval = oldval;
		oldval = InterlockedCompareExchange64(dest, olderval | val, olderval);
	} while (olderval != oldval);
	return oldval;
}

inline uint64_t __umulh(uint64_t a, uint64_t b)
{
	uint64_t result;
	__asm__("mulq %[b]" : "=d" (result) : [a] "a" (a), [b] "rm" (b));
	return result;
}

inline int64_t  __mulh(int64_t a, int64_t b)
{
	int64_t result;
	__asm__("imulq %[b]" : "=d" (result) : [a] "a" (a), [b] "rm" (b));
	return result;
}

#ifndef __APPLE__
#define _aligned_malloc(size,alignment) memalign(alignment,size)
#else
void * _aligned_malloc(size_t size, size_t alignment);
int clock_gettime(int foo, struct timespec *ts);
#define wxIsNaN(x) ((x) != (x))

#ifndef CLOCK_MONOTONIC
#define CLOCK_MONOTONIC 0
#endif /* !CLOCK_MONOTONIC */

#endif /* !__APPLE__ */

#define _aligned_free free

#define DWORD int32_t
#endif
