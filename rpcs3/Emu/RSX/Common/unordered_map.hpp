#pragma once

#ifdef RSX_USE_STD_MAP
#include <unordered_map>

namespace rsx
{
	template<typename T, typename U>
	using unordered_map = std::unordered_map<T, U>;
}
#else
#include "3rdparty/robin_hood/include/robin_hood.h"

namespace rsx
{
	template<typename T, typename U>
	using unordered_map = ::robin_hood::unordered_map<T, U>;
}
#endif
