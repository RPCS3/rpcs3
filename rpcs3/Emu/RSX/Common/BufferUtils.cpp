#include "stdafx.h"
#include "BufferUtils.h"
#include "../rsx_methods.h"
#include "../RSXThread.h"

#include "util/to_endian.hpp"
#include "util/sysinfo.hpp"
#include "Utilities/JIT.h"
#include "util/asm.hpp"

#if defined(ARCH_X64)
#include "emmintrin.h"
#include "immintrin.h"
#endif

#if !defined(_MSC_VER)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif

#ifdef ARCH_ARM64
#if !defined(_MSC_VER)
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif
#undef FORCE_INLINE
#include "Emu/CPU/sse2neon.h"
#endif

#if defined(_MSC_VER) || !defined(__SSE2__)
#define PLAIN_FUNC
#define SSE4_1_FUNC
#define AVX2_FUNC
#define AVX3_FUNC
#else
#ifndef __clang__
#define PLAIN_FUNC __attribute__((optimize("no-tree-vectorize")))
#else
#define PLAIN_FUNC
#endif
#define SSE4_1_FUNC __attribute__((__target__("sse4.1")))
#define AVX2_FUNC __attribute__((__target__("avx2")))
#define AVX3_FUNC __attribute__((__target__("avx512f,avx512bw,avx512dq,avx512cd,avx512vl")))
#ifndef __AVX2__
using __m256i = long long __attribute__((vector_size(32)));
#endif
#endif // _MSC_VER

SSE4_1_FUNC static inline u16 sse41_hmin_epu16(__m128i x)
{
	return _mm_cvtsi128_si32(_mm_minpos_epu16(x));
}

SSE4_1_FUNC static inline u16 sse41_hmax_epu16(__m128i x)
{
	return ~_mm_cvtsi128_si32(_mm_minpos_epu16(_mm_xor_si128(x, _mm_set1_epi32(-1))));
}

#if defined(__AVX512F__) && defined(__AVX512VL__) && defined(__AVX512DQ__) && defined(__AVX512CD__) && defined(__AVX512BW__)
[[maybe_unused]] constexpr bool s_use_ssse3 = true;
constexpr bool s_use_sse4_1 = true;
constexpr bool s_use_avx2 = true;
constexpr bool s_use_avx3 = true;
#elif defined(__AVX2__)
[[maybe_unused]] constexpr bool s_use_ssse3 = true;
constexpr bool s_use_sse4_1 = true;
constexpr bool s_use_avx2 = true;
constexpr bool s_use_avx3 = false;
#elif defined(__SSE4_1__)
[[maybe_unused]] constexpr bool s_use_ssse3 = true;
constexpr bool s_use_sse4_1 = true;
constexpr bool s_use_avx2 = false;
constexpr bool s_use_avx3 = false;
#elif defined(__SSSE3__)
[[maybe_unused]] constexpr bool s_use_ssse3 = true;
constexpr bool s_use_sse4_1 = false;
constexpr bool s_use_avx2 = false;
constexpr bool s_use_avx3 = false;
#elif defined(ARCH_X64)
[[maybe_unused]] const bool s_use_ssse3 = utils::has_ssse3();
const bool s_use_sse4_1 = utils::has_sse41();
const bool s_use_avx2 = utils::has_avx2();
const bool s_use_avx3 = utils::has_avx512();
#else
[[maybe_unused]] constexpr bool s_use_ssse3 = true; // Non x86
constexpr bool s_use_sse4_1 = true; // Non x86
constexpr bool s_use_avx2 = false;
constexpr bool s_use_avx3 = false;
#endif

const __m128i s_bswap_u32_mask = _mm_set_epi8(
	0xC, 0xD, 0xE, 0xF,
	0x8, 0x9, 0xA, 0xB,
	0x4, 0x5, 0x6, 0x7,
	0x0, 0x1, 0x2, 0x3);

const __m128i s_bswap_u16_mask = _mm_set_epi8(
	0xE, 0xF, 0xC, 0xD,
	0xA, 0xB, 0x8, 0x9,
	0x6, 0x7, 0x4, 0x5,
	0x2, 0x3, 0x0, 0x1);

namespace utils
{
	template <typename T, typename U>
	[[nodiscard]] auto bless(const std::span<U>& span)
	{
		return std::span<T>(bless<T>(span.data()), sizeof(U) * span.size() / sizeof(T));
	}
}

