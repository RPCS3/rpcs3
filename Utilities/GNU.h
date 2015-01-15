#pragma once

#include <emmintrin.h>

#ifdef _WIN64
#define thread_local __declspec(thread)
#elif __APPLE__
#define thread_local __thread
#endif

#ifdef _WIN64
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
static __forceinline uint8_t InterlockedCompareExchange(volatile uint8_t* dest, uint8_t exch, uint8_t comp)
{
#if defined(__GNUG__)
	return __sync_val_compare_and_swap(dest, comp, exch);
#else
	return _InterlockedCompareExchange8((volatile char*)dest, exch, comp);
#endif
}
static __forceinline uint16_t InterlockedCompareExchange(volatile uint16_t* dest, uint16_t exch, uint16_t comp)
{
#if defined(__GNUG__)
	return __sync_val_compare_and_swap(dest, comp, exch);
#else
	return _InterlockedCompareExchange16((volatile short*)dest, exch, comp);
#endif
}
static __forceinline uint32_t InterlockedCompareExchange(volatile uint32_t* dest, uint32_t exch, uint32_t comp)
{
#if defined(__GNUG__)
	return __sync_val_compare_and_swap(dest, comp, exch);
#else
	return _InterlockedCompareExchange((volatile long*)dest, exch, comp);
#endif
}
static __forceinline uint64_t InterlockedCompareExchange(volatile uint64_t* dest, uint64_t exch, uint64_t comp)
{
#if defined(__GNUG__)
	return __sync_val_compare_and_swap(dest, comp, exch);
#else
	return _InterlockedCompareExchange64((volatile long long*)dest, exch, comp);
#endif
}
#endif

static __forceinline bool InterlockedCompareExchangeTest(volatile uint8_t* dest, uint8_t exch, uint8_t comp)
{
#if defined(__GNUG__)
	return __sync_bool_compare_and_swap(dest, comp, exch);
#else
	return (uint8_t)_InterlockedCompareExchange8((volatile char*)dest, exch, comp) == comp;
#endif
}
static __forceinline bool InterlockedCompareExchangeTest(volatile uint16_t* dest, uint16_t exch, uint16_t comp)
{
#if defined(__GNUG__)
	return __sync_bool_compare_and_swap(dest, comp, exch);
#else
	return (uint16_t)_InterlockedCompareExchange16((volatile short*)dest, exch, comp) == comp;
#endif
}
static __forceinline bool InterlockedCompareExchangeTest(volatile uint32_t* dest, uint32_t exch, uint32_t comp)
{
#if defined(__GNUG__)
	return __sync_bool_compare_and_swap(dest, comp, exch);
#else
	return (uint32_t)_InterlockedCompareExchange((volatile long*)dest, exch, comp) == comp;
#endif
}
static __forceinline bool InterlockedCompareExchangeTest(volatile uint64_t* dest, uint64_t exch, uint64_t comp)
{
#if defined(__GNUG__)
	return __sync_bool_compare_and_swap(dest, comp, exch);
#else
	return (uint64_t)_InterlockedCompareExchange64((volatile long long*)dest, exch, comp) == comp;
#endif
}

#ifndef InterlockedExchange
static __forceinline uint8_t InterlockedExchange(volatile uint8_t* dest, uint8_t value)
{
#if defined(__GNUG__)
	return __sync_lock_test_and_set(dest, value);
#else
	return _InterlockedExchange8((volatile char*)dest, value);
#endif
}
static __forceinline uint16_t InterlockedExchange(volatile uint16_t* dest, uint16_t value)
{
#if defined(__GNUG__)
	return __sync_lock_test_and_set(dest, value);
#else
	return _InterlockedExchange16((volatile short*)dest, value);
#endif
}
static __forceinline uint32_t InterlockedExchange(volatile uint32_t* dest, uint32_t value)
{
#if defined(__GNUG__)
	return __sync_lock_test_and_set(dest, value);
#else
	return _InterlockedExchange((volatile long*)dest, value);
#endif
}
static __forceinline uint64_t InterlockedExchange(volatile uint64_t* dest, uint64_t value)
{
#if defined(__GNUG__)
	return __sync_lock_test_and_set(dest, value);
#else
	return _InterlockedExchange64((volatile long long*)dest, value);
#endif
}
#endif

