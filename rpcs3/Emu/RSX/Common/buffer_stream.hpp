#pragma once

#include "util/types.hpp"

#if defined(ARCH_X64)
#include "emmintrin.h"
#include "immintrin.h"
#endif

#ifdef ARCH_ARM64
#ifndef _MSC_VER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif
#include "Emu/CPU/sse2neon.h"
#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif
#endif

namespace utils
{
	/**
	 * Stream a 128 bits vector to dst.
	 */
	static inline
		void stream_vector(void* dst, u32 x, u32 y, u32 z, u32 w)
	{
		const __m128i vector = _mm_set_epi32(w, z, y, x);
		_mm_stream_si128(reinterpret_cast<__m128i*>(dst), vector);
	}

	static inline
		void stream_vector(void* dst, f32 x, f32 y, f32 z, f32 w)
	{
		stream_vector(dst, std::bit_cast<u32>(x), std::bit_cast<u32>(y), std::bit_cast<u32>(z), std::bit_cast<u32>(w));
	}

	/**
	 * Stream a 128 bits vector from src to dst.
	 */
	static inline
		void stream_vector_from_memory(void* dst, void* src)
	{
		const __m128i vector = _mm_loadu_si128(reinterpret_cast<__m128i*>(src));
		_mm_stream_si128(reinterpret_cast<__m128i*>(dst), vector);
	}
}
