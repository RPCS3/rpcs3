#pragma once
#include "platform.h"

namespace common
{
#if defined(_MSC_VER)

	// Unsigned 128-bit integer implementation
	struct alignas(16) u128
	{
		std::uint64_t lo, hi;

		u128() = default;

		u128(const u128&) = default;

		u128(std::uint64_t l)
			: lo(l)
			, hi(0)
		{
		}

		u128 operator +(const u128& r) const
		{
			u128 value;
			_addcarry_u64(_addcarry_u64(0, r.lo, lo, &value.lo), r.hi, hi, &value.hi);
			return value;
		}

		friend u128 operator +(const u128& l, std::uint64_t r)
		{
			u128 value;
			_addcarry_u64(_addcarry_u64(0, r, l.lo, &value.lo), l.hi, 0, &value.hi);
			return value;
		}

		friend u128 operator +(std::uint64_t l, const u128& r)
		{
			u128 value;
			_addcarry_u64(_addcarry_u64(0, r.lo, l, &value.lo), 0, r.hi, &value.hi);
			return value;
		}

		u128 operator -(const u128& r) const
		{
			u128 value;
			_subborrow_u64(_subborrow_u64(0, r.lo, lo, &value.lo), r.hi, hi, &value.hi);
			return value;
		}

		friend u128 operator -(const u128& l, std::uint64_t r)
		{
			u128 value;
			_subborrow_u64(_subborrow_u64(0, r, l.lo, &value.lo), 0, l.hi, &value.hi);
			return value;
		}

		friend u128 operator -(std::uint64_t l, const u128& r)
		{
			u128 value;
			_subborrow_u64(_subborrow_u64(0, r.lo, l, &value.lo), r.hi, 0, &value.hi);
			return value;
		}

		u128 operator +() const
		{
			return *this;
		}

		u128 operator -() const
		{
			u128 value;
			_subborrow_u64(_subborrow_u64(0, lo, 0, &value.lo), hi, 0, &value.hi);
			return value;
		}

		u128& operator ++()
		{
			_addcarry_u64(_addcarry_u64(0, 1, lo, &lo), 0, hi, &hi);
			return *this;
		}

		u128 operator ++(int)
		{
			u128 value = *this;
			_addcarry_u64(_addcarry_u64(0, 1, lo, &lo), 0, hi, &hi);
			return value;
		}

		u128& operator --()
		{
			_subborrow_u64(_subborrow_u64(0, 1, lo, &lo), 0, hi, &hi);
			return *this;
		}

		u128 operator --(int)
		{
			u128 value = *this;
			_subborrow_u64(_subborrow_u64(0, 1, lo, &lo), 0, hi, &hi);
			return value;
		}

		u128 operator ~() const
		{
			u128 value;
			value.lo = ~lo;
			value.hi = ~hi;
			return value;
		}

		u128 operator &(const u128& r) const
		{
			u128 value;
			value.lo = lo & r.lo;
			value.hi = hi & r.hi;
			return value;
		}

		u128 operator |(const u128& r) const
		{
			u128 value;
			value.lo = lo | r.lo;
			value.hi = hi | r.hi;
			return value;
		}

		u128 operator ^(const u128& r) const
		{
			u128 value;
			value.lo = lo ^ r.lo;
			value.hi = hi ^ r.hi;
			return value;
		}

		u128& operator +=(const u128& r)
		{
			_addcarry_u64(_addcarry_u64(0, r.lo, lo, &lo), r.hi, hi, &hi);
			return *this;
		}

		u128& operator +=(std::uint64_t r)
		{
			_addcarry_u64(_addcarry_u64(0, r, lo, &lo), 0, hi, &hi);
			return *this;
		}

		u128& operator &=(const u128& r)
		{
			lo &= r.lo;
			hi &= r.hi;
			return *this;
		}

		u128& operator |=(const u128& r)
		{
			lo |= r.lo;
			hi |= r.hi;
			return *this;
		}

		u128& operator ^=(const u128& r)
		{
			lo ^= r.lo;
			hi ^= r.hi;
			return *this;
		}
	};
#else
	using u128 = __uint128_t;
#endif

	CHECK_SIZE_ALIGN(u128, 16, 16);
}
