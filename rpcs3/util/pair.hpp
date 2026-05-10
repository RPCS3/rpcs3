#pragma once

#include <type_traits>

namespace utils
{
	template <typename T1, typename T2>
		requires std::is_trivially_copyable_v<T1> && std::is_trivially_destructible_v<T1> &&
		         std::is_trivially_copyable_v<T2> && std::is_trivially_destructible_v<T2>
	struct pair
	{
		T1 first {};
		T2 second {};
	};
}
