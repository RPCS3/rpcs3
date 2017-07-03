#pragma once

/*
This header helps to extend scoped enum types (enum class) in two possible ways:
1) Enabling bitwise operators for enums
2) Advanced bs_t<> template (this converts enum type to another "bitset" enum type)

To enable bitwise operators, enum scope must contain `__bitwise_ops` entry.

enum class flags
{
	__bitwise_ops, // Not essential, but recommended to put it first

	flag1 = 1 << 0,
	flag2 = 1 << 1,
};

Examples:
`flags::flag1 | flags::flag2` - bitwise OR
`flags::flag1 & flags::flag2` - bitwise AND
`flags::flag1 ^ flags::flag2` - bitwise XOR
`~flags::flag1` - bitwise NEG

To enable bs_t<> template, enum scope must contain `__bitset_enum_max` entry.

enum class flagzz : u32
{
	flag1, // Bit indices start from zero
	flag2,

	__bitset_enum_max // It must be the last value
};

Now some operators are enabled for two enum types: `flagzz` and `bs_t<flagzz>`.
These are very different from previously described bitwise operators.

Examples:
`+flagzz::flag1` - unary `+` operator convert flagzz value to bs_t<flagzz>
`flagzz::flag1 + flagzz::flag2` - bitset union
`flagzz::flag1 - flagzz::flag2` - bitset difference
Intersection (&) and symmetric difference (^) is also available.

*/

#include "types.h"

// Helper template
template<typename T>
struct bs_base
{
	// Underlying type
	using under = std::underlying_type_t<T>;

	// Actual bitset type
	enum class type : under
	{
		null = 0, // Empty bitset

		__bitset_set_type = 0 // SFINAE marker
	};

	static constexpr std::size_t bitmax = sizeof(T) * 8;
	static constexpr std::size_t bitsize = static_cast<under>(T::__bitset_enum_max);

	static_assert(std::is_enum<T>::value, "bs_t<> error: invalid type (must be enum)");
	static_assert(!bitsize || bitsize <= bitmax, "bs_t<> error: invalid __bitset_enum_max");

	// Helper function
	static constexpr under shift(T value)
	{
		return static_cast<under>(1) << static_cast<under>(value);
	}

	friend type& operator +=(type& lhs, type rhs)
	{
		reinterpret_cast<under&>(lhs) |= static_cast<under>(rhs);
		return lhs;
	}

	friend type& operator -=(type& lhs, type rhs)
	{
		reinterpret_cast<under&>(lhs) &= ~static_cast<under>(rhs);
		return lhs;
	}

	friend type& operator &=(type& lhs, type rhs)
	{
		reinterpret_cast<under&>(lhs) &= static_cast<under>(rhs);
		return lhs;
	}

	friend type& operator ^=(type& lhs, type rhs)
	{
		reinterpret_cast<under&>(lhs) ^= static_cast<under>(rhs);
		return lhs;
	}

	friend type& operator +=(type& lhs, T rhs)
	{
		reinterpret_cast<under&>(lhs) |= shift(rhs);
		return lhs;
	}

	friend type& operator -=(type& lhs, T rhs)
	{
		reinterpret_cast<under&>(lhs) &= ~shift(rhs);
		return lhs;
	}

	friend type& operator &=(type& lhs, T rhs)
	{
		reinterpret_cast<under&>(lhs) &= shift(rhs);
		return lhs;
	}

	friend type& operator ^=(type& lhs, T rhs)
	{
		reinterpret_cast<under&>(lhs) ^= shift(rhs);
		return lhs;
	}

	friend constexpr type operator +(type lhs, type rhs)
	{
		return static_cast<type>(static_cast<under>(lhs) | static_cast<under>(rhs));
	}

	friend constexpr type operator -(type lhs, type rhs)
	{
		return static_cast<type>(static_cast<under>(lhs) & ~static_cast<under>(rhs));
	}

	friend constexpr type operator &(type lhs, type rhs)
	{
		return static_cast<type>(static_cast<under>(lhs) & static_cast<under>(rhs));
	}

	friend constexpr type operator ^(type lhs, type rhs)
	{
		return static_cast<type>(static_cast<under>(lhs) ^ static_cast<under>(rhs));
	}

	friend constexpr type operator &(type lhs, T rhs)
	{
		return static_cast<type>(static_cast<under>(lhs) & shift(rhs));
	}

