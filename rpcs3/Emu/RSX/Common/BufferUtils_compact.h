#pragma once

namespace
{
	template <usz Bytes, usz Lanes>
	constexpr auto make_compress_table()
	{
		std::array<std::array<u8, 16>, 1u << Lanes> table{};

		for (usz mask = 0; mask < table.size(); ++mask)
		{
			table[mask].fill(0xff);
			usz out = 0;

			for (usz lane = 0; lane < Lanes; ++lane)
			{
				if (mask & (1u << lane))
				{
					for (usz byte = 0; byte < Bytes; ++byte)
					{
						table[mask][out++] = static_cast<u8>(lane * Bytes + byte);
					}
				}
			}
		}

		return table;
	}

	alignas(64) constexpr auto s_compress_u16 = make_compress_table<2, 8>();
	alignas(64) constexpr auto s_compress_u32 = make_compress_table<4, 4>();

#if defined(ARCH_ARM64)
	template <typename T>
	std::tuple<T, T, u32> upload_swapped_neon_skip_restart(std::span<to_be_t<const T>> src, std::span<T> dst, T restart_index)
	{
		const u32 count = ::size32(src);
		u32 i = 0;
		u32 written = 0;
		T min_index = static_cast<T>(-1);
		T max_index = 0;

		if constexpr (sizeof(T) == 2)
		{
			auto min = vdupq_n_u16(-1);
			auto max = vdupq_n_u16(0);
			const u16 weights[] = {1, 2, 4, 8, 16, 32, 64, 128};

			for (; count - i >= 8; i += 8)
			{
				const auto value = vreinterpretq_u16_u8(vrev16q_u8(vld1q_u8(reinterpret_cast<const u8*>(src.data() + i))));
				const auto removed = vceqq_u16(value, vdupq_n_u16(restart_index));
				min = vminq_u16(min, vorrq_u16(value, removed));
				max = vmaxq_u16(max, vbicq_u16(value, removed));
				const u32 mask = vaddvq_u16(vbicq_u16(vld1q_u16(weights), removed));
				const auto packed = vqtbl1q_u8(vreinterpretq_u8_u16(value), vld1q_u8(s_compress_u16[mask].data()));
				// written <= i, so the full store stays inside the input-sized destination.
				vst1q_u8(reinterpret_cast<u8*>(dst.data() + written), packed);
				written += std::popcount(mask);
			}

			min_index = vminvq_u16(min);
			max_index = vmaxvq_u16(max);
		}
		else
		{
			auto min = vdupq_n_u32(-1);
			auto max = vdupq_n_u32(0);
			const u32 weights[] = {1, 2, 4, 8};

			for (; count - i >= 4; i += 4)
			{
				const auto value = vreinterpretq_u32_u8(vrev32q_u8(vld1q_u8(reinterpret_cast<const u8*>(src.data() + i))));
				const auto removed = vceqq_u32(value, vdupq_n_u32(restart_index));
				min = vminq_u32(min, vorrq_u32(value, removed));
				max = vmaxq_u32(max, vbicq_u32(value, removed));
				const u32 mask = vaddvq_u32(vbicq_u32(vld1q_u32(weights), removed));
				const auto packed = vqtbl1q_u8(vreinterpretq_u8_u32(value), vld1q_u8(s_compress_u32[mask].data()));
				vst1q_u8(reinterpret_cast<u8*>(dst.data() + written), packed);
				written += std::popcount(mask);
			}

			min_index = vminvq_u32(min);
			max_index = vmaxvq_u32(max);
		}

		for (; i < count; ++i)
		{
			const T value = src[i];
			if (value != restart_index)
			{
				min_index = std::min(min_index, value);
				max_index = std::max(max_index, value);
				dst[written++] = value;
			}
		}

		return {min_index, max_index, written};
	}
#elif defined(ARCH_X64)
	alignas(64) constexpr auto s_compress_u32_avx2 = []
	{
		std::array<std::array<u32, 8>, 256> table{};

		for (u32 mask = 0; mask < 256; ++mask)
		{
			u32 out = 0;

			for (u32 lane = 0; lane < 8; ++lane)
			{
				if (mask & (1u << lane))
				{
					table[mask][out++] = lane;
				}
			}
		}

		return table;
	}();

