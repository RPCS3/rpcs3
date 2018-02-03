#include "stdafx.h"
#include "rsx_utils.h"
#include "rsx_methods.h"
#include "Emu/RSX/GCM.h"
#include "Common/BufferUtils.h"
#include "overlays.h"
#include "Utilities/sysinfo.h"

extern "C"
{
#include "libswscale/swscale.h"
}

namespace rsx
{
	void convert_scale_image(u8 *dst, AVPixelFormat dst_format, int dst_width, int dst_height, int dst_pitch,
		const u8 *src, AVPixelFormat src_format, int src_width, int src_height, int src_pitch, int src_slice_h, bool bilinear)
	{
		std::unique_ptr<SwsContext, void(*)(SwsContext*)> sws(sws_getContext(src_width, src_height, src_format,
			dst_width, dst_height, dst_format, bilinear ? SWS_FAST_BILINEAR : SWS_POINT, NULL, NULL, NULL), sws_freeContext);

		sws_scale(sws.get(), &src, &src_pitch, 0, src_slice_h, &dst, &dst_pitch);
	}

	void convert_scale_image(std::unique_ptr<u8[]>& dst, AVPixelFormat dst_format, int dst_width, int dst_height, int dst_pitch,
		const u8 *src, AVPixelFormat src_format, int src_width, int src_height, int src_pitch, int src_slice_h, bool bilinear)
	{
		dst.reset(new u8[dst_pitch * dst_height]);
		convert_scale_image(dst.get(), dst_format, dst_width, dst_height, dst_pitch,
			src, src_format, src_width, src_height, src_pitch, src_slice_h, bilinear);
	}

	void clip_image(u8 *dst, const u8 *src, int clip_x, int clip_y, int clip_w, int clip_h, int bpp, int src_pitch, int dst_pitch)
	{
		u8 *pixels_src = (u8*)src + clip_y * src_pitch + clip_x * bpp;
		u8 *pixels_dst = dst;
		const u32 row_length = clip_w * bpp;

		for (int y = 0; y < clip_h; ++y)
		{
			std::memmove(pixels_dst, pixels_src, row_length);
			pixels_src += src_pitch;
			pixels_dst += dst_pitch;
		}
	}

	void clip_image(std::unique_ptr<u8[]>& dst, const u8 *src,
		int clip_x, int clip_y, int clip_w, int clip_h, int bpp, int src_pitch, int dst_pitch)
	{
		dst.reset(new u8[clip_h * dst_pitch]);
		clip_image(dst.get(), src, clip_x, clip_y, clip_w, clip_h, bpp, src_pitch, dst_pitch);
	}

	void fill_scale_offset_matrix(void *dest_, bool transpose,
		float offset_x, float offset_y, float offset_z,
		float scale_x, float scale_y, float scale_z)
	{
		char *dest = (char*)dest_;

		if (transpose)
		{
			stream_vector(dest + 4 * sizeof(f32) * 0, scale_x, 0, 0, 0);
			stream_vector(dest + 4 * sizeof(f32) * 1, 0, scale_y, 0, 0);
			stream_vector(dest + 4 * sizeof(f32) * 2, 0, 0, scale_z, 0);
			stream_vector(dest + 4 * sizeof(f32) * 3, offset_x, offset_y, offset_z, 1);
		}
		else
		{
			stream_vector(dest + 4 * sizeof(f32) * 0, scale_x, 0, 0, offset_x);
			stream_vector(dest + 4 * sizeof(f32) * 1, 0, scale_y, 0, offset_y);
			stream_vector(dest + 4 * sizeof(f32) * 2, 0, 0, scale_z, offset_z);
			stream_vector(dest + 4 * sizeof(f32) * 3, 0.f, 0.f, 0.f, 1.f);
		}
	}

	void fill_window_matrix(void *dest, bool transpose)
	{
		u16 height = method_registers.shader_window_height();
		window_origin origin = method_registers.shader_window_origin();
		window_pixel_center pixelCenter = method_registers.shader_window_pixel();

		f32 offset_x = f32(method_registers.shader_window_offset_x());
		f32 offset_y = f32(method_registers.shader_window_offset_y());
		f32 scale_y = 1.0;

		if (origin == window_origin::bottom)
		{
			offset_y = height - offset_y + 1;
			scale_y = -1.0f;
		}

		if (false && pixelCenter == window_pixel_center::half)
		{
			offset_x += 0.5f;
			offset_y += 0.5f;
		}

		fill_scale_offset_matrix(dest, transpose, offset_x, offset_y, 0.0f, 1.0f, scale_y, 1.0f);
	}

