#pragma once

#include "util/types.hpp"
#include "util/tsc.hpp"
#include "util/atomic.hpp"
#include <functional>

extern bool g_use_rtm;
extern u64 g_rtm_tx_limit1;

#ifdef _M_X64
#ifdef _MSC_VER
extern "C"
{
	u32 _xbegin();
	void _xend();
	void _mm_pause();
	void _mm_prefetch(const char*, int);
	void _m_prefetchw(const volatile void*);

	uchar _rotl8(uchar, uchar);
	ushort _rotl16(ushort, uchar);
	u64 __popcnt64(u64);

	s64 __mulh(s64, s64);
	u64 __umulh(u64, u64);

	s64 _div128(s64, s64, s64, s64*);
	u64 _udiv128(u64, u64, u64, u64*);
	void __debugbreak();
}
#include <intrin.h>
#else
#include <immintrin.h>
#endif
#endif

namespace utils
{
	// Transaction helper (result = pair of success and op result, or just bool)
	template <typename F, typename R = std::invoke_result_t<F>>
	inline auto tx_start(F op)
	{
#if defined(ARCH_X64)
		uint status = -1;

		for (auto stamp0 = get_tsc(), stamp1 = stamp0; g_use_rtm && stamp1 - stamp0 <= g_rtm_tx_limit1; stamp1 = get_tsc())
		{
#ifndef _MSC_VER
			__asm__ goto ("xbegin %l[retry];" ::: "memory" : retry);
#else
			status = _xbegin();

			if (status != _XBEGIN_STARTED) [[unlikely]]
			{
				goto retry;
			}
#endif

			if constexpr (std::is_void_v<R>)
			{
				std::invoke(op);
#ifndef _MSC_VER
				__asm__ volatile ("xend;" ::: "memory");
#else
				_xend();
#endif
				return true;
			}
			else
			{
				auto result = std::invoke(op);
#ifndef _MSC_VER
				__asm__ volatile ("xend;" ::: "memory");
#else
				_xend();
#endif
				return std::make_pair(true, std::move(result));
			}

			retry:
#ifndef _MSC_VER
			__asm__ volatile ("movl %%eax, %0;" : "=r" (status) :: "memory");
#endif
			if (!status) [[unlikely]]
			{
				break;
			}
		}
#else
		static_cast<void>(op);
#endif

		if constexpr (std::is_void_v<R>)
		{
			return false;
		}
		else
		{
			return std::make_pair(false, R());
		}
	};

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

#ifdef _M_X64
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

#ifdef _M_X64
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

#if defined(_M_X64) && !defined(__clang__)
		return _m_prefetchw(ptr);
#else
		return __builtin_prefetch(ptr, 1, 0);
#endif
	}

	constexpr u8 rol8(u8 x, u8 n)
	{
		if (std::is_constant_evaluated())
		{
			return (x << (n & 7)) | (x >> ((-n & 7)));
		}

#ifdef _MSC_VER
		return _rotl8(x, n);
#elif defined(__clang__)
		return __builtin_rotateleft8(x, n);
#elif defined(ARCH_X64)
		return __builtin_ia32_rolqi(x, n);
#else
		return (x << (n & 7)) | (x >> ((-n & 7)));
#endif
	}

	constexpr u16 rol16(u16 x, u16 n)
	{
		if (std::is_constant_evaluated())
		{
			return (x << (n & 15)) | (x >> ((-n & 15)));
		}

#ifdef _MSC_VER
		return _rotl16(x, static_cast<uchar>(n));
#elif defined(__clang__)
		return __builtin_rotateleft16(x, n);
#elif defined(ARCH_X64)
		return __builtin_ia32_rolhi(x, n);
#else
		return (x << (n & 15)) | (x >> ((-n & 15)));
#endif
	}

	constexpr u32 rol32(u32 x, u32 n)
	{
		if (std::is_constant_evaluated())
		{
			return (x << (n & 31)) | (x >> (((0 - n) & 31)));
		}

#ifdef _MSC_VER
		return _rotl(x, n);
#elif defined(__clang__)
		return __builtin_rotateleft32(x, n);
#else
		return (x << (n & 31)) | (x >> (((0 - n) & 31)));
#endif
	}

	constexpr u64 rol64(u64 x, u64 n)
	{
		if (std::is_constant_evaluated())
		{
			return (x << (n & 63)) | (x >> (((0 - n) & 63)));
		}

#ifdef _MSC_VER
		return _rotl64(x, static_cast<int>(n));
#elif defined(__clang__)
		return __builtin_rotateleft64(x, n);
#else
		return (x << (n & 63)) | (x >> (((0 - n) & 63)));
#endif
	}

	constexpr u32 popcnt64(u64 v)
	{
#if !defined(_MSC_VER) || defined(__SSE4_2__)
		if (std::is_constant_evaluated())
#endif
		{
			v = (v & 0xaaaaaaaaaaaaaaaa) / 2 + (v & 0x5555555555555555);
			v = (v & 0xcccccccccccccccc) / 4 + (v & 0x3333333333333333);
			v = (v & 0xf0f0f0f0f0f0f0f0) / 16 + (v & 0x0f0f0f0f0f0f0f0f);
			v = (v & 0xff00ff00ff00ff00) / 256 + (v & 0x00ff00ff00ff00ff);
			v = ((v & 0xffff0000ffff0000) >> 16) + (v & 0x0000ffff0000ffff);
			return static_cast<u32>((v >> 32) + v);
		}

#if !defined(_MSC_VER) || defined(__SSE4_2__)
#ifdef _MSC_VER
		return static_cast<u32>(__popcnt64(v));
#else
		return __builtin_popcountll(v);
#endif
#endif
	}

	constexpr u32 popcnt128(const u128& v)
	{
#ifdef _MSC_VER
		return popcnt64(v.lo) + popcnt64(v.hi);
#else
		return popcnt64(v) + popcnt64(v >> 64);
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
		if (u64 lo = static_cast<u64>(arg))
			return std::countr_zero<u64>(lo);
		else
			return std::countr_zero<u64>(arg >> 64) + 64;
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
		if (u64 hi = static_cast<u64>(arg >> 64))
			return std::countl_zero<u64>(hi);
		else
			return std::countl_zero<u64>(arg) + 64;
#endif
	}

	inline void pause()
	{
#if defined(ARCH_ARM64)
		__asm__ volatile("yield");
#elif defined(_M_X64)
		_mm_pause();
#elif defined(ARCH_X64)
		__builtin_ia32_pause();
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

#if is_u128_emulated
		if constexpr (sizeof(T) <= sizeof(u128) / 2)
		{
			return static_cast<T>(u128_from_mul(value, numerator) / u64{denominator});
		}
#endif

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
