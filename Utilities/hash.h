#pragma once

#include "util/types.hpp"

namespace rpcs3
{
	template<typename T>
	static size_t hash_base(T value)
	{
		return static_cast<size_t>(value);
	}

	template<typename T, typename U>
	static size_t hash_struct_base(const T& value)
	{
		// FNV 64-bit
		size_t result = 14695981039346656037ull;
		const U *bits = reinterpret_cast<const U*>(&value);

		for (size_t n = 0; n < (sizeof(T) / sizeof(U)); ++n)
		{
			result ^= bits[n];
			result *= 1099511628211ull;
		}

		return result;
	}

	template<typename T>
	static size_t hash_struct(const T& value)
	{
		static constexpr auto block_sz = sizeof(T);

		if constexpr ((block_sz & 0x7) == 0)
		{
			return hash_struct_base<T, u64>(value);
		}

		if constexpr ((block_sz & 0x3) == 0)
		{
			return hash_struct_base<T, u32>(value);
		}

		if constexpr ((block_sz & 0x1) == 0)
		{
			return hash_struct_base<T, u16>(value);
		}

		return hash_struct_base<T, u8>(value);
	}
}
