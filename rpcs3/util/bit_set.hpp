#pragma once

#include "types.hpp"
#include <bitset>

// Exception friendly std::bitset
template <usz Bits>
struct bit_set
{
public:
	constexpr bit_set() noexcept : m_bitset() {}
	constexpr bit_set(usz val) noexcept : m_bitset(val) {}

	[[nodiscard]] bool test(usz pos, std::source_location src_loc = std::source_location::current()) const
	{
		if (pos >= Bits) [[unlikely]]
			fmt::raw_range_error(src_loc, format_object_simplified(pos), Bits);

		return m_bitset[pos];
	}

	[[nodiscard]] bool test_unsafe(usz pos) const
	{
		return m_bitset[pos];
	}

	bit_set& set(usz pos, bool val = true, std::source_location src_loc = std::source_location::current())
	{
		if (pos >= Bits) [[unlikely]]
			fmt::raw_range_error(src_loc, format_object_simplified(pos), Bits);

		m_bitset[pos] = val;
		return *this;
	}

	bit_set& set_unsafe(usz pos, bool val = true)
	{
		m_bitset[pos] = val;
		return *this;
	}

	bit_set& reset(usz pos, std::source_location src_loc = std::source_location::current())
	{
		if (pos >= Bits) [[unlikely]]
			fmt::raw_range_error(src_loc, format_object_simplified(pos), Bits);

		m_bitset[pos] = false;
		return *this;
	}

	bit_set& reset_unsafe(usz pos)
	{
		m_bitset.reset(pos);
		return *this;
	}

	constexpr bit_set& reset() noexcept
	{
		m_bitset.reset();
		return *this;
	}

	[[nodiscard]] constexpr unsigned long to_ulong() const noexcept requires(Bits <= 32)
	{
		return m_bitset.to_ulong();
	}

	[[nodiscard]] constexpr unsigned long long to_ullong() const noexcept requires(Bits <= 64)
	{
		return m_bitset.to_ullong();
	}

	[[nodiscard]] constexpr bool any() const noexcept
	{
		return m_bitset.any();
	}

	[[nodiscard]] constexpr bool none() const noexcept
	{
		return m_bitset.none();
	}

	[[nodiscard]] constexpr bool all() const noexcept
	{
		return m_bitset.all();
	}

	[[nodiscard]] constexpr usz count() const noexcept
	{
		return m_bitset.count();
	}

	[[nodiscard]] constexpr usz size() const noexcept
	{
		return Bits;
	}

	// Helps us getting the source location when using the [] operator
	struct location_index
	{
		template <typename T> requires std::convertible_to<T, usz>
		location_index(T&& pos, std::source_location src_loc = std::source_location::current())
			: index(static_cast<usz>(std::forward<T>(pos))), loc(src_loc)
		{}

		usz index;
		std::source_location loc;
	};

	[[nodiscard]] constexpr bool operator[](location_index index) const
	{
		return test(index.index, index.loc);
	}

	constexpr bit_set& operator&=(const bit_set& r) noexcept
	{
		m_bitset &= r.m_bitset;
		return *this;
	}

	constexpr bit_set& operator|=(const bit_set& r) noexcept
	{
		m_bitset |= r.m_bitset;
		return *this;
	}

	constexpr bit_set& operator^=(const bit_set& r) noexcept
	{
		m_bitset ^= r.m_bitset;
		return *this;
	}

	constexpr bit_set& operator<<=(usz pos) noexcept
	{
		m_bitset <<= pos;
		return *this;
	}

	constexpr bit_set& operator>>=(usz pos) noexcept
	{
		m_bitset >>= pos;
		return *this;
	}

	[[nodiscard]] constexpr bit_set operator<<(const usz pos) const noexcept
	{
		return m_bitset << pos;
	}

	[[nodiscard]] constexpr bit_set operator>>(const usz pos) const noexcept
	{
		return m_bitset >> pos;
	}

private:
	constexpr bit_set(std::bitset<Bits>&& set) noexcept : m_bitset(set) {}

	std::bitset<Bits> m_bitset;
};
