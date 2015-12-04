#pragma once

// BitField access helper class (N bits from I position), intended to be put in union
template<typename T, u32 I, u32 N> class bf_t
{
	// Checks
	static_assert(I < sizeof(T) * 8, "bf_t<> error: I out of bounds");
	static_assert(N < sizeof(T) * 8, "bf_t<> error: N out of bounds");
	static_assert(I + N <= sizeof(T) * 8, "bf_t<> error: values out of bounds");

	// Underlying data type
	using type = typename std::remove_cv<T>::type;

	// Underlying value type (native endianness)
	using vtype = typename to_ne<type>::type;

	// Mask of size N
	constexpr static vtype s_mask = (static_cast<vtype>(1) << N) - 1;

	// Underlying data member
	type m_data;

	// Conversion operator helper (uses SFINAE)
	template<typename T2, typename = void> struct converter {};

	template<typename T2> struct converter<T2, std::enable_if_t<std::is_unsigned<T2>::value>>
	{
		// Load unsigned value
		static inline T2 convert(const type& data)
		{
			return (data >> I) & s_mask;
		}
	};

	template<typename T2> struct converter<T2, std::enable_if_t<std::is_signed<T2>::value>>
	{
		// Load signed value (sign-extended)
		static inline T2 convert(const type& data)
		{
			return data << (sizeof(T) * 8 - I - N) >> (sizeof(T) * 8 - N);
		}
	};

public:
	// Assignment operator (store bitfield value)
	bf_t& operator =(vtype value)
	{
		m_data = (m_data & ~(s_mask << I)) | (value & s_mask) << I;
		return *this;
	}

	// Conversion operator (load bitfield value)
	operator vtype() const
	{
		return converter<vtype>::convert(m_data);
	}

	// Get raw data with mask applied
	type unshifted() const
	{
		return (m_data & (s_mask << I));
	}

	// Optimized bool conversion
	explicit operator bool() const
	{
		return unshifted() != 0;
	}

	// Postfix increment operator
	vtype operator ++(int)
	{
		vtype result = *this;
		*this = result + 1;
		return result;
	}

	// Prefix increment operator
	bf_t& operator ++()
	{
		return *this = *this + 1;
	}

	// Postfix decrement operator
	vtype operator --(int)
	{
		vtype result = *this;
		*this = result - 1;
		return result;
	}

	// Prefix decrement operator
	bf_t& operator --()
	{
		return *this = *this - 1;
	}

	// Addition assignment operator
	bf_t& operator +=(vtype right)
	{
		return *this = *this + right;
	}

	// Subtraction assignment operator
	bf_t& operator -=(vtype right)
	{
		return *this = *this - right;
	}

	// Multiplication assignment operator
	bf_t& operator *=(vtype right)
	{
		return *this = *this * right;
	}

	// Bitwise AND assignment operator
	bf_t& operator &=(vtype right)
	{
		m_data &= (right & s_mask) << I;
		return *this;
	}

	// Bitwise OR assignment operator
	bf_t& operator |=(vtype right)
	{
		m_data |= (right & s_mask) << I;
		return *this;
	}

	// Bitwise XOR assignment operator
	bf_t& operator ^=(vtype right)
	{
		m_data ^= (right & s_mask) << I;
		return *this;
	}
};

template<typename T, u32 I, u32 N> using bf_be_t = bf_t<be_t<T>, I, N>;

template<typename T, u32 I, u32 N> using bf_le_t = bf_t<le_t<T>, I, N>;
