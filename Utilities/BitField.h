#pragma once

#include "util/types.hpp"
#include "Utilities/StrFmt.h"

#ifndef _MSC_VER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
#endif

template <typename T, uint N>
struct bf_base
{
	using type = T;
	using vtype = std::common_type_t<type>;
	using utype = std::make_unsigned_t<vtype>;

	static constexpr bool can_be_packed = N < (sizeof(int) * 8 + (std::is_unsigned_v<vtype> ? 1 : 0)) && sizeof(vtype) > sizeof(int);
	using compact_type = std::conditional_t<can_be_packed, std::conditional_t<std::is_unsigned_v<vtype>, uint, int>, vtype>;

	// Datatype bitsize
	static constexpr uint bitmax = sizeof(T) * 8; static_assert(N - 1 < bitmax, "bf_base<> error: N out of bounds");

	// Field bitsize
	static constexpr uint bitsize = N;

	// All ones mask
	static constexpr utype mask1 = static_cast<utype>(~static_cast<utype>(0));

	// Value mask
	static constexpr utype vmask = mask1 >> (bitmax - bitsize);

protected:
	type m_data;
};

// Bitfield accessor (N bits from I position, 0 is LSB)
template <typename T, uint I, uint N>
struct bf_t : bf_base<T, N>
{
	using type = typename bf_t::type;
	using vtype = typename bf_t::vtype;
	using utype = typename bf_t::utype;
	using compact_type = typename bf_t::compact_type;

	// Field offset
	static constexpr uint bitpos = I; static_assert(bitpos + N <= bf_t::bitmax, "bf_t<> error: I out of bounds");

	// Get bitmask of size N, at I pos
	static constexpr utype data_mask()
	{
		return static_cast<utype>(static_cast<utype>(bf_t::mask1 >> (bf_t::bitmax - bf_t::bitsize)) << bitpos);
	}

	// Bitfield extraction
	static constexpr compact_type extract(const T& data) noexcept
	{
		if constexpr (std::is_signed_v<T>)
		{
			// Load signed value (sign-extended)
			return static_cast<compact_type>(static_cast<vtype>(static_cast<utype>(data) << (bf_t::bitmax - bitpos - N)) >> (bf_t::bitmax - N));
		}
		else
		{
			// Load unsigned value
			return static_cast<compact_type>((static_cast<utype>(data) >> bitpos) & bf_t::vmask);
		}
	}

	// Bitfield insertion
	static constexpr vtype insert(compact_type value)
	{
		return static_cast<vtype>((value & bf_t::vmask) << bitpos);
	}

	// Load bitfield value
	constexpr operator compact_type() const noexcept
	{
		return extract(this->m_data);
	}

	// Load raw data with mask applied
	constexpr T unshifted() const
	{
		return static_cast<T>(this->m_data & data_mask());
	}

	// Optimized bool conversion (must be removed if inappropriate)
	explicit constexpr operator bool() const noexcept
	{
		return unshifted() != 0u;
	}

	// Store bitfield value
	bf_t& operator =(compact_type value) noexcept
	{
		this->m_data = static_cast<vtype>((this->m_data & ~data_mask()) | insert(value));
		return *this;
	}

	compact_type operator ++(int)
	{
		compact_type result = *this;
		*this = static_cast<compact_type>(result + 1u);
		return result;
	}

	bf_t& operator ++()
	{
		return *this = static_cast<compact_type>(*this + 1u);
	}

	compact_type operator --(int)
	{
		compact_type result = *this;
		*this = static_cast<compact_type>(result - 1u);
		return result;
	}

	bf_t& operator --()
	{
		return *this = static_cast<compact_type>(*this - 1u);
	}

	bf_t& operator +=(compact_type right)
	{
		return *this = static_cast<compact_type>(*this + right);
	}

	bf_t& operator -=(compact_type right)
	{
		return *this = static_cast<compact_type>(*this - right);
	}

	bf_t& operator *=(compact_type right)
	{
		return *this = static_cast<compact_type>(*this * right);
	}

	bf_t& operator &=(compact_type right)
	{
		this->m_data &= static_cast<vtype>(((static_cast<utype>(right + 0u) & bf_t::vmask) << bitpos) | ~(bf_t::vmask << bitpos));
		return *this;
	}