namespace
{
	template <bool Compare>
	PLAIN_FUNC auto copy_data_swap_u32_naive(u32* dst, const u32* src, u32 count)
	{
		u32 result = 0;

#ifdef __clang__
		#pragma clang loop vectorize(disable) interleave(disable) unroll(disable)
#endif
		for (u32 i = 0; i < count; i++)
		{
			const u32 data = stx::se_storage<u32>::swap(src[i]);

			if constexpr (Compare)
			{
				result |= data ^ dst[i];
			}

			dst[i] = data;
		}

		if constexpr (Compare)
		{
			return static_cast<bool>(result);
		}
	}

#if defined(ARCH_X64)
	template <bool Compare, uint Size, bool Avx = true>
	void build_copy_data_swap_u32_avx3(native_asm& c, native_args& args)
	{
		using namespace asmjit;

		native_vec_t<Size> vdata{0};
		native_vec_t<Size> vmask{1};
		native_vec_t<Size> vcmp{2};

		// Load and broadcast shuffle mask
		if constexpr (!Avx)
			c.movaps(vmask, x86::oword_ptr(uptr(&s_bswap_u32_mask)));
		if constexpr (Size == 16 && Avx)
			c.vmovaps(vmask, x86::oword_ptr(uptr(&s_bswap_u32_mask)));
		if constexpr (Size >= 32)
			c.vbroadcasti32x4(vmask, x86::oword_ptr(uptr(&s_bswap_u32_mask)));

		// Clear vcmp (bitwise inequality accumulator)
		if constexpr (Compare && Avx)
			c.vxorps(x86::xmm2, x86::xmm2, x86::xmm2);
		if constexpr (Compare && !Avx)
			c.xorps(x86::xmm2, x86::xmm2);
		c.mov(args[3].r32(), -1);
		c.xor_(x86::eax, x86::eax);

		build_incomplete_loop(c, x86::eax, args[2].r32(), Size / 4, [&]
		{
			if constexpr (Avx)
			{
				c.vmovdqu32(vdata, x86::ptr(args[1], x86::rax, 2, 0, Size));
				c.vpshufb(vdata, vdata, vmask);
				if constexpr (Compare)
					c.vpternlogd(vcmp, vdata, x86::ptr(args[0], x86::rax, 2, 0, Size), 0xf6); // orAxorBC
				c.vmovdqu32(x86::ptr(args[0], x86::rax, 2, 0, Size), vdata);
			}
			else
			{
				c.movdqu(vdata, x86::oword_ptr(args[1], x86::rax, 2, 0));
				c.pshufb(vdata, vmask);
				if constexpr (Compare)
				{
					c.movups(x86::xmm3, x86::oword_ptr(args[0], x86::rax, 2, 0));
					c.xorps(x86::xmm3, vdata);
					c.orps(vcmp, x86::xmm3);
				}
				c.movups(x86::oword_ptr(args[0], x86::rax, 2, 0), vdata);
			}
		});

		if constexpr (Avx)
		{
			c.bzhi(args[3].r32(), args[3].r32(), args[2].r32());
			c.kmovw(x86::k1, args[3].r32());
			c.k(x86::k1).z().vmovdqu32(vdata, x86::ptr(args[1], x86::rax, 2, 0, Size));
			c.vpshufb(vdata, vdata, vmask);
			if constexpr (Compare)
				c.k(x86::k1).vpternlogd(vcmp, vdata, x86::ptr(args[0], x86::rax, 2, 0, Size), 0xf6);
			c.k(x86::k1).vmovdqu32(x86::ptr(args[0], x86::rax, 2, 0, Size), vdata);
		}
		else
		{
			build_loop(c, x86::eax, args[2].r32(), [&]
			{
				c.movd(vdata, x86::dword_ptr(args[1], x86::rax, 2, 0));
				c.pshufb(vdata, vmask);
				if constexpr (Compare)
				{
					c.movd(x86::xmm3, x86::dword_ptr(args[0], x86::rax, 2, 0));
					c.pxor(x86::xmm3, vdata);
					c.por(vcmp, x86::xmm3);
				}
				c.movd(x86::dword_ptr(args[0], x86::rax, 2, 0), vdata);
			});
		}

		if (Compare)
		{
			if constexpr (!Avx)
			{
				c.ptest(vcmp, vcmp);
			}
			else if constexpr (Size != 64)
			{
				c.vptest(vcmp, vcmp);
			}
			else
			{
				c.vptestmd(x86::k1, vcmp, vcmp);
				c.ktestw(x86::k1, x86::k1);
			}

			c.setnz(x86::al);
		}

		if constexpr (Avx)
			c.vzeroupper();
		c.ret();
	}

	template <bool Compare>
	void build_copy_data_swap_u32(native_asm& c, native_args& args)
	{
		using namespace asmjit;

		if (utils::has_avx512())
		{
			if (utils::has_avx512_icl())
			{
				build_copy_data_swap_u32_avx3<Compare, 64>(c, args);
				return;
			}

			build_copy_data_swap_u32_avx3<Compare, 32>(c, args);
			return;
		}

		if (utils::has_sse41())
		{
			build_copy_data_swap_u32_avx3<Compare, 16, false>(c, args);
			return;
		}

		c.jmp(&copy_data_swap_u32_naive<Compare>);
	}
#elif defined(ARCH_ARM64)
	template <bool Compare>
	void build_copy_data_swap_u32(native_asm& c, native_args& args)
	{
		c.b(&copy_data_swap_u32_naive<Compare>);
	}
#endif
}

