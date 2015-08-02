#pragma once

#include <emmintrin.h>

// temporarily (until noexcept is available); use `noexcept(true)` instead of `noexcept` if necessary
#if defined(_MSC_VER) && _MSC_VER <= 1800
#define noexcept _NOEXCEPT_OP
#endif

#if defined(_MSC_VER) && _MSC_VER <= 1800
#define thread_local __declspec(thread)
#elif __APPLE__
#define thread_local __thread
#endif

#if defined(_MSC_VER)
#define never_inline __declspec(noinline)
#else
#define never_inline __attribute__((noinline))
#endif

#if defined(_MSC_VER)
#define safe_buffers __declspec(safebuffers)
#else
#define safe_buffers
#endif

#if defined(_MSC_VER)
#define force_inline __forceinline
#else
#define force_inline __attribute__((always_inline))
#endif

#if defined(_MSC_VER)
#define set_alignment(x) _CRT_ALIGN(x)
#else
#define set_alignment(x) __attribute__((aligned(x)))
#endif

#if defined(__GNUG__)

#include <stdlib.h>
#include <cstdint>

#ifndef __APPLE__
#include <malloc.h>
#endif

#define _fpclass(x) std::fpclassify(x)
#define _byteswap_ushort(x) __builtin_bswap16(x)
#define _byteswap_ulong(x) __builtin_bswap32(x)
#define _byteswap_uint64(x) __builtin_bswap64(x)
#define INFINITE 0xFFFFFFFF

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

#ifdef __APPLE__

int clock_gettime(int foo, struct timespec *ts);
#define wxIsNaN(x) ((x) != (x))

#ifndef CLOCK_MONOTONIC
#define CLOCK_MONOTONIC 0
#endif /* !CLOCK_MONOTONIC */

#endif /* __APPLE__ */

template<typename T, typename T2> inline std::enable_if_t<std::is_arithmetic<T>::value, T> sync_val_compare_and_swap(volatile T* dest, T2 comp, T2 exch)
{
	return __sync_val_compare_and_swap(dest, comp, exch);
}

template<typename T, typename T2> inline std::enable_if_t<std::is_arithmetic<T>::value, bool> sync_bool_compare_and_swap(volatile T* dest, T2 comp, T2 exch)
{
	return __sync_bool_compare_and_swap(dest, comp, exch);
}

template<typename T, typename T2> inline std::enable_if_t<std::is_arithmetic<T>::value, T> sync_lock_test_and_set(volatile T* dest, T2 value)
{
	return __sync_lock_test_and_set(dest, value);
}

template<typename T, typename T2> inline std::enable_if_t<std::is_arithmetic<T>::value, T> sync_fetch_and_add(volatile T* dest, T2 value)
{
	return __sync_fetch_and_add(dest, value);
}

template<typename T, typename T2> inline std::enable_if_t<std::is_arithmetic<T>::value, T> sync_fetch_and_sub(volatile T* dest, T2 value)
{
	return __sync_fetch_and_sub(dest, value);
}

template<typename T, typename T2> inline std::enable_if_t<std::is_arithmetic<T>::value, T> sync_fetch_and_or(volatile T* dest, T2 value)
{
	return __sync_fetch_and_or(dest, value);
}

template<typename T, typename T2> inline std::enable_if_t<std::is_arithmetic<T>::value, T> sync_fetch_and_and(volatile T* dest, T2 value)
{
	return __sync_fetch_and_and(dest, value);
}

template<typename T, typename T2> inline std::enable_if_t<std::is_arithmetic<T>::value, T> sync_fetch_and_xor(volatile T* dest, T2 value)
{
	return __sync_fetch_and_xor(dest, value);
}

#endif /* __GNUG__ */

#if defined(_MSC_VER)

// atomic compare and swap functions

inline uint8_t sync_val_compare_and_swap(volatile uint8_t* dest, uint8_t comp, uint8_t exch)
{
	return _InterlockedCompareExchange8((volatile char*)dest, exch, comp);
}

inline uint16_t sync_val_compare_and_swap(volatile uint16_t* dest, uint16_t comp, uint16_t exch)
{
	return _InterlockedCompareExchange16((volatile short*)dest, exch, comp);
}

inline uint32_t sync_val_compare_and_swap(volatile uint32_t* dest, uint32_t comp, uint32_t exch)
{
	return _InterlockedCompareExchange((volatile long*)dest, exch, comp);
}

inline uint64_t sync_val_compare_and_swap(volatile uint64_t* dest, uint64_t comp, uint64_t exch)
{
	return _InterlockedCompareExchange64((volatile long long*)dest, exch, comp);
}

inline bool sync_bool_compare_and_swap(volatile uint8_t* dest, uint8_t comp, uint8_t exch)
{
	return (uint8_t)_InterlockedCompareExchange8((volatile char*)dest, exch, comp) == comp;
}