	friend constexpr type operator ^(type lhs, T rhs)
	{
		return static_cast<type>(static_cast<under>(lhs) ^ shift(rhs));
	}

	friend constexpr type operator &(T lhs, type rhs)
	{
		return static_cast<type>(shift(lhs) & static_cast<under>(rhs));
	}

	friend constexpr type operator ^(T lhs, type rhs)
	{
		return static_cast<type>(shift(lhs) ^ static_cast<under>(rhs));
	}

	friend constexpr bool operator ==(T lhs, type rhs)
	{
		return shift(lhs) == rhs;
	}

	friend constexpr bool operator ==(type lhs, T rhs)
	{
		return lhs == shift(rhs);
	}

	friend constexpr bool operator !=(T lhs, type rhs)
	{
		return shift(lhs) != rhs;
	}

	friend constexpr bool operator !=(type lhs, T rhs)
	{
		return lhs != shift(rhs);
	}

	friend constexpr bool test(type value)
	{
		return static_cast<under>(value) != 0;
	}

	friend constexpr bool test(type lhs, type rhs)
	{
		return (static_cast<under>(lhs) & static_cast<under>(rhs)) != 0;
	}

	friend constexpr bool test(type lhs, T rhs)
	{
		return (static_cast<under>(lhs) & shift(rhs)) != 0;
	}

	friend constexpr bool test(T lhs, type rhs)
	{
		return (shift(lhs) & static_cast<under>(rhs)) != 0;
	}

	friend bool test_and_set(type& lhs, type rhs)
	{
		return test_and_set(reinterpret_cast<under&>(lhs), static_cast<under>(rhs));
	}

	friend bool test_and_set(type& lhs, T rhs)
	{
		return test_and_set(reinterpret_cast<under&>(lhs), shift(rhs));
	}

	friend bool test_and_reset(type& lhs, type rhs)
	{
		return test_and_reset(reinterpret_cast<under&>(lhs), static_cast<under>(rhs));
	}

	friend bool test_and_reset(type& lhs, T rhs)
	{
		return test_and_reset(reinterpret_cast<under&>(lhs), shift(rhs));
	}

	friend bool test_and_complement(type& lhs, type rhs)
	{
		return test_and_complement(reinterpret_cast<under&>(lhs), static_cast<under>(rhs));
	}

	friend bool test_and_complement(type& lhs, T rhs)
	{
		return test_and_complement(reinterpret_cast<under&>(lhs), shift(rhs));
	}
};

// Bitset type for enum class with available bits [0, T::__bitset_enum_max)
template<typename T>
using bs_t = typename bs_base<T>::type;

// Unary '+' operator: promote plain enum value to bitset value
template<typename T, typename = decltype(T::__bitset_enum_max)>
constexpr bs_t<T> operator +(T value)
{
	return static_cast<bs_t<T>>(bs_base<T>::shift(value));
}

// Binary '+' operator: bitset union
template<typename T, typename = decltype(T::__bitset_enum_max)>
constexpr bs_t<T> operator +(T lhs, T rhs)
{
	return static_cast<bs_t<T>>(bs_base<T>::shift(lhs) | bs_base<T>::shift(rhs));
}

// Binary '+' operator: bitset union
template<typename T, typename = decltype(T::__bitset_enum_max)>
constexpr bs_t<T> operator +(typename bs_base<T>::type lhs, T rhs)
{
	return static_cast<bs_t<T>>(static_cast<typename bs_base<T>::under>(lhs) | bs_base<T>::shift(rhs));
}

// Binary '+' operator: bitset union
template<typename T, typename = decltype(T::__bitset_enum_max)>
constexpr bs_t<T> operator +(T lhs, typename bs_base<T>::type rhs)
{
	return static_cast<bs_t<T>>(bs_base<T>::shift(lhs) | static_cast<typename bs_base<T>::under>(rhs));
}

// Binary '-' operator: bitset difference
template<typename T, typename = decltype(T::__bitset_enum_max)>
constexpr bs_t<T> operator -(T lhs, T rhs)
{
	return static_cast<bs_t<T>>(bs_base<T>::shift(lhs) & ~bs_base<T>::shift(rhs));
}