#ifndef InterlockedOr
static __forceinline uint8_t InterlockedOr(volatile uint8_t* dest, uint8_t value)
{
#if defined(__GNUG__)
	return __sync_fetch_and_or(dest, value);
#else
	return _InterlockedOr8((volatile char*)dest, value);
#endif
}
static __forceinline uint16_t InterlockedOr(volatile uint16_t* dest, uint16_t value)
{
#if defined(__GNUG__)
	return __sync_fetch_and_or(dest, value);
#else
	return _InterlockedOr16((volatile short*)dest, value);
#endif
}
static __forceinline uint32_t InterlockedOr(volatile uint32_t* dest, uint32_t value)
{
#if defined(__GNUG__)
	return __sync_fetch_and_or(dest, value);
#else
	return _InterlockedOr((volatile long*)dest, value);
#endif
}
static __forceinline uint64_t InterlockedOr(volatile uint64_t* dest, uint64_t value)
{
#if defined(__GNUG__)
	return __sync_fetch_and_or(dest, value);
#else
	return _InterlockedOr64((volatile long long*)dest, value);
#endif
}
#endif

#ifndef InterlockedAnd
static __forceinline uint8_t InterlockedAnd(volatile uint8_t* dest, uint8_t value)
{
#if defined(__GNUG__)
	return __sync_fetch_and_and(dest, value);
#else
	return _InterlockedAnd8((volatile char*)dest, value);
#endif
}
static __forceinline uint16_t InterlockedAnd(volatile uint16_t* dest, uint16_t value)
{
#if defined(__GNUG__)
	return __sync_fetch_and_and(dest, value);
#else
	return _InterlockedAnd16((volatile short*)dest, value);
#endif
}
static __forceinline uint32_t InterlockedAnd(volatile uint32_t* dest, uint32_t value)
{
#if defined(__GNUG__)
	return __sync_fetch_and_and(dest, value);
#else
	return _InterlockedAnd((volatile long*)dest, value);
#endif
}
static __forceinline uint64_t InterlockedAnd(volatile uint64_t* dest, uint64_t value)
{
#if defined(__GNUG__)
	return __sync_fetch_and_and(dest, value);
#else
	return _InterlockedAnd64((volatile long long*)dest, value);
#endif
}
#endif

#ifndef InterlockedXor
static __forceinline uint8_t InterlockedXor(volatile uint8_t* dest, uint8_t value)
{
#if defined(__GNUG__)
	return __sync_fetch_and_xor(dest, value);
#else
	return _InterlockedXor8((volatile char*)dest, value);
#endif
}
static __forceinline uint16_t InterlockedXor(volatile uint16_t* dest, uint16_t value)
{
#if defined(__GNUG__)
	return __sync_fetch_and_xor(dest, value);
#else
	return _InterlockedXor16((volatile short*)dest, value);
#endif
}
static __forceinline uint32_t InterlockedXor(volatile uint32_t* dest, uint32_t value)
{
#if defined(__GNUG__)
	return __sync_fetch_and_xor(dest, value);
#else
	return _InterlockedXor((volatile long*)dest, value);
#endif
}
static __forceinline uint64_t InterlockedXor(volatile uint64_t* dest, uint64_t value)
{
#if defined(__GNUG__)
	return __sync_fetch_and_xor(dest, value);
#else
	return _InterlockedXor64((volatile long long*)dest, value);
#endif
}
#endif

static __forceinline uint32_t cntlz32(uint32_t arg)
{
#if defined(_MSC_VER)
	unsigned long res;
	if (!_BitScanReverse(&res, arg))
	{
		return 32;
	}
	else
	{
		return res ^ 31;
	}
#else
	if (arg)
	{
		return __builtin_clzll((uint64_t)arg) - 32;
	}
	else
	{
		return 32;
	}
#endif
}

static __forceinline uint64_t cntlz64(uint64_t arg)
{
#if defined(_MSC_VER)
	unsigned long res;
	if (!_BitScanReverse64(&res, arg))
	{
		return 64;
	}
	else
	{
		return res ^ 63;
	}
#else
	if (arg)
	{
		return __builtin_clzll(arg);
	}
	else
	{
		return 64;
	}
#endif
}

// compare 16 packed unsigned bytes (greater than)
static __forceinline __m128i _mm_cmpgt_epu8(__m128i A, __m128i B)
{
	// (A xor 0x80) > (B xor 0x80)
	return _mm_cmpgt_epi8(_mm_xor_si128(A, _mm_set1_epi8(-128)), _mm_xor_si128(B, _mm_set1_epi8(-128)));
}

// compare 16 packed unsigned bytes (less or equal)
static __forceinline __m128i _mm_cmple_epu8(__m128i A, __m128i B)
{
	// ((B xor 0x80) > (A xor 0x80)) || A == B
	return _mm_or_si128(_mm_cmpgt_epu8(B, A), _mm_cmpeq_epi8(A, B));
}