	bf_t& operator |=(compact_type right)
	{
		this->m_data |= static_cast<vtype>((static_cast<utype>(right + 0u) & bf_t::vmask) << bitpos);
		return *this;
	}

	bf_t& operator ^=(compact_type right)
	{
		this->m_data ^= static_cast<vtype>((static_cast<utype>(right + 0u) & bf_t::vmask) << bitpos);
		return *this;
	}
};

template <typename T, uint I, uint N>
struct std::common_type<bf_t<T, I, N>, bf_t<T, I, N>> : std::common_type<T> {};

template <typename T, uint I, uint N, typename T2>
struct std::common_type<bf_t<T, I, N>, T2> : std::common_type<T2, std::common_type_t<T>> {};

template <typename T, uint I, uint N, typename T2>
struct std::common_type<T2, bf_t<T, I, N>> : std::common_type<std::common_type_t<T>, T2> {};

// Field pack (concatenated from left to right)
template <typename F = void, typename... Fields>
struct cf_t : bf_base<typename F::type, F::bitsize + cf_t<Fields...>::bitsize>
{
	using type = typename cf_t::type;
	using vtype = typename cf_t::vtype;
	using utype = typename cf_t::utype;
	using compact_type = typename cf_t::compact_type;

	// Get disjunction of all "data" masks of concatenated values
	static constexpr vtype data_mask()
	{
		return static_cast<vtype>(F::data_mask() | cf_t<Fields...>::data_mask());
	}

	// Extract all bitfields and concatenate
	static constexpr compact_type extract(const type& data)
	{
		return static_cast<compact_type>(static_cast<utype>(F::extract(data)) << cf_t<Fields...>::bitsize | cf_t<Fields...>::extract(data));
	}

	// Split bitfields and insert them
	static constexpr vtype insert(compact_type value)
	{
		return static_cast<vtype>(F::insert(value >> cf_t<Fields...>::bitsize) | cf_t<Fields...>::insert(value));
	}

	// Load value
	constexpr operator compact_type() const noexcept
	{
		return extract(this->m_data);
	}

	// Store value
	cf_t& operator =(compact_type value) noexcept
	{
		this->m_data = (this->m_data & ~data_mask()) | insert(value);
		return *this;
	}
};

// Empty field pack (recursion terminator)
template <>
struct cf_t<void>
{
	static constexpr uint bitsize = 0;

	static constexpr uint data_mask()
	{
		return 0;
	}

	template <typename T>
	static constexpr auto extract(const T&) -> decltype(+T())
	{
		return 0;
	}

	template <typename T>
	static constexpr T insert(T /*value*/)
	{
		return 0;
	}
};

// Fixed field (provides constant values in field pack)
template <typename T, T V, uint N>
struct ff_t : bf_base<T, N>
{
	using type = typename ff_t::type;
	using vtype = typename ff_t::vtype;

	// Return constant value
	static constexpr vtype extract(const type&)
	{
		static_assert((V & ff_t::vmask) == V, "ff_t<> error: V out of bounds");
		return V;
	}

	// Get value
	constexpr operator vtype() const noexcept
	{
		return V;
	}
};

#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif

template <typename T, uint I, uint N>
struct fmt_unveil<bf_t<T, I, N>>
{
	using type = typename fmt_unveil<std::common_type_t<T>>::type;

	static inline auto get(const bf_t<T, I, N>& bf)
	{
		return fmt_unveil<type>::get(bf);
	}
};

template <typename F, typename... Fields>
struct fmt_unveil<cf_t<F, Fields...>>
{
	using type = typename fmt_unveil<std::common_type_t<typename F::type>>::type;

	static inline auto get(const cf_t<F, Fields...>& cf)
	{
		return fmt_unveil<type>::get(cf);
	}
};

template <typename T, T V, uint N>
struct fmt_unveil<ff_t<T, V, N>>
{
	using type = typename fmt_unveil<std::common_type_t<T>>::type;

	static inline auto get(const ff_t<T, V, N>& ff)
	{
		return fmt_unveil<type>::get(ff);
	}
};
