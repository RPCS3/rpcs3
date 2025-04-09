#pragma once

#ifdef RSX_USE_STD_MAP
#include <unordered_map>

namespace rsx
{
	template<typename T, typename U>
	using unordered_map = std::unordered_map<T, U>;
}
#else
#include "3rdparty/unordered_dense/include/unordered_dense.h"

namespace rsx
{
	template<typename T, typename U>
	using unordered_map = ankerl::unordered_dense::map<T, U>;
}
#endif
