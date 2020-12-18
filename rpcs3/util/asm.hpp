#pragma once

#include "util/types.hpp"

extern bool g_use_rtm;
extern u64 g_rtm_tx_limit1;

namespace utils
{
	// Transaction helper (result = pair of success and op result, or just bool)
	template <typename F, typename R = std::invoke_result_t<F>>
	inline auto tx_start(F op)
	{
		uint status = -1;

		for (auto stamp0 = __rdtsc(), stamp1 = stamp0; g_use_rtm && stamp1 - stamp0 <= g_rtm_tx_limit1; stamp1 = __rdtsc())
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

#ifdef _MSC_VER
		return _mm_prefetch(reinterpret_cast<const char*>(ptr), _MM_HINT_T1);
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

#ifdef _MSC_VER
		return _mm_prefetch(reinterpret_cast<const char*>(ptr), _MM_HINT_T0);
#else
		return __builtin_prefetch(ptr, 0, 3);
#endif
	}

	constexpr void prefetch_write(void* ptr)
	{
		if (std::is_constant_evaluated())
		{
			return;
		}

		return _m_prefetchw(ptr);
	}

	constexpr u8 rol8(u8 x, u8 n)
	{
		if (std::is_constant_evaluated())
		{
			return (x << (n & 7)) | (x >> ((-n & 7)));
		}

#ifdef _MSC_VER
		return _rotl8(x, n);
#else
		return __rolb(x, n);
#endif
	}

	constexpr u16 rol16(u16 x, u16 n)
	{
		if (std::is_constant_evaluated())
		{
			return (x << (n & 15)) | (x >> ((-n & 15)));
		}

#ifdef _MSC_VER
		return _rotl16(x, n);
#else
		return __rolw(x, n);
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
#else
		return __rold(x, n);
#endif
	}

	constexpr u32 ror32(u32 x, u32 n)
	{
		if (std::is_constant_evaluated())
		{
			return (x >> (n & 31)) | (x << (((0 - n) & 31)));
		}

#ifdef _MSC_VER
		return _rotr(x, n);
#else
		return __rord(x, n);
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
#else
		return __rolq(x, static_cast<int>(n));
#endif
	}

	constexpr u64 umulh64(u64 x, u64 y)
	{
#ifdef _MSC_VER
		if (std::is_constant_evaluated())
		{
			u128 a = (u32)x * (u64)(u32)y;
			u128 b = (x >> 32) * (u32)y;
			u128 c = (u32)x * (y >> 32);
			u128 d = (x >> 32) * (y >> 32);
			a += (b << 32);
			a += (c << 32);
			a.hi += d.lo;
			return a.hi;
		}

		return __umulh(x, y);
#else
		return (u128{x} * u128{y}) >> 64;
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

	// Synchronization helper (cache-friendly busy waiting)
	inline void busy_wait(std::size_t cycles = 3000)
	{
		const u64 start = __rdtsc();
		do _mm_pause();
		while (__rdtsc() - start < cycles);
	}
} // namespace utils

using utils::busy_wait;