	template <typename T>
	AVX2_FUNC std::tuple<T, T, u32> upload_swapped_avx2_skip_restart(std::span<to_be_t<const T>> src, std::span<T> dst, T restart_index)
	{
		const u32 count = ::size32(src);
		u32 i = 0;
		u32 written = 0;
		auto min = _mm256_set1_epi32(-1);
		auto max = _mm256_setzero_si256();
		const auto swap = _mm256_broadcastsi128_si256(sizeof(T) == 2 ? s_bswap_u16_mask : s_bswap_u32_mask);
		const auto restart = sizeof(T) == 2 ? _mm256_set1_epi16(restart_index) : _mm256_set1_epi32(restart_index);
		constexpr u32 lanes = 32 / sizeof(T);

		for (; count - i >= lanes; i += lanes)
		{
			const auto raw = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(src.data() + i));
			const auto value = _mm256_shuffle_epi8(raw, swap);

			if constexpr (sizeof(T) == 2)
			{
				const auto removed = _mm256_cmpeq_epi16(value, restart);
				min = _mm256_min_epu16(min, _mm256_or_si256(value, removed));
				max = _mm256_max_epu16(max, _mm256_andnot_si256(removed, value));
				const auto lo_mask = _mm256_castsi256_si128(removed);
				const auto hi_mask = _mm256_extracti128_si256(removed, 1);
				const u32 mask = static_cast<u32>(~_mm_movemask_epi8(_mm_packs_epi16(lo_mask, hi_mask))) & 0xffff;
				const u32 lo = mask & 255;
				const u32 hi = mask >> 8;
				const auto a = _mm_shuffle_epi8(_mm256_castsi256_si128(value), _mm_loadu_si128(reinterpret_cast<const __m128i*>(s_compress_u16[lo].data())));
				const auto b = _mm_shuffle_epi8(_mm256_extracti128_si256(value, 1), _mm_loadu_si128(reinterpret_cast<const __m128i*>(s_compress_u16[hi].data())));
				_mm_storeu_si128(reinterpret_cast<__m128i*>(dst.data() + written), a);
				written += std::popcount(lo);
				_mm_storeu_si128(reinterpret_cast<__m128i*>(dst.data() + written), b);
				written += std::popcount(hi);
			}
			else
			{
				const auto removed = _mm256_cmpeq_epi32(value, restart);
				min = _mm256_min_epu32(min, _mm256_or_si256(value, removed));
				max = _mm256_max_epu32(max, _mm256_andnot_si256(removed, value));
				const u32 mask = static_cast<u32>(~_mm256_movemask_ps(_mm256_castsi256_ps(removed))) & 255;
				const auto indices = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(s_compress_u32_avx2[mask].data()));
				const auto packed = _mm256_permutevar8x32_epi32(value, indices);
				_mm256_storeu_si256(reinterpret_cast<__m256i*>(dst.data() + written), packed);
				written += std::popcount(mask);
			}
		}

		alignas(32) T min_lanes[lanes];
		alignas(32) T max_lanes[lanes];
		_mm256_store_si256(reinterpret_cast<__m256i*>(min_lanes), min);
		_mm256_store_si256(reinterpret_cast<__m256i*>(max_lanes), max);
		T min_index = *std::min_element(std::begin(min_lanes), std::end(min_lanes));
		T max_index = *std::max_element(std::begin(max_lanes), std::end(max_lanes));

		for (; i < count; ++i)
		{
			const T value = src[i];
			if (value != restart_index)
			{
				min_index = std::min(min_index, value);
				max_index = std::max(max_index, value);
				dst[written++] = value;
			}
		}

		return {min_index, max_index, written};
	}
#endif
} // namespace