// Binary '-' operator: bitset difference
template<typename T, typename = decltype(T::__bitset_enum_max)>
constexpr bs_t<T> operator -(typename bs_base<T>::type lhs, T rhs)
{
	return static_cast<bs_t<T>>(static_cast<typename bs_base<T>::under>(lhs) & ~bs_base<T>::shift(rhs));
}

// Binary '-' operator: bitset difference
template<typename T, typename = decltype(T::__bitset_enum_max)>
constexpr bs_t<T> operator -(T lhs, typename bs_base<T>::type rhs)
{
	return static_cast<bs_t<T>>(bs_base<T>::shift(lhs) & ~static_cast<typename bs_base<T>::under>(rhs));
}

template<typename BS, typename T>
struct atomic_add<BS, T, void_t<decltype(T::__bitset_enum_max), std::enable_if_t<std::is_same<BS, bs_t<T>>::value>>>
{
	using under = typename bs_base<T>::under;

	static inline bs_t<T> op1(bs_t<T>& left, T right)
	{
		return static_cast<bs_t<T>>(atomic_storage<under>::fetch_or(reinterpret_cast<under&>(left), bs_base<T>::shift(right)));
	}

	static constexpr auto fetch_op = &op1;

	static inline bs_t<T> op2(bs_t<T>& left, T right)
	{
		return static_cast<bs_t<T>>(atomic_storage<under>::or_fetch(reinterpret_cast<under&>(left), bs_base<T>::shift(right)));
	}

	static constexpr auto op_fetch = &op2;
	static constexpr auto atomic_op = &op2;
};

template<typename BS, typename T>
struct atomic_sub<BS, T, void_t<decltype(T::__bitset_enum_max), std::enable_if_t<std::is_same<BS, bs_t<T>>::value>>>
{
	using under = typename bs_base<T>::under;

	static inline bs_t<T> op1(bs_t<T>& left, T right)
	{
		return static_cast<bs_t<T>>(atomic_storage<under>::fetch_and(reinterpret_cast<under&>(left), ~bs_base<T>::shift(right)));
	}

	static constexpr auto fetch_op = &op1;

	static inline bs_t<T> op2(bs_t<T>& left, T right)
	{
		return static_cast<bs_t<T>>(atomic_storage<under>::and_fetch(reinterpret_cast<under&>(left), ~bs_base<T>::shift(right)));
	}

	static constexpr auto op_fetch = &op2;
	static constexpr auto atomic_op = &op2;
};

template<typename BS, typename T>
struct atomic_and<BS, T, void_t<decltype(T::__bitset_enum_max), std::enable_if_t<std::is_same<BS, bs_t<T>>::value>>>
{
	using under = typename bs_base<T>::under;

	static inline bs_t<T> op1(bs_t<T>& left, T right)
	{
		return static_cast<bs_t<T>>(atomic_storage<under>::fetch_and(reinterpret_cast<under&>(left), bs_base<T>::shift(right)));
	}

	static constexpr auto fetch_op = &op1;

	static inline bs_t<T> op2(bs_t<T>& left, T right)
	{
		return static_cast<bs_t<T>>(atomic_storage<under>::and_fetch(reinterpret_cast<under&>(left), bs_base<T>::shift(right)));
	}

	static constexpr auto op_fetch = &op2;
	static constexpr auto atomic_op = &op2;
};

template<typename BS, typename T>
struct atomic_xor<BS, T, void_t<decltype(T::__bitset_enum_max), std::enable_if_t<std::is_same<BS, bs_t<T>>::value>>>
{
	using under = typename bs_base<T>::under;

	static inline bs_t<T> op1(bs_t<T>& left, T right)
	{
		return static_cast<bs_t<T>>(atomic_storage<under>::fetch_xor(reinterpret_cast<under&>(left), bs_base<T>::shift(right)));
	}

	static constexpr auto fetch_op = &op1;

	static inline bs_t<T> op2(bs_t<T>& left, T right)
	{
		return static_cast<bs_t<T>>(atomic_storage<under>::xor_fetch(reinterpret_cast<under&>(left), bs_base<T>::shift(right)));
	}

	static constexpr auto op_fetch = &op2;
	static constexpr auto atomic_op = &op2;
};

template<typename T>
struct atomic_add<T, T, void_t<decltype(T::__bitset_set_type)>>
{
	using under = std::underlying_type_t<T>;

