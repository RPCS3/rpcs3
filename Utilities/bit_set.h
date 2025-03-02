#pragma once

/*
This header implements bs_t<> class for scoped enum types (enum class).
To enable bs_t<>, enum scope must contain `__bitset_enum_max` entry.

enum class flagzz : u32
{
	flag1, // Bit indices start from zero
	flag2,

	__bitset_enum_max // It must be the last value
};

This also enables helper operators for this enum type.

Examples:
`+flagzz::flag1` - unary `+` operator convert flagzz value to bs_t<flagzz>
`flagzz::flag1 + flagzz::flag2` - bitset union
`flagzz::flag1 - flagzz::flag2` - bitset difference
Intersection (&) and symmetric difference (^) is also available.
*/

#include "util/types.hpp"
#include "util/atomic.hpp"
#include "Utilities/StrFmt.h"

template <typename T>
concept BitSetEnum = std::is_enum_v<T> && requires(T x)
{
	T::__bitset_enum_max;
};

template <BitSetEnum T>
class atomic_bs_t;

// Bitset type for enum class with available bits [0, T::__bitset_enum_max)
template <BitSetEnum T>
class bs_t final
{
public:
	// Underlying type
	using under = std::underlying_type_t<T>;

	ENABLE_BITWISE_SERIALIZATION;

private:
	// Underlying value
	under m_data;

	friend class atomic_bs_t<T>;

	// Value constructor
	constexpr explicit bs_t(int, under data) noexcept
		: m_data(data)
	{
	}

public:
	static constexpr usz bitmax = sizeof(T) * 8;
	static constexpr usz bitsize = static_cast<under>(T::__bitset_enum_max);

	static_assert(std::is_enum_v<T>, "bs_t<> error: invalid type (must be enum)");
	static_assert(bitsize <= bitmax, "bs_t<> error: invalid __bitset_enum_max");
	static_assert(bitsize != bitmax || std::is_unsigned_v<under>, "bs_t<> error: invalid __bitset_enum_max (sign bit)");

	// Helper function
	static constexpr under shift(T value)
	{
		return static_cast<under>(1) << static_cast<under>(value);
	}

	bs_t() = default;

	// Construct from a single bit
	constexpr bs_t(T bit) noexcept
		: m_data(shift(bit))
	{
	}

	// Test for empty bitset
	constexpr explicit operator bool() const noexcept
	{
		return m_data != 0;
	}

	// Extract underlying data
	constexpr explicit operator under() const noexcept
	{
		return m_data;
	}

	// Copy
	constexpr bs_t operator +() const
	{
		return *this;
	}

	constexpr bs_t& operator +=(bs_t rhs)
	{
		m_data |= static_cast<under>(rhs);
		return *this;
	}

	constexpr bs_t& operator -=(bs_t rhs)
	{
		m_data &= ~static_cast<under>(rhs);
		return *this;
	}

	constexpr bs_t& operator &=(bs_t rhs)
	{
		m_data &= static_cast<under>(rhs);
		return *this;
	}

	constexpr bs_t& operator ^=(bs_t rhs)
	{
		m_data ^= static_cast<under>(rhs);
		return *this;
	}

	friend constexpr bs_t operator +(bs_t lhs, bs_t rhs)
	{
		return bs_t(0, lhs.m_data | rhs.m_data);
	}

	friend constexpr bs_t operator -(bs_t lhs, bs_t rhs)
	{
		return bs_t(0, lhs.m_data & ~rhs.m_data);
	}

	friend constexpr bs_t operator &(bs_t lhs, bs_t rhs)
	{
		return bs_t(0, lhs.m_data & rhs.m_data);
	}

	friend constexpr bs_t operator ^(bs_t lhs, bs_t rhs)
	{
		return bs_t(0, lhs.m_data ^ rhs.m_data);
	}

	constexpr bool operator ==(bs_t rhs) const noexcept
	{
		return m_data == rhs.m_data;
	}

	constexpr bool test_and_set(T bit)
	{
		bool r = (m_data & shift(bit)) != 0;
		m_data |= shift(bit);
		return r;
	}

	constexpr bool test_and_reset(T bit)
	{
		bool r = (m_data & shift(bit)) != 0;
		m_data &= ~shift(bit);
		return r;
	}

	constexpr bool test_and_complement(T bit)
	{
		bool r = (m_data & shift(bit)) != 0;
		m_data ^= shift(bit);
		return r;
	}

	constexpr bool all_of(bs_t arg) const
	{
		return (m_data & arg.m_data) == arg.m_data;
	}

	constexpr bool none_of(bs_t arg) const
	{
		return (m_data & arg.m_data) == 0;
	}
};

// Unary '+' operator: promote plain enum value to bitset value
template <BitSetEnum T>
constexpr bs_t<T> operator +(T bit)
{
	return bs_t<T>(bit);
}

// Binary '+' operator: bitset union
template <BitSetEnum T, typename U> requires (std::is_constructible_v<bs_t<T>, U>)
constexpr bs_t<T> operator +(T lhs, const U& rhs)
{
	return bs_t<T>(lhs) + bs_t<T>(rhs);
}

// Binary '+' operator: bitset union
template <typename U, BitSetEnum T> requires (std::is_constructible_v<bs_t<T>, U> && !std::is_enum_v<U>)
constexpr bs_t<T> operator +(const U& lhs, T rhs)
{
	return bs_t<T>(lhs) + bs_t<T>(rhs);
}

// Binary '-' operator: bitset difference
template <BitSetEnum T, typename U> requires (std::is_constructible_v<bs_t<T>, U>)
constexpr bs_t<T> operator -(T lhs, const U& rhs)
{
	return bs_t<T>(lhs) - bs_t<T>(rhs);
}