#if !defined(__APPLE__) || defined(ARCH_X64)
DECLARE(copy_data_swap_u32) = build_function_asm<void(*)(u32*, const u32*, u32)>("copy_data_swap_u32", &build_copy_data_swap_u32<false>);
DECLARE(copy_data_swap_u32_cmp) = build_function_asm<bool(*)(u32*, const u32*, u32)>("copy_data_swap_u32_cmp", &build_copy_data_swap_u32<true>);
#else
DECLARE(copy_data_swap_u32) = copy_data_swap_u32_naive<false>;
DECLARE(copy_data_swap_u32_cmp) = copy_data_swap_u32_naive<true>;
#endif

namespace
{
	template <typename T>
	constexpr T index_limit()
	{
		return -1;
	}

	template <typename T>
	const T& min_max(T& min, T& max, const T& value)
	{
		if (value < min)
			min = value;

		if (value > max)
			max = value;

		return value;
	}

	struct untouched_impl
	{
#if defined(ARCH_X64)
		AVX3_FUNC
		static
		std::tuple<u16, u16, u32> upload_u16_swapped_avx3(const void *src, void *dst, u32 count)
		{
			const __m512i s_bswap_u16_mask512 = _mm512_broadcast_i64x2(s_bswap_u16_mask);

			const __m512i s_remainder_mask = _mm512_set_epi16(
			0x20, 0x1F, 0x1E, 0x1D,
			0x1C, 0x1B, 0x1A, 0x19,
			0x18, 0x17, 0x16, 0x15,
			0x14, 0x13, 0x12, 0x11,
			0x10, 0xF, 0xE, 0xD,
			0xC, 0xB, 0xA, 0x9,
			0x8, 0x7, 0x6, 0x5,
			0x4, 0x3, 0x2, 0x1);

			auto src_stream = static_cast<const __m512*>(src);
			auto dst_stream = static_cast<__m512*>(dst);

			__m512i min = _mm512_set1_epi16(-1);
			__m512i max = _mm512_set1_epi16(0);

			const auto iterations = count / 32;
			for (unsigned n = 0; n < iterations; ++n)
			{
				const __m512i raw = _mm512_loadu_si512(src_stream++);
				const __m512i value = _mm512_shuffle_epi8(raw, s_bswap_u16_mask512);
				max = _mm512_max_epu16(max, value);
				min = _mm512_min_epu16(min, value);
				_mm512_store_si512(dst_stream++, value);
			}

			if ((iterations * 32) < count )
			{
				const u16 remainder = (count - (iterations * 32));
				const __m512i remBroadcast = _mm512_set1_epi16(remainder);
				const __mmask32 mask = _mm512_cmpge_epi16_mask(remBroadcast, s_remainder_mask);
				const __m512i raw = _mm512_maskz_loadu_epi16(mask, src_stream++);
				const __m512i value = _mm512_shuffle_epi8(raw, s_bswap_u16_mask512);
				max = _mm512_mask_max_epu16(max, mask, max, value);
				min = _mm512_mask_min_epu16(min, mask, min, value);
				_mm512_mask_storeu_epi16(dst_stream++, mask, value);
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

			const u16 min_index = sse41_hmin_epu16(min3);
			const u16 max_index = sse41_hmax_epu16(max3);

			return std::make_tuple(min_index, max_index, count);
		}

		AVX2_FUNC
		static
		std::tuple<u16, u16, u32> upload_u16_swapped_avx2(const void *src, void *dst, u32 count)
		{
			const __m256i shuffle_mask = _mm256_set_m128i(s_bswap_u16_mask, s_bswap_u16_mask);

			auto src_stream = static_cast<const __m256i*>(src);
			auto dst_stream = static_cast<__m256i*>(dst);

			__m256i min = _mm256_set1_epi16(-1);
			__m256i max = _mm256_set1_epi16(0);

			const auto iterations = count / 16;
			for (unsigned n = 0; n < iterations; ++n)
			{
				const __m256i raw = _mm256_loadu_si256(src_stream++);
				const __m256i value = _mm256_shuffle_epi8(raw, shuffle_mask);
				max = _mm256_max_epu16(max, value);
				min = _mm256_min_epu16(min, value);
				_mm256_store_si256(dst_stream++, value);
			}

			__m128i tmp = _mm256_extracti128_si256(min, 1);
			__m128i min2 = _mm256_castsi256_si128(min);
			min2 = _mm_min_epu16(min2, tmp);

			tmp = _mm256_extracti128_si256(max, 1);
			__m128i max2 = _mm256_castsi256_si128(max);
			max2 = _mm_max_epu16(max2, tmp);

			const u16 min_index = sse41_hmin_epu16(min2);
			const u16 max_index = sse41_hmax_epu16(max2);

			return std::make_tuple(min_index, max_index, count);
		}
#endif

		SSE4_1_FUNC
		static
		std::tuple<u16, u16, u32> upload_u16_swapped_sse4_1(const void *src, void *dst, u32 count)
		{
			auto src_stream = static_cast<const __m128i*>(src);
			auto dst_stream = static_cast<__m128i*>(dst);

			__m128i min = _mm_set1_epi16(-1);
			__m128i max = _mm_set1_epi16(0);

			const auto iterations = count / 8;
			for (unsigned n = 0; n < iterations; ++n)
			{
				const __m128i raw = _mm_loadu_si128(src_stream++);
				const __m128i value = _mm_shuffle_epi8(raw, s_bswap_u16_mask);
				max = _mm_max_epu16(max, value);
				min = _mm_min_epu16(min, value);
				_mm_store_si128(dst_stream++, value);
			}

			const u16 min_index = sse41_hmin_epu16(min);
			const u16 max_index = sse41_hmax_epu16(max);

			return std::make_tuple(min_index, max_index, count);
		}

		SSE4_1_FUNC
		static
		std::tuple<u32, u32, u32> upload_u32_swapped_sse4_1(const void *src, void *dst, u32 count)
		{
			auto src_stream = static_cast<const __m128i*>(src);
			auto dst_stream = static_cast<__m128i*>(dst);

			__m128i min = _mm_set1_epi32(~0u);
			__m128i max = _mm_set1_epi32(0);

			const auto iterations = count / 4;
			for (unsigned n = 0; n < iterations; ++n)
			{
				const __m128i raw = _mm_loadu_si128(src_stream++);
				const __m128i value = _mm_shuffle_epi8(raw, s_bswap_u32_mask);
				max = _mm_max_epu32(max, value);
				min = _mm_min_epu32(min, value);
				_mm_store_si128(dst_stream++, value);
			}

			__m128i tmp = _mm_srli_si128(min, 8);
			min = _mm_min_epu32(min, tmp);
			tmp = _mm_srli_si128(min, 4);
			min = _mm_min_epu32(min, tmp);

			tmp = _mm_srli_si128(max, 8);
			max = _mm_max_epu32(max, tmp);
			tmp = _mm_srli_si128(max, 4);
			max = _mm_max_epu32(max, tmp);

			const u32 min_index = _mm_cvtsi128_si32(min);
			const u32 max_index = _mm_cvtsi128_si32(max);

			return std::make_tuple(min_index, max_index, count);
		}

		template<typename T>
		static
		std::tuple<T, T, u32> upload_untouched(std::span<to_be_t<const T>> src, std::span<T> dst)
		{
			T min_index, max_index;
			u32 written;
			u32 remaining = ::size32(src);

			if (s_use_sse4_1 && remaining >= 32)
			{
				if constexpr (std::is_same<T, u32>::value)
				{
					const auto count = (remaining & ~0x3);
					std::tie(min_index, max_index, written) = upload_u32_swapped_sse4_1(src.data(), dst.data(), count);
				}
				else if constexpr (std::is_same<T, u16>::value)
				{
					if (s_use_avx3)
					{
#if defined(ARCH_X64)

						// Handle remainder in function
						std::tie(min_index, max_index, written) = upload_u16_swapped_avx3(src.data(), dst.data(), remaining);
						return std::make_tuple(min_index, max_index, written);
					}
					else if (s_use_avx2)
					{
						const auto count = (remaining & ~0xf);
						std::tie(min_index, max_index, written) = upload_u16_swapped_avx2(src.data(), dst.data(), count);
#endif
					}
					else
					{
						const auto count = (remaining & ~0x7);
						std::tie(min_index, max_index, written) = upload_u16_swapped_sse4_1(src.data(), dst.data(), count);
					}
				}
				else
				{
					fmt::throw_exception("Unreachable");
				}

				remaining -= written;
			}
			else
			{
				min_index = index_limit<T>();
				max_index = 0;
				written = 0;
			}

			while (remaining--)
			{
				T index = src[written];
				dst[written++] = min_max(min_index, max_index, index);
			}

			return std::make_tuple(min_index, max_index, written);
		}
	};

