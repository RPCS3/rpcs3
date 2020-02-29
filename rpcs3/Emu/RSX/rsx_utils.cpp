#include "stdafx.h"
#include "rsx_utils.h"
#include "rsx_methods.h"
#include "Emu/RSX/GCM.h"
#include "Common/BufferUtils.h"
#include "Overlays/overlays.h"
#include "Utilities/sysinfo.h"

#ifdef _MSC_VER
#pragma warning(push, 0)
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wextra"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif
extern "C"
{
#include "libswscale/swscale.h"
}
#ifdef _MSC_VER
#pragma warning(pop)
#else
#pragma GCC diagnostic pop
#endif

namespace rsx
{
	atomic_t<u64> g_rsx_shared_tag{ 0 };

	void convert_scale_image(u8 *dst, AVPixelFormat dst_format, int dst_width, int dst_height, int dst_pitch,
		const u8 *src, AVPixelFormat src_format, int src_width, int src_height, int src_pitch, int src_slice_h, bool bilinear)
	{
		std::unique_ptr<SwsContext, void(*)(SwsContext*)> sws(sws_getContext(src_width, src_height, src_format,
			dst_width, dst_height, dst_format, bilinear ? SWS_FAST_BILINEAR : SWS_POINT, NULL, NULL, NULL), sws_freeContext);

		sws_scale(sws.get(), &src, &src_pitch, 0, src_slice_h, &dst, &dst_pitch);
	}

	void clip_image(u8 *dst, const u8 *src, int clip_x, int clip_y, int clip_w, int clip_h, int bpp, int src_pitch, int dst_pitch)
	{
		const u8* pixels_src = src + clip_y * src_pitch + clip_x * bpp;
		u8 *pixels_dst = dst;
		const u32 row_length = clip_w * bpp;

		for (int y = 0; y < clip_h; ++y)
		{
			std::memcpy(pixels_dst, pixels_src, row_length);
			pixels_src += src_pitch;
			pixels_dst += dst_pitch;
		}
	}

	void clip_image_may_overlap(u8 *dst, const u8 *src, int clip_x, int clip_y, int clip_w, int clip_h, int bpp, int src_pitch, int dst_pitch, u8 *buffer)
	{
		src += clip_y * src_pitch + clip_x * bpp;

		const u32 buffer_pitch = bpp * clip_w;
		u8* buf = buffer;

		// Read the whole buffer from source
		for (int y = 0; y < clip_h; ++y)
		{
			std::memcpy(buf, src, buffer_pitch);
			src += src_pitch;
			buf += buffer_pitch;
		}

		buf = buffer;

		// Write to destination
		for (int y = 0; y < clip_h; ++y)
		{
			std::memcpy(dst, buf, buffer_pitch);
			dst += dst_pitch;
			buf += buffer_pitch;
		}
	}

	//Convert decoded integer values for CONSTANT_BLEND_FACTOR into f32 array in 0-1 range
	std::array<float, 4> get_constant_blend_colors()
	{
		//TODO: check another color formats (probably all integer formats with > 8-bits wide channels)
		if (rsx::method_registers.surface_color() == rsx::surface_color_format::w16z16y16x16)
		{
			u16 blend_color_r = rsx::method_registers.blend_color_16b_r();
			u16 blend_color_g = rsx::method_registers.blend_color_16b_g();
			u16 blend_color_b = rsx::method_registers.blend_color_16b_b();
			u16 blend_color_a = rsx::method_registers.blend_color_16b_a();

			return { blend_color_r / 65535.f, blend_color_g / 65535.f, blend_color_b / 65535.f, blend_color_a / 65535.f };
		}
		else
		{
			u8 blend_color_r = rsx::method_registers.blend_color_8b_r();
			u8 blend_color_g = rsx::method_registers.blend_color_8b_g();
			u8 blend_color_b = rsx::method_registers.blend_color_8b_b();
			u8 blend_color_a = rsx::method_registers.blend_color_8b_a();

			return { blend_color_r / 255.f, blend_color_g / 255.f, blend_color_b / 255.f, blend_color_a / 255.f };
		}
	}

