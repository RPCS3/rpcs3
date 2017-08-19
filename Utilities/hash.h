#pragma once
#include <stdint.h>

namespace rpcs3
{
	template<typename T>
	static size_t hash_base(T value)
	{
		return static_cast<size_t>(value);
	}

	template<typename T>
	static size_t hash_struct(const T& value)
	{
		// FNV 64-bit
		size_t result = 14695981039346656037ull;
		const unsigned char *bytes = reinterpret_cast<const unsigned char*>(&value);

		for (size_t n = 0; n < sizeof(T); ++n)
		{
			result ^= bytes[n];
			result *= 1099511628211ull;
		}

		return result;
	}
}