	struct primitive_restart_impl
	{
#if defined(ARCH_X64)
		AVX3_FUNC
		static
		std::tuple<u16, u16> upload_u16_swapped_avx3(const void *src, void *dst, u32 count, u16 restart_index)
		{
			const __m512i s_bswap_u16_mask512 = _mm512_broadcast_i64x2(s_bswap_u16_mask);

			const __m512i s_remainder_mask = _mm512_set_epi16(
			0x20, 0x1F, 0x1E, 0x1D,
			0x1C, 0x1B, 0x1A, 0x19,
			0x18, 0x17, 0x16, 0x15,
			0x14, 0x13, 0x12, 0x11,
			0x10, 0xF, 0xE, 0xD,
			0xC, 0xB, 0xA, 0x9,
			0x8, 0x7, 0x6, 0x5,
			0x4, 0x3, 0x2, 0x1);

			auto src_stream = static_cast<const __m512*>(src);
			auto dst_stream = static_cast<__m512*>(dst);

			__m512i restart = _mm512_set1_epi16(restart_index);
			__m512i min = _mm512_set1_epi16(-1);
			__m512i max = _mm512_set1_epi16(0);
			const __m512i ones = _mm512_set1_epi16(-1);

			const auto iterations = count / 32;
			for (unsigned n = 0; n < iterations; ++n)
			{
				const __m512i raw = _mm512_loadu_si512(src_stream++);
				const __m512i value = _mm512_shuffle_epi8(raw, s_bswap_u16_mask512);
				const __mmask32 mask = _mm512_cmpneq_epi16_mask(restart, value);
				const __m512i value_with_max_restart = _mm512_mask_blend_epi16(mask, ones, value);
				max = _mm512_mask_max_epu16(max, mask, max, value);
				min = _mm512_mask_min_epu16(min, mask, min, value);
				_mm512_store_si512(dst_stream++, value_with_max_restart);
			}

			if ((iterations * 32) < count )
			{
				const u16 remainder = (count - (iterations * 32));
				const __m512i remBroadcast = _mm512_set1_epi16(remainder);
				const __mmask32 mask = _mm512_cmpge_epi16_mask(remBroadcast, s_remainder_mask);
				const __m512i raw = _mm512_maskz_loadu_epi16(mask, src_stream++);
				const __m512i value = _mm512_shuffle_epi8(raw, s_bswap_u16_mask512);
				const __mmask32 mask2 = _mm512_cmpneq_epi16_mask(restart, value);
				const __mmask32 mask3 = _kand_mask32(mask, mask2);
				const __m512i value_with_max_restart = _mm512_mask_blend_epi16(mask3, ones, value);
				max = _mm512_mask_max_epu16(max, mask3, max, value);
				min = _mm512_mask_min_epu16(min, mask3, min, value);
				_mm512_mask_storeu_epi16(dst_stream++, mask, value_with_max_restart);
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

			const u16 min_index = sse41_hmin_epu16(min3);
			const u16 max_index = sse41_hmax_epu16(max3);

			return std::make_tuple(min_index, max_index);
		}

		AVX2_FUNC
		static
		std::tuple<u16, u16> upload_u16_swapped_avx2(const void *src, void *dst, u32 iterations, u16 restart_index)
		{
			const __m256i shuffle_mask = _mm256_set_m128i(s_bswap_u16_mask, s_bswap_u16_mask);

			auto src_stream = static_cast<const __m256i*>(src);
			auto dst_stream = static_cast<__m256i*>(dst);

			__m256i restart = _mm256_set1_epi16(restart_index);
			__m256i min = _mm256_set1_epi16(-1);
			__m256i max = _mm256_set1_epi16(0);

			for (unsigned n = 0; n < iterations; ++n)
			{
				const __m256i raw = _mm256_loadu_si256(src_stream++);
				const __m256i value = _mm256_shuffle_epi8(raw, shuffle_mask);
				const __m256i mask = _mm256_cmpeq_epi16(restart, value);
				const __m256i value_with_min_restart = _mm256_andnot_si256(mask, value);
				const __m256i value_with_max_restart = _mm256_or_si256(mask, value);
				max = _mm256_max_epu16(max, value_with_min_restart);
				min = _mm256_min_epu16(min, value_with_max_restart);
				_mm256_store_si256(dst_stream++, value_with_max_restart);
			}

			__m128i tmp = _mm256_extracti128_si256(min, 1);
			__m128i min2 = _mm256_castsi256_si128(min);
			min2 = _mm_min_epu16(min2, tmp);

			tmp = _mm256_extracti128_si256(max, 1);
			__m128i max2 = _mm256_castsi256_si128(max);
			max2 = _mm_max_epu16(max2, tmp);

			const u16 min_index = sse41_hmin_epu16(min2);
			const u16 max_index = sse41_hmax_epu16(max2);

			return std::make_tuple(min_index, max_index);
		}
#endif

		SSE4_1_FUNC
		static
		std::tuple<u16, u16> upload_u16_swapped_sse4_1(const void *src, void *dst, u32 iterations, u16 restart_index)
		{
			auto src_stream = static_cast<const __m128i*>(src);
			auto dst_stream = static_cast<__m128i*>(dst);

			__m128i restart = _mm_set1_epi16(restart_index);
			__m128i min = _mm_set1_epi16(-1);
			__m128i max = _mm_set1_epi16(0);

			for (unsigned n = 0; n < iterations; ++n)
			{
				const __m128i raw = _mm_loadu_si128(src_stream++);
				const __m128i value = _mm_shuffle_epi8(raw, s_bswap_u16_mask);
				const __m128i mask = _mm_cmpeq_epi16(restart, value);
				const __m128i value_with_min_restart = _mm_andnot_si128(mask, value);
				const __m128i value_with_max_restart = _mm_or_si128(mask, value);
				max = _mm_max_epu16(max, value_with_min_restart);
				min = _mm_min_epu16(min, value_with_max_restart);
				_mm_store_si128(dst_stream++, value_with_max_restart);
			}

			const u16 min_index = sse41_hmin_epu16(min);
			const u16 max_index = sse41_hmax_epu16(max);

			return std::make_tuple(min_index, max_index);
		}

		SSE4_1_FUNC
		static
		std::tuple<u32, u32> upload_u32_swapped_sse4_1(const void *src, void *dst, u32 iterations, u32 restart_index)
		{
			auto src_stream = static_cast<const __m128i*>(src);
			auto dst_stream = static_cast<__m128i*>(dst);

			__m128i restart = _mm_set1_epi32(restart_index);
			__m128i min = _mm_set1_epi32(0xffffffff);
			__m128i max = _mm_set1_epi32(0);

			for (unsigned n = 0; n < iterations; ++n)
			{
				const __m128i raw = _mm_loadu_si128(src_stream++);
				const __m128i value = _mm_shuffle_epi8(raw, s_bswap_u32_mask);
				const __m128i mask = _mm_cmpeq_epi32(restart, value);
				const __m128i value_with_min_restart = _mm_andnot_si128(mask, value);
				const __m128i value_with_max_restart = _mm_or_si128(mask, value);
				max = _mm_max_epu32(max, value_with_min_restart);
				min = _mm_min_epu32(min, value_with_max_restart);
				_mm_store_si128(dst_stream++, value_with_max_restart);
			}

			__m128i tmp = _mm_srli_si128(min, 8);
			min = _mm_min_epu32(min, tmp);
			tmp = _mm_srli_si128(min, 4);
			min = _mm_min_epu32(min, tmp);

			tmp = _mm_srli_si128(max, 8);
			max = _mm_max_epu32(max, tmp);
			tmp = _mm_srli_si128(max, 4);
			max = _mm_max_epu32(max, tmp);

			const u32 min_index = _mm_cvtsi128_si32(min);
			const u32 max_index = _mm_cvtsi128_si32(max);

			return std::make_tuple(min_index, max_index);
		}

		template<typename T>
		static
		std::tuple<T, T, u32> upload_untouched(std::span<to_be_t<const T>> src, std::span<T> dst, T restart_index, bool skip_restart)
		{
			T min_index = index_limit<T>();
			T max_index = 0;
			u32 written = 0;
			u32 length = ::size32(src);

			if (length >= 32 && !skip_restart)
			{
				if constexpr (std::is_same<T, u16>::value)
				{
					if (s_use_avx3)
					{
#if defined(ARCH_X64)
						// Handle remainder in function
						written = length;
						std::tie(min_index, max_index) = upload_u16_swapped_avx3(src.data(), dst.data(), length, restart_index);
						return std::make_tuple(min_index, max_index, written);
					}
					else if (s_use_avx2)
					{
						u32 iterations = length >> 4;
						written = length & ~0xF;
						std::tie(min_index, max_index) = upload_u16_swapped_avx2(src.data(), dst.data(), iterations, restart_index);
#endif
					}
					else if (s_use_sse4_1)
					{
						u32 iterations = length >> 3;
						written = length & ~0x7;
						std::tie(min_index, max_index) = upload_u16_swapped_sse4_1(src.data(), dst.data(), iterations, restart_index);
					}
				}
				else if constexpr (std::is_same<T, u32>::value)
				{
					if (s_use_sse4_1)
					{
						u32 iterations = length >> 2;
						written = length & ~0x3;
						std::tie(min_index, max_index) = upload_u32_swapped_sse4_1(src.data(), dst.data(), iterations, restart_index);
					}
				}
				else
				{
					fmt::throw_exception("Unreachable");
				}
			}

			for (u32 i = written; i < length; ++i)
			{
				T index = src[i];
				if (index == restart_index)
				{
					if (!skip_restart)
					{
						dst[written++] = index_limit<T>();
					}
				}
				else
				{
					dst[written++] = min_max(min_index, max_index, index);
				}
			}

			return std::make_tuple(min_index, max_index, written);
		}
	};

