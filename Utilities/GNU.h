#pragma once

#ifdef _WIN32
#define thread_local __declspec(thread)
#elif __APPLE__
#define thread_local __thread
#endif

#ifdef _WIN32
#define __noinline __declspec(noinline)
#else
#define __noinline __attribute__((noinline))
#endif

template<size_t size>
void strcpy_trunc(char(&dst)[size], const std::string& src)
{
	const size_t count = (src.size() >= size) ? size - 1 /* truncation */ : src.size();
	memcpy(dst, src.c_str(), count);
	dst[count] = 0;
}

template<size_t size, size_t rsize>
void strcpy_trunc(char(&dst)[size], const char(&src)[rsize])
{
	const size_t count = (rsize >= size) ? size - 1 /* truncation */ : rsize;
	memcpy(dst, src, count);
	dst[count] = 0;
}

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
#define INFINITE 0xFFFFFFFF
#define _CRT_ALIGN(x) __attribute__((aligned(x)))
#define InterlockedCompareExchange(ptr,new_val,old_val)  __sync_val_compare_and_swap(ptr,old_val,new_val)
#define InterlockedExchange(ptr, value) __sync_lock_test_and_set(ptr, value)
#define InterlockedOr(ptr, value) __sync_fetch_and_or(ptr, value)
#define InterlockedAnd(ptr, value) __sync_fetch_and_and(ptr, value)
#define InterlockedXor(ptr, value) __sync_fetch_and_xor(ptr, value)

//inline int64_t InterlockedOr64(volatile int64_t *dest, int64_t val)
//{
//	int64_t olderval;
//	int64_t oldval = *dest;
//	do
//	{
//		olderval = oldval;
//		oldval = __sync_val_compare_and_swap(dest, olderval | val, olderval);
//	} while (olderval != oldval);
//	return oldval;
//}

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


void * _aligned_malloc(size_t size, size_t alignment);

#ifdef __APPLE__
int clock_gettime(int foo, struct timespec *ts);
#define wxIsNaN(x) ((x) != (x))

#ifndef CLOCK_MONOTONIC
#define CLOCK_MONOTONIC 0
#endif /* !CLOCK_MONOTONIC */

#endif /* __APPLE__ */

#define _aligned_free free

#define DWORD int32_t
#endif

#ifndef InterlockedCompareExchange
static __forceinline uint16_t InterlockedCompareExchange(volatile uint16_t* dest, uint16_t exch, uint16_t comp)
{
	return _InterlockedCompareExchange16((volatile short*)dest, exch, comp);
}
static __forceinline uint32_t InterlockedCompareExchange(volatile uint32_t* dest, uint32_t exch, uint32_t comp)
{
	return _InterlockedCompareExchange((volatile long*)dest, exch, comp);
}
static __forceinline uint64_t InterlockedCompareExchange(volatile uint64_t* dest, uint64_t exch, uint64_t comp)
{
	return _InterlockedCompareExchange64((volatile long long*)dest, exch, comp);
}
#endif

#ifndef InterlockedExchange
static __forceinline uint16_t InterlockedExchange(volatile uint16_t* dest, uint16_t value)
{
	return _InterlockedExchange16((volatile short*)dest, value);
}
static __forceinline uint32_t InterlockedExchange(volatile uint32_t* dest, uint32_t value)
{
	return _InterlockedExchange((volatile long*)dest, value);
}
static __forceinline uint64_t InterlockedExchange(volatile uint64_t* dest, uint64_t value)
{
	return _InterlockedExchange64((volatile long long*)dest, value);
}
#endif

#ifndef InterlockedOr
static __forceinline uint16_t InterlockedOr(volatile uint16_t* dest, uint16_t value)
{
	return _InterlockedOr16((volatile short*)dest, value);
}
static __forceinline uint32_t InterlockedOr(volatile uint32_t* dest, uint32_t value)
{
	return _InterlockedOr((volatile long*)dest, value);
}
static __forceinline uint64_t InterlockedOr(volatile uint64_t* dest, uint64_t value)
{
	return _InterlockedOr64((volatile long long*)dest, value);
}
#endif

#ifndef InterlockedAnd
static __forceinline uint16_t InterlockedAnd(volatile uint16_t* dest, uint16_t value)
{
	return _InterlockedAnd16((volatile short*)dest, value);
}
static __forceinline uint32_t InterlockedAnd(volatile uint32_t* dest, uint32_t value)
{
	return _InterlockedAnd((volatile long*)dest, value);
}
static __forceinline uint64_t InterlockedAnd(volatile uint64_t* dest, uint64_t value)
{
	return _InterlockedAnd64((volatile long long*)dest, value);
}
#endif

#ifndef InterlockedXor
static __forceinline uint16_t InterlockedXor(volatile uint16_t* dest, uint16_t value)
{
	return _InterlockedXor16((volatile short*)dest, value);
}
static __forceinline uint32_t InterlockedXor(volatile uint32_t* dest, uint32_t value)
{
	return _InterlockedXor((volatile long*)dest, value);
}
static __forceinline uint64_t InterlockedXor(volatile uint64_t* dest, uint64_t value)
{
	return _InterlockedXor64((volatile long long*)dest, value);
}
#endif