	/* Fast image scaling routines
	* Only uses fast nearest scaling and integral scaling factors
	* T - Dst type
	* U - Src type
	* N - Sample count
	*/
	template <typename T, typename U>
	void scale_image_fallback_impl(T* dst, const U* src, u16 src_width, u16 src_height, u16 dst_pitch, u16 src_pitch, u8 element_size, u8 samples_u, u8 samples_v)
	{
		u32 dst_offset = 0;
		u32 src_offset = 0;

		u32 padding = (dst_pitch - (src_pitch * samples_u)) / sizeof(T);

		for (u16 h = 0; h < src_height; ++h)
		{
			const auto row_start = dst_offset;
			for (u16 w = 0; w < src_width; ++w)
			{
				for (u8 n = 0; n < samples_u; ++n)
				{
					dst[dst_offset++] = src[src_offset];
				}

				src_offset++;
			}

			dst_offset += padding;

			for (int n = 1; n < samples_v; ++n)
			{
				memcpy(&dst[dst_offset], &dst[row_start], dst_pitch);
				dst_offset += dst_pitch;
			}
		}
	}

	void scale_image_fallback(void* dst, const void* src, u16 src_width, u16 src_height, u16 dst_pitch, u16 src_pitch, u8 element_size, u8 samples_u, u8 samples_v)
	{
		switch (element_size)
		{
		case 1:
			scale_image_fallback_impl<u8, u8>(static_cast<u8*>(dst), static_cast<const u8*>(src), src_width, src_height, dst_pitch, src_pitch, element_size, samples_u, samples_v);
			break;
		case 2:
			scale_image_fallback_impl<u16, u16>(static_cast<u16*>(dst), static_cast<const u16*>(src), src_width, src_height, dst_pitch, src_pitch, element_size, samples_u, samples_v);
			break;
		case 4:
			scale_image_fallback_impl<u32, u32>(static_cast<u32*>(dst), static_cast<const u32*>(src), src_width, src_height, dst_pitch, src_pitch, element_size, samples_u, samples_v);
			break;
		default:
			fmt::throw_exception("unsupported element size %d" HERE, element_size);
		}
	}

	void scale_image_fallback_with_byte_swap(void* dst, const void* src, u16 src_width, u16 src_height, u16 dst_pitch, u16 src_pitch, u8 element_size, u8 samples_u, u8 samples_v)
	{
		switch (element_size)
		{
		case 1:
			scale_image_fallback_impl<u8, u8>(static_cast<u8*>(dst), static_cast<const u8*>(src), src_width, src_height, dst_pitch, src_pitch, element_size, samples_u, samples_v);
			break;
		case 2:
			scale_image_fallback_impl<u16, be_t<u16>>(static_cast<u16*>(dst), static_cast<const be_t<u16>*>(src), src_width, src_height, dst_pitch, src_pitch, element_size, samples_u, samples_v);
			break;
		case 4:
			scale_image_fallback_impl<u32, be_t<u32>>(static_cast<u32*>(dst), static_cast<const be_t<u32>*>(src), src_width, src_height, dst_pitch, src_pitch, element_size, samples_u, samples_v);
			break;
		default:
			fmt::throw_exception("unsupported element size %d" HERE, element_size);
		}
	}

	template <typename T, typename U, int N>
	void scale_image_impl(T* dst, const U* src, u16 src_width, u16 src_height, u16 padding)
	{
		u32 dst_offset = 0;
		u32 src_offset = 0;

		for (u16 h = 0; h < src_height; ++h)
		{
			for (u16 w = 0; w < src_width; ++w)
			{
				for (u8 n = 0; n < N; ++n)
				{
					dst[dst_offset++] = src[src_offset];
				}

				//Fetch next pixel
				src_offset++;
			}

			//Pad this row
			dst_offset += padding;
		}
	}

	template <int N>
	void scale_image_fast(void *dst, const void *src, u8 element_size, u16 src_width, u16 src_height, u16 padding)
	{
		switch (element_size)
		{
		case 1:
			scale_image_impl<u8, u8, N>(static_cast<u8*>(dst), static_cast<const u8*>(src), src_width, src_height, padding);
			break;
		case 2:
			scale_image_impl<u16, u16, N>(static_cast<u16*>(dst), static_cast<const u16*>(src), src_width, src_height, padding);
			break;
		case 4:
			scale_image_impl<u32, u32, N>(static_cast<u32*>(dst), static_cast<const u32*>(src), src_width, src_height, padding);
			break;
		case 8:
			scale_image_impl<u64, u64, N>(static_cast<u64*>(dst), static_cast<const u64*>(src), src_width, src_height, padding);
			break;
		default:
			fmt::throw_exception("unsupported pixel size %d" HERE, element_size);
		}
	}

