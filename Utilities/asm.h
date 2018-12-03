﻿#pragma once

#include "types.h"

namespace utils
{
	inline u32 cntlz32(u32 arg, bool nonzero = false)
	{
#ifdef _MSC_VER
		ulong res;
		return _BitScanReverse(&res, arg) || nonzero ? res ^ 31 : 32;
#else
		return arg || nonzero ? __builtin_clz(arg) : 32;
#endif
	}

	inline u64 cntlz64(u64 arg, bool nonzero = false)
	{
#ifdef _MSC_VER
		ulong res;
		return _BitScanReverse64(&res, arg) || nonzero ? res ^ 63 : 64;
#else
		return arg || nonzero ? __builtin_clzll(arg) : 64;
#endif
	}

	inline u32 cnttz32(u32 arg, bool nonzero = false)
	{
#ifdef _MSC_VER
		ulong res;
		return _BitScanForward(&res, arg) || nonzero ? res : 32;
#else
		return arg || nonzero ? __builtin_ctz(arg) : 32;
#endif
	}

	inline u64 cnttz64(u64 arg, bool nonzero = false)
	{
#ifdef _MSC_VER
		ulong res;
		return _BitScanForward64(&res, arg) || nonzero ? res : 64;
#else
		return arg || nonzero ? __builtin_ctzll(arg) : 64;
#endif
	}

	inline u8 popcnt16(u16 arg)
	{
		const u32 a1 = arg & 0x5555;
		const u32 a2 = (arg >> 1) & 0x5555;
		const u32 a3 = a1 + a2;
		const u32 b1 = a3 & 0x3333;
		const u32 b2 = (a3 >> 2) & 0x3333;
		const u32 b3 = b1 + b2;
		const u32 c1 = b3 & 0x0f0f;
		const u32 c2 = (b3 >> 4) & 0x0f0f;
		const u32 c3 = c1 + c2;
		return static_cast<u8>(c3 + (c3 >> 8));
	}

// Rotate helpers
#if defined(__GNUG__)

	inline u8 rol8(u8 x, u8 n)
	{
		u8 result = x;
		__asm__("rolb %[n], %[result]" : [result] "+g"(result) : [n] "c"(n));
		return result;
	}

	inline u8 ror8(u8 x, u8 n)
	{
		u8 result = x;
		__asm__("rorb %[n], %[result]" : [result] "+g"(result) : [n] "c"(n));
		return result;
	}

	inline u16 rol16(u16 x, u16 n)
	{
		u16 result = x;
		__asm__("rolw %b[n], %[result]" : [result] "+g"(result) : [n] "c"(n));
		return result;
	}

	inline u16 ror16(u16 x, u16 n)
	{
		u16 result = x;
		__asm__("rorw %b[n], %[result]" : [result] "+g"(result) : [n] "c"(n));
		return result;
	}

	inline u32 rol32(u32 x, u32 n)
	{
		u32 result = x;
		__asm__("roll %b[n], %[result]" : [result] "+g"(result) : [n] "c"(n));
		return result;
	}

	inline u32 ror32(u32 x, u32 n)
	{
		u32 result = x;
		__asm__("rorl %b[n], %[result]" : [result] "+g"(result) : [n] "c"(n));
		return result;
	}

	inline u64 rol64(u64 x, u64 n)
	{
		u64 result = x;
		__asm__("rolq %b[n], %[result]" : [result] "+g"(result) : [n] "c"(n));
		return result;
	}

	inline u64 ror64(u64 x, u64 n)
	{
		u64 result = x;
		__asm__("rorq %b[n], %[result]" : [result] "+g"(result) : [n] "c"(n));
		return result;
	}

	inline u64 umulh64(u64 a, u64 b)
	{
		u64 result;
		__asm__("mulq %[b]" : "=d"(result) : [a] "a"(a), [b] "rm"(b));
		return result;
	}

	inline s64 mulh64(s64 a, s64 b)
	{
		s64 result;
		__asm__("imulq %[b]" : "=d"(result) : [a] "a"(a), [b] "rm"(b));
		return result;
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
