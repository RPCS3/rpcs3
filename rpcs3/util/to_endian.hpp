#pragma once

#include "util/types.hpp"
#include "util/endian.hpp"

union v128;

// Type converter: converts native endianness arithmetic/enum types to appropriate se_t<> type
template <typename T, bool Se>
struct to_se
{
	template <typename T2>
	struct to_se_
	{
		using type = T2;
	};

	template <typename T2>
		requires std::is_arithmetic_v<T2> || std::is_enum_v<T2>
	struct to_se_<T2>
	{
		using type = std::conditional_t<(sizeof(T2) > 1), se_t<T2, Se>, T2>;
	};

	// Convert arithmetic and enum types
	using type = typename to_se_<T>::type;
};

template <bool Se>
struct to_se<v128, Se>
{
	using type = se_t<v128, Se, 16>;
};

template <bool Se>
struct to_se<u128, Se>
{
	using type = se_t<u128, Se>;
};

template <bool Se>
struct to_se<s128, Se>
{
	using type = se_t<s128, Se>;
};

template <typename T, bool Se>
	requires(!std::is_array_v<T>)
struct to_se<const T, Se>
{
	// Move const qualifier
	using type = const typename to_se<T, Se>::type;
};

template <typename T, bool Se>
	requires(!std::is_array_v<T> && !std::is_const_v<T>)
struct to_se<volatile T, Se>
{
	// Move volatile qualifier
	using type = volatile typename to_se<T, Se>::type;
};

template <typename T, bool Se>
struct to_se<T[], Se>
{
	// Move array qualifier
	using type = typename to_se<T, Se>::type[];
};

template <typename T, bool Se, usz N>
struct to_se<T[N], Se>
{
	// Move array qualifier
	using type = typename to_se<T, Se>::type[N];
};

// BE/LE aliases for to_se<>
template <typename T>
using to_be_t = typename to_se<T, std::endian::little == std::endian::native>::type;
template <typename T>
using to_le_t = typename to_se<T, std::endian::big == std::endian::native>::type;