	template <int N>
	void scale_image_fast_with_byte_swap(void *dst, const void *src, u8 element_size, u16 src_width, u16 src_height, u16 padding)
	{
		switch (element_size)
		{
		case 1:
			scale_image_impl<u8, u8, N>(static_cast<u8*>(dst), static_cast<const u8*>(src), src_width, src_height, padding);
			break;
		case 2:
			scale_image_impl<u16, be_t<u16>, N>(static_cast<u16*>(dst), static_cast<const be_t<u16>*>(src), src_width, src_height, padding);
			break;
		case 4:
			scale_image_impl<u32, be_t<u32>, N>(static_cast<u32*>(dst), static_cast<const be_t<u32>*>(src), src_width, src_height, padding);
			break;
		case 8:
			scale_image_impl<u64, be_t<u64>, N>(static_cast<u64*>(dst), static_cast<const be_t<u64>*>(src), src_width, src_height, padding);
			break;
		default:
			fmt::throw_exception("unsupported pixel size %d" HERE, element_size);
		}
	}

	void scale_image_nearest(void* dst, const void* src, u16 src_width, u16 src_height, u16 dst_pitch, u16 src_pitch, u8 element_size, u8 samples_u, u8 samples_v, bool swap_bytes)
	{
		//Scale this image by repeating pixel data n times
		//n = expected_pitch / real_pitch
		//Use of fixed argument templates for performance reasons

		const u16 dst_width = dst_pitch / element_size;
		const u16 padding = dst_width - (src_width * samples_u);

		if (!swap_bytes)
		{
			if (samples_v == 1)
			{
				switch (samples_u)
				{
				case 1:
					scale_image_fast<1>(dst, src, element_size, src_width, src_height, padding);
					break;
				case 2:
					scale_image_fast<2>(dst, src, element_size, src_width, src_height, padding);
					break;
				case 3:
					scale_image_fast<3>(dst, src, element_size, src_width, src_height, padding);
					break;
				case 4:
					scale_image_fast<4>(dst, src, element_size, src_width, src_height, padding);
					break;
				case 8:
					scale_image_fast<8>(dst, src, element_size, src_width, src_height, padding);
					break;
				case 16:
					scale_image_fast<16>(dst, src, element_size, src_width, src_height, padding);
					break;
				default:
					scale_image_fallback(dst, src, src_width, src_height, dst_pitch, src_pitch, element_size, samples_u, 1);
				}
			}
			else
			{
				scale_image_fallback(dst, src, src_width, src_height, dst_pitch, src_pitch, element_size, samples_u, samples_v);
			}
		}
		else
		{
			if (samples_v == 1)
			{
				switch (samples_u)
				{
				case 1:
					scale_image_fast_with_byte_swap<1>(dst, src, element_size, src_width, src_height, padding);
					break;
				case 2:
					scale_image_fast_with_byte_swap<2>(dst, src, element_size, src_width, src_height, padding);
					break;
				case 3:
					scale_image_fast_with_byte_swap<3>(dst, src, element_size, src_width, src_height, padding);
					break;
				case 4:
					scale_image_fast_with_byte_swap<4>(dst, src, element_size, src_width, src_height, padding);
					break;
				case 8:
					scale_image_fast_with_byte_swap<8>(dst, src, element_size, src_width, src_height, padding);
					break;
				case 16:
					scale_image_fast_with_byte_swap<16>(dst, src, element_size, src_width, src_height, padding);
					break;
				default:
					scale_image_fallback_with_byte_swap(dst, src, src_width, src_height, dst_pitch, src_pitch, element_size, samples_u, 1);
				}
			}
			else
			{
				scale_image_fallback_with_byte_swap(dst, src, src_width, src_height, dst_pitch, src_pitch, element_size, samples_u, samples_v);
			}
		}
	}

