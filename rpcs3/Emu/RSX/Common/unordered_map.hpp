#pragma once

#ifdef RSX_USE_STD_MAP
#include <unordered_map>

namespace rsx
{
	template<
		typename _Key,
		typename _Tp,
		typename _Hash = std::hash<_Key>,
		typename _Pred = std::equal_to<_Key>>
	>
	using unordered_map = std::unordered_map<
		_Key, _Tp, _Hash, _Pred
	>;
}
#else
#include "3rdparty/unordered_dense/include/unordered_dense.h"

namespace rsx
{
	template <
		typename Key,
		typename T,
		typename Hash = ankerl::unordered_dense::hash<Key>,
		typename KeyEqual = std::equal_to<Key>
	>
	using unordered_map = ankerl::unordered_dense::map<
		Key, T, Hash, KeyEqual
	>;
}
#endif
