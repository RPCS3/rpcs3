#include "stdafx.h"
#include "BufferUtils.h"
#include "util/to_endian.hpp"
#include "util/sysinfo.hpp"
#include "Utilities/JIT.h"
#include "util/v128.hpp"
#include "util/simd.hpp"

#if !defined(_MSC_VER)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif

#if defined(ARCH_ARM64)
#if !defined(_MSC_VER)
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif
#undef FORCE_INLINE
#include "Emu/CPU/sse2neon.h"
#endif

#if defined(_MSC_VER) || !defined(__SSE2__)
#define SSE4_1_FUNC
#define AVX2_FUNC
#define AVX3_FUNC
#else
#define SSE4_1_FUNC __attribute__((__target__("sse4.1")))
#define AVX2_FUNC __attribute__((__target__("avx2")))
#define AVX3_FUNC __attribute__((__target__("avx512f,avx512bw,avx512dq,avx512cd,avx512vl")))
#endif // _MSC_VER

#if defined(__AVX512F__) && defined(__AVX512VL__) && defined(__AVX512DQ__) && defined(__AVX512CD__) && defined(__AVX512BW__)
[[maybe_unused]] constexpr bool s_use_ssse3 = true;
[[maybe_unused]] constexpr bool s_use_sse4_1 = true;
[[maybe_unused]] constexpr bool s_use_avx2 = true;
[[maybe_unused]] constexpr bool s_use_avx3 = true;
#elif defined(__AVX2__)
[[maybe_unused]] constexpr bool s_use_ssse3 = true;
[[maybe_unused]] constexpr bool s_use_sse4_1 = true;
[[maybe_unused]] constexpr bool s_use_avx2 = true;
[[maybe_unused]] constexpr bool s_use_avx3 = false;
#elif defined(__SSE4_1__)
[[maybe_unused]] constexpr bool s_use_ssse3 = true;
[[maybe_unused]] constexpr bool s_use_sse4_1 = true;
[[maybe_unused]] constexpr bool s_use_avx2 = false;
[[maybe_unused]] constexpr bool s_use_avx3 = false;
#elif defined(__SSSE3__)
[[maybe_unused]] constexpr bool s_use_ssse3 = true;
[[maybe_unused]] constexpr bool s_use_sse4_1 = false;
[[maybe_unused]] constexpr bool s_use_avx2 = false;
[[maybe_unused]] constexpr bool s_use_avx3 = false;
#elif defined(ARCH_X64)
[[maybe_unused]] const bool s_use_ssse3 = utils::has_ssse3();
[[maybe_unused]] const bool s_use_sse4_1 = utils::has_sse41();
[[maybe_unused]] const bool s_use_avx2 = utils::has_avx2();
[[maybe_unused]] const bool s_use_avx3 = utils::has_avx512();
#else
[[maybe_unused]] constexpr bool s_use_ssse3 = true; // Non x86
[[maybe_unused]] constexpr bool s_use_sse4_1 = true; // Non x86
[[maybe_unused]] constexpr bool s_use_avx2 = false;
[[maybe_unused]] constexpr bool s_use_avx3 = false;
#endif