	static inline T op1(T& left, T right)
	{
		return static_cast<T>(atomic_storage<under>::fetch_or(reinterpret_cast<under&>(left), static_cast<under>(right)));
	}

	static constexpr auto fetch_op = &op1;

	static inline T op2(T& left, T right)
	{
		return static_cast<T>(atomic_storage<under>::or_fetch(reinterpret_cast<under&>(left), static_cast<under>(right)));
	}

	static constexpr auto op_fetch = &op2;
	static constexpr auto atomic_op = &op2;
};

template<typename T>
struct atomic_sub<T, T, void_t<decltype(T::__bitset_set_type)>>
{
	using under = std::underlying_type_t<T>;

	static inline T op1(T& left, T right)
	{
		return static_cast<T>(atomic_storage<under>::fetch_and(reinterpret_cast<under&>(left), ~static_cast<under>(right)));
	}

	static constexpr auto fetch_op = &op1;

	static inline T op2(T& left, T right)
	{
		return static_cast<T>(atomic_storage<under>::and_fetch(reinterpret_cast<under&>(left), ~static_cast<under>(right)));
	}

	static constexpr auto op_fetch = &op2;
	static constexpr auto atomic_op = &op2;
};

template<typename T>
struct atomic_and<T, T, void_t<decltype(T::__bitset_set_type)>>
{
	using under = std::underlying_type_t<T>;

	static inline T op1(T& left, T right)
	{
		return static_cast<T>(atomic_storage<under>::fetch_and(reinterpret_cast<under&>(left), static_cast<under>(right)));
	}

	static constexpr auto fetch_op = &op1;

	static inline T op2(T& left, T right)
	{
		return static_cast<T>(atomic_storage<under>::and_fetch(reinterpret_cast<under&>(left), static_cast<under>(right)));
	}

	static constexpr auto op_fetch = &op2;
	static constexpr auto atomic_op = &op2;
};

template<typename T>
struct atomic_xor<T, T, void_t<decltype(T::__bitset_set_type)>>
{
	using under = std::underlying_type_t<T>;

	static inline T op1(T& left, T right)
	{
		return static_cast<T>(atomic_storage<under>::fetch_xor(reinterpret_cast<under&>(left), static_cast<under>(right)));
	}

	static constexpr auto fetch_op = &op1;

	static inline T op2(T& left, T right)
	{
		return static_cast<T>(atomic_storage<under>::xor_fetch(reinterpret_cast<under&>(left), static_cast<under>(right)));
	}

	static constexpr auto op_fetch = &op2;
	static constexpr auto atomic_op = &op2;
};

template<typename BS, typename T>
struct atomic_test_and_set<BS, T, void_t<decltype(T::__bitset_enum_max), std::enable_if_t<std::is_same<BS, bs_t<T>>::value>>>
{
	using under = typename bs_base<T>::under;

	static inline bool _op(bs_t<T>& left, T value)
	{
		return atomic_storage<under>::bts(reinterpret_cast<under&>(left), static_cast<uint>(static_cast<under>(value)));
	}

	static constexpr auto fetch_op = &_op;
	static constexpr auto op_fetch = &_op;
	static constexpr auto atomic_op = &_op;
};

template<typename BS, typename T>
struct atomic_test_and_reset<BS, T, void_t<decltype(T::__bitset_enum_max), std::enable_if_t<std::is_same<BS, bs_t<T>>::value>>>
{
	using under = typename bs_base<T>::under;

	static inline bool _op(bs_t<T>& left, T value)
	{
		return atomic_storage<under>::btr(reinterpret_cast<under&>(left), static_cast<uint>(static_cast<under>(value)));
	}

	static constexpr auto fetch_op = &_op;
	static constexpr auto op_fetch = &_op;
	static constexpr auto atomic_op = &_op;
};

template<typename BS, typename T>
struct atomic_test_and_complement<BS, T, void_t<decltype(T::__bitset_enum_max), std::enable_if_t<std::is_same<BS, bs_t<T>>::value>>>
{
	using under = typename bs_base<T>::under;

	static inline bool _op(bs_t<T>& left, T value)
	{
		return atomic_storage<under>::btc(reinterpret_cast<under&>(left), static_cast<uint>(static_cast<under>(value)));
	}

