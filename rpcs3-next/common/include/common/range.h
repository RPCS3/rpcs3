#pragma once
#include <type_traits>

namespace common
{
	template<typename T1, typename T2> struct range
	{
		T1 min; // first value
		T2 max; // second value
	};

	template<typename T1, typename T2> constexpr range<std::decay_t<T1>, std::decay_t<T2>> make_range(T1&& min, T2&& max)
	{
		return{ std::forward<T1>(min), std::forward<T2>(max) };
	}

	template<typename T1, typename T2, typename T> constexpr bool operator <(const range<T1, T2>& range_, const T& value)
	{
		return range_.min < value && range_.max < value;
	}

	template<typename T1, typename T2, typename T> constexpr bool operator <(const T& value, const range<T1, T2>& range_)
	{
		return value < range_.min && value < range_.max;
	}

	template<typename T1, typename T2, typename T> constexpr bool operator ==(const range<T1, T2>& range_, const T& value)
	{
		return !(value < range_.min) && !(range_.max < value);
	}

	template<typename T1, typename T2, typename T> constexpr bool operator ==(const T& value, const range<T1, T2>& range_)
	{
		return !(value < range_.min) && !(range_.max < value);
	}
}
