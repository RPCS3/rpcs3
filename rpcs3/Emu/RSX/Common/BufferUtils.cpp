#include "stdafx.h"
#include "BufferUtils.h"
#include "../rsx_methods.h"
#include "../RSXThread.h"

#include "util/to_endian.hpp"
#include "util/sysinfo.hpp"
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
#define SSSE3_FUNC
#define SSE4_1_FUNC
#define AVX2_FUNC
#define AVX3_FUNC
#else
#ifndef __clang__
#define PLAIN_FUNC __attribute__((optimize("no-tree-vectorize")))
#define SSSE3_FUNC __attribute__((__target__("ssse3"))) __attribute__((optimize("tree-vectorize")))
#else
#define PLAIN_FUNC
#define SSSE3_FUNC __attribute__((__target__("ssse3")))
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
constexpr bool s_use_ssse3 = true;
constexpr bool s_use_sse4_1 = true;
constexpr bool s_use_avx2 = true;
constexpr bool s_use_avx3 = true;
#elif defined(__AVX2__)
constexpr bool s_use_ssse3 = true;
constexpr bool s_use_sse4_1 = true;
constexpr bool s_use_avx2 = true;
constexpr bool s_use_avx3 = false;
#elif defined(__SSE4_1__)
constexpr bool s_use_ssse3 = true;
constexpr bool s_use_sse4_1 = true;
constexpr bool s_use_avx2 = false;
constexpr bool s_use_avx3 = false;
#elif defined(__SSSE3__)
constexpr bool s_use_ssse3 = true;
constexpr bool s_use_sse4_1 = false;
constexpr bool s_use_avx2 = false;
constexpr bool s_use_avx3 = false;
#elif defined(ARCH_X64)
const bool s_use_ssse3 = utils::has_ssse3();
const bool s_use_sse4_1 = utils::has_sse41();
const bool s_use_avx2 = utils::has_avx2();
const bool s_use_avx3 = utils::has_avx512();
#else
constexpr bool s_use_ssse3 = true; // Non x86
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

	template <bool Compare>
	SSSE3_FUNC auto copy_data_swap_u32_ssse3(u32* dst, const u32* src, u32 count)
	{
		u32 result = 0;

#ifdef __clang__
		#pragma clang loop vectorize(enable) interleave(disable) unroll(disable)
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
	template <bool Compare, int Size, typename RT>
	void build_copy_data_swap_u32_avx3(asmjit::x86::Assembler& c, std::array<asmjit::x86::Gp, 4>& args, const RT& rmask, const RT& rload, const RT& rtest)
	{
		using namespace asmjit;

		Label loop = c.newLabel();
		Label tail = c.newLabel();

		// Get start alignment offset
		c.mov(args[3].r32(), args[0].r32());
		c.and_(args[3].r32(), Size * 4 - 1);

		// Load and duplicate shuffle mask
		c.vbroadcasti32x4(rmask, x86::oword_ptr(uptr(&s_bswap_u32_mask)));
		if (Compare)
			c.vpxor(x86::xmm2, x86::xmm2, x86::xmm2);

		c.or_(x86::eax, -1);
		// Small data: skip to tail (ignore alignment)
		c.cmp(args[2].r32(), Size);
		c.jbe(tail);

		// Generate mask for first iteration, adjust args using alignment offset
		c.sub(args[1], args[3]);
		c.shr(args[3].r32(), 2);
		c.shlx(x86::eax, x86::eax, args[3].r32());
		c.kmovw(x86::k1, x86::eax);
		c.and_(args[0], -Size * 4);
		c.add(args[2].r32(), args[3].r32());

		c.k(x86::k1).z().vmovdqu32(rload, x86::Mem(args[1], 0, Size * 4u));
		c.vpshufb(rload, rload, rmask);
		if (Compare)
			c.k(x86::k1).z().vpxord(rtest, rload, x86::Mem(args[0], 0, Size * 4u));
		c.k(x86::k1).vmovdqa32(x86::Mem(args[0], 0, Size * 4u), rload);
		c.lea(args[0], x86::qword_ptr(args[0], Size * 4));
		c.lea(args[1], x86::qword_ptr(args[1], Size * 4));
		c.sub(args[2].r32(), Size);

		c.or_(x86::eax, -1);
		c.align(AlignMode::kCode, 16);

		c.bind(loop);
		c.cmp(args[2].r32(), Size);
		c.jbe(tail);
		c.vmovdqu32(rload, x86::Mem(args[1], 0, Size * 4u));
		c.vpshufb(rload, rload, rmask);
		if (Compare)
			c.vpternlogd(rtest, rload, x86::Mem(args[0], 0, Size * 4u), 0xf6); // orAxorBC
		c.vmovdqa32(x86::Mem(args[0], 0, Size * 4u), rload);
		c.lea(args[0], x86::qword_ptr(args[0], Size * 4));
		c.lea(args[1], x86::qword_ptr(args[1], Size * 4));
		c.sub(args[2].r32(), Size);
		c.jmp(loop);

		c.bind(tail);
		c.bzhi(x86::eax, x86::eax, args[2].r32());
		c.kmovw(x86::k1, x86::eax);
		c.k(x86::k1).z().vmovdqu32(rload, x86::Mem(args[1], 0, Size * 4u));
		c.vpshufb(rload, rload, rmask);
		if (Compare)
			c.k(x86::k1).vpternlogd(rtest, rload, x86::Mem(args[0], 0, Size * 4u), 0xf6);
		c.k(x86::k1).vmovdqu32(x86::Mem(args[0], 0, Size * 4u), rload);

		if (Compare)
		{
			if constexpr (Size != 16)
			{
				c.vptest(rtest, rtest);
			}
			else
			{
				c.vptestmd(x86::k1, rtest, rtest);
				c.ktestw(x86::k1, x86::k1);
			}

			c.setnz(x86::al);
		}

#ifndef __AVX__
		c.vzeroupper();
#endif
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
				build_copy_data_swap_u32_avx3<Compare, 16>(c, args, x86::zmm0, x86::zmm1, x86::zmm2);
				return;
			}

			build_copy_data_swap_u32_avx3<Compare, 8>(c, args, x86::ymm0, x86::ymm1, x86::ymm2);
			return;
		}

		if (utils::has_ssse3())
		{
			c.jmp(&copy_data_swap_u32_ssse3<Compare>);
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

built_function<void(*)(u32*, const u32*, u32)> copy_data_swap_u32("copy_data_swap_u32", &build_copy_data_swap_u32<false>);

built_function<bool(*)(u32*, const u32*, u32)> copy_data_swap_u32_cmp("copy_data_swap_u32_cmp", &build_copy_data_swap_u32<true>);

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
					const auto count = (remaining & ~0x7);
					std::tie(min_index, max_index, written) = upload_u16_swapped_sse4_1(src.data(), dst.data(), count);
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
					if (s_use_avx2)
					{
#if defined(ARCH_X64)
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

void stream_vector(void *dst, u32 x, u32 y, u32 z, u32 w)
{
	const __m128i vector = _mm_set_epi32(w, z, y, x);
	_mm_stream_si128(reinterpret_cast<__m128i*>(dst), vector);
}

void stream_vector(void *dst, f32 x, f32 y, f32 z, f32 w)
{
	stream_vector(dst, std::bit_cast<u32>(x), std::bit_cast<u32>(y), std::bit_cast<u32>(z), std::bit_cast<u32>(w));
}
void stream_vector_from_memory(void *dst, void *src)
{
	const __m128i vector = _mm_loadu_si128(reinterpret_cast<__m128i*>(src));
	_mm_stream_si128(reinterpret_cast<__m128i*>(dst), vector);
}