	template<typename T>
	std::tuple<T, T, u32> upload_untouched(std::span<to_be_t<const T>> src, std::span<T> dst, rsx::primitive_type draw_mode, bool is_primitive_restart_enabled, u32 primitive_restart_index)
	{
		if (!is_primitive_restart_enabled)
		{
			return untouched_impl::upload_untouched(src, dst);
		}
		else if constexpr (std::is_same<T, u16>::value)
		{
			if (primitive_restart_index > 0xffff)
			{
				return untouched_impl::upload_untouched(src, dst);
			}
			else
			{
				return primitive_restart_impl::upload_untouched(src, dst, static_cast<u16>(primitive_restart_index), is_primitive_disjointed(draw_mode));
			}
		}
		else
		{
			return primitive_restart_impl::upload_untouched(src, dst, primitive_restart_index, is_primitive_disjointed(draw_mode));
		}
	}

	template<typename T>
	std::tuple<T, T, u32> expand_indexed_triangle_fan(std::span<to_be_t<const T>> src, std::span<T> dst, bool is_primitive_restart_enabled, u32 primitive_restart_index)
	{
		const T invalid_index = index_limit<T>();

		T min_index = invalid_index;
		T max_index = 0;

		ensure((dst.size() >= 3 * (src.size() - 2)));

		u32 dst_idx = 0;

		bool needs_anchor = true;
		T anchor = invalid_index;
		T last_index = invalid_index;

		for (const T index : src)
		{
			if (needs_anchor)
			{
				if (is_primitive_restart_enabled && index == primitive_restart_index)
					continue;

				anchor = min_max(min_index, max_index, index);
				needs_anchor = false;
				continue;
			}

			if (is_primitive_restart_enabled && index == primitive_restart_index)
			{
				needs_anchor = true;
				last_index = invalid_index;
				continue;
			}

			if (last_index == invalid_index)
			{
				//Need at least one anchor and one outer index to create a triangle
				last_index = min_max(min_index, max_index, index);
				continue;
			}

			dst[dst_idx++] = anchor;
			dst[dst_idx++] = last_index;
			dst[dst_idx++] = min_max(min_index, max_index, index);

			last_index = index;
		}

		return std::make_tuple(min_index, max_index, dst_idx);
	}

