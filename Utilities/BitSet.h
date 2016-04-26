#pragma once

#include "types.h"

// Small bitset for enum class types with available values [0, BitSize).
// T must be either enum type or convertible to (registered with via simple_t<T>).
// Internal representation is single value of type T.
template<typename T, std::size_t BitSize = sizeof(T) * CHAR_BIT>
struct bitset_t
{
	using type = simple_t<T>;
	using under = std::underlying_type_t<type>;

	static constexpr auto bitsize = BitSize;

	bitset_t() = default;

	constexpr bitset_t(type _enum_const)
		: m_value(static_cast<type>(shift(_enum_const)))
	{
	}

	constexpr bitset_t(under raw_value, const std::nothrow_t&)
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

	bitset_t& operator +=(bitset_t rhs)
	{
		return *this = { _value() | rhs._value(), std::nothrow };
	}

	bitset_t& operator -=(bitset_t rhs)
	{
		return *this = { _value() & ~rhs._value(), std::nothrow };
	}

	bitset_t& operator &=(bitset_t rhs)
	{
		return *this = { _value() & rhs._value(), std::nothrow };
	}

	bitset_t& operator ^=(bitset_t rhs)
	{
		return *this = { _value() ^ rhs._value(), std::nothrow };
	}

	friend constexpr bitset_t operator +(bitset_t lhs, bitset_t rhs)
	{
		return{ lhs._value() | rhs._value(), std::nothrow };
	}

	friend constexpr bitset_t operator -(bitset_t lhs, bitset_t rhs)
	{
		return{ lhs._value() & ~rhs._value(), std::nothrow };
	}

	friend constexpr bitset_t operator &(bitset_t lhs, bitset_t rhs)
	{
		return{ lhs._value() & rhs._value(), std::nothrow };
	}

	friend constexpr bitset_t operator ^(bitset_t lhs, bitset_t rhs)
	{
		return{ lhs._value() ^ rhs._value(), std::nothrow };
	}

	bool test(bitset_t rhs) const
	{
		const under v = _value();
		const under s = rhs._value();
		return (v & s) != 0;
	}

	bool test_and_set(bitset_t rhs)
	{
		const under v = _value();
		const under s = rhs._value();
		*this = { v | s, std::nothrow };
		return (v & s) != 0;
	}

	bool test_and_reset(bitset_t rhs)
	{
		const under v = _value();
		const under s = rhs._value();
		*this = { v & ~s, std::nothrow };
		return (v & s) != 0;
	}

	bool test_and_complement(bitset_t rhs)
	{
		const under v = _value();
		const under s = rhs._value();
		*this = { v ^ s, std::nothrow };
		return (v & s) != 0;
	}

private:
	static constexpr under shift(const T& value)
	{
		return static_cast<under>(value) < BitSize ? static_cast<under>(1) << static_cast<under>(value) : throw value;
	}

	T m_value;
};

template<typename T, typename RT = T>
constexpr RT make_bitset()
{
	return RT{};
}

// Fold enum constants into bitset_t<> (must be implemented with constexpr initializer_list constructor instead)
template<typename T = void, typename Arg, typename... Args, typename RT = std::conditional_t<std::is_void<T>::value, bitset_t<Arg>, T>>
constexpr RT make_bitset(Arg&& _enum_const, Args&&... args)
{
	return RT{ std::forward<Arg>(_enum_const) } + make_bitset<RT>(std::forward<Args>(args)...);
}

template<typename T, typename CT>
struct atomic_add<bitset_t<T>, CT, std::enable_if_t<std::is_enum<T>::value>>
{
	using under = typename bitset_t<T>::under;

	static inline bitset_t<T> op1(bitset_t<T>& left, bitset_t<T> right)
	{
		return{ atomic_storage<under>::fetch_or(reinterpret_cast<under&>(left), right._value()), std::nothrow };
	}

	static constexpr auto fetch_op = &op1;