	void convert_le_f32_to_be_d24(void *dst, void *src, u32 row_length_in_texels, u32 num_rows)
	{
		const u32 num_pixels = row_length_in_texels * num_rows;
		verify(HERE), (num_pixels & 3) == 0;

		const auto num_iterations = (num_pixels >> 2);

		__m128i* dst_ptr = static_cast<__m128i*>(dst);
		__m128i* src_ptr = static_cast<__m128i*>(src);

		const __m128 scale_vector = _mm_set1_ps(16777214.f);

#if defined (_MSC_VER) || defined (__SSSE3__)
		if (utils::has_ssse3()) [[likely]]
		{
			const __m128i swap_mask = _mm_set_epi8
			(
				0xF, 0xC, 0xD, 0xE,
				0xB, 0x8, 0x9, 0xA,
				0x7, 0x4, 0x5, 0x6,
				0x3, 0x0, 0x1, 0x2
			);

			for (u32 n = 0; n < num_iterations; ++n)
			{
				const __m128i src_vector = _mm_loadu_si128(src_ptr);
				const __m128i result = _mm_cvtps_epi32(_mm_mul_ps(_mm_castsi128_ps(src_vector), scale_vector));
				const __m128i shuffled_vector = _mm_shuffle_epi8(result, swap_mask);
				_mm_stream_si128(dst_ptr, shuffled_vector);
				++dst_ptr;
				++src_ptr;
			}

			return;
		}
#endif

		const __m128i mask1 = _mm_set1_epi32(0xFF00FF00);
		const __m128i mask2 = _mm_set1_epi32(0x00FF0000);
		const __m128i mask3 = _mm_set1_epi32(0x000000FF);

		for (u32 n = 0; n < num_iterations; ++n)
		{
			const __m128i src_vector = _mm_loadu_si128(src_ptr);
			const __m128i result = _mm_cvtps_epi32(_mm_mul_ps(_mm_castsi128_ps(src_vector), scale_vector));

			const __m128i v1 = _mm_and_si128(result, mask1);
			const __m128i v2 = _mm_and_si128(_mm_slli_epi32(result, 16), mask2);
			const __m128i v3 = _mm_and_si128(_mm_srli_epi32(result, 16), mask3);
			const __m128i shuffled_vector = _mm_or_si128(_mm_or_si128(v1, v2), v3);

			_mm_stream_si128(dst_ptr, shuffled_vector);
			++dst_ptr;
			++src_ptr;
		}
	}

	void convert_le_d24x8_to_be_d24x8(void *dst, void *src, u32 row_length_in_texels, u32 num_rows)
	{
		const u32 num_pixels = row_length_in_texels * num_rows;
		verify(HERE), (num_pixels & 3) == 0;

		const auto num_iterations = (num_pixels >> 2);

		__m128i* dst_ptr = static_cast<__m128i*>(dst);
		__m128i* src_ptr = static_cast<__m128i*>(src);

#if defined (_MSC_VER) || defined (__SSSE3__)
		if (utils::has_ssse3()) [[likely]]
		{
			const __m128i swap_mask = _mm_set_epi8
			(
				0xF, 0xC, 0xD, 0xE,
				0xB, 0x8, 0x9, 0xA,
				0x7, 0x4, 0x5, 0x6,
				0x3, 0x0, 0x1, 0x2
			);

			for (u32 n = 0; n < num_iterations; ++n)
			{
				const __m128i src_vector = _mm_loadu_si128(src_ptr);
				const __m128i shuffled_vector = _mm_shuffle_epi8(src_vector, swap_mask);
				_mm_stream_si128(dst_ptr, shuffled_vector);
				++dst_ptr;
				++src_ptr;
			}

			return;
		}
#endif

		const __m128i mask1 = _mm_set1_epi32(0xFF00FF00);
		const __m128i mask2 = _mm_set1_epi32(0x00FF0000);
		const __m128i mask3 = _mm_set1_epi32(0x000000FF);

		for (u32 n = 0; n < num_iterations; ++n)
		{
			const __m128i src_vector = _mm_loadu_si128(src_ptr);
			const __m128i v1 = _mm_and_si128(src_vector, mask1);
			const __m128i v2 = _mm_and_si128(_mm_slli_epi32(src_vector, 16), mask2);
			const __m128i v3 = _mm_and_si128(_mm_srli_epi32(src_vector, 16), mask3);
			const __m128i shuffled_vector = _mm_or_si128(_mm_or_si128(v1, v2), v3);

			_mm_stream_si128(dst_ptr, shuffled_vector);
			++dst_ptr;
			++src_ptr;
		}
	}

	void convert_le_d24x8_to_le_f32(void *dst, void *src, u32 row_length_in_texels, u32 num_rows)
	{
		const u32 num_pixels = row_length_in_texels * num_rows;
		verify(HERE), (num_pixels & 3) == 0;

		const auto num_iterations = (num_pixels >> 2);

		__m128i* dst_ptr = static_cast<__m128i*>(dst);
		__m128i* src_ptr = static_cast<__m128i*>(src);

		const __m128 scale_vector = _mm_set1_ps(1.f / 16777214.f);
		const __m128i mask = _mm_set1_epi32(0x00FFFFFF);
		for (u32 n = 0; n < num_iterations; ++n)
		{
			const __m128 src_vector = _mm_cvtepi32_ps(_mm_and_si128(mask, _mm_loadu_si128(src_ptr)));
			const __m128 normalized_vector = _mm_mul_ps(src_vector, scale_vector);
			_mm_stream_si128(dst_ptr, _mm_castps_si128(normalized_vector));
			++dst_ptr;
			++src_ptr;
		}
	}

#ifdef TEXTURE_CACHE_DEBUG
	tex_cache_checker_t tex_cache_checker = {};
#endif
}
