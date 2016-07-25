#pragma once

#include <cstdint>
#include <climits>
#include <type_traits>

using schar = signed char;
using uchar = unsigned char;
using ushort = unsigned short;
using uint = unsigned int;
using ulong = unsigned long;
using ullong = unsigned long long;
using llong = long long;

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

using s8 = std::int8_t;
using s16 = std::int16_t;
using s32 = std::int32_t;
using s64 = std::int64_t;

namespace gsl
{
	enum class byte : u8;
}

template<typename T1, typename T2>
struct bijective_pair
{
	T1 v1;
	T2 v2;
};

template<typename T, std::size_t Align = alignof(T), std::size_t Size = sizeof(T)>
struct se_storage;

template<typename T, bool Se = true, std::size_t Align = alignof(T)>
class se_t;

// Specialization with static constexpr bijective_pair<T1, T2> map[] member expected
template<typename T1, typename T2>
struct bijective;

template<typename T, std::size_t Size = sizeof(T)>
struct atomic_storage;

template<typename T1, typename T2, typename = void>
struct atomic_add;

template<typename T1, typename T2, typename = void>
struct atomic_sub;

template<typename T1, typename T2, typename = void>
struct atomic_and;

template<typename T1, typename T2, typename = void>
struct atomic_or;

template<typename T1, typename T2, typename = void>
struct atomic_xor;

template<typename T, typename = void>
struct atomic_pre_inc;

template<typename T, typename = void>
struct atomic_post_inc;

template<typename T, typename = void>
struct atomic_pre_dec;

template<typename T, typename = void>
struct atomic_post_dec;

template<typename T1, typename T2, typename = void>
struct atomic_test_and_set;

template<typename T1, typename T2, typename = void>
struct atomic_test_and_reset;

template<typename T1, typename T2, typename = void>
struct atomic_test_and_complement;

template<typename T>
class atomic_t;

#ifdef _MSC_VER
using std::void_t;
#else
namespace void_details
{
	template<class... >
	struct make_void
	{
		using type = void;
	};
}

template<class... T> using void_t = typename void_details::make_void<T...>::type;
#endif

// Extract T::simple_type if available, remove cv qualifiers
template<typename T, typename = void>
struct simple_type_helper
{
	using type = typename std::remove_cv<T>::type;
};

template<typename T>
struct simple_type_helper<T, void_t<typename T::simple_type>>
{
	using type = typename T::simple_type;
};

template<typename T> using simple_t = typename simple_type_helper<T>::type;

// Bool type equivalent
class b8
{
	u8 m_value;

public:
	b8() = default;

	constexpr b8(bool value)
		: m_value(value)
	{
	}

	constexpr operator bool() const
	{
		return m_value != 0;
	}
};

// Bool wrapper for restricting bool result conversions
struct explicit_bool_t
{
	const bool value;

	constexpr explicit_bool_t(bool value)
		: value(value)
	{
	}

	explicit constexpr operator bool() const
	{
		return value;
	}
};

#ifndef _MSC_VER
using u128 = __uint128_t;
using s128 = __int128_t;
#else

#include "intrin.h"

// Unsigned 128-bit integer implementation (TODO)
struct alignas(16) u128
{
	u64 lo, hi;

	u128() = default;

	constexpr u128(u64 l)
		: lo(l)
		, hi(0)
	{
	}

	friend u128 operator +(const u128& l, const u128& r)
	{
		u128 value;
		_addcarry_u64(_addcarry_u64(0, r.lo, l.lo, &value.lo), r.hi, l.hi, &value.hi);
		return value;
	}

	friend u128 operator +(const u128& l, u64 r)
	{
		u128 value;
		_addcarry_u64(_addcarry_u64(0, r, l.lo, &value.lo), l.hi, 0, &value.hi);
		return value;
	}

	friend u128 operator +(u64 l, const u128& r)
	{
		u128 value;
		_addcarry_u64(_addcarry_u64(0, r.lo, l, &value.lo), 0, r.hi, &value.hi);
		return value;
	}

	friend u128 operator -(const u128& l, const u128& r)
	{
		u128 value;
		_subborrow_u64(_subborrow_u64(0, r.lo, l.lo, &value.lo), r.hi, l.hi, &value.hi);
		return value;
	}

	friend u128 operator -(const u128& l, u64 r)
	{
		u128 value;
		_subborrow_u64(_subborrow_u64(0, r, l.lo, &value.lo), 0, l.hi, &value.hi);
		return value;
	}

	friend u128 operator -(u64 l, const u128& r)
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

	friend u128 operator &(const u128& l, const u128& r)
	{
		u128 value;
		value.lo = l.lo & r.lo;
		value.hi = l.hi & r.hi;
		return value;
	}

	friend u128 operator |(const u128& l, const u128& r)
	{
		u128 value;
		value.lo = l.lo | r.lo;
		value.hi = l.hi | r.hi;
		return value;
	}

	friend u128 operator ^(const u128& l, const u128& r)
	{
		u128 value;
		value.lo = l.lo ^ r.lo;
		value.hi = l.hi ^ r.hi;
		return value;
	}

	u128& operator +=(const u128& r)
	{
		_addcarry_u64(_addcarry_u64(0, r.lo, lo, &lo), r.hi, hi, &hi);
		return *this;
	}

	u128& operator +=(uint64_t r)
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

// Signed 128-bit integer implementation (TODO)
struct alignas(16) s128
{
	u64 lo;
	s64 hi;

	s128() = default;

