#pragma once

#include "ProgramStateCache.h"

#include "util/asm.hpp"

#if defined(ARCH_X64)
#include "emmintrin.h"
#endif

template <typename Traits>
void program_state_cache<Traits>::fill_fragment_constants_buffer(std::span<f32> dst_buffer, const RSXFragmentProgram &fragment_program, bool sanitize) const
{
	const auto I = m_fragment_shader_cache.find(fragment_program);
	if (I == m_fragment_shader_cache.end())
		return;

	ensure((dst_buffer.size_bytes() >= ::narrow<int>(I->second.FragmentConstantOffsetCache.size()) * 16u));

	f32* dst = dst_buffer.data();
	alignas(16) f32 tmp[4];
	for (usz offset_in_fragment_program : I->second.FragmentConstantOffsetCache)
	{
		char* data = static_cast<char*>(fragment_program.get_data()) + offset_in_fragment_program;

#if defined(ARCH_X64)
		const __m128i vector = _mm_loadu_si128(reinterpret_cast<__m128i*>(data));
		const __m128i shuffled_vector = _mm_or_si128(_mm_slli_epi16(vector, 8), _mm_srli_epi16(vector, 8));
#else
		for (u32 i = 0; i < 4; i++)
		{
			const u32 value = reinterpret_cast<u32*>(data)[i];
			tmp[i] = std::bit_cast<f32, u32>(((value >> 8) & 0xff00ff) | ((value << 8) & 0xff00ff00));
		}
#endif

		if (!patch_table.is_empty())
		{
#if defined(ARCH_X64)
			_mm_store_ps(tmp, _mm_castsi128_ps(shuffled_vector));
#endif

			for (int i = 0; i < 4; ++i)
			{
				bool patched = false;
				for (auto& e : patch_table.db)
				{
					//TODO: Use fp comparison with fabsf without hurting performance
					patched = e.second.test_and_set(tmp[i], &dst[i]);
					if (patched)
					{
						break;
					}
				}

				if (!patched)
				{
					dst[i] = tmp[i];
				}
			}
		}
		else if (sanitize)
		{
#if defined(ARCH_X64)
			//Convert NaNs and Infs to 0
			const auto masked = _mm_and_si128(shuffled_vector, _mm_set1_epi32(0x7fffffff));
			const auto valid = _mm_cmplt_epi32(masked, _mm_set1_epi32(0x7f800000));
			const auto result = _mm_and_si128(shuffled_vector, valid);
			_mm_stream_si128(utils::bless<__m128i>(dst), result);
#else
			for (u32 i = 0; i < 4; i++)
			{
				const u32 value = std::bit_cast<u32>(tmp[i]);
				tmp[i] = (value & 0x7fffffff) < 0x7f800000 ? value : 0;
			}

			std::memcpy(dst, tmp, 16);
#endif
		}
		else
		{
#if defined(ARCH_X64)
			_mm_stream_si128(utils::bless<__m128i>(dst), shuffled_vector);
#else
			std::memcpy(dst, tmp, 16);
#endif
		}

		dst += 4;
	}
}
