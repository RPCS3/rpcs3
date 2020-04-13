#pragma once

#include "types.h"

namespace utils
{
// Rotate helpers
#if defined(__GNUG__)

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
