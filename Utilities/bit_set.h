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
class atomic_bs_t;

// Bitset type for enum class with available bits [0, T::__bitset_enum_max)
template <typename T>
class bs_t final
{
public:
	// Underlying type
	using under = std::underlying_type_t<T>;

private:
	// Underlying value
	under m_data;

	friend class atomic_bs_t<T>;

	// Value constructor
	constexpr explicit bs_t(int, under data)
		: m_data(data)
	{
	}

public:
	static constexpr usz bitmax = sizeof(T) * 8;
	static constexpr usz bitsize = static_cast<under>(T::__bitset_enum_max);

	static_assert(std::is_enum<T>::value, "bs_t<> error: invalid type (must be enum)");
	static_assert(bitsize <= bitmax, "bs_t<> error: invalid __bitset_enum_max");
	static_assert(bitsize != bitmax || std::is_unsigned<under>::value, "bs_t<> error: invalid __bitset_enum_max (sign bit)");

	// Helper function
	static constexpr under shift(T value)
	{
		return static_cast<under>(1) << static_cast<under>(value);
	}

	bs_t() = default;

	// Construct from a single bit
	constexpr bs_t(T bit)
		: m_data(shift(bit))
	{
	}

	// Test for empty bitset
	constexpr explicit operator bool() const
	{
		return m_data != 0;
	}

	// Extract underlying data
	constexpr explicit operator under() const
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

	constexpr bs_t operator +(bs_t rhs) const
	{
		return bs_t(0, m_data | rhs.m_data);
	}

	constexpr bs_t operator -(bs_t rhs) const
	{
		return bs_t(0, m_data & ~rhs.m_data);
	}

	constexpr bs_t operator &(bs_t rhs) const
	{
		return bs_t(0, m_data & rhs.m_data);
	}

	constexpr bs_t operator ^(bs_t rhs) const
	{
		return bs_t(0, m_data ^ rhs.m_data);
	}

	constexpr bool operator ==(bs_t rhs) const
	{
		return m_data == rhs.m_data;
	}

	constexpr bool operator !=(bs_t rhs) const
	{
		return m_data != rhs.m_data;
	}

	constexpr bool test(bs_t rhs) const
	{
		return (m_data & rhs.m_data) != 0;
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
};

// Unary '+' operator: promote plain enum value to bitset value
template <typename T, typename = decltype(T::__bitset_enum_max)>
constexpr bs_t<T> operator +(T bit)
{
	return bit;
}

// Binary '+' operator: bitset union
template <typename T, typename = decltype(T::__bitset_enum_max)>
constexpr bs_t<T> operator +(T lhs, T rhs)
{
	return bs_t<T>(lhs) + bs_t<T>(rhs);
}

// Binary '-' operator: bitset difference
template <typename T, typename = decltype(T::__bitset_enum_max)>
constexpr bs_t<T> operator -(T lhs, T rhs)
{
	return bs_t<T>(lhs) - bs_t<T>(rhs);
}

// Binary '&' operator: bitset intersection
template <typename T, typename = decltype(T::__bitset_enum_max)>
constexpr bs_t<T> operator &(T lhs, T rhs)
{
	return bs_t<T>(lhs) & bs_t<T>(rhs);
}

// Binary '^' operator: bitset symmetric difference
template <typename T, typename = decltype(T::__bitset_enum_max)>
constexpr bs_t<T> operator ^(T lhs, T rhs)
{
	return bs_t<T>(lhs) ^ bs_t<T>(rhs);
}

// Atomic bitset specialization with optimized operations
template <typename T>
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
	auto operator ++() = delete;
	auto operator --() = delete;
	auto operator ++(int) = delete;
	auto operator --(int) = delete;

	bs_t operator +(bs_t rhs) const
	{
		return bs_t(0, base::load().m_data | rhs.m_data);
	}

	bs_t operator -(bs_t rhs) const
	{
		return bs_t(0, base::load().m_data & ~rhs.m_data);
	}

	bs_t operator &(bs_t rhs) const
	{
		return bs_t(0, base::load().m_data & rhs.m_data);
	}

	bs_t operator ^(bs_t rhs) const
	{
		return bs_t(0, base::load().m_data ^ rhs.m_data);
	}

	bs_t operator ==(bs_t rhs) const
	{
		return base::load().m_data == rhs.m_data;
	}

	bs_t operator !=(bs_t rhs) const
	{
		return base::load().m_data != rhs.m_data;
	}

	friend bs_t operator +(bs_t lhs, const atomic_bs_t& rhs)
	{
		return bs_t(0, lhs.m_data | rhs.load().m_data);
	}

	friend bs_t operator -(bs_t lhs, const atomic_bs_t& rhs)
	{
		return bs_t(0, lhs.m_data & ~rhs.load().m_data);
	}

	friend bs_t operator &(bs_t lhs, const atomic_bs_t& rhs)
	{
		return bs_t(0, lhs.m_data & rhs.load().m_data);
	}

	friend bs_t operator ^(bs_t lhs, const atomic_bs_t& rhs)
	{
		return bs_t(0, lhs.m_data ^ rhs.load().m_data);
	}

	friend bs_t operator ==(bs_t lhs, const atomic_bs_t& rhs)
	{
		return lhs.m_data == rhs.load().m_data;
	}

	friend bs_t operator !=(bs_t lhs, const atomic_bs_t& rhs)
	{
		return lhs.m_data != rhs.load().m_data;
	}

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
};

template <typename T>
struct fmt_unveil<bs_t<T>, void>
{
	// Format as is
	using type = bs_t<T>;

	static inline u64 get(const bs_t<T>& bitset)
	{
		return static_cast<std::underlying_type_t<T>>(bitset);
	}
};