	template<typename T>
	std::tuple<T, T, u32> expand_indexed_quads(std::span<to_be_t<const T>> src, std::span<T> dst, bool is_primitive_restart_enabled, u32 primitive_restart_index)
	{
		T min_index = index_limit<T>();
		T max_index = 0;

		ensure((4 * dst.size_bytes() >= 6 * src.size_bytes()));

		u32 dst_idx = 0;
		u8 set_size = 0;
		T tmp_indices[4];

		for (const T index : src)
		{
			if (is_primitive_restart_enabled && index == primitive_restart_index)
			{
				//empty temp buffer
				set_size = 0;
				continue;
			}

			tmp_indices[set_size++] = min_max(min_index, max_index, index);

			if (set_size == 4)
			{
				// First triangle
				dst[dst_idx++] = tmp_indices[0];
				dst[dst_idx++] = tmp_indices[1];
				dst[dst_idx++] = tmp_indices[2];
				// Second triangle
				dst[dst_idx++] = tmp_indices[2];
				dst[dst_idx++] = tmp_indices[3];
				dst[dst_idx++] = tmp_indices[0];

				set_size = 0;
			}
		}

		return std::make_tuple(min_index, max_index, dst_idx);
	}
}

// Only handle quads and triangle fan now
bool is_primitive_native(rsx::primitive_type draw_mode)
{
	switch (draw_mode)
	{
	case rsx::primitive_type::points:
	case rsx::primitive_type::lines:
	case rsx::primitive_type::line_strip:
	case rsx::primitive_type::triangles:
	case rsx::primitive_type::triangle_strip:
	case rsx::primitive_type::quad_strip:
		return true;
	case rsx::primitive_type::line_loop:
	case rsx::primitive_type::polygon:
	case rsx::primitive_type::triangle_fan:
	case rsx::primitive_type::quads:
		return false;
	case rsx::primitive_type::invalid:
		break;
	}

	fmt::throw_exception("Wrong primitive type");
}

bool is_primitive_disjointed(rsx::primitive_type draw_mode)
{
	switch (draw_mode)
	{
	case rsx::primitive_type::line_loop:
	case rsx::primitive_type::line_strip:
	case rsx::primitive_type::polygon:
	case rsx::primitive_type::quad_strip:
	case rsx::primitive_type::triangle_fan:
	case rsx::primitive_type::triangle_strip:
		return false;
	default:
		return true;
	}
}

u32 get_index_count(rsx::primitive_type draw_mode, u32 initial_index_count)
{
	// Index count
	if (is_primitive_native(draw_mode))
		return initial_index_count;

	switch (draw_mode)
	{
	case rsx::primitive_type::line_loop:
		return initial_index_count + 1;
	case rsx::primitive_type::polygon:
	case rsx::primitive_type::triangle_fan:
		return (initial_index_count - 2) * 3;
	case rsx::primitive_type::quads:
		return (6 * initial_index_count) / 4;
	default:
		return 0;
	}
}

u32 get_index_type_size(rsx::index_array_type type)
{
	switch (type)
	{
	case rsx::index_array_type::u16: return sizeof(u16);
	case rsx::index_array_type::u32: return sizeof(u32);
	}
	fmt::throw_exception("Wrong index type");
}

void write_index_array_for_non_indexed_non_native_primitive_to_buffer(char* dst, rsx::primitive_type draw_mode, unsigned count)
{
	auto typedDst = reinterpret_cast<u16*>(dst);
	switch (draw_mode)
	{
	case rsx::primitive_type::line_loop:
		for (unsigned i = 0; i < count; ++i)
			typedDst[i] = i;
		typedDst[count] = 0;
		return;
	case rsx::primitive_type::triangle_fan:
	case rsx::primitive_type::polygon:
		for (unsigned i = 0; i < (count - 2); i++)
		{
			typedDst[3 * i] = 0;
			typedDst[3 * i + 1] = i + 2 - 1;
			typedDst[3 * i + 2] = i + 2;
		}
		return;
	case rsx::primitive_type::quads:
		for (unsigned i = 0; i < count / 4; i++)
		{
			// First triangle
			typedDst[6 * i] = 4 * i;
			typedDst[6 * i + 1] = 4 * i + 1;
			typedDst[6 * i + 2] = 4 * i + 2;
			// Second triangle
			typedDst[6 * i + 3] = 4 * i + 2;
			typedDst[6 * i + 4] = 4 * i + 3;
			typedDst[6 * i + 5] = 4 * i;
		}
		return;
	case rsx::primitive_type::quad_strip:
	case rsx::primitive_type::points:
	case rsx::primitive_type::lines:
	case rsx::primitive_type::line_strip:
	case rsx::primitive_type::triangles:
	case rsx::primitive_type::triangle_strip:
		fmt::throw_exception("Native primitive type doesn't require expansion");
	case rsx::primitive_type::invalid:
		break;
	}

	fmt::throw_exception("Tried to load invalid primitive type");
}


namespace
{
	template<typename T>
	std::tuple<T, T, u32> write_index_array_data_to_buffer_impl(std::span<T> dst,
		std::span<const be_t<T>> src,
		rsx::primitive_type draw_mode, bool restart_index_enabled, u32 restart_index,
		const std::function<bool(rsx::primitive_type)>& expands)
	{
		if (!expands(draw_mode)) [[likely]]
		{
			return upload_untouched<T>(src, dst, draw_mode, restart_index_enabled, restart_index);
		}

		switch (draw_mode)
		{
		case rsx::primitive_type::line_loop:
		{
			const auto &returnvalue = upload_untouched<T>(src, dst, draw_mode, restart_index_enabled, restart_index);
			const auto index_count = dst.size_bytes() / sizeof(T);
			dst[index_count] = src[0];
			return returnvalue;
		}
		case rsx::primitive_type::polygon:
		case rsx::primitive_type::triangle_fan:
		{
			return expand_indexed_triangle_fan<T>(src, dst, restart_index_enabled, restart_index);
		}
		case rsx::primitive_type::quads:
		{
			return expand_indexed_quads<T>(src, dst, restart_index_enabled, restart_index);
		}
		default:
			fmt::throw_exception("Unknown draw mode (0x%x)", static_cast<u8>(draw_mode));
		}
	}
}

std::tuple<u32, u32, u32> write_index_array_data_to_buffer(std::span<std::byte> dst_ptr,
	std::span<const std::byte> src_ptr,
	rsx::index_array_type type, rsx::primitive_type draw_mode, bool restart_index_enabled, u32 restart_index,
	const std::function<bool(rsx::primitive_type)>& expands)
{
	switch (type)
	{
	case rsx::index_array_type::u16:
	{
		return write_index_array_data_to_buffer_impl<u16>(utils::bless<u16>(dst_ptr), utils::bless<const be_t<u16>>(src_ptr),
			draw_mode, restart_index_enabled, restart_index, expands);
	}
	case rsx::index_array_type::u32:
	{
		return write_index_array_data_to_buffer_impl<u32>(utils::bless<u32>(dst_ptr), utils::bless<const be_t<u32>>(src_ptr),
			draw_mode, restart_index_enabled, restart_index, expands);
	}
	default:
		fmt::throw_exception("Unreachable");
	}
}
