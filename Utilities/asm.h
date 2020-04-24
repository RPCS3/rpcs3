﻿#pragma once

#include "types.h"

namespace utils
{
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