// Binary '-' operator: bitset difference
template <typename U, BitSetEnum T> requires (std::is_constructible_v<bs_t<T>, U> && !std::is_enum_v<U>)
constexpr bs_t<T> operator -(const U& lhs, T rhs)
{
	return bs_t<T>(lhs) - bs_t<T>(rhs);
}

// Binary '&' operator: bitset intersection
template <BitSetEnum T, typename U> requires (std::is_constructible_v<bs_t<T>, U>)
constexpr bs_t<T> operator &(T lhs, const U& rhs)
{
	return bs_t<T>(lhs) & bs_t<T>(rhs);
}

// Binary '&' operator: bitset intersection
template <typename U, BitSetEnum T> requires (std::is_constructible_v<bs_t<T>, U> && !std::is_enum_v<U>)
constexpr bs_t<T> operator &(const U& lhs, T rhs)
{
	return bs_t<T>(lhs) & bs_t<T>(rhs);
}

// Binary '^' operator: bitset symmetric difference
template <BitSetEnum T, typename U> requires (std::is_constructible_v<bs_t<T>, U>)
constexpr bs_t<T> operator ^(T lhs, const U& rhs)
{
	return bs_t<T>(lhs) ^ bs_t<T>(rhs);
}

// Binary '^' operator: bitset symmetric difference
template <typename U, BitSetEnum T> requires (std::is_constructible_v<bs_t<T>, U> && !std::is_enum_v<U>)
constexpr bs_t<T> operator ^(const U& lhs, T rhs)
{
	return bs_t<T>(lhs) ^ bs_t<T>(rhs);
}

// Atomic bitset specialization with optimized operations
template <BitSetEnum T>
class atomic_bs_t : public atomic_t<::bs_t<T>>
{
	// Corresponding bitset type
	using bs_t = ::bs_t<T>;

	// Base class
	using base = atomic_t<::bs_t<T>>;

	// Use underlying m_data
	using base::m_data;

public:
	// Underlying type
	using under = typename bs_t::under;

	atomic_bs_t() = default;

	atomic_bs_t(const atomic_bs_t&) = delete;

	atomic_bs_t& operator =(const atomic_bs_t&) = delete;

	explicit constexpr atomic_bs_t(bs_t value)
		: base(value)
	{
	}

	explicit constexpr atomic_bs_t(T bit)
		: base(bit)
	{
	}

	using base::operator bs_t;

	explicit operator bool() const
	{
		return static_cast<bool>(base::load());
	}

	explicit operator under() const
	{
		return static_cast<under>(base::load());
	}

	bs_t operator +() const
	{
		return base::load();
	}

	bs_t fetch_add(const bs_t& rhs)
	{
		return bs_t(0, atomic_storage<under>::fetch_or(m_data.m_data, rhs.m_data));
	}

	bs_t add_fetch(const bs_t& rhs)
	{
		return bs_t(0, atomic_storage<under>::or_fetch(m_data.m_data, rhs.m_data));
	}

	bs_t operator +=(const bs_t& rhs)
	{
		return add_fetch(rhs);
	}

	bs_t fetch_sub(const bs_t& rhs)
	{
		return bs_t(0, atomic_storage<under>::fetch_and(m_data.m_data, ~rhs.m_data));
	}

	bs_t sub_fetch(const bs_t& rhs)
	{
		return bs_t(0, atomic_storage<under>::and_fetch(m_data.m_data, ~rhs.m_data));
	}

	bs_t operator -=(const bs_t& rhs)
	{
		return sub_fetch(rhs);
	}

	bs_t fetch_and(const bs_t& rhs)
	{
		return bs_t(0, atomic_storage<under>::fetch_and(m_data.m_data, rhs.m_data));
	}

	bs_t and_fetch(const bs_t& rhs)
	{
		return bs_t(0, atomic_storage<under>::and_fetch(m_data.m_data, rhs.m_data));
	}

	bs_t operator &=(const bs_t& rhs)
	{
		return and_fetch(rhs);
	}

	bs_t fetch_xor(const bs_t& rhs)
	{
		return bs_t(0, atomic_storage<under>::fetch_xor(m_data.m_data, rhs.m_data));
	}

	bs_t xor_fetch(const bs_t& rhs)
	{
		return bs_t(0, atomic_storage<under>::xor_fetch(m_data.m_data, rhs.m_data));
	}

	bs_t operator ^=(const bs_t& rhs)
	{
		return xor_fetch(rhs);
	}

	auto fetch_or(const bs_t&) = delete;
	auto or_fetch(const bs_t&) = delete;
	auto operator |=(const bs_t&) = delete;

	bool test_and_set(T rhs)
	{
		return atomic_storage<under>::bts(m_data.m_data, static_cast<uint>(static_cast<under>(rhs)));
	}

	bool test_and_reset(T rhs)
	{
		return atomic_storage<under>::btr(m_data.m_data, static_cast<uint>(static_cast<under>(rhs)));
	}

	bool test_and_invert(T rhs)
	{
		return atomic_storage<under>::btc(m_data.m_data, static_cast<uint>(static_cast<under>(rhs)));
	}

	bool bit_test_set(uint bit) = delete;
	bool bit_test_reset(uint bit) = delete;
	bool bit_test_invert(uint bit) = delete;

	bool all_of(bs_t arg) const
	{
		return base::load().all_of(arg);
	}

	bool none_of(bs_t arg) const
	{
		return base::load().none_of(arg);
	}
};

template <typename T>
struct fmt_unveil<bs_t<T>>
{
	// Format as is
	using type = bs_t<T>;

	static inline u64 get(const bs_t<T>& bitset)
	{
		return static_cast<std::underlying_type_t<T>>(bitset);
	}
};