	constexpr s128(s64 l)
		: hi(l >> 63)
		, lo(l)
	{
	}

	constexpr s128(u64 l)
		: hi(0)
		, lo(l)
	{
	}
};
#endif

namespace std
{
	/* Let's hack. */

	template<>
	struct is_integral<u128> : true_type
	{
	};

	template<>
	struct is_integral<s128> : true_type
	{
	};

	template<>
	struct make_unsigned<u128>
	{
		using type = u128;
	};

	template<>
	struct make_unsigned<s128>
	{
		using type = u128;
	};

	template<>
	struct make_signed<u128>
	{
		using type = s128;
	};

	template<>
	struct make_signed<s128>
	{
		using type = s128;
	};
}

static_assert(std::is_arithmetic<u128>::value && std::is_integral<u128>::value && alignof(u128) == 16 && sizeof(u128) == 16, "Wrong u128 implementation");
static_assert(std::is_arithmetic<s128>::value && std::is_integral<s128>::value && alignof(s128) == 16 && sizeof(s128) == 16, "Wrong s128 implementation");

union alignas(2) f16
{
	u16 _u16;
	u8 _u8[2];

	explicit f16(u16 raw)
	{
		_u16 = raw;
	}

	explicit operator float() const
	{
		// See http://stackoverflow.com/a/26779139
		// The conversion doesn't handle NaN/Inf
		u32 raw = ((_u16 & 0x8000) << 16) | // Sign (just moved)
			(((_u16 & 0x7c00) + 0x1C000) << 13) | // Exponent ( exp - 15 + 127)
			((_u16 & 0x03FF) << 13); // Mantissa
		return (float&)raw;
	}
};

using f32 = float;
using f64 = double;

struct ignore
{
	template<typename T>
	ignore(T)
	{
	}
};

// Simplified hash algorithm for pointers. May be used in std::unordered_(map|set).
template<typename T, std::size_t Align = alignof(T)>
struct pointer_hash
{
	std::size_t operator()(T* ptr) const
	{
		return reinterpret_cast<std::uintptr_t>(ptr) / Align;
	}
};

template<typename T, std::size_t Shift = 0>
struct value_hash
{
	std::size_t operator()(T value) const
	{
		return static_cast<std::size_t>(value) >> Shift;
	}
};

// Contains value of any POD type with fixed size and alignment. TT<> is the type converter applied.
// For example, `simple_t` may be used to remove endianness.
template<template<typename> class TT, std::size_t S, std::size_t A = S>
struct alignas(A) any_pod
{
	std::aligned_storage_t<S, A> data;

	any_pod() = default;

	template<typename T, typename T2 = TT<T>, typename = std::enable_if_t<std::is_pod<T2>::value && sizeof(T2) == S && alignof(T2) <= A>>
	any_pod(const T& value)
	{
		reinterpret_cast<T2&>(data) = value;
	}

	template<typename T, typename T2 = TT<T>, typename = std::enable_if_t<std::is_pod<T2>::value && sizeof(T2) == S && alignof(T2) <= A>>
	T2& as()
	{
		return reinterpret_cast<T2&>(data);
	}

	template<typename T, typename T2 = TT<T>, typename = std::enable_if_t<std::is_pod<T2>::value && sizeof(T2) == S && alignof(T2) <= A>>
	const T2& as() const
	{
		return reinterpret_cast<const T2&>(data);
	}
};

using any16 = any_pod<simple_t, sizeof(u16)>;
using any32 = any_pod<simple_t, sizeof(u32)>;
using any64 = any_pod<simple_t, sizeof(u64)>;

// Allows to define integer convertible to multiple enum types
template<typename T = void, typename... Ts>
struct multicast : multicast<Ts...>
{
	static_assert(std::is_enum<T>::value, "multicast<> error: invalid conversion type (enum type expected)");

	multicast() = default;

	template<typename UT>
	constexpr multicast(const UT& value)
		: multicast<Ts...>(value)
		, m_value{ value } // Forbid narrowing
	{
	}

	constexpr operator T() const
	{
		// Cast to enum type
		return static_cast<T>(m_value);
	}

private:
	std::underlying_type_t<T> m_value;
};

// Recursion terminator
template<>
struct multicast<void>
{
	multicast() = default;

	template<typename UT>
	constexpr multicast(const UT& value)
	{
	}
};

template<typename T1, typename T2 = const char*, typename T = T1, typename DT = T2>
T2 bijective_find(const T& left, const DT& def = {})
{
	for (std::size_t i = 0; i < sizeof(bijective<T1, T2>::map) / sizeof(bijective_pair<T1, T2>); i++)
	{
		if (bijective<T1, T2>::map[i].v1 == left)
		{
			return bijective<T1, T2>::map[i].v2;
		}
	}

	return def;
}

// Formatting helper, type-specific preprocessing for improving safety and functionality
template<typename T, typename = void>
struct unveil
{
	// TODO
	static inline const T& get(const T& arg)
	{
		return arg;
	}
};

template<typename T>
struct unveil<T, void_t<decltype(std::declval<T>().c_str())>>
{
	static inline const char* get(const T& arg)
	{
		return arg.c_str();
	}
};

// Tagged ID type
template<typename T = void, typename ID = u32>
class id_value
{
	// Initial value
	mutable ID m_value{ static_cast<ID>(-1) };

	// Allow access for ID manager
	friend class idm;

	// Update ID
	void operator =(const ID& value) const
	{
		m_value = value;
	}

public:
	constexpr id_value() {}

	// Get the value
	operator ID() const
	{
		return m_value;
	}
};
