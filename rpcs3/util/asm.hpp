#pragma once

#include "util/types.hpp"
#include "util/tsc.hpp"
#include "util/atomic.hpp"
#include <functional>

#ifdef ARCH_X64
#ifdef _MSC_VER
#include <intrin.h>
#else
#include <immintrin.h>
#include <x86intrin.h>
#endif
#endif

namespace utils
{
	// Try to prefetch to Level 2 cache since it's not split to data/code on most processors
	template <typename T>
	constexpr void prefetch_exec(T func)
	{
		if (std::is_constant_evaluated())
		{
			return;
		}

		const u64 value = reinterpret_cast<u64>(func);
		const void* ptr = reinterpret_cast<const void*>(value);

#ifdef ARCH_X64
		return _mm_prefetch(static_cast<const char*>(ptr), _MM_HINT_T1);
#else
		return __builtin_prefetch(ptr, 0, 2);
#endif
	}

	// Try to prefetch to Level 1 cache
	constexpr void prefetch_read(const void* ptr)
	{
		if (std::is_constant_evaluated())
		{
			return;
		}

#ifdef ARCH_X64
		return _mm_prefetch(static_cast<const char*>(ptr), _MM_HINT_T0);
#else
		return __builtin_prefetch(ptr, 0, 3);
#endif
	}

	constexpr void prefetch_write(const void* ptr)
	{
		if (std::is_constant_evaluated())
		{
			return;
		}

#if defined(ARCH_X64)
		return _m_prefetchw(const_cast<void*>(ptr));
#else
		return __builtin_prefetch(ptr, 1, 0);
#endif
	}

	constexpr u32 popcnt128(const u128& v)
	{
#ifdef _MSC_VER
		return std::popcount(v.lo) + std::popcount(v.hi);
#else
		return std::popcount(v);
#endif
	}

	constexpr u64 umulh64(u64 x, u64 y)
	{
#ifdef _MSC_VER
		if (std::is_constant_evaluated())
#endif
		{
			return static_cast<u64>((u128{x} * u128{y}) >> 64);
		}

#ifdef _MSC_VER
		return __umulh(x, y);
#endif
	}

	inline s64 mulh64(s64 x, s64 y)
	{
#ifdef _MSC_VER
		return __mulh(x, y);
#else
		return (s128{x} * s128{y}) >> 64;
#endif
	}

	inline s64 div128(s64 high, s64 low, s64 divisor, s64* remainder = nullptr)
	{
#ifdef _MSC_VER
		s64 rem = 0;
		s64 r = _div128(high, low, divisor, &rem);

		if (remainder)
		{
			*remainder = rem;
		}
#else
		const s128 x = (u128{static_cast<u64>(high)} << 64) | u64(low);
		const s128 r = x / divisor;

		if (remainder)
		{
			*remainder = x % divisor;
		}
#endif
		return r;
	}

	inline u64 udiv128(u64 high, u64 low, u64 divisor, u64* remainder = nullptr)
	{
#ifdef _MSC_VER
		u64 rem = 0;
		u64 r = _udiv128(high, low, divisor, &rem);

		if (remainder)
		{
			*remainder = rem;
		}
#else
		const u128 x = (u128{high} << 64) | low;
		const u128 r = x / divisor;

		if (remainder)
		{
			*remainder = x % divisor;
		}
#endif
		return r;
	}

#ifdef _MSC_VER
	inline u128 operator/(u128 lhs, u64 rhs)
	{
		u64 rem = 0;
		return _udiv128(lhs.hi, lhs.lo, rhs, &rem);
	}
#endif

	constexpr u32 ctz128(u128 arg)
	{
#ifdef _MSC_VER
		if (!arg.lo)
			return std::countr_zero(arg.hi) + 64u;
		else
			return std::countr_zero(arg.lo);
#else
		return std::countr_zero(arg);
#endif
	}

