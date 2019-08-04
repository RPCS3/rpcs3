#pragma once

#include "types.h"

namespace utils
{
	inline u32 cntlz32(u32 arg, bool nonzero = false)
	{
#ifdef _MSC_VER
		ulong res;
		return _BitScanReverse(&res, arg) || nonzero ? res ^ 31 : 32;
#elif __LZCNT__
		return _lzcnt_u32(arg);
#else
		return arg || nonzero ? __builtin_clz(arg) : 32;
#endif
	}

	inline u64 cntlz64(u64 arg, bool nonzero = false)
	{
#ifdef _MSC_VER
		ulong res;
		return _BitScanReverse64(&res, arg) || nonzero ? res ^ 63 : 64;
#elif __LZCNT__
		return _lzcnt_u64(arg);
#else
		return arg || nonzero ? __builtin_clzll(arg) : 64;
#endif
	}

	inline u32 cnttz32(u32 arg, bool nonzero = false)
	{
#ifdef _MSC_VER
		ulong res;
		return _BitScanForward(&res, arg) || nonzero ? res : 32;
#elif __BMI__
		return _tzcnt_u32(arg);
#else
		return arg || nonzero ? __builtin_ctz(arg) : 32;
#endif
	}

	inline u64 cnttz64(u64 arg, bool nonzero = false)
	{
#ifdef _MSC_VER
		ulong res;
		return _BitScanForward64(&res, arg) || nonzero ? res : 64;
#elif __BMI__
		return _tzcnt_u64(arg);
#else
		return arg || nonzero ? __builtin_ctzll(arg) : 64;
#endif
	}

	inline u8 popcnt32(u32 arg)
	{
#ifdef _MSC_VER
		const u32 a1 = arg & 0x55555555;
		const u32 a2 = (arg >> 1) & 0x55555555;
		const u32 a3 = a1 + a2;
		const u32 b1 = a3 & 0x33333333;
		const u32 b2 = (a3 >> 2) & 0x33333333;
		const u32 b3 = b1 + b2;
		const u32 c3 = (b3 + (b3 >> 4)) & 0x0f0f0f0f;
		const u32 d3 = c3 + (c3 >> 8);
		return static_cast<u8>(d3 + (d3 >> 16));
#else
		return __builtin_popcount(arg);
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
#endif
} // namespace utils