	void fill_viewport_matrix(void *buffer, bool transpose)
	{
		f32 offset_x = method_registers.viewport_offset_x();
		f32 offset_y = method_registers.viewport_offset_y();
		f32 offset_z = method_registers.viewport_offset_z();

		f32 scale_x = method_registers.viewport_scale_x();
		f32 scale_y = method_registers.viewport_scale_y();
		f32 scale_z = method_registers.viewport_scale_z();

		fill_scale_offset_matrix(buffer, transpose, offset_x, offset_y, offset_z, scale_x, scale_y, scale_z);
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
	void scale_image_fallback_impl(T* dst, const U* src, u16 src_width, u16 src_height, u16 dst_pitch, u16 src_pitch, u8 pixel_size, u8 samples_u, u8 samples_v)
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

	void scale_image_fallback(void* dst, const void* src, u16 src_width, u16 src_height, u16 dst_pitch, u16 src_pitch, u8 pixel_size, u8 samples_u, u8 samples_v)
	{
		switch (pixel_size)
		{
		case 1:
			scale_image_fallback_impl<u8, u8>((u8*)dst, (const u8*)src, src_width, src_height, dst_pitch, src_pitch, pixel_size, samples_u, samples_v);
			break;
		case 2:
			scale_image_fallback_impl<u16, u16>((u16*)dst, (const u16*)src, src_width, src_height, dst_pitch, src_pitch, pixel_size, samples_u, samples_v);
			break;
		case 4:
			scale_image_fallback_impl<u32, u32>((u32*)dst, (const u32*)src, src_width, src_height, dst_pitch, src_pitch, pixel_size, samples_u, samples_v);
			break;
		case 8:
			scale_image_fallback_impl<u64, u64>((u64*)dst, (const u64*)src, src_width, src_height, dst_pitch, src_pitch, pixel_size, samples_u, samples_v);
			break;
		case 16:
			scale_image_fallback_impl<u128, u128>((u128*)dst, (const u128*)src, src_width, src_height, dst_pitch, src_pitch, pixel_size, samples_u, samples_v);
			break;
		default:
			fmt::throw_exception("unsupported pixel size %d" HERE, pixel_size);
		}
	}

	void scale_image_fallback_with_byte_swap(void* dst, const void* src, u16 src_width, u16 src_height, u16 dst_pitch, u16 src_pitch, u8 pixel_size, u8 samples_u, u8 samples_v)
	{
		switch (pixel_size)
		{
		case 1:
			scale_image_fallback_impl<u8, u8>((u8*)dst, (const u8*)src, src_width, src_height, dst_pitch, src_pitch, pixel_size, samples_u, samples_v);
			break;
		case 2:
			scale_image_fallback_impl<u16, be_t<u16>>((u16*)dst, (const be_t<u16>*)src, src_width, src_height, dst_pitch, src_pitch, pixel_size, samples_u, samples_v);
			break;
		case 4:
			scale_image_fallback_impl<u32, be_t<u32>>((u32*)dst, (const be_t<u32>*)src, src_width, src_height, dst_pitch, src_pitch, pixel_size, samples_u, samples_v);
			break;
		case 8:
			scale_image_fallback_impl<u64, be_t<u64>>((u64*)dst, (const be_t<u64>*)src, src_width, src_height, dst_pitch, src_pitch, pixel_size, samples_u, samples_v);
			break;
		case 16:
			scale_image_fallback_impl<u128, be_t<u128>>((u128*)dst, (const be_t<u128>*)src, src_width, src_height, dst_pitch, src_pitch, pixel_size, samples_u, samples_v);
			break;
		default:
			fmt::throw_exception("unsupported pixel size %d" HERE, pixel_size);
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
	void scale_image_fast(void *dst, const void *src, u8 pixel_size, u16 src_width, u16 src_height, u16 padding)
	{
		switch (pixel_size)
		{
		case 1:
			scale_image_impl<u8, u8, N>((u8*)dst, (const u8*)src, src_width, src_height, padding);
			break;
		case 2:
			scale_image_impl<u16, u16, N>((u16*)dst, (const u16*)src, src_width, src_height, padding);
			break;
		case 4:
			scale_image_impl<u32, u32, N>((u32*)dst, (const u32*)src, src_width, src_height, padding);
			break;
		case 8:
			scale_image_impl<u64, u64, N>((u64*)dst, (const u64*)src, src_width, src_height, padding);
			break;
		default:
			fmt::throw_exception("unsupported pixel size %d" HERE, pixel_size);
		}
	}

	template <int N>
	void scale_image_fast_with_byte_swap(void *dst, const void *src, u8 pixel_size, u16 src_width, u16 src_height, u16 padding)
	{
		switch (pixel_size)
		{
		case 1:
			scale_image_impl<u8, u8, N>((u8*)dst, (const u8*)src, src_width, src_height, padding);
			break;
		case 2:
			scale_image_impl<u16, be_t<u16>, N>((u16*)dst, (const be_t<u16>*)src, src_width, src_height, padding);
			break;
		case 4:
			scale_image_impl<u32, be_t<u32>, N>((u32*)dst, (const be_t<u32>*)src, src_width, src_height, padding);
			break;
		case 8:
			scale_image_impl<u64, be_t<u64>, N>((u64*)dst, (const be_t<u64>*)src, src_width, src_height, padding);
			break;
		default:
			fmt::throw_exception("unsupported pixel size %d" HERE, pixel_size);
		}
	}

	void scale_image_nearest(void* dst, const void* src, u16 src_width, u16 src_height, u16 dst_pitch, u16 src_pitch, u8 pixel_size, u8 samples_u, u8 samples_v, bool swap_bytes)
	{
		//Scale this image by repeating pixel data n times
		//n = expected_pitch / real_pitch
		//Use of fixed argument templates for performance reasons

		const u16 dst_width = dst_pitch / pixel_size;
		const u16 padding = dst_width - (src_width * samples_u);

		if (!swap_bytes)
		{
			if (samples_v == 1)
			{
				switch (samples_u)
				{
				case 1:
					scale_image_fast<1>(dst, src, pixel_size, src_width, src_height, padding);
					break;
				case 2:
					scale_image_fast<2>(dst, src, pixel_size, src_width, src_height, padding);
					break;
				case 3:
					scale_image_fast<3>(dst, src, pixel_size, src_width, src_height, padding);
					break;
				case 4:
					scale_image_fast<4>(dst, src, pixel_size, src_width, src_height, padding);
					break;
				case 8:
					scale_image_fast<8>(dst, src, pixel_size, src_width, src_height, padding);
					break;
				case 16:
					scale_image_fast<16>(dst, src, pixel_size, src_width, src_height, padding);
					break;
				default:
					scale_image_fallback(dst, src, src_width, src_height, dst_pitch, src_pitch, pixel_size, samples_u, 1);
				}
			}
			else
			{
				scale_image_fallback(dst, src, src_width, src_height, dst_pitch, src_pitch, pixel_size, samples_u, samples_v);
			}
		}
		else
		{
			if (samples_v == 1)
			{
				switch (samples_u)
				{
				case 1:
					scale_image_fast_with_byte_swap<1>(dst, src, pixel_size, src_width, src_height, padding);
					break;
				case 2:
					scale_image_fast_with_byte_swap<2>(dst, src, pixel_size, src_width, src_height, padding);
					break;
				case 3:
					scale_image_fast_with_byte_swap<3>(dst, src, pixel_size, src_width, src_height, padding);
					break;
				case 4:
					scale_image_fast_with_byte_swap<4>(dst, src, pixel_size, src_width, src_height, padding);
					break;
				case 8:
					scale_image_fast_with_byte_swap<8>(dst, src, pixel_size, src_width, src_height, padding);
					break;
				case 16:
					scale_image_fast_with_byte_swap<16>(dst, src, pixel_size, src_width, src_height, padding);
					break;
				default:
					scale_image_fallback_with_byte_swap(dst, src, src_width, src_height, dst_pitch, src_pitch, pixel_size, samples_u, 1);
				}
			}
			else
			{
				scale_image_fallback_with_byte_swap(dst, src, src_width, src_height, dst_pitch, src_pitch, pixel_size, samples_u, samples_v);
			}
		}
	}

	void convert_le_f32_to_be_d24(void *dst, void *src, u32 row_length_in_texels, u32 num_rows)
	{
		const u32 num_pixels = row_length_in_texels * num_rows;
		verify(HERE), (num_pixels & 3) == 0;

		const auto num_iterations = (num_pixels >> 2);

		__m128i* dst_ptr = (__m128i*)dst;
		__m128i* src_ptr = (__m128i*)src;

		const __m128 scale_vector = _mm_set1_ps(16777214.f);

#if defined (_MSC_VER) || defined (__SSSE3__)
		if (LIKELY(utils::has_ssse3()))
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
				const __m128i result = _mm_cvtps_epi32(_mm_mul_ps((__m128&)src_vector, scale_vector));
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
			const __m128i result = _mm_cvtps_epi32(_mm_mul_ps((__m128&)src_vector, scale_vector));

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

		__m128i* dst_ptr = (__m128i*)dst;
		__m128i* src_ptr = (__m128i*)src;

#if defined (_MSC_VER) || defined (__SSSE3__)
		if (LIKELY(utils::has_ssse3()))
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
}