	constexpr u32 clz128(u128 arg)
	{
#ifdef _MSC_VER
		if (arg.hi)
			return std::countl_zero(arg.hi);
		else
			return std::countl_zero(arg.lo) + 64;
#else
		return std::countl_zero(arg);
#endif
	}

	inline void pause()
	{
#if defined(ARCH_ARM64)
		__asm__ volatile("yield");
#elif defined(ARCH_X64)
		_mm_pause();
#else
#error "Missing utils::pause() implementation"
#endif
	}

	// Synchronization helper (cache-friendly busy waiting)
	inline void busy_wait(usz cycles = 3000)
	{
		const u64 stop = get_tsc() + cycles;
		do pause();
		while (get_tsc() < stop);
	}

	// Align to power of 2
	template <typename T, typename U>
		requires std::is_unsigned_v<T>
	constexpr std::make_unsigned_t<std::common_type_t<T, U>> align(T value, U align)
	{
		return static_cast<std::make_unsigned_t<std::common_type_t<T, U>>>((value + (align - 1)) & (T{0} - align));
	}

	// General purpose aligned division, the result is rounded up not truncated
	template <typename T>
		requires std::is_unsigned_v<T>
	constexpr T aligned_div(T value, std::type_identity_t<T> align)
	{
		return static_cast<T>(value / align + T{!!(value % align)});
	}

	// General purpose aligned division, the result is rounded to nearest
	template <typename T>
		requires std::is_integral_v<T>
	constexpr T rounded_div(T value, std::type_identity_t<T> align)
	{
		if constexpr (std::is_unsigned_v<T>)
		{
			return static_cast<T>(value / align + T{(value % align) > (align / 2)});
		}

		return static_cast<T>(value / align + (value > 0 ? T{(value % align) > (align / 2)} : 0 - T{(value % align) < (align / 2)}));
	}

	// Multiplying by ratio, semi-resistant to overflows
	template <UnsignedInt T>
	constexpr T rational_mul(T value, std::type_identity_t<T> numerator, std::type_identity_t<T> denominator)
	{
		if constexpr (sizeof(T) <= sizeof(u64) / 2)
		{
			return static_cast<T>(value * u64{numerator} / u64{denominator});
		}

		return static_cast<T>(value / denominator * numerator + (value % denominator) * numerator / denominator);
	}

	template <UnsignedInt T>
	constexpr T add_saturate(T addend1, T addend2)
	{
		return static_cast<T>(~addend1) < addend2 ? T{umax} : static_cast<T>(addend1 + addend2);
	}

	template <UnsignedInt T>
	constexpr T sub_saturate(T minuend, T subtrahend)
	{
		return minuend < subtrahend ? T{0} : static_cast<T>(minuend - subtrahend);
	}

	template <UnsignedInt T>
	constexpr T mul_saturate(T factor1, T factor2)
	{
		return factor1 > 0 && T{umax} / factor1 < factor2 ? T{umax} : static_cast<T>(factor1 * factor2);
	}

	inline void trigger_write_page_fault(void* ptr)
	{
#if defined(ARCH_X64) && !defined(_MSC_VER)
		__asm__ volatile("lock orl $0, 0(%0)" :: "r" (ptr));
#elif defined(ARCH_ARM64)
		u32 value = 0;
		u32* u32_ptr = static_cast<u32*>(ptr);
		__asm__ volatile("ldset %w0, %w0, %1" : "+r"(value), "=Q"(*u32_ptr) : "r"(value));
#else
		*static_cast<atomic_t<u32> *>(ptr) += 0;
#endif
	}

	inline void trap()
	{
#ifdef _M_X64
		__debugbreak();
#elif defined(ARCH_X64)
		__asm__ volatile("int3");
#elif defined(ARCH_ARM64)
		__asm__ volatile("brk 0x42");
#else
#error "Missing utils::trap() implementation"
#endif
	}
} // namespace utils

using utils::busy_wait;

#ifdef _MSC_VER
using utils::operator/;
#endif