	static constexpr auto fetch_op = &_op;
	static constexpr auto op_fetch = &_op;
	static constexpr auto atomic_op = &_op;
};

template<typename T>
struct atomic_test_and_set<T, T, void_t<decltype(T::__bitset_set_type)>>
{
	using under = std::underlying_type_t<T>;

	static inline bool _op(T& left, T value)
	{
		return atomic_storage<under>::test_and_set(reinterpret_cast<under&>(left), static_cast<under>(value));
	}

	static constexpr auto fetch_op = &_op;
	static constexpr auto op_fetch = &_op;
	static constexpr auto atomic_op = &_op;
};

template<typename T>
struct atomic_test_and_reset<T, T, void_t<decltype(T::__bitset_set_type)>>
{
	using under = std::underlying_type_t<T>;

	static inline bool _op(T& left, T value)
	{
		return atomic_storage<under>::test_and_reset(reinterpret_cast<under&>(left), static_cast<under>(value));
	}

	static constexpr auto fetch_op = &_op;
	static constexpr auto op_fetch = &_op;
	static constexpr auto atomic_op = &_op;
};

template<typename T>
struct atomic_test_and_complement<T, T, void_t<decltype(T::__bitset_set_type)>>
{
	using under = std::underlying_type_t<T>;

	static inline bool _op(T& left, T value)
	{
		return atomic_storage<under>::test_and_complement(reinterpret_cast<under&>(left), static_cast<under>(value));
	}

	static constexpr auto fetch_op = &_op;
	static constexpr auto op_fetch = &_op;
	static constexpr auto atomic_op = &_op;
};

// Binary '|' operator: bitwise OR
template<typename T, typename = decltype(T::__bitwise_ops)>
constexpr T operator |(T lhs, T rhs)
{
	return static_cast<T>(std::underlying_type_t<T>(lhs) | std::underlying_type_t<T>(rhs));
}

// Binary '&' operator: bitwise AND
template<typename T, typename = decltype(T::__bitwise_ops)>
constexpr T operator &(T lhs, T rhs)
{
	return static_cast<T>(std::underlying_type_t<T>(lhs) & std::underlying_type_t<T>(rhs));
}

// Binary '^' operator: bitwise XOR
template<typename T, typename = decltype(T::__bitwise_ops)>
constexpr T operator ^(T lhs, T rhs)
{
	return static_cast<T>(std::underlying_type_t<T>(lhs) ^ std::underlying_type_t<T>(rhs));
}

// Unary '~' operator: bitwise NEG
template<typename T, typename = decltype(T::__bitwise_ops)>
constexpr T operator ~(T value)
{
	return static_cast<T>(~std::underlying_type_t<T>(value));
}

// Bitwise OR assignment
template<typename T, typename = decltype(T::__bitwise_ops)>
inline T& operator |=(T& lhs, T rhs)
{
	reinterpret_cast<std::underlying_type_t<T>&>(lhs) |= std::underlying_type_t<T>(rhs);
	return lhs;
}

// Bitwise AND assignment
template<typename T, typename = decltype(T::__bitwise_ops)>
inline T& operator &=(T& lhs, T rhs)
{
	reinterpret_cast<std::underlying_type_t<T>&>(lhs) &= std::underlying_type_t<T>(rhs);
	return lhs;
}

// Bitwise XOR assignment
template<typename T, typename = decltype(T::__bitwise_ops)>
inline T& operator ^=(T& lhs, T rhs)
{
	reinterpret_cast<std::underlying_type_t<T>&>(lhs) ^= std::underlying_type_t<T>(rhs);
	return lhs;
}

template<typename T, typename = decltype(T::__bitwise_ops)>
constexpr bool test(T value)
{
	return std::underlying_type_t<T>(value) != 0;
}

template<typename T, typename = decltype(T::__bitwise_ops)>
constexpr bool test(T lhs, T rhs)
{
	return (std::underlying_type_t<T>(lhs) & std::underlying_type_t<T>(rhs)) != 0;
}

template<typename T, typename = decltype(T::__bitwise_ops)>
inline bool test_and_set(T& lhs, T rhs)
{
	return test_and_set(reinterpret_cast<std::underlying_type_t<T>&>(lhs), std::underlying_type_t<T>(rhs));
}