inline bool sync_bool_compare_and_swap(volatile uint16_t* dest, uint16_t comp, uint16_t exch)
{
	return (uint16_t)_InterlockedCompareExchange16((volatile short*)dest, exch, comp) == comp;
}

inline bool sync_bool_compare_and_swap(volatile uint32_t* dest, uint32_t comp, uint32_t exch)
{
	return (uint32_t)_InterlockedCompareExchange((volatile long*)dest, exch, comp) == comp;
}

inline bool sync_bool_compare_and_swap(volatile uint64_t* dest, uint64_t comp, uint64_t exch)
{
	return (uint64_t)_InterlockedCompareExchange64((volatile long long*)dest, exch, comp) == comp;
}

// atomic exchange functions

inline uint8_t sync_lock_test_and_set(volatile uint8_t* dest, uint8_t value)
{
	return _InterlockedExchange8((volatile char*)dest, value);
}

inline uint16_t sync_lock_test_and_set(volatile uint16_t* dest, uint16_t value)
{
	return _InterlockedExchange16((volatile short*)dest, value);
}

inline uint32_t sync_lock_test_and_set(volatile uint32_t* dest, uint32_t value)
{
	return _InterlockedExchange((volatile long*)dest, value);
}

inline uint64_t sync_lock_test_and_set(volatile uint64_t* dest, uint64_t value)
{
	return _InterlockedExchange64((volatile long long*)dest, value);
}

// atomic add functions

inline uint8_t sync_fetch_and_add(volatile uint8_t* dest, uint8_t value)
{
	return _InterlockedExchangeAdd8((volatile char*)dest, value);
}

inline uint16_t sync_fetch_and_add(volatile uint16_t* dest, uint16_t value)
{
	return _InterlockedExchangeAdd16((volatile short*)dest, value);
}

inline uint32_t sync_fetch_and_add(volatile uint32_t* dest, uint32_t value)
{
	return _InterlockedExchangeAdd((volatile long*)dest, value);
}

inline uint64_t sync_fetch_and_add(volatile uint64_t* dest, uint64_t value)
{
	return _InterlockedExchangeAdd64((volatile long long*)dest, value);
}

// atomic sub functions

inline uint8_t sync_fetch_and_sub(volatile uint8_t* dest, uint8_t value)
{
	return _InterlockedExchangeAdd8((volatile char*)dest, -(char)value);
}

inline uint16_t sync_fetch_and_sub(volatile uint16_t* dest, uint16_t value)
{
	return _InterlockedExchangeAdd16((volatile short*)dest, -(short)value);
}

inline uint32_t sync_fetch_and_sub(volatile uint32_t* dest, uint32_t value)
{
	return _InterlockedExchangeAdd((volatile long*)dest, -(long)value);
}

inline uint64_t sync_fetch_and_sub(volatile uint64_t* dest, uint64_t value)
{
	return _InterlockedExchangeAdd64((volatile long long*)dest, -(long long)value);
}

// atomic `bitwise or` functions

inline uint8_t sync_fetch_and_or(volatile uint8_t* dest, uint8_t value)
{
	return _InterlockedOr8((volatile char*)dest, value);
}

inline uint16_t sync_fetch_and_or(volatile uint16_t* dest, uint16_t value)
{
	return _InterlockedOr16((volatile short*)dest, value);
}

inline uint32_t sync_fetch_and_or(volatile uint32_t* dest, uint32_t value)
{
	return _InterlockedOr((volatile long*)dest, value);
}

inline uint64_t sync_fetch_and_or(volatile uint64_t* dest, uint64_t value)
{
	return _InterlockedOr64((volatile long long*)dest, value);
}

// atomic `bitwise and` functions

inline uint8_t sync_fetch_and_and(volatile uint8_t* dest, uint8_t value)
{
	return _InterlockedAnd8((volatile char*)dest, value);
}

inline uint16_t sync_fetch_and_and(volatile uint16_t* dest, uint16_t value)
{
	return _InterlockedAnd16((volatile short*)dest, value);
}

inline uint32_t sync_fetch_and_and(volatile uint32_t* dest, uint32_t value)
{
	return _InterlockedAnd((volatile long*)dest, value);
}

inline uint64_t sync_fetch_and_and(volatile uint64_t* dest, uint64_t value)
{
	return _InterlockedAnd64((volatile long long*)dest, value);
}

// atomic `bitwise xor` functions

inline uint8_t sync_fetch_and_xor(volatile uint8_t* dest, uint8_t value)
{
	return _InterlockedXor8((volatile char*)dest, value);
}

inline uint16_t sync_fetch_and_xor(volatile uint16_t* dest, uint16_t value)
{
	return _InterlockedXor16((volatile short*)dest, value);
}

