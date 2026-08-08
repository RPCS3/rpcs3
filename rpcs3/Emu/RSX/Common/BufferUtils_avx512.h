#pragma once

#if defined(_MSC_VER) || !defined(__SSE2__)
#define AVX512_SSE4_1_FUNC
#define AVX512_AVX3_FUNC
#define AVX512_ICL_FUNC
#else
#define AVX512_SSE4_1_FUNC __attribute__((__target__("sse4.1")))
#define AVX512_AVX3_FUNC __attribute__((__target__("avx512f,avx512bw,avx512dq,avx512cd,avx512vl")))
#define AVX512_ICL_FUNC __attribute__((__target__("avx512f,avx512bw,avx512dq,avx512cd,avx512vl,avx512bitalg,avx512ifma,avx512vbmi,avx512vbmi2,avx512vnni,avx512vpopcntdq")))
#endif

namespace
{
const v128 s_avx512_bswap_u32_mask = v128::from32(0x00010203, 0x04050607, 0x08090a0b, 0x0c0d0e0f);
const v128 s_avx512_bswap_u16_mask = v128::from32(0x02030001, 0x06070405, 0x0a0b0809, 0x0e0f0c0d);

AVX512_SSE4_1_FUNC static inline u16 avx512_hmin_epu16(__m128i x)
{
	return _mm_cvtsi128_si32(_mm_minpos_epu16(x));
}

AVX512_SSE4_1_FUNC static inline u16 avx512_hmax_epu16(__m128i x)
{
	return ~_mm_cvtsi128_si32(_mm_minpos_epu16(_mm_xor_si128(x, _mm_set1_epi32(-1))));
}

AVX512_ICL_FUNC std::tuple<u16, u16, u32> upload_u16_swapped_avx512_icl_skip_restart(std::span<to_be_t<const u16>> src, std::span<u16> dst, u16 restart_index)
{
	const __m512i s_bswap_u16_mask512 = _mm512_broadcast_i64x2(s_avx512_bswap_u16_mask);

	const auto* src_stream = static_cast<const __m512i*>(static_cast<const void*>(src.data()));
	auto* dst_stream = dst.data();
	const u32 count = ::size32(src);

	const __m512i restart = _mm512_set1_epi16(restart_index);
	__m512i min = _mm512_set1_epi16(-1);
	__m512i max = _mm512_set1_epi16(0);
	const __m512i ones = _mm512_set1_epi16(-1);

	u32 written = 0;

	const u32 iterations = count / 32;
	for (u32 i = 0; i < iterations; i++)
	{
		const __m512i raw = _mm512_loadu_si512(src_stream++);
		const __m512i value = _mm512_shuffle_epi8(raw, s_bswap_u16_mask512);
		const __mmask32 mask = _mm512_cmpneq_epi16_mask(restart, value);
		const __m512i value_with_max_restart = _mm512_mask_blend_epi16(mask, ones, value);

		max = _mm512_mask_max_epu16(max, mask, max, value);
		min = _mm512_mask_min_epu16(min, mask, min, value);
		const __m512i packed = _mm512_maskz_compress_epi16(mask, value_with_max_restart);

		const u32 processed = static_cast<u32>(std::popcount(static_cast<u32>(mask)));
		_mm512_storeu_si512(dst_stream, packed);
		dst_stream += processed;
		written += processed;
	}

	const u32 remainder = count % 32;
	if (remainder > 0)
	{
		const __mmask32 rem_mask = (1U << remainder) - 1;
		const __m512i raw = _mm512_maskz_loadu_epi16(rem_mask, src_stream);
		const __m512i value = _mm512_shuffle_epi8(raw, s_bswap_u16_mask512);
		const __mmask32 mask = _mm512_mask_cmpneq_epi16_mask(rem_mask, restart, value);

		const __m512i value_with_max_restart = _mm512_mask_blend_epi16(mask, ones, value);
		max = _mm512_mask_max_epu16(max, mask, max, value);
		min = _mm512_mask_min_epu16(min, mask, min, value);
		const __m512i packed = _mm512_maskz_compress_epi16(mask, value_with_max_restart);

		const u32 processed = static_cast<u32>(std::popcount(static_cast<u32>(mask)));
		const __mmask32 store_mask = (1U << processed) - 1;
		_mm512_mask_storeu_epi16(dst_stream, store_mask, packed);
		written += processed;
	}

	__m256i tmp256 = _mm512_extracti64x4_epi64(min, 1);
	__m256i min2 = _mm512_castsi512_si256(min);
	min2 = _mm256_min_epu16(min2, tmp256);
	__m128i tmp = _mm256_extracti128_si256(min2, 1);
	__m128i min3 = _mm256_castsi256_si128(min2);
	min3 = _mm_min_epu16(min3, tmp);

	tmp256 = _mm512_extracti64x4_epi64(max, 1);
	__m256i max2 = _mm512_castsi512_si256(max);
	max2 = _mm256_max_epu16(max2, tmp256);
	tmp = _mm256_extracti128_si256(max2, 1);
	__m128i max3 = _mm256_castsi256_si128(max2);
	max3 = _mm_max_epu16(max3, tmp);

	const u16 min_index = avx512_hmin_epu16(min3);
	const u16 max_index = avx512_hmax_epu16(max3);

	return std::make_tuple(min_index, max_index, written);
}

AVX512_AVX3_FUNC std::tuple<u32, u32, u32> upload_u32_swapped_avx3_skip_restart(std::span<to_be_t<const u32>> src, std::span<u32> dst, u32 restart_index)
{
	const __m512i s_bswap_u32_mask512 = _mm512_broadcast_i32x4(s_avx512_bswap_u32_mask);

	const auto* src_stream = static_cast<const __m512i*>(static_cast<const void*>(src.data()));
	auto* dst_stream = dst.data();
	const u32 count = ::size32(src);

	const __m512i restart = _mm512_set1_epi32(restart_index);
	__m512i min = _mm512_set1_epi32(-1);
	__m512i max = _mm512_set1_epi32(0);
	const __m512i ones = _mm512_set1_epi32(-1);

	u32 written = 0;

	const u32 iterations = count / 16;
	for (u32 i = 0; i < iterations; i++)
	{
		const __m512i raw = _mm512_loadu_si512(src_stream++);
		const __m512i value = _mm512_shuffle_epi8(raw, s_bswap_u32_mask512);
		const __mmask16 mask = _mm512_cmpneq_epi32_mask(restart, value);
		const __m512i value_with_max_restart = _mm512_mask_blend_epi32(mask, ones, value);

		max = _mm512_mask_max_epu32(max, mask, max, value);
		min = _mm512_mask_min_epu32(min, mask, min, value);
		const __m512i packed = _mm512_maskz_compress_epi32(mask, value_with_max_restart);

		const u32 processed = static_cast<u32>(std::popcount(static_cast<u32>(mask)));
		_mm512_storeu_si512(dst_stream, packed);
		dst_stream += processed;
		written += processed;
	}

	const u32 remainder = count % 16;
	if (remainder > 0)
	{
		const __mmask16 rem_mask = (1U << remainder) - 1;
		const __m512i raw = _mm512_maskz_loadu_epi32(rem_mask, src_stream);
		const __m512i value = _mm512_shuffle_epi8(raw, s_bswap_u32_mask512);

		const __mmask16 mask = _mm512_mask_cmpneq_epi32_mask(rem_mask, restart, value);
		const __m512i value_with_max_restart = _mm512_mask_blend_epi32(mask, ones, value);
		max = _mm512_mask_max_epu32(max, mask, max, value);
		min = _mm512_mask_min_epu32(min, mask, min, value);
		const __m512i packed = _mm512_maskz_compress_epi32(mask, value_with_max_restart);

		const u32 processed = static_cast<u32>(std::popcount(static_cast<u32>(mask)));
		const __mmask16 store_mask = (1U << processed) - 1;
		_mm512_mask_storeu_epi32(dst_stream, store_mask, packed);
		written += processed;
	}

	const u32 min_index = _mm512_reduce_min_epu32(min);
	const u32 max_index = _mm512_reduce_max_epu32(max);

	return std::make_tuple(min_index, max_index, written);
}

}
