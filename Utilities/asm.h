#pragma once

#include "types.h"

namespace utils
{
	// Transaction helper (Max = max attempts) (result = pair of success and op result, which is only meaningful on success)
	template <uint Max = 10, typename F, typename R = std::invoke_result_t<F>>
	inline auto tx_start(F op)
	{
		uint status = -1;

		for (uint i = 0; i < Max; i++)
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
			if (!(status & _XABORT_RETRY)) [[unlikely]]
			{
				// In order to abort transaction, tx_abort() can be used, so there is no need for "special" return value
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

	// Special function to abort transaction
	[[noreturn]] FORCE_INLINE void tx_abort()
	{
#ifndef _MSC_VER
		__asm__ volatile ("xabort $0;" ::: "memory");
		__builtin_unreachable();
#else
		_xabort(0);
		__assume(0);
#endif
	}


// Rotate helpers
#if defined(__GNUG__)

	inline u8 rol8(u8 x, u8 n)
	{
#if __has_builtin(__builtin_rotateleft8)
		return __builtin_rotateleft8(x, n);
#else
		u8 result = x;
		__asm__("rolb %[n], %[result]" : [result] "+g"(result) : [n] "c"(n));
		return result;
#endif
	}

	inline u8 ror8(u8 x, u8 n)
	{
#if __has_builtin(__builtin_rotateright8)
		return __builtin_rotateright8(x, n);
#else
		u8 result = x;
		__asm__("rorb %[n], %[result]" : [result] "+g"(result) : [n] "c"(n));
		return result;
#endif
	}

	inline u16 rol16(u16 x, u16 n)
	{
#if __has_builtin(__builtin_rotateleft16)
		return __builtin_rotateleft16(x, n);
#else
		u16 result = x;
		__asm__("rolw %b[n], %[result]" : [result] "+g"(result) : [n] "c"(n));
		return result;
#endif
	}

	inline u16 ror16(u16 x, u16 n)
	{
#if __has_builtin(__builtin_rotateright16)
		return __builtin_rotateright16(x, n);
#else
		u16 result = x;
		__asm__("rorw %b[n], %[result]" : [result] "+g"(result) : [n] "c"(n));
		return result;
#endif
	}

	inline u32 rol32(u32 x, u32 n)
	{
#if __has_builtin(__builtin_rotateleft32)
		return __builtin_rotateleft32(x, n);
#else
		u32 result = x;
		__asm__("roll %b[n], %[result]" : [result] "+g"(result) : [n] "c"(n));
		return result;
#endif
	}

	inline u32 ror32(u32 x, u32 n)
	{
#if __has_builtin(__builtin_rotateright32)
		return __builtin_rotateright32(x, n);
#else
		u32 result = x;
		__asm__("rorl %b[n], %[result]" : [result] "+g"(result) : [n] "c"(n));
		return result;
#endif
	}

	inline u64 rol64(u64 x, u64 n)
	{
#if __has_builtin(__builtin_rotateleft64)
		return __builtin_rotateleft64(x, n);
#else
		u64 result = x;
		__asm__("rolq %b[n], %[result]" : [result] "+g"(result) : [n] "c"(n));
		return result;
#endif
	}

	inline u64 ror64(u64 x, u64 n)
	{
#if __has_builtin(__builtin_rotateright64)
		return __builtin_rotateright64(x, n);
#else
		u64 result = x;
		__asm__("rorq %b[n], %[result]" : [result] "+g"(result) : [n] "c"(n));
		return result;
#endif
	}

	constexpr u64 umulh64(u64 a, u64 b)
	{
		const __uint128_t x = a;
		const __uint128_t y = b;
		return (x * y) >> 64;
	}

	constexpr s64 mulh64(s64 a, s64 b)
	{
		const __int128_t x = a;
		const __int128_t y = b;
		return (x * y) >> 64;
	}

	constexpr s64 div128(s64 high, s64 low, s64 divisor, s64* remainder = nullptr)
	{
		const __int128_t x = (__uint128_t{u64(high)} << 64) | u64(low);
		const __int128_t r = x / divisor;

		if (remainder)
		{
			*remainder = x % divisor;
		}

		return r;
	}

	constexpr u64 udiv128(u64 high, u64 low, u64 divisor, u64* remainder = nullptr)
	{
		const __uint128_t x = (__uint128_t{high} << 64) | low;
		const __uint128_t r = x / divisor;

		if (remainder)
		{
			*remainder = x % divisor;
		}

		return r;
	}

#elif defined(_MSC_VER)
	inline u8 rol8(u8 x, u8 n)
	{
		return _rotl8(x, n);
	}

	inline u8 ror8(u8 x, u8 n)
	{
		return _rotr8(x, n);
	}

	inline u16 rol16(u16 x, u16 n)
	{
		return _rotl16(x, (u8)n);
	}

	inline u16 ror16(u16 x, u16 n)
	{
		return _rotr16(x, (u8)n);
	}

	inline u32 rol32(u32 x, u32 n)
	{
		return _rotl(x, (int)n);
	}

	inline u32 ror32(u32 x, u32 n)
	{
		return _rotr(x, (int)n);
	}

	inline u64 rol64(u64 x, u64 n)
	{
		return _rotl64(x, (int)n);
	}

	inline u64 ror64(u64 x, u64 n)
	{
		return _rotr64(x, (int)n);
	}

	inline u64 umulh64(u64 x, u64 y)
	{
		return __umulh(x, y);
	}

	inline s64 mulh64(s64 x, s64 y)
	{
		return __mulh(x, y);
	}

	inline s64 div128(s64 high, s64 low, s64 divisor, s64* remainder = nullptr)
	{
		s64 rem;
		s64 r = _div128(high, low, divisor, &rem);

		if (remainder)
		{
			*remainder = rem;
		}

		return r;
	}

	inline u64 udiv128(u64 high, u64 low, u64 divisor, u64* remainder = nullptr)
	{
		u64 rem;
		u64 r = _udiv128(high, low, divisor, &rem);

		if (remainder)
		{
			*remainder = rem;
		}

		return r;
	}
#endif
} // namespace utils
