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

// Formatting helper, type-specific preprocessing for improving safety and functionality
template<typename T, typename = void>
struct fmt_unveil;

struct fmt_type_info;

namespace fmt
{
	template<typename... Args>
	const fmt_type_info* get_type_info();
}

template<typename T, std::size_t Align = alignof(T), std::size_t Size = sizeof(T)>
struct se_storage;

template<typename T, bool Se = true, std::size_t Align = alignof(T)>
class se_t;

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

template<typename T, typename = std::enable_if_t<std::is_integral<T>::value>>
constexpr T align(const T& value, std::uint64_t align)
{
	return static_cast<T>((value + (align - 1)) & ~(align - 1));
}

namespace fmt
{
	[[noreturn]] void raw_error(const char* msg);

	[[noreturn]] void raw_narrow_error(const char* msg, const fmt_type_info* sup, u64 arg);
}

// Narrow cast (throws on failure)
template<typename To = void, typename From, typename = decltype(static_cast<To>(std::declval<From>()))>
inline To narrow(const From& value, const char* msg = nullptr)
{
	// Allow "narrowing to void" and ensure it always fails in this case
	auto&& result = static_cast<std::conditional_t<std::is_void<To>::value, From, To>>(value);
	if (std::is_void<To>::value || static_cast<From>(result) != value)
	{
		// Pack value as formatting argument
		fmt::raw_narrow_error(msg, fmt::get_type_info<typename fmt_unveil<From>::type>(), fmt_unveil<From>::get(value));
	}

	return static_cast<std::conditional_t<std::is_void<To>::value, void, decltype(result)>>(result);
}

// Returns u32 size() for container
template<typename CT, typename = decltype(static_cast<u32>(std::declval<CT>().size()))>
inline u32 size32(const CT& container, const char* msg = nullptr)
{
	return narrow<u32>(container.size(), msg);
}

// Returns u32 size for an array
template<typename T, std::size_t Size>
constexpr u32 size32(const T(&)[Size], const char* msg = nullptr)
{
	return static_cast<u32>(Size);
}

template<typename T1, typename = std::enable_if_t<std::is_integral<T1>::value>>
constexpr bool test(const T1& value)
{
	return value != 0;
}

template<typename T1, typename T2, typename = std::enable_if_t<std::is_integral<T1>::value && std::is_integral<T2>::value>>
constexpr bool test(const T1& lhs, const T2& rhs)
{
	return (lhs & rhs) != 0;
}

template<typename T, typename T2, typename = std::enable_if_t<std::is_integral<T>::value && std::is_integral<T2>::value>>
inline bool test_and_set(T& lhs, const T2& rhs)
{
	const bool result = (lhs & rhs) != 0;
	lhs |= rhs;
	return result;
}

template<typename T, typename T2, typename = std::enable_if_t<std::is_integral<T>::value && std::is_integral<T2>::value>>
inline bool test_and_reset(T& lhs, const T2& rhs)
{
	const bool result = (lhs & rhs) != 0;
	lhs &= ~rhs;
	return result;
}

template<typename T, typename T2, typename = std::enable_if_t<std::is_integral<T>::value && std::is_integral<T2>::value>>
inline bool test_and_complement(T& lhs, const T2& rhs)
{
	const bool result = (lhs & rhs) != 0;
	lhs ^= rhs;
	return result;
}

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

// Allows to define integer convertible to multiple types
template<typename T, T Value, typename T1 = void, typename... Ts>
struct multicast : multicast<T, Value, Ts...>
{
	constexpr multicast() = default;

	// Implicit conversion to desired type
	constexpr operator T1() const
	{
		return static_cast<T1>(Value);
	}
};

// Recursion terminator
template<typename T, T Value>
struct multicast<T, Value, void>
{
	constexpr multicast() = default;
	
	// Explicit conversion to base type
	explicit constexpr operator T() const
	{
		return Value;
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

template<typename T, typename ID>
struct fmt_unveil<id_value<T, ID>>
{
	using type = typename fmt_unveil<ID>::type;

	static inline auto get(const id_value<T, ID>& value)
	{
		return fmt_unveil<ID>::get(value);
	}
};
