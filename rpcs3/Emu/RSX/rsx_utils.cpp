#include "stdafx.h"
#include "rsx_utils.h"
#include "rsx_methods.h"
#include "Emu/RSX/GCM.h"
#include "Common/BufferUtils.h"

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
}
