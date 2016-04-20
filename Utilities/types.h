#pragma once

#include <new>
#include <typeinfo>
#include <type_traits>
#include <exception>
#include <utility>
#include <cstdint>
#include <cmath>

#include "Platform.h"

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
	enum class byte : std::uint8_t;
}

// Specialization with static constexpr pair<T1, T2> map[] member expected
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

template<typename T, typename = void>
struct unveil;

// TODO: replace with std::void_t when available
namespace void_details
{
	template<class... >
	struct make_void
	{
		using type = void;
	};
}

template<class... T> using void_t = typename void_details::make_void<T...>::type;

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
	std::uint8_t m_value;

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
	std::uint64_t lo, hi;

	u128() = default;

	constexpr u128(std::uint64_t l)
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

	friend u128 operator -(const u128& l, const u128& r)
	{
		u128 value;
		_subborrow_u64(_subborrow_u64(0, r.lo, l.lo, &value.lo), r.hi, l.hi, &value.hi);
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
	std::uint64_t lo;
	std::int64_t hi;

	s128() = default;

	constexpr s128(std::int64_t l)
		: hi(l >> 63)
		, lo(l)
	{
	}

	constexpr s128(std::uint64_t l)
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

// Small bitset for enum class types with available values [0, bitsize).
// T must be either enum type or convertible to (registered with via simple_t<T>).
// Internal representation is single value of type T.
template<typename T>
struct mset
{
	using type = simple_t<T>;
	using under = std::underlying_type_t<type>;

	static constexpr auto bitsize = sizeof(type) * CHAR_BIT;

	mset() = default;

	constexpr mset(type _enum_const)
		: m_value(static_cast<type>(shift(_enum_const)))
	{
	}

	constexpr mset(under raw_value, const std::nothrow_t&)
		: m_value(static_cast<T>(raw_value))
	{
	}

	// Get underlying value
	constexpr under _value() const
	{
		return static_cast<under>(m_value);
	}

	explicit constexpr operator bool() const
	{
		return _value() ? true : false;
	}

	mset& operator +=(mset rhs)
	{
		return *this = { _value() | rhs._value(), std::nothrow };
	}

	mset& operator -=(mset rhs)
	{
		return *this = { _value() & ~rhs._value(), std::nothrow };
	}

	mset& operator &=(mset rhs)
	{
		return *this = { _value() & rhs._value(), std::nothrow };
	}

	mset& operator ^=(mset rhs)
	{
		return *this = { _value() ^ rhs._value(), std::nothrow };
	}

	friend constexpr mset operator +(mset lhs, mset rhs)
	{
		return{ lhs._value() | rhs._value(), std::nothrow };
	}

	friend constexpr mset operator -(mset lhs, mset rhs)
	{
		return{ lhs._value() & ~rhs._value(), std::nothrow };
	}

	friend constexpr mset operator &(mset lhs, mset rhs)
	{
		return{ lhs._value() & rhs._value(), std::nothrow };
	}

	friend constexpr mset operator ^(mset lhs, mset rhs)
	{
		return{ lhs._value() ^ rhs._value(), std::nothrow };
	}

	bool test(mset rhs) const
	{
		const under v = _value();
		const under s = rhs._value();
		return (v & s) != 0;
	}

	bool test_and_set(mset rhs)
	{
		const under v = _value();
		const under s = rhs._value();
		*this = { v | s, std::nothrow };
		return (v & s) != 0;
	}

	bool test_and_reset(mset rhs)
	{
		const under v = _value();
		const under s = rhs._value();
		*this = { v & ~s, std::nothrow };
		return (v & s) != 0;
	}

	bool test_and_complement(mset rhs)
	{
		const under v = _value();
		const under s = rhs._value();
		*this = { v ^ s, std::nothrow };
		return (v & s) != 0;
	}

private:
	[[noreturn]] static under xrange()
	{
		throw std::out_of_range("mset<>: bit out of range");
	}

	static constexpr under shift(const T& value)
	{
		return static_cast<under>(value) < bitsize ? static_cast<under>(1) << static_cast<under>(value) : xrange();
	}

	T m_value;
};

template<typename T, typename RT = T>
constexpr RT to_mset()
{
	return RT{};
}

// Fold enum constants into mset<>
template<typename T = void, typename Arg, typename... Args, typename RT = std::conditional_t<std::is_void<T>::value, mset<Arg>, T>>
constexpr RT to_mset(Arg&& _enum_const, Args&&... args)
{
	return RT{ std::forward<Arg>(_enum_const) } + to_mset<RT>(std::forward<Args>(args)...);
}

template<typename T, typename CT>
struct atomic_add<mset<T>, CT, std::enable_if_t<std::is_enum<T>::value>>
{
	using under = typename mset<T>::under;

	static force_inline mset<T> op1(mset<T>& left, mset<T> right)
	{
		return{ atomic_storage<under>::fetch_or(reinterpret_cast<under&>(left), right._value()), std::nothrow };
	}

	static constexpr auto fetch_op = &op1;

	static force_inline mset<T> op2(mset<T>& left, mset<T> right)
	{
		return{ atomic_storage<under>::or_fetch(reinterpret_cast<under&>(left), right._value()), std::nothrow };
	}

	static constexpr auto op_fetch = &op2;
	static constexpr auto atomic_op = &op2;
};

template<typename T, typename CT>
struct atomic_sub<mset<T>, CT, std::enable_if_t<std::is_enum<T>::value>>
{
	using under = typename mset<T>::under;

	static force_inline mset<T> op1(mset<T>& left, mset<T> right)
	{
		return{ atomic_storage<under>::fetch_and(reinterpret_cast<under&>(left), ~right._value()), std::nothrow };
	}

	static constexpr auto fetch_op = &op1;

	static force_inline mset<T> op2(mset<T>& left, mset<T> right)
	{
		return{ atomic_storage<under>::and_fetch(reinterpret_cast<under&>(left), ~right._value()), std::nothrow };
	}

	static constexpr auto op_fetch = &op2;
	static constexpr auto atomic_op = &op2;
};

template<typename T, typename CT>
struct atomic_and<mset<T>, CT, std::enable_if_t<std::is_enum<T>::value>>
{
	using under = typename mset<T>::under;

	static force_inline mset<T> op1(mset<T>& left, mset<T> right)
	{
		return{ atomic_storage<under>::fetch_and(reinterpret_cast<under&>(left), right._value()), std::nothrow };
	}

	static constexpr auto fetch_op = &op1;

	static force_inline mset<T> op2(mset<T>& left, mset<T> right)
	{
		return{ atomic_storage<under>::and_fetch(reinterpret_cast<under&>(left), right._value()), std::nothrow };
	}

	static constexpr auto op_fetch = &op2;
	static constexpr auto atomic_op = &op2;
};

template<typename T, typename CT>
struct atomic_xor<mset<T>, CT, std::enable_if_t<std::is_enum<T>::value>>
{
	using under = typename mset<T>::under;

	static force_inline mset<T> op1(mset<T>& left, mset<T> right)
	{
		return{ atomic_storage<under>::fetch_xor(reinterpret_cast<under&>(left), right._value()), std::nothrow };
	}

	static constexpr auto fetch_op = &op1;

	static force_inline mset<T> op2(mset<T>& left, mset<T> right)
	{
		return{ atomic_storage<under>::xor_fetch(reinterpret_cast<under&>(left), right._value()), std::nothrow };
	}

	static constexpr auto op_fetch = &op2;
	static constexpr auto atomic_op = &op2;
};

template<typename T>
struct atomic_test_and_set<mset<T>, T, std::enable_if_t<std::is_enum<T>::value>>
{
	using under = typename mset<T>::under;

	static force_inline bool _op(mset<T>& left, const T& value)
	{
		return atomic_storage<under>::bts(reinterpret_cast<under&>(left), static_cast<uint>(value));
	}

	static constexpr auto atomic_op = &_op;
};

template<typename T>
struct atomic_test_and_reset<mset<T>, T, std::enable_if_t<std::is_enum<T>::value>>
{
	using under = typename mset<T>::under;

	static force_inline bool _op(mset<T>& left, const T& value)
	{
		return atomic_storage<under>::btr(reinterpret_cast<under&>(left), static_cast<uint>(value));
	}

	static constexpr auto atomic_op = &_op;
};

template<typename T>
struct atomic_test_and_complement<mset<T>, T, std::enable_if_t<std::is_enum<T>::value>>
{
	using under = typename mset<T>::under;

	static force_inline bool _op(mset<T>& left, const T& value)
	{
		return atomic_storage<under>::btc(reinterpret_cast<under&>(left), static_cast<uint>(value));
	}

	static constexpr auto atomic_op = &_op;
};

template<typename T1, typename T2 = const char*, typename T = T1, typename DT = T2>
T2 bijective_find(const T& left, const DT& def = {})
{
	for (const auto& pair : bijective<T1, T2>::map)
	{
		if (pair.first == left)
		{
			return pair.second;
		}
	}

	return def;
}


template<typename T>
struct size2_base
{
	T width, height;

	constexpr size2_base() : width{}, height{}
	{
	}

	constexpr size2_base(T width, T height) : width{ width }, height{ height }
	{
	}
	
	constexpr size2_base(const size2_base& rhs) : width{ rhs.width }, height{ rhs.height }
	{
	}

	constexpr size2_base operator -(const size2_base& rhs) const
	{
		return{ width - rhs.width, height - rhs.height };
	}
	constexpr size2_base operator -(T rhs) const
	{
		return{ width - rhs, height - rhs };
	}
	constexpr size2_base operator +(const size2_base& rhs) const
	{
		return{ width + rhs.width, height + rhs.height };
	}
	constexpr size2_base operator +(T rhs) const
	{
		return{ width + rhs, height + rhs };
	}
	constexpr size2_base operator /(const size2_base& rhs) const
	{
		return{ width / rhs.width, height / rhs.height };
	}
	constexpr size2_base operator /(T rhs) const
	{
		return{ width / rhs, height / rhs };
	}
	constexpr size2_base operator *(const size2_base& rhs) const
	{
		return{ width * rhs.width, height * rhs.height };
	}
	constexpr size2_base operator *(T rhs) const
	{
		return{ width * rhs, height * rhs };
	}

	size2_base& operator -=(const size2_base& rhs)
	{
		width -= rhs.width;
		height -= rhs.height;
		return *this;
	}
	size2_base& operator -=(T rhs)
	{
		width -= rhs;
		height -= rhs;
		return *this;
	}
	size2_base& operator +=(const size2_base& rhs)
	{
		width += rhs.width;
		height += rhs.height;
		return *this;
	}
	size2_base& operator +=(T rhs)
	{
		width += rhs;
		height += rhs;
		return *this;
	}
	size2_base& operator /=(const size2_base& rhs)
	{
		width /= rhs.width;
		height /= rhs.height;
		return *this;
	}
	size2_base& operator /=(T rhs)
	{
		width /= rhs;
		height /= rhs;
		return *this;
	}
	size2_base& operator *=(const size2_base& rhs)
	{
		width *= rhs.width;
		height *= rhs.height;
		return *this;
	}
	size2_base& operator *=(T rhs)
	{
		width *= rhs;
		height *= rhs;
		return *this;
	}

	constexpr bool operator == (const size2_base& rhs) const
	{
		return width == rhs.width && height == rhs.height;
	}

	constexpr bool operator != (const size2_base& rhs) const
	{
		return width != rhs.width || height != rhs.height;
	}

	template<typename NT>
	constexpr operator size2_base<NT>() const
	{
		return{ (NT)width, (NT)height };
	}
};

template<typename T>
struct position1_base
{
	T x;

	position1_base operator -(const position1_base& rhs) const
	{
		return{ x - rhs.x };
	}
	position1_base operator -(T rhs) const
	{
		return{ x - rhs };
	}
	position1_base operator +(const position1_base& rhs) const
	{
		return{ x + rhs.x };
	}
	position1_base operator +(T rhs) const
	{
		return{ x + rhs };
	}
	template<typename RhsT>
	position1_base operator *(RhsT rhs) const
	{
		return{ T(x * rhs) };
	}
	position1_base operator *(const position1_base& rhs) const
	{
		return{ T(x * rhs.x) };
	}
	template<typename RhsT>
	position1_base operator /(RhsT rhs) const
	{
		return{ x / rhs };
	}
	position1_base operator /(const position1_base& rhs) const
	{
		return{ x / rhs.x };
	}

	position1_base& operator -=(const position1_base& rhs)
	{
		x -= rhs.x;
		return *this;
	}
	position1_base& operator -=(T rhs)
	{
		x -= rhs;
		return *this;
	}
	position1_base& operator +=(const position1_base& rhs)
	{
		x += rhs.x;
		return *this;
	}
	position1_base& operator +=(T rhs)
	{
		x += rhs;
		return *this;
	}

	template<typename RhsT>
	position1_base& operator *=(RhsT rhs) const
	{
		x *= rhs;
		return *this;
	}
	position1_base& operator *=(const position1_base& rhs) const
	{
		x *= rhs.x;
		return *this;
	}
	template<typename RhsT>
	position1_base& operator /=(RhsT rhs) const
	{
		x /= rhs;
		return *this;
	}
	position1_base& operator /=(const position1_base& rhs) const
	{
		x /= rhs.x;
		return *this;
	}

	bool operator ==(const position1_base& rhs) const
	{
		return x == rhs.x;
	}

	bool operator ==(T rhs) const
	{
		return x == rhs;
	}

	bool operator !=(const position1_base& rhs) const
	{
		return !(*this == rhs);
	}

	bool operator !=(T rhs) const
	{
		return !(*this == rhs);
	}

	template<typename NT>
	operator position1_base<NT>() const
	{
		return{ (NT)x };
	}

	double distance(const position1_base& to)
	{
		return abs(x - to.x);
	}
};

template<typename T>
struct position2_base
{
	T x, y;

	constexpr position2_base() : x{}, y{}
	{
	}

	constexpr position2_base(T x, T y) : x{ x }, y{ y }
	{
	}

	constexpr position2_base(const position2_base& rhs) : x{ rhs.x }, y{ rhs.y }
	{
	}

	constexpr bool operator >(const position2_base& rhs) const
	{
		return x > rhs.x && y > rhs.y;
	}
	constexpr bool operator >(T rhs) const
	{
		return x > rhs && y > rhs;
	}
	constexpr bool operator <(const position2_base& rhs) const
	{
		return x < rhs.x && y < rhs.y;
	}
	constexpr bool operator <(T rhs) const
	{
		return x < rhs && y < rhs;
	}
	constexpr bool operator >=(const position2_base& rhs) const
	{
		return x >= rhs.x && y >= rhs.y;
	}
	constexpr bool operator >=(T rhs) const
	{
		return x >= rhs && y >= rhs;
	}
	constexpr bool operator <=(const position2_base& rhs) const
	{
		return x <= rhs.x && y <= rhs.y;
	}
	constexpr bool operator <=(T rhs) const
	{
		return x <= rhs && y <= rhs;
	}

	constexpr position2_base operator -(const position2_base& rhs) const
	{
		return{ x - rhs.x, y - rhs.y };
	}
	constexpr position2_base operator -(T rhs) const
	{
		return{ x - rhs, y - rhs };
	}
	constexpr position2_base operator +(const position2_base& rhs) const
	{
		return{ x + rhs.x, y + rhs.y };
	}
	constexpr position2_base operator +(T rhs) const
	{
		return{ x + rhs, y + rhs };
	}
	template<typename RhsT>
	constexpr position2_base operator *(RhsT rhs) const
	{
		return{ T(x * rhs), T(y * rhs) };
	}
	constexpr position2_base operator *(const position2_base& rhs) const
	{
		return{ T(x * rhs.x),  T(y * rhs.y) };
	}
	template<typename RhsT>
	constexpr position2_base operator /(RhsT rhs) const
	{
		return{ x / rhs, y / rhs };
	}
	constexpr position2_base operator /(const position2_base& rhs) const
	{
		return{ x / rhs.x, y / rhs.y };
	}
	constexpr position2_base operator /(const size2_base<T>& rhs) const
	{
		return{ x / rhs.width, y / rhs.height };
	}

	position2_base& operator -=(const position2_base& rhs)
	{
		x -= rhs.x;
		y -= rhs.y;
		return *this;
	}
	position2_base& operator -=(T rhs)
	{
		x -= rhs;
		y -= rhs;
		return *this;
	}
	position2_base& operator +=(const position2_base& rhs)
	{
		x += rhs.x;
		y += rhs.y;
		return *this;
	}
	position2_base& operator +=(T rhs)
	{
		x += rhs;
		y += rhs;
		return *this;
	}

	template<typename RhsT>
	position2_base& operator *=(RhsT rhs)
	{
		x *= rhs;
		y *= rhs;
		return *this;
	}
	position2_base& operator *=(const position2_base& rhs)
	{
		x *= rhs.x;
		y *= rhs.y;
		return *this;
	}
	template<typename RhsT>
	position2_base& operator /=(RhsT rhs)
	{
		x /= rhs;
		y /= rhs;
		return *this;
	}
	position2_base& operator /=(const position2_base& rhs)
	{
		x /= rhs.x;
		y /= rhs.y;
		return *this;
	}

	constexpr bool operator ==(const position2_base& rhs) const
	{
		return x == rhs.x && y == rhs.y;
	}

	constexpr bool operator ==(T rhs) const
	{
		return x == rhs && y == rhs;
	}

	constexpr bool operator !=(const position2_base& rhs) const
	{
		return !(*this == rhs);
	}

	constexpr bool operator !=(T rhs) const
	{
		return !(*this == rhs);
	}

	template<typename NT>
	constexpr operator position2_base<NT>() const
	{
		return{ (NT)x, (NT)y };
	}

	double distance(const position2_base& to) const
	{
		return std::sqrt(double((x - to.x) * (x - to.x) + (y - to.y) * (y - to.y)));
	}
};

template<typename T>
struct position3_base
{
	T x, y, z;
	/*
	position3_base() : x{}, y{}, z{}
	{
	}

	position3_base(T x, T y, T z) : x{ x }, y{ y }, z{ z }
	{
	}
	*/

	position3_base operator -(const position3_base& rhs) const
	{
		return{ x - rhs.x, y - rhs.y, z - rhs.z };
	}
	position3_base operator -(T rhs) const
	{
		return{ x - rhs, y - rhs, z - rhs };
	}
	position3_base operator +(const position3_base& rhs) const
	{
		return{ x + rhs.x, y + rhs.y, z + rhs.z };
	}
	position3_base operator +(T rhs) const
	{
		return{ x + rhs, y + rhs, z + rhs };
	}

	position3_base& operator -=(const position3_base& rhs)
	{
		x -= rhs.x;
		y -= rhs.y;
		z -= rhs.z;
		return *this;
	}
	position3_base& operator -=(T rhs)
	{
		x -= rhs;
		y -= rhs;
		z -= rhs;
		return *this;
	}
	position3_base& operator +=(const position3_base& rhs)
	{
		x += rhs.x;
		y += rhs.y;
		z += rhs.z;
		return *this;
	}
	position3_base& operator +=(T rhs)
	{
		x += rhs;
		y += rhs;
		z += rhs;
		return *this;
	}

	bool operator ==(const position3_base& rhs) const
	{
		return x == rhs.x && y == rhs.y && z == rhs.z;
	}

	bool operator ==(T rhs) const
	{
		return x == rhs && y == rhs && z == rhs;
	}

	bool operator !=(const position3_base& rhs) const
	{
		return !(*this == rhs);
	}

	bool operator !=(T rhs) const
	{
		return !(*this == rhs);
	}

	template<typename NT>
	operator position3_base<NT>() const
	{
		return{ (NT)x, (NT)y, (NT)z };
	}
};

template<typename T>
struct position4_base
{
	T x, y, z, w;

	constexpr position4_base() : x{}, y{}, z{}, w{}
	{
	}

	constexpr position4_base(T x, T y = {}, T z = {}, T w = {T(1)}) : x{ x }, y{ y }, z{ z }, w{ w }
	{
	}

	constexpr position4_base operator -(const position4_base& rhs) const
	{
		return{ x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w };
	}
	constexpr position4_base operator -(T rhs) const
	{
		return{ x - rhs, y - rhs, z - rhs, w - rhs };
	}
	constexpr position4_base operator +(const position4_base& rhs) const
	{
		return{ x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w };
	}
	constexpr position4_base operator +(T rhs) const
	{
		return{ x + rhs, y + rhs, z + rhs, w + rhs };
	}

	position4_base& operator -=(const position4_base& rhs)
	{
		x -= rhs.x;
		y -= rhs.y;
		z -= rhs.z;
		w -= rhs.w;
		return *this;
	}
	position4_base& operator -=(T rhs)
	{
		x -= rhs;
		y -= rhs;
		z -= rhs;
		w -= rhs;
		return *this;
	}
	position4_base& operator +=(const position4_base& rhs)
	{
		x += rhs.x;
		y += rhs.y;
		z += rhs.z;
		w += rhs.w;
		return *this;
	}
	position4_base& operator +=(T rhs)
	{
		x += rhs;
		y += rhs;
		z += rhs;
		w += rhs;
		return *this;
	}

	constexpr bool operator ==(const position4_base& rhs) const
	{
		return x == rhs.x && y == rhs.y && z == rhs.z && w == rhs.w;
	}

	constexpr bool operator ==(T rhs) const
	{
		return x == rhs && y == rhs && z == rhs && w == rhs;
	}

	constexpr bool operator !=(const position4_base& rhs) const
	{
		return !(*this == rhs);
	}

	constexpr bool operator !=(T rhs) const
	{
		return !(*this == rhs);
	}

	template<typename NT>
	constexpr operator position4_base<NT>() const
	{
		return{ (NT)x, (NT)y, (NT)z, (NT)w };
	}
};

template<typename T>
using position_base = position2_base<T>;

template<typename T>
struct coord_base
{
	union
	{
		position_base<T> position;
		struct { T x, y; };
	};

	union
	{
		size2_base<T> size;
		struct { T width, height; };
	};

	constexpr coord_base() : position{}, size{}
#ifdef _MSC_VER
		//compiler error
		, x{}, y{}, width{}, height{}
#endif
	{
	}

	constexpr coord_base(const position_base<T>& position, const size2_base<T>& size)
		: position{ position }, size{ size }
#ifdef _MSC_VER
		, x{ position.x }, y{ position.y }, width{ size.width }, height{ size.height }
#endif
	{
	}

	constexpr coord_base(T x, T y, T width, T height) : x{ x }, y{ y }, width{ width }, height{ height }
	{
	}

	constexpr bool test(const position_base<T>& position) const
	{
		if (position.x < x || position.x >= x + width)
			return false;

		if (position.y < y || position.y >= y + height)
			return false;

		return true;
	}

	constexpr bool operator == (const coord_base& rhs) const
	{
		return position == rhs.position && size == rhs.size;
	}

	constexpr bool operator != (const coord_base& rhs) const
	{
		return position != rhs.position || size != rhs.size;
	}

	template<typename NT>
	constexpr operator coord_base<NT>() const
	{
		return{ (NT)x, (NT)y, (NT)width, (NT)height };
	}
};

template<typename T>
struct area_base
{
	T x1, x2;
	T y1, y2;

	constexpr area_base() : x1{}, x2{}, y1{}, y2{}
	{
	}

	constexpr area_base(T x1, T y1, T x2, T y2) : x1{ x1 }, x2{ x2 }, y1{ y1 }, y2{ y2 }
	{
	}

	constexpr area_base(const coord_base<T>& coord) : x1{ coord.x }, x2{ coord.x + coord.width }, y1{ coord.y }, y2{ coord.y + coord.height }
	{
	}

	constexpr operator coord_base<T>() const
	{
		return{ x1, y1, x2 - x1, y2 - y1 };
	}

	void flip_vertical()
	{
		std::swap(y1, y2);
	}

	void flip_horizontal()
	{
		std::swap(x1, x2);
	}

	constexpr area_base flipped_vertical() const
	{
		return{ x1, y2, x2, y1 };
	}

	constexpr area_base flipped_horizontal() const
	{
		return{ x2, y1, x1, y2 };
	}

	constexpr bool operator == (const area_base& rhs) const
	{
		return x1 == rhs.x1 && x2 == rhs.x2 && y1 == rhs.y1 && y2 == rhs.y2;
	}

	constexpr bool operator != (const area_base& rhs) const
	{
		return !(*this == rhs);
	}

	constexpr area_base operator - (const size2_base<T>& size) const
	{
		return{ x1 - size.width, y1 - size.height, x2 - size.width, y2 - size.height };
	}
	constexpr area_base operator - (const T& value) const
	{
		return{ x1 - value, y1 - value, x2 - value, y2 - value };
	}
	constexpr area_base operator + (const size2_base<T>& size) const
	{
		return{ x1 + size.width, y1 + size.height, x2 + size.width, y2 + size.height };
	}
	constexpr area_base operator + (const T& value) const
	{
		return{ x1 + value, y1 + value, x2 + value, y2 + value };
	}
	constexpr area_base operator / (const size2_base<T>& size) const
	{
		return{ x1 / size.width, y1 / size.height, x2 / size.width, y2 / size.height };
	}
	constexpr area_base operator / (const T& value) const
	{
		return{ x1 / value, y1 / value, x2 / value, y2 / value };
	}
	constexpr area_base operator * (const size2_base<T>& size) const
	{
		return{ x1 * size.width, y1 * size.height, x2 * size.width, y2 * size.height };
	}
	constexpr area_base operator * (const T& value) const
	{
		return{ x1 * value, y1 * value, x2 * value, y2 * value };
	}

	template<typename NT>
	constexpr operator area_base<NT>() const
	{
		return{(NT)x1, (NT)y1, (NT)x2, (NT)y2};
	}
};

template<typename T>
struct size3_base
{
	T width, height, depth;
	/*
	size3_base() : width{}, height{}, depth{}
	{
	}

	size3_base(T width, T height, T depth) : width{ width }, height{ height }, depth{ depth }
	{
	}
	*/
};

template<typename T>
struct coord3_base
{
	union
	{
		position3_base<T> position;
		struct { T x, y, z; };
	};

	union
	{
		size3_base<T> size;
		struct { T width, height, depth; };
	};

	constexpr coord3_base() : position{}, size{}
	{
	}

	constexpr coord3_base(const position3_base<T>& position, const size3_base<T>& size) : position{ position }, size{ size }
	{
	}

	constexpr coord3_base(T x, T y, T z, T width, T height, T depth) : x{ x }, y{ y }, z{ z }, width{ width }, height{ height }, depth{ depth }
	{
	}

	constexpr bool test(const position3_base<T>& position) const
	{
		if (position.x < x || position.x >= x + width)
			return false;

		if (position.y < y || position.y >= y + height)
			return false;

		if (position.z < z || position.z >= z + depth)
			return false;

		return true;
	}

	template<typename NT>
	constexpr operator coord3_base<NT>() const
	{
		return{ (NT)x, (NT)y, (NT)z, (NT)width, (NT)height, (NT)depth };
	}
};


template<typename T>
struct color4_base
{
	union
	{
		struct
		{
			T r, g, b, a;
		};

		struct
		{
			T x, y, z, w;
		};

		T rgba[4];
		T xyzw[4];
	};

	color4_base()
		: x{}
		, y{}
		, z{}
		, w{ T(1) }
	{
	}

	color4_base(T x, T y = {}, T z = {}, T w = {})
		: x(x)
		, y(y)
		, z(z)
		, w(w)
	{
	}

	bool operator == (const color4_base& rhs) const
	{
		return r == rhs.r && g == rhs.g && b == rhs.b && a == rhs.a;
	}

	bool operator != (const color4_base& rhs) const
	{
		return !(*this == rhs);
	}

	template<typename NT>
	operator color4_base<NT>() const
	{
		return{ (NT)x, (NT)y, (NT)z, (NT)w };
	}
};

template<typename T>
struct color3_base
{
	union
	{
		struct
		{
			T r, g, b;
		};

		struct
		{
			T x, y, z;
		};

		T rgb[3];
		T xyz[3];
	};

	constexpr color3_base(T x = {}, T y = {}, T z = {})
		: x(x)
		, y(y)
		, z(z)
	{
	}

	constexpr bool operator == (const color3_base& rhs) const
	{
		return r == rhs.r && g == rhs.g && b == rhs.b;
	}

	constexpr bool operator != (const color3_base& rhs) const
	{
		return !(*this == rhs);
	}

	template<typename NT>
	constexpr operator color3_base<NT>() const
	{
		return{ (NT)x, (NT)y, (NT)z };
	}
};

template<typename T>
struct color2_base
{
	union
	{
		struct
		{
			T r, g;
		};

		struct
		{
			T x, y;
		};

		T rg[2];
		T xy[2];
	};

	constexpr color2_base(T x = {}, T y = {})
		: x(x)
		, y(y)
	{
	}

	constexpr bool operator == (const color2_base& rhs) const
	{
		return r == rhs.r && g == rhs.g;
	}

	constexpr bool operator != (const color2_base& rhs) const
	{
		return !(*this == rhs);
	}

	template<typename NT>
	constexpr operator color2_base<NT>() const
	{
		return{ (NT)x, (NT)y };
	}
};

template<typename T>
struct color1_base
{
	union
	{
		T r;
		T x;
	};

	constexpr color1_base(T x = {})
		: x(x)
	{
	}

	constexpr bool operator == (const color1_base& rhs) const
	{
		return r == rhs.r;
	}

	constexpr bool operator != (const color1_base& rhs) const
	{
		return !(*this == rhs);
	}

	template<typename NT>
	constexpr operator color1_base<NT>() const
	{
		return{ (NT)x };
	}
};

//specializations
using positioni = position_base<int>;
using positionf = position_base<float>;
using positiond = position_base<double>;

using coordi = coord_base<int>;
using coordf = coord_base<float>;
using coordd = coord_base<double>;

using areai = area_base<int>;
using areaf = area_base<float>;
using aread = area_base<double>;

using position1i = position1_base<int>;
using position1f = position1_base<float>;
using position1d = position1_base<double>;

using position2i = position2_base<int>;
using position2f = position2_base<float>;
using position2d = position2_base<double>;

using position3i = position3_base<int>;
using position3f = position3_base<float>;
using position3d = position3_base<double>;

using position4i = position4_base<int>;
using position4f = position4_base<float>;
using position4d = position4_base<double>;

using size2i = size2_base<int>;
using size2f = size2_base<float>;
using size2d = size2_base<double>;

using sizei = size2i;
using sizef = size2f;
using sized = size2d;

using size3i = size3_base<int>;
using size3f = size3_base<float>;
using size3d = size3_base<double>;

using coord3i = coord3_base<int>;
using coord3f = coord3_base<float>;
using coord3d = coord3_base<double>;

using color4i = color4_base<int>;
using color4f = color4_base<float>;
using color4d = color4_base<double>;

using color3i = color3_base<int>;
using color3f = color3_base<float>;
using color3d = color3_base<double>;

using color2i = color2_base<int>;
using color2f = color2_base<float>;
using color2d = color2_base<double>;

using color1i = color1_base<int>;
using color1f = color1_base<float>;
using color1d = color1_base<double>;