const v128 s_bswap_u32_mask = v128::from32(0x00010203, 0x04050607, 0x08090a0b, 0x0c0d0e0f);
const v128 s_bswap_u16_mask = v128::from32(0x02030001, 0x06070405, 0x0a0b0809, 0x0e0f0c0d);

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
	auto copy_data_swap_u32_naive(u32* dst, const u32* src, u32 count)
	{
		u32 result = 0;

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
	template <bool Compare>
	void build_copy_data_swap_u32(asmjit::simd_builder& c, native_args& args)
	{
		using namespace asmjit;

		// Load and broadcast shuffle mask
		if (utils::has_ssse3())
		{
			c.vec_set_const(c.v1, s_bswap_u32_mask);
		}

		// Clear v2 (bitwise inequality accumulator)
		if constexpr (Compare)
		{
			c.vec_set_all_zeros(c.v2);
		}

		c.build_loop(sizeof(u32), x86::eax, args[2].r32(), [&]
		{
			c.zero_if_not_masked().vec_load_unaligned(sizeof(u32), c.v0, c.ptr_scale_for_vec(sizeof(u32), args[1], x86::rax));

			if (utils::has_ssse3())
			{
				c.vec_shuffle_xi8(c.v0, c.v0, c.v1);
			}
			else
			{
				c.emit(x86::Inst::kIdMovdqa, c.v1, c.v0);
				c.emit(x86::Inst::kIdPsrlw, c.v0, 8);
				c.emit(x86::Inst::kIdPsllw, c.v1, 8);
				c.emit(x86::Inst::kIdPor, c.v0, c.v1);
				c.emit(x86::Inst::kIdPshuflw, c.v0, c.v0, 0b10110001);
				c.emit(x86::Inst::kIdPshufhw, c.v0, c.v0, 0b10110001);
			}

			if constexpr (Compare)
			{
				if (utils::has_avx512())
				{
					c.keep_if_not_masked().emit(x86::Inst::kIdVpternlogd, c.v2, c.v0, c.ptr_scale_for_vec(sizeof(u32), args[0], x86::rax), 0xf6); // orAxorBC
				}
				else
				{
					c.zero_if_not_masked().vec_load_unaligned(sizeof(u32), c.v3, c.ptr_scale_for_vec(sizeof(u32), args[0], x86::rax));
					c.vec_xor(sizeof(u32), c.v3, c.v3, c.v0);
					c.vec_or(sizeof(u32), c.v2, c.v2, c.v3);
				}
			}

			c.keep_if_not_masked().vec_store_unaligned(sizeof(u32), c.v0, c.ptr_scale_for_vec(sizeof(u32), args[0], x86::rax));
		}, [&]
		{
			if constexpr (Compare)
			{
				if (c.vsize == 16 && c.vmask == 0)
				{
					// Fix for AVX2 path
					c.vextracti128(x86::xmm0, x86::ymm2, 1);
					c.vpor(x86::xmm2, x86::xmm2, x86::xmm0);
				}
			}
		});

		if constexpr (Compare)
		{
			if (c.vsize == 32 && c.vmask == 0)
				c.vec_clobbering_test(16, x86::xmm2, x86::xmm2);
			else
				c.vec_clobbering_test(c.vsize, c.v2, c.v2);
			c.setnz(x86::al);
		}

		c.vec_cleanup_ret();
	}
#endif
}

#if defined(ARCH_X64)
DECLARE(copy_data_swap_u32) = build_function_asm<void(*)(u32*, const u32*, u32), asmjit::simd_builder>("copy_data_swap_u32", &build_copy_data_swap_u32<false>);
DECLARE(copy_data_swap_u32_cmp) = build_function_asm<bool(*)(u32*, const u32*, u32), asmjit::simd_builder>("copy_data_swap_u32_cmp", &build_copy_data_swap_u32<true>);
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
		template <typename T>
		static u64 upload_untouched_naive(const be_t<T>* src, T* dst, u32 count)
		{
			u32 written = 0;
			T max_index = 0;
			T min_index = -1;

			while (count--)
			{
				T index = src[written];
				dst[written++] = min_max(min_index, max_index, index);
			}

			return (u64{max_index} << 32) | u64{min_index};
		}

#if defined(ARCH_X64)
		template <typename T>
		static void build_upload_untouched(asmjit::simd_builder& c, native_args& args)
		{
			using namespace asmjit;

			if (!utils::has_sse41())
			{
				c.jmp(&upload_untouched_naive<T>);
				return;
			}

			c.vec_set_const(c.v1, sizeof(T) == 2 ? s_bswap_u16_mask : s_bswap_u32_mask);
			c.vec_set_all_ones(c.v2); // vec min
			c.vec_set_all_zeros(c.v3); // vec max

			c.build_loop(sizeof(T), x86::eax, args[2].r32(), [&]
			{
				c.zero_if_not_masked().vec_load_unaligned(sizeof(T), c.v0, c.ptr_scale_for_vec(sizeof(T), args[0], x86::rax));

				if (utils::has_ssse3())
				{
					c.vec_shuffle_xi8(c.v0, c.v0, c.v1);
				}
				else
				{
					c.emit(x86::Inst::kIdMovdqa, c.v1, c.v0);
					c.emit(x86::Inst::kIdPsrlw, c.v0, 8);
					c.emit(x86::Inst::kIdPsllw, c.v1, 8);
					c.emit(x86::Inst::kIdPor, c.v0, c.v1);

					if constexpr (sizeof(T) == 4)
					{
						c.emit(x86::Inst::kIdPshuflw, c.v0, c.v0, 0b10110001);
						c.emit(x86::Inst::kIdPshufhw, c.v0, c.v0, 0b10110001);
					}
				}

				c.keep_if_not_masked().vec_umax(sizeof(T), c.v3, c.v3, c.v0);
				c.keep_if_not_masked().vec_umin(sizeof(T), c.v2, c.v2, c.v0);
				c.keep_if_not_masked().vec_store_unaligned(sizeof(T), c.v0, c.ptr_scale_for_vec(sizeof(T), args[1], x86::rax));
			}, [&]
			{
				// Compress horizontally, protect high values
				c.vec_extract_high(sizeof(T), c.v0, c.v3);
				c.vec_umax(sizeof(T), c.v3, c.v3, c.v0);
				c.vec_extract_high(sizeof(T), c.v0, c.v2);
				c.vec_umin(sizeof(T), c.v2, c.v2, c.v0);
			});

			c.vec_extract_gpr(sizeof(T), x86::edx, c.v3);
			c.vec_extract_gpr(sizeof(T), x86::eax, c.v2);
			c.shl(x86::rdx, 32);
			c.or_(x86::rax, x86::rdx);
			c.vec_cleanup_ret();
		}

		static inline auto upload_xi16 = build_function_asm<u64(*)(const be_t<u16>*, u16*, u32), asmjit::simd_builder>("untouched_upload_xi16", &build_upload_untouched<u16>);
		static inline auto upload_xi32 = build_function_asm<u64(*)(const be_t<u32>*, u32*, u32), asmjit::simd_builder>("untouched_upload_xi32", &build_upload_untouched<u32>);