inline uint32_t sync_fetch_and_xor(volatile uint32_t* dest, uint32_t value)
{
	return _InterlockedXor((volatile long*)dest, value);
}

inline uint64_t sync_fetch_and_xor(volatile uint64_t* dest, uint64_t value)
{
	return _InterlockedXor64((volatile long long*)dest, value);
}

#endif /* _MSC_VER */

inline uint32_t cntlz32(uint32_t arg)
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

inline uint64_t cntlz64(uint64_t arg)
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
inline __m128i sse_cmpgt_epu8(__m128i A, __m128i B)
{
	// (A xor 0x80) > (B xor 0x80)
	const auto sign = _mm_set1_epi32(0x80808080);
	return _mm_cmpgt_epi8(_mm_xor_si128(A, sign), _mm_xor_si128(B, sign));
}

inline __m128i sse_cmpgt_epu16(__m128i A, __m128i B)
{
	const auto sign = _mm_set1_epi32(0x80008000);
	return _mm_cmpgt_epi16(_mm_xor_si128(A, sign), _mm_xor_si128(B, sign));
}

inline __m128i sse_cmpgt_epu32(__m128i A, __m128i B)
{
	const auto sign = _mm_set1_epi32(0x80000000);
	return _mm_cmpgt_epi32(_mm_xor_si128(A, sign), _mm_xor_si128(B, sign));
}

inline __m128 sse_exp2_ps(__m128 A)
{
	const auto x0 = _mm_max_ps(_mm_min_ps(A, _mm_set1_ps(127.4999961f)), _mm_set1_ps(-127.4999961f));
	const auto x1 = _mm_add_ps(x0, _mm_set1_ps(0.5f));
	const auto x2 = _mm_sub_epi32(_mm_cvtps_epi32(x1), _mm_and_si128(_mm_castps_si128(_mm_cmpnlt_ps(_mm_setzero_ps(), x1)), _mm_set1_epi32(1)));
	const auto x3 = _mm_sub_ps(x0, _mm_cvtepi32_ps(x2));
	const auto x4 = _mm_mul_ps(x3, x3);
	const auto x5 = _mm_mul_ps(x3, _mm_add_ps(_mm_mul_ps(_mm_add_ps(_mm_mul_ps(x4, _mm_set1_ps(0.023093347705f)), _mm_set1_ps(20.20206567f)), x4), _mm_set1_ps(1513.906801f)));
	const auto x6 = _mm_mul_ps(x5, _mm_rcp_ps(_mm_sub_ps(_mm_add_ps(_mm_mul_ps(_mm_set1_ps(233.1842117f), x4), _mm_set1_ps(4368.211667f)), x5)));
	return _mm_mul_ps(_mm_add_ps(_mm_add_ps(x6, x6), _mm_set1_ps(1.0f)), _mm_castsi128_ps(_mm_slli_epi32(_mm_add_epi32(x2, _mm_set1_epi32(127)), 23)));
}

inline __m128 sse_log2_ps(__m128 A)
{
	const auto _1 = _mm_set1_ps(1.0f);
	const auto _c = _mm_set1_ps(1.442695040f);
	const auto x0 = _mm_max_ps(A, _mm_castsi128_ps(_mm_set1_epi32(0x00800000)));
	const auto x1 = _mm_or_ps(_mm_and_ps(x0, _mm_castsi128_ps(_mm_set1_epi32(0x807fffff))), _1);
	const auto x2 = _mm_rcp_ps(_mm_add_ps(x1, _1));
	const auto x3 = _mm_mul_ps(_mm_sub_ps(x1, _1), x2);
	const auto x4 = _mm_add_ps(x3, x3);
	const auto x5 = _mm_mul_ps(x4, x4);
	const auto x6 = _mm_add_ps(_mm_mul_ps(_mm_add_ps(_mm_mul_ps(_mm_set1_ps(-0.7895802789f), x5), _mm_set1_ps(16.38666457f)), x5), _mm_set1_ps(-64.1409953f));
	const auto x7 = _mm_rcp_ps(_mm_add_ps(_mm_mul_ps(_mm_add_ps(_mm_mul_ps(_mm_set1_ps(-35.67227983f), x5), _mm_set1_ps(312.0937664f)), x5), _mm_set1_ps(-769.6919436f)));
	const auto x8 = _mm_cvtepi32_ps(_mm_sub_epi32(_mm_srli_epi32(_mm_castps_si128(x0), 23), _mm_set1_epi32(127)));
	return _mm_add_ps(_mm_mul_ps(_mm_mul_ps(_mm_mul_ps(_mm_mul_ps(x5, x6), x7), x4), _c), _mm_add_ps(_mm_mul_ps(x4, _c), x8));
}