template<typename T, typename = decltype(T::__bitwise_ops)>
inline bool test_and_reset(T& lhs, T rhs)
{
	return test_and_reset(reinterpret_cast<std::underlying_type_t<T>&>(lhs), std::underlying_type_t<T>(rhs));
}

template<typename T, typename = decltype(T::__bitwise_ops)>
inline bool test_and_complement(T& lhs, T rhs)
{
	return test_and_complement(reinterpret_cast<std::underlying_type_t<T>&>(lhs), std::underlying_type_t<T>(rhs));
}

template<typename T>
struct atomic_or<T, T, void_t<decltype(T::__bitwise_ops), std::enable_if_t<std::is_enum<T>::value>>>
{
	using under = std::underlying_type_t<T>;

	static inline T op1(T& left, T right)
	{
		return static_cast<T>(atomic_storage<under>::fetch_or(reinterpret_cast<under&>(left), static_cast<under>(right)));
	}

	static constexpr auto fetch_op = &op1;

	static inline T op2(T& left, T right)
	{
		return static_cast<T>(atomic_storage<under>::or_fetch(reinterpret_cast<under&>(left), static_cast<under>(right)));
	}

	static constexpr auto op_fetch = &op2;
	static constexpr auto atomic_op = &op2;
};

template<typename T>
struct atomic_and<T, T, void_t<decltype(T::__bitwise_ops), std::enable_if_t<std::is_enum<T>::value>>>
{
	using under = std::underlying_type_t<T>;

	static inline T op1(T& left, T right)
	{
		return static_cast<T>(atomic_storage<under>::fetch_and(reinterpret_cast<under&>(left), static_cast<under>(right)));
	}

	static constexpr auto fetch_op = &op1;

	static inline T op2(T& left, T right)
	{
		return static_cast<T>(atomic_storage<under>::and_fetch(reinterpret_cast<under&>(left), static_cast<under>(right)));
	}

	static constexpr auto op_fetch = &op2;
	static constexpr auto atomic_op = &op2;
};

template<typename T>
struct atomic_xor<T, T, void_t<decltype(T::__bitwise_ops), std::enable_if_t<std::is_enum<T>::value>>>
{
	using under = std::underlying_type_t<T>;

	static inline T op1(T& left, T right)
	{
		return static_cast<T>(atomic_storage<under>::fetch_xor(reinterpret_cast<under&>(left), static_cast<under>(right)));
	}

	static constexpr auto fetch_op = &op1;

	static inline T op2(T& left, T right)
	{
		return static_cast<T>(atomic_storage<under>::xor_fetch(reinterpret_cast<under&>(left), static_cast<under>(right)));
	}

	static constexpr auto op_fetch = &op2;
	static constexpr auto atomic_op = &op2;
};

template<typename T>
struct atomic_test_and_set<T, T, void_t<decltype(T::__bitwise_ops), std::enable_if_t<std::is_enum<T>::value>>>
{
	using under = std::underlying_type_t<T>;

	static inline bool _op(T& left, T value)
	{
		return atomic_storage<under>::test_and_set(reinterpret_cast<under&>(left), static_cast<under>(value));
	}

	static constexpr auto fetch_op = &_op;
	static constexpr auto op_fetch = &_op;
	static constexpr auto atomic_op = &_op;
};

template<typename T>
struct atomic_test_and_reset<T, T, void_t<decltype(T::__bitwise_ops), std::enable_if_t<std::is_enum<T>::value>>>
{
	using under = std::underlying_type_t<T>;

	static inline bool _op(T& left, T value)
	{
		return atomic_storage<under>::test_and_reset(reinterpret_cast<under&>(left), static_cast<under>(value));
	}

	static constexpr auto fetch_op = &_op;
	static constexpr auto op_fetch = &_op;
	static constexpr auto atomic_op = &_op;
};

template<typename T>
struct atomic_test_and_complement<T, T, void_t<decltype(T::__bitwise_ops), std::enable_if_t<std::is_enum<T>::value>>>
{
	using under = std::underlying_type_t<T>;

	static inline bool _op(T& left, T value)
	{
		return atomic_storage<under>::test_and_complement(reinterpret_cast<under&>(left), static_cast<under>(value));
	}

	static constexpr auto fetch_op = &_op;
	static constexpr auto op_fetch = &_op;
	static constexpr auto atomic_op = &_op;
};
