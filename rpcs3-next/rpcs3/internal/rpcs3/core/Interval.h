#pragma once

template<typename T1, typename T2> struct range_t
{
	T1 _min; // first value
	T2 _max; // second value
};

template<typename T1, typename T2> constexpr range_t<std::decay_t<T1>, std::decay_t<T2>> make_range(T1&& _min, T2&& _max)
{
	return{ std::forward<T1>(_min), std::forward<T2>(_max) };
}

template<typename T1, typename T2, typename T> constexpr bool operator <(const range_t<T1, T2>& range, const T& value)
{
	return range._min < value && range._max < value;
}

template<typename T1, typename T2, typename T> constexpr bool operator <(const T& value, const range_t<T1, T2>& range)
{
	return value < range._min && value < range._max;
}

template<typename T1, typename T2, typename T> constexpr bool operator ==(const range_t<T1, T2>& range, const T& value)
{
	return !(value < range._min) && !(range._max < value);
}

template<typename T1, typename T2, typename T> constexpr bool operator ==(const T& value, const range_t<T1, T2>& range)
{
	return !(value < range._min) && !(range._max < value);
}
