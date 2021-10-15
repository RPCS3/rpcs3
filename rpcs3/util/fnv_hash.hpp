#pragma once

#include "util/types.hpp"
#include <cstring>

namespace rpcs3
{
	constexpr usz fnv_seed = 14695981039346656037ull;
	constexpr usz fnv_prime = 1099511628211ull;

	template <typename T>
	static usz hash_base(T value)
	{
		return static_cast<usz>(value);
	}

	template <typename T, typename = std::enable_if_t<std::is_integral<T>::value>>
	static inline usz hash64(usz hash_value, T data)
	{
		hash_value ^= data;
		hash_value *= fnv_prime;
		return hash_value;
	}

	template <typename T, typename U>
	static usz hash_struct_base(const T& value)
	{
		// FNV 64-bit
		usz result = fnv_seed;
		const uchar* bits = reinterpret_cast<const uchar*>(&value);

		for (usz n = 0; n < (sizeof(T) / sizeof(U)); ++n)
		{
			U val{};
			std::memcpy(&val, bits + (n * sizeof(U)), sizeof(U));
			result = hash64(result, val);
		}

		return result;
	}

	template <typename T>
	static usz hash_struct(const T& value)
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
