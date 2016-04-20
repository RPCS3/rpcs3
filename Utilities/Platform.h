#pragma once

#include <cstdint>
#include <immintrin.h>
#include <emmintrin.h>

#define IS_LE_MACHINE 1
#define IS_BE_MACHINE 0

#ifdef _MSC_VER
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

#ifdef _MSC_VER
#define ASSUME(cond) __assume(cond)
#define LIKELY(cond) (cond)
#define UNLIKELY(cond) (cond)
#else
#define ASSUME(cond) do { if (!(cond)) __builtin_unreachable(); } while (0)
#define LIKELY(cond) __builtin_expect(!!(cond), 1)
#define UNLIKELY(cond) __builtin_expect(!!(cond), 0)
#endif

// Some platforms don't support thread_local well yet.
#ifndef _MSC_VER
#define thread_local __thread
#endif

#ifdef _MSC_VER
#define never_inline __declspec(noinline)
#else
#define never_inline __attribute__((noinline))
#endif

#ifdef _MSC_VER
#define force_inline __forceinline
#else
#define force_inline __attribute__((always_inline)) inline
#endif

#if defined(__GNUG__)

#include <stdlib.h>

#define _fpclass(x) std::fpclassify(x)
#define INFINITE 0xFFFFFFFF

#ifdef __APPLE__

// XXX only supports a single timer
#define TIMER_ABSTIME -1
/* The opengroup spec isn't clear on the mapping from REALTIME to CALENDAR
 being appropriate or not.
 http://pubs.opengroup.org/onlinepubs/009695299/basedefs/time.h.html */
#define CLOCK_REALTIME  1 // #define CALENDAR_CLOCK 1 from mach/clock_types.h
#define CLOCK_MONOTONIC 0 // #define SYSTEM_CLOCK 0

typedef int clockid_t;

/* the mach kernel uses struct mach_timespec, so struct timespec
    is loaded from <sys/_types/_timespec.h> for compatability */
// struct timespec { time_t tv_sec; long tv_nsec; };

int clock_gettime(clockid_t clk_id, struct timespec *tp);

#endif /* __APPLE__ */
#endif /* __GNUG__ */

inline std::uint32_t cntlz32(std::uint32_t arg)
{
#if defined(_MSC_VER)
	unsigned long res;
	return _BitScanReverse(&res, arg) ? res ^ 31 : 32;
#else
	return arg ? __builtin_clzll(arg) - 32 : 32;
#endif
}

inline std::uint64_t cntlz64(std::uint64_t arg)
{
#if defined(_MSC_VER)
	unsigned long res;
	return _BitScanReverse64(&res, arg) ? res ^ 63 : 64;
#else
	return arg ? __builtin_clzll(arg) : 64;
#endif
}

template<typename T>
struct add_flags_result_t
{
	T result;
	bool carry;
	//bool overflow;
	bool zero;
	bool sign;

	add_flags_result_t() = default;

	// Straighforward ADD with flags
	add_flags_result_t(T a, T b)
		: result(a + b)
		, carry(result < a)
		//, overflow((result ^ ~(a ^ b)) >> (sizeof(T) * 8 - 1) != 0)
		, zero(result == 0)
		, sign(result >> (sizeof(T) * 8 - 1) != 0)
	{
	}

	// Straighforward ADC with flags
	add_flags_result_t(T a, T b, bool c)
		: add_flags_result_t(a, b)
	{
		add_flags_result_t r(result, c);
		result = r.result;
		carry |= r.carry;
		//overflow |= r.overflow;
		zero = r.zero;
		sign = r.sign;
	}
};

inline add_flags_result_t<std::uint32_t> add32_flags(std::uint32_t a, std::uint32_t b)
{
	//add_flags_result_t<std::uint32_t> r;
	//r.carry = _addcarry_u32(0, a, b, &r.result) != 0;
	//r.zero = r.result == 0;
	//r.sign = r.result >> 31;
	//return r;

	return{ a, b };
}

inline add_flags_result_t<std::uint32_t> add32_flags(std::uint32_t a, std::uint32_t b, bool c)
{
	return{ a, b, c };
}

inline add_flags_result_t<std::uint64_t> add64_flags(std::uint64_t a, std::uint64_t b)
{
	return{ a, b };
}

inline add_flags_result_t<std::uint64_t> add64_flags(std::uint64_t a, std::uint64_t b, bool c)
{
	return{ a, b, c };
}

// Compare 16 packed unsigned bytes (greater than)
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

// Helper function, used by ""_u16, ""_u32, ""_u64
constexpr std::uint8_t to_u8(char c)
{
	return static_cast<std::uint8_t>(c);
}

// Convert 2-byte string to u16 value like reinterpret_cast does
constexpr std::uint16_t operator""_u16(const char* s, std::size_t length)
{
	return length != 2 ? throw s :
#if IS_LE_MACHINE == 1
		to_u8(s[1]) << 8 | to_u8(s[0]);
#endif
}

// Convert 4-byte string to u32 value like reinterpret_cast does
constexpr std::uint32_t operator""_u32(const char* s, std::size_t length)
{
	return length != 4 ? throw s :
#if IS_LE_MACHINE == 1
		to_u8(s[3]) << 24 | to_u8(s[2]) << 16 | to_u8(s[1]) << 8 | to_u8(s[0]);
#endif
}

// Convert 8-byte string to u64 value like reinterpret_cast does
constexpr std::uint64_t operator""_u64(const char* s, std::size_t length)
{
	return length != 8 ? throw s :
#if IS_LE_MACHINE == 1
		static_cast<std::uint64_t>(to_u8(s[7]) << 24 | to_u8(s[6]) << 16 | to_u8(s[5]) << 8 | to_u8(s[4])) << 32 | to_u8(s[3]) << 24 | to_u8(s[2]) << 16 | to_u8(s[1]) << 8 | to_u8(s[0]);
#endif
}