#endif

		template <typename T>
		static std::tuple<T, T, u32> upload_untouched(std::span<to_be_t<const T>> src, std::span<T> dst)
		{
			T min_index, max_index;
			u32 count = ::size32(src);
			u64 r;

#if defined(ARCH_X64)
			if constexpr (sizeof(T) == 2)
				r = upload_xi16(src.data(), dst.data(), count);
			else
				r = upload_xi32(src.data(), dst.data(), count);
#else
			r = upload_untouched_naive(src.data(), dst.data(), count);
#endif

			min_index = static_cast<T>(r);
			max_index = static_cast<T>(r >> 32);

			return std::make_tuple(min_index, max_index, count);
		}
	};

	struct primitive_restart_impl
	{
		template <typename T>
		static inline u64 upload_untouched_naive(const be_t<T>* src, T* dst, u32 count, T restart_index)
		{
			T min_index = index_limit<T>();
			T max_index = 0;

			for (u32 i = 0; i < count; ++i)
			{
				T index = src[i].value();
				dst[i] = index == restart_index ? index_limit<T>() : min_max(min_index, max_index, index);
			}

			return (u64{max_index} << 32) | u64{min_index};
		}

#ifdef ARCH_X64
		template <typename T>
		static void build_upload_untouched(asmjit::simd_builder& c, native_args& args)
		{
			using namespace asmjit;

			if (!utils::has_sse41())
			{
				c.jmp(&upload_untouched_naive<T>);
				return;
			}

			c.vec_set_const(c.v1, sizeof(T) == 2 ? s_bswap_u16_mask : s_bswap_u32_mask);
			c.vec_set_all_ones(c.v2); // vec min
			c.vec_set_all_zeros(c.v3); // vec max
			c.vec_broadcast_gpr(sizeof(T), c.v4, args[3].r32());

			c.build_loop(sizeof(T), x86::eax, args[2].r32(), [&]
			{
				c.zero_if_not_masked().vec_load_unaligned(sizeof(T), c.v0, c.ptr_scale_for_vec(sizeof(T), args[0], x86::rax));

				if (utils::has_ssse3())
				{
					c.vec_shuffle_xi8(c.v0, c.v0, c.v1);
				}
				else
				{
					c.emit(x86::Inst::kIdMovdqa, c.v1, c.v0);
					c.emit(x86::Inst::kIdPsrlw, c.v0, 8);
					c.emit(x86::Inst::kIdPsllw, c.v1, 8);
					c.emit(x86::Inst::kIdPor, c.v0, c.v1);

					if constexpr (sizeof(T) == 4)
					{
						c.emit(x86::Inst::kIdPshuflw, c.v0, c.v0, 0b10110001);
						c.emit(x86::Inst::kIdPshufhw, c.v0, c.v0, 0b10110001);
					}
				}

				c.vec_cmp_eq(sizeof(T), c.v5, c.v4, c.v0);
				c.vec_andn(sizeof(T), c.v5, c.v5, c.v0);
				c.keep_if_not_masked().vec_umax(sizeof(T), c.v3, c.v3, c.v5);
				c.vec_cmp_eq(sizeof(T), c.v5, c.v4, c.v0);
				c.vec_or(sizeof(T), c.v0, c.v0, c.v5);
				c.keep_if_not_masked().vec_umin(sizeof(T), c.v2, c.v2, c.v0);
				c.keep_if_not_masked().vec_store_unaligned(sizeof(T), c.v0, c.ptr_scale_for_vec(sizeof(T), args[1], x86::rax));
			}, [&]
			{
				// Compress horizontally, protect high values
				c.vec_extract_high(sizeof(T), c.v0, c.v3);
				c.vec_umax(sizeof(T), c.v3, c.v3, c.v0);
				c.vec_extract_high(sizeof(T), c.v0, c.v2);
				c.vec_umin(sizeof(T), c.v2, c.v2, c.v0);
			});

			c.vec_extract_gpr(sizeof(T), x86::edx, c.v3);
			c.vec_extract_gpr(sizeof(T), x86::eax, c.v2);
			c.shl(x86::rdx, 32);
			c.or_(x86::rax, x86::rdx);
			c.vec_cleanup_ret();
		}

		static inline auto upload_xi16 = build_function_asm<u64(*)(const be_t<u16>*, u16*, u32, u32), asmjit::simd_builder>("restart_untouched_upload_xi16", &build_upload_untouched<u16>);
		static inline auto upload_xi32 = build_function_asm<u64(*)(const be_t<u32>*, u32*, u32, u32), asmjit::simd_builder>("restart_untouched_upload_xi32", &build_upload_untouched<u32>);
#endif

		template <typename T>
		static inline std::tuple<T, T, u32> upload_untouched(std::span<to_be_t<const T>> src, std::span<T> dst, T restart_index)
		{
			T min_index, max_index;
			u32 count = ::size32(src);
			u64 r;

#if defined(ARCH_X64)
			if constexpr (sizeof(T) == 2)
				r = upload_xi16(src.data(), dst.data(), count, restart_index);
			else
				r = upload_xi32(src.data(), dst.data(), count, restart_index);
#else
			r = upload_untouched_naive(src.data(), dst.data(), count, restart_index);
#endif

			min_index = static_cast<T>(r);
			max_index = static_cast<T>(r >> 32);

			return std::make_tuple(min_index, max_index, count);
		}
	};

	template <typename T>
	NEVER_INLINE std::tuple<T, T, u32> upload_untouched_skip_restart(std::span<to_be_t<const T>> src, std::span<T> dst, T restart_index)
	{
		T min_index = index_limit<T>();
		T max_index = 0;
		u32 written = 0;
		u32 length = ::size32(src);

		for (u32 i = written; i < length; ++i)
		{
			T index = src[i];
			if (index != restart_index)
			{
				dst[written++] = min_max(min_index, max_index, index);
			}
		}

		return std::make_tuple(min_index, max_index, written);
	}

	template<typename T, typename U = remove_be_t<T>>
		requires std::is_same_v<U, u32> || std::is_same_v<U, u16>
	std::tuple<T, T, u32> upload_untouched(std::span<to_be_t<const T>> src, std::span<T> dst, rsx::primitive_type draw_mode, bool is_primitive_restart_enabled, u32 primitive_restart_index)
	{
		if constexpr (std::is_same_v<T, u16>)
		{
			if (primitive_restart_index > 0xffff)
			{
				// Will never trip index restart, unpload untouched
				is_primitive_restart_enabled = false;
			}
		}

		if (!is_primitive_restart_enabled)
		{
			return untouched_impl::upload_untouched(src, dst);
		}

		if (is_primitive_disjointed(draw_mode))
		{
			return upload_untouched_skip_restart(src, dst, static_cast<U>(primitive_restart_index));
		}

		return primitive_restart_impl::upload_untouched(src, dst, static_cast<U>(primitive_restart_index));
	}

	void iota16(u16* dst, u32 count)
	{
		unsigned i = 0;
#if defined(ARCH_X64) || defined(ARCH_ARM64)
		const unsigned step = 8;                          // We do 8 entries per step
		const __m128i vec_step = _mm_set1_epi16(8);     // Constant to increment the raw values
		__m128i values = _mm_set_epi16(7, 6, 5, 4, 3, 2, 1, 0);
		__m128i* vec_ptr = utils::bless<__m128i>(dst);

		for (; (i + step) <= count; i += step, vec_ptr++)
		{
			_mm_stream_si128(vec_ptr, values);
			_mm_add_epi16(values,  vec_step);
		}
#endif
		for (; i < count; ++i)
			dst[i] = i;
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
		iota16(typedDst, count);
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
