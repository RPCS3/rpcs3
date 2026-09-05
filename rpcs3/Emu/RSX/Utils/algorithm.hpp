#pragma once

#include <cmath>
#include <util/types.hpp>

namespace rsx
{
	static constexpr u32 floor_log2(u32 value)
	{
		return value <= 1 ? 0 : std::countl_zero(value) ^ 31;
	}

	static constexpr u32 ceil_log2(u32 value)
	{
		return floor_log2(value) + u32{!!(value & (value - 1))};
	}

	static constexpr u32 next_pow2(u32 x)
	{
		if (x <= 2) return x;

		return static_cast<u32>((1ULL << 32) >> std::countl_zero(x - 1));
	}

	static inline bool fcmp(float a, float b, float epsilon = 0.000001f)
	{
		return fabsf(a - b) < epsilon;
	}

	// General purpose alignment without power-of-2 constraint
	template <typename T, typename U>
	static inline T align2(T value, U alignment)
	{
		return ((value + alignment - 1) / alignment) * alignment;
	}

	// General purpose downward alignment without power-of-2 constraint
	template <typename T, typename U>
	static inline T align_down2(T value, U alignment)
	{
		return (value / alignment) * alignment;
	}

	// Copy memory in inverse direction from source
	// Used to scale negatively x axis while transfering image data
	template <typename Ts = u8, typename Td = Ts>
	static void memcpy_r(void* dst, void* src, usz size)
	{
		for (u32 i = 0; i < size; i++)
		{
			*(static_cast<Td*>(dst) + i) = *(static_cast<Ts*>(src) - i);
		}
	}
}