	static inline bitset_t<T> op2(bitset_t<T>& left, bitset_t<T> right)
	{
		return{ atomic_storage<under>::or_fetch(reinterpret_cast<under&>(left), right._value()), std::nothrow };
	}

	static constexpr auto op_fetch = &op2;
	static constexpr auto atomic_op = &op2;
};

template<typename T, typename CT>
struct atomic_sub<bitset_t<T>, CT, std::enable_if_t<std::is_enum<T>::value>>
{
	using under = typename bitset_t<T>::under;

	static inline bitset_t<T> op1(bitset_t<T>& left, bitset_t<T> right)
	{
		return{ atomic_storage<under>::fetch_and(reinterpret_cast<under&>(left), ~right._value()), std::nothrow };
	}

	static constexpr auto fetch_op = &op1;

	static inline bitset_t<T> op2(bitset_t<T>& left, bitset_t<T> right)
	{
		return{ atomic_storage<under>::and_fetch(reinterpret_cast<under&>(left), ~right._value()), std::nothrow };
	}

	static constexpr auto op_fetch = &op2;
	static constexpr auto atomic_op = &op2;
};

template<typename T, typename CT>
struct atomic_and<bitset_t<T>, CT, std::enable_if_t<std::is_enum<T>::value>>
{
	using under = typename bitset_t<T>::under;

	static inline bitset_t<T> op1(bitset_t<T>& left, bitset_t<T> right)
	{
		return{ atomic_storage<under>::fetch_and(reinterpret_cast<under&>(left), right._value()), std::nothrow };
	}

	static constexpr auto fetch_op = &op1;

	static inline bitset_t<T> op2(bitset_t<T>& left, bitset_t<T> right)
	{
		return{ atomic_storage<under>::and_fetch(reinterpret_cast<under&>(left), right._value()), std::nothrow };
	}

	static constexpr auto op_fetch = &op2;
	static constexpr auto atomic_op = &op2;
};

template<typename T, typename CT>
struct atomic_xor<bitset_t<T>, CT, std::enable_if_t<std::is_enum<T>::value>>
{
	using under = typename bitset_t<T>::under;

	static inline bitset_t<T> op1(bitset_t<T>& left, bitset_t<T> right)
	{
		return{ atomic_storage<under>::fetch_xor(reinterpret_cast<under&>(left), right._value()), std::nothrow };
	}

	static constexpr auto fetch_op = &op1;

	static inline bitset_t<T> op2(bitset_t<T>& left, bitset_t<T> right)
	{
		return{ atomic_storage<under>::xor_fetch(reinterpret_cast<under&>(left), right._value()), std::nothrow };
	}

	static constexpr auto op_fetch = &op2;
	static constexpr auto atomic_op = &op2;
};

template<typename T>
struct atomic_test_and_set<bitset_t<T>, T, std::enable_if_t<std::is_enum<T>::value>>
{
	using under = typename bitset_t<T>::under;

	static inline bool _op(bitset_t<T>& left, const T& value)
	{
		return atomic_storage<under>::bts(reinterpret_cast<under&>(left), static_cast<uint>(value));
	}

	static constexpr auto atomic_op = &_op;
};

template<typename T>
struct atomic_test_and_reset<bitset_t<T>, T, std::enable_if_t<std::is_enum<T>::value>>
{
	using under = typename bitset_t<T>::under;

	static inline bool _op(bitset_t<T>& left, const T& value)
	{
		return atomic_storage<under>::btr(reinterpret_cast<under&>(left), static_cast<uint>(value));
	}

	static constexpr auto atomic_op = &_op;
};

template<typename T>
struct atomic_test_and_complement<bitset_t<T>, T, std::enable_if_t<std::is_enum<T>::value>>
{
	using under = typename bitset_t<T>::under;

	static inline bool _op(bitset_t<T>& left, const T& value)
	{
		return atomic_storage<under>::btc(reinterpret_cast<under&>(left), static_cast<uint>(value));
	}

	static constexpr auto atomic_op = &_op;
};